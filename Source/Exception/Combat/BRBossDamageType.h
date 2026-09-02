#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "BRBossDamageType.generated.h"

/** Incoming boss damage contract. Generic damage is deliberately not parryable. */
UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRBossDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UBRBossDamageType();

	UFUNCTION(BlueprintPure, Category="Exception|Combat")
	bool CanBeParried() const { return bCanBeParried; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Combat")
	bool bCanBeParried = false;
};

/** Opt-in damage type used only by patterns explicitly authored as parryable. */
UCLASS()
class EXCEPTION_API UBRParryableBossDamageType : public UBRBossDamageType
{
	GENERATED_BODY()

public:
	UBRParryableBossDamageType();
};
