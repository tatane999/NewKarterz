#include "NKItems.h"
#include "NKTrack.h"
#include "NKKart.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

ANKItems::ANKItems()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); RootComponent = Root;
}

UInstancedStaticMeshComponent* ANKItems::MkISM(const TCHAR* Name, const TCHAR* Mesh, const FLinearColor& C, UMaterialInstanceDynamic** OutMID, const TCHAR* MatPath, bool bShadow)
{
	UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this, Name);
	I->SetupAttachment(Root); I->SetMobility(EComponentMobility::Movable);
	I->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Mesh));
	I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	I->SetCastShadow(bShadow);
	UMaterialInterface* Base = MatPath ? LoadObject<UMaterialInterface>(nullptr, MatPath) : nullptr;
	if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Base)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
		M->SetVectorParameterValue(TEXT("Color"), C); I->SetMaterial(0, M);
		if (OutMID) *OutMID = M;
	}
	I->RegisterComponent();
	return I;
}

bool ANKItems::Ground(const FVector& At, float& Z) const
{
	FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(NKItemGround), true, this);
	if (GetWorld()->LineTraceSingleByChannel(H, At + FVector(0,0,500), At - FVector(0,0,3000), ECC_WorldStatic, Q)) { Z = H.ImpactPoint.Z; return true; }
	return false;
}

void ANKItems::Setup(ANKTrack* InTrack)
{
	Track = InTrack;
	{ UMaterialInstanceDynamic* BM=nullptr; IBox = MkISM(TEXT("Boxes"), TEXT("/Engine/BasicShapes/Cube.Cube"), FLinearColor(0.3f,0.6f,1.f), &BM, TEXT("/Game/NK/Mat/M_ItemBox.M_ItemBox"), false); BoxMID = BM; }
	ICore   = MkISM(TEXT("Cores"),   TEXT("/Engine/BasicShapes/Cube.Cube"),     FLinearColor(5.f,5.f,4.f), nullptr, TEXT("/Game/NK/Mat/M_ItemCore.M_ItemCore"), false);
	IBanana = MkISM(TEXT("Bananas"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), FLinearColor(1.f,0.8f,0.05f), nullptr, TEXT("/Game/NK/Mat/M_ItemGloss.M_ItemGloss"));
	IGreen  = MkISM(TEXT("Green"),   TEXT("/Engine/BasicShapes/Sphere.Sphere"), FLinearColor(0.05f,0.7f,0.1f), nullptr, TEXT("/Game/NK/Mat/M_ItemGloss.M_ItemGloss"));
	IRed    = MkISM(TEXT("Red"),     TEXT("/Engine/BasicShapes/Sphere.Sphere"), FLinearColor(0.85f,0.03f,0.03f), nullptr, TEXT("/Game/NK/Mat/M_ItemGloss.M_ItemGloss"));
	IRim    = MkISM(TEXT("Rims"),    TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FLinearColor(0.95f,0.95f,0.9f), nullptr, TEXT("/Game/NK/Mat/M_ItemGloss.M_ItemGloss"));
	IOrb    = MkISM(TEXT("Orbs"),    TEXT("/Engine/BasicShapes/Sphere.Sphere"), FLinearColor(0.4f,2.5f,3.f), nullptr, TEXT("/Game/NK/Mat/M_ItemCore.M_ItemCore"), false);
	// boîtes du circuit + deux rangées supplémentaires à 1/3 et 2/3 du tour
	TArray<FVector> Src = Track->Boxes;
	for (float Fr : {0.33f, 0.66f})
	{
		const int32 i = Track->Wrap((int32)(Track->N*Fr));
		for (float j : {-1.5f, -0.5f, 0.5f, 1.5f}) Src.Add(Track->P[i] + Track->Nor[i]*j*Track->HalfW*0.42f);
	}
	for (const FVector& S : Src) { FNKBox B; float Z; B.Pos = S; if (Ground(S, Z)) B.Pos.Z = Z + 175.f; Boxes.Add(B); }
}

FString ANKItems::ItemName(int32 It)
{
	switch (It) { case NKI_Banana: return TEXT("BANANE"); case NKI_Green: return TEXT("CARAPACE VERTE"); case NKI_Red: return TEXT("CARAPACE ROUGE");
		case NKI_Turbo: return TEXT("TURBO"); case NKI_Shield: return TEXT("BOUCLIER"); default: return TEXT(""); }
}
FLinearColor ANKItems::ItemColor(int32 It)
{
	switch (It) { case NKI_Banana: return FLinearColor(1.f,0.85f,0.1f); case NKI_Green: return FLinearColor(0.15f,0.85f,0.2f); case NKI_Red: return FLinearColor(0.95f,0.15f,0.1f);
		case NKI_Turbo: return FLinearColor(1.f,0.5f,0.05f); case NKI_Shield: return FLinearColor(0.25f,0.85f,1.f); default: return FLinearColor::White; }
}

