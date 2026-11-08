#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AmnesiaHUD.generated.h"

enum class EAmnesiaMenuMode : uint8;
class USoundBase;
class FAmnesiaEscapeProcessor;

/**
 *  RPG style HUD for Amnesia.
 *  - Framed life bar for the hero in the top left corner, with HP numbers
 *  - Matching life bars over nearby or damaged enemies
 *  - Damage trails that drain slowly after each hit
 */
UCLASS()
class AAmnesiaHUD : public AHUD
{
	GENERATED_BODY()

protected:

	/** Name shown over the hero's life bar */
	UPROPERTY(EditAnywhere, Category="HUD")
	FText HeroName = FText::FromString(TEXT("ARDEN"));

	/** Top left corner of the hero's life bar, at 1080p */
	UPROPERTY(EditAnywhere, Category="HUD")
	FVector2D HeroBarPosition = FVector2D(48.0f, 64.0f);

	/** Size of the hero's life bar, at 1080p */
	UPROPERTY(EditAnywhere, Category="HUD")
	FVector2D HeroBarSize = FVector2D(360.0f, 26.0f);

	/** Size of enemy life bars, at 1080p */
	UPROPERTY(EditAnywhere, Category="HUD")
	FVector2D EnemyBarSize = FVector2D(110.0f, 11.0f);

	/** Height above an enemy's capsule where its life bar is drawn */
	UPROPERTY(EditAnywhere, Category="HUD", meta = (Units = "cm"))
	float EnemyBarHeight = 45.0f;

	/** Enemies closer than this to the hero always show their life bar. Others only show it once damaged */
	UPROPERTY(EditAnywhere, Category="HUD", meta = (ClampMin = 0, Units = "cm"))
	float EnemyBarRange = 1600.0f;

	/** How fast the damage trail drains, in bar lengths per second */
	UPROPERTY(EditAnywhere, Category="HUD", meta = (ClampMin = 0.01))
	float TrailDrainSpeed = 0.6f;

	/** Gold outer frame */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor FrameColor = FLinearColor(0.78f, 0.6f, 0.26f, 1.0f);

	/** Dark inner frame */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor InnerFrameColor = FLinearColor(0.1f, 0.06f, 0.03f, 1.0f);

	/** Empty part of the bar */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor BackgroundColor = FLinearColor(0.08f, 0.02f, 0.02f, 0.92f);

	/** Bright upper half of the fill */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor FillColor = FLinearColor(0.8f, 0.08f, 0.1f, 1.0f);

	/** Darker lower half of the fill */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor FillShadeColor = FLinearColor(0.45f, 0.02f, 0.04f, 1.0f);

	/** How long the level up plate stays on screen */
	UPROPERTY(EditAnywhere, Category="HUD", meta = (ClampMin = 1, Units = "s"))
	float LevelUpBannerTime = 3.5f;

	/** XP bar fill */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor XPColor = FLinearColor(0.15f, 0.55f, 0.85f, 1.0f);

	/** Recently lost HP */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor TrailColor = FLinearColor(0.95f, 0.68f, 0.22f, 1.0f);

	/** HP numbers and hero name */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor TextColor = FLinearColor(1.0f, 0.95f, 0.85f, 1.0f);

	/** Gold color of the floating XP numbers */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor XPPopupColor = FLinearColor(1.0f, 0.78f, 0.15f, 1.0f);

	/** How long a floating XP number stays up */
	UPROPERTY(EditAnywhere, Category="HUD", meta = (ClampMin = 0.1, Units = "s"))
	float XPPopupDuration = 1.6f;

	/** How far a floating XP number rises, at 1080p */
	UPROPERTY(EditAnywhere, Category="HUD", meta = (ClampMin = 0))
	float XPPopupRise = 90.0f;

	/** A floating text in the world: XP, damage and heal numbers */
	struct FTextPopup
	{
		FVector WorldLocation = FVector::ZeroVector;
		FString Text;
		FLinearColor Color = FLinearColor::White;
		float Scale = 1.5f;
		float Duration = 1.0f;
		float StartTime = 0.0f;
	};

	TArray<FTextPopup> TextPopups;

	/** If true, the main menu opens when the level starts */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	bool bShowMainMenu = true;

	/** Test helper: set on the running HUD to close the menu as if New Game was picked */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	bool bDebugCloseMenu = false;

	/** Test helper: set on the running HUD to open or close the pause menu */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	bool bDebugTogglePauseMenu = false;

	/** Test helper: set on the running HUD to go to the main menu, as the button of the pause menu does */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	bool bDebugMainMenu = false;

	/** Test helper: set on the running HUD to show the settings page of the open menu */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	bool bDebugOpenSettings = false;

	/** Test helper: set on the running HUD to 0..3 to pick that graphics quality, as the buttons do. Goes back to -1 */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	int32 DebugGraphicsQuality = -1;

	/** Test helper: writes every sound effect that plays to the log */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	bool bDebugLogSounds = false;

