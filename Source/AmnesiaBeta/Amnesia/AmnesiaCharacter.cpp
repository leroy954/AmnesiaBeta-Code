#include "AmnesiaCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "EnhancedInputComponent.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "AmnesiaCheckpoint.h"
#include "AmnesiaCompanion.h"
#include "AmnesiaCat.h"
#include "AmnesiaChakram.h"
#include "AmnesiaHeroAnimInstance.h"
#include "AmnesiaEnemy.h"
#include "AmnesiaFireball.h"
#include "AmnesiaHUD.h"
#include "AmnesiaProgress.h"
#include "AmnesiaSkills.h"
#include "CombatPlayerController.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AAmnesiaCharacter::AAmnesiaCharacter()
{
	// face the movement direction instead of the controller
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CompanionClass = AAmnesiaCompanion::StaticClass();

	// configure the character movement component for side scrolling
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	Movement->GravityScale = 1.75f;
	Movement->MaxAcceleration = 1500.0f;
	Movement->BrakingFrictionFactor = 1.0f;
	Movement->bUseSeparateBrakingFriction = true;

	Movement->MaxWalkSpeed = 500.0f;
	Movement->MinAnalogWalkSpeed = 20.0f;
	Movement->BrakingDecelerationWalking = 2000.0f;

	Movement->JumpZVelocity = 750.0f;
	Movement->AirControl = 1.0f;

	Movement->RotationRate = FRotator(0.0f, 1000.0f, 0.0f);
	Movement->bOrientRotationToMovement = true;

	// keep the character on the side scrolling plane
	Movement->SetPlaneConstraintNormal(FVector(0.0f, 1.0f, 0.0f));
	Movement->bConstrainToPlane = true;
	Movement->bSnapToPlaneAtStart = true;

	// allow double jump and coyote time. The first jump counts twice when we're already falling
	JumpMaxCount = 3;

	// set up the side view camera
	SetupSideCamera();

	// create the hand-held weapon. It's purely visual, hits come from the attack traces
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), WeaponSocketName);
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// also receive the hero light on lighting channel 1
	Weapon->LightingChannels.bChannel1 = true;

	// the model of the other weapons: it sits in the same hand and shows when one of them is equipped
	WeaponProp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponProp"));
	WeaponProp->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponProp->LightingChannels.bChannel1 = true;
	WeaponProp->SetCastShadow(false);
	WeaponProp->SetVisibility(false);

	// weapon aura effect. The Niagara system is picked in the Blueprint
	WeaponAura = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeaponAura"));
	WeaponAura->SetupAttachment(Weapon);
	WeaponAura->SetAutoActivate(true);

	// the skills
	Skills = CreateDefaultSubobject<UAmnesiaSkillComponent>(TEXT("Skills"));

	// ticks only for the debug swing helper
	PrimaryActorTick.bCanEverTick = true;

}

void AAmnesiaCharacter::Fly(int32 Enabled)
{
	bFlying = Enabled < 0 ? !bFlying : Enabled != 0;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (bFlying)
	{
		Movement->MaxFlySpeed = FlySpeed;
		Movement->BrakingDecelerationFlying = 4000.0f;
		Movement->SetMovementMode(MOVE_Flying);

		// the double jump is spent while flying
		bHasDoubleJumped = false;

	} else {

		Movement->SetMovementMode(MOVE_Falling);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, bFlying ? TEXT("Fly: ON (W/S up and down)") : TEXT("Fly: OFF"));
	}
}

void AAmnesiaCharacter::BeginPlay()
{
	// use the side camera distance for the regular and the death camera
	DefaultCameraDistance = SideCameraDistance;
	DeathCameraDistance = SideCameraDistance * 1.25f;

	Super::BeginPlay();

	// switch to the Anim Blueprint that plays swings over running legs
	if (!HeroAnimClass.IsNull())
	{
		if (UClass* AnimClass = HeroAnimClass.LoadSynchronous())
		{
			if (GetMesh()->GetAnimClass() != AnimClass)
			{
				GetMesh()->SetAnimInstanceClass(AnimClass);
			}
		}
	}

	// reapply the side camera in case a Blueprint changed the boom
	SetupSideCamera();

	CurrentCameraPitch = SideCameraPitch;
	TargetCameraPitch = SideCameraPitch;

	// put the weapon in the hand, in case a Blueprint changed the socket
	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, WeaponSocketName);
	WeaponProp->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, WeaponSocketName);

	// what the Blueprint gave the sword is kept, to put the sword back when another weapon is put away
	SwordAuraRelative = WeaponAura->GetRelativeTransform();
	SwordGroundAnimation = GroundSlashAnimation;
	SwordGroundSlashes = GroundSlashes;

	// where the blade is, for the air slash hit checks
	FindBlade();

	// hide parts of the character mesh, like the sheathed weapon
	for (const FName& BoneName : HiddenMeshBones)
	{
		GetMesh()->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
	}

	// the HUD shows our HP, so hide the floating life bar
	if (bHideWorldLifeBar)
	{
		if (UWidgetComponent* LifeBarComponent = FindComponentByClass<UWidgetComponent>())
		{
			LifeBarComponent->SetHiddenInGame(true);
		}
	}

	// remember the walk speed so we can go back to it after sprinting
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;


	// remember the level 1 stats that the level ups build on
	BaseMaxHP = MaxHP;
	BaseMeleeDamage = MeleeDamage;
	BaseAirSlashDamage = AirSlashDamage;

	// the little dragon comes along once it is bought. It outlives the hero, so a respawned hero finds it already flying
	if (UAmnesiaProgressSubsystem* DragonSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		if (DragonSystem->Owns(AmnesiaDragonItemId()))
		{
			SpawnCompanion();
		}
	}

	// far enemies sleep, checked a few times a second
	UpdateEnemyDormancy();
	GetWorld()->GetTimerManager().SetTimer(DormancyTimer, this, &AAmnesiaCharacter::UpdateEnemyDormancy, 0.4f, true);

	// a moment of safety after spawning, and the enemies go back to where they started
	HurtImmuneUntil = GetWorld()->GetTimeSeconds() + 2.5f;

	if (GetWorld()->GetTimeSeconds() > 1.0f)
	{
		for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
		{
			It->ResetToHome();
		}
	}

	// a respawned hero is a new actor, so take the level and XP from the saved progress
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		UAmnesiaSaveGame* Progress = ProgressSystem->GetProgress();

		Level = FMath::Clamp(Progress->Level, 1, MaxLevel);
		CurrentXP = Progress->XP;
		ApplyLevelStats();
		CurrentHP = MaxHP;

		// continuing a saved game starts at its checkpoint
		if (ProgressSystem->bPendingCheckpointStart && Progress->CheckpointIndex >= 0)
		{
			ProgressSystem->bPendingCheckpointStart = false;
			SetActorLocation(Progress->CheckpointTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);

			if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(GetController()))
			{
				PC->SetRespawnTransform(Progress->CheckpointTransform);
			}
		}
	}

	// the weapon of the saved game is in the hand
	ReloadWeaponFromProgress();

	// the skills were loaded with the component, now the hero, the sword and the dragon follow them
	Skills->Refresh();
}

void AAmnesiaCharacter::HealBy(float Amount, bool bQuiet)
{
	if (CurrentHP <= 0.0f || Amount <= 0.0f)
	{
		return;
	}

	const float Before = CurrentHP;
	CurrentHP = FMath::Min(CurrentHP + Amount, MaxHP);

	if (CurrentHP > Before && !bQuiet)
	{
		HealSounds.PlayAt(this, GetActorLocation());

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
			{
				HUD->AddNumberPopup(GetActorLocation() + FVector(0.0f, 0.0f, 130.0f), CurrentHP - Before, FLinearColor(0.35f, 1.0f, 0.4f), TEXT("+"));
			}
		}
	}
}

void AAmnesiaCharacter::HealFully()
{
	HealBy(MaxHP);
}

void AAmnesiaCharacter::OnPausePressed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
		{
			HUD->OpenPauseMenu();
		}
	}
}

void AAmnesiaCharacter::OnAdminPressed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
		{
			HUD->ToggleAdminPanel();
		}
	}
}

void AAmnesiaCharacter::OnFlameBurstPressed()
{
	Skills->TryCast(EAmnesiaActive::FlameBurst);
}

void AAmnesiaCharacter::OnFrostNovaPressed()
{
	Skills->TryCast(EAmnesiaActive::FrostNova);
}

void AAmnesiaCharacter::OnCycleElementPressed()
{
	Skills->CycleElement();
}

void AAmnesiaCharacter::OnSkillScreenPressed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
		{
			HUD->OpenSkillScreen();
		}
	}
}