int32 ANKItems::Roll(int32 Rank, int32 Num) const
{
	// distribution selon le classement : les premiers reçoivent des objets défensifs, les derniers des objets de rattrapage
	const float F = Num>1 ? (float)Rank/(Num-1) : 0.f;
	const int32 W0[NKI_Count] = {45,40, 0, 0,15};
	const int32 W1[NKI_Count] = {20,30,25,15,10};
	const int32 W2[NKI_Count] = { 5,10,35,35,15};
	const int32* W = F<0.25f ? W0 : F<0.6f ? W1 : W2;
	int32 Tot=0; for (int32 i=0;i<NKI_Count;i++) Tot+=W[i];
	int32 r = FMath::RandRange(0, Tot-1);
	for (int32 i=0;i<NKI_Count;i++){ if (r < W[i]) return i; r -= W[i]; }
	return NKI_Banana;
}

void ANKItems::HitKart(ANKKart* K)
{
	if (!K) return;
	StatHits++;
	if (K->ShieldT > 0) { K->ShieldT = 0; return; }
	K->SpinT = 1.3f; K->Speed *= 0.3f; K->Drift = 0; K->DriftT = 0; K->TurboLv = 0; K->BoostT = 0; K->HitFlash = 1.6f;
}

void ANKItems::Use(ANKKart* K, const TArray<TObjectPtr<ANKKart>>& Karts)
{
	if (!K || K->Item < 0 || K->RollT > 0) return;
	const int32 It = K->Item; K->Item = NKI_None; StatUsed++;
	const FVector Pos = K->GetActorLocation(); const FVector Fwd = K->GetActorForwardVector().GetSafeNormal2D();
	FNKProj P; P.Type = It; P.Owner = K; P.Idx = K->Idx;
	switch (It)
	{
	case NKI_Turbo: K->BoostT = FMath::Max(K->BoostT, 1.4f); return;
	case NKI_Shield: K->ShieldT = 8.f; return;
	case NKI_Banana: P.Pos = Pos - Fwd*330.f; P.Life = 60.f; { float Z; if (Ground(P.Pos, Z)) P.Pos.Z = Z; } break;
	case NKI_Red:
		for (ANKKart* O : Karts) if (O && O->Rank == K->Rank-1) P.Target = O;
		// pas de cible : se comporte comme une verte
		P.Life = 9.f; P.Pos = Pos + Fwd*380.f + FVector(0,0,50); P.Vel = Fwd*(FMath::Max(K->Speed,0.f)+3800.f); break;
	case NKI_Green: default:
		P.Life = 7.f; P.Pos = Pos + Fwd*380.f + FVector(0,0,50); P.Vel = Fwd*(FMath::Max(K->Speed,0.f)+3800.f); break;
	}
	Projs.Add(P);
}

