#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/TacticalWeaponDataAsset.h"
#include "TacticalHUDWidget.generated.h"

UCLASS()
class TACTICALCQBSYSTEM_API UTacticalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "TacticalCQB|HUD")
	void BP_OnWeaponSlotChanged(ETacticalWeaponSlot ActiveSlot, const FText& Slot1Name, const FText& Slot2Name);

	UFUNCTION(BlueprintImplementableEvent, Category = "TacticalCQB|HUD")
	void BP_OnAmmoUpdated(int32 CurrentMag, int32 ReserveAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category = "TacticalCQB|HUD")
	void BP_OnHumanShieldPromptsVisible(bool bVisible, int32 RemainingGrenades);

	UFUNCTION(BlueprintImplementableEvent, Category = "TacticalCQB|HUD")
	void BP_OnWorldPickupPrompt(bool bVisible, const FText& WeaponName);

	UFUNCTION(BlueprintImplementableEvent, Category = "TacticalCQB|HUD")
	void BP_OnBulletTimeUpdated(bool bActive, float FocusAlpha);
};