void AAmnesiaCharacter::OnShopPressed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
		{
			HUD->OpenShopScreen();
		}
	}
}

void AAmnesiaCharacter::OnPetPressed()
{
	if (CurrentHP <= 0.0f)
	{
		return;
	}

	if (AAmnesiaCat* Cat = AAmnesiaCat::FindPettable(this))
	{
		Cat->Pet(this);
	}
}

AAmnesiaCompanion* AAmnesiaCharacter::SpawnCompanion()
{
	if (!CompanionClass)
	{
		return nullptr;
	}

	// one dragon is enough
	if (TActorIterator<AAmnesiaCompanion> Existing(GetWorld()); Existing)
	{
		return *Existing;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return GetWorld()->SpawnActor<AAmnesiaCompanion>(CompanionClass, GetActorLocation() + FVector(0.0f, 0.0f, 160.0f), FRotator::ZeroRotator, Params);
}

void AAmnesiaCharacter::SummonDragon()
{
	if (AAmnesiaCompanion* Dragon = SpawnCompanion())
	{
		// the skills reach the new dragon, and it greets the hero with a roll
		Skills->Refresh();
		Dragon->Celebrate();
	}
}

void AAmnesiaCharacter::GainGold(int32 Amount)
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->AddGold(Amount);
	}
}

void AAmnesiaCharacter::Gold(int32 Amount)
{
	GainGold(Amount);
}

void AAmnesiaCharacter::InfinitePoints(int32 On)
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->bInfiniteSkillPoints = On != 0;
	}
}

void AAmnesiaCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	HealBy(Healing);
}

void AAmnesiaCharacter::HandleDeath()
{
	DeathSounds.PlayAt(this, GetActorLocation());

	// keep the level and XP for the respawned hero
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->StoreHero(Level, CurrentXP);
	}

	GetWorld()->GetTimerManager().ClearTimer(SlashHitTimer);
	GetWorld()->GetTimerManager().ClearTimer(AirSlashHitTimer);
	GetWorld()->GetTimerManager().ClearTimer(AirSlashHitEndTimer);
	bAirSlashHitActive = false;
	bAttackPending = false;
	bGuardHeld = false;
	EndGuard();

	Super::HandleDeath();
}

void AAmnesiaCharacter::AddCameraShake(float Strength)
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		if (!ProgressSystem->GetProgress()->bScreenShake)
		{
			return;
		}
	}

	ShakeStrength = FMath::Max(ShakeStrength, Strength);
}

void AAmnesiaCharacter::StartHitStop(AActor* Target)
{
	if (HitStopTime <= 0.0f)
	{
		return;
	}

	EndHitStop();

	CustomTimeDilation = 0.05f;
	HitStopActors.Add(this);

	if (Target)
	{
		Target->CustomTimeDilation = 0.05f;
		HitStopActors.Add(Target);
	}

	// the world timer runs at normal speed, only the two actors are slowed
	GetWorld()->GetTimerManager().SetTimer(HitStopTimer, this, &AAmnesiaCharacter::EndHitStop, HitStopTime, false);
}

void AAmnesiaCharacter::EndHitStop()
{
	for (const TWeakObjectPtr<AActor>& Actor : HitStopActors)
	{
		if (Actor.IsValid())
		{
			Actor->CustomTimeDilation = 1.0f;
		}
	}

	HitStopActors.Reset();
}

void AAmnesiaCharacter::OnAttackLanded(AActor* Target, const FVector& ImpactPoint, float Damage)
{
	StartHitStop(Target);
	AddCameraShake(HitShakeStrength);

	HitSounds.PlayAt(this, ImpactPoint);
}

void AAmnesiaCharacter::Checkpoint(int32 Index)
{
	for (TActorIterator<AAmnesiaCheckpoint> It(GetWorld()); It; ++It)
	{
		if (It->GetCheckpointIndex() == Index)
		{
			SetActorLocation(It->GetSpawnTransform().GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			return;
		}
	}
}

void AAmnesiaCharacter::UpdateEnemyDormancy()
{
	const FVector Here = GetActorLocation();

	for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
	{
		const FVector There = It->GetActorLocation();
		It->SetDormant(FMath::Abs(There.X - Here.X) > EnemyWakeRange || FMath::Abs(There.Z - Here.Z) > EnemyWakeRange * 0.5f);
	}
}

void AAmnesiaCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(DormancyTimer);

	Super::EndPlay(EndPlayReason);

	// clear the swing and air slash timers
	GetWorld()->GetTimerManager().ClearTimer(SlashHitTimer);
	GetWorld()->GetTimerManager().ClearTimer(SlashEndTimer);
	GetWorld()->GetTimerManager().ClearTimer(SwingSoundTimer);
	GetWorld()->GetTimerManager().ClearTimer(AirSlashHitTimer);
	GetWorld()->GetTimerManager().ClearTimer(AirSlashHitEndTimer);
	GetWorld()->GetTimerManager().ClearTimer(AirSlashEndTimer);
}

void AAmnesiaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAmnesiaCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAmnesiaCharacter::DoJumpEnd);

			// the left button: let go it swings the combo, held it is the heavy attack
			EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Completed, this, &AAmnesiaCharacter::OnAttackReleased);

		// Pause menu. The HUD takes Escape first (the editor would stop the game with it), these are the fallback
		PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AAmnesiaCharacter::OnPausePressed);
		PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AAmnesiaCharacter::OnPausePressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AAmnesiaCharacter::OnPausePressed);

		// admin panel for testing
		PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &AAmnesiaCharacter::OnAdminPressed);

		// skills: Q and E are the active skills, R changes the element on the sword, T opens the skill screen
		PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AAmnesiaCharacter::OnFlameBurstPressed);
		PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AAmnesiaCharacter::OnFrostNovaPressed);
		PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AAmnesiaCharacter::OnCycleElementPressed);
		PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &AAmnesiaCharacter::OnSkillScreenPressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AAmnesiaCharacter::OnFlameBurstPressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AAmnesiaCharacter::OnFrostNovaPressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_LeftShoulder, IE_Pressed, this, &AAmnesiaCharacter::OnCycleElementPressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &AAmnesiaCharacter::OnSkillScreenPressed);

		// the shop
		PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AAmnesiaCharacter::OnShopPressed);

		// the weapons: X changes the weapon in the hand
		PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &AAmnesiaCharacter::OnCycleWeaponPressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &AAmnesiaCharacter::OnCycleWeaponPressed);

		// the cats of the zones
		PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AAmnesiaCharacter::OnPetPressed);
		PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AAmnesiaCharacter::OnPetPressed);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AAmnesiaCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAmnesiaCharacter::DoSprintEnd);
	}
}

void AAmnesiaCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// reset the double jump and coyote time
	bHasDoubleJumped = false;
	bCanCoyoteJump = false;

	// ground attacks are available again as soon as we land
	bIsAirSlashing = false;
	GetWorld()->GetTimerManager().ClearTimer(AirSlashEndTimer);

	if (CurrentHP > 0.0f)
	{
		LandSounds.PlayAt(this, GetActorLocation());
	}

	FootstepCountdown = FootstepInterval * 0.5f;
}

void AAmnesiaCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// did we start falling without jumping? Then we walked off a ledge
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling && !bPressedJump)
	{
		// save the game time so we can check it later for coyote time jumps
		LastFallTime = GetWorld()->GetTimeSeconds();
		bCanCoyoteJump = true;
	}
}

void AAmnesiaCharacter::DoMove(float Right, float Forward)
{
	// only the horizontal axis matters while side scrolling
	AddMovementInput(FVector::ForwardVector, bGuarding ? Right * GuardSpeedMult : Right);

	// W and S move up and down while flying
	if (bFlying)
	{
		AddMovementInput(FVector::UpVector, Forward);
	}
}

void AAmnesiaCharacter::DoLook(float Yaw, float Pitch)
{
	// stub
}

void AAmnesiaCharacter::DoJumpStart()
{
	// ignore jumps while we're dead or flying
	if (CurrentHP <= 0.0f || bFlying)
	{
		return;
	}

	// if we're grounded, do a regular jump
	if (!GetCharacterMovement()->IsFalling())
	{
		JumpSounds.PlayAt(this, GetActorLocation());

		Jump();
		return;
	}

	// are we still within coyote time after walking off a ledge?
	if (bCanCoyoteJump && GetWorld()->GetTimeSeconds() - LastFallTime < MaxCoyoteTime)
	{
		bCanCoyoteJump = false;

		Jump();
		return;
	}

	// otherwise try a double jump
	if (!bHasDoubleJumped)
	{
		// raise the double jump flag and let the CMC handle the jump
		bHasDoubleJumped = true;
		bCanCoyoteJump = false;

		Jump();
	}
}

