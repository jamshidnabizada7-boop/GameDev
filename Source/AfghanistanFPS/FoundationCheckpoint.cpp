#include "FoundationSession.h"
#include "FoundationSave.h"
#include "FoundationBridge.h"
#include "FoundationThrowables.h"
#include "Phase2Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FString CheckpointSlot=TEXT("FoundationCheckpoint");
    bool ValidSlot(const FString& Slot)
    {
        if(Slot.IsEmpty() || Slot.Len()>64)return false;
        for(TCHAR Character:Slot)
            if(!FChar::IsAlnum(Character) && Character!=TEXT('_') && Character!=TEXT('-'))return false;
        return true;
    }

    FFoundationSavedProp CaptureProp(const AFoundationInteractable& Actor)
    {
        FFoundationSavedProp State;
        State.Id=Actor.StableId;
        State.ItemId=Actor.ItemId;
        State.Kind=Actor.Kind;
        State.Transform=Actor.GetActorTransform();
        State.Runtime=Actor.ActorHasTag(TEXT("RuntimeItem"));
        State.Consumed=Actor.Consumed;
        State.Locked=Actor.Locked;
        State.Open=Actor.Open;
        State.Broken=Actor.Broken;
        State.LightOn=Actor.LightOn;
        State.Angle=Actor.DoorAngle;
        State.Durability=Actor.Durability;
        State.ItemAmount=Actor.ItemAmount;
        State.Magazine=Actor.WeaponMagazine;
        State.Reserve=Actor.WeaponReserve;
        return State;
    }
}

bool UFoundationSession::CanCheckpoint(const APhase2Character* Player) const
{
    return Player && Player->IsPlayerControlled() && !Player->P2IsDead()
        && !Player->P2Traversing && !Player->TacticalTarget
        && !Player->InteractionManager->Using && !Player->InteractionManager->MirrorActive
        && !Player->Equipment->IsHoldingThrowable() && !Player->Equipment->Reloading
        && !Player->Equipment->Equipping && Player->GetCharacterMovement()->IsMovingOnGround();
}

void UFoundationSession::SaveDefaultSlot(){SaveCheckpoint(CheckpointSlot);}
void UFoundationSession::LoadDefaultSlot(){LoadCheckpoint(CheckpointSlot);}

bool UFoundationSession::SaveCheckpoint(const FString& Slot)
{
    auto* Player=Cast<APhase2Character>(GetOwner());
    if(!CanCheckpoint(Player) || Player->P2Stance!=0 || !ValidSlot(Slot))
    {
        Message=NSLOCTEXT("Foundation","CheckpointBusy","Stand on clear ground and finish the current action before saving.");
        return false;
    }
    for(TActorIterator<AFoundationThrowable> It(GetWorld());It;++It)
    {
        if(It->Launched && !It->Consumed && (It->Kind==EFoundationThrowableKind::Frag || It->Kind==EFoundationThrowableKind::Flash || It->Kind==EFoundationThrowableKind::Smoke))
        {
            Message=NSLOCTEXT("Foundation","CheckpointLiveThrowable","Wait for the throwable effect to end before saving.");
            return false;
        }
    }
    Player->Equipment->SyncAmmo();
    auto* Save=NewObject<UFoundationCheckpoint>(this);
    Save->Map=UGameplayStatics::GetCurrentLevelName(this,true);
    Save->PlayerTransform=Player->GetActorTransform();
    Save->View=Player->GetControlRotation();
    Save->Health=FoundationBridge::Number(FoundationBridge::Health(Player),TEXT("CurrentHealth"));
    Save->Items=Player->InteractionManager->OwnedItems;
    Save->ObjectiveComplete=ObjectiveComplete;
    Save->ActiveSlot=Player->Equipment->ActiveSlot;
    Save->Holstered=Player->Equipment->Holstered;
    for(const auto& Weapon:Player->Equipment->Slots)
    {
        if(!Weapon.Definition)continue;
        auto& Stored=Save->Weapons.AddDefaulted_GetRef();
        Stored.Id=Weapon.Definition->WeaponId;
        Stored.Magazine=Weapon.Magazine;
        Stored.Reserve=Weapon.Reserve;
        Stored.Mode=Weapon.Mode;
    }
    for(TActorIterator<AFoundationInteractable> It(GetWorld());It;++It)
        if(!It->StableId.IsNone() && !(It->ActorHasTag(TEXT("RuntimeItem")) && It->Consumed))
            Save->Props.Add(CaptureProp(**It));
    const bool Saved=FoundationSave::Write(Save,Slot);
    Message=Saved?NSLOCTEXT("Foundation","CheckpointSaved","Checkpoint saved."):NSLOCTEXT("Foundation","CheckpointSaveFailed","Could not write checkpoint.");
    return Saved;
}

