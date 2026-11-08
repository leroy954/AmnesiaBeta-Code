#include "AmnesiaMenu.h"
#include "AmnesiaProgress.h"
#include "Fonts/CompositeFont.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor Gold(0.95f, 0.76f, 0.36f, 1.0f);
	const FLinearColor BrightGold(1.0f, 0.9f, 0.55f, 1.0f);
	const FLinearColor Cream(0.95f, 0.9f, 0.8f, 1.0f);
	const FLinearColor Muted(0.6f, 0.55f, 0.45f, 1.0f);

	FSlateFontInfo MenuFont(const char* Style, int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Style, Size);
	}
}

// ---------------------------------------------------------------- the shared decoration

void FAmnesiaUIDecor::Init(UObject* DividerTexture, const FAmnesiaUIIcons& Icons)
{
	// the faded line stretches, the ornament at its end keeps its shape
	if (DividerTexture)
	{
		DividerBrush.SetResourceObject(DividerTexture);
		DividerBrush.ImageSize = FVector2D(192.0f, 36.0f);
		DividerBrush.DrawAs = ESlateBrushDrawType::Box;
		DividerBrush.Margin = FMargin(0.0f, 0.0f, 0.3f, 0.0f);
		DividerBrush.TintColor = FSlateColor(Gold);
	}

	// one brush for every picture
	for (const TPair<FName, UObject*>& Pair : Icons)
	{
		if (!Pair.Value)
		{
			continue;
		}

		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Pair.Value);
		Brush->ImageSize = FVector2D(96.0f, 96.0f);
		Brush->DrawAs = ESlateBrushDrawType::Image;
		IconBrushes.Add(Pair.Key, Brush);
	}
}

const FSlateBrush* FAmnesiaUIDecor::FindIcon(FName Id) const
{
	const TSharedPtr<FSlateBrush>* Found = IconBrushes.Find(Id);
	return Found ? Found->Get() : nullptr;
}