void AAmnesiaCharacter::DoJumpEnd()
{
	StopJumping();
}

void AAmnesiaCharacter::DoComboAttackStart()
{
	// slash downwards while we're in the air
	if (AirSlashAnimation && CurrentHP > 0.0f && GetCharacterMovement()->IsFalling())
	{
		if (!bIsAirSlashing)
		{
			AirSlash();
		}

		return;
	}

	// swing the sword with Arden's own animation
	if (GroundSlashAnimation && GroundSlashes.Num() > 0)
	{
		// a tap swings and a hold is the heavy attack: wait for the release or the hold time (Tick)
		EndGuard();
		bAttackPending = true;
		bAttackButtonDown = true;
		AttackPressTime = GetWorld()->GetTimeSeconds();
		return;
	}

	Super::DoComboAttackStart();
}

void AAmnesiaCharacter::DoSprintStart()
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed * Skills->GetStats().SpeedMult;
}

void AAmnesiaCharacter::DoSprintEnd()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * Skills->GetStats().SpeedMult;
}

int32 AAmnesiaCharacter::GetXPToNextLevel() const
{
	if (IsMaxLevel())
	{
		return 0;
	}

	// level 1 needs FirstLevelXP, every following level needs the multiplier times the previous one
	return FMath::RoundToInt(FirstLevelXP * FMath::Pow(LevelXPMultiplier, Level - 1));
}

void AAmnesiaCharacter::GainXP(int32 Amount)
{
	if (Amount <= 0 || IsMaxLevel())
	{
		return;
	}

	CurrentXP += Amount;

	bool bLeveledUp = false;

	while (!IsMaxLevel() && CurrentXP >= GetXPToNextLevel())
	{
		CurrentXP -= GetXPToNextLevel();
		++Level;
		bLeveledUp = true;
		LastLevelUpTime = GetWorld()->GetTimeSeconds();
	}

	if (bLeveledUp)
	{
		ApplyLevelStats();
		Skills->OnLevelChanged();

		LevelUpSounds.Play2D(this);
	}

	// nothing to carry over at the max level
	if (IsMaxLevel())
	{
		CurrentXP = 0;
	}

	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->StoreHero(Level, CurrentXP);
	}
}

void AAmnesiaCharacter::RecomputeStats()
{
	// stronger and tougher with every level, and with the skills
	const int32 LevelsGained = Level - 1;
	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();

	MaxHP = BaseMaxHP + HPPerLevel * LevelsGained + SkillStats.BonusMaxHP;
	MeleeDamage = BaseMeleeDamage * (1.0f + DamageBonusPerLevel * LevelsGained) * SkillStats.DamageMult;
	AirSlashDamage = BaseAirSlashDamage * (1.0f + DamageBonusPerLevel * LevelsGained) * SkillStats.DamageMult;
}

void AAmnesiaCharacter::ApplyLevelStats()
{
	RecomputeStats();

	// a full heal as a reward, but don't bring a dead hero back. A kill can still land after the hero died
	if (CurrentHP > 0.0f)
	{
		CurrentHP = MaxHP;
	}
}

void AAmnesiaCharacter::OnSkillsChanged()
{
	const float OldMaxHP = MaxHP;

	RecomputeStats();

	// more max HP arrives as health, less max HP cuts the health down
	if (CurrentHP > 0.0f)
	{
		CurrentHP = FMath::Clamp(CurrentHP + FMath::Max(MaxHP - OldMaxHP, 0.0f), 0.0f, MaxHP);
	}

	if (!bFlying && BaseWalkSpeed > 0.0f)
	{
		GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting ? SprintSpeed : BaseWalkSpeed) * Skills->GetStats().SpeedMult;
	}

	// the dragon's fire, color and shield follow the skills
	for (TActorIterator<AAmnesiaCompanion> It(GetWorld()); It; ++It)
	{
		It->ApplySkills(this);
	}
}

void AAmnesiaCharacter::Lvl(int32 NewLevel)
{
	const int32 OldLevel = Level;

	Level = FMath::Clamp(NewLevel, 1, MaxLevel);
	CurrentXP = 0;
	ApplyLevelStats();
	Skills->OnLevelChanged();

	if (Level > OldLevel)
	{
		LastLevelUpTime = GetWorld()->GetTimeSeconds();
	}

	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->StoreHero(Level, CurrentXP);
	}
}

void AAmnesiaCharacter::God(int32 Enabled)
{
	bInvulnerable = Enabled < 0 ? !bInvulnerable : Enabled != 0;

	// refill the life bar when turning it on, in case we were hurt already
	if (bInvulnerable && CurrentHP > 0.0f)
	{
		CurrentHP = MaxHP;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, bInvulnerable ? TEXT("God mode: ON") : TEXT("God mode: OFF"));
	}
}

void AAmnesiaCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	// no damage and no hit reaction while invulnerable, or right after the last hit
	const float Now = GetWorld()->GetTimeSeconds();

	if (bInvulnerable || CurrentHP <= 0.0f || Now < HurtImmuneUntil)
	{
		return;
	}

	// the dragon's shield takes the hit, and the armor softens what gets through
	if (Skills->TryCatchHit())
	{
		HurtImmuneUntil = Now + HurtInvulnerabilityTime;
		return;
	}

	Damage *= Skills->GetStats().DamageTakenMult;

	// the guard takes its share of what is left
	const bool bGuardedHit = bGuarding;

	if (bGuardedHit)
	{
		Damage *= GuardDamageMult;
	}

	HurtImmuneUntil = Now + HurtInvulnerabilityTime;

	const float Before = CurrentHP;

	Super::ApplyDamage(Damage, DamageCauser, DamageLocation, DamageImpulse);

	if (CurrentHP < Before)
	{
		AddCameraShake(HurtShakeStrength);

		if (CurrentHP > 0.0f)
		{
			HurtSounds.PlayAt(this, GetActorLocation());
		}

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
			{
				HUD->AddNumberPopup(GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Before - FMath::Max(CurrentHP, 0.0f), bGuardedHit ? FLinearColor(0.5f, 0.78f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.2f), TEXT("-"));
			}
		}
	}
}

float AAmnesiaCharacter::GetTimeSinceLevelUp() const
{
	return GetWorld()->GetTimeSeconds() - LastLevelUpTime;
}

void AAmnesiaCharacter::AirSlash()
{
	bIsAirSlashing = true;
	bAirSlashBounced = false;
	bHasLastBlade = false;
	AirSlashHitActors.Reset();

	const float Rate = FMath::Max(AirSlashPlayRate, 0.1f);

	if (bDebugLogBlade)
	{
		UE_LOG(LogTemp, Display, TEXT("Air slash starts t=%.3f, animation from %.2f, rate %.2f"), GetWorld()->GetTimeSeconds(), AirSlashStartTime, Rate);
	}

	// one cut, from the start of the swing to its end: play just that part of the animation on top of whatever the
	// Anim Blueprint is doing. The hero's own anim instance keeps the legs in the jump and lets them walk on after landing
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (UAmnesiaHeroAnimInstance* HeroAnim = Cast<UAmnesiaHeroAnimInstance>(AnimInstance))
		{
			HeroAnim->PlayOverlay(AirSlashAnimation, AirSlashStartTime, AirSlashLength, Rate);
			AirSlashMontage = nullptr;

		} else {

			AirSlashMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(AirSlashAnimation, AirSlashSlotName, 0.1f, 0.2f, Rate, 1, -1.0f, AirSlashStartTime);
		}
	}

	// stop falling for a moment so the slash has some hang time
	FVector Velocity = GetCharacterMovement()->Velocity;
	Velocity.Z = FMath::Max(Velocity.Z, 0.0f);
	GetCharacterMovement()->Velocity = Velocity;

	// real time the blade starts and stops hurting, and the swing ends
	const float HitStart = FMath::Max(AirSlashHitWindow.X, 0.0f) / Rate;
	const float HitEnd = FMath::Max(AirSlashHitWindow.Y / Rate, HitStart + 0.02f);
	const float Length = FMath::Max(AirSlashLength / Rate, HitEnd + 0.02f);

	// the whoosh goes with the blade, just before it comes down
	const float SwingDelay = HitStart - 0.06f;

	if (SwingDelay > 0.01f)
	{
		GetWorld()->GetTimerManager().SetTimer(SwingSoundTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SwingSounds.PlayAt(this, GetActorLocation(), 1.1f);

		}), SwingDelay, false);

	} else {

		SwingSounds.PlayAt(this, GetActorLocation(), 1.1f);
	}

	GetWorld()->GetTimerManager().SetTimer(AirSlashHitTimer, this, &AAmnesiaCharacter::AirSlashHitBegin, FMath::Max(HitStart, 0.01f), false);
	GetWorld()->GetTimerManager().SetTimer(AirSlashHitEndTimer, this, &AAmnesiaCharacter::AirSlashHitStop, HitEnd, false);
	GetWorld()->GetTimerManager().SetTimer(AirSlashEndTimer, this, &AAmnesiaCharacter::AirSlashEnded, Length, false);
}

