#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRStoryIntroDirector.generated.h"

class ACameraActor;
class ABRNelCompanion;
class APlayerController;
class APawn;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Story Intro Director"))
class EXCEPTION_API ABRStoryIntroDirector : public AActor
{
	GENERATED_BODY()

public:
	ABRStoryIntroDirector();

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void PlayIntro();

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void SkipIntro();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story")
	bool bPlayOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story", meta=(ClampMin="0.0", Units="s"))
	float StartDelay = 0.45f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Story")
	TArray<TObjectPtr<ACameraActor>> ShotCameras;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Story|Nel")
	TObjectPtr<ABRNelCompanion> OpeningNelCompanion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story")
	TArray<float> ShotTimes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story", meta=(MultiLine="true"))
	FText OpeningLog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story", meta=(MultiLine="true"))
	FText OpeningNelLine;

private:
	void ShowNextShot();
	void FinishIntro();
	void RestorePlayer();

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> CachedPC;

	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPawn;

	FTimerHandle IntroTimer;
	int32 ShotIndex = 0;
	bool bDidPlay = false;
};
