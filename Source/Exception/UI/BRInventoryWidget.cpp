#include "BRInventoryWidget.h"

#include "BRInventoryComponent.h"
#include "BRInventorySlotWidget.h"
#include "Player/Controller/ExceptionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor PanelColor(0.035f, 0.038f, 0.042f, 0.96f);
	const FLinearColor PanelSoftColor(0.075f, 0.071f, 0.065f, 0.9f);
	const FLinearColor ActiveTabColor(0.72f, 0.62f, 0.38f, 0.95f);
	const FLinearColor InactiveTabColor(0.13f, 0.13f, 0.13f, 0.88f);
	const FLinearColor TextMainColor(0.92f, 0.88f, 0.78f, 1.0f);
	const FLinearColor TextMutedColor(0.62f, 0.60f, 0.55f, 1.0f);
}

void UBRInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildInventoryWidget();
	BindDesignerWidgets();
	RefreshInventory();
}

void UBRInventoryWidget::SetInventoryComponent(UBRInventoryComponent* NewInventoryComponent)
{
	InventoryComponent = NewInventoryComponent;
	RefreshInventory();
}

void UBRInventoryWidget::SetInventorySlots(const TArray<FBRInventorySlot>& Slots)
{
	CachedSlots = Slots;
	RefreshSlotList();
}

void UBRInventoryWidget::SetInventorySlot(int32 SlotIndex, const FBRInventorySlot& InventorySlot)
{
	if (SlotIndex < 0)
	{
		return;
	}

	if (CachedSlots.Num() <= SlotIndex)
	{
		CachedSlots.SetNum(SlotIndex + 1);
	}

	CachedSlots[SlotIndex] = InventorySlot;
	RefreshSlotList();
}

void UBRInventoryWidget::RefreshInventory()
{
	if (!InventoryComponent)
	{
		CachedSlots.Reset();
		RefreshSlotList();
		return;
	}

	SetInventorySlots(InventoryComponent->GetSlots());
}

void UBRInventoryWidget::BindDesignerWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindButton = [this](const TCHAR* Name)
	{
		return Cast<UButton>(WidgetTree->FindWidget(FName(Name)));
	};
	auto FindText = [this](const TCHAR* Name)
	{
		return Cast<UTextBlock>(WidgetTree->FindWidget(FName(Name)));
	};

	if (UButton* CloseButton = FindButton(TEXT("CloseButton")))
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleCloseClicked);
	}

	const TCHAR* ButtonNames[] =
	{
		TEXT("AllTabButton"),
		TEXT("EquipmentTabButton"),
		TEXT("ConsumableTabButton"),
		TEXT("KeyItemTabButton"),
		TEXT("QuestItemTabButton")
	};
	const TCHAR* TextNames[] =
	{
		TEXT("AllTabText"),
		TEXT("EquipmentTabText"),
		TEXT("ConsumableTabText"),
		TEXT("KeyItemTabText"),
		TEXT("QuestItemTabText")
	};

	TabButtons.SetNum(UE_ARRAY_COUNT(ButtonNames));
	TabTexts.SetNum(UE_ARRAY_COUNT(TextNames));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ButtonNames); ++Index)
	{
		if (UButton* Button = FindButton(ButtonNames[Index]))
		{
			TabButtons[Index] = Button;
		}
		if (UTextBlock* Text = FindText(TextNames[Index]))
		{
			TabTexts[Index] = Text;
		}
	}

	if (TabButtons.IsValidIndex(0) && TabButtons[0])
	{
		TabButtons[0]->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleAllTabClicked);
	}
	if (TabButtons.IsValidIndex(1) && TabButtons[1])
	{
		TabButtons[1]->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleEquipmentTabClicked);
	}
	if (TabButtons.IsValidIndex(2) && TabButtons[2])
	{
		TabButtons[2]->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleConsumableTabClicked);
	}
	if (TabButtons.IsValidIndex(3) && TabButtons[3])
	{
		TabButtons[3]->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleKeyItemTabClicked);
	}
	if (TabButtons.IsValidIndex(4) && TabButtons[4])
	{
		TabButtons[4]->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleQuestItemTabClicked);
	}

	if (UTextBlock* Text = FindText(TEXT("CountText")))
	{
		InventoryCountText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("ItemName")))
	{
		ItemNameText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("ItemType")))
	{
		ItemTypeText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("ItemEffect")))
	{
		ItemEffectText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("ItemDescription")))
	{
		ItemDescriptionText = Text;
	}

	if (UUniformGridPanel* SlotGrid = Cast<UUniformGridPanel>(WidgetTree->FindWidget(TEXT("SlotGrid"))))
	{
		BuildInventorySlots(SlotGrid);
	}

	UpdateTabVisuals();
}