void AAmnesiaCharacter::AirSlashHitBegin()
{
	bAirSlashHitActive = true;
	bHasLastBlade = false;

	if (bDebugLogBlade)
	{
		UE_LOG(LogTemp, Display, TEXT("Air slash: the blade starts to hurt t=%.3f"), GetWorld()->GetTimeSeconds());
	}

	AirSlashTrace();
}

void AAmnesiaCharacter::AirSlashHitStop()
{
	bAirSlashHitActive = false;
	bHasLastBlade = false;

	if (bDebugLogBlade)
	{
		UE_LOG(LogTemp, Display, TEXT("Air slash: the blade stops hurting t=%.3f"), GetWorld()->GetTimeSeconds());
	}
}

void AAmnesiaCharacter::AirSlashTrace()
{
	if (!Weapon)
	{
		return;
	}

	// the blade right now, from the hilt to the tip
	const FTransform WeaponTransform = Weapon->GetComponentTransform();
	const FVector Base = WeaponTransform.TransformPosition(BladeBaseLocal);
	const FVector Tip = WeaponTransform.TransformPosition(BladeTipLocal);

	// and where it was at the last check: the sweeps run between the two, so a fast swing can't jump over a target.
	// The first check has no earlier place, it only looks where the blade is
	const FVector OldBase = bHasLastBlade ? LastBladeBase : Base;
	const FVector OldTip = bHasLastBlade ? LastBladeTip : Tip;

	LastBladeBase = Base;
	LastBladeTip = Tip;
	bHasLastBlade = true;

	const float Facing = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(AirSlashBladeRadius);

	AActor* FirstTarget = nullptr;

	// a few spheres along the blade, and each one a few places between where it was and where it is, so close together
	// that nothing slips through. Overlap checks, not sweeps: a sweep stops at the first thing it meets, and that could be
	// something else than the enemy
	const int32 Samples = 5;

	for (int32 Sample = 0; Sample < Samples; ++Sample)
	{
		const float Alpha = static_cast<float>(Sample) / (Samples - 1);
		const FVector From = FMath::Lerp(OldBase, OldTip, Alpha);
		const FVector To = FMath::Lerp(Base, Tip, Alpha);
		const int32 Steps = FMath::Clamp(FMath::CeilToInt(FVector::Dist(From, To) / FMath::Max(AirSlashBladeRadius, 1.0f)), 1, 20);

		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const FVector Place = FMath::Lerp(From, To, static_cast<float>(Step) / Steps);

			TArray<FOverlapResult> Overlaps;
			GetWorld()->OverlapMultiByObjectType(Overlaps, Place, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);

			for (const FOverlapResult& Overlap : Overlaps)
			{
				AActor* HitActor = Overlap.GetActor();

				if (!HitActor || AirSlashHitActors.Contains(TWeakObjectPtr<AActor>(HitActor)))
				{
					continue;
				}

				const float InFront = (HitActor->GetActorLocation().X - GetActorLocation().X) * Facing;
				ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor);

				if (bDebugLogBlade)
				{
					UE_LOG(LogTemp, Display, TEXT("Air slash: sample %d touches %s, %.0f cm in front of the hero, damageable %d"), Sample, *HitActor->GetName(), InFront, Damageable != nullptr);
				}

				// nothing that stands behind the hero
				if (InFront < -40.0f)
				{
					continue;
				}

				if (Damageable)
				{
					AirSlashHitActors.Add(TWeakObjectPtr<AActor>(HitActor));

					// knock the target away from the hero
					const FVector Impulse(Facing * AirSlashKnockback, 0.0f, AirSlashKnockback * 0.5f);

					Damageable->ApplyDamage(AirSlashDamage, this, Place, Impulse);

					// call the BP handler to play effects, etc.
					DealtDamage(AirSlashDamage, Place);

					// life steal, burning and chilling
					if (AAmnesiaEnemy* HitEnemy = Cast<AAmnesiaEnemy>(HitActor))
					{
						Skills->OnHeroHit(HitEnemy, AirSlashDamage, false);
					}

					if (!FirstTarget)
					{
						FirstTarget = HitActor;
					}
				}
			}
		}
	}

	if (FirstTarget)
	{
		OnAttackLanded(FirstTarget, FirstTarget->GetActorLocation(), AirSlashDamage);

		// bounce off the first thing the slash hits, and allow another double jump
		if (!bAirSlashBounced)
		{
			bAirSlashBounced = true;
			LaunchCharacter(FVector(0.0f, 0.0f, AirSlashBounceSpeed), false, true);
			bHasDoubleJumped = false;
		}
	}
}

void AAmnesiaCharacter::AirSlashEnded()
{
	bIsAirSlashing = false;
	bAirSlashHitActive = false;

	// cut the animation off where the swing ends
	if (AirSlashMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.15f, AirSlashMontage);
		}

		AirSlashMontage = nullptr;
	}
}

void AAmnesiaCharacter::FindBlade()
{
	// the blade runs along the longest side of the sword mesh, and the end nearer the weapon's origin is the hilt
	USkeletalMesh* SwordMesh = Weapon ? Weapon->GetSkeletalMeshAsset() : nullptr;

	if (!SwordMesh)
	{
		return;
	}

	const FBoxSphereBounds Bounds = SwordMesh->GetBounds();

	int32 Axis = 0;

	for (int32 Candidate = 1; Candidate < 3; ++Candidate)
	{
		if (Bounds.BoxExtent[Candidate] > Bounds.BoxExtent[Axis])
		{
			Axis = Candidate;
		}
	}

	FVector EndA = Bounds.Origin;
	FVector EndB = Bounds.Origin;
	EndA[Axis] -= Bounds.BoxExtent[Axis];
	EndB[Axis] += Bounds.BoxExtent[Axis];

	const bool bAIsHilt = EndA.SizeSquared() < EndB.SizeSquared();

	BladeBaseLocal = bAIsHilt ? EndA : EndB;
	BladeTipLocal = bAIsHilt ? EndB : EndA;
}

void AAmnesiaCharacter::SetCameraPitch(float NewPitch, float BlendSpeed)
{
	TargetCameraPitch = NewPitch;
	CameraPitchBlendSpeed = BlendSpeed;
}

void AAmnesiaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// blend the camera pitch
	if (!FMath::IsNearlyEqual(CurrentCameraPitch, TargetCameraPitch, 0.01f))
	{
		CurrentCameraPitch = FMath::FInterpTo(CurrentCameraPitch, TargetCameraPitch, DeltaTime, CameraPitchBlendSpeed);
		GetCameraBoom()->SetRelativeRotation(FRotator(CurrentCameraPitch, -90.0f, 0.0f));
	}

	// camera shake: jitter the end of the boom and let it settle
	if (ShakeStrength > 0.05f)
	{
		const float Time = GetWorld()->GetRealTimeSeconds() * 38.0f;
		GetCameraBoom()->SocketOffset = FVector(0.0f, FMath::Sin(Time * 1.3f) * ShakeStrength, FMath::Sin(Time + 1.7f) * ShakeStrength * 0.8f);
		ShakeStrength = FMath::FInterpTo(ShakeStrength, 0.0f, DeltaTime / FMath::Max(CustomTimeDilation, 0.05f), 9.0f);

	} else if (ShakeStrength > 0.0f) {

		ShakeStrength = 0.0f;
		GetCameraBoom()->SocketOffset = FVector::ZeroVector;
	}

	// footsteps in step with the run, on the ground of the current zone
	const FAmnesiaSoundSet& Steps = ZoneFootstepSounds.IsSet() ? ZoneFootstepSounds : FootstepSounds;

	if (Steps.IsSet() && CurrentHP > 0.0f && GetCharacterMovement()->IsMovingOnGround())
	{
		const float Speed = GetVelocity().Size2D();

		if (Speed > 60.0f)
		{
			FootstepCountdown -= DeltaTime * (Speed / FMath::Max(BaseWalkSpeed, 1.0f));

			if (FootstepCountdown <= 0.0f)
			{
				FootstepCountdown = FootstepInterval;
				Steps.PlayAt(this, GetActorLocation() - FVector(0, 0, 80), FMath::FRandRange(0.85f, 1.0f));
			}
		}
	}

	// the air slash checks its blade every frame while the swing can hurt
	if (bAirSlashHitActive)
	{
		AirSlashTrace();
	}

	// test helper: where the blade goes during a swing
	if (bDebugLogBlade && Weapon)
	{
		const float BladeFacing = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;
		const FTransform WeaponTransform = Weapon->GetComponentTransform();
		const FVector BladeBase = WeaponTransform.TransformPosition(BladeBaseLocal) - GetActorLocation();
		const FVector BladeTip = WeaponTransform.TransformPosition(BladeTipLocal) - GetActorLocation();

		UE_LOG(LogTemp, Display, TEXT("Blade t=%.3f base=%.0f,%.0f tip=%.0f,%.0f"), GetWorld()->GetTimeSeconds(), BladeBase.X * BladeFacing, BladeBase.Z, BladeTip.X * BladeFacing, BladeTip.Z);
	}

	// test helper: an air slash even on the ground
	if (bDebugAirSlash)
	{
		bDebugAirSlash = false;

		if (AirSlashAnimation && !bIsAirSlashing)
		{
			AirSlash();
		}
	}

	// test helper: put a weapon in the hand
	if (!DebugWeapon.IsNone())
	{
		const FName Wanted = DebugWeapon;
		DebugWeapon = NAME_None;
		EquipWeapon(Wanted);
	}

	// test helpers: the attack buttons
	if (bDebugGroundSlash)
	{
		bDebugGroundSlash = false;
		DoComboAttackStart();
		OnAttackReleased();
	}

	if (bDebugHeavySlash)
	{
		bDebugHeavySlash = false;
		HeavyAttack();
	}

	if (bDebugHoldAttack)
	{
		bDebugHoldAttack = false;
		DoComboAttackStart();
	}

	if (bDebugGuard)
	{
		bDebugGuard = false;

		// acts like the right button: pressed when it is up, let go when it is down
		if (bGuardHeld)
		{
			DoChargedAttackEnd();

		} else {

			DoChargedAttackStart();
		}
	}

	// the left button: let go it is a tap that swings the combo, held long enough it is the heavy attack
	if (bAttackPending)
	{
		if (CurrentHP <= 0.0f)
		{
			bAttackPending = false;

		} else if (!bAttackButtonDown) {

			bAttackPending = false;
			GroundSlash(false);

		} else if (GetWorld()->GetTimeSeconds() - AttackPressTime >= HeavyHoldTime) {

			bAttackPending = false;
			HeavyAttack();
		}
	}

	// while the right button is held the guard comes back as soon as it can: after a jump, a hit or a swing
	if (bGuardHeld && !bGuarding)
	{
		StartGuard();
	}

	// a guard cannot stand in the air, and it keeps facing the enemy
	if (bGuarding)
	{
		if (GetCharacterMovement()->IsFalling())
		{
			EndGuard();

		} else {

			UpdateGuardFacing(false);
		}
	}

	// test helper for trying out parts of the sword animation
	if (bDebugSlash)
	{
		bDebugSlash = false;

		FAmnesiaSlash DebugSlash;
		DebugSlash.StartTime = DebugSlashStartDurationRate.X;
		DebugSlash.Duration = DebugSlashStartDurationRate.Y;
		DebugSlash.PlayRate = DebugSlashStartDurationRate.Z;
		DebugSlash.HitTime = DebugSlash.Duration * 0.5f;

		PlaySlash(DebugSlash, 1.0f, 1.0f);
	}
}

void AAmnesiaCharacter::DoChargedAttackStart()
{
	bGuardHeld = true;
	StartGuard();
}

void AAmnesiaCharacter::DoChargedAttackEnd()
{
	bGuardHeld = false;
	EndGuard();
}

void AAmnesiaCharacter::HeavyAttack()
{
	if (GroundSlashAnimation && GroundSlashes.Num() > 0 && !GetCharacterMovement()->IsFalling())
	{
		GroundSlash(true);
		return;
	}

	// the heavy attack in the air is the air slash
	if (AirSlashAnimation && CurrentHP > 0.0f && GetCharacterMovement()->IsFalling() && !bIsAirSlashing)
	{
		AirSlash();
	}
}

void AAmnesiaCharacter::OnAttackReleased()
{
	bAttackButtonDown = false;
}

void AAmnesiaCharacter::StartGuard()
{
	if (bGuarding || CurrentHP <= 0.0f || GetCharacterMovement()->IsFalling() || bGroundSlashing || bIsAirSlashing)
	{
		return;
	}

	bGuarding = true;

	// the hero turns to the nearest enemy and stays turned, whichever way the walking goes
	bTurnedWithMovement = GetCharacterMovement()->bOrientRotationToMovement;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	UpdateGuardFacing(true);

	// the pose is held: the overlay plays the chosen second of the animation at a crawl, and fades when the guard ends
	if (!GuardAnimation || GuardAnimation->GetPathName() != GuardAnimationPath)
	{
		GuardAnimation = LoadObject<UAnimSequenceBase>(nullptr, *GuardAnimationPath);
	}

	if (UAmnesiaHeroAnimInstance* HeroAnim = Cast<UAmnesiaHeroAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		if (GuardAnimation)
		{
			// upper body only: the legs keep standing or walking with the graph, so the hips stay turned to the enemy
			HeroAnim->PlayOverlay(GuardAnimation, GuardPoseTime, 1000.0f, 0.05f, true);
		}
	}
}

void AAmnesiaCharacter::UpdateGuardFacing(bool bForce)
{
	const float Now = GetWorld()->GetTimeSeconds();

	// the nearest living enemy along the walking line, looked for a few times a second
	if (bForce || Now >= NextGuardTargetTime)
	{
		NextGuardTargetTime = Now + 0.2f;
		GuardTarget = nullptr;

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(GuardFacingRange), QueryParams);

		float Nearest = TNumericLimits<float>::Max();

		for (const FOverlapResult& Overlap : Overlaps)
		{
			AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Overlap.GetActor());

			if (Enemy && Enemy->GetHealth() > 0.0f)
			{
				const float Distance = FMath::Abs(Enemy->GetActorLocation().X - GetActorLocation().X);

				if (Distance < Nearest)
				{
					Nearest = Distance;
					GuardTarget = Enemy;
				}
			}
		}
	}

	if (const AAmnesiaEnemy* Target = GuardTarget.Get())
	{
		SetActorRotation(FRotator(0.0f, Target->GetActorLocation().X >= GetActorLocation().X ? 0.0f : 180.0f, 0.0f));
	}
}

void AAmnesiaCharacter::EndGuard()
{
	if (!bGuarding)
	{
		return;
	}

	bGuarding = false;
	GuardTarget = nullptr;
	GetCharacterMovement()->bOrientRotationToMovement = bTurnedWithMovement;

	if (UAmnesiaHeroAnimInstance* HeroAnim = Cast<UAmnesiaHeroAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		HeroAnim->StopOverlay();
	}
}

