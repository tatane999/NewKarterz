#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKFx.generated.h"

class ANKKart; class UInstancedStaticMeshComponent;

// petit système de particules maison (ISM + données par instance) : étincelles, poussière, flammes, étoiles
struct FNKPart { FVector Pos=FVector::ZeroVector, Vel=FVector::ZeroVector; FLinearColor Col=FLinearColor::White; float Life=0, Max=1, S0=10, S1=10, Drag=1.f, Grav=0; uint8 Kind=0; bool bAmb=false; float Phase=0; float Yaw=0; };

UCLASS()
class NEWKARTERZ_API ANKFx : public AActor
{
	GENERATED_BODY()
public:
	ANKFx();
	void Setup(const FString& Deco);
	void Update(float Dt, const TArray<TObjectPtr<ANKKart>>& Karts, const FVector& CamPos);
	void Burst(const FVector& At, const FLinearColor& C, int32 N, float Speed, float Size);   // étoiles (objet touché)
	void Ring(const FVector& At, int32 N);                                                      // poussière à l'atterrissage

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IAdd;   // additif (étincelles, flammes)
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IDust;  // translucide (poussière, neige)
	TArray<FNKPart> P; FLinearColor DustCol = FLinearColor(0.75f,0.68f,0.52f); float Acc=0; FString Biome; int32 NumAmb=0, NumMarks=0;
	TMap<TWeakObjectPtr<ANKKart>, FVector> LastMark;
	TMap<TWeakObjectPtr<ANKKart>, bool> WasAir;
private:
	void Add(const FNKPart& Q) { if (P.Num() < 6000) P.Add(Q); }
};
