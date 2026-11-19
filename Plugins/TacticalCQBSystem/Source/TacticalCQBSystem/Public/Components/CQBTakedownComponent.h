#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CQBTakedownComponent.generated.h"

class ATacticalEnemyCharacter;
class ATacticalFragGrenade;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHumanShieldStateChanged, bool, bIsHoldingShield);

UCLASS(ClassGroup=(TacticalCQB), meta=(BlueprintSpawnableComponent))
class TACTICALCQBSYSTEM_API UCQBTakedownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCQBTakedownComponent();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|CQB")
	bool TryGrabHumanShield(USceneComponent* CameraAnchor);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|CQB")
	bool TryExecuteShieldOrNeckSnap();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|CQB")
	bool TryFrontSpartanKick();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|CQB")
	bool TryPlantGrenadeOnShieldOrThrow(const FVector& ThrowOrigin, const FVector& ThrowDirection);

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|CQB")
	bool IsHoldingHumanShield() const;

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|CQB")
	int32 GetRemainingGrenades() const { return RemainingGrenades; }

	UPROPERTY(BlueprintAssignable, Category = "TacticalCQB|CQB")
	FOnHumanShieldStateChanged OnHumanShieldStateChanged;

protected:
	ATacticalEnemyCharacter* FindEnemyInRange(float Range) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CQB")
	float GrabRange = 175.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CQB")
	float KickRange = 190.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CQB")
	float SpartanKickImpulse = 42000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	TSubclassOf<ATacticalFragGrenade> FragGrenadeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	int32 RemainingGrenades = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	float ThrowSpeed = 1350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> KickImpactSound = nullptr;

	UPROPERTY()
	TObjectPtr<ATacticalEnemyCharacter> HeldEnemy = nullptr;
};
