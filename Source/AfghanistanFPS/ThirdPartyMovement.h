#pragma once
#include "CoreMinimal.h"

// Adapted from Project Borealis PBPlayerCharacter.h::SimpleSpline and
// PBPlayerCharacter.cpp::RecalculateBaseEyeHeight, commit a06250ea7a95f157a8e30a80c9e4bf2264ba7cb0.
// Copyright (c) 2019-2025 Project Borealis. MIT; see ThirdPartyNotices/PBCharacterMovement-LICENSE.txt.
namespace Phase2Movement
{
    inline float SimpleSpline(float Value)
    {
        const float ValueSquared = Value * Value;
        return 3.0f * ValueSquared - 2.0f * ValueSquared * Value;
    }
    inline float EyeTransition(float From, float To, float Alpha)
    {
        return FMath::Lerp(From, To, SimpleSpline(FMath::Clamp(Alpha, 0.f, 1.f)));
    }
}