TSharedRef<SWidget> FAmnesiaUIDecor::Diamond(float Size) const
{
	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		.RenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(45.0f))))
		.RenderTransformPivot(FVector2D(0.5f, 0.5f))
		[
			SNew(SImage).Image(&GoldBrush)
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::Corners(float Size, float Inset) const
{
	// one corner, top left: a diamond in a ring, and a line along each edge that ends in a small diamond
	auto Piece = [this, Size](float FlipX, float FlipY) -> TSharedRef<SWidget>
	{
		const float Center = Size * 0.26f;
		const float Ring = Size * 0.36f;
		const float Core = Size * 0.22f;
		const float ArmStart = Center + Core * 0.9f;
		const float ArmLength = FMath::Max(Size - ArmStart - 4.0f, 4.0f);
		const FSlateRenderTransform Turn(FQuat2D(FMath::DegreesToRadians(45.0f)));

		return SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			.RenderTransform(FSlateRenderTransform(FScale2D(FlipX, FlipY)))
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			[
				SNew(SOverlay)

				// the lines
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(ArmStart, Center - 1.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(ArmLength).HeightOverride(2.0f)[ SNew(SImage).Image(&GoldBrush) ]
				]

				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Center - 1.0f, ArmStart, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(2.0f).HeightOverride(ArmLength)[ SNew(SImage).Image(&GoldBrush) ]
				]

				// the small diamonds at their ends
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Size - 6.0f, Center - 3.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(6.0f).HeightOverride(6.0f).RenderTransform(Turn).RenderTransformPivot(FVector2D(0.5f, 0.5f))[ SNew(SImage).Image(&BrightGoldBrush) ]
				]

				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Center - 3.0f, Size - 6.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(6.0f).HeightOverride(6.0f).RenderTransform(Turn).RenderTransformPivot(FVector2D(0.5f, 0.5f))[ SNew(SImage).Image(&BrightGoldBrush) ]
				]

				// the ring and the diamond in it
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Center - Ring * 0.5f, Center - Ring * 0.5f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(Ring).HeightOverride(Ring).RenderTransform(Turn).RenderTransformPivot(FVector2D(0.5f, 0.5f))[ SNew(SImage).Image(&CornerRingBrush) ]
				]

				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Center - Core * 0.5f, Center - Core * 0.5f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(Core).HeightOverride(Core).RenderTransform(Turn).RenderTransformPivot(FVector2D(0.5f, 0.5f))[ SNew(SImage).Image(&BrightGoldBrush) ]
				]
			];
	};

	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)

		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Inset)
		[
			Piece(1.0f, 1.0f)
		]

		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(Inset)
		[
			Piece(-1.0f, 1.0f)
		]

		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(Inset)
		[
			Piece(1.0f, -1.0f)
		]

		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Inset)
		[
			Piece(-1.0f, -1.0f)
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::WithCorners(TSharedRef<SWidget> Content, float Size, float Inset) const
{
	return SNew(SOverlay)

		+ SOverlay::Slot()
		[
			Content
		]

		+ SOverlay::Slot()
		[
			Corners(Size, Inset)
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::Medallion(FName Icon, float Size, bool bRound, TAttribute<FLinearColor> Tint) const
{
	const FSlateBrush* Picture = FindIcon(Icon);
	const float Inset = Size * (bRound ? 0.17f : 0.09f);

	TSharedRef<SOverlay> Content = SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SImage).Image(bRound ? static_cast<const FSlateBrush*>(&MedalFillBrush) : static_cast<const FSlateBrush*>(&PlaqueFillBrush))
		];

	if (Picture)
	{
		Content->AddSlot().Padding(Inset)
		[
			SNew(SImage).Image(Picture)
		];

	} else {

		// no picture: a diamond holds the place
		Content->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			Diamond(Size * 0.3f)
		];
	}

	Content->AddSlot()
	[
		SNew(SImage).Image(bRound ? static_cast<const FSlateBrush*>(&MedalRingBrush) : static_cast<const FSlateBrush*>(&PlaqueRingBrush))
	];

	// a thin second ring a little inside the gold one
	if (bRound)
	{
		Content->AddSlot().Padding(Size * 0.09f)
		[
			SNew(SImage).Image(&MedalInnerRingBrush)
		];
	}

	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.Padding(FMargin(0.0f))
			.ColorAndOpacity(Tint)
			[
				Content
			]
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::Trio(float Size) const
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Medallion(FName("chakram"), Size * 0.74f, true)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f)
		[
			Medallion(FName("axe"), Size, true)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Medallion(FName("wand"), Size * 0.74f, true)
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::Ribbon(TSharedRef<SWidget> Content, float Height) const
{
	// the pointed ends are squares turned on their corner, half behind the body of the ribbon
	const float Tip = Height * 0.7f;
	const float Half = Tip * 0.5f;
	const FSlateRenderTransform Turn(FQuat2D(FMath::DegreesToRadians(45.0f)));

	return SNew(SBox)
		.Padding(FMargin(Tip * 0.21f, 0.0f))
		[
			SNew(SOverlay)

			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(Tip).HeightOverride(Tip).RenderTransform(Turn).RenderTransformPivot(FVector2D(0.5f, 0.5f))[ SNew(SImage).Image(&RibbonTipBrush) ]
			]

			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(Tip).HeightOverride(Tip).RenderTransform(Turn).RenderTransformPivot(FVector2D(0.5f, 0.5f))[ SNew(SImage).Image(&RibbonTipBrush) ]
			]

			+ SOverlay::Slot().Padding(Half, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(&RibbonBrush)
				.Padding(FMargin(16.0f, 0.0f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SBox).MinDesiredHeight(Height)[ Content ]
				]
			]

			// a thin gold line inside the body
			+ SOverlay::Slot().Padding(Half + 7.0f, 6.0f)
			[
				SNew(SImage).Image(&RibbonLineBrush).Visibility(EVisibility::HitTestInvisible)
			]
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::Ornament(TSharedRef<SWidget> Center) const
{
	// two ornate dividers pointing at the center
	if (DividerBrush.GetResourceObject())
	{
		return SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(36.0f)[ SNew(SImage).Image(&DividerBrush) ]
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f)
			[
				Center
			]

			// the right one is the same art mirrored
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.HeightOverride(36.0f)
				.RenderTransform(FSlateRenderTransform(FScale2D(-1.0f, 1.0f)))
				.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				[
					SNew(SImage).Image(&DividerBrush)
				]
			];
	}

	// a gold line with the center in the middle
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SBox).HeightOverride(2.0f)[ SNew(SImage).Image(&FaintGoldBrush) ]
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f)
		[
			Center
		]

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SBox).HeightOverride(2.0f)[ SNew(SImage).Image(&FaintGoldBrush) ]
		];
}

