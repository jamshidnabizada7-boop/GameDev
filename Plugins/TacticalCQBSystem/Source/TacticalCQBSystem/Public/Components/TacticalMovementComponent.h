#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TacticalMovementComponent.generated.h"

UENUM(BlueprintType)
enum class ETacticalMovementState : uint8
{
	Normal       UMETA(DisplayName = "Normal"),
	Sprinting    UMETA(DisplayName = "Sprinting"),
	Sliding      UMETA(DisplayName = "Combat Slide"),
	SupineDive   UMETA(DisplayName = "Max Payne Supine Dive"),
	Vaulting     UMETA(DisplayName = "Parkour Vault")
};

USTRUCT(BlueprintType)
struct FWallBraceInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WallBrace")
	bool bIsBracing = false;

	UPROPERTY(BlueprintReadOnly, Category = "WallBrace")
	bool bIsRightSide = false;

	UPROPERTY(BlueprintReadOnly, Category = "WallBrace")
	FVector HandTargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "WallBrace")
	FVector WallNormal = FVector::ZeroVector;
};

UCLASS()
class TACTICALCQBSYSTEM_API UTacticalMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UTacticalMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetMaxSpeed() const override;

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Movement")
	void SetSprinting(bool bNewSprinting);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Movement")
	bool TryStartCombatSlide();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Movement")
	bool TryStartSupineDive(const FVector& DiveDirection);

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Movement")
	bool TryVaultOverObstacle();

	UFUNCTION(BlueprintCallable, Category = "TacticalCQB|Movement")
	void SetLeanInput(float LeanValue);

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Movement")
	ETacticalMovementState GetTacticalState() const { return TacticalState; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Movement")
	FWallBraceInfo GetWallBraceInfo() const { return WallBraceInfo; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Movement")
	float GetCurrentLeanAlpha() const { return CurrentLeanAlpha; }

	UFUNCTION(BlueprintPure, Category = "TacticalCQB|Movement")
	bool IsShowingLowerBodyLegs() const
	{
		return TacticalState == ETacticalMovementState::Sliding ||
			   TacticalState == ETacticalMovementState::SupineDive ||
			   TacticalState == ETacticalMovementState::Vaulting;
	}

protected:
	void UpdateWallBraceIK();
	void UpdateActiveManeuver(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float SprintSpeed = 680.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float SlideInitialSpeed = 920.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float SlideDuration = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SupineDive")
	float SupineDiveImpulse = 640.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SupineDive")
	float SupineDiveDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float MaxVaultHeight = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float MinVaultHeight = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaultDuration = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallBrace")
	float WallBraceTraceDistance = 78.0f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ETacticalMovementState TacticalState = ETacticalMovementState::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FWallBraceInfo WallBraceInfo;

	bool bWantsSprint = false;
	float TargetLeanAlpha = 0.0f;
	float CurrentLeanAlpha = 0.0f;
	float ManeuverTimer = 0.0f;
	FVector VaultStartLocation = FVector::ZeroVector;
	FVector VaultTargetLocation = FVector::ZeroVector;
	FVector SlideDirection = FVector::ForwardVector;
};
