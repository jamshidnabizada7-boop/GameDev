#include "FoundationSession.h"
#include "FoundationSave.h"
#include "FoundationAI.h"
#include "FoundationBridge.h"
#include "Phase2Character.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/TextRenderComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    const FString ProfileSlot=TEXT("FoundationPreferences");
    const TCHAR* DefaultContext=TEXT("/Game/Input/IMC_Default.IMC_Default");
    const FName Rebindable[]={TEXT("E"),TEXT("R"),TEXT("T"),TEXT("LeftShift"),TEXT("SpaceBar"),TEXT("LeftControl"),TEXT("W"),TEXT("A"),TEXT("S"),TEXT("D")};

    bool SupportedOriginal(FName Name)
    {
        for(FName Key:Rebindable)
            if(Key==Name)return true;
        return false;
    }

    bool ValidReplacement(const FKey& Key)
    {
        return Key.IsValid() && !Key.IsAnalog() && !Key.IsGamepadKey()
            && Key!=EKeys::Escape && Key!=EKeys::F5 && Key!=EKeys::F9 && Key!=EKeys::F10
            && Key!=EKeys::Y && Key!=EKeys::U && Key!=EKeys::O && Key!=EKeys::K;
    }

    APlayerController* GetController(const UFoundationSession* Session)
    {
        const auto* Player=Cast<APhase2Character>(Session->GetOwner());
        return Player?Cast<APlayerController>(Player->GetController()):nullptr;
    }

    FInputKeyBinding& BindPressed(UInputComponent* Input,const FKey& Key)
    {
        FInputKeyBinding Binding(FInputChord(Key),IE_Pressed);
        Binding.bExecuteWhenPaused=true;
        return Input->KeyBindings.Add_GetRef(MoveTemp(Binding));
    }
}

UFoundationSession::UFoundationSession()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
}

void UFoundationSession::BindInput(UInputComponent* Input)
{
    for(const FKey& Key:{EKeys::Escape,EKeys::F10,EKeys::Gamepad_Special_Right})
        BindPressed(Input,Key).KeyDelegate.GetDelegateForManualSet().BindUObject(this,&UFoundationSession::TogglePause);
    BindPressed(Input,EKeys::F5).KeyDelegate.GetDelegateForManualSet().BindUObject(this,&UFoundationSession::SaveDefaultSlot);
    BindPressed(Input,EKeys::F9).KeyDelegate.GetDelegateForManualSet().BindUObject(this,&UFoundationSession::LoadDefaultSlot);
}

void UFoundationSession::Initialize(APhase2Character* Player)
{
    auto* PC=GetController(this);
    if(!PC || !PC->IsLocalController())return;
    Preferences=FoundationSave::Read<UFoundationPreferences>(ProfileSlot);
    if(!Preferences || Preferences->Version!=1)Preferences=NewObject<UFoundationPreferences>(this);
    for(auto It=Preferences->ReboundKeys.CreateIterator();It;++It)
        if(!SupportedOriginal(It.Key()) || !ValidReplacement(It.Value()))It.RemoveCurrent();

    OriginalContext=LoadObject<UInputMappingContext>(nullptr,DefaultContext);
    SoundMix=LoadObject<USoundMix>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/Mix_Foundation.Mix_Foundation"));
    MusicClass=LoadObject<USoundClass>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/SC_Music.SC_Music"));
    EffectsClass=LoadObject<USoundClass>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/SC_Effects.SC_Effects"));
    SettingsSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/AfghanFPS/Audio/Foundation/FoundationUse.FoundationUse"));
    if(SoundMix)UGameplayStatics::PushSoundMixModifier(this,SoundMix);

    Overlay=CreateWidget<UFoundationSessionWidget>(PC,UFoundationSessionWidget::StaticClass());
    Overlay->Source=this;
    Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    Overlay->AddToViewport(12);
    Initialized=true;
    ApplyPreferences();
    ApplyBindings();
    auto* Launch=GetWorld()->GetGameInstance()->GetSubsystem<UFoundationLaunchState>();
    if(Launch && !Launch->EnteredGame && GetWorld()->WorldType==EWorldType::Game &&
        !FParse::Param(FCommandLine::Get(),TEXT("FoundationPlaytest")))
        OpenMenu(true);
}

