#include "AmnesiaHUD.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaEnemy.h"
#include "CanvasItem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "AmnesiaMenu.h"
#include "AmnesiaAdminPanel.h"
#include "AmnesiaSkillScreen.h"
#include "AmnesiaSkills.h"
#include "AmnesiaShopScreen.h"
#include "AmnesiaWeapons.h"
#include "AmnesiaCompanion.h"
#include "AmnesiaCat.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Widgets/SViewport.h"
#include "AmnesiaProgress.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundBase.h"

/**
 *  The pictures the menus, the shop and the skill screen decorate themselves with: the weapons of the weapon table,
 *  and one picture for each of the other skill branches
 */
static FAmnesiaUIIcons MakeDecorIcons(TFunctionRef<UObject*(const TSoftObjectPtr<UObject>&)> Load, const TSoftObjectPtr<UObject>& DragonPicture)
{
	FAmnesiaUIIcons Icons;

	for (const FAmnesiaWeaponDef& Weapon : FAmnesiaWeapons::All())
	{
		if (Weapon.Id != FAmnesiaWeapons::SwordId() && !Weapon.IconPath.IsEmpty())
		{
			Icons.Add(Weapon.Id, Load(TSoftObjectPtr<UObject>(FSoftObjectPath(Weapon.IconPath))));
		}
	}

	Icons.Add(FName("steel"), Load(TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/KiroshoSkIconsPk0/Textures/T_FuryAttack.T_FuryAttack")))));
	Icons.Add(FName("fire"), Load(TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/KiroshoSkIconsPk0/Textures/T_Berserk.T_Berserk")))));
	Icons.Add(FName("ice"), Load(TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/KiroshoSkIconsPk0/Textures/T_EnergyRecharge.T_EnergyRecharge")))));
	Icons.Add(FName("dragon"), Load(DragonPicture));

	return Icons;
}

/**
 *  Takes the Escape key before anything else sees it. In the editor the Stop command of the play session
 *  gets the key ahead of the player's input, so a binding on the pawn never fires there.
 */
class FAmnesiaEscapeProcessor : public IInputProcessor
{
public:

	explicit FAmnesiaEscapeProcessor(AAmnesiaHUD* InHUD) : HUD(InHUD) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() != EKeys::Escape || !HUD.IsValid())
		{
			return false;
		}

		// only while the game has the keyboard: other editor windows keep their own Escape
		const TSharedPtr<SViewport> GameWidget = (GEngine && GEngine->GameViewport) ? GEngine->GameViewport->GetGameViewportWidget() : nullptr;

		if (!GameWidget.IsValid() || !(SlateApp.GetUserFocusedWidget(0) == GameWidget || SlateApp.HasUserFocusedDescendants(GameWidget.ToSharedRef(), 0)))
		{
			return false;
		}

		// a held key repeats, but only the first press counts
		return InKeyEvent.IsRepeat() || HUD->HandleEscape();
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("AmnesiaEscape"); }

private:

	TWeakObjectPtr<AAmnesiaHUD> HUD;
};

AAmnesiaHUD::AAmnesiaHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AAmnesiaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	// sizes are authored for 1080p and scaled to the viewport height
	const float UIScale = Canvas->ClipY / 1080.0f;

	// nothing but the menu while it is open
	if (Menu.IsValid() || SkillScreen.IsValid() || ShopScreen.IsValid())
	{
		return;
	}

	DrawEnemyBars(UIScale);
	DrawHeroBar(UIScale);
	DrawSkillBar(UIScale);
	DrawCats(UIScale);
	DrawXPPopups(UIScale);
	DrawBanner(UIScale);
	DrawDeathScreen(UIScale);
}

void AAmnesiaHUD::AddXPPopup(const FVector& WorldLocation, int32 Amount)
{
	AddTextPopup(WorldLocation, FString::Printf(TEXT("+%d XP"), Amount), XPPopupColor, 1.6f, XPPopupDuration);
}

void AAmnesiaHUD::AddNumberPopup(const FVector& WorldLocation, float Amount, const FLinearColor& Color, const FString& Prefix)
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		if (!ProgressSystem->GetProgress()->bDamageNumbers)
		{
			return;
		}
	}

	// whole numbers stay whole, small fractions get one decimal
	const bool bWhole = FMath::IsNearlyEqual(Amount, FMath::RoundToFloat(Amount), 0.05f);
	const FString Number = bWhole ? FString::Printf(TEXT("%d"), FMath::RoundToInt(Amount)) : FString::Printf(TEXT("%.1f"), Amount);

	AddTextPopup(WorldLocation, Prefix + Number, Color, 1.5f, 0.9f);
}

void AAmnesiaHUD::AddGoldPopup(const FVector& WorldLocation, int32 Amount)
{
	// a little under the XP, so the two don't print over each other
	AddTextPopup(WorldLocation - FVector(0.0f, 0.0f, 55.0f), FString::Printf(TEXT("+%d arany"), Amount), GoldPopupColor, 1.35f, XPPopupDuration);
}

void AAmnesiaHUD::AddTextPopup(const FVector& WorldLocation, const FString& Text, const FLinearColor& Color, float Scale, float Duration)
{
	FTextPopup& Popup = TextPopups.AddDefaulted_GetRef();
	Popup.WorldLocation = WorldLocation;
	Popup.Text = Text;
	Popup.Color = Color;
	Popup.Scale = Scale;
	Popup.Duration = Duration;
	Popup.StartTime = GetWorld()->GetTimeSeconds();
}

void AAmnesiaHUD::DrawXPPopups(float UIScale)
{
	const float Now = GetWorld()->GetTimeSeconds();

	for (int32 Index = TextPopups.Num() - 1; Index >= 0; --Index)
	{
		const FTextPopup& Popup = TextPopups[Index];
		const float Age = Now - Popup.StartTime;

		if (Age >= Popup.Duration)
		{
			TextPopups.RemoveAtSwap(Index);
			continue;
		}

		const FVector ScreenPosition = Project(Popup.WorldLocation, false);

		if (ScreenPosition.Z <= 0.0f)
		{
			continue;
		}

		const float Progress = Age / Popup.Duration;

		// rise with an ease out, stay solid for a while, then fade
		const float Rise = XPPopupRise * UIScale * (1.0f - FMath::Square(1.0f - Progress));
		const float Alpha = Progress < 0.6f ? 1.0f : 1.0f - (Progress - 0.6f) / 0.4f;

		const FString& Text = Popup.Text;

		// numbers pop in a bit bigger
		const float Pop = 1.0f + 0.5f * FMath::Square(1.0f - FMath::Clamp(Age / 0.15f, 0.0f, 1.0f));
		const float Scale = Popup.Scale * UIScale * Pop;

		float Width = 0.0f;
		float Height = 0.0f;
		UFont* Font = GEngine->GetMediumFont();
		GetTextSize(Text, Width, Height, Font, Scale);

		FLinearColor Color = Popup.Color;
		Color.A = Alpha;

		FCanvasTextItem Item(FVector2D(ScreenPosition.X - Width * 0.5f, ScreenPosition.Y - Rise), FText::FromString(Text), Font, Color);
		Item.Scale = FVector2D(Scale, Scale);
		Item.bOutlined = true;
		Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, Alpha);
		Canvas->DrawItem(Item);
	}
}

