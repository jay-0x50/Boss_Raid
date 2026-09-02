#include "BRStoryPathGate.h"

#include "BRHiddenStorySubsystem.h"
#include "BRNarrativeQueueSubsystem.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ABRStoryPathGate::ABRStoryPathGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ABRStoryPathGate::BeginPlay()
{
	Super::BeginPlay();
	CacheGatePieceTransforms();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			Story->OnMainBossProgressChanged.AddDynamic(this, &ABRStoryPathGate::HandleMainBossProgressChanged);
		}
	}

	RefreshGateState(true);
}

void ABRStoryPathGate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			Story->OnMainBossProgressChanged.RemoveDynamic(this, &ABRStoryPathGate::HandleMainBossProgressChanged);
		}
	}
	GetWorldTimerManager().ClearTimer(RevealTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ABRStoryPathGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bOpening)
	{
		SetActorTickEnabled(false);
		return;
	}

	OpenElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(OpenElapsed / FMath::Max(OpenDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float MoveAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.2f);

	for (int32 Index = 0; Index < GatePieces.Num(); ++Index)
	{
		AActor* Piece = GatePieces[Index];
		if (!Piece || !GatePieceStartTransforms.IsValidIndex(Index))
		{
			continue;
		}

		const FTransform& Start = GatePieceStartTransforms[Index];
		const float SideSign = static_cast<float>(Index - (GatePieces.Num() / 2));
		const FVector TargetLocation = Start.GetLocation()
			- FVector::UpVector * SinkDistance * MoveAlpha
			+ GetActorRightVector() * SideSign * 160.0f * MoveAlpha;
		FRotator TargetRotation = Start.Rotator();
		TargetRotation.Pitch += (Index % 2 == 0 ? -12.0f : 8.0f) * MoveAlpha;
		TargetRotation.Roll += SideSign * 16.0f * MoveAlpha;
		Piece->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (!bCollisionReleased && Alpha >= CollisionReleaseAlpha)
	{
		bCollisionReleased = true;
		SetGateCollision(false);
	}

	if (Alpha >= 1.0f)
	{
		bOpening = false;
		bGateOpen = true;
		for (AActor* Piece : GatePieces)
		{
			if (Piece)
			{
				Piece->SetActorHiddenInGame(true);
			}
		}
		SetActorTickEnabled(false);
	}
}

void ABRStoryPathGate::OpenGate(bool bPlayReveal)
{
	if (bGateOpen || bOpening)
	{
		return;
	}

	bOpening = true;
	bCollisionReleased = false;
	OpenElapsed = 0.0f;
	SetActorTickEnabled(true);

	if (!GateOpenLine.IsEmpty())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UBRNarrativeQueueSubsystem* StoryQueue = GI->GetSubsystem<UBRNarrativeQueueSubsystem>())
			{
				StoryQueue->ShowNelLine(GateOpenLine, false, FMath::Max(3.5f, OpenDuration + 0.8f));
			}
		}
	}

	if (bPlayReveal && bPlayRevealOnUnlock)
	{
		StartRevealCamera();
	}
}

void ABRStoryPathGate::RefreshGateState(bool bInstant)
{
	const UGameInstance* GI = GetGameInstance();
	const UBRHiddenStorySubsystem* Story = GI ? GI->GetSubsystem<UBRHiddenStorySubsystem>() : nullptr;
	const bool bShouldBeOpen = RequiredBossId.IsNone() || (Story && Story->IsMainBossDefeated(RequiredBossId));

	if (bShouldBeOpen)
	{
		if (bInstant)
		{
			SetGateOpenInstant();
		}
		else
		{
			OpenGate(true);
		}
	}
	else if (bInstant)
	{
		SetGateClosedInstant();
	}
}

void ABRStoryPathGate::HandleMainBossProgressChanged(FName BossId, int32 DefeatedCount)
{
	if (BossId.IsNone())
	{
		RefreshGateState(true);
		return;
	}

	if (BossId == RequiredBossId)
	{
		OpenGate(true);
	}
}

void ABRStoryPathGate::CacheGatePieceTransforms()
{
	GatePieceStartTransforms.SetNum(GatePieces.Num());
	for (int32 Index = 0; Index < GatePieces.Num(); ++Index)
	{
		if (const AActor* Piece = GatePieces[Index])
		{
			GatePieceStartTransforms[Index] = Piece->GetActorTransform();
		}
	}
}

void ABRStoryPathGate::SetGateOpenInstant()
{
	bOpening = false;
	bGateOpen = true;
	bCollisionReleased = true;
	SetActorTickEnabled(false);
	SetGateCollision(false);
	for (AActor* Piece : GatePieces)
	{
		if (Piece)
		{
			Piece->SetActorHiddenInGame(true);
		}
	}
}

void ABRStoryPathGate::SetGateClosedInstant()
{
	bOpening = false;
	bGateOpen = false;
	bCollisionReleased = false;
	OpenElapsed = 0.0f;
	SetActorTickEnabled(false);
	for (int32 Index = 0; Index < GatePieces.Num(); ++Index)
	{
		AActor* Piece = GatePieces[Index];
		if (!Piece)
		{
			continue;
		}
		if (GatePieceStartTransforms.IsValidIndex(Index))
		{
			Piece->SetActorTransform(GatePieceStartTransforms[Index], false, nullptr, ETeleportType::TeleportPhysics);
		}
		Piece->SetActorHiddenInGame(false);
	}
	SetGateCollision(true);
}

void ABRStoryPathGate::SetGateCollision(bool bEnabled)
{
	for (AActor* Piece : GatePieces)
	{
		if (Piece)
		{
			Piece->SetActorEnableCollision(bEnabled);
		}
	}
}

void ABRStoryPathGate::StartRevealCamera()
{
	if (!RevealCamera)
	{
		return;
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PreviousViewTarget = PC->GetViewTarget();
		PC->SetViewTargetWithBlend(RevealCamera, 0.4f);
		GetWorldTimerManager().SetTimer(RevealTimerHandle, this, &ABRStoryPathGate::RestorePlayerCamera, OpenDuration, false);
	}
}

void ABRStoryPathGate::RestorePlayerCamera()
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		AActor* Target = PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : PC->GetPawn();
		if (Target)
		{
			PC->SetViewTargetWithBlend(Target, 0.5f);
		}
	}
	PreviousViewTarget.Reset();
}
