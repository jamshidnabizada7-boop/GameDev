#include "Phase2Character.h"
#include "FoundationAI.h"
#include "FoundationSession.h"
#include "Phase2AnimInstance.h"
#include "FoundationInteraction.h"
#include "FoundationEquipment.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "ThirdPartyMovement.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

APhase2Character::APhase2Character(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UPhase2CharacterMovement>(ACharacter::CharacterMovementComponentName))
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    InteractionManager=CreateDefaultSubobject<UFoundationInteractionComponent>(TEXT("InteractionManager"));
    Equipment=CreateDefaultSubobject<UFoundationEquipment>(TEXT("Equipment"));
    FoundationSession=CreateDefaultSubobject<UFoundationSession>(TEXT("FoundationSession"));
}

float APhase2Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (AppliedDamage > 0.0f && FoundationSession)
    {
        FoundationSession->DamageFeedback(DamageCauser, AppliedDamage);
    }
    return AppliedDamage;
}

bool APhase2Character::P2IsDead() const
{
    const FBoolProperty* P = FindFProperty<FBoolProperty>(GetClass(), TEXT("bIsDead"));
    return P && P->GetPropertyValue_InContainer(this);
}

void APhase2Character::BeginPlay()
{
    Super::BeginPlay();
    StandRadius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
    StandHalf = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    MeshOffset = GetMesh()->GetRelativeLocation();
    P2Camera = FindComponentByClass<UCameraComponent>();
    TArray<USkeletalMeshComponent*> Meshes;
    GetComponents(Meshes);
    for (auto* M : Meshes) if (M != GetMesh()) P2Arms = M;
    if (P2Camera)
    {
        NeutralCameraTransform = P2Camera->GetRelativeTransform();
        NeutralCameraOffset = GetActorTransform().InverseTransformPosition(P2Camera->GetComponentLocation());
        EyeHeight = StandHalf + NeutralCameraOffset.Z;
        EyeFrom = EyeTo = VisualEye = EyeHeight;
        NeutralViewCorrection = GetControlRotation().Quaternion().Inverse() * P2Camera->GetComponentQuat();
        P2Camera->bUsePawnControlRotation = false;
    }
    TArray<USceneComponent*> Components;
    GetComponents(Components);
    for (auto* C : Components) if (C->GetName().Contains(TEXT("FP_WeaponMount"))) WeaponMount = C;
    if (WeaponMount) WeaponNeutral = WeaponMount->GetRelativeTransform();
    TArray<UChildActorComponent*> WeaponChildren; GetComponents(WeaponChildren);
    for(auto* Child:WeaponChildren) if(auto* Weapon=Child->GetChildActor())
    {
        TArray<USceneComponent*> Parts; Weapon->GetComponents(Parts);
        for(auto* Part:Parts)
        {
            if(Part->GetName()==TEXT("MuzzlePoint") && WeaponMount && P2Camera && Part->IsAttachedTo(WeaponMount))
            {
                const FQuat Correction = FQuat::FindBetweenNormals(Part->GetForwardVector(), P2Camera->GetForwardVector());
                WeaponMount->SetWorldRotation(Correction * WeaponMount->GetComponentQuat());
                WeaponNeutral = WeaponMount->GetRelativeTransform();
            }
            if(Part->GetName()==TEXT("GripPoint_Right")) P2GripRight=Part;
            if(Part->GetName()==TEXT("GripPoint_Left")) P2GripLeft=Part;
        }
    }
    GetMesh()->SetAnimInstanceClass(UPhase2AnimInstance::StaticClass());
    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetMesh()->bCastHiddenShadow = true;
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (P2Arms)
    {
        // Keep the verified head-socket hierarchy. The animation proxy places hands
        // around the camera-mounted weapon using component-space two-bone IK.
        P2Arms->SetLeaderPoseComponent(nullptr);
        P2Arms->SetAnimInstanceClass(UPhase2AnimInstance::StaticClass());
        P2Arms->SetVisibility(true);
        P2Arms->SetHiddenInGame(false);
        P2Arms->SetOwnerNoSee(false);
        P2Arms->SetOnlyOwnerSee(true);
        P2Arms->SetCastShadow(false);
        P2Arms->HideBoneByName(TEXT("neck_01"), EPhysBodyOp::PBO_None);
        P2Arms->HideBoneByName(TEXT("thigh_l"), EPhysBodyOp::PBO_None);
        P2Arms->HideBoneByName(TEXT("thigh_r"), EPhysBodyOp::PBO_None);
        P2Arms->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    GetCharacterMovement()->MaxAcceleration = 3000;
    GetCharacterMovement()->BrakingDecelerationWalking = 3000;
    GetCharacterMovement()->MaxStepHeight = 45;
    RefreshSpeed();
}

void APhase2Character::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    auto* E = Cast<UEnhancedInputComponent>(Input);
    if (!E) { UE_LOG(LogTemp, Error, TEXT("PHASE2: EnhancedInputComponent missing")); return; }
    InteractionManager->BindInput(E);
    Equipment->BindInput(E);
    FoundationSession->BindInput(Input);
    Input->BindKey(EKeys::Y,IE_Pressed,this,&APhase2Character::Bash);
    Input->BindKey(EKeys::U,IE_Pressed,this,&APhase2Character::GrabOrRelease);
    Input->BindKey(EKeys::O,IE_Pressed,this,&APhase2Character::HideBody);
    Input->BindKey(EKeys::K,IE_Pressed,this,&APhase2Character::Whistle);
    Input->BindKey(EKeys::Five,IE_Pressed,this,&APhase2Character::OrderFollow);
    Input->BindKey(EKeys::Six,IE_Pressed,this,&APhase2Character::OrderHold);
    Input->BindKey(EKeys::MiddleMouseButton,IE_Pressed,this,&APhase2Character::OrderMove);
    Input->BindKey(EKeys::Seven,IE_Pressed,this,&APhase2Character::OrderCover);
    Input->BindKey(EKeys::Eight,IE_Pressed,this,&APhase2Character::OrderFocus);
    Input->BindKey(EKeys::Nine,IE_Pressed,this,&APhase2Character::OrderCeaseFire);
    auto Bind = [this,E](const TCHAR* Name, auto On, auto Off)
    {
        UInputAction* A = LoadObject<UInputAction>(nullptr, *FString::Printf(TEXT("/Game/Input/Actions/%s.%s"),Name,Name));
        if (!A) { UE_LOG(LogTemp, Error, TEXT("PHASE2: missing action %s"),Name); return; }
        BoundActions.Add(A);
        E->BindAction(A, ETriggerEvent::Started, this, On);
        E->BindAction(A, ETriggerEvent::Completed, this, Off);
        E->BindAction(A, ETriggerEvent::Canceled, this, Off);
    };
    Bind(TEXT("IA_Sprint"), &APhase2Character::SprintOn, &APhase2Character::SprintOff);
    Bind(TEXT("IA_Crouch"), &APhase2Character::CrouchOn, &APhase2Character::CrouchOff);
    Bind(TEXT("IA_SlowWalk"), &APhase2Character::SlowOn, &APhase2Character::SlowOff);
    Bind(TEXT("IA_LeanLeft"), &APhase2Character::LeftOn, &APhase2Character::LeftOff);
    Bind(TEXT("IA_LeanRight"), &APhase2Character::RightOn, &APhase2Character::RightOff);
    auto BindOnce = [this,E](const TCHAR* Name, auto On)
    {
        UInputAction* A=LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/Input/Actions/%s.%s"),Name,Name));
        if(A) { BoundActions.Add(A); E->BindAction(A,ETriggerEvent::Started,this,On); }
    };
    BindOnce(TEXT("IA_Prone"), &APhase2Character::ProneToggle);
    // Enhanced Input's delegate expects a void method.
    if (UInputAction* A=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/Actions/IA_Traversal.IA_Traversal")))
    {
        BoundActions.Add(A);
        E->BindActionValueLambda(A,ETriggerEvent::Started,[this](const FInputActionValue&){ P2TryTraversal(); });
    }
}

void APhase2Character::SprintOn() { bSprintHeld=true; RefreshSpeed(); }
void APhase2Character::SprintOff() { bSprintHeld=false; RefreshSpeed(); }
void APhase2Character::SlowOn() { bSlowHeld=true; RefreshSpeed(); }
void APhase2Character::SlowOff() { bSlowHeld=false; RefreshSpeed(); }
void APhase2Character::CrouchOn() { bCrouchHeld=true; if(P2Stance!=2) P2SetStance(1); }
void APhase2Character::CrouchOff() { bCrouchHeld=false; if(P2Stance==1) P2SetStance(0); }
void APhase2Character::ProneToggle() { P2SetStance(P2Stance==2 ? (bCrouchHeld?1:0):2); }
void APhase2Character::LeftOn() { bLeftHeld=true; }
void APhase2Character::LeftOff() { bLeftHeld=false; }
void APhase2Character::RightOn() { bRightHeld=true; }
void APhase2Character::RightOff() { bRightHeld=false; }
void APhase2Character::FireFeedback()
{
    if(P2IsDead()) return;
    TArray<UChildActorComponent*> WeaponChildren; GetComponents(WeaponChildren);
    for(auto* Child:WeaponChildren) if(auto* Weapon=Child->GetChildActor())
        if(auto* Ammo=FindFProperty<FIntProperty>(Weapon->GetClass(),TEXT("CurrentAmmo")))
            if(Ammo->GetPropertyValue_InContainer(Weapon)>0) P2Recoil=1;
}

void APhase2Character::SyncBlueprintState()
{
    if(auto* P=FindFProperty<FBoolProperty>(GetClass(),TEXT("bIsSprinting"))) P->SetPropertyValue_InContainer(this,P2Sprinting);
    if(auto* P=FindFProperty<FBoolProperty>(GetClass(),TEXT("bIsCrawling"))) P->SetPropertyValue_InContainer(this,P2Stance==2);
    if(auto* P=FindFProperty<FBoolProperty>(GetClass(),TEXT("SlowWalkActive"))) P->SetPropertyValue_InContainer(this,bSlowHeld && P2Stance==0 && !P2IsDead());
    if(auto* P=FindFProperty<FByteProperty>(GetClass(),TEXT("PlayerStance"))) P->SetPropertyValue_InContainer(this,uint8(P2Stance));
}

void APhase2Character::RefreshSpeed()
{
    P2Sprinting=bSprintHeld && !bSlowHeld && P2Stance==0 && !P2Traversing && !P2IsDead() && !TacticalTarget;
    float SlowSpeed=200;
    if(const auto* P=FindFProperty<FNumericProperty>(GetClass(),TEXT("SlowWalkSpeed")))
        if(P->IsFloatingPoint()) SlowSpeed=FMath::Clamp(float(P->GetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(this))),1.f,450.f);
    GetCharacterMovement()->MaxWalkSpeed=P2Stance==2?100.f:P2Stance==1?200.f:P2Sprinting?700.f:bSlowHeld?SlowSpeed:450.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched=200;
    if(TacticalTarget)
    {
        GetCharacterMovement()->MaxWalkSpeed=FMath::Min(GetCharacterMovement()->MaxWalkSpeed,180.f);
        GetCharacterMovement()->MaxWalkSpeedCrouched=180;
    }
    SyncBlueprintState();
}

