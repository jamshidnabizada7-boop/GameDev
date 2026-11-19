#include "Components/TacticalMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

UTacticalMovementComponent::UTacticalMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
	MaxWalkSpeed = 440.0f;
	MaxWalkSpeedCrouched = 240.0f;
	SetCrouchedHalfHeight(48.0f);
	BrakingDecelerationWalking = 1800.0f;
}

float UTacticalMovementComponent::GetMaxSpeed() const
{
	if (TacticalState == ETacticalMovementState::Sprinting)
	{
		return SprintSpeed;
	}
	if (TacticalState == ETacticalMovementState::Sliding)
	{
		return SlideInitialSpeed;
	}
	if (TacticalState == ETacticalMovementState::SupineDive)
	{
		return SupineDiveImpulse;
	}
	return Super::GetMaxSpeed();
}

void UTacticalMovementComponent::SetSprinting(bool bNewSprinting)
{
	bWantsSprint = bNewSprinting;
	if (TacticalState == ETacticalMovementState::Normal && bWantsSprint)
	{
		TacticalState = ETacticalMovementState::Sprinting;
	}
	else if (TacticalState == ETacticalMovementState::Sprinting && !bWantsSprint)
	{
		TacticalState = ETacticalMovementState::Normal;
	}
}

void UTacticalMovementComponent::SetLeanInput(float LeanValue)
{
	TargetLeanAlpha = FMath::Clamp(LeanValue, -1.0f, 1.0f);
}

bool UTacticalMovementComponent::TryStartCombatSlide()
{
	if (!IsMovingOnGround() || Velocity.Size2D() < 360.0f || TacticalState == ETacticalMovementState::Sliding)
	{
		return false;
	}

	TacticalState = ETacticalMovementState::Sliding;
	ManeuverTimer = SlideDuration;
	SlideDirection = Velocity.GetSafeNormal2D();
	Velocity = SlideDirection * SlideInitialSpeed;
	bWantsToCrouch = true;
	return true;
}

bool UTacticalMovementComponent::TryStartSupineDive(const FVector& DiveDirection)
{
	if (TacticalState == ETacticalMovementState::SupineDive || TacticalState == ETacticalMovementState::Vaulting)
	{
		return false;
	}

	TacticalState = ETacticalMovementState::SupineDive;
	ManeuverTimer = SupineDiveDuration;
	const FVector Dir = DiveDirection.IsNearlyZero() ? -CharacterOwner->GetActorForwardVector() : DiveDirection.GetSafeNormal2D();
	Launch(Dir * SupineDiveImpulse + FVector(0.0f, 0.0f, 220.0f));
	bWantsToCrouch = true;
	return true;
}

bool UTacticalMovementComponent::TryVaultOverObstacle()
{
	if (!CharacterOwner || TacticalState == ETacticalMovementState::Vaulting)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector Origin = CharacterOwner->GetActorLocation();
	const FVector Forward = CharacterOwner->GetActorForwardVector();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalVaultTrace), false, CharacterOwner);

	// 1. Forward chest/waist hit detection
	FHitResult FrontHit;
	const FVector FrontStart = Origin - FVector(0.f, 0.f, 20.f);
	const FVector FrontEnd = FrontStart + Forward * 105.0f;
	if (!World->LineTraceSingleByChannel(FrontHit, FrontStart, FrontEnd, ECC_Visibility, Params))
	{
		return false;
	}

	// 2. Downward top-edge detection to verify obstacle height is within vaultable bounds
	FHitResult TopHit;
	const FVector TopStart = FrontHit.ImpactPoint + Forward * 38.0f + FVector(0.f, 0.f, MaxVaultHeight);
	const FVector TopEnd = FrontHit.ImpactPoint + Forward * 38.0f - FVector(0.f, 0.f, 20.0f);
	if (!World->LineTraceSingleByChannel(TopHit, TopStart, TopEnd, ECC_Visibility, Params))
	{
		return false;
	}

	const float FeetZ = Origin.Z - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float ObstacleHeight = TopHit.ImpactPoint.Z - FeetZ;
	if (ObstacleHeight < MinVaultHeight || ObstacleHeight > MaxVaultHeight)
	{
		return false;
	}

	// 3. Landing point behind obstacle
	VaultStartLocation = Origin;
	VaultTargetLocation = TopHit.ImpactPoint + Forward * 95.0f + FVector(0.f, 0.f, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 6.0f);

	TacticalState = ETacticalMovementState::Vaulting;
	ManeuverTimer = VaultDuration;
	SetMovementMode(MOVE_Flying);
	return true;
}

void UTacticalMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	CurrentLeanAlpha = FMath::FInterpTo(CurrentLeanAlpha, TargetLeanAlpha, DeltaTime, 11.0f);
	UpdateActiveManeuver(DeltaTime);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWallBraceIK();
}

void UTacticalMovementComponent::UpdateActiveManeuver(float DeltaTime)
{
	if (!CharacterOwner) return;

	if (TacticalState == ETacticalMovementState::Sliding)
	{
		ManeuverTimer -= DeltaTime;
		const float Alpha = FMath::Clamp(ManeuverTimer / SlideDuration, 0.0f, 1.0f);
		Velocity = SlideDirection * FMath::Lerp(220.0f, SlideInitialSpeed, Alpha);
		if (ManeuverTimer <= 0.0f)
		{
			TacticalState = bWantsSprint ? ETacticalMovementState::Sprinting : ETacticalMovementState::Normal;
			bWantsToCrouch = false;
		}
	}
	else if (TacticalState == ETacticalMovementState::SupineDive)
	{
		ManeuverTimer -= DeltaTime;
		if (ManeuverTimer <= 0.0f)
		{
			TacticalState = ETacticalMovementState::Normal;
			bWantsToCrouch = false;
		}
	}
	else if (TacticalState == ETacticalMovementState::Vaulting)
	{
		ManeuverTimer -= DeltaTime;
		const float Progress = 1.0f - FMath::Clamp(ManeuverTimer / VaultDuration, 0.0f, 1.0f);
		FVector NewLoc = FMath::Lerp(VaultStartLocation, VaultTargetLocation, Progress);
		NewLoc.Z += FMath::Sin(Progress * PI) * 28.0f; // smooth arc over cover
		CharacterOwner->SetActorLocation(NewLoc, true);

		if (ManeuverTimer <= 0.0f)
		{
			TacticalState = bWantsSprint ? ETacticalMovementState::Sprinting : ETacticalMovementState::Normal;
			SetMovementMode(MOVE_Walking);
		}
	}
}

void UTacticalMovementComponent::UpdateWallBraceIK()
{
	WallBraceInfo.bIsBracing = false;
	if (!CharacterOwner || TacticalState == ETacticalMovementState::Vaulting || TacticalState == ETacticalMovementState::SupineDive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector EyeLoc = CharacterOwner->GetPawnViewLocation();
	const FVector RightVec = CharacterOwner->GetActorRightVector();
	const FVector FwdVec = CharacterOwner->GetActorForwardVector();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WallBraceIK), false, CharacterOwner);

	// Check right wall first (0.25s–0.75s), then left wall (3.00s, 10.00s)
	FHitResult Hit;
	const FVector RightEnd = EyeLoc + (RightVec * 0.85f + FwdVec * 0.45f).GetSafeNormal() * WallBraceTraceDistance;
	if (World->LineTraceSingleByChannel(Hit, EyeLoc, RightEnd, ECC_Visibility, Params))
	{
		WallBraceInfo.bIsBracing = true;
		WallBraceInfo.bIsRightSide = true;
		WallBraceInfo.HandTargetWorldLocation = Hit.ImpactPoint + Hit.ImpactNormal * 4.0f;
		WallBraceInfo.WallNormal = Hit.ImpactNormal;
		return;
	}

	const FVector LeftEnd = EyeLoc + (-RightVec * 0.85f + FwdVec * 0.45f).GetSafeNormal() * WallBraceTraceDistance;
	if (World->LineTraceSingleByChannel(Hit, EyeLoc, LeftEnd, ECC_Visibility, Params))
	{
		WallBraceInfo.bIsBracing = true;
		WallBraceInfo.bIsRightSide = false;
		WallBraceInfo.HandTargetWorldLocation = Hit.ImpactPoint + Hit.ImpactNormal * 4.0f;
		WallBraceInfo.WallNormal = Hit.ImpactNormal;
	}
}
