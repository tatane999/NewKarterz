#include "NKGameMode.h"
#include "NKTrack.h"
#include "NKKart.h"
#include "NKItems.h"
#include "CanvasItem.h"
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

const TCHAR* ANKGameMode::TrackNames[4] = { TEXT("Lagon Turquoise"), TEXT("Forêt des Lucioles"), TEXT("Volcan Grondant"), TEXT("Pic Givré") };

static bool JustPressed(APlayerController* PC, std::initializer_list<FKey> Ks)
{
	if (!PC) return false;
	for (const FKey& K : Ks) if (PC->WasInputKeyJustPressed(K)) return true;
	return false;
}
#define K_CONFIRM {EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom}
#define K_BACK    {EKeys::Escape, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right}
#define K_LEFT    {EKeys::Left, EKeys::Q, EKeys::A, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left}
#define K_RIGHT   {EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right}
#define K_UP      {EKeys::Up, EKeys::Z, EKeys::W, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_LeftStick_Up}
#define K_DOWN    {EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_LeftStick_Down}

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
	CharNames = CN;
	int32 PlayerChar = UGameplayStatics::GetIntOption(OptionsString, TEXT("char"), CN.IndexOfByKey(TEXT("Melannie")));
	if (!CN.IsValidIndex(PlayerChar)) PlayerChar = 0;
	CharSel = PlayerChar; TrackSel = TrackIndex;
	int32 MenuStart = UGameplayStatics::GetIntOption(OptionsString, TEXT("menu"), 1);
	if (FParse::Param(FCommandLine::Get(), TEXT("nkauto"))) MenuStart = 0;
	TArray<int32> Pool; for (int32 i=0;i<CN.Num();i++) if (i!=PlayerChar) Pool.Add(i);
	for (int32 i=Pool.Num()-1;i>0;i--) Pool.Swap(i, FMath::RandRange(0,i));
	for (int32 s=0; s<NumKarts; s++)
	{
		ANKKart* K = GetWorld()->SpawnActor<ANKKart>(ANKKart::StaticClass(), FTransform::Identity, P);
		const int32 Ci = s==PlayerSlot ? PlayerChar : Pool[s % Pool.Num()];
		K->Init(Track, s, s==PlayerSlot, Ci, CN[Ci]);
		Karts.Add(K); if (s==PlayerSlot) { Player = K; PlayerInitModel = Ci; }
	}
	Items = GetWorld()->SpawnActor<ANKItems>(ANKItems::StaticClass(), FTransform::Identity, P); if (Items) Items->Setup(Track);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->Possess(Player); PC->SetViewTarget(Player); PC->bShowMouseCursor=false; PC->SetInputMode(FInputModeGameOnly()); }
	if (MenuStart > 0)
	{
		State = ENKState::Menu; MenuPage = MenuStart-1;
		MenuCam = GetWorld()->SpawnActor<ACameraActor>(Player->GetActorLocation() + FVector(0,0,1500), FRotator(-30,0,0), P);
		if (MenuCam) { MenuCam->GetCameraComponent()->SetFieldOfView(60.f); MenuCam->GetCameraComponent()->bConstrainAspectRatio = false;
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(MenuCam); }
	}
}

void ANKGameMode::Ranks()
{
	TArray<ANKKart*> A; for (ANKKart* K : Karts) A.Add(K);
	A.Sort([](const ANKKart& a, const ANKKart& b){ if (a.bFinished!=b.bFinished) return a.bFinished; if (a.bFinished) return a.FinishTime<b.FinishTime; return a.Prog>b.Prog; });
	for (int32 i=0;i<A.Num();i++) A[i]->Rank=i;
}

void ANKGameMode::Restart(int32 NewTrack, int32 InMenuPage)
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName().Replace(*GetWorld()->StreamingLevelsPrefix, TEXT(""))), true,
		FString::Printf(TEXT("track=%d?char=%d?menu=%d"), NewTrack, CharSel, InMenuPage));
}

