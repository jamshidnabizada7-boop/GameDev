#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TacticalWeaponDataAsset.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;
class ATacticalShellCasing;
class ATacticalWeaponPickup;

UENUM(BlueprintType)
enum class ETacticalWeaponSlot : uint8
{
	Primary      UMETA(DisplayName = "Slot 1 - Primary (MP7 / Cruiser)"),
	Sidearm      UMETA(DisplayName = "Slot 2 - Sidearm (1911)"),
	Melee        UMETA(DisplayName = "Slot 3 - Melee (Karambit)"),
	DualWield    UMETA(DisplayName = "Slot 4 - Dual Wield")
};

UENUM(BlueprintType)
enum class ETacticalFireMode : uint8
{
	FullAuto     UMETA(DisplayName = "Full Auto"),
	SemiAuto     UMETA(DisplayName = "Semi Auto"),
	PumpShotgun  UMETA(DisplayName = "Pump Shotgun"),
	MeleeSlash   UMETA(DisplayName = "Melee")
};

UCLASS(BlueprintType)
class TACTICALCQBSYSTEM_API UTacticalWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName WeaponID = TEXT("MP7");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName = FText::FromString(TEXT("MP7"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	ETacticalWeaponSlot Slot = ETacticalWeaponSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	ETacticalFireMode FireMode = ETacticalFireMode::FullAuto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USkeletalMesh> WeaponMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FName ShellEjectSocketName = TEXT("AmmoEject");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSubclassOf<ATacticalShellCasing> ShellCasingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSubclassOf<ATacticalWeaponPickup> WorldPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float BaseDamage = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float HeadshotMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float FireRateRPM = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	int32 PelletsPerShot = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float BaseSpreadDegrees = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float ADSSpreadMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float MaxRange = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float RagdollDeathImpulse = 18000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 MagazineCapacity = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 StartingReserveAmmo = 120;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	float ReloadDuration = 1.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachments")
	bool bSupportsSuppressor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachments")
	bool bStartsSuppressed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachments")
	float SuppressorAttachDuration = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Rig")
	FVector HipOffset = FVector(18.0f, 10.0f, -12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Rig")
	FVector ADSOffset = FVector(14.0f, 0.0f, -6.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Rig")
	FVector ShieldCantedOffset = FVector(16.0f, 7.5f, -7.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Rig")
	FRotator ShieldCantedRotation = FRotator(0.0f, 0.0f, -28.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Rig")
	float ADSFieldOfView = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float VerticalRecoilPitch = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float HorizontalRecoilYaw = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	float ProceduralKickbackCm = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio & VFX")
	TObjectPtr<USoundBase> FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio & VFX")
	TObjectPtr<USoundBase> SuppressedFireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio & VFX")
	TObjectPtr<USoundBase> ReloadSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio & VFX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimMontage> FireMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimMontage> EquipMontage = nullptr;
};
