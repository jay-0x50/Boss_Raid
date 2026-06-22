#include "Boss/Base/BRBossBase.h"

#include "Boss/AI/BRBossAIController.h"
#include "Boss/Team/BRBossTeamCoordinator.h"
#include "BRStatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

ABRBossBase::ABRBossBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ABRBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	BossCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BossCollision"));
	SetRootComponent(BossCollision);
	BossCollision->InitCapsuleSize(BossCollisionRadius, BossCollisionHalfHeight);
	BossCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BossCollision->SetCollisionObjectType(ECC_Pawn);
	BossCollision->SetCollisionResponseToAllChannels(ECR_Block);
	BossCollision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BossCollision->SetGenerateOverlapEvents(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetupAttachment(BossCollision);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(VisualRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionObjectType(ECC_Pawn);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetVisibility(false);
	MeshComponent->SetHiddenInGame(true);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(VisualRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetCollisionObjectType(ECC_Pawn);
	SkeletalMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	SkeletalMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SkeletalMeshComponent->SetGenerateOverlapEvents(false);
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetHiddenInGame(true);

	StatComponent = CreateDefaultSubobject<UBRStatComponent>(TEXT("StatComponent"));
}

void ABRBossBase::BeginPlay()
{
	Super::BeginPlay();

	if (!GetController())
	{
		SpawnDefaultController();
	}

	ApplyBossCollisionSettings();
	ApplyMeshVisualTransform();

	if (StatComponent)
	{
		StatComponent->ConfigureMaxStats(InitialMaxHP, 0.0f, InitialMaxGroggy);
		StatComponent->OnHPChanged.AddDynamic(this, &ABRBossBase::HandleHPChanged);
		StatComponent->OnGroggyChanged.AddDynamic(this, &ABRBossBase::HandleGroggyChanged);
		StatComponent->OnDead.AddDynamic(this, &ABRBossBase::HandleDead);
		StatComponent->OnGroggy.AddDynamic(this, &ABRBossBase::HandleGroggy);
	}

	if (TeamCoordinator)
	{
		TeamCoordinator->RegisterBoss(this);
	}

	ResetBoss();
}

void ABRBossBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TeamCoordinator)
	{
		TeamCoordinator->UnregisterBoss(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ABRBossBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyBossCollisionSettings();
	ApplyMeshVisualTransform();
}

void ABRBossBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->RefreshBossBlackboard();
		if (!BossAIController->IsBehaviorTreeActive())
		{
			UpdateBossAI(DeltaSeconds);
		}
	}
	else
	{
		UpdateBossAI(DeltaSeconds);
	}

	DrawBossDebug();
}

void ABRBossBase::ApplyMeshVisualTransform()
{
	if (VisualRoot)
	{
		VisualRoot->SetRelativeLocation(FVector::ZeroVector);
		VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
		VisualRoot->SetRelativeScale3D(FVector::OneVector);
	}

	const bool bHasStaticMesh = MeshComponent && MeshComponent->GetStaticMesh();
	const bool bHasSkeletalMesh = SkeletalMeshComponent && SkeletalMeshComponent->GetSkeletalMeshAsset();
	const bool bUseSkeletalMesh = bHasSkeletalMesh && (VisualMeshType == EBRBossVisualMeshType::SkeletalMesh || !bHasStaticMesh);
	const bool bUseStaticMesh = bHasStaticMesh && !bUseSkeletalMesh;

	if (MeshComponent)
	{
		MeshComponent->SetRelativeLocation(MeshRelativeLocation);
		MeshComponent->SetRelativeRotation(MeshRelativeRotation);
		MeshComponent->SetRelativeScale3D(MeshRelativeScale);
		MeshComponent->SetVisibility(bUseStaticMesh);
		MeshComponent->SetHiddenInGame(!bUseStaticMesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetRelativeLocation(MeshRelativeLocation);
		SkeletalMeshComponent->SetRelativeRotation(MeshRelativeRotation);
		SkeletalMeshComponent->SetRelativeScale3D(MeshRelativeScale);
		SkeletalMeshComponent->SetVisibility(bUseSkeletalMesh);
		SkeletalMeshComponent->SetHiddenInGame(!bUseSkeletalMesh);
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMeshComponent->SetGenerateOverlapEvents(false);
		if (bUseSkeletalMesh && SkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationSingleNode)
		{
			SkeletalMeshComponent->Play(true);
		}
	}
}

void ABRBossBase::ApplyBossCollisionSettings()
{
	if (!BossCollision)
	{
		return;
	}

	BossCollision->SetCapsuleSize(BossCollisionRadius, BossCollisionHalfHeight, true);
	BossCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BossCollision->SetCollisionObjectType(ECC_Pawn);
	BossCollision->SetCollisionResponseToAllChannels(ECR_Block);
	BossCollision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BossCollision->SetGenerateOverlapEvents(false);
}

void ABRBossBase::ClearBaseTimers()
{
	GetWorldTimerManager().ClearTimer(GroggyTimerHandle);
}

void ABRBossBase::OnBossReset()
{
}

void ABRBossBase::OnBossDeadInternal()
{
}

void ABRBossBase::OnBossGroggyInternal()
{
}

void ABRBossBase::OnBossRecoveredFromGroggyInternal()
{
}

void ABRBossBase::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
}

void ABRBossBase::UpdateBossAI(float DeltaSeconds)
{
}

void ABRBossBase::DrawBossDebug() const
{
}

FString ABRBossBase::GetBossDebugName() const
{
	return GetName();
}
