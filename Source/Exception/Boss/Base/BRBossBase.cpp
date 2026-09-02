#include "Boss/Base/BRBossBase.h"

#include "Boss/AI/BRBossAIController.h"
#include "Boss/Feedback/BRBossHitCameraShake.h"
#include "Boss/Team/BRBossTeamCoordinator.h"
#include "BRStatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

ABRBossBase::ABRBossBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ABRBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	HitCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCapsule"));
	SetRootComponent(HitCapsule);
	HitCapsule->InitCapsuleSize(90.0f, 140.0f);
	HitCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitCapsule->SetCollisionObjectType(ECC_Pawn);
	HitCapsule->SetCollisionResponseToAllChannels(ECR_Block);
	HitCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	HitCapsule->SetGenerateOverlapEvents(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetupAttachment(HitCapsule);

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
	CombatHitCameraShakeClass = UBRBossHitCameraShake::StaticClass();
}

void ABRBossBase::BeginPlay()
{
	Super::BeginPlay();
	InitialBossTransform = GetActorTransform();
	if (VisualRoot)
	{
		// Blueprint component transforms are finalized by BeginPlay. Keep the
		// authored per-boss forward-axis correction as the procedural baseline.
		VisualRootBaseRelativeTransform = VisualRoot->GetRelativeTransform();
		bVisualRootBaseTransformCaptured = true;
	}

	if (!GetController())
	{
		SpawnDefaultController();
	}

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
	if (!bCombatAIEnabled)
	{
		PrepareForArenaInactive();
	}
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
	ApplyMeshVisualTransform();
}

void ABRBossBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyGroundGravity(DeltaSeconds);

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

	UpdateProceduralIdleMotion(DeltaSeconds);
	UpdateProceduralHitReaction(DeltaSeconds);
	DrawBossDebug();
}

void ABRBossBase::ApplyMeshVisualTransform()
{
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
		SetBossAnimationPlaying(bUseSkeletalMesh && bCombatAIEnabled && !bIsDead);
	}
}

void ABRBossBase::UpdateProceduralIdleMotion(float DeltaSeconds)
{
	if (!VisualRoot)
	{
		return;
	}

	const bool bHasStaticMesh = MeshComponent && MeshComponent->GetStaticMesh();
	const bool bHasSkeletalMesh = SkeletalMeshComponent && SkeletalMeshComponent->GetSkeletalMeshAsset();
	const bool bUseSkeletalMesh = bHasSkeletalMesh && (VisualMeshType == EBRBossVisualMeshType::SkeletalMesh || !bHasStaticMesh);
	const bool bUseStaticMesh = bHasStaticMesh && !bUseSkeletalMesh;
	const TObjectPtr<UAnimationAsset>* StageAnimation = StageAnimations.Find(CurrentAnimationStage);
	const bool bHasStageAnimation = StageAnimation && StageAnimation->Get();
	const bool bHasAnimationBlueprint = bUseSkeletalMesh
		&& SkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationBlueprint
		&& SkeletalMeshComponent->GetAnimInstance();
	const bool bNeedsFallback = bUseStaticMesh || (bUseSkeletalMesh && !bHasStageAnimation && !bHasAnimationBlueprint);
	const bool bIsSafeFallbackStage = CurrentAnimationStage == EBRBossAnimationStage::Idle
		|| CurrentAnimationStage == EBRBossAnimationStage::Move
		|| CurrentAnimationStage == EBRBossAnimationStage::PatternRecovery;
	const bool bCanUseFallback = bUseProceduralIdleFallback
		&& bNeedsFallback
		&& bIsSafeFallbackStage
		&& bCombatAIEnabled
		&& !bIsAttacking
		&& !bIsGroggy
		&& !bIsBeingExecuted
		&& !bIsDead;

	if (!bVisualRootBaseTransformCaptured)
	{
		VisualRootBaseRelativeTransform = VisualRoot->GetRelativeTransform();
		bVisualRootBaseTransformCaptured = true;
	}

	const FVector BaseLocation = VisualRootBaseRelativeTransform.GetLocation();
	const FQuat BaseRotation = VisualRootBaseRelativeTransform.GetRotation();
	if (!bCanUseFallback)
	{
		ProceduralIdleBlendAlpha = 0.0f;
		VisualRoot->SetRelativeLocationAndRotation(BaseLocation, BaseRotation.Rotator());
		VisualRoot->SetRelativeScale3D(VisualRootBaseRelativeTransform.GetScale3D());
		return;
	}

	ProceduralIdleTime += FMath::Max(DeltaSeconds, 0.0f);
	ProceduralIdleBlendAlpha = FMath::FInterpTo(ProceduralIdleBlendAlpha, 1.0f, DeltaSeconds, 4.0f);
	const float MotionPhase = ProceduralIdleTime * ProceduralIdleFrequency * 2.0f * PI;
	const float BobOffset = FMath::Sin(MotionPhase) * ProceduralIdleBobAmplitude * ProceduralIdleBlendAlpha;
	const float LeanAngle = FMath::Sin(MotionPhase * 0.5f) * ProceduralIdleLeanAngle * ProceduralIdleBlendAlpha;

	const FVector LocalBob = BaseRotation.RotateVector(FVector(0.0f, 0.0f, BobOffset));
	const FQuat LocalLean = FRotator(LeanAngle, 0.0f, 0.0f).Quaternion();
	VisualRoot->SetRelativeLocationAndRotation(BaseLocation + LocalBob, (BaseRotation * LocalLean).Rotator());
	VisualRoot->SetRelativeScale3D(VisualRootBaseRelativeTransform.GetScale3D());
}