bool UFoundationSession::LoadCheckpoint(const FString& Slot)
{
    auto* Player=Cast<APhase2Character>(GetOwner());
    if(!CanCheckpoint(Player) || Player->P2Stance!=0 || !ValidSlot(Slot))
    {
        Message=NSLOCTEXT("Foundation","CheckpointLoadBusy","Stand on clear ground and finish the current action before loading.");
        return false;
    }
    const auto* Save=FoundationSave::Read<UFoundationCheckpoint>(Slot);
    TArray<FFoundationWeaponSlot> Weapons;
    auto* Health=FoundationBridge::Health(Player);
    const auto Invalid=[this]()
    {
        Message=NSLOCTEXT("Foundation","CheckpointInvalid","This checkpoint is missing, damaged, or from another version.");
        return false;
    };
    if(!Save || Save->Version!=1 || Save->Map!=UGameplayStatics::GetCurrentLevelName(this,true)
        || Save->PlayerTransform.ContainsNaN() || !Save->PlayerTransform.GetRotation().IsNormalized()
        || Save->View.ContainsNaN() || !FMath::IsFinite(Save->Health)
        || Save->Health<=0 || Save->Health>FoundationBridge::Number(Health,TEXT("MaxHealth"),100)
        || Save->Props.Num()>2048 || Save->Items.Num()>128
        || !Player->Equipment->DecodeCheckpointWeapons(Save->Weapons,Save->ActiveSlot,Weapons))return Invalid();

    for(const auto& Item:Save->Items)
        if(Item.Key.IsNone() || Item.Value<0 || Item.Value>999)return Invalid();

    TMap<FName,AFoundationInteractable*> Placed;
    TArray<AFoundationInteractable*> OldDrops;
    for(TActorIterator<AFoundationInteractable> It(GetWorld());It;++It)
    {
        if(It->ActorHasTag(TEXT("RuntimeItem")))OldDrops.Add(*It);
        else Placed.Add(It->StableId,*It);
    }
    TSet<FName> Seen;
    for(const auto& State:Save->Props)
    {
        if(State.Id.IsNone() || Seen.Contains(State.Id) || State.Transform.ContainsNaN()
            || !FMath::IsFinite(State.Angle) || State.Angle<0 || State.Angle>90
            || !FMath::IsFinite(State.Durability) || State.ItemAmount<0 || State.ItemAmount>999
            || State.Magazine<-1 || State.Magazine>999 || State.Reserve<-1 || State.Reserve>999)return Invalid();
        Seen.Add(State.Id);
        if(State.Runtime)
        {
            if(Placed.Contains(State.Id) || State.Kind!=EFoundationInteractableKind::Weapon || State.ItemId.IsNone())return Invalid();
        }
        else if(!Placed.Contains(State.Id) || Placed[State.Id]->Kind!=State.Kind)return Invalid();
    }

    FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundationCheckpoint),false,Player);
    if(AActor* Gun=FoundationBridge::Weapon(Player))Query.AddIgnoredActor(Gun);
    const auto* Capsule=Player->GetCapsuleComponent();
    const FCollisionShape Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius()-1,Capsule->GetScaledCapsuleHalfHeight()-1);
    if(GetWorld()->OverlapBlockingTestByChannel(Save->PlayerTransform.GetLocation(),FQuat::Identity,ECC_Pawn,Shape,Query,
        Capsule->GetCollisionResponseToChannels()))
    {
        Message=NSLOCTEXT("Foundation","CheckpointBlocked","The saved position is currently blocked.");
        return false;
    }

    TMap<FName,AFoundationInteractable*> Staged;
    for(const auto& State:Save->Props)
    {
        if(!State.Runtime)continue;
        auto* Item=GetWorld()->SpawnActorDeferred<AFoundationInteractable>(AFoundationInteractable::StaticClass(),State.Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Item)
        {
            for(const auto& Entry:Staged)Entry.Value->Destroy();
            return Invalid();
        }
        Item->StableId=State.Id;
        Item->Tags.Add(TEXT("RuntimeItem"));
        Item->Kind=State.Kind;
        Staged.Add(State.Id,Item);
    }

    // All references and the destination have been checked. Apply the checkpoint once.
    for(auto* Item:OldDrops)Item->Destroy();
    for(TActorIterator<AFoundationThrowable> It(GetWorld());It;++It)It->Destroy();
    for(const auto& State:Save->Props)
    {
        auto* Prop=State.Runtime?Staged[State.Id]:Placed[State.Id];
        if(State.Runtime)UGameplayStatics::FinishSpawningActor(Prop,State.Transform);
        Prop->RestoreCheckpointState(State);
    }
    Player->InteractionManager->OwnedItems=Save->Items;
    Player->Equipment->RestoreCheckpointWeapons(MoveTemp(Weapons),Save->ActiveSlot,Save->Holstered);
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorTransform(Save->PlayerTransform,false,nullptr,ETeleportType::TeleportPhysics);
    Player->GetController()->SetControlRotation(Save->View);
    FoundationBridge::SetNumber(Health,TEXT("CurrentHealth"),Save->Health);
    FoundationBridge::RefreshHUD(Player);
    ObjectiveComplete=Save->ObjectiveComplete;
    DamageAlpha=0;
    Message=NSLOCTEXT("Foundation","CheckpointLoaded","Checkpoint loaded.");
    return true;
}

