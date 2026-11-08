#include "AmnesiaShopScreen.h"
#include "AmnesiaProgress.h"
#include "AmnesiaWeapons.h"
#include "Fonts/CompositeFont.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor ShopGold(0.95f, 0.76f, 0.36f, 1.0f);
	const FLinearColor ShopBrightGold(1.0f, 0.9f, 0.55f, 1.0f);
	const FLinearColor ShopCream(0.95f, 0.9f, 0.8f, 1.0f);
	const FLinearColor ShopMuted(0.62f, 0.57f, 0.47f, 1.0f);
	const FLinearColor ShopWarning(1.0f, 0.55f, 0.3f, 1.0f);
	const FLinearColor ShopGreen(0.55f, 0.95f, 0.5f, 1.0f);

	// every row keeps this height whatever its texts say, so the centered screen does not jump
	const float ShopRowHeight = 232.0f;
}

// ---------------------------------------------------------------- what is sold

const TArray<FAmnesiaShopItem>& FAmnesiaShop::Items()
{
	static TArray<FAmnesiaShopItem> List;

	if (List.Num() == 0)
	{
		FAmnesiaShopItem& Dragon = List.AddDefaulted_GetRef();
		Dragon.Id = AmnesiaDragonItemId();
		Dragon.Name = TEXT("Követő sárkány");
		Dragon.Subtitle = TEXT("Társ  •  tűzzel harcol");
		Dragon.Detail = TEXT("Egy kis sárkány, aki a válladnál repül, és tűzgolyókat köp az ellenfeleidre.");
		Dragon.Perks = { TEXT("Tűzgolyókat lő az ellenfelekre"), TEXT("Fényt ad a sötétben"), TEXT("Új képességág: Sárkány") };
		Dragon.Price = 200;
		Dragon.LongDetail = TEXT("Egy kis sárkány, aki a válladnál repül, és tűzgolyókat köp az ellenfeleidre, amíg te harcolsz. A fénye megvilágítja a sötét barlangokat. A Sárkány képességágban erősítheted a tűzgolyóit, gyorsíthatod a tüzelését, fényesebbé teheted, és a pajzsával fel is fog egy ütést, ami neked szólt.");
		Dragon.Controls = TEXT("Nincs gombja: magától harcol melletted   •   A képességeit a T gombbal nyitható képességfán tanulhatod meg");
		Dragon.Branch = EAmnesiaSkillBranch::Dragon;

		// the weapons of the weapon table: each one opens a branch of skills
		for (const FAmnesiaWeaponDef& Weapon : FAmnesiaWeapons::All())
		{
			if (Weapon.Id == FAmnesiaWeapons::SwordId())
			{
				continue;
			}

			FAmnesiaShopItem& Item = List.AddDefaulted_GetRef();
			Item.Id = Weapon.Id;
			Item.Name = Weapon.Name;
			Item.Subtitle = Weapon.Subtitle;
			Item.Detail = Weapon.Detail;
			Item.Perks = Weapon.Perks;
			Item.Price = Weapon.Price;
			Item.LongDetail = Weapon.LongDetail;
			Item.Controls = Weapon.Controls;
			Item.Branch = Weapon.Branch;
		}
	}

	return List;
}

const FAmnesiaShopItem* FAmnesiaShop::Find(FName Id)
{
	for (const FAmnesiaShopItem& Item : Items())
	{
		if (Item.Id == Id)
		{
			return &Item;
		}
	}

	return nullptr;
}

// ---------------------------------------------------------------- the screen

