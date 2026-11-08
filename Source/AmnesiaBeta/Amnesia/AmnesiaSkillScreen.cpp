#include "AmnesiaSkillScreen.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor SkillGold(0.95f, 0.76f, 0.36f, 1.0f);
	const FLinearColor SkillBrightGold(1.0f, 0.9f, 0.55f, 1.0f);
	const FLinearColor SkillCream(0.95f, 0.9f, 0.8f, 1.0f);
	const FLinearColor SkillMuted(0.62f, 0.57f, 0.47f, 1.0f);
	const FLinearColor SkillWarning(1.0f, 0.55f, 0.3f, 1.0f);
	const FLinearColor SkillLocked(0.22f, 0.19f, 0.17f, 1.0f);

	// every font of the screen is this much smaller than the sizes in the layout, so the texts stay on one line
	const float SkillFontScale = 0.8f;

	// the details panel keeps this height for every skill, so the centered screen does not jump when a text takes one more line
	const float SkillDetailsHeight = 184.0f;
}

// ---------------------------------------------------------------- the padlock

void SAmnesiaPadlock::Construct(const FArguments& InArgs)
{
	const float Height = InArgs._Size;
	const float Width = Height * 0.8f;
	const float BodyHeight = Height * 0.56f;
	const float ShackleWidth = Width * 0.64f;
	const float ShackleHeight = Height * 0.72f;
	const float Stroke = FMath::Max(Height * 0.12f, 2.0f);

	ShackleBrush = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), ShackleWidth * 0.5f, InArgs._Color, Stroke);
	BodyBrush = FSlateRoundedBoxBrush(InArgs._Color, Height * 0.1f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
	KeyholeBrush = FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.03f, 0.03f, 1.0f), Stroke * 0.6f, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(Width)
		.HeightOverride(Height)
		[
			SNew(SOverlay)

			// the shackle: an outlined capsule, its lower half hides behind the body
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top)
			[
				SNew(SBox)
				.WidthOverride(ShackleWidth)
				.HeightOverride(ShackleHeight)
				[
					SNew(SImage).Image(&ShackleBrush)
				]
			]

			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBox)
				.HeightOverride(BodyHeight)
				[
					SNew(SImage).Image(&BodyBrush)
				]
			]

			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, BodyHeight * 0.22f)
			[
				SNew(SBox)
				.WidthOverride(Stroke * 1.3f)
				.HeightOverride(BodyHeight * 0.4f)
				[
					SNew(SImage).Image(&KeyholeBrush)
				]
			]
		]
	];
}

// ---------------------------------------------------------------- one skill

