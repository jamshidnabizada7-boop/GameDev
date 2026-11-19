#include "AI/TacticalAIController.h"
#include "AI/TacticalEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"

ATacticalAIController::ATacticalAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATacticalAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ATacticalEnemyCharacter* Enemy = Cast<ATacticalEnemyCharacter>(GetPawn());
	if (!Enemy || !Enemy->IsAlive() || Enemy->IsHeldAsShield())
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	const float Dist = FVector::Dist(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (Dist <= DetectionRange && LineOfSightTo(PlayerPawn))
	{
		SetFocus(PlayerPawn);
		if (Dist > PreferredCombatDistance)
		{
			MoveToActor(PlayerPawn, PreferredCombatDistance * 0.8f);
		}
	}
}
