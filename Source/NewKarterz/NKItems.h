#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKItems.generated.h"

class ANKTrack; class ANKKart; class UInstancedStaticMeshComponent; class UMaterialInstanceDynamic;

// objets : -1 = aucun
enum ENKItem : int32 { NKI_None=-1, NKI_Banana=0, NKI_Green, NKI_Red, NKI_Turbo, NKI_Shield, NKI_Count };

struct FNKProj { int32 Type=0; FVector Pos=FVector::ZeroVector; FVector Vel=FVector::ZeroVector; float Life=0, Age=0; int32 Idx=0; TWeakObjectPtr<ANKKart> Owner; TWeakObjectPtr<ANKKart> Target; };
struct FNKBox { FVector Pos=FVector::ZeroVector; float Respawn=0; };

UCLASS()
class NEWKARTERZ_API ANKItems : public AActor
{
	GENERATED_BODY()
public:
	ANKItems();
	void Setup(ANKTrack* InTrack);
	void Update(float Dt, const TArray<TObjectPtr<ANKKart>>& Karts);
	void Use(ANKKart* K, const TArray<TObjectPtr<ANKKart>>& Karts);
	void HitKart(ANKKart* K);
	static FString ItemName(int32 It);
	static FLinearColor ItemColor(int32 It);

	int32 StatUsed=0, StatHits=0, StatBoxes=0;
	TArray<FNKBox> Boxes; TArray<FNKProj> Projs; float Time=0;

	UPROPERTY() TObjectPtr<ANKTrack> Track;
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IBox;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IBanana;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IGreen;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IRed;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IOrb;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BoxMID;
private:
	int32 Roll(int32 Rank, int32 Num) const;
	bool Ground(const FVector& At, float& Z) const;
	UInstancedStaticMeshComponent* MkISM(const TCHAR* Name, const TCHAR* Mesh, const FLinearColor& C, UMaterialInstanceDynamic** OutMID=nullptr);
};