bool APhase2Character::CapsuleClear(const FVector& C,float R,float H) const
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(Phase2Clearance),false,this);
    return !GetWorld()->OverlapBlockingTestByChannel(C,FQuat::Identity,ECC_Pawn,
        FCollisionShape::MakeCapsule(R-0.5f,H-0.5f),Q,GetCapsuleComponent()->GetCollisionResponseToChannels());
}

bool APhase2Character::P2SetStance(int32 S)
{
    if(S<0 || S>2 || P2IsDead() || P2Traversing || TacticalTarget || GetCharacterMovement()->IsFalling()) return false;
    if(S==P2Stance) return true;
    const float H=S==0?StandHalf:S==1?60.f:30.f;
    const float R=S==2?26.f:StandRadius;
    const float OldH=GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    const FVector C=GetActorLocation()+FVector(0,0,H-OldH);
    if(!CapsuleClear(C,R,H)) return false;
    if(S==2 && GetWorld()->OverlapBlockingTestByChannel(C-FVector(0,0,H)+FVector(0,0,24),
        GetActorQuat(),ECC_Pawn,FCollisionShape::MakeBox(FVector(83,25,19)),
        FCollisionQueryParams(SCENE_QUERY_STAT(Phase2ProneEntry),false,this))) return false;
    // Keep the capsule bottom fixed; the upright prone capsule is supplemented
    // with a swept horizontal body volume in Tick (it is not rotated).
    GetCapsuleComponent()->SetCapsuleSize(R,H,false);
    SetActorLocation(C,false,nullptr,ETeleportType::TeleportPhysics);
    GetCapsuleComponent()->UpdateOverlaps();
    P2Stance=S;
    GetMesh()->SetRelativeLocation(MeshOffset+FVector(0,0,StandHalf-H));
    EyeFrom=VisualEye; EyeTo=S==0?EyeHeight:S==1?105.f:39.f; EyeElapsed=0;
    RefreshSpeed();
    return true;
}