void UBRInventoryWidget::HandleCloseClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->HideInventoryWidget();
	}
}

void UBRInventoryWidget::HandleAllTabClicked()
{
	SetActiveTab(EBRInventoryTab::All);
}

void UBRInventoryWidget::HandleEquipmentTabClicked()
{
	SetActiveTab(EBRInventoryTab::Equipment);
}

void UBRInventoryWidget::HandleConsumableTabClicked()
{
	SetActiveTab(EBRInventoryTab::Consumable);
}

void UBRInventoryWidget::HandleKeyItemTabClicked()
{
	SetActiveTab(EBRInventoryTab::KeyItem);
}

void UBRInventoryWidget::HandleQuestItemTabClicked()
{
	SetActiveTab(EBRInventoryTab::QuestItem);
}

void UBRInventoryWidget::BuildInventoryWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryMainBorder"));
	MainBorder->SetBrushColor(PanelColor);
	MainBorder->SetPadding(FMargin(22.0f));
	if (UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainBorder))
	{
		MainSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MainSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MainSlot->SetSize(FVector2D(1080.0f, 620.0f));
		MainSlot->SetPosition(FVector2D::ZeroVector);
	}

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryMainBox"));
	MainBorder->SetContent(MainBox);

	UHorizontalBox* HeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryHeader"));
	if (UVerticalBoxSlot* HeaderSlot = MainBox->AddChildToVerticalBox(HeaderBox))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryTitle"));
	TitleText->SetText(FText::FromString(TEXT("Inventory")));
	TitleText->SetColorAndOpacity(FSlateColor(TextMainColor));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 34));
	if (UHorizontalBoxSlot* TitleSlot = HeaderBox->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->SetBackgroundColor(InactiveTabColor);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleCloseClicked);
	if (UHorizontalBoxSlot* CloseSlot = HeaderBox->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
		CloseSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
	CloseText->SetText(FText::FromString(TEXT("Close")));
	CloseText->SetJustification(ETextJustify::Center);
	CloseText->SetColorAndOpacity(FSlateColor(TextMainColor));
	CloseText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
	CloseButton->AddChild(CloseText);

	UHorizontalBox* BodyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryBody"));
	if (UVerticalBoxSlot* BodySlot = MainBox->AddChildToVerticalBox(BodyBox))
	{
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* TabBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TabBorder"));
	TabBorder->SetBrushColor(FLinearColor(0.018f, 0.018f, 0.018f, 0.82f));
	TabBorder->SetPadding(FMargin(10.0f));
	if (UHorizontalBoxSlot* TabPanelSlot = BodyBox->AddChildToHorizontalBox(TabBorder))
	{
		TabPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		TabPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	USizeBox* TabSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TabSizeBox"));
	TabSizeBox->SetWidthOverride(190.0f);
	TabBorder->SetContent(TabSizeBox);

	UVerticalBox* TabBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TabBox"));
	TabSizeBox->AddChild(TabBox);

	auto AddTabButton = [this, TabBox](const TCHAR* ButtonName, EBRInventoryTab Tab) -> UButton*
	{
		UButton* TabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		TabButton->SetBackgroundColor(InactiveTabColor);

		UTextBlock* TabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TabText->SetText(GetTabText(Tab));
		TabText->SetColorAndOpacity(FSlateColor(TextMainColor));
		TabText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
		TabText->SetJustification(ETextJustify::Left);
		TabButton->AddChild(TabText);

		if (UVerticalBoxSlot* ButtonSlot = TabBox->AddChildToVerticalBox(TabButton))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		TabButtons.Add(TabButton);
		TabTexts.Add(TabText);
		return TabButton;
	};

	UButton* AllTabButton = AddTabButton(TEXT("AllTabButton"), EBRInventoryTab::All);
	UButton* EquipmentTabButton = AddTabButton(TEXT("EquipmentTabButton"), EBRInventoryTab::Equipment);
	UButton* ConsumableTabButton = AddTabButton(TEXT("ConsumableTabButton"), EBRInventoryTab::Consumable);
	UButton* KeyItemTabButton = AddTabButton(TEXT("KeyItemTabButton"), EBRInventoryTab::KeyItem);
	UButton* QuestItemTabButton = AddTabButton(TEXT("QuestItemTabButton"), EBRInventoryTab::QuestItem);
	AllTabButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleAllTabClicked);
	EquipmentTabButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleEquipmentTabClicked);
	ConsumableTabButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleConsumableTabClicked);
	KeyItemTabButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleKeyItemTabClicked);
	QuestItemTabButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleQuestItemTabClicked);

	UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryHint"));
	HintText->SetText(FText::FromString(TEXT("Click a usable item to consume it.")));
	HintText->SetColorAndOpacity(FSlateColor(TextMutedColor));
	HintText->SetAutoWrapText(true);
	HintText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
	if (UVerticalBoxSlot* HintSlot = TabBox->AddChildToVerticalBox(HintText))
	{
		HintSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
		HintSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	UBorder* GridBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GridBorder"));
	GridBorder->SetBrushColor(PanelSoftColor);
	GridBorder->SetPadding(FMargin(16.0f));
	if (UHorizontalBoxSlot* GridBorderSlot = BodyBox->AddChildToHorizontalBox(GridBorder))
	{
		GridBorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GridBorderSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	UVerticalBox* GridBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GridBox"));
	GridBorder->SetContent(GridBox);

	InventoryCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryCountText"));
	InventoryCountText->SetText(FText::FromString(TEXT("0 items")));
	InventoryCountText->SetColorAndOpacity(FSlateColor(TextMutedColor));
	InventoryCountText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13));
	if (UVerticalBoxSlot* CountSlot = GridBox->AddChildToVerticalBox(InventoryCountText))
	{
		CountSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UUniformGridPanel* SlotGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("InventorySlotGrid"));
	SlotGrid->SetSlotPadding(FMargin(7.0f));
	if (UVerticalBoxSlot* GridSlot = GridBox->AddChildToVerticalBox(SlotGrid))
	{
		GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* DetailBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailBorder"));
	DetailBorder->SetBrushColor(FLinearColor(0.018f, 0.018f, 0.018f, 0.88f));
	DetailBorder->SetPadding(FMargin(16.0f));
	if (UHorizontalBoxSlot* DetailSlot = BodyBox->AddChildToHorizontalBox(DetailBorder))
	{
		DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	USizeBox* DetailSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DetailSizeBox"));
	DetailSizeBox->SetWidthOverride(300.0f);
	DetailBorder->SetContent(DetailSizeBox);

	UVerticalBox* DetailBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailBox"));
	DetailSizeBox->AddChild(DetailBox);

	UTextBlock* DetailTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailTitle"));
	DetailTitle->SetText(FText::FromString(TEXT("Item Details")));
	DetailTitle->SetColorAndOpacity(FSlateColor(TextMutedColor));
	DetailTitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	DetailBox->AddChildToVerticalBox(DetailTitle);

	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	ItemNameText->SetText(FText::FromString(TEXT("Empty")));
	ItemNameText->SetColorAndOpacity(FSlateColor(TextMainColor));
	ItemNameText->SetAutoWrapText(true);
	ItemNameText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 24));
	if (UVerticalBoxSlot* NameSlot = DetailBox->AddChildToVerticalBox(ItemNameText))
	{
		NameSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 12.0f));
	}

	ItemTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemTypeText"));
	ItemTypeText->SetText(FText::FromString(TEXT("-")));
	ItemTypeText->SetColorAndOpacity(FSlateColor(ActiveTabColor));
	ItemTypeText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
	DetailBox->AddChildToVerticalBox(ItemTypeText);

	ItemEffectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemEffectText"));
	ItemEffectText->SetText(FText::FromString(TEXT("-")));
	ItemEffectText->SetColorAndOpacity(FSlateColor(TextMainColor));
	ItemEffectText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* EffectSlot = DetailBox->AddChildToVerticalBox(ItemEffectText))
	{
		EffectSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 16.0f));
	}

	ItemDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemDescriptionText"));
	ItemDescriptionText->SetText(FText::FromString(TEXT("No item selected.")));
	ItemDescriptionText->SetColorAndOpacity(FSlateColor(TextMutedColor));
	ItemDescriptionText->SetAutoWrapText(true);
	DetailBox->AddChildToVerticalBox(ItemDescriptionText);

	BuildInventorySlots(SlotGrid);
	UpdateTabVisuals();
}