void AAmnesiaCharacter::GroundSlash(bool bHeavy)
{
	if (CurrentHP <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	// busy until the end of the current swing, apart from a short chain window
	if (bGroundSlashing && Now < SlashEndTime - SlashChainWindow)
	{
		return;
	}

	// what the heavy attack does, if the weapon has learned it
	CurrentSpecial = bHeavy ? GetUnlockedSpecial() : EAmnesiaWeaponSpecial::None;
	const float WeaponRate = GetWeaponRateMult();

	// a weapon with a heavy swing of its own. The wand casts its bolt on every click: a tap is a quicker, lighter bolt,
	// the held button the full one with the special
	const FAmnesiaWeaponDef* SlashWeaponDef = GetEquippedWeaponDef();
	const bool bWeaponShootsBolt = SlashWeaponDef && SlashWeaponDef->bShootsBolt;
	const bool bWeaponThrowsDisc = SlashWeaponDef && SlashWeaponDef->bThrowsDisc;

	// every disc is out: no attack until one comes back to the hand
	if (bWeaponThrowsDisc && CountThrownChakrams() >= GetMaxChakrams())
	{
		return;
	}

	if (bHasWeaponHeavy && (bHeavy || bWeaponShootsBolt || bWeaponThrowsDisc))
	{
		bBoltSwing = bWeaponShootsBolt;
		bDiscSwing = bWeaponThrowsDisc;
		bDiscHeavy = bHeavy;
		PlaySlash(WeaponHeavySlash, WeaponRate * (bHeavy ? 1.0f : BoltTapRateScale), bHeavy ? 1.0f : BoltTapDamageScale);
		return;
	}

	bBoltSwing = false;
	bDiscSwing = false;

	// continue the combo, or start it over after a pause
	SlashIndex = Now - SlashEndTime > SlashComboResetTime ? 0 : (SlashIndex + 1) % GroundSlashes.Num();

	// the heavy swing is slower and hits harder
	PlaySlash(GroundSlashes[SlashIndex], (bHeavy ? 0.75f : 1.0f) * WeaponRate, bHeavy ? 2.0f : 1.0f);
}

void AAmnesiaCharacter::PlaySlash(const FAmnesiaSlash& Slash, float RateScale, float DamageScale)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	// a swing can have an animation of its own: the casts of the wand, the throw of the chakram
	UAnimSequenceBase* Animation = Slash.Animation ? Slash.Animation.Get() : GroundSlashAnimation;

	if (!AnimInstance || !Animation)
	{
		return;
	}

	CurrentSlash = Slash;
	CurrentSlashDamageScale = DamageScale;

	const float Rate = FMath::Max(Slash.PlayRate * RateScale, 0.1f);

	if (bDebugLogBlade)
	{
		UE_LOG(LogTemp, Display, TEXT("Swing starts t=%.3f, animation from %.2f, rate %.2f"), GetWorld()->GetTimeSeconds(), Slash.StartTime, Rate);
	}

	const float HitDelay = FMath::Max(Slash.HitTime / Rate, 0.01f);
	const float Length = FMath::Max(Slash.Duration / Rate, HitDelay + 0.02f);

	// play just the wanted part of the animation. The hero's own anim instance lays it over the legs' run,
	// so swinging while running keeps the legs running; a plain one plays it on the whole body
	if (UAmnesiaHeroAnimInstance* HeroAnim = Cast<UAmnesiaHeroAnimInstance>(AnimInstance))
	{
		HeroAnim->PlayOverlay(Animation, Slash.StartTime, Slash.Duration, Rate, Slash.bUpperBodyOnly);
		SlashMontage = nullptr;

	} else {

		SlashMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(Animation, GroundSlashSlotName, 0.06f, 0.15f, Rate, 1, -1.0f, Slash.StartTime);
	}

	bGroundSlashing = true;
	SlashEndTime = GetWorld()->GetTimeSeconds() + Length;

	// a thrust steps into the enemy: a short push forward that the ground friction soon stops
	if (Slash.Step > 0.0f && GetCharacterMovement()->IsMovingOnGround())
	{
		const float StepDir = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;
		LaunchCharacter(FVector(StepDir * Slash.Step, 0.0f, 0.0f), true, false);
	}

	// the whoosh goes with the blade, just before it lands, not with the wind up
	const float SwingDelay = HitDelay - 0.12f;
	const float SwingVolume = DamageScale > 1.5f ? 1.15f : 1.0f;

	if (SwingDelay > 0.01f)
	{
		GetWorld()->GetTimerManager().SetTimer(SwingSoundTimer, FTimerDelegate::CreateWeakLambda(this, [this, SwingVolume]()
		{
			SwingSounds.PlayAt(this, GetActorLocation(), SwingVolume);

		}), SwingDelay, false);

	} else {

		SwingSounds.PlayAt(this, GetActorLocation(), SwingVolume);
	}

	GetWorld()->GetTimerManager().SetTimer(SlashHitTimer, this, &AAmnesiaCharacter::SlashHit, HitDelay, false);
	GetWorld()->GetTimerManager().SetTimer(SlashEndTimer, this, &AAmnesiaCharacter::SlashEnded, Length, false);
}

void AAmnesiaCharacter::ShootBolt(float Facing)
{
	const FVector Forward(Facing, 0.0f, 0.0f);
	const FVector Hand = GetActorLocation() + Forward * 80.0f + FVector(0.0f, 0.0f, 60.0f);
	const float Damage = MeleeDamage * CurrentSlash.DamageScale * CurrentSlashDamageScale * GetWeaponDamageMult();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// flies straight ahead, the hero gets the kill
	if (AAmnesiaFireball* Bolt = GetWorld()->SpawnActor<AAmnesiaFireball>(AAmnesiaFireball::StaticClass(), Hand, Forward.Rotation(), Params))
	{
		Bolt->Launch(nullptr, this, Damage);
	}
}

int32 AAmnesiaCharacter::CountThrownChakrams()
{
	ThrownChakrams.RemoveAll([](const TWeakObjectPtr<AAmnesiaChakram>& Disc) { return !Disc.IsValid(); });
	return ThrownChakrams.Num();
}

int32 AAmnesiaCharacter::GetMaxChakrams() const
{
	return 1 + Skills->GetStats().ChakramExtraDiscs;
}

void AAmnesiaCharacter::ThrowChakram(float Facing)
{
	if (CountThrownChakrams() >= GetMaxChakrams() || !WeaponProp)
	{
		return;
	}

	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();
	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();

	FAmnesiaChakramThrow Throw;
	Throw.Damage = MeleeDamage * CurrentSlash.DamageScale * CurrentSlashDamageScale * GetWeaponDamageMult();
	Throw.Range = bDiscHeavy ? ChakramHeavyRange : ChakramTapRange;
	Throw.Speed = ChakramSpeed * GetWeaponRateMult();
	Throw.Bounces = SkillStats.ChakramBounces;

	// the held throw whirls at the end of its flight, once its skill is learned
	if (CurrentSpecial == EAmnesiaWeaponSpecial::Whirl)
	{
		Throw.WhirlTime = ChakramWhirlTime;
		Throw.WhirlDamage = MeleeDamage * SkillStats.ChakramWhirlScale * GetWeaponDamageMult();
	}

	// it leaves the hand at chest height and flies level, so it meets small and tall enemies alike
	const FVector Hand = WeaponProp->GetComponentLocation();
	const FVector Start(Hand.X + Facing * 30.0f, GetActorLocation().Y, GetActorLocation().Z + 35.0f);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AAmnesiaChakram* Disc = GetWorld()->SpawnActor<AAmnesiaChakram>(AAmnesiaChakram::StaticClass(), Start, FRotator::ZeroRotator, Params))
	{
		Disc->Throw(this, WeaponProp->GetStaticMesh(), Start, Facing, Throw, Def ? Def->Color : FLinearColor::White);
		ThrownChakrams.Add(Disc);

		// the last disc left the hand: it is empty until one comes back
		if (ThrownChakrams.Num() >= GetMaxChakrams())
		{
			WeaponProp->SetVisibility(false);
			bAuraHiddenForThrow = WeaponAura->IsVisible();
			WeaponAura->SetVisibility(false);
		}
	}
}

void AAmnesiaCharacter::OnChakramHit(AAmnesiaEnemy* Enemy, float Damage, const FVector& Point, float Direction)
{
	if (!Enemy)
	{
		return;
	}

	Enemy->ApplyDamage(Damage, this, Point, FVector(Direction * 140.0f, 0.0f, 50.0f));
	DealtDamage(Damage, Point);

	// life steal, burning and chilling, as with a blow in hand
	Skills->OnHeroHit(Enemy, Damage, false);
	OnWeaponHit(Enemy, Damage);
}

void AAmnesiaCharacter::OnChakramCaught(AAmnesiaChakram* Disc)
{
	ThrownChakrams.Remove(Disc);
	CountThrownChakrams();

	// back in the hand, if the disc is still the weapon there
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (Def && Def->bThrowsDisc && WeaponProp)
	{
		WeaponProp->SetVisibility(true);

		if (bAuraHiddenForThrow)
		{
			WeaponAura->SetVisibility(true);
		}
	}

	bAuraHiddenForThrow = false;
}

bool AAmnesiaCharacter::SlashTouchesBody(const FHitResult& Hit, const FVector& Start, const FVector& End) const
{
	// a character's body is its capsule: how far the weapon is from the line through the capsule, less the radius
	if (const ACharacter* Character = Cast<ACharacter>(Hit.GetActor()))
	{
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const FVector Center = Capsule->GetComponentLocation();
		const float Radius = Capsule->GetScaledCapsuleRadius();
		const FVector Half(0.0f, 0.0f, FMath::Max(Capsule->GetScaledCapsuleHalfHeight() - Radius, 0.0f));

		FVector OnWeapon;
		FVector OnBody;
		FMath::SegmentDistToSegmentSafe(Start, End, Center + Half, Center - Half, OnWeapon, OnBody);

		return FVector::Dist(OnWeapon, OnBody) - Radius <= SlashBodyTolerance;
	}

	const UPrimitiveComponent* Body = Hit.GetComponent();

	if (!Body)
	{
		return true;
	}

	// anything else: the nearest the weapon comes to it, along the line from the hand to the tip
	constexpr int32 Steps = 6;
	float Nearest = TNumericLimits<float>::Max();

	for (int32 Step = 0; Step <= Steps; ++Step)
	{
		FVector Closest;
		const float Distance = Body->GetClosestPointOnCollision(FMath::Lerp(Start, End, static_cast<float>(Step) / Steps), Closest);

		// a body that cannot tell how far it is counts as touched
		if (Distance < 0.0f)
		{
			return true;
		}

		Nearest = FMath::Min(Nearest, Distance);
	}

	return Nearest <= SlashBodyTolerance;
}

