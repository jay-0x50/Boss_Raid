#pragma once

#include "CoreMinimal.h"
#include "Boss/Pattern/BRPatternBossBase.h"
#include "BRCMDBoss.generated.h"

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="CMD Final Boss"))
class EXCEPTION_API ABRCMDBoss : public ABRPatternBossBase
{
	GENERATED_BODY()

public:
	ABRCMDBoss();

	UFUNCTION(BlueprintCallable, Category="Exception|CMD")
	void ConfigureCMDPatterns();

protected:
	virtual void OnBossReset() override;
	virtual void OnBossDeadInternal() override;
	virtual void OnBossPhaseChanged(EBRBossPhase NewPhase) override;
	virtual FString GetBossDebugName() const override;
};
