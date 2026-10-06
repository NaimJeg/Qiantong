#include "Misc/AutomationTest.h"
#include "DemoBattle.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
using namespace QiantongDemo;
namespace
{
FBattle Duel()
{
    FBattle B; B.Reset(2); B.Obstacles.Reset(); B.Units.SetNum(2);
    B.Wave = 3; B.Entering=false;
    B.Units[0].X = 1500; B.Units[0].Position = 2500; B.Units[0].Aim = 180000;
    B.Units[1].Id = 101; B.Units[1].Ally = false; B.Units[1].Generation=3;
    B.Units[1].X = 3500; B.Units[1].Position = 2500; B.Units[1].Aim = 180000;
    return B;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoSpatialTest, "Qiantong.Demo.SpaceAndAim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoSpatialTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Wrap clockwise shortest arc"), TurnTowards(359000, 1000), 1000);
    TestEqual(TEXT("Wrap counterclockwise shortest arc"), TurnTowards(1000, 359000), 359000);
    TestEqual(TEXT("Turn capped at 9 degrees per tick"), TurnTowards(0, 180000), 9000);
    auto B = Duel(); B.Units[1].Damage = 0;
    B.Step(); TestEqual(TEXT("Cannot shoot backwards"), B.Shots, 1); // only aligned enemy fires
    TestEqual(TEXT("Rotation limited"), B.Units[0].Aim, 189000);
    const int32 Hp = B.Units[1].Hp;
    for (int32 I=0; I<19; ++I) B.Step();
    TestTrue(TEXT("Aligned laser damages immediately"), B.Units[1].Hp < Hp);
    B = Duel(); B.Obstacles.Add(2 + 2 * GridWidth);
    TestFalse(TEXT("Wall occludes ray"), B.ClearRay(1500,2500,3500,2500));
    TestFalse(TEXT("Ray cannot graze wall corner"), B.ClearRay(1500,1500,3500,3500));
    TestTrue(TEXT("Parallel clear ray"), B.ClearRay(1500,1500,3500,1500));
    B.Units[0].Aim = 0; B.Units[1].Damage = 0;
    bool Routed = false, Fired = false;
    for (int32 I=0; I<100 && !B.IsOver(); ++I)
    {
        B.Step(); Routed |= B.Units[0].Position != 2500;
        for (const auto& U : B.Units) TestFalse(TEXT("No wall penetration"), B.Blocked(U.X/1000,U.Position/1000));
        for (const auto& S : B.LastShots)
        {
            TestTrue(TEXT("Every direct ray unobstructed"), B.ClearRay(S.StartX,S.StartY,S.EndX,S.EndY));
            if (S.Source == 1) Fired = true;
        }
    }
    TestTrue(TEXT("Routes around obstruction"), Routed);
    TestTrue(TEXT("Finds a firing position"), Fired);
    B = Duel(); for (auto& U : B.Units) { U.Hp=10; U.Damage=20; }
    B.Units[0].Aim=0; B.Step(); TestEqual(TEXT("Simultaneous mutual kill"), B.Winner, 3);
    for(int32 Option:{1,2,3})
    {
        B=Duel(); B.Tick=100; B.Units[0].Aim=0; B.Units[1].Damage=0;
        FUnit Extra=B.Units[1]; Extra.Id=102; Extra.X=4500; B.Units.Add(Extra); // Separate firing cells keep this fixture stationary.
        B.Step(Option);
        TestEqual(TEXT("Primary protocol damage"),B.Units[1].Hp,Option==1?555:570);
        TestEqual(TEXT("Secondary cone / blast damage"),B.Units[2].Hp,Option==1?600:585);
    }
    B=Duel(); B.Tick=100; B.Units[0].Aim=0; B.Units[1].Damage=0;
    FUnit Outside=B.Units[1]; Outside.Id=102; Outside.Position=3500; B.Units.Add(Outside);
    B.Step(2); TestEqual(TEXT("Twin laser cannot bypass turn cone"),B.Units[2].Hp,600);
    B = Duel(); B.Tick = 5999; for (auto& U : B.Units) U.Damage=0;
    B.Step(); TestEqual(TEXT("Timeout is draw"), B.Winner, 3);
    B.Reset(2); B.Tick=5999; B.DescentTicks=40; B.Step();
    TestEqual(TEXT("Timeout also applies during descent"),B.Winner,3);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoExploreTest, "Qiantong.Demo.Exploration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoExploreTest::RunTest(const FString& Parameters)
{
    auto B = Duel(); B.Units[0].Aim=0;
    B.Hazards.Add({B.Units[0].X,B.Units[0].Position,24});
    const int32 Hp=B.Units[0].Hp; B.Units[1].Damage=0; B.Units[0].Damage=0;
    for (int32 I=0; I<25; ++I) B.Step();
    TestEqual(TEXT("Evades telegraphed danger before detonation"), B.Units[0].Hp, Hp);
    TestTrue(TEXT("Evasion recorded"), B.EvasionSteps > 0);
    B = Duel(); B.Hazards.Add({B.Units[0].X,B.Units[0].Position,1});
    B.Units[1].Damage=0; B.Step();
    TestEqual(TEXT("Due hazard deals authoritative damage"), B.Units[0].Hp, 540);
    B.Reset(2); while(B.Entering) B.Step(); B.Tick=100; B.Step(1); B.Step(2);
    TestEqual(TEXT("Choice applied once next tick"), B.ChoiceTick,101);
    TestEqual(TEXT("Repeated choice rejected"), B.Choice,1);
    B.Units[0].Hp=321; B.Units[1].Hp=0;
    for (auto& U:B.Units) if (!U.Ally) U.Hp=0;
    B.Step(); TestTrue(TEXT("Clear starts descent, not final win"), B.DescentTicks>0 && !B.IsOver());
    for (int32 I=0; I<40; ++I) B.Step();
    TestEqual(TEXT("Second wave spawns"), B.Wave,2);
    TestEqual(TEXT("HP carried across sectors"), B.Units[0].Hp,321);
    TestEqual(TEXT("Dead allies not resurrected"), B.Units[1].Hp,0);
    TestEqual(TEXT("Choice persists"), B.Choice,1);
    FBattle SeedA,SeedB; SeedA.Reset(2,1); SeedB.Reset(2,2);
    TestTrue(TEXT("Different seeds change layout"),SeedA.Obstacles!=SeedB.Obstacles);
    for (uint32 Seed=1; Seed<=24; ++Seed)
    {
        FBattle A, Copy; A.Reset(2,Seed); Copy.Reset(2,Seed);
        TestTrue(TEXT("Generated map is connected"), A.MapConnected());
        TestTrue(TEXT("Seed replays exact obstacles"), A.Obstacles==Copy.Obstacles);
        TestTrue(TEXT("Map has obstacles"), A.Obstacles.Num()>=6);
        while(!A.IsOver())
        {
            const auto Before=A.Units; const int32 OldWave=A.Wave;
            A.Step(A.CanChoose()?3:0); Copy.Step(Copy.CanChoose()?3:0);
            if(A.Wave==OldWave) for(int32 I=0;I<A.Units.Num();++I)
                TestTrue(TEXT("Per-step turn bound for every unit"),FMath::Abs(AngleDelta(Before[I].Aim,A.Units[I].Aim))<=MaxTurn);
            for(const auto& Shot:A.LastShots)
                TestTrue(TEXT("All generated rays respect walls"),A.ClearRay(Shot.StartX,Shot.StartY,Shot.EndX,Shot.EndY));
            for(const auto& U:A.Units) if(U.Hp>0 && !A.Entering)
                TestFalse(TEXT("Every live position free"),A.Blocked(U.X/1000,U.Position/1000));
            TestTrue(TEXT("Wave remains connected"), A.MapConnected());
        }
        TestEqual(TEXT("Seed command replay"), A.ResultJson(),Copy.ResultJson());
        TestEqual(TEXT("All three sectors converge to victory"),A.Winner,1);
        TestEqual(TEXT("Three waves complete"),A.WavesCleared,3);
        TestTrue(TEXT("Threats and avoidance exercised"),A.HazardsCreated>0 && A.EvasionSteps>0);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoContinuityTest, "Qiantong.Demo.Continuity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoContinuityTest::RunTest(const FString& Parameters)
{
    FBattle B; B.Reset(2);
    TestEqual(TEXT("All three enemy waves allocated before play"),B.Units.Num(),23);
    TestTrue(TEXT("First wave enters from outside"),B.Entering);
    for(const auto& U:B.Units) if(U.Ally) TestTrue(TEXT("Friends initially wholly outside"),B.Offscreen(U.Position));
    while(B.Entering) B.Step();
    B.Units[0].Hp=321;
    B.Units[0].X=2700; // Preserve exact sub-cell X, not just lane.
    for(auto& U:B.Units) if(!U.Ally && U.Generation==1) U.Hp=0;
    B.Step();
    const auto OldUnits=B.Units; const auto OldMap=B.Obstacles;
    const int32 Size=B.Units.Num();
    int32 LastCamera=B.CameraY;
    while(B.DescentTicks)
    {
        const auto Before=B.Units; const int32 OldCamera=B.CameraY;
        B.Step();
        TestTrue(TEXT("Camera advances monotonically"),B.CameraY>LastCamera);
        TestTrue(TEXT("Camera bounded speed"),B.CameraY-LastCamera<=650);
        for(int32 I=0;I<Size;++I)
        {
            TestEqual(TEXT("No slot replacement"),B.Units[I].Id,OldUnits[I].Id);
            if(B.Units[I].Ally) TestEqual(TEXT("Ally X unchanged across descent"),B.Units[I].X,OldUnits[I].X);
            if(FMath::Abs(B.Units[I].Position-Before[I].Position)>200)
            {
                TestTrue(TEXT("Relocation origin outside old view"),Before[I].Position+800<OldCamera || Before[I].Position-800>OldCamera+13000);
                TestTrue(TEXT("Relocation destination outside new view"),B.Offscreen(B.Units[I].Position));
                TestEqual(TEXT("Relocate only after camera stops"),B.DescentTicks,0);
            }
        }
        for(int32 Cell:OldMap) TestTrue(TEXT("Existing obstacles persist"),B.Obstacles.Contains(Cell));
        LastCamera=B.CameraY;
    }
    TestEqual(TEXT("Camera never snaps back at wave boundary"),B.CameraY,26000);
    TestEqual(TEXT("Ally HP retained"),B.Units[0].Hp,321);
    TestEqual(TEXT("Pool size unchanged"),B.Units.Num(),Size);
    TestTrue(TEXT("Enter after stopped camera"),B.Entering);
    for(const auto& U:B.Units) if(U.Ally && U.Hp>0) TestTrue(TEXT("Friends start entering offscreen"),B.Offscreen(U.Position));
    TestTrue(TEXT("Next enemies already moved to cover before camera arrives"),B.Find(201)->Position!=10500+WaveStride);
    while(B.Entering)
    {
        const auto Before=B.Units; B.Step();
        TestEqual(TEXT("Camera stationary throughout entry"),B.CameraY,26000);
        for(int32 I=0;I<Size;++I)
        {
            if(B.Units[I].Ally) TestEqual(TEXT("Friendly entry preserves screen X"),B.Units[I].X,Before[I].X);
            TestTrue(TEXT("Walks in, never pops"),FMath::Abs(B.Units[I].Position-Before[I].Position)<=200);
        }
    }
    TestEqual(TEXT("Preplaced enemy generation preserved"),B.Find(201)->Generation,2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoClockTest, "Qiantong.Demo.Clock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoClockTest::RunTest(const FString& Parameters)
{
    uint32 Expected=0;
    for(int32 Fps:{30,60,120})
    {
        FBattle B; B.Reset(2); FClock C;
        for(int32 Frame=0;Frame<Fps*310 && !B.IsOver();++Frame)
            for(int32 N=C.Advance(1.0/Fps,1,false);N>0;--N) B.Step();
        TestTrue(TEXT("Frame scenario ends"),B.IsOver());
        if(!Expected) Expected=B.EventHash;
        TestEqual(TEXT("FPS invariant"),B.EventHash,Expected);
    }
    for(int32 Speed:{2,5,10})
    {
        FBattle B; B.Reset(2); FClock C;
        for(int32 Frame=0;Frame<30000 && !B.IsOver();++Frame)
            for(int32 N=C.Advance(1.0/60,Speed,Frame%3==0);N>0;--N) B.Step();
        TestEqual(TEXT("Pause and speed invariant"),B.EventHash,Expected);
    }
    FClock C;
    TestEqual(TEXT("Catchup cap"),C.Advance(1,1,false),8);
    TestEqual(TEXT("Remainder retained"),C.Advance(0,1,false),8);
    TestEqual(TEXT("Remainder drained"),C.Advance(0,1,false),4);
    TestEqual(TEXT("Paused time ignored"),C.Advance(20,1,true),0);
    TestEqual(TEXT("No pause catchup"),C.Advance(0,1,false),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemoGoldenTest, "Qiantong.Demo.Golden",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemoGoldenTest::RunTest(const FString& Parameters)
{
    for(int32 Allies:{2,5}) for(int32 Option:{0,1,2,3})
    {
        FBattle B; B.Reset(Allies);
        while(!B.IsOver()) B.Step(B.CanChoose()?Option:0);
        TestEqual(TEXT("Default scenario wins"),B.Winner,1);
        TestEqual(TEXT("Full descent"),B.WavesCleared,3);
        const FString Name=FString::Printf(TEXT("continuous-%d-option%d.json"),Allies,Option);
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("DemoGolden");
        IFileManager::Get().MakeDirectory(*Folder,true);
        FFileHelper::SaveStringToFile(B.ResultJson(),*(Folder/Name));
        FString Expected;
        if(FFileHelper::LoadFileToString(Expected,*(FPaths::ProjectDir()/TEXT("Tests/Golden")/Name)))
            TestEqual(TEXT("Reviewed Golden exact JSON"),B.ResultJson(),Expected);
        else AddError(TEXT("Missing reviewed Golden: ")+Name);
    }
    return true;
}
#endif