void ABRBossBase::SetBossAnimationPlaying(bool bShouldPlay)
{
	if (!SkeletalMeshComponent || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		return;
	}

	if (bShouldPlay)
	{
		SkeletalMeshComponent->Play(true);
		return;
	}

	SkeletalMeshComponent->Stop();
	SkeletalMeshComponent->SetPosition(0.0f, false);
}

void ABRBossBase::PlayBossStageAnimation(EBRBossAnimationStage Stage, FName ActionName)
{
	if (!SkeletalMeshComponent || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		return;
	}

	UAnimationAsset* AnimationToPlay = nullptr;
	if (!ActionName.IsNone())
	{
		if (const TObjectPtr<UAnimationAsset>* FoundAnimation = ActionAnimations.Find(ActionName))
		{
			AnimationToPlay = FoundAnimation->Get();
		}
	}

	if (!AnimationToPlay)
	{
		if (const TObjectPtr<UAnimationAsset>* FoundAnimation = StageAnimations.Find(Stage))
		{
			AnimationToPlay = FoundAnimation->Get();
		}
	}

	// Most existing boss Blueprints already have an intro animation. Reuse it
	// for the phase break until a dedicated transition animation is assigned.
	if (!AnimationToPlay && Stage == EBRBossAnimationStage::PhaseTransition)
	{
		if (const TObjectPtr<UAnimationAsset>* FoundAnimation = StageAnimations.Find(EBRBossAnimationStage::Intro))
		{
			AnimationToPlay = FoundAnimation->Get();
		}
	}

	if (!AnimationToPlay)
	{
		return;
	}

	const bool bLoopAnimation = Stage == EBRBossAnimationStage::Idle || Stage == EBRBossAnimationStage::Move || Stage == EBRBossAnimationStage::Groggy;
	if (CurrentBossAnimationAsset == AnimationToPlay && bLoopAnimation)
	{
		return;
	}

	SkeletalMeshComponent->SetAnimation(AnimationToPlay);
	SkeletalMeshComponent->Play(bLoopAnimation);
	CurrentBossAnimationAsset = AnimationToPlay;
}

void ABRBossBase::NotifyBossAnimationStage(EBRBossAnimationStage Stage, FName ActionName)
{
	CurrentAnimationStage = Stage;
	PlayBossStageAnimation(Stage, ActionName);
	OnAnimationStageChanged.Broadcast(Stage, ActionName);
	BP_BossAnimationStageChanged(Stage, ActionName);
}

