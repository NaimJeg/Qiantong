#pragma once
#include "DemoBattle.h"

namespace QiantongDemo
{
// Non-authoritative samples. No Actor or render callback can write back to FBattle.
struct FRenderUnit
{
    int32 Id = 0;
    FVector Previous = FVector::ZeroVector, Current = FVector::ZeroVector;
    void Push(const FUnit& Unit, bool Snap = false)
    {
        Previous = Current;
        Current = FVector(Unit.X, Unit.Position, Unit.Aim);
        // Walking is bounded to 200/tick. Larger jumps are offscreen relocations.
        if (Snap || Id != Unit.Id || FMath::Abs(Current.Y - Previous.Y) > 200)
            Previous = Current;
        Id = Unit.Id;
    }
    FVector Sample(double Alpha) const
    {
        Alpha = FMath::Clamp(Alpha, 0.0, 1.0);
        return FVector(FMath::Lerp(Previous.X, Current.X, Alpha),
            FMath::Lerp(Previous.Y, Current.Y, Alpha),
            Previous.Z + AngleDelta(int32(Previous.Z), int32(Current.Z)) * Alpha);
    }
};
struct FRenderSnapshot
{
    TArray<FRenderUnit> Units;
    double PreviousCamera = 0, CurrentCamera = 0;
    void Capture(const FBattle& Battle, bool Snap = false)
    {
        Snap |= Units.Num() != Battle.Units.Num();
        Units.SetNum(Battle.Units.Num());
        for (int32 I = 0; I < Units.Num(); ++I) Units[I].Push(Battle.Units[I], Snap);
        PreviousCamera = Snap ? Battle.CameraY : CurrentCamera;
        CurrentCamera = Battle.CameraY;
    }
};
}
