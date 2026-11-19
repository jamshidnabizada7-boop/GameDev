#include "FoundationAI.h"
#include "FoundationBridge.h"
#include "FoundationEquipment.h"
#include "FoundationSession.h"
#include "FoundationThrowables.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Components/ChildActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Builders/CubeBuilder.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Model.h"
#endif

AFoundationAIController::AFoundationAIController()
{
    Senses=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Senses"));SetPerceptionComponent(*Senses);
    Sight=CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));Sight->SightRadius=1600;Sight->LoseSightRadius=2000;Sight->PeripheralVisionAngleDegrees=55;Sight->SetMaxAge(3);
    Sight->DetectionByAffiliation.bDetectEnemies=true;Sight->DetectionByAffiliation.bDetectNeutrals=true;Sight->DetectionByAffiliation.bDetectFriendlies=false;
    Hearing=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));Hearing->HearingRange=2200;Hearing->SetMaxAge(4);
    Hearing->DetectionByAffiliation.bDetectEnemies=true;Hearing->DetectionByAffiliation.bDetectNeutrals=true;Hearing->DetectionByAffiliation.bDetectFriendlies=true;
    Senses->ConfigureSense(*Sight);Senses->ConfigureSense(*Hearing);Senses->SetDominantSense(Sight->GetSenseImplementation());
}
void AFoundationAIController::OnPossess(APawn* PossessedPawn)
{
    Super::OnPossess(PossessedPawn);if(auto* N=Cast<AFoundationNPC>(PossessedPawn))
    {SetGenericTeamId(N->GetGenericTeamId());Sight->SightRadius=N->SightRadius;Sight->LoseSightRadius=N->SightRadius+400;Hearing->HearingRange=N->HearingRadius;Senses->ConfigureSense(*Sight);Senses->ConfigureSense(*Hearing);}
    Senses->OnTargetPerceptionUpdated.AddUniqueDynamic(this,&AFoundationAIController::Perceived);
}
void AFoundationAIController::Perceived(AActor* Actor,FAIStimulus Stimulus)
{if(auto* N=Cast<AFoundationNPC>(GetPawn()))N->NotifyPerception(Actor,Stimulus);}

AFoundationNPC::AFoundationNPC(const FObjectInitializer& Initializer):Super(Initializer)
{
    AIControllerClass=AFoundationAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCapsuleComponent()->InitCapsuleSize(34,96);bUseControllerRotationYaw=false;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    GetMesh()->SetSkeletalMesh(Manny.Object);GetMesh()->SetRelativeLocation(FVector(0,0,-96));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    AIEyes=CreateDefaultSubobject<UCameraComponent>(TEXT("AIEyes"));AIEyes->SetupAttachment(GetMesh(),TEXT("head"));AIEyes->SetAbsolute(false,true,false);AIEyes->bAutoActivate=false;
    AIWeaponMount=CreateDefaultSubobject<USceneComponent>(TEXT("AIWeaponMount"));AIWeaponMount->SetupAttachment(GetRootComponent());AIWeaponMount->SetRelativeLocation(FVector(35,12,65));
    AIWeapon=CreateDefaultSubobject<UChildActorComponent>(TEXT("AIWeapon"));AIWeapon->SetupAttachment(AIWeaponMount);
    static ConstructorHelpers::FClassFinder<AActor> Weapon(TEXT("/Game/AfghanFPS/Weapons/BP_Weapon_Base"));if(Weapon.Succeeded())AIWeapon->SetChildActorClass(Weapon.Class);
    StateLabel=CreateDefaultSubobject<UTextRenderComponent>(TEXT("AwarenessLabel"));StateLabel->SetupAttachment(GetRootComponent());StateLabel->SetWorldSize(15);StateLabel->SetHorizontalAlignment(EHTA_Center);
    FootAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("Footsteps"));FootAudio->SetupAttachment(GetRootComponent());FootAudio->bAutoActivate=false;
    DragHandle=CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("BodyDrag"));
    GunFlash=CreateDefaultSubobject<UPointLightComponent>(TEXT("GunFlash"));GunFlash->SetupAttachment(AIWeaponMount);GunFlash->SetRelativeLocation(FVector(59,0,2.5));GunFlash->SetIntensity(1000);GunFlash->SetAttenuationRadius(120);GunFlash->SetCastShadows(false);GunFlash->SetVisibility(false);
}
bool AFoundationNPC::AvoidGrenade(float Now)
{
    if(Now>=NextGrenadeScan && !GrenadeThreat.IsValid())
    {
        NextGrenadeScan=Now+.15f;
        FVector Eye;FRotator View;GetActorEyesViewPoint(Eye,View);
        for(TActorIterator<AFoundationThrowable> It(GetWorld());It;++It)
        {
            if(It->Kind!=EFoundationThrowableKind::Frag || !It->Launched || It->RemainingFuse<=0 || It->Detonated)continue;
            const FVector Delta=It->GetActorLocation()-Eye;
            if(Delta.SizeSquared()>FMath::Square(900.f) || FVector::DotProduct(View.Vector(),Delta.GetSafeNormal())<-.2f)continue;
            FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationGrenadeSight),false,this);Q.AddIgnoredActor(*It);
            if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,It->GetActorLocation(),ECC_Visibility,Q))continue;
            GrenadeThreat=*It;NextGrenadePath=0;break;
        }
    }
    auto* Threat=GrenadeThreat.Get();
    if(!Threat || Threat->Detonated || Threat->RemainingFuse<=0)
    {
        GrenadeThreat.Reset();
        if(EvadingGrenade){EvadingGrenade=false;GetCharacterMovement()->MaxWalkSpeed=210;NextMove=0;}
        return false;
    }
    EvadingGrenade=true;GetCharacterMovement()->MaxWalkSpeed=600;
    if(auto* AI=Cast<AAIController>(GetController()))AI->ClearFocus(EAIFocusPriority::Gameplay);
    if(Now>=NextGrenadePath)
    {
        NextGrenadePath=Now+.45f;
        auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        FVector Best=FVector::ZeroVector;float Score=-1;
        const FVector Away=(GetActorLocation()-Threat->GetActorLocation()).GetSafeNormal2D();
        for(int32 Index=0;Nav && Index<12;++Index)
        {
            const FVector Candidate=GetActorLocation()+FRotator(0,Index*30.f,0).RotateVector(Away)*750;
            FNavLocation Point;
            if(!Nav->ProjectPointToNavigation(Candidate,Point,FVector(140,140,250)))continue;
            const float Safety=FVector::Dist2D(Point.Location,Threat->GetActorLocation());
            if(Safety<650)continue;
            const auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(this,GetActorLocation(),Point.Location,this);
            if(!Path || !Path->IsValid() || Path->IsPartial())continue;
            const float Value=Safety-.5f*Path->GetPathLength();
            if(Value>Score){Score=Value;Best=Point.Location;}
        }
        if(Score>=0 && MoveToPoint(Best)){++GrenadeEscapes;LastDecision=TEXT("Escaping visible live grenade along complete navigation path");}
        else LastDecision=TEXT("Live grenade seen; no safe reachable destination");
    }
    return true;
}

