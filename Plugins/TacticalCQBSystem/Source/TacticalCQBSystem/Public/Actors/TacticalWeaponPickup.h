#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalWeaponPickup.generated.h"

class UStaticMeshComponent;
class UTacticalWeaponDataAsset;

UCLASS()
class TACTICALCQBSYSTEM_API ATacticalWeaponPickup : public AActor
{
	GENERATED_BODY()

public:
	ATacticalWeaponPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	TObjectPtr<UTacticalWeaponDataAsset> WeaponData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 AmmoInMag = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;
};
