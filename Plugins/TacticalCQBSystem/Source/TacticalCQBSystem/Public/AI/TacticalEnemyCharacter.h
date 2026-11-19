#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TacticalEnemyCharacter.generated.h"

class UTacticalWeaponDataAsset;
class ATacticalWeaponPickup;

UCLASS()
class TACTICALCQBSYSTEM_API ATacticalEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATacticalEnemyCharacter();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Enemy")
	void ApplyBallisticDamage(float Damage, const FHitResult& HitResult, const FVector& ShotDirection, float RagdollImpulse, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Enemy")
	void EnterHumanShieldState(USceneComponent* HolderComponent, FName AttachSocket = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Enemy")
	void ReleaseAndKick(const FVector& KickImpulse);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Enemy")
	void ExecuteSilentTakedown();

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Enemy")
	bool IsAlive() const { return !bIsDead; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Enemy")
	bool IsHeldAsShield() const { return bIsHeldAsShield; }

protected:
	void ActivateRagdoll(const FVector& Impulse, FName BoneName);
	void DropWeaponIntoWorld();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UTacticalWeaponDataAsset> CarriedWeaponData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsHeldAsShield = false;
};