void ABRBossBase::ApplyGroundGravity(float DeltaSeconds)
{
	if (!bUseGroundGravity || DeltaSeconds <= 0.0f || !GetWorld())
	{
		return;
	}

	const FVector ActorLocation = GetActorLocation();
	const FVector TraceStart = ActorLocation;
	const FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, GroundTraceActorHalfHeight + GroundTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossGroundGravity), false, this);
	FHitResult Hit;
	const bool bHitGround = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	if (bHitGround)
	{
		const float DesiredActorZ = Hit.Location.Z + GroundTraceActorHalfHeight;
		const float HeightDelta = ActorLocation.Z - DesiredActorZ;

		if (HeightDelta > GroundSnapTolerance)
		{
			VerticalFallSpeed = FMath::Max(VerticalFallSpeed + GroundGravity * DeltaSeconds, 0.0f);
			const float FallDistance = FMath::Min(VerticalFallSpeed * DeltaSeconds, HeightDelta);
			AddActorWorldOffset(FVector(0.0f, 0.0f, -FallDistance), true);
			return;
		}

		if (FMath::Abs(HeightDelta) > KINDA_SMALL_NUMBER)
		{
			SetActorLocation(FVector(ActorLocation.X, ActorLocation.Y, DesiredActorZ), true);
		}

		VerticalFallSpeed = 0.0f;
		return;
	}

	VerticalFallSpeed = FMath::Max(VerticalFallSpeed + GroundGravity * DeltaSeconds, 0.0f);
	AddActorWorldOffset(FVector(0.0f, 0.0f, -VerticalFallSpeed * DeltaSeconds), true);
}

void ABRBossBase::ClearBaseTimers()
{
	GetWorldTimerManager().ClearTimer(GroggyTimerHandle);
	GetWorldTimerManager().ClearTimer(PhaseTransitionTimerHandle);
}

void ABRBossBase::StartProceduralHitReaction(AActor* DamageCauser)
{
	ProceduralHitReactionTime = FMath::Max(ProceduralHitReactionDuration, KINDA_SMALL_NUMBER);

	const FVector AwayFromDamage = DamageCauser
		? FVector(GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D()
		: -GetActorForwardVector();
	const FVector SafeWorldDirection = AwayFromDamage.IsNearlyZero() ? -GetActorForwardVector() : AwayFromDamage;
	ProceduralHitReactionDirection = GetActorTransform().InverseTransformVectorNoScale(SafeWorldDirection).GetSafeNormal();
}

void ABRBossBase::UpdateProceduralHitReaction(float DeltaSeconds)
{
	if (!VisualRoot || ProceduralHitReactionTime <= 0.0f || ProceduralHitReactionDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	ProceduralHitReactionTime = FMath::Max(ProceduralHitReactionTime - FMath::Max(DeltaSeconds, 0.0f), 0.0f);
	const float ReactionAlpha = ProceduralHitReactionTime / ProceduralHitReactionDuration;
	const float KickAlpha = FMath::Sin(ReactionAlpha * PI * 0.5f);
	VisualRoot->AddLocalOffset(ProceduralHitReactionDirection * ProceduralHitReactionDistance * KickAlpha);
	VisualRoot->AddLocalRotation(FRotator(-2.5f * KickAlpha, 0.0f, ProceduralHitReactionDirection.Y * 2.0f * KickAlpha));
}

void ABRBossBase::PlayCameraFeedbackForActor(AActor* FeedbackActor, float ShakeScale, float RumbleIntensity) const
{
	const APawn* FeedbackPawn = Cast<APawn>(FeedbackActor);
	APlayerController* PlayerController = FeedbackPawn ? Cast<APlayerController>(FeedbackPawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (CombatHitCameraShakeClass && ShakeScale > KINDA_SMALL_NUMBER)
	{
		PlayerController->ClientStartCameraShake(CombatHitCameraShakeClass, ShakeScale);
	}

	if (RumbleIntensity > KINDA_SMALL_NUMBER)
	{
		PlayerController->PlayDynamicForceFeedback(
			FMath::Clamp(RumbleIntensity, 0.0f, 1.0f),
			0.12f,
			true,
			true,
			true,
			true,
			EDynamicForceFeedbackAction::Start);
	}
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
