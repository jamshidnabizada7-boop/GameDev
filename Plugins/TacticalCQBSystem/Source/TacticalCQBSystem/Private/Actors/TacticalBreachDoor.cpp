#include "Actors/TacticalBreachDoor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/SceneComponent.h"
#include "Components/TacticalTimeDilationComponent.h"
#include "Kismet/GameplayStatics.h"

ATacticalBreachDoor::ATacticalBreachDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	HingePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivot"));
	SetRootComponent(HingePivot);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(HingePivot);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void ATacticalBreachDoor::BreachOpen(const FVector& BreacherLocation, bool bKickBreach)
{
	if (bIsOpen) return;
	bIsOpen = true;

	const FVector ToBreacher = (BreacherLocation - GetActorLocation()).GetSafeNormal();
	const float Side = FVector::DotProduct(GetActorForwardVector(), ToBreacher);
	TargetYaw = (Side >= 0.0f) ? -OpenAngleDegrees : OpenAngleDegrees;

	if (BreachSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreachSound, GetActorLocation());
	}

	if (bTriggersBulletTimeOnBreach)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (UTacticalTimeDilationComponent* TimeComp = PlayerPawn->FindComponentByClass<UTacticalTimeDilationComponent>())
			{
				TimeComp->TriggerTimedBulletTime(BreachBulletTimeDuration);
			}
		}
	}
}

void ATacticalBreachDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsOpen && !FMath::IsNearlyEqual(CurrentYaw, TargetYaw, 0.25f))
	{
		CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, BreachSwingSpeed);
		HingePivot->SetRelativeRotation(FRotator(0.0f, CurrentYaw, 0.0f));
	}
}
