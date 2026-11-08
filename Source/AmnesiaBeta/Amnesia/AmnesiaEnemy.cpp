#include "AmnesiaEnemy.h"
#include "AmnesiaFrost.h"
#include "AmnesiaCreatureAnimInstance.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaHUD.h"
#include "AmnesiaPickup.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

AAmnesiaEnemy::AAmnesiaEnemy()
{
	// use a plain AI Controller. The side scrolling brain runs on this character's tick
	AIControllerClass = AAIController::StaticClass();

	// configure the character movement component for side scrolling
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	Movement->MaxWalkSpeed = PatrolSpeed;

	// face the walking direction instead of the controller rotation
	Movement->bUseControllerDesiredRotation = false;
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	// don't walk off platforms
	Movement->bCanWalkOffLedges = false;

	// keep the enemy on the side scrolling plane
	Movement->SetPlaneConstraintNormal(FVector(0.0f, 1.0f, 0.0f));
	Movement->bConstrainToPlane = true;
	Movement->bSnapToPlaneAtStart = true;
}

void AAmnesiaEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// dead enemies don't think
	if (CurrentHP <= 0.0f)
	{
		return;
	}

	UpdateLungeVisual();
	UpdateStatus();

	// heavy steps while walking
	if (FootstepSounds.IsSet() && GetCharacterMovement()->IsMovingOnGround())
	{
		const float Speed = GetVelocity().Size2D();

		if (Speed > 40.0f)
		{
			FootstepCountdown -= DeltaTime * FMath::Clamp(Speed / 150.0f, 0.6f, 2.2f);

			if (FootstepCountdown <= 0.0f)
			{
				FootstepCountdown = FootstepInterval;
				FootstepSounds.PlayAt(this, GetActorLocation());
			}
		}
	}

	if (bDebugPlayAnimation)
	{
		bDebugPlayAnimation = false;

		if (UAmnesiaCreatureAnimInstance* Creature = GetCreatureAnimInstance())
		{
			Creature->PlayAction(DebugAnimation, 1.0f, false);
		}
	}

	// hold still while attacking or stunned
	if (bIsAttacking || GetWorld()->GetTimeSeconds() < StunEndTime)
	{
		return;
	}

	ACharacter* Player = GetTargetPlayer();

	if (Player)
	{
		const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();

		// once we're chasing, keep going until the player gets far enough away
		const float CurrentAggroRange = bIsChasing ? LoseAggroRange : AggroRange;

		bIsChasing = FMath::Abs(ToPlayer.X) <= CurrentAggroRange && FMath::Abs(ToPlayer.Z) <= AggroHeight;

	} else {

		bIsChasing = false;
	}

	if (bIsChasing)
	{
		UpdateChase(Player);

	} else {

		UpdatePatrol(DeltaTime);
	}
}

void AAmnesiaEnemy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	const float PreviousHP = CurrentHP;
	FVector Impulse = DamageImpulse;

	// the ice: hits hurt more, and nothing pushes a frozen enemy around
	if (IsFrozen() && Damage > 0.0f)
	{
		Damage *= FrozenDamageMult;
		Impulse = FVector::ZeroVector;
	}

	// fractional damage leaves float crumbs like 0.0000001 HP. A hit that brings the enemy that low kills it
	if (Damage > 0.0f && CurrentHP > 0.0f && CurrentHP - Damage < 0.01f)
	{
		Damage = CurrentHP + 0.01f;
	}

	Super::ApplyDamage(Damage, DamageCauser, DamageLocation, Impulse);

	// hit feedback: flash white, a sound and a damage number
	if (CurrentHP < PreviousHP)
	{
		FlashHit();

		HitSounds.PlayAt(this, GetActorLocation());

		// a pain grunt on some of the hits that don't kill
		if (CurrentHP > 0.0f && FMath::FRand() < HurtSoundChance)
		{
			HurtSounds.PlayAt(this, GetActorLocation());
		}

		if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(DamageCauser))
		{
			if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
			{
				if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
				{
					HUD->AddNumberPopup(GetActorLocation() + FVector(FMath::FRandRange(-30.0f, 30.0f), 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.6f), PreviousHP - FMath::Max(CurrentHP, 0.0f), FLinearColor(1.0f, 0.95f, 0.8f), TEXT(""));
				}
			}
		}
	}

	// the hero gets XP for the killing blow
	if (PreviousHP > 0.0f && CurrentHP <= 0.0f)
	{
		RewardKill(DamageCauser);
	}

	// only react if we actually lost HP and survived. The ice holds a frozen enemy still: no hit reaction, no turning around
	if (CurrentHP >= PreviousHP || CurrentHP <= 0.0f || IsFrozen())
	{
		return;
	}

	// interrupt the single animation attack
	CancelSingleAttack();

	// play a hit reaction instead of the partial ragdoll if we have one
	if (HitAnimations.Num() > 0)
	{
		if (UAmnesiaCreatureAnimInstance* Creature = GetCreatureAnimInstance())
		{
			GetMesh()->SetPhysicsBlendWeight(0.0f);

			Creature->PlayAction(HitAnimations[FMath::RandRange(0, HitAnimations.Num() - 1)]);
		}
	}

	// stay stunned for a moment, then go after whoever hit us
	StunEndTime = FMath::Max(GetWorld()->GetTimeSeconds() + HitStunTime, FreezeEndTime);
	bIsChasing = true;

	if (DamageCauser)
	{
		FaceDirection(DamageCauser->GetActorLocation().X - GetActorLocation().X);
	}
}