void SAmnesiaSkillNode::Construct(const FArguments& InArgs)
{
	SkillId = InArgs._SkillId;
	Skills = InArgs._Skills;
	BranchColor = InArgs._BranchColor;
	ClickSound = InArgs._ClickSound;
	HoverSound = InArgs._HoverSound;
	OnSelected = InArgs._OnSelected;
	OnActivate = InArgs._OnActivate;

	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SkillId);
	const int32 MaxRank = Def ? Def->MaxRank : 1;
	const bool bElement = Def && Def->Kind == EAmnesiaSkillKind::Element;
	const bool bActive = Def && Def->Kind == EAmnesiaSkillKind::Active;
	const FLinearColor Branch = BranchColor;

	// a pip for every rank
	TSharedRef<SHorizontalBox> Pips = SNew(SHorizontalBox);

	for (int32 Index = 0; Index < MaxRank; ++Index)
	{
		Pips->AddSlot().AutoWidth().Padding(2.5f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(13.0f)
			.HeightOverride(7.0f)
			[
				SNew(SImage)
				.Image(InArgs._PipBrush)
				.ColorAndOpacity_Lambda([this, Index, Branch]() -> FSlateColor
				{
					const bool bFilled = Skills.IsValid() && Skills->GetRank(SkillId) > Index;
					return FSlateColor(bFilled ? Branch : FLinearColor(0.17f, 0.15f, 0.13f, 1.0f));
				})
			]
		];
	}

	const FSlateFontInfo BadgeFont = InArgs._BadgeFont;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(92.0f)
			.HeightOverride(92.0f)
			[
				SNew(SOverlay)

				// the dark plate under the icon
				+ SOverlay::Slot()
				[
					SNew(SImage).Image(InArgs._FillBrush).Visibility(EVisibility::HitTestInvisible)
				]

				// the icon: dim while locked
				+ SOverlay::Slot().Padding(6.0f)
				[
					SNew(SImage)
					.Image(InArgs._IconBrush)
					.Visibility(EVisibility::HitTestInvisible)
					.ColorAndOpacity_Lambda([this]() -> FSlateColor
					{
						switch (GetState())
						{
						case EState::Locked:
							return FSlateColor(FLinearColor(0.22f, 0.22f, 0.25f, 1.0f));

						case EState::Available:
							return FSlateColor(FLinearColor(0.68f, 0.68f, 0.68f, 1.0f));

						default:
							return FSlateColor(FLinearColor::White);
						}
					})
				]

				// the frame: the color of the branch once the skill is learned, gold at its top rank
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(InArgs._OutlineBrush)
					.Visibility(EVisibility::HitTestInvisible)
					.ColorAndOpacity_Lambda([this, Branch]() -> FSlateColor
					{
						switch (GetState())
						{
						case EState::Locked:
							return FSlateColor(SkillLocked);

						case EState::Available:
							return FSlateColor(Branch * 0.55f + FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));

						case EState::Learned:
							return FSlateColor(Branch);

						default:
							return FSlateColor(SkillBrightGold);
						}
					})
				]

				// the ring that shows which skill is looked at
				+ SOverlay::Slot().Padding(-3.0f)
				[
					SNew(SImage)
					.Image(InArgs._OutlineBrush)
					.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.95f)))
					.Visibility_Lambda([this]() -> EVisibility
					{
						return IsHovered() || HasKeyboardFocus() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
				]

				// the padlock of a branch that is not open yet
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SAmnesiaPadlock)
						.Size(36.0f)
						.Color(FLinearColor(0.82f, 0.77f, 0.66f, 0.95f))
						.Visibility_Lambda([this]() -> EVisibility
						{
							const FAmnesiaSkillDef* LockedDef = FAmnesiaSkillTree::Find(SkillId);
							return Skills.IsValid() && LockedDef && Skills->IsBranchLocked(LockedDef->Branch) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
						})
					]

					// the key of an active skill
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 5.0f, 4.0f)
				[
					SNew(STextBlock)
					.Visibility(bActive ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
					.Text(FText::FromString(SkillId == FName("fire_burst") ? TEXT("Q") : TEXT("E")))
					.Font(BadgeFont)
					.ColorAndOpacity(FSlateColor(SkillBrightGold))
					.ShadowOffset(FVector2D(1.5f, 1.5f))
					.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
				]

				// the element that is on the sword
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("AKTÍV")))
					.Font(BadgeFont)
					.ColorAndOpacity(FSlateColor(SkillBrightGold))
					.ShadowOffset(FVector2D(1.5f, 1.5f))
					.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
					.Visibility_Lambda([this, bElement]() -> EVisibility
					{
						if (!bElement || !Skills.IsValid() || Skills->GetRank(SkillId) <= 0)
						{
							return EVisibility::Collapsed;
						}

						const EAmnesiaElement Mine = SkillId == FName("fire_blade") ? EAmnesiaElement::Fire : EAmnesiaElement::Ice;
						return Skills->GetElement() == Mine ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
				]
			]
		]

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 7.0f, 0.0f, 0.0f)
		[
			Pips
		]
	];
}

SAmnesiaSkillNode::EState SAmnesiaSkillNode::GetState() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SkillId);

	if (!Def || !Skills.IsValid())
	{
		return EState::Locked;
	}

	// a branch that is not open yet shows all its skills locked
	if (Skills->IsBranchLocked(Def->Branch))
	{
		return EState::Locked;
	}

	const int32 Rank = Skills->GetRank(SkillId);

	if (Rank >= Def->MaxRank)
	{
		return EState::Maxed;
	}

	if (Rank > 0)
	{
		return EState::Learned;
	}

	return Def->Requires.IsNone() || Skills->GetRank(Def->Requires) > 0 ? EState::Available : EState::Locked;
}

FReply SAmnesiaSkillNode::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	OnSelected.ExecuteIfBound(SkillId);
	return FReply::Handled();
}

