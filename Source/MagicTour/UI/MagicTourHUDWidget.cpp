// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MagicTourHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
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
