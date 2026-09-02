#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRNelDialogueWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRNelDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void SetDialogue(const FText& InTitle, const FText& InText, bool bInHiddenHint = false);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidget();
	void RefreshText();

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> LineText;

	FText SavedTitle;
	FString FullText;
	float TypeTime = 0.0f;
	float TypeSpeed = 0.028f;
	int32 ShownCount = 0;
	bool bHiddenHint = false;
};
