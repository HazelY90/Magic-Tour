// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MagicTourHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

bool UMagicTourHUDWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return true;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Two anchored bars remain centered across aspect ratios and viewport sizes.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			Index == 0 ? TEXT("CrosshairH") : TEXT("CrosshairV"));
		Bar->SetBrushColor(FLinearColor::White);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		Crosshair.Add(Bar);
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Bar);
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(Index == 0 ? FVector2D(14.0f, 2.0f) : FVector2D(2.0f, 14.0f));
	}

	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HUDPanel"));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetPosition(FVector2D(32.0f, 32.0f));
	PanelSlot->SetSize(FVector2D(360.0f, 110.0f));

	UTextBlock* HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
	HealthText->SetText(NSLOCTEXT("MagicTourHUD", "Health", "Health"));
	HealthText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo HealthFont = HealthText->GetFont();
	HealthFont.Size = 16;
	HealthText->SetFont(HealthFont);
	Panel->AddChildToVerticalBox(HealthText);

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	HealthBar->SetPercent(1.0f);
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.72f, 0.05f, 0.05f, 1.0f));
	UVerticalBoxSlot* HealthSlot = Panel->AddChildToVerticalBox(HealthBar);
	HealthSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	HealthSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TaskGap"));
	Gap->SetSize(FVector2D(1.0f, 10.0f));
	Panel->AddChildToVerticalBox(Gap);

	TaskText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TaskText"));
	TaskText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.25f, 1.0f)));
	FSlateFontInfo TaskFont = TaskText->GetFont();
	TaskFont.Size = 20;
	TaskText->SetFont(TaskFont);
	Panel->AddChildToVerticalBox(TaskText);

	// A full-screen scrim blocks gameplay clicks while the centered action has focus.
	DeathOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeathOverlay"));
	DeathOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	DeathOverlay->SetPadding(FMargin(0.0f));
	UCanvasPanelSlot* OverlaySlot = Root->AddChildToCanvas(DeathOverlay);
	OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	OverlaySlot->SetOffsets(FMargin(0.0f));
	OverlaySlot->SetZOrder(20);
	UCanvasPanel* DeathRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DeathRoot"));
	DeathOverlay->SetContent(DeathRoot);
	UBorder* DeathPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeathPanel"));
	DeathPanel->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.07f, 1.0f));
	DeathPanel->SetPadding(FMargin(28.0f));
	UCanvasPanelSlot* DeathSlot = DeathRoot->AddChildToCanvas(DeathPanel);
	DeathSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	DeathSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	DeathSlot->SetPosition(FVector2D::ZeroVector);
	DeathSlot->SetSize(FVector2D(460.0f, 230.0f));
	UVerticalBox* DeathContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeathContent"));
	DeathPanel->SetContent(DeathContent);
	UTextBlock* DeathTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeathTitle"));
	DeathTitle->SetText(NSLOCTEXT("MagicTourHUD", "DeathTitle", "You Died"));
	DeathTitle->SetJustification(ETextJustify::Center);
	DeathTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo TitleFont = DeathTitle->GetFont();
	TitleFont.Size = 30;
	DeathTitle->SetFont(TitleFont);
	DeathContent->AddChildToVerticalBox(DeathTitle);
	UTextBlock* DeathText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeathText"));
	DeathText->SetText(NSLOCTEXT("MagicTourHUD", "DeathText", "Return to the starting point with full health."));
	DeathText->SetJustification(ETextJustify::Center);
	DeathText->SetAutoWrapText(true);
	DeathText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.82f, 0.86f)));
	FSlateFontInfo BodyFont = DeathText->GetFont();
	BodyFont.Size = 16;
	DeathText->SetFont(BodyFont);
	UVerticalBoxSlot* TextSlot = DeathContent->AddChildToVerticalBox(DeathText);
	TextSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 20.0f));
	USpacer* DeathGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("DeathGap"));
	DeathContent->AddChildToVerticalBox(DeathGap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	USizeBox* ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RespawnSize"));
	ButtonBox->SetHeightOverride(46.0f);
	DeathContent->AddChildToVerticalBox(ButtonBox);
	// UButton enables keyboard and gamepad focus by default.
	RespawnButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RespawnButton"));
	RespawnButton->SetBackgroundColor(FLinearColor(0.2f, 0.32f, 0.5f));
	RespawnButton->OnClicked.AddDynamic(this, &UMagicTourHUDWidget::RespawnClicked);
	ButtonBox->SetContent(RespawnButton);
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RespawnText"));
	ButtonText->SetText(NSLOCTEXT("MagicTourHUD", "Respawn", "Respawn"));
	ButtonText->SetJustification(ETextJustify::Center);
	ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo ButtonFont = ButtonText->GetFont();
	ButtonFont.Size = 18;
	ButtonText->SetFont(ButtonFont);
	RespawnButton->SetContent(ButtonText);

	// Victory uses a separate modal beneath death so respawning remains available.
	VictoryOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VictoryOverlay"));
	VictoryOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	VictoryOverlay->SetPadding(FMargin(0.0f));
	UCanvasPanelSlot* VictoryOverlaySlot = Root->AddChildToCanvas(VictoryOverlay);
	VictoryOverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VictoryOverlaySlot->SetOffsets(FMargin(0.0f));
	VictoryOverlaySlot->SetZOrder(19);
	UCanvasPanel* VictoryRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VictoryRoot"));
	VictoryOverlay->SetContent(VictoryRoot);
	UBorder* VictoryPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VictoryPanel"));
	VictoryPanel->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.07f, 1.0f));
	VictoryPanel->SetPadding(FMargin(28.0f));
	UCanvasPanelSlot* VictorySlot = VictoryRoot->AddChildToCanvas(VictoryPanel);
	VictorySlot->SetAnchors(FAnchors(0.5f, 0.5f));
	VictorySlot->SetAlignment(FVector2D(0.5f, 0.5f));
	VictorySlot->SetPosition(FVector2D::ZeroVector);
	VictorySlot->SetSize(FVector2D(460.0f, 230.0f));
	UVerticalBox* VictoryContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VictoryContent"));
	VictoryPanel->SetContent(VictoryContent);
	UTextBlock* VictoryTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryTitle"));
	VictoryTitle->SetText(NSLOCTEXT("MagicTourHUD", "VictoryTitle", "Victory"));
	VictoryTitle->SetJustification(ETextJustify::Center);
	VictoryTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	VictoryTitle->SetFont(TitleFont);
	VictoryContent->AddChildToVerticalBox(VictoryTitle);
	UTextBlock* VictoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryText"));
	VictoryText->SetText(NSLOCTEXT("MagicTourHUD", "VictoryText", "All enemies have been defeated."));
	VictoryText->SetJustification(ETextJustify::Center);
	VictoryText->SetAutoWrapText(true);
	VictoryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.82f, 0.86f)));
	VictoryText->SetFont(BodyFont);
	UVerticalBoxSlot* VictoryTextSlot = VictoryContent->AddChildToVerticalBox(VictoryText);
	VictoryTextSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 20.0f));
	USpacer* VictoryGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("VictoryGap"));
	VictoryContent->AddChildToVerticalBox(VictoryGap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	USizeBox* ConfirmBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ConfirmSize"));
	ConfirmBox->SetHeightOverride(46.0f);
	VictoryContent->AddChildToVerticalBox(ConfirmBox);
	ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
	ConfirmButton->SetBackgroundColor(FLinearColor(0.2f, 0.32f, 0.5f));
	ConfirmButton->OnClicked.AddDynamic(this, &UMagicTourHUDWidget::ConfirmClicked);
	ConfirmBox->SetContent(ConfirmButton);
	UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmText"));
	ConfirmText->SetText(NSLOCTEXT("MagicTourHUD", "Confirm", "OK"));
	ConfirmText->SetJustification(ETextJustify::Center);
	ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfirmText->SetFont(ButtonFont);
	ConfirmButton->SetContent(ConfirmText);
	ShowDeath(false);
	ShowVictory(false);

	UpdateTask();
	return true;
}

