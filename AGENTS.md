# AGENTS.md — 《无尽深渊：千瞳》UE5 原型长期工程契约

> 📌 状态：**必读 · 现行**
> 适用对象：Codex / 人类开发者 / 后续自动化 Agent。
> 目标：用 Unreal Engine 5 快速验证《千瞳》的战斗、弹道、竖井空间与决策节奏；**UE5 不是最终生产技术栈**。长期目标仍是 **Cocos Creator 3.8.8 + TypeScript + 移动/小游戏**。
>
> 本文件规定“怎么实现”；产品设计细节以 `docs/REQUIREMENTS.md` 指向的上游设计主文本为准。发生冲突时，**禁止自行脑补并继续实现**。

## 2026-10-06 现行执行修订（优先于下文旧阶段政策）

用户已批准玩法优先、UE 原生实现及可打包蓝图演示。当前单场原型按 `docs/spec/DEMO_RULES.md` 执行，不受缺失的上游细则阻塞；这些是实验规则，不是最终产品裁决。

- 新规则允许在 QiantongUE 中用 UE 值类型集中模拟；Blueprint 极薄，仅管理资产、表现参数和实例化对象。全部逻辑（含启动、Tick、交互）由 C++ 完成。不预建 Portable/双后端/通用能力框架。
- QiantongCore 保持纯 C++，保留现有库及测试，不要求新玩法先进入 Core。下文 G0 仅约束该库。
- 固定 50ms、稳定 ID、单一权威状态、UI 命令、视觉碰撞不结算伤害继续有效。
- Portable 抽离、G6 对照及 P1.5 TS 验证后置到正式进入 Cocos 前；不阻塞 UE 原型卡验收。
- 本机使用已编译的 UE 5.8.3 源码版启动、项目编译与打包。引擎源码和插件源码只读；允许构建产物。未经要求不更改工程关联。
- 当前 DEMO-03：连续累计地图、随机障碍、镜头先行；友军完全画外保X重布，镜头停稳后走入；敌人提前布置/寻找掩体，由镜头带入。正常波次保留所有单位Actor。即时激光、限速瞄准和自动避险延续。遵循 DEMO_RULES v3；能量/热量/技能/存档/养成仍后置。
- 新增演示任务的完成门：项目构建或 Live Coding、规则自动化、无 View 模拟、蓝图编译、Editor PIE 运行证据。用户在 DEMO-02 明确本轮及以后可经 Terminal/MCP 在编辑器完成验收；不要求重复打包。旧包仅为 v1，不能代表新版。旧 P0/P1 全量门不代表本轮范围。
- 保留未提交工作与历史证据；不为原型实施改变引擎代码。下文冲突的 Portable-first/阶段门条款由本节覆盖。

---

## 0. Codex 开工顺序

每个新会话按顺序执行：

1. 读 `AGENTS.md`。
2. 读 `docs/REQUIREMENTS.md`。
3. 读 `docs/spec/PROTOTYPE_LEDGER.md`，确认任务依赖的条目均为 `已定`。
4. 读 `docs/ARCHITECTURE.md`，确认当前结构与目标结构。
5. 读 `docs/TASK_BOARD.md`，只领取一个未完成闭环块。
6. 若是首次初始化，再读 `docs/plan/INITIALIZATION.md`；P0/P1 开发读 `docs/plan/P0-P1.md`。
7. 执行前检查 `git status`，不得覆盖用户未提交工作。
8. 先补/改测试，再做最小实现，再跑验收。
9. 结构/契约变化必须**同一 commit**更新 `docs/ARCHITECTURE.md`；任务状态与证据更新 `docs/TASK_BOARD.md`；落地写 `docs/exec/CHANGELOG.md`。

**不得因为“原型”跳过架构边界。原型可以少做功能，不可以先制造迁移债。**

---

## 1. 项目最高级目标

### 1.1 UE5 的定位

UE5 承担：

- 交互与表现验证；
- 竖屏 2D/2.5D 战场快速搭建；
- 弹道、命中反馈、编队与竖井空间读感实验；
- Battle Lab 调试工具；
- 本机可玩的垂直切片。
- 成熟引擎系统的架构研究，以及隔离在引擎中立契约之后的参考实现/原型加速器。

UE5 **不承担**：

- 最终规则真相源；
- 最终存档格式真相源；
- 最终内容数据库；
- 依赖 UE 专属系统才能成立的玩法逻辑。

