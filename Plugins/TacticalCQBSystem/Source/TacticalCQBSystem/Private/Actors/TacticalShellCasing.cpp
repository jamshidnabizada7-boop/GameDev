#include "Actors/TacticalShellCasing.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ATacticalShellCasing::ATacticalShellCasing()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 6.0f;

	ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
	SetRootComponent(ShellMesh);
	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	ShellMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ShellMesh->SetNotifyRigidBodyCollision(true);
	ShellMesh->OnComponentHit.AddDynamic(this, &ATacticalShellCasing::OnShellHit);
}

void ATacticalShellCasing::LaunchShell(const FVector& EjectVelocity, const FVector& AngularSpin)
{
	if (ShellMesh)
	{
		ShellMesh->AddImpulse(EjectVelocity, NAME_None, true);
		ShellMesh->AddAngularImpulseInDegrees(AngularSpin, NAME_None, true);
	}
}

void ATacticalShellCasing::OnShellHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bHasPlayedBounce && BounceSound)
	{
		bHasPlayedBounce = true;
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, Hit.ImpactPoint, 0.55f, FMath::FRandRange(0.92f, 1.08f));
	}
}