bool APhase2Character::CanJumpInternal_Implementation() const
{
    return P2Stance==0 && !P2Traversing && !TacticalTarget && !P2IsDead() && Super::CanJumpInternal_Implementation();
}

void APhase2Character::GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const
{
    if(P2Camera) { Location=P2Camera->GetComponentLocation(); Rotation=P2Camera->GetComponentRotation(); }
    else Super::GetActorEyesViewPoint(Location,Rotation);
}

void APhase2Character::FaceRotation(FRotator Rotation,float Dt)
{
    if(P2Stance==2)
    {
        const FVector Center=GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-24);
        if(GetWorld()->OverlapBlockingTestByChannel(Center,FRotator(0,Rotation.Yaw,0).Quaternion(),ECC_Pawn,
            FCollisionShape::MakeBox(FVector(83,25,19)),FCollisionQueryParams(SCENE_QUERY_STAT(Phase2ProneTurn),false,this))) return;
    }
    Super::FaceRotation(Rotation,Dt);
}

bool APhase2Character::SegmentClear(const FVector& A,const FVector& B) const
{
    FHitResult Hit;
    return !GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,
        GetCapsuleComponent()->GetCollisionShape(),FCollisionQueryParams(SCENE_QUERY_STAT(Phase2Traversal),false,this),
        GetCapsuleComponent()->GetCollisionResponseToChannels());
}

