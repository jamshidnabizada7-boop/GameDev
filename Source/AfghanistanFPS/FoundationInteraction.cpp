#include "FoundationInteraction.h"
#include "FoundationBridge.h"
#include "FoundationEquipment.h"
#include "FoundationSession.h"
#include "FoundationAI.h"
#include "Phase2Character.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/AudioComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Misc/App.h"

UFoundationInteractionComponent::UFoundationInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickInterval=0.05f;
}
void UFoundationInteractionComponent::BeginPlay()
{
    Super::BeginPlay();Player=Cast<APhase2Character>(GetOwner());
#if WITH_EDITOR
    if(GetOwner()->IsEditorOnly()) SetComponentTickEnabled(false);
#endif
}
void UFoundationInteractionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelUse();EndMirror();if(PromptWidget) PromptWidget->RemoveFromParent();
    Super::EndPlay(Reason);
}
void UFoundationInteractionComponent::BindInput(UEnhancedInputComponent* Input)
{
    if(!Input) return;
    if(auto* Action=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/Actions/IA_Interact.IA_Interact")))
    {
        Input->BindAction(Action,ETriggerEvent::Started,this,&UFoundationInteractionComponent::BeginUse);
        Input->BindAction(Action,ETriggerEvent::Completed,this,&UFoundationInteractionComponent::EndUse);
        Input->BindAction(Action,ETriggerEvent::Canceled,this,&UFoundationInteractionComponent::CancelUse);
    }
    if(auto* Action=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/Actions/IA_InteractionAdjust.IA_InteractionAdjust")))
        Input->BindActionValueLambda(Action,ETriggerEvent::Triggered,[this](const FInputActionValue& V){AdjustUse(V.Get<float>());});
}
FString UFoundationInteractionComponent::InspectFocus() const
{
    if(!Player.IsValid() || !Player->P2Camera) return TEXT("No player/camera");
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationFocusProbe),false,Player.Get());
    TInlineComponentArray<UChildActorComponent*> C(Player.Get());for(auto* Child:C) if(Child->GetChildActor()) Q.AddIgnoredActor(Child->GetChildActor());
    const FVector Start=Player->P2Camera->GetComponentLocation();FHitResult Hit;
    GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Player->P2Camera->GetForwardVector()*UseDistance,ECC_Visibility,Q);
    return FString::Printf(TEXT("owner=%s reach=%.1f start=%s direction=%s hit=%s component=%s interface=%d"),*GetNameSafe(GetOwner()),UseDistance,*Start.ToString(),*Player->P2Camera->GetForwardVector().ToString(),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),Hit.GetActor()?Hit.GetActor()->Implements<UFoundationInteractableInterface>():false);
}
AActor* UFoundationInteractionComponent::TraceFocus() const
{
    if(!Player.IsValid() || !Player->P2Camera) return nullptr;
    // Adapted from FPSCore InteractionComponent::WorldInteract / InteractionIndicator,
    // commit 97993c21c5519628f2462e2660930f0483913b17. Copyright 2022/2023 Ellie Kelemen.
    // MIT notice: ThirdPartyNotices/FPSCore-LICENSE.txt. Blueprint-safe interface
    // dispatch, LOS revalidation and cancellation are project-specific additions.
    // Interact with the same simple collision that blocks the player. Thin,
    // scaled graybox panels can miss complex triangle traces in this engine.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FoundationFocus),false,Player.Get());
    TInlineComponentArray<UChildActorComponent*> ChildrenOfPlayer(Player.Get());
    for(auto* C:ChildrenOfPlayer) if(C->GetChildActor()) Params.AddIgnoredActor(C->GetChildActor());
    const FVector Start=Player->P2Camera->GetComponentLocation();
    const FVector End=Start+Player->P2Camera->GetForwardVector()*UseDistance;
    FHitResult Hit;
    if(!GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params)) return nullptr;
    AActor* Target=Hit.GetActor();
    if(!IsValid(Target)) return nullptr;
    if(Target->Implements<UFoundationInteractableInterface>()) return Target;
    return FoundationBridge::LegacyInteract(Target,Player.Get(),false)?Target:nullptr;
}
bool UFoundationInteractionComponent::Validate(AActor* Target) const
{
    if(!IsValid(Target) || !Player.IsValid() || Player->P2IsDead() || Player->P2Traversing) return false;
    if(Player->Equipment && Player->Equipment->IsHoldingThrowable())return false;
    const auto* Native=Cast<AFoundationInteractable>(Target);
    const auto* NPC=Cast<AFoundationNPC>(Target);
    const FVector Point=Native?Native->InteractionPoint():NPC?NPC->UsePoint():Target->GetActorLocation();
    const FVector Eye=Player->P2Camera?Player->P2Camera->GetComponentLocation():Player->GetActorLocation();
    if(FVector::Dist(Eye,Point)>UseDistance+35) return false;
    FHitResult Hit;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationUseLOS),false,Player.Get());
    TInlineComponentArray<UChildActorComponent*> OwnedChildren(Player.Get());
    for(auto* C:OwnedChildren) if(C->GetChildActor()) Q.AddIgnoredActor(C->GetChildActor());
    if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Point,ECC_Visibility,Q)) return Hit.GetActor()==Target;
    return true;
}
void UFoundationInteractionComponent::BeginUse()
{
    if(MirrorActive) {EndMirror();return;}
    if(!Player.IsValid() || Player->TacticalTarget)return;
    AActor* Target=TraceFocus();FocusedActor=Target;
    if(!Validate(Target)) return;
    if(!Target->Implements<UFoundationInteractableInterface>())
    {
        if(FoundationBridge::LegacyInteract(Target,Player.Get(),true)) {++CompletedTransactions;LastResult=TEXT("Legacy interaction completed");}
        return;
    }
    if(auto* Door=Cast<AFoundationInteractable>(Target))
        if(Door->Kind==EFoundationInteractableKind::Door && Player->P2Stance==1)
        {HeldActor=Door;BeginMirror(Door);return;}
    ActiveInfo=IFoundationInteractableInterface::Execute_GetUseInfo(Target,Player.Get());
    Prompt=ActiveInfo.Prompt;
    if(!ActiveInfo.Allowed) {LastResult=TEXT("Denied: requirements");return;}
    HeldActor=Target;UseElapsed=0;Progress=0;Adjusted=false;Completed=false;
    InstantReachRemaining=0;
    Using=IFoundationInteractableInterface::Execute_ReceiveUse(Target,Player.Get(),EFoundationUsePhase::Begin,0);
    if(Using && ActiveInfo.Mode==EFoundationUseMode::Immediate)
    {
        const FVector Contact=ReachPoint();
        Completed=IFoundationInteractableInterface::Execute_ReceiveUse(Target,Player.Get(),EFoundationUsePhase::Complete,1);
        if(Completed) {++CompletedTransactions;LastResult=TEXT("Interaction completed");}
        EndUse();
        if(Completed)
        {
            InstantReachPoint=Contact;
            InstantReachRemaining=.30f;
        }
    }
}
void UFoundationInteractionComponent::EndUse()
{
    // Mirror is a toggle; releasing the opening press must retain its door anchor.
    if(MirrorActive) return;
    if(Using && HeldActor.IsValid())
        IFoundationInteractableInterface::Execute_ReceiveUse(HeldActor.Get(),Player.Get(),EFoundationUsePhase::End,UseElapsed);
    Using=false;HeldActor.Reset();ReachAlpha=0;Progress=0;
}
void UFoundationInteractionComponent::CancelUse()
{
    if(Using && HeldActor.IsValid())
        IFoundationInteractableInterface::Execute_ReceiveUse(HeldActor.Get(),Player.Get(),EFoundationUsePhase::Cancel,UseElapsed);
    if(Using) LastResult=TEXT("Canceled: released, occluded or out of range");
    Using=false;HeldActor.Reset();ReachAlpha=0;Progress=0;
}
void UFoundationInteractionComponent::AdjustUse(float Steps)
{
    if(!Using || !HeldActor.IsValid() || !Validate(HeldActor.Get()) || FMath::IsNearlyZero(Steps)) return;
    Adjusted=true;
    IFoundationInteractableInterface::Execute_ReceiveUse(HeldActor.Get(),Player.Get(),EFoundationUsePhase::Adjust,Steps);
}
int32 UFoundationInteractionComponent::ItemCount(FName Id) const
{
    const int32* Count=OwnedItems.Find(Id);return Count?*Count:0;
}
bool UFoundationInteractionComponent::GrantItem(FName Id,int32 Count)
{
    if(Count<=0 || !Player.IsValid() || Player->P2IsDead()) return false;
    bool Accepted=false;
    if(Id==TEXT("health")) Accepted=FoundationBridge::Heal(Player.Get(),float(Count));
    else if(Id.ToString().StartsWith(TEXT("weapon_")))
    {if(auto* E=Player->FindComponentByClass<UFoundationEquipment>())if(E->AddWeapon(Id)){OwnedItems.FindOrAdd(Id)=1;Accepted=true;}}
    else if(Id==TEXT("ammo") || Id.ToString().StartsWith(TEXT("ammo_")))
    {
        auto* E=Player->FindComponentByClass<UFoundationEquipment>();
        const FName Type=Id==TEXT("ammo")?NAME_None:FName(*Id.ToString().RightChop(5));
        Accepted=E?E->AddReserve(Type,Count):(Id==TEXT("ammo") && FoundationBridge::AddAmmo(Player.Get(),Count));
    }
    else if(ItemCount(Id)==0 || !Id.ToString().StartsWith(TEXT("weapon_")))
    {
        const bool Throwable=Id==TEXT("knife") || Id==TEXT("stone") || Id==TEXT("frag") || Id==TEXT("smoke") || Id==TEXT("flash");
        if(!Throwable || ItemCount(Id)<99){OwnedItems.FindOrAdd(Id)=FMath::Clamp(ItemCount(Id)+Count,0,Throwable?99:999);Accepted=true;}
    }
    if(Accepted) OnItemClaimed.Broadcast(Id,Count);
    return Accepted;
}
FVector UFoundationInteractionComponent::ReachPoint() const
{
    if(HeldActor.IsValid())
    {
        if(const auto* A=Cast<AFoundationInteractable>(HeldActor.Get())) return A->InteractionPoint();
        if(const auto* NPC=Cast<AFoundationNPC>(HeldActor.Get())) return NPC->UsePoint();
        return HeldActor->GetActorLocation();
    }
    if(InstantReachRemaining>0)return InstantReachPoint;
    return Player.IsValid() && Player->P2Camera?Player->P2Camera->GetComponentLocation():FVector::ZeroVector;
}
void UFoundationInteractionComponent::BeginMirror(AFoundationInteractable* Door)
{
    if(!Door || Door->DoorAngle>5 || !Player.IsValid() || Player->P2IsDead()) return;
    auto* PC=Cast<APlayerController>(Player->GetController());if(!PC) return;
    MirrorStartRotation=PC->GetControlRotation();
    const FVector Base=Door->GetActorLocation();
    const FVector Forward=Door->GetActorForwardVector();
    const float Side=FVector::DotProduct(Player->GetActorLocation()-Base,Forward)<0?1.f:-1.f;
    const FVector Location=Door->GetActorTransform().TransformPosition(FVector(Side*40,55,8));
    MirrorBaseRotation=(Forward*Side).Rotation();MirrorBaseRotation.Pitch=8;
    MirrorCamera=GetWorld()->SpawnActor<ACameraActor>(Location,MirrorBaseRotation);
    if(!MirrorCamera) return;
    MirrorCamera->GetCameraComponent()->SetFieldOfView(70);
    PC->SetViewTarget(MirrorCamera);
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->DisableMovement();
    MirrorActive=true;SetComponentTickInterval(0);LastResult=TEXT("Tactical mirror active");
}
void UFoundationInteractionComponent::EndMirror()
{
    if(!MirrorActive) return;
    if(Player.IsValid())
    {
        if(auto* PC=Cast<APlayerController>(Player->GetController()))
        {PC->SetViewTarget(Player.Get());PC->SetControlRotation(MirrorStartRotation);}
        Player->GetCharacterMovement()->SetMovementMode(Player->P2IsDead()?MOVE_None:MOVE_Walking);
    }
    if(MirrorCamera) MirrorCamera->Destroy();MirrorCamera=nullptr;
    MirrorActive=false;HeldActor.Reset();SetComponentTickInterval(0.05f);LastResult=TEXT("Tactical mirror closed");
}
void UFoundationInteractionComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,Type,Function);
    if(!Player.IsValid() || !Player->IsPlayerControlled() || !Player->P2Camera) return;
    if(Player->P2IsDead() || Player->P2Traversing)
    {InstantReachRemaining=0;CancelUse();EndMirror();FocusedActor=nullptr;Prompt=FText::GetEmpty();return;}
    if(!PromptWidget)
        if(auto* PC=Cast<APlayerController>(Player->GetController()))
        {
            auto* Widget=CreateWidget<UFoundationPromptWidget>(PC,UFoundationPromptWidget::StaticClass());
            Widget->Source=this;PromptWidget=Widget;Widget->AddToViewport(15);
            OriginalFov=Player->P2Camera->FieldOfView;
        }
    if(MirrorActive)
    {
        if(Player->P2IsDead() || Player->P2Stance!=1 || !Validate(HeldActor.Get())) {EndMirror();return;}
        auto* PC=Cast<APlayerController>(Player->GetController());
        if(PC && MirrorCamera)
        {
            const FRotator Delta=(PC->GetControlRotation()-MirrorStartRotation).GetNormalized();
            MirrorCamera->SetActorRotation(MirrorBaseRotation+FRotator(FMath::Clamp(Delta.Pitch,-15.f,20.f),FMath::Clamp(Delta.Yaw,-35.f,35.f),0));
        }
        MirrorFrameMilliseconds=float(FApp::GetDeltaTime()*1000);
        Prompt=FText::FromString(TEXT("Tactical mirror  |  E: return  |  Mouse: pan"));return;
    }
    AActor* Next=TraceFocus();
    if(!Next && !Using && FMath::Abs(Player->P2Lean)>.1f)
        if(const auto* Door=Cast<AFoundationInteractable>(FocusedActor))
            if(Door->Kind==EFoundationInteractableKind::Door && Door->DoorAngle>=5 && Door->DoorAngle<=45 && Validate(Door?FocusedActor.Get():nullptr)) Next=FocusedActor;
    if(Using && HeldActor.IsValid() && Validate(HeldActor.Get())) Next=HeldActor.Get();
    if(Using && (!Validate(HeldActor.Get()) || Next!=HeldActor.Get())) CancelUse();
    FocusedActor=Next;
    Prompt=Next?FText::FromString(TEXT("E: interact")):FText::GetEmpty();
    if(Next && Next->Implements<UFoundationInteractableInterface>())
        Prompt=IFoundationInteractableInterface::Execute_GetUseInfo(Next,Player.Get()).Prompt;
    bool WantPeek=false;
    if(const auto* Door=Cast<AFoundationInteractable>(Next))
        WantPeek=Door->Kind==EFoundationInteractableKind::Door && Door->DoorAngle>=5 && Door->DoorAngle<=45 && FMath::Abs(Player->P2Lean)>0.1f;
    if(Peeking && !WantPeek) RestoringPeek=true;
    if(WantPeek) RestoringPeek=false;
    if(WantPeek || RestoringPeek)
        Player->P2Camera->SetFieldOfView(FMath::FInterpTo(Player->P2Camera->FieldOfView,WantPeek?65.f:OriginalFov,Dt,10));
    if(RestoringPeek && FMath::IsNearlyEqual(Player->P2Camera->FieldOfView,OriginalFov,.05f))
    {Player->P2Camera->SetFieldOfView(OriginalFov);RestoringPeek=false;}
    Peeking=WantPeek;
    if(!Using && InstantReachRemaining>0)
    {
        InstantReachRemaining=FMath::Max(0.f,InstantReachRemaining-Dt);
        ReachAlpha=FMath::Sin(PI*InstantReachRemaining/.30f);
        return;
    }
    if(!Using || !HeldActor.IsValid() || Completed) {ReachAlpha=FMath::FInterpTo(ReachAlpha,0.f,Dt,10);return;}
    UseElapsed+=Dt;
    const float Value=ActiveInfo.Mode==EFoundationUseMode::Hold?FMath::Clamp(UseElapsed/FMath::Max(0.1f,ActiveInfo.HoldSeconds),0.f,1.f):Dt;
    const bool Ready=IFoundationInteractableInterface::Execute_ReceiveUse(HeldActor.Get(),Player.Get(),EFoundationUsePhase::Update,Value);
    if(!Ready) {UseElapsed=0;Progress=0;return;}
    Progress=ActiveInfo.Mode==EFoundationUseMode::Hold?Value:0;
    ReachAlpha=ActiveInfo.Mode==EFoundationUseMode::Hold?FMath::Sin(Progress*PI):Cast<AFoundationNPC>(HeldActor.Get())?.75f:0.f;
    if(ActiveInfo.Mode==EFoundationUseMode::Hold && Progress>=1)
    {
        Completed=IFoundationInteractableInterface::Execute_ReceiveUse(HeldActor.Get(),Player.Get(),EFoundationUsePhase::Complete,1);
        if(Completed) {++CompletedTransactions;LastResult=TEXT("Hold completed");}
        else {UseElapsed=0;Progress=0;LastResult=TEXT("No capacity or requirements unmet");}
    }
}
TSharedRef<SWidget> UFoundationPromptWidget::RebuildWidget()
{
    return SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(20,0,20,115)
    [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",18)).ColorAndOpacity(FLinearColor::White)
     .ShadowOffset(FVector2D(2,2)).ShadowColorAndOpacity(FLinearColor::Black)
     .Text_Lambda([this]
     {
         if(!Source.IsValid()) return FText::GetEmpty();
         if(const auto* Player=Cast<APhase2Character>(Source->GetOwner()))
         {
             if(Player->HoldingHostage)return NSLOCTEXT("Tactical","ReleaseHostage","U to release");
             if(Source->Prompt.IsEmpty() && !Player->TacticalCaption.IsEmpty())return Player->TacticalCaption;
         }
         const FString Progress=Source->Using && Source->Progress>0?FString::Printf(TEXT("  %d%%"),FMath::RoundToInt(Source->Progress*100)):TEXT("");
         return FText::FromString(Source->Prompt.ToString()+Progress);
     })];
}

