#include "FoundationAI.h"

#if !UE_BUILD_SHIPPING
#include "FoundationSession.h"
#include "Phase2Character.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace
{
    const FName PatrolId(TEXT("P5_Enemy_Patrol"));

    void VerifySandboxNavigation(UWorld* World)
    {
        if(!World || !World->IsGameWorld())
            return;

        AFoundationNPC* Patrol=nullptr;
        for(TActorIterator<AFoundationNPC> It(World);It;++It)
            if(It->StableId==PatrolId)
            {
                Patrol=*It;
                break;
            }
        if(!Patrol || Patrol->PatrolRoute.Num()<2 || !Patrol->GetController())
        {
            UE_LOG(LogTemp,Error,TEXT("FoundationNavSmoke FAIL: patrol fixture missing or unpossessed"));
            return;
        }

        // Observe the normal AI tick and navigator. The probe issues no orders
        // and leaves the fixture, player, configuration and save slots untouched.
        const auto Probe=[WeakPatrol=TWeakObjectPtr<AFoundationNPC>(Patrol),Start=Patrol->GetActorLocation(),
                          Arrivals=Patrol->PatrolArrivals]()
        {
            AFoundationNPC* NPC=WeakPatrol.Get();
            if(!NPC)
            {
                UE_LOG(LogTemp,Error,TEXT("FoundationNavSmoke FAIL: patrol fixture disappeared"));
                return;
            }
            const FVector Goal=NPC->PatrolRoute[0];
            const auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(
                NPC,NPC->GetActorLocation(),Goal,NPC);
            const bool CompletePath=Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num()>1;
            const float Travel=FVector::Dist2D(Start,NPC->GetActorLocation());
            const int32 NewArrivals=NPC->PatrolArrivals-Arrivals;
            const bool Passed=CompletePath && (NewArrivals>=2 || (NewArrivals>=1 && Travel>100.f));
            if(Passed)
            {
                UE_LOG(LogTemp,Display,TEXT("FoundationNavSmoke PASS: completePath=1 arrivals=%d displacement=%.1f"),
                    NewArrivals,Travel);
            }
            else
            {
                UE_LOG(LogTemp,Error,TEXT("FoundationNavSmoke FAIL: completePath=%d arrivals=%d displacement=%.1f"),
                    CompletePath,NewArrivals,Travel);
            }
        };
        FTimerHandle Handle;
        World->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateLambda(Probe),6.f,false);
    }

    // Packaged development benchmark only: populate the current sandbox
    // before frame capture, leaving the normal AI controller/tick running.
    void FillPopulation(UWorld* World)
    {
        if(!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("FoundationPlaytest")))return;
        int32 Count=0;
        for(TActorIterator<AFoundationNPC> It(World);It;++It)++Count;
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        for(int32 Index=Count;Index<10;++Index)
            World->SpawnActor<AFoundationNPC>(AFoundationNPC::StaticClass(),FVector(400+175*(Index%5),200+200*(Index/5),30),FRotator(0,180,0),Spawn);
        int32 Live=0,Possessed=0;
        for(TActorIterator<AFoundationNPC> It(World);It;++It){++Live;if(It->GetController())++Possessed;}
        UE_LOG(LogTemp,Display,TEXT("FoundationPopulation NPCs=%d possessed=%d"),Live,Possessed);
    }
    FAutoConsoleCommandWithWorld PopulationBenchmark(TEXT("Foundation.FillPopulation"),
        TEXT("Populate a development playtest for the ten-NPC frame capture."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&FillPopulation));

    FAutoConsoleCommandWithWorld NavigationSmoke(
        TEXT("Foundation.VerifyNavigation"),
        TEXT("Observe the sandbox patrol for six seconds and log full-path and movement results."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&VerifySandboxNavigation));

    void CheckRelaunch(UWorld* World,bool Write)
    {
        if(!World || !World->IsGameWorld())return;
        FString TestProfile;
        if(!FParse::Param(FCommandLine::Get(),TEXT("FoundationPlaytest")) ||
            !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),TestProfile) || TestProfile.IsEmpty())return;
        FTimerHandle Handle;
        World->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(World,[World,Write]()
        {
            auto* Player=Cast<APhase2Character>(UGameplayStatics::GetPlayerPawn(World,0));
            const FString Slot=TEXT("FoundationRelaunchProbe");
            const FName Token=TEXT("FoundationRelaunchToken");
            bool Passed=false;
            if(Player && Player->FoundationSession && Player->InteractionManager)
            {
                auto& Items=Player->InteractionManager->OwnedItems;
                auto* Session=Player->FoundationSession.Get();
                if(Write && !UGameplayStatics::DoesSaveGameExist(Slot,0) &&
                    !UGameplayStatics::DoesSaveGameExist(TEXT("FoundationPreferences"),0) && !Items.Contains(Token))
                {
                    Items.Add(Token,17);
                    Passed=Session->SaveCheckpoint(Slot) && Session->Rebind(TEXT("E"),EKeys::P);
                    if(Passed)
                    {
                        Session->Preferences->HighContrast=true;
                        Session->Preferences->LookSensitivity=1.75f;
                        Passed=Session->SavePreferences();
                    }
                    Items.Remove(Token);
                }
                else if(!Write && !Items.Contains(Token))
                {
                    Passed=Session->LoadCheckpoint(Slot) && Items.FindRef(Token)==17 &&
                        Session->EffectiveKey(TEXT("E"))==EKeys::P && Session->Preferences->HighContrast &&
                        FMath::IsNearlyEqual(Session->Preferences->LookSensitivity,1.75f);
                }
            }
            UE_LOG(LogTemp,Display,TEXT("FoundationRelaunch %s %s"),Write?TEXT("write"):TEXT("read"),Passed?TEXT("PASS"):TEXT("FAIL"));
        }),4.f,false);
    }
    FAutoConsoleCommandWithWorld WriteCheckpointSmoke(TEXT("Foundation.WriteRelaunchProbe"),
        TEXT("Write a dedicated cross-launch QA checkpoint in a fresh test profile."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){CheckRelaunch(World,true);}));
    FAutoConsoleCommandWithWorld ReadCheckpointSmoke(TEXT("Foundation.ReadRelaunchProbe"),
        TEXT("Load and verify the QA checkpoint written by an earlier process."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){CheckRelaunch(World,false);}));

    void BeginInteractionSoak(UWorld* World)
    {
        if(!World || !World->IsGameWorld())return;
        FTimerHandle StartHandle;
        World->GetTimerManager().SetTimer(StartHandle,FTimerDelegate::CreateWeakLambda(World,[World]()
        {
            auto* Player=Cast<APhase2Character>(UGameplayStatics::GetPlayerPawn(World,0));
            auto* Controller=UGameplayStatics::GetPlayerController(World,0);
            AFoundationInteractable* Terminal=nullptr;
            for(TActorIterator<AFoundationInteractable> It(World);It;++It)
                if(It->StableId==TEXT("Foundation_TrainingTerminal"))Terminal=*It;
            AFoundationNPC* Patrol=nullptr;
            for(TActorIterator<AFoundationNPC> It(World);It;++It)
                if(It->StableId==PatrolId)Patrol=*It;
            const FString Slot=TEXT("FoundationMixedSoak");
            if(!Player || !Controller || !Terminal || !Patrol || !Player->P2Camera ||
                UGameplayStatics::DoesSaveGameExist(Slot,0) || !Player->FoundationSession->SaveCheckpoint(Slot))
            {
                UE_LOG(LogTemp,Error,TEXT("FoundationMixedSoak FAIL: fixture or fresh profile required"));
                return;
            }
            const FVector Origin=Player->GetActorLocation();
            const double Start=FPlatformTime::Seconds();
            const int32 FirstArrival=Patrol->PatrolArrivals;
            auto Handle=MakeShared<FTimerHandle>();
            auto Tick=[World,Handle,Controller,Terminal,Patrol,Slot,Origin,Start,FirstArrival,
                WeakPlayer=TWeakObjectPtr<APhase2Character>(Player),Phase=0,Cycles=0,ShotsBefore=int32(INDEX_NONE)]() mutable
            {
                auto* P=WeakPlayer.Get();
                bool Ok=P && IsValid(Controller) && IsValid(Terminal) && IsValid(Patrol);
                if(Ok)
                {
                    switch(Phase)
                    {
                    case 0:
                        Controller->SetControlRotation((Terminal->InteractionPoint()-P->P2Camera->GetComponentLocation()).Rotation());
                        break;
                    case 1:
                        Ok=P->InteractionManager->FocusedActor==Terminal;
                        if(!Ok)UE_LOG(LogTemp,Display,TEXT("FoundationMixedSoak focus=%s %s"),
                            *GetNameSafe(P->InteractionManager->FocusedActor),*P->InteractionManager->InspectFocus());
                        if(Ok)UPhase2Tools::TestKey(Controller,Cycles%2?TEXT("E"):TEXT("Gamepad_FaceButton_Left"),true);
                        break;
                    case 2:
                        UPhase2Tools::TestKey(Controller,Cycles%2?TEXT("E"):TEXT("Gamepad_FaceButton_Left"),false);
                        Ok=P->FoundationSession->ObjectiveComplete;
                        UPhase2Tools::TestKey(Controller,TEXT("S"),true);
                        break;
                    case 3:
                        UPhase2Tools::TestKey(Controller,TEXT("S"),false);
                        Ok=FVector::Dist2D(Origin,P->GetActorLocation())>15.f;
                        break;
                    case 5:
                        Ok=P->FoundationSession->LoadCheckpoint(Slot) && !P->FoundationSession->ObjectiveComplete &&
                            FVector::Dist(Origin,P->GetActorLocation())<2.f;
                        if(Ok)++Cycles;
                        break;
                    case 6:
                        if(FPlatformTime::Seconds()-Start<600)break;
                        ShotsBefore=P->Equipment->ShotsFired;
                        UPhase2Tools::TestKey(Controller,TEXT("Gamepad_RightTrigger"),true);
                        break;
                    case 7:
                        if(ShotsBefore==INDEX_NONE)break;
                        UPhase2Tools::TestKey(Controller,TEXT("Gamepad_RightTrigger"),false);
                        Ok=P->Equipment->ShotsFired>ShotsBefore;
                        break;
                    }
                }
                const double Elapsed=FPlatformTime::Seconds()-Start;
                const bool Finished=ShotsBefore!=INDEX_NONE && Phase==7;
                if(Finished)Ok=Ok && Cycles>=10 && Patrol && IsValid(Patrol) && Patrol->PatrolArrivals>FirstArrival+2;
                if(!Ok || Finished)
                {
                    if(IsValid(Controller))
                    {
                        UPhase2Tools::TestKey(Controller,TEXT("S"),false);
                        UPhase2Tools::TestKey(Controller,TEXT("E"),false);
                        UPhase2Tools::TestKey(Controller,TEXT("Gamepad_FaceButton_Left"),false);
                        UPhase2Tools::TestKey(Controller,TEXT("Gamepad_RightTrigger"),false);
                    }
                    UE_LOG(LogTemp,Display,TEXT("FoundationMixedSoak %s phase=%d cycles=%d elapsed=%.2f"),
                        Ok?TEXT("PASS"):TEXT("FAIL"),Phase,Cycles,Elapsed);
                    UGameplayStatics::DeleteGameInSlot(Slot,0);
                    // Clearing an executing timer releases this delegate and its captures.
                    FTimerHandle FinishedHandle=*Handle;
                    World->GetTimerManager().ClearTimer(FinishedHandle);
                    return;
                }
                if(Phase==5 && Cycles%5==0)
                    UE_LOG(LogTemp,Display,TEXT("FoundationMixedSoak progress cycles=%d elapsed=%.2f"),Cycles,Elapsed);
                Phase=(Phase+1)%20;
            };
            World->GetTimerManager().SetTimer(*Handle,FTimerDelegate::CreateWeakLambda(World,MoveTemp(Tick)),.5f,true);
        }),4.f,false);
    }
    FAutoConsoleCommandWithWorld MixedSoak(TEXT("Foundation.MixedSoak"),
        TEXT("Repeat terminal interaction, movement and checkpoint restore for ten minutes in an isolated sandbox profile."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&BeginInteractionSoak));

    void VerifyTitleFlow(UWorld* World)
    {
        if(!World || !World->IsGameWorld())return;
        auto* Instance=World->GetGameInstance();
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(Instance,
            [Instance,Step=0,OldPawn=TWeakObjectPtr<APhase2Character>(),Guard=TWeakObjectPtr<AFoundationNPC>(),InitialShots=0,ResponseSamples=0](float) mutable
        {
            auto* CurrentWorld=Instance->GetWorld();
            auto* Player=Cast<APhase2Character>(UGameplayStatics::GetPlayerPawn(CurrentWorld,0));
            auto* Session=Player?Player->FoundationSession.Get():nullptr;
            if(!Session || !FSlateApplication::IsInitialized())return false;
            auto State=Session->GetSessionState();
            auto Capture=[](const TCHAR* Name)
            {
                FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ScreenShotDir(),Name),true,false);
            };
            bool Ok=true;
            switch(Step)
            {
            case 0:
                Ok=State==EFoundationSessionState::Menu && UGameplayStatics::IsGamePaused(CurrentWorld);
                if(Ok)Capture(TEXT("Title.png"));
                break;
            case 1:
            {
                FKeyEvent Event(EKeys::Enter,FModifierKeysState(),0,false,0,0);
                FSlateApplication::Get().ProcessKeyDownEvent(Event);
                FSlateApplication::Get().ProcessKeyUpEvent(Event);
                break;
            }
            case 2:
                Ok=State==EFoundationSessionState::Playing && !UGameplayStatics::IsGamePaused(CurrentWorld);
                if(Ok)Capture(TEXT("Playing.png"));
                break;
            case 3:
                Session->TogglePause();
                break;
            case 4:
                Ok=State==EFoundationSessionState::Paused && UGameplayStatics::IsGamePaused(CurrentWorld);
                if(Ok)Capture(TEXT("Paused.png"));
                break;
            case 5:
                Session->TogglePause();
                break;
            case 6:
                Ok=State==EFoundationSessionState::Playing && !UGameplayStatics::IsGamePaused(CurrentWorld);
                OldPawn=Player;
                for(TActorIterator<AFoundationNPC> It(CurrentWorld);It;++It)
                {
                    if(It->StableId==PatrolId)Guard=*It;
                    else
                    {
                        // An inactive ally is still a valid target. Keep competing
                        // actors outside this isolated perception trial.
                        It->SetAIEnabled(false);
                        It->SetActorHiddenInGame(true);
                        It->SetActorEnableCollision(false);
                        It->SetActorLocation(It->GetActorLocation()+FVector(0,0,10000),false,nullptr,ETeleportType::TeleportPhysics);
                        It->GetCharacterMovement()->DisableMovement();
                    }
                }
                Ok=Ok && Guard.IsValid();
                if(Ok)
                {
                    Guard->SetAIEnabled(false);
                    Guard->PatrolRoute.Reset();
                    Guard->GetCharacterMovement()->StopMovementImmediately();
                    Guard->GetCharacterMovement()->DisableMovement();
                    Guard->SetActorLocationAndRotation(FVector(2600,180,-1.6),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
                    if(auto* Control=Guard->GetController())Control->SetControlRotation(FRotator::ZeroRotator);
                    Player->SetActorLocation(FVector(2250,180,-1.6),false,nullptr,ETeleportType::TeleportPhysics);
                    InitialShots=Guard->ShotsFired;
                    Guard->SetAIEnabled(true);
                }
                break;
            case 7:
                Ok=Guard.IsValid() && !Guard->SeesTarget;
                Player->SetActorLocation(FVector(3100,180,-1.6),false,nullptr,ETeleportType::TeleportPhysics);
                Player->GetController()->SetControlRotation(FRotator(0,180,0));
                break;
            case 8:
            {
                const bool Responded=Guard.IsValid() && Guard->SeesTarget && Guard->Target==Player && Guard->ShotsFired>InitialShots;
                if(!Responded && ++ResponseSamples<4)return true;
                UE_LOG(LogTemp,Display,TEXT("FoundationSightAndResponse %s sees=%d shots=%d awareness=%.2f target=%s npc=%s facing=%s player=%s"),
                    Responded?TEXT("PASS"):TEXT("FAIL"),Guard.IsValid() && Guard->SeesTarget,
                    Guard.IsValid()?Guard->ShotsFired-InitialShots:0,Guard.IsValid()?Guard->Awareness:0.f,
                    Guard.IsValid()?*GetNameSafe(Guard->Target):TEXT("none"),
                    Guard.IsValid()?*Guard->GetActorLocation().ToCompactString():TEXT("none"),
                    Guard.IsValid()?*Guard->GetActorRotation().ToCompactString():TEXT("none"),
                    *Player->GetActorLocation().ToCompactString());
                UGameplayStatics::ApplyDamage(Player,150.f,Player->GetController(),Player,nullptr);
                break;
            }
            case 9:
                Ok=State==EFoundationSessionState::Dead;
                if(Ok)Capture(TEXT("Dead.png"));
                break;
            case 10:
                UPhase2Tools::TestKey(UGameplayStatics::GetPlayerController(CurrentWorld,0),TEXT("R"),true);
                break;
            case 11:
                UPhase2Tools::TestKey(UGameplayStatics::GetPlayerController(CurrentWorld,0),TEXT("R"),false);
                Ok=Player!=OldPawn.Get() && State==EFoundationSessionState::Playing;
                break;
            }
            if(!Ok || Step==11)
            {
                UE_LOG(LogTemp,Display,TEXT("FoundationTitleFlow %s step=%d state=%d"),Ok?TEXT("PASS"):TEXT("FAIL"),Step,int32(State));
                return false;
            }
            ++Step;
            return true;
        }),1.5f);
    }
    FAutoConsoleCommandWithWorld TitleFlow(TEXT("Foundation.VerifyTitleFlow"),
        TEXT("Check the title, play and pause transitions and capture all three screens."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyTitleFlow));
}
#endif
