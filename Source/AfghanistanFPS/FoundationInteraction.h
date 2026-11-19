#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "Blueprint/UserWidget.h"
#include "FoundationInteraction.generated.h"
class UEnhancedInputComponent;class UStaticMeshComponent;class UPointLightComponent;
class UAudioComponent;class USoundBase;class ACameraActor;class APhase2Character;

UENUM(BlueprintType)
enum class EFoundationUseMode:uint8 { Immediate, Hold, Continuous };
UENUM(BlueprintType)
enum class EFoundationUsePhase:uint8 { Begin, Update, Complete, End, Cancel, Adjust };
UENUM(BlueprintType)
enum class EFoundationInteractableKind:uint8 { Door, Drawer, LightSwitch, LootCrate, Breakable, Ammo, Health, Weapon, Key, Item, Terminal };
USTRUCT(BlueprintType)
struct FFoundationUseInfo
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText Prompt;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EFoundationUseMode Mode=EFoundationUseMode::Immediate;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float HoldSeconds=0.8f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Allowed=true;
};
UINTERFACE(BlueprintType)
class AFGHANISTANFPS_API UFoundationInteractableInterface:public UInterface { GENERATED_BODY() };
class AFGHANISTANFPS_API IFoundationInteractableInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent,BlueprintCallable) FFoundationUseInfo GetUseInfo(APawn* User) const;
    UFUNCTION(BlueprintNativeEvent,BlueprintCallable) bool ReceiveUse(APawn* User,EFoundationUsePhase Phase,float Value);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFoundationItemClaimed,FName,ItemId,int32,Count);

UCLASS(ClassGroup=(Foundation),meta=(BlueprintSpawnableComponent))
class AFGHANISTANFPS_API UFoundationInteractionComponent:public UActorComponent
{
    GENERATED_BODY()
public:
    UFoundationInteractionComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function) override;
    void BindInput(UEnhancedInputComponent* Input);
    UFUNCTION(BlueprintCallable) void BeginUse();
    UFUNCTION(BlueprintCallable) void EndUse();
    UFUNCTION(BlueprintCallable) void CancelUse();
    UFUNCTION(BlueprintCallable) void AdjustUse(float Steps);
    UFUNCTION(BlueprintCallable) bool GrantItem(FName ItemId,int32 Count);
    UFUNCTION(BlueprintPure) int32 ItemCount(FName ItemId) const;
    UFUNCTION(BlueprintCallable) void EndMirror();
    UFUNCTION(BlueprintPure) FVector ReachPoint() const;
    UFUNCTION(BlueprintPure) bool ControlsViewFov() const {return Peeking || RestoringPeek;}
    UFUNCTION(BlueprintCallable) FString InspectFocus() const;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Interaction") float UseDistance=250;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> FocusedActor;
    UPROPERTY(BlueprintReadOnly) FText Prompt;
    UPROPERTY(BlueprintReadOnly) float Progress=0;
    UPROPERTY(BlueprintReadOnly) float ReachAlpha=0;
    UPROPERTY(BlueprintReadOnly) bool Using=false;
    UPROPERTY(BlueprintReadOnly) bool MirrorActive=false;
    UPROPERTY(BlueprintReadOnly) bool Peeking=false;
    UPROPERTY(BlueprintReadOnly) FString LastResult;
    UPROPERTY(BlueprintReadOnly) int32 CompletedTransactions=0;
    UPROPERTY(BlueprintReadOnly) float MirrorFrameMilliseconds=0;
    UPROPERTY(BlueprintReadOnly) TMap<FName,int32> OwnedItems;
    UPROPERTY(BlueprintAssignable) FFoundationItemClaimed OnItemClaimed;
private:
    AActor* TraceFocus() const;
    bool Validate(AActor* Target) const;
    void BeginMirror(class AFoundationInteractable* Door);
    UPROPERTY() TObjectPtr<UUserWidget> PromptWidget;
    UPROPERTY() TObjectPtr<ACameraActor> MirrorCamera;
    TWeakObjectPtr<AActor> HeldActor;
    FVector InstantReachPoint=FVector::ZeroVector;
    float InstantReachRemaining=0;
    TWeakObjectPtr<APhase2Character> Player;
    FFoundationUseInfo ActiveInfo;
    float UseElapsed=0,IdleAccumulator=0;
    bool Completed=false,Adjusted=false,RestoringPeek=false;
    FRotator MirrorStartRotation;
    FRotator MirrorBaseRotation;
    float OriginalFov=90;
};

UCLASS()
class AFGHANISTANFPS_API UFoundationPromptWidget:public UUserWidget
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UFoundationInteractionComponent> Source;
    virtual TSharedRef<SWidget> RebuildWidget() override;
};

UCLASS(Blueprintable)
class AFGHANISTANFPS_API AFoundationInteractable:public AActor,public IFoundationInteractableInterface
{
    GENERATED_BODY()
public:
    AFoundationInteractable();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
    virtual FFoundationUseInfo GetUseInfo_Implementation(APawn* User) const override;
    virtual bool ReceiveUse_Implementation(APawn* User,EFoundationUsePhase Phase,float Value) override;
    UFUNCTION(BlueprintCallable) void Configure(EFoundationInteractableKind NewKind);
    UFUNCTION(BlueprintCallable) void SetDoorTarget(float Degrees);
    UFUNCTION(BlueprintCallable) void SetListening(bool Enabled);
    UFUNCTION(BlueprintPure) FVector InteractionPoint() const;
    void RestoreCheckpointState(const struct FFoundationSavedProp& State);
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Panel;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UAudioComponent> InteriorAudio;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UAudioComponent> ExteriorAudio;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EFoundationInteractableKind Kind=EFoundationInteractableKind::Door;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName StableId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemId=TEXT("ammo");
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 ItemAmount=30;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 WeaponMagazine=-1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 WeaponReserve=-1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Locked=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RequiredKey=TEXT("armory_key");
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<USoundBase> InteriorSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<USoundBase> UseSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<TObjectPtr<AActor>> LinkedLights;
    UPROPERTY(BlueprintReadOnly) float DoorAngle=0;
    UPROPERTY(BlueprintReadOnly) float TargetAngle=0;
    UPROPERTY(BlueprintReadOnly) float PickAngle=0;
    UPROPERTY(BlueprintReadOnly) bool Listening=false;
    UPROPERTY(BlueprintReadOnly) bool Consumed=false;
    UPROPERTY(BlueprintReadOnly) bool Open=false;
    UPROPERTY(BlueprintReadOnly) bool Broken=false;
    UPROPERTY(BlueprintReadOnly) bool LightOn=true;
    UPROPERTY(BlueprintReadOnly) float Durability=50;
    UPROPERTY(BlueprintReadOnly) int32 UseCount=0;
    UPROPERTY(BlueprintReadOnly) bool Blocked=false;
private:
    void Consume();
    bool DoorPoseClear(float Angle) const;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Debris;
    float DrawerAlpha=0,DoorUseSeconds=0;
    bool DoorAdjusted=false,Picking=false;
    FVector PickupStart;
    TWeakObjectPtr<APawn> CurrentUser;
};