### 1.2 长期迁移目标

预期迁移关系：

```text
Portable C++ Rules          -> TypeScript rules/
UE Application/Bridge       -> Cocos core/game host
UE View / Niagara / UMG     -> Cocos view / NodePool / Tween / Sprite
UE Platform adapters        -> Cocos/WeChat platform adapters
JSON/IDs/Scenario fixtures  -> 原样保留或机械转换
```

**验收标准不是“看起来解耦”，而是代表性 Golden Scenario 能在 C++ 与 TypeScript 产生等价结果。**

---

## 2. 依赖方向：硬约束

目标依赖只能单向：

```text
Data / Config DTO
      ↓
QiantongCore          # 纯规则、纯状态、确定性模拟
      ↓ events/snapshot ↑ commands/config
Application / Bridge  # 驱动 fixed-step，做格式/平台转换
      ↓
UE View / UI / FX     # Actor/Component/UMG/Niagara，只消费状态
```

允许：

```text
QiantongUE -> QiantongCore
Tools/TestHost -> QiantongCore
QiantongUE/ReferenceBackends -> QiantongCore 中的引擎中立 Contract
```

Application/Bridge 负责组合 Portable implementation 或 UE Reference Backend。Contract 的 DTO、稳定 ID 与行为规范归项目所有；Core 不反向 include 或链接 UE 后端。后端替换及验收见第 6.1 节。

禁止：

```text
QiantongCore -> QiantongUE
QiantongCore -> Unreal Engine headers/types
QiantongCore -> Renderer/UI/Input/Audio/SaveGame
View -> 修改 Core 内部对象
```

---

## 3. QiantongCore 红线

`QiantongCore` 中**不得出现**以下 UE 依赖或实现；允许独立定义引擎中立的 State/Condition/Effect/TagSet 等模型：

- `UObject` / `AActor` / `UActorComponent`；
- `FVector` / `FTransform` / `FName` / `FString` / `TArray` / `TMap`；
- `CoreMinimal.h`、任何 Unreal Engine 头文件；
- `UPROPERTY` / `UFUNCTION` / 反射宏；
- Gameplay Ability System；
- Gameplay Tags 作为玩法数据模型；
- Behavior Tree / StateTree / EQS；
- NavMesh / NavigationSystem；
- Chaos / UE Projectile collision 作为伤害真相；
- `FMath::Rand`、`FRandomStream` 或平台随机数；
- 世界时钟、真实时间、帧率作为规则判断依据；
- Blueprint 作为规则实现；
- DataAsset/DataTable 作为产品内容唯一来源。

Core 优先使用：

- ISO C++ 标准库；
- POD/普通 struct；
- 显式 ID；
- 值语义；
- 纯函数/小型 state transition；
- 明确量纲的整数或受控数值类型；
- 项目自有确定性 PRNG。

若某功能借助 UE 高阶系统更容易验证，先定义**引擎中立语义契约**，UE 可实现表现适配或隔离的 Reference Backend，再独立实现项目所需的可移植子集。

---

## 4. 数据与数值纪律

### 4.1 ID

跨层引用必须使用稳定 ID，不把对象地址当身份：

```text
EntityId
ContentId
WeaponId
MechId
AffixId
SkillId
ScenarioId
```

Core 内不持有 UE 对象指针。

### 4.2 时间

- Core 使用**固定模拟步长**。
- 第一版技术基线：`SIM_STEP_MS = 50`（20 Hz 规则模拟）。
- UE 每帧可以插值渲染，但不得改变 Core 结果。
- 改 `SIM_STEP_MS` 属于架构/回放兼容变更，必须记录并重跑 Golden Tests。

### 4.3 量纲

禁止“裸 float 到处传”。至少约定：

- 时间：毫秒或 sim tick；
- 射程/距离：`milli-grid`，`1 grid = 1000`；
- 比率：basis points，`10000 = 1.0`，或集中定义的定点数类型；
- HP/能量/热量：整数单位；
- 屏幕像素与 UE 世界单位**只存在 View/Bridge**。

如果确实需要浮点，必须把浮点误差与跨 TS 端的等价规则写进测试，不得默认 IEEE 运算顺序“自然一致”。

### 4.4 随机