void AAmnesiaEnemy::ResetToHome()
{
	if (CurrentHP <= 0.0f)
	{
		return;
	}

	CancelSingleAttack();

	SetActorLocation(HomeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();

	CurrentHP = MaxHP;
	bIsChasing = false;
	StunEndTime = 0.0f;
	ClearStatus();
	PatrolWaitEndTime = GetWorld()->GetTimeSeconds() + PatrolWaitTime;
}

void AAmnesiaEnemy::SetDormant(bool bDormant)
{
	// the dead finish their fall, the asleep stay asleep
	if (bIsDormant == bDormant || CurrentHP <= 0.0f)
	{
		return;
	}

	bIsDormant = bDormant;

	if (bDormant)
	{
		CancelSingleAttack();
		GetCharacterMovement()->StopMovementImmediately();
		bIsChasing = false;
		BreakIce(false);
	}

	SetActorTickEnabled(!bDormant);
	GetCharacterMovement()->SetComponentTickEnabled(!bDormant);

	// no animation means no bone updates, so the physics bodies of the skeleton stop moving too
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetComponentTickEnabled(!bDormant);
	Body->bPauseAnims = bDormant;

	// a sleeping enemy is out of the scene altogether: hidden, and its capsule and skeleton bodies out of the physics world.
	// A level has many of them, and each skeleton's bodies cost physics time every frame even when nothing moves
	UCapsuleComponent* Capsule = GetCapsuleComponent();

	if (bDormant)
	{
		Body->SuspendClothingSimulation();

		SavedCapsuleCollision = Capsule->GetCollisionEnabled();
		SavedMeshCollision = Body->GetCollisionEnabled();
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetActorHiddenInGame(true);

	} else {

		Capsule->SetCollisionEnabled(SavedCapsuleCollision);
		Body->SetCollisionEnabled(SavedMeshCollision);
		SetActorHiddenInGame(false);
		Body->ResumeClothingSimulation();
	}
}

void AAmnesiaEnemy::FlashHit()
{
	if (!FlashMaterial)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Amnesia/Materials/M_StatusOverlay.M_StatusOverlay")))
		{
			FlashMaterial = UMaterialInstanceDynamic::Create(Base, this);
		}
	}

	if (FlashMaterial)
	{
		// the body keeps its texture, its edge shines
		FlashMaterial->SetVectorParameterValue(TEXT("Color"), HitFlashColor);
		FlashMaterial->SetScalarParameterValue(TEXT("Opacity"), HitFlashOpacity);
		FlashMaterial->SetScalarParameterValue(TEXT("RimBoost"), HitFlashRimBoost);
		FlashMaterial->SetScalarParameterValue(TEXT("RimPower"), 1.6f);

		GetMesh()->SetOverlayMaterial(FlashMaterial);
		GetWorld()->GetTimerManager().SetTimer(FlashTimer, this, &AAmnesiaEnemy::EndFlash, HitFlashTime, false);
	}
}

void AAmnesiaEnemy::EndFlash()
{
	GetMesh()->SetOverlayMaterial(bStatusActive ? StatusMaterial.Get() : nullptr);
}

