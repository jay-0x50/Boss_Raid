#pragma once

#include "CoreMinimal.h"
#include "Boss/Pattern/BRPatternBossBase.h"
#include "BRPythonBoss.generated.h"

UENUM(BlueprintType)
enum class EBRPythonBossIdentity : uint8
{
	Vethara,
	Aurathos
};

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Python Twin Boss"))
class EXCEPTION_API ABRPythonBoss : public ABRPatternBossBase
{
	GENERATED_BODY()

public:
	ABRPythonBoss();

	virtual void ApplyTeamSlot(int32 TeamSlotIndex) override;

	UFUNCTION(BlueprintCallable, Category="Exception|Python")
	void ConfigurePythonPatterns();

protected:
	virtual void OnBossReset() override;
	virtual void OnBossPhaseChanged(EBRBossPhase NewPhase) override;
	virtual FString GetBossDebugName() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Python")
	bool bUseTeamSlotRole = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Python")
	EBRPythonBossIdentity PythonBossIdentity = EBRPythonBossIdentity::Vethara;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Python", meta=(ClampMin="0.0"))
	float VetharaDamageMultiplier = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Python", meta=(ClampMin="0.0"))
	float AurathosDamageMultiplier = 1.15f;
};
