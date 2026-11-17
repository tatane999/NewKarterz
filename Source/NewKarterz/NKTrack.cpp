#include "NKTrack.h"
#include "PhysicsEngine/BodySetup.h"
#include "NewKarterz.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

static FORCEINLINE FVector JS(double x, double y, double z) { return FVector(x*100.0, z*100.0, y*100.0); }
// ajoute un triangle orienté pour que sa face avant regarde vers Want
static void Tri(TArray<int32>& T, const TArray<FVector>& V, int32 a, int32 b, int32 c, const FVector& Want)
{
	const FVector n = FVector::CrossProduct(V[b]-V[a], V[c]-V[a]);
	if (FVector::DotProduct(n, Want) > 0) T.Append({a,c,b}); else T.Append({a,b,c});
}

ANKTrack::ANKTrack()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* R = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); RootComponent = R;
	Road = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Road")); Road->SetupAttachment(R);
	Terrain = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain")); Terrain->SetupAttachment(R);
	Edges = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Edges")); Edges->SetupAttachment(R);
	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water")); Water->SetupAttachment(R);
	for (UProceduralMeshComponent* C : {Road.Get(), Terrain.Get(), Edges.Get()})
	{
		C->bUseAsyncCooking = true; C->SetCollisionProfileName(TEXT("BlockAll")); C->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); C->bUseComplexAsSimpleCollision = true;
		C->SetMobility(EComponentMobility::Static);
	}
	Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UMaterialInterface* ANKTrack::Mat(const FString& Name) const
{
	UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/NK/Mat/%s_%s.%s_%s"), *Name, *Deco, *Name, *Deco));
	if (!M) M = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/NK/Mat/%s.%s"), *Name, *Name));
	return M;
}

bool ANKTrack::Build(int32 Index)
{
	FString Json; const FString Path = FPaths::ProjectContentDir() / FString::Printf(TEXT("NKData/track%d.json"), Index);
	if (!FFileHelper::LoadFileToString(Json, *Path)) { UE_LOG(LogNK, Error, TEXT("Piste introuvable %s"), *Path); return false; }
	TSharedPtr<FJsonObject> O; TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Rd, O) || !O.IsValid()) return false;
	TrackName = O->GetStringField(TEXT("name")); Deco = O->GetStringField(TEXT("deco")); bWater = O->GetBoolField(TEXT("water"));
	HalfW = O->GetNumberField(TEXT("halfW"))*100.f;
	const TArray<TSharedPtr<FJsonValue>>& JP = O->GetArrayField(TEXT("P"));
	const TArray<TSharedPtr<FJsonValue>>& JN = O->GetArrayField(TEXT("NOR"));
	N = JP.Num(); P.SetNum(N); Nor.SetNum(N); Tan.SetNum(N); Lim.SetNum(N); OpenF.SetNum(N);
	for (int32 i=0;i<N;i++)
	{
		const auto& a = JP[i]->AsArray(); P[i] = JS(a[0]->AsNumber(), a[1]->AsNumber(), a[2]->AsNumber());
		const auto& b = JN[i]->AsArray(); Nor[i] = FVector(b[0]->AsNumber(), b[1]->AsNumber(), 0).GetSafeNormal();
	}
	const auto& JL = O->GetArrayField(TEXT("lim")); const auto& JO = O->GetArrayField(TEXT("openF"));
	for (int32 i=0;i<N;i++){ Lim[i] = (JL.IsValidIndex(i)?JL[i]->AsNumber():(HalfW/100.f+3.4))*100.f; OpenF[i] = JO.IsValidIndex(i)?JO[i]->AsNumber():0.f; }
	for (int32 i=0;i<N;i++) Tan[i] = (P[Wrap(i+1)]-P[Wrap(i-1)]).GetSafeNormal2D();
	for (const auto& V : O->GetArrayField(TEXT("cuts")))
	{
		const TSharedPtr<FJsonObject> c = V->AsObject(); FNKCut K; K.A=c->GetIntegerField(TEXT("a")); K.B=c->GetIntegerField(TEXT("b")); K.bJump = c->GetStringField(TEXT("type"))==TEXT("jump");
		K.W = c->GetNumberField(TEXT("w"))*100.f; for (const auto& q : c->GetArrayField(TEXT("C"))){ const auto& a=q->AsArray(); K.C.Add(JS(a[0]->AsNumber(),a[1]->AsNumber(),a[2]->AsNumber())); }
		Cuts.Add(K);
	}
	for (const auto& V : O->GetArrayField(TEXT("boxes"))){ const auto& a=V->AsArray(); Boxes.Add(JS(a[0]->AsNumber(),a[1]->AsNumber(),a[2]->AsNumber())); }
	const TSharedPtr<FJsonObject> Ter = O->GetObjectField(TEXT("terrain"));
	Res = Ter->GetIntegerField(TEXT("res")); Ext = Ter->GetNumberField(TEXT("ext")); Cx = Ter->GetNumberField(TEXT("cx")); Cz = Ter->GetNumberField(TEXT("cz"));
	const auto& JH = O->GetArrayField(TEXT("H")); H.SetNum(JH.Num()); for (int32 i=0;i<JH.Num();i++) H[i]=JH[i]->AsNumber();
	FBox B(ForceInit); for (const FVector& p : P) B += p; BBCenter = B.GetCenter(); BBRadius = B.GetExtent().Size2D();
	BuildTerrain(); BuildRoad(); BuildEdges(); if (bWater) BuildWater(); BuildDecor(Index);
	UE_LOG(LogNK, Log, TEXT("Piste %s : %d points"), *TrackName, N);
	return true;
}