void AAmnesiaHUD::DrawHeroBar(float UIScale)
{
	const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());

	if (!Hero || Hero->GetMaxHealth() <= 0.0f)
	{
		return;
	}

	const float Ratio = FMath::Clamp(Hero->GetHealth() / Hero->GetMaxHealth(), 0.0f, 1.0f);
	const float Trail = UpdateTrail(Hero, Ratio);

	const FVector2D Position = HeroBarPosition * UIScale;
	const FVector2D Size = HeroBarSize * UIScale;

	UFont* Font = GEngine->GetMediumFont();

	// hero name over the bar
	const float NameScale = 1.7f * UIScale;
	FCanvasTextItem NameItem(FVector2D(Position.X + 6.0f * UIScale, Position.Y - 46.0f * UIScale), HeroName, Font, FrameColor);
	NameItem.Scale = FVector2D(NameScale, NameScale);
	NameItem.bOutlined = true;
	NameItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
	Canvas->DrawItem(NameItem);

	DrawLifeBar(Position, Size, Ratio, Trail, FMath::RoundToInt(Hero->GetMaxHealth()), 3.0f * UIScale);

	// HP numbers in the middle of the bar
	const FString HPText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(FMath::Max(Hero->GetHealth(), 0.0f)), FMath::RoundToInt(Hero->GetMaxHealth()));
	const float TextScale = 1.3f * UIScale;

	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(HPText, TextWidth, TextHeight, Font, TextScale);

	FCanvasTextItem HPItem(FVector2D(Position.X + (Size.X - TextWidth) * 0.5f, Position.Y + (Size.Y - TextHeight) * 0.5f), FText::FromString(HPText), Font, TextColor);
	HPItem.Scale = FVector2D(TextScale, TextScale);
	HPItem.bOutlined = true;
	HPItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
	Canvas->DrawItem(HPItem);

	// level next to the name
	const FString LevelText = FString::Printf(TEXT("LV %d"), Hero->GetHeroLevel());
	float NameWidth = 0.0f;
	float NameHeight = 0.0f;
	GetTextSize(HeroName.ToString(), NameWidth, NameHeight, Font, NameScale);

	FCanvasTextItem LevelItem(FVector2D(Position.X + 6.0f * UIScale + NameWidth + 16.0f * UIScale, Position.Y - 46.0f * UIScale), FText::FromString(LevelText), Font, TextColor);
	LevelItem.Scale = FVector2D(NameScale, NameScale);
	LevelItem.bOutlined = true;
	LevelItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
	Canvas->DrawItem(LevelItem);

	DrawGold(Position, Size, UIScale);

	// thin XP bar under the life bar
	const float XPRatio = Hero->IsMaxLevel() ? 1.0f : FMath::Clamp(static_cast<float>(Hero->GetCurrentXP()) / FMath::Max(1, Hero->GetXPToNextLevel()), 0.0f, 1.0f);
	const FVector2D XPPosition(Position.X, Position.Y + Size.Y + 14.0f * UIScale);
	const FVector2D XPSize(Size.X, 8.0f * UIScale);
	const float XPFrame = 2.0f * UIScale;

	DrawRect(FrameColor, XPPosition.X - XPFrame, XPPosition.Y - XPFrame, XPSize.X + XPFrame * 2.0f, XPSize.Y + XPFrame * 2.0f);
	DrawRect(BackgroundColor, XPPosition.X, XPPosition.Y, XPSize.X, XPSize.Y);
	DrawRect(XPColor, XPPosition.X, XPPosition.Y, XPSize.X * XPRatio, XPSize.Y);
	DrawRect(FLinearColor(0.7f, 0.95f, 1.0f, 0.5f), XPPosition.X, XPPosition.Y + XPSize.Y * 0.15f, XPSize.X * XPRatio, FMath::Max(1.0f, XPSize.Y * 0.2f));

	// short banner after a level up
	const float SinceLevelUp = Hero->GetTimeSinceLevelUp();

	if (SinceLevelUp < LevelUpBannerTime)
	{
		DrawLevelUpBanner(Hero->GetHeroLevel(), SinceLevelUp, UIScale);
	}
}

void AAmnesiaHUD::DrawLevelUpBanner(int32 NewLevel, float Age, float UIScale)
{
	// "az" before numbers read with a vowel first: 1 (egy), 5 (öt), 50..59
	const bool bVowel = NewLevel == 1 || NewLevel == 5 || (NewLevel >= 50 && NewLevel <= 59);
	FString Sub = FString::Printf(TEXT("%s elérte %s %d. szintet"), *HeroName.ToString(), bVowel ? TEXT("az") : TEXT("a"), NewLevel);

	if (const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn()))
	{
		if (const UAmnesiaSkillComponent* Skills = Hero->GetSkills())
		{
			Sub += FString::Printf(TEXT("   •   +%d képességpont (T)"), Skills->PointsPerLevel);
		}
	}

	DrawOrnateBanner(TEXT("SZINTLÉPÉS!"), Sub, Age, LevelUpBannerTime, 0.26f, 3.6f, UIScale);
}

void AAmnesiaHUD::DrawOrnateBanner(const FString& Title, const FString& Sub, float Age, float Duration, float CenterYFraction, float TitleSize, float UIScale)
{
	UFont* Font = GEngine->GetMediumFont();

	// fade in quickly, hold, then fade out
	const float FadeIn = FMath::Clamp(Age / 0.25f, 0.0f, 1.0f);
	const float FadeOut = FMath::Clamp((Duration - Age) / 0.9f, 0.0f, 1.0f);
	const float Alpha = FMath::Min(FadeIn, FadeOut);

	// the text pops in bigger and settles
	const float Pop = 1.0f + 0.45f * FMath::Square(1.0f - FMath::Clamp(Age / 0.4f, 0.0f, 1.0f));

	const FLinearColor Gold(1.0f, 0.82f, 0.32f, Alpha);
	const FLinearColor PaleGold(1.0f, 0.95f, 0.7f, Alpha);
	const FLinearColor Ember(1.0f, 0.45f, 0.1f, Alpha * 0.55f);

	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * CenterYFraction;

	// the plate: a dark band that opens up from the middle, framed in gold
	const float Open = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(Age / 0.35f, 0.0f, 1.0f));
	const float HalfWidth = 430.0f * UIScale * Open;
	const float HalfHeight = 78.0f * UIScale;

	for (int32 Step = 0; Step < 8; ++Step)
	{
		// the band fades towards its ends
		const float Fraction = static_cast<float>(Step) / 8.0f;
		const float StepWidth = HalfWidth * (1.0f - Fraction);

		DrawRect(FLinearColor(0.02f, 0.01f, 0.03f, 0.16f * Alpha), CenterX - StepWidth, CenterY - HalfHeight, StepWidth * 2.0f, HalfHeight * 2.0f);
	}

	const float Line = 3.0f * UIScale;

	DrawRect(Gold, CenterX - HalfWidth, CenterY - HalfHeight, HalfWidth * 2.0f, Line);
	DrawRect(Gold, CenterX - HalfWidth, CenterY + HalfHeight - Line, HalfWidth * 2.0f, Line);
	DrawRect(FLinearColor(Gold.R, Gold.G, Gold.B, Alpha * 0.5f), CenterX - HalfWidth * 0.9f, CenterY - HalfHeight + Line * 2.5f, HalfWidth * 1.8f, Line * 0.5f);
	DrawRect(FLinearColor(Gold.R, Gold.G, Gold.B, Alpha * 0.5f), CenterX - HalfWidth * 0.9f, CenterY + HalfHeight - Line * 3.0f, HalfWidth * 1.8f, Line * 0.5f);

	// ornaments at the ends and in the middle of the top and bottom lines: a skill in a gold frame at each end, fire on the left and ice on the right
	const TCHAR* EndIcons[2] = { TEXT("fire_burst"), TEXT("ice_nova") };

	for (int32 End = 0; End < 2; ++End)
	{
		const float SideX = End == 0 ? CenterX - HalfWidth : CenterX + HalfWidth;

		DrawDiamond(FVector2D(SideX, CenterY), HalfHeight * 1.1f, Gold);
		DrawDiamond(FVector2D(SideX, CenterY), HalfHeight * 0.7f, FLinearColor(0.1f, 0.04f, 0.02f, Alpha));

		if (UTexture* Icon = Cast<UTexture>(GetSkillIcon(FName(EndIcons[End]))))
		{
			const float Picture = HalfHeight * 0.8f;
			const float Frame = 3.0f * UIScale;

			DrawRect(Gold, SideX - Picture * 0.5f - Frame, CenterY - Picture * 0.5f - Frame, Picture + Frame * 2.0f, Picture + Frame * 2.0f);
			DrawTexture(Icon, SideX - Picture * 0.5f, CenterY - Picture * 0.5f, Picture, Picture, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor(1.0f, 1.0f, 1.0f, Alpha));

		} else {

			DrawDiamond(FVector2D(SideX, CenterY), HalfHeight * 0.35f, Ember);
		}
	}

	DrawDiamond(FVector2D(CenterX, CenterY - HalfHeight), 22.0f * UIScale, Gold);
	DrawDiamond(FVector2D(CenterX, CenterY + HalfHeight), 22.0f * UIScale, Gold);

	// small diamonds on the top and bottom lines, between the middle and the ends
	for (const float Offset : { -0.55f, 0.55f })
	{
		DrawDiamond(FVector2D(CenterX + HalfWidth * Offset, CenterY - HalfHeight), 12.0f * UIScale, PaleGold);
		DrawDiamond(FVector2D(CenterX + HalfWidth * Offset, CenterY + HalfHeight), 12.0f * UIScale, PaleGold);
	}

	// the title with a warm glow behind it
	const float TitleScale = TitleSize * UIScale * Pop;

	float TitleWidth = 0.0f;
	float TitleHeight = 0.0f;
	GetTextSize(Title, TitleWidth, TitleHeight, Font, TitleScale);

	const FVector2D TitlePosition(CenterX - TitleWidth * 0.5f, CenterY - HalfHeight * 0.95f - (Pop - 1.0f) * 20.0f * UIScale);

	for (const FVector2D& Offset : { FVector2D(-3.0f, 0.0f), FVector2D(3.0f, 0.0f), FVector2D(0.0f, -3.0f), FVector2D(0.0f, 3.0f) })
	{
		FCanvasTextItem GlowItem(TitlePosition + Offset * UIScale, FText::FromString(Title), Font, Ember);
		GlowItem.Scale = FVector2D(TitleScale, TitleScale);
		Canvas->DrawItem(GlowItem);
	}

	FCanvasTextItem TitleItem(TitlePosition, FText::FromString(Title), Font, Gold);
	TitleItem.Scale = FVector2D(TitleScale, TitleScale);
	TitleItem.bOutlined = true;
	TitleItem.OutlineColor = FLinearColor(0.1f, 0.03f, 0.0f, Alpha);
	Canvas->DrawItem(TitleItem);

	// the line under it
	const float SubScale = 1.7f * UIScale;

	float SubWidth = 0.0f;
	float SubHeight = 0.0f;
	GetTextSize(Sub, SubWidth, SubHeight, Font, SubScale);

	FCanvasTextItem SubItem(FVector2D(CenterX - SubWidth * 0.5f, CenterY + HalfHeight * 0.38f), FText::FromString(Sub), Font, PaleGold);
	SubItem.Scale = FVector2D(SubScale, SubScale);
	SubItem.bOutlined = true;
	SubItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, Alpha);
	Canvas->DrawItem(SubItem);
}

