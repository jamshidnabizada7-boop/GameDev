#pragma once
#include "CoreMinimal.h"
// Compatibility boundary for the existing Blueprint health, weapon and HUD.
namespace FoundationBridge
{
    bool LegacyInteract(AActor* Target,AActor* Interactor,bool Execute);
    double Number(const UObject* Object,FName Name,double Fallback=0);
    bool SetNumber(UObject* Object,FName Name,double Value);
    UObject* Object(UObject* Owner,FName Name);
    bool Call(UObject* Object,FName Function,const TArray<double>& Values={});
    UActorComponent* Health(AActor* Actor);
    AActor* Weapon(AActor* Actor);
    bool Heal(AActor* Actor,float Amount);
    bool AddAmmo(AActor* Actor,int32 Amount);
    void RefreshHUD(AActor* Actor);
}
