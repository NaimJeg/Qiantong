# ARCHITECTURE — UE5 Prototype

> 📌 状态：**必读 · 现行**
> 版本：v0.7 — 连续地图 / 提前布敌 / 跨波次Actor生命周期。
> 本文件先记录当前代码事实；后续章节明确标为尚未落地的目标。结构/事件契约变更必须与代码同 commit 更新。

## 0. 当前演示结构（2026-10-06，DEMO-03）

- `DemoBattle` v3 开局生成9×65连续逻辑网格，视口覆盖13行；三波之间含连接区域，障碍生成后跨波保留。X、累计纵向Position、CameraY、XorShift32状态、危险、波次均属FBattle值状态。BFS固定邻接顺序找安全射击格/掩体，整格墙体阻挡移动及射线；单位没有物理碰撞，射击站位主动分散。
- 毫度 Aim 每50ms最多转9000（180°/秒），≤3000误差且射程/遮挡通过才能生成即时 FShot。射线使用逻辑坐标线段与闭合障碍格相交检测，擦边算阻挡；角度由 atan2 转整毫度。此数值实现已测 Win64，不宣称跨 TS 对齐。
- FShot 附带当次命中起终点，View 显示完整激光闪光，取消旧版飞行光点。View 销毁/重建不修改模拟。预警按固定 tick 到期汇总伤害；它不是在途子弹。
- 开局预置三波6/7/8敌人，Generation限定战斗归属；未来敌人每tick寻找掩体但不参与当前波伤害/胜负。清场后镜头650/tick下行40tick，友军最多200/tick且保持X；停稳后仅完全画外的友军纵向重布到视口上方1200，再走入1500处。敌人不重布；HP、死亡、冷却、选择、ID保留。地图有限三波，总时限6000tick。
- UnitView开局创建23/26个，正常波次不销毁/重建；仅显式新局或调试命令重建。单位Actor使用累计世界坐标，Director宿主和正交相机一起下行（避免自动正交裁剪以旧原点为参考），HUD按累计坐标减RenderCameraY投影。激光保存逻辑端点，每帧投影。旧固定横梁隐藏，连续网格/障碍由HUD绘制。
- v3新增镜头、掩体、画外重布与入场事件；独立continuous Golden保留v1/v2样本。export附带continuous-view-lifecycle.json，记录创建/销毁计数、每个稳定ID的Actor路径及ObjectId，用于跨波身份验收。
- `qt.demo` 项目控制台入口支持 restart/seed/pause/step/seek/finish/export/capture/rebuild/squad/choose，只调用现有 Director。用于本机 PIE 验收，无 Python/MCP 运行时依赖；`Tools/Send-EditorInput.ps1` 在检查窗口归属和焦点后输入本机命令。
- 场景仍为 L_Demo，五个纯数据蓝图不增加业务图表。最新版本通过 Editor/Live Coding 与 MCP 验收；旧 out/Demo Windows 包保持 v1。

以下基础模块结构延续 DEMO-01：

- `QiantongUE` Runtime 模块现含 `DemoBattle`（集中固定步长规则及 JSON 结果）、`DemoRuntime`（Director/UnitView/HUD/Controller/GameMode）和 UE 自动化测试。模块依赖 Core/CoreUObject/Engine/InputCore/Json 及保留的 QiantongCore。
- `FBattle` 是项目 UE 值对象，不读取 World、Actor、碰撞或墙钟。Director 独占它，50ms 驱动；UI 经 Controller 提交选择。稳定 ID、整数距离/HP、同 tick 汇总伤害，详见 DEMO_RULES。
- `ADemoDirector` 持有正交相机、背景网格及可销毁重建的 UnitView。单位 Actor 不 Tick、不拥有 HP。HUD 以 720×1280 设计坐标绘制，宽高比不同时居中留边。
- `Content/Demo/Blueprints` 下 5 个 Blueprint 仅继承原生类并引用资产/类，无业务图表：BP_DemoDirector、BP_UnitView、BP_DemoHUD、BP_DemoController、BP_DemoGameMode。
- `Content/Demo/Maps/L_Demo` 是独立入口关卡，包含 Director 与竖井静态几何；4 个无光照材质供背景/框架/阵营。原 Lvl_Main 保留。
- 运行时无 Python/MCP 依赖。项目启用的 MCP、Terminal、EditorToolset、Python 和编辑脚本插件限定 Editor target。Game target 可单独 cook/package；普通启动不打开 MCP。
- 当前工具链为已编译 UE 5.8.3，源目录只读。旧 EngineAssociation 保留；脚本显式传 EngineRoot。关闭本原型不需要的 Lumen/光追，未更改引擎源码。
- `Tools/CreateDemoAssets.py` 为一次性编辑器资产生成器，已有演示地图时拒绝覆盖；最终资产直接入库，玩家不运行生成器。`Tools/PackageDemo.ps1` 输出到忽略目录 out/Demo。
- Portable 库仍只提供版本和独立 CMake 验证；新玩法并未迁入它。Portable/TS 对照在正式迁移 Cocos 前执行。

