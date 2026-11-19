#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "InputModifiers.h"
#include "Blueprint/UserWidget.h"
#include "FoundationThrowables.h"
#include "FoundationEquipment.generated.h"
class APhase2Character;class UEnhancedInputComponent;class UChildActorComponent;class UPointLightComponent;
class USoundBase;class AFoundationInteractable;
class UMaterialInterface;
class UWidget;

UENUM(BlueprintType)
enum class EFoundationFireMode:uint8 { Semi,Auto,Burst };
UCLASS(BlueprintType)
class AFGHANISTANFPS_API UFoundationWeaponDefinition:public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName WeaponId=TEXT("weapon_rifle_base");
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName AmmoType=TEXT("rifle");
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<AActor> ActorClass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UMaterialInterface> VisualMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Capacity=30;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 InitialReserve=90;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float ShotInterval=.13f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float ReloadSeconds=1.5f;
    // Height above barrel of the model-specific sight line, in weapon local units.
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SightHeight=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool UseGripPose=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FVector> GripFingerContacts;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FVector> GripFingerBends;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FRotator LeftWristRotation=FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FRotator RightWristRotation=FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftCurlAxis=FVector::RightVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector RightCurlAxis=-FVector::RightVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float LeftFingerCurl=55;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float RightFingerCurl=55;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseDamage=20;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EFoundationFireMode DefaultMode=EFoundationFireMode::Semi;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool AllowPenetration=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool ProjectileMode=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool CompactVariant=false;
};
USTRUCT(BlueprintType)
struct FFoundationWeaponSlot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UFoundationWeaponDefinition> Definition;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Magazine=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Reserve=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EFoundationFireMode Mode=EFoundationFireMode::Semi;
};
UCLASS(ClassGroup=(Foundation),meta=(BlueprintSpawnableComponent))
class AFGHANISTANFPS_API UFoundationEquipment:public UActorComponent
{
    GENERATED_BODY()
public:
    UFoundationEquipment();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function) override;
    void BindInput(UEnhancedInputComponent* Input);
    UFUNCTION(BlueprintCallable) bool AddWeapon(FName Id,int32 Magazine=-1,int32 Reserve=-1);
    UFUNCTION(BlueprintPure) bool CanAcceptWeapon(FName Id) const;
    UFUNCTION(BlueprintCallable) bool SelectSlot(int32 Index);
    UFUNCTION(BlueprintCallable) bool DropWeapon();
    UFUNCTION(BlueprintCallable) bool Reload();
    UFUNCTION(BlueprintCallable) bool FireOne();
    UFUNCTION(BlueprintCallable) void CycleMode();
    UFUNCTION(BlueprintCallable) void ToggleHolster();
    UFUNCTION(BlueprintCallable) void Inspect();
    UFUNCTION(BlueprintCallable) bool AddReserve(FName AmmoType,int32 Count);
    UFUNCTION(BlueprintPure) FString Summary() const;
    UFUNCTION(BlueprintPure) bool CanAct() const;
    UFUNCTION(BlueprintCallable) void SyncAmmo();
    UFUNCTION(BlueprintCallable) void BeginThrow();
    UFUNCTION(BlueprintCallable) void ReleaseThrow();
    UFUNCTION(BlueprintCallable) void CycleThrowable();
    UFUNCTION(BlueprintCallable) void SelectKnife();
    UFUNCTION(BlueprintCallable) void SelectStone();
    UFUNCTION(BlueprintPure) bool IsHoldingThrowable() const {return IsValid(HeldThrowable);}
    UFUNCTION(BlueprintPure) FVector ThrowableHandPoint() const;
    UFUNCTION(BlueprintCallable) void NotifyProjectileImpact(AActor* Victim,float HealthBefore);
    bool DecodeCheckpointWeapons(const TArray<struct FFoundationSavedWeapon>& Saved,int32 Active,TArray<FFoundationWeaponSlot>& Out) const;
    void RestoreCheckpointWeapons(TArray<FFoundationWeaponSlot> Saved,int32 Active,bool WasHolstered);
    void ClearIntent();
    UPROPERTY(BlueprintReadOnly) TArray<FFoundationWeaponSlot> Slots;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<TObjectPtr<UFoundationWeaponDefinition>> AvailableDefinitions;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1",ClampMax="8")) int32 SlotLimit=2;
    UPROPERTY(BlueprintReadOnly) int32 ActiveSlot=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) bool Reloading=false;
    UPROPERTY(BlueprintReadOnly) bool Holstered=false;
    UPROPERTY(BlueprintReadOnly) bool Inspecting=false;
    UPROPERTY(BlueprintReadOnly) bool Equipping=false;
    UPROPERTY(BlueprintReadOnly) float AdsAlpha=0;
    float SprintFovAlpha=0;
    UPROPERTY(BlueprintReadOnly) float SpreadDegrees=2;
    UPROPERTY(BlueprintReadOnly) bool MuzzleBlocked=false;
    UPROPERTY(BlueprintReadOnly) int32 ShotsFired=0;
    UPROPERTY(BlueprintReadOnly) int32 DryClicks=0;
    UPROPERTY(BlueprintReadOnly) FString LastAction;
    UPROPERTY(BlueprintReadOnly) EFoundationThrowableKind SelectedThrowable=EFoundationThrowableKind::Frag;
    UPROPERTY(BlueprintReadOnly) float FlashAlpha=0;
    UPROPERTY(BlueprintReadOnly) int32 ThrowsReleased=0;
    UPROPERTY(BlueprintReadOnly) int32 ShellEjections=0;
    UPROPERTY(BlueprintReadOnly) int32 ProjectileImpacts=0;
    UPROPERTY(BlueprintReadOnly) bool LastPenetrated=false;
    UPROPERTY(BlueprintReadOnly) float LastShotDamage=0;
    UPROPERTY(BlueprintReadOnly) FName LastDamageZone;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AFoundationThrowable> HeldThrowable;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseFov=90;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float LookSensitivity=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool InvertY=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<USoundBase> ShotSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<USoundBase> DrySound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<USoundBase> ReloadSound;
