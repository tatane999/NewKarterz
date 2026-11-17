#include "NKKart.h"
#include "NKTrack.h"
#include "NewKarterz.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

ANKKart::ANKKart()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); RootComponent = Root;
	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual")); Visual->SetupAttachment(Root);
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body")); Body->SetupAttachment(Visual);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame")); Flame->SetupAttachment(Visual); Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision); Flame->SetVisibility(false);
	Shadow = nullptr;
	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm")); Arm->SetupAttachment(Root);
	Arm->TargetArmLength = 720.f; Arm->SocketOffset = FVector(0,0,215); Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = true; Arm->CameraLagSpeed = 9.f; Arm->bEnableCameraRotationLag = true; Arm->CameraRotationLagSpeed = 6.f;
	Arm->SetRelativeRotation(FRotator(-8,0,0)); Arm->bInheritPitch=false; Arm->bInheritRoll=false;
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam")); Cam->SetupAttachment(Arm); Cam->FieldOfView = 80.f;
	Cam->PostProcessSettings.bOverride_MotionBlurAmount = true; Cam->PostProcessSettings.MotionBlurAmount = 0.18f;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ANKKart::Init(ANKTrack* InTrack, int32 Slot, bool bPlayer, int32 KartModel, const FString& DriverName)
{
	Track = InTrack; bIsPlayer = bPlayer; Name = DriverName;
	SetModel(KartModel);
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	Flame->SetStaticMesh(Cone); Flame->SetRelativeLocation(FVector(-235,0,70)); Flame->SetRelativeRotation(FRotator(90,0,0)); Flame->SetRelativeScale3D(FVector(0.35f,0.35f,0.8f));
	if (UMaterialInterface* FM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NK/Mat/M_Flame.M_Flame"))) Flame->SetMaterial(0, FM);
	// grille de départ
	const int32 Row = Slot/2; const float Side = (Slot%2)? -1.f: 1.f;
	Idx = Track->Wrap(-8 - Row*6); const FVector P = Track->P[Idx] + Track->Nor[Idx]*Side*Track->HalfW*0.45f;
	Prog = Idx - Track->N; // négatif avant la ligne
	Heading = FMath::RadiansToDegrees(FMath::Atan2(Track->Tan[Idx].Y, Track->Tan[Idx].X));
	SetActorLocation(P + FVector(0,0,20)); SetActorRotation(FRotator(0,Heading,0));
	GroundZ = P.Z; LastSafe = Idx;
	AILane = FMath::FRandRange(-0.55f,0.55f); AISkill = FMath::FRandRange(0.9f,0.99f);
	if (!bIsPlayer){ Cam->Deactivate(); Arm->Deactivate(); }
}

void ANKKart::SetModel(int32 KartModel)
{
	Model = KartModel;
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/NK/Chars/c%d.c%d"), KartModel, KartModel));
	if (!M) M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/NK/Karts/k%d.k%d"), KartModel%13, KartModel%13));
	if (!M) M = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body->SetStaticMesh(M);
	// les GLB sont orientés +Z avant (three.js) : on tourne de -90° pour l'avant +X d'Unreal
	Body->SetRelativeRotation(FRotator(0,-90,0)); Body->SetRelativeScale3D(FVector(1.f));
	if (M && M->GetName()==TEXT("Cube")) { Body->SetRelativeScale3D(FVector(2.4f,1.4f,0.6f)); }
}

void ANKKart::BuildInput()
{
	IMC = NewObject<UInputMappingContext>(this);
	auto MkA = [&](EInputActionValueType T){ UInputAction* A = NewObject<UInputAction>(this); A->ValueType = T; return A; };
	AStr = MkA(EInputActionValueType::Axis1D); AGas = MkA(EInputActionValueType::Boolean); ABrk = MkA(EInputActionValueType::Boolean); ADrf = MkA(EInputActionValueType::Boolean); AItm = MkA(EInputActionValueType::Boolean);
	auto Neg = [&](FEnhancedActionKeyMapping& Mp){ Mp.Modifiers.Add(NewObject<UInputModifierNegate>(this)); };
	// direction : gauche = +1 (comme la version web)
	for (FKey K : {EKeys::Q, EKeys::A, EKeys::Left}) IMC->MapKey(AStr, K);
	for (FKey K : {EKeys::D, EKeys::Right}) Neg(IMC->MapKey(AStr, K));
	Neg(IMC->MapKey(AStr, EKeys::Gamepad_LeftX));
	for (FKey K : {EKeys::Z, EKeys::W, EKeys::Up, EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_RightTriggerAxis}) IMC->MapKey(AGas, K);
	for (FKey K : {EKeys::S, EKeys::Down, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_LeftTriggerAxis}) IMC->MapKey(ABrk, K);
	for (FKey K : {EKeys::SpaceBar, EKeys::LeftShift, EKeys::Gamepad_RightShoulder}) IMC->MapKey(ADrf, K);
	for (FKey K : {EKeys::E, EKeys::LeftControl, EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_FaceButton_Right}) IMC->MapKey(AItm, K);
}

void ANKKart::SetupPlayerInputComponent(UInputComponent* IC)
{
	Super::SetupPlayerInputComponent(IC);
	if (!IMC) BuildInput();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{ Sub->ClearAllMappings(); Sub->AddMappingContext(IMC, 0); }
	if (UEnhancedInputComponent* E = Cast<UEnhancedInputComponent>(IC))
	{
		E->BindAction(AStr, ETriggerEvent::Triggered, this, &ANKKart::OnSteer); E->BindAction(AStr, ETriggerEvent::Completed, this, &ANKKart::OnSteer);
		E->BindAction(AGas, ETriggerEvent::Triggered, this, &ANKKart::OnGas); E->BindAction(AGas, ETriggerEvent::Completed, this, &ANKKart::OnGas);
		E->BindAction(ABrk, ETriggerEvent::Triggered, this, &ANKKart::OnBrk); E->BindAction(ABrk, ETriggerEvent::Completed, this, &ANKKart::OnBrk);
		E->BindAction(ADrf, ETriggerEvent::Triggered, this, &ANKKart::OnDrf); E->BindAction(ADrf, ETriggerEvent::Completed, this, &ANKKart::OnDrf);
		E->BindAction(AItm, ETriggerEvent::Triggered, this, &ANKKart::OnItm); E->BindAction(AItm, ETriggerEvent::Completed, this, &ANKKart::OnItm);
	}
}

bool ANKKart::TraceGround(const FVector& At, float& OutZ, FVector& OutN) const
{
	FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(NKGround), true, this);
	const FVector S = At + FVector(0,0,400), E = At - FVector(0,0,3000);
	if (GetWorld()->LineTraceSingleByChannel(Hit, S, E, ECC_WorldStatic, Q)) { OutZ = Hit.ImpactPoint.Z; OutN = Hit.ImpactNormal; return true; }
	return false;
}