void ANKGameMode::StartRace()
{
	// un adversaire qui aurait le même personnage reprend l'ancien modèle du joueur
	const int32 Old = PlayerInitModel;
	if (Old != CharSel) for (ANKKart* K : Karts) if (K != Player && K->Model == CharSel) { K->SetModel(Old); if (CharNames.IsValidIndex(Old)) K->Name = CharNames[Old]; }
	Player->SetModel(CharSel); if (CharNames.IsValidIndex(CharSel)) Player->Name = CharNames[CharSel];
	State = ENKState::Countdown; Cd = 3.9f;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTargetWithBlend(Player, 1.2f, VTBlend_EaseInOut, 2.f);
}

void ANKGameMode::TickMenu(float Dt, APlayerController* PC)
{
	MenuT += Dt;
	if (MenuPage == 0)
	{
		if (JustPressed(PC, K_CONFIRM)) MenuPage = 1;
		else if (JustPressed(PC, K_BACK)) { if (PC) PC->ConsoleCommand(TEXT("quit")); }
	}
	else if (MenuPage == 1)
	{
		const int32 Nc = FMath::Max(1, CharNames.Num());
		if (JustPressed(PC, K_LEFT))  { CharSel = (CharSel + Nc - 1) % Nc; Player->SetModel(CharSel); }
		if (JustPressed(PC, K_RIGHT)) { CharSel = (CharSel + 1) % Nc; Player->SetModel(CharSel); }
		if (JustPressed(PC, K_CONFIRM)) MenuPage = 2;
		else if (JustPressed(PC, K_BACK)) MenuPage = 0;
	}
	else
	{
		if (JustPressed(PC, K_UP))   TrackSel = (TrackSel + 3) % 4;
		if (JustPressed(PC, K_DOWN)) TrackSel = (TrackSel + 1) % 4;
		if (JustPressed(PC, K_CONFIRM)) { if (TrackSel == TrackIndex) StartRace(); else Restart(TrackSel, 0); return; }
		else if (JustPressed(PC, K_BACK)) MenuPage = 1;
	}
	// caméra du menu : survol large (titre), gros plan qui tourne autour du kart (perso), vue haute (circuit)
	if (MenuCam && Player)
	{
		const FVector C = Player->GetActorLocation() + FVector(0,0,90);
		const float R = MenuPage==1 ? 820.f : MenuPage==0 ? 2600.f : 5200.f;
		const float Hh = MenuPage==1 ? 260.f : MenuPage==0 ? 900.f : 3200.f;
		const float A = MenuT * (MenuPage==1 ? 0.45f : 0.12f) + (MenuPage==1 ? 2.2f : 0.f);
		const FVector Want = C + FVector(FMath::Cos(A)*R, FMath::Sin(A)*R, Hh);
		const FVector Pos = FMath::VInterpTo(MenuCam->GetActorLocation(), Want, Dt, 2.5f);
		const FVector Look = MenuPage==1 ? C + FVector(0,0,20) : C;
		MenuCam->SetActorLocation(Pos); MenuCam->SetActorRotation((Look - Pos).Rotation());
	}
}

void ANKGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Track || !Player) return;
	Dt = FMath::Min(Dt, 1.f/30.f);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	{ // test des menus : -nkmenu (captures nk_menu_*.png puis quitte)
		static bool bMenuTest = FParse::Param(FCommandLine::Get(), TEXT("nkmenu")); static float T=0; static int32 St=0;
		if (bMenuTest)
		{
			T += Dt;
			auto Shot = [&](const TCHAR* N){ FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("nk_menu_%s.png"), N), false, false); St++; };
			if (St==0 && T>2.f) Shot(TEXT("0_titre"));
			else if (St==1 && T>2.5f) { MenuPage = 1; St++; }
			else if (St==2 && T>5.f) Shot(TEXT("1_perso"));
			else if (St==3 && T>5.5f) { CharSel = (CharSel+3) % FMath::Max(1,CharNames.Num()); Player->SetModel(CharSel); St++; }
			else if (St==4 && T>8.f) Shot(TEXT("2_perso"));
			else if (St==5 && T>8.5f) { MenuPage = 2; TrackSel = TrackIndex; St++; }
			else if (St==6 && T>11.f) Shot(TEXT("3_circuit"));
			else if (St==7 && T>11.5f) { StartRace(); St++; }
			else if (St==8 && T>20.f) { bPaused = true; PauseSel = 1; St++; }
			else if (St==9 && T>21.f) Shot(TEXT("4_pause"));
			else if (St==10 && T>21.5f) { bPaused = false; for (ANKKart* K : Karts) if (K->Rank < 6) { K->bFinished = true; K->FinishTime = RaceTime + K->Rank*1.7f; } Player->bFinished = true; Player->FinishTime = RaceTime + 9.f; St++; }
			else if (St==11 && T>24.f) Shot(TEXT("5_resultats"));
			else if (St==12 && T>25.5f) { if (PC) PC->ConsoleCommand(TEXT("quit")); St++; }
		}
	}
	if (State==ENKState::Menu)
	{
		TickMenu(Dt, PC);
		if (State==ENKState::Menu) { for (ANKKart* K : Karts) K->Simulate(Dt, FNKInput()); if (Items) Items->Update(Dt, Karts); }
		return;
	}
	if (bPaused)
	{
		if (JustPressed(PC, K_UP)) PauseSel = (PauseSel + 4) % 5;
		if (JustPressed(PC, K_DOWN)) PauseSel = (PauseSel + 1) % 5;
		if (JustPressed(PC, {EKeys::Escape, EKeys::P, EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Right})) bPaused = false;
		else if (JustPressed(PC, K_CONFIRM))
		{
			switch (PauseSel)
			{
			case 0: bPaused = false; break;
			case 1: Restart(TrackIndex, 0); break;
			case 2: Restart(TrackIndex, 3); break;
			case 3: Restart(TrackIndex, 1); break;
			default: if (PC) PC->ConsoleCommand(TEXT("quit")); break;
			}
		}
		return;
	}
	if (State==ENKState::Finished)
	{
		if (JustPressed(PC, K_CONFIRM)) { Restart(TrackIndex, 0); return; }
		if (JustPressed(PC, {EKeys::N, EKeys::Gamepad_Special_Right})) { Restart((TrackIndex+1)%4, 0); return; }
		if (JustPressed(PC, {EKeys::Escape, EKeys::Gamepad_FaceButton_Right})) { Restart(TrackIndex, 3); return; }
	}
	else if (JustPressed(PC, {EKeys::Escape, EKeys::P, EKeys::Gamepad_Special_Right})) { bPaused = true; PauseSel = 0; return; }
	if (State==ENKState::Countdown)
	{
		Cd -= Dt; if (Cd<=0) State = ENKState::Race;
		for (ANKKart* K : Karts) K->Simulate(Dt, FNKInput());
		if (Items) Items->Update(Dt, Karts);
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
		  const FVector L=Player->GetActorLocation(); FString Dbg=FString::Printf(TEXT("t=%.1f objets=%d touches=%d boites=%d pos=%s gz=%.0f grounded=%d idx=%d speed=%.0f P=%s cam=%s\n"), RaceTime, Items?Items->StatUsed:0, Items?Items->StatHits:0, Items?Items->StatBoxes:0, *L.ToString(), Player->GroundZ, Player->bGrounded, Player->Idx, Player->Speed, *Track->P[Player->Idx].ToString(), *Player->Cam->GetComponentLocation().ToString());
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
		const bool bUse = In.Item && !K->bPrevItem; K->bPrevItem = In.Item;
		if (bUse && Items && K->Item >= 0 && K->RollT <= 0) Items->Use(K, Karts);
		if (!K->bFinished && K->Laps > TotalLaps) { K->bFinished = true; K->FinishTime = RaceTime; }
	}
	if (Items) Items->Update(Dt, Karts);
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
	auto Box = [&](float X, float Y, float W, float H, FLinearColor C)
	{
		FCanvasTileItem T(FVector2D(X,Y), FVector2D(W,H), C); T.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(T);
	};
	const FLinearColor Gold(1,0.82f,0.35f), Dim(1,1,1,0.55f);
	auto Hint = [&](const FString& T){ Box(0, Sy-46*Sc, Sx, 46*Sc, FLinearColor(0,0,0,0.5f)); Txt(T, Sx/2, Sy-38*Sc, 0.85f*Sc, FLinearColor(1,1,1,0.85f), true); };

	// ---------- menus ----------
	if (GM->State == ENKState::Menu)
	{
		const float Pulse = 0.6f + 0.4f*FMath::Sin(GM->MenuT*4.f);
		if (GM->MenuPage == 0)
		{
			Box(0, Sy*0.18f, Sx, Sy*0.30f, FLinearColor(0,0,0,0.35f));
			Txt(TEXT("NEWKARTERZ"), Sx/2+4*Sc, Sy*0.20f+4*Sc, 6.f*Sc, FLinearColor(0.9f,0.25f,0.1f,0.9f), true);
			Txt(TEXT("NEWKARTERZ"), Sx/2, Sy*0.20f, 6.f*Sc, Gold, true);
			Txt(TEXT("Course de karts"), Sx/2, Sy*0.20f + 110*Sc, 1.3f*Sc, FLinearColor::White, true);
			Txt(TEXT("Appuie sur Entrée / A"), Sx/2, Sy*0.68f, 1.4f*Sc, FLinearColor(1,1,1,Pulse), true);
			Hint(TEXT("Entrée / A : jouer    ·    Échap : quitter"));
		}
		else if (GM->MenuPage == 1)
		{
			const int32 Nc = FMath::Max(1, GM->CharNames.Num());
			Txt(TEXT("CHOISIS TON PILOTE"), Sx/2, 40*Sc, 2.f*Sc, Gold, true);
			const FString Nm = GM->CharNames.IsValidIndex(GM->CharSel) ? GM->CharNames[GM->CharSel] : TEXT("?");
			Box(Sx/2-260*Sc, Sy*0.70f, 520*Sc, 90*Sc, FLinearColor(0,0,0,0.5f));
			Txt(Nm, Sx/2, Sy*0.70f + 10*Sc, 2.6f*Sc, FLinearColor::White, true);
			Txt(TEXT("<"), Sx/2-230*Sc, Sy*0.70f + 14*Sc, 2.4f*Sc, FLinearColor(1,1,1,Pulse), true);
			Txt(TEXT(">"), Sx/2+230*Sc, Sy*0.70f + 14*Sc, 2.4f*Sc, FLinearColor(1,1,1,Pulse), true);
			Txt(FString::Printf(TEXT("%d / %d"), GM->CharSel+1, Nc), Sx/2, Sy*0.70f + 96*Sc, 0.9f*Sc, Dim, true);
			// voisins
			for (int32 d : {-2,-1,1,2})
			{
				const int32 i = (GM->CharSel + d + Nc*4) % Nc; if (!GM->CharNames.IsValidIndex(i)) continue;
				Txt(GM->CharNames[i], Sx/2 + d*(d==-1||d==1?400.f:620.f)*Sc, Sy*0.70f + 30*Sc, 1.0f*Sc, FLinearColor(1,1,1,d==-1||d==1?0.6f:0.3f), true);
			}
			Hint(TEXT("Q D / ← → / croix : changer    ·    Entrée / A : valider    ·    Échap / B : retour"));
		}
		else
		{
			Txt(TEXT("CHOISIS TON CIRCUIT"), Sx/2, 40*Sc, 2.f*Sc, Gold, true);
			const float W = 560*Sc, X = Sx/2 - W/2, Y0 = Sy*0.30f, Hh = 74*Sc;
			for (int32 i=0;i<4;i++)
			{
				const bool bSel = i == GM->TrackSel; const float Y = Y0 + i*(Hh+12*Sc);
				Box(X, Y, W, Hh, bSel ? FLinearColor(1,0.6f,0.1f,0.75f) : FLinearColor(0,0,0,0.5f));
				Txt(FString::Printf(TEXT("%d.  %s"), i+1, ANKGameMode::TrackNames[i]), X + 30*Sc, Y + 16*Sc, 1.6f*Sc, bSel ? FLinearColor::White : FLinearColor(1,1,1,0.75f));
			}
			Txt(FString::Printf(TEXT("Pilote : %s    ·    %d tours    ·    %d karts"), GM->CharNames.IsValidIndex(GM->CharSel)?*GM->CharNames[GM->CharSel]:TEXT("?"), GM->TotalLaps, GM->Karts.Num()), Sx/2, Y0 + 4*(Hh+12*Sc) + 16*Sc, 1.0f*Sc, Dim, true);
			Hint(TEXT("Z S / ↑ ↓ : choisir    ·    Entrée / A : c'est parti !    ·    Échap / B : retour"));
		}
		return;
	}

	ANKKart* P = GM->Player; const int32 R = P->Rank+1;
	Txt(FString::Printf(TEXT("%d%s"), R, R==1?TEXT("er"):TEXT("e")), 30*Sc, 20*Sc, 3.2f*Sc, FLinearColor(1,0.82f,0.4f));
	Txt(FString::Printf(TEXT("/%d"), GM->Karts.Num()), 30*Sc+90*Sc, 52*Sc, 1.3f*Sc, FLinearColor::White);
	Txt(FString::Printf(TEXT("TOUR %d/%d"), FMath::Clamp(P->Laps,1,GM->TotalLaps), GM->TotalLaps), 34*Sc, 112*Sc, 1.2f*Sc, FLinearColor::White);
	const int32 Ms = (int32)(GM->RaceTime*1000); Txt(FString::Printf(TEXT("%d:%02d.%02d"), Ms/60000, (Ms/1000)%60, (Ms/10)%100), Sx-200*Sc, 24*Sc, 1.5f*Sc, FLinearColor::White);
	Txt(FString::Printf(TEXT("%d KM/H"), FMath::RoundToInt(FMath::Abs(P->Speed)*0.036f)), Sx-230*Sc, Sy-80*Sc, 1.8f*Sc, FLinearColor::White);
	if (P->Drift && P->TurboLv>0) Txt(P->TurboLv==3?TEXT("TURBO VIOLET"):P->TurboLv==2?TEXT("TURBO ORANGE"):TEXT("TURBO BLEU"), Sx/2, Sy-120*Sc, 1.3f*Sc, P->TurboLv==3?FLinearColor(0.8f,0.3f,1):P->TurboLv==2?FLinearColor(1,0.55f,0.1f):FLinearColor(0.3f,0.7f,1), true);
	// case d'objet
	{
		const float X = 30*Sc, Y = 150*Sc, W = 96*Sc;
		FCanvasTileItem Bg(FVector2D(X,Y), FVector2D(W,W), FLinearColor(0,0,0,0.45f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
		int32 Show = P->Item; if (P->RollT > 0) Show = ((int32)(GM->RaceTime*14.f)) % NKI_Count;
		if (Show >= 0)
		{
			const FLinearColor C = ANKItems::ItemColor(Show);
			FCanvasTileItem In(FVector2D(X+14*Sc,Y+14*Sc), FVector2D(W-28*Sc,W-28*Sc), C*FLinearColor(1,1,1,P->RollT>0?0.6f:0.95f)); In.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(In);
			const FString Nm = ANKItems::ItemName(Show);
			Txt(Nm.Left(1), X+W/2, Y+18*Sc, 2.4f*Sc, FLinearColor::White, true);
			Txt(Nm, X+W/2, Y+W+4*Sc, 0.75f*Sc, C, true);
		}
		else Txt(TEXT("E / LB : objet"), X+W/2, Y+W+4*Sc, 0.6f*Sc, FLinearColor(1,1,1,0.5f), true);
		if (P->ShieldT > 0) Txt(FString::Printf(TEXT("BOUCLIER %.0f s"), FMath::CeilToFloat(P->ShieldT)), Sx/2, Sy-160*Sc, 1.1f*Sc, ANKItems::ItemColor(NKI_Shield), true);
		if (P->HitFlash > 0) Txt(TEXT("TOUCHÉ !"), Sx/2, Sy*0.36f, 2.6f*Sc, FLinearColor(1,0.25f,0.2f,FMath::Min(1.f,P->HitFlash)), true);
	}
	Txt(GM->Track->TrackName, Sx/2, 18*Sc, 1.1f*Sc, FLinearColor(1,1,1,0.85f), true);
	if (GM->State==ENKState::Countdown){ const int32 n = FMath::CeilToInt(GM->Cd-0.9f); Txt(n>0?FString::FromInt(n):TEXT("PARTEZ !"), Sx/2, Sy*0.32f, 5.f*Sc, FLinearColor(1,0.85f,0.3f), true); }
	if (GM->State==ENKState::Finished)
	{
		// résultats : classement complet
		const float W = 620*Sc, X = Sx/2 - W/2, Y0 = 110*Sc, Rh = 34*Sc;
		Txt(R==1?TEXT("VICTOIRE !"):FString::Printf(TEXT("ARRIVÉE : %d%s"), R, TEXT("e")), Sx/2, 30*Sc, 2.6f*Sc, Gold, true);
		TArray<ANKKart*> A; for (ANKKart* K : GM->Karts) A.Add(K); A.Sort([](const ANKKart& a, const ANKKart& b){ return a.Rank < b.Rank; });
		Box(X-10*Sc, Y0-10*Sc, W+20*Sc, A.Num()*Rh+20*Sc, FLinearColor(0,0,0,0.55f));
		for (int32 i=0;i<A.Num();i++)
		{
			ANKKart* K = A[i]; const float Y = Y0 + i*Rh; const bool bMe = K == P;
			if (bMe) Box(X, Y, W, Rh-2*Sc, FLinearColor(1,0.6f,0.1f,0.6f));
			const FLinearColor C = bMe ? FLinearColor::White : FLinearColor(1,1,1,0.85f);
			Txt(FString::Printf(TEXT("%d"), i+1), X + 16*Sc, Y + 4*Sc, 1.05f*Sc, i<3 ? Gold : C);
			Txt(K->Name, X + 80*Sc, Y + 4*Sc, 1.05f*Sc, C);
			FString T;
			if (K->bFinished) { const int32 Ms2 = (int32)(K->FinishTime*1000); T = FString::Printf(TEXT("%d:%02d.%02d"), Ms2/60000, (Ms2/1000)%60, (Ms2/10)%100); }
			else T = FString::Printf(TEXT("tour %d/%d"), FMath::Clamp(K->Laps,1,GM->TotalLaps), GM->TotalLaps);
			Txt(T, X + W - 170*Sc, Y + 4*Sc, 1.05f*Sc, K->bFinished ? C : Dim);
		}
		Hint(TEXT("Entrée / A : rejouer    ·    N / Start : circuit suivant    ·    Échap / B : changer de circuit"));
	}
	else if (GM->bPaused)
	{
		Box(0, 0, Sx, Sy, FLinearColor(0,0,0,0.55f));
		Txt(TEXT("PAUSE"), Sx/2, Sy*0.16f, 3.4f*Sc, Gold, true);
		static const TCHAR* Opt[5] = { TEXT("Reprendre"), TEXT("Recommencer la course"), TEXT("Changer de circuit"), TEXT("Menu principal"), TEXT("Quitter le jeu") };
		const float W = 460*Sc, X = Sx/2 - W/2, Y0 = Sy*0.34f, Hh = 58*Sc;
		for (int32 i=0;i<5;i++)
		{
			const bool bSel = i == GM->PauseSel; const float Y = Y0 + i*(Hh+10*Sc);
			Box(X, Y, W, Hh, bSel ? FLinearColor(1,0.6f,0.1f,0.8f) : FLinearColor(0,0,0,0.5f));
			Txt(Opt[i], Sx/2, Y + 12*Sc, 1.35f*Sc, bSel ? FLinearColor::White : FLinearColor(1,1,1,0.75f), true);
		}
		Hint(TEXT("Z S / ↑ ↓ : choisir    ·    Entrée / A : valider    ·    Échap / Start : reprendre"));
	}
	else Txt(TEXT("Échap / Start : pause"), Sx-230*Sc, Sy-30*Sc, 0.7f*Sc, FLinearColor(1,1,1,0.6f));
}
