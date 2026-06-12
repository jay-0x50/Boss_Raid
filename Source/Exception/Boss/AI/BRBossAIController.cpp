#include "Boss/AI/BRBossAIController.h"

#include "Boss/Base/BRBossBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/BlackboardData.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ABRBossAIController::ABRBossAIController()
{
	BossBlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BossBlackboard"));
	BossBehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BossBehaviorTree"));
}

void ABRBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ABRBossBase* Boss = Cast<ABRBossBase>(InPawn))
	{
		SetBossAIEnabled(Boss->IsCombatAIEnabled());
	}
}

void ABRBossAIController::OnUnPossess()
{
	if (BossBehaviorTreeComponent)
	{
		BossBehaviorTreeComponent->StopTree(EBTStopMode::Safe);
	}

	bBehaviorTreeActive = false;
	Super::OnUnPossess();
}

void ABRBossAIController::SetBossAIEnabled(bool bEnabled)
{
	ABRBossBase* Boss = GetControlledBoss();
	if (!Boss)
	{
		return;
	}

	EnsureBlackboard();
	RefreshBossBlackboard();

	if (!bEnabled)
	{
		if (BossBehaviorTreeComponent)
		{
			BossBehaviorTreeComponent->StopTree(EBTStopMode::Safe);
		}
		bBehaviorTreeActive = false;
		return;
	}

	UBehaviorTree* BossBehaviorTree = Boss->GetBossBehaviorTree();
	if (BossBehaviorTree && Boss->ShouldRunBehaviorTree())
	{
		bBehaviorTreeActive = RunBehaviorTree(BossBehaviorTree);
	}
	else
	{
		bBehaviorTreeActive = false;
	}
}

void ABRBossAIController::RefreshBossBlackboard()
{
	if (!EnsureBlackboard())
	{
		return;
	}

	ABRBossBase* Boss = GetControlledBoss();
	if (!Boss || !BossBlackboardComponent)
	{
		return;
	}

	AActor* Target = Boss->GetCurrentTarget();
	if (!Target)
	{
		Target = UGameplayStatics::GetPlayerCharacter(Boss, 0);
	}

	const float DistanceToTarget = Target
		? FVector::Dist(Boss->GetActorLocation(), Target->GetActorLocation())
		: 0.0f;

	BossBlackboardComponent->SetValueAsObject(BRBossBlackboardKeys::SelfActor, Boss);
	BossBlackboardComponent->SetValueAsObject(BRBossBlackboardKeys::TargetActor, Target);
	BossBlackboardComponent->SetValueAsFloat(BRBossBlackboardKeys::DistanceToTarget, DistanceToTarget);
	BossBlackboardComponent->SetValueAsFloat(BRBossBlackboardKeys::HPPercent, Boss->GetHPPercent());
	BossBlackboardComponent->SetValueAsFloat(BRBossBlackboardKeys::GroggyPercent, Boss->GetGroggyPercent());
	BossBlackboardComponent->SetValueAsBool(BRBossBlackboardKeys::IsAIEnabled, Boss->IsCombatAIEnabled());
	BossBlackboardComponent->SetValueAsBool(BRBossBlackboardKeys::IsDead, Boss->IsDead());
	BossBlackboardComponent->SetValueAsBool(BRBossBlackboardKeys::IsGroggy, Boss->IsGroggy());
	BossBlackboardComponent->SetValueAsBool(BRBossBlackboardKeys::IsAttacking, Boss->IsAttacking());
	BossBlackboardComponent->SetValueAsBool(BRBossBlackboardKeys::IsPhase2, Boss->GetBossPhase() == EBRBossPhase::Phase2);
	BossBlackboardComponent->SetValueAsBool(BRBossBlackboardKeys::TeamMateAttacking, Boss->IsTeamMateAttacking());
}

bool ABRBossAIController::IsBehaviorTreeActive() const
{
	return bBehaviorTreeActive && BrainComponent && BrainComponent->IsRunning();
}

ABRBossBase* ABRBossAIController::GetControlledBoss() const
{
	return Cast<ABRBossBase>(GetPawn());
}

UBlackboardData* ABRBossAIController::GetOrCreateRuntimeBlackboardAsset()
{
	if (RuntimeBlackboardAsset)
	{
		return RuntimeBlackboardAsset;
	}

	RuntimeBlackboardAsset = NewObject<UBlackboardData>(this, TEXT("RuntimeBossBlackboard"));
	if (!RuntimeBlackboardAsset)
	{
		return nullptr;
	}

	UBlackboardKeyType_Object* SelfActorKey = RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Object>(BRBossBlackboardKeys::SelfActor);
	if (SelfActorKey)
	{
		SelfActorKey->BaseClass = ABRBossBase::StaticClass();
	}

	UBlackboardKeyType_Object* TargetActorKey = RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Object>(BRBossBlackboardKeys::TargetActor);
	if (TargetActorKey)
	{
		TargetActorKey->BaseClass = AActor::StaticClass();
	}

	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Float>(BRBossBlackboardKeys::DistanceToTarget);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Float>(BRBossBlackboardKeys::HPPercent);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Float>(BRBossBlackboardKeys::GroggyPercent);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Bool>(BRBossBlackboardKeys::IsAIEnabled);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Bool>(BRBossBlackboardKeys::IsDead);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Bool>(BRBossBlackboardKeys::IsGroggy);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Bool>(BRBossBlackboardKeys::IsAttacking);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Bool>(BRBossBlackboardKeys::IsPhase2);
	RuntimeBlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Bool>(BRBossBlackboardKeys::TeamMateAttacking);

	return RuntimeBlackboardAsset;
}

bool ABRBossAIController::EnsureBlackboard()
{
	ABRBossBase* Boss = GetControlledBoss();
	if (!Boss)
	{
		return false;
	}

	UBlackboardData* BlackboardAsset = nullptr;
	if (const UBehaviorTree* BossBehaviorTree = Boss->GetBossBehaviorTree())
	{
		BlackboardAsset = BossBehaviorTree->BlackboardAsset;
	}

	if (!BlackboardAsset)
	{
		BlackboardAsset = GetOrCreateRuntimeBlackboardAsset();
	}

	if (!BlackboardAsset)
	{
		return false;
	}

	UBlackboardComponent* BlackboardComponent = BossBlackboardComponent;
	const bool bBlackboardReady = UseBlackboard(BlackboardAsset, BlackboardComponent);
	BossBlackboardComponent = BlackboardComponent;
	return bBlackboardReady;
}