bool APhase2Character::P2TryTraversal()
{
    P2TraversalResult=TEXT("Rejected: state");
    if(P2IsDead() || P2Traversing || TacticalTarget || P2Stance!=0 || !GetCharacterMovement()->IsMovingOnGround()) return false;
    const float H=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float R=GetCapsuleComponent()->GetScaledCapsuleRadius();
    const FVector Bottom=GetActorLocation()-FVector(0,0,H);
    const FVector Forward=FRotator(0,GetControlRotation().Yaw,0).Vector();
    const FCollisionQueryParams Q(SCENE_QUERY_STAT(Phase2Ledge),false,this);
    FHitResult Wall,Top;
    // Adapted from ALS Refactored AlsCharacter_Actions.cpp mantling detection,
    // commit b754d6f0f2bb03741d301f8fb88077ebfe561e17. MIT, see ThirdPartyNotices.
    // Forward hit -> walkable downward hit -> capsule target/ceiling clearance.
    const FVector Start=Bottom+FVector(0,0,65);
    if(!GetWorld()->SweepSingleByChannel(Wall,Start,Start+Forward*135,FQuat::Identity,ECC_Pawn,
        FCollisionShape::MakeSphere(18),Q) || GetCharacterMovement()->IsWalkable(Wall) ||
        !Wall.GetComponent() || Wall.GetComponent()->Mobility==EComponentMobility::Movable)
    { P2TraversalResult=TEXT("Rejected: no stationary wall"); return false; }
    const FVector Beyond=Wall.ImpactPoint+Forward*(R+18);
    if(!GetWorld()->LineTraceSingleByChannel(Top,FVector(Beyond.X,Beyond.Y,Bottom.Z+155),
        FVector(Beyond.X,Beyond.Y,Bottom.Z+35),ECC_Pawn,Q) || !GetCharacterMovement()->IsWalkable(Top))
    { P2TraversalResult=TEXT("Rejected: no walkable ledge"); return false; }
    const float Height=Top.ImpactPoint.Z-Bottom.Z;
    if(Height<40 || Height>140) { P2TraversalResult=TEXT("Rejected: height"); return false; }
    FVector End=Top.ImpactPoint+FVector(0,0,H+3);
    const bool bVault=Height<=85;
    if(bVault)
    {
        FHitResult Landing;
        const FVector Far=Wall.ImpactPoint+Forward*(R+145);
        if(!GetWorld()->LineTraceSingleByChannel(Landing,FVector(Far.X,Far.Y,Top.ImpactPoint.Z+5),
            FVector(Far.X,Far.Y,Bottom.Z-35),ECC_Pawn,Q) || !GetCharacterMovement()->IsWalkable(Landing))
        { P2TraversalResult=TEXT("Rejected: landing"); return false; }
        End=Landing.ImpactPoint+FVector(0,0,H+3);
    }
    if(!CapsuleClear(End,R,H)) { P2TraversalResult=TEXT("Rejected: blocked landing"); return false; }
    const FVector Lift(GetActorLocation().X,GetActorLocation().Y,Top.ImpactPoint.Z+H+6);
    const FVector Across(End.X,End.Y,Lift.Z);
    if(!SegmentClear(GetActorLocation(),Lift) || !SegmentClear(Lift,Across) || !SegmentClear(Across,End))
    { P2TraversalResult=TEXT("Rejected: blocked path"); return false; }
    TraversalPoints={GetActorLocation(),Lift,Across,End};
    TraversalSegment=0; TraversalElapsed=0; P2Traversing=true;
    P2TraversalResult=bVault?TEXT("Vault in progress"):TEXT("Climb in progress");
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->MaxFlySpeed=0;
    bLeftHeld=bRightHeld=false;
    RefreshSpeed();
    return true;
}

