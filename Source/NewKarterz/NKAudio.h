#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKAudio.generated.h"

class ANKGameMode; class ANKKart; class UAudioComponent; class USoundBase; class USoundAttenuation;

// Sons et musique : musique par circuit (version « dernier tour »), jingles, moteurs, dérapages, bruitages d'événements.
// Musiques : MintoDog (OpenGameArt, CC0). Bruitages : synthétisés (Tools/sfx_gen.py).
struct FNKKartSnd { bool bAir=false; int32 Drift=0; float Boost=0, Roll=0, Hit=0, Shield=0; int32 Item=-1, Laps=0; };

UCLASS()
class NEWKARTERZ_API ANKAudio : public AActor
{
	GENERATED_BODY()
public:
	ANKAudio();
	void Setup(ANKGameMode* InGM);
	void Update(float Dt);
	void UI(bool bOk);
	USoundBase* S(const TCHAR* N);

	UPROPERTY() TObjectPtr<ANKGameMode> GM;
	UPROPERTY() TObjectPtr<UAudioComponent> Music;
	UPROPERTY() TObjectPtr<USoundAttenuation> Att;
	UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Engines;
	UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Drifts;
	UPROPERTY() TMap<FString, TObjectPtr<USoundBase>> Cache;
	TArray<FNKKartSnd> Prev;
	int32 MusicMode=-1; int32 LastBeep=99; float FinishT=-1; bool bGo=false; int32 Rank0=0;
private:
	void Play(const TCHAR* N, ANKKart* K, float Vol=1.f, float Pitch=1.f);
	void SetMusic(int32 Mode);
};
