#include "Components/TacticalTimeDilationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UTacticalTimeDilationComponent::UTacticalTimeDilationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTacticalTimeDilationComponent::ToggleBulletTime()
{
	SetBulletTimeActive(!bBulletTimeActive);
}

void UTacticalTimeDilationComponent::TriggerTimedBulletTime(float DurationSeconds)
{
	TimedOverrideRemaining = FMath::Max(TimedOverrideRemaining, DurationSeconds);
	SetBulletTimeActive(true);
}

void UTacticalTimeDilationComponent::SetBulletTimeActive(bool bActive)
{
	if (bActive && CurrentFocus < 5.0f && TimedOverrideRemaining <= 0.0f)
	{
		return;
	}

	if (bBulletTimeActive != bActive)
	{
		bBulletTimeActive = bActive;
		OnBulletTimeChanged.Broadcast(bBulletTimeActive);
	}
}

void UTacticalTimeDilationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World) return;

	// Compute unscaled real-time delta so drain/interp speed feels identical during slow-mo
	const float CurrentWorldDilation = FMath::Max(0.01f, UGameplayStatics::GetGlobalTimeDilation(World));
	const float RealDeltaTime = DeltaTime / CurrentWorldDilation;

	if (TimedOverrideRemaining > 0.0f)
	{
		TimedOverrideRemaining -= RealDeltaTime;
		if (TimedOverrideRemaining <= 0.0f && bBulletTimeActive)
		{
			SetBulletTimeActive(false);
		}
	}

	if (bBulletTimeActive)
	{
		if (TimedOverrideRemaining <= 0.0f)
		{
			CurrentFocus = FMath::Max(0.0f, CurrentFocus - FocusDrainPerRealSecond * RealDeltaTime);
			if (CurrentFocus <= 0.0f)
			{
				SetBulletTimeActive(false);
			}
		}
	}
	else
	{
		CurrentFocus = FMath::Min(MaxFocus, CurrentFocus + FocusRegenPerSecond * RealDeltaTime);
	}

	const float TargetScale = bBulletTimeActive ? SlowMoTimeScale : 1.0f;
	if (!FMath::IsNearlyEqual(CurrentTimeScale, TargetScale, 0.002f))
	{
		CurrentTimeScale = FMath::FInterpTo(CurrentTimeScale, TargetScale, RealDeltaTime, TransitionInterpSpeed);
		UGameplayStatics::SetGlobalTimeDilation(World, CurrentTimeScale);
	}
}
