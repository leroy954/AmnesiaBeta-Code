#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"

class SButton;

/**
 *  Test and admin panel, opened in the game with K.
 *  Switches god mode, flying and endless skill points, gives gold, and teleports the hero to the start of any zone.
 */
class SAmnesiaAdminPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAmnesiaAdminPanel) {}
		/** Zone names, in level order */
		SLATE_ARGUMENT(TArray<FString>, ZoneNames)
		/** Read and set the god mode and flying states */
		SLATE_ARGUMENT(TFunction<bool()>, IsGod)
		SLATE_ARGUMENT(TFunction<void(bool)>, SetGod)
		SLATE_ARGUMENT(TFunction<bool()>, IsFlying)
		SLATE_ARGUMENT(TFunction<void(bool)>, SetFlying)
		/** Endless skill points, for testing the skills, and gold, for testing the shop */
		SLATE_ARGUMENT(TFunction<bool()>, IsInfinitePoints)
		SLATE_ARGUMENT(TFunction<void(bool)>, SetInfinitePoints)
		SLATE_ARGUMENT(TFunction<void(int32)>, AddGold)
		/** The hero's level and the highest one it can reach, and a change of the level by some steps (up or down) */
		SLATE_ARGUMENT(TFunction<int32()>, GetLevel)
		SLATE_ARGUMENT(TFunction<int32()>, GetMaxLevel)
		SLATE_ARGUMENT(TFunction<void(int32)>, ChangeLevel)
		/** Teleports to a zone, by its index in ZoneNames */
		SLATE_ARGUMENT(TFunction<void(int32)>, Teleport)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Gives keyboard and gamepad focus to the first control */
	void FocusFirst();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	TSharedRef<SWidget> MakeToggle(const FText& Label, TFunction<bool()> Get, TFunction<void(bool)> Set);
	TSharedRef<SWidget> MakeButton(const FText& Label, FSimpleDelegate Action, float Height = 44.0f, int32 FontSize = 17);

	FSimpleDelegate OnClose;
	TSharedPtr<SButton> FirstButton;

	FSlateColorBrush DimBrush = FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.025f, 0.02f, 0.95f), 10.0f, FLinearColor(0.78f, 0.6f, 0.26f, 1.0f), 3.0f);
	FSlateRoundedBoxBrush ButtonNormal = FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.06f, 0.04f, 0.96f), 6.0f, FLinearColor(0.5f, 0.38f, 0.16f, 1.0f), 2.0f);
	FSlateRoundedBoxBrush ButtonHovered = FSlateRoundedBoxBrush(FLinearColor(0.24f, 0.15f, 0.06f, 1.0f), 6.0f, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonPressed = FSlateRoundedBoxBrush(FLinearColor(0.13f, 0.08f, 0.04f, 1.0f), 6.0f, FLinearColor(0.9f, 0.7f, 0.3f, 1.0f), 2.5f);
	FSlateColorBrush FaintGoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 0.45f));
	FButtonStyle ButtonStyle;
};