void SAmnesiaSkillNode::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

	FSlateApplication::Get().PlaySound(HoverSound);
	OnSelected.ExecuteIfBound(SkillId);
}

FReply SAmnesiaSkillNode::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FSlateApplication::Get().PlaySound(ClickSound);
		OnActivate.ExecuteIfBound(SkillId);

		return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
	}

	return FReply::Unhandled();
}

FReply SAmnesiaSkillNode::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		FSlateApplication::Get().PlaySound(ClickSound);
		OnActivate.ExecuteIfBound(SkillId);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ---------------------------------------------------------------- the screen

FSlateFontInfo SAmnesiaSkillScreen::MakeFont(bool bBold, int32 Size) const
{
	const TSharedPtr<const FCompositeFont>& Wanted = bBold ? BoldFont : RegularFont;
	const TSharedPtr<const FCompositeFont>& Other = bBold ? RegularFont : BoldFont;
	const float Scaled = FMath::Max(Size * SkillFontScale, 10.0f);

	if (Wanted.IsValid() || Other.IsValid())
	{
		return FSlateFontInfo(Wanted.IsValid() ? Wanted : Other, Scaled);
	}

	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FMath::RoundToInt(Scaled));
}

FSlateFontInfo SAmnesiaSkillScreen::MakeBodyFont(bool bBold, int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FMath::RoundToInt(FMath::Max(Size * SkillFontScale, 10.0f)));
}