float ANKTrack::HeightAtGrid(float GX, float GY) const
{
	GX = FMath::Clamp(GX, 0.f, (float)Res-1.001f); GY = FMath::Clamp(GY, 0.f, (float)Res-1.001f);
	const int32 x=(int32)GX, y=(int32)GY; const float fx=GX-x, fy=GY-y;
	const float a=H[y*Res+x], b=H[y*Res+x+1], c=H[(y+1)*Res+x], d=H[(y+1)*Res+x+1];
	return FMath::Lerp(FMath::Lerp(a,b,fx), FMath::Lerp(c,d,fx), fy);
}

void ANKTrack::BuildTerrain()
{
	// maillage visuel fin + maillage de collision plus grossier
	auto Make = [&](int32 Step, int32 Section, bool bCollide)
	{
		const int32 M = (Res-1)/Step+1; TArray<FVector> V; TArray<int32> T; TArray<FVector> Nn; TArray<FVector2D> UV; TArray<FColor> Col; TArray<FProcMeshTangent> Tg;
		V.Reserve(M*M); Nn.Reserve(M*M); UV.Reserve(M*M);
		const float Cell = 2.f*Ext/(Res-1)*Step;
		for (int32 j=0;j<M;j++) for (int32 i=0;i<M;i++)
		{
			const float x = Cx-Ext+2.f*Ext*(i*Step)/(Res-1), z = Cz-Ext+2.f*Ext*(j*Step)/(Res-1);
			const float h = HeightAtGrid(i*Step, j*Step);
			V.Add(JS(x,h,z));
			const float hl=HeightAtGrid(i*Step-Step,j*Step), hr=HeightAtGrid(i*Step+Step,j*Step), hd=HeightAtGrid(i*Step,j*Step-Step), hu=HeightAtGrid(i*Step,j*Step+Step);
			Nn.Add(FVector(-(hr-hl)/(2*Cell), -(hu-hd)/(2*Cell), 1).GetSafeNormal());
			UV.Add(FVector2D(x/8.f, z/8.f));
		}
		T.Reserve((M-1)*(M-1)*6);
		for (int32 j=0;j<M-1;j++) for (int32 i=0;i<M-1;i++)
		{
			const int32 a=j*M+i, b=a+1, c=a+M, d=c+1;
			T.Append({a,c,b, b,c,d});
		}
		Terrain->CreateMeshSection(Section, V, T, Nn, UV, Col, Tg, bCollide);
	};
	Make(2, 0, false);
	Make(4, 1, true); Terrain->SetMeshSectionVisible(1, false);
	if (UMaterialInterface* M = Mat(TEXT("MI_Terrain"))) Terrain->SetMaterial(0, M);
	Terrain->SetCastShadow(true);
}

