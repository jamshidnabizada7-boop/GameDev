#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalShellCasing.generated.h"

class UStaticMeshComponent;
class USoundBase;

UCLASS()
class TACTICALCQBSYSTEM_API ATacticalShellCasing : public AActor
{
	GENERATED_BODY()

public:
	ATacticalShellCasing();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Shell")
	void LaunchShell(const FVector& EjectVelocity, const FVector& AngularSpin);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShellMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> BounceSound = nullptr;

	UFUNCTION()
	void OnShellHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool bHasPlayedBounce = false;
};
