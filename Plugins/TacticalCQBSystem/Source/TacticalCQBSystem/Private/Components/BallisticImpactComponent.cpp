#include "Components/BallisticImpactComponent.h"
#include "AI/TacticalEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"

UBallisticImpactComponent::UBallisticImpactComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBallisticImpactComponent::HandleBallisticHit(const FHitResult& HitResult, const FVector& ShotDirection, float Damage, float RagdollImpulse)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return;
	}

	if (ATacticalEnemyCharacter* Enemy = Cast<ATacticalEnemyCharacter>(HitActor))
	{
		SpawnBloodSplatter(HitResult, ShotDirection);
		Enemy->ApplyBallisticDamage(Damage, HitResult, ShotDirection, RagdollImpulse, GetOwner());
	}
	else
	{
		SpawnSurfaceBulletHole(HitResult);
	}
}

void UBallisticImpactComponent::SpawnBloodSplatter(const FHitResult& HitResult, const FVector& ShotDirection, float ExitTraceDistance)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (BloodBurstVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, BloodBurstVFX, HitResult.ImpactPoint, ShotDirection.Rotation());
	}

	if (FleshImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, FleshImpactSound, HitResult.ImpactPoint);
	}

	if (BloodDecalMaterials.Num() == 0) return;

	// Exit-wound projection onto wall or floor behind enemy (exact match to 0.75s, 9.00s, 21.00s)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BloodExitTrace), true);
	Params.AddIgnoredActor(HitResult.GetActor());
	Params.AddIgnoredActor(GetOwner());

	const FVector TraceStart = HitResult.ImpactPoint + ShotDirection * 15.0f;
	const FVector TraceEnd = TraceStart + (ShotDirection + FVector(0.f, 0.f, -0.18f)).GetSafeNormal() * ExitTraceDistance;

	FHitResult WallHit;
	if (World->LineTraceSingleByChannel(WallHit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		UMaterialInterface* DecalMat = BloodDecalMaterials[FMath::RandRange(0, BloodDecalMaterials.Num() - 1)];
		if (DecalMat)
		{
			const float Size = FMath::FRandRange(BloodDecalSizeRange.X, BloodDecalSizeRange.Y);
			FRotator DecalRot = (-WallHit.ImpactNormal).Rotation();
			DecalRot.Roll = FMath::FRandRange(0.0f, 360.0f);

			UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
				World, DecalMat, FVector(16.0f, Size, Size), WallHit.ImpactPoint, DecalRot, DecalLifeSpan);
			if (Decal)
			{
				Decal->SetFadeScreenSize(0.001f);
			}
		}
	}
}

void UBallisticImpactComponent::SpawnSurfaceBulletHole(const FHitResult& HitResult)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (ConcreteSparkVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ConcreteSparkVFX, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation());
	}

	if (ConcreteImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ConcreteImpactSound, HitResult.ImpactPoint);
	}

	if (BulletHoleDecalMaterial)
	{
		FRotator DecalRot = (-HitResult.ImpactNormal).Rotation();
		DecalRot.Roll = FMath::FRandRange(0.0f, 360.0f);
		UGameplayStatics::SpawnDecalAtLocation(
			World, BulletHoleDecalMaterial, FVector(4.0f, 7.0f, 7.0f), HitResult.ImpactPoint, DecalRot, DecalLifeSpan);
	}
}
