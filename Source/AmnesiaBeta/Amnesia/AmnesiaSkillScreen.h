#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Sound/SlateSound.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "AmnesiaSkills.h"
#include "AmnesiaMenu.h"

class UAmnesiaSkillComponent;
class SAmnesiaSkillNode;
struct FCompositeFont;

/** Icon textures by skill id */
typedef TMap<FName, UObject*> FAmnesiaSkillIcons;

/**
 *  A small padlock drawn from rounded boxes, shown on what is locked
 */
class SAmnesiaPadlock : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAmnesiaPadlock)
		: _Size(30.0f)
		, _Color(FLinearColor(0.8f, 0.75f, 0.65f, 1.0f))
	{}
		/** Height of the padlock */
		SLATE_ARGUMENT(float, Size)
		SLATE_ARGUMENT(FLinearColor, Color)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	FSlateRoundedBoxBrush ShackleBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 8.0f, FLinearColor::White, 3.0f);
	FSlateRoundedBoxBrush BodyBrush = FSlateRoundedBoxBrush(FLinearColor::White, 3.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
	FSlateRoundedBoxBrush KeyholeBrush = FSlateRoundedBoxBrush(FLinearColor::Black, 2.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
};

/**
 *  One skill on the tree: its icon in a frame that shows if it is locked, ready to learn, learned or at its top rank,
 *  and a pip for every rank. Works with the mouse, the arrow keys and a gamepad.
 */
class SAmnesiaSkillNode : public SCompoundWidget
{
public:

	DECLARE_DELEGATE_OneParam(FOnSkillAction, FName);

	SLATE_BEGIN_ARGS(SAmnesiaSkillNode)
		: _Skills(nullptr)
		, _IconBrush(nullptr)
		, _FillBrush(nullptr)
		, _OutlineBrush(nullptr)
		, _PipBrush(nullptr)
		, _BranchColor(FLinearColor::White)
	{}
		SLATE_ARGUMENT(FName, SkillId)
		SLATE_ARGUMENT(UAmnesiaSkillComponent*, Skills)
		SLATE_ARGUMENT(const FSlateBrush*, IconBrush)
		SLATE_ARGUMENT(const FSlateBrush*, FillBrush)
		SLATE_ARGUMENT(const FSlateBrush*, OutlineBrush)
		SLATE_ARGUMENT(const FSlateBrush*, PipBrush)
		SLATE_ARGUMENT(FLinearColor, BranchColor)
		SLATE_ARGUMENT(FSlateFontInfo, BadgeFont)
		SLATE_ARGUMENT(FSlateSound, ClickSound)
		SLATE_ARGUMENT(FSlateSound, HoverSound)

		/** The skill is looked at, with the mouse or the keyboard */
		SLATE_EVENT(FOnSkillAction, OnSelected)

		/** The skill is clicked or confirmed */
		SLATE_EVENT(FOnSkillAction, OnActivate)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FName GetSkillId() const { return SkillId; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	enum class EState : uint8
	{
		Locked,
		Available,
		Learned,
		Maxed
	};

	EState GetState() const;

	FName SkillId;
	TWeakObjectPtr<UAmnesiaSkillComponent> Skills;
	FLinearColor BranchColor = FLinearColor::White;
	FSlateSound ClickSound;
	FSlateSound HoverSound;
	FOnSkillAction OnSelected;
	FOnSkillAction OnActivate;
};

/**
 *  The skill screen: four columns of skills (steel, fire, ice, dragon), the skill points, the element on the sword
 *  and a panel with the details of the skill that is looked at.
 */
class SAmnesiaSkillScreen : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAmnesiaSkillScreen)
		: _Skills(nullptr)
		, _FrameTexture(nullptr)
		, _DividerTexture(nullptr)
		, _FontRegular(nullptr)
		, _FontBold(nullptr)
		, _ClickSound(nullptr)
		, _HoverSound(nullptr)
	{}
		SLATE_ARGUMENT(UAmnesiaSkillComponent*, Skills)

		/** Optional art: a 9-slice frame drawn in gold around the panel */
		SLATE_ARGUMENT(UObject*, FrameTexture)

		/** Optional art for the dividers, and the pictures of the decoration */
		SLATE_ARGUMENT(UObject*, DividerTexture)
		SLATE_ARGUMENT(FAmnesiaUIIcons, Emblems)

		/** Optional font faces. Without them the default font is used */
		SLATE_ARGUMENT(UObject*, FontRegular)
		SLATE_ARGUMENT(UObject*, FontBold)

		SLATE_ARGUMENT(UObject*, ClickSound)
		SLATE_ARGUMENT(UObject*, HoverSound)

		SLATE_ARGUMENT(FAmnesiaSkillIcons, Icons)

		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Gives keyboard and gamepad focus to the first skill */
	void FocusFirstNode();

	/** Closes the screen */
	void GoBack();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	FSlateFontInfo MakeFont(bool bBold, int32 Size) const;

	/** The plain engine font, for text that is read, not looked at */
	static FSlateFontInfo MakeBodyFont(bool bBold, int32 Size);
	TSharedRef<SWidget> MakeButton(const FText& Label, FSimpleDelegate Action);
	TSharedRef<SWidget> MakeBranch(EAmnesiaSkillBranch Branch);
	TSharedRef<SWidget> MakeDetails();
	TSharedRef<SWidget> WithFrame(TSharedRef<SWidget> Content, float Inset);

	/** The input mode of the game gives the focus to the whole screen a moment after it opened, so the first skill gets it again a little later */
	EActiveTimerReturnType FocusTimer(double InCurrentTime, float InDeltaTime);

	/** A skill was looked at or clicked */
	void SelectSkill(FName Id);
	void ActivateSkill(FName Id);

	/** The pieces of the details panel */
	FText GetDetailName() const;
	FText GetDetailKind() const;
	FText GetDetailText() const;
	FText GetDetailNow() const;
	FText GetDetailNext() const;
	FText GetDetailAction() const;
	FSlateColor GetDetailColor() const;
	const FSlateBrush* GetDetailIcon() const;

	TWeakObjectPtr<UAmnesiaSkillComponent> Skills;
	FSimpleDelegate OnClose;

	FName SelectedId;

	/** What a failed try to learn said, shown for a moment */
	FString Complaint;
	double ComplaintUntil = 0.0;

	/** The font faces as composite fonts: Slate only takes a font object that provides one, a bare font face is shown as boxes */
	TSharedPtr<const FCompositeFont> RegularFont;
	TSharedPtr<const FCompositeFont> BoldFont;
	FSlateSound ClickSound;
	FSlateSound HoverSound;

	TArray<TSharedPtr<SAmnesiaSkillNode>> Nodes;
	TMap<FName, TSharedPtr<FSlateBrush>> IconBrushes;

	FSlateColorBrush DimBrush = FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);
	FSlateColorBrush GoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 1.0f));
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.025f, 0.02f, 0.97f), 10.0f, FLinearColor(0.3f, 0.22f, 0.1f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush InnerPanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.04f, 0.03f, 0.9f), 6.0f, FLinearColor(0.45f, 0.34f, 0.15f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush ColumnBrush = FSlateRoundedBoxBrush(FLinearColor(0.045f, 0.032f, 0.026f, 0.85f), 8.0f, FLinearColor(0.3f, 0.22f, 0.1f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush NodeFillBrush = FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.04f, 0.035f, 1.0f), 10.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
	FSlateRoundedBoxBrush NodeOutlineBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 10.0f, FLinearColor::White, 3.0f);
	FSlateRoundedBoxBrush ButtonNormal = FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.06f, 0.04f, 0.96f), 6.0f, FLinearColor(0.5f, 0.38f, 0.16f, 1.0f), 2.0f);
	FSlateRoundedBoxBrush ButtonHovered = FSlateRoundedBoxBrush(FLinearColor(0.24f, 0.15f, 0.06f, 1.0f), 6.0f, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonPressed = FSlateRoundedBoxBrush(FLinearColor(0.13f, 0.08f, 0.04f, 1.0f), 6.0f, FLinearColor(0.9f, 0.7f, 0.3f, 1.0f), 2.5f);
	FSlateBrush FrameBrush;
	FAmnesiaUIDecor Decor;
	FButtonStyle ButtonStyle;
};
