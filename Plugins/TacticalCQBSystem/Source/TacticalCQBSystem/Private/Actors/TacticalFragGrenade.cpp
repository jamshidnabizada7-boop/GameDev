#include "Actors/TacticalFragGrenade.h"
#include "AI/TacticalEnemyCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"

ATacticalFragGrenade::ATacticalFragGrenade()
{
	PrimaryActorTick.bCanEverTick = false;

	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	SetRootComponent(GrenadeMesh);
	GrenadeMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.35f;
	ProjectileMovement->Friction = 0.5f;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 2400.0f;

	ArmedRedGlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ArmedRedGlowLight"));
	ArmedRedGlowLight->SetupAttachment(GrenadeMesh);
	ArmedRedGlowLight->SetLightColor(FLinearColor(1.0f, 0.06f, 0.03f));
	ArmedRedGlowLight->SetIntensity(3200.0f);
	ArmedRedGlowLight->SetAttenuationRadius(120.0f);
}

void ATacticalFragGrenade::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ATacticalFragGrenade::Detonate, FuseSeconds, false);
}

void ATacticalFragGrenade::ThrowGrenade(const FVector& Velocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Velocity;
		ProjectileMovement->Activate(true);
	}
}

void ATacticalFragGrenade::StickToTarget(USceneComponent* TargetComponent, FName SocketName)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (TargetComponent)
	{
		AttachToComponent(TargetComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	}
}

void ATacticalFragGrenade::Detonate()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Origin = GetActorLocation();

	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ExplosionVFX, Origin);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ExplosionSound, Origin);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(BlastRadius);
	if (World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Sphere))
	{
		for (const FOverlapResult& Res : Overlaps)
		{
			if (ATacticalEnemyCharacter* Enemy = Cast<ATacticalEnemyCharacter>(Res.GetActor()))
			{
				const FVector Dir = (Enemy->GetActorLocation() - Origin).GetSafeNormal() + FVector(0.f, 0.f, 0.35f);
				FHitResult DummyHit;
				DummyHit.ImpactPoint = Enemy->GetActorLocation();
				DummyHit.BoneName = TEXT("pelvis");
				Enemy->ApplyBallisticDamage(BlastDamage, DummyHit, Dir.GetSafeNormal(), BlastRagdollImpulse, GetInstigator());
			}
		}
	}

	Destroy();
}