void AAmnesiaEnemy::DropLoot()
{
	if (FMath::FRand() > HealDropChance)
	{
		return;
	}

	UClass* PickupClass = AAmnesiaPickup::StaticClass();

	for (int32 Index = 0; Index < FMath::Max(HealDropCount, 1); ++Index)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector Location = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);

		if (AAmnesiaPickup* Pickup = GetWorld()->SpawnActor<AAmnesiaPickup>(PickupClass, Location, FRotator::ZeroRotator, Params))
		{
			Pickup->Pop(FVector(FMath::FRandRange(-220.0f, 220.0f), 0.0f, FMath::FRandRange(420.0f, 620.0f)));
		}
	}
}

void AAmnesiaEnemy::HandleDeath()
{
	// cancel any pending attack
	CancelSingleAttack();
	ClearStatus();

	DeathSounds.PlayAt(this, GetActorLocation());

	if (DeathFallSounds.IsSet())
	{
		GetWorld()->GetTimerManager().SetTimer(DeathFallTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			DeathFallSounds.PlayAt(this, GetActorLocation());

		}), FMath::Max(DeathFallDelay, 0.01f), false);
	}

	UAmnesiaCreatureAnimInstance* Creature = GetCreatureAnimInstance();

	// ragdoll if we don't have a death animation
	if (!DeathAnimation || !Creature)
	{
		Super::HandleDeath();
		return;
	}

	// hide the life bar
	if (UWidgetComponent* LifeBarComponent = FindComponentByClass<UWidgetComponent>())
	{
		LifeBarComponent->SetHiddenInGame(true);
	}

	// disable the collision capsule to avoid being hit again while dead
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable character movement
	GetCharacterMovement()->DisableMovement();

	// play the death animation and stay on its last frame
	GetMesh()->SetPhysicsBlendWeight(0.0f);
	Creature->PlayAction(DeathAnimation, 1.0f, true);

	// call the died delegate to notify any subscribers
	OnEnemyDied.Broadcast();

	// set up the death timer
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &AAmnesiaEnemy::RemoveFromLevel, DeathRemovalTime);
}

void AAmnesiaEnemy::BeginPlay()
{
	Super::BeginPlay();

	// stronger enemies further into the level
	const int32 LevelsAbove = FMath::Max(EnemyLevel - 1, 0);

	MaxHP *= HPMultiplier * (1.0f + (HPBonusPerLevel + ExtraHPBonusPerLevel) * LevelsAbove);
	CurrentHP = MaxHP;
	MeleeDamage *= DamageMultiplier * (1.0f + (DamageBonusPerLevel + ExtraDamageBonusPerLevel) * LevelsAbove);
	XPReward = FMath::RoundToInt(XPReward * XPMultiplier * (1.0f + XPBonusPerLevel * LevelsAbove));

	// more aggressive: attacks follow each other faster, a hit stuns for less, the chase is quicker
	AttackCooldown *= AttackCooldownScale;
	HitStunTime *= HitStunScale;
	ChaseSpeed *= ChaseSpeedScale;

	// grow the enemy. The capsule and mesh scale with the actor, so lift it to keep the feet on the ground
	if (!FMath::IsNearlyEqual(SizeScale, 1.0f))
	{
		const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		SetActorScale3D(GetActorScale3D() * SizeScale);
		AddActorWorldOffset(FVector(0.0f, 0.0f, HalfHeight * (SizeScale - 1.0f) + 5.0f));

		AttackRange *= SizeScale;
	}

	// attach the hammer, shield and so on
	for (const FAmnesiaEnemyProp& Prop : AttachedProps)
	{
		if (!Prop.Mesh)
		{
			continue;
		}

		UStaticMeshComponent* PropComponent = NewObject<UStaticMeshComponent>(this);
		PropComponent->SetStaticMesh(Prop.Mesh);
		PropComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PropComponent->SetCanEverAffectNavigation(false);
		PropComponent->RegisterComponent();
		PropComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Prop.Socket);
		PropComponent->SetRelativeTransform(Prop.Offset);
	}

	// remember the mesh rest pose for the lunge
	MeshBaseRotation = GetMesh()->GetRelativeRotation().Quaternion();
	MeshBaseLocation = GetMesh()->GetRelativeLocation();

	// center the patrol path on the starting point
	HomeX = GetActorLocation().X;
	HomeLocation = GetActorLocation();

	// start patrolling in the direction we're facing
	PatrolDirection = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;

	// the HUD draws our HP, so hide the floating life bar
	if (bHideWorldLifeBar)
	{
		if (UWidgetComponent* LifeBarComponent = FindComponentByClass<UWidgetComponent>())
		{
			LifeBarComponent->SetHiddenInGame(true);
		}
	}

	// hand our animations to the creature anim instance
	if (UAmnesiaCreatureAnimInstance* Creature = GetCreatureAnimInstance())
	{
		Creature->IdleAnimation = IdleAnimation;
		Creature->WalkAnimation = WalkAnimation;
		Creature->RunAnimation = RunAnimation;
		Creature->WalkAnimationSpeed = WalkAnimationSpeed;
		Creature->RunAnimationSpeed = RunAnimationSpeed;

		// walk while patrolling, run while chasing
		Creature->RunThreshold = (PatrolSpeed + ChaseSpeed) * 0.5f;
	}
}

