#include "BRStoryIntroDirector.h"

#include "BRNarrativeQueueSubsystem.h"
#include "Story/BRNelCompanion.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABRStoryIntroDirector::ABRStoryIntroDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	OpeningLog = FText::FromString(TEXT("> SPAWN Hendel.exe\n> TASK: Terminate all unhandled exceptions.\n> RUN"));
	OpeningNelLine = FText::FromString(TEXT("깨어났네. 기억이 없어도 괜찮아. 우선 저 빛이 새어드는 곳으로 나가."));
}

void ABRStoryIntroDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bPlayOnStart)
	{
		GetWorldTimerManager().SetTimer(IntroTimer, this, &ABRStoryIntroDirector::PlayIntro, StartDelay, false);
	}
}

void ABRStoryIntroDirector::PlayIntro()
{
	if (bDidPlay || !GetWorld())
	{
		return;
	}

	CachedPC = UGameplayStatics::GetPlayerController(this, 0);
	CachedPawn = CachedPC ? CachedPC->GetPawn() : nullptr;
	if (!CachedPC || !CachedPawn)
	{
		GetWorldTimerManager().SetTimer(IntroTimer, this, &ABRStoryIntroDirector::PlayIntro, 0.25f, false);
		return;
	}

	bDidPlay = true;
	ShotIndex = 0;
	CachedPC->SetCinematicMode(true, false, true, true, true);

	if (CachedPC->PlayerCameraManager)
	{
		CachedPC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.15f, FLinearColor::Black, false, true);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRNarrativeQueueSubsystem* StoryQueue = GI->GetSubsystem<UBRNarrativeQueueSubsystem>())
		{
			StoryQueue->ShowSystemLog(OpeningLog, 4.0f, FText::FromString(TEXT("BOOT SEQUENCE // UNKNOWN LAYER")));
			StoryQueue->ShowNelLine(OpeningNelLine, false, 4.6f);
		}
	}

	if (OpeningNelCompanion)
	{
		OpeningNelCompanion->Appear(8.5f);
	}

	ShowNextShot();
}

void ABRStoryIntroDirector::SkipIntro()
{
	if (!bDidPlay)
	{
		bDidPlay = true;
	}
	FinishIntro();
}

void ABRStoryIntroDirector::ShowNextShot()
{
	if (!CachedPC || ShotIndex >= ShotCameras.Num())
	{
		FinishIntro();
		return;
	}

	ACameraActor* ShotCamera = ShotCameras[ShotIndex];
	if (ShotCamera)
	{
		CachedPC->SetViewTargetWithBlend(ShotCamera, ShotIndex == 0 ? 0.0f : 0.7f, VTBlend_Cubic);
	}

	const float ShotTime = ShotTimes.IsValidIndex(ShotIndex) ? ShotTimes[ShotIndex] : 2.8f;
	++ShotIndex;
	GetWorldTimerManager().SetTimer(IntroTimer, this, &ABRStoryIntroDirector::ShowNextShot, FMath::Max(0.4f, ShotTime), false);
}

void ABRStoryIntroDirector::FinishIntro()
{
	GetWorldTimerManager().ClearTimer(IntroTimer);
	if (!CachedPC || !CachedPawn)
	{
		return;
	}

	CachedPC->SetViewTargetWithBlend(CachedPawn, 1.0f, VTBlend_Cubic);
	GetWorldTimerManager().SetTimer(IntroTimer, this, &ABRStoryIntroDirector::RestorePlayer, 1.0f, false);
}

void ABRStoryIntroDirector::RestorePlayer()
{
	if (CachedPC)
	{
		CachedPC->SetCinematicMode(false, false, true, true, true);
	}
}