FSlateFontInfo SAmnesiaShopScreen::MakeFont(bool bBold, int32 Size) const
{
	const TSharedPtr<const FCompositeFont>& Wanted = bBold ? BoldFont : RegularFont;
	const TSharedPtr<const FCompositeFont>& Other = bBold ? RegularFont : BoldFont;

	if (Wanted.IsValid() || Other.IsValid())
	{
		return FSlateFontInfo(Wanted.IsValid() ? Wanted : Other, static_cast<float>(Size));
	}

	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

FSlateFontInfo SAmnesiaShopScreen::MakeBodyFont(bool bBold, int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

void SAmnesiaShopScreen::Construct(const FArguments& InArgs)
{
	GoldGetter = InArgs._GetGold;
	OwnedGetter = InArgs._IsOwned;
	OnBuy = InArgs._OnBuy;
	OnClose = InArgs._OnClose;

	// a font face asset becomes a composite font with that one face
	auto MakeComposite = [](UObject* FontFace) -> TSharedPtr<const FCompositeFont>
	{
		if (!FontFace)
		{
			return nullptr;
		}

		TSharedRef<FStandaloneCompositeFont> Composite = MakeShared<FStandaloneCompositeFont>();
		FTypefaceEntry& Entry = Composite->DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = FName("Default");
		Entry.Font = FFontData(FontFace);

		return Composite;
	};

	RegularFont = MakeComposite(InArgs._FontRegular);
	BoldFont = MakeComposite(InArgs._FontBold);

	if (InArgs._ClickSound)
	{
		ClickSound.SetResourceObject(InArgs._ClickSound);
	}

	if (InArgs._HoverSound)
	{
		HoverSound.SetResourceObject(InArgs._HoverSound);
	}

	if (InArgs._PurchaseSound)
	{
		PurchaseSound.SetResourceObject(InArgs._PurchaseSound);
	}

	Decor.Init(InArgs._DividerTexture, InArgs._Emblems);

	// pixel art frame, like the menu's
	if (InArgs._FrameTexture)
	{
		FrameBrush.SetResourceObject(InArgs._FrameTexture);
		FrameBrush.ImageSize = FVector2D(96.0f, 96.0f);
		FrameBrush.DrawAs = ESlateBrushDrawType::Box;
		FrameBrush.Margin = FMargin(1.0f / 3.0f);
		FrameBrush.TintColor = FSlateColor(ShopGold);
	}

	ButtonStyle = FButtonStyle()
		.SetPressedSound(ClickSound)
		.SetHoveredSound(HoverSound)
		.SetNormal(ButtonNormal)
		.SetHovered(ButtonHovered)
		.SetPressed(ButtonPressed)
		.SetDisabled(ButtonDisabled)
		.SetNormalForeground(ShopCream)
		.SetHoveredForeground(ShopBrightGold)
		.SetPressedForeground(ShopGold)
		.SetDisabledForeground(ShopMuted)
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// one brush for every picture
	for (const TPair<FName, UObject*>& Pair : InArgs._Images)
	{
		if (!Pair.Value)
		{
			continue;
		}

		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Pair.Value);
		Brush->ImageSize = FVector2D(190.0f, 190.0f);
		Brush->DrawAs = ESlateBrushDrawType::Image;
		ImageBrushes.Add(Pair.Key, Brush);
	}

	// a row for everything that is sold, and a page for each of them
	const TArray<FAmnesiaShopItem>& Items = FAmnesiaShop::Items();
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		Rows->AddSlot().AutoHeight().Padding(0.0f, 5.0f)
		[
			MakeItemRow(Items[Index], Index)
		];
	}

	// every page is this high, so the centered screen does not jump when the page changes
	const bool bCompactList = Items.Num() > 2;
	const float RowStep = (bCompactList ? 160.0f + 16.0f : ShopRowHeight + 32.0f) + 10.0f;
	const float PageHeight = FMath::Max(Items.Num() * RowStep, 790.0f);

	SAssignNew(Pages, SWidgetSwitcher).WidgetIndex(0);

	Pages->AddSlot()
	[
		SNew(SBox).HeightOverride(PageHeight)
		[
			Rows
		]
	];

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		Pages->AddSlot()
		[
			SNew(SBox).HeightOverride(PageHeight)
			[
				MakeDetailPage(Items[Index], Index)
			]
		];
	}

	// the footer: what the last purchase said, and the way back
	TSharedRef<SWidget> BackWidget = MakeButton(FText::FromString(TEXT("Vissza")), FSimpleDelegate::CreateLambda([this]() { GoBack(); }), BackButton);

	TSharedRef<SHorizontalBox> Footer =
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SBox)
			.HeightOverride(30.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(MakeBodyFont(false, 16))
				.Text_Lambda([this]() -> FText
				{
					if (FPlatformTime::Seconds() < MessageUntil)
					{
						return FText::FromString(Message);
					}

					return FText::FromString(ActiveItem >= 0 ? TEXT("B vagy Esc: vissza a listához") : TEXT("B vagy Esc: vissza"));
				})
				.ColorAndOpacity_Lambda([this]() -> FSlateColor
				{
					return FSlateColor(FPlatformTime::Seconds() < MessageUntil ? MessageColor : ShopMuted);
				})
			]
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			BackWidget
		];

	TSharedRef<SWidget> Panel =
		SNew(SBox)
		.WidthOverride(1160.0f)
		[
			Decor.WithCorners(
			WithFrame(
			SNew(SBorder)
			.BorderImage(&PanelBrush)
			.Padding(FMargin(44.0f, 30.0f))
			[
				SNew(SVerticalBox)

				// title and the gold
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)

					// the title on a ribbon, and the three weapons between two dividers
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						Decor.Ribbon(
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BOLT")))
							.Font(MakeFont(true, 37))
							.ColorAndOpacity(ShopGold)
							.ShadowOffset(FVector2D(3.0f, 3.0f))
							.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f)),
							62.0f)
					]

					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(20.0f, 0.0f)
					[
						Decor.Ornament(Decor.Trio(58.0f))
					]

					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(&InnerPanelBrush)
						.Padding(FMargin(22.0f, 8.0f))
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Arany")))
								.Font(MakeFont(false, 16))
								.ColorAndOpacity(ShopCream)
							]

							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.0f, 0.0f, 0.0f, 0.0f)
							[
								MakeCoin(32.0f)
							]

							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Font(MakeFont(true, 32))
								.Text_Lambda([this]() -> FText { return FText::FromString(FString::Printf(TEXT("%d"), GetGoldNow())); })
								.ColorAndOpacity(ShopBrightGold)
							]
						]
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 6.0f)
				[
					Decor.Ornament(Decor.Diamond(11.0f))
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					Pages.ToSharedRef()
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 8.0f)
				[
					Decor.Ornament(Decor.Diamond(11.0f))
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					Footer
				]
			], 6.0f),
			64.0f, 3.0f)
		];

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&DimBrush)
		]

		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			// never bigger than the screen: a small window shrinks it
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::DownOnly)
			[
				Panel
			]
		]
	];
}