void ANKTrack::BuildRoad()
{
	TArray<FVector> V; TArray<int32> T; TArray<FVector> Nn; TArray<FVector2D> UV; TArray<FColor> Col; TArray<FProcMeshTangent> Tg;
	float Dist=0; const int32 K=6; // subdivisions transversales
	for (int32 i=0;i<=N;i++)
	{
		const int32 ii = i%N; if (i>0) Dist += FVector::Dist(P[ii], P[(i-1)%N]);
		for (int32 k=0;k<=K;k++)
		{
			const float u = (float)k/K, lat = FMath::Lerp(HalfW, -HalfW, u);
			V.Add(P[ii] + Nor[ii]*lat + FVector(0,0,6)); Nn.Add(FVector::UpVector);
			UV.Add(FVector2D(u*2.f, Dist/(HalfW*1.0f))); Tg.Add(FProcMeshTangent(Tan[ii], false));
		}
	}
	for (int32 i=0;i<N;i++) for (int32 k=0;k<K;k++)
	{
		const int32 a=i*(K+1)+k, b=a+1, c=a+(K+1), d=c+1; Tri(T,V,a,b,c,FVector::UpVector); Tri(T,V,b,d,c,FVector::UpVector);
	}
	Road->CreateMeshSection(0, V, T, Nn, UV, Col, Tg, true);
	if (UMaterialInterface* M = Mat(TEXT("MI_Road"))) Road->SetMaterial(0, M);
	// raccourcis : bande de terre
	int32 S=1;
	for (const FNKCut& C : Cuts)
	{
		TArray<FVector> V2; TArray<int32> T2; TArray<FVector> N2; TArray<FVector2D> U2; TArray<FProcMeshTangent> G2; float D2=0;
		for (int32 i=0;i<C.C.Num();i++)
		{
			const FVector t = (C.C[FMath::Min(i+1,C.C.Num()-1)]-C.C[FMath::Max(i-1,0)]).GetSafeNormal2D(); const FVector n(-t.Y, t.X, 0);
			if (i>0) D2 += FVector::Dist(C.C[i],C.C[i-1]);
			V2.Add(C.C[i]+n*C.W+FVector(0,0,8)); V2.Add(C.C[i]-n*C.W+FVector(0,0,8));
			N2.Add(FVector::UpVector); N2.Add(FVector::UpVector); U2.Add(FVector2D(0,D2/C.W)); U2.Add(FVector2D(1,D2/C.W)); G2.Add(FProcMeshTangent(t,false)); G2.Add(FProcMeshTangent(t,false));
			if (i>0){ const int32 a=(i-1)*2; Tri(T2,V2,a,a+1,a+2,FVector::UpVector); Tri(T2,V2,a+1,a+3,a+2,FVector::UpVector); }
		}
		Road->CreateMeshSection(S, V2, T2, N2, U2, Col, G2, true);
		if (UMaterialInterface* M = Mat(TEXT("MI_Dirt"))) Road->SetMaterial(S, M);
		S++;
	}
}

