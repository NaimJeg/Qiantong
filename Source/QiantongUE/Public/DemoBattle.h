#pragma once
#include "CoreMinimal.h"

namespace QiantongDemo
{
constexpr double StepSeconds = 0.05;
constexpr int32 GridWidth = 9, GridHeight = 13, CellSize = 1000;
constexpr int32 ViewDepth=13000, WaveStride=26000, CameraStep=650, OffscreenMargin=800;
constexpr int32 MapRows=65;
constexpr int32 MaxTurn = 9000, AimTolerance = 3000, AttackRange = 3500;
QIANTONGUE_API int32 AngleDelta(int32 From, int32 To);
QIANTONGUE_API int32 TurnTowards(int32 From, int32 To);
enum class EAction : uint8 { Approach, Aim, Engage, Evade, Descend, Enter, Cover, Dead };
struct FUnit
{
    int32 Id = 0, Generation = 1;
    bool Ally = true;
    int32 Hp = 600, MaxHp = 600, Damage = 30, Interval = 12;
    int32 X = 4500, Position = 1500;
    int32 Target = 0, NextShot = 0, Aim = 90000;
    int32 Waypoint = -1;
    bool Engaged = false;
    EAction Action = EAction::Approach;
};
struct FShot
{
    int32 Source = 0, Target = 0, Damage = 0, Kind = 0;
    int32 StartX = 0, StartY = 0, EndX = 0, EndY = 0;
};
struct FHazard { int32 X = 0, Y = 0, ImpactTick = 0; };
class QIANTONGUE_API FBattle
{
public:
    void Reset(int32 Allies, uint32 InSeed = 1);
    void Step(int32 Choose = 0);
    bool CanChoose() const { return Tick >= 100 && Choice == 0 && !IsOver() && DescentTicks == 0 && !Entering; }
    bool IsOver() const { return Winner != 0; }
    const FUnit* Find(int32 Id) const;
    FString ResultJson() const;
    bool Blocked(int32 Col, int32 Row) const;
    bool ClearRay(int32 X1,int32 Y1,int32 X2,int32 Y2) const;
    bool MapConnected() const;
    bool Offscreen(int32 Y) const { return Y+OffscreenMargin<CameraY || Y-OffscreenMargin>CameraY+ViewDepth; }
    bool Dangerous(int32 X,int32 Y) const;
    TArray<FUnit> Units;
    TArray<FShot> LastShots;
    const TArray<int32>& GetObstacles() const { return Obstacles; }
    void SetObstacles(const TArray<int32>& Cells);
    TArray<FHazard> Hazards;
    int32 Tick=0, Choice=0, ChoiceTick=0, Winner=0;
    int32 Shots=0, TargetSwitches=0, AllyCount=2;
    int32 Wave=1, WaveStartTick=0, WavesCleared=0, DescentTicks=0;
    int32 CameraY=0, Relocations=0;
    bool Entering=true;
    int32 EvasionSteps=0, HazardsCreated=0, HazardHits=0;
    uint32 Seed=1, Rng=1, EventHash=2166136261u;
private:
    TArray<int32> Obstacles;
    uint8 ObstacleMask[GridWidth * MapRows] = {};
    bool OccupiedCell(int32 Cell) const { return Cell >= 0 && Cell < GridWidth * MapRows && ObstacleMask[Cell] != 0; }
    void Event(int32 Type,int32 A,int32 B,int32 Value);
    uint32 Random();
    void StartWave();
    void GenerateMap();
    void AdvanceCover();
    void Walk(FUnit& U);
    int32 NextCell(const FUnit& U,const FUnit* Target,bool Evading) const;
};
struct QIANTONGUE_API FClock
{
    double Remainder = 0;
    int32 Advance(double Delta,int32 Speed,bool Paused);
};
}