TSharedRef<SWidget> SAmnesiaShopScreen::WithFrame(TSharedRef<SWidget> Content, float Inset)
{
	if (!FrameBrush.GetResourceObject())
	{
		return Content;
	}

	return SNew(SOverlay)

		+ SOverlay::Slot()
		[
			Content
		]

		+ SOverlay::Slot().Padding(Inset)
		[
			SNew(SImage)
			.Image(&FrameBrush)
			.Visibility(EVisibility::HitTestInvisible)
		];
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakeCoin(float Size)
{
	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SImage).Image(&CoinRimBrush)
			]

			+ SOverlay::Slot().Padding(Size * 0.2f)
			[
				SNew(SImage).Image(&CoinFaceBrush)
			]
		];
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakeButton(const FText& Label, FSimpleDelegate Action, TSharedPtr<SButton>& OutButton)
{
	return SNew(SBox)
		.WidthOverride(230.0f)
		.HeightOverride(50.0f)
		[
			SAssignNew(OutButton, SButton)
			.ButtonStyle(&ButtonStyle)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([Action]()
			{
				Action.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(MakeFont(true, 18))
				.ShadowOffset(FVector2D(2.0f, 2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f))
			]
		];
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakePicture(FName Id, float Size, float Padding)
{
	const TSharedPtr<FSlateBrush>* Image = ImageBrushes.Find(Id);

	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SImage).Image(&PictureFillBrush)
			]

			+ SOverlay::Slot().Padding(Padding)
			[
				SNew(SImage).Image(Image ? Image->Get() : static_cast<const FSlateBrush*>(&PictureFillBrush))
			]

			+ SOverlay::Slot()
			[
				SNew(SImage).Image(&PictureOutlineBrush)
			]
		];
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakePricePlaque(int32 Price)
{
	// the price on a little plaque
	return SNew(SBorder)
		.BorderImage(&PlaqueBrush)
		.Padding(FMargin(22.0f, 4.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				MakeCoin(32.0f)
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), Price)))
				.Font(MakeFont(true, 34))
				.ColorAndOpacity(ShopBrightGold)
				.ShadowOffset(FVector2D(2.0f, 2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
			]
		];
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakeStatusText(FName Id, int32 Price)
{
	return SNew(SBox)
		.WidthOverride(250.0f)
		.HeightOverride(26.0f)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Font(MakeBodyFont(false, 15))
			.Text_Lambda([this, Id, Price]() -> FText
			{
				if (IsOwnedNow(Id))
				{
					return FText::FromString(TEXT("A tiéd!"));
				}

				const int32 Missing = Price - GetGoldNow();
				return Missing > 0 ? FText::FromString(FString::Printf(TEXT("Még %d arany kell"), Missing)) : FText::GetEmpty();
			})
			.ColorAndOpacity_Lambda([this, Id]() -> FSlateColor
			{
				return FSlateColor(IsOwnedNow(Id) ? ShopGreen : ShopWarning);
			})
		];
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakeHeading(const FString& Text)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.Font(MakeFont(true, 21))
		.ColorAndOpacity(ShopGold)
		.ShadowOffset(FVector2D(2.0f, 2.0f))
		.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakeItemRow(const FAmnesiaShopItem& Item, int32 Index)
{
	const FName Id = Item.Id;
	const int32 Price = Item.Price;

	// with more than two things on sale the rows are smaller, so all of them fit on the screen
	const bool bCompact = FAmnesiaShop::Items().Num() > 2;
	const float RowHeight = bCompact ? 160.0f : ShopRowHeight;
	const float Picture = bCompact ? 128.0f : 200.0f;

	// what the thing does
	TSharedRef<SVerticalBox> Perks = SNew(SVerticalBox);

	if (bCompact)
	{
		// the small rows run the perks together in one paragraph, so they take two lines and not three
		FString Joined;

		for (const FString& Perk : Item.Perks)
		{
			Joined += FString(Joined.IsEmpty() ? TEXT("•  ") : TEXT("     •  ")) + Perk;
		}

		Perks->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Joined))
			.Font(MakeBodyFont(false, 13))
			.ColorAndOpacity(ShopBrightGold)
			.AutoWrapText(true)
		];

	} else {

		for (const FString& Perk : Item.Perks)
		{
			Perks->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString(TEXT("•  ")) + Perk))
				.Font(MakeBodyFont(false, 15))
				.ColorAndOpacity(ShopBrightGold)
			];
		}
	}

	// the button that opens the page of the thing, where it can be bought
	TSharedPtr<SButton> DetailsButton;

	TSharedRef<SWidget> DetailsWidget =
		SNew(SBox)
		.WidthOverride(250.0f)
		.HeightOverride(54.0f)
		[
			SAssignNew(DetailsButton, SButton)
			.ButtonStyle(&ButtonStyle)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([this, Index]()
			{
				OpenDetails(Index);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(MakeFont(true, 18))
				.Text(FText::FromString(TEXT("RÉSZLETEK")))
				.ShadowOffset(FVector2D(2.0f, 2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f))
			]
		];

	ListButtons.Add(DetailsButton);

	return Decor.WithCorners(
		SNew(SBorder)
		.BorderImage(&InnerPanelBrush)
		.Padding(bCompact ? FMargin(18.0f, 8.0f) : FMargin(20.0f, 16.0f))
		[
			SNew(SBox)
			.HeightOverride(RowHeight)
			[
				SNew(SHorizontalBox)

				// the picture
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 26.0f, 0.0f)
				[
					MakePicture(Id, Picture, 5.0f)
				]

				// the name and what it is
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item.Name))
						.Font(MakeFont(true, 28))
						.ColorAndOpacity(ShopGold)
						.ShadowOffset(FVector2D(2.0f, 2.0f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
					]

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item.Subtitle))
						.Font(MakeBodyFont(false, bCompact ? 13 : 14))
						.ColorAndOpacity(ShopMuted)
					]

					// room for two lines, whatever the text takes
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bCompact ? 5.0f : 10.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.HeightOverride(bCompact ? 42.0f : 54.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Item.Detail))
							.Font(MakeBodyFont(false, bCompact ? 15 : 16))
							.ColorAndOpacity(ShopCream)
							.AutoWrapText(true)
						]
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bCompact ? 5.0f : 10.0f, 0.0f, 0.0f)
					[
						Perks
					]
				]

				// the price and the button to the page
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(26.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						MakePricePlaque(Price)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						DetailsWidget
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						MakeStatusText(Id, Price)
					]
				]
			]
		], 22.0f, 2.0f);
}

