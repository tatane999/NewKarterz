#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "NKGameMode.generated.h"

class ANKTrack; class ANKKart; class ANKItems; class ACameraActor; class APlayerController;

UENUM() enum class ENKState : uint8 { Menu, Countdown, Race, Finished };

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
	UPROPERTY() TObjectPtr<ANKItems> Items;
	ENKState State = ENKState::Countdown; float Cd = 3.9f, RaceTime = 0; int32 TrackIndex = 0; int32 TotalLaps = 3; int32 NumKarts = 12;
	void SpawnEnvironment();
	void Ranks();
	void Restart(int32 NewTrack, int32 MenuPage=0);
	// menus
	int32 MenuPage=0, CharSel=0, PlayerInitModel=0, TrackSel=0, PauseSel=0; bool bPaused=false; float MenuT=0;
	TArray<FString> CharNames;
	UPROPERTY() TObjectPtr<ACameraActor> MenuCam;
	static const TCHAR* TrackNames[4];
	void TickMenu(float Dt, APlayerController* PC);
	void StartRace();
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
