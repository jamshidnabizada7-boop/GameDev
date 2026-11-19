#include "Components/CQBTakedownComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AI/TacticalEnemyCharacter.h"
#include "Actors/TacticalFragGrenade.h"
#include "Actors/TacticalBreachDoor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UCQBTakedownComponent::UCQBTakedownComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCQBTakedownComponent::IsHoldingHumanShield() const
{
	return HeldEnemy != nullptr && HeldEnemy->IsAlive();
}

ATacticalEnemyCharacter* UCQBTakedownComponent::FindEnemyInRange(float Range) const
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return nullptr;

	const FVector Start = OwnerChar->GetPawnViewLocation();
	const FVector End = Start + OwnerChar->GetControlRotation().Vector() * Range;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CQBTrace), false, OwnerChar);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(36.0f), Params))
	{
		ATacticalEnemyCharacter* Enemy = Cast<ATacticalEnemyCharacter>(Hit.GetActor());
		if (Enemy && Enemy->IsAlive())
		{
			return Enemy;
		}
	}
	return nullptr;
}

bool UCQBTakedownComponent::TryGrabHumanShield(USceneComponent* CameraAnchor)
{
	if (IsHoldingHumanShield() || !CameraAnchor) return false;

	if (ATacticalEnemyCharacter* Enemy = FindEnemyInRange(GrabRange))
	{
		HeldEnemy = Enemy;
		HeldEnemy->EnterHumanShieldState(CameraAnchor);
		OnHumanShieldStateChanged.Broadcast(true);
		return true;
	}
	return false;
}

bool UCQBTakedownComponent::TryExecuteShieldOrNeckSnap()
{
	if (IsHoldingHumanShield())
	{
		HeldEnemy->ExecuteSilentTakedown();
		HeldEnemy = nullptr;
		OnHumanShieldStateChanged.Broadcast(false);
		return true;
	}

	if (ATacticalEnemyCharacter* Enemy = FindEnemyInRange(GrabRange))
	{
		Enemy->ExecuteSilentTakedown();
		return true;
	}
	return false;
}

bool UCQBTakedownComponent::TryFrontSpartanKick()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return false;

	const FVector Forward = OwnerChar->GetControlRotation().Vector();
	const FVector KickVec = (Forward + FVector(0.f, 0.f, 0.22f)).GetSafeNormal() * SpartanKickImpulse;

	// 1. Kick held human shield away (exact match to 5.25s stairwell grenade kick)
	if (IsHoldingHumanShield())
	{
		HeldEnemy->ReleaseAndKick(KickVec);
		HeldEnemy = nullptr;
		OnHumanShieldStateChanged.Broadcast(false);
		if (KickImpactSound) UGameplayStatics::PlaySoundAtLocation(this, KickImpactSound, OwnerChar->GetActorLocation());
		return true;
	}

	// 2. Kick standing enemy or breachable door
	const FVector Start = OwnerChar->GetPawnViewLocation();
	const FVector End = Start + Forward * KickRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpartanKick), false, OwnerChar);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(32.0f), Params))
	{
		if (ATacticalBreachDoor* Door = Cast<ATacticalBreachDoor>(Hit.GetActor()))
		{
			Door->BreachOpen(OwnerChar->GetActorLocation(), true);
			return true;
		}
		if (ATacticalEnemyCharacter* Enemy = Cast<ATacticalEnemyCharacter>(Hit.GetActor()))
		{
			Enemy->ReleaseAndKick(KickVec);
			if (KickImpactSound) UGameplayStatics::PlaySoundAtLocation(this, KickImpactSound, Hit.ImpactPoint);
			return true;
		}
	}
	return false;
}

bool UCQBTakedownComponent::TryPlantGrenadeOnShieldOrThrow(const FVector& ThrowOrigin, const FVector& ThrowDirection)
{
	if (RemainingGrenades <= 0 || !FragGrenadeClass) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	ATacticalFragGrenade* Grenade = GetWorld()->SpawnActor<ATacticalFragGrenade>(
		FragGrenadeClass, ThrowOrigin, ThrowDirection.Rotation(), SpawnParams);
	if (!Grenade) return false;

	RemainingGrenades--;

	// Stick live grenade directly onto human shield (4.75s)
	if (IsHoldingHumanShield())
	{
		Grenade->StickToTarget(Cast<USceneComponent>(HeldEnemy->GetMesh()), TEXT("spine_03"));
		return true;
	}

	// Otherwise throw overhead down corridor (3.25s)
	Grenade->ThrowGrenade(ThrowDirection.GetSafeNormal() * ThrowSpeed + FVector(0.f, 0.f, 180.f));
	return true;
}