void AAmnesiaEnemy::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the attack timers
	GetWorld()->GetTimerManager().ClearTimer(AttackHitTimer);
	GetWorld()->GetTimerManager().ClearTimer(AttackEndTimer);
	GetWorld()->GetTimerManager().ClearTimer(DeathFallTimer);
}

UAmnesiaCreatureAnimInstance* AAmnesiaEnemy::GetCreatureAnimInstance() const
{
	return Cast<UAmnesiaCreatureAnimInstance>(GetMesh()->GetAnimInstance());
}

ACharacter* AAmnesiaEnemy::GetTargetPlayer() const
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	// ignore the player while they're dead and waiting to respawn
	if (!Player || Player->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_None)
	{
		return nullptr;
	}

	return Player;
}

void AAmnesiaEnemy::UpdatePatrol(float DeltaTime)
{
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed * GetSpeedScale();

	// wait at the end of the patrol path
	if (GetWorld()->GetTimeSeconds() < PatrolWaitEndTime)
	{
		BlockedTime = 0.0f;
		return;
	}

	// keep track of how long a wall or ledge has been stopping us
	BlockedTime = FMath::Abs(GetVelocity().X) < 5.0f ? BlockedTime + DeltaTime : 0.0f;

	// have we reached the end of the patrol path, or are we stuck?
	const float OffsetFromHome = GetActorLocation().X - HomeX;

	if (OffsetFromHome * PatrolDirection >= PatrolDistance || BlockedTime >= BlockedTurnTime)
	{
		// turn around and wait for a bit
		PatrolDirection *= -1.0f;
		PatrolWaitEndTime = GetWorld()->GetTimeSeconds() + PatrolWaitTime;
		BlockedTime = 0.0f;

		return;
	}

	AddMovementInput(FVector(PatrolDirection, 0.0f, 0.0f));
}

void AAmnesiaEnemy::UpdateChase(const ACharacter* Player)
{
	const float DeltaX = Player->GetActorLocation().X - GetActorLocation().X;

	// is the player within attack range?
	if (FMath::Abs(DeltaX) <= AttackRange)
	{
		// turn towards the player and attack once the cooldown is over
		FaceDirection(DeltaX);

		if (GetWorld()->GetTimeSeconds() - LastAttackTime >= AttackCooldown / FMath::Max(GetSpeedScale(), 0.3f))
		{
			StartAttack();
		}

		return;
	}

	// run towards the player
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * GetSpeedScale();

	AddMovementInput(FVector(DeltaX > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f));
}

void AAmnesiaEnemy::FaceDirection(float Direction)
{
	SetActorRotation(FRotator(0.0f, Direction >= 0.0f ? 0.0f : 180.0f, 0.0f));
}

