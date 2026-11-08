#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Sound/SlateSound.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "AmnesiaMenu.h"
#include "AmnesiaSkills.h"

class SButton;
class SWidgetSwitcher;
struct FCompositeFont;

/** Pictures of the things sold, by item id */
typedef TMap<FName, UObject*> FAmnesiaShopImages;

/** One thing that is sold. The list is written in AmnesiaShopScreen.cpp */
struct FAmnesiaShopItem
{
	FName Id;
	FString Name;
	FString Subtitle;
	FString Detail;
	TArray<FString> Perks;

	/** The detail page: what the thing is and does, the buttons that use it, and the branch of skills it opens */
	FString LongDetail;
	FString Controls;
	EAmnesiaSkillBranch Branch = EAmnesiaSkillBranch::Dragon;

	/** Gold */
	int32 Price = 0;
};

/** What the shop sells */
struct FAmnesiaShop
{
	static const TArray<FAmnesiaShopItem>& Items();
	static const FAmnesiaShopItem* Find(FName Id);
};

/**
 *  The shop: a framed panel with a row for everything that is sold, the gold in the purse and a button to buy.
 *  Works with the mouse, the keyboard and a gamepad.
 */
class SAmnesiaShopScreen : public SCompoundWidget
{
public:

	/** Asks the game to sell the item. True if it was sold */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShopBuy, FName);

	SLATE_BEGIN_ARGS(SAmnesiaShopScreen)
		: _FrameTexture(nullptr)
		, _DividerTexture(nullptr)
		, _FontRegular(nullptr)
		, _FontBold(nullptr)
		, _ClickSound(nullptr)
		, _HoverSound(nullptr)
		, _PurchaseSound(nullptr)
	{}
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
		SLATE_ARGUMENT(UObject*, PurchaseSound)

		/** The picture of each item */
		SLATE_ARGUMENT(FAmnesiaShopImages, Images)

		/** The gold in the purse, and what is bought already */
		SLATE_ARGUMENT(TFunction<int32()>, GetGold)
		SLATE_ARGUMENT(TFunction<bool(FName)>, IsOwned)

		SLATE_EVENT(FOnShopBuy, OnBuy)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Gives keyboard and gamepad focus to the first buy button that can be used */
	void FocusFirst();

	/** Tries to buy an item, as its button does */
	void TryBuy(FName Id);

	/** Closes the shop */
	void GoBack();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	FSlateFontInfo MakeFont(bool bBold, int32 Size) const;

	/** The plain engine font, for text that is read, not looked at */
	static FSlateFontInfo MakeBodyFont(bool bBold, int32 Size);

	TSharedRef<SWidget> MakeCoin(float Size);
	TSharedRef<SWidget> MakeButton(const FText& Label, FSimpleDelegate Action, TSharedPtr<SButton>& OutButton);
	TSharedRef<SWidget> MakeItemRow(const FAmnesiaShopItem& Item, int32 Index);
	TSharedRef<SWidget> WithFrame(TSharedRef<SWidget> Content, float Inset);

	/** The page of one thing: the picture, the long text, the buttons that use it, the skills it opens, and the buy button */
	TSharedRef<SWidget> MakeDetailPage(const FAmnesiaShopItem& Item, int32 Index);

	/** The pieces of the rows and the pages */
	TSharedRef<SWidget> MakePicture(FName Id, float Size, float Padding);
	TSharedRef<SWidget> MakePricePlaque(int32 Price);
	TSharedRef<SWidget> MakeStatusText(FName Id, int32 Price);
	TSharedRef<SWidget> MakeHeading(const FString& Text);

	/** Shows the page of an item, or the list again */
	void OpenDetails(int32 ItemIndex);
	void ShowList();

	/** Gives the keyboard and the gamepad to the first button of the page that is shown */
	void FocusCurrent();

	/** The input mode of the game gives the focus to the whole screen a moment after it opened, so the buttons get it again a little later */
	EActiveTimerReturnType FocusTimer(double InCurrentTime, float InDeltaTime);

	int32 GetGoldNow() const { return GoldGetter ? GoldGetter() : 0; }
	bool IsOwnedNow(FName Id) const { return OwnedGetter && OwnedGetter(Id); }

	TFunction<int32()> GoldGetter;
	TFunction<bool(FName)> OwnedGetter;
	FOnShopBuy OnBuy;
	FSimpleDelegate OnClose;

	/** What the last try to buy said, shown for a moment in the footer */
	FString Message;
	FLinearColor MessageColor = FLinearColor::White;
	double MessageUntil = 0.0;

	/** The font faces as composite fonts: Slate only takes a font object that provides one, a bare font face is shown as boxes */
	TSharedPtr<const FCompositeFont> RegularFont;
	TSharedPtr<const FCompositeFont> BoldFont;
	FSlateSound ClickSound;
	FSlateSound HoverSound;
	FSlateSound PurchaseSound;

	/** The button that opens the page of every item, and the buy button on that page, in order, and the back button */
	TArray<TSharedPtr<SButton>> ListButtons;
	TArray<TSharedPtr<SButton>> BuyButtons;
	TSharedPtr<SButton> BackButton;

	/** The list and a page for every item, one shown at a time. ActiveItem is the item whose page shows, -1 for the list */
	TSharedPtr<SWidgetSwitcher> Pages;
	int32 ActiveItem = -1;

	TMap<FName, TSharedPtr<FSlateBrush>> ImageBrushes;

	FSlateColorBrush DimBrush = FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
	FSlateColorBrush GoldBrush = FSlateColorBrush(FLinearColor(0.78f, 0.6f, 0.26f, 1.0f));
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.025f, 0.02f, 0.97f), 10.0f, FLinearColor(0.3f, 0.22f, 0.1f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush InnerPanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.04f, 0.03f, 0.9f), 6.0f, FLinearColor(0.45f, 0.34f, 0.15f, 1.0f), 1.5f);
	FSlateRoundedBoxBrush PlaqueBrush = FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.022f, 0.016f, 1.0f), 16.0f, FLinearColor(0.55f, 0.42f, 0.18f, 1.0f), 2.0f);
	FSlateRoundedBoxBrush PictureFillBrush = FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.03f, 0.025f, 1.0f), 10.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
	FSlateRoundedBoxBrush PictureOutlineBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 10.0f, FLinearColor(0.78f, 0.6f, 0.26f, 1.0f), 3.0f);
	FSlateRoundedBoxBrush CoinRimBrush = FSlateRoundedBoxBrush(FLinearColor(0.95f, 0.74f, 0.25f, 1.0f), 16.0f, FLinearColor(0.45f, 0.28f, 0.06f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush CoinFaceBrush = FSlateRoundedBoxBrush(FLinearColor(0.78f, 0.55f, 0.14f, 1.0f), 9.6f, FLinearColor(1.0f, 0.86f, 0.45f, 1.0f), 1.8f);
	FSlateRoundedBoxBrush ButtonNormal = FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.06f, 0.04f, 0.96f), 6.0f, FLinearColor(0.5f, 0.38f, 0.16f, 1.0f), 2.0f);
	FSlateRoundedBoxBrush ButtonHovered = FSlateRoundedBoxBrush(FLinearColor(0.24f, 0.15f, 0.06f, 1.0f), 6.0f, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonPressed = FSlateRoundedBoxBrush(FLinearColor(0.13f, 0.08f, 0.04f, 1.0f), 6.0f, FLinearColor(0.9f, 0.7f, 0.3f, 1.0f), 2.5f);
	FSlateRoundedBoxBrush ButtonDisabled = FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.05f, 0.04f, 0.7f), 6.0f, FLinearColor(0.28f, 0.23f, 0.16f, 1.0f), 1.5f);
	FSlateBrush FrameBrush;
	FAmnesiaUIDecor Decor;
	FButtonStyle ButtonStyle;
};