EFoundationSessionState UFoundationSession::GetSessionState() const
{
    if(!Initialized)return EFoundationSessionState::Boot;
    if(TitleOpen)return EFoundationSessionState::Menu;
    if(MenuOpen)return EFoundationSessionState::Paused;
    const auto* Player=Cast<APhase2Character>(GetOwner());
    return Player && Player->P2IsDead()?EFoundationSessionState::Dead:EFoundationSessionState::Playing;
}

void UFoundationSession::StartPlaying()
{
    if(!TitleOpen)return;
    TitleOpen=false;
    TogglePause();
    if(MenuOpen){TitleOpen=true;return;}
    if(auto* Launch=GetWorld()->GetGameInstance()->GetSubsystem<UFoundationLaunchState>())
        Launch->EnteredGame=true;
}

void UFoundationSession::EndPlay(const EEndPlayReason::Type Reason)
{
    for(const auto& Number:DamageNumbers)
        if(Number.IsValid())Number->Destroy();
    DamageNumbers.Reset();
    if(Overlay)Overlay->RemoveFromParent();
    if(Menu)Menu->RemoveFromParent();
    if(SoundMix)UGameplayStatics::PopSoundMixModifier(this,SoundMix);
    if(auto* PC=GetController(this))
        if(auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            if(RuntimeContext)
            {
                Subsystem->RemoveMappingContext(RuntimeContext);
                if(OriginalContext)Subsystem->AddMappingContext(OriginalContext,OriginalPriority);
            }
    Super::EndPlay(Reason);
}

void UFoundationSession::ApplyPreferences()
{
    auto* Player=Cast<APhase2Character>(GetOwner());
    if(!Player || !Preferences)return;
    auto Normalize=[](float Value,float Min,float Max){return FMath::IsFinite(Value)?FMath::Clamp(Value,Min,Max):1.f;};
    Preferences->LookSensitivity=Normalize(Preferences->LookSensitivity,.25f,3);
    Preferences->TextScale=Normalize(Preferences->TextScale,1,1.5f);
    Preferences->Master=Normalize(Preferences->Master,0,1);
    Preferences->Effects=Normalize(Preferences->Effects,0,1);
    Preferences->Music=Normalize(Preferences->Music,0,1);
    Player->Equipment->LookSensitivity=Preferences->LookSensitivity;
    Player->Equipment->InvertY=Preferences->InvertY;
    if(auto* HUD=Cast<UUserWidget>(FoundationBridge::Object(Player,TEXT("AmmoHUD"))))
        HUD->SetRenderScale(FVector2D(Preferences->TextScale,Preferences->TextScale));
    if(SoundMix && EffectsClass)
        UGameplayStatics::SetSoundMixClassOverride(this,SoundMix,EffectsClass,Preferences->Master*Preferences->Effects,1,.1f);
    if(SoundMix && MusicClass)
        UGameplayStatics::SetSoundMixClassOverride(this,SoundMix,MusicClass,Preferences->Master*Preferences->Music,1,.1f);
}

FKey UFoundationSession::EffectiveKey(FName Original) const
{
    if(Preferences)
        if(const FKey* Found=Preferences->ReboundKeys.Find(Original))return *Found;
    return FKey(Original);
}

bool UFoundationSession::Rebind(FName Original,FKey Replacement)
{
    if(!Preferences || !SupportedOriginal(Original) || !ValidReplacement(Replacement) || !OriginalContext)return false;
    if(Replacement!=FKey(Original))
        for(const auto& Mapping:OriginalContext->GetMappings())
            if(Mapping.Key.GetFName()!=Original && EffectiveKey(Mapping.Key.GetFName())==Replacement)
            {
                Message=NSLOCTEXT("Foundation","RebindConflict","That key is already in use.");
                return false;
            }
    if(Replacement==FKey(Original))Preferences->ReboundKeys.Remove(Original);
    else Preferences->ReboundKeys.Add(Original,Replacement);
    ApplyBindings();
    return SavePreferences();
}

void UFoundationSession::ApplyBindings()
{
    auto* PC=GetController(this);
    auto* Input=PC?Cast<UEnhancedPlayerInput>(PC->PlayerInput):nullptr;
    auto* Subsystem=PC?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()):nullptr;
    if(!Input || !Subsystem || !Preferences || !OriginalContext)return;
    int32 CurrentPriority=0; if(Subsystem->HasMappingContext(OriginalContext,CurrentPriority))OriginalPriority=CurrentPriority;
    if(RuntimeContext)Subsystem->RemoveMappingContext(RuntimeContext);
    Subsystem->RemoveMappingContext(OriginalContext);
    RuntimeContext=nullptr;
    RuntimeContext=DuplicateObject<UInputMappingContext>(OriginalContext,this);
    // The template already supplies both sticks and jump. Bind the remaining
    // controller buttons to the same actions the keyboard and mouse use.
    const TMap<FKey,FKey> ControllerButtons={
        {EKeys::E,EKeys::Gamepad_FaceButton_Left},
        {EKeys::R,EKeys::Gamepad_RightShoulder},
        {EKeys::LeftMouseButton,EKeys::Gamepad_RightTrigger},
        {EKeys::RightMouseButton,EKeys::Gamepad_LeftTrigger},
        {EKeys::LeftControl,EKeys::Gamepad_FaceButton_Right},
        {EKeys::LeftShift,EKeys::Gamepad_LeftThumbstick}
    };
    for(const auto& Mapping:OriginalContext->GetMappings())
    {
        if(const FKey* Button=ControllerButtons.Find(Mapping.Key))
        {
            const bool AlreadyMapped=RuntimeContext->GetMappings().ContainsByPredicate(
                [&](const FEnhancedActionKeyMapping& Item){return Item.Action==Mapping.Action && Item.Key==*Button;});
            if(!AlreadyMapped)
            {
                auto& ControllerMapping=RuntimeContext->MapKey(Mapping.Action,*Button);
                ControllerMapping.Modifiers=Mapping.Modifiers;
                ControllerMapping.Triggers=Mapping.Triggers;
            }
        }
        const FKey Key=EffectiveKey(Mapping.Key.GetFName());
        if(Key==Mapping.Key)continue;
        RuntimeContext->UnmapKey(Mapping.Action,Mapping.Key);
        auto& Rebound=RuntimeContext->MapKey(Mapping.Action,Key);
        Rebound.Modifiers=Mapping.Modifiers;
        Rebound.Triggers=Mapping.Triggers;
    }
    Subsystem->AddMappingContext(RuntimeContext,OriginalPriority);
}