- 使用项目自有、跨语言容易复刻的 32-bit PRNG，例如 XorShift32/PCG32 的固定版本；
- seed 属于 `BattleState/RunState` 可序列化状态；
- 同 seed + 同输入命令必须同结果；
- 禁止依赖 hash-map/unordered-map 的遍历顺序决定玩法结果。

---

## 5. 战斗模型约束

### 5.1 规则空间与表现空间分离

Core 的基础战斗空间继续使用**射程公共刻度**，不是 UE 世界坐标。

```text
logicalDistance + lane/formation semantic
                ↓ View mapping
UE world/screen coordinates
```

P0/P1 的规则空间仍为射程轴与 lane/formation。可以在隔离实验中研究 NavMesh/物理查询，但其输出必须转换为上述空间契约；不得因工具选择擅自引入 RTS 寻路或完整二维物理玩法。

### 5.2 单位不拥有独立规则 Tick

禁止：

```text
Actor Tick -> 找目标 -> 算伤害 -> 改敌人 Actor
```

必须：

```text
BattleSimulation::Step()
  -> 更新全部 UnitState / ProjectileSemanticState
  -> 生成 SimEvent + Snapshot
  -> ViewProxy 读取并表现
```

Actor/Component 是 ViewProxy，不是规则实体。

上述流程描述 Portable 基准路径。第 6.1 节的参考后端可在 Host 调度下替换某个契约的实现，统一返回中立结果；不能让单位 Actor 独立推进或同时由两个后端结算。

### 5.3 弹道是“双层语义”

规则契约决定以下事实（Portable 路径由 Core 实现，参考路径遵循第 6.1 节）：

- 射手；
- 目标；
- impact tick；
- direct / pierce / bounce / blast / multishot 等语义；
- 命中结果与伤害。

View 决定：

- 直线、抛物线、Bezier、Spline 等轨迹；
- Trail、Niagara、Sprite/Mesh；
- 命中闪光与屏幕反馈。

**视觉弹丸的碰撞不能反向决定 Core 是否命中。**

### 5.4 AI

P0/P1 使用自有可移植决策层：

```text
Condition -> Action
Selector / FirstMatch
idle / approach / engage / reposition / flee / dead
```

AI 配置数据驱动。P0/P1 即可通过 BT/StateTree/EQS Reference Backend 探索与验证同一 AI Contract；Portable evaluator 是无 UE 验收路径。UE 节点/资产不成为规则、配置或存档真相源。

---

## 6. UE 层使用边界

允许在 UE Adapter/View 薄层使用：

- Enhanced Input；
- UMG；
- Niagara；
- Paper2D / Sprite / Quad / Mesh；
- Material；
- Animation Blueprint / Sequencer / Camera（仅表现，不以动画回调结算规则）；
- Actor/Component；
- UE JSON / 文件 IO；
- Editor Utility（仅开发工具）。

但必须满足：

1. 关掉 Niagara，规则仍完整运行；
2. 删除 UI，Headless/Core Harness 仍可跑；
3. 把 Actor 表现换成 DebugShape，不改变胜负/伤害/AI；
4. 保存数据可表示为 plain JSON-like DTO，而不是 UObject graph；
5. 正式业务规则由引擎中立 Contract 定义；Blueprint 可做场景拼装、表现及隔离的参考后端实验，不作为唯一规则定义或可移植实现。

P0/P1 允许 GAS、BT/StateTree、EQS、NavMesh、MassEntity、Chaos 等作为参考后端或研究对象；不得让这些系统定义不可替代的 Gameplay Semantic。Lumen/Nanite 可用于原型显示与资产便利，不作为玩法条件或移动端内容预算依据。满足本章的后端实验无需再次申请“解除系统禁令”；新增或冲突的玩法语义仍按 ledger/DECISIONS 流程处理。

### 6.1 UE Reference System / Portable Reimplementation Policy

**可以使用 UE 高级功能；禁止依赖 UE 专属语义。可以研究 UE 源码；提炼设计思想并独立重实现，不复制 Engine Code 到 Portable/Cocos 实现。**

```text
Engine-neutral Contract（输入、输出、状态、时间、数值、顺序）
  ├─ Portable implementation：纯 C++，随后独立实现 TypeScript 版本
  └─ UE Reference Backend：GAS / StateTree / EQS 等，经 Adapter 转换
Application/Bridge 选择后端；View 消费统一 Snapshot/Event
```