private:
    void TriggerOn();void TriggerOff();void AimOn();void AimOff();
    void SlotOne();void SlotTwo();void NextSlot();
    void ApplySlot();void SetWeaponVisible(bool Visible);
    UFoundationWeaponDefinition* FindDefinition(FName Id) const;
    bool Initialize();
    void ShowImpact(const FHitResult& Hit,bool CharacterHit);
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ImpactMarks;
    UPROPERTY() TObjectPtr<UMaterialInterface> CharacterImpactMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> SurfaceImpactMaterial;
    TArray<double> ImpactExpiry;
    int32 NextImpactMark=0;
    TWeakObjectPtr<APhase2Character> Player;
    UPROPERTY() TObjectPtr<UChildActorComponent> Holder;
    UPROPERTY() TObjectPtr<USceneComponent> Mount;
    UPROPERTY() TObjectPtr<USceneComponent> Muzzle;
    UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
    UPROPERTY() TObjectPtr<UUserWidget> StatusWidget;
    UPROPERTY() TObjectPtr<UWidget> Crosshair;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PreviewSegments;
    float HoldStarted=0,NextPreview=0;
    FVector ThrowVelocity() const;
    void UpdatePreview();void HidePreview();
    TArray<TWeakObjectPtr<AActor>> ShellActors;
    bool Initialized=false,FireHeld=false,AimHeld=false;
    int32 BurstRemaining=0;
    int32 PendingSlot=INDEX_NONE;
    float NextShot=0,StateEnds=0,StateStarted=0,FlashEnds=0;
    float AdsRestore=0;
};
UCLASS()
class AFGHANISTANFPS_API UFoundationAimModifier:public UInputModifier
{
    GENERATED_BODY()
public:
    virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* Input,FInputActionValue Value,float Dt) override;
};
UCLASS()
class AFGHANISTANFPS_API UFoundationEquipmentWidget:public UUserWidget
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UFoundationEquipment> Source;
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
