#include "Actors/TacticalWeaponPickup.h"
#include "Components/StaticMeshComponent.h"

ATacticalWeaponPickup::ATacticalWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);
	PickupMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}
