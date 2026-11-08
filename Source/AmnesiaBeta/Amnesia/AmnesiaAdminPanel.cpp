#include "AmnesiaAdminPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor AdminGold(0.95f, 0.76f, 0.36f, 1.0f);
	const FLinearColor AdminBrightGold(1.0f, 0.9f, 0.55f, 1.0f);
	const FLinearColor AdminCream(0.95f, 0.9f, 0.8f, 1.0f);
	const FLinearColor AdminMuted(0.6f, 0.55f, 0.45f, 1.0f);
	const FLinearColor AdminOnGreen(0.45f, 0.95f, 0.5f, 1.0f);

	FSlateFontInfo AdminPanelFont(const char* Style, int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Style, Size);
	}
}

void SAmnesiaAdminPanel::Construct(const FArguments& InArgs)
{
	OnClose = InArgs._OnClose;

	ButtonStyle = FButtonStyle()
		.SetNormal(ButtonNormal)
		.SetHovered(ButtonHovered)
		.SetPressed(ButtonPressed)
		.SetNormalForeground(AdminCream)
		.SetHoveredForeground(AdminBrightGold)
		.SetPressedForeground(AdminGold)
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// the zone buttons, two per row
	TSharedRef<SUniformGridPanel> Zones = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	const TFunction<void(int32)> Teleport = InArgs._Teleport;
	const TFunction<void(int32)> AddGold = InArgs._AddGold;

	// the level: four equal buttons under the number, two steps down and two up
	const TFunction<int32()> GetLevel = InArgs._GetLevel;
	const TFunction<int32()> GetMaxLevel = InArgs._GetMaxLevel;
	const TFunction<void(int32)> ChangeLevel = InArgs._ChangeLevel;

	TSharedRef<SUniformGridPanel> LevelButtons = SNew(SUniformGridPanel).SlotPadding(FMargin(4.0f, 0.0f));
	const int32 LevelSteps[4] = { -5, -1, 1, 5 };

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const int32 Step = LevelSteps[Index];
		const FString Label = Step > 0 ? FString::Printf(TEXT("+%d"), Step) : FString::Printf(TEXT("%d"), Step);

		LevelButtons->AddSlot(Index, 0)
		[
			MakeButton(FText::FromString(Label), FSimpleDelegate::CreateLambda([ChangeLevel, Step]()
			{
				if (ChangeLevel)
				{
					ChangeLevel(Step);
				}
			}), 40.0f, 16)
		];
	}

	for (int32 Index = 0; Index < InArgs._ZoneNames.Num(); ++Index)
	{
		const FString Label = FString::Printf(TEXT("%d. %s"), Index + 1, *InArgs._ZoneNames[Index]);

		Zones->AddSlot(Index % 2, Index / 2)
		[
			MakeButton(FText::FromString(Label), FSimpleDelegate::CreateLambda([Teleport, Index]()
			{
				if (Teleport)
				{
					Teleport(Index);
				}
			}))
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
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.0f, 0.0f, 80.0f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(470.0f)
			[
				SNew(SBorder)
				.BorderImage(&PanelBrush)
				.Padding(FMargin(30.0f, 24.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ADMIN")))
						.Font(AdminPanelFont("Bold", 34))
						.ColorAndOpacity(AdminGold)
						.ShadowOffset(FVector2D(2.0f, 2.0f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Tesztelés  •  K vagy Esc: bezár")))
						.Font(AdminPanelFont("Italic", 13))
						.ColorAndOpacity(AdminMuted)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						MakeToggle(FText::FromString(TEXT("Sebezhetetlenség (God)")), InArgs._IsGod, InArgs._SetGod)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						MakeToggle(FText::FromString(TEXT("Repülés (Fly)  •  W/S: fel-le")), InArgs._IsFlying, InArgs._SetFlying)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						MakeToggle(FText::FromString(TEXT("Végtelen képességpont")), InArgs._IsInfinitePoints, InArgs._SetInfinitePoints)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Karakter szintje"))).Font(AdminPanelFont("Regular", 17)).ColorAndOpacity(AdminCream)
						]

						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([GetLevel, GetMaxLevel]()
							{
								return FText::FromString(FString::Printf(TEXT("%d / %d"), GetLevel ? GetLevel() : 1, GetMaxLevel ? GetMaxLevel() : 1));
							})
							.Font(AdminPanelFont("Bold", 17))
							.ColorAndOpacity(AdminBrightGold)
						]
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						LevelButtons
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						MakeButton(FText::FromString(TEXT("+1000 arany")), FSimpleDelegate::CreateLambda([AddGold]()
						{
							if (AddGold)
							{
								AddGold(1000);
							}
						}), 40.0f, 16)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 6.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("TELEPORT"))).Font(AdminPanelFont("Bold", 16)).ColorAndOpacity(AdminGold)
						]

						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SBox).HeightOverride(2.0f)[ SNew(SImage).Image(&FaintGoldBrush) ]
						]
					]

					+ SVerticalBox::Slot().AutoHeight()
					[
						Zones
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						MakeButton(FText::FromString(TEXT("Bezár")), FSimpleDelegate::CreateLambda([this]() { OnClose.ExecuteIfBound(); }), 40.0f, 16)
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SAmnesiaAdminPanel::MakeToggle(const FText& Label, TFunction<bool()> Get, TFunction<void(bool)> Set)
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(Label).Font(AdminPanelFont("Regular", 17)).ColorAndOpacity(AdminCream)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text_Lambda([Get]() { return FText::FromString(Get && Get() ? TEXT("BE") : TEXT("KI")); })
			.ColorAndOpacity_Lambda([Get]() { return FSlateColor(Get && Get() ? AdminOnGreen : AdminMuted); })
			.Font(AdminPanelFont("Bold", 15))
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([Get]() { return Get && Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([Set](ECheckBoxState State)
			{
				if (Set)
				{
					Set(State == ECheckBoxState::Checked);
				}
			})
		];
}

TSharedRef<SWidget> SAmnesiaAdminPanel::MakeButton(const FText& Label, FSimpleDelegate Action, float Height, int32 FontSize)
{
	TSharedPtr<SButton> Button;

	TSharedRef<SWidget> Widget =
		SNew(SBox)
		.HeightOverride(Height)
		[
			SAssignNew(Button, SButton)
			.ButtonStyle(&ButtonStyle)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([Action]()
			{
				Action.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(Label).Font(AdminPanelFont("Bold", FontSize))
			]
		];

	if (!FirstButton.IsValid())
	{
		FirstButton = Button;
	}

	return Widget;
}

void SAmnesiaAdminPanel::FocusFirst()
{
	if (FirstButton.IsValid())
	{
		FSlateApplication::Get().SetAllUserFocus(FirstButton, EFocusCause::SetDirectly);

	} else {

		FSlateApplication::Get().SetAllUserFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
}

FReply SAmnesiaAdminPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::K || Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		OnClose.ExecuteIfBound();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