void SAmnesiaSkillScreen::Construct(const FArguments& InArgs)
{
	Skills = InArgs._Skills;
	OnClose = InArgs._OnClose;
	SelectedId = FName("steel_power");

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

	Decor.Init(InArgs._DividerTexture, InArgs._Emblems);

	// pixel art frame, like the menu's
	if (InArgs._FrameTexture)
	{
		FrameBrush.SetResourceObject(InArgs._FrameTexture);
		FrameBrush.ImageSize = FVector2D(96.0f, 96.0f);
		FrameBrush.DrawAs = ESlateBrushDrawType::Box;
		FrameBrush.Margin = FMargin(1.0f / 3.0f);
		FrameBrush.TintColor = FSlateColor(SkillGold);
	}

	ButtonStyle = FButtonStyle()
		.SetPressedSound(ClickSound)
		.SetHoveredSound(HoverSound)
		.SetNormal(ButtonNormal)
		.SetHovered(ButtonHovered)
		.SetPressed(ButtonPressed)
		.SetNormalForeground(SkillCream)
		.SetHoveredForeground(SkillBrightGold)
		.SetPressedForeground(SkillGold)
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// one brush for every icon
	for (const TPair<FName, UObject*>& Pair : InArgs._Icons)
	{
		if (!Pair.Value)
		{
			continue;
		}

		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Pair.Value);
		Brush->ImageSize = FVector2D(80.0f, 80.0f);
		Brush->DrawAs = ESlateBrushDrawType::Image;
		IconBrushes.Add(Pair.Key, Brush);
	}

	// the four columns
	TSharedRef<SHorizontalBox> Columns = SNew(SHorizontalBox);

	for (const EAmnesiaSkillBranch Branch : { EAmnesiaSkillBranch::Steel, EAmnesiaSkillBranch::Fire, EAmnesiaSkillBranch::Ice, EAmnesiaSkillBranch::Dragon,
		EAmnesiaSkillBranch::Chakram, EAmnesiaSkillBranch::Axe, EAmnesiaSkillBranch::Wand })
	{
		// every branch stands on a dark panel of its own
		Columns->AddSlot().FillWidth(1.0f).Padding(6.0f, 0.0f)
		[
			Decor.WithCorners(
				SNew(SBorder)
				.BorderImage(&ColumnBrush)
				.Padding(FMargin(6.0f, 10.0f, 6.0f, 8.0f))
				[
					MakeBranch(Branch)
				],
				20.0f, 2.0f)
		];
	}

	// the element on the sword and the buttons
	TSharedRef<SHorizontalBox> Footer =
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(MakeFont(true, 19))
			.Text_Lambda([this]() -> FText
			{
				const EAmnesiaElement Element = Skills.IsValid() ? Skills->GetElement() : EAmnesiaElement::None;
				return FText::FromString(FString::Printf(TEXT("A kard eleme: %s     (R: váltás)"), *FAmnesiaSkillTree::ElementName(Element)));
			})
			.ColorAndOpacity_Lambda([this]() -> FSlateColor
			{
				const EAmnesiaElement Element = Skills.IsValid() ? Skills->GetElement() : EAmnesiaElement::None;
				return FSlateColor(Element == EAmnesiaElement::Fire ? FAmnesiaSkillTree::BranchColor(EAmnesiaSkillBranch::Fire)
					: (Element == EAmnesiaElement::Ice ? FAmnesiaSkillTree::BranchColor(EAmnesiaSkillBranch::Ice) : SkillCream));
			})
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f)
		[
			MakeButton(FText::FromString(TEXT("Visszaállítás")), FSimpleDelegate::CreateLambda([this]()
			{
				if (Skills.IsValid())
				{
					Skills->ResetSkills();
				}
			}))
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			MakeButton(FText::FromString(TEXT("Vissza")), FSimpleDelegate::CreateRaw(this, &SAmnesiaSkillScreen::GoBack))
		];

	TSharedRef<SWidget> Panel =
		SNew(SBox)
		.WidthOverride(1800.0f)
		[
			Decor.WithCorners(
			WithFrame(
			SNew(SBorder)
			.BorderImage(&PanelBrush)
			.Padding(FMargin(44.0f, 30.0f))
			[
				SNew(SVerticalBox)

				// title and the skill points
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)

					// the title on a ribbon, and the three weapons between two dividers
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						Decor.Ribbon(
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("KÉPESSÉGEK")))
							.Font(MakeFont(true, 46))
							.ColorAndOpacity(SkillGold)
							.ShadowOffset(FVector2D(3.0f, 3.0f))
							.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f)),
							62.0f)
					]

					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(28.0f, 0.0f)
					[
						Decor.Ornament(Decor.Trio(64.0f))
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
								.Text(FText::FromString(TEXT("Képességpont")))
								.Font(MakeFont(false, 20))
								.ColorAndOpacity(SkillCream)
							]

							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Font_Lambda([this]() -> FSlateFontInfo
								{
									// the engine font has the infinity sign, the Cinzel face does not
									return Skills.IsValid() && Skills->HasInfinitePoints() ? MakeBodyFont(true, 50) : MakeFont(true, 40);
								})
								.Text_Lambda([this]() -> FText
								{
									if (Skills.IsValid() && Skills->HasInfinitePoints())
									{
										return FText::FromString(TEXT("∞"));
									}

									return FText::FromString(FString::Printf(TEXT("%d"), Skills.IsValid() ? Skills->GetFreePoints() : 0));
								})
								.ColorAndOpacity_Lambda([this]() -> FSlateColor
								{
									return FSlateColor(Skills.IsValid() && Skills->GetFreePoints() > 0 ? SkillBrightGold : SkillMuted);
								})
							]
						]
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 8.0f)
				[
					Decor.Ornament(Decor.Diamond(11.0f))
				]

				// the four columns of skills
				+ SVerticalBox::Slot().AutoHeight()
				[
					Columns
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 8.0f)
				[
					Decor.Ornament(Decor.Diamond(11.0f))
				]

				// what the skill that is looked at does
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeDetails()
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
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

TSharedRef<SWidget> SAmnesiaSkillScreen::WithFrame(TSharedRef<SWidget> Content, float Inset)
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

TSharedRef<SWidget> SAmnesiaSkillScreen::MakeButton(const FText& Label, FSimpleDelegate Action)
{
	return SNew(SBox)
		.WidthOverride(230.0f)
		.HeightOverride(50.0f)
		[
			SNew(SButton)
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
				.Font(MakeFont(true, 20))
				.ShadowOffset(FVector2D(2.0f, 2.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f))
			]
		];
}