void AFoundationNPC::BeginPlay()
{
    Super::BeginPlay();
    // Only count NPCs admitted to this world. Placed actors may already be
    // iterable before their BeginPlay, and editor worlds can coexist with PIE.
    int32 Admitted=0;
    for(TActorIterator<AFoundationNPC> It(GetWorld());It;++It)
        if(It->bOccupiesPopulationSlot && !It->IsActorBeingDestroyed()) ++Admitted;
    if(Admitted>=10)
    {
        UE_LOG(LogTemp,Display,TEXT("Foundation NPC population full; skipped %s"),*GetName());
        Destroy();
        return;
    }
    bOccupiesPopulationSlot=true;
    SpawnTransform=GetActorTransform();if(StableId.IsNone())StableId=GetFName();
    Equipment->SetComponentTickEnabled(false);InteractionManager->SetComponentTickEnabled(false);GetMesh()->SetOwnerNoSee(false);GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    P2ThirdPersonHands=true;GetCharacterMovement()->MaxWalkSpeed=210;GetCharacterMovement()->bOrientRotationToMovement=true;
    HealthComponent=FoundationBridge::Health(this);
    if(!HealthComponent)
    {if(UClass* C=LoadClass<UActorComponent>(nullptr,TEXT("/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C"))){HealthComponent=NewObject<UActorComponent>(this,C,TEXT("BPC_Health"));AddInstanceComponent(HealthComponent);HealthComponent->RegisterComponent();}}
    FoundationBridge::Call(HealthComponent,TEXT("InitializeHealth"),{StartingHealth});
    FootAudio->SetSound(LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/FoundationUse.FoundationUse")));
    FSoundAttenuationSettings Att;Att.bAttenuate=true;Att.bSpatialize=true;Att.AttenuationShape=EAttenuationShape::Sphere;Att.AttenuationShapeExtents=FVector(100);Att.FalloffDistance=1400;FootAudio->AdjustAttenuation(Att);
    SetAIEnabled(AIEnabled);
    Enter(PatrolRoute.Num()?EFoundationAIState::Patrol:EFoundationAIState::Idle,1);
}
float AFoundationNPC::HealthValue() const{return float(FoundationBridge::Number(HealthComponent,TEXT("CurrentHealth"),StartingHealth));}
bool AFoundationNPC::IsThreat(AActor* Actor) const
{
    if(auto* N=Cast<AFoundationNPC>(Actor))return N!=this && N->Friendly!=Friendly && !N->bIsDead
        && N->State!=EFoundationAIState::Downed && N->State!=EFoundationAIState::Restrained && N->State!=EFoundationAIState::Hostage;
    if(auto* P=Cast<APhase2Character>(Actor))return !Friendly && P->IsPlayerControlled() && !P->P2IsDead();return false;
}
void AFoundationNPC::GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const
{
    Location=GetMesh()->GetSocketLocation(TEXT("head"))+GetActorForwardVector()*4;
    Rotation=State==EFoundationAIState::Combat || State==EFoundationAIState::Stagger?(LastKnownLocation+FVector(0,0,45)-Location).Rotation():GetActorRotation();
}
bool AFoundationNPC::CanSee(AActor* Actor) const
{
    if(!IsValid(Actor) || Actor->IsHidden() || FVector::DistSquared(GetActorLocation(),Actor->GetActorLocation())>FMath::Square(SightRadius+400))return false;
    FVector Start;FRotator R;GetActorEyesViewPoint(Start,R);const FVector End=Actor->GetActorLocation()+FVector(0,0,45);
    for(TActorIterator<AFoundationThrowable> It(GetWorld());It;++It)if(It->Kind==EFoundationThrowableKind::Smoke && It->Detonated && FMath::PointDistToSegmentSquared(It->Body->GetComponentLocation(),Start,End)<FMath::Square(It->SmokeRadius))return false;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationAISight),false,this);if(AIWeapon->GetChildActor())Q.AddIgnoredActor(AIWeapon->GetChildActor());
    return !GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Q) || Hit.GetActor()==Actor;
}
void AFoundationNPC::NotifyPerception(AActor* Actor,const FAIStimulus& Stimulus)
{
    if(!AIEnabled || bIsDead || Context!=EContext::None || State==EFoundationAIState::Surrender
        || State==EFoundationAIState::Downed || State==EFoundationAIState::Restrained || !Actor)return;
    if(Stimulus.Type==UAISense::GetSenseID<UAISense_Sight>())
    {
        if(!IsThreat(Actor))
        {
            auto* Other=Cast<AFoundationNPC>(Actor);
            if(Other && Other->Friendly==Friendly && !Other->BodyHidden
                && (Other->IsBody() || Other->State==EFoundationAIState::Hostage)
                && Stimulus.WasSuccessfullySensed() && !SeesTarget)
            {
                LastKnownLocation=Other->UsePoint();Awareness=FMath::Max(Awareness,.5f);
                Enter(EFoundationAIState::Investigate,8);NextMove=0;
            }
            return;
        }
        if(Stimulus.WasSuccessfullySensed()){Target=Actor;HasSightCandidate=true;LastKnownLocation=Stimulus.StimulusLocation;LastDecision=TEXT("Sight candidate");}
        else if(Target==Actor)HasSightCandidate=false;
    }
    else if(Stimulus.Type==UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
    {
        if(auto* Other=Cast<AFoundationNPC>(Actor))if(Other->Friendly==Friendly && Stimulus.Tag!=TEXT("Gunshot") && Stimulus.Tag!=TEXT("Explosion"))return;
        if(Actor==this)return;++HeardEvents;LastHeardLocation=Stimulus.StimulusLocation;LastNoiseTag=Stimulus.Tag;
        if(!SeesTarget && State!=EFoundationAIState::Stagger && State!=EFoundationAIState::Stunned && State!=EFoundationAIState::Cover)
        {LastKnownLocation=LastHeardLocation;Awareness=FMath::Max(Awareness,.35f);Enter(EFoundationAIState::Investigate,10);NextMove=0;LastDecision=TEXT("Investigating actual stimulus location");}
    }
}
void AFoundationNPC::Enter(EFoundationAIState NewState,float Seconds)
{State=NewState;Until=GetWorld()->GetTimeSeconds()+Seconds;if(NewState!=EFoundationAIState::Combat)if(auto* C=Cast<AAIController>(GetController()))C->ClearFocus(EAIFocusPriority::Gameplay);}
void AFoundationNPC::SetAIEnabled(bool Enabled)
{
    AIEnabled=Enabled;if(auto* C=Cast<AFoundationAIController>(GetController()))
    {C->Senses->SetSenseEnabled(UAISense_Sight::StaticClass(),Enabled);C->Senses->SetSenseEnabled(UAISense_Hearing::StaticClass(),Enabled);if(!Enabled){C->StopMovement();C->ClearFocus(EAIFocusPriority::Gameplay);C->Senses->ForgetAll();}}
    if(!Enabled){HasSightCandidate=false;SeesTarget=false;Target=nullptr;Awareness=0;}
}
bool AFoundationNPC::MoveToPoint(FVector Point)
{
    auto* C=Cast<AAIController>(GetController());auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());FNavLocation Projected;
    if(!C || !Nav || !Nav->ProjectPointToNavigation(Point,Projected,FVector(120,120,220))){LastMoveAccepted=false;return false;}
    MoveGoal=Projected.Location;LastMoveAccepted=C->MoveToLocation(MoveGoal,30,false,true,false,true,nullptr,false)!=EPathFollowingRequestResult::Failed;return LastMoveAccepted;
}
bool AFoundationNPC::ChooseCover()
{
    FVector Best=FVector::ZeroVector;float Score=TNumericLimits<float>::Max();bool Found=false;
    for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("AI_Cover")))
    {
        const FVector Point=It->GetActorLocation();FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationCover),false,this);if(Target)Q.AddIgnoredActor(Target);
        const bool Covered=GetWorld()->LineTraceSingleByChannel(Hit,LastKnownLocation+FVector(0,0,50),Point+FVector(0,0,125),ECC_Visibility,Q);
        const float D=FVector::DistSquared(GetActorLocation(),Point);if(Covered && D<Score){Score=D;Best=Point;Found=true;}
    }
    if(Found && MoveToPoint(Best)){++CoverSelections;Enter(EFoundationAIState::Cover,5);LastDecision=TEXT("Moving to physically occluded cover");return true;}return false;
}
void AFoundationNPC::FireAtTarget()
{
    const float Now=GetWorld()->GetTimeSeconds();if(Now<NextFire || !SeesTarget || !IsThreat(Target) || !CanSee(Target) || State!=EFoundationAIState::Combat || HoldFire)return;
    auto* W=AIWeapon->GetChildActor();if(!W)return;
    FVector Eye;FRotator Aim;GetActorEyesViewPoint(Eye,Aim);
    FHitResult Block;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationAIMuzzle),false,this);Q.AddIgnoredActor(W);
    if(GetWorld()->SweepSingleByChannel(Block,Eye,GunFlash->GetComponentLocation(),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(2),Q)){++BlockedShots;NextFire=Now+.3f;return;}
    const int32 Ammo=int32(FoundationBridge::Number(W,TEXT("CurrentAmmo"))),Reserve=int32(FoundationBridge::Number(W,TEXT("ReserveAmmo")));
    if(Ammo<=0)
    {
        if(Reserve<=0){Surrender();return;}
        if(!Reloading){Reloading=true;NextFire=Now+1.5f;return;}
        const int32 Transfer=FMath::Min(30,Reserve);FoundationBridge::SetNumber(W,TEXT("CurrentAmmo"),Transfer);FoundationBridge::SetNumber(W,TEXT("ReserveAmmo"),Reserve-Transfer);Reloading=false;NextFire=Now+.3f;return;
    }
    const int32 Before=Ammo;FoundationBridge::Call(W,TEXT("TryFire"));if(FoundationBridge::Number(W,TEXT("CurrentAmmo"))>=Before)return;
    ++ShotsFired;NextFire=Now+.8f;GunFlash->SetVisibility(true);FlashUntil=Now+.04f;P2Recoil=1;
    if(auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Shot.Shot")))UGameplayStatics::PlaySoundAtLocation(this,S,GunFlash->GetComponentLocation(),.65f);
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),GunFlash->GetComponentLocation(),1,this,3000,TEXT("Gunshot"));
}
void AFoundationNPC::DropAIWeapon()
{
    auto* W=AIWeapon->GetChildActor();if(!W)return;
    auto* Item=GetWorld()->SpawnActorDeferred<AFoundationInteractable>(AFoundationInteractable::StaticClass(),FTransform(GetActorRotation(),GetActorLocation()+GetActorForwardVector()*70-FVector(0,0,96)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(Item){Item->Kind=EFoundationInteractableKind::Weapon;Item->ItemId=TEXT("weapon_rifle_base");Item->ItemAmount=1;Item->WeaponMagazine=int32(FoundationBridge::Number(W,TEXT("CurrentAmmo")));Item->WeaponReserve=int32(FoundationBridge::Number(W,TEXT("ReserveAmmo")));Item->StableId=FName(*FString::Printf(TEXT("RuntimeAIWeapon_%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));Item->Tags.Add(TEXT("RuntimeItem"));UGameplayStatics::FinishSpawningActor(Item,Item->GetActorTransform());}
    AIWeapon->SetChildActorClass(nullptr);P2GripLeft=P2GripRight=nullptr;
}
void AFoundationNPC::Surrender()
{
    if(bIsDead || State==EFoundationAIState::Surrender)return;DropAIWeapon();if(auto* C=Cast<AAIController>(GetController())){C->StopMovement();C->ClearFocus(EAIFocusPriority::Gameplay);}
    P2SetStance(1);GetCharacterMovement()->DisableMovement();Target=nullptr;SeesTarget=HasSightCandidate=false;Enter(EFoundationAIState::Surrender);LastDecision=TEXT("Surrendered; firearm dropped");
}
void AFoundationNPC::Die()
{
    if(bIsDead)return;bIsDead=true;ReleaseContext();DeathAt=GetWorld()->GetTimeSeconds();Enter(EFoundationAIState::Dead);Target=nullptr;SeesTarget=HasSightCandidate=false;GunFlash->SetVisibility(false);
    if(auto* C=Cast<AAIController>(GetController())){C->StopMovement();C->ClearFocus(EAIFocusPriority::Gameplay);}GetCharacterMovement()->DisableMovement();GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(AIWeapon->GetChildActor())AIWeapon->GetChildActor()->SetActorHiddenInGame(true);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);GetMesh()->SetAllBodiesSimulatePhysics(true);GetMesh()->SetSimulatePhysics(true);GetMesh()->WakeAllRigidBodies();StateLabel->SetVisibility(false);
}
float AFoundationNPC::TakeDamage(float Damage,const FDamageEvent& Event,AController* EventInstigator,AActor* Causer)
{
    if(bIsDead || !HealthComponent || Damage<=0)return 0;
    const float Before=HealthValue();
    const bool Downing=Friendly && CanBeRevived && State!=EFoundationAIState::Downed && Damage>=Before;
    if(Downing){Down();return Damage;}
    const float Result=Super::TakeDamage(Damage,Event,EventInstigator,Causer);
    if(Result<=0)return 0;
    // Native characters do not have the legacy Blueprint AnyDamage graph.
    // Preserve derived Blueprint handlers that already forwarded this event.
    if(HealthValue()==Before)FoundationBridge::Call(HealthComponent,TEXT("ApplyDamage"),{Result});
    if(HealthValue()<=0){Die();return Result;}
    if(Context!=EContext::None)ReleaseContext();
    if(State==EFoundationAIState::Surrender || State==EFoundationAIState::Restrained || State==EFoundationAIState::Downed)return Result;
    P2StaggerAlpha=1;LastDecision=TEXT("Hit stagger interrupts attack");Enter(EFoundationAIState::Stagger,.25f+FMath::Clamp(Damage/100,.0f,.5f));NextFire=FMath::Max(NextFire,Until);
    if(EventInstigator && IsThreat(EventInstigator->GetPawn()) && CanSee(EventInstigator->GetPawn())){Target=EventInstigator->GetPawn();HasSightCandidate=true;LastKnownLocation=Target->GetActorLocation();Awareness=1;}
    return Result;
}
bool AFoundationNPC::ResetForDrill()
{
    if(!bIsDead || Context!=EContext::None)return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationRespawn),false,this);
    if(GetWorld()->OverlapBlockingTestByChannel(SpawnTransform.GetLocation(),SpawnTransform.GetRotation(),ECC_Pawn,FCollisionShape::MakeCapsule(34,96),Q))return false;
    GetMesh()->SetAllBodiesSimulatePhysics(false);GetMesh()->SetSimulatePhysics(false);GetMesh()->AttachToComponent(GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-96),FRotator(0,-90,0));GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));SetActorTransform(SpawnTransform,false,nullptr,ETeleportType::TeleportPhysics);bIsDead=false;BodyHidden=false;SetActorHiddenInGame(false);
    if(auto* P=FindFProperty<FBoolProperty>(HealthComponent->GetClass(),TEXT("bIsDead")))P->SetPropertyValue_InContainer(HealthComponent,false);FoundationBridge::Call(HealthComponent,TEXT("InitializeHealth"),{StartingHealth});
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);P2SetStance(0);Awareness=0;Target=nullptr;HasSightCandidate=false;StateLabel->SetVisibility(true);
    if(AIWeapon->GetChildActor())AIWeapon->GetChildActor()->SetActorHiddenInGame(false);
    if(auto* C=Cast<AFoundationAIController>(GetController()))C->Senses->ForgetAll();
    Enter(EFoundationAIState::Idle,1);return true;
}
bool AFoundationNPC::IssueOrder(EFoundationSquadOrder Order,FVector Point,AActor* Actor)
{
    if(!Friendly || bIsDead || State==EFoundationAIState::Downed || !AIEnabled || Context!=EContext::None)return false;
    if(Order==EFoundationSquadOrder::Focus && !IsThreat(Actor))return false;
    if(Order==EFoundationSquadOrder::Move && !MoveToPoint(Point))return false;
    if(Order==EFoundationSquadOrder::Cover){LastKnownLocation=Point;if(!ChooseCover())return false;}
    SquadOrder=Order;OrderPoint=Point;HoldFire=Order==EFoundationSquadOrder::CeaseFire;
    if(Order==EFoundationSquadOrder::Follow)Leader=Actor;
    if(Order==EFoundationSquadOrder::Focus && IsThreat(Actor)){Target=Actor;LastKnownLocation=Point;HasSightCandidate=true;Awareness=1;}
    if(Order==EFoundationSquadOrder::Move || Order==EFoundationSquadOrder::Follow)Enter(EFoundationAIState::Idle);
    if(Order==EFoundationSquadOrder::Hold || Order==EFoundationSquadOrder::CeaseFire)
    {
        if(auto* C=Cast<AAIController>(GetController()))C->StopMovement();
        if(State!=EFoundationAIState::Combat)Enter(EFoundationAIState::Idle);
    }
    LastDecision=TEXT("Squad order accepted");return true;
}
void AFoundationNPC::StepSound()
{
    ++Footsteps;bool Muffled=false,Listening=false;auto* Player=UGameplayStatics::GetPlayerPawn(this,0);
    if(Player)for(TActorIterator<AFoundationInteractable> It(GetWorld());It;++It)if(It->Kind==EFoundationInteractableKind::Door && It->DoorAngle<45 && FVector::Dist(It->GetActorLocation(),GetActorLocation())<500 && FVector::Dist(It->GetActorLocation(),Player->GetActorLocation())<350)
    {
        const FVector F=It->GetActorForwardVector();if(FVector::DotProduct(GetActorLocation()-It->GetActorLocation(),F)*FVector::DotProduct(Player->GetActorLocation()-It->GetActorLocation(),F)<0){Muffled=true;Listening=It->Listening;break;}
    }
    FootAudio->SetLowPassFilterEnabled(Muffled && !Listening);FootAudio->SetLowPassFilterFrequency(1000);FootAudio->SetVolumeMultiplier(Muffled?(Listening?.6f:.2f):.3f);FootAudio->SetPitchMultiplier(.55f);FootAudio->Play();
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),.2f,this,500,TEXT("Footstep"));
}
void AFoundationNPC::Tick(float Dt)
{
    // Use ACharacter's tick: the playable parent's tick intentionally hides its
    // parked preview when not player-controlled. NPCs share its pose/movement,
    // not that preview or local-player equipment behavior.
    ACharacter::Tick(Dt);const float Now=GetWorld()->GetTimeSeconds();if(!HealthComponent)return;
    UpdateContext(Dt);
    if(HealthValue()<=0)Die();if(bIsDead){if(RespawnDelay>0 && Now-DeathAt>RespawnDelay)ResetForDrill();return;}
    P2PoseCrouch=FMath::FInterpTo(P2PoseCrouch,P2Stance==1?1.f:0.f,Dt,10);P2SurrenderAlpha=FMath::FInterpTo(P2SurrenderAlpha,State==EFoundationAIState::Surrender?1.f:0.f,Dt,8);P2StaggerAlpha=FMath::FInterpTo(P2StaggerAlpha,0.f,Dt,8);
    P2PoseProne=FMath::FInterpTo(P2PoseProne,P2Stance==2?1.f:0.f,Dt,10);
    AIEyes->SetWorldRotation(FRotator(0,GetActorRotation().Yaw,0));if(Now>=FlashUntil)GunFlash->SetVisibility(false);
    StepDistance+=GetVelocity().Size2D()*Dt;if(StepDistance>130 && !GetCharacterMovement()->IsFalling()){StepDistance=0;StepSound();}
    if(Now>=NextLabel)
    {
        NextLabel=Now+.2f;
        FVector Eye;FRotator Rotation;GetActorEyesViewPoint(Eye,Rotation);
        StateLabel->SetWorldLocation(Eye+FVector(0,0,30));
        bool HighContrast=false;
        if(const auto* PC=UGameplayStatics::GetPlayerController(this,0))
        {
            if(PC->PlayerCameraManager)
                StateLabel->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation()-Eye).Rotation());
            const auto* Player=Cast<APhase2Character>(PC->GetPawn());
            HighContrast=Player && Player->FoundationSession && Player->FoundationSession->Preferences
                && Player->FoundationSession->Preferences->HighContrast;
        }
        const TCHAR* Name=State==EFoundationAIState::Combat?TEXT("Engaging"):State==EFoundationAIState::Investigate?TEXT("Investigating"):State==EFoundationAIState::Search?TEXT("Searching"):State==EFoundationAIState::Cover?TEXT("Taking cover"):State==EFoundationAIState::Surrender?TEXT("Surrendered"):TEXT("Patrolling");
        const TCHAR* RoleLabel=HighContrast?(Friendly?TEXT("ALLY | "):TEXT("OPPONENT | ")):TEXT("");
        StateLabel->SetText(FText::FromString(FString::Printf(TEXT("%s%s  HP %.0f  %.0f%%"),RoleLabel,Name,HealthValue(),Awareness*100)));
        StateLabel->SetWorldSize(HighContrast?19.f:15.f);
        StateLabel->SetTextRenderColor(HighContrast?FColor::White:
            (State==EFoundationAIState::Combat?FColor::Red:Awareness>.2f?FColor::Yellow:FColor::White));
    }
    if(!AIEnabled || Context!=EContext::None || State==EFoundationAIState::Surrender || State==EFoundationAIState::Restrained || State==EFoundationAIState::Downed)return;
    for(TActorIterator<AFoundationThrowable> It(GetWorld());It;++It)if(It->Kind==EFoundationThrowableKind::Flash && It->Detonated && Now-It->DetonatedAt<.25f && FVector::DistSquared(It->GetActorLocation(),GetActorLocation())<FMath::Square(700.f))
    {FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationAIFlash),false,this);Q.AddIgnoredActor(*It);FVector E;FRotator R;GetActorEyesViewPoint(E,R);if(!GetWorld()->LineTraceSingleByChannel(H,E,It->GetActorLocation(),ECC_Visibility,Q))Enter(EFoundationAIState::Stunned,2.5f);}
    if(State==EFoundationAIState::Stagger || State==EFoundationAIState::Stunned){const bool HitStagger=State==EFoundationAIState::Stagger;if(auto* C=Cast<AAIController>(GetController()))C->StopMovement();if(Now<Until)return;if(!HitStagger || HealthValue()>=StartingHealth*.8f || !ChooseCover())Enter(EFoundationAIState::Search,5);}
    if(AvoidGrenade(Now))return;
    SeesTarget=HasSightCandidate && IsThreat(Target) && CanSee(Target);
    if(SeesTarget)
    {LastKnownLocation=Target->GetActorLocation();LastSeen=Now;float Factor=1;if(auto* P=Cast<APhase2Character>(Target.Get()))Factor=P->P2Stance==2?.4f:P->P2Stance==1?.65f:1;Awareness=FMath::Clamp(Awareness+Dt*Factor/FMath::Max(.2f,DetectionSeconds),0.f,1.f);if(Awareness>=1 && State!=EFoundationAIState::Cover)Enter(EFoundationAIState::Combat);}
    else Awareness=FMath::Max(0.f,Awareness-Dt*.08f);
    if(!Friendly)
    {
        auto* P=Cast<APhase2Character>(UGameplayStatics::GetPlayerPawn(this,0));
        if(P && !P->P2IsDead() && P->Equipment && P->Equipment->AdsAlpha>.8f && FVector::DistSquared(P->GetActorLocation(),GetActorLocation())<FMath::Square(350.f))
        {const FVector To=(P->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();FVector Eye;FRotator R;P->GetActorEyesViewPoint(Eye,R);if(FVector::DotProduct(GetActorForwardVector(),To)<-.45f && FVector::DotProduct(R.Vector(),(GetMesh()->GetSocketLocation(TEXT("head"))-Eye).GetSafeNormal())>.95f && CanSee(P)){Surrender();return;}}
        if(HealthValue()<StartingHealth*.25f && Awareness>.5f){Surrender();return;}
    }
    if(Friendly && SquadOrder==EFoundationSquadOrder::Follow && Leader && Now>=NextMove && FVector::Dist2D(GetActorLocation(),Leader->GetActorLocation())>250){MoveToPoint(Leader->GetActorLocation());NextMove=Now+.7f;}
    if(Friendly && SquadOrder==EFoundationSquadOrder::Move && FVector::Dist2D(GetActorLocation(),OrderPoint)<90)SquadOrder=EFoundationSquadOrder::Hold;
    switch(State)
    {
    case EFoundationAIState::Idle:
        if(!Friendly && Now>=Until && PatrolRoute.Num()){Enter(EFoundationAIState::Patrol);NextMove=0;}break;
    case EFoundationAIState::Patrol:
        if(PatrolRoute.Num() && !Friendly){const FVector Goal=PatrolRoute[RouteIndex%PatrolRoute.Num()];if(FVector::Dist2D(GetActorLocation(),Goal)<70){++PatrolArrivals;RouteIndex=(RouteIndex+1)%PatrolRoute.Num();Enter(EFoundationAIState::Idle,1);if(auto* C=Cast<AAIController>(GetController()))C->StopMovement();}else if(Now>=NextMove){MoveToPoint(Goal);NextMove=Now+1;}}break;
    case EFoundationAIState::Combat:
        GetCharacterMovement()->MaxWalkSpeed=350;
        if(!SeesTarget){Enter(EFoundationAIState::Search,6);NextMove=0;break;}
        if(auto* C=Cast<AAIController>(GetController()))C->SetFocus(Target);
        FireAtTarget();if(!Friendly && FVector::Dist2D(GetActorLocation(),LastKnownLocation)>900 && Now>=NextMove){MoveToPoint(LastKnownLocation);NextMove=Now+1;}break;
    case EFoundationAIState::Investigate:case EFoundationAIState::Search:
        if(Now>=Until){Target=nullptr;HasSightCandidate=false;Awareness=0;Enter(EFoundationAIState::Idle,1);break;}
        if(!Friendly && Now>=NextMove){MoveToPoint(LastKnownLocation);NextMove=Now+1;}
        if(FVector::Dist2D(GetActorLocation(),LastKnownLocation)<100){if(auto* C=Cast<AAIController>(GetController()))C->StopMovement();AddActorWorldRotation(FRotator(0,Dt*25,0));}break;
    case EFoundationAIState::Cover:
        if(Now>=Until){P2SetStance(0);Enter(EFoundationAIState::Search,5);NextMove=0;}
        else if(FVector::Dist2D(GetActorLocation(),MoveGoal)<90){if(auto* C=Cast<AAIController>(GetController()))C->StopMovement();P2SetStance(1);}
        break;
    default:break;
    }
    // An unlocked door can be requested through the same interface contract.
    if(GetVelocity().Size2D()<10 && State!=EFoundationAIState::Idle && State!=EFoundationAIState::Combat)
        for(TActorIterator<AFoundationInteractable> It(GetWorld());It;++It)if(It->Kind==EFoundationInteractableKind::Door && !It->Locked && It->DoorAngle<20 && FVector::Dist2D(GetActorLocation(),It->GetActorLocation())<180)
        {IFoundationInteractableInterface::Execute_ReceiveUse(*It,this,EFoundationUsePhase::Begin,0);IFoundationInteractableInterface::Execute_ReceiveUse(*It,this,EFoundationUsePhase::End,0);break;}
}
FString UFoundationAITools::BuildArenaNavigation(UObject* Context)
{
#if WITH_EDITOR
    UWorld* World=Context?Context->GetWorld():nullptr;if(!World || !GEditor || GEditor->PlayWorld)return TEXT("FAILED: editor world required");
    ANavMeshBoundsVolume* Volume=nullptr;for(TActorIterator<ANavMeshBoundsVolume> It(World);It;++It)if(It->ActorHasTag(TEXT("FoundationNav"))){Volume=*It;break;}
    if(!Volume){Volume=World->SpawnActor<ANavMeshBoundsVolume>();Volume->Tags.Add(TEXT("FoundationNav"));Volume->SetActorLabel(TEXT("FoundationNavBounds"));}
    if(!Volume->Brush){Volume->Brush=NewObject<UModel>(Volume,NAME_None,RF_Transactional);Volume->Brush->Initialize(Volume,true);Volume->GetBrushComponent()->Brush=Volume->Brush;}
    auto* Cube=NewObject<UCubeBuilder>();Cube->X=4800;Cube->Y=2200;Cube->Z=800;Cube->Hollow=false;UBrushBuilder* Builder=Cube;
    if(!Builder->Build(World,Volume))return TEXT("FAILED: volume brush build");
    Volume->SetActorLocation(FVector(1200,0,100));
    Volume->Brush->BuildBound();
    Volume->GetBrushComponent()->UpdateBounds();

    // The editor brush previews these extents, but its generated model can be
    // stripped by the cooker. Navigation queries the volume's component bounds.
    // Save a non-colliding runtime component so both builds use the same box.
    Volume->Modify();
    constexpr auto BoundsName=TEXT("FoundationRuntimeNavBounds");
    auto* RuntimeBounds=FindObject<UBoxComponent>(Volume,BoundsName);
    if(!RuntimeBounds)
    {
        RuntimeBounds=NewObject<UBoxComponent>(Volume,BoundsName,RF_Transactional);
        RuntimeBounds->SetupAttachment(Volume->GetRootComponent());
        Volume->AddInstanceComponent(RuntimeBounds);
    }
    RuntimeBounds->Modify();
    RuntimeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RuntimeBounds->SetCanEverAffectNavigation(false);
    RuntimeBounds->SetHiddenInGame(true);
    RuntimeBounds->SetRelativeTransform(FTransform::Identity);
    RuntimeBounds->SetBoxExtent(FVector(Cube->X,Cube->Y,Cube->Z)*.5f);
    if(!RuntimeBounds->IsRegistered())RuntimeBounds->RegisterComponent();
    Volume->PostEditChange();
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);if(!Nav)return TEXT("FAILED: no navigation system");Nav->OnNavigationBoundsUpdated(Volume);Nav->Build();World->MarkPackageDirty();
    return FString::Printf(TEXT("NAVIGATION_BUILD_REQUESTED bounds=%s"),*Volume->GetComponentsBoundingBox(true).ToString());
#else
    return TEXT("Editor-only navigation authoring");
#endif
}
TArray<FVector> UFoundationAITools::PathTo(APawn* Pawn,FVector Goal)
{
    TArray<FVector> Result;if(Pawn)if(auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(Pawn,Pawn->GetActorLocation(),Goal,Pawn))if(Path->IsValid())Result=Path->PathPoints;return Result;
}