void AAmnesiaEnemy::StartAttack()
{
	LastAttackTime = GetWorld()->GetTimeSeconds();

	AttackSounds.PlayAt(this, GetActorLocation());

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!AnimInstance)
	{
		return;
	}

	// no attack animations: lunge with the whole body
	if (Attacks.Num() == 0 && LungeDuration > 0.0f)
	{
		bIsAttacking = true;
		LungeStartTime = GetWorld()->GetTimeSeconds();
		CurrentAttack.SourceBone = LungeSourceBone;

		GetWorld()->GetTimerManager().SetTimer(AttackHitTimer, this, &AAmnesiaEnemy::SingleAttackHit, FMath::Max(LungeHitTime, 0.01f), false);
		GetWorld()->GetTimerManager().SetTimer(AttackEndTimer, this, &AAmnesiaEnemy::SingleAttackEnded, LungeDuration, false);

		return;
	}

	// use the combo attack montage if we don't have single animation attacks
	if (Attacks.Num() == 0)
	{
		DoAIComboAttack();

		// don't get stuck in the attacking state if the montage couldn't play
		if (!AnimInstance->Montage_IsPlaying(ComboAttackMontage))
		{
			bIsAttacking = false;
		}

		return;
	}

	// pick one of the attacks
	CurrentAttack = Attacks[FMath::RandRange(0, Attacks.Num() - 1)];

	if (!CurrentAttack.Animation)
	{
		return;
	}

	const float PlayRate = FMath::Max(CurrentAttack.PlayRate, 0.1f);

	if (UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(AnimInstance))
	{
		// play the attack on the creature anim instance and end it with a timer
		const float Length = Creature->PlayAction(CurrentAttack.Animation, PlayRate);

		GetWorld()->GetTimerManager().SetTimer(AttackEndTimer, this, &AAmnesiaEnemy::SingleAttackEnded, FMath::Max(Length, 0.1f), false);

	} else {

		// play the attack as a dynamic montage on the Anim Blueprint slot
		SingleAttackMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(CurrentAttack.Animation, AttackSlotName, 0.25f, 0.25f, PlayRate);

		if (!SingleAttackMontage)
		{
			return;
		}

		// reset the attacking flag when the montage ends
		AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, SingleAttackMontage);
	}

	// raise the attacking flag and schedule the hit check
	bIsAttacking = true;

	GetWorld()->GetTimerManager().SetTimer(AttackHitTimer, this, &AAmnesiaEnemy::SingleAttackHit, FMath::Max(CurrentAttack.HitTime / PlayRate, 0.01f), false);
}

void AAmnesiaEnemy::UpdateLungeVisual()
{
	if (LungeDuration <= 0.0f)
	{
		return;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - LungeStartTime;

	// rest pose
	float Curve = 0.0f;

	if (Elapsed >= 0.0f && Elapsed < LungeDuration)
	{
		// lean back until the hit time, snap forward through it, then ease back to rest
		const float HitTime = FMath::Clamp(LungeHitTime, 0.05f, LungeDuration - 0.05f);

		if (Elapsed < HitTime * 0.75f)
		{
			Curve = -0.35f * FMath::Sin(Elapsed / (HitTime * 0.75f) * UE_HALF_PI);

		} else if (Elapsed < HitTime) {

			Curve = FMath::Lerp(-0.35f, 1.0f, (Elapsed - HitTime * 0.75f) / (HitTime * 0.25f));

		} else {

			Curve = FMath::Lerp(1.0f, 0.0f, FMath::SmoothStep(0.0f, 1.0f, (Elapsed - HitTime) / (LungeDuration - HitTime)));
		}

		bLungePoseDirty = true;

	} else if (bLungePoseDirty) {

		// finish by putting the mesh back at rest once
		bLungePoseDirty = false;

	} else {

		return;
	}

	// negative pitch leans the top of the body forward along the actor's X axis
	const FQuat Lean = FRotator(-LungePitch * Curve, 0.0f, 0.0f).Quaternion();

	GetMesh()->SetRelativeRotation(Lean * MeshBaseRotation);
	GetMesh()->SetRelativeLocation(MeshBaseLocation + FVector(LungeStep * FMath::Max(Curve, 0.0f), 0.0f, 0.0f));
}

void AAmnesiaEnemy::SingleAttackHit()
{
	DoAttackTrace(CurrentAttack.SourceBone);
}

void AAmnesiaEnemy::SingleAttackEnded()
{
	bIsAttacking = false;
}

void AAmnesiaEnemy::CancelSingleAttack()
{
	GetWorld()->GetTimerManager().ClearTimer(AttackHitTimer);
	GetWorld()->GetTimerManager().ClearTimer(AttackEndTimer);

	// stop the lunge pose right away
	LungeStartTime = -1000.0f;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (SingleAttackMontage)
		{
			AnimInstance->Montage_Stop(0.1f, SingleAttackMontage);
		}

		if (UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(AnimInstance))
		{
			Creature->StopAction();
		}
	}

	bIsAttacking = false;
}

// ---------------------------------------------------------------- loot and status effects

