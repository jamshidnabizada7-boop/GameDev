#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "FoundationEquipment.h"
#include "FoundationInteraction.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FoundationSession.generated.h"

class APhase2Character;
class UInputMappingContext;
class USoundMix;
class USoundClass;
class USoundBase;

UENUM(BlueprintType)
enum class EFoundationSessionState : uint8 { Boot, Menu, Playing, Paused, Dead };

UCLASS()
class AFGHANISTANFPS_API UFoundationLaunchState : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    bool EnteredGame=false;
};

USTRUCT()
struct FFoundationSavedWeapon
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() int32 Magazine=0;
    UPROPERTY() int32 Reserve=0;
    UPROPERTY() EFoundationFireMode Mode=EFoundationFireMode::Semi;
};

USTRUCT()
struct FFoundationSavedProp
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName ItemId;
    UPROPERTY() EFoundationInteractableKind Kind=EFoundationInteractableKind::Item;
    UPROPERTY() FTransform Transform;
    UPROPERTY() bool Runtime=false;
    UPROPERTY() bool Consumed=false;
    UPROPERTY() bool Locked=false;
    UPROPERTY() bool Open=false;
    UPROPERTY() bool Broken=false;
    UPROPERTY() bool LightOn=true;
    UPROPERTY() float Angle=0;
    UPROPERTY() float Durability=50;
    UPROPERTY() int32 ItemAmount=0;
    UPROPERTY() int32 Magazine=-1;
    UPROPERTY() int32 Reserve=-1;
};

UCLASS()
class AFGHANISTANFPS_API UFoundationCheckpoint : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Version=1;
    UPROPERTY() FString Map;
    UPROPERTY() FTransform PlayerTransform;
    UPROPERTY() FRotator View;
    UPROPERTY(BlueprintReadOnly) float Health=100;
    UPROPERTY() int32 ActiveSlot=INDEX_NONE;
    UPROPERTY() bool Holstered=false;
    UPROPERTY() bool ObjectiveComplete=false;
    UPROPERTY() TMap<FName,int32> Items;
    UPROPERTY() TArray<FFoundationSavedWeapon> Weapons;
    UPROPERTY() TArray<FFoundationSavedProp> Props;
};

UCLASS()
class AFGHANISTANFPS_API UFoundationPreferences : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Version=1;
    UPROPERTY(BlueprintReadWrite) float LookSensitivity=1;
    UPROPERTY() float TextScale=1;
    UPROPERTY() float Master=1;
    UPROPERTY() float Music=1;
    UPROPERTY() float Effects=1;
    UPROPERTY() bool InvertY=false;
    UPROPERTY() bool Captions=true;
    UPROPERTY(BlueprintReadWrite) bool HighContrast=false;
    UPROPERTY() bool AimAssist=false;
    UPROPERTY(BlueprintReadWrite) bool CameraMotion=true;
    UPROPERTY(BlueprintReadWrite) bool ShowDamageNumbers=false;
    UPROPERTY() TMap<FName,FKey> ReboundKeys;
};

UCLASS(ClassGroup=(Foundation),meta=(BlueprintSpawnableComponent))
class AFGHANISTANFPS_API UFoundationSession : public UActorComponent
{
    GENERATED_BODY()
public:
    UFoundationSession();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function) override;
    void BindInput(UInputComponent* Input);
    UFUNCTION(BlueprintCallable) void TogglePause();
    UFUNCTION(BlueprintCallable) void StartPlaying();
    UFUNCTION(BlueprintPure) EFoundationSessionState GetSessionState() const;
    UFUNCTION(BlueprintCallable) bool SaveCheckpoint(const FString& Slot);
    UFUNCTION(BlueprintCallable) bool LoadCheckpoint(const FString& Slot);
    UFUNCTION(BlueprintCallable) bool SavePreferences();
    UFUNCTION(BlueprintCallable) bool Rebind(FName Original,FKey Replacement);
    UFUNCTION(BlueprintCallable) void RestoreDefaults();
    UFUNCTION(BlueprintCallable) void CompleteObjective();
    UFUNCTION(BlueprintPure) FText DamageCueText() const;
    void DamageFeedback(AActor* Causer,float Damage);
    void PresentAppliedDamage(const FVector& Point,float Amount);
    void Caption(const FText& Text);
    FKey EffectiveKey(FName Original) const;

    UPROPERTY(BlueprintReadOnly) bool MenuOpen=false;
    UPROPERTY(BlueprintReadOnly) bool ObjectiveComplete=false;
    UPROPERTY(BlueprintReadOnly) FText Message;
    UPROPERTY(BlueprintReadOnly) FText Subtitle;
    UPROPERTY(BlueprintReadOnly) float DamageAlpha=0;
    UPROPERTY(BlueprintReadOnly) float DamageAngle=0;
    UPROPERTY(BlueprintReadOnly) bool DamageHasDirection=false;
    UPROPERTY(BlueprintReadOnly) float LastDisplayedDamage=0;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UFoundationPreferences> Preferences;
private:
    friend class UFoundationSessionWidget;
    void Initialize(APhase2Character* Player);
    void OpenMenu(bool Title);
    void ApplyPreferences();
    void ApplyBindings();
    void UpdateGamepadAim(APhase2Character* Player,float Dt);
    void SaveDefaultSlot();
    void LoadDefaultSlot();
    void PlaySettingsSound(bool Hover) const;
    bool CanCheckpoint(const APhase2Character* Player) const;
    UPROPERTY() TObjectPtr<class UFoundationSessionWidget> Overlay;
    UPROPERTY() TObjectPtr<class UFoundationSessionWidget> Menu;
    UPROPERTY() TObjectPtr<UInputMappingContext> OriginalContext;
    UPROPERTY() TObjectPtr<UInputMappingContext> RuntimeContext;
    UPROPERTY() TObjectPtr<USoundMix> SoundMix;
    UPROPERTY() TObjectPtr<USoundClass> MusicClass;
    UPROPERTY() TObjectPtr<USoundClass> EffectsClass;
    UPROPERTY() TObjectPtr<USoundBase> SettingsSound;
    TArray<TWeakObjectPtr<class ATextRenderActor>> DamageNumbers;
    int32 OriginalPriority=0;
    bool Initialized=false;
    bool TitleOpen=false;
    float SubtitleUntil=0;
};

UCLASS()
class AFGHANISTANFPS_API UFoundationSessionWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UFoundationSession> Source;
    bool PauseMenu=false;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    FName CaptureFrom;
};
