#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Phase2Character.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputAction;
class UFoundationInteractionComponent;
class UFoundationEquipment;

// The existing player Blueprint remains the playable class and owns combat/HUD.
UCLASS()
class AFGHANISTANFPS_API APhase2Character : public ACharacter
{
    GENERATED_BODY()
public:
    APhase2Character(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual bool CanJumpInternal_Implementation() const override;
    virtual void GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const override;
    virtual void FaceRotation(FRotator Rotation, float DeltaTime) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* EventInstigator,AActor* DamageSource) override;
    UFUNCTION(BlueprintCallable, Category="Phase2") bool P2SetStance(int32 NewStance);
    UFUNCTION(BlueprintCallable, Category="Phase2") bool P2TryTraversal();
    UFUNCTION(BlueprintPure, Category="Phase2") bool P2IsDead() const;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") int32 P2Stance = 0;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") bool P2Sprinting = false;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") bool P2Traversing = false;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") float P2Lean = 0;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") float P2PoseCrouch = 0;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") float P2PoseProne = 0;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") float P2Recoil = 0;
    UPROPERTY(BlueprintReadOnly, Category="Foundation") float P2SurrenderAlpha=0;
    UPROPERTY(BlueprintReadOnly, Category="Foundation") float P2StaggerAlpha=0;
    UPROPERTY(BlueprintReadOnly, Category="Foundation") bool P2ThirdPersonHands=false;
    UPROPERTY(BlueprintReadOnly, Category="Foundation") int32 P2Footsteps=0;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") FString P2TraversalResult;
    UPROPERTY(EditDefaultsOnly, Category="Phase2") bool P2DeveloperDiagnostics = false;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") TObjectPtr<USkeletalMeshComponent> P2Arms;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") TObjectPtr<UCameraComponent> P2Camera;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") TObjectPtr<USceneComponent> P2GripRight;
    UPROPERTY(BlueprintReadOnly, Category="Phase2") TObjectPtr<USceneComponent> P2GripLeft;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Foundation") TObjectPtr<UFoundationInteractionComponent> InteractionManager;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Foundation") TObjectPtr<UFoundationEquipment> Equipment;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Foundation") TObjectPtr<class UFoundationSession> FoundationSession;
    UPROPERTY(BlueprintReadOnly,Category="Foundation") TObjectPtr<class AFoundationNPC> TacticalTarget;
    UPROPERTY(BlueprintReadOnly,Category="Foundation") bool HoldingHostage=false;
    UPROPERTY(BlueprintReadOnly,Category="Foundation") FText TacticalCaption;
    bool CanUseContext() const;
    bool UsesCameraMotion() const;
private:
    friend class AFoundationNPC;
    void Bash();
    void GrabOrRelease();
    void HideBody();
    void Whistle();
    void OrderFollow();
    void OrderHold();
    void OrderMove();
    void OrderCover();
    void OrderFocus();
    void OrderCeaseFire();
    void SendSquadOrder(uint8 Order);
    void ShowTacticalCaption(const FText& Value);
    float NextBash=0;
    float NextWhistle=0;
    float CaptionUntil=0;
    void SprintOn(); void SprintOff(); void CrouchOn(); void CrouchOff();
    void SlowOn(); void SlowOff();
    void ProneToggle(); void LeftOn(); void LeftOff(); void RightOn(); void RightOff(); void FireFeedback();
    void RefreshSpeed(); void UpdateCamera(float Dt); void UpdateTraversal(float Dt);
    void FinishTraversal(const FString& Result); void SyncBlueprintState();
    bool CapsuleClear(const FVector& Center, float Radius, float Height) const;
    bool SegmentClear(const FVector& A, const FVector& B) const;
    bool bSprintHeld = false, bCrouchHeld = false, bSlowHeld = false, bLeftHeld = false, bRightHeld = false;
    bool bWasDead = false;
    float StandRadius = 42, StandHalf = 96, EyeHeight = 165;
    float EyeFrom = 165, EyeTo = 165, EyeElapsed = 1, VisualEye = 165;
    FVector NeutralCameraOffset = FVector::ZeroVector;
    FTransform NeutralCameraTransform;
    FQuat NeutralViewCorrection = FQuat::Identity;
    FVector MeshOffset = FVector::ZeroVector;
    FTransform WeaponNeutral;
    UPROPERTY() TObjectPtr<USceneComponent> WeaponMount;
    UPROPERTY() TArray<TObjectPtr<UInputAction>> BoundActions;
    TArray<FVector> TraversalPoints;
    int32 TraversalSegment = 0;
    float TraversalElapsed = 0;
    float MotionClock = 0;
    float LandingTime = 1.f;
    float LandingStrength = 0.f;
    float DeathViewAlpha = 0.f;
    float FootstepDistance=0;
};

UCLASS()
class AFGHANISTANFPS_API UPhase2CharacterMovement : public UCharacterMovementComponent
{
    GENERATED_BODY()
protected:
    virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuat& Rotation, bool bSweep,
        FHitResult* OutHit=nullptr, ETeleportType Teleport=ETeleportType::None) override;
};

// Small, explicit automation bridge: requests possessed PIE rather than Simulate.
UCLASS()
class AFGHANISTANFPS_API UPhase2Tools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Phase2|Testing") static bool BeginPlayerTest();
    UFUNCTION(BlueprintCallable, Category="Phase2|Testing") static bool InjectAction(APlayerController* Player, UInputAction* Action, FVector Value);
    UFUNCTION(BlueprintCallable, Category="Phase2|Testing") static bool TestKey(APlayerController* Player, FName KeyName, bool Pressed, float Amount=1);
    UFUNCTION(BlueprintCallable, Category="Phase2|Integration") static FString IntegratePlayer(UObject* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Phase2|Testing") static AActor* TestCamera(AActor* Player, FVector Location, FRotator Rotation);
    UFUNCTION(BlueprintCallable, Category="Phase2|Testing") static AActor* TestBlocker(AActor* Player, FVector Location, FVector Size);
    UFUNCTION(BlueprintCallable, Category="Foundation|Input") static bool MapInput(UObject* Context,UObject* Action,FName KeyName);
    UFUNCTION(BlueprintCallable, Category="Foundation|Input") static bool AddAimModifier(UObject* Action);
};

