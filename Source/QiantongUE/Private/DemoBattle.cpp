#include "DemoBattle.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

CSV_DEFINE_CATEGORY(Qiantong, true);

namespace QiantongDemo
{
namespace
{
int64 DistanceSq(int32 X,int32 Y,int32 X2,int32 Y2)
{ return int64(X-X2)*(X-X2)+int64(Y-Y2)*(Y-Y2); }
int32 Bearing(int32 X,int32 Y,int32 X2,int32 Y2)
{ return (FMath::RoundToInt(FMath::Atan2(double(Y2-Y),double(X2-X))*180000.0/PI)+360000)%360000; }
// Stable neighbor order is part of ruleVersion demo-2.
const int32 DX[]={0,-1,1,0}, DY[]={1,0,0,-1};
}
int32 AngleDelta(int32 From,int32 To)
{
    int32 D=(To-From+540000)%360000-180000;
    return D == -180000 ? 180000 : D;
}
int32 TurnTowards(int32 From,int32 To)
{ return (From+FMath::Clamp(AngleDelta(From,To),-MaxTurn,MaxTurn)+360000)%360000; }
uint32 FBattle::Random()
{ Rng^=Rng<<13; Rng^=Rng>>17; Rng^=Rng<<5; return Rng; }
void FBattle::Reset(int32 Allies,uint32 InSeed)
{
    *this=FBattle(); AllyCount=Allies==5?5:2; Seed=InSeed?InSeed:1; Rng=Seed;
    GenerateMap();
    for(int32 I=0;I<AllyCount;++I)
    { FUnit U; U.Id=I+1; U.X=(2+I)*1000+500; U.Position=-1200; U.Action=EAction::Enter; Units.Add(U); }
    // Finite three-wave prototype: actors/entities exist before the camera reaches them.
    for(int32 W=1;W<=3;++W) for(int32 I=0;I<5+W;++I)
    {
        FUnit U; U.Id=W*100+I+1; U.Generation=W; U.Ally=false; U.Hp=U.MaxHp=90;
        U.Damage=8; U.Interval=32; U.X=(I%7+1)*1000+500;
        U.Position=(W-1)*WaveStride+(10+I/7)*1000+500; U.Aim=270000; U.Action=EAction::Cover;
        Units.Add(U);
    }
    Event(0,AllyCount,3,static_cast<int32>(Seed)); StartWave();
}
void FBattle::SetObstacles(const TArray<int32>& Cells)
{
    FMemory::Memzero(ObstacleMask);
    Obstacles.Reset();
    for(int32 Cell:Cells) if(Cell>=0 && Cell<GridWidth*MapRows && !OccupiedCell(Cell))
    { ObstacleMask[Cell]=1; Obstacles.Add(Cell); }
    Obstacles.Sort();
}
bool FBattle::Blocked(int32 Col,int32 Row) const
{ return Col<0 || Col>=GridWidth || Row<0 || Row>=MapRows || OccupiedCell(Row*GridWidth+Col); }
bool FBattle::MapConnected() const
{
    TArray<int32> Queue; TArray<uint8> Visited; Visited.Init(0,GridWidth*MapRows);
    for(int32 C=0;C<GridWidth*MapRows;++C) if(!Blocked(C%GridWidth,C/GridWidth)) { Queue.Add(C); Visited[C]=1; break; }
    for(int32 I=0;I<Queue.Num();++I)
        for(int32 N=0;N<4;++N)
        {
            const int32 X=Queue[I]%GridWidth+DX[N],Y=Queue[I]/GridWidth+DY[N],C=Y*GridWidth+X;
            if(!Blocked(X,Y) && !Visited[C]) { Queue.Add(C); Visited[C]=1; }
        }
    return Queue.Num()==GridWidth*MapRows-Obstacles.Num();
}
void FBattle::GenerateMap()
{
    for(int32 Segment=0;Segment<5;++Segment)
    {
        const int32 Before=Obstacles.Num();
        for(int32 Try=0;Try<200 && Obstacles.Num()<Before+9+Segment%3;++Try)
        {
            const int32 X=Random()%GridWidth,Y=Segment*GridHeight+3+Random()%6,C=Y*GridWidth+X;
            if(X==4 || OccupiedCell(C)) continue;
            Obstacles.Add(C); ObstacleMask[C]=1;
            if(!MapConnected()) { Obstacles.Pop(); ObstacleMask[C]=0; }
        }
    }
    Obstacles.Sort(); for(int32 C:Obstacles) Event(8,0,C,0);
}
void FBattle::StartWave()
{
    Hazards.Reset(); Entering=true;
    // Only friendly units are recycled, and only while both endpoints are wholly outside.
    for(auto& U:Units) if(U.Ally && U.Hp>0)
    {
        if(Wave>1 && Offscreen(U.Position))
        {
            const int32 Old=U.Position; U.Position=CameraY-1200; ++Relocations;
            Event(15,U.Id,Old,U.Position);
        }
        U.Target=0; U.Waypoint=-1; U.Engaged=false; U.Action=EAction::Enter;
    }
    Event(9,Wave,Units.Num(),static_cast<int32>(Rng));
}
void FBattle::Walk(FUnit& U)
{
    if(U.Waypoint<0) return;
    const int32 X=U.Waypoint%GridWidth*1000+500,Y=U.Waypoint/GridWidth*1000+500;
    if(U.X!=X) U.X+=FMath::Clamp(X-U.X,-200,200);
    else U.Position+=FMath::Clamp(Y-U.Position,-200,200);
    if(U.X==X && U.Position==Y) U.Waypoint=-1;
}
void FBattle::AdvanceCover()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(QT_AdvanceCover);
    CSV_SCOPED_TIMING_STAT(Qiantong, QT_AdvanceCover);
    // Future enemies continue moving independently of camera position and wave transitions.
    TArray<FUnit> Next=Units;
    for(int32 I=0;I<Units.Num();++I)
    {
        const auto& U=Units[I]; auto& V=Next[I];
        if(U.Ally || U.Hp<=0 || (U.Generation==Wave && !Entering && DescentTicks==0)) continue;
        bool Covered=false;
        for(int32 N=0;N<4;++N)
            if(OccupiedCell((U.Position/1000+DY[N])*GridWidth+U.X/1000+DX[N])) Covered=true;
        if(V.Waypoint<0 && !Covered) V.Waypoint=NextCell(U,nullptr,false);
        Walk(V); V.Action=EAction::Cover;
        Event(16,V.Id,V.X,V.Position);
    }
    Units=MoveTemp(Next);
}
const FUnit* FBattle::Find(int32 Id) const
{ return Units.FindByPredicate([Id](const FUnit& U){return U.Id==Id;}); }
void FBattle::Event(int32 Type,int32 A,int32 B,int32 Value)
{
    for(int32 Word:{Tick,Type,A,B,Value})
        for(int32 Shift=0;Shift<32;Shift+=8)
            EventHash=(EventHash^((static_cast<uint32>(Word)>>Shift)&255u))*16777619u;
}
bool FBattle::ClearRay(int32 X1,int32 Y1,int32 X2,int32 Y2) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(QT_ClearRay);
    CSV_SCOPED_TIMING_STAT(Qiantong, QT_ClearRay);
    // Row supercover: clip the segment to each crossed row, then visit its X span.
    // Include both neighbors at exact grid boundaries (closed edge/corner semantics).
    const int32 First=FMath::Max(0,FMath::CeilToInt(double(FMath::Min(Y1,Y2))/CellSize)-1);
    const int32 Last=FMath::Min(MapRows-1,FMath::FloorToInt(double(FMath::Max(Y1,Y2))/CellSize));
    for(int32 Row=First;Row<=Last;++Row)
    {
        double Near=0,Far=1;
        if(Y1!=Y2)
        {
            const double A=double(Row*CellSize-Y1)/double(Y2-Y1);
            const double B=double((Row+1)*CellSize-Y1)/double(Y2-Y1);
            Near=FMath::Max(0.0,FMath::Min(A,B)); Far=FMath::Min(1.0,FMath::Max(A,B));
        }
        const double A=X1+double(X2-X1)*Near,B=X1+double(X2-X1)*Far;
        // Conservative epsilon only widens candidates; exact slab below decides hits.
        const int32 Left=FMath::Max(0,FMath::CeilToInt((FMath::Min(A,B)-1.e-7)/CellSize)-1);
        const int32 Right=FMath::Min(GridWidth-1,FMath::FloorToInt((FMath::Max(A,B)+1.e-7)/CellSize));
        for(int32 Col=Left;Col<=Right;++Col) if(OccupiedCell(Row*GridWidth+Col))
        {
            double HitNear=Near,HitFar=Far;
            if(X1==X2) { if(X1>=Col*CellSize && X1<=(Col+1)*CellSize) return false; }
            else
            {
                const double C=double(Col*CellSize-X1)/double(X2-X1);
                const double D=double((Col+1)*CellSize-X1)/double(X2-X1);
                HitNear=FMath::Max(HitNear,FMath::Min(C,D));
                HitFar=FMath::Min(HitFar,FMath::Max(C,D));
                if(HitNear<=HitFar) return false;
            }
        }
    }
    return true;
}
bool FBattle::Dangerous(int32 X,int32 Y) const
{
    for(const auto& H:Hazards) if(DistanceSq(X,Y,H.X,H.Y)<=1200LL*1200) return true;
    return false;
}
int32 FBattle::NextCell(const FUnit& U,const FUnit* Target,bool Evading) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(QT_NextCell);
    CSV_SCOPED_TIMING_STAT(Qiantong, QT_NextCell);
    const int32 BaseRow=U.Ally?CameraY/1000:(U.Generation-1)*WaveStride/1000;
    const int32 Offset=BaseRow*GridWidth,Start=U.Position/1000*GridWidth+U.X/1000;
    if(Start<Offset || Start>=Offset+GridWidth*GridHeight) return -1;
    TArray<int32> Queue; Queue.Add(Start);
    int32 Parent[GridWidth*GridHeight]; for(int32& P:Parent) P=-1; Parent[Start-Offset]=Start;
    for(int32 I=0;I<Queue.Num();++I)
    {
        const int32 C=Queue[I],X=(C%GridWidth)*1000+500,Y=(C/GridWidth)*1000+500;
        bool Occupied=false,Covered=false;
        for(const auto& Other:Units) if(Other.Hp>0 && Other.Ally==U.Ally && Other.Id!=U.Id &&
            (DistanceSq(X,Y,Other.X,Other.Position)<700LL*700 || Other.Waypoint==C)) Occupied=true;
        if(!Target) for(int32 N=0;N<4;++N)
            if(OccupiedCell((C/GridWidth+DY[N])*GridWidth+C%GridWidth+DX[N])) Covered=true;
        if(C!=Start && !Occupied && !Dangerous(X,Y) && (Evading || (!Target && Covered) || (Target &&
            DistanceSq(X,Y,Target->X,Target->Position)<=int64(AttackRange)*AttackRange && ClearRay(X,Y,Target->X,Target->Position))))
        {
            int32 Next=C;
            while(Parent[Next-Offset]!=Start) Next=Parent[Next-Offset];
            return Next;
        }
        for(int32 N=0;N<4;++N)
        {
            const int32 NX=C%GridWidth+DX[N],NY=C/GridWidth+DY[N],NC=NY*GridWidth+NX;
            if(NY<BaseRow || NY>=BaseRow+GridHeight || Blocked(NX,NY) || Parent[NC-Offset]!=-1) continue;
            if(!Evading && Dangerous(NX*1000+500,NY*1000+500)) continue;
            Parent[NC-Offset]=C; Queue.Add(NC);
        }
    }
    return -1;
}
void FBattle::Step(int32 Choose)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(QT_BattleStep);
    CSV_SCOPED_TIMING_STAT(Qiantong, QT_BattleStep);
    if(IsOver()) return;
    const bool AcceptChoice=CanChoose() && Choose>=1 && Choose<=3;
    ++Tick; LastShots.Reset();
    if(AcceptChoice) { Choice=Choose; ChoiceTick=Tick; Event(1,0,0,Choice); }
    AdvanceCover();
    if(DescentTicks>0)
    {
        if(Tick>=6000) { Winner=3; Event(6,Winner,Wave,Tick); return; }
        CameraY+=CameraStep;
        for(auto& U:Units) if(U.Ally && U.Hp>0)
        {
            // Preserve X; obstruction cannot hold back the faster camera.
            if(ClearRay(U.X,U.Position,U.X,U.Position+200)) U.Position+=200;
            Event(17,U.Id,U.X,U.Position);
        }
        Event(18,CameraY,DescentTicks,0);
        --DescentTicks;
        if(!DescentTicks) { ++Wave; StartWave(); }
        return;
    }
    if(Entering)
    {
        bool Ready=true;
        for(auto& U:Units) if(U.Ally && U.Hp>0)
        {
            const int32 Goal=CameraY+1500;
            U.Position+=FMath::Clamp(Goal-U.Position,-200,200);
            U.Action=EAction::Enter; Ready &= U.Position==Goal;
            Event(19,U.Id,U.X,U.Position);
        }
        if(Ready) { Entering=false; WaveStartTick=Tick; Event(20,Wave,CameraY,0); }
        if(Tick>=6000) { Winner=3; Event(6,Winner,Wave,Tick); }
        return;
    }
    if(Tick-WaveStartTick>=60 && (Tick-WaveStartTick-60)%120==0)
        for(const auto& U:Units) if(U.Ally && U.Hp>0)
        { Hazards.Add({U.X,U.Position,Tick+24}); ++HazardsCreated; Event(10,U.X,U.Position,Tick+24); break; }
    for(auto& U:Units)
    {
        if(U.Hp<=0 || (!U.Ally && U.Generation!=Wave)) continue;
        const FUnit* Target=Find(U.Target);
        if(!Target || Target->Hp<=0)
        {
            const FUnit* Best=nullptr;
            for(const auto& V:Units) if(V.Ally!=U.Ally && V.Hp>0 && (V.Ally || V.Generation==Wave))
            {
                const int64 D=DistanceSq(U.X,U.Position,V.X,V.Position);
                const int64 BD=Best?DistanceSq(U.X,U.Position,Best->X,Best->Position):MAX_int64;
                if(D<BD || (D==BD && (!Best || V.Id<Best->Id))) Best=&V;
            }
            const int32 Id=Best?Best->Id:0;
            if(U.Target!=Id) { U.Target=Id; ++TargetSwitches; Event(2,U.Id,Id,0); }
        }
    }
    TArray<FUnit> Next=Units;
    for(int32 I=0;I<Units.Num();++I)
    {
        const FUnit& U=Units[I]; FUnit& V=Next[I];
        if(U.Hp<=0) { V.Action=EAction::Dead; V.Engaged=false; continue; }
        if(!U.Ally && U.Generation!=Wave) continue;
        const auto* T=Find(U.Target);
        const bool Evade=Dangerous(U.X,U.Position);
        bool Crowded=false;
        for(const auto& Other:Units) if(Other.Hp>0 && Other.Ally==U.Ally && Other.Id<U.Id &&
            DistanceSq(U.X,U.Position,Other.X,Other.Position)<700LL*700) Crowded=true;
        const bool CanFire=!Crowded && T && T->Hp>0 && DistanceSq(U.X,U.Position,T->X,T->Position)<=int64(AttackRange)*AttackRange && ClearRay(U.X,U.Position,T->X,T->Position);
        V.Engaged=false; V.Action=Evade?EAction::Evade:CanFire?EAction::Aim:EAction::Approach;
        if(Evade) ++EvasionSteps;
        if(T) V.Aim=TurnTowards(U.Aim,Bearing(U.X,U.Position,T->X,T->Position));
        // Finish each center-to-center edge; do not replan halfway and cut a corner.
        if(V.Waypoint>=0 && !Evade && Dangerous((V.Waypoint%GridWidth)*1000+500,(V.Waypoint/GridWidth)*1000+500))
            V.Waypoint=U.Position/1000*GridWidth+U.X/1000;
        if(V.Waypoint<0 && (Evade || !CanFire)) V.Waypoint=NextCell(U,T,Evade);
        if(V.Waypoint>=0)
        {
            const int32 X=V.Waypoint%GridWidth*1000+500,Y=V.Waypoint/GridWidth*1000+500;
            // Grid edge geometry is axis aligned. Recenter first when replanning from an edge.
            if(V.X!=X) V.X+=FMath::Clamp(X-V.X,-200,200);
            else V.Position+=FMath::Clamp(Y-V.Position,-200,200);
            if(V.X==X && V.Position==Y) V.Waypoint=-1;
        }
    }
    Units=MoveTemp(Next);
    for(auto& U:Units)
    {
        if(U.Hp<=0 || (!U.Ally && U.Generation!=Wave)) continue;
        const auto* T=Find(U.Target);
        if(!T || T->Hp<=0 || Dangerous(U.X,U.Position) ||
            DistanceSq(U.X,U.Position,T->X,T->Position)>int64(AttackRange)*AttackRange ||
            !ClearRay(U.X,U.Position,T->X,T->Position) ||
            FMath::Abs(AngleDelta(U.Aim,Bearing(U.X,U.Position,T->X,T->Position)))>AimTolerance) continue;
        U.Engaged=true; U.Action=EAction::Engage;
        if(Tick<U.NextShot) continue;
        U.NextShot=Tick+U.Interval;
        const int32 Kind=U.Ally?Choice:0,Damage=Kind==1?U.Damage*150/100:U.Damage;
        LastShots.Add({U.Id,T->Id,Damage,Kind,U.X,U.Position,T->X,T->Position}); ++Shots;
        if(Kind==2 || Kind==3)
        {
            TArray<const FUnit*> Extra;
            for(const auto& V:Units)
            {
                if(V.Hp<=0 || V.Ally==U.Ally || V.Id==T->Id || (!V.Ally && V.Generation!=Wave)) continue;
                const FUnit& Origin=Kind==2?U:*T;
                const int32 Range=Kind==2?AttackRange:1500;
                if(DistanceSq(Origin.X,Origin.Position,V.X,V.Position)>int64(Range)*Range || !ClearRay(Origin.X,Origin.Position,V.X,V.Position)) continue;
                if(Kind==2 && FMath::Abs(AngleDelta(U.Aim,Bearing(U.X,U.Position,V.X,V.Position)))>AimTolerance) continue;
                Extra.Add(&V);
            }
            Extra.Sort([&U](const FUnit& A,const FUnit& B){
                const int64 DA=DistanceSq(U.X,U.Position,A.X,A.Position),DB=DistanceSq(U.X,U.Position,B.X,B.Position);
                return DA==DB?A.Id<B.Id:DA<DB;
            });
            const int32 Count=Kind==2?FMath::Min(1,Extra.Num()):Extra.Num();
            for(int32 I=0;I<Count;++I)
            {
                const auto& V=*Extra[I]; const auto& Origin=Kind==2?U:*T;
                LastShots.Add({U.Id,V.Id,Damage/2,Kind,Origin.X,Origin.Position,V.X,V.Position});
            }
        }
    }
    LastShots.Sort([](const FShot& A,const FShot& B){return A.Source==B.Source?A.Target<B.Target:A.Source<B.Source;});
    for(const auto& S:LastShots) Event(3,S.Source,S.Target,S.Damage);
    for(auto& U:Units)
    {
        if(U.Hp<=0) continue;
        int32 Damage=0;
        for(const auto& S:LastShots) if(S.Target==U.Id) Damage+=S.Damage;
        for(const auto& H:Hazards) if(H.ImpactTick<=Tick && DistanceSq(U.X,U.Position,H.X,H.Y)<=1200LL*1200)
        { Damage+=60; ++HazardHits; Event(11,U.Id,H.X,H.Y); }
        if(Damage) { U.Hp=FMath::Max(0,U.Hp-Damage); Event(4,U.Id,0,Damage); }
        if(!U.Hp) { U.Action=EAction::Dead; U.Engaged=false; Event(5,U.Id,0,0); }
        Event(12,U.Id,U.X,U.Position); Event(13,U.Id,U.Aim,static_cast<int32>(U.Action));
    }
    Hazards.RemoveAll([this](const FHazard& H){return H.ImpactTick<=Tick;});
    bool Allies=false,Enemies=false;
    for(const auto& U:Units) if(U.Hp>0) { if(U.Ally) Allies=true; else if(U.Generation==Wave) Enemies=true; }
    if(!Allies) Winner=Enemies?2:3;
    else if(!Enemies)
    {
        ++WavesCleared; Event(14,Wave,WavesCleared,0); Hazards.Reset();
        if(Wave==3) Winner=1;
        else { DescentTicks=40; for(auto& U:Units) if(U.Hp>0) {U.Action=EAction::Descend; U.Engaged=false;} }
    }
    if(!Winner && Tick>=6000) Winner=3;
    if(Winner) Event(6,Winner,Wave,Tick);
    else if(Tick==100) Event(7,0,0,0);
}
FString FBattle::ResultJson() const
{
    const TSharedRef<FJsonObject> Root=MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("ruleVersion"),TEXT("demo-3"));
    Root->SetStringField(TEXT("fixture"),FString::Printf(TEXT("continuous-%d"),AllyCount));
    Root->SetNumberField(TEXT("seed"),Seed); Root->SetNumberField(TEXT("rngState"),Rng);
    Root->SetStringField(TEXT("winner"),Winner==1?TEXT("allies"):Winner==2?TEXT("enemies"):Winner==3?TEXT("draw"):TEXT("running"));
    Root->SetNumberField(TEXT("simTicks"),Tick); Root->SetNumberField(TEXT("shots"),Shots);
    Root->SetNumberField(TEXT("targetSwitches"),TargetSwitches);
    Root->SetNumberField(TEXT("cameraY"),CameraY); Root->SetBoolField(TEXT("entering"),Entering); Root->SetNumberField(TEXT("relocations"),Relocations);
    Root->SetNumberField(TEXT("wave"),Wave); Root->SetNumberField(TEXT("wavesCleared"),WavesCleared);
    Root->SetNumberField(TEXT("descentTicks"),DescentTicks); Root->SetNumberField(TEXT("evasionSteps"),EvasionSteps);
    Root->SetNumberField(TEXT("hazardsCreated"),HazardsCreated); Root->SetNumberField(TEXT("hazardHits"),HazardHits);
    Root->SetStringField(TEXT("eventHash"),FString::Printf(TEXT("%08x"),EventHash));
    for(const TCHAR* Key:{TEXT("energy"),TEXT("heat"),TEXT("skillCasts")}) Root->SetField(Key,MakeShared<FJsonValueNull>());
    TArray<TSharedPtr<FJsonValue>> Hp,Commands;
    for(const auto& U:Units)
    {
        auto Item=MakeShared<FJsonObject>(); Item->SetNumberField(TEXT("id"),U.Id); Item->SetNumberField(TEXT("generation"),U.Generation); Item->SetNumberField(TEXT("hp"),U.Hp);
        Item->SetNumberField(TEXT("x"),U.X); Item->SetNumberField(TEXT("y"),U.Position); Item->SetNumberField(TEXT("aim"),U.Aim);
        Hp.Add(MakeShared<FJsonValueObject>(Item));
    }
    if(Choice)
    {
        auto Cmd=MakeShared<FJsonObject>(); Cmd->SetNumberField(TEXT("tick"),ChoiceTick); Cmd->SetNumberField(TEXT("option"),Choice);
        Commands.Add(MakeShared<FJsonValueObject>(Cmd));
    }
    Root->SetArrayField(TEXT("remainingHp"),Hp); Root->SetArrayField(TEXT("commands"),Commands);
    FString Result; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Result)); return Result;
}
int32 FClock::Advance(double Delta,int32 Speed,bool Paused)
{
    if(Paused || !FMath::IsFinite(Delta) || Delta<0) return 0;
    Remainder+=Delta*FMath::Clamp(Speed,1,10); int32 Steps=0;
    while(Remainder+1.e-9>=StepSeconds && Steps<8) { Remainder-=StepSeconds; ++Steps; }
    return Steps;
}
}