void UMagicTourHUDWidget::SetProgress(AMagicTourGameMode* GameMode)
{
	if (Progress)
	{
		Progress->OnCheckpointChanged.RemoveDynamic(this, &UMagicTourHUDWidget::ProgressChanged);
	}

	Progress = GameMode;
	if (Progress)
	{
		Progress->OnCheckpointChanged.AddUniqueDynamic(this, &UMagicTourHUDWidget::ProgressChanged);
	}

	UpdateTask();
}

void UMagicTourHUDWidget::SetHealth(float Value)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(Value, 0.0f, 1.0f));
	}
}

void UMagicTourHUDWidget::ShowDeath(bool isDead)
{
	isDeathShown = isDead;
	if (DeathOverlay)
	{
		DeathOverlay->SetVisibility(isDead ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (VictoryOverlay)
	{
		VictoryOverlay->SetVisibility(isVictoryShown && !isDeathShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	UpdateCrosshair();
}

void UMagicTourHUDWidget::ShowVictory(bool isVisible)
{
	isVictoryShown = isVisible;
	if (VictoryOverlay)
	{
		VictoryOverlay->SetVisibility(isVictoryShown && !isDeathShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	UpdateCrosshair();
}

void UMagicTourHUDWidget::UpdateCrosshair()
{
	const bool isModalShown = isDeathShown || isVictoryShown;
	for (UBorder* Bar : Crosshair)
	{
		if (Bar)
		{
			Bar->SetVisibility(isModalShown ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}
}

void UMagicTourHUDWidget::RespawnClicked()
{
	// Ignore stale activations after the controller has returned to gameplay.
	if (isDeathShown)
	{
		OnRespawn.Broadcast();
	}
}

void UMagicTourHUDWidget::ConfirmClicked()
{
	// Hidden or superseded prompts must not complete the task.
	if (isVictoryShown && !isDeathShown)
	{
		OnConfirm.Broadcast();
	}
}

void UMagicTourHUDWidget::NativeDestruct()
{
	if (Progress)
	{
		Progress->OnCheckpointChanged.RemoveDynamic(this, &UMagicTourHUDWidget::ProgressChanged);
	}

	Super::NativeDestruct();
}

void UMagicTourHUDWidget::ProgressChanged(int32 ReachedIndex, int32 TotalCheckpoints, EMagicTourStage Stage)
{
	UpdateTask();
}

void UMagicTourHUDWidget::UpdateTask()
{
	if (!TaskText)
	{
		return;
	}

	const FText StageText = Progress ? Progress->GetStageText() : NSLOCTEXT("MagicTourHUD", "ReachForest", "Reach the Forest Entrance");
	TaskText->SetText(FText::Format(NSLOCTEXT("MagicTourHUD", "TaskFormat", "Task: {0}"), StageText));
}