void AAmnesiaHUD::ShowBanner(const FText& Title, const FText& Sub)
{
	BannerTitle = Title.ToString();
	BannerSub = Sub.ToString();
	BannerStartTime = GetWorld()->GetTimeSeconds();
}

void AAmnesiaHUD::DrawBanner(float UIScale)
{
	const float Age = GetWorld()->GetTimeSeconds() - BannerStartTime;

	if (Age >= 0.0f && Age < 3.5f)
	{
		DrawOrnateBanner(BannerTitle, BannerSub, Age, 3.5f, 0.42f, 3.0f, UIScale);
	}
}

TArray<TPair<FString, FString>> AAmnesiaHUD::GetControlLines()
{
	return {
		{ TEXT("A / D"), TEXT("Mozgás") },
		{ TEXT("Space"), TEXT("Ugrás, dupla ugrás") },
		{ TEXT("Shift"), TEXT("Futás") },
		{ TEXT("Bal egér"), TEXT("Támadás (kombó)") },
		{ TEXT("Bal egér nyomva"), TEXT("Erős csapás") },
		{ TEXT("Jobb egér nyomva"), TEXT("Védekezés (fele sebzés)") },
		{ TEXT("Ugrás + bal egér"), TEXT("Lecsapás a levegőből") },
		{ TEXT("Q"), TEXT("Lángcsapás") },
		{ TEXT("E"), TEXT("Fagyasztó nova") },
		{ TEXT("R"), TEXT("Elemváltás") },
		{ TEXT("X"), TEXT("Fegyverváltás") },
		{ TEXT("T"), TEXT("Képességek") },
		{ TEXT("B"), TEXT("Bolt") },
		{ TEXT("F"), TEXT("Cica simogatása") },
		{ TEXT("P / Esc"), TEXT("Szünet") },
	};
}

void AAmnesiaHUD::DrawDeathScreen(float UIScale)
{
	const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());
	const float Now = GetWorld()->GetTimeSeconds();

	if (!Hero || Hero->GetHealth() > 0.0f)
	{
		DeathStartTime = -1.0f;
		return;
	}

	if (DeathStartTime < 0.0f)
	{
		DeathStartTime = Now;
	}

	const float Age = Now - DeathStartTime;
	const float Alpha = FMath::Clamp(Age / 1.2f, 0.0f, 1.0f);

	// darken with a red tint
	DrawRect(FLinearColor(0.06f, 0.0f, 0.0f, 0.65f * Alpha), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);

	// a dark red band across the screen behind the words, with thin lines and a diamond in the middle of each
	const float BandTop = Canvas->ClipY * 0.36f - 30.0f * UIScale;
	const float BandBottom = Canvas->ClipY * 0.36f + 170.0f * UIScale;
	const FLinearColor BandLine(0.7f, 0.2f, 0.12f, Alpha);

	DrawRect(FLinearColor(0.12f, 0.0f, 0.0f, 0.55f * Alpha), 0.0f, BandTop, Canvas->ClipX, BandBottom - BandTop);
	DrawRect(BandLine, 0.0f, BandTop, Canvas->ClipX, 2.0f * UIScale);
	DrawRect(BandLine, 0.0f, BandBottom - 2.0f * UIScale, Canvas->ClipX, 2.0f * UIScale);
	DrawDiamond(FVector2D(Canvas->ClipX * 0.5f, BandTop), 22.0f * UIScale, BandLine);
	DrawDiamond(FVector2D(Canvas->ClipX * 0.5f, BandBottom), 22.0f * UIScale, BandLine);

	UFont* Font = GEngine->GetMediumFont();
	const FString Title = TEXT("ELESTÉL");
	const float TitleScale = 5.0f * UIScale * (1.0f + 0.15f * (1.0f - Alpha));

	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Title, Width, Height, Font, TitleScale);

	FCanvasTextItem TitleItem(FVector2D((Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * 0.36f), FText::FromString(Title), Font, FLinearColor(0.85f, 0.12f, 0.1f, Alpha));
	TitleItem.Scale = FVector2D(TitleScale);
	TitleItem.bOutlined = true;
	TitleItem.OutlineColor = FLinearColor(FrameColor.R * 0.6f, FrameColor.G * 0.5f, FrameColor.B * 0.3f, Alpha);
	Canvas->DrawItem(TitleItem);

	const FString Sub = TEXT("Visszatérés az utolsó pihenőhelyhez...");
	const float SubScale = 1.6f * UIScale;
	GetTextSize(Sub, Width, Height, Font, SubScale);

	FCanvasTextItem SubItem(FVector2D((Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * 0.36f + 110.0f * UIScale), FText::FromString(Sub), Font, FLinearColor(1.0f, 0.9f, 0.75f, Alpha));
	SubItem.Scale = FVector2D(SubScale);
	SubItem.bOutlined = true;
	SubItem.OutlineColor = FLinearColor(0, 0, 0, Alpha);
	Canvas->DrawItem(SubItem);
}

void AAmnesiaHUD::BeginPlay()
{
	Super::BeginPlay();

	if (FSlateApplication::IsInitialized())
	{
		EscapeProcessor = MakeShared<FAmnesiaEscapeProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(EscapeProcessor);
	}

	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	// the engine puts the volume back to full on every level load, so it is applied again here
	if (ProgressSystem)
	{
		ProgressSystem->ApplyAudioSettings();
	}

	if (ProgressSystem && ProgressSystem->bSkipMainMenuOnce)
	{
		ProgressSystem->bSkipMainMenuOnce = false;

		// continuing reloads the level from the menu: give keyboard, mouse and gamepad back to the game,
		// or the focus stays with the menu that is gone and nothing answers
		if (APlayerController* PC = GetOwningPlayerController())
		{
			PC->SetPause(false);
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());
		}

		FSlateApplication::Get().SetAllUserFocusToGameViewport();
		return;
	}

	if (bShowMainMenu)
	{
		OpenMenu(EAmnesiaMenuMode::Main);
	}
}

void AAmnesiaHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EscapeProcessor.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(EscapeProcessor);
		}

		EscapeProcessor.Reset();
	}

	CloseMenu();

	if (AdminPanel.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(AdminPanel.ToSharedRef());
		AdminPanel.Reset();
	}

	if (SkillScreen.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SkillScreen.ToSharedRef());
		SkillScreen.Reset();
	}

	if (ShopScreen.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShopScreen.ToSharedRef());
		ShopScreen.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AAmnesiaHUD::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// test helper: closes the menu as if New Game was picked
	if (bDebugCloseMenu)
	{
		bDebugCloseMenu = false;

		if (Menu.IsValid())
		{
			OnMenuNewGame();
		}
	}

	// test helper: presses Continue in the main menu
	if (bDebugContinue)
	{
		bDebugContinue = false;

		if (Menu.IsValid())
		{
			OnMenuContinue();
		}
	}

	FAmnesiaSoundSet::bLogPlays = bDebugLogSounds;

	if (bDebugToggleAdmin)
	{
		bDebugToggleAdmin = false;
		ToggleAdminPanel();
	}

	if (bDebugToggleSkills)
	{
		bDebugToggleSkills = false;

		if (SkillScreen.IsValid())
		{
			CloseSkillScreen();

		} else {

			OpenSkillScreen();
		}
	}

	// test helpers: gold for the shop, endless skill points
	if (DebugAddGold != 0)
	{
		if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
		{
			ProgressSystem->AddGold(DebugAddGold);
		}

		DebugAddGold = 0;
	}

	if (DebugInfinitePoints >= 0)
	{
		if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
		{
			ProgressSystem->bInfiniteSkillPoints = DebugInfinitePoints != 0;
		}

		DebugInfinitePoints = -1;
	}

	// test helper: the same level change as the buttons of the admin panel
	if (DebugChangeLevel != 0)
	{
		ChangeHeroLevel(DebugChangeLevel);
		DebugChangeLevel = 0;
	}

	if (bDebugToggleShop)
	{
		bDebugToggleShop = false;

		if (ShopScreen.IsValid())
		{
			CloseShopScreen();

		} else {

			OpenShopScreen();
		}
	}

	if (bDebugBuyDragon)
	{
		bDebugBuyDragon = false;

		if (ShopScreen.IsValid())
		{
			ShopScreen->TryBuy(AmnesiaDragonItemId());
		}
	}

	if (!DebugBuyItem.IsNone())
	{
		const FName Wanted = DebugBuyItem;
		DebugBuyItem = NAME_None;

		if (ShopScreen.IsValid())
		{
			ShopScreen->TryBuy(Wanted);
		}
	}

	if (bAdminClosePending)
	{
		bAdminClosePending = false;

		if (AdminPanel.IsValid())
		{
			ToggleAdminPanel();
		}
	}

	// test helpers for the settings: the page itself, and a graphics choice
	if (bDebugOpenSettings)
	{
		bDebugOpenSettings = false;

		if (Menu.IsValid())
		{
			Menu->ShowSettings();
		}
	}

	if (DebugGraphicsQuality >= 0)
	{
		if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
		{
			ProgressSystem->GetProgress()->GraphicsQuality = FMath::Clamp(DebugGraphicsQuality, 0, 3);
			OnMenuSettingsChanged();
		}

		DebugGraphicsQuality = -1;
	}

	// test helper: goes to the main menu, as the button of the pause menu does
	if (bDebugMainMenu)
	{
		bDebugMainMenu = false;
		OnMenuMainMenu();
	}

	// test helper: opens the pause menu, or resumes if it is open
	if (bDebugTogglePauseMenu)
	{
		bDebugTogglePauseMenu = false;

		if (Menu.IsValid())
		{
			CloseMenuAndResume();

		} else {

			OpenPauseMenu();
		}
	}
}

void AAmnesiaHUD::OpenPauseMenu()
{
	if (!Menu.IsValid() && !AdminPanel.IsValid() && !SkillScreen.IsValid() && !ShopScreen.IsValid())
	{
		OpenMenu(EAmnesiaMenuMode::Pause);
	}
}

bool AAmnesiaHUD::HandleEscape()
{
	// the admin panel closes outside its own key handler, like from its buttons
	if (AdminPanel.IsValid())
	{
		bAdminClosePending = true;
		return true;
	}

	// the shop and the skill screen close first
	if (ShopScreen.IsValid())
	{
		ShopScreen->GoBack();
		return true;
	}

	if (SkillScreen.IsValid())
	{
		SkillScreen->GoBack();
		return true;
	}

	// settings go back to the buttons, the pause menu resumes, the main menu stays
	if (Menu.IsValid())
	{
		Menu->GoBack();
		return true;
	}

	OpenPauseMenu();
	return Menu.IsValid();
}

void AAmnesiaHUD::OpenSkillScreen()
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());

	if (SkillScreen.IsValid() || ShopScreen.IsValid() || AdminPanel.IsValid() || !Hero || Hero->GetHealth() <= 0.0f || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	// the main menu is not the place for it, the pause menu hands over to the skill screen
	if (Menu.IsValid())
	{
		if (!bMenuIsPause)
		{
			return;
		}

		bSkillScreenFromPause = true;
		CloseMenu();

	} else {

		bSkillScreenFromPause = false;
	}

	// load the art, fonts and sounds once, and keep them alive for the brushes
	auto LoadSkillAsset = [this](const TSoftObjectPtr<UObject>& Asset) -> UObject*
	{
		UObject* Loaded = Asset.IsNull() ? nullptr : Asset.LoadSynchronous();

		if (Loaded)
		{
			LoadedMenuAssets.AddUnique(Loaded);
		}

		return Loaded;
	};

	FAmnesiaSkillIcons Icons;

	for (const FAmnesiaSkillDef& Def : FAmnesiaSkillTree::All())
	{
		if (UObject* Icon = GetSkillIcon(Def.Id))
		{
			Icons.Add(Def.Id, Icon);
		}
	}

	SkillScreen = SNew(SAmnesiaSkillScreen)
		.Skills(Hero->GetSkills())
		.FrameTexture(LoadSkillAsset(MenuFrameTexture))
		.DividerTexture(LoadSkillAsset(MenuDividerTexture))
		.Emblems(MakeDecorIcons(LoadSkillAsset, DragonPortrait))
		.FontRegular(LoadSkillAsset(SkillFontRegular))
		.FontBold(LoadSkillAsset(SkillFontBold))
		.ClickSound(LoadSkillAsset(MenuClickSound))
		.HoverSound(LoadSkillAsset(MenuHoverSound))
		.Icons(Icons)
		.OnClose(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::CloseSkillScreen));

	GEngine->GameViewport->AddViewportWidgetContent(SkillScreen.ToSharedRef(), 55);

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(SkillScreen);
		PC->SetInputMode(InputMode);
	}

	SkillScreen->FocusFirstNode();
}

void AAmnesiaHUD::CloseSkillScreen()
{
	if (!SkillScreen.IsValid())
	{
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SkillScreen.ToSharedRef());
	}

	SkillScreen.Reset();

	// the pause menu comes back if that is where the screen was opened from
	if (bSkillScreenFromPause)
	{
		bSkillScreenFromPause = false;
		OpenMenu(EAmnesiaMenuMode::Pause);
		return;
	}

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

UObject* AAmnesiaHUD::GetSkillIcon(FName Id)
{
	if (const TObjectPtr<UObject>* Found = SkillIcons.Find(Id))
	{
		return Found->Get();
	}

	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(Id);
	UObject* Icon = Def ? TSoftObjectPtr<UObject>(FSoftObjectPath(Def->Icon)).LoadSynchronous() : nullptr;

	SkillIcons.Add(Id, Icon);
	return Icon;
}