TSharedRef<SWidget> FAmnesiaUIDecor::Keycap(const FString& Key, const FSlateFontInfo& Font) const
{
	return SNew(SBorder)
		.BorderImage(&KeycapBrush)
		.Padding(FMargin(9.0f, 2.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Key))
			.Font(Font)
			.ColorAndOpacity(BrightGold)
		];
}

// ---------------------------------------------------------------- the menu

FSlateFontInfo SAmnesiaMenu::MakeFont(bool bBold, const char* DefaultStyle, int32 Size) const
{
	const TSharedPtr<const FCompositeFont>& Wanted = bBold ? BoldFont : RegularFont;

	if (Wanted.IsValid())
	{
		return FSlateFontInfo(Wanted, static_cast<float>(Size));
	}

	return FCoreStyle::GetDefaultFontStyle(DefaultStyle, Size);
}

void SAmnesiaMenu::Construct(const FArguments& InArgs)
{
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

	Mode = InArgs._Mode;
	Settings = InArgs._Settings;
	OnNewGame = InArgs._OnNewGame;
	OnContinue = InArgs._OnContinue;
	OnResume = InArgs._OnResume;
	OnSkills = InArgs._OnSkills;
	OnShop = InArgs._OnShop;
	OnMainMenu = InArgs._OnMainMenu;
	OnQuit = InArgs._OnQuit;
	OnSettingsChanged = InArgs._OnSettingsChanged;

	// pixel art frame, drawn at twice its size with crisp 9-slice corners
	if (InArgs._FrameTexture)
	{
		FrameBrush.SetResourceObject(InArgs._FrameTexture);
		FrameBrush.ImageSize = FVector2D(96.0f, 96.0f);
		FrameBrush.DrawAs = ESlateBrushDrawType::Box;
		FrameBrush.Margin = FMargin(1.0f / 3.0f);
		FrameBrush.TintColor = FSlateColor(Gold);

		// the ornate frame takes over, so the panel outline only darkens the edge
		PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.025f, 0.02f, 0.94f), 10.0f, FLinearColor(0.3f, 0.22f, 0.1f, 1.0f), 1.5f);
	}

	Decor.Init(InArgs._DividerTexture, InArgs._Emblems);

	FSlateSound ClickSound;
	FSlateSound HoverSound;

	if (InArgs._ClickSound)
	{
		ClickSound.SetResourceObject(InArgs._ClickSound);
	}

	if (InArgs._HoverSound)
	{
		HoverSound.SetResourceObject(InArgs._HoverSound);
	}

	ButtonStyle = FButtonStyle()
		.SetPressedSound(ClickSound)
		.SetHoveredSound(HoverSound)
		.SetNormal(ButtonNormal)
		.SetHovered(ButtonHovered)
		.SetPressed(ButtonPressed)
		.SetDisabled(ButtonDisabled)
		.SetNormalForeground(Cream)
		.SetHoveredForeground(BrightGold)
		.SetPressedForeground(Gold)
		.SetDisabledForeground(Muted)
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// the graphics choices are toggle buttons that look like the menu's buttons, the chosen one filled gold
	QualityStyle = FCheckBoxStyle()
		.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
		.SetUncheckedImage(ButtonNormal)
		.SetUncheckedHoveredImage(ButtonHovered)
		.SetUncheckedPressedImage(ButtonPressed)
		.SetCheckedImage(ButtonSelected)
		.SetCheckedHoveredImage(ButtonSelectedHovered)
		.SetCheckedPressedImage(ButtonSelected)
		.SetForegroundColor(Cream)
		.SetHoveredForegroundColor(BrightGold)
		.SetPressedForegroundColor(Gold)
		.SetCheckedForegroundColor(BrightGold)
		.SetCheckedHoveredForegroundColor(BrightGold)
		.SetCheckedPressedForegroundColor(BrightGold)
		.SetPadding(FMargin(4.0f, 0.0f))
		.SetCheckedSound(ClickSound)
		.SetUncheckedSound(ClickSound)
		.SetHoveredSound(HoverSound);

	const bool bMain = Mode == EAmnesiaMenuMode::Main;
	const float CrestSize = bMain ? 80.0f : 68.0f;

	DimBrush = FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, bMain ? 0.35f : 0.55f));

	// the button list of the first page
	TSharedRef<SVerticalBox> Buttons = SNew(SVerticalBox);

	auto AddButton = [this, &Buttons](const TCHAR* Label, FSimpleDelegate Action, bool bEnabled)
	{
		Buttons->AddSlot().AutoHeight().Padding(0.0f, 7.0f)
		[
			MakeButton(FText::FromString(Label), Action, bEnabled)
		];
	};

	if (bMain)
	{
		AddButton(TEXT("Folytatás"), OnContinue, InArgs._bCanContinue);
		AddButton(TEXT("Új kaland"), OnNewGame, true);

	} else {

		AddButton(TEXT("Folytatás"), OnResume, true);
		AddButton(TEXT("Képességek"), OnSkills, true);
		AddButton(TEXT("Bolt"), OnShop, true);
	}

	AddButton(TEXT("Beállítások"), FSimpleDelegate::CreateLambda([this]() { ShowPage(1); }), true);

	if (!bMain)
	{
		AddButton(TEXT("Főmenü"), OnMainMenu, true);
	}

	AddButton(TEXT("Kilépés"), OnQuit, true);

	// the framed panel
	TSharedRef<SWidget> Panel =
		SNew(SBox)
		.WidthOverride(540.0f)
		[
			Decor.WithCorners(
			WithFrame(
			SNew(SBorder)
			.BorderImage(&PanelBrush)
			.Padding(FMargin(44.0f, 34.0f))
			[
				SNew(SVerticalBox)

				// the three weapons in medallions, and the title on a ribbon that starts behind them
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SOverlay)

					+ SOverlay::Slot().Padding(0.0f, CrestSize * 0.8f, 0.0f, 0.0f)
					[
						Decor.Ribbon(
							SNew(STextBlock)
							.Text(FText::FromString(bMain ? TEXT("AMNESIA") : TEXT("SZÜNET")))
							.Font(MakeFont(true, "Bold", bMain ? 50 : 46))
							.ColorAndOpacity(Gold)
							.ShadowOffset(FVector2D(3.0f, 3.0f))
							.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f)),
							bMain ? 86.0f : 72.0f)
					]

					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top)
					[
						Decor.Trio(CrestSize)
					]
				]

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(bMain ? TEXT("Az elfeledett erdő krónikája") : TEXT("Pihenj meg egy pillanatra")))
					.Font(MakeFont(false, "Italic", 17))
					.ColorAndOpacity(Cream)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 20.0f)
				[
					MakeOrnament()
				]

				// pages: the buttons and the settings
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(Switcher, SWidgetSwitcher)
					.WidgetIndex(0)

					+ SWidgetSwitcher::Slot()
					[
						Buttons
					]

					+ SWidgetSwitcher::Slot()
					[
						MakeSettingsPage()
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 0.0f)
				[
					MakeOrnament(true)
				]

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Egér, nyilak vagy kontroller  •  Enter: kiválaszt")))
					.Font(MenuFont("Regular", 12))
					.ColorAndOpacity(Muted)
				]
			], 6.0f),
			52.0f, 3.0f)
		];

	// the pause menu also lists the controls next to the panel
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Panel
		];

	if (!bMain)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(28.0f, 0.0f, 0.0f, 0.0f)
		[
			MakeControlsPanel(InArgs._ControlLines)
		];
	}

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&DimBrush)
		]

		+ SOverlay::Slot()
		.HAlign(bMain ? HAlign_Left : HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(bMain ? FMargin(120.0f, 0.0f, 0.0f, 0.0f) : FMargin(0.0f))
		[
			Row
		]
	];
}