TSharedRef<SWidget> SAmnesiaSkillScreen::MakeBranch(EAmnesiaSkillBranch Branch)
{
	const FLinearColor Color = FAmnesiaSkillTree::BranchColor(Branch);

	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);

	// the emblem of the branch: a weapon for the weapon branches, dimmed while the branch is locked
	FName Emblem = FName("steel");
	bool bWeaponBranch = false;

	switch (Branch)
	{
	case EAmnesiaSkillBranch::Fire: Emblem = FName("fire"); break;
	case EAmnesiaSkillBranch::Ice: Emblem = FName("ice"); break;
	case EAmnesiaSkillBranch::Dragon: Emblem = FName("dragon"); break;
	case EAmnesiaSkillBranch::Chakram: Emblem = FName("chakram"); bWeaponBranch = true; break;
	case EAmnesiaSkillBranch::Axe: Emblem = FName("axe"); bWeaponBranch = true; break;
	case EAmnesiaSkillBranch::Wand: Emblem = FName("wand"); bWeaponBranch = true; break;
	default: break;
	}

	Column->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		Decor.Medallion(Emblem, 52.0f, bWeaponBranch, TAttribute<FLinearColor>::CreateLambda([this, Branch]() -> FLinearColor
		{
			return Skills.IsValid() && Skills->IsBranchLocked(Branch) ? FLinearColor(0.4f, 0.4f, 0.4f, 1.0f) : FLinearColor::White;
		}))
	];

	Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 2.0f)
		[
			SNew(SAmnesiaPadlock)
			.Size(24.0f)
			.Color(Color * 0.9f)
			.Visibility_Lambda([this, Branch]() -> EVisibility
			{
				return Skills.IsValid() && Skills->IsBranchLocked(Branch) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
		]

		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FAmnesiaSkillTree::BranchName(Branch)))
			.Font(MakeFont(true, 27))
			.ColorAndOpacity(Color)
			.ShadowOffset(FVector2D(2.0f, 2.0f))
			.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
		]
	];

	Column->AddSlot().AutoHeight().Padding(50.0f, 4.0f, 50.0f, 16.0f)
	[
		SNew(SBox).HeightOverride(2.0f)[ SNew(SImage).Image(&WhiteBrush).ColorAndOpacity(Color * 0.8f) ]
	];

	// the skills of this branch, top to bottom
	TArray<const FAmnesiaSkillDef*> Defs;

	for (const FAmnesiaSkillDef& Def : FAmnesiaSkillTree::All())
	{
		if (Def.Branch == Branch)
		{
			Defs.Add(&Def);
		}
	}

	Defs.Sort([](const FAmnesiaSkillDef& A, const FAmnesiaSkillDef& B) { return A.Tier < B.Tier; });

	for (int32 Index = 0; Index < Defs.Num(); ++Index)
	{
		const FAmnesiaSkillDef* Def = Defs[Index];

		// the line from the skill above, lit once that one is learned
		if (Index > 0)
		{
			Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(5.0f)
				.HeightOverride(24.0f)
				[
					SNew(SImage)
					.Image(&WhiteBrush)
					.ColorAndOpacity_Lambda([this, Def, Color]() -> FSlateColor
					{
						const bool bOpen = Skills.IsValid() && (Def->Requires.IsNone() || Skills->GetRank(Def->Requires) > 0);
						return FSlateColor(bOpen ? Color : SkillLocked);
					})
				]
			];
		}

		const TSharedPtr<FSlateBrush>* Icon = IconBrushes.Find(Def->Id);

		TSharedPtr<SAmnesiaSkillNode> Node;

		Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SAssignNew(Node, SAmnesiaSkillNode)
			.SkillId(Def->Id)
			.Skills(Skills.Get())
			.IconBrush(Icon ? Icon->Get() : static_cast<const FSlateBrush*>(&NodeFillBrush))
			.FillBrush(&NodeFillBrush)
			.OutlineBrush(&NodeOutlineBrush)
			.PipBrush(&WhiteBrush)
			.BranchColor(Color)
			.BadgeFont(MakeFont(true, 12))
			.ClickSound(ClickSound)
			.HoverSound(HoverSound)
			.OnSelected(SAmnesiaSkillNode::FOnSkillAction::CreateSP(this, &SAmnesiaSkillScreen::SelectSkill))
			.OnActivate(SAmnesiaSkillNode::FOnSkillAction::CreateSP(this, &SAmnesiaSkillScreen::ActivateSkill))
		];

		Nodes.Add(Node);
	}

	return Column;
}