下列 P0-01 记录保留为历史；原第 1–12 节是迁移参考目标，不是本轮待实现清单。验收状态以 TASK_BOARD 和 CHANGELOG 为准。

## 0.1 历史结构（2026-10-05，P0-01）

```text
QiantongCore.uproject
CMakeLists.txt
Source/
  QiantongCore.Target.cs
  QiantongCoreEditor.Target.cs
  QiantongCore/
    QiantongCore.Build.cs
    Public/Qiantong/CoreVersion.h
    Private/CoreVersion.cpp
  QiantongUE/
    QiantongUE.Build.cs
    Private/QiantongUE.cpp
Tests/Core/
  CoreSmoke.cpp
  ModuleLayoutTests.py
Tools/Build.ps1
Config/
Content/Qiantong/Level/Lvl_Main.umap
docs/
  spec/PROTOTYPE_LEDGER.md
  plan/INITIALIZATION.md
  plan/P0-P1.md
  exec/CHANGELOG.md
```

- 已有 UE **5.8.1** 源码版工程；`EngineAssociation` 保持 `{CDA1C7D2-461D-B7E1-E215-BBAA9D33582A}`。本机引擎位于 `D:/UE/Source/UnrealEngine-5.8.1-release`，版本由 `Engine/Build/Build.version` 与已有 Editor 日志交叉确认。两份 Target 使用 `Unreal5_8` include order 与 `BuildSettingsVersion.V7`。
- 当前唯一 Runtime 模块为 `QiantongUE`，负责游戏模块注册，并在启动日志调用 `Qiantong::CoreVersion()` 验证 Portable 库链接。它只直接依赖 UE `Core` 和项目 `QiantongCore`；尚无 Actor、输入或 UI，不预装其依赖。
- `QiantongCore` 的 `.h/.cpp` 是 ISO C++20，由独立 CMake STATIC target 编译，不使用 UE include、宏、PCH 或分配器。`QiantongCore.Build.cs` 仅为 UBT External 接口，向宿主提供公开头目录与 Win64 静态库；Core 不是动态加载的 UE Runtime 模块，不注册 UObject。
- Core 当前仅提供库版本 `0.1.0`（非玩法、存档或回放版本）以验收真实跨模块链接。CTest 运行独立链接 smoke 与模块接线检查；配置时的负向编译探针确认 `CoreMinimal.h` 不可见。完整 G0 扫描门仍属 P0-02；没有 BattleSimulation、规则测试、Bridge 或 ViewProxy。下列第 1–12 节仍为后续目标，不代表已有实现。
- `.uproject` 仅加载 `QiantongUE`，两份原名 Target 引用它；显示工程名仍为 `QiantongCore`。旧模板及 `/Script/QiantongCore` 包名重定向到 `/Script/QiantongUE`，现有地图二进制不改。
- DOC-01 仅修订文档政策，允许按需接入 UE Reference Backend；目前没有 Contract 接口、参考后端或双后端测试代码。本次未添加 UE 模块依赖。
- 现有地图与插件配置保留；Lumen/Nanite 等模板渲染设置仍在，但尚无 gameplay 实现或依赖。这些设置不构成表现验收证据。
- Git 基线跟踪源码、Config、地图和文档；缓存、生成的解决方案、构建输出及本机 Editor 用户配置由 `.gitignore` 排除。二进制 UE 资产通过 `.gitattributes` 禁止文本换行转换。
- Android 文件服务 `SecurityToken` 在共享配置中留空。修改前原始配置保存在不入库的 `Saved/LocalBaseline/DefaultEngine.ini`，不作为规则数据或项目真相源。