void AAmnesiaCharacter::SlashHit()
{
	const float Facing = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;
	const FAmnesiaWeaponDef* WeaponDef = GetEquippedWeaponDef();
	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();

	// the wand's heavy cast throws a bolt forward instead of hitting in hand
	if (bBoltSwing)
	{
		bBoltSwing = false;
		ShootBolt(Facing);

		if (CurrentSpecial != EAmnesiaWeaponSpecial::None)
		{
			PerformSpecial(CurrentSpecial, Facing);
			CurrentSpecial = EAmnesiaWeaponSpecial::None;
		}

		return;
	}

	// the chakram flies off the hand instead of hitting in hand
	if (bDiscSwing)
	{
		bDiscSwing = false;
		ThrowChakram(Facing);
		CurrentSpecial = EAmnesiaWeaponSpecial::None;
		return;
	}

	// the weapon in the hand changes how far the swing reaches and how fat it is
	const float Reach = GetWeaponReachMult();
	const float HitRadius = CurrentSlash.HitRadius * GetWeaponRadiusMult();

	const FVector TraceStart = GetActorLocation() + FVector(Facing * 30.0f, 0.0f, 0.0f);
	const FVector TraceEnd = TraceStart + FVector(CurrentSlash.HitOffset.X * Reach * Facing, 0.0f, CurrentSlash.HitOffset.Y);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(HitRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> OutHits;
	TSet<AActor*> DamagedActors;

	GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);

	const float Damage = MeleeDamage * CurrentSlash.DamageScale * CurrentSlashDamageScale * GetWeaponDamageMult();

	for (const FHitResult& CurrentHit : OutHits)
	{
		AActor* HitActor = CurrentHit.GetActor();

		if (!HitActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		// the sweep finds what is near the swing, only what the weapon really reaches is hurt
		if (!SlashTouchesBody(CurrentHit, TraceStart, TraceEnd))
		{
			continue;
		}

		if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
		{
			DamagedActors.Add(HitActor);

			const float HitDamage = Damage;

			Damageable->ApplyDamage(HitDamage, this, CurrentHit.ImpactPoint, FVector(Facing * SlashKnockback, 0.0f, SlashKnockback * 0.4f));
			DealtDamage(HitDamage, CurrentHit.ImpactPoint);

			// life steal, burning and chilling, and what the weapon adds
			if (AAmnesiaEnemy* HitEnemy = Cast<AAmnesiaEnemy>(HitActor))
			{
				Skills->OnHeroHit(HitEnemy, HitDamage, false);
				OnWeaponHit(HitEnemy, HitDamage);
			}
		}
	}

	// one hit stop for the whole swing, on the first target
	for (AActor* Target : DamagedActors)
	{
		OnAttackLanded(Target, Target->GetActorLocation(), Damage);
		break;
	}

	// the heavy attack of the weapon, once its last skill is learned
	if (CurrentSpecial != EAmnesiaWeaponSpecial::None)
	{
		PerformSpecial(CurrentSpecial, Facing);
		CurrentSpecial = EAmnesiaWeaponSpecial::None;
	}
}

void AAmnesiaCharacter::SlashEnded()
{
	bGroundSlashing = false;
	SlashEndTime = GetWorld()->GetTimeSeconds();

	// cut the animation off where the swing ends
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (SlashMontage)
		{
			AnimInstance->Montage_Stop(0.15f, SlashMontage);
		}
	}
}

// ---------------------------------------------------------------- the weapons

const FAmnesiaWeaponDef* AAmnesiaCharacter::GetEquippedWeaponDef() const
{
	if (EquippedWeapon.IsNone() || EquippedWeapon == FAmnesiaWeapons::SwordId())
	{
		return nullptr;
	}

	return FAmnesiaWeapons::Find(EquippedWeapon);
}

UStaticMesh* AAmnesiaCharacter::GetWeaponPropMesh() const
{
	return GetEquippedWeaponDef() && WeaponProp ? WeaponProp->GetStaticMesh() : nullptr;
}

float AAmnesiaCharacter::GetWeaponPropScale() const
{
	return WeaponProp ? WeaponProp->GetComponentScale().GetAbsMax() : 1.0f;
}

bool AAmnesiaCharacter::EquipWeapon(FName Id)
{
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);
	const FAmnesiaWeaponDef* Def = FAmnesiaWeapons::Find(Id);

	if (!ProgressSystem || !Def || (Id != FAmnesiaWeapons::SwordId() && !ProgressSystem->Owns(Id)))
	{
		return false;
	}

	EquippedWeapon = Id == FAmnesiaWeapons::SwordId() ? NAME_None : Id;
	ProgressSystem->GetProgress()->EquippedWeapon = EquippedWeapon;
	ProgressSystem->SaveGame();

	ApplyWeapon(true);
	return true;
}

void AAmnesiaCharacter::CycleWeapon()
{
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	if (!ProgressSystem || CurrentHP <= 0.0f)
	{
		return;
	}

	TArray<FName> Owned;

	for (const FAmnesiaWeaponDef& Def : FAmnesiaWeapons::All())
	{
		if (Def.Id == FAmnesiaWeapons::SwordId() || ProgressSystem->Owns(Def.Id))
		{
			Owned.Add(Def.Id);
		}
	}

	if (Owned.Num() < 2)
	{
		return;
	}

	const int32 Current = Owned.IndexOfByKey(GetEquippedWeaponId());
	EquipWeapon(Owned[(Current + 1) % Owned.Num()]);
}

void AAmnesiaCharacter::ReloadWeaponFromProgress()
{
	FName Wanted;

	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		Wanted = ProgressSystem->GetProgress()->EquippedWeapon;

		// a weapon that is not owned (a new game, a weapon that was taken out of the game) is not in the hand
		if (!Wanted.IsNone() && (!ProgressSystem->Owns(Wanted) || !FAmnesiaWeapons::Find(Wanted)))
		{
			Wanted = NAME_None;
		}
	}

	EquippedWeapon = Wanted;
	ApplyWeapon(false);
}

void AAmnesiaCharacter::ApplyWeapon(bool bAnnounce)
{
	// in the editor the numbers can be changed in a file without a build
	FAmnesiaWeapons::ReloadTuning();

	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	// the swings of the sword, or of the weapon if it has its own
	GroundSlashAnimation = SwordGroundAnimation;
	GroundSlashes = SwordGroundSlashes;
	bHasWeaponHeavy = false;

	auto MakeSlash = [this](const FAmnesiaWeaponSlash& Source) -> FAmnesiaSlash
	{
		FAmnesiaSlash Result;
		Result.StartTime = Source.StartTime;
		Result.HitTime = Source.HitTime;
		Result.Duration = Source.Duration;
		Result.PlayRate = Source.PlayRate;
		Result.DamageScale = Source.DamageScale;
		Result.HitOffset = Source.HitOffset;
		Result.HitRadius = Source.HitRadius;
		Result.Step = Source.Step;
		Result.bUpperBodyOnly = Source.bUpperBodyOnly;

		if (!Source.Animation.IsEmpty())
		{
			if (UAnimSequenceBase* Loaded = LoadObject<UAnimSequenceBase>(nullptr, *Source.Animation))
			{
				LoadedWeaponAssets.AddUnique(Loaded);
				Result.Animation = Loaded;
			}
		}

		return Result;
	};

	// the combo of the weapon in the hand; the sword's entry in the table can give it mocap swings in place of the Blueprint's
	const FAmnesiaWeaponDef* ComboDef = Def ? Def : FAmnesiaWeapons::Find(FAmnesiaWeapons::SwordId());

	if (ComboDef && ComboDef->Combo.Num() > 0)
	{
		GroundSlashes.Reset();

		for (const FAmnesiaWeaponSlash& Source : ComboDef->Combo)
		{
			GroundSlashes.Add(MakeSlash(Source));
		}
	}

	if (!Def)
	{
		WeaponProp->SetVisibility(false);
		Weapon->SetVisibility(true);

		WeaponAura->AttachToComponent(Weapon, FAttachmentTransformRules::KeepRelativeTransform);
		WeaponAura->SetRelativeTransform(SwordAuraRelative);

	} else {

		UStaticMesh* PropMesh = LoadObject<UStaticMesh>(nullptr, *Def->MeshPath);

		WeaponProp->SetStaticMesh(PropMesh);
		WeaponProp->SetRelativeLocationAndRotation(Def->HandLocation, Def->HandRotation);
		WeaponProp->SetRelativeScale3D(Def->HandScale);
		WeaponProp->SetVisibility(PropMesh != nullptr);

		// the sword is put away: it stays in the hand, unseen, because the air slash reads its blade
		Weapon->SetVisibility(false);

		WeaponAura->AttachToComponent(WeaponProp, FAttachmentTransformRules::KeepRelativeTransform);
		WeaponAura->SetRelativeLocationAndRotation(Def->AuraLocation, Def->AuraRotation);
		WeaponAura->SetRelativeScale3D(Def->AuraScale);

		if (Def->bHasHeavy)
		{
			WeaponHeavySlash = MakeSlash(Def->Heavy);
			bHasWeaponHeavy = true;
		}
	}

	// the aura and its shape follow the weapon
	Skills->ApplyAura();

	// the next combo starts from the first swing
	SlashIndex = 0;

	if (!bAnnounce)
	{
		return;
	}

	const FString Name = Def ? Def->Name : TEXT("Kard");
	const FLinearColor Color = Def ? Def->Color : FLinearColor(0.95f, 0.78f, 0.38f);

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
		{
			HUD->AddTextPopup(GetActorLocation() + FVector(0.0f, 0.0f, 150.0f), Name, Color, 1.5f, 1.6f);
		}
	}

	if (USoundBase* Draw = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/UI/SW_UI_SwordDraw_01.SW_UI_SwordDraw_01")))
	{
		UGameplayStatics::PlaySound2D(this, Draw, 0.8f);
	}
}