bool UFoundationSession::SavePreferences()
{
    if(!Preferences)return false;
    ApplyPreferences();
    const bool Saved=FoundationSave::Write(Preferences,ProfileSlot);
    Message=Saved?NSLOCTEXT("Foundation","SettingsSaved","Settings saved."):NSLOCTEXT("Foundation","SettingsFailed","Could not save settings.");
    return Saved;
}

void UFoundationSession::RestoreDefaults()
{
    Preferences=NewObject<UFoundationPreferences>(this);
    ApplyBindings();
    SavePreferences();
}

void UFoundationSession::CompleteObjective()
{
    ObjectiveComplete=true;
    Caption(NSLOCTEXT("Foundation","ObjectiveComplete","Training terminal activated."));
}

void UFoundationSession::Caption(const FText& Text)
{
    Subtitle=Text;
    SubtitleUntil=GetWorld()->GetTimeSeconds()+2.5f;
}

void UFoundationSession::DamageFeedback(AActor* Causer,float Damage)
{
    const auto* Player=Cast<APhase2Character>(GetOwner());
    if(!Player || !Player->IsPlayerControlled() || Damage<=0)return;
    DamageAlpha=FMath::Clamp(Damage/25,.35f,1.f);
    const FVector Offset=IsValid(Causer) && Causer!=Player?Causer->GetActorLocation()-Player->GetActorLocation():FVector::ZeroVector;
    DamageHasDirection=Offset.SizeSquared2D()>1.f;
    DamageAngle=DamageHasDirection?FMath::FindDeltaAngleDegrees(Player->GetControlRotation().Yaw,Offset.Rotation().Yaw):0.f;
}

