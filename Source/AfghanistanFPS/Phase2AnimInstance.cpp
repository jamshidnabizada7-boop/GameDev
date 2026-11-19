#include "Phase2AnimInstance.h"
#include "Phase2Character.h"
#include "FoundationInteraction.h"
#include "FoundationEquipment.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimSequence.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"
#include "UObject/ConstructorHelpers.h"

// Original procedural prototype overlays. Existing Epic mannequin sequences
// supply normal locomotion; no external animation binaries are imported.
struct FPhase2AnimProxy : FAnimInstanceProxy
{
    explicit FPhase2AnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    UAnimSequence* Sequence=nullptr;
    float Time=0, Clock=0, Speed=0, Crouch=0, Prone=0, Lean=0, Recoil=0, LandTime=1, Surrender=0, Stagger=0;
    bool FP=false, Falling=false, WasFalling=false, Traversing=false,ThirdHands=false;
    FTransform ComponentWorld;
    FVector FootOffset[2]={FVector::ZeroVector,FVector::ZeroVector};
    FQuat FootTilt[2]={FQuat::Identity,FQuat::Identity};
    FVector RightHand=FVector::ZeroVector, LeftHand=FVector::ZeroVector;
    FVector RightElbow=FVector::ZeroVector, LeftElbow=FVector::ZeroVector;
    virtual void PreUpdate(UAnimInstance* Instance,float Dt) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Dt);
        auto* C=Cast<APhase2Character>(Instance->TryGetPawnOwner());
        auto* I=Cast<UPhase2AnimInstance>(Instance);
        if(!C || !I || I->Sequences.Num()<6) return;
        Clock+=Dt; Time+=Dt; LandTime+=Dt;
        Speed=C->GetVelocity().Size2D(); Crouch=C->P2PoseCrouch; Prone=C->P2PoseProne;
        Lean=C->P2Lean; Recoil=C->P2Recoil; Traversing=C->P2Traversing;
        ThirdHands=C->P2ThirdPersonHands;Surrender=C->P2SurrenderAlpha;Stagger=C->P2StaggerAlpha;
        FP=Instance->GetSkelMeshComponent()==C->P2Arms;
        Falling=C->GetCharacterMovement()->IsFalling();
        if(WasFalling && !Falling) LandTime=0;
        WasFalling=Falling;
        const int Index=Falling?(C->GetVelocity().Z>50?3:4):LandTime<0.22f?5:Speed>500?2:Speed>5?1:0;
        UAnimSequence* Next=I->Sequences[Index];
        if(Next!=Sequence) { Sequence=Next; Time=0; }
        if(Sequence && Time>Sequence->GetPlayLength()) Time=FMath::Fmod(Time,Sequence->GetPlayLength());
        ComponentWorld=Instance->GetSkelMeshComponent()->GetComponentTransform();
        if(!FP)
        {
            const bool Grounded=C->GetCharacterMovement()->IsMovingOnGround() && !Traversing && Prone<0.01f;
            const auto* Capsule=C->GetCapsuleComponent();
            const double FloorZ=Capsule->GetComponentLocation().Z-Capsule->GetScaledCapsuleHalfHeight();
            const FName Feet[]={TEXT("foot_l"),TEXT("foot_r")};
            FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundationFootPlacement),false,C);
            for(int32 Side=0; Side<2; ++Side)
            {
                FVector Offset=FVector::ZeroVector;
                FQuat Tilt=FQuat::Identity;
                if(Grounded)
                {
                    FVector Point=Instance->GetSkelMeshComponent()->GetSocketLocation(Feet[Side]);
                    Point.Z=FloorZ;
                    FHitResult Hit;
                    if(C->GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector::UpVector*32,
                        Point-FVector::UpVector*32,ECC_Visibility,Query) && C->GetCharacterMovement()->IsWalkable(Hit))
                    {
                        const double Height=FMath::Clamp(Hit.ImpactPoint.Z-FloorZ,-24.0,24.0);
                        Offset=ComponentWorld.InverseTransformVector(FVector::UpVector*Height);
                        const FVector Up=ComponentWorld.InverseTransformVectorNoScale(FVector::UpVector);
                        const FVector Normal=ComponentWorld.InverseTransformVectorNoScale(Hit.ImpactNormal);
                        Tilt=FQuat::FindBetweenNormals(Up,Normal);
                    }
                }
                FootOffset[Side]=Grounded?FMath::VInterpTo(FootOffset[Side],Offset,Dt,16.f):FVector::ZeroVector;
                FootTilt[Side]=Grounded?FQuat::Slerp(FootTilt[Side],Tilt,FMath::Clamp(Dt*16.f,0.f,1.f)):FQuat::Identity;
            }
        }
        if(C->P2Camera)
        {
            const FTransform View=C->P2Camera->GetComponentTransform();
            RightHand=ComponentWorld.InverseTransformPosition(View.TransformPosition(FVector(37,14,-22)));
            LeftHand=ComponentWorld.InverseTransformPosition(View.TransformPosition(FVector(54,13,-18)));
            if(C->P2GripRight) RightHand=ComponentWorld.InverseTransformPosition(C->P2GripRight->GetComponentLocation());
            if(C->P2GripLeft) LeftHand=ComponentWorld.InverseTransformPosition(C->P2GripLeft->GetComponentLocation());
            if(FP && C->InteractionManager && C->InteractionManager->ReachAlpha>0)
            {
                const FVector Origin=View.GetLocation();
                const FVector Reach=Origin+(C->InteractionManager->ReachPoint()-Origin).GetClampedToMaxSize(55);
                LeftHand=FMath::Lerp(LeftHand,ComponentWorld.InverseTransformPosition(Reach),C->InteractionManager->ReachAlpha);
            }
            if(FP && C->Equipment && C->Equipment->IsHoldingThrowable())
                LeftHand=ComponentWorld.InverseTransformPosition(C->Equipment->ThrowableHandPoint());
            if(Surrender>.001f)
            {
                RightHand=FMath::Lerp(RightHand,ComponentWorld.InverseTransformPosition(View.TransformPosition(FVector(5,32,22))),Surrender);
                LeftHand=FMath::Lerp(LeftHand,ComponentWorld.InverseTransformPosition(View.TransformPosition(FVector(5,-32,22))),Surrender);
            }
            RightElbow=ComponentWorld.InverseTransformPosition(View.TransformPosition(FVector(20,26,-45)));
            LeftElbow=ComponentWorld.InverseTransformPosition(View.TransformPosition(FVector(28,-20,-48)));
        }
    }

    virtual bool Evaluate(FPoseContext& Output) override
    {
        Output.ResetToRefPose();
        if(Sequence)
        {
            FAnimationPoseData Data(Output);
            Sequence->GetAnimationPose(Data,FAnimExtractContext(double(Time),false));
        }
        const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
        auto Id=[&Bones](const TCHAR* Name)
        {
            const int32 MeshIndex=Bones.GetPoseBoneIndexForBoneName(FName(Name));
            return MeshIndex==INDEX_NONE?FCompactPoseBoneIndex(INDEX_NONE):Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
        };
        auto Rotate=[&](const TCHAR* Name,FVector Axis,float Degrees)
        {
            const auto B=Id(Name);
            if(B.IsValid()) { auto& T=Output.Pose[B]; T.SetRotation((T.GetRotation()*FQuat(Axis,FMath::DegreesToRadians(Degrees))).GetNormalized()); }
        };
        const auto Pelvis=Id(TEXT("pelvis"));
        // Lower the hips to keep the downhill ankle within leg reach.
        const FVector HipShift=FootOffset[0].Z<FootOffset[1].Z?FootOffset[0]:FootOffset[1];
        const FVector FloorShift=HipShift.Z<0?HipShift:FVector::ZeroVector;
        if(!FP && Pelvis.IsValid())
        {
            Output.Pose[Pelvis].AddToTranslation(FVector(0,0,-56*Crouch)+FloorShift);
            Rotate(TEXT("spine_01"),FVector::ForwardVector,-12*Crouch);
            Rotate(TEXT("spine_03"),FVector::RightVector,Lean*7);
            Rotate(TEXT("spine_01"),FVector::ForwardVector,-Stagger*14);
        }
        if(!FP && Prone>0.001f)
        {
            // Blend toward a horizontal reference posture. The animation root
            // lies above the floor; knees and arms cycle for visible crawling.
            for(FCompactPoseBoneIndex B:Output.Pose.ForEachBoneIndex())
                Output.Pose[B].Blend(Output.Pose[B],Output.Pose.GetRefPose(B),Prone);
            auto& Root=Output.Pose[FCompactPoseBoneIndex(0)];
            Root.SetRotation(FQuat(FVector::ForwardVector,FMath::DegreesToRadians(-90*Prone))*Root.GetRotation());
            Root.AddToTranslation(FVector(0,-65*Prone,24*Prone));
            const float Cycle=FMath::Sin(Clock*6)*FMath::Clamp(Speed/100.f,0.f,1.f)*Prone;
            Rotate(TEXT("thigh_l"),FVector::RightVector,10+Cycle*12);
            Rotate(TEXT("thigh_r"),FVector::RightVector,10-Cycle*12);
            Rotate(TEXT("calf_l"),FVector::RightVector,-18-Cycle*15);
            Rotate(TEXT("calf_r"),FVector::RightVector,-18+Cycle*15);
            Rotate(TEXT("upperarm_l"),FVector::RightVector,-55-Cycle*15);
            Rotate(TEXT("upperarm_r"),FVector::RightVector,55-Cycle*15);
            Rotate(TEXT("lowerarm_l"),FVector::RightVector,-50);
            Rotate(TEXT("lowerarm_r"),FVector::RightVector,50);
        }
        FCSPose<FCompactPose> CS;
        CS.InitPose(Output.Pose);
        auto IK=[&](const TCHAR* Upper,const TCHAR* Lower,const TCHAR* End,const FVector& Target,const FVector& Pole)
        {
            const auto A=Id(Upper), B=Id(Lower), D=Id(End);
            if(!A.IsValid() || !B.IsValid() || !D.IsValid()) return;
            FTransform TA=CS.GetComponentSpaceTransform(A), TB=CS.GetComponentSpaceTransform(B), TD=CS.GetComponentSpaceTransform(D);
            if(FP && (FStringView(End)==TEXT("hand_l") || FStringView(End)==TEXT("hand_r")))
            {
                // Advance the camera-only shoulder when a grip sits beyond the
                // two-bone reach; preserve forearm and upper arm lengths.
                const FVector Delta=Target-TA.GetLocation();
                const double Reach=FVector::Distance(TA.GetLocation(),TB.GetLocation())
                    +FVector::Distance(TB.GetLocation(),TD.GetLocation());
                const FVector Shift=Delta.GetSafeNormal()*FMath::Clamp(Delta.Size()-Reach+0.05,0.,20.);
                TA.AddToTranslation(Shift); TB.AddToTranslation(Shift); TD.AddToTranslation(Shift);
            }
            const FQuat PreviousLower=TB.GetRotation();
            AnimationCore::SolveTwoBoneIK(TA,TB,TD,Pole,Target,false,1.0,1.0);
            if(FStringView(End).StartsWith(TEXT("hand_")))
                TD.SetRotation((TB.GetRotation()*PreviousLower.Inverse()*TD.GetRotation()).GetNormalized());
            else
                TD.SetRotation((FootTilt[FStringView(End)==TEXT("foot_l")?0:1]*TD.GetRotation()).GetNormalized());
            TArray<FBoneTransform> Changes={FBoneTransform(A,TA),FBoneTransform(B,TB),FBoneTransform(D,TD)};
            CS.SafeSetCSBoneTransforms(Changes);
        };
        if(!FP && Prone<0.01f && (Crouch>0.001f || !FootOffset[0].IsNearlyZero() || !FootOffset[1].IsNearlyZero()
            || !FootTilt[0].IsIdentity() || !FootTilt[1].IsIdentity()))
        {
            // Keep each foot at its animated floor position after lowering the
            // pelvis. IK folds the legs instead of scaling an upright mannequin.
            const auto L=Id(TEXT("foot_l")), R=Id(TEXT("foot_r"));
            if(L.IsValid() && R.IsValid())
            {
                const FVector Compensation=FVector(0,0,56*Crouch)-FloorShift;
                FVector LT=CS.GetComponentSpaceTransform(L).GetLocation()+Compensation+FootOffset[0];
                FVector RT=CS.GetComponentSpaceTransform(R).GetLocation()+Compensation+FootOffset[1];
                IK(TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l"),LT,FVector(-20,90,45));
                IK(TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r"),RT,FVector(20,90,45));
            }
        }
        if(FP || ThirdHands || Surrender>.001f)
        {
            IK(TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),LeftHand,LeftElbow);
            IK(TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r"),RightHand,RightElbow);
        }
        else if(Traversing)
        {
            IK(TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),FVector(-26,55,140),FVector(-60,20,100));
            IK(TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r"),FVector(26,55,140),FVector(60,20,100));
        }
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(CS,Output.Pose);
        if(FP)
        {
            // Curl fingers around the prototype's existing grip markers.
            for(const TCHAR* Side:{TEXT("l"),TEXT("r")})
                for(const TCHAR* Finger:{TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")})
                    for(int32 Segment=1;Segment<=3;++Segment)
                    {
                        const FString Name=FString::Printf(TEXT("%s_%02d_%s"),Finger,Segment,Side);
                        Rotate(*Name,FVector::UpVector,Side[0]==TCHAR('l')?45.f:-45.f);
                    }
        }
        Output.Pose.NormalizeRotations();
        return true;
    }
};

UPhase2AnimInstance::UPhase2AnimInstance()
{
    static const TCHAR* Paths[]={
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"),
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"),
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"),
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump"),
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop.MM_Fall_Loop"),
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land.MM_Land")};
    for(const TCHAR* Path:Paths)
    {
        ConstructorHelpers::FObjectFinder<UAnimSequence> Found(Path);
        Sequences.Add(Found.Object);
    }
}
FAnimInstanceProxy* UPhase2AnimInstance::CreateAnimInstanceProxy() { return new FPhase2AnimProxy(this); }
void UPhase2AnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
