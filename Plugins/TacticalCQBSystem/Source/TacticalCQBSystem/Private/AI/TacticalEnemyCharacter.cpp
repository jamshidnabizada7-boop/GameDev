#include "AI/TacticalEnemyCharacter.h"
#include "Actors/TacticalWeaponPickup.h"
#include "Data/TacticalWeaponDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

ATacticalEnemyCharacter::ATacticalEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentHealth = MaxHealth;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	GetCharacterMovement()->MaxWalkSpeed = 340.0f;
}

void ATacticalEnemyCharacter::ApplyBallisticDamage(float Damage, const FHitResult& HitResult, const FVector& ShotDirection, float RagdollImpulse, AActor* DamageCauser)
{
	if (bIsDead)
	{
		if (GetMesh()->IsSimulatingPhysics())
		{
			GetMesh()->AddImpulseAtLocation(ShotDirection * (RagdollImpulse * 0.45f), HitResult.ImpactPoint, HitResult.BoneName);
		}
		return;
	}

	const bool bHeadshot = HitResult.BoneName == TEXT("head") || HitResult.BoneName == TEXT("neck_01");
	CurrentHealth -= bHeadshot ? Damage * 2.5f : Damage;

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		bIsHeldAsShield = false;
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		DropWeaponIntoWorld();
		ActivateRagdoll(ShotDirection * RagdollImpulse, HitResult.BoneName);
	}
}

void ATacticalEnemyCharacter::EnterHumanShieldState(USceneComponent* HolderComponent, FName AttachSocket)
{
	if (bIsDead || !HolderComponent) return;

	bIsHeldAsShield = true;
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	AttachToComponent(HolderComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
	SetActorRelativeLocation(FVector(58.0f, -14.0f, -68.0f));
	SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
}

void ATacticalEnemyCharacter::ReleaseAndKick(const FVector& KickImpulse)
{
	bIsHeldAsShield = false;
	bIsDead = true;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	DropWeaponIntoWorld();
	ActivateRagdoll(KickImpulse, TEXT("spine_03"));
}

void ATacticalEnemyCharacter::ExecuteSilentTakedown()
{
	bIsHeldAsShield = false;
	bIsDead = true;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	DropWeaponIntoWorld();
	ActivateRagdoll(GetActorForwardVector() * 4500.0f, TEXT("head"));
}

void ATacticalEnemyCharacter::ActivateRagdoll(const FVector& Impulse, FName BoneName)
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		SkelMesh->SetSimulatePhysics(true);
		SkelMesh->WakeAllRigidBodies();
		SkelMesh->AddImpulse(Impulse, BoneName, false);
	}
}

void ATacticalEnemyCharacter::DropWeaponIntoWorld()
{
	if (!CarriedWeaponData || !CarriedWeaponData->WorldPickupClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<ATacticalWeaponPickup>(
		CarriedWeaponData->WorldPickupClass,
		GetActorLocation() + GetActorRightVector() * 25.0f,
		GetActorRotation(),
		SpawnParams);
}
