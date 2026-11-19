#include "FoundationEquipment.h"
#include "FoundationSession.h"
#include "FoundationBridge.h"
#include "FoundationInteraction.h"
#include "Phase2Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Perception/AISense_Hearing.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"

namespace
{
bool Flag(UObject* O,FName N)
{if(O) if(auto* P=FindFProperty<FBoolProperty>(O->GetClass(),N))return P->GetPropertyValue_InContainer(O);return false;}
void CallFlag(UObject* O,FName N,bool V)
{
    if(!O)return;UFunction* F=O->FindFunction(N);if(!F)return;FStructOnScope A(F);
    for(TFieldIterator<FBoolProperty> P(F);P;++P)if(P->HasAnyPropertyFlags(CPF_Parm))P->SetPropertyValue_InContainer(A.GetStructMemory(),V);
    O->ProcessEvent(F,A.GetStructMemory());
}
}
UFoundationEquipment::UFoundationEquipment()
{PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;}
void UFoundationEquipment::BeginPlay()
{
    Super::BeginPlay();Player=Cast<APhase2Character>(GetOwner());
#if WITH_EDITOR
    if(GetOwner()->IsEditorOnly())SetComponentTickEnabled(false);
#endif
}
void UFoundationEquipment::EndPlay(const EEndPlayReason::Type Reason)
{if(StatusWidget)StatusWidget->RemoveFromParent();Super::EndPlay(Reason);}
UFoundationWeaponDefinition* UFoundationEquipment::FindDefinition(FName Id) const
{
    for(const auto& D:AvailableDefinitions)if(D && D->WeaponId==Id)return D.Get();
    const TCHAR* Path=Id==TEXT("weapon_rifle_base")?TEXT("/Game/AfghanFPS/Weapons/Foundation/DA_RifleFoundation.DA_RifleFoundation"):
        Id==TEXT("weapon_rifle_alt")?TEXT("/Game/AfghanFPS/Weapons/Foundation/DA_AlternateFoundation.DA_AlternateFoundation"):nullptr;
    return Path?LoadObject<UFoundationWeaponDefinition>(nullptr,Path):nullptr;
}
bool UFoundationEquipment::Initialize()
{
    if(!Player.IsValid() || !Player->P2Camera || !Player->IsPlayerControlled())return false;
    auto* Weapon=FoundationBridge::Weapon(Player.Get());if(!Weapon)return false;
    TInlineComponentArray<UChildActorComponent*> Children(Player.Get());for(auto* C:Children)if(C->GetChildActor()==Weapon)Holder=C;
    TInlineComponentArray<USceneComponent*> Parts(Player.Get());for(auto* C:Parts)if(C->GetName().Contains(TEXT("FP_WeaponMount")))Mount=C;
    auto* Definition=FindDefinition(TEXT("weapon_rifle_base"));if(!Definition || !Holder)return false;
    FFoundationWeaponSlot Slot;Slot.Definition=Definition;Slot.Magazine=FMath::Clamp(int32(FoundationBridge::Number(Weapon,TEXT("CurrentAmmo"))),0,Definition->Capacity);
    Slot.Reserve=int32(FoundationBridge::Number(Weapon,TEXT("ReserveAmmo")));Slot.Mode=Definition->DefaultMode;Slots.Add(Slot);ActiveSlot=0;
    Player->InteractionManager->OwnedItems.FindOrAdd(Definition->WeaponId)=1;
    for(const auto& Pair:TArray<TPair<FName,int32>>{{TEXT("knife"),3},{TEXT("stone"),5},{TEXT("frag"),2},{TEXT("smoke"),1},{TEXT("flash"),1}})
        if(!Player->InteractionManager->OwnedItems.Contains(Pair.Key))Player->InteractionManager->OwnedItems.Add(Pair.Key,Pair.Value);
    ShotSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Shot.Shot"));
    DrySound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Dry.Dry"));
    ReloadSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Reload.Reload"));
    ApplySlot();Initialized=true;
    // Character Tick establishes the neutral camera/mount transform. Equipment
    // adds its current action pose after that, then arms solve to those grips.
    AddTickPrerequisiteActor(Player.Get());
    AddTickPrerequisiteComponent(Player->GetMesh());
    if(Player->P2Arms)Player->P2Arms->AddTickPrerequisiteComponent(this);
    Crosshair=Cast<UWidget>(FoundationBridge::Object(FoundationBridge::Object(Player.Get(),TEXT("AmmoHUD")),TEXT("CrosshairText")));
    if(auto* PC=Cast<APlayerController>(Player->GetController()))
    {auto* W=CreateWidget<UFoundationEquipmentWidget>(PC,UFoundationEquipmentWidget::StaticClass());W->Source=this;StatusWidget=W;W->SetVisibility(ESlateVisibility::HitTestInvisible);W->AddToViewport(12);}
    for(int32 I=0;I<12;++I)
    {
        auto* Segment=NewObject<UStaticMeshComponent>(Player.Get());Segment->SetupAttachment(Player->GetRootComponent());Segment->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        if(auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/AfghanFPS/Weapons/Foundation/M_Trajectory.M_Trajectory")))Segment->SetMaterial(0,Material);
        Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);Segment->SetCastShadow(false);Segment->SetVisibility(false);Segment->RegisterComponent();PreviewSegments.Add(Segment);
    }
    LastAction=TEXT("Equipment ready");return true;
}
void UFoundationEquipment::BindInput(UEnhancedInputComponent* Input)
{
    auto Once=[this,Input](const TCHAR* Name,auto Method)
    {if(auto* A=LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/Input/Actions/%s.%s"),Name,Name)))Input->BindAction(A,ETriggerEvent::Started,this,Method);};
    auto Held=[this,Input](const TCHAR* Name,auto On,auto Off)
    {if(auto* A=LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/Input/Actions/%s.%s"),Name,Name)))
    {Input->BindAction(A,ETriggerEvent::Started,this,On);Input->BindAction(A,ETriggerEvent::Completed,this,Off);Input->BindAction(A,ETriggerEvent::Canceled,this,Off);}};
    Held(TEXT("IA_Fire"),&UFoundationEquipment::TriggerOn,&UFoundationEquipment::TriggerOff);
    Held(TEXT("IA_ADS"),&UFoundationEquipment::AimOn,&UFoundationEquipment::AimOff);
    Held(TEXT("IA_Throw"),&UFoundationEquipment::BeginThrow,&UFoundationEquipment::ReleaseThrow);
    Once(TEXT("IA_ThrowableCycle"),&UFoundationEquipment::CycleThrowable);Once(TEXT("IA_SelectKnife"),&UFoundationEquipment::SelectKnife);Once(TEXT("IA_SelectStone"),&UFoundationEquipment::SelectStone);
    Once(TEXT("IA_WeaponOne"),&UFoundationEquipment::SlotOne);Once(TEXT("IA_WeaponTwo"),&UFoundationEquipment::SlotTwo);
    Once(TEXT("IA_WeaponNext"),&UFoundationEquipment::NextSlot);Once(TEXT("IA_FireMode"),&UFoundationEquipment::CycleMode);
    Once(TEXT("IA_Holster"),&UFoundationEquipment::ToggleHolster);Once(TEXT("IA_Inspect"),&UFoundationEquipment::Inspect);
    if(auto* A=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/Actions/IA_Reload.IA_Reload")))Input->BindActionValueLambda(A,ETriggerEvent::Started,[this](const FInputActionValue&){Reload();});
    if(auto* A=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/Actions/IA_DropWeapon.IA_DropWeapon")))Input->BindActionValueLambda(A,ETriggerEvent::Started,[this](const FInputActionValue&){DropWeapon();});
}
void UFoundationEquipment::SyncAmmo()
{
    if(!Slots.IsValidIndex(ActiveSlot) || !Player.IsValid())return;
    if(auto* Weapon=FoundationBridge::Weapon(Player.Get()))
    {Slots[ActiveSlot].Magazine=int32(FoundationBridge::Number(Weapon,TEXT("CurrentAmmo")));Slots[ActiveSlot].Reserve=int32(FoundationBridge::Number(Weapon,TEXT("ReserveAmmo")));}
}
void UFoundationEquipment::ApplySlot()
{
    if(!Holder || !Slots.IsValidIndex(ActiveSlot))return;
    auto& S=Slots[ActiveSlot];if(!S.Definition)return;
    if(IsValid(Flash))Flash->DestroyComponent();Flash=nullptr;
    if(S.Definition->ActorClass && Holder->GetChildActorClass()!=S.Definition->ActorClass)Holder->SetChildActorClass(S.Definition->ActorClass);
    AActor* W=Holder->GetChildActor();if(!W)return;W->SetOwner(Player.Get());
    FoundationBridge::SetNumber(W,TEXT("CurrentAmmo"),S.Magazine);FoundationBridge::SetNumber(W,TEXT("ReserveAmmo"),S.Reserve);
    FoundationBridge::SetNumber(W,TEXT("FireInterval"),FMath::Max(.05f,S.Definition->ShotInterval-.005f));
    FoundationBridge::SetNumber(W,TEXT("BaseDamage"),S.Definition->BaseDamage);
    Player->P2GripLeft=nullptr;Player->P2GripRight=nullptr;Muzzle=nullptr;
    TInlineComponentArray<USceneComponent*> Parts(W);for(auto* C:Parts)
    {if(C->GetName()==TEXT("GripPoint_Left"))Player->P2GripLeft=C;if(C->GetName()==TEXT("GripPoint_Right"))Player->P2GripRight=C;if(C->GetName()==TEXT("MuzzlePoint"))Muzzle=C;}
    if(S.Definition->VisualMaterial){TInlineComponentArray<UStaticMeshComponent*> Meshes(W);for(auto* Mesh:Meshes)Mesh->SetMaterial(0,S.Definition->VisualMaterial);}
    if(S.Definition->CompactVariant)
    {
        TInlineComponentArray<UStaticMeshComponent*> Meshes(W);for(auto* Mesh:Meshes)
        {
            if(Mesh->GetName()==TEXT("Mesh_Stock"))Mesh->SetVisibility(false);
            if(Mesh->GetName()==TEXT("Mesh_Receiver"))Mesh->SetRelativeScale3D(FVector(.18,.06,.08));
            if(Mesh->GetName()==TEXT("Mesh_Barrel")){Mesh->SetRelativeScale3D(FVector(.025,.025,.15));Mesh->SetRelativeLocation(FVector(16,0,1.5));}
            if(Mesh->GetName()==TEXT("Mesh_Grip"))Mesh->SetRelativeLocation(FVector(-3,0,-8));
        }
        if(Muzzle)Muzzle->SetRelativeLocation(FVector(24,0,1.5));
        if(Player->P2GripRight)Player->P2GripRight->SetRelativeLocation(FVector(-3,0,-8));
        if(Player->P2GripLeft)Player->P2GripLeft->SetRelativeLocation(FVector(5,-2,-6));
    }
    Flash=NewObject<UPointLightComponent>(W);Flash->SetupAttachment(Muzzle?Muzzle.Get():W->GetRootComponent());Flash->SetIntensity(1200);Flash->SetAttenuationRadius(130);Flash->SetLightColor(FLinearColor(1,.55f,.12f));Flash->SetCastShadows(false);Flash->RegisterComponent();Flash->SetVisibility(false);
    SetWeaponVisible(!Holstered);FoundationBridge::RefreshHUD(Player.Get());
}
bool UFoundationEquipment::AddWeapon(FName Id,int32 Magazine,int32 Reserve)
{
    if(!CanAcceptWeapon(Id))return false;
    auto* D=FindDefinition(Id);if(!D)return false;
    FFoundationWeaponSlot S;S.Definition=D;S.Magazine=Magazine<0?D->Capacity:FMath::Clamp(Magazine,0,D->Capacity);S.Reserve=Reserve<0?D->InitialReserve:FMath::Clamp(Reserve,0,999);S.Mode=D->DefaultMode;
    Slots.Add(S);LastAction=TEXT("Weapon added to free slot");
    if(ActiveSlot==INDEX_NONE)
    {
        // Pickup completion is still inside the interaction transaction. The
        // public switch command correctly rejects that state; initial equip
        // must activate the accepted slot directly rather than leaving it empty.
        ActiveSlot=Slots.Num()-1;Holstered=false;Reloading=Inspecting=false;
        StateStarted=GetWorld()->GetTimeSeconds();StateEnds=StateStarted+.4f;Equipping=true;ApplySlot();
    }
    return true;
}
bool UFoundationEquipment::CanAcceptWeapon(FName Id) const
{
    if(!Initialized || !Player.IsValid() || Player->P2IsDead() || Slots.Num()>=FMath::Clamp(SlotLimit,1,8) || !FindDefinition(Id))return false;
    for(const auto& S:Slots)if(S.Definition && S.Definition->WeaponId==Id)return false;return true;
}
bool UFoundationEquipment::SelectSlot(int32 Index)
{
    if(!Player.IsValid() || Player->P2IsDead() || Player->TacticalTarget || !Slots.IsValidIndex(Index) || Index==ActiveSlot || Equipping || Player->P2Traversing || Player->InteractionManager->Using || Player->InteractionManager->MirrorActive)return false;
    SyncAmmo();Reloading=Inspecting=false;BurstRemaining=0;FireHeld=false;AimHeld=false;PendingSlot=Index;Holstered=false;
    StateStarted=GetWorld()->GetTimeSeconds();StateEnds=StateStarted+.65f;Equipping=true;LastAction=TEXT("Lowering weapon");return true;
}
void UFoundationEquipment::SlotOne(){SelectSlot(0);}void UFoundationEquipment::SlotTwo(){SelectSlot(1);}
void UFoundationEquipment::NextSlot(){if(Slots.Num()>1)SelectSlot((ActiveSlot+1)%Slots.Num());}
void UFoundationEquipment::SetWeaponVisible(bool Visible)
{if(Holder && Holder->GetChildActor())Holder->GetChildActor()->SetActorHiddenInGame(!Visible);if(Player.IsValid() && Player->P2Arms)Player->P2Arms->SetVisibility(Visible);}
bool UFoundationEquipment::DropWeapon()
{
    if(!CanAct() || !Slots.IsValidIndex(ActiveSlot))return false;SyncAmmo();const auto S=Slots[ActiveSlot];
    FVector Eye;FRotator View;Player->GetActorEyesViewPoint(Eye,View);FVector Location=Eye+View.Vector()*90;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationDrop),false,Player.Get());if(Holder->GetChildActor())Q.AddIgnoredActor(Holder->GetChildActor());
    if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Location,ECC_Visibility,Q)){LastAction=TEXT("Drop blocked");return false;}
    if(GetWorld()->LineTraceSingleByChannel(Hit,Location,Location-FVector(0,0,400),ECC_Visibility,Q))Location.Z=Hit.ImpactPoint.Z;
    auto* Item=GetWorld()->SpawnActorDeferred<AFoundationInteractable>(AFoundationInteractable::StaticClass(),FTransform(FRotator::ZeroRotator,Location),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);if(!Item)return false;
    Item->Kind=EFoundationInteractableKind::Weapon;Item->StableId=FName(*FString::Printf(TEXT("RuntimeWeapon_%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    Item->Tags.Add(TEXT("RuntimeItem"));Item->ItemId=S.Definition->WeaponId;Item->ItemAmount=1;Item->WeaponMagazine=S.Magazine;Item->WeaponReserve=S.Reserve;UGameplayStatics::FinishSpawningActor(Item,Item->GetActorTransform());
    Player->InteractionManager->OwnedItems.Remove(S.Definition->WeaponId);Slots.RemoveAt(ActiveSlot);ActiveSlot=INDEX_NONE;
    if(Slots.Num())SelectSlot(0);else{Holder->SetChildActorClass(nullptr);Player->P2GripLeft=nullptr;Player->P2GripRight=nullptr;FoundationBridge::RefreshHUD(Player.Get());}
    LastAction=TEXT("Dropped weapon with its ammunition");return true;
}
bool UFoundationEquipment::CanAct() const
{
    return Initialized && Player.IsValid() && !Player->P2IsDead() && !Player->P2Traversing && !Player->TacticalTarget && !Reloading && !Equipping && !Inspecting && !Holstered && !IsHoldingThrowable() &&
        !(Player->InteractionManager && (Player->InteractionManager->Using || Player->InteractionManager->MirrorActive));
}
bool UFoundationEquipment::Reload()
{
    if(!CanAct() || !Slots.IsValidIndex(ActiveSlot))return false;SyncAmmo();const auto& S=Slots[ActiveSlot];
    if(S.Magazine>=S.Definition->Capacity || S.Reserve<=0){LastAction=TEXT("Reload rejected: full or no reserve");return false;}
    Reloading=true;AimHeld=false;BurstRemaining=0;StateStarted=GetWorld()->GetTimeSeconds();StateEnds=StateStarted+S.Definition->ReloadSeconds;
    if(ReloadSound)UGameplayStatics::PlaySoundAtLocation(this,ReloadSound,Player->GetActorLocation());LastAction=TEXT("Reloading");return true;
}
void UFoundationEquipment::TriggerOn()
{if(!CanAct() || Player->P2Sprinting){FireHeld=false;BurstRemaining=0;return;}FireHeld=true;if(Slots.IsValidIndex(ActiveSlot) && Slots[ActiveSlot].Mode==EFoundationFireMode::Burst)BurstRemaining=3;FireOne();}
void UFoundationEquipment::TriggerOff(){FireHeld=false;}
void UFoundationEquipment::AimOn(){AimHeld=true;}void UFoundationEquipment::AimOff(){AimHeld=false;}
void UFoundationEquipment::CycleMode()
{if(!CanAct() || !Slots.IsValidIndex(ActiveSlot))return;auto& S=Slots[ActiveSlot];S.Mode=EFoundationFireMode((uint8(S.Mode)+1)%3);BurstRemaining=0;LastAction=TEXT("Fire mode changed");}
void UFoundationEquipment::ToggleHolster()
{if(!Initialized || !Player.IsValid() || Player->P2IsDead() || Reloading || Equipping || !Slots.IsValidIndex(ActiveSlot))return;Holstered=!Holstered;AimHeld=false;FireHeld=false;BurstRemaining=0;Equipping=true;StateStarted=GetWorld()->GetTimeSeconds();StateEnds=StateStarted+.3f;SetWeaponVisible(!Holstered);LastAction=Holstered?TEXT("Holstered"):TEXT("Drawn");}
void UFoundationEquipment::Inspect()
{if(!CanAct())return;Inspecting=true;AimHeld=false;FireHeld=false;BurstRemaining=0;StateStarted=GetWorld()->GetTimeSeconds();StateEnds=StateStarted+1.4f;LastAction=TEXT("Inspecting");}
bool UFoundationEquipment::AddReserve(FName Type,int32 Count)
{
    if(!Initialized || Count<=0 || !Player.IsValid() || Player->P2IsDead())return false;SyncAmmo();
    int32 Index=ActiveSlot;
    if(Type!=NAME_None)for(int32 I=0;I<Slots.Num();++I)if(Slots[I].Definition->AmmoType==Type){Index=I;break;}
    if(!Slots.IsValidIndex(Index) || (Type!=NAME_None && Slots[Index].Definition->AmmoType!=Type))return false;
    auto& S=Slots[Index];const int32 Before=S.Reserve;S.Reserve=FMath::Clamp(S.Reserve+Count,0,999);if(Before==S.Reserve)return false;
    if(Index==ActiveSlot){FoundationBridge::SetNumber(FoundationBridge::Weapon(Player.Get()),TEXT("ReserveAmmo"),S.Reserve);FoundationBridge::RefreshHUD(Player.Get());}return true;
}
bool UFoundationEquipment::FireOne()
{
    if(!CanAct() || !Slots.IsValidIndex(ActiveSlot) || Player->P2Sprinting)return false;
    const float Now=GetWorld()->GetTimeSeconds();if(Now<NextShot)return false;SyncAmmo();auto& S=Slots[ActiveSlot];auto* Weapon=FoundationBridge::Weapon(Player.Get());if(!Weapon)return false;
    if(S.Magazine<=0){++DryClicks;NextShot=Now+.25f;FoundationBridge::Call(FoundationBridge::Object(Player.Get(),TEXT("AmmoHUD")),TEXT("ShowEmptyMagWarning"));if(DrySound)UGameplayStatics::PlaySoundAtLocation(this,DrySound,Player->P2Camera->GetComponentLocation());LastAction=TEXT("Empty magazine");BurstRemaining=0;return false;}
    if(MuzzleBlocked){LastAction=TEXT("Muzzle blocked by cover");return false;}
    const FRotator Original=Player->P2Camera->GetComponentRotation();FRotator Aim=Original;
    Aim.Pitch+=FMath::FRandRange(-SpreadDegrees,SpreadDegrees);Aim.Yaw+=FMath::FRandRange(-SpreadDegrees,SpreadDegrees);
    Player->P2Camera->SetWorldRotation(Aim);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationBallistics),false,Player.Get());Q.AddIgnoredActor(Weapon);FHitResult Hit;
    const FVector Eye=Player->P2Camera->GetComponentLocation();GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Aim.Vector()*5000,ECC_Visibility,Q);
    float Multiplier=FMath::GetMappedRangeValueClamped(FVector2D(1000,5000),FVector2D(1,.5f),Hit.bBlockingHit?Hit.Distance:5000);
    const FString Bone=Hit.BoneName.ToString();LastDamageZone=TEXT("Body");
    if(Bone.Contains(TEXT("head")) || Bone.Contains(TEXT("neck")))LastDamageZone=TEXT("Head");
    else if(Bone.Contains(TEXT("arm")) || Bone.Contains(TEXT("hand")) || Bone.Contains(TEXT("thigh")) || Bone.Contains(TEXT("calf")) || Bone.Contains(TEXT("leg")) || Bone.Contains(TEXT("foot")))LastDamageZone=TEXT("Limb");
    else if(Hit.BoneName.IsNone() && Hit.GetActor())
    {
        // Simple capsule/box targets do not return a skeletal bone. Classify
        // their hit height against the actual rig instead of treating all hits
        // as torso damage. Targets without this rig retain body damage.
        TInlineComponentArray<USkeletalMeshComponent*> Meshes(Hit.GetActor());
        for(auto* Mesh:Meshes)if(Mesh->DoesSocketExist(TEXT("head")) && Mesh->DoesSocketExist(TEXT("pelvis")))
        {
            const FVector Head=Mesh->GetSocketLocation(TEXT("head")),Pelvis=Mesh->GetSocketLocation(TEXT("pelvis"));
            if(Hit.ImpactPoint.Z>=Head.Z-12)LastDamageZone=TEXT("Head");
            else if(Hit.ImpactPoint.Z<Pelvis.Z-15 || FMath::Abs(FVector::DotProduct(Hit.ImpactPoint-Pelvis,Hit.GetActor()->GetActorRightVector()))>25)LastDamageZone=TEXT("Limb");
            break;
        }
    }
    if(LastDamageZone==TEXT("Head"))Multiplier*=2;else if(LastDamageZone==TEXT("Limb"))Multiplier*=.65f;
    LastShotDamage=S.Definition->BaseDamage*Multiplier;LastPenetrated=false;
    FoundationBridge::SetNumber(Weapon,TEXT("BaseDamage"),S.Definition->ProjectileMode?0:LastShotDamage);
    const double Range=FoundationBridge::Number(Weapon,TEXT("MaxRange"),5000);
    if(S.Definition->ProjectileMode)FoundationBridge::SetNumber(Weapon,TEXT("MaxRange"),0);
    auto* HitHealth=FoundationBridge::Health(Hit.GetActor());
    const float HealthBefore=FoundationBridge::Number(HitHealth,TEXT("CurrentHealth"));
    const int32 Before=S.Magazine;FoundationBridge::Call(Weapon,TEXT("TryFire"));
    if(S.Definition->ProjectileMode)FoundationBridge::SetNumber(Weapon,TEXT("MaxRange"),Range);
    Player->P2Camera->SetWorldRotation(Original);SyncAmmo();
    if(S.Magazine>=Before)return false;
    if(!S.Definition->ProjectileMode && HealthBefore>0 && IsValid(HitHealth))
        Player->FoundationSession->PresentAppliedDamage(Hit.ImpactPoint,
            HealthBefore-FoundationBridge::Number(HitHealth,TEXT("CurrentHealth")));
    const FVector Origin=Muzzle?Muzzle->GetComponentLocation():Eye;
    if(S.Definition->ProjectileMode)
    {
        auto* Bullet=GetWorld()->SpawnActorDeferred<AFoundationThrowable>(AFoundationThrowable::StaticClass(),FTransform(Aim,Origin),Player.Get(),Player.Get(),ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Bullet){FoundationBridge::SetNumber(Weapon,TEXT("CurrentAmmo"),Before);SyncAmmo();return false;}
        Bullet->Kind=EFoundationThrowableKind::Bullet;Bullet->Damage=LastShotDamage;UGameplayStatics::FinishSpawningActor(Bullet,Bullet->GetActorTransform());
        const FVector Target=Hit.bBlockingHit?Hit.ImpactPoint:Eye+Aim.Vector()*5000;Bullet->Launch((Target-Origin).GetSafeNormal()*22000);
        for(FName N:{FName(TEXT("bLastHitConfirmed")),FName(TEXT("bLastHitLethal"))})if(auto* P=FindFProperty<FBoolProperty>(Weapon->GetClass(),N))P->SetPropertyValue_InContainer(Weapon,false);
    }
    else if(S.Definition->AllowPenetration && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Penetrable")) && Hit.GetComponent())
    {
        FHitResult Exit;
        if(Hit.GetComponent()->LineTraceComponent(Exit,Hit.ImpactPoint+Aim.Vector()*30,Hit.ImpactPoint-Aim.Vector(),Q) && FVector::Dist(Exit.ImpactPoint,Hit.ImpactPoint)<=25)
        {
            FHitResult Behind;FCollisionQueryParams BehindQ=Q;BehindQ.AddIgnoredActor(Hit.GetActor());
            if(GetWorld()->LineTraceSingleByChannel(Behind,Exit.ImpactPoint+Aim.Vector()*2,Eye+Aim.Vector()*5000,ECC_Visibility,BehindQ) && Behind.GetActor())
            {
                auto* Health=FoundationBridge::Health(Behind.GetActor());const bool Alive=FoundationBridge::Number(Health,TEXT("CurrentHealth"))>0;
                UGameplayStatics::ApplyPointDamage(Behind.GetActor(),LastShotDamage*.5f,Aim.Vector(),Behind,Player->GetController(),Weapon,UDamageType::StaticClass());LastPenetrated=true;
                if(Alive)for(FName N:{FName(TEXT("bLastHitConfirmed")),FName(TEXT("bLastHitLethal"))})if(auto* P=FindFProperty<FBoolProperty>(Weapon->GetClass(),N))P->SetPropertyValue_InContainer(Weapon,N==TEXT("bLastHitConfirmed") || FoundationBridge::Number(Health,TEXT("CurrentHealth"))<=0);
            }
        }
    }
    ++ShotsFired;if(BurstRemaining>0)--BurstRemaining;NextShot=Now+S.Definition->ShotInterval;Player->P2Recoil=1;
    if(auto* PC=Cast<APlayerController>(Player->GetController())){FRotator R=PC->GetControlRotation();R.Pitch=FMath::ClampAngle(R.Pitch+.55f,-85,85);R.Yaw+=FMath::FRandRange(-.15f,.15f);PC->SetControlRotation(R);}
    if(ShotSound)UGameplayStatics::PlaySoundAtLocation(this,ShotSound,Origin);
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),Origin,1,Player.Get(),3000,TEXT("Gunshot"));
    if(Flash){Flash->SetVisibility(true);FlashEnds=Now+.04f;}
    ShellActors.RemoveAll([](const TWeakObjectPtr<AActor>& A){return !A.IsValid();});
    if(ShellActors.Num()>=16){if(ShellActors[0].IsValid())ShellActors[0]->Destroy();ShellActors.RemoveAt(0);}
    if(auto* Shell=GetWorld()->SpawnActor<AStaticMeshActor>(Origin+Player->P2Camera->GetRightVector()*8,FRotator(0,0,70)))
    {
        auto* Mesh=Shell->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Shell->SetActorScale3D(FVector(.01,.01,.04));
        Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);Mesh->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);Mesh->SetMassOverrideInKg(NAME_None,.009f,true);Mesh->SetSimulatePhysics(true);
        Mesh->SetPhysicsLinearVelocity(Player->P2Camera->GetRightVector()*150+FVector::UpVector*70);Shell->SetLifeSpan(3);ShellActors.Add(Shell);++ShellEjections;
    }
    if(Hit.bBlockingHit && !S.Definition->ProjectileMode)
        ShowImpact(Hit,HealthBefore>0 && IsValid(HitHealth) &&
            FoundationBridge::Number(HitHealth,TEXT("CurrentHealth"))<HealthBefore);
    UObject* HUD=FoundationBridge::Object(Player.Get(),TEXT("AmmoHUD"));if(Flag(Weapon,TEXT("bLastHitConfirmed")))CallFlag(HUD,TEXT("ShowHitMarker"),Flag(Weapon,TEXT("bLastHitLethal")));
    FoundationBridge::RefreshHUD(Player.Get());LastAction=TEXT("Shot fired");return true;
}
void UFoundationEquipment::ShowImpact(const FHitResult& Hit,bool CharacterHit)
{
    // Marks are owned by the player and reused in a ring. Collision stays off
    // so a previous hit cannot intercept a later trace or an interaction.
    constexpr int32 MaxMarks=16;
    if(!ImpactMarks.IsValidIndex(NextImpactMark))
    {
        auto* Mark=NewObject<UStaticMeshComponent>(GetOwner());
        Mark->ComponentTags.Add(TEXT("FoundationImpactMark"));
        Mark->SetMobility(EComponentMobility::Movable);
        Mark->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Mark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mark->SetCanEverAffectNavigation(false);
        Mark->SetCastShadow(false);
        Mark->RegisterComponent();
        ImpactMarks.Add(Mark);
        ImpactExpiry.Add(0);
    }
    auto* Mark=ImpactMarks[NextImpactMark].Get();
    if(!CharacterImpactMaterial)
    {
        if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
        {
            auto* Tint=UMaterialInstanceDynamic::Create(Base,this);
            Tint->SetVectorParameterValue(TEXT("Color"),FLinearColor(.42f,.015f,.025f));
            CharacterImpactMaterial=Tint;
        }
    }
    if(!SurfaceImpactMaterial)
        SurfaceImpactMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/AfghanFPS/Weapons/Foundation/M_Explosion.M_Explosion"));
    Mark->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Mark->SetWorldLocationAndRotation(Hit.ImpactPoint+Hit.ImpactNormal,Hit.ImpactNormal.Rotation());
    Mark->SetWorldScale3D(CharacterHit?FVector(.015,.09,.07):FVector(.04));
    if(CharacterHit && Hit.GetComponent())
        Mark->AttachToComponent(Hit.GetComponent(),FAttachmentTransformRules::KeepWorldTransform,Hit.BoneName);
    Mark->SetMaterial(0,CharacterHit?CharacterImpactMaterial:SurfaceImpactMaterial);
    Mark->SetVisibility(true);
    ImpactExpiry[NextImpactMark]=GetWorld()->GetTimeSeconds()+(CharacterHit?.6f:.15f);
    NextImpactMark=(NextImpactMark+1)%MaxMarks;
}
void UFoundationEquipment::NotifyProjectileImpact(AActor* Victim,float HealthBefore)
{
    if(HealthBefore<=0 || !Player.IsValid())return;++ProjectileImpacts;
    const bool Lethal=FoundationBridge::Number(FoundationBridge::Health(Victim),TEXT("CurrentHealth"))<=0;
    CallFlag(FoundationBridge::Object(Player.Get(),TEXT("AmmoHUD")),TEXT("ShowHitMarker"),Lethal);
}
void UFoundationEquipment::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,Type,Function);if(!Initialized && !Initialize())return;if(!Player.IsValid() || !Player->IsPlayerControlled())return;
    const float Now=GetWorld()->GetTimeSeconds();if(Flash && Now>=FlashEnds)Flash->SetVisibility(false);
    for(int32 Index=0;Index<ImpactMarks.Num();++Index)
        if(ImpactMarks[Index]->IsVisible() && Now>=ImpactExpiry[Index])ImpactMarks[Index]->SetVisibility(false);
    FlashAlpha=FMath::FInterpConstantTo(FlashAlpha,0.f,Dt,.4f);
    if(HeldThrowable && (HeldThrowable->Detonated || Player->P2IsDead()))
    {if(!HeldThrowable->Detonated)HeldThrowable->Launch(FVector::ZeroVector);HeldThrowable=nullptr;HidePreview();SetWeaponVisible(!Holstered);}
    if(HeldThrowable && Now>=NextPreview){NextPreview=Now+.05f;UpdatePreview();}
    if(Player->P2IsDead() || Player->P2Traversing){PendingSlot=INDEX_NONE;Reloading=Inspecting=Equipping=false;AimHeld=FireHeld=false;BurstRemaining=0;}
    if(Player->P2Sprinting)BurstRemaining=0;
    if(Equipping && PendingSlot!=INDEX_NONE && Now>=StateStarted+(StateEnds-StateStarted)*.5f)
    {
        if(Slots.IsValidIndex(PendingSlot))
        {
            ActiveSlot=PendingSlot;
            ApplySlot();
            LastAction=TEXT("Raising weapon");
        }
        PendingSlot=INDEX_NONE;
    }
    if((Reloading || Equipping || Inspecting) && Now>=StateEnds)
    {
        if(Reloading && Slots.IsValidIndex(ActiveSlot))
        {SyncAmmo();auto& S=Slots[ActiveSlot];const int32 Transfer=FMath::Min(S.Definition->Capacity-S.Magazine,S.Reserve);S.Magazine+=Transfer;S.Reserve-=Transfer;auto* W=FoundationBridge::Weapon(Player.Get());FoundationBridge::SetNumber(W,TEXT("CurrentAmmo"),S.Magazine);FoundationBridge::SetNumber(W,TEXT("ReserveAmmo"),S.Reserve);FoundationBridge::RefreshHUD(Player.Get());LastAction=TEXT("Reload completed");}
        Reloading=Equipping=Inspecting=false;
    }
    const bool WantAds=AimHeld && Slots.IsValidIndex(ActiveSlot) && CanAct() && !Player->P2Sprinting && !Player->InteractionManager->ControlsViewFov();
    AdsAlpha=FMath::FInterpTo(AdsAlpha,WantAds?1.f:0.f,Dt,12);
    const bool CosmeticMotion=Player->UsesCameraMotion();
    const bool WorldView=!Player->InteractionManager->MirrorActive && !Player->InteractionManager->ControlsViewFov();
    const bool SprintView=CosmeticMotion && WorldView && Player->P2Sprinting
        && Player->GetCharacterMovement()->IsMovingOnGround() && Player->GetVelocity().SizeSquared2D()>FMath::Square(450.f);
    SprintFovAlpha=CosmeticMotion?FMath::FInterpTo(SprintFovAlpha,SprintView?1.f:0.f,Dt,8):0.f;
    if(WorldView)
        Player->P2Camera->SetFieldOfView(FMath::Lerp(BaseFov+SprintFovAlpha*7.f,65.f,AdsAlpha));
    if(WorldView && CosmeticMotion && AdsAlpha>.01f)Player->P2Camera->AddWorldRotation(FRotator(FMath::Sin(Now*.9f)*.04f*AdsAlpha,FMath::Sin(Now*.7f)*.06f*AdsAlpha,0));
    const float Move=FMath::Clamp(Player->GetVelocity().Size2D()/450,0.f,1.f);
    SpreadDegrees=FMath::Lerp(1.8f+Move*1.8f, .2f+Move*.35f,AdsAlpha);
    if(Player->P2Stance==1)SpreadDegrees*=.7f;if(Player->P2Stance==2)SpreadDegrees*=.45f;
    if(Player->GetCharacterMovement()->IsFalling())SpreadDegrees+=3;
    if(Crosshair)
    {
        Crosshair->SetVisibility(Player->P2IsDead() || AdsAlpha>.85f?ESlateVisibility::Hidden:ESlateVisibility::HitTestInvisible);
        const float Scale=1+SpreadDegrees*.08f;Crosshair->SetRenderScale(FVector2D(Scale,Scale));
    }
    MuzzleBlocked=false;
    if(Muzzle && Slots.IsValidIndex(ActiveSlot))
    {FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationMuzzle),false,Player.Get());Q.AddIgnoredActor(Holder->GetChildActor());const FVector Start=Player->P2Camera->GetComponentLocation();MuzzleBlocked=GetWorld()->SweepSingleByChannel(Hit,Start,Muzzle->GetComponentLocation()+Player->P2Camera->GetForwardVector()*12,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(3),Q);}
    if(Mount)
    {
        const float Progress=FMath::Clamp((Now-StateStarted)/FMath::Max(.1f,StateEnds-StateStarted),0.f,1.f);
        const float Lower=Equipping?FMath::SmoothStep(0.f,1.f,1.f-FMath::Abs(2.f*Progress-1.f)):0.f;
        const float State=(Reloading || Inspecting)?FMath::Sin(Progress*PI):0.f;
        Mount->AddLocalOffset(FVector(-Lower*8,0,-Lower*22));
        Mount->AddLocalRotation(FRotator(-Lower*22,0,Lower*8));
        Mount->AddLocalOffset(FVector(-AdsAlpha*5,-AdsAlpha*10,AdsAlpha*4-State*12));
        Mount->AddLocalRotation(FRotator(Reloading?-State*30:0,Inspecting?State*55:0,Inspecting?State*25:0));
        if(AdsAlpha>.01f && Muzzle)
        {
            const FVector Eye=Player->P2Camera->GetComponentLocation(),Forward=Player->P2Camera->GetForwardVector();
            const FVector Desired=Eye+Forward*FMath::Max(25.f,float(FVector::DotProduct(Muzzle->GetComponentLocation()-Eye,Forward)))-Player->P2Camera->GetUpVector()*.8f;
            const float Height=Slots.IsValidIndex(ActiveSlot) && Slots[ActiveSlot].Definition
                ?Slots[ActiveSlot].Definition->SightHeight:0.f;
            const FVector Sight=Muzzle->GetComponentLocation()+Muzzle->GetUpVector()*Height*Muzzle->GetComponentScale().Z;
            Mount->AddWorldOffset((Desired-Sight).GetClampedToMaxSize(25)*AdsAlpha);
        }
    }
    if(CanAct() && Slots.IsValidIndex(ActiveSlot) && ((FireHeld && Slots[ActiveSlot].Mode==EFoundationFireMode::Auto) || BurstRemaining>0))FireOne();
}
FString UFoundationEquipment::Summary() const
{
    if(!Slots.IsValidIndex(ActiveSlot))return TEXT("No firearm | E: pick up");const auto& S=Slots[ActiveSlot];
    const TCHAR* Mode=S.Mode==EFoundationFireMode::Semi?TEXT("SEMI"):S.Mode==EFoundationFireMode::Auto?TEXT("AUTO"):TEXT("BURST");
    const TCHAR* Item=SelectedThrowable==EFoundationThrowableKind::Frag?TEXT("frag"):SelectedThrowable==EFoundationThrowableKind::Smoke?TEXT("smoke"):SelectedThrowable==EFoundationThrowableKind::Flash?TEXT("flash"):SelectedThrowable==EFoundationThrowableKind::Knife?TEXT("knife"):TEXT("stone");
    const bool Timed=SelectedThrowable==EFoundationThrowableKind::Frag || SelectedThrowable==EFoundationThrowableKind::Smoke || SelectedThrowable==EFoundationThrowableKind::Flash;
    const FString Action=HeldThrowable?(Timed?FString::Printf(TEXT("%s FUSE %.1fs | release T to throw"),Item,HeldThrowable->RemainingFuse):FString::Printf(TEXT("Release T to throw %s"),Item)):
        FString::Printf(TEXT("Hold T%s %s (%d) | V: cycle"),Timed?TEXT(" cooks"):TEXT(":"),Item,Player.IsValid()?Player->InteractionManager->ItemCount(FName(Item)):0);
    return FString::Printf(TEXT("%s  [%s]  %s\n%s"),*S.Definition->DisplayName.ToString(),Mode,Reloading?TEXT("RELOADING"):Equipping?TEXT("EQUIPPING"):Inspecting?TEXT("INSPECTING"):Holstered?TEXT("HOLSTERED"):MuzzleBlocked?TEXT("COVER BLOCKS MUZZLE"):TEXT(""),*Action);
}
FInputActionValue UFoundationAimModifier::ModifyRaw_Implementation(const UEnhancedPlayerInput* Input,FInputActionValue Value,float Dt)
{
    if(!Input || Value.GetValueType()!=EInputActionValueType::Axis2D)return Value;
    auto* PC=Cast<APlayerController>(Input->GetOuter());APawn* P=PC?PC->GetPawn():nullptr;auto* E=P?P->FindComponentByClass<UFoundationEquipment>():nullptr;if(!E)return Value;
    FVector2D V=Value.Get<FVector2D>()*E->LookSensitivity*FMath::Lerp(1.f,.6f,E->AdsAlpha);if(E->InvertY)V.Y=-V.Y;return FInputActionValue(V);
}
TSharedRef<SWidget> UFoundationEquipmentWidget::RebuildWidget()
{
    return SNew(SOverlay)
        +SOverlay::Slot()[SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity_Lambda([this]{return FLinearColor(1,1,1,Source.IsValid()?Source->FlashAlpha:0.f);})]
        +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(20,0,35,135)
        [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).ColorAndOpacity(FLinearColor::White).ShadowOffset(FVector2D(1,1)).ShadowColorAndOpacity(FLinearColor::Black)
        .Text_Lambda([this]{return Source.IsValid()?FText::FromString(Source->Summary()):FText::GetEmpty();})];
}

