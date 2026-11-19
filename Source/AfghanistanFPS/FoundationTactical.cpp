#include "FoundationAI.h"
#include "FoundationBridge.h"
#include "FoundationEquipment.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Sound/SoundBase.h"

bool APhase2Character::CanUseContext() const
{
    return IsPlayerControlled() && !P2IsDead() && !P2Traversing && P2Stance!=2
        && !P2Sprinting && !TacticalTarget && !Equipment->IsHoldingThrowable()
        && !Equipment->Reloading && !Equipment->Equipping && !Equipment->Inspecting
        && !InteractionManager->MirrorActive && GetCharacterMovement()->IsMovingOnGround();
}

void APhase2Character::ShowTacticalCaption(const FText& Value)
{
    TacticalCaption=Value;
    CaptionUntil=GetWorld()->GetTimeSeconds()+2.5f;
}

static AActor* ContextAim(APhase2Character* Player, float Distance, FHitResult& Hit, bool IgnoreFriendlies=false)
{
    if(!Player->P2Camera)return nullptr;
    const FVector Start=Player->P2Camera->GetComponentLocation();
    const FVector End=Start+Player->P2Camera->GetForwardVector()*Distance;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundationTacticalAim),false,Player);
    if(auto* Weapon=FoundationBridge::Weapon(Player))Query.AddIgnoredActor(Weapon);
    if(IgnoreFriendlies)
        for(TActorIterator<AFoundationNPC> NPC(Player->GetWorld());NPC;++NPC)
            if(NPC->Friendly)
            {
                Query.AddIgnoredActor(*NPC);
                if(NPC->AIWeapon->GetChildActor())Query.AddIgnoredActor(NPC->AIWeapon->GetChildActor());
            }
    Player->GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);
    return Hit.GetActor();
}

void APhase2Character::Bash()
{
    const float Now=GetWorld()->GetTimeSeconds();
    if(!CanUseContext() || InteractionManager->Using || Now<NextBash)return;
    NextBash=Now+.65f;
    FHitResult Hit;
    if(AActor* Victim=ContextAim(this,150,Hit))
    {
        const AFoundationNPC* NPC=Cast<AFoundationNPC>(Victim);
        if(!NPC || !NPC->Friendly)
            UGameplayStatics::ApplyDamage(Victim,35,GetController(),this,UDamageType::StaticClass());
    }
    P2Recoil=.8f;
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),.4f,this,550,TEXT("Melee"));
}

void APhase2Character::GrabOrRelease()
{
    if(TacticalTarget && HoldingHostage){TacticalTarget->ReleaseContext();return;}
    if(!CanUseContext() || InteractionManager->Using)return;
    FHitResult Hit;
    if(auto* NPC=Cast<AFoundationNPC>(ContextAim(this,165,Hit)))
    {
        if(!NPC->TryGrab(this))ShowTacticalCaption(NSLOCTEXT("Tactical","GrabDenied","Cannot grab this target"));
    }
}

void APhase2Character::HideBody()
{
    if(TacticalTarget && TacticalTarget->TryHide(this))InteractionManager->EndUse();
    else if(TacticalTarget)ShowTacticalCaption(NSLOCTEXT("Tactical","HideDenied","Drag the body into a marked hiding area"));
}

void APhase2Character::Whistle()
{
    const float Now=GetWorld()->GetTimeSeconds();
    if(!CanUseContext() || InteractionManager->Using || Now<NextWhistle)return;
    NextWhistle=Now+3.f;
    auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Whistle.Whistle"));
    if(Sound)UGameplayStatics::PlaySoundAtLocation(this,Sound,GetActorLocation());
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),1,this,1500,TEXT("Whistle"));
}

void APhase2Character::OrderFollow(){SendSquadOrder(uint8(EFoundationSquadOrder::Follow));}
void APhase2Character::OrderHold(){SendSquadOrder(uint8(EFoundationSquadOrder::Hold));}
void APhase2Character::OrderMove(){SendSquadOrder(uint8(EFoundationSquadOrder::Move));}
void APhase2Character::OrderCover(){SendSquadOrder(uint8(EFoundationSquadOrder::Cover));}
void APhase2Character::OrderFocus(){SendSquadOrder(uint8(EFoundationSquadOrder::Focus));}
void APhase2Character::OrderCeaseFire(){SendSquadOrder(uint8(EFoundationSquadOrder::CeaseFire));}

