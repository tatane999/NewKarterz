#include "NKGameMode.h"
#include "NKTrack.h"
#include "NKKart.h"
#include "NewKarterz.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

static const TCHAR* GNames[] = { TEXT("Melannie"), TEXT("Rocco"), TEXT("Kouign"), TEXT("Noisette"), TEXT("Biscotte"), TEXT("Praline"), TEXT("Caramel"), TEXT("Galette"), TEXT("Crêpe"), TEXT("Sablé"), TEXT("Nougat"), TEXT("Chouquette") };

ANKGameMode::ANKGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr; HUDClass = ANKHUD::StaticClass();
}

void ANKGameMode::SpawnEnvironment()
{
	UWorld* W = GetWorld(); FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FString& D = Track->Deco;
	// soleil
	ADirectionalLight* Sun = W->SpawnActor<ADirectionalLight>(FVector(0,0,10000), D==TEXT("rock")?FRotator(-22,160,0):D==TEXT("snow")?FRotator(-28,145,0):FRotator(-42,150,0), P);
	if (Sun){ UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()); L->SetMobility(EComponentMobility::Movable); L->SetIntensity(D==TEXT("rock")?6.f:10.f); L->SetAtmosphereSunLight(true); L->SetDynamicShadowDistanceMovableLight(25000.f);
		L->SetLightColor(D==TEXT("rock")?FLinearColor(1.f,0.62f,0.45f):FLinearColor(1.f,0.96f,0.9f)); }
	// atmosphère, ciel, nuages, brouillard
	AActor* Sky = W->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, P);
	USceneComponent* SR = NewObject<USceneComponent>(Sky, TEXT("R")); Sky->SetRootComponent(SR); SR->RegisterComponent();
	USkyAtmosphereComponent* Atm = NewObject<USkyAtmosphereComponent>(Sky, TEXT("Atm")); Atm->SetupAttachment(SR); Atm->RegisterComponent();
	if (D==TEXT("rock")) { Atm->SetRayleighScattering(FLinearColor(0.3f,0.08f,0.04f)); Atm->SetMieScatteringScale(0.08f); }
	UVolumetricCloudComponent* Cl = NewObject<UVolumetricCloudComponent>(Sky, TEXT("Clouds")); Cl->SetupAttachment(SR); Cl->RegisterComponent();
	if (UMaterialInterface* CM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"))) Cl->SetMaterial(CM);
	AExponentialHeightFog* Fog = W->SpawnActor<AExponentialHeightFog>(FVector(0,0,-500), FRotator::ZeroRotator, P);
	if (Fog){ UExponentialHeightFogComponent* F = Fog->GetComponent(); F->SetFogDensity(D==TEXT("snow")?0.035f:D==TEXT("rock")?0.05f:0.012f); F->SetFogHeightFalloff(0.08f); F->SetVolumetricFog(true);
		F->SetVolumetricFogScatteringDistribution(0.6f); if (D==TEXT("rock")) F->SetFogInscatteringColor(FLinearColor(0.35f,0.08f,0.03f)); }
	ASkyLight* SL = W->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (SL){ USkyLightComponent* C = SL->GetLightComponent(); C->SetMobility(EComponentMobility::Movable); C->bRealTimeCapture = true; C->SetIntensity(1.6f); C->RecaptureSky(); }
	APostProcessVolume* PPV = W->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (PPV){ PPV->bUnbound = true; FPostProcessSettings& S = PPV->Settings;
		S.bOverride_BloomIntensity=true; S.BloomIntensity=0.55f;
		S.bOverride_AutoExposureMinBrightness=true; S.AutoExposureMinBrightness=0.6f; S.bOverride_AutoExposureMaxBrightness=true; S.AutoExposureMaxBrightness=4.f;
		S.bOverride_ColorSaturation=true; S.ColorSaturation=FVector4(1.12f,1.12f,1.12f,1.f);
		S.bOverride_VignetteIntensity=true; S.VignetteIntensity=0.3f;
		S.bOverride_DynamicGlobalIlluminationMethod=true; S.DynamicGlobalIlluminationMethod=EDynamicGlobalIlluminationMethod::Lumen;
		S.bOverride_ReflectionMethod=true; S.ReflectionMethod=EReflectionMethod::Lumen; }
}

