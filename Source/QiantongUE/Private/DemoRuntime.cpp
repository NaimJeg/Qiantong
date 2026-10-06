#include "DemoRuntime.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UnrealClient.h"

namespace
{
const FLinearColor Ink(0.006f, 0.010f, 0.017f), Panel(0.013f, 0.022f, 0.032f);
const FLinearColor Teal(0.25f, 0.95f, 0.8f), Coral(1.f, 0.34f, 0.28f), Gold(1.f, 0.78f, 0.36f);
const FLinearColor White(0.89f, 0.94f, 0.96f), Muted(0.48f, 0.61f, 0.66f);
ADemoDirector* FindDirector(UWorld* World)
{
    if (World) for (TActorIterator<ADemoDirector> It(World); It; ++It) return *It;
    return nullptr;
}
// Narrow, project-owned console controls for local Editor/PIE acceptance.
FAutoConsoleCommandWithWorldAndArgs DemoConsole(TEXT("qt.demo"),
    TEXT("restart | seed | pause | step | seek <tick> [choice] | finish [choice] | export | capture | rebuild | squad | choose <1..3>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        ADemoDirector* D=FindDirector(World);
        if(!D && GEngine) for(const auto& Context:GEngine->GetWorldContexts())
            if(Context.WorldType==EWorldType::PIE) { D=FindDirector(Context.World()); if(D) break; }
        if(!D || Args.IsEmpty()) return;
        const FString& A=Args[0];
        if(A==TEXT("restart")) D->Restart();
        else if(A==TEXT("seed")) D->NewSeed();
        else if(A==TEXT("pause")) D->Paused=!D->Paused;
        else if(A==TEXT("step")) D->SingleStep();
        else if(A==TEXT("rebuild")) D->RebuildViews();
        else if(A==TEXT("squad")) D->Restart(D->GetBattle().AllyCount==2?5:2);
        else if(A==TEXT("choose") && Args.Num()>1) D->Choose(FCString::Atoi(*Args[1]));
        else if(A==TEXT("seek") || A==TEXT("finish"))
        {
            const int32 End=A==TEXT("finish")?6000:Args.Num()>1?FMath::Clamp(FCString::Atoi(*Args[1]),0,6000):0;
            const int32 Option=A==TEXT("finish")?(Args.Num()>1?FCString::Atoi(*Args[1]):0):(Args.Num()>2?FCString::Atoi(*Args[2]):0);
            D->Paused=true;
            while(D->GetBattle().Tick<End && !D->GetBattle().IsOver()) { D->Choose(Option); D->SingleStep(); }
        }
        else if(A==TEXT("capture")) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/ExplorePIE.png"),false,false);
        D->ExportResult();
        UE_LOG(LogTemp,Display,TEXT("QIANTONG_CONSOLE: %s tick=%d wave=%d hash=%08x"),*A,D->GetBattle().Tick,D->GetBattle().Wave,D->GetBattle().EventHash);
    }));