### 当前构建入口

从仓库根目录运行：

```powershell
# 无需 UE：配置、编译 C++20 静态库，运行 CTest
powershell -NoProfile -ExecutionPolicy Bypass -File Tools/Build.ps1 -CoreOnly

# 同样的 Core 检查通过后，构建 UE 5.8.1 Development Editor 目标
powershell -NoProfile -ExecutionPolicy Bypass -File Tools/Build.ps1 -EngineRoot 'D:/UE/Source/UnrealEngine-5.8.1-release'
```

脚本使用 Visual Studio 2022 x64 / Release（动态 CRT），产物放在忽略目录 `build/core/Release/`。UE 构建完整 Editor 目标，由 UBT 生成模块清单；源码引擎可能需要同步重编译其公共模块。先运行脚本，再从 Editor/IDE 使用工程；Core 变更后也走此入口。UBT 跟踪 Core 输入，库缺失或比源码旧时明确失败，避免误用旧库。External 库链接目前仅实现 Win64；CMake Core 本身不含平台 API。非默认 Debug CRT 需单独构建 CMake Debug 库，不属于本卡验收范围。

本卡不创建无消费者的 Battle/AI/技能/数据空目录，也不引入占位玩法。任务通过状态与命令结果以 TASK_BOARD / CHANGELOG 为准。

---

## 1. 目标目录

```text
QiantongPrototype/
├─ AGENTS.md
├─ QiantongPrototype.uproject
├─ Config/
├─ Content/
│  └─ Prototype/                 # UE 专属视觉/场景资产
├─ Data/                         # 引擎中立内容数据
│  ├─ weapons.json
│  ├─ mechs.json
│  ├─ affixes.json
│  ├─ skills.json
│  ├─ enemies.json
│  ├─ ai-profiles.json
│  └─ encounters.json
├─ Source/
│  ├─ QiantongCore/              # 纯 ISO C++ gameplay semantics
│  │  ├─ Public/Qiantong/
│  │  │  ├─ Base/
│  │  │  ├─ Battle/
│  │  │  ├─ AI/
│  │  │  ├─ Ballistics/
│  │  │  ├─ Loadout/
│  │  │  ├─ Run/
│  │  │  └─ Save/
│  │  └─ Private/
│  └─ QiantongUE/                # Unreal host / adapter / view
│     ├─ Bridge/
│     ├─ ReferenceBackends/       # 按需创建，隔离 UE 参考实现
│     ├─ View/
│     ├─ UI/
│     └─ Platform/
├─ Tests/
│  ├─ Core/
│  ├─ Scenarios/
│  └─ Golden/
├─ Tools/
│  ├─ CoreBoundary/
│  └─ ScenarioRunner/
└─ docs/
```

> 若实际初始化使用不同 UE module 名，可以改名；**分层语义不能改**。

---

## 2. 模块职责

### QiantongCore

职责：项目引擎中立 Contract、纯状态与确定性 Portable gameplay state transition。参考后端共享这些契约，不在 Core 中实现 UE 调用。

允许输入：

- Config DTO；
- `GameCommand`；
- 固定 `SimTick`；
- seed；
- 前一状态。

允许输出：

- 新状态；
- `SimEvent[]`；
- 只读 Snapshot。

不得：加载资源、播放音效、生成 Actor、读屏幕尺寸、获取系统时间。

### QiantongUE

职责按 Host、可选参考后端与表现/平台划分：

```text
Bridge/    fixed-step host, Core<->UE conversion
ReferenceBackends/  optional GAS/StateTree/EQS adapters behind neutral contracts
View/      unit/projectile/stage presentation
UI/        UMG/view-model/input intents
Platform/  JSON/file/save/audio/device adapter
```

UI 不直接改 BattleState；UI 产生 `GameCommand` 或 Application action。

### 可选 Reference Backend（目标契约，尚未实现）