	/** Where the main menu camera looks from */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	FTransform MenuCameraTransform = FTransform(FRotator(-7.0f, -68.0f, 0.0f), FVector(1300.0f, 300.0f, 450.0f));

	/** Camera used behind the main menu */
	TWeakObjectPtr<class ACameraActor> MenuCamera;

	/** Menu art and sounds, loaded when a menu opens. The menu falls back to plain gold lines without them */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	TSoftObjectPtr<UObject> MenuFrameTexture = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/UI/FantasyBorders/T_FB_Border_020.T_FB_Border_020")));

	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	TSoftObjectPtr<UObject> MenuDividerTexture = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/UI/FantasyBorders/T_FB_DividerFade_003.T_FB_DividerFade_003")));

	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	TSoftObjectPtr<UObject> MenuClickSound = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Audio/UI/SW_UI_Click_01.SW_UI_Click_01")));

	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	TSoftObjectPtr<UObject> MenuHoverSound = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Audio/UI/SW_UI_Hover_01.SW_UI_Hover_01")));

	/** Played when a new adventure starts */
	UPROPERTY(EditAnywhere, Category="HUD|Menu")
	TSoftObjectPtr<USoundBase> NewGameSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/UI/SW_UI_SwordDraw_01.SW_UI_SwordDraw_01")));

	/** Keeps the loaded menu assets alive while a menu shows them */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedMenuAssets;

	/** The skill screen, if open */
	TSharedPtr<class SAmnesiaSkillScreen> SkillScreen;

	/** True when the skill screen was opened from the pause menu, which comes back when it closes */
	bool bSkillScreenFromPause = false;

	/** The shop, if open */
	TSharedPtr<class SAmnesiaShopScreen> ShopScreen;

	/** True when the shop was opened from the pause menu, which comes back when it closes */
	bool bShopScreenFromPause = false;

	/** The dragon's picture in the shop, and the sound of a purchase */
	UPROPERTY(EditAnywhere, Category="HUD|Shop")
	TSoftObjectPtr<UObject> DragonPortrait = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Amnesia/UI/Shop/T_Shop_Dragon.T_Shop_Dragon")));

	UPROPERTY(EditAnywhere, Category="HUD|Shop")
	TSoftObjectPtr<UObject> PurchaseSound = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Audio/Hero/SW_Hero_LevelUp_01.SW_Hero_LevelUp_01")));

	/** Test helpers: open or close the shop, and press the buy button of the dragon in it */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	bool bDebugToggleShop = false;

	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	bool bDebugBuyDragon = false;

	/** Test helper: the id of something sold in the shop (chakram, axe, wand, dragon) here presses its buy button, once */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	FName DebugBuyItem;

	/** Test helpers: a number here is added to the gold once; 1 or 0 here switches the endless skill points on or off. -1 does nothing */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	int32 DebugAddGold = 0;

	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	int32 DebugInfinitePoints = -1;

	/** Test helper: a number here changes the hero's level by that many steps once, like the buttons of the admin panel */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	int32 DebugChangeLevel = 0;

	/** The gold counter: the number drawn last and when it last grew, for the glow */
	int32 LastShownGold = -1;
	float GoldGlowStart = -100.0f;

	/** Gold of the floating gold numbers */
	UPROPERTY(EditAnywhere, Category="HUD|Colors")
	FLinearColor GoldPopupColor = FLinearColor(1.0f, 0.93f, 0.55f, 1.0f);

	/** True while the menu that is open is the pause menu, not the main menu */
	bool bMenuIsPause = false;

	/** The main menu was opened from a game in progress: the world still holds that game */
	bool bReturnedToMainMenu = false;

	/** Fonts of the skill screen. Without them the engine font is used */
	UPROPERTY(EditAnywhere, Category="HUD|Skills")
	TSoftObjectPtr<UObject> SkillFontRegular = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/UI/Fonts/Cinzel-Regular.Cinzel-Regular")));

	UPROPERTY(EditAnywhere, Category="HUD|Skills")
	TSoftObjectPtr<UObject> SkillFontBold = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/UI/Fonts/Cinzel-Bold.Cinzel-Bold")));

	/** Test helper: opens the skill screen, or closes it if it is open */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	bool bDebugToggleSkills = false;

	/** Icons of the skills, loaded when first needed */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UObject>> SkillIcons;

	/** The admin panel, if open */
	TSharedPtr<class SAmnesiaAdminPanel> AdminPanel;

	/** Set by the panel's buttons; the panel closes on the next tick, outside its own click */
	bool bAdminClosePending = false;

	/** Test helper: set on the running HUD to press Continue in the main menu */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	bool bDebugContinue = false;

	/** Test helper: set on the running HUD to open or close the admin panel */
	UPROPERTY(EditAnywhere, Category="HUD|Debug")
	bool bDebugToggleAdmin = false;

	/** The open menu, if any */
	TSharedPtr<class SAmnesiaMenu> Menu;

	/** Gets the Escape key before the editor does (it would stop the play session) and before the game's input */
	TSharedPtr<FAmnesiaEscapeProcessor> EscapeProcessor;