FText UFoundationSession::DamageCueText() const
{
    if(!DamageHasDirection || DamageAlpha<.1f)return FText::GetEmpty();
    if(Preferences && Preferences->HighContrast)
    {
        if(FMath::Abs(DamageAngle)>135)return NSLOCTEXT("Foundation","DamageBehind","Damage from behind");
        if(DamageAngle>45)return NSLOCTEXT("Foundation","DamageRight","Damage from the right");
        if(DamageAngle<-45)return NSLOCTEXT("Foundation","DamageLeft","Damage from the left");
        return NSLOCTEXT("Foundation","DamageFront","Damage from ahead");
    }
    if(FMath::Abs(DamageAngle)>135)return FText::FromString(TEXT("\n\n▼"));
    if(DamageAngle>45)return FText::FromString(TEXT("         ▶"));
    if(DamageAngle<-45)return FText::FromString(TEXT("◀         "));
    return FText::FromString(TEXT("▲\n\n"));
}

void UFoundationSession::PlaySettingsSound(bool Hover) const
{
    if(SettingsSound && MenuOpen)
        UGameplayStatics::PlaySound2D(this,SettingsSound,Hover?.16f:.5f,Hover?1.2f:1.f);
}

void UFoundationSession::PresentAppliedDamage(const FVector& Point,float Amount)
{
    const auto* Player=Cast<APhase2Character>(GetOwner());
    if(!Player || !Player->IsLocallyControlled() || !Player->P2Camera || !Preferences
        || !Preferences->ShowDamageNumbers || !FMath::IsFinite(Amount) || Amount<=0 || Point.ContainsNaN())return;
    DamageNumbers.RemoveAll([](const auto& Number){return !Number.IsValid();});
    if(DamageNumbers.Num()>=12)
    {
        DamageNumbers[0]->Destroy();
        DamageNumbers.RemoveAt(0);
    }
    const FVector ToCamera=(Player->P2Camera->GetComponentLocation()-Point).GetSafeNormal();
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags|=RF_Transient;
    auto* Number=GetWorld()->SpawnActor<ATextRenderActor>(Point+ToCamera*8+FVector(0,0,18),ToCamera.Rotation(),Spawn);
    if(!Number)return;
    auto* Text=Number->GetTextRender();
    FNumberFormattingOptions Format;
    Format.MaximumFractionalDigits=1;
    Text->SetText(FText::AsNumber(Amount,&Format));
    Text->SetHorizontalAlignment(EHTA_Center);
    Text->SetWorldSize(16.f);
    Text->SetTextRenderColor(Preferences->HighContrast?FColor::Yellow:FColor::White);
    Number->SetActorEnableCollision(false);
    Number->SetLifeSpan(.85f);
    DamageNumbers.Add(Number);
    LastDisplayedDamage=Amount;
}

void UFoundationSession::TogglePause()
{
    auto* Player=Cast<APhase2Character>(GetOwner());
    auto* PC=GetController(this);
    if(!PC || !Player || !Initialized || TitleOpen)return;
    if(MenuOpen)
    {
        if(!PC->SetPause(false))return;
        MenuOpen=false;
        if(Menu)Menu->RemoveFromParent();
        Menu=nullptr;
        PC->bShowMouseCursor=false;
        PC->SetInputMode(FInputModeGameOnly());
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        return;
    }
    OpenMenu(false);
}

void UFoundationSession::OpenMenu(bool Title)
{
    auto* Player=Cast<APhase2Character>(GetOwner());
    auto* PC=GetController(this);
    if(!PC || !Player || MenuOpen || Player->P2IsDead() || Player->Equipment->IsHoldingThrowable())return;
    Player->Equipment->ClearIntent();
    if(Player->InteractionManager->MirrorActive)Player->InteractionManager->EndMirror();
    else Player->InteractionManager->CancelUse();
    if(!PC->SetPause(true))return;
    MenuOpen=true;
    TitleOpen=Title;
    Menu=CreateWidget<UFoundationSessionWidget>(PC,UFoundationSessionWidget::StaticClass());
    Menu->Source=this;
    Menu->PauseMenu=true;
    Menu->SetIsFocusable(true);
    Menu->AddToViewport(30);
    PC->bShowMouseCursor=true;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(Menu->TakeWidget());
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
}

