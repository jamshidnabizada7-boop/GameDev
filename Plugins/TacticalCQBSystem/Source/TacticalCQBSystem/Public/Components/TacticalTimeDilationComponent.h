#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalTimeDilationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBulletTimeChanged, bool, bIsActive);

UCLASS(ClassGroup=(TacticalCQB), meta=(BlueprintSpawnableComponent))
class TACTICALCQBSYSTEM_API UTacticalTimeDilationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTacticalTimeDilationComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|BulletTime")
	void ToggleBulletTime();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|BulletTime")
	void TriggerTimedBulletTime(float DurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|BulletTime")
	void SetBulletTimeActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|BulletTime")
	bool IsBulletTimeActive() const { return bBulletTimeActive; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|BulletTime")
	float GetFocusAlpha() const { return MaxFocus > 0.f ? CurrentFocus / MaxFocus : 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "TacticalCQB|BulletTime")
	FOnBulletTimeChanged OnBulletTimeChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletTime")
	float SlowMoTimeScale = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletTime")
	float TransitionInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletTime")
	float MaxFocus = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletTime")
	float FocusDrainPerRealSecond = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletTime")
	float FocusRegenPerSecond = 12.0f;

	float CurrentFocus = 100.0f;
	float CurrentTimeScale = 1.0f;
	float TimedOverrideRemaining = 0.0f;
	bool bBulletTimeActive = false;
};