void ANKGameMode::BeginPlay()
{
	Super::BeginPlay();
	TrackIndex = UGameplayStatics::GetIntOption(OptionsString, TEXT("track"), 0);
	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Track = GetWorld()->SpawnActor<ANKTrack>(ANKTrack::StaticClass(), FTransform::Identity, P);
	if (!Track || !Track->Build(TrackIndex)) { UE_LOG(LogNK, Error, TEXT("Echec de construction de la piste")); return; }
	SpawnEnvironment();
	const int32 PlayerSlot = NumKarts-1;
	TArray<FString> CN; { FString J; if (FFileHelper::LoadFileToString(J, *(FPaths::ProjectContentDir()/TEXT("NKData/chars.json")))) { TArray<TSharedPtr<FJsonValue>> A; TSharedRef<TJsonReader<>> R=TJsonReaderFactory<>::Create(J); if (FJsonSerializer::Deserialize(R,A)) for (auto& v:A) CN.Add(v->AsString()); } }
	if (CN.Num()==0) for (const TCHAR* n : GNames) CN.Add(n);
	int32 PlayerChar = CN.IndexOfByKey(TEXT("Melannie")); if (PlayerChar<0) PlayerChar=0;
	TArray<int32> Pool; for (int32 i=0;i<CN.Num();i++) if (i!=PlayerChar) Pool.Add(i);
	for (int32 i=Pool.Num()-1;i>0;i--) Pool.Swap(i, FMath::RandRange(0,i));
	for (int32 s=0; s<NumKarts; s++)
	{
		ANKKart* K = GetWorld()->SpawnActor<ANKKart>(ANKKart::StaticClass(), FTransform::Identity, P);
		const int32 Ci = s==PlayerSlot ? PlayerChar : Pool[s % Pool.Num()];
		K->Init(Track, s, s==PlayerSlot, Ci, CN[Ci]);
		Karts.Add(K); if (s==PlayerSlot) Player = K;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->Possess(Player); PC->SetViewTarget(Player); PC->bShowMouseCursor=false; PC->SetInputMode(FInputModeGameOnly()); }
}

void ANKGameMode::Ranks()
{
	TArray<ANKKart*> A; for (ANKKart* K : Karts) A.Add(K);
	A.Sort([](const ANKKart& a, const ANKKart& b){ if (a.bFinished!=b.bFinished) return a.bFinished; if (a.bFinished) return a.FinishTime<b.FinishTime; return a.Prog>b.Prog; });
	for (int32 i=0;i<A.Num();i++) A[i]->Rank=i;
}

void ANKGameMode::Restart(int32 NewTrack)
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName().Replace(*GetWorld()->StreamingLevelsPrefix, TEXT(""))), true, FString::Printf(TEXT("track=%d"), NewTrack));
}

void ANKGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Track || !Player) return;
	Dt = FMath::Min(Dt, 1.f/30.f);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		const bool bN = PC->IsInputKeyDown(EKeys::N) || (PC->IsInputKeyDown(EKeys::Gamepad_Special_Right) && State==ENKState::Finished);
		const bool bR = PC->IsInputKeyDown(EKeys::Enter) || (PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom) && State==ENKState::Finished);
		if (bN && !bPrevNext) Restart((TrackIndex+1)%4);
		if (bR && !bPrevRestart && State==ENKState::Finished) Restart(TrackIndex);
		bPrevNext=bN; bPrevRestart=bR;
		if (PC->IsInputKeyDown(EKeys::Escape)) { PC->ConsoleCommand(TEXT("quit")); }
	}
	if (State==ENKState::Countdown)
	{
		Cd -= Dt; if (Cd<=0) State = ENKState::Race;
		for (ANKKart* K : Karts) K->Simulate(Dt, FNKInput());
		return;
	}
	RaceTime += Dt;
	{ // mode test automatique : -nkauto (le joueur est piloté) / -nkshots (captures puis quitter)
		static bool bAuto = FParse::Param(FCommandLine::Get(), TEXT("nkauto")); static bool bShots = FParse::Param(FCommandLine::Get(), TEXT("nkshots"));
		if (bAuto && !Player->bFinished) Player->PIn = Player->ComputeAI(Dt);
		static int32 Shot=0; const float Times[] = {2.f, 9.f, 18.f, 30.f, 45.f};
		static ACameraActor* DCam=nullptr;
		if (bShots && Shot<5 && RaceTime>=Times[Shot]-0.3f && !DCam && (Shot==1||Shot==3))
		{ DCam = GetWorld()->SpawnActor<ACameraActor>(); const FVector L=Player->GetActorLocation(); const FVector F=Player->GetActorForwardVector();
		  if (Shot==1) { DCam->SetActorLocation(L + FVector(0,0,6000) - F*3000); DCam->SetActorRotation((L-DCam->GetActorLocation()).Rotation()); }
		  else { DCam->SetActorLocation(L + FVector(0,0,600) + Player->GetActorRightVector()*1500); DCam->SetActorRotation((L-DCam->GetActorLocation()).Rotation()); }
		  if (APlayerController* Q=GetWorld()->GetFirstPlayerController()) Q->SetViewTarget(DCam); }
		if (bShots && Shot<5 && RaceTime>=Times[Shot]) { FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("nk_t%d_%d.png"), TrackIndex, Shot), false, false);
		  const FVector L=Player->GetActorLocation(); FString Dbg=FString::Printf(TEXT("t=%.1f pos=%s gz=%.0f grounded=%d idx=%d speed=%.0f P=%s cam=%s\n"), RaceTime, *L.ToString(), Player->GroundZ, Player->bGrounded, Player->Idx, Player->Speed, *Track->P[Player->Idx].ToString(), *Player->Cam->GetComponentLocation().ToString());
		  FFileHelper::SaveStringToFile(Dbg, *(FPaths::ProjectSavedDir()/TEXT("nkdbg.txt")), FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
		  Shot++; if (DCam && Shot!=2 && Shot!=4){} }
		if (DCam && (Shot==2||Shot==4) && RaceTime>=Times[Shot-1]+0.5f) { if (APlayerController* Q=GetWorld()->GetFirstPlayerController()) Q->SetViewTarget(Player); DCam->Destroy(); DCam=nullptr; }
		if (bShots && Shot>=5 && RaceTime>48.f) { if (APlayerController* Q=GetWorld()->GetFirstPlayerController()) Q->ConsoleCommand(TEXT("quit")); }
	}
	for (ANKKart* K : Karts)
	{
		FNKInput In = (K==Player && !K->bFinished) ? K->PIn : K->ComputeAI(Dt);
		if (K!=Player) { K->AIMul = K->AISkill * (Player->Rank < K->Rank ? 1.04f : 0.97f); }
		K->Simulate(Dt, In);
		if (!K->bFinished && K->Laps > TotalLaps) { K->bFinished = true; K->FinishTime = RaceTime; }
	}
	// collisions entre karts
	for (int32 it=0; it<3; it++) for (int32 i=0;i<Karts.Num();i++) for (int32 j=i+1;j<Karts.Num();j++)
	{
		ANKKart* A = Karts[i]; ANKKart* B = Karts[j]; FVector d = B->GetActorLocation()-A->GetActorLocation(); d.Z=0; const float L = d.Size();
		const float RR=320.f; if (L < RR && L > 1.f && FMath::Abs(B->GetActorLocation().Z-A->GetActorLocation().Z) < 150.f)
		{ const FVector n = d/L*(RR-L)*0.5f; A->SetActorLocation(A->GetActorLocation()-n); B->SetActorLocation(B->GetActorLocation()+n); const float push=(RR-L)/RR*(it==0?1.f:0.f); if (FVector::DotProduct(A->GetActorForwardVector(), d/L)>0.6f) A->Speed*=1.f-0.05f*push; if (FVector::DotProduct(B->GetActorForwardVector(), -d/L)>0.6f) B->Speed*=1.f-0.05f*push; }
	}
	{ static float Dbg=0; Dbg+=Dt; if (Dbg>2.f){ Dbg=0; const FVector L=Player->GetActorLocation(); UE_LOG(LogNK, Log, TEXT("DBG pos=%s gz=%.0f grounded=%d idx=%d prog=%.0f speed=%.0f gas=%d steer=%.2f laps=%d P=%s"), *L.ToString(), Player->GroundZ, Player->bGrounded, Player->Idx, Player->Prog, Player->Speed, Player->PIn.Gas, Player->PIn.Steer, Player->Laps, *Track->P[Player->Idx].ToString()); } }
	Ranks();
	if (Player->bFinished && State!=ENKState::Finished) State = ENKState::Finished;
}