void APhase2Character::FinishTraversal(const FString& Result)
{
    P2Traversing=false; P2TraversalResult=Result;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(P2IsDead()?MOVE_None:MOVE_Falling);
    RefreshSpeed();
}

void APhase2Character::UpdateTraversal(float Dt)
{
    TraversalElapsed+=Dt;
    const float Duration=TraversalSegment==1?0.32f:0.22f;
    const float Alpha=FMath::Clamp(TraversalElapsed/Duration,0.f,1.f);
    const FVector Target=FMath::Lerp(TraversalPoints[TraversalSegment],TraversalPoints[TraversalSegment+1],Phase2Movement::SimpleSpline(Alpha));
    FHitResult Hit;
    GetCharacterMovement()->SafeMoveUpdatedComponent(Target-GetActorLocation(),GetActorQuat(),true,Hit);
    if(Hit.bBlockingHit) { FinishTraversal(TEXT("Aborted: dynamic obstruction")); return; }
    if(Alpha>=1)
    {
        ++TraversalSegment; TraversalElapsed=0;
        if(TraversalSegment>=3) FinishTraversal(TEXT("Crossed obstacle"));
    }
}

bool APhase2Character::UsesCameraMotion() const
{
    return !FoundationSession || !FoundationSession->Preferences || FoundationSession->Preferences->CameraMotion;
}

