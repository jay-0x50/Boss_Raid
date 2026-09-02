#include "Combat/BRBossDamageType.h"

UBRBossDamageType::UBRBossDamageType()
{
	bCanBeParried = false;
}

UBRParryableBossDamageType::UBRParryableBossDamageType()
{
	bCanBeParried = true;
}