AFoundationInteractable::AFoundationInteractable()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bStartWithTickEnabled=false;
    SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));SetRootComponent(SceneRoot);
    Hinge=CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));Hinge->SetupAttachment(SceneRoot);
    Panel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));Panel->SetupAttachment(Hinge);
    Panel->SetMobility(EComponentMobility::Movable);Panel->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Panel->SetStaticMesh(Cube.Object);
    Light=CreateDefaultSubobject<UPointLightComponent>(TEXT("SwitchLight"));Light->SetupAttachment(SceneRoot);
    Light->SetRelativeLocation(FVector(120,0,180));Light->SetIntensity(4000);Light->SetAttenuationRadius(450);
    InteriorAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("InteriorSound"));InteriorAudio->SetupAttachment(SceneRoot);InteriorAudio->bAutoActivate=false;
    ExteriorAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("ExteriorAmbience"));ExteriorAudio->SetupAttachment(SceneRoot);ExteriorAudio->bAutoActivate=false;
    for(int32 I=0;I<3;++I)
    {
        auto* Piece=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Debris%d"),I));
        Piece->SetupAttachment(SceneRoot);Piece->SetStaticMesh(Cube.Object);Piece->SetCollisionProfileName(TEXT("NoCollision"));Debris.Add(Piece);
    }
}
void AFoundationInteractable::OnConstruction(const FTransform& Transform) {Super::OnConstruction(Transform);Configure(Kind);}
void AFoundationInteractable::Configure(EFoundationInteractableKind NewKind)
{
    Kind=NewKind;Hinge->SetRelativeRotation(FRotator(0,DoorAngle,0));
    Panel->SetRelativeRotation(FRotator::ZeroRotator);
    FVector Size(50,50,50),Position(0,0,25);
    if(Kind==EFoundationInteractableKind::Door) {Size=FVector(12,110,214);Position=FVector(6,55,113);}
    if(Kind==EFoundationInteractableKind::Drawer) {Size=FVector(90,60,25);Position=FVector(0,0,70);}
    if(Kind==EFoundationInteractableKind::LightSwitch) {Size=FVector(12,22,30);Position=FVector(0,0,100);}
    if(Kind==EFoundationInteractableKind::Key) {Size=FVector(30,15,8);Position=FVector(0,0,75);}
    if(Kind==EFoundationInteractableKind::Weapon) {Size=FVector(80,12,15);Position=FVector(0,0,70);}
    if(Kind==EFoundationInteractableKind::Item) {Size=FVector(12,12,12);Position=FVector(0,0,8);}
    if(Kind==EFoundationInteractableKind::Terminal) {Size=FVector(35,60,90);Position=FVector(0,0,45);}
    Panel->SetRelativeScale3D(Size/100.f);Panel->SetRelativeLocation(Position);
    Panel->SetVisibility(!Consumed && !Broken);Panel->SetCollisionEnabled(Consumed || Broken?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
    if(Kind==EFoundationInteractableKind::Ammo || Kind==EFoundationInteractableKind::Health || Kind==EFoundationInteractableKind::Key || Kind==EFoundationInteractableKind::Weapon || Kind==EFoundationInteractableKind::Item)
        Panel->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    Light->SetVisibility(Kind==EFoundationInteractableKind::LightSwitch && LightOn);
    for(int32 I=0;I<Debris.Num();++I)
    {
        Debris[I]->SetRelativeScale3D(FVector(.22,.28,.12));Debris[I]->SetRelativeLocation(FVector((I-1)*28,(I%2)*25,8));
        Debris[I]->SetRelativeRotation(FRotator(10,I*35,12));Debris[I]->SetVisibility(Broken);
    }
}
void AFoundationInteractable::BeginPlay()
{
    Super::BeginPlay();if(StableId.IsNone()) StableId=GetFName();PickupStart=Panel->GetRelativeLocation();
    if(!UseSound)UseSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/FoundationUse.FoundationUse"));
    if(Kind==EFoundationInteractableKind::Door && InteriorSound)
    {
        FSoundAttenuationSettings Settings;Settings.bAttenuate=true;Settings.AttenuationShapeExtents=FVector(350);Settings.FalloffDistance=350;
        InteriorAudio->SetSound(InteriorSound);InteriorAudio->AdjustAttenuation(Settings);InteriorAudio->Play();
        ExteriorAudio->SetSound(InteriorSound);ExteriorAudio->AdjustAttenuation(Settings);ExteriorAudio->SetPitchMultiplier(.7f);ExteriorAudio->Play();SetListening(false);
    }
}
FVector AFoundationInteractable::InteractionPoint() const {return Panel->Bounds.Origin;}
FFoundationUseInfo AFoundationInteractable::GetUseInfo_Implementation(APawn* User) const
{
    FFoundationUseInfo Info;Info.Allowed=!Consumed && !Broken;
    auto* Manager=User?User->FindComponentByClass<UFoundationInteractionComponent>():nullptr;
    if(Kind==EFoundationInteractableKind::Door)
    {
        Info.Mode=EFoundationUseMode::Continuous;
        const auto* P=Cast<APhase2Character>(User);
        if(P && P->P2Stance==1) Info.Prompt=FText::FromString(TEXT("E: tactical mirror  |  Stand to open or listen"));
        else if(Locked && (!Manager || Manager->ItemCount(RequiredKey)==0))
        {
            const bool Tool=Manager && Manager->ItemCount(TEXT("lockpick"))>0;
            Info.Mode=EFoundationUseMode::Hold;Info.HoldSeconds=1.5f;Info.Allowed=Tool;
            Info.Prompt=FText::FromString(Tool?FString::Printf(TEXT("Hold E + wheel: align pick to 35 degrees  [%d]"),FMath::RoundToInt(PickAngle)):TEXT("Locked: armory key or lockpick required"));
        }
        else Info.Prompt=FText::FromString(Listening?TEXT("Listening  |  Release E to stop"):TEXT("Tap E: open/close  |  Hold E: listen  |  Hold E + wheel: gradual"));
    }
    else if(Kind==EFoundationInteractableKind::Terminal)
    {
        const auto* Player=Cast<APhase2Character>(User);
        const auto* Session=Player?Player->FoundationSession.Get():nullptr;
        Info.Allowed=Info.Allowed && Session && !Session->ObjectiveComplete && !Player->P2IsDead();
        Info.Prompt=Session && Session->ObjectiveComplete
            ?NSLOCTEXT("Foundation","TerminalActivated","Training terminal active")
            :NSLOCTEXT("Foundation","TerminalPrompt","Activate training terminal");
    }
    else if(Kind==EFoundationInteractableKind::LightSwitch) Info.Prompt=FText::FromString(TEXT("E: toggle light"));
    else if(Kind==EFoundationInteractableKind::Drawer) Info.Prompt=FText::FromString(TEXT("E: open/close drawer"));
    else if(Kind==EFoundationInteractableKind::Breakable) Info.Prompt=FText::FromString(TEXT("E or fire: break crate"));
    else
    {
        Info.Mode=EFoundationUseMode::Hold;Info.HoldSeconds=.8f;
        const TCHAR* Label=Kind==EFoundationInteractableKind::LootCrate?TEXT("search crate"):
            Kind==EFoundationInteractableKind::Ammo?TEXT("take ammunition"):
            Kind==EFoundationInteractableKind::Health?TEXT("take health pack"):
            Kind==EFoundationInteractableKind::Weapon?TEXT("pick up weapon"):
            Kind==EFoundationInteractableKind::Item?TEXT("take equipment"):
            ItemId==TEXT("lockpick")?TEXT("take lockpick"):TEXT("take armory key");
        Info.Prompt=FText::FromString(FString::Printf(TEXT("Hold E: %s"),Label));
        if(Kind==EFoundationInteractableKind::Weapon)
            if(auto* Equipment=User?User->FindComponentByClass<UFoundationEquipment>():nullptr)
                if(!Equipment->CanAcceptWeapon(ItemId)){Info.Allowed=false;Info.Prompt=FText::FromString(TEXT("Weapon already carried or slots full | X: drop current"));}
    }
    return Info;
}
bool AFoundationInteractable::ReceiveUse_Implementation(APawn* User,EFoundationUsePhase Phase,float Value)
{
    auto* Manager=User?User->FindComponentByClass<UFoundationInteractionComponent>():nullptr;
    if(Consumed || Broken || (!Manager && Kind!=EFoundationInteractableKind::Door)) return false;
    if(Phase==EFoundationUsePhase::Begin)
    {
        CurrentUser=User;DoorUseSeconds=0;DoorAdjusted=false;Picking=false;PickupStart=Panel->GetRelativeLocation();
        if(Kind==EFoundationInteractableKind::Door && Locked)
        {
            if(Manager && Manager->ItemCount(RequiredKey)>0)
            {Locked=false;if(UseSound) UGameplayStatics::PlaySoundAtLocation(this,UseSound,InteractionPoint());}
            else Picking=Manager && Manager->ItemCount(TEXT("lockpick"))>0;
            if(Locked && !Picking) return false;
        }
        return true;
    }
    if(Phase==EFoundationUsePhase::Adjust)
    {
        if(Kind!=EFoundationInteractableKind::Door) return false;
        DoorAdjusted=true;SetListening(false);
        if(Picking) PickAngle=FMath::Clamp(PickAngle+Value*5,0.f,90.f);
        else SetDoorTarget(TargetAngle+Value*5);
        return true;
    }
    if(Phase==EFoundationUsePhase::Update)
    {
        if(Picking) return FMath::Abs(PickAngle-35)<=3;
        if(Kind==EFoundationInteractableKind::Door)
        {DoorUseSeconds+=Value;if(DoorUseSeconds>.35f && !DoorAdjusted && DoorAngle<5) SetListening(true);}
        else if(Kind!=EFoundationInteractableKind::LootCrate && Value>.55f)
        {
            if(const auto* P=Cast<APhase2Character>(User)) if(P->P2Camera)
            {
                const FVector End=P->P2Camera->GetComponentTransform().TransformPosition(FVector(45,-12,-15));
                const FVector Start=GetActorTransform().TransformPosition(PickupStart);
                FHitResult Hit;Panel->SetWorldLocation(FMath::Lerp(Start,End,FMath::Clamp((Value-.55f)/.45f,0.f,1.f)),true,&Hit);
            }
        }
        return true;
    }
    if(Phase==EFoundationUsePhase::Cancel || Phase==EFoundationUsePhase::End)
    {
        if(Kind==EFoundationInteractableKind::Door)
        {
            if(Phase==EFoundationUsePhase::End && !Locked && !Picking && !DoorAdjusted && DoorUseSeconds<.35f)
            {SetDoorTarget(DoorAngle<5?90:0);if(UseSound) UGameplayStatics::PlaySoundAtLocation(this,UseSound,InteractionPoint());}
            else if(Phase==EFoundationUsePhase::Cancel) {TargetAngle=DoorAngle;SetActorTickEnabled(false);}
            SetListening(false);Picking=false;
        }
        else if(!Consumed) {FHitResult Hit;Panel->SetRelativeLocation(PickupStart,true,&Hit);}
        CurrentUser.Reset();return true;
    }
    if(Phase!=EFoundationUsePhase::Complete) return false;
    auto Cue=[this](){if(UseSound) UGameplayStatics::PlaySoundAtLocation(this,UseSound,InteractionPoint());};
    if(Kind==EFoundationInteractableKind::Terminal)
    {
        auto* Player=Cast<APhase2Character>(User);
        if(!Player || Player->P2IsDead() || !Player->FoundationSession || Player->FoundationSession->ObjectiveComplete)return false;
        Player->FoundationSession->CompleteObjective();
        ++UseCount;
        Cue();
        return true;
    }
    if(Picking) {Locked=false;Picking=false;++UseCount;Cue();return true;}
    if(Kind==EFoundationInteractableKind::LightSwitch)
    {
        LightOn=!LightOn;Light->SetVisibility(LightOn);
        for(const auto& Actor:LinkedLights) if(IsValid(Actor))
        {TInlineComponentArray<ULightComponent*> Lights(Actor.Get());for(auto* L:Lights) L->SetVisibility(LightOn);}
        ++UseCount;Cue();return true;
    }
    if(Kind==EFoundationInteractableKind::Drawer)
    {Open=!Open;SetActorTickEnabled(true);++UseCount;Cue();return true;}
    if(Kind==EFoundationInteractableKind::Breakable)
    {FDamageEvent Event;TakeDamage(100,Event,User->GetController(),User);Cue();return true;}
    if(Kind==EFoundationInteractableKind::Door) return true;
    if(Kind==EFoundationInteractableKind::Weapon)
    {
        auto* Equipment=User->FindComponentByClass<UFoundationEquipment>();
        if(Equipment && Equipment->AddWeapon(ItemId,WeaponMagazine,WeaponReserve))
        {Manager->OwnedItems.FindOrAdd(ItemId)=1;Manager->OnItemClaimed.Broadcast(ItemId,1);Consume();++UseCount;Cue();return true;}
        return false;
    }
    if(Manager->GrantItem(ItemId,ItemAmount)) {Consume();++UseCount;Cue();return true;}
    return false;
}
void AFoundationInteractable::Consume()
{
    Consumed=true;Panel->SetVisibility(false);Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AFoundationInteractable::SetListening(bool Enabled)
{
    Listening=Enabled;
    InteriorAudio->SetLowPassFilterEnabled(!Enabled);InteriorAudio->SetLowPassFilterFrequency(1000);
    InteriorAudio->SetVolumeMultiplier(Enabled?.6f:.2f);
    ExteriorAudio->SetVolumeMultiplier(Enabled?.09f:.15f);
}
bool AFoundationInteractable::DoorPoseClear(float Angle) const
{
    const FQuat Rotation=GetActorQuat()*FRotator(0,Angle,0).Quaternion();
    const FVector Center=GetActorTransform().TransformPosition(FRotator(0,Angle,0).RotateVector(FVector(6,55,113)));
    const FVector Half=FVector(5.5f,54.5f,106.5f)*GetActorScale3D().GetAbs();
    return !GetWorld()->OverlapBlockingTestByChannel(Center,Rotation,ECC_Pawn,FCollisionShape::MakeBox(Half),
        FCollisionQueryParams(SCENE_QUERY_STAT(FoundationDoorSweep),false,this));
}
void AFoundationInteractable::SetDoorTarget(float Degrees)
{
    if(Locked) return;TargetAngle=FMath::Clamp(Degrees,0.f,90.f);Blocked=false;SetActorTickEnabled(true);
}
void AFoundationInteractable::Tick(float Dt)
{
    Super::Tick(Dt);
    if(Kind==EFoundationInteractableKind::Door)
    {
        float Remaining=FMath::FInterpConstantTo(DoorAngle,TargetAngle,Dt,90)-DoorAngle;
        for(int32 I=0;I<64 && FMath::Abs(Remaining)>.01f;++I)
        {
            const float Step=FMath::Clamp(Remaining,-2.f,2.f);
            if(!DoorPoseClear(DoorAngle+Step)) {Blocked=true;TargetAngle=DoorAngle;break;}
            DoorAngle+=Step;Remaining-=Step;
        }
        Hinge->SetRelativeRotation(FRotator(0,DoorAngle,0));
        if(FMath::IsNearlyEqual(DoorAngle,TargetAngle,.05f)) SetActorTickEnabled(false);
    }
    else if(Kind==EFoundationInteractableKind::Drawer)
    {
        const float Next=FMath::FInterpConstantTo(DrawerAlpha,Open?1.f:0.f,Dt,2);
        FHitResult Hit;Panel->SetRelativeLocation(FVector(0,Next*70,70),true,&Hit);
        if(Hit.bBlockingHit) {Blocked=true;Open=DrawerAlpha>.5f;SetActorTickEnabled(false);return;}
        DrawerAlpha=Next;if(FMath::IsNearlyEqual(DrawerAlpha,Open?1.f:0.f)) SetActorTickEnabled(false);
    }
}
float AFoundationInteractable::TakeDamage(float Damage,const FDamageEvent& Event,AController* EventInstigator,AActor* Causer)
{
    if(Kind!=EFoundationInteractableKind::Breakable || Broken) return 0;
    const float Applied=FMath::Clamp(Damage,0.f,Durability);Durability-=Applied;
    if(Durability<=0)
    {Broken=true;Panel->SetVisibility(false);Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);for(const auto& Piece:Debris) Piece->SetVisibility(true);}
    return Applied;
}
