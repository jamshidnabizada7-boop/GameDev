#pragma once
#include "CoreMinimal.h"
#include "Phase2Character.h"
#include "FoundationInteraction.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"
#include "FoundationAI.generated.h"
class UAIPerceptionComponent;class UAISenseConfig_Sight;class UAISenseConfig_Hearing;
class UChildActorComponent;class UTextRenderComponent;class UAudioComponent;class UPointLightComponent;
class UPhysicsHandleComponent;

UENUM(BlueprintType)
enum class EFoundationAIState:uint8 { Idle,Patrol,Investigate,Combat,Cover,Search,Stagger,Surrender,Dead,Stunned,Restrained,Hostage,Downed };
UENUM(BlueprintType)
enum class EFoundationSquadOrder:uint8 { Hold,Follow,Move,Cover,Focus,CeaseFire };

UCLASS()
class AFGHANISTANFPS_API AFoundationAIController:public AAIController
{
    GENERATED_BODY()
public:
    AFoundationAIController();
    virtual void OnPossess(APawn* PossessedPawn) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAIPerceptionComponent> Senses;
    UPROPERTY() TObjectPtr<UAISenseConfig_Sight> Sight;
    UPROPERTY() TObjectPtr<UAISenseConfig_Hearing> Hearing;
private:
    UFUNCTION() void Perceived(AActor* Actor,FAIStimulus Stimulus);
};

UCLASS(Blueprintable)
class AFGHANISTANFPS_API AFoundationNPC:public APhase2Character,public IGenericTeamAgentInterface,public IFoundationInteractableInterface
{
    GENERATED_BODY()
public:
    AFoundationNPC(const FObjectInitializer& Initializer);
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const override;
    virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
    virtual FGenericTeamId GetGenericTeamId() const override {return FGenericTeamId(Friendly?0:1);}
    virtual FFoundationUseInfo GetUseInfo_Implementation(APawn* User) const override;
    virtual bool ReceiveUse_Implementation(APawn* User,EFoundationUsePhase Phase,float Value) override;
    void NotifyPerception(AActor* Actor,const FAIStimulus& Stimulus);
    UFUNCTION(BlueprintCallable) void SetAIEnabled(bool Enabled);
    UFUNCTION(BlueprintPure) float HealthValue() const;
    UFUNCTION(BlueprintPure) bool CanSee(AActor* Actor) const;
    UFUNCTION(BlueprintCallable) bool MoveToPoint(FVector Point);
    UFUNCTION(BlueprintCallable) bool ChooseCover();
    UFUNCTION(BlueprintCallable) void Surrender();
    UFUNCTION(BlueprintCallable) bool ResetForDrill();
    UFUNCTION(BlueprintCallable) bool IssueOrder(EFoundationSquadOrder Order,FVector Point,AActor* Actor=nullptr);
    FVector UsePoint() const;
    bool TryGrab(APhase2Character* Player);
    bool TryHide(APhase2Character* Player);
    void ReleaseContext();
    bool IsBehind(const APawn* User) const;
    UPROPERTY(BlueprintReadOnly) bool BodyHidden=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool CanBeRevived=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName StableId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Friendly=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool AIEnabled=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float StartingHealth=100;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SightRadius=1600;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float HearingRadius=2200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float DetectionSeconds=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float RespawnDelay=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FVector> PatrolRoute;
    UPROPERTY(BlueprintReadOnly) EFoundationAIState State=EFoundationAIState::Idle;
    UPROPERTY(BlueprintReadOnly) EFoundationSquadOrder SquadOrder=EFoundationSquadOrder::Hold;
    UPROPERTY(BlueprintReadOnly) float Awareness=0;
    UPROPERTY(BlueprintReadOnly) bool SeesTarget=false;
    UPROPERTY(BlueprintReadOnly) bool bIsDead=false;
    UPROPERTY(BlueprintReadOnly) FVector LastKnownLocation;
    UPROPERTY(BlueprintReadOnly) FVector LastHeardLocation;
    UPROPERTY(BlueprintReadOnly) FVector MoveGoal;
    UPROPERTY(BlueprintReadOnly) FName LastNoiseTag;
    UPROPERTY(BlueprintReadOnly) int32 HeardEvents=0;
    UPROPERTY(BlueprintReadOnly) int32 ShotsFired=0;
    UPROPERTY(BlueprintReadOnly) int32 Footsteps=0;
    UPROPERTY(BlueprintReadOnly) int32 PatrolArrivals=0;
    UPROPERTY(BlueprintReadOnly) int32 CoverSelections=0;
    UPROPERTY(BlueprintReadOnly) int32 BlockedShots=0;
    UPROPERTY(BlueprintReadOnly) bool LastMoveAccepted=false;
    UPROPERTY(BlueprintReadOnly) FString LastDecision;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Target;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USceneComponent> AIWeaponMount;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UChildActorComponent> AIWeapon;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UCameraComponent> AIEyes;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UTextRenderComponent> StateLabel;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UAudioComponent> FootAudio;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPointLightComponent> GunFlash;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UActorComponent> HealthComponent;
private:
    bool bOccupiesPopulationSlot=false;
    bool IsThreat(AActor* Actor) const;
    void Enter(EFoundationAIState NewState,float Seconds=0);
    void FireAtTarget();void Die();void StepSound();void DropAIWeapon();
    void StopAI();
    void UpdateContext(float Dt);
    void Down();
    bool Revive();
    bool IsBody() const;
    UPROPERTY() TObjectPtr<UPhysicsHandleComponent> DragHandle;
    TWeakObjectPtr<APhase2Character> ContextPlayer;
    enum class EContext : uint8 { None, Takedown, Restrain, Drag, Revive, Hostage };
    EContext Context=EContext::None;
    bool HasSightCandidate=false,Reloading=false,HoldFire=false;
    float Until=0,LastSeen=-100,NextFire=0,NextMove=0,NextLabel=0,StepDistance=0,FlashUntil=0,DeathAt=0;
    int32 RouteIndex=0;
    FTransform SpawnTransform;
    UPROPERTY() TObjectPtr<AActor> Leader;
    FVector OrderPoint;
};

UCLASS()
class AFGHANISTANFPS_API UFoundationAITools:public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) static FString BuildArenaNavigation(UObject* Context);
    UFUNCTION(BlueprintCallable) static TArray<FVector> PathTo(APawn* Pawn,FVector Goal);
};