	/** Checkpoint style banner */
	FString BannerTitle;
	FString BannerSub;
	float BannerStartTime = -100.0f;

	/** Time the hero died, for the death screen fade */
	float DeathStartTime = -1.0f;

	/** Damage trail ratio per actor */
	TMap<TWeakObjectPtr<const AActor>, float> TrailRatios;

public:

	AAmnesiaHUD();

	/** Draws the life bars */
	virtual void DrawHUD() override;

	/** Shows a gold "+N XP" that floats up from a world location */
	void AddXPPopup(const FVector& WorldLocation, int32 Amount);

	/** Shows a "+N arany" that floats up from a world location, a little under where the XP shows */
	void AddGoldPopup(const FVector& WorldLocation, int32 Amount);

	/** Shows a damage or heal number. Respects the damage numbers setting */
	void AddNumberPopup(const FVector& WorldLocation, float Amount, const FLinearColor& Color, const FString& Prefix);

	/** Shows any floating text */
	void AddTextPopup(const FVector& WorldLocation, const FString& Text, const FLinearColor& Color, float Scale, float Duration);

	/** Shows an ornate banner in the middle of the screen, used by checkpoints */
	void ShowBanner(const FText& Title, const FText& Sub);

	/** Opens the pause menu, if no menu is open */
	void OpenPauseMenu();

	/** The Escape key: closes the admin panel, steps back in the open menu, or opens the pause menu. True if it used the key */
	bool HandleEscape();

	/** Opens the admin panel, or closes it if it is open */
	void ToggleAdminPanel();

	/** True while the admin panel is open */
	bool IsAdminPanelOpen() const { return AdminPanel.IsValid(); }

	/** Raises or lowers the hero's level by some steps, within 1 and the highest level. The admin panel's level buttons use it */
	void ChangeHeroLevel(int32 Steps);

	/** Opens the skill screen from the game or from the pause menu */
	void OpenSkillScreen();
	void CloseSkillScreen();
	bool IsSkillScreenOpen() const { return SkillScreen.IsValid(); }

	/** Opens the shop from the game or from the pause menu */
	void OpenShopScreen();
	void CloseShopScreen();
	bool IsShopScreenOpen() const { return ShopScreen.IsValid(); }

	virtual void Tick(float DeltaTime) override;

	/** The control lines shown in the hint panel and the pause menu */
	static TArray<TPair<FString, FString>> GetControlLines();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The skill points hint, the active skills with their recharge, the element and the dragon's shield */
	void DrawSkillBar(float UIScale);

	/** The Zzz over a sleeping cat that is near, the prompt over the cat the hero can pet, and the cat counter in the top right corner */
	void DrawCats(float UIScale);

	/** A paw print from a few discs, for the cat counter */
	void DrawPaw(const FVector2D& Center, float Size, const FLinearColor& Color);

	/** The cat counter: how many were shown last and when it last grew, for the glow */
	int32 LastShownCats = -1;
	float CatGlowStart = -100.0f;

	/** The icon texture of a skill, or null if it is not in the project */
	UObject* GetSkillIcon(FName Id);

	/** Sells an item of the shop: takes the gold, hands over the item and brings the dragon. False if it did not work */
	bool BuyShopItem(FName Id);

	/** The gold counter at the right end of the life bar, with a coin drawn from a few discs */
	void DrawGold(const FVector2D& BarPosition, const FVector2D& BarSize, float UIScale);
	void DrawCoin(const FVector2D& Center, float Radius);

	/** Menu handling */
	void OpenMenu(EAmnesiaMenuMode Mode);
	void CloseMenu();
	void CloseMenuAndResume();
	void OnMenuNewGame();
	void OnMenuContinue();
	void OnMenuMainMenu();
	void OnMenuQuit();
	void OnMenuSettingsChanged();

	/** Draws the ornate plate used by the level up and checkpoint banners */
	void DrawOrnateBanner(const FString& Title, const FString& Sub, float Age, float Duration, float CenterYFraction, float TitleSize, float UIScale);

	/** Draws the checkpoint banner */
	void DrawBanner(float UIScale);

	/** Darkens the screen while the hero is dead */
	void DrawDeathScreen(float UIScale);

	/** Draws and expires the floating XP numbers */
	void DrawXPPopups(float UIScale);

	/** Draws the hero's framed life bar, name and HP numbers */
	void DrawHeroBar(float UIScale);

	/** Draws the golden "SZINTLÉPÉS!" plate in the middle of the screen */
	void DrawLevelUpBanner(int32 NewLevel, float Age, float UIScale);

	/** Draws life bars over enemies */
	void DrawEnemyBars(float UIScale);

	/** Draws one framed life bar */
	void DrawLifeBar(const FVector2D& Position, const FVector2D& Size, float Ratio, float TrailRatio, int32 Segments, float FrameThickness);

	/** Draws a diamond, used for the bar end caps */
	void DrawDiamond(const FVector2D& Center, float Size, const FLinearColor& Color);

	/** Updates and returns the damage trail ratio for an actor */
	float UpdateTrail(const AActor* TrailOwner, float Ratio);
};
