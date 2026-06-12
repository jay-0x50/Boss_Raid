#pragma once

#include "CoreMinimal.h"
#include "Boss/Pattern/BRPatternBossBase.h"
#include "BRSelvaraBoss.generated.h"

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Selvara Boss"))
class EXCEPTION_API ABRSelvaraBoss : public ABRPatternBossBase
{
	GENERATED_BODY()

public:
	ABRSelvaraBoss();

	UFUNCTION(BlueprintCallable, Category="Exception|Selvara")
	void ConfigureSelvaraPatterns();

protected:
	virtual void OnBossReset() override;
	virtual void OnBossPhaseChanged(EBRBossPhase NewPhase) override;
	virtual FString GetBossDebugName() const override;
};