void AAmnesiaEnemy::RewardKill(AActor* Killer)
{
	DropLoot();

	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(Killer))
	{
		AAmnesiaHUD* HUD = nullptr;

		if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
		{
			HUD = Cast<AAmnesiaHUD>(PC->GetHUD());
		}

		const FVector Above = GetActorLocation() + FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 20.0f);

		// show the reward over the enemy's head
		if (HUD && XPReward > 0 && !Hero->IsMaxLevel())
		{
			HUD->AddXPPopup(Above, XPReward);
		}

		Hero->GainXP(XPReward);
	}
}

void AAmnesiaEnemy::ApplyStatusDamage(float Amount, AActor* Source)
{
	if (CurrentHP <= 0.0f || Amount <= 0.0f)
	{
		return;
	}

	const float Previous = CurrentHP;
	float Damage = Amount;

	// the same float crumb rule as for hits
	if (CurrentHP - Damage < 0.01f)
	{
		Damage = CurrentHP + 0.01f;
	}

	FDamageEvent DamageEvent;
	TakeDamage(Damage, DamageEvent, nullptr, Source);

	// the base class starts a partial ragdoll for every hit, a burn must not shake the enemy
	if (CurrentHP > 0.0f)
	{
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}

	if (Previous > 0.0f && CurrentHP <= 0.0f)
	{
		RewardKill(Source);
	}
}