void APhase2Character::UpdateCamera(float Dt)
{
    if(!P2Camera) return;
    EyeElapsed+=Dt;
    VisualEye=Phase2Movement::EyeTransition(EyeFrom,EyeTo,EyeElapsed/0.24f);
    const FRotator View=GetControlRotation();
    const FRotator Yaw(0,View.Yaw,0);
    const FVector Right=Yaw.RotateVector(FVector::RightVector);
    const FVector Bottom=GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    LandingTime+=Dt;
    const bool CosmeticMotion=UsesCameraMotion();
    const float LandingAlpha=FMath::Clamp(LandingTime/.25f,0.f,1.f);
    const bool Dead=P2IsDead();
    DeathViewAlpha=FMath::FInterpConstantTo(DeathViewAlpha,Dead?1.f:0.f,Dt,2.5f);
    const float DeathBlend=CosmeticMotion?FMath::SmoothStep(0.f,1.f,DeathViewAlpha):0.f;
    const float LandingDip=CosmeticMotion && !Dead?FMath::Sin(PI*LandingAlpha)*LandingStrength:0.f;
    const float DeathDrop=DeathBlend*FMath::Min(42.f,VisualEye*.45f);
    FVector Neutral=Bottom+Yaw.RotateVector(FVector(NeutralCameraOffset.X,NeutralCameraOffset.Y,0))
        +FVector(0,0,VisualEye-LandingDip-DeathDrop);
    const float Target=(P2IsDead() || P2Traversing || P2Stance==2)?0.f:float(int(bRightHeld)-int(bLeftHeld));
    float Desired=FMath::FInterpTo(P2Lean,Target,Dt,10);
    FHitResult Hit;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(Phase2Lean),false,this);
    // Sweep from the capsule axis as well as from the neutral eye so that leaning
    // cannot start on the far side of a wall or clip while changing stance.
    const FVector Axis=Bottom+FVector(0,0,VisualEye);
    if(GetWorld()->SweepSingleByChannel(Hit,Axis,Neutral,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(9),Q))
        Neutral=FMath::Lerp(Axis,Neutral,FMath::Max(0.f,Hit.Time-0.02f));
    const FVector DesiredEye=Neutral+Right*(Desired*32);
    if(GetWorld()->SweepSingleByChannel(Hit,Neutral,DesiredEye,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(9),Q))
        Desired*=FMath::Max(0.f,Hit.Time-0.03f);
    P2Lean=Desired;
    if(P2Arms)
    {
        const FVector ArmOrigin=Neutral+Right*(P2Lean*32)-Yaw.Vector()*12-FVector(0,0,150);
        P2Arms->SetWorldLocationAndRotation(ArmOrigin,FRotator(0,View.Yaw-90,0));
    }
    P2Camera->SetWorldLocation(Neutral+Right*(P2Lean*32));
    const FQuat CameraMotion=CosmeticMotion
        ?FRotator((Dead?0.f:P2Recoil*.45f)-DeathBlend*8.f,0.f,P2Lean*8.f+DeathBlend*12.f).Quaternion()
        :FQuat::Identity;
    P2Camera->SetWorldRotation(View.Quaternion()*NeutralViewCorrection*CameraMotion);
    if(WeaponMount)
    {
        FTransform T=WeaponNeutral;
        const float Speed=GetVelocity().Size2D();
        const float Bob=CosmeticMotion?FMath::Sin(MotionClock*(P2Sprinting?13.f:9.f))*FMath::Clamp(Speed/450.f,0.f,1.f):0.f;
        const float Kick=CosmeticMotion?P2Recoil*3:0.f;
        const float HipWeight=Equipment?1.f-Equipment->AdsAlpha:1.f;
        const float Idle=CosmeticMotion && !Dead && GetCharacterMovement()->IsMovingOnGround()
            ?(1.f-FMath::Clamp(Speed/90.f,0.f,1.f))*HipWeight:0.f;
        const float Breath=Idle*FMath::Sin(MotionClock*1.65f);
        const float Air=CosmeticMotion && GetCharacterMovement()->IsFalling()
            ?FMath::Clamp(GetVelocity().Z/500.f,-1.f,1.f)*HipWeight:0.f;
        const float Tuck=(P2PoseCrouch+1.5f*P2PoseProne)*HipWeight;
        T.AddToTranslation(FVector(-Kick-2.f*Tuck,Breath*.28f,
            Bob*.5f+Breath*.35f-Air*1.8f-LandingDip*.35f-Tuck-(P2Sprinting?3.f:0.f)));
        T.SetRotation(WeaponNeutral.GetRotation()*FRotator(Kick+Breath*.32f-Air*3.f,
            0,(P2Sprinting?-12.f:0.f)-Tuck*4.f).Quaternion());
        WeaponMount->SetRelativeTransform(T);
    }
}

void APhase2Character::Tick(float Dt)
{
    Super::Tick(Dt);
    // The map contains a parked Blueprint preview at Z=10000. It must not fall
    // into the arena as a second, blocking player after PIE begins.
    if(!IsPlayerControlled())
    {
        SetActorHiddenInGame(true); SetActorEnableCollision(false);
        GetMesh()->SetComponentTickEnabled(false);
        if(P2Arms) P2Arms->SetComponentTickEnabled(false);
        GetCharacterMovement()->DisableMovement(); SetActorTickEnabled(false);
        return;
    }
    MotionClock+=Dt;
    if(!TacticalCaption.IsEmpty() && GetWorld()->GetTimeSeconds()>=CaptionUntil)TacticalCaption=FText::GetEmpty();
    const bool Dead=P2IsDead();
    if(Dead && !bWasDead)
    {
        bSprintHeld=bSlowHeld=bLeftHeld=bRightHeld=bCrouchHeld=false;
        if(P2Traversing) FinishTraversal(TEXT("Aborted: death"));
        P2Lean=0; RefreshSpeed(); GetCharacterMovement()->DisableMovement();
        if(auto* PC=Cast<APlayerController>(GetController());PC && PC->PlayerCameraManager)
            PC->PlayerCameraManager->StartCameraFade(0.f,.72f,.55f,FLinearColor::Black,false,true);
    }
    bWasDead=Dead;
    if(!Dead && !GetCharacterMovement()->IsFalling() && !P2Traversing)
    {
        FootstepDistance+=GetVelocity().Size2D()*Dt;
        if(FootstepDistance>(P2Sprinting?180.f:P2Stance==2?100.f:140.f))
        {
            FootstepDistance=0;++P2Footsteps;
            const float Loud=P2Stance==2?.04f:P2Stance==1?.08f:bSlowHeld?.12f:P2Sprinting?1.f:.45f;
            const float Range=P2Stance==2?120.f:P2Stance==1?250.f:bSlowHeld?350.f:P2Sprinting?1400.f:800.f;
            UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),Loud,this,Range,TEXT("Footstep"));
            if(auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/FoundationUse.FoundationUse")))UGameplayStatics::PlaySoundAtLocation(this,S,GetActorLocation()-FVector(0,0,90),Loud*.35f,.5f);
        }
    }
    if(!Dead && P2Stance==1 && !bCrouchHeld) P2SetStance(0);
    if(P2Traversing) UpdateTraversal(Dt);
    P2PoseCrouch=FMath::FInterpTo(P2PoseCrouch,P2Stance==1?1.f:0.f,Dt,12);
    P2PoseProne=FMath::FInterpTo(P2PoseProne,P2Stance==2?1.f:0.f,Dt,12);
    P2Recoil=FMath::FInterpTo(P2Recoil,0.f,Dt,14);
    UpdateCamera(Dt);
    if(P2DeveloperDiagnostics && FMath::FloorToInt(MotionClock)!=FMath::FloorToInt(MotionClock-Dt))
    {
        UE_LOG(LogTemp,Display,TEXT("P2 stance=%d speed=%.1f half=%.1f lean=%.3f position=%s traversal=%s"),
            P2Stance,GetVelocity().Size2D(),GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),P2Lean,
            *GetActorLocation().ToCompactString(),*P2TraversalResult);
    }
}

