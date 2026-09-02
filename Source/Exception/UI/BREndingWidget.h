#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BREndingWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBREndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void SetEnding(const FText& InTitle, const FText& InText, bool bInHiddenEnding, float InShowTime);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidget();
	void RefreshEnding();

	UPROPERTY()
	TObjectPtr<UBorder> Backdrop;

	UPROPERTY()
	TObjectPtr<UTextBlock> ChapterText;

	UPROPERTY()
	TObjectPtr<UTextBlock> EndingTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> EndingBody;

	UPROPERTY()
	TObjectPtr<UTextBlock> FooterText;

	FText SavedTitle;
	FText SavedText;
	float LifeTime = 0.0f;
	float ShowTime = 10.0f;
	bool bHiddenEnding = false;
};
