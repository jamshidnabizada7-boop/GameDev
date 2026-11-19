#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BallisticImpactComponent.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

UCLASS(ClassGroup=(TacticalCQB), meta=(BlueprintSpawnableComponent))
class TACTICALCQBSYSTEM_API UBallisticImpactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBallisticImpactComponent();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Impacts")
	void HandleBallisticHit(const FHitResult& HitResult, const FVector& ShotDirection, float Damage, float RagdollImpulse);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Impacts")
	void SpawnBloodSplatter(const FHitResult& HitResult, const FVector& ShotDirection, float ExitTraceDistance = 320.0f);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Impacts")
	void SpawnSurfaceBulletHole(const FHitResult& HitResult);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Blood")
	TObjectPtr<UNiagaraSystem> BloodBurstVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Blood")
	TArray<TObjectPtr<UMaterialInterface>> BloodDecalMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Blood")
	FVector2D BloodDecalSizeRange = FVector2D(38.0f, 82.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Surface")
	TObjectPtr<UNiagaraSystem> ConcreteSparkVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Surface")
	TObjectPtr<UMaterialInterface> BulletHoleDecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Surface")
	float DecalLifeSpan = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> FleshImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> ConcreteImpactSound = nullptr;
};