void ANKItems::Update(float Dt, const TArray<TObjectPtr<ANKKart>>& Karts)
{
	Time += Dt;
	// états des karts (roulette, bouclier, flash)
	for (ANKKart* K : Karts)
	{
		if (!K) continue;
		if (K->RollT > 0) { K->RollT -= Dt; if (K->RollT <= 0) { K->RollT = 0; K->Item = K->PendingItem; } }
		K->ShieldT = FMath::Max(0.f, K->ShieldT - Dt); K->HitFlash = FMath::Max(0.f, K->HitFlash - Dt);
	}
	// ramassage des boîtes
	for (FNKBox& B : Boxes)
	{
		if (B.Respawn > 0) { B.Respawn -= Dt; continue; }
		for (ANKKart* K : Karts)
		{
			if (!K) continue; const FVector D = K->GetActorLocation() - B.Pos;
			if (D.SizeSquared2D() < 270.f*270.f && FMath::Abs(D.Z) < 320.f)
			{
				B.Respawn = 2.5f; StatBoxes++;
				if (K->Item < 0 && K->RollT <= 0) { K->RollT = 1.1f; K->PendingItem = Roll(K->Rank, Karts.Num()); }
				break;
			}
		}
	}
	// projectiles et pièges
	for (int32 i = Projs.Num()-1; i >= 0; i--)
	{
		FNKProj& P = Projs[i]; P.Age += Dt; P.Life -= Dt;
		bool bDead = P.Life <= 0;
		if (P.Type == NKI_Green || P.Type == NKI_Red)
		{
			if (P.Type == NKI_Red && P.Target.IsValid())
			{
				ANKKart* T = P.Target.Get();
				const FVector To = T->GetActorLocation() - P.Pos;
				const FVector Want = To.Size2D() < 3200.f ? To.GetSafeNormal2D() : (Track->P[Track->Wrap(P.Idx+14)] - P.Pos).GetSafeNormal2D();
				const float Spd = FMath::Max(5400.f, T->Speed + 1600.f);
				P.Vel = (P.Vel.GetSafeNormal2D() + Want*FMath::Min(1.f, Dt*7.f)).GetSafeNormal2D() * Spd;
			}
			P.Pos += P.Vel*Dt;
			P.Idx = Track->Nearest(P.Pos, P.Idx);
			const float Lat = Track->Lateral(P.Pos, P.Idx), L = Track->Lim[P.Idx];
			if (FMath::Abs(Lat) > L)
			{
				const FVector N = Track->Nor[P.Idx];
				P.Pos -= N*(Lat - FMath::Sign(Lat)*L);
				if (P.Type == NKI_Green) P.Vel -= 2.f*FVector::DotProduct(P.Vel, N)*N; // rebond sur les bords
			}
			float Z; if (Ground(P.Pos, Z)) P.Pos.Z = FMath::FInterpTo(P.Pos.Z, Z + 50.f, Dt, 20.f); else bDead = true;
		}
		if (!bDead)
		{
			const float R = P.Type == NKI_Banana ? 200.f : 240.f;
			for (ANKKart* K : Karts)
			{
				if (!K || (K == P.Owner.Get() && P.Age < 0.7f)) continue;
				const FVector D = K->GetActorLocation() - P.Pos;
				if (D.SizeSquared2D() < R*R && FMath::Abs(D.Z) < 250.f) { HitKart(K); bDead = true; break; }
			}
		}
		// les carapaces détruisent les bananes
		if (!bDead && P.Type != NKI_Banana)
			for (int32 j = Projs.Num()-1; j >= 0; j--) if (j!=i && Projs[j].Type==NKI_Banana && FVector::DistSquared(Projs[j].Pos, P.Pos) < 180.f*180.f) { Projs[j].Life = 0; bDead = true; break; }
		if (bDead) Projs.RemoveAt(i);
	}
	// rendu
	if (BoxMID) BoxMID->SetVectorParameterValue(TEXT("Color"), FLinearColor::MakeFromHSV8((uint8)FMath::Fmod(Time*60.f, 255.f), 220, 255));
	TArray<FTransform> XB, XBa, XG, XR, XO, XC, XRim;
	for (const FNKBox& B : Boxes) if (B.Respawn <= 0)
	{
		const FVector Bp = B.Pos + FVector(0,0,FMath::Sin(Time*3.f + B.Pos.X*0.01f)*15.f);
		XB.Add(FTransform(FRotator(Time*40.f, Time*90.f, 20.f), Bp, FVector(1.75f)));
		XC.Add(FTransform(FRotator(-Time*70.f, -Time*120.f, 45.f), Bp, FVector(0.5f + 0.08f*FMath::Sin(Time*6.f))));
	}
	for (const FNKProj& P : Projs)
	{
		if (P.Type == NKI_Banana)
		{
			// banane : deux ellipsoïdes inclinés en V
			XBa.Add(FTransform(FRotator(35, P.Pos.X*0.1f, 0), P.Pos + FVector(0,0,28), FVector(0.22f,0.22f,0.62f)));
			XBa.Add(FTransform(FRotator(-35, P.Pos.X*0.1f, 0), P.Pos + FVector(0,0,28), FVector(0.22f,0.22f,0.62f)));
		}
		else
		{
			(P.Type == NKI_Red ? XR : XG).Add(FTransform(FRotator(0, Time*720.f, 0), P.Pos + FVector(0,0,8), FVector(0.62f,0.62f,0.42f)));
			XRim.Add(FTransform(FRotator(0, Time*720.f, 0), P.Pos - FVector(0,0,4), FVector(0.66f,0.66f,0.09f)));
		}
	}
	for (ANKKart* K : Karts) if (K && K->ShieldT > 0)
		for (int32 o = 0; o < 3; o++) { const float A = Time*4.f + o*2.094f; XO.Add(FTransform(FRotator::ZeroRotator, K->GetActorLocation() + FVector(FMath::Cos(A)*240.f, FMath::Sin(A)*240.f, 110.f), FVector(0.4f))); }
	auto Set = [](UInstancedStaticMeshComponent* I, const TArray<FTransform>& X){ if (!I) return; I->ClearInstances(); if (X.Num()) I->AddInstances(X, false, true); };
	Set(IBox, XB); Set(ICore, XC); Set(IBanana, XBa); Set(IGreen, XG); Set(IRed, XR); Set(IRim, XRim); Set(IOrb, XO);
}