```text
项目 Contract：plain DTO / stable ID / 行为与时间规范
  ↑ implements                    ↑ adapts
Portable C++ implementation       UE Reference Backend
  ↑ selects                       ↑ selects
             Application/Bridge
                     ↓
            Snapshot/Event → View/UI
```

Contract 可在 Core 的对应子系统中定义，不预建通用接口框架。以 TargetSelector 为例，Host 可选择 PortableTargetSelector 或 UEEQSTargetSelector；两者接受相同事实/候选 DTO，输出稳定 EntityId。Core 不 include、链接或持有 UE 类型，Host 负责转换、检查结果并按规定 tick/顺序提交。

每次运行只选一个权威后端；对照运行使用独立状态副本。参考后端不得通过异步回调、Actor Tick、动画或碰撞事件直接写 BattleState。跨 tick 的语义状态可表达为中立 DTO；UE 句柄仅在 Adapter 内作运行时映射，不进入规范配置、存档或 UI ViewModel。未满足确定性与 DTO 边界的系统只可用于非权威实验。

双后端只针对实际采用参考实现的系统：允许 UE 实验先行，标为探索中；对应规则卡完成前，Portable 必须在无 UE harness 中跑通，并按 `AGENTS.md` G6 比较逐 tick 行为、事件顺序及 Golden 结果。参考实现不自动拥有裁决权，差异回到 Contract/ledger 处理。P1.5 的 C++/TS 对照仍独立必需。

UE 专属图表、DataAsset 可为实验适配/可重建缓存，不是可写规则数据库。表现系统则通过 FxRequest/ViewModel/GameCommand 等项目消息替换，不要求复刻 Niagara/UMG；Lumen/Nanite 不进入移动端玩法或内容标准。研究/复刻预算与源码独立实现纪律统一见 `AGENTS.md` 6.1。

---

## 3. 首批核心类型

P0 最低集合：

```text
Base/
  Types.h
  Fixed.h                 # 如需要
  DeterministicRng.h

Battle/
  BattleConfig.h
  BattleState.h
  UnitState.h
  UnitStats.h
  GameCommand.h
  SimEvent.h
  BattleSnapshot.h
  BattleSimulation.h/.cpp

Ballistics/
  ShotPlan.h
  ShotSemantic.h

AI/
  AiProfile.h
  AiDecision.h
  AiEvaluator.h/.cpp
```

第一阶段**不要**一开始就建几十个抽象基类。

采用“数据 + 纯过程”优先：

```cpp
BattleStepResult StepBattle(
    const BattleConfig& config,
    BattleState state,
    std::span<const GameCommand> commands);
```

具体 API 可以调整，但必须保持：状态输入清楚、事件输出清楚、无 UE side effect。

---

## 4. 状态拥有关系

```text
UQiantongBattleHost / host object
  owns
BattleSimulation / BattleState
  owns values only

UE View Registry
  EntityId -> weak Actor/ViewProxy
```

上图是 Portable 默认路径。采用参考后端时，Host 另拥有适配器及运行时对象，权威语义状态仍须通过中立 DTO 表达；不能形成第二套独立 HP/AI 真相源。

View Registry 允许丢失/重建。

必须能执行：

```text
DestroyAllViews()
RebuildViewsFromSnapshot()
```

且规则状态不丢失。

---

## 5. 固定步长 Host

UE 帧循环：

```text
Frame Delta
   ↓ accumulate
while accumulator >= SIM_STEP:
   collect queued commands
   Step selected implementation through neutral contract
   emit events/snapshot
   accumulator -= SIM_STEP
   ↓
View interpolation(alpha)
```

限制：

- 一帧可补多个 sim step；
- 必须有最大 catch-up 防止死循环；
- 性能掉帧只能影响视觉/延迟，不能让规则改成 variable-dt；
- Pause 若存在必须是 Host 层控制是否推进，不是 Core 偷读世界 pause 状态。

---

## 6. 事件契约

P0/P1 先建立小而稳定的事件集，不按 FX 粒度发事件。

示例：

```text
BattleStarted
UnitSpawned
TargetChanged
ShotFired          -> contains ShotPlan/ShotId
HitResolved
DamageApplied
SkillStarted
SkillResolved
UnitDied
BattleEnded
ChoiceOffered
ChoiceApplied
```