void APhase2Character::SendSquadOrder(uint8 Value)
{
    if(P2IsDead() || InteractionManager->MirrorActive)return;
    const auto Order=EFoundationSquadOrder(Value);
    FHitResult Hit;
    AActor* Target=ContextAim(this,2500,Hit,Order==EFoundationSquadOrder::Move);
    if((Order==EFoundationSquadOrder::Move || Order==EFoundationSquadOrder::Focus) && !Target)return;
    const FVector Point=Target?Hit.ImpactPoint:GetActorLocation();
    bool Accepted=false;
    for(TActorIterator<AFoundationNPC> NPC(GetWorld());NPC;++NPC)
        if(NPC->Friendly && FVector::DistSquared(NPC->GetActorLocation(),GetActorLocation())<FMath::Square(4500.f))
            Accepted|=NPC->IssueOrder(Order,Point,Order==EFoundationSquadOrder::Follow?static_cast<AActor*>(this):Target);
    if(Accepted)
    {
        if(Order==EFoundationSquadOrder::Move || Order==EFoundationSquadOrder::Focus)
            DrawDebugSphere(GetWorld(),Point+FVector(0,0,24),20,12,FColor::Cyan,false,3,0,2);
        const FText Command=StaticEnum<EFoundationSquadOrder>()->GetDisplayNameTextByValue(Value);
        ShowTacticalCaption(FText::Format(NSLOCTEXT("Tactical","Accepted","Squad: {0}"),Command));
    }
    else ShowTacticalCaption(NSLOCTEXT("Tactical","NoSquad","No squadmate can take that order"));
}

bool AFoundationNPC::IsBody() const
{
    return (bIsDead || State==EFoundationAIState::Restrained) && !BodyHidden;
}

FVector AFoundationNPC::UsePoint() const
{
    return IsBody()?GetMesh()->GetSocketLocation(TEXT("pelvis")):GetActorLocation();
}

bool AFoundationNPC::IsBehind(const APawn* User) const
{
    const FVector Approach=(User->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
    return FVector::DotProduct(GetActorForwardVector(),Approach)<-.5f;
}

void AFoundationNPC::StopAI()
{
    if(auto* AI=Cast<AAIController>(GetController()))
    {
        AI->StopMovement();
        AI->ClearFocus(EAIFocusPriority::Gameplay);
    }
    GetCharacterMovement()->StopMovementImmediately();
    Target=nullptr;
    SeesTarget=HasSightCandidate=false;
}

FFoundationUseInfo AFoundationNPC::GetUseInfo_Implementation(APawn* User) const
{
    FFoundationUseInfo Info;
    Info.Allowed=false;
    auto* Player=Cast<APhase2Character>(User);
    if(!Player || BodyHidden)return Info;
    if(ContextPlayer==Player)
    {
        if(Context==EContext::Drag)Info.Prompt=NSLOCTEXT("Tactical","Dragging","Release E to drop · O to hide");
        return Info;
    }
    if(!Player->CanUseContext() || Context!=EContext::None || FVector::DistSquared2D(User->GetActorLocation(),UsePoint())>FMath::Square(185.f))
        return Info;
    if(IsBody())
    {
        Info.Mode=EFoundationUseMode::Continuous;
        Info.Prompt=NSLOCTEXT("Tactical","Drag","Hold E to drag");
    }
    else if(Friendly && State==EFoundationAIState::Downed)
    {
        Info.Mode=EFoundationUseMode::Hold;
        Info.HoldSeconds=3.f;
        Info.Prompt=NSLOCTEXT("Tactical","Revive","Hold E to help squadmate");
    }
    else if(!Friendly && (State==EFoundationAIState::Surrender || (IsBehind(User) && Awareness<.35f)))
    {
        Info.Mode=EFoundationUseMode::Hold;
        Info.HoldSeconds=.8f;
        Info.Prompt=State==EFoundationAIState::Surrender
            ?NSLOCTEXT("Tactical","Restrain","Hold E to restrain")
            :NSLOCTEXT("Tactical","Takedown","Hold E for takedown · U to grab");
    }
    else return Info;
    Info.Allowed=true;
    return Info;
}

bool AFoundationNPC::ReceiveUse_Implementation(APawn* User,EFoundationUsePhase Phase,float Value)
{
    auto* Player=Cast<APhase2Character>(User);
    if(!Player)return false;
    if(Phase==EFoundationUsePhase::Begin)
    {
        if(!GetUseInfo_Implementation(User).Allowed)return false;
        if(IsBody())
        {
            DragHandle->GrabComponentAtLocation(GetMesh(),TEXT("pelvis"),UsePoint());
            if(!DragHandle->GetGrabbedComponent())return false;
            Context=EContext::Drag;
        }
        else if(Friendly)Context=EContext::Revive;
        else if(State==EFoundationAIState::Surrender)Context=EContext::Restrain;
        else Context=EContext::Takedown;
        ContextPlayer=Player;
        Player->TacticalTarget=this;
        Player->RefreshSpeed();
        StopAI();
        return true;
    }
    if(ContextPlayer!=Player)return false;
    if(Phase==EFoundationUsePhase::End || Phase==EFoundationUsePhase::Cancel)
    {
        ReleaseContext();
        return true;
    }
    if(Phase==EFoundationUsePhase::Update)
        return Context!=EContext::Takedown || IsBehind(User);
    if(Phase!=EFoundationUsePhase::Complete)return false;
    if(Context==EContext::Revive)
    {
        const bool Result=Revive();
        ReleaseContext();
        return Result;
    }
    if(Context!=EContext::Takedown && Context!=EContext::Restrain)return false;
    if(Context==EContext::Takedown && !IsBehind(User))return false;
    DropAIWeapon();
    Enter(EFoundationAIState::Restrained);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->WakeAllRigidBodies();
    StateLabel->SetVisibility(false);
    ReleaseContext();
    return true;
}

bool AFoundationNPC::TryGrab(APhase2Character* Player)
{
    if(!Player || !Player->CanUseContext() || Friendly || IsBody() || Context!=EContext::None || !IsBehind(Player)
        || FVector::DistSquared2D(GetActorLocation(),Player->GetActorLocation())>FMath::Square(155.f))return false;
    if(State!=EFoundationAIState::Surrender && Awareness>=.35f)
    {
        Target=Player;
        LastKnownLocation=Player->GetActorLocation();
        Enter(EFoundationAIState::Search,6);
        return false;
    }
    StopAI();
    DropAIWeapon();
    P2SetStance(0);
    GetCharacterMovement()->DisableMovement();
    Context=EContext::Hostage;
    ContextPlayer=Player;
    Player->TacticalTarget=this;
    Player->HoldingHostage=true;
    Player->RefreshSpeed();
    Enter(EFoundationAIState::Hostage);
    return true;
}

void AFoundationNPC::ReleaseContext()
{
    if(Context==EContext::Drag)DragHandle->ReleaseComponent();
    if(Context==EContext::Hostage && !bIsDead)
    {
        // Release in place, with a swept stumble instead of a forced teleport.
        FHitResult Hit;
        SetActorLocation(GetActorLocation()+GetActorForwardVector()*55,true,&Hit);
        Surrender();
    }
    if(auto* Player=ContextPlayer.Get())
    {
        Player->TacticalTarget=nullptr;
        Player->HoldingHostage=false;
        Player->RefreshSpeed();
    }
    ContextPlayer.Reset();
    Context=EContext::None;
}

void AFoundationNPC::UpdateContext(float Dt)
{
    if(Context==EContext::None)return;
    auto* Player=ContextPlayer.Get();
    if(!Player || Player->P2IsDead() || Player->P2Traversing || Player->P2Stance==2
        || FVector::DistSquared2D(Player->GetActorLocation(),UsePoint())>FMath::Square(260.f))
    {
        ReleaseContext();
        if(Player)Player->InteractionManager->CancelUse();
        return;
    }
    if(Context==EContext::Hostage)
    {
        const FVector Desired=Player->GetActorLocation()+Player->GetActorForwardVector()*68
            -Player->GetActorRightVector()*38+FVector(0,0,96-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        FHitResult Hit;
        SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(),Desired,Dt,280),true,&Hit);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),Player->GetActorRotation(),Dt,10));
    }
    if(Context==EContext::Drag)
    {
        const FVector Pelvis=UsePoint();
        const FVector Desired=Player->GetActorLocation()+Player->GetActorForwardVector()*125
            -FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-26);
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundationBodyDrag),false,this);
        Query.AddIgnoredActor(Player);
        const FVector Next=FMath::VInterpConstantTo(Pelvis,Desired,Dt,230);
        // The pelvis can settle within a few centimeters of the floor. Test
        // the horizontal clearance above it while Chaos handles floor contact.
        const FVector Clearance(0,0,22);
        const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,Pelvis+Clearance,Next+Clearance,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(18),Query);
        DragHandle->SetTargetLocation(Blocked?Pelvis:Next);
    }
}