void ANKTrack::BuildEdges()
{
	// vibreurs rouges/blancs + barrières là où la piste est fermée
	TArray<FVector> V; TArray<int32> T; TArray<FVector> Nn; TArray<FVector2D> UV; TArray<FColor> Col; TArray<FProcMeshTangent> Tg;
	const FColor CA = Deco==TEXT("snow")?FColor(40,120,230):Deco==TEXT("rock")?FColor(255,120,20):FColor(220,30,40);
	for (int32 s : {1,-1})
	{
		const int32 Base = V.Num();
		for (int32 i=0;i<=N;i++)
		{
			const int32 ii=i%N; const FColor C = ((i/6)%2)?FColor::White:CA;
			V.Add(P[ii]+Nor[ii]*s*HalfW+FVector(0,0,9)); V.Add(P[ii]+Nor[ii]*s*(HalfW+110.f)+FVector(0,0,4));
			Nn.Add(FVector::UpVector); Nn.Add(FVector::UpVector); UV.Add(FVector2D(0,i)); UV.Add(FVector2D(1,i)); Col.Add(C); Col.Add(C);
			Tg.Add(FProcMeshTangent(Tan[ii],false)); Tg.Add(FProcMeshTangent(Tan[ii],false));
		}
		for (int32 i=0;i<N;i++){ const int32 a=Base+i*2; Tri(T,V,a,a+1,a+2,FVector::UpVector); Tri(T,V,a+1,a+3,a+2,FVector::UpVector); }
	}
	Edges->CreateMeshSection(0, V, T, Nn, UV, Col, Tg, false);
	const FLinearColor LA = FLinearColor(CA); 
	if (UMaterialInterface* SM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NK/Mat/M_Stripe.M_Stripe")))
	{ UMaterialInstanceDynamic* D0 = UMaterialInstanceDynamic::Create(SM, this); D0->SetVectorParameterValue(TEXT("ColA"), LA); D0->SetScalarParameterValue(TEXT("UseV"), 1.f); D0->SetScalarParameterValue(TEXT("Period"), 12.f); Edges->SetMaterial(0, D0);
	  CurbMID = D0; }
	// barrières (collision) aux limites fermées
	TArray<FVector> V2; TArray<int32> T2; TArray<FVector> N2; TArray<FVector2D> U2; TArray<FColor> C2; TArray<FProcMeshTangent> G2;
	for (int32 s : {1,-1})
	{
		int32 Run=-1;
		for (int32 i=0;i<=N;i++)
		{
			const int32 ii=i%N; const bool Closed = OpenF[ii]<0.01f;
			if (Closed)
			{
				const FVector Bp = P[ii]+Nor[ii]*s*(Lim[ii]+20.f);
				const int32 a=V2.Num(); V2.Add(Bp+FVector(0,0,-30)); V2.Add(Bp+FVector(0,0,95));
				N2.Add(-Nor[ii]*s); N2.Add(-Nor[ii]*s); U2.Add(FVector2D(i*0.25f,0)); U2.Add(FVector2D(i*0.25f,1)); G2.Add(FProcMeshTangent(Tan[ii],false)); G2.Add(FProcMeshTangent(Tan[ii],false));
				const FColor C = ((i/4)%2)?FColor(235,235,235):CA; C2.Add(C); C2.Add(C);
				if (Run>=0){ Tri(T2,V2,Run,Run+1,a,-Nor[ii]*s); Tri(T2,V2,Run+1,a+1,a,-Nor[ii]*s); }
				Run=a;
			}
			else Run=-1;
		}
	}
	Edges->CreateMeshSection(1, V2, T2, N2, U2, C2, G2, true);
	if (UMaterialInterface* SM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/NK/Mat/M_Stripe.M_Stripe")))
	{ UMaterialInstanceDynamic* D1 = UMaterialInstanceDynamic::Create(SM, this); D1->SetVectorParameterValue(TEXT("ColA"), FLinearColor(CA)); D1->SetScalarParameterValue(TEXT("UseV"), 0.f); D1->SetScalarParameterValue(TEXT("Period"), 2.f); Edges->SetMaterial(1, D1); }
}

void ANKTrack::BuildWater()
{
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!Plane) return;
	Water->SetStaticMesh(Plane);
	const float S = Ext*2.2f; // plan de 100 cm => facteur
	Water->SetWorldScale3D(FVector(S, S, 1));
	Water->SetWorldLocation(JS(Cx, -0.45, Cz));
	if (UMaterialInterface* M = Mat(TEXT("M_Water"))) Water->SetMaterial(0, M);
}

int32 ANKTrack::Nearest(const FVector& Pos, int32 Hint, int32 Window) const
{
	int32 Best=Hint; float Bd=TNumericLimits<float>::Max();
	for (int32 d=-Window; d<=Window; d++){ const int32 i=Wrap(Hint+d); const float q=FVector::DistSquared2D(P[i],Pos); if (q<Bd){Bd=q;Best=i;} }
	return Best;
}
int32 ANKTrack::NearestGlobal(const FVector& Pos) const
{
	int32 Best=0; float Bd=TNumericLimits<float>::Max();
	for (int32 i=0;i<N;i++){ const float q=FVector::DistSquared2D(P[i],Pos); if (q<Bd){Bd=q;Best=i;} }
	return Best;
}

static FTransform JSMatrixToUE(const TArray<TSharedPtr<FJsonValue>>& E)
{
	auto e=[&](int i){ return (double)E[i]->AsNumber(); };
	FMatrix M;
	M.M[0][0]=e(0); M.M[0][1]=e(2); M.M[0][2]=e(1); M.M[0][3]=0;
	M.M[1][0]=e(8); M.M[1][1]=e(10); M.M[1][2]=e(9); M.M[1][3]=0;
	M.M[2][0]=e(4); M.M[2][1]=e(6); M.M[2][2]=e(5); M.M[2][3]=0;
	M.M[3][0]=e(12)*100.0; M.M[3][1]=e(14)*100.0; M.M[3][2]=e(13)*100.0; M.M[3][3]=1;
	return FTransform(M);
}

void ANKTrack::BuildDecor(int32 Index)
{
	// décor fixe (fusionné, coordonnées monde)
	if (UStaticMesh* SM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/NK/Decor/t%d_static.t%d_static"), Index, Index)))
	{
				UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this); C->SetStaticMesh(SM); C->SetupAttachment(RootComponent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->RegisterComponent(); DecorComps.Add(C);
	}
	// éléments répétés (arbres, rochers, herbes...) en instances
	FString Json; if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir()/FString::Printf(TEXT("NKData/t%d_inst.json"), Index)))) return;
	TArray<TSharedPtr<FJsonValue>> Arr; TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Json); if (!FJsonSerializer::Deserialize(Rd, Arr)) return;
	// remplacements par des modèles photoscannés (Poly Haven, CC0) : NKData/tN_repl.json = {"i1":{"mesh":"ph_fir","k":0.75}}
	TSharedPtr<FJsonObject> Repl; { FString RJ; if (FFileHelper::LoadFileToString(RJ, *(FPaths::ProjectContentDir()/FString::Printf(TEXT("NKData/t%d_repl.json"), Index)))) { TSharedRef<TJsonReader<>> R2 = TJsonReaderFactory<>::Create(RJ); FJsonSerializer::Deserialize(R2, Repl); } }
	int32 Total=0;
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject(); const FString Id = O->GetStringField(TEXT("id"));
		UStaticMesh* SM = nullptr; float K = 1.f;
		if (Repl.IsValid() && Repl->HasField(Id)) { const TSharedPtr<FJsonObject> E = Repl->GetObjectField(Id); const FString MN = E->GetStringField(TEXT("mesh")); K = E->GetNumberField(TEXT("k"));
			SM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/NK/PH/%s.%s"), *MN, *MN)); }
		if (!SM) { K = 1.f; SM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/NK/Decor/t%d_%s.t%d_%s"), Index, *Id, Index, *Id)); }
		if (!SM) continue;
		UHierarchicalInstancedStaticMeshComponent* HC = NewObject<UHierarchicalInstancedStaticMeshComponent>(this); HC->SetStaticMesh(SM); HC->SetupAttachment(RootComponent);
		HC->SetCollisionEnabled(ECollisionEnabled::NoCollision); HC->SetCullDistances(0, 60000); HC->RegisterComponent();
		TArray<FTransform> Xf; for (const TSharedPtr<FJsonValue>& M : O->GetArrayField(TEXT("M"))) { FTransform T = JSMatrixToUE(M->AsArray()); if (K!=1.f) T.SetScale3D(T.GetScale3D()*K); Xf.Add(T); }
		HC->AddInstances(Xf, false, true); Total += Xf.Num(); DecorComps.Add(HC);
	}
	UE_LOG(LogNK, Log, TEXT("Décor : %d types, %d instances"), Arr.Num(), Total);
}
