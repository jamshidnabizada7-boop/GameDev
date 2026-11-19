#include "FoundationBridge.h"
#include "Components/ActorComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

bool FoundationBridge::LegacyInteract(AActor* Target,AActor* Interactor,bool Execute)
{
    if(!Target || !Interactor) return false;
    UClass* Contract=LoadObject<UClass>(nullptr,TEXT("/Game/AfghanFPS/Interaction/Interfaces/BPI_Interactable.BPI_Interactable_C"));
    if(!Contract || !Target->GetClass()->ImplementsInterface(Contract)) return false;
    UFunction* Function=Target->FindFunction(TEXT("Interact"));if(!Function) return false;
    auto* Parameter=FindFProperty<FObjectPropertyBase>(Function,TEXT("Interactor"));
    if(!Parameter || !Parameter->HasAnyPropertyFlags(CPF_Parm) || !Interactor->IsA(Parameter->PropertyClass)) return false;
    if(Execute)
    {
        FStructOnScope Args(Function);
        Parameter->SetObjectPropertyValue_InContainer(Args.GetStructMemory(),Interactor);
        Target->ProcessEvent(Function,Args.GetStructMemory());
    }
    return true;
}

double FoundationBridge::Number(const UObject* O,FName N,double Fallback)
{
    if(!O) return Fallback;
    if(const auto* P=FindFProperty<FNumericProperty>(O->GetClass(),N))
    {
        const void* V=P->ContainerPtrToValuePtr<void>(O);
        return P->IsFloatingPoint()?P->GetFloatingPointPropertyValue(V):double(P->GetSignedIntPropertyValue(V));
    }
    return Fallback;
}
bool FoundationBridge::SetNumber(UObject* O,FName N,double V)
{
    if(!O) return false;
    if(auto* P=FindFProperty<FNumericProperty>(O->GetClass(),N))
    {
        void* Address=P->ContainerPtrToValuePtr<void>(O);
        if(P->IsFloatingPoint()) P->SetFloatingPointPropertyValue(Address,V);
        else P->SetIntPropertyValue(Address,int64(V));
        return true;
    }
    return false;
}
UObject* FoundationBridge::Object(UObject* O,FName N)
{
    if(O) if(auto* P=FindFProperty<FObjectPropertyBase>(O->GetClass(),N)) return P->GetObjectPropertyValue_InContainer(O);
    return nullptr;
}
bool FoundationBridge::Call(UObject* O,FName N,const TArray<double>& Values)
{
    if(!O) return false;
    UFunction* F=O->FindFunction(N); if(!F) return false;
    FStructOnScope Args(F); int32 Index=0;
    for(TFieldIterator<FProperty> It(F);It;++It)
    {
        FProperty* P=*It;
        if(!P->HasAnyPropertyFlags(CPF_Parm) || P->HasAnyPropertyFlags(CPF_ReturnParm)) continue;
        auto* Numeric=CastField<FNumericProperty>(P);
        if(!Numeric || !Values.IsValidIndex(Index)) return false;
        void* V=P->ContainerPtrToValuePtr<void>(Args.GetStructMemory());
        if(Numeric->IsFloatingPoint()) Numeric->SetFloatingPointPropertyValue(V,Values[Index]);
        else Numeric->SetIntPropertyValue(V,int64(Values[Index]));
        ++Index;
    }
    O->ProcessEvent(F,Args.GetStructMemory()); return true;
}
UActorComponent* FoundationBridge::Health(AActor* A)
{
    if(!A) return nullptr;
    TInlineComponentArray<UActorComponent*> Components(A);
    for(auto* C:Components) if(C->GetClass()->GetName().Contains(TEXT("BPC_Health"))) return C;
    return nullptr;
}
AActor* FoundationBridge::Weapon(AActor* A)
{
    if(!A) return nullptr;
    TInlineComponentArray<UChildActorComponent*> Components(A);
    for(auto* C:Components) if(auto* Child=C->GetChildActor())
        if(FindFProperty<FNumericProperty>(Child->GetClass(),TEXT("CurrentAmmo"))) return Child;
    return nullptr;
}
void FoundationBridge::RefreshHUD(AActor* A)
{
    UObject* HUD=Object(A,TEXT("AmmoHUD"));
    auto* H=Health(A);auto* W=Weapon(A);
    Call(HUD,TEXT("UpdateHealth"),{Number(H,TEXT("CurrentHealth")),Number(H,TEXT("MaxHealth"),100)});
    Call(HUD,TEXT("UpdateAmmo"),{Number(W,TEXT("CurrentAmmo")),Number(W,TEXT("ReserveAmmo"))});
}
bool FoundationBridge::Heal(AActor* A,float Amount)
{
    auto* H=Health(A);if(!H || Amount<=0) return false;
    const double Before=Number(H,TEXT("CurrentHealth"));
    const double After=FMath::Min(Before+Amount,Number(H,TEXT("MaxHealth"),100));
    if(Before<=0 || After<=Before) return false;
    SetNumber(H,TEXT("CurrentHealth"),After);
    if(auto* P=FindFProperty<FMulticastDelegateProperty>(H->GetClass(),TEXT("OnHealthChanged")))
    {
        FStructOnScope Args(P->SignatureFunction);int32 Index=0;
        for(TFieldIterator<FProperty> It(P->SignatureFunction);It;++It)
            if(auto* Numeric=CastField<FNumericProperty>(*It))
                if(Numeric->HasAnyPropertyFlags(CPF_Parm))
                {
                    const double V=Index++==0?After:Number(H,TEXT("MaxHealth"),100);
                    void* Address=Numeric->ContainerPtrToValuePtr<void>(Args.GetStructMemory());
                    if(Numeric->IsFloatingPoint()) Numeric->SetFloatingPointPropertyValue(Address,V);
                    else Numeric->SetIntPropertyValue(Address,int64(V));
                }
        P->GetMulticastDelegate(P->ContainerPtrToValuePtr<void>(H))->ProcessDelegate<UObject>(Args.GetStructMemory());
    }
    RefreshHUD(A);return true;
}
bool FoundationBridge::AddAmmo(AActor* A,int32 Amount)
{
    auto* W=Weapon(A);if(!W || Amount<=0) return false;
    const int32 Before=int32(Number(W,TEXT("ReserveAmmo")));
    const int32 After=FMath::Clamp(Before+Amount,0,999);
    if(After==Before) return false;
    SetNumber(W,TEXT("ReserveAmmo"),After);RefreshHUD(A);return true;
}