原则：

- Event 描述**规则已经发生的事实**；
- View 不通过“动画播完回调”完成伤害结算；
- 如果表现丢了一帧 Event，可以用 Snapshot 恢复最终状态。

---

## 7. 弹道契约

Core：

```text
ShotPlan
  shotId
  sourceEntity
  targetEntity
  fireTick
  impactTick
  semantic
  payload/ref
  visualProfileId
```

UE：

```text
visualProfileId
  -> projectile proxy
  -> curve/speed/trail/impact FX
```

`visualProfileId` 不能决定最终伤害。

需要“旅行时间有战术意义”时，Core 按 `impactTick` 排队结算；否则可以 fire/impact 同 tick，但 View 仍可播放延迟动画，前提是不误导玩家。

---

## 8. AI 契约

AI 的 Portable 实现是 Core 内的确定性 evaluator；P0/P1 可由 Host 接入 StateTree/BT/EQS 参考实现来探索同一 Contract。配置与输出不使用 UE 节点或 Gameplay Tags 类型。

建议输入：

```text
SelfState
VisibleBattleFacts
AiProfile
SimTick
RngState（仅必要时）
```

输出：

```text
AiIntent
  desiredState
  targetId
  moveIntent
  abilityIntent
```

P1 不需要通用行为树编辑器。先证明现有六态 + target policy 能解释战斗。

---

## 9. 数据加载

推荐流程：

```text
Data/*.json
   ↓ QiantongUE/Platform JSON parser
validated host DTO
   ↓ explicit conversion
QiantongCore config structs
```

Core 不解析 JSON。

P0 可以先用 C++ fixture；P1 再接 JSON，以减少初始化变量。

---

## 10. UE 表现结构

初版推荐：

```text
AQiantongBattleStage
UQiantongBattleHostComponent / subsystem-like host
AQiantongUnitView
AQiantongProjectileView / pooled proxy
UQiantongBattleHUD
```

名称可以调整。关键是：

- Actor 不持有权威 HP；
- View 的位置来自 Snapshot + Layout Mapper；
- UI 读 ViewModel/Snapshot；
- 单位 View 默认 `PrimaryActorTick.bCanEverTick = false`，统一 ViewSystem 更新；
- 大量瞬时 FX 走池化。

---

## 11. 竖屏布局

规则层没有 `720×1280`。

UE View 建：

```text
LogicalLayoutSpec
  -> LogicalToScreen
  -> ScreenToWorld (if needed)
```

第一阶段保持上游空间语义：

- 720 宽为设计基准；
- 左侧角色栏保留；
- 中央/井筒区域不被 UI 侵占；
- 母舰只露屏幕内的下部；
- 下方为敌人上涌方向；
- 常态层先单屏/纵向扩展，不为大层 2×2 提前引入 NavMesh。

---

## 12. TypeScript 对照目标

每个 Core C++ 模块设计时写出未来对应：

```text
QiantongCore/Battle/BattleState.*   -> assets/scripts/rules/battle/BattleState.ts
QiantongCore/AI/*                    -> assets/scripts/rules/ai/*
QiantongCore/Ballistics/*            -> assets/scripts/rules/ballistics/*
QiantongCore/Run/*                   -> assets/scripts/rules/run/*
```

避免 C++ 模板元编程、复杂继承、所有权技巧成为业务语义。**能写成朴素 TS 的结构，优先于“漂亮的 C++”。**

---

## 13. 当前变更追踪

| 版本 | 变化 |
|---|---|
| v0.1 | 建立 Portable Core / UE Adapter / View 的初始目标结构。 |
| v0.2 | P0-00：确认 UE 5.8.1 与真实模板模块，恢复规范文档路径，建立 Git 排除和资产属性；明确 G0 基线红项及尚未实现的目标。 |
| v0.3 | DOC-01：允许隔离的 UE Reference Backend，补充中立契约、单一权威状态、按需双后端和 G6 迁移门；代码现状未变。 |
| v0.4 | P0-01：CMake Portable 静态库与 UBT External 接口、QiantongUE Runtime 注册、独立 smoke / 接线测试及统一构建入口；目标结构与当前实现继续分开记录。 |
