#pragma once

#include "CoreMinimal.h"
#include "Boss/Pattern/BRPatternBossBase.h"
#include "BRVritraBoss.generated.h"

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Vritra Boss"))
class EXCEPTION_API ABRVritraBoss : public ABRPatternBossBase
{
	GENERATED_BODY()

public:
	ABRVritraBoss();

	UFUNCTION(BlueprintCallable, Category="Exception|Vritra")
	void ConfigureVritraPatterns();

protected:
	virtual void OnBossReset() override;
	virtual void OnBossPhaseChanged(EBRBossPhase NewPhase) override;
	virtual FString GetBossDebugName() const override;
};