FVector WorldPosition(FVector2D P) { return FVector(640 - P.Y, P.X - 360, 10); }
}
ADemoUnitView::ADemoUnitView()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
    RootComponent = Mesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCastShadow(false);
}
ADemoDirector::ADemoDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeLocation(FVector(0, 0, 1500)); Camera->SetRelativeRotation(FRotator(-90, 0, 0));
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic; Camera->OrthoWidth = 720;
    Camera->AspectRatio = 720.f / 1280.f; Camera->bConstrainAspectRatio = true;
    Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    Camera->PostProcessSettings.AutoExposureBias = 0;
    Deck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShaftDeck")); Deck->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Deck->SetStaticMesh(Cube.Object); Deck->SetRelativeScale3D(FVector(12.8, 7.2, .05));
    Deck->SetCollisionEnabled(ECollisionEnabled::NoCollision); Deck->SetCastShadow(false);
    UnitViewClass = ADemoUnitView::StaticClass();
}
void ADemoDirector::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (StageMaterial) Deck->SetMaterial(0, StageMaterial);
}
void ADemoDirector::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(this);
    SmokeTest = FParse::Param(FCommandLine::Get(), TEXT("DemoSmoke"));
    FParse::Value(FCommandLine::Get(), TEXT("DemoOption="), SmokeOption);
    FParse::Value(FCommandLine::Get(), TEXT("DemoCaptureTick="), CaptureTicks);
    // Legacy decorative crossbars were screen-fixed. Continuous grid below replaces them.
    for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It) It->SetActorHiddenInGame(true);
    Restart(2);
    UE_LOG(LogTemp, Display, TEXT("QIANTONG_DEMO_READY: native logic, asset-only Blueprints"));
}
FVector2D ADemoDirector::ScreenPosition(const QiantongDemo::FUnit& U) const
{
    return MapPosition(U.X,U.Position);
}
FVector2D ADemoDirector::MapPosition(int32 X,int32 Y) const
{
    return FVector2D(205+X*.05,235+(Y-RenderCameraY)*.05);
}
void ADemoDirector::Restart(int32 Allies)
{
    Battle.Reset(Allies == 0 ? Battle.AllyCount : Allies,Battle.Seed); Clock = {}; RenderCameraY=0; Traces.Reset(); PendingChoice = 0;
    Paused = false; Notice = TEXT("Auto aim / evade / descend. Choose a protocol at 5s."); RebuildViews();
}
void ADemoDirector::NewSeed()
{
    const uint32 Seed=Battle.Seed+1;
    Battle.Reset(Battle.AllyCount,Seed); Clock={}; RenderCameraY=0; Traces.Reset(); PendingChoice=0;
    Paused=false; RebuildViews(); Notice=TEXT("New shaft layout. R replays this seed.");
}
void ADemoDirector::Choose(int32 Option) { if (Battle.CanChoose() && PendingChoice == 0 && Option >= 1 && Option <= 3) PendingChoice = Option; }
void ADemoDirector::RebuildViews()
{
    for (ADemoUnitView* V : Views) if (IsValid(V)) { V->Destroy(); ++ViewsDestroyed; }
    Views.Reset();
    for (const QiantongDemo::FUnit& U : Battle.Units)
    {
        ADemoUnitView* V = GetWorld()->SpawnActor<ADemoUnitView>(UnitViewClass);
        if (V)
        {
            ++ViewsCreated; V->Tags.Add(FName(*FString::Printf(TEXT("UnitId_%d"),U.Id)));
            V->SetActorScale3D(U.Ally ? FVector(.22, .22, .16) : FVector(.18, .18, .12));
            if (UMaterialInterface* M = U.Ally ? AllyMaterial.Get() : EnemyMaterial.Get()) V->Mesh->SetMaterial(0, M);
        }
        Views.Add(V);
    }
    SyncViews();
}
void ADemoDirector::SyncViews()
{
    // Keep the orthographic view target beside its camera as the viewport descends.
    // Auto orthographic planes use the owning actor as their reference point.
    SetActorLocation(FVector(-RenderCameraY*.05,0,0));
    Camera->SetRelativeLocation(FVector(0,0,1500));
    Deck->SetRelativeLocation(FVector::ZeroVector);
    for (int32 I = 0; I < Views.Num(); ++I) if (IsValid(Views[I]))
    {
        Views[I]->SetActorHiddenInGame(Battle.Units[I].Hp <= 0 || ScreenPosition(Battle.Units[I]).Y < 219 || ScreenPosition(Battle.Units[I]).Y > 901);
        Views[I]->SetActorLocation(WorldPosition(FVector2D(205+Battle.Units[I].X*.05,235+Battle.Units[I].Position*.05)));
        Views[I]->SetActorRotation(FRotator(0, Battle.Units[I].Aim / 1000.f + 90, 0));
    }
}
void ADemoDirector::Step()
{
    if (Battle.IsOver()) return;
    if (SmokeTest && Battle.CanChoose() && SmokeOption > 0) PendingChoice = SmokeOption;
    const int32 PreviousWave=Battle.Wave;
    Battle.Step(PendingChoice); PendingChoice = 0;
    if (PreviousWave!=Battle.Wave) { Traces.Reset();
        UE_LOG(LogTemp,Display,TEXT("QIANTONG_WAVE: %d seed=%u"),Battle.Wave,Battle.Seed); }
    for (const QiantongDemo::FShot& S : Battle.LastShots)
    {
        const auto* A = Battle.Find(S.Source); const auto* B = Battle.Find(S.Target);
        if (A && B)
        {
            Traces.Add({FVector2D(S.StartX,S.StartY),FVector2D(S.EndX,S.EndY),.18f,A->Ally,S.Kind});
        }
    }
    if (Battle.Choice && Battle.ChoiceTick == Battle.Tick) Notice = TEXT("Protocol installed. Watch your squad's next volley.");
    SyncViews();
    // Normal play never rebuilds actors at a wave boundary.
    if (CaptureTicks > 0 && Battle.Tick >= CaptureTicks)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/DemoCombat.png"), false, false);
        CaptureTicks = 0;
    }
    if (Battle.IsOver())
    {
        ExportResult();
        UE_LOG(LogTemp, Display, TEXT("QIANTONG_DEMO_END: tick=%d winner=%d hash=%08x"), Battle.Tick, Battle.Winner, Battle.EventHash);
    }
}
void ADemoDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (FDemoTrace& T : Traces) if(!Paused) T.Life -= DeltaSeconds;
    Traces.RemoveAll([](const FDemoTrace& T) { return T.Life <= 0; });
    const int32 Steps = Clock.Advance(DeltaSeconds, Speed, Paused || Battle.IsOver());
    for (int32 I = 0; I < Steps; ++I) Step();
    RenderCameraY=FMath::FInterpConstantTo(RenderCameraY,double(Battle.CameraY),double(DeltaSeconds),13000.0*Speed);
    SyncViews();
    if (SmokeTest && Battle.IsOver())
    {
        if (SmokePhase == 0) { SmokePhase = 1; Restart(5); Speed = 10; }
        else { UE_LOG(LogTemp, Display, TEXT("QIANTONG_DEMO_SMOKE_PASS")); FPlatformMisc::RequestExit(false); }
    }
}
void ADemoDirector::SingleStep() { Paused = true; Traces.Reset(); Step(); RenderCameraY=Battle.CameraY; SyncViews(); }
void ADemoDirector::CycleSpeed() { Speed = Speed == 1 ? 2 : Speed == 2 ? 5 : Speed == 5 ? 10 : 1; }
bool ADemoDirector::ExportResult()
{
    const FString Folder = FPaths::ProjectSavedDir() / TEXT("DemoResults"); IFileManager::Get().MakeDirectory(*Folder, true);
    const bool Ok = FFileHelper::SaveStringToFile(Battle.ResultJson(), *(Folder / FString::Printf(TEXT("continuous-%d-seed%u-option%d.json"), Battle.AllyCount, Battle.Seed, Battle.Choice)));
    auto Audit=MakeShared<FJsonObject>(); Audit->SetNumberField(TEXT("tick"),Battle.Tick);
    Audit->SetNumberField(TEXT("wave"),Battle.Wave); Audit->SetNumberField(TEXT("cameraY"),Battle.CameraY);
    Audit->SetNumberField(TEXT("created"),ViewsCreated); Audit->SetNumberField(TEXT("destroyed"),ViewsDestroyed);
    TArray<TSharedPtr<FJsonValue>> Identities;
    for(int32 I=0;I<Views.Num();++I) if(IsValid(Views[I]))
    {
        auto Item=MakeShared<FJsonObject>(); Item->SetNumberField(TEXT("unitId"),Battle.Units[I].Id);
        Item->SetStringField(TEXT("actor"),Views[I]->GetPathName());
        Item->SetNumberField(TEXT("objectId"),Views[I]->GetUniqueID()); Identities.Add(MakeShared<FJsonValueObject>(Item));
    }
    Audit->SetArrayField(TEXT("actors"),Identities);
    FString AuditJson; FJsonSerializer::Serialize(Audit,TJsonWriterFactory<>::Create(&AuditJson));
    FFileHelper::SaveStringToFile(AuditJson,*(Folder/TEXT("continuous-view-lifecycle.json")));
    Notice = Ok ? TEXT("Result saved to Saved / DemoResults") : TEXT("Could not save result. Check write permissions.");
    return Ok;
}
ADemoController::ADemoController() { bShowMouseCursor = true; bEnableClickEvents = true; bEnableMouseOverEvents = true; }
void ADemoController::BeginPlay()
{
    Super::BeginPlay(); FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
}
void ADemoController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::N, IE_Pressed, this, &ADemoController::NewSeed);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ADemoController::Restart);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ADemoController::Pause);
    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ADemoController::Speed);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ADemoController::One);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ADemoController::Two);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ADemoController::Three);
    InputComponent->BindKey(EKeys::Period, IE_Pressed, this, &ADemoController::Step);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ADemoController::Quit);
}
void ADemoController::Action(FName Name)
{
    ADemoDirector* D = FindDirector(GetWorld()); if (!D) return;
    UE_LOG(LogTemp, Display, TEXT("QIANTONG_DEMO_ACTION: %s tick=%d"), *Name.ToString(), D->GetBattle().Tick);
    if (Name == TEXT("restart") || Name == TEXT("deploy")) D->Restart();
    else if (Name == TEXT("seed")) D->NewSeed();
    else if (Name == TEXT("pause")) D->Paused = !D->Paused;
    else if (Name == TEXT("speed")) D->CycleSpeed();
    else if (Name == TEXT("step")) D->SingleStep();
    else if (Name == TEXT("squad")) D->Restart(D->GetBattle().AllyCount == 2 ? 5 : 2);
    else if (Name == TEXT("export")) D->ExportResult();
    else if (Name == TEXT("rebuild")) D->RebuildViews();
    else if (Name == TEXT("choice1")) D->Choose(1);
    else if (Name == TEXT("choice2")) D->Choose(2);
    else if (Name == TEXT("choice3")) D->Choose(3);
}
void ADemoController::NewSeed() { Action(TEXT("seed")); }
void ADemoController::Restart() { Action(TEXT("restart")); }
void ADemoController::Pause() { Action(TEXT("pause")); }
void ADemoController::Speed() { Action(TEXT("speed")); }
void ADemoController::One() { Action(TEXT("choice1")); }
void ADemoController::Two() { Action(TEXT("choice2")); }
void ADemoController::Three() { Action(TEXT("choice3")); }
void ADemoController::Step() { Action(TEXT("step")); }
void ADemoController::Quit() { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); }
ADemoGameMode::ADemoGameMode() { DefaultPawnClass = nullptr; PlayerControllerClass = ADemoController::StaticClass(); HUDClass = ADemoHUD::StaticClass(); }
ADemoDirector* ADemoHUD::Director() const { return FindDirector(GetWorld()); }
void ADemoHUD::Rect(float X, float Y, float W, float H, FLinearColor Color) { DrawRect(Color, OffsetX + X * Scale, OffsetY + Y * Scale, W * Scale, H * Scale); }
void ADemoHUD::Text(const FString& Value, float X, float Y, float Size, FLinearColor Color) { DrawText(Value, Color, OffsetX + X * Scale, OffsetY + Y * Scale, GEngine->GetMediumFont(), Size * Scale / 16.f, false); }
void ADemoHUD::Line(FVector2D A, FVector2D B, FLinearColor Color, float Width) { DrawLine(OffsetX + A.X * Scale, OffsetY + A.Y * Scale, OffsetX + B.X * Scale, OffsetY + B.Y * Scale, Color, Width * Scale); }
void ADemoHUD::Button(FName Name, const FString& Label, float X, float Y, float W, bool Active)
{
    Rect(X, Y, W, 44, Active ? FLinearColor(.13f,.25f,.27f) : Panel);
    Text(Label, X + 12, Y + 12, 15, Active ? Teal : White);
    AddHitBox(FVector2D(OffsetX + X * Scale, OffsetY + Y * Scale), FVector2D(W * Scale, 44 * Scale), Name, true);
}
void ADemoHUD::NotifyHitBoxClick(FName Name) { if (ADemoController* PC = Cast<ADemoController>(GetOwningPlayerController())) PC->Action(Name); }
void ADemoHUD::DrawHUD()
{
    Super::DrawHUD(); if (!Canvas) return;
    Scale = FMath::Min(Canvas->SizeX / 720.f, Canvas->SizeY / 1280.f);
    OffsetX = (Canvas->SizeX - 720 * Scale) * .5f; OffsetY = (Canvas->SizeY - 1280 * Scale) * .5f;
    ADemoDirector* D = Director();
    if (!D) { Text(TEXT("Demo director missing from level"), 30, 100, 24, Coral); return; }
    const auto& B = D->GetBattle();
    Rect(0, 0, 720, 235, Ink); Rect(0, 235, 205, 650, Ink); Rect(655,235,65,650,Ink); Rect(0, 885, 720, 395, Ink);
    Text(TEXT("Q I A N T O N G"), 28, 28, 28, White);
    Text(TEXT("ABYSS / EXPLORATION PROTOTYPE"), 30, 69, 13, Teal);
    Text(FString::Printf(TEXT("SECTOR %02d/03   %05.1fs"), B.Wave, B.Tick * .05), 439, 36, 16, Muted);
    Rect(28, 111, 664, 2, Teal);
    Text(TEXT("MOTHERSHIP  /  DESCENT CONTROL"), 205, 133, 17, Muted);
    Rect(178, 177, 487, 9, Panel); Rect(290, 173, 265, 16, Teal * .5f);
    Text(TEXT("SQUAD"), 24, 228, 17, Teal);
    int32 AlliesAlive = 0, EnemiesAlive = 0;
    for (const auto& U : B.Units)
    {
        if (U.Hp > 0) { if (U.Ally) ++AlliesAlive; else if(U.Generation==B.Wave) ++EnemiesAlive; }
        if (U.Ally)
        {
            const float Y = 273 + (U.Id - 1) * 116;
            Rect(16, Y, 120, 99, Panel);
            Text(FString::Printf(TEXT("QT / %02d"), U.Id), 26, Y + 12, 17, U.Hp > 0 ? White : Muted);
            Text(U.Hp <= 0 ? TEXT("OFFLINE") : U.Action == QiantongDemo::EAction::Evade ? TEXT("EVADE") : U.Action == QiantongDemo::EAction::Descend ? TEXT("DESCEND") : U.Action == QiantongDemo::EAction::Enter ? TEXT("ENTER") : U.Engaged ? TEXT("FIRING") : U.Action == QiantongDemo::EAction::Aim ? TEXT("AIMING") : TEXT("REPOSITION"), 26, Y + 39, 12, U.Hp > 0 ? Teal : Coral);
            Rect(26, Y + 68, 100, 5, Ink); Rect(26, Y + 68, 100.f * U.Hp / U.MaxHp, 5, Teal);
            Text(FString::Printf(TEXT("%d / %d"), U.Hp, U.MaxHp), 26, Y + 80, 11, Muted);
        }
    }
    // Grid and obstacles visualize authoritative logical state, never Actor collision.
    for(int32 X=0;X<=QiantongDemo::GridWidth;++X)
        Line({D->MapPosition(X*1000,0).X,235},{D->MapPosition(X*1000,0).X,885},FLinearColor(.08f,.13f,.15f));
    const int32 FirstRow=FMath::FloorToInt(D->GetCameraDepth()/1000.0);
    for(int32 Y=FirstRow;Y<=FirstRow+QiantongDemo::GridHeight+1;++Y)
    {
        const float Row=D->MapPosition(0,Y*1000).Y;
        if(Row>=235 && Row<=885) Line({205,Row},{655,Row},FLinearColor(.08f,.13f,.15f));
    }
    for(int32 Cell:B.Obstacles)
    {
        const auto P=D->MapPosition(Cell%QiantongDemo::GridWidth*1000,Cell/QiantongDemo::GridWidth*1000);
        const float Top=FMath::Max(235.f,float(P.Y)+1),Bottom=FMath::Min(885.f,float(P.Y)+49);
        if(Top>=Bottom) continue;
        Rect(P.X+1,Top,48,Bottom-Top,FLinearColor(.13f,.18f,.20f));
        if(P.Y>=235 && P.Y+50<=885) {
            Rect(P.X+4,P.Y+4,42,3,Muted);
            Line(P+FVector2D(10,14),P+FVector2D(36,40),Muted*.55f,2);
        }
    }
    for(const auto& H:B.Hazards)
    {
        const auto P=D->MapPosition(H.X,H.Y);
        const float Radius=60;
        for(int32 I=0;I<32;++I)
        {
            const double A=I*PI/16,A2=(I+1)*PI/16;
            Line(P+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius,
                 P+FVector2D(FMath::Cos(A2),FMath::Sin(A2))*Radius,Coral,3);
        }
        Line(P-FVector2D(15,15),P+FVector2D(15,15),Coral,2);
        Line(P+FVector2D(15,-15),P+FVector2D(-15,15),Coral,2);
        Text(FString::Printf(TEXT("%.1fs"),(H.ImpactTick-B.Tick)*.05),P.X-16,P.Y-9,13,Coral);
    }
    if (D->ShowTargetLines) for(const auto& U:B.Units) if(U.Hp>0)
    {
        const auto P=D->ScreenPosition(U);
        if(P.Y<250 || P.Y>865) continue;
        const double A=U.Aim*PI/180000.;
        const FVector2D Direction(FMath::Cos(A),FMath::Sin(A));
        Line(P,P+Direction*30,U.Ally?Teal:Coral,3);
        Line(P+Direction*30,P+Direction*37,White,2);
    }
    for (const auto& T : D->Traces)
    {
        const FLinearColor C = !T.Ally ? Coral : T.Kind == 1 ? Gold : T.Kind == 3 ? FLinearColor(.73f,.5f,1) : Teal;
        const float Alpha = FMath::Clamp(1 - T.Life / .18f, 0.f, 1.f);
        const FVector2D Start=D->MapPosition(T.Start.X,T.Start.Y),End=D->MapPosition(T.End.X,T.End.Y);
        const FVector2D Tip = End;
        Line(Start, Tip, C * (1-Alpha), T.Kind == 1 ? 6 : 4);
        Line(Start, Tip, White * (1-Alpha), 1);
        Rect(Tip.X - 3, Tip.Y - 3, 6, 6, C);
        if (T.Kind == 3 && Alpha > .65f)
            for (int32 I = 0; I < 16; ++I)
            { const double A = I * PI / 8, A2 = (I+1)*PI/8; const float R = 10 + Alpha * 26;
              Line(End + FVector2D(FMath::Cos(A),FMath::Sin(A))*R, End + FVector2D(FMath::Cos(A2),FMath::Sin(A2))*R, C, 2); }
    }
    for (const auto& U : B.Units) if (U.Hp > 0)
    {
        const FVector2D P = D->ScreenPosition(U); if(P.Y<250 || P.Y>865) continue; const FLinearColor C = U.Ally ? Teal : Coral;
        // Outlines remain readable even while the viewport compiles materials.
        Rect(P.X - 15, P.Y - 20, 30, 4, Panel); Rect(P.X - 15, P.Y - 20, 30.f * U.Hp / U.MaxHp, 4, C);
        Text(FString::Printf(TEXT("%02d"), U.Id), P.X - 12, P.Y + 15, 11, C);
    }
    if(B.DescentTicks>0 || B.Entering) Text(B.Entering?TEXT("SQUAD ENTERING / ENEMIES IN COVER"):TEXT("SECTOR CLEAR / CAMERA DESCENDING"),220,935,13,Teal);
    Text(FString::Printf(TEXT("DEPTH %dm   SEED %u"),B.CameraY/100,B.Seed),220,211,14,Teal);
    Text(FString::Printf(TEXT("%d ALLIES    /    %d HOSTILES"), AlliesAlive, EnemiesAlive), 218, 905, 17, Muted);
    if (B.IsOver())
    {
        Rect(189, 430, 472, 188, Ink); Rect(189, 430, 4, 188, B.Winner == 1 ? Teal : Coral);
        Text(B.Winner == 1 ? TEXT("EXPEDITION CLEAR") : B.Winner == 2 ? TEXT("SQUAD LOST") : TEXT("STALEMATE"), 218, 460, 32, B.Winner == 1 ? Teal : Coral);
        Text(FString::Printf(TEXT("%.1fs  /  %d volleys"), B.Tick * .05, B.Shots), 219, 515, 18, White);
        Button(TEXT("deploy"),TEXT("DEPLOY AGAIN  [R]"),219,554,300,true);
    }
    Text(B.CanChoose() ? TEXT("INSTALL A PROTOCOL  /  COMBAT CONTINUES") : B.Choice ? TEXT("PROTOCOL INSTALLED") : TEXT("PROTOCOL UPLINK  /  AVAILABLE AT 5s"), 28, 972, 17, B.CanChoose() ? Gold : Muted);
    const TCHAR* Names[] = {TEXT("1  OVERCHARGE"),TEXT("2  TWIN SHOT"),TEXT("3  SHOCKWAVE")};
    const TCHAR* Hints[] = {TEXT("+50% direct damage"),TEXT("Extra target / 50%"),TEXT("Area damage / 50%")};
    for (int32 I = 0; I < 3; ++I)
    {
        const float X = 28 + I * 225;
        Rect(X, 1010, 214, 94, Panel);
        const FLinearColor C = B.Choice == I + 1 ? Teal : B.CanChoose() ? Gold : Muted;
        Rect(X, 1010, 214, 3, C); Text(Names[I],X+12,1029,17,C); Text(Hints[I],X+12,1066,12,Muted);
        if (B.CanChoose()) AddHitBox({OffsetX+X*Scale,OffsetY+1010*Scale},{214*Scale,94*Scale},FName(*FString::Printf(TEXT("choice%d"),I+1)),true);
    }
    Button(TEXT("pause"),D->Paused ? TEXT("RESUME") : TEXT("PAUSE"),28,1122,100,D->Paused);
    Button(TEXT("speed"),FString::Printf(TEXT("%dx SPEED"),D->Speed),138,1122,107);
    Button(TEXT("step"),TEXT("STEP"),255,1122,82);
    Button(TEXT("squad"),FString::Printf(TEXT("%d UNITS"), B.AllyCount),347,1122,102);
    Button(TEXT("restart"),TEXT("RESTART"),459,1122,111);
    Button(TEXT("export"),TEXT("EXPORT"),580,1122,112);
    Text(D->Notice,28,1187,12,Muted);
    Button(TEXT("seed"),TEXT("NEW SEED [N]"),540,1180,152);
    Text(TEXT("SPACE pause   TAB speed   1/2/3 select   R restart   ESC exit"),28,1223,11,Muted);
    Text(TEXT("3 SECTORS / INSTANT LASER / 180 DEG PER SECOND"),28,1253,10,Teal);
}