TSharedRef<SWidget> SAmnesiaShopScreen::MakeDetailPage(const FAmnesiaShopItem& Item, int32 Index)
{
	const FName Id = Item.Id;
	const int32 Price = Item.Price;

	// the buy button: off when the thing is bought already or the gold is not enough
	TSharedPtr<SButton> BuyButton;

	TSharedRef<SWidget> BuyWidget =
		SNew(SBox)
		.WidthOverride(250.0f)
		.HeightOverride(54.0f)
		[
			SAssignNew(BuyButton, SButton)
			.ButtonStyle(&ButtonStyle)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.IsEnabled_Lambda([this, Id, Price]() -> bool
			{
				return !IsOwnedNow(Id) && GetGoldNow() >= Price;
			})
			.OnClicked_Lambda([this, Id]()
			{
				TryBuy(Id);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(MakeFont(true, 18))
				.Text_Lambda([this, Id, Price]() -> FText
				{
					if (IsOwnedNow(Id))
					{
						return FText::FromString(TEXT("MEGVÁSÁROLVA"));
					}

					return FText::FromString(GetGoldNow() >= Price ? TEXT("MEGVESZEM") : TEXT("KEVÉS ARANY"));
				})
				.ShadowOffset(FVector2D(2.0f, 2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f))
			]
		];

	BuyButtons.Add(BuyButton);

	// what the thing does, one line each
	TSharedRef<SVerticalBox> Perks = SNew(SVerticalBox);

	for (const FString& Perk : Item.Perks)
	{
		Perks->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString(TEXT("•  ")) + Perk))
			.Font(MakeBodyFont(false, 16))
			.ColorAndOpacity(ShopBrightGold)
		];
	}

	// the skills of the branch the thing opens, from the top of the column down
	TArray<const FAmnesiaSkillDef*> BranchSkills;

	for (const FAmnesiaSkillDef& Skill : FAmnesiaSkillTree::All())
	{
		if (Skill.Branch == Item.Branch)
		{
			BranchSkills.Add(&Skill);
		}
	}

	BranchSkills.Sort([](const FAmnesiaSkillDef& A, const FAmnesiaSkillDef& B) { return A.Tier < B.Tier; });

	TSharedRef<SVerticalBox> Skills = SNew(SVerticalBox);

	for (const FAmnesiaSkillDef* Skill : BranchSkills)
	{
		const FString Meta = FString::Printf(TEXT("max %d szint   •   %d képességpont szintenként"), Skill->MaxRank, Skill->Cost);

		Skills->AddSlot().AutoHeight().Padding(0.0f, 3.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				MakePicture(Skill->Id, 46.0f, 3.0f)
			]

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Skill->Name))
						.Font(MakeFont(true, 18))
						.ColorAndOpacity(ShopCream)
					]

					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Meta))
						.Font(MakeBodyFont(false, 13))
						.ColorAndOpacity(ShopMuted)
					]
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Skill->Detail))
					.Font(MakeBodyFont(false, 13))
					.ColorAndOpacity(ShopCream)
					.AutoWrapText(true)
				]
			]
		];
	}

	return SNew(SHorizontalBox)

		// the picture, the price and the buy button
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 4.0f, 34.0f, 0.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				MakePicture(Id, 270.0f, 8.0f)
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 22.0f, 0.0f, 0.0f)
			[
				MakePricePlaque(Price)
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				BuyWidget
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				MakeStatusText(Id, Price)
			]
		]

		// the texts
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Name))
				.Font(MakeFont(true, 40))
				.ColorAndOpacity(ShopGold)
				.ShadowOffset(FVector2D(3.0f, 3.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Subtitle))
				.Font(MakeBodyFont(false, 15))
				.ColorAndOpacity(ShopMuted)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 10.0f)
			[
				Decor.Ornament(Decor.Diamond(9.0f))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.LongDetail.IsEmpty() ? Item.Detail : Item.LongDetail))
				.Font(MakeBodyFont(false, 16))
				.ColorAndOpacity(ShopCream)
				.AutoWrapText(true)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 4.0f)
			[
				MakeHeading(TEXT("Mit tud"))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				Perks
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Item.Controls.IsEmpty() ? 0.0f : 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Visibility(Item.Controls.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				.Text(FText::FromString(Item.Controls))
				.Font(MakeBodyFont(false, 15))
				.ColorAndOpacity(ShopMuted)
				.AutoWrapText(true)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 4.0f)
			[
				MakeHeading(TEXT("Képességek, amiket ezzel feloldasz"))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				Skills
			]
		];
}

