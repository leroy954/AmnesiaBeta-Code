#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"

class UAmnesiaSaveGame;
struct FCompositeFont;
class SButton;
class SWidgetSwitcher;

/** Which menu is shown */
enum class EAmnesiaMenuMode : uint8
{
	Main,
	Pause
};

/** Key and action pairs for the controls list */
typedef TArray<TPair<FString, FString>> FAmnesiaControlLines;

/** Pictures the decoration is made of, by name: chakram, axe, wand (weapons), dragon, fire, ice, steel (skill branches) */
typedef TMap<FName, UObject*> FAmnesiaUIIcons;

/**
 *  The RPG decoration the menus, the shop and the skill screen share: gold filigree for the corners of a panel,
 *  medallions with a weapon or a skill in them, title ribbons, dividers and keycaps.
 *  Every screen that uses it keeps one set of the brushes.
 */
struct FAmnesiaUIDecor
{
	/** Makes the brushes: the divider art, and one for every picture. Both are optional */
	void Init(UObject* DividerTexture, const FAmnesiaUIIcons& Icons);

	/** The filigree of the four corners of a panel, to lay over it. Size is how far it reaches along the edges, Inset how far from the edge it starts */
	TSharedRef<SWidget> Corners(float Size, float Inset) const;

	/** The content with the corner filigree over it */
	TSharedRef<SWidget> WithCorners(TSharedRef<SWidget> Content, float Size, float Inset) const;

	TSharedRef<SWidget> Diamond(float Size) const;

	/** A picture in a gold ring: round for the weapons, a rounded square for art that fills its square. The tint dims it */
	TSharedRef<SWidget> Medallion(FName Icon, float Size, bool bRound, TAttribute<FLinearColor> Tint = FLinearColor::White) const;

	/** The chakram, the axe and the wand in medallions, the axe in the middle and bigger. Size is the one of the axe */
	TSharedRef<SWidget> Trio(float Size) const;

	/** A crimson ribbon with pointed ends around the content */
	TSharedRef<SWidget> Ribbon(TSharedRef<SWidget> Content, float Height) const;

	/** A divider: a faded gold line to each side of the center, which keeps its own width */
	TSharedRef<SWidget> Ornament(TSharedRef<SWidget> Center) const;

	/** A key drawn as a little cap */
	TSharedRef<SWidget> Keycap(const FString& Key, const FSlateFontInfo& Font) const;

	const FSlateBrush* FindIcon(FName Id) const;

	FSlateBrush DividerBrush;
	TMap<FName, TSharedPtr<FSlateBrush>> IconBrushes;

