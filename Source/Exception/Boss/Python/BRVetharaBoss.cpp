#include "Boss/Python/BRVetharaBoss.h"

ABRVetharaBoss::ABRVetharaBoss()
{
	bUseTeamSlotRole = false;
	PythonBossIdentity = EBRPythonBossIdentity::Vethara;
	VisualMeshType = EBRBossVisualMeshType::SkeletalMesh;
	TeamRole = EBRBossTeamRole::Ranged;
	ConfigurePythonPatterns();
}