void ANKKart::Tick(float Dt) { Super::Tick(Dt); }

FNKInput ANKKart::ComputeAI(float Dt)
{
	FNKInput In; In.Gas = true;
	const int32 Ahead = 10 + (int32)(Speed/180.f);
	const int32 Ti = Track->Wrap(Idx + Ahead);
	const float Lane = AILane * Track->HalfW * 0.75f;
	const FVector Tgt = Track->P[Ti] + Track->Nor[Ti]*Lane;
	const FVector Me = GetActorLocation();
	const float Want = FMath::RadiansToDegrees(FMath::Atan2(Tgt.Y-Me.Y, Tgt.X-Me.X));
	const float Diff = FMath::FindDeltaAngleDegrees(Heading, Want);
	In.Steer = FMath::Clamp(-Diff/22.f, -1.f, 1.f);
	// dérapage dans les grands virages
	const FVector T0 = Track->Tan[Idx], T1 = Track->Tan[Track->Wrap(Idx+40)];
	const float Bend = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(T0,T1),-1.0,1.0)));
	AIDriftCd -= Dt;
	if (Bend > 38.f && Speed > 2000.f && AIDriftCd<=0 && !Drift) { In.DriftHeld = true; In.DriftPressed = true; AIDriftCd = 1.5f; }
	else if (Drift && Bend > 15.f) In.DriftHeld = true;
	// objets : utilisation après un court délai aléatoire (carapace rouge seulement s'il y a quelqu'un devant)
	if (Item >= 0 && RollT <= 0)
	{
		AIItemT += Dt;
		if (AIItemT > AIItemDelay && (Item != 2 || Rank > 0)) { In.Item = true; AIItemT = 0; AIItemDelay = FMath::FRandRange(0.8f, 3.5f); }
	}
	return In;
}

