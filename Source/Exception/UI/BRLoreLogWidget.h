#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRLoreLogWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRLoreLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	void SetLog(const FText& InTitle, const FText& InText, bool bBossMessage = false);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidget();
	void RefreshText();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> BodyText;

	FText SavedTitle;
	FString FullText;
	float TypeTime = 0.0f;
	float TypeSpeed = 0.022f;
	int32 ShownCount = 0;
	bool bBossStyle = false;
};
