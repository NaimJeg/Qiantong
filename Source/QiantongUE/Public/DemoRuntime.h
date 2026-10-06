#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "DemoBattle.h"
#include "DemoRuntime.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UCameraComponent;

// Asset-only Blueprint subclasses choose meshes and materials. All behavior is native.
UCLASS(Blueprintable)
class QIANTONGUE_API ADemoUnitView : public AActor
{
    GENERATED_BODY()
public:
    ADemoUnitView();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="View") TObjectPtr<UStaticMeshComponent> Mesh;
};

struct FDemoTrace
{
    FVector2D Start, End;
    float Life = 0.18f;
    bool Ally = true;
    int32 Kind = 0;
};

UCLASS(Blueprintable)
class QIANTONGUE_API ADemoDirector : public AActor
{
    GENERATED_BODY()
public:
    ADemoDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scene") TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scene") TObjectPtr<UStaticMeshComponent> Deck;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Assets") TSubclassOf<ADemoUnitView> UnitViewClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Assets") TObjectPtr<UMaterialInterface> StageMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Assets") TObjectPtr<UMaterialInterface> AllyMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Assets") TObjectPtr<UMaterialInterface> EnemyMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") bool ShowTargetLines = true;
    void Restart(int32 Allies = 0);
    void Choose(int32 Option);
    void NewSeed();
    FVector2D MapPosition(int32 X, int32 Y) const;
    double GetCameraDepth() const { return RenderCameraY; }
    void SingleStep();
    void CycleSpeed();
    void RebuildViews();
    bool ExportResult();
    const QiantongDemo::FBattle& GetBattle() const { return Battle; }
    FVector2D ScreenPosition(const QiantongDemo::FUnit& Unit) const;
    bool Paused = false;
    int32 Speed = 1;
    TArray<FDemoTrace> Traces;
    FString Notice;
private:
    void Step();
    void SyncViews();
    QiantongDemo::FBattle Battle;
    QiantongDemo::FClock Clock;
    int32 PendingChoice = 0;
    double RenderCameraY=0;
    int32 ViewsCreated=0, ViewsDestroyed=0;
    UPROPERTY() TArray<TObjectPtr<ADemoUnitView>> Views;
    bool SmokeTest = false;
    int32 SmokePhase = 0;
    int32 SmokeOption = 0;
    int32 CaptureTicks = 0;
};

UCLASS(Blueprintable)
class QIANTONGUE_API ADemoHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
private:
    float Scale = 1, OffsetX = 0, OffsetY = 0;
    void Rect(float X, float Y, float W, float H, FLinearColor Color);
    void Text(const FString& Value, float X, float Y, float Size, FLinearColor Color);
    void Line(FVector2D A, FVector2D B, FLinearColor Color, float Width = 1);
    void Button(FName Name, const FString& Label, float X, float Y, float W, bool Active = false);
    ADemoDirector* Director() const;
};

UCLASS(Blueprintable)
class QIANTONGUE_API ADemoController : public APlayerController
{
    GENERATED_BODY()
public:
    ADemoController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    void Action(FName Name);
private:
    void NewSeed(); void Restart(); void Pause(); void Speed(); void One(); void Two(); void Three(); void Step(); void Quit();
};

UCLASS(Blueprintable)
class QIANTONGUE_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ADemoGameMode();
};