void UFoundationSession::UpdateGamepadAim(APhase2Character* Player,float Dt)
{
    auto* PC=GetController(this);
    if(!PC || !Preferences->AimAssist || Player->Equipment->AdsAlpha<.85f || !Player->Equipment->CanAct())return;
    const float X=PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
    const float Y=PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY);
    if(FMath::Abs(X)+FMath::Abs(Y)<.15f)return;
    const FVector Eye=Player->P2Camera->GetComponentLocation();
    const FRotator View=PC->GetControlRotation();
    float Best=FMath::Cos(FMath::DegreesToRadians(5.f));
    FRotator Correction=View;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundationGamepadAim),false,Player);
    if(auto* Weapon=FoundationBridge::Weapon(Player))Query.AddIgnoredActor(Weapon);
    for(TActorIterator<AFoundationNPC> NPC(GetWorld());NPC;++NPC)
    {
        if(NPC->Friendly || NPC->bIsDead || NPC->State==EFoundationAIState::Surrender || NPC->State==EFoundationAIState::Restrained)continue;
        const FVector Aim=NPC->GetActorLocation()+FVector(0,0,48)-Eye;
        if(Aim.SizeSquared()>FMath::Square(1200.f))continue;
        const float Dot=FVector::DotProduct(View.Vector(),Aim.GetSafeNormal());
        if(Dot<=Best)continue;
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Aim,ECC_Visibility,Query) && Hit.GetActor()!=*NPC)continue;
        Best=Dot;
        Correction=Aim.Rotation();
    }
    PC->SetControlRotation(FMath::RInterpConstantTo(View,Correction,Dt,8.f));
}

void UFoundationSession::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,Type,Function);
    auto* Player=Cast<APhase2Character>(GetOwner());
    if(!Player || !Player->IsLocallyControlled() || !Player->IsPlayerControlled())return;
    if(!Initialized)Initialize(Player);
    if(!Initialized || MenuOpen)return;
    DamageAlpha=FMath::Max(0.f,DamageAlpha-Dt*2);
    if(Player->P2Camera)
        for(auto& Number:DamageNumbers)
            if(Number.IsValid())
            {
                if(Preferences->CameraMotion)Number->AddActorWorldOffset(FVector(0,0,18.f*Dt));
                Number->SetActorRotation((Player->P2Camera->GetComponentLocation()-Number->GetActorLocation()).Rotation());
            }
    if(GetWorld()->GetTimeSeconds()>=SubtitleUntil)Subtitle=FText::GetEmpty();
    UpdateGamepadAim(Player,Dt);
}