TSharedRef<SWidget> SAmnesiaMenu::MakeButton(const FText& Label, FSimpleDelegate Action, bool bEnabled)
{
	TSharedPtr<SButton> Button;

	TSharedRef<SWidget> Widget =
		SNew(SBox)
		.HeightOverride(58.0f)
		[
			SAssignNew(Button, SButton)
			.ButtonStyle(&ButtonStyle)
			.IsEnabled(bEnabled)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([Action]()
			{
				Action.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(SOverlay)

				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(MakeFont(true, "Bold", 22))
					.ShadowOffset(FVector2D(2.0f, 2.0f))
					.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f))
				]

				// a small diamond at each end
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(18.0f, 0.0f, 0.0f, 0.0f)
				[
					Decor.Diamond(7.0f)
				]

				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
				[
					Decor.Diamond(7.0f)
				]
			]
		];

	if (Switcher.IsValid() && Switcher->GetActiveWidgetIndex() == 1)
	{
		SettingsBackButton = Button;

	} else {

		MainButtons.Add(Button);
	}

	return Widget;
}

TSharedRef<SWidget> SAmnesiaMenu::WithFrame(TSharedRef<SWidget> Content, float Inset)
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

		+ SOverlay::Slot()
		.Padding(Inset)
		[
			SNew(SImage)
			.Image(&FrameBrush)
			.Visibility(EVisibility::HitTestInvisible)
		];
}