void UBRInventoryWidget::BuildInventorySlots(UUniformGridPanel* SlotGrid)
{
	if (!SlotGrid || SlotWidgets.Num() > 0)
	{
		return;
	}

	SlotWidgets.SetNum(DisplaySlotCount);
	for (int32 DisplayIndex = 0; DisplayIndex < DisplaySlotCount; ++DisplayIndex)
	{
		UClass* WidgetClass = SlotWidgetClass ? SlotWidgetClass.Get() : UBRInventorySlotWidget::StaticClass();
		UBRInventorySlotWidget* SlotWidget = CreateWidget<UBRInventorySlotWidget>(GetOwningPlayer(), WidgetClass);
		SlotWidgets[DisplayIndex] = SlotWidget;
		if (!SlotWidget)
		{
			continue;
		}
		if (UUniformGridSlot* GridSlot = SlotGrid->AddChildToUniformGrid(SlotWidget, DisplayIndex / 6, DisplayIndex % 6))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UBRInventoryWidget::RefreshSlotList()
{
	FilteredSlotIndices.Reset();
	for (int32 SlotIndex = 0; SlotIndex < CachedSlots.Num(); ++SlotIndex)
	{
		if (DoesSlotMatchTab(CachedSlots[SlotIndex]))
		{
			FilteredSlotIndices.Add(SlotIndex);
		}
	}

	for (int32 DisplayIndex = 0; DisplayIndex < SlotWidgets.Num(); ++DisplayIndex)
	{
		UBRInventorySlotWidget* SlotWidget = SlotWidgets[DisplayIndex];
		if (!SlotWidget)
		{
			continue;
		}

		if (FilteredSlotIndices.IsValidIndex(DisplayIndex))
		{
			const int32 SourceSlotIndex = FilteredSlotIndices[DisplayIndex];
			SlotWidget->SetVisibility(ESlateVisibility::Visible);
			SlotWidget->SetSlotData(SourceSlotIndex, CachedSlots[SourceSlotIndex]);
		}
		else
		{
			SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
			SlotWidget->SetSlotData(INDEX_NONE, FBRInventorySlot());
		}
	}

	if (InventoryCountText)
	{
		InventoryCountText->SetText(FText::FromString(FString::Printf(TEXT("%s  /  %d item%s"),
			*GetTabText(ActiveTab).ToString(),
			FilteredSlotIndices.Num(),
			FilteredSlotIndices.Num() == 1 ? TEXT("") : TEXT("s"))));
	}

	if (FilteredSlotIndices.Num() > 0)
	{
		UpdateDetailsPanel(CachedSlots[FilteredSlotIndices[0]]);
	}
	else
	{
		ClearDetailsPanel();
	}
}

void UBRInventoryWidget::SetActiveTab(EBRInventoryTab NewTab)
{
	if (ActiveTab == NewTab)
	{
		return;
	}

	ActiveTab = NewTab;
	UpdateTabVisuals();
	RefreshSlotList();
}

bool UBRInventoryWidget::DoesSlotMatchTab(const FBRInventorySlot& InventorySlot) const
{
	if (InventorySlot.IsEmpty())
	{
		return false;
	}

	if (ActiveTab == EBRInventoryTab::All)
	{
		return true;
	}

	const EBRInventoryItemCategory Category = GetItemType(InventorySlot);
	switch (ActiveTab)
	{
	case EBRInventoryTab::Equipment:
		return Category == EBRInventoryItemCategory::Equipment;
	case EBRInventoryTab::Consumable:
		return Category == EBRInventoryItemCategory::Consumable;
	case EBRInventoryTab::KeyItem:
		return Category == EBRInventoryItemCategory::KeyItem;
	case EBRInventoryTab::QuestItem:
		return Category == EBRInventoryItemCategory::QuestItem;
	default:
		return true;
	}
}

EBRInventoryItemCategory UBRInventoryWidget::GetItemType(const FBRInventorySlot& InventorySlot) const
{
	if (InventorySlot.Item.Category != EBRInventoryItemCategory::Misc)
	{
		return InventorySlot.Item.Category;
	}

	switch (InventorySlot.Item.Effect)
	{
	case EBRInventoryItemEffect::HealHP:
	case EBRInventoryItemEffect::RestoreStamina:
	case EBRInventoryItemEffect::RestoreAll:
	case EBRInventoryItemEffect::GrantUpgradePoint:
		return EBRInventoryItemCategory::Consumable;
	case EBRInventoryItemEffect::HiddenRootWeapon:
		return EBRInventoryItemCategory::Equipment;
	default:
		break;
	}

	const FString ItemId = InventorySlot.Item.ItemId.ToString();
	if (ItemId.StartsWith(TEXT("Weapon_")) || ItemId.Contains(TEXT("Armor")) || ItemId.Contains(TEXT("Ring")))
	{
		return EBRInventoryItemCategory::Equipment;
	}

	if (ItemId.Contains(TEXT("Quest")) || ItemId.Contains(TEXT("Ending")) || ItemId.Contains(TEXT("Story")))
	{
		return EBRInventoryItemCategory::QuestItem;
	}

	return InventorySlot.Item.bUsable ? EBRInventoryItemCategory::Consumable : EBRInventoryItemCategory::KeyItem;
}

FText UBRInventoryWidget::GetTabText(EBRInventoryTab Tab) const
{
	switch (Tab)
	{
	case EBRInventoryTab::Equipment:
		return FText::FromString(TEXT("Equipment"));
	case EBRInventoryTab::Consumable:
		return FText::FromString(TEXT("Consumables"));
	case EBRInventoryTab::KeyItem:
		return FText::FromString(TEXT("Key Items"));
	case EBRInventoryTab::QuestItem:
		return FText::FromString(TEXT("Quest Items"));
	default:
		return FText::FromString(TEXT("All Items"));
	}
}

FString UBRInventoryWidget::GetCategoryDisplayName(EBRInventoryItemCategory Category) const
{
	switch (Category)
	{
	case EBRInventoryItemCategory::Equipment:
		return TEXT("Equipment");
	case EBRInventoryItemCategory::Consumable:
		return TEXT("Consumable");
	case EBRInventoryItemCategory::KeyItem:
		return TEXT("Key Item");
	case EBRInventoryItemCategory::QuestItem:
		return TEXT("Quest Item");
	default:
		return TEXT("Misc");
	}
}

void UBRInventoryWidget::UpdateDetailsPanel(const FBRInventorySlot& InventorySlot)
{
	if (!ItemNameText || !ItemTypeText || !ItemEffectText || !ItemDescriptionText || InventorySlot.IsEmpty())
	{
		return;
	}

	const FText ItemName = InventorySlot.Item.DisplayName.IsEmpty() ? FText::FromName(InventorySlot.Item.ItemId) : InventorySlot.Item.DisplayName;
	ItemNameText->SetText(ItemName);

	const EBRInventoryItemCategory Category = GetItemType(InventorySlot);
	ItemTypeText->SetText(FText::FromString(FString::Printf(TEXT("%s  /  x%d"), *GetCategoryDisplayName(Category), InventorySlot.Quantity)));

	FString EffectText = TEXT("No active effect.");
	switch (InventorySlot.Item.Effect)
	{
	case EBRInventoryItemEffect::HealHP:
		EffectText = FString::Printf(TEXT("Restores %.0f HP."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::RestoreStamina:
		EffectText = FString::Printf(TEXT("Restores %.0f stamina."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::RestoreAll:
		EffectText = FString::Printf(TEXT("Restores %.0f HP and stamina."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::GrantUpgradePoint:
		EffectText = FString::Printf(TEXT("Grants %.0f upgrade point."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::HiddenRootWeapon:
		EffectText = FString::Printf(TEXT("Authority damage against CMD x%.1f."), InventorySlot.Item.EffectValue);
		break;
	default:
		break;
	}

	ItemEffectText->SetText(FText::FromString(EffectText));
	ItemDescriptionText->SetText(InventorySlot.Item.Description.IsEmpty()
		? FText::FromString(TEXT("No description."))
		: InventorySlot.Item.Description);
}

void UBRInventoryWidget::ClearDetailsPanel()
{
	if (ItemNameText)
	{
		ItemNameText->SetText(FText::FromString(TEXT("Empty")));
	}
	if (ItemTypeText)
	{
		ItemTypeText->SetText(GetTabText(ActiveTab));
	}
	if (ItemEffectText)
	{
		ItemEffectText->SetText(FText::FromString(TEXT("-")));
	}
	if (ItemDescriptionText)
	{
		ItemDescriptionText->SetText(FText::FromString(TEXT("There are no items in this category.")));
	}
}

void UBRInventoryWidget::UpdateTabVisuals()
{
	for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
	{
		UButton* TabButton = TabButtons[Index];
		UTextBlock* TabText = TabTexts.IsValidIndex(Index) ? TabTexts[Index] : nullptr;
		if (!TabButton)
		{
			continue;
		}

		const EBRInventoryTab Tab = static_cast<EBRInventoryTab>(Index);
		const bool bIsActive = Tab == ActiveTab;
		TabButton->SetBackgroundColor(bIsActive ? ActiveTabColor : InactiveTabColor);
		if (TabText)
		{
			TabText->SetColorAndOpacity(FSlateColor(bIsActive ? FLinearColor(0.03f, 0.03f, 0.03f, 1.0f) : TextMainColor));
		}
	}
}