void APhase2Character::Landed(const FHitResult& Hit)
{
    const float ImpactSpeed=-GetVelocity().Z;
    Super::Landed(Hit);
    if(IsPlayerControlled() && !P2IsDead())
    {
        LandingTime=0.f;
        LandingStrength=FMath::GetMappedRangeValueClamped(FVector2f(100,650),FVector2f(0,6),ImpactSpeed);
        UAISense_Hearing::ReportNoiseEvent(GetWorld(),Hit.ImpactPoint,.7f,this,1200,TEXT("Landing"));
    }
}

bool UPhase2CharacterMovement::MoveUpdatedComponentImpl(const FVector& Delta,const FQuat& Rotation,
    bool bSweep,FHitResult* OutHit,ETeleportType Teleport)
{
    const auto* C=Cast<APhase2Character>(CharacterOwner);
    if(C && C->P2Stance==2 && bSweep && (FMath::Abs(Delta.X)+FMath::Abs(Delta.Y)>KINDA_SMALL_NUMBER))
    {
        const FVector Center=C->GetActorLocation()-FVector(0,0,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-24);
        FHitResult BodyHit;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(Phase2ProneBody),false,C);
        if(GetWorld()->SweepSingleByChannel(BodyHit,Center,Center+Delta,C->GetActorQuat(),ECC_Pawn,
            FCollisionShape::MakeBox(FVector(83,25,19)),Q,C->GetCapsuleComponent()->GetCollisionResponseToChannels()))
        {
            const bool Moved=Super::MoveUpdatedComponentImpl(Delta*FMath::Max(0.f,BodyHit.Time-0.01f),Rotation,bSweep,OutHit,Teleport);
            if(OutHit) *OutHit=BodyHit;
            return Moved;
        }
    }
    return Super::MoveUpdatedComponentImpl(Delta,Rotation,bSweep,OutHit,Teleport);
}

bool UPhase2Tools::BeginPlayerTest()
{
#if WITH_EDITOR
    if(!GEditor || GEditor->PlayWorld) return false;
    FRequestPlaySessionParams P;
    P.WorldType=EPlaySessionWorldType::PlayInEditor;
    P.SessionDestination=EPlaySessionDestinationType::InProcess;
    GEditor->RequestPlaySession(P);
    return true;
#else
    return false;
#endif
}

bool UPhase2Tools::InjectAction(APlayerController* Player,UInputAction* Action,FVector Value)
{
    if(!Player || !Action) return false;
    auto* Input=Cast<UEnhancedPlayerInput>(Player->PlayerInput);
    if(!Input) return false;
    FInputActionValue V=Action->ValueType==EInputActionValueType::Boolean?FInputActionValue(Value.X!=0):
        Action->ValueType==EInputActionValueType::Axis1D?FInputActionValue(float(Value.X)):
        Action->ValueType==EInputActionValueType::Axis2D?FInputActionValue(FVector2D(Value.X,Value.Y)):FInputActionValue(Value);
    Input->InjectInputForAction(Action,V);
    return true;
}

