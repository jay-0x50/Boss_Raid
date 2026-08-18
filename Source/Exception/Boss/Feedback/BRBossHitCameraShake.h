#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "BRBossHitCameraShake.generated.h"

/**
 * Small code-only shake used by boss combat. Keeping the shake in C++ means
 * basic hit feedback still works when a Blueprint has no camera-shake asset.
 */
UCLASS()
class EXCEPTION_API UBRBossHitShakePattern final : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UBRBossHitShakePattern(const FObjectInitializer& ObjectInitializer);

private:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual void ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;

	void FillShakeResult(float Time, FCameraShakePatternUpdateResult& OutResult) const;

	float ElapsedTime = 0.0f;
	float ShakeDuration = 0.22f;
};

UCLASS()
class EXCEPTION_API UBRBossHitCameraShake final : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UBRBossHitCameraShake(const FObjectInitializer& ObjectInitializer);
};
