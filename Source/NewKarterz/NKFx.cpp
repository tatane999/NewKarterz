#include "NKFx.h"
#include "NKKart.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ANKFx::ANKFx()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); RootComponent = Root;
}

static UInstancedStaticMeshComponent* MkFx(AActor* O, USceneComponent* R, const TCHAR* Name, const TCHAR* Mat)
{
	UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(O, Name);
	I->SetupAttachment(R); I->SetMobility(EComponentMobility::Movable);
	I->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	I->SetCollisionEnabled(ECollisionEnabled::NoCollision); I->SetCastShadow(false);
	I->NumCustomDataFloats = 4;
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, Mat)) I->SetMaterial(0, M);
	I->RegisterComponent();
	return I;
}

void ANKFx::Setup(const FString& Deco)
{
	Biome = Deco;
	IAdd  = MkFx(this, Root, TEXT("FxAdd"),  TEXT("/Game/NK/Mat/M_FxAdd.M_FxAdd"));
	IDust = MkFx(this, Root, TEXT("FxDust"), TEXT("/Game/NK/Mat/M_FxDust.M_FxDust"));
	DustCol = Deco==TEXT("palm") ? FLinearColor(0.78f,0.68f,0.5f) : Deco==TEXT("pine") ? FLinearColor(0.42f,0.36f,0.26f)
	        : Deco==TEXT("snow") ? FLinearColor(0.95f,0.97f,1.f) : FLinearColor(0.3f,0.26f,0.24f);
}

void ANKFx::Burst(const FVector& At, const FLinearColor& C, int32 N, float Speed, float Size)
{
	for (int32 i=0;i<N;i++)
	{
		FNKPart Q; Q.Kind=0; Q.Pos=At; Q.Vel=FMath::VRand()*Speed*FMath::FRandRange(0.5f,1.f) + FVector(0,0,Speed*0.6f);
		Q.Col=C; Q.Max=Q.Life=FMath::FRandRange(0.4f,0.8f); Q.S0=Size; Q.S1=Size*0.2f; Q.Drag=2.f; Q.Grav=900.f; Add(Q);
	}
}

void ANKFx::Ring(const FVector& At, int32 N)
{
	for (int32 i=0;i<N;i++)
	{
		const float A = i*2.f*PI/N; FNKPart Q; Q.Kind=1; Q.Pos=At+FVector(FMath::Cos(A),FMath::Sin(A),0)*60.f;
		Q.Vel=FVector(FMath::Cos(A),FMath::Sin(A),0.15f)*FMath::FRandRange(500.f,800.f); Q.Col=DustCol;
		Q.Max=Q.Life=FMath::FRandRange(0.6f,1.0f); Q.S0=50.f; Q.S1=170.f; Q.Drag=3.f; Add(Q);
	}
}