void AAmnesiaHUD::DrawSkillBar(float UIScale)
{
	const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());
	UAmnesiaSkillComponent* Skills = Hero ? Hero->GetSkills() : nullptr;

	if (!Skills || Hero->GetHealth() <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector2D Origin = HeroBarPosition * UIScale;
	const FVector2D BarSize = HeroBarSize * UIScale;

	UFont* Font = GEngine->GetMediumFont();

	auto DrawLabel = [this, Font](const FString& Text, float X, float Y, float Scale, const FLinearColor& Color)
	{
		FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
		Item.Scale = FVector2D(Scale, Scale);
		Item.bOutlined = true;
		Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
		Canvas->DrawItem(Item);
	};

	float X = Origin.X;
	float Y = Origin.Y + BarSize.Y + 40.0f * UIScale;

	// the weapon in the hand and the key that changes it, once there is more than one weapon to change between
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		int32 Owned = 1;

		for (const FAmnesiaWeaponDef& Def : FAmnesiaWeapons::All())
		{
			if (Def.Id != FAmnesiaWeapons::SwordId() && ProgressSystem->Owns(Def.Id))
			{
				++Owned;
			}
		}

		if (Owned > 1)
		{
			const FAmnesiaWeaponDef* Equipped = Hero->GetEquippedWeaponDef();
			DrawLabel(FString::Printf(TEXT("X  -  fegyver: %s"), Equipped ? *Equipped->Name : TEXT("Kard")), X, Y, 1.3f * UIScale, FLinearColor(0.95f, 0.88f, 0.7f, 0.9f));
			Y += 30.0f * UIScale;
		}
	}

	// unspent skill points
	const int32 FreePoints = Skills->GetFreePoints();

	if (FreePoints > 0)
	{
		const float Pulse = 0.72f + 0.28f * FMath::Sin(Now * 4.0f);
		const FString PointsText = Skills->HasInfinitePoints() ? FString(TEXT("T  -  végtelen képességpont")) : FString::Printf(TEXT("T  -  %d képességpont"), FreePoints);
		DrawLabel(PointsText, X, Y, 1.5f * UIScale, FLinearColor(1.0f, 0.82f, 0.35f, Pulse));
		Y += 36.0f * UIScale;
	}

	// the skills used with a key: the icon, the recharge as a dark curtain, the key
	const float Slot = 60.0f * UIScale;
	const float Gap = 10.0f * UIScale;
	float SlotX = X;
	bool bAnyShown = false;

	const EAmnesiaActive Actives[2] = { EAmnesiaActive::FlameBurst, EAmnesiaActive::FrostNova };
	const TCHAR* Keys[2] = { TEXT("Q"), TEXT("E") };
	const TCHAR* Ids[2] = { TEXT("fire_burst"), TEXT("ice_nova") };

	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (!Skills->IsActiveUnlocked(Actives[Index]))
		{
			continue;
		}

		bAnyShown = true;

		const float Left = Skills->GetCooldownLeft(Actives[Index]);
		const float Total = FMath::Max(Skills->GetCooldownTotal(Actives[Index]), 0.01f);
		const FLinearColor Color = FAmnesiaSkillTree::BranchColor(Index == 0 ? EAmnesiaSkillBranch::Fire : EAmnesiaSkillBranch::Ice);
		const float Frame = 3.0f * UIScale;

		DrawRect(Left > 0.0f ? FLinearColor(0.35f, 0.3f, 0.25f, 1.0f) : Color, SlotX - Frame, Y - Frame, Slot + Frame * 2.0f, Slot + Frame * 2.0f);
		DrawRect(BackgroundColor, SlotX, Y, Slot, Slot);

		if (UTexture* Icon = Cast<UTexture>(GetSkillIcon(FName(Ids[Index]))))
		{
			DrawTexture(Icon, SlotX, Y, Slot, Slot, 0.0f, 0.0f, 1.0f, 1.0f, Left > 0.0f ? FLinearColor(0.6f, 0.6f, 0.6f, 1.0f) : FLinearColor::White);
		}

		if (Left > 0.0f)
		{
			const float Fraction = FMath::Clamp(Left / Total, 0.0f, 1.0f);
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), SlotX, Y, Slot, Slot * Fraction);
			DrawLabel(FString::Printf(TEXT("%d"), FMath::CeilToInt(Left)), SlotX + Slot * 0.3f, Y + Slot * 0.18f, 1.6f * UIScale, FLinearColor(1.0f, 0.95f, 0.85f, 1.0f));
		}

		DrawLabel(Keys[Index], SlotX + Slot - 20.0f * UIScale, Y + Slot - 30.0f * UIScale, 1.55f * UIScale, FLinearColor(1.0f, 0.92f, 0.6f, 1.0f));

		SlotX += Slot + Gap;
	}

	// the element on the sword, changed with R
	const FAmnesiaSkillStats& Stats = Skills->GetStats();

	if (Stats.bFireBlade || Stats.bIceBlade)
	{
		const EAmnesiaElement Element = Skills->GetElement();
		const FLinearColor ElementColor = Element == EAmnesiaElement::Fire ? FAmnesiaSkillTree::BranchColor(EAmnesiaSkillBranch::Fire)
			: (Element == EAmnesiaElement::Ice ? FAmnesiaSkillTree::BranchColor(EAmnesiaSkillBranch::Ice) : FLinearColor(0.85f, 0.85f, 0.8f, 1.0f));

		DrawDiamond(FVector2D(SlotX + 22.0f * UIScale, Y + Slot * 0.5f), 38.0f * UIScale, FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		DrawDiamond(FVector2D(SlotX + 22.0f * UIScale, Y + Slot * 0.5f), 30.0f * UIScale, ElementColor);
		DrawLabel(TEXT("R"), SlotX + 52.0f * UIScale, Y + Slot * 0.5f - 12.0f * UIScale, 1.1f * UIScale, FLinearColor(0.9f, 0.85f, 0.7f, 1.0f));
		bAnyShown = true;
	}

	if (bAnyShown)
	{
		Y += Slot + 16.0f * UIScale;
	}

	// the dragon's shield: a thin bar that fills while it comes back
	const float Guard = Skills->GetGuardCharge();

	if (Guard >= 0.0f)
	{
		const float Width = 150.0f * UIScale;
		const float Height = 7.0f * UIScale;
		const FLinearColor GuardColor = Guard >= 1.0f ? FLinearColor(0.5f, 1.0f, 0.55f, 1.0f) : FLinearColor(0.35f, 0.6f, 0.4f, 1.0f);

		DrawRect(FrameColor, X - 2.0f * UIScale, Y - 2.0f * UIScale, Width + 4.0f * UIScale, Height + 4.0f * UIScale);
		DrawRect(BackgroundColor, X, Y, Width, Height);
		DrawRect(GuardColor, X, Y, Width * Guard, Height);
		DrawLabel(TEXT("Sárkánypajzs"), X + Width + 10.0f * UIScale, Y - 10.0f * UIScale, 1.15f * UIScale, FLinearColor(0.75f, 0.9f, 0.75f, 0.9f));
	}
}

void AAmnesiaHUD::OpenShopScreen()
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());

	if (ShopScreen.IsValid() || SkillScreen.IsValid() || AdminPanel.IsValid() || !Hero || Hero->GetHealth() <= 0.0f || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	// the main menu is not the place for it, the pause menu hands over to the shop
	if (Menu.IsValid())
	{
		if (!bMenuIsPause)
		{
			return;
		}

		bShopScreenFromPause = true;
		CloseMenu();

	} else {

		bShopScreenFromPause = false;
	}

	// load the art, fonts and sounds once, and keep them alive for the brushes
	auto LoadShopAsset = [this](const TSoftObjectPtr<UObject>& Asset) -> UObject*
	{
		UObject* Loaded = Asset.IsNull() ? nullptr : Asset.LoadSynchronous();

		if (Loaded)
		{
			LoadedMenuAssets.AddUnique(Loaded);
		}

		return Loaded;
	};

	// the dragon's picture, or its skill icon if there is no picture
	FAmnesiaShopImages Images;
	UObject* Portrait = LoadShopAsset(DragonPortrait);
	Images.Add(AmnesiaDragonItemId(), Portrait ? Portrait : GetSkillIcon(FName("dragon_fire")));

	// the picture of every weapon
	for (const FAmnesiaWeaponDef& Weapon : FAmnesiaWeapons::All())
	{
		if (Weapon.Id != FAmnesiaWeapons::SwordId() && !Weapon.IconPath.IsEmpty())
		{
			Images.Add(Weapon.Id, LoadShopAsset(TSoftObjectPtr<UObject>(FSoftObjectPath(Weapon.IconPath))));
		}
	}

	// the icons of the skills, for the pages of the things that open them
	for (const FAmnesiaSkillDef& Skill : FAmnesiaSkillTree::All())
	{
		if (UObject* Icon = GetSkillIcon(Skill.Id))
		{
			Images.Add(Skill.Id, Icon);
		}
	}

	TWeakObjectPtr<AAmnesiaHUD> WeakThis(this);

	ShopScreen = SNew(SAmnesiaShopScreen)
		.FrameTexture(LoadShopAsset(MenuFrameTexture))
		.DividerTexture(LoadShopAsset(MenuDividerTexture))
		.Emblems(MakeDecorIcons(LoadShopAsset, DragonPortrait))
		.FontRegular(LoadShopAsset(SkillFontRegular))
		.FontBold(LoadShopAsset(SkillFontBold))
		.ClickSound(LoadShopAsset(MenuClickSound))
		.HoverSound(LoadShopAsset(MenuHoverSound))
		.PurchaseSound(LoadShopAsset(PurchaseSound))
		.Images(Images)
		.GetGold([WeakThis]() -> int32
		{
			UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(WeakThis.Get());
			return ProgressSystem ? ProgressSystem->GetGold() : 0;
		})
		.IsOwned([WeakThis](FName Id) -> bool
		{
			UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(WeakThis.Get());
			return ProgressSystem && ProgressSystem->Owns(Id);
		})
		.OnBuy(SAmnesiaShopScreen::FOnShopBuy::CreateUObject(this, &AAmnesiaHUD::BuyShopItem))
		.OnClose(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::CloseShopScreen));

	GEngine->GameViewport->AddViewportWidgetContent(ShopScreen.ToSharedRef(), 55);

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ShopScreen);
		PC->SetInputMode(InputMode);
	}

	ShopScreen->FocusFirst();
}

void AAmnesiaHUD::CloseShopScreen()
{
	if (!ShopScreen.IsValid())
	{
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShopScreen.ToSharedRef());
	}

	ShopScreen.Reset();

	// the pause menu comes back if that is where the shop was opened from
	if (bShopScreenFromPause)
	{
		bShopScreenFromPause = false;
		OpenMenu(EAmnesiaMenuMode::Pause);
		return;
	}

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

bool AAmnesiaHUD::BuyShopItem(FName Id)
{
	const FAmnesiaShopItem* Item = FAmnesiaShop::Find(Id);
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());

	if (!Item || !ProgressSystem || !Hero || ProgressSystem->Owns(Id) || !ProgressSystem->SpendGold(Item->Price))
	{
		return false;
	}

	ProgressSystem->GiveItem(Id);

	// the dragon flies in when the shop closes, the banner tells what opened up
	if (Id == AmnesiaDragonItemId())
	{
		Hero->SummonDragon();
		ShowBanner(FText::FromString(TEXT("A SÁRKÁNY CSATLAKOZOTT")), FText::FromString(TEXT("Megnyílt a Sárkány képességág")));

	} else if (const FAmnesiaWeaponDef* Weapon = FAmnesiaWeapons::Find(Id)) {

		// the new weapon is in the hand at once, and its branch of skills is open
		Hero->EquipWeapon(Id);
		ShowBanner(FText::FromString(FString::Printf(TEXT("ÚJ FEGYVER: %s"), *Weapon->NameUpper)), FText::FromString(TEXT("Új képességág nyílt meg")));
	}

	ProgressSystem->SaveGame();
	return true;
}

