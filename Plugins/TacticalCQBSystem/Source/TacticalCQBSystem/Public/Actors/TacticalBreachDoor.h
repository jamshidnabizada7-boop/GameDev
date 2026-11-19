#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalBreachDoor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USoundBase;

UCLASS()
class TACTICALCQBSYSTEM_API ATacticalBreachDoor : public AActor
{
	GENERATED_BODY()

public:
	ATacticalBreachDoor();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Door")
	void BreachOpen(const FVector& BreacherLocation, bool bKickBreach = false);

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Door")
	bool IsBreached() const { return bIsOpen; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> HingePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float OpenAngleDegrees = 108.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float BreachSwingSpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bTriggersBulletTimeOnBreach = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float BreachBulletTimeDuration = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> BreachSound = nullptr;

	bool bIsOpen = false;
	float TargetYaw = 0.0f;
	float CurrentYaw = 0.0f;
};
