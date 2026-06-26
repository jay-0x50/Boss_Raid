#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRPauseMenuWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Menu")
	void RefreshMenu();

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleLevelVitalityClicked();

	UFUNCTION()
	void HandleLevelEnduranceClicked();

	UFUNCTION()
	void HandleLevelPowerClicked();

	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleInventoryClicked();

	UFUNCTION()
	void HandleTitleClicked();

	UFUNCTION()
	void HandleQuitClicked();

	void BuildMenuWidget();
	class UButton* AddMenuButton(class UVerticalBox* ParentBox, const FText& Label);

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PointsText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UTextBlock> VitalityText;

	UPROPERTY()
	TObjectPtr<UTextBlock> EnduranceText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PowerText;
};
