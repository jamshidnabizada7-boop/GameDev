#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/TacticalWeaponDataAsset.h"
#include "TacticalWeaponComponent.generated.h"

class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FTacticalWeaponInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UTacticalWeaponDataAsset> Data = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 ReserveAmmo = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bSuppressorAttached = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSlotChanged, ETacticalWeaponSlot, NewSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoUpdated, int32, CurrentMag, int32, Reserve);

UCLASS(ClassGroup=(TacticalCQB), meta=(BlueprintSpawnableComponent))
class TACTICALCQBSYSTEM_API UTacticalWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTacticalWeaponComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void InitializeWeaponMesh(USkeletalMeshComponent* InWeaponMesh);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void EquipSlot(ETacticalWeaponSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void PickupAndReplaceWeapon(UTacticalWeaponDataAsset* NewWeaponData, int32 OverrideAmmo = -1);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void SetAiming(bool bNewAiming);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void StartReload();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Weapon")
	void ToggleSuppressor();

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Weapon")
	UTacticalWeaponDataAsset* GetActiveWeaponData() const;

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Weapon")
	ETacticalWeaponSlot GetActiveSlot() const { return ActiveSlot; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Weapon")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Weapon")
	float GetADSAlpha() const { return ADSAlpha; }

	UPROPERTY(BlueprintAssignable, Category = "TacticalCQB|Weapon")
	FOnWeaponSlotChanged OnWeaponSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "TacticalCQB|Weapon")
	FOnAmmoUpdated OnAmmoUpdated;

protected:
	void FireSingleShot();
	void SpawnShellCasing();
	void FinishReload();
	void UpdateProceduralWeaponRig(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	TMap<ETacticalWeaponSlot, TObjectPtr<UTacticalWeaponDataAsset>> DefaultLoadout;

	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	TMap<ETacticalWeaponSlot, FTacticalWeaponInstance> LoadoutInstances;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComp = nullptr;

	ETacticalWeaponSlot ActiveSlot = ETacticalWeaponSlot::Primary;
	bool bIsFiring = false;
	bool bIsAiming = false;
	bool bIsReloading = false;
	float ADSAlpha = 0.0f;
	float RecoilKickOffset = 0.0f;
	FTimerHandle AutoFireTimer;
	FTimerHandle ReloadTimer;
};
