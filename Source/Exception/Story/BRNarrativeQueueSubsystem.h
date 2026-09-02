#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BRNarrativeQueueSubsystem.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EBRNarrativeType : uint8
{
	SystemLog,
	Nel,
	Boss,
	Ending
};

USTRUCT(BlueprintType)
struct FBRNarrativeMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Story")
	EBRNarrativeType Type = EBRNarrativeType::SystemLog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Story")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Story", meta=(MultiLine="true"))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Story", meta=(ClampMin="0.5", Units="s"))
	float ShowTime = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Story")
	bool bHiddenHint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Story")
	bool bDeferDuringCombat = false;
};

UCLASS()
class EXCEPTION_API UBRNarrativeQueueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void AddMessage(const FBRNarrativeMessage& Message);

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void AddDeferredMessage(const FBRNarrativeMessage& Message);

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ShowSystemLog(const FText& Text, float ShowTime = 4.0f, const FText& Title = FText());

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ShowNelLine(const FText& Text, bool bHiddenHint = false, float ShowTime = 4.5f);

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ShowSystemLogDeferred(const FText& Text, float ShowTime = 4.0f, const FText& Title = FText());

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ShowNelLineDeferred(const FText& Text, bool bHiddenHint = false, float ShowTime = 4.5f);

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ShowBossLine(const FText& BossTitle, const FText& Text, float ShowTime = 4.5f);

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ShowEnding(const FText& Title, const FText& Text, bool bHiddenEnding, float ShowTime = 10.0f);

	UFUNCTION(BlueprintPure, Category="Exception|Story")
	bool IsShowingMessage() const { return ActiveWidget != nullptr; }

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void ClearMessages();

private:
	void TryShowNext();
	void FinishMessage();
	void ScheduleCombatRetry();
	bool IsCombatActive() const;
	APlayerController* GetLocalPC() const;

	UPROPERTY(Transient)
	TArray<FBRNarrativeMessage> MessageQueue;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveWidget;

	FTimerHandle MessageTimer;
	FTimerHandle CombatRetryTimer;
};