void AAmnesiaEnemy::ApplyBurn(float DamagePerSecond, float Duration, AActor* Source, UNiagaraSystem* Effect)
{
	if (CurrentHP <= 0.0f || Duration <= 0.0f || DamagePerSecond <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	// fresh fire hurts after half a second, a fire that burns already keeps the stronger of the two
	if (Now >= BurnEndTime)
	{
		NextBurnTick = Now + 0.5f;
		BurnDps = DamagePerSecond;

	} else {

		BurnDps = FMath::Max(BurnDps, DamagePerSecond);
	}

	BurnEndTime = FMath::Max(BurnEndTime, Now + Duration);
	BurnSource = Source;

	if (Effect)
	{
		BurnFxSystem = Effect;
	}

	bStatusActive = true;
}

void AAmnesiaEnemy::ApplyChill(float Slow, float Duration, UNiagaraSystem* Effect)
{
	if (CurrentHP <= 0.0f || Duration <= 0.0f || Slow <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float Wanted = FMath::Clamp(Slow, 0.0f, 0.85f);

	ChillSlow = Now < ChillEndTime ? FMath::Max(ChillSlow, Wanted) : Wanted;
	ChillEndTime = FMath::Max(ChillEndTime, Now + Duration);

	if (Effect)
	{
		FreezeFxSystem = Effect;
	}

	bStatusActive = true;
}

void AAmnesiaEnemy::ApplyFreeze(float Duration, UNiagaraSystem* Effect, float ShatterDamage, AActor* Source)
{
	if (CurrentHP <= 0.0f || Duration <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bWasFrozen = Now < FreezeEndTime;

	FreezeEndTime = FMath::Max(FreezeEndTime, Now + Duration);
	StunEndTime = FMath::Max(StunEndTime, FreezeEndTime);

	// frozen again before the ice broke: it keeps the harder of the two shatters
	FreezeShatterDamage = FMath::Max(bWasFrozen ? FreezeShatterDamage : 0.0f, ShatterDamage);

	if (Source)
	{
		FreezeSource = Source;
	}

	if (Effect)
	{
		FreezeFxSystem = Effect;
	}

	CancelSingleAttack();
	GetCharacterMovement()->StopMovementImmediately();

	// the crystals grow around the body
	if (!bIceUp || !Frost.IsValid())
	{
		SpawnFrost();
	}

	bStatusActive = true;
}

void AAmnesiaEnemy::SpawnFrost()
{
	if (Frost.IsValid() || !GetWorld())
	{
		return;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AAmnesiaFrost* Ice = GetWorld()->SpawnActor<AAmnesiaFrost>(AAmnesiaFrost::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Ice->Grow(this, Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
		Frost = Ice;
		bIceUp = true;
	}
}

void AAmnesiaEnemy::BreakIce(bool bShatter)
{
	const bool bHadIce = bIceUp;
	bIceUp = false;

	if (AAmnesiaFrost* Ice = Frost.Get())
	{
		if (bShatter)
		{
			Ice->Shatter();

		} else {

			Ice->Melt();
		}
	}

	Frost.Reset();

	// the broken ice cuts: it hurts once, when it breaks by itself
	const float Cut = FreezeShatterDamage;
	FreezeShatterDamage = 0.0f;

	if (bShatter && bHadIce && Cut > 0.0f && CurrentHP > 0.0f)
	{
		ApplyStatusDamage(Cut, FreezeSource.Get());
	}
}

void AAmnesiaEnemy::ApplyBleed(float DamagePerSecond, float Duration, AActor* Source)
{
	if (CurrentHP <= 0.0f || Duration <= 0.0f || DamagePerSecond <= 0.0f)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	if (Now >= BleedEndTime)
	{
		NextBleedTick = Now + 0.5f;
		BleedDps = DamagePerSecond;

	} else {

		BleedDps = FMath::Max(BleedDps, DamagePerSecond);
	}

	BleedEndTime = FMath::Max(BleedEndTime, Now + Duration);
	BleedSource = Source;
	bStatusActive = true;
}

void AAmnesiaEnemy::ApplyStun(float Seconds)
{
	if (CurrentHP <= 0.0f || Seconds <= 0.0f)
	{
		return;
	}

	StunEndTime = FMath::Max(StunEndTime, GetWorld()->GetTimeSeconds() + Seconds);

	CancelSingleAttack();
	GetCharacterMovement()->StopMovementImmediately();
}

bool AAmnesiaEnemy::IsFrozen() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < FreezeEndTime;
}

float AAmnesiaEnemy::GetSpeedScale() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < ChillEndTime ? 1.0f - ChillSlow : 1.0f;
}

void AAmnesiaEnemy::UpdateStatus()
{
	if (!bStatusActive)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bBurning = Now < BurnEndTime;
	const bool bChilled = Now < ChillEndTime;
	const bool bFrozen = Now < FreezeEndTime;
	const bool bBleeding = Now < BleedEndTime;

	// the freeze is over: the ice breaks
	if (bIceUp && !bFrozen)
	{
		BreakIce(true);

		if (CurrentHP <= 0.0f)
		{
			return;
		}
	}

	if (!bBurning && !bChilled && !bFrozen && !bBleeding)
	{
		ClearStatus();
		return;
	}

	// the wound bleeds away a bit of health every half second
	if (bBleeding && Now >= NextBleedTick)
	{
		NextBleedTick = Now + 0.5f;
		ApplyStatusDamage(BleedDps * 0.5f, BleedSource.Get());

		if (CurrentHP <= 0.0f)
		{
			return;
		}
	}

	// the fire burns away a bit of health every half second
	if (bBurning && Now >= NextBurnTick)
	{
		NextBurnTick = Now + 0.5f;
		ApplyStatusDamage(BurnDps * 0.5f, BurnSource.Get());

		if (CurrentHP <= 0.0f)
		{
			return;
		}
	}

	// a glow over the body that is brightest at its rim: ice blue when frozen or chilled, a flickering orange when burning
	if (!StatusMaterial)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Amnesia/Materials/M_StatusOverlay.M_StatusOverlay")))
		{
			StatusMaterial = UMaterialInstanceDynamic::Create(Base, this);
		}
	}

	if (StatusMaterial)
	{
		if (bFrozen)
		{
			StatusMaterial->SetVectorParameterValue(TEXT("Color"), FreezeTint);
			StatusMaterial->SetScalarParameterValue(TEXT("Opacity"), FreezeTintOpacity);
			StatusMaterial->SetScalarParameterValue(TEXT("RimBoost"), FreezeRimBoost);
			StatusMaterial->SetScalarParameterValue(TEXT("RimPower"), FreezeRimPower);

		} else if (bBurning) {

			// a flicker from two unrelated waves, so the fire does not pulse like a heartbeat
			const float Flicker = 0.6f * FMath::Sin(Now * 13.0f) + 0.4f * FMath::Sin(Now * 31.0f + 1.3f);
			StatusMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(BurnTint.R, BurnTint.G + 0.12f * Flicker, BurnTint.B));
			StatusMaterial->SetScalarParameterValue(TEXT("Opacity"), BurnTintOpacity * (1.0f + 0.25f * Flicker));
			StatusMaterial->SetScalarParameterValue(TEXT("RimBoost"), BurnRimBoost * (1.0f + 0.15f * Flicker));
			StatusMaterial->SetScalarParameterValue(TEXT("RimPower"), RimPower);

		} else if (bChilled) {

			StatusMaterial->SetVectorParameterValue(TEXT("Color"), ChillTint);
			StatusMaterial->SetScalarParameterValue(TEXT("Opacity"), ChillTintOpacity);
			StatusMaterial->SetScalarParameterValue(TEXT("RimBoost"), ChillRimBoost);
			StatusMaterial->SetScalarParameterValue(TEXT("RimPower"), RimPower);

		} else {

			StatusMaterial->SetVectorParameterValue(TEXT("Color"), BleedTint);
			StatusMaterial->SetScalarParameterValue(TEXT("Opacity"), BleedTintOpacity);
			StatusMaterial->SetScalarParameterValue(TEXT("RimBoost"), BleedRimBoost);
			StatusMaterial->SetScalarParameterValue(TEXT("RimPower"), RimPower);
		}

		// the white flash of a hit goes first
		if (!GetWorld()->GetTimerManager().IsTimerActive(FlashTimer))
		{
			GetMesh()->SetOverlayMaterial(StatusMaterial);
		}
	}

	// a frozen enemy holds its pose
	GetMesh()->GlobalAnimRateScale = bFrozen ? 0.0f : 1.0f;

	// the flames: blue ones on the frozen, red ones on the burning
	UNiagaraSystem* WantedFx = bFrozen ? FreezeFxSystem.Get() : (bBurning ? BurnFxSystem.Get() : nullptr);

	if (WantedFx != StatusFxSystem)
	{
		DestroyStatusFlames();
		StatusFxSystem = WantedFx;

		if (WantedFx)
		{
			SpawnStatusFlames(WantedFx);
		}
	}
}

void AAmnesiaEnemy::SpawnStatusFlames(UNiagaraSystem* System)
{
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float BaseScale = FMath::Max(HalfHeight / StatusFxHeightDivisor, 0.6f) * StatusFlameScale;

	// flames at different heights and off the middle line, so the body burns in several places and not under one big sheet:
	// the height as a part of the half height, the sideways offset, and the size against the biggest
	struct FSpot
	{
		float Height;
		float Side;
		float Size;
	};

	static const FSpot Spots[] = { { -0.5f, 0.12f, 1.0f }, { 0.0f, -0.15f, 0.85f }, { 0.45f, 0.12f, 0.8f }, { 0.85f, -0.05f, 0.6f }, { -0.15f, 0.2f, 0.7f }, { 0.3f, -0.2f, 0.65f } };

	for (int32 Index = 0; Index < FMath::Min(StatusFlameCount, static_cast<int32>(UE_ARRAY_COUNT(Spots))); ++Index)
	{
		const FSpot& Spot = Spots[Index];

		UNiagaraComponent* Flame = UNiagaraFunctionLibrary::SpawnSystemAttached(System, GetCapsuleComponent(), NAME_None,
			FVector(HalfHeight * Spot.Side, 12.0f, HalfHeight * Spot.Height), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true);

		if (!Flame)
		{
			continue;
		}

		Flame->SetVariableFloat(FName("User.Scale Overall"), BaseScale * Spot.Size);
		Flame->SetVariableFloat(FName("User.Lifetime Min"), StatusFlameLifetime);
		Flame->SetVariableFloat(FName("User.Lifetime Max"), StatusFlameLifetime * 1.2f);
		Flame->SetVariableFloat(FName("User.Overall Speed"), StatusFlameSpeed);
		Flame->SetVariableFloat(FName("User.Scale Size Horizontal"), StatusFlameWidth);
		Flame->SetVariableFloat(FName("User.Scale Size Vertical"), StatusFlameHeight);
		StatusFxList.Add(Flame);
	}
}

void AAmnesiaEnemy::DestroyStatusFlames()
{
	for (UNiagaraComponent* Flame : StatusFxList)
	{
		if (Flame)
		{
			Flame->DestroyComponent();
		}
	}

	StatusFxList.Reset();
}

void AAmnesiaEnemy::ClearStatus()
{
	BurnEndTime = 0.0f;
	ChillEndTime = 0.0f;
	FreezeEndTime = 0.0f;
	BleedEndTime = 0.0f;
	bStatusActive = false;

	// the ice breaks if the enemy died in it, and just melts if the enemy was only put back
	BreakIce(CurrentHP <= 0.0f);

	DestroyStatusFlames();
	StatusFxSystem = nullptr;

	if (USkeletalMeshComponent* Body = GetMesh())
	{
		Body->GlobalAnimRateScale = 1.0f;

		if (!GetWorld() || !GetWorld()->GetTimerManager().IsTimerActive(FlashTimer))
		{
			Body->SetOverlayMaterial(nullptr);
		}
	}
}
