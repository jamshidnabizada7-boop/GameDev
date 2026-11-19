#include "FoundationThrowables.h"
#include "FoundationEquipment.h"
#include "FoundationBridge.h"
#include "Phase2Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AFoundationThrowable::AFoundationThrowable()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bStartWithTickEnabled=false;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThrowableBody"));SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));Body->SetStaticMesh(Sphere.Object);
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));Body->SetNotifyRigidBodyCollision(true);Body->SetUseCCD(true);Body->SetLinearDamping(.05f);Body->SetAngularDamping(.2f);
    Body->OnComponentHit.AddDynamic(this,&AFoundationThrowable::Hit);
    Projectile=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("BallisticMovement"));Projectile->bAutoActivate=false;
    Projectile->bRotationFollowsVelocity=true;Projectile->bForceSubStepping=true;Projectile->ProjectileGravityScale=0;Projectile->SetUpdatedComponent(Body);
    // A free joint supplies a per-body collision exclusion for the thrower's
    // capsule. Move-ignore lists alone do not affect simulated rigid bodies.
    OwnerIgnoreConstraint=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("OwnerCollisionExclusion"));OwnerIgnoreConstraint->SetupAttachment(Body);
    OwnerIgnoreConstraint->SetLinearXLimit(LCM_Free,0);OwnerIgnoreConstraint->SetLinearYLimit(LCM_Free,0);OwnerIgnoreConstraint->SetLinearZLimit(LCM_Free,0);
    OwnerIgnoreConstraint->SetAngularSwing1Limit(ACM_Free,0);OwnerIgnoreConstraint->SetAngularSwing2Limit(ACM_Free,0);OwnerIgnoreConstraint->SetAngularTwistLimit(ACM_Free,0);OwnerIgnoreConstraint->SetDisableCollision(true);
}
void AFoundationThrowable::OnConstruction(const FTransform& Transform){Super::OnConstruction(Transform);Configure(Kind);}
void AFoundationThrowable::Configure(EFoundationThrowableKind NewKind)
{
    Kind=NewKind;
    const bool Blade=Kind==EFoundationThrowableKind::Knife;
    Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Blade?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Body->SetRelativeScale3D(Blade?FVector(.30,.025,.006):Kind==EFoundationThrowableKind::Bullet?FVector(.02):Kind==EFoundationThrowableKind::Stone?FVector(.07):FVector(.10));
    Body->SetMassOverrideInKg(NAME_None,Kind==EFoundationThrowableKind::Stone?.12f:Blade?.35f:.5f,true);
    Body->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
}
FName AFoundationThrowable::InventoryId() const
{
    switch(Kind){case EFoundationThrowableKind::Stone:return TEXT("stone");case EFoundationThrowableKind::Knife:return TEXT("knife");case EFoundationThrowableKind::Frag:return TEXT("frag");case EFoundationThrowableKind::Smoke:return TEXT("smoke");case EFoundationThrowableKind::Flash:return TEXT("flash");default:return NAME_None;}
}
void AFoundationThrowable::HoldAt(APawn* User,USceneComponent* Hand)
{
    if(!User || !Hand)return;SetOwner(User);SetInstigator(User);Held=true;Launched=false;
    Body->SetSimulatePhysics(false);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttachToComponent(Hand,FAttachmentTransformRules::KeepRelativeTransform);SetActorRelativeLocation(FVector(42,10,-16));SetActorRelativeRotation(FRotator::ZeroRotator);
    Armed=Kind==EFoundationThrowableKind::Frag || Kind==EFoundationThrowableKind::Smoke || Kind==EFoundationThrowableKind::Flash;
    RemainingFuse=FuseSeconds;SetActorTickEnabled(Armed);
}
void AFoundationThrowable::Launch(FVector Velocity)
{
    if(Detonated || Consumed || Launched)return;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);Held=false;Launched=true;
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));Body->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    if(GetOwner())Body->IgnoreActorWhenMoving(GetOwner(),true);
    if(Kind==EFoundationThrowableKind::Bullet)
    {
        Body->SetSimulatePhysics(false);Projectile->SetUpdatedComponent(Body);Projectile->Velocity=Velocity;Projectile->InitialSpeed=Velocity.Size();Projectile->MaxSpeed=Velocity.Size();Projectile->Activate(true);SetLifeSpan(3);
    }
    else
    {
        Body->SetSimulatePhysics(true);Body->SetPhysicsLinearVelocity(Velocity);Body->SetPhysicsAngularVelocityInDegrees(Kind==EFoundationThrowableKind::Knife?FVector(0,0,0):FVector(180,120,90));
        if(auto* Character=Cast<ACharacter>(GetInstigator()))OwnerIgnoreConstraint->SetConstrainedComponents(Body,NAME_None,Character->GetCapsuleComponent(),NAME_None);
        if(!Armed && (Kind==EFoundationThrowableKind::Frag || Kind==EFoundationThrowableKind::Smoke || Kind==EFoundationThrowableKind::Flash)){Armed=true;RemainingFuse=FuseSeconds;}
        SetActorTickEnabled(Armed);
    }
}
void AFoundationThrowable::ImpactCue(const FVector& Location,float Loudness,FName Tag)
{
    const float Now=GetWorld()->GetTimeSeconds();if(Now-LastNoise<.15f)return;LastNoise=Now;++ImpactEvents;LastImpact=Location;
    UAISense_Hearing::ReportNoiseEvent(GetWorld(),Location,Loudness,GetInstigator(),Kind==EFoundationThrowableKind::Stone?1400:700,Tag);
    if(auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/FoundationUse.FoundationUse")))UGameplayStatics::PlaySoundAtLocation(this,Sound,Location,.8f,Kind==EFoundationThrowableKind::Stone?1.5f:.8f);
}
void AFoundationThrowable::Hit(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Result)
{
    if(!Launched || Detonated || Consumed || !Other || Other==GetOwner())return;
    if(Kind==EFoundationThrowableKind::Bullet || Kind==EFoundationThrowableKind::Knife)
    {
        if(Impacted)return;Impacted=true;const FVector Direction=Kind==EFoundationThrowableKind::Bullet?Projectile->Velocity.GetSafeNormal():Body->GetPhysicsLinearVelocity().GetSafeNormal();
        const float HealthBefore=float(FoundationBridge::Number(FoundationBridge::Health(Other),TEXT("CurrentHealth")));
        UGameplayStatics::ApplyPointDamage(Other,Damage,Direction,Result,GetInstigatorController(),this,UDamageType::StaticClass());
        if(auto* Pawn=GetInstigator())if(auto* Equipment=Pawn->FindComponentByClass<UFoundationEquipment>())Equipment->NotifyProjectileImpact(Other,HealthBefore);
        ImpactCue(Result.ImpactPoint,.6f,TEXT("KnifeImpact"));
        if(Kind==EFoundationThrowableKind::Bullet){Destroy();return;}
        OwnerIgnoreConstraint->BreakConstraint();Body->SetSimulatePhysics(false);Projectile->Deactivate();if(OtherComponent)AttachToComponent(OtherComponent,FAttachmentTransformRules::KeepWorldTransform,Result.BoneName);
        Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));Body->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    }
    else if(Impulse.Size()>Body->GetMass()*120)
        ImpactCue(Result.ImpactPoint,Kind==EFoundationThrowableKind::Stone?.8f:.4f,Kind==EFoundationThrowableKind::Stone?FName(TEXT("StoneImpact")):FName(TEXT("ThrownImpact")));
}
void AFoundationThrowable::Tick(float Dt)
{
    Super::Tick(Dt);if(Armed && !Detonated){RemainingFuse=FMath::Max(0.f,RemainingFuse-Dt);if(RemainingFuse<=0)Detonate();}
}
void AFoundationThrowable::Detonate()
{
    if(Detonated || !Armed)return;Detonated=true;Armed=false;Held=false;DetonatedAt=GetWorld()->GetTimeSeconds();
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);OwnerIgnoreConstraint->BreakConstraint();Body->SetSimulatePhysics(false);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Projectile->Deactivate();SetActorTickEnabled(false);
    const FVector Location=Body->GetComponentLocation();UAISense_Hearing::ReportNoiseEvent(GetWorld(),Location,1,GetInstigator(),5000,TEXT("Explosion"));
    if(Kind==EFoundationThrowableKind::Frag)
    {
        TArray<AActor*> Ignore{this};UGameplayStatics::ApplyRadialDamageWithFalloff(this,120,10,Location,100,350,1,UDamageType::StaticClass(),Ignore,this,GetInstigatorController(),ECC_Visibility);
        Body->SetWorldScale3D(FVector(4));if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/AfghanFPS/Weapons/Foundation/M_Explosion.M_Explosion")))Body->SetMaterial(0,M);
        if(auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Explosion.Explosion")))UGameplayStatics::PlaySoundAtLocation(this,S,Location);
        SetLifeSpan(.2f);
    }
    else if(Kind==EFoundationThrowableKind::Smoke)
    {
        Body->SetWorldScale3D(FVector(SmokeRadius/50));if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/AfghanFPS/Weapons/Foundation/M_Smoke.M_Smoke")))Body->SetMaterial(0,M);
        SetLifeSpan(8);
    }
    else if(Kind==EFoundationThrowableKind::Flash)
    {
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
        {
            APlayerController* PC=It->Get();auto* P=PC?Cast<APhase2Character>(PC->GetPawn()):nullptr;if(!P || !P->P2Camera || !P->Equipment)continue;
            const FVector Eye=P->P2Camera->GetComponentLocation(),ToFlash=Location-Eye;const float Distance=ToFlash.Size();
            if(Distance>1000 || FVector::DotProduct(P->P2Camera->GetForwardVector(),ToFlash.GetSafeNormal())<-.2f)continue;
            FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FoundationFlash),false,this);Q.AddIgnoredActor(P);
            if(!GetWorld()->LineTraceSingleByChannel(Hit,Location,Eye,ECC_Visibility,Q))P->Equipment->FlashAlpha=FMath::Max(P->Equipment->FlashAlpha,FMath::Clamp(1-Distance/1000,.1f,1.f));
        }
        Body->SetVisibility(false);SetLifeSpan(2);
    }
}
FFoundationUseInfo AFoundationThrowable::GetUseInfo_Implementation(APawn* User) const
{
    FFoundationUseInfo Info;Info.Mode=EFoundationUseMode::Hold;Info.HoldSeconds=.5f;
    Info.Allowed=!Held && !Consumed && !Detonated && (Kind==EFoundationThrowableKind::Stone || (Kind==EFoundationThrowableKind::Knife && Impacted)) && Body->GetPhysicsLinearVelocity().Size()<80;
    Info.Prompt=FText::FromString(Info.Allowed?(Kind==EFoundationThrowableKind::Knife?TEXT("Hold E: retrieve knife"):TEXT("Hold E: retrieve stone")):TEXT(""));return Info;
}
bool AFoundationThrowable::ReceiveUse_Implementation(APawn* User,EFoundationUsePhase Phase,float Value)
{
    if(!GetUseInfo_Implementation(User).Allowed)return false;
    if(Phase==EFoundationUsePhase::Complete)
    {
        auto* Manager=User?User->FindComponentByClass<UFoundationInteractionComponent>():nullptr;if(!Manager || !Manager->GrantItem(InventoryId(),1))return false;
        Consumed=true;Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetActorHiddenInGame(true);Destroy();return true;
    }
    return true;
}
