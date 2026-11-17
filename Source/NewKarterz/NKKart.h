#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "NKKart.generated.h"

class ANKTrack; class USpringArmComponent; class UCameraComponent; class UStaticMeshComponent; class UInputAction; class UInputMappingContext;
class UNiagaraComponent;

USTRUCT() struct FNKInput { GENERATED_BODY() float Steer=0; bool Gas=false; bool Brake=false; bool DriftHeld=false; bool DriftPressed=false; bool Item=false; };

UCLASS()
class NEWKARTERZ_API ANKKart : public APawn
{
	GENERATED_BODY()
public:
	ANKKart();
	virtual void Tick(float Dt) override;
	virtual void SetupPlayerInputComponent(UInputComponent* IC) override;

	void Init(ANKTrack* InTrack, int32 Slot, bool bPlayer, int32 KartModel, const FString& DriverName);
	void Simulate(float Dt, const FNKInput& In);
	FNKInput ComputeAI(float Dt);
	void Respawn();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Visual;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shadow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Cam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flame;

	UPROPERTY() TObjectPtr<ANKTrack> Track;
	bool bIsPlayer=false; FString Name; int32 Rank=0; bool bFinished=false; float FinishTime=0;
	// état physique (cm, s)
	float Speed=0, Heading=0, VZ=0, Air=0; bool bGrounded=true;
	int32 Drift=0; float DriftT=0, BoostT=0, SpinT=0, SteerVis=0, HopSq=0; bool bHopReady=false; int32 TurboLv=0;
	int32 Idx=0; float Prog=0; int32 Laps=0; float LastLapT=0; bool bOff=false;
	float MaxSpeed=3100.f, Accel=1700.f; float AIMul=1.f, AISkill=0.95f, AILane=0.f, AIDriftCd=0;
	FVector GroundN=FVector::UpVector; float GroundZ=0; int32 LastSafe=0;

	// entrées joueur
	FNKInput PIn; bool bPrevDrift=false;
	UPROPERTY() TObjectPtr<UInputMappingContext> IMC;
	UPROPERTY() TObjectPtr<UInputAction> AStr; UPROPERTY() TObjectPtr<UInputAction> AGas; UPROPERTY() TObjectPtr<UInputAction> ABrk; UPROPERTY() TObjectPtr<UInputAction> ADrf; UPROPERTY() TObjectPtr<UInputAction> AItm;
	void OnSteer(const FInputActionValue& V){ PIn.Steer = V.Get<float>(); }
	void OnGas(const FInputActionValue& V){ PIn.Gas = V.Get<bool>(); }
	void OnBrk(const FInputActionValue& V){ PIn.Brake = V.Get<bool>(); }
	void OnDrf(const FInputActionValue& V){ PIn.DriftHeld = V.Get<bool>(); }
	void OnItm(const FInputActionValue& V){ PIn.Item = V.Get<bool>(); }
	void BuildInput();
private:
	bool TraceGround(const FVector& At, float& OutZ, FVector& OutN) const;
};