void AFoundationInteractable::RestoreCheckpointState(const FFoundationSavedProp& State)
{
    CurrentUser.Reset();
    SetListening(false);
    Consumed=State.Consumed;
    Locked=State.Locked;
    Broken=State.Broken;
    Open=State.Open;
    LightOn=State.LightOn;
    DoorAngle=TargetAngle=State.Angle;
    Durability=State.Durability;
    ItemId=State.ItemId;
    ItemAmount=State.ItemAmount;
    WeaponMagazine=State.Magazine;
    WeaponReserve=State.Reserve;
    Configure(Kind);
    PickupStart=Panel->GetRelativeLocation();
    DrawerAlpha=Open?1.f:0.f;
    if(Kind==EFoundationInteractableKind::Drawer)Panel->SetRelativeLocation(PickupStart+FVector(0,70*DrawerAlpha,0));
    for(const auto& Actor:LinkedLights)
        if(IsValid(Actor))
        {
            TInlineComponentArray<ULightComponent*> Lights(Actor.Get());
            for(auto* Linked:Lights)Linked->SetVisibility(LightOn);
        }
    SetActorTickEnabled(false);
}

bool UFoundationEquipment::DecodeCheckpointWeapons(const TArray<FFoundationSavedWeapon>& Saved,int32 Active,TArray<FFoundationWeaponSlot>& Out) const
{
    if(!Initialized || !Holder || Saved.Num()>SlotLimit || (Saved.IsEmpty()?Active!=INDEX_NONE:!Saved.IsValidIndex(Active)))return false;
    TSet<FName> Seen;
    for(const auto& Weapon:Saved)
    {
        auto* Definition=FindDefinition(Weapon.Id);
        if(!Definition || !Definition->ActorClass || Seen.Contains(Weapon.Id) || Weapon.Magazine<0 || Weapon.Magazine>Definition->Capacity
            || Weapon.Reserve<0 || Weapon.Reserve>999 || uint8(Weapon.Mode)>uint8(EFoundationFireMode::Burst))return false;
        Seen.Add(Weapon.Id);
        Out.Add({Definition,Weapon.Magazine,Weapon.Reserve,Weapon.Mode});
    }
    return true;
}

void UFoundationEquipment::ClearIntent()
{
    AimHeld=FireHeld=false;
    BurstRemaining=0;
}

void UFoundationEquipment::RestoreCheckpointWeapons(TArray<FFoundationWeaponSlot> Saved,int32 Active,bool WasHolstered)
{
    Slots=MoveTemp(Saved);
    ActiveSlot=Active;
    Holstered=WasHolstered;
    Reloading=Inspecting=Equipping=false;
    ClearIntent();
    if(Slots.IsValidIndex(ActiveSlot))ApplySlot();
    else
    {
        Holder->SetChildActorClass(nullptr);
        Player->P2GripLeft=Player->P2GripRight=nullptr;
        Muzzle=nullptr;
        Flash=nullptr;
    }
}
