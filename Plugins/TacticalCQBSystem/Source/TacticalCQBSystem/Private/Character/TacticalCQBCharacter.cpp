#include "Character/TacticalCQBCharacter.h"
#include "Components/TacticalMovementComponent.h"
#include "Components/TacticalWeaponComponent.h"
#include "Components/CQBTakedownComponent.h"
#include "Components/TacticalTimeDilationComponent.h"
#include "Components/BallisticImpactComponent.h"
#include "Actors/TacticalWeaponPickup.h"
#include "Actors/TacticalBreachDoor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

ATacticalCQBCharacter::ATacticalCQBCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTacticalMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// Full-body lower limbs mesh (GetMesh()) remains visible to owner for kicks, slides, vaults, and supine dives
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(10.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(DefaultFOV);

	FirstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmsMesh"));
	FirstPersonArmsMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonArmsMesh->SetOnlyOwnerSee(true);
	FirstPersonArmsMesh->CastShadow = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(FirstPersonCamera);
	WeaponMesh->SetOnlyOwnerSee(true);
	WeaponMesh->CastShadow = false;

	WeaponComponent = CreateDefaultSubobject<UTacticalWeaponComponent>(TEXT("WeaponComponent"));
	CQBComponent = CreateDefaultSubobject<UCQBTakedownComponent>(TEXT("CQBComponent"));
	TimeDilationComponent = CreateDefaultSubobject<UTacticalTimeDilationComponent>(TEXT("TimeDilationComponent"));
	ImpactComponent = CreateDefaultSubobject<UBallisticImpactComponent>(TEXT("ImpactComponent"));
}

void ATacticalCQBCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	if (WeaponComponent)
	{
		WeaponComponent->InitializeWeaponMesh(WeaponMesh);
	}
}

void ATacticalCQBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateProceduralCamera(DeltaTime);
}

UTacticalMovementComponent* ATacticalCQBCharacter::GetTacticalMovement() const
{
	return Cast<UTacticalMovementComponent>(GetCharacterMovement());
}

void ATacticalCQBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Move) EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ATacticalCQBCharacter::Input_Move);
		if (IA_Look) EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ATacticalCQBCharacter::Input_Look);
		if (IA_JumpVault) EIC->BindAction(IA_JumpVault, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_JumpOrVault);
		if (IA_Sprint)
		{
			EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_SprintStart);
			EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ATacticalCQBCharacter::Input_SprintStop);
		}
		if (IA_SlideCrouch) EIC->BindAction(IA_SlideCrouch, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_SlideOrCrouch);
		if (IA_SupineDive) EIC->BindAction(IA_SupineDive, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_SupineDive);
		if (IA_LeanLeft)
		{
			EIC->BindAction(IA_LeanLeft, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_LeanLeftStart);
			EIC->BindAction(IA_LeanLeft, ETriggerEvent::Completed, this, &ATacticalCQBCharacter::Input_LeanStop);
		}
		if (IA_LeanRight)
		{
			EIC->BindAction(IA_LeanRight, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_LeanRightStart);
			EIC->BindAction(IA_LeanRight, ETriggerEvent::Completed, this, &ATacticalCQBCharacter::Input_LeanStop);
		}
		if (IA_Fire)
		{
			EIC->BindAction(IA_Fire, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_FireStart);
			EIC->BindAction(IA_Fire, ETriggerEvent::Completed, this, &ATacticalCQBCharacter::Input_FireStop);
		}
		if (IA_ADS)
		{
			EIC->BindAction(IA_ADS, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_ADSStart);
			EIC->BindAction(IA_ADS, ETriggerEvent::Completed, this, &ATacticalCQBCharacter::Input_ADSStop);
		}
		if (IA_Reload) EIC->BindAction(IA_Reload, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_Reload);
		if (IA_Suppressor) EIC->BindAction(IA_Suppressor, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_ToggleSuppressor);
		if (IA_InteractExecute) EIC->BindAction(IA_InteractExecute, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_InteractOrExecute);
		if (IA_KickBreach) EIC->BindAction(IA_KickBreach, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_KickOrBreach);
		if (IA_Grenade) EIC->BindAction(IA_Grenade, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_Grenade);
		if (IA_BulletTime) EIC->BindAction(IA_BulletTime, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_BulletTime);
		if (IA_Slot1) EIC->BindAction(IA_Slot1, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_Slot1);
		if (IA_Slot2) EIC->BindAction(IA_Slot2, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_Slot2);
		if (IA_Slot3) EIC->BindAction(IA_Slot3, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_Slot3);
		if (IA_Slot4) EIC->BindAction(IA_Slot4, ETriggerEvent::Started, this, &ATacticalCQBCharacter::Input_Slot4);
	}
}

void ATacticalCQBCharacter::Input_Move(const FInputActionValue& Value)
{
	LastMoveInput = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), LastMoveInput.Y);
	AddMovementInput(GetActorRightVector(), LastMoveInput.X);
}

void ATacticalCQBCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Look = Value.Get<FVector2D>();
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y);
}

void ATacticalCQBCharacter::Input_JumpOrVault()
{
	if (UTacticalMovementComponent* Move = GetTacticalMovement())
	{
		if (Move->TryVaultOverObstacle()) return;
	}
	Jump();
}

void ATacticalCQBCharacter::Input_SprintStart() { if (auto* M = GetTacticalMovement()) M->SetSprinting(true); }
void ATacticalCQBCharacter::Input_SprintStop()  { if (auto* M = GetTacticalMovement()) M->SetSprinting(false); }

void ATacticalCQBCharacter::Input_SlideOrCrouch()
{
	if (UTacticalMovementComponent* Move = GetTacticalMovement())
	{
		if (Move->TryStartCombatSlide()) return;
	}
	if (bIsCrouched) UnCrouch(); else Crouch();
}