bool AFoundationNPC::TryHide(APhase2Character* Player)
{
    if(Context!=EContext::Drag || ContextPlayer!=Player)return false;
    for(TActorIterator<AActor> Zone(GetWorld());Zone;++Zone)
    {
        if(!Zone->ActorHasTag(TEXT("BodyHide")) || FVector::DistSquared2D(Zone->GetActorLocation(),UsePoint())>FMath::Square(100.f))continue;
        ReleaseContext();
        BodyHidden=true;
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SetActorHiddenInGame(true);
        return true;
    }
    return false;
}

void AFoundationNPC::Down()
{
    ReleaseContext();
    StopAI();
    FoundationBridge::SetNumber(HealthComponent,TEXT("CurrentHealth"),1);
    P2SetStance(2);
    GetCharacterMovement()->MaxWalkSpeed=70;
    GetCharacterMovement()->MaxWalkSpeedCrouched=70;
    AIWeapon->SetVisibility(false,true);
    Enter(EFoundationAIState::Downed);
}

bool AFoundationNPC::Revive()
{
    if(State!=EFoundationAIState::Downed)return false;
    FoundationBridge::Heal(this,49);
    P2SetStance(0);
    GetCharacterMovement()->MaxWalkSpeed=210;
    AIWeapon->SetVisibility(true,true);
    Enter(EFoundationAIState::Idle);
    return true;
}