void SAmnesiaShopScreen::OpenDetails(int32 ItemIndex)
{
	if (!Pages.IsValid() || !FAmnesiaShop::Items().IsValidIndex(ItemIndex))
	{
		return;
	}

	ActiveItem = ItemIndex;
	Pages->SetActiveWidgetIndex(ItemIndex + 1);
	Message.Empty();
	MessageUntil = 0.0;
	FocusCurrent();
}

void SAmnesiaShopScreen::ShowList()
{
	if (!Pages.IsValid())
	{
		return;
	}

	ActiveItem = -1;
	Pages->SetActiveWidgetIndex(0);
	FocusCurrent();
}

void SAmnesiaShopScreen::TryBuy(FName Id)
{
	const FAmnesiaShopItem* Item = FAmnesiaShop::Find(Id);

	if (!Item)
	{
		return;
	}

	if (OnBuy.IsBound() && OnBuy.Execute(Id))
	{
		FSlateApplication::Get().PlaySound(PurchaseSound);

		Message = FString::Printf(TEXT("Megvetted: %s!"), *Item->Name);
		MessageColor = ShopGreen;
		MessageUntil = FPlatformTime::Seconds() + 8.0;

		// the buy button is off now, so the keyboard goes to the way back
		if (BackButton.IsValid())
		{
			FSlateApplication::Get().SetUserFocus(0, BackButton, EFocusCause::SetDirectly);
		}

	} else {

		Message = IsOwnedNow(Id) ? TEXT("Ezt már megvetted.") : TEXT("Nincs elég arany.");
		MessageColor = ShopWarning;
		MessageUntil = FPlatformTime::Seconds() + 3.0;
	}
}