bool UPhase2Tools::TestKey(APlayerController* Player,FName KeyName,bool Pressed,float Amount)
{
    if(!Player) return false;
    const FKey Key(KeyName);
    if(!Key.IsValid()) return false;
    const bool Axis=Key.IsAxis1D() || Key.IsAxis2D();
    FInputKeyEventArgs Args(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,
        Axis?IE_Axis:Pressed?IE_Pressed:IE_Released,Pressed?Amount:0.f,false,FPlatformTime::Cycles64());
    Args.DeltaTime=1.f/60.f; Args.NumSamples=1;
    return Player->InputKey(Args);
}

bool UPhase2Tools::MapInput(UObject* Context,UObject* Action,FName KeyName)
{
    auto* Mapping=Cast<UInputMappingContext>(Context);auto* Input=Cast<UInputAction>(Action);
    const FKey Key(KeyName);if(!Mapping || !Input || !Key.IsValid()) return false;
    for(const auto& M:Mapping->GetMappings()) if(M.Action==Input && M.Key==Key) return true;
    Mapping->Modify();Mapping->MapKey(Input,Key);Mapping->MarkPackageDirty();return true;
}

bool UPhase2Tools::AddAimModifier(UObject* Object)
{
    auto* Action=Cast<UInputAction>(Object);if(!Action)return false;
    for(const auto& M:Action->Modifiers)if(M && M->IsA<UFoundationAimModifier>())return true;
    Action->Modify();Action->Modifiers.Add(NewObject<UFoundationAimModifier>(Action));Action->MarkPackageDirty();return true;
}

FString UPhase2Tools::IntegratePlayer(UObject* Object)
{
#if WITH_EDITOR
    UBlueprint* BP=Cast<UBlueprint>(Object);
    if(!BP || GEditor->PlayWorld) return TEXT("FAILED: invalid Blueprint or PIE active");
    TArray<UEdGraph*> Graphs;
    BP->GetAllGraphs(Graphs);
    int32 Disabled=0;
    const TArray<FString> Actions={TEXT("IA_Sprint"),TEXT("IA_Crouch"),TEXT("IA_Prone"),TEXT("IA_LeanLeft"),TEXT("IA_LeanRight"),TEXT("IA_Traversal"),TEXT("IA_SlowWalk"),TEXT("IA_Interact"),TEXT("IA_Fire")};
    for(UEdGraph* G:Graphs) for(UEdGraphNode* N:G->Nodes)
    {
        if(N->GetClass()->GetName()==TEXT("K2Node_EnhancedInputAction"))
        {
            const FString Title=N->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
            if(Actions.ContainsByPredicate([&Title](const FString& A){return Title.Contains(A);}))
            {
                N->Modify();
                for(UEdGraphPin* P:N->Pins)
                    if(P->Direction==EGPD_Output && P->PinType.PinCategory==UEdGraphSchema_K2::PC_Exec) P->BreakAllPinLinks();
                ++Disabled;
            }
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    return FString::Printf(TEXT("%s: disabled %d legacy movement input nodes"),BP->Status==BS_Error?TEXT("FAILED"):TEXT("COMPILED"),Disabled);
#else
    return TEXT("FAILED: editor only");
#endif
}

AActor* UPhase2Tools::TestCamera(AActor* Player,FVector Location,FRotator Rotation)
{
#if WITH_EDITOR
    return Player && Player->GetWorld()->WorldType==EWorldType::PIE?
        Player->GetWorld()->SpawnActor<ACameraActor>(Location,Rotation):nullptr;
#else
    return nullptr;
#endif
}
AActor* UPhase2Tools::TestBlocker(AActor* Player,FVector Location,FVector Size)
{
#if WITH_EDITOR
    if(!Player || Player->GetWorld()->WorldType!=EWorldType::PIE) return nullptr;
    auto* A=Player->GetWorld()->SpawnActor<AStaticMeshActor>(Location,FRotator::ZeroRotator);
    auto* M=A->GetStaticMeshComponent();
    M->SetMobility(EComponentMobility::Movable);
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    M->SetCollisionProfileName(TEXT("BlockAll"));
    A->SetActorScale3D(Size/100.f);
    M->SetMobility(EComponentMobility::Static);
    return A;
#else
    return nullptr;
#endif
}
