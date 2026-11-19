#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Phase2AnimInstance.generated.h"
class UAnimSequence;
UCLASS(Transient)
class AFGHANISTANFPS_API UPhase2AnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPhase2AnimInstance();
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Sequences;
};
