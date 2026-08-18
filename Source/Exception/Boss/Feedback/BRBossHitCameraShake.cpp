#include "Boss/Feedback/BRBossHitCameraShake.h"

UBRBossHitShakePattern::UBRBossHitShakePattern(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBRBossHitShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(ShakeDuration);
	OutInfo.BlendIn = 0.0f;
	OutInfo.BlendOut = 0.08f;
}

void UBRBossHitShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	ElapsedTime = 0.0f;
}

void UBRBossHitShakePattern::UpdateShakePatternImpl(
	const FCameraShakePatternUpdateParams& Params,
	FCameraShakePatternUpdateResult& OutResult)
{
	ElapsedTime = FMath::Min(ElapsedTime + FMath::Max(Params.DeltaTime, 0.0f), ShakeDuration);
	FillShakeResult(ElapsedTime, OutResult);
}

void UBRBossHitShakePattern::ScrubShakePatternImpl(
	const FCameraShakePatternScrubParams& Params,
	FCameraShakePatternUpdateResult& OutResult)
{
	FillShakeResult(FMath::Clamp(Params.AbsoluteTime, 0.0f, ShakeDuration), OutResult);
}

bool UBRBossHitShakePattern::IsFinishedImpl() const
{
	return ElapsedTime >= ShakeDuration;
}

void UBRBossHitShakePattern::FillShakeResult(float Time, FCameraShakePatternUpdateResult& OutResult) const
{
	const float LifeAlpha = ShakeDuration > KINDA_SMALL_NUMBER
		? 1.0f - FMath::Clamp(Time / ShakeDuration, 0.0f, 1.0f)
		: 0.0f;
	const float MainWave = FMath::Sin(Time * 23.0f * 2.0f * PI) * LifeAlpha;
	const float SideWave = FMath::Sin((Time * 31.0f * 2.0f * PI) + 0.7f) * LifeAlpha;

	OutResult.Location = FVector(-2.0f * LifeAlpha, SideWave * 1.5f, MainWave * 2.5f);
	OutResult.Rotation = FRotator(MainWave * 1.2f, SideWave * 0.8f, SideWave * 0.35f);
	OutResult.FOV = MainWave * 0.15f;
}

UBRBossHitCameraShake::UBRBossHitCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBRBossHitShakePattern>(TEXT("RootShakePattern")))
{
	bSingleInstance = true;
}