void ANKFx::Update(float Dt, const TArray<TObjectPtr<ANKKart>>& Karts, const FVector& CamPos)
{
	Acc += Dt;
	// émetteurs
	for (ANKKart* K : Karts)
	{
		if (!K) continue;
		const FVector Pos = K->GetActorLocation();
		if (FVector::DistSquared(Pos, CamPos) > 9000.f*9000.f) continue;
		const FVector F = K->GetActorForwardVector(), R = K->GetActorRightVector(), U(0,0,1);
		const FVector WL = Pos - F*115.f - R*85.f + U*12.f, WR = Pos - F*115.f + R*85.f + U*12.f;
		// étincelles de dérapage (couleur du niveau de turbo)
		if (K->Drift && K->bGrounded)
		{
			const FLinearColor C = K->TurboLv==3 ? FLinearColor(0.75f,0.3f,1.f) : K->TurboLv==2 ? FLinearColor(1.f,0.45f,0.08f) : K->TurboLv==1 ? FLinearColor(0.25f,0.6f,1.f) : FLinearColor(1.f,0.85f,0.55f);
			const int32 N = K->TurboLv>0 ? 3 : 1;
			for (int32 i=0;i<N;i++) for (const FVector& W : {WL, WR})
			{
				FNKPart Q; Q.Kind=0; Q.Pos=W; Q.Vel=-F*FMath::FRandRange(200.f,600.f) + R*FMath::FRandRange(-350.f,350.f) + U*FMath::FRandRange(150.f,520.f);
				Q.Col=C*(K->TurboLv>0?1.f:0.12f); Q.Max=Q.Life=FMath::FRandRange(0.18f,0.4f); Q.S0=K->TurboLv>0?9.f:5.f; Q.S1=2.f; Q.Drag=1.5f; Q.Grav=1400.f; Add(Q);
			}
		}
		// poussière hors piste / en dérapage
		if (K->bGrounded && FMath::Abs(K->Speed) > 700.f && (K->bOff || K->Drift) && FMath::FRand() < (K->bOff ? 0.8f : 0.35f))
			for (const FVector& W : {WL, WR})
			{
				FNKPart Q; Q.Kind=1; Q.Pos=W+FVector(0,0,20); Q.Vel=-F*K->Speed*0.08f + R*FMath::FRandRange(-120.f,120.f) + U*FMath::FRandRange(40.f,140.f);
				Q.Col=DustCol; Q.Max=Q.Life=FMath::FRandRange(0.6f,1.1f); Q.S0=35.f; Q.S1=FMath::FRandRange(130.f,200.f); Q.Drag=2.5f; Add(Q);
			}
		// flammes du turbo
		if (K->BoostT > 0)
			for (int32 i=0;i<3;i++)
			{
				FNKPart Q; Q.Kind=0; Q.Pos=Pos - F*235.f + U*70.f + R*FMath::FRandRange(-20.f,20.f); Q.Vel=-F*FMath::FRandRange(500.f,1100.f) + FMath::VRand()*80.f + U*60.f;
				Q.Col=FMath::Lerp(FLinearColor(1.f,0.75f,0.2f), FLinearColor(1.f,0.25f,0.05f), FMath::FRand()); Q.Max=Q.Life=FMath::FRandRange(0.12f,0.25f); Q.S0=26.f; Q.S1=6.f; Q.Drag=3.f; Add(Q);
			}
		// atterrissage
		bool& bAir = WasAir.FindOrAdd(K); if (bAir && K->bGrounded) Ring(Pos, 14); bAir = !K->bGrounded;
		// touché : étoiles
		if (K->HitFlash > 1.5f) Burst(Pos + U*120.f, FLinearColor(1.f,0.85f,0.2f), 6, 700.f, 14.f);
	}
	// ambiance du circuit autour de la caméra : neige, lucioles, braises
	{
		const int32 Want = Biome==TEXT("snow") ? 2200 : Biome==TEXT("pine") ? 260 : Biome==TEXT("rock") ? 320 : 0;
		const float Rad = Biome==TEXT("snow") ? 2600.f : 3800.f;
		int32 Spawn = FMath::Min(Want - NumAmb, 90);
		for (int32 i=0;i<Spawn;i++)
		{
			FNKPart Q; Q.bAmb=true; Q.Phase=FMath::FRand()*6.28f;
			const FVector Off(FMath::FRandRange(-Rad,Rad), FMath::FRandRange(-Rad,Rad), 0);
			if (Biome==TEXT("snow")) { Q.Kind=1; Q.Pos=CamPos+Off+FVector(0,0,FMath::FRandRange(-200.f,1500.f)); Q.Vel=FVector(120.f,60.f,-170.f); Q.Col=FLinearColor(1,1,1); Q.Max=Q.Life=FMath::FRandRange(5.f,9.f); Q.S0=Q.S1=FMath::FRandRange(8.f,15.f); Q.Drag=0; }
			else if (Biome==TEXT("pine")) { Q.Kind=0; Q.Pos=CamPos+Off+FVector(0,0,FMath::FRandRange(-500.f,300.f)); Q.Vel=FMath::VRand()*60.f; Q.Col=FLinearColor(0.75f,1.f,0.25f)*0.9f; Q.Max=Q.Life=FMath::FRandRange(3.f,6.f); Q.S0=Q.S1=12.f; Q.Drag=0; }
			else { Q.Kind=0; Q.Pos=CamPos+Off+FVector(0,0,FMath::FRandRange(-600.f,400.f)); Q.Vel=FVector(FMath::FRandRange(-40.f,40.f),FMath::FRandRange(-40.f,40.f),FMath::FRandRange(90.f,220.f)); Q.Col=FLinearColor(1.f,0.35f,0.05f)*0.6f; Q.Max=Q.Life=FMath::FRandRange(2.5f,5.f); Q.S0=6.f; Q.S1=2.f; Q.Drag=0; }
			Add(Q);
		}
	}
	// simulation + rendu
	TArray<FTransform> XA, XD; TArray<float> CA, CD; NumAmb = 0; for (const FNKPart& Q : P) if (Q.bAmb) NumAmb++;
	for (int32 i=P.Num()-1;i>=0;i--)
	{
		FNKPart& Q = P[i]; Q.Life -= Dt;
		if (Q.bAmb && FVector::DistSquared2D(Q.Pos, CamPos) > 5000.f*5000.f) Q.Life = 0;
		if (Q.Life <= 0) { P.RemoveAtSwap(i); continue; }
		if (Q.bAmb && Q.Kind==0 && Biome==TEXT("pine")) Q.Vel += FVector(FMath::Sin(Acc*1.3f+Q.Phase), FMath::Cos(Acc*1.1f+Q.Phase), FMath::Sin(Acc*0.9f+Q.Phase*2.f)*0.5f)*60.f*Dt;
		Q.Vel *= FMath::Max(0.f, 1.f - Q.Drag*Dt); Q.Vel.Z -= Q.Grav*Dt; Q.Pos += Q.Vel*Dt;
		const float T = 1.f - Q.Life/Q.Max; const float S = FMath::Lerp(Q.S0, Q.S1, T) / 50.f; // sphère de base = 100 cm
		if (Q.Kind == 0)
		{
			// étincelle étirée dans le sens de la vitesse
			const float L = FMath::Clamp(Q.Vel.Size()*0.0009f, 1.f, 4.f);
			XA.Add(FTransform(Q.Vel.Rotation(), Q.Pos, FVector(S*L, S, S)));
			const float A = Q.bAmb ? FMath::Min(1.f, FMath::Min(T*5.f, (1.f-T)*4.f)) * (Biome==TEXT("pine") ? 0.5f+0.5f*FMath::Sin(Acc*3.f+Q.Phase) : 1.f) : 1.f - T*T;
			CA.Append({Q.Col.R, Q.Col.G, Q.Col.B, A});
		}
		else
		{
			XD.Add(FTransform(FRotator::ZeroRotator, Q.Pos, FVector(S)));
			CD.Append({Q.Col.R, Q.Col.G, Q.Col.B, Q.bAmb ? 0.9f*FMath::Min(1.f, FMath::Min(T*5.f, (1.f-T)*5.f)) : 0.42f*(1.f - T)*FMath::Min(1.f, T*6.f)});
		}
	}
	auto Push = [](UInstancedStaticMeshComponent* I, const TArray<FTransform>& X, const TArray<float>& C)
	{
		if (!I) return; I->ClearInstances(); if (!X.Num()) return;
		I->AddInstances(X, false, true);
		for (int32 k=0;k<X.Num();k++) I->SetCustomData(k, TArrayView<const float>(&C[k*4], 4), false);
		I->MarkRenderStateDirty();
	};
	Push(IAdd, XA, CA); Push(IDust, XD, CD);
}