void AAmnesiaHUD::DrawPaw(const FVector2D& Center, float Size, const FLinearColor& Color)
{
	auto Disc = [this](const FVector2D& At, float Radius, const FLinearColor& DiscColor)
	{
		FCanvasNGonItem Item(At, FVector2D(Radius, Radius), 20, DiscColor);
		Canvas->DrawItem(Item);
	};

	// the pad and four toes, each first in a dark outline a little bigger than itself
	const FLinearColor Outline(0.05f, 0.03f, 0.02f, Color.A);
	const float Grow = Size * 0.06f;

	const FVector2D Pad = Center + FVector2D(0.0f, Size * 0.2f);
	const float PadRadius = Size * 0.3f;
	const FVector2D Toes[4] = { Center + FVector2D(-Size * 0.36f, -Size * 0.06f), Center + FVector2D(-Size * 0.13f, -Size * 0.3f),
		Center + FVector2D(Size * 0.13f, -Size * 0.3f), Center + FVector2D(Size * 0.36f, -Size * 0.06f) };
	const float ToeRadius = Size * 0.13f;

	Disc(Pad, PadRadius + Grow, Outline);

	for (const FVector2D& Toe : Toes)
	{
		Disc(Toe, ToeRadius + Grow, Outline);
	}

	Disc(Pad, PadRadius, Color);

	for (const FVector2D& Toe : Toes)
	{
		Disc(Toe, ToeRadius, Color);
	}
}

void AAmnesiaHUD::DrawCats(float UIScale)
{
	const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	if (!Hero || !ProgressSystem || Hero->GetHealth() <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	UFont* Font = GEngine->GetMediumFont();

	auto DrawCatText = [this, Font](const FString& Text, float CenterX, float Top, float Scale, const FLinearColor& Color)
	{
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Text, Width, Height, Font, Scale);

		FCanvasTextItem Item(FVector2D(CenterX - Width * 0.5f, Top), FText::FromString(Text), Font, Color);
		Item.Scale = FVector2D(Scale, Scale);
		Item.bOutlined = true;
		Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.9f);
		Canvas->DrawItem(Item);
	};

	// the Zzz of the sleepers that are near: three letters that rise and fade one after the other
	for (const TWeakObjectPtr<AAmnesiaCat>& Weak : AAmnesiaCat::GetAll())
	{
		const AAmnesiaCat* Cat = Weak.Get();

		if (!Cat || Cat->IsHidden() || !Cat->IsAsleep())
		{
			continue;
		}

		const float Distance = FMath::Abs(Hero->GetActorLocation().X - Cat->GetActorLocation().X);

		if (Distance > Cat->GetNoticeRange())
		{
			continue;
		}

		const FVector Screen = Project(Cat->GetMarkerLocation(), false);

		if (Screen.Z <= 0.0f)
		{
			continue;
		}

		// fades in over the last third of the distance
		const float Visible = FMath::Clamp((Cat->GetNoticeRange() - Distance) / (Cat->GetNoticeRange() * 0.3f), 0.0f, 1.0f);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float Phase = FMath::Fmod(Now * 0.5f + Index * 0.34f, 1.0f);
			const float Rise = Phase * 44.0f * UIScale;
			const float Sway = FMath::Sin(Now * 1.7f + Index * 1.3f) * 7.0f * UIScale;
			const float Scale = (1.0f + 0.9f * Phase) * UIScale;
			const float Alpha = FMath::Sin(Phase * UE_PI) * Visible;

			DrawCatText(Index == 2 ? TEXT("Z") : TEXT("z"), Screen.X + Sway + Phase * 16.0f * UIScale, Screen.Y - Rise - 20.0f * UIScale, Scale, FLinearColor(0.75f, 0.88f, 1.0f, Alpha));
		}
	}

	// the prompt over the cat the hero can pet. Not during the hop: the popups of the reward float up from the same place
	if (const AAmnesiaCat* Near = AAmnesiaCat::FindPettable(Hero))
	{
		const FVector Screen = Project(Near->GetMarkerLocation() + FVector(0.0f, 0.0f, 10.0f), false);

		if (Screen.Z > 0.0f && !Near->IsHopping())
		{
			const float Pulse = 0.75f + 0.25f * FMath::Sin(Now * 5.0f);
			DrawCatText(Near->IsAsleep() ? TEXT("F  -  Megsimogat") : TEXT("F  -  Simogatás"), Screen.X, Screen.Y - 46.0f * UIScale, 1.4f * UIScale, FLinearColor(1.0f, 0.86f, 0.5f, Pulse));
		}
	}

	// the counter in the top right corner: a paw and how many of the cats were found. It glows when a new one is found
	const int32 Total = ProgressSystem->GetCatTotal();

	if (Total <= 0)
	{
		return;
	}

	const int32 Found = ProgressSystem->GetCatsFound();

	if (Found != LastShownCats)
	{
		if (LastShownCats >= 0 && Found > LastShownCats)
		{
			CatGlowStart = Now;
		}

		LastShownCats = Found;
	}

	const float Glow = FMath::Clamp(1.0f - (Now - CatGlowStart) / 0.9f, 0.0f, 1.0f);
	const FLinearColor Color = FMath::Lerp(Found >= Total ? FLinearColor(1.0f, 0.9f, 0.5f, 1.0f) : FLinearColor(0.95f, 0.8f, 0.55f, 1.0f), FLinearColor::White, Glow);
	const FString Text = FString::Printf(TEXT("%d / %d"), Found, Total);
	const float Scale = (1.45f + 0.25f * Glow) * UIScale;

	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);

	const float Right = Canvas->ClipX - 52.0f * UIScale;
	const float Top = 22.0f * UIScale;

	FCanvasTextItem Item(FVector2D(Right - TextWidth, Top), FText::FromString(Text), Font, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.bOutlined = true;
	Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
	Canvas->DrawItem(Item);

	DrawPaw(FVector2D(Right - TextWidth - 24.0f * UIScale, Top + TextHeight * 0.5f), 30.0f * UIScale, Color);
}

void AAmnesiaHUD::DrawCoin(const FVector2D& Center, float Radius)
{
	// a gold coin from a few discs: a dark rim, the face, an inner ring
	auto Disc = [this, &Center](float DiscRadius, const FLinearColor& Color)
	{
		FCanvasNGonItem Item(Center, FVector2D(DiscRadius, DiscRadius), 28, Color);
		Canvas->DrawItem(Item);
	};

	Disc(Radius, FLinearColor(0.1f, 0.045f, 0.004f, 1.0f));
	Disc(Radius * 0.86f, FLinearColor(0.75f, 0.45f, 0.07f, 1.0f));
	Disc(Radius * 0.64f, FLinearColor(0.38f, 0.19f, 0.02f, 1.0f));
	Disc(Radius * 0.54f, FLinearColor(0.85f, 0.56f, 0.1f, 1.0f));
}

void AAmnesiaHUD::DrawGold(const FVector2D& BarPosition, const FVector2D& BarSize, float UIScale)
{
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	if (!ProgressSystem)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const int32 Gold = ProgressSystem->GetGold();

	// the number glows for a moment when gold comes in
	if (Gold != LastShownGold)
	{
		if (LastShownGold >= 0 && Gold > LastShownGold)
		{
			GoldGlowStart = Now;
		}

		LastShownGold = Gold;
	}

	const float Glow = FMath::Clamp(1.0f - (Now - GoldGlowStart) / 0.7f, 0.0f, 1.0f);
	const FLinearColor Color = FMath::Lerp(FLinearColor(1.0f, 0.82f, 0.35f, 1.0f), FLinearColor::White, Glow);

	UFont* Font = GEngine->GetMediumFont();
	const float Scale = (1.55f + 0.3f * Glow) * UIScale;
	const FString Text = FString::Printf(TEXT("%d"), Gold);

	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);

	// at the right end of the bar, level with the name
	const float Right = BarPosition.X + BarSize.X;
	const float Top = BarPosition.Y - 46.0f * UIScale;
	const float Radius = 12.0f * UIScale;

	FCanvasTextItem Item(FVector2D(Right - TextWidth, Top), FText::FromString(Text), Font, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.bOutlined = true;
	Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
	Canvas->DrawItem(Item);

	DrawCoin(FVector2D(Right - TextWidth - 10.0f * UIScale - Radius, Top + TextHeight * 0.5f), Radius);
}

void AAmnesiaHUD::ChangeHeroLevel(int32 Steps)
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn());

	if (!Hero || Steps == 0)
	{
		return;
	}

	// Lvl keeps the level between 1 and the highest one, starts the new level with no XP, heals and keeps the level in the progress
	Hero->Lvl(Hero->GetHeroLevel() + Steps);
}

