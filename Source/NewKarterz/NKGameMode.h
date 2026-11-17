#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "NKGameMode.generated.h"

class ANKTrack; class ANKKart;

UENUM() enum class ENKState : uint8 { Countdown, Race, Finished };

UCLASS()
class NEWKARTERZ_API ANKGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ANKGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;
	UPROPERTY() TObjectPtr<ANKTrack> Track;
	UPROPERTY() TArray<TObjectPtr<ANKKart>> Karts;
	UPROPERTY() TObjectPtr<ANKKart> Player;
	ENKState State = ENKState::Countdown; float Cd = 3.9f, RaceTime = 0; int32 TrackIndex = 0; int32 TotalLaps = 3; int32 NumKarts = 12;
	void SpawnEnvironment();
	void Ranks();
	void Restart(int32 NewTrack);
private:
	bool bPrevNext=false, bPrevRestart=false;
};

UCLASS()
class NEWKARTERZ_API ANKHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
