#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TacticalAIController.generated.h"

UCLASS()
class TACTICALCQBSYSTEM_API ATacticalAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATacticalAIController();

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PreferredCombatDistance = 520.0f;
};