TSharedRef<SWidget> SAmnesiaMenu::MakeOrnament(bool bWithIcons)
{
	if (!bWithIcons)
	{
		return Decor.Ornament(Decor.Diamond(12.0f));
	}

	// a fire and an ice skill on both sides of the diamond
	return Decor.Ornament(
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Decor.Medallion(FName("fire"), 34.0f, false)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f)
		[
			Decor.Diamond(12.0f)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Decor.Medallion(FName("ice"), 34.0f, false)
		]);
}

TSharedRef<SWidget> SAmnesiaMenu::MakeCheckRow(const FText& Label, bool* Value)
{
	TWeakObjectPtr<UAmnesiaSaveGame> WeakSettings = Settings;

	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(Label).Font(MenuFont("Regular", 19)).ColorAndOpacity(Cream)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([WeakSettings, Value]()
			{
				return WeakSettings.IsValid() && *Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, WeakSettings, Value](ECheckBoxState State)
			{
				if (WeakSettings.IsValid())
				{
					*Value = State == ECheckBoxState::Checked;
					OnSettingsChanged.ExecuteIfBound();
				}
			})
		];
}

TSharedRef<SWidget> SAmnesiaMenu::MakeQualityRow()
{
	TWeakObjectPtr<UAmnesiaSaveGame> WeakSettings = Settings;

	static const TCHAR* Names[] = { TEXT("Alacsony"), TEXT("Közepes"), TEXT("Magas"), TEXT("Max") };

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

	for (int32 Quality = 0; Quality < static_cast<int32>(UE_ARRAY_COUNT(Names)); ++Quality)
	{
		Row->AddSlot().FillWidth(1.0f).Padding(Quality > 0 ? 6.0f : 0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.HeightOverride(40.0f)
			[
				SNew(SCheckBox)
				.Style(&QualityStyle)
				.IsChecked_Lambda([WeakSettings, Quality]()
				{
					return WeakSettings.IsValid() && WeakSettings->GraphicsQuality == Quality ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, WeakSettings, Quality](ECheckBoxState)
				{
					// clicking the chosen one again keeps it chosen
					if (WeakSettings.IsValid())
					{
						WeakSettings->GraphicsQuality = Quality;
						OnSettingsChanged.ExecuteIfBound();
					}
				})
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Names[Quality])).Font(MenuFont("Bold", 16))
					]
				]
			]
		];
	}

	return Row;
}

TSharedRef<SWidget> SAmnesiaMenu::MakeSettingsPage()
{
	UAmnesiaSaveGame* Save = Settings.Get();
	TWeakObjectPtr<UAmnesiaSaveGame> WeakSettings = Settings;

	TSharedRef<SVerticalBox> Page = SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("BEÁLLÍTÁSOK")))
				.Font(MakeFont(true, "Bold", 24))
				.ColorAndOpacity(Gold)
			]

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(16.0f, 0.0f, 0.0f, 0.0f)
			[
				Decor.Ornament(Decor.Diamond(9.0f))
			]
		]

		// volume
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(140.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Hangerő"))).Font(MenuFont("Regular", 19)).ColorAndOpacity(Cream)
				]
			]

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				// the arrow keys and the d-pad move it straight away, in steps of 5%. The mouse drags it smoothly
				.RequiresControllerLock(false)
				.StepSize(0.05f)
				.Value_Lambda([WeakSettings]() { return WeakSettings.IsValid() ? WeakSettings->MasterVolume : 0.8f; })
				.SliderBarColor(FLinearColor(0.35f, 0.27f, 0.12f))
				.SliderHandleColor(Gold)
				.OnValueChanged_Lambda([this, WeakSettings](float Value)
				{
					if (WeakSettings.IsValid())
					{
						WeakSettings->MasterVolume = Value;
						OnSettingsChanged.ExecuteIfBound();
					}
				})
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(54.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([WeakSettings]()
					{
						return FText::FromString(FString::Printf(TEXT("%d%%"), WeakSettings.IsValid() ? FMath::RoundToInt(WeakSettings->MasterVolume * 100.0f) : 80));
					})
					.Font(MenuFont("Bold", 18))
					.ColorAndOpacity(BrightGold)
				]
			]
		]

		// graphics quality
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Grafika"))).Font(MenuFont("Regular", 19)).ColorAndOpacity(Cream)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			MakeQualityRow()
		];

	if (Save)
	{
		Page->AddSlot().AutoHeight().Padding(0.0f, 8.0f)
		[
			MakeCheckRow(FText::FromString(TEXT("Képernyő rázkódás")), &Save->bScreenShake)
		];

		Page->AddSlot().AutoHeight().Padding(0.0f, 8.0f)
		[
			MakeCheckRow(FText::FromString(TEXT("Sebzés számok")), &Save->bDamageNumbers)
		];
	}

	TSharedPtr<SButton> Back;

	Page->AddSlot().AutoHeight().Padding(0.0f, 22.0f, 0.0f, 0.0f)
	[
		SNew(SBox)
		.HeightOverride(52.0f)
		[
			SAssignNew(Back, SButton)
			.ButtonStyle(&ButtonStyle)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([this]()
			{
				ShowPage(0);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Vissza"))).Font(MenuFont("Bold", 20))
			]
		]
	];

	SettingsBackButton = Back;

	return Page;
}

