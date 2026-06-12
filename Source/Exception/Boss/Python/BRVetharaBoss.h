#pragma once

#include "CoreMinimal.h"
#include "Boss/Python/BRPythonBoss.h"
#include "BRVetharaBoss.generated.h"

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Vethara, Unhandled Exception"))
class EXCEPTION_API ABRVetharaBoss : public ABRPythonBoss
{
	GENERATED_BODY()

public:
	ABRVetharaBoss();
};
