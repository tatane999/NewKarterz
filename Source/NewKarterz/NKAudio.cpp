#include "NKAudio.h"
#include "NKGameMode.h"
#include "NKKart.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"

ANKAudio::ANKAudio()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

USoundBase* ANKAudio::S(const TCHAR* N)
{
	if (TObjectPtr<USoundBase>* F = Cache.Find(N)) return *F;
	USoundBase* Snd = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/NK/Audio/%s.%s"), N, N));
	Cache.Add(N, Snd);
	return Snd;
}

static const TCHAR* TrackMusic[4][2] = {
	{ TEXT("sunny_city_bpm120"),          TEXT("sunny_city_climax_bpm135") },
	{ TEXT("joyful_jungle_bpm140"),       TEXT("joyful_jungle_climax_bpm155") },
	{ TEXT("darkness_road_remake_bpm165"),TEXT("darkness_road_climax_remake_bpm180") },
	{ TEXT("sky_blue_street_bpm165"),     TEXT("sky_blue_street_climax_bpm180") } };

void ANKAudio::Setup(ANKGameMode* InGM)
{
	GM = InGM;
	Att = NewObject<USoundAttenuation>(this);
	Att->Attenuation.bAttenuate = true; Att->Attenuation.bSpatialize = true;
	Att->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	Att->Attenuation.AttenuationShapeExtents = FVector(600.f); Att->Attenuation.FalloffDistance = 5000.f;
	Music = NewObject<UAudioComponent>(this); Music->SetupAttachment(RootComponent); Music->bIsUISound = true; Music->bAutoActivate = false; Music->RegisterComponent();
	for (ANKKart* K : GM->Karts)
	{
		const bool bP = K == GM->Player;
		UAudioComponent* E = NewObject<UAudioComponent>(K); E->SetupAttachment(K->GetRootComponent()); E->SetSound(S(TEXT("engine")));
		E->bAutoActivate = false; if (!bP) E->AttenuationSettings = Att; E->SetVolumeMultiplier(bP ? 0.32f : 0.5f); E->RegisterComponent(); E->Play(FMath::FRand()*1.5f);
		UAudioComponent* D = NewObject<UAudioComponent>(K); D->SetupAttachment(K->GetRootComponent()); D->SetSound(S(TEXT("drift")));
		D->bAutoActivate = false; if (!bP) D->AttenuationSettings = Att; D->SetVolumeMultiplier(0.f); D->RegisterComponent(); D->Play(FMath::FRand());
		Engines.Add(E); Drifts.Add(D);
	}
	Prev.SetNum(GM->Karts.Num());
}

void ANKAudio::Play(const TCHAR* N, ANKKart* K, float Vol, float Pitch)
{
	USoundBase* Snd = S(N); if (!Snd) return;
	if (!K || K == GM->Player) UGameplayStatics::PlaySound2D(this, Snd, Vol, Pitch);
	else UGameplayStatics::PlaySoundAtLocation(this, Snd, K->GetActorLocation(), Vol*0.8f, Pitch, 0.f, Att);
}

void ANKAudio::UI(bool bOk) { Play(bOk ? TEXT("ui_ok") : TEXT("ui_move"), nullptr, 0.8f); }

void ANKAudio::SetMusic(int32 Mode)
{
	// 0 menu, 1 course, 2 dernier tour, 3 arrivée
	if (Mode == MusicMode || !Music) return;
	MusicMode = Mode;
	const int32 T = FMath::Clamp(GM->TrackIndex, 0, 3);
	const TCHAR* N = Mode==0 ? TEXT("fever_stadium_bpm165") : Mode==1 ? TrackMusic[T][0] : Mode==2 ? TrackMusic[T][1]
		: (Rank0 == 0 ? TEXT("j_best_loop") : Rank0 < 4 ? TEXT("j_good_loop") : TEXT("j_bad_loop"));
	Music->Stop(); Music->SetSound(S(N)); Music->SetVolumeMultiplier(Mode==3 ? 0.55f : 0.42f);
	if (Mode == 3) Music->Play(0.f); else Music->FadeIn(Mode==2 ? 0.3f : 1.2f, 1.f);
}

