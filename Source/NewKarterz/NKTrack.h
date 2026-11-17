#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKTrack.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;

USTRUCT()
struct FNKCut { GENERATED_BODY() int32 A=0; int32 B=0; bool bJump=false; TArray<FVector> C; float W=500.f; };

UCLASS()
class NEWKARTERZ_API ANKTrack : public AActor
{
	GENERATED_BODY()
public:
	ANKTrack();
	bool Build(int32 Index);

	// données de piste (unités Unreal : cm, Z vers le haut)
	TArray<FVector> P;      // ligne centrale
	TArray<FVector> Nor;    // normale latérale (horizontale)
	TArray<FVector> Tan;    // tangente
	TArray<float> Lim;      // limite latérale (cm)
	TArray<float> OpenF;    // zone hors-piste ouverte
	TArray<FNKCut> Cuts;
	TArray<FVector> Boxes;
	float HalfW=900.f; int32 N=0; FString TrackName; FString Deco; bool bWater=false;
	float WaterZ=-35.f;
	FVector BBCenter; float BBRadius=0.f;

	int32 Nearest(const FVector& Pos, int32 Hint, int32 Window=80) const;
	int32 NearestGlobal(const FVector& Pos) const;
	float Lateral(const FVector& Pos, int32 I) const { return FVector::DotProduct(Pos-P[I], Nor[I]); }
	int32 Wrap(int32 I) const { return ((I%N)+N)%N; }
	int32 Circ(int32 D) const { return ((D%N)+N+N/2)%N-N/2; }

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Road;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Terrain;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Edges;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Water;
	UPROPERTY() TObjectPtr<UObject> CurbMID;
	UPROPERTY() TArray<TObjectPtr<UActorComponent>> DecorComps;

private:
	TArray<float> H; int32 Res=0; float Ext=0, Cx=0, Cz=0;
	float HeightAtGrid(float GX, float GY) const;
	void BuildRoad();
	void BuildTerrain();
	void BuildEdges();
	void BuildWater();
	void BuildDecor(int32 Index);
	UMaterialInterface* Mat(const FString& Name) const;
};