void ATacticalCQBCharacter::Input_SupineDive()
{
	if (UTacticalMovementComponent* Move = GetTacticalMovement())
	{
		const FVector DiveDir = (GetActorForwardVector() * LastMoveInput.Y + GetActorRightVector() * LastMoveInput.X);
		if (Move->TryStartSupineDive(DiveDir) && TimeDilationComponent)
		{
			TimeDilationComponent->TriggerTimedBulletTime(1.25f);
		}
	}
}

void ATacticalCQBCharacter::Input_LeanLeftStart()  { if (auto* M = GetTacticalMovement()) M->SetLeanInput(-1.0f); }
void ATacticalCQBCharacter::Input_LeanRightStart() { if (auto* M = GetTacticalMovement()) M->SetLeanInput(1.0f); }
void ATacticalCQBCharacter::Input_LeanStop()       { if (auto* M = GetTacticalMovement()) M->SetLeanInput(0.0f); }

void ATacticalCQBCharacter::Input_FireStart() { if (WeaponComponent) WeaponComponent->StartFire(); }
void ATacticalCQBCharacter::Input_FireStop()  { if (WeaponComponent) WeaponComponent->StopFire(); }
void ATacticalCQBCharacter::Input_ADSStart()  { if (WeaponComponent) WeaponComponent->SetAiming(true); }
void ATacticalCQBCharacter::Input_ADSStop()   { if (WeaponComponent) WeaponComponent->SetAiming(false); }
void ATacticalCQBCharacter::Input_Reload()    { if (WeaponComponent) WeaponComponent->StartReload(); }
void ATacticalCQBCharacter::Input_ToggleSuppressor() { if (WeaponComponent) WeaponComponent->ToggleSuppressor(); }

void ATacticalCQBCharacter::Input_InteractOrExecute()
{
	// Priority 1: Execute held human shield
	if (CQBComponent && CQBComponent->IsHoldingHumanShield())
	{
		CQBComponent->TryExecuteShieldOrNeckSnap();
		return;
	}

	// Priority 2: World weapon pickup or breachable door
	const FVector Start = GetPawnViewLocation();
	const FVector End = Start + GetControlRotation().Vector() * 210.0f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalInteract), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (ATacticalWeaponPickup* Pickup = Cast<ATacticalWeaponPickup>(Hit.GetActor()))
		{
			if (WeaponComponent && Pickup->WeaponData)
			{
				WeaponComponent->PickupAndReplaceWeapon(Pickup->WeaponData, Pickup->AmmoInMag);
				Pickup->Destroy();
				return;
			}
		}
		if (ATacticalBreachDoor* Door = Cast<ATacticalBreachDoor>(Hit.GetActor()))
		{
			Door->BreachOpen(GetActorLocation(), false);
			return;
		}
	}

	// Priority 3: Grab nearby enemy as human shield
	if (CQBComponent)
	{
		CQBComponent->TryGrabHumanShield(FirstPersonCamera);
	}
}

void ATacticalCQBCharacter::Input_KickOrBreach()
{
	if (CQBComponent) CQBComponent->TryFrontSpartanKick();
}

void ATacticalCQBCharacter::Input_Grenade()
{
	if (CQBComponent)
	{
		CQBComponent->TryPlantGrenadeOnShieldOrThrow(
			GetPawnViewLocation() + GetControlRotation().Vector() * 35.0f,
			GetControlRotation().Vector());
	}
}

void ATacticalCQBCharacter::Input_BulletTime() { if (TimeDilationComponent) TimeDilationComponent->ToggleBulletTime(); }
void ATacticalCQBCharacter::Input_Slot1() { if (WeaponComponent) WeaponComponent->EquipSlot(ETacticalWeaponSlot::Primary); }
void ATacticalCQBCharacter::Input_Slot2() { if (WeaponComponent) WeaponComponent->EquipSlot(ETacticalWeaponSlot::Sidearm); }
void ATacticalCQBCharacter::Input_Slot3() { if (WeaponComponent) WeaponComponent->EquipSlot(ETacticalWeaponSlot::Melee); }
void ATacticalCQBCharacter::Input_Slot4() { if (WeaponComponent) WeaponComponent->EquipSlot(ETacticalWeaponSlot::DualWield); }

void ATacticalCQBCharacter::UpdateProceduralCamera(float DeltaTime)
{
	if (!FirstPersonCamera) return;
	UTacticalMovementComponent* Move = GetTacticalMovement();
	if (!Move) return;

	const float LeanAlpha = Move->GetCurrentLeanAlpha();
	const ETacticalMovementState State = Move->GetTacticalState();

	float TargetZ = bIsCrouched ? 34.0f : 64.0f;
	if (State == ETacticalMovementState::Sliding) TargetZ = 24.0f;
	if (State == ETacticalMovementState::SupineDive) TargetZ = 18.0f;

	const float TargetY = LeanAlpha * 28.0f;
	const FVector CurrentLoc = FirstPersonCamera->GetRelativeLocation();
	FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(CurrentLoc, FVector(10.0f, TargetY, TargetZ), DeltaTime, 12.0f));

	// Dynamic FOV (ADS zoom + sprint/slide widen)
	float TargetFOV = DefaultFOV;
	if (WeaponComponent && WeaponComponent->GetActiveWeaponData())
	{
		TargetFOV = FMath::Lerp(DefaultFOV, WeaponComponent->GetActiveWeaponData()->ADSFieldOfView, WeaponComponent->GetADSAlpha());
	}
	if (State == ETacticalMovementState::Sliding || State == ETacticalMovementState::SupineDive)
	{
		TargetFOV += 8.0f;
	}
	FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, 12.0f));
}
