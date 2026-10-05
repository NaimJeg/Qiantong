# ARCHITECTURE — UE5 Prototype

> 📌 状态：**必读 · 现行**
> 版本：v0.3 — P0-00 代码现状 / DOC-01 参考后端契约。
> 本文件先记录当前代码事实；后续章节明确标为尚未落地的目标。结构/事件契约变更必须与代码同 commit 更新。

## 0. 当前结构（2026-10-05，P0-00）

```text
QiantongCore.uproject
Source/
  QiantongCore.Target.cs
  QiantongCoreEditor.Target.cs
  QiantongCore/
    QiantongCore.Build.cs
    QiantongCore.h
    QiantongCore.cpp
Config/
Content/Qiantong/Level/Lvl_Main.umap
docs/
  spec/PROTOTYPE_LEDGER.md
  plan/INITIALIZATION.md
  plan/P0-P1.md
  exec/CHANGELOG.md
```

- 已有 UE **5.8.1** 源码版工程；`EngineAssociation` 保持 `{CDA1C7D2-461D-B7E1-E215-BBAA9D33582A}`。本机引擎位于 `D:/UE/Source/UnrealEngine-5.8.1-release`，版本由 `Engine/Build/Build.version` 与已有 Editor 日志交叉确认。两份 Target 使用 `Unreal5_8` include order 与 `BuildSettingsVersion.V7`。
- 当前唯一 Runtime 模块 `QiantongCore` **仍是 UE 模板游戏模块，不是 Portable Core**：包含 `CoreMinimal.h`、模块注册宏，并依赖 Core/CoreUObject/Engine/InputCore/EnhancedInput。G0 是已知基线红项，需由 P0-01 拆分、P0-02 建门。
- 当前没有 BattleSimulation、独立 Core harness、Bridge 或 ViewProxy；P0-01 起的任务仍为 TODO。下列第 1–12 节是后续实现的目标契约，不代表已有实现。
- DOC-01 仅修订文档政策，允许按需接入 UE Reference Backend；目前没有 Contract 接口、参考后端或双后端测试代码。本次未添加 UE 模块依赖。
- 现有地图与插件配置保留；Lumen/Nanite 等模板渲染设置仍在，但尚无 gameplay 实现或依赖。这些设置不构成表现验收证据。
- Git 基线跟踪源码、Config、地图和文档；缓存、生成的解决方案、构建输出及本机 Editor 用户配置由 `.gitignore` 排除。二进制 UE 资产通过 `.gitattributes` 禁止文本换行转换。
- Android 文件服务 `SecurityToken` 在共享配置中留空。修改前原始配置保存在不入库的 `Saved/LocalBaseline/DefaultEngine.ini`，不作为规则数据或项目真相源。

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