FVector UFoundationEquipment::ThrowableHandPoint() const
{return HeldThrowable?HeldThrowable->Body->GetComponentLocation():Player.IsValid() && Player->P2Camera?Player->P2Camera->GetComponentLocation():FVector::ZeroVector;}
void UFoundationEquipment::CycleThrowable()
{if(!CanAct())return;SelectedThrowable=EFoundationThrowableKind((uint8(SelectedThrowable)%5)+1);}
void UFoundationEquipment::SelectKnife(){if(CanAct())SelectedThrowable=EFoundationThrowableKind::Knife;}
void UFoundationEquipment::SelectStone(){if(CanAct())SelectedThrowable=EFoundationThrowableKind::Stone;}
void UFoundationEquipment::BeginThrow()
{
    if(!CanAct())return;
    const TCHAR* Id=SelectedThrowable==EFoundationThrowableKind::Frag?TEXT("frag"):SelectedThrowable==EFoundationThrowableKind::Smoke?TEXT("smoke"):SelectedThrowable==EFoundationThrowableKind::Flash?TEXT("flash"):SelectedThrowable==EFoundationThrowableKind::Knife?TEXT("knife"):TEXT("stone");
    if(Player->InteractionManager->ItemCount(FName(Id))<=0){LastAction=TEXT("No selected throwable remaining");return;}
    const FVector Eye=Player->P2Camera->GetComponentLocation(),End=Player->P2Camera->GetComponentTransform().TransformPosition(FVector(42,10,-16));
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationThrowSpace),false,Player.Get());if(Holder && Holder->GetChildActor())Q.AddIgnoredActor(Holder->GetChildActor());
    if(GetWorld()->SweepSingleByChannel(Hit,Eye,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(6),Q)){LastAction=TEXT("No room to prepare throw");return;}
    auto* Item=GetWorld()->SpawnActorDeferred<AFoundationThrowable>(AFoundationThrowable::StaticClass(),FTransform(Player->P2Camera->GetComponentRotation(),End),Player.Get(),Player.Get(),ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Item)return;Item->Kind=SelectedThrowable;UGameplayStatics::FinishSpawningActor(Item,Item->GetActorTransform());
    Item->HoldAt(Player.Get(),Player->P2Camera);HeldThrowable=Item;--Player->InteractionManager->OwnedItems.FindOrAdd(FName(Id));
    HoldStarted=GetWorld()->GetTimeSeconds();AimHeld=FireHeld=false;BurstRemaining=0;
    if(Holder && Holder->GetChildActor())Holder->GetChildActor()->SetActorHiddenInGame(true);
    LastAction=TEXT("Throwable held; fuse runs after arming");UpdatePreview();
}
FVector UFoundationEquipment::ThrowVelocity() const
{
    if(!Player.IsValid() || !Player->P2Camera)return FVector::ZeroVector;
    const bool Knife=SelectedThrowable==EFoundationThrowableKind::Knife;
    const float Speed=Knife?2000.f:FMath::Lerp(650.f,1400.f,FMath::Clamp((GetWorld()->GetTimeSeconds()-HoldStarted)/1.2f,0.f,1.f));
    return Player->P2Camera->GetForwardVector()*Speed+FVector::UpVector*(Knife?60.f:220.f);
}
void UFoundationEquipment::ReleaseThrow()
{
    if(!HeldThrowable)return;if(!HeldThrowable->Detonated){HeldThrowable->Launch(ThrowVelocity());++ThrowsReleased;}
    HeldThrowable=nullptr;HidePreview();SetWeaponVisible(!Holstered);LastAction=TEXT("Throwable released");
}
void UFoundationEquipment::HidePreview(){for(const auto& Segment:PreviewSegments)if(Segment)Segment->SetVisibility(false);}
void UFoundationEquipment::UpdatePreview()
{
    if(!HeldThrowable || !Player.IsValid())return;
    FPredictProjectilePathParams Params(3,HeldThrowable->Body->GetComponentLocation(),ThrowVelocity(),2,ECC_Visibility,Player.Get());
    Params.ActorsToIgnore.Add(HeldThrowable);if(Holder && Holder->GetChildActor())Params.ActorsToIgnore.Add(Holder->GetChildActor());Params.SimFrequency=15;Params.bTraceComplex=false;
    FPredictProjectilePathResult Result;UGameplayStatics::PredictProjectilePath(this,Params,Result);
    const int32 Num=Result.PathData.Num(),Stride=FMath::Max(1,FMath::CeilToInt(float(Num-1)/12));
    for(int32 I=0;I<PreviewSegments.Num();++I)
    {
        auto* Segment=PreviewSegments[I].Get();const int32 A=I*Stride,B=FMath::Min((I+1)*Stride,Num-1);
        if(A>=B || !Segment){if(Segment)Segment->SetVisibility(false);continue;}
        const FVector Start=Result.PathData[A].Location,End=Result.PathData[B].Location,Delta=End-Start;
        Segment->SetWorldLocationAndRotation((Start+End)*.5,FRotationMatrix::MakeFromZ(Delta).Rotator());Segment->SetWorldScale3D(FVector(.006,.006,Delta.Size()/100));Segment->SetVisibility(true);
    }
}