TSharedRef<SWidget> SAmnesiaSkillScreen::MakeDetails()
{
	return Decor.WithCorners(
		SNew(SBorder)
		.BorderImage(&InnerPanelBrush)
		.Padding(FMargin(22.0f, 16.0f))
		[
			SNew(SBox)
			.HeightOverride(SkillDetailsHeight)
			[
				SNew(SHorizontalBox)

				// the icon, big
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 22.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(112.0f)
					.HeightOverride(112.0f)
					[
						SNew(SOverlay)

						+ SOverlay::Slot()
						[
							SNew(SImage).Image(&NodeFillBrush)
						]

						+ SOverlay::Slot().Padding(7.0f)
						[
							SNew(SImage).Image_Lambda([this]() -> const FSlateBrush* { return GetDetailIcon(); })
						]

						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(&NodeOutlineBrush)
							.ColorAndOpacity_Lambda([this]() -> FSlateColor { return GetDetailColor(); })
						]
					]
				]

				// the name and what it does: the description takes the room that is left, so the lines under it stay where they are
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText { return GetDetailName(); })
						.Font(MakeFont(true, 30))
						.ColorAndOpacity_Lambda([this]() -> FSlateColor { return GetDetailColor(); })
						.ShadowOffset(FVector2D(2.0f, 2.0f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
					]

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText { return GetDetailKind(); })
						.Font(MakeBodyFont(false, 16))
						.ColorAndOpacity(SkillMuted)
					]

					+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText { return GetDetailText(); })
						.Font(MakeBodyFont(false, 19))
						.ColorAndOpacity(SkillCream)
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText { return GetDetailNow(); })
						.Font(MakeBodyFont(true, 20))
						.ColorAndOpacity(SkillBrightGold)
					]

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText { return GetDetailNext(); })
						.Font(MakeBodyFont(false, 19))
						.ColorAndOpacity(SkillMuted)
					]
				]

				// what to do with it
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(26.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(340.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText { return GetDetailAction(); })
						.Font(MakeFont(true, 19))
						.AutoWrapText(true)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity_Lambda([this]() -> FSlateColor
						{
							return FSlateColor(FPlatformTime::Seconds() < ComplaintUntil ? SkillWarning : SkillCream);
						})
					]
				]
			]
		], 24.0f, 2.0f);
}

void SAmnesiaSkillScreen::SelectSkill(FName Id)
{
	SelectedId = Id;
}

void SAmnesiaSkillScreen::ActivateSkill(FName Id)
{
	SelectedId = Id;

	UAmnesiaSkillComponent* Component = Skills.Get();
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(Id);

	if (!Component || !Def)
	{
		return;
	}

	// a learned element goes on or off the sword
	if (Def->Kind == EAmnesiaSkillKind::Element && Component->GetRank(Id) > 0)
	{
		const EAmnesiaElement Mine = Id == FName("fire_blade") ? EAmnesiaElement::Fire : EAmnesiaElement::Ice;
		Component->SetElement(Component->GetElement() == Mine ? EAmnesiaElement::None : Mine);
		return;
	}

	if (!Component->Learn(Id))
	{
		FString Reason;
		Component->CanLearn(Id, &Reason);

		Complaint = Reason;
		ComplaintUntil = FPlatformTime::Seconds() + 2.5;

	} else {

		ComplaintUntil = 0.0;
	}
}

FText SAmnesiaSkillScreen::GetDetailName() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);
	return FText::FromString(Def ? Def->Name : FString(TEXT("Képességek")));
}

FText SAmnesiaSkillScreen::GetDetailKind() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);

	if (!Def)
	{
		return FText::GetEmpty();
	}

	const int32 Rank = Skills.IsValid() ? Skills->GetRank(SelectedId) : 0;
	const FString Branch = FAmnesiaSkillTree::BranchName(Def->Branch);

	switch (Def->Kind)
	{
	case EAmnesiaSkillKind::Element:
		return FText::FromString(FString::Printf(TEXT("%s  •  elem: a kard töltése  •  %s"), *Branch, Rank > 0 ? TEXT("megtanulva") : TEXT("még nincs megtanulva")));

	case EAmnesiaSkillKind::Active:
		return FText::FromString(FString::Printf(TEXT("%s  •  aktív képesség  •  billentyű: %s  •  %s"), *Branch, Def->Id == FName("fire_burst") ? TEXT("Q") : TEXT("E"),
			Rank > 0 ? TEXT("megtanulva") : TEXT("még nincs megtanulva")));

	default:
		return FText::FromString(FString::Printf(TEXT("%s  •  passzív képesség  •  rang %d / %d"), *Branch, Rank, Def->MaxRank));
	}
}