	FSlateColorBrush GoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 1.0f));
	FSlateColorBrush BrightGoldBrush = FSlateColorBrush(FLinearColor(1.0f, 0.86f, 0.45f, 1.0f));
	FSlateColorBrush FaintGoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 0.45f));
	FSlateRoundedBoxBrush CornerRingBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, FLinearColor(0.78f, 0.6f, 0.26f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush MedalFillBrush = FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.035f, 0.03f, 1.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
	FSlateRoundedBoxBrush MedalRingBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), FLinearColor(0.88f, 0.68f, 0.3f, 1.0f), 3.0f);
	FSlateRoundedBoxBrush MedalInnerRingBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), FLinearColor(0.78f, 0.6f, 0.26f, 0.5f), 1.0f);
	FSlateRoundedBoxBrush PlaqueFillBrush = FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.035f, 0.03f, 1.0f), 9.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
	FSlateRoundedBoxBrush PlaqueRingBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 9.0f, FLinearColor(0.88f, 0.68f, 0.3f, 1.0f), 3.0f);
	FSlateRoundedBoxBrush RibbonBrush = FSlateRoundedBoxBrush(FLinearColor(0.3f, 0.065f, 0.08f, 1.0f), 3.0f, FLinearColor(0.85f, 0.65f, 0.28f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush RibbonTipBrush = FSlateRoundedBoxBrush(FLinearColor(0.15f, 0.03f, 0.04f, 1.0f), 2.0f, FLinearColor(0.7f, 0.52f, 0.22f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush RibbonLineBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 2.0f, FLinearColor(0.95f, 0.76f, 0.36f, 0.45f), 1.0f);
	FSlateRoundedBoxBrush KeycapBrush = FSlateRoundedBoxBrush(FLinearColor(0.13f, 0.09f, 0.055f, 1.0f), 4.0f, FLinearColor(0.55f, 0.42f, 0.18f, 1.0f), 1.5f);
};

/**
 *  RPG style main and pause menu, drawn with Slate: a dark panel with a gold frame,
 *  ornaments, framed buttons, a settings page and the controls list.
 *  Works with the mouse, the keyboard arrows and a gamepad.
 */
class SAmnesiaMenu : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAmnesiaMenu)
		: _Mode(EAmnesiaMenuMode::Main)
		, _bCanContinue(false)
		, _Settings(nullptr)
		, _FrameTexture(nullptr)
		, _DividerTexture(nullptr)
		, _ClickSound(nullptr)
		, _HoverSound(nullptr)
		, _FontRegular(nullptr)
		, _FontBold(nullptr)
	{}
		SLATE_ARGUMENT(EAmnesiaMenuMode, Mode)
		SLATE_ARGUMENT(bool, bCanContinue)
		SLATE_ARGUMENT(UAmnesiaSaveGame*, Settings)
		SLATE_ARGUMENT(FAmnesiaControlLines, ControlLines)
		/** Optional art: a 9-slice frame drawn in gold around the panels, and a one-sided divider used for the ornaments */
		SLATE_ARGUMENT(UObject*, FrameTexture)
		SLATE_ARGUMENT(UObject*, DividerTexture)

		/** Optional pictures for the decoration: the weapons, and the skill branches */
		SLATE_ARGUMENT(FAmnesiaUIIcons, Emblems)
		/** Optional button sounds */
		SLATE_ARGUMENT(UObject*, ClickSound)
		SLATE_ARGUMENT(UObject*, HoverSound)

		/** Optional font faces for the title and the buttons */
		SLATE_ARGUMENT(UObject*, FontRegular)
		SLATE_ARGUMENT(UObject*, FontBold)
		SLATE_EVENT(FSimpleDelegate, OnNewGame)
		SLATE_EVENT(FSimpleDelegate, OnContinue)
		SLATE_EVENT(FSimpleDelegate, OnResume)
		SLATE_EVENT(FSimpleDelegate, OnSkills)
		SLATE_EVENT(FSimpleDelegate, OnShop)
		SLATE_EVENT(FSimpleDelegate, OnMainMenu)
		SLATE_EVENT(FSimpleDelegate, OnQuit)
		SLATE_EVENT(FSimpleDelegate, OnSettingsChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Gives keyboard and gamepad focus to the first button */
	void FocusFirstButton();

	/** Backs out of the settings page, or closes the pause menu. The main menu stays */
	void GoBack();

	/** Opens the settings page, as if the button was clicked */
	void ShowSettings() { ShowPage(1); }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	TSharedRef<SWidget> MakeButton(const FText& Label, FSimpleDelegate Action, bool bEnabled = true);

	/** The menu font: the font face if there is one, otherwise the default style */
	FSlateFontInfo MakeFont(bool bBold, const char* DefaultStyle, int32 Size) const;
	TSharedRef<SWidget> MakeOrnament(bool bWithIcons = false);
	TSharedRef<SWidget> MakeSettingsPage();
	TSharedRef<SWidget> MakeControlsPanel(const FAmnesiaControlLines& Lines);
	TSharedRef<SWidget> MakeCheckRow(const FText& Label, bool* Value);

	/** The graphics quality choices: four toggle buttons, the chosen one filled gold */
	TSharedRef<SWidget> MakeQualityRow();

	/** Puts the gold frame over a panel, if the frame art is loaded */
	TSharedRef<SWidget> WithFrame(TSharedRef<SWidget> Content, float Inset);

	void ShowPage(int32 Page);

	EAmnesiaMenuMode Mode = EAmnesiaMenuMode::Main;

	TWeakObjectPtr<UAmnesiaSaveGame> Settings;

	FSimpleDelegate OnNewGame;
	FSimpleDelegate OnContinue;
	FSimpleDelegate OnResume;
	FSimpleDelegate OnSkills;
	FSimpleDelegate OnShop;
	FSimpleDelegate OnMainMenu;
	FSimpleDelegate OnQuit;
	FSimpleDelegate OnSettingsChanged;

	TArray<TSharedPtr<SButton>> MainButtons;
	TSharedPtr<SButton> SettingsBackButton;
	TSharedPtr<SWidgetSwitcher> Switcher;

	/** Brushes and styles live as long as the widget */
	FSlateColorBrush DimBrush = FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
	FSlateColorBrush GoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 1.0f));
	FSlateColorBrush FaintGoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 0.45f));
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.025f, 0.02f, 0.94f), 10.0f, FLinearColor(0.78f, 0.6f, 0.26f, 1.0f), 3.0f);
	FSlateColorBrush RowBrush = FSlateColorBrush(FLinearColor(0.1f, 0.07f, 0.045f, 0.55f));
	FSlateRoundedBoxBrush InnerPanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.04f, 0.03f, 0.9f), 6.0f, FLinearColor(0.45f, 0.34f, 0.15f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush ButtonNormal = FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.06f, 0.04f, 0.96f), 6.0f, FLinearColor(0.5f, 0.38f, 0.16f, 1.0f), 2.0f);
	FSlateRoundedBoxBrush ButtonHovered = FSlateRoundedBoxBrush(FLinearColor(0.24f, 0.15f, 0.06f, 1.0f), 6.0f, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonPressed = FSlateRoundedBoxBrush(FLinearColor(0.13f, 0.08f, 0.04f, 1.0f), 6.0f, FLinearColor(0.9f, 0.7f, 0.3f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonDisabled = FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.05f, 0.04f, 0.6f), 6.0f, FLinearColor(0.28f, 0.23f, 0.16f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush ButtonSelected = FSlateRoundedBoxBrush(FLinearColor(0.5f, 0.34f, 0.1f, 1.0f), 6.0f, FLinearColor(1.0f, 0.85f, 0.45f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonSelectedHovered = FSlateRoundedBoxBrush(FLinearColor(0.62f, 0.43f, 0.14f, 1.0f), 6.0f, FLinearColor(1.0f, 0.9f, 0.55f, 1.0f), 2.5f);
	FSlateBrush FrameBrush;
	FAmnesiaUIDecor Decor;
	FButtonStyle ButtonStyle;
	FCheckBoxStyle QualityStyle;

	TSharedPtr<const FCompositeFont> RegularFont;
	TSharedPtr<const FCompositeFont> BoldFont;
};