- 按实际需要为目标选择、AI、技能等系统建立契约，不预建通用引擎框架，也不要求每个系统都有两个后端。采用 UE 参考实现的系统必须补齐同契约的 Portable 实现与对照证据后，才可标记该规则卡完成。
- 允许先运行 UE 参考实验来确定所需能力；Portable 尚未补齐时，标为探索中，不计入规则闭环或 P1/P1.5 通过。参考结果是对照样本，不是自动正确的 Oracle；差异按契约判定，设计分叉回到 ledger，不能直接覆写 Golden。
- 每次实验只有一个选定后端提交权威结果；另一个只能在相同初态的隔离副本上对照。Host 按固定 sim tick 驱动，UE 内部回调不得绕过契约写 Core；DTO 边界检查 ID、范围及结果合法性，禁止双重结算。影响后续决策的状态须可表达为 plain DTO，不能藏在 UE 对象中。
- `FGameplayAbilitySpecHandle`、`FGameplayEffectSpec`、`FGameplayTagContainer`、UStateTree 节点、UObject 指针等只能留在 UE Adapter/Reference Backend；不得传播到 Core、规范配置、存档或 UI 业务 ViewModel。项目 TagSet 使用稳定 string/ID，自有模型不依赖 Gameplay Tags。
- 后端使用相同 fixture、seed、命令序列、固定步长、排序和数值规则。无法满足确定性/隔离要求的 UE 功能只能用于非权威实验，不进入可替换规则路径。

每次引入前，在对应任务记录“复刻预算”：要验证的项目问题、最小能力子集、Cocos 现有等价能力、适配或独立实现的成本、明确不迁移的功能，以及可复现场景和退出条件。主要服务 UE 编辑器或 AAA 渲染的能力不迁移；Mass/SoA 等存储优化需实际规模或性能证据，不以全量复刻 UE 为目标。

| UE 系统 | 可提炼的项目子集或替换方式（按需选择） |
|---|---|
| StateTree / BT | State、Condition、Transition、Task、Priority、Enter/Tick/Exit |
| GAS / Gameplay Tags | Ability/Effect evaluator、Cost、Cooldown、自有 TagSet |
| EQS | Candidates → Filter → Score → Select；同分排序明确 |
| MassEntity | 简化批处理或 SoA 存储，不强制引入完整 ECS |
| Niagara / UMG / Enhanced Input | FxRequest / ViewModel / GameCommand；替换表现与输入 Adapter |
| NavMesh / Chaos | lane/grid/graph 或最小碰撞、sweep、击退数学；视觉碰撞不决定伤害 |
| Lumen / Nanite | 无需对应实现，不迁移 AAA 渲染系统 |