TSharedRef<SWidget> SAmnesiaMenu::MakeControlsPanel(const FAmnesiaControlLines& Lines)
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 14.0f)
		[
			Decor.Ribbon(
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("IRÁNYÍTÁS")))
				.Font(MakeFont(true, "Bold", 22))
				.ColorAndOpacity(Gold)
				.ShadowOffset(FVector2D(2.0f, 2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f)),
				40.0f)
		];

	// the keys that start a skill or a screen get its picture
	static const TCHAR* IconKeys[] = { TEXT("Q"), TEXT("E"), TEXT("X"), TEXT("T") };
	static const TCHAR* IconNames[] = { TEXT("fire"), TEXT("ice"), TEXT("axe"), TEXT("steel") };

	int32 Row = 0;

	for (const TPair<FString, FString>& Line : Lines)
	{
		FName IconName = NAME_None;

		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(IconKeys)); ++Index)
		{
			if (Line.Key == IconKeys[Index])
			{
				IconName = FName(IconNames[Index]);
			}
		}

		// the lines are lit and dark in turn, so the eye keeps its row
		List->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(&RowBrush)
			.BorderBackgroundColor(Row % 2 == 0 ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) : FLinearColor(1.0f, 1.0f, 1.0f, 0.0f))
			.Padding(FMargin(8.0f, 4.0f))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(26.0f).HeightOverride(26.0f)
					[
						IconName.IsNone() ? SNullWidget::NullWidget : Decor.Medallion(IconName, 26.0f, false)
					]
				]

				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(176.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().AutoWidth()
						[
							Decor.Keycap(Line.Key, MenuFont("Bold", 15))
						]
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Line.Value)).Font(MenuFont("Regular", 16)).ColorAndOpacity(Cream)
				]
			]
		];

		++Row;
	}

	return SNew(SBox)
		.WidthOverride(500.0f)
		[
			Decor.WithCorners(
				WithFrame(
				SNew(SBorder)
				.BorderImage(&InnerPanelBrush)
				.Padding(FMargin(26.0f, 24.0f))
				[
					List
				], 4.0f),
				34.0f, 3.0f)
		];
}

void SAmnesiaMenu::ShowPage(int32 Page)
{
	if (!Switcher.IsValid())
	{
		return;
	}

	Switcher->SetActiveWidgetIndex(Page);

	if (Page == 1 && SettingsBackButton.IsValid())
	{
		FSlateApplication::Get().SetAllUserFocus(SettingsBackButton, EFocusCause::SetDirectly);

	} else {

		FocusFirstButton();
	}
}

void SAmnesiaMenu::FocusFirstButton()
{
	for (const TSharedPtr<SButton>& Button : MainButtons)
	{
		if (Button.IsValid() && Button->IsEnabled())
		{
			FSlateApplication::Get().SetAllUserFocus(Button, EFocusCause::SetDirectly);
			return;
		}
	}

	FSlateApplication::Get().SetAllUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

void SAmnesiaMenu::GoBack()
{
	if (Switcher.IsValid() && Switcher->GetActiveWidgetIndex() == 1)
	{
		ShowPage(0);

	} else if (Mode == EAmnesiaMenuMode::Pause) {

		OnResume.ExecuteIfBound();
	}
}

FReply SAmnesiaMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape || Key == EKeys::P || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right)
	{
		GoBack();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