void AAmnesiaHUD::ToggleAdminPanel()
{
	APlayerController* PC = GetOwningPlayerController();

	if (!PC || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	// closing: back to the game
	if (AdminPanel.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(AdminPanel.ToSharedRef());
		AdminPanel.Reset();

		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		return;
	}

	// not over the main or pause menu
	if (Menu.IsValid())
	{
		return;
	}

	// zones and the rest place each one starts at
	static const TArray<TPair<FString, int32>> Zones = {
		{ TEXT("Erdő"), 0 }, { TEXT("Falu"), 2 }, { TEXT("Havas erdő"), 4 },
		{ TEXT("Romok"), 5 }, { TEXT("Barlang"), 7 }, { TEXT("Vihar"), 9 },
	};

	TArray<FString> Names;

	for (const TPair<FString, int32>& Zone : Zones)
	{
		Names.Add(Zone.Key);
	}

	TWeakObjectPtr<AAmnesiaHUD> WeakThis(this);

	auto GetHero = [WeakThis]() -> AAmnesiaCharacter*
	{
		return WeakThis.IsValid() ? Cast<AAmnesiaCharacter>(WeakThis->GetOwningPawn()) : nullptr;
	};

	AdminPanel = SNew(SAmnesiaAdminPanel)
		.ZoneNames(Names)
		.IsGod([GetHero]() { const AAmnesiaCharacter* Hero = GetHero(); return Hero && Hero->IsInvulnerable(); })
		.SetGod([GetHero](bool bOn) { if (AAmnesiaCharacter* Hero = GetHero()) { Hero->God(bOn ? 1 : 0); } })
		.IsFlying([GetHero]() { const AAmnesiaCharacter* Hero = GetHero(); return Hero && Hero->IsFlying(); })
		.SetFlying([GetHero](bool bOn) { if (AAmnesiaCharacter* Hero = GetHero()) { Hero->Fly(bOn ? 1 : 0); } })
		.IsInfinitePoints([WeakThis]() { const UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(WeakThis.Get()); return ProgressSystem && ProgressSystem->bInfiniteSkillPoints; })
		.SetInfinitePoints([WeakThis](bool bOn) { if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(WeakThis.Get())) { ProgressSystem->bInfiniteSkillPoints = bOn; } })
		.AddGold([WeakThis](int32 Amount) { if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(WeakThis.Get())) { ProgressSystem->AddGold(Amount); } })
		.GetLevel([GetHero]() -> int32 { const AAmnesiaCharacter* Hero = GetHero(); return Hero ? Hero->GetHeroLevel() : 1; })
		.GetMaxLevel([GetHero]() -> int32 { const AAmnesiaCharacter* Hero = GetHero(); return Hero ? Hero->GetMaxLevel() : 1; })
		.ChangeLevel([WeakThis](int32 Steps) { if (WeakThis.IsValid()) { WeakThis->ChangeHeroLevel(Steps); } })
		.Teleport([GetHero, WeakThis](int32 Index)
		{
			AAmnesiaCharacter* Hero = GetHero();

			if (Hero && Hero->GetHealth() > 0.0f && Zones.IsValidIndex(Index))
			{
				Hero->Checkpoint(Zones[Index].Value);
			}

			// straight back into the game at the new place. The panel closes next frame, not inside its own click
			if (WeakThis.IsValid())
			{
				WeakThis->bAdminClosePending = true;
			}
		})
		.OnClose(FSimpleDelegate::CreateLambda([WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->bAdminClosePending = true;
			}
		}));

	GEngine->GameViewport->AddViewportWidgetContent(AdminPanel.ToSharedRef(), 60);

	PC->SetPause(true);
	PC->SetShowMouseCursor(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(AdminPanel);
	PC->SetInputMode(InputMode);

	AdminPanel->FocusFirst();
}

void AAmnesiaHUD::OpenMenu(EAmnesiaMenuMode Mode)
{
	CloseMenu();

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	// load the art and sounds once, and keep them alive for the brushes
	auto LoadMenuAsset = [this](const TSoftObjectPtr<UObject>& Asset) -> UObject*
	{
		UObject* Loaded = Asset.IsNull() ? nullptr : Asset.LoadSynchronous();

		if (Loaded)
		{
			LoadedMenuAssets.AddUnique(Loaded);
		}

		return Loaded;
	};

	Menu = SNew(SAmnesiaMenu)
		.FrameTexture(LoadMenuAsset(MenuFrameTexture))
		.DividerTexture(LoadMenuAsset(MenuDividerTexture))
		.Emblems(MakeDecorIcons(LoadMenuAsset, DragonPortrait))
		.ClickSound(LoadMenuAsset(MenuClickSound))
		.HoverSound(LoadMenuAsset(MenuHoverSound))
		.FontRegular(LoadMenuAsset(SkillFontRegular))
		.FontBold(LoadMenuAsset(SkillFontBold))
		.Mode(Mode)
		.bCanContinue(ProgressSystem && ProgressSystem->HasSavedGame() && ProgressSystem->GetProgress()->CheckpointIndex >= 0)
		.Settings(ProgressSystem ? ProgressSystem->GetProgress() : nullptr)
		.ControlLines(GetControlLines())
		.OnNewGame(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OnMenuNewGame))
		.OnContinue(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OnMenuContinue))
		.OnResume(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::CloseMenuAndResume))
		.OnSkills(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OpenSkillScreen))
		.OnShop(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OpenShopScreen))
		.OnMainMenu(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OnMenuMainMenu))
		.OnQuit(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OnMenuQuit))
		.OnSettingsChanged(FSimpleDelegate::CreateUObject(this, &AAmnesiaHUD::OnMenuSettingsChanged));

	GEngine->GameViewport->AddViewportWidgetContent(Menu.ToSharedRef(), 50);
	bMenuIsPause = Mode == EAmnesiaMenuMode::Pause;

	if (APlayerController* PC = GetOwningPlayerController())
	{
		// the main menu looks over the forest from its own camera
		if (Mode == EAmnesiaMenuMode::Main)
		{
			// a camera tagged MenuCamera in the level frames the menu view, so it can be placed by hand
			FTransform CameraTransform = MenuCameraTransform;

			for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
			{
				if (It->ActorHasTag(FName("MenuCamera")))
				{
					CameraTransform = It->GetActorTransform();
					break;
				}
			}

			if (!MenuCamera.IsValid())
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				MenuCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraTransform, Params);

				// fill the whole viewport, no letterbox bars
				if (MenuCamera.IsValid())
				{
					MenuCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
				}
			}

			if (MenuCamera.IsValid())
			{
				PC->SetViewTarget(MenuCamera.Get());
			}
		}

		PC->SetPause(true);
		PC->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Menu);
		PC->SetInputMode(InputMode);
	}

	Menu->FocusFirstButton();
}

void AAmnesiaHUD::CloseMenu()
{
	if (Menu.IsValid())
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
		}

		Menu.Reset();
	}
}

void AAmnesiaHUD::CloseMenuAndResume()
{
	CloseMenu();

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());

		// glide from the menu view down to the hero
		if (MenuCamera.IsValid() && PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(PC->GetPawn(), 1.8f, VTBlend_EaseInOut, 2.0f);
			MenuCamera->SetLifeSpan(2.5f);
			MenuCamera.Reset();
		}
	}
}

void AAmnesiaHUD::OnMenuNewGame()
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->StartNewGame();

		// the level still holds the old game (dead enemies, lit checkpoints), so it starts again from the beginning
		if (bReturnedToMainMenu)
		{
			ProgressSystem->bSkipMainMenuOnce = true;
			CloseMenu();
			UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
			return;
		}
	}

	// the hero already in the level read the old progress, so reset it
	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn()))
	{
		Hero->GetSkills()->ReloadFromProgress();
		Hero->ReloadWeaponFromProgress();
		Hero->Lvl(1);
		Hero->ResetLevelUpBanner();
	}

	// the dragon has to be bought again in the new adventure
	TArray<AAmnesiaCompanion*> Dragons;

	for (TActorIterator<AAmnesiaCompanion> It(GetWorld()); It; ++It)
	{
		Dragons.Add(*It);
	}

	for (AAmnesiaCompanion* Dragon : Dragons)
	{
		Dragon->Destroy();
	}

	// the cats fall asleep again: they are all to be found anew
	for (const TWeakObjectPtr<AAmnesiaCat>& Weak : AAmnesiaCat::GetAll())
	{
		if (AAmnesiaCat* Cat = Weak.Get())
		{
			Cat->RefreshFromProgress();
		}
	}

	CloseMenuAndResume();

	// draw the sword for the new adventure
	if (USoundBase* Sound = NewGameSound.IsNull() ? nullptr : NewGameSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void AAmnesiaHUD::OnMenuContinue()
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->LoadGame();
		ProgressSystem->bContinueFromCheckpoint = true;
		ProgressSystem->bPendingCheckpointStart = true;
		ProgressSystem->bSkipMainMenuOnce = true;
	}

	CloseMenu();

	// reload the level so the hero and the lit checkpoints start from the save
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void AAmnesiaHUD::OnMenuMainMenu()
{
	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwningPawn()))
	{
		if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
		{
			ProgressSystem->StoreHero(Hero->GetHeroLevel(), Hero->GetCurrentXP());
			ProgressSystem->SaveGame();
		}
	}

	// the main menu opens over the same level: no reload, the forest stays behind it
	bReturnedToMainMenu = true;
	OpenMenu(EAmnesiaMenuMode::Main);
}

