#pragma once
#include "CoreMinimal.h"
#include "FoundationInteraction.h"
#include "FoundationThrowables.generated.h"
class UStaticMeshComponent;class UProjectileMovementComponent;
class UPhysicsConstraintComponent;
UENUM(BlueprintType)
enum class EFoundationThrowableKind:uint8 { Bullet,Stone,Knife,Frag,Smoke,Flash };

UCLASS(Blueprintable)
class AFGHANISTANFPS_API AFoundationThrowable:public AActor,public IFoundationInteractableInterface
{
    GENERATED_BODY()
public:
    AFoundationThrowable();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float Dt) override;
    UFUNCTION(BlueprintCallable) void Configure(EFoundationThrowableKind NewKind);
    UFUNCTION(BlueprintCallable) void HoldAt(APawn* User,USceneComponent* Hand);
    UFUNCTION(BlueprintCallable) void Launch(FVector Velocity);
    UFUNCTION(BlueprintCallable) void Detonate();
    UFUNCTION(BlueprintPure) FName InventoryId() const;
    virtual FFoundationUseInfo GetUseInfo_Implementation(APawn* User) const override;
    virtual bool ReceiveUse_Implementation(APawn* User,EFoundationUsePhase Phase,float Value) override;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UProjectileMovementComponent> Projectile;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPhysicsConstraintComponent> OwnerIgnoreConstraint;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EFoundationThrowableKind Kind=EFoundationThrowableKind::Stone;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float FuseSeconds=3;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Damage=100;
    UPROPERTY(BlueprintReadOnly) float RemainingFuse=0;
    UPROPERTY(BlueprintReadOnly) bool Held=false;
    UPROPERTY(BlueprintReadOnly) bool Launched=false;
    UPROPERTY(BlueprintReadOnly) bool Impacted=false;
    UPROPERTY(BlueprintReadOnly) bool Detonated=false;
    UPROPERTY(BlueprintReadOnly) bool Consumed=false;
    UPROPERTY(BlueprintReadOnly) int32 ImpactEvents=0;
    UPROPERTY(BlueprintReadOnly) FVector LastImpact;
    UPROPERTY(BlueprintReadOnly) float DetonatedAt=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SmokeRadius=300;
private:
    UFUNCTION() void Hit(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Result);
    void ImpactCue(const FVector& Location,float Loudness,FName Tag);
    bool Armed=false;
    float LastNoise=-100;
};
