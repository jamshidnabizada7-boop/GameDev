#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalFragGrenade.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class TACTICALCQBSYSTEM_API ATacticalFragGrenade : public AActor
{
	GENERATED_BODY()

public:
	ATacticalFragGrenade();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Grenade")
	void ThrowGrenade(const FVector& Velocity);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Grenade")
	void StickToTarget(USceneComponent* TargetComponent, FName SocketName = TEXT("spine_03"));

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Grenade")
	void Detonate();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GrenadeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> ArmedRedGlowLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float FuseSeconds = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float BlastRadius = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float BlastDamage = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float BlastRagdollImpulse = 48000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX & Audio")
	TObjectPtr<UNiagaraSystem> ExplosionVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX & Audio")
	TObjectPtr<USoundBase> ExplosionSound = nullptr;

	FTimerHandle FuseTimerHandle;
};