FReply UFoundationSessionWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(PauseMenu && Source.IsValid())
    {
        const FKey Key=Event.GetKey();
        if(Source->TitleOpen && (Key==EKeys::Enter || Key==EKeys::Gamepad_FaceButton_Bottom))
        {
            Source->StartPlaying();
            return FReply::Handled();
        }
        if(!CaptureFrom.IsNone())
        {
            if(Key!=EKeys::Escape && Key!=EKeys::Gamepad_FaceButton_Right)Source->Rebind(CaptureFrom,Key);
            CaptureFrom=NAME_None;
            return FReply::Handled();
        }
        if(Key==EKeys::Escape || Key==EKeys::F10 || Key==EKeys::Gamepad_FaceButton_Right || Key==EKeys::Gamepad_Special_Right)
        {
            Source->TogglePause();
            return FReply::Handled();
        }
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

TSharedRef<SWidget> UFoundationSessionWidget::RebuildWidget()
{
    if(PauseMenu && Source.IsValid() && Source->TitleOpen)
    {
        return SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center)
            .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
            .BorderBackgroundColor(FLinearColor(.008f,.012f,.017f,1.f))
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)
                [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",32))
                    .Text(NSLOCTEXT("Foundation","GameTitle","Afghanistan FPS"))]
                +SVerticalBox::Slot().AutoHeight()
                [SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(18,10)).OnClicked_Lambda([this]()
                    {if(Source.IsValid())Source->StartPlaying();return FReply::Handled();})
                    [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",20))
                        .Text(NSLOCTEXT("Foundation","StartPlay","Play"))]]];
    }
    if(!PauseMenu)
    {
        const auto Tint=[this]()
        {
            return Source.IsValid() && Source->Preferences && Source->Preferences->HighContrast?FLinearColor::Yellow:FLinearColor::White;
        };
        const auto ContrastBacking=[this]()
        {
            return Source.IsValid() && Source->Preferences && Source->Preferences->HighContrast
                ?FLinearColor(0,0,0,.92f):FLinearColor::Transparent;
        };
        return SNew(SOverlay)
        +SOverlay::Slot()
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor_Lambda([this]()
            {
                const float Alpha=Source.IsValid()?Source->DamageAlpha:0;
                return FLinearColor(.7f,.05f,.02f,Alpha*.23f);
            })
        ]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBorder).Padding(FMargin(12,5)).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
            .BorderBackgroundColor_Lambda(ContrastBacking).Visibility_Lambda([this]()
            {
                return Source.IsValid() && Source->DamageHasDirection && Source->DamageAlpha>=.1f
                    ?EVisibility::HitTestInvisible:EVisibility::Collapsed;
            })
            [
                SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",26)).ShadowOffset(FVector2D(2,2)).ColorAndOpacity_Lambda(Tint)
                .Text_Lambda([this](){return Source.IsValid()?Source->DamageCueText():FText::GetEmpty();})
            ]
        ]
        +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(28,24)
        [
            SNew(STextBlock).Font_Lambda([this](){return FCoreStyle::GetDefaultFontStyle("Bold",Source.IsValid()?FMath::RoundToInt(17*Source->Preferences->TextScale):17);})
            .ShadowOffset(FVector2D(2,2)).ColorAndOpacity_Lambda(Tint)
            .Text_Lambda([this]()
            {
                return Source.IsValid() && Source->ObjectiveComplete
                    ?NSLOCTEXT("Foundation","ObjectiveDone","Training terminal activated")
                    :NSLOCTEXT("Foundation","Objective","Activate the training terminal");
            })
        ]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(24,0,24,45)
        [
            SNew(STextBlock).Font_Lambda([this](){return FCoreStyle::GetDefaultFontStyle("Bold",Source.IsValid()?FMath::RoundToInt(17*Source->Preferences->TextScale):17);})
            .ShadowOffset(FVector2D(2,2)).ColorAndOpacity_Lambda(Tint)
            .Text_Lambda([this]()
            {
                return Source.IsValid() && Source->Preferences->Captions?Source->Subtitle:FText::GetEmpty();
            })
        ];
    }

    const auto List=SNew(SVerticalBox);
    List->AddSlot().AutoHeight().Padding(0,0,0,8)
    [
        SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",28)).Text(NSLOCTEXT("Foundation","Paused","Paused"))
    ];
    const auto Button=[this,List](TFunction<FText()> Label,TFunction<void()> Click)
    {
        List->AddSlot().AutoHeight().Padding(0,3)
        [
            SNew(SButton).ContentPadding(FMargin(12,5)).OnHovered_Lambda([this]()
            {
                if(Source.IsValid())Source->PlaySettingsSound(true);
            }).OnClicked_Lambda([this,Click]()
            {
                if(Source.IsValid())
                {
                    Source->PlaySettingsSound(false);
                    Click();
                }
                return FReply::Handled();
            })
            [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",17)).Text_Lambda(MoveTemp(Label))]
        ];
    };
    const auto Fixed=[&](const FText& Title,TFunction<void()> Click){Button([Title](){return Title;},MoveTemp(Click));};
    Fixed(NSLOCTEXT("Foundation","Resume","Resume"),[this](){Source->TogglePause();});
    Fixed(NSLOCTEXT("Foundation","Save","Save checkpoint"),[this](){Source->SaveDefaultSlot();});
    Fixed(NSLOCTEXT("Foundation","Load","Load checkpoint"),[this](){Source->LoadDefaultSlot();});

    const auto Toggle=[&](const FText& Label,bool UFoundationPreferences::*Member)
    {
        Button([this,Label,Member]()
        {
            return Source.IsValid()?FText::Format(NSLOCTEXT("Foundation","ToggleValue","{0} · {1}"),Label,
                (Source->Preferences.Get()->*Member)?NSLOCTEXT("Foundation","On","On"):NSLOCTEXT("Foundation","Off","Off")):Label;
        },[this,Member]()
        {
            auto& Setting=Source->Preferences.Get()->*Member;
            Setting=!Setting;
            Source->SavePreferences();
        });
    };
    const auto Step=[&](const FText& Label,float UFoundationPreferences::*Member,float Min,float Max,float Increment)
    {
        Button([this,Label,Member]()
        {
            return Source.IsValid()?FText::Format(NSLOCTEXT("Foundation","ScalarValue","{0} · {1}%"),Label,
                FText::AsNumber(FMath::RoundToInt((Source->Preferences.Get()->*Member)*100))):Label;
        },[this,Member,Min,Max,Increment]()
        {
            auto& Setting=Source->Preferences.Get()->*Member;
            Setting=Setting>=Max-.01f?Min:FMath::Min(Max,Setting+Increment);
            Source->SavePreferences();
        });
    };
    Step(NSLOCTEXT("Foundation","Sensitivity","Look sensitivity"),&UFoundationPreferences::LookSensitivity,.25f,3.f,.25f);
    Toggle(NSLOCTEXT("Foundation","Invert","Invert vertical look"),&UFoundationPreferences::InvertY);
    Toggle(NSLOCTEXT("Foundation","AimAssist","Gamepad aim assist"),&UFoundationPreferences::AimAssist);
    Toggle(NSLOCTEXT("Foundation","CameraMotion","Camera motion"),&UFoundationPreferences::CameraMotion);
    Toggle(NSLOCTEXT("Foundation","Captions","Captions"),&UFoundationPreferences::Captions);
    Toggle(NSLOCTEXT("Foundation","Contrast","High contrast"),&UFoundationPreferences::HighContrast);
    Toggle(NSLOCTEXT("Foundation","DamageNumbers","Floating damage numbers"),&UFoundationPreferences::ShowDamageNumbers);
    Step(NSLOCTEXT("Foundation","TextSize","HUD text size"),&UFoundationPreferences::TextScale,1,1.5f,.25f);
    Step(NSLOCTEXT("Foundation","Master","Master volume"),&UFoundationPreferences::Master,0,1,.2f);
    Step(NSLOCTEXT("Foundation","Music","Music volume"),&UFoundationPreferences::Music,0,1,.2f);
    Step(NSLOCTEXT("Foundation","Effects","Effects volume"),&UFoundationPreferences::Effects,0,1,.2f);
    const FText Labels[]={NSLOCTEXT("Foundation","InteractKey","Interact"),NSLOCTEXT("Foundation","ReloadKey","Reload / restart"),
        NSLOCTEXT("Foundation","ThrowKey","Throw"),NSLOCTEXT("Foundation","SprintKey","Sprint"),NSLOCTEXT("Foundation","JumpKey","Jump"),
        NSLOCTEXT("Foundation","CrouchKey","Crouch"),NSLOCTEXT("Foundation","ForwardKey","Move forward"),NSLOCTEXT("Foundation","LeftKey","Move left"),
        NSLOCTEXT("Foundation","BackKey","Move back"),NSLOCTEXT("Foundation","RightKey","Move right")};
    for(int32 Index=0;Index<UE_ARRAY_COUNT(Rebindable);++Index)
    {
        const FName Key=Rebindable[Index];
        const FText Label=Labels[Index];
        Button([this,Key,Label]()
        {
            if(CaptureFrom==Key)return NSLOCTEXT("Foundation","CaptureKey","Press a new key or Escape to cancel");
            return Source.IsValid()?FText::Format(NSLOCTEXT("Foundation","Binding","{0} · {1}"),Label,Source->EffectiveKey(Key).GetDisplayName()):Label;
        },[this,Key](){CaptureFrom=Key;});
    }
    Fixed(NSLOCTEXT("Foundation","Reset","Restore default settings"),[this](){Source->RestoreDefaults();});
    List->AddSlot().AutoHeight().Padding(0,8)
    [
        SNew(STextBlock).AutoWrapText(true).Text_Lambda([this](){return Source.IsValid()?Source->Message:FText::GetEmpty();})
    ];
    return SNew(SOverlay)
    +SOverlay::Slot()
    [
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(FLinearColor(.008f,.012f,.017f,.9f))
    ]
    +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Fill).Padding(0,24)
    [
        SNew(SBox).WidthOverride(420)
        [SNew(SScrollBox)+SScrollBox::Slot().Padding(20,8)[List]]
    ];
}