float AAmnesiaCharacter::GetWeaponDamageMult() const
{
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (!Def)
	{
		return 1.0f;
	}

	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();
	float Mult = Def->DamageMult;

	if (Def->Branch == EAmnesiaSkillBranch::Chakram)
	{
		Mult *= SkillStats.ChakramDamageMult;

	} else if (Def->Branch == EAmnesiaSkillBranch::Axe) {

		Mult *= SkillStats.AxeDamageMult;

	} else if (Def->Branch == EAmnesiaSkillBranch::Wand) {

		Mult *= SkillStats.WandDamageMult;
	}

	return Mult;
}

float AAmnesiaCharacter::GetWeaponRateMult() const
{
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (!Def)
	{
		return 1.0f;
	}

	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();
	float Mult = Def->RateMult;

	if (Def->Branch == EAmnesiaSkillBranch::Wand)
	{

		Mult *= SkillStats.WandRateMult;
	}

	return Mult;
}

float AAmnesiaCharacter::GetWeaponReachMult() const
{
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (!Def)
	{
		return 1.0f;
	}

	return Def->ReachMult;
}

float AAmnesiaCharacter::GetWeaponRadiusMult() const
{
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (!Def)
	{
		return 1.0f;
	}

	return Def->RadiusMult * (Def->Branch == EAmnesiaSkillBranch::Axe ? Skills->GetStats().AxeRadiusMult : 1.0f);
}

EAmnesiaWeaponSpecial AAmnesiaCharacter::GetUnlockedSpecial() const
{
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (!Def)
	{
		return EAmnesiaWeaponSpecial::None;
	}

	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();

	switch (Def->Special)
	{
	case EAmnesiaWeaponSpecial::Quake:
		return SkillStats.bAxeQuake ? EAmnesiaWeaponSpecial::Quake : EAmnesiaWeaponSpecial::None;

	case EAmnesiaWeaponSpecial::Whirl:
		return SkillStats.bChakramWhirl ? EAmnesiaWeaponSpecial::Whirl : EAmnesiaWeaponSpecial::None;

	case EAmnesiaWeaponSpecial::Blast:
		return SkillStats.bWandBlast ? EAmnesiaWeaponSpecial::Blast : EAmnesiaWeaponSpecial::None;

	default:
		return EAmnesiaWeaponSpecial::None;
	}
}

void AAmnesiaCharacter::OnWeaponHit(AAmnesiaEnemy* Enemy, float Damage)
{
	const FAmnesiaWeaponDef* Def = GetEquippedWeaponDef();

	if (!Def || !Enemy)
	{
		return;
	}

	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();

	// the axe's wound bleeds out over three seconds
	if (Def->Branch == EAmnesiaSkillBranch::Axe && SkillStats.AxeBleedScale > 0.0f)
	{
		Enemy->ApplyBleed(Damage * SkillStats.AxeBleedScale / 3.0f, 3.0f, this);
	}

	// the wand's magic jumps to the enemies that stand next to the target
	if (Def->Branch == EAmnesiaSkillBranch::Wand && SkillStats.WandSplash > 0.0f)
	{
		for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
		{
			AAmnesiaEnemy* Other = *It;

			if (Other == Enemy || Other->GetHealth() <= 0.0f || Other->IsDormant() || FVector::Dist(Other->GetActorLocation(), Enemy->GetActorLocation()) > 320.0f)
			{
				continue;
			}

			Other->ApplyDamage(Damage * SkillStats.WandSplash, this, Other->GetActorLocation(), FVector::ZeroVector);
		}
	}
}

void AAmnesiaCharacter::PerformSpecial(EAmnesiaWeaponSpecial Special, float Facing)
{
	const FAmnesiaSkillStats& SkillStats = Skills->GetStats();
	const FVector Origin = GetActorLocation();
	const float Mult = GetWeaponDamageMult();

	FVector Center = Origin;
	float Radius = 0.0f;
	float Damage = 0.0f;
	float StunTime = 0.0f;
	float Push = 0.0f;

	switch (Special)
	{
	case EAmnesiaWeaponSpecial::Quake:
		// the ground shakes: a wave of dark flames runs out to both sides
		Center = Origin + FVector(Facing * 160.0f, 0.0f, -20.0f);
		Radius = 440.0f;
		Damage = MeleeDamage * SkillStats.AxeQuakeScale * Mult;
		StunTime = 1.6f;
		Push = 260.0f;

		Skills->PlayFlashStyled(Origin + FVector(Facing * 90.0f, 0.0f, -50.0f), EAmnesiaFxStyle::Dark);
		Skills->PlayWaveStyled(Origin, 1.0f, 60.0f, 440.0f, 5, 1.0f, EAmnesiaFxStyle::Dark);
		Skills->PlayWaveStyled(Origin, -1.0f, 60.0f, 440.0f, 5, 1.0f, EAmnesiaFxStyle::Dark);
		AddCameraShake(14.0f);
		break;

	case EAmnesiaWeaponSpecial::Blast:
		// a blast of magic in front of the hero
		Center = Origin + FVector(Facing * 300.0f, 0.0f, 40.0f);
		Radius = 380.0f;
		Damage = MeleeDamage * SkillStats.WandBlastScale * Mult;
		Push = 380.0f;

		Skills->PlayFlashStyled(Center, EAmnesiaFxStyle::Mystic);
		Skills->PlayWaveStyled(Origin, Facing, 120.0f, 560.0f, 6, 1.1f, EAmnesiaFxStyle::Mystic);
		AddCameraShake(12.0f);
		break;

	case EAmnesiaWeaponSpecial::Whirl:
		// the thrown disc does the whirl itself
		return;

	default:
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), QueryParams);

	TSet<AAmnesiaEnemy*> Done;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Overlap.GetActor());

		if (!Enemy || Done.Contains(Enemy) || Enemy->GetHealth() <= 0.0f || Enemy->IsDormant())
		{
			continue;
		}

		Done.Add(Enemy);

		const float Away = Enemy->GetActorLocation().X >= Origin.X ? 1.0f : -1.0f;

		Enemy->ApplyDamage(Damage, this, Enemy->GetActorLocation(), FVector(Away * Push, 0.0f, Push * 0.6f));
		Enemy->ApplyStun(StunTime);
		Skills->OnHeroHit(Enemy, Damage, false);
	}
}

void AAmnesiaCharacter::OnCycleWeaponPressed()
{
	CycleWeapon();
}

void AAmnesiaCharacter::SetupSideCamera()
{
	USpringArmComponent* Boom = GetCameraBoom();

	// look at the character from the +Y side, so +X is screen right
	Boom->SetUsingAbsoluteRotation(true);
	Boom->SetRelativeRotation(FRotator(SideCameraPitch, -90.0f, 0.0f));

	Boom->bUsePawnControlRotation = false;
	Boom->bDoCollisionTest = false;
	Boom->bEnableCameraRotationLag = false;

	Boom->TargetOffset = FVector(0.0f, 0.0f, SideCameraHeight);
	Boom->SocketOffset = FVector::ZeroVector;

	// keep the camera at the end of the boom
	GetFollowCamera()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
}