void ANKHUD::DrawHUD()
{
	Super::DrawHUD();
	ANKGameMode* GM = GetWorld()->GetAuthGameMode<ANKGameMode>(); if (!GM || !GM->Player || !Canvas) return;
	UFont* Big = GEngine->GetLargeFont(); UFont* Med = GEngine->GetMediumFont();
	const float Sx = Canvas->ClipX, Sy = Canvas->ClipY, Sc = Sy/720.f;
	auto Txt = [&](const FString& T, float X, float Y, float S, FLinearColor C, bool bCenter=false)
	{
		float w,h; Canvas->StrLen(Big, T, w, h); w*=S; FCanvasTextItem I(FVector2D(bCenter?X-w/2:X, Y), FText::FromString(T), Big, C); I.Scale = FVector2D(S,S); I.EnableShadow(FLinearColor(0,0,0,0.7f)); Canvas->DrawItem(I);
	};
	ANKKart* P = GM->Player; const int32 R = P->Rank+1;
	Txt(FString::Printf(TEXT("%d%s"), R, R==1?TEXT("er"):TEXT("e")), 30*Sc, 20*Sc, 3.2f*Sc, FLinearColor(1,0.82f,0.4f));
	Txt(FString::Printf(TEXT("/%d"), GM->Karts.Num()), 30*Sc+90*Sc, 52*Sc, 1.3f*Sc, FLinearColor::White);
	Txt(FString::Printf(TEXT("TOUR %d/%d"), FMath::Clamp(P->Laps,1,GM->TotalLaps), GM->TotalLaps), 34*Sc, 112*Sc, 1.2f*Sc, FLinearColor::White);
	const int32 Ms = (int32)(GM->RaceTime*1000); Txt(FString::Printf(TEXT("%d:%02d.%02d"), Ms/60000, (Ms/1000)%60, (Ms/10)%100), Sx-200*Sc, 24*Sc, 1.5f*Sc, FLinearColor::White);
	Txt(FString::Printf(TEXT("%d KM/H"), FMath::RoundToInt(FMath::Abs(P->Speed)*0.036f)), Sx-230*Sc, Sy-80*Sc, 1.8f*Sc, FLinearColor::White);
	if (P->Drift && P->TurboLv>0) Txt(P->TurboLv==3?TEXT("TURBO VIOLET"):P->TurboLv==2?TEXT("TURBO ORANGE"):TEXT("TURBO BLEU"), Sx/2, Sy-120*Sc, 1.3f*Sc, P->TurboLv==3?FLinearColor(0.8f,0.3f,1):P->TurboLv==2?FLinearColor(1,0.55f,0.1f):FLinearColor(0.3f,0.7f,1), true);
	Txt(GM->Track->TrackName, Sx/2, 18*Sc, 1.1f*Sc, FLinearColor(1,1,1,0.85f), true);
	if (GM->State==ENKState::Countdown){ const int32 n = FMath::CeilToInt(GM->Cd-0.9f); Txt(n>0?FString::FromInt(n):TEXT("PARTEZ !"), Sx/2, Sy*0.32f, 5.f*Sc, FLinearColor(1,0.85f,0.3f), true); }
	if (GM->State==ENKState::Finished)
	{
		Txt(R==1?TEXT("VICTOIRE !"):FString::Printf(TEXT("ARRIVÉE : %d%s"), R, R==1?TEXT("er"):TEXT("e")), Sx/2, Sy*0.28f, 3.5f*Sc, FLinearColor(1,0.85f,0.3f), true);
		Txt(TEXT("Entrée / A : rejouer    ·    N / Start : circuit suivant"), Sx/2, Sy*0.42f, 1.0f*Sc, FLinearColor::White, true);
	}
	else Txt(TEXT("N : circuit suivant"), Sx-210*Sc, Sy-30*Sc, 0.7f*Sc, FLinearColor(1,1,1,0.6f));
}
