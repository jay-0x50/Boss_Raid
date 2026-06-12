#pragma once

#include "CoreMinimal.h"
#include "Boss/Python/BRPythonBoss.h"
#include "BRAurathosBoss.generated.h"

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Aurathos, Fatal Process"))
class EXCEPTION_API ABRAurathosBoss : public ABRPythonBoss
{
	GENERATED_BODY()

public:
	ABRAurathosBoss();
};