void ANKKart::Respawn()
{
	const int32 I = LastSafe; const FVector P = Track->P[I];
	SetActorLocation(P + FVector(0,0,150)); Heading = FMath::RadiansToDegrees(FMath::Atan2(Track->Tan[I].Y, Track->Tan[I].X));
	Speed = 0; VZ = 0; Drift = 0; BoostT = 0; Idx = I; bGrounded = false;
}

void ANKKart::Simulate(float Dt, const FNKInput& In0)
{
	if (!Track) return;
	FNKInput In = In0;
	const bool bPressed = In.DriftHeld && !bPrevDrift; bPrevDrift = In.DriftHeld; if (!bIsPlayer) {} else In.DriftPressed = bPressed;
	FVector Pos = GetActorLocation();
	SpinT = FMath::Max(0.f, SpinT-Dt); BoostT = FMath::Max(0.f, BoostT-Dt);
	SteerVis = FMath::FInterpTo(SteerVis, In.Steer, Dt, 10.f);

	// vitesse cible
	float Max = MaxSpeed * AIMul;
	const float Lat = Track->Lateral(Pos, Idx);
	bOff = FMath::Abs(Lat) > Track->HalfW + 90.f && bGrounded;
	bool bOnCut=false; for (const FNKCut& C : Track->Cuts){ for (const FVector& q : C.C){ if (FVector::DistSquared2D(q,Pos) < C.W*C.W){ bOnCut=true; break; } } if (bOnCut) break; }
	if (bOff && !bOnCut && BoostT<=0) Max *= 0.58f;
	if (BoostT>0) Max *= 1.35f;
	if (SpinT>0) { Speed = FMath::FInterpTo(Speed, 0, Dt, 3.f); }
	else if (In.Gas) Speed += (Speed < Max ? Accel : -Accel*1.6f) * Dt * (BoostT>0?2.f:1.f);
	else if (In.Brake) Speed -= (Speed>0 ? 3200.f : 900.f) * Dt;
	else Speed = FMath::FInterpTo(Speed, 0, Dt, 0.8f);
	Speed = FMath::Clamp(Speed, -900.f, Max*1.02f + (BoostT>0?400.f:0.f));

	// saut / dérapage
	if (In.DriftPressed && bGrounded && SpinT<=0) { VZ = 520.f; bGrounded = false; bHopReady = true; HopSq = 1.f; }
	if (!In.DriftHeld) bHopReady = false;
	if (bGrounded && bHopReady && !Drift && FMath::Abs(In.Steer) > 0.3f && Speed > 1200.f) { Drift = In.Steer>0?1:-1; DriftT = 0; TurboLv=0; }
	if (Drift)
	{
		DriftT += Dt * (FMath::Abs(In.Steer)>0.5f && FMath::Sign(In.Steer)==Drift ? 1.25f : 0.8f);
		TurboLv = DriftT > 2.4f ? 3 : DriftT > 1.4f ? 2 : DriftT > 0.65f ? 1 : 0;
		if (!In.DriftHeld || Speed < 900.f)
		{
			if (TurboLv>0) BoostT = FMath::Max(BoostT, TurboLv==3?1.6f:TurboLv==2?1.05f:0.55f);
			Drift = 0; DriftT = 0; TurboLv = 0;
		}
	}
	// direction
	const float SpdF = FMath::Clamp(FMath::Abs(Speed)/MaxSpeed, 0.f, 1.2f);
	float Turn = 125.f * FMath::Clamp(SpdF*2.2f, 0.f, 1.f) * (1.f - 0.3f*SpdF);
	float SteerAmt = In.Steer;
	if (Drift) SteerAmt = Drift*0.62f + In.Steer*0.5f;
	if (!bGrounded) Turn *= 0.45f;
	Heading -= SteerAmt * Turn * Dt * (Speed>=0?1.f:-1.f);

	// déplacement
	const FVector Fwd(FMath::Cos(FMath::DegreesToRadians(Heading)), FMath::Sin(FMath::DegreesToRadians(Heading)), 0);
	FVector Vel = Fwd * Speed;
	if (Drift) { const FVector Side(-Fwd.Y, Fwd.X, 0); Vel += Side * Drift * Speed * 0.16f; }
	Pos += Vel * Dt;

	// suivi de la ligne centrale + limite latérale
	const int32 Ni = Track->Nearest(Pos, Idx);
	const int32 D = Track->Circ(Ni - Idx); Prog += D; Idx = Ni;
	const float Lat2 = Track->Lateral(Pos, Ni); const float L = Track->Lim[Ni];
	if (FMath::Abs(Lat2) > L && !bOnCut)
	{
		Pos -= Track->Nor[Ni] * (Lat2 - FMath::Sign(Lat2)*L);
		Speed *= (1.f - 2.5f*Dt); if (Drift) { Drift=0; DriftT=0; }
	}

	// sol
	float Gz; FVector Gn;
	const bool bHit = TraceGround(Pos, Gz, Gn);
	if (!bHit) Gz = -100000.f;
	if (bGrounded)
	{
		if (Gz < Pos.Z - 120.f) { bGrounded = false; VZ = FMath::Max(0.f, (Pos.Z-GroundZ)); Air = 0; }
		else { Pos.Z = Gz; GroundN = FMath::VInterpTo(GroundN, Gn, Dt, 8.f); }
	}
	if (!bGrounded)
	{
		VZ -= 3200.f * Dt; Pos.Z += VZ * Dt; Air += Dt;
		if (Pos.Z <= Gz) { Pos.Z = Gz; bGrounded = true; VZ = 0; if (Air > 0.45f && bHopReady==false && Air<3.f && Speed>1500.f && In.DriftHeld) BoostT = FMath::Max(BoostT, 0.5f); Air = 0; }
	}
	GroundZ = Pos.Z;
	if (Pos.Z < -2500.f || (Track->bWater && bGrounded && Gz < Track->WaterZ - 900.f)) { SetActorLocation(Pos); Respawn(); return; }
	if (bGrounded && FMath::Abs(Lat2) < Track->HalfW) LastSafe = Ni;

	// tours
	const int32 L2 = FMath::FloorToInt(Prog / Track->N) + 1;
	if (L2 > Laps) { Laps = L2; }

	SetActorLocation(Pos);
	SetActorRotation(FRotator(0, Heading, 0));
	// visuel : inclinaison au sol, roulis, saut
	const FVector Up = GroundN.GetSafeNormal();
	const float Pitch = -FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Up, Fwd), Up.Z));
	const FVector Rt(-Fwd.Y, Fwd.X, 0);
	const float Roll = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Up, Rt), Up.Z)) + SteerVis*3.f*(Drift?1.8f:1.f);
	HopSq = FMath::Max(0.f, HopSq - Dt*4.f);
	const float Yaw = Drift ? -Drift*24.f : 0.f;
	Visual->SetRelativeRotation(FMath::RInterpTo(Visual->GetRelativeRotation(), FRotator(Pitch, Yaw + (SpinT>0?SpinT*800.f:0.f), Roll), Dt, 10.f));
	Visual->SetRelativeScale3D(FVector(1.f, 1.f, 1.f - FMath::Sin(HopSq*PI)*0.12f));
	Flame->SetVisibility(BoostT>0);
	if (bIsPlayer && Cam)
	{
		Cam->SetFieldOfView(FMath::FInterpTo(Cam->FieldOfView, 78.f + SpdF*10.f + (BoostT>0?10.f:0.f), Dt, 4.f));
		Arm->SetRelativeRotation(FRotator(-8, Drift? -Drift*8.f:0, 0));
	}
}