FText SAmnesiaSkillScreen::GetDetailText() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);
	return FText::FromString(Def ? Def->Detail : FString());
}

FText SAmnesiaSkillScreen::GetDetailNow() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);

	if (!Def)
	{
		return FText::GetEmpty();
	}

	const int32 Rank = Skills.IsValid() ? Skills->GetRank(SelectedId) : 0;
	const FString Value = FAmnesiaSkillTree::ValueText(*Def, Rank);

	if (Rank <= 0)
	{
		return FText::FromString(TEXT("Még nincs megtanulva"));
	}

	return FText::FromString(Value.IsEmpty() ? FString(TEXT("Megtanulva")) : FString::Printf(TEXT("Jelenleg: %s"), *Value));
}

FText SAmnesiaSkillScreen::GetDetailNext() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);

	if (!Def)
	{
		return FText::GetEmpty();
	}

	const int32 Rank = Skills.IsValid() ? Skills->GetRank(SelectedId) : 0;

	if (Rank >= Def->MaxRank)
	{
		return FText::FromString(TEXT("Legmagasabb rang"));
	}

	const FString Value = FAmnesiaSkillTree::ValueText(*Def, Rank + 1);

	if (Value.IsEmpty())
	{
		return FText::FromString(FString::Printf(TEXT("Ára: %d képességpont"), Def->Cost));
	}

	return FText::FromString(FString::Printf(TEXT("%s: %s   (ára: %d pont)"), Rank > 0 ? TEXT("Következő rang") : TEXT("Az első rang"), *Value, Def->Cost));
}

FText SAmnesiaSkillScreen::GetDetailAction() const
{
	if (FPlatformTime::Seconds() < ComplaintUntil)
	{
		return FText::FromString(Complaint);
	}

	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);

	if (!Def || !Skills.IsValid())
	{
		return FText::GetEmpty();
	}

	if (Def->Kind == EAmnesiaSkillKind::Element && Skills->GetRank(SelectedId) > 0)
	{
		return FText::FromString(TEXT("Enter vagy kattintás: a kard elemének be- és kikapcsolása"));
	}

	FString Reason;

	if (Skills->CanLearn(SelectedId, &Reason))
	{
		return FText::FromString(TEXT("Enter vagy kattintás: tanulás"));
	}

	return FText::FromString(Reason);
}

FSlateColor SAmnesiaSkillScreen::GetDetailColor() const
{
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(SelectedId);
	return FSlateColor(Def ? FAmnesiaSkillTree::BranchColor(Def->Branch) : SkillGold);
}

const FSlateBrush* SAmnesiaSkillScreen::GetDetailIcon() const
{
	const TSharedPtr<FSlateBrush>* Icon = IconBrushes.Find(SelectedId);
	return Icon ? Icon->Get() : static_cast<const FSlateBrush*>(&NodeFillBrush);
}

EActiveTimerReturnType SAmnesiaSkillScreen::FocusTimer(double InCurrentTime, float InDeltaTime)
{
	if (Nodes.Num() > 0 && Nodes[0].IsValid())
	{
		FSlateApplication::Get().SetUserFocus(0, Nodes[0], EFocusCause::SetDirectly);
	}

	return EActiveTimerReturnType::Stop;
}

void SAmnesiaSkillScreen::FocusFirstNode()
{
	if (Nodes.Num() > 0 && Nodes[0].IsValid())
	{
		SelectedId = Nodes[0]->GetSkillId();
	}

	// once now, and once more after the input mode of the game has had its say
	RegisterActiveTimer(0.2f, FWidgetActiveTimerDelegate::CreateSP(this, &SAmnesiaSkillScreen::FocusTimer));
	FocusTimer(0.0, 0.0f);
}

void SAmnesiaSkillScreen::GoBack()
{
	OnClose.ExecuteIfBound();
}

FReply SAmnesiaSkillScreen::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape || Key == EKeys::P || Key == EKeys::T || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right)
	{
		GoBack();
		return FReply::Handled();
	}

	if (Key == EKeys::R || Key == EKeys::Gamepad_LeftShoulder)
	{
		if (Skills.IsValid())
		{
			Skills->CycleElement();
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}