源码研究流程为“阅读 → 用项目语言描述问题与契约 → 独立实现 → 场景对照”。研究记录注明来源、版本与采用的思想；禁止将 UE Engine Code 复制或逐行翻译到 Portable/Cocos 代码。Epic 官方 FAQ 明确区分学习知识后独立编写与复制引擎代码；本项目据此采用上述源码纪律。[Epic 官方源码 FAQ](https://www.unrealengine.com/ue-on-github/)（核验：2026-10-05）

---

## 7. 内容与配置

产品内容使用**引擎中立 schema**。推荐仓库形态：

```text
Data/
  weapons.json
  mechs.json
  affixes.json
  skills.json
  enemies.json
  ai-profiles.json
  encounters.json
```

规则层只接收解析后的 DTO；JSON 解析属于 Host/Adapter。

禁止：

- 同一数值同时存在 JSON 与 Blueprint/DataAsset 两个可写真相源；
- UE 资源名成为 gameplay stable ID；
- View 自动反向生成设计数值。

UE 可建立 `ContentId -> SoftObjectPath` 的视觉映射表，以及 Reference Backend 的可重建运行时映射/缓存。后者从中立配置转换，不成为第二份可写玩法真相源。

---

## 8. 保存与回放

保存结构从原型期即按未来 Cocos 可表达格式设计：

```text
SaveEnvelope
  schemaVersion
  savedAt        # host metadata，不能影响规则
  meta
  run
```

规则数据只允许：number / boolean / string / array / plain record / stable ID 等可映射结构。

不得以 `USaveGame` 的 UObject 图作为规范格式；可以用它做**包装容器**，内部仍保存规范 DTO。

---

## 9. 测试与门

任何规则实现至少需要：

### G0 — Core Boundary

- Core 不含 Unreal include/type/macro；
- 依赖方向无反转。

### G1 — Build

- UE Development Editor 构建通过；
- 独立 Core tests/harness 通过。

### G2 — Determinism

- 固定 seed 重复执行，关键 snapshot/event hash 一致；
- 改渲染帧率不改变结果。

### G3 — Golden Scenario

输出至少包含：

```text
winner
simTicks
remainingHp[]
energy
heat
shots
skillCasts
targetSwitches
eventHash
```

### G4 — View Isolation

至少一个测试/模式证明：无 Niagara、无正式 Sprite、无 UI 时，BattleSimulation 仍能完整收束。

### G5 — Visual Evidence（只针对 View/UI 任务）

表现任务不能只报“编译通过”，必须提供运行截图/录屏/可复现实验入口。

### G6 — Reference Backend Portability（采用 UE 规则参考后端时）

- 记录 Contract 版本、后端/引擎版本、fixture、seed、命令序列与复现入口。
- UE Reference 与 Portable C++ 对照逐 tick 的目标、状态转换、技能合法性、资源、伤害及事件顺序，并比较 G3 全部适用字段；不能只比较赢家。中立化时只剔除预先声明的表现字段，不隐去规则差异。
- 对非法目标、同分候选、死亡/取消、成本不足、冷却边界等适用边界补测试；数值差异必须在契约中预先说明，不能用宽松误差掩盖行为漂移。
- 关闭 UE Reference Backend 后，Portable harness 独立通过同一场景；Core、配置、存档和 UI 业务模型无 UE 类型外泄。
- 对照通过仅证明已测子集。P1.5 仍须代表性 Portable C++/TypeScript Golden 对齐；UE 参考后端的通过不能替代 TS 迁移门。

---

## 10. 文档与 commit 纪律

- `docs/spec/PROTOTYPE_LEDGER.md`：原型架构/验证条目的唯一现行源。
- `docs/DECISIONS.md`：仅放待拍板，不存已定历史。
- `docs/TASK_BOARD.md`：任务与验收状态唯一源。
- `docs/ARCHITECTURE.md`：**今天代码长什么样**，不是愿景论文。
- `docs/plan/*`：怎么做到。
- `docs/exec/CHANGELOG.md`：做过什么。

规则：

1. 设计冲突未解决，不进代码。
2. 结构/事件契约/模块边界变化，和代码**同 commit**更新 ARCHITECTURE。
3. commit message 带任务/条目 ID，例如：`[P0-03][ARC-004] add deterministic sim clock`。
4. 不改用户历史提交，不擅自 reset/rebase/force push。
5. 不提交密钥、引擎缓存、构建产物。
6. 不为“让门变绿”删除有价值的断言；先证明门错还是实现错。

---

## 11. 完成定义

Codex 不得用“implemented”“done”描述未验收工作。

一张卡完成至少满足：

```text
代码/文档产物在位
+ 对应自动测试通过
+ 门通过或明确记录基线红项
+ 若属表现任务有运行证据
+ TASK_BOARD 更新证据
+ ARCHITECTURE（若结构变化）同 commit 更新
```

若受环境阻塞（UE 未安装、无法运行 Editor 等），可以完成**静态层**，但必须写成：

> “静态实现完成；运行验收未执行，原因：……；不得标记整卡 Done。”

---

## 12. 明确禁止的“原型捷径”

出现以下做法时应停止并重构，而不是继续堆功能：

- “只在 Blueprint 定义业务规则，不提取 Contract 或 Portable 对照”；
- “先让 Actor 自己打，之后再抽 Core”；
- “直接用 Projectile Collision 决定伤害”；
- “用 Behavior Tree/GAS 作为唯一规则定义，没有中立契约、复刻预算和对照场景”；
- “DataAsset 先当数据库”；
- “SaveGame 先随便序列化 UObject”；
- “每个单位 Tick 反正现在单位不多”；
- “Math.random/FMath::Rand 只是原型”；
- “设计还没定，先写个 placeholder 行为”；
- “只要 UE 里跑起来就算 P1 完成”。

这些路线会使 UE 变成隐性规则真相源。遵循第 6.1 节的参考后端探索与独立重实现不属于上述捷径。