void SAmnesiaShopScreen::FocusFirst()
{
	// once now, and once more after the input mode of the game has had its say
	RegisterActiveTimer(0.2f, FWidgetActiveTimerDelegate::CreateSP(this, &SAmnesiaShopScreen::FocusTimer));
	FocusTimer(0.0, 0.0f);
}

EActiveTimerReturnType SAmnesiaShopScreen::FocusTimer(double InCurrentTime, float InDeltaTime)
{
	FocusCurrent();
	return EActiveTimerReturnType::Stop;
}

void SAmnesiaShopScreen::FocusCurrent()
{
	// the first thing to open on the list, or the buy button of the open page if it can be used
	TSharedPtr<SButton> Wanted;

	if (ActiveItem < 0)
	{
		Wanted = ListButtons.Num() > 0 ? ListButtons[0] : nullptr;

	} else if (BuyButtons.IsValidIndex(ActiveItem)) {

		Wanted = BuyButtons[ActiveItem];
	}

	if (Wanted.IsValid() && Wanted->IsEnabled())
	{
		FSlateApplication::Get().SetUserFocus(0, Wanted, EFocusCause::SetDirectly);

	} else if (BackButton.IsValid()) {

		FSlateApplication::Get().SetUserFocus(0, BackButton, EFocusCause::SetDirectly);
	}
}

void SAmnesiaShopScreen::GoBack()
{
	// from a page the way back leads to the list, from the list out of the shop
	if (ActiveItem >= 0)
	{
		ShowList();
		return;
	}

	OnClose.ExecuteIfBound();
}

FReply SAmnesiaShopScreen::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape || Key == EKeys::B || Key == EKeys::P || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		GoBack();
		return FReply::Handled();
	}

	// no button has the focus: Enter buys the thing on the page, or opens the page of the first thing on the list
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		const TArray<FAmnesiaShopItem>& Items = FAmnesiaShop::Items();

		if (ActiveItem >= 0 && Items.IsValidIndex(ActiveItem))
		{
			TryBuy(Items[ActiveItem].Id);
			return FReply::Handled();
		}

		if (Items.Num() > 0)
		{
			OpenDetails(0);
			return FReply::Handled();
		}
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