void AAmnesiaHUD::OnMenuQuit()
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->SaveGame();
	}

	CloseMenu();
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false);
}

void AAmnesiaHUD::OnMenuSettingsChanged()
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->ApplyAudioSettings();
		ProgressSystem->ApplyGraphicsSettings();
		ProgressSystem->SaveGame();
	}
}

void AAmnesiaHUD::DrawEnemyBars(float UIScale)
{
	const APawn* Hero = GetOwningPawn();

	for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
	{
		const AAmnesiaEnemy* Enemy = *It;

		// skip dead enemies
		if (Enemy->GetMaxHealth() <= 0.0f || Enemy->GetHealth() <= 0.0f)
		{
			continue;
		}

		const float Ratio = FMath::Clamp(Enemy->GetHealth() / Enemy->GetMaxHealth(), 0.0f, 1.0f);

		// only show full bars when the enemy is close to the hero
		const bool bNearHero = Hero && FVector::Dist(Hero->GetActorLocation(), Enemy->GetActorLocation()) <= EnemyBarRange;

		if (!bNearHero && Ratio >= 1.0f)
		{
			continue;
		}

		const float Trail = UpdateTrail(Enemy, Ratio);

		// draw the bar over the enemy's head
		const FVector WorldPosition = Enemy->GetActorLocation() + FVector(0.0f, 0.0f, Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + EnemyBarHeight);
		const FVector ScreenPosition = Project(WorldPosition, false);

		// skip enemies behind the camera
		if (ScreenPosition.Z <= 0.0f)
		{
			continue;
		}

		const FVector2D Size = EnemyBarSize * UIScale;

		const FVector2D BarPosition(ScreenPosition.X - Size.X * 0.5f, ScreenPosition.Y - Size.Y * 0.5f);

		DrawLifeBar(BarPosition, Size, Ratio, Trail, FMath::RoundToInt(Enemy->GetMaxHealth()), 2.0f * UIScale);

		// level label over the bar, red when the enemy outlevels the hero
		const AAmnesiaCharacter* HeroCharacter = Cast<AAmnesiaCharacter>(Hero);
		const bool bTougher = HeroCharacter && Enemy->GetEnemyLevel() > HeroCharacter->GetHeroLevel();

		const FString EnemyLevelText = FString::Printf(TEXT("Lv %d"), Enemy->GetEnemyLevel());
		const float LevelScale = 1.7f * UIScale;
		const FLinearColor LevelColor = bTougher ? FLinearColor(1.0f, 0.35f, 0.3f) : FLinearColor(1.0f, 0.88f, 0.5f);

		float LevelWidth = 0.0f;
		float LevelHeight = 0.0f;
		GetTextSize(EnemyLevelText, LevelWidth, LevelHeight, GEngine->GetMediumFont(), LevelScale);

		// small framed plate behind the text so it reads against any background
		const float PlateX = BarPosition.X + (Size.X - LevelWidth) * 0.5f - 10.0f * UIScale;
		const float PlateY = BarPosition.Y - LevelHeight - 12.0f * UIScale;
		const float PlateW = LevelWidth + 20.0f * UIScale;
		const float PlateH = LevelHeight + 4.0f * UIScale;
		const float PlateFrame = 2.0f * UIScale;

		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), PlateX, PlateY, PlateW, PlateH);
		DrawRect(bTougher ? FLinearColor(0.8f, 0.15f, 0.12f) : FrameColor, PlateX, PlateY, PlateW, PlateFrame);
		DrawRect(bTougher ? FLinearColor(0.8f, 0.15f, 0.12f) : FrameColor, PlateX, PlateY + PlateH - PlateFrame, PlateW, PlateFrame);
		DrawDiamond(FVector2D(PlateX, PlateY + PlateH * 0.5f), PlateH * 0.7f, bTougher ? FLinearColor(0.8f, 0.15f, 0.12f) : FrameColor);
		DrawDiamond(FVector2D(PlateX + PlateW, PlateY + PlateH * 0.5f), PlateH * 0.7f, bTougher ? FLinearColor(0.8f, 0.15f, 0.12f) : FrameColor);

		FCanvasTextItem LevelItem(FVector2D(PlateX + 10.0f * UIScale, PlateY), FText::FromString(EnemyLevelText), GEngine->GetMediumFont(), LevelColor);
		LevelItem.Scale = FVector2D(LevelScale, LevelScale);
		LevelItem.bOutlined = true;
		LevelItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.95f);
		Canvas->DrawItem(LevelItem);
	}
}

void AAmnesiaHUD::DrawLifeBar(const FVector2D& Position, const FVector2D& Size, float Ratio, float TrailRatio, int32 Segments, float FrameThickness)
{
	const float Inner = FMath::Max(1.0f, FrameThickness * 0.4f);

	// drop shadow
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f), Position.X, Position.Y, Size.X + FrameThickness * 2.0f, Size.Y + FrameThickness * 2.0f);

	// gold outer frame, dark inner frame and the empty background
	DrawRect(FrameColor, Position.X - FrameThickness, Position.Y - FrameThickness, Size.X + FrameThickness * 2.0f, Size.Y + FrameThickness * 2.0f);
	DrawRect(InnerFrameColor, Position.X - Inner, Position.Y - Inner, Size.X + Inner * 2.0f, Size.Y + Inner * 2.0f);
	DrawRect(BackgroundColor, Position.X, Position.Y, Size.X, Size.Y);

	// recently lost HP
	if (TrailRatio > Ratio)
	{
		DrawRect(TrailColor, Position.X + Size.X * Ratio, Position.Y, Size.X * (TrailRatio - Ratio), Size.Y);
	}

	// fill with a darker lower half and a thin highlight for some depth
	if (Ratio > 0.0f)
	{
		const float FillWidth = Size.X * Ratio;

		DrawRect(FillShadeColor, Position.X, Position.Y, FillWidth, Size.Y);
		DrawRect(FillColor, Position.X, Position.Y, FillWidth, Size.Y * 0.55f);
		DrawRect(FLinearColor(1.0f, 0.55f, 0.5f, 0.55f), Position.X, Position.Y + Size.Y * 0.12f, FillWidth, FMath::Max(1.0f, Size.Y * 0.1f));
	}

	// one notch per HP
	if (Segments > 1 && Segments <= 30)
	{
		for (int32 Segment = 1; Segment < Segments; ++Segment)
		{
			const float NotchX = Position.X + Size.X * Segment / Segments;
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), NotchX - Inner * 0.5f, Position.Y, Inner, Size.Y);
		}
	}

	// gem end caps
	const float CapSize = Size.Y + FrameThickness * 3.0f;
	const float CenterY = Position.Y + Size.Y * 0.5f;

	for (const float CapX : { Position.X - FrameThickness, Position.X + Size.X + FrameThickness })
	{
		DrawDiamond(FVector2D(CapX, CenterY), CapSize, FrameColor);
		DrawDiamond(FVector2D(CapX, CenterY), CapSize * 0.5f, FillColor);
	}
}

void AAmnesiaHUD::DrawDiamond(const FVector2D& Center, float Size, const FLinearColor& Color)
{
	// a square turned 45 degrees, sized so its corners span the requested size
	const float Side = Size * 0.7071f;

	FCanvasTileItem Tile(Center - FVector2D(Side * 0.5f, Side * 0.5f), FVector2D(Side, Side), Color);
	Tile.Rotation = FRotator(0.0f, 45.0f, 0.0f);
	Tile.PivotPoint = FVector2D(0.5f, 0.5f);
	Tile.BlendMode = SE_BLEND_Translucent;

	Canvas->DrawItem(Tile);
}

float AAmnesiaHUD::UpdateTrail(const AActor* TrailOwner, float Ratio)
{
	float* Trail = TrailRatios.Find(TrailOwner);

	if (!Trail)
	{
		Trail = &TrailRatios.Add(TrailOwner, Ratio);
	}

	// heals show up right away, damage drains slowly
	*Trail = Ratio > *Trail ? Ratio : FMath::FInterpConstantTo(*Trail, Ratio, GetWorld()->GetDeltaSeconds(), TrailDrainSpeed);

	return *Trail;
}