void ANKAudio::Update(float Dt)
{
	if (!GM || !GM->Player) return;
	ANKKart* P = GM->Player;
	// musique
	if (GM->State == ENKState::Menu) SetMusic(0);
	else if (GM->State == ENKState::Countdown) { if (MusicMode == 0 || MusicMode == -1) { Music->FadeOut(0.6f, 0.f); MusicMode = -2; Play(TEXT("j_grid"), nullptr, 0.6f); } }
	else if (GM->State == ENKState::Race)
	{
		if (P->Laps >= GM->TotalLaps && MusicMode != 2) { Play(TEXT("j_finallap"), nullptr, 0.7f); SetMusic(2); }
		else if (P->Laps < GM->TotalLaps) SetMusic(1);
	}
	else if (GM->State == ENKState::Finished && MusicMode != 3 && FinishT < 0)
	{
		Rank0 = P->Rank; FinishT = 0; Music->FadeOut(0.3f, 0.f);
		Play(Rank0 == 0 ? TEXT("j_best") : Rank0 < 4 ? TEXT("j_good") : TEXT("j_bad"), nullptr, 0.7f);
	}
	if (FinishT >= 0 && MusicMode != 3) { FinishT += Dt; if (FinishT > 4.5f) SetMusic(3); }
	if (Music) Music->SetPaused(GM->bPaused);
	// décompte
	if (GM->State == ENKState::Countdown)
	{
		const int32 N = FMath::CeilToInt(GM->Cd - 0.9f);
		if (N != LastBeep && N > 0 && N <= 3) Play(TEXT("beep"), nullptr, 0.7f);
		LastBeep = N; bGo = false;
	}
	else if (GM->State == ENKState::Race && !bGo) { bGo = true; Play(TEXT("go"), nullptr, 0.7f); }
	// karts
	for (int32 i = 0; i < GM->Karts.Num(); i++)
	{
		ANKKart* K = GM->Karts[i]; if (!K) continue;
		FNKKartSnd& Q = Prev[i];
		const float SpdF = FMath::Clamp(FMath::Abs(K->Speed)/K->MaxSpeed, 0.f, 1.4f);
		if (Engines.IsValidIndex(i) && Engines[i])
		{
			Engines[i]->SetPitchMultiplier(0.7f + SpdF*1.55f + (K->BoostT>0 ? 0.3f : 0.f) + (K->bGrounded ? 0.f : 0.15f));
			Engines[i]->SetPaused(GM->bPaused);
		}
		if (Drifts.IsValidIndex(i) && Drifts[i])
		{
			const float Want = (K->Drift && K->bGrounded) ? (K == P ? 0.28f : 0.35f) : (K->bOff && K->bGrounded && SpdF > 0.3f ? 0.08f : 0.f);
			Drifts[i]->SetVolumeMultiplier(FMath::FInterpTo(Drifts[i]->VolumeMultiplier, Want, Dt, 12.f));
			Drifts[i]->SetPitchMultiplier(0.85f + 0.12f*K->TurboLv); Drifts[i]->SetPaused(GM->bPaused);
		}
		if (GM->State != ENKState::Menu)
		{
			if (Q.bAir && K->bGrounded) Play(TEXT("land"), K, 0.6f);
			if (K->BoostT > Q.Boost + 0.2f) Play(TEXT("boost"), K, 0.8f);
			if (K->RollT > 0 && Q.Roll <= 0) Play(TEXT("itembox"), K, 0.7f);
			if (K == P && K->RollT > 0 && FMath::FloorToInt(K->RollT*14.f) != FMath::FloorToInt(Q.Roll*14.f)) Play(TEXT("tick"), K, 0.35f);
			if (K->RollT <= 0 && Q.Roll > 0 && K == P) Play(TEXT("itemget"), K, 0.7f);
			if (K->HitFlash > Q.Hit + 0.5f) Play(TEXT("hit"), K, 0.9f);
			if (K->ShieldT > Q.Shield + 1.f) Play(TEXT("shield"), K, 0.7f);
			else if (Q.Item >= 0 && K->Item < 0 && K->RollT <= 0 && K->BoostT <= Q.Boost + 0.2f) Play(TEXT("throw"), K, 0.7f);
			if (K == P && K->Laps > Q.Laps && Q.Laps >= 1 && K->Laps <= GM->TotalLaps) Play(TEXT("lap"), K, 0.6f);
		}
		Q.bAir = !K->bGrounded; Q.Drift = K->Drift; Q.Boost = K->BoostT; Q.Roll = K->RollT; Q.Hit = K->HitFlash; Q.Shield = K->ShieldT; Q.Item = K->Item; Q.Laps = K->Laps;
	}
}
