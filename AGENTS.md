# AGENTS.md — 《无尽深渊：千瞳》UE5 原型长期工程契约

> 📌 状态：**必读 · 现行**
> 适用对象：Codex / 人类开发者 / 后续自动化 Agent。
> 目标：用 Unreal Engine 5 快速验证《千瞳》的战斗、弹道、竖井空间与决策节奏；**UE5 不是最终生产技术栈**。长期目标仍是 **Cocos Creator 3.8.8 + TypeScript + 移动/小游戏**。
>
> 本文件规定“怎么实现”；产品设计细节以 `docs/REQUIREMENTS.md` 指向的上游设计主文本为准。发生冲突时，**禁止自行脑补并继续实现**。

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

UE5 只承担：

- 交互与表现验证；
- 竖屏 2D/2.5D 战场快速搭建；
- 弹道、命中反馈、编队与竖井空间读感实验；
- Battle Lab 调试工具；
- 本机可玩的垂直切片。

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
```

禁止：

```text
QiantongCore -> QiantongUE
QiantongCore -> Unreal Engine headers/types
QiantongCore -> Renderer/UI/Input/Audio/SaveGame
View -> 修改 Core 内部对象
```

---

## 3. QiantongCore 红线

`QiantongCore` 中**不得出现**以下依赖或概念：

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

若某功能“只有用 UE 高阶功能才容易实现”，先在 Core 定义**语义契约**，UE 只实现表现适配。

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

不得因为 UE 是 3D 引擎，就在 P0/P1 引入真正的 RTS NavMesh 或完整二维物理世界。

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

### 5.3 弹道是“双层语义”

Core 决定：

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

AI 配置数据驱动。UE Behavior Tree / StateTree 可以以后用于**调试比较**，不得成为生产规则真相源。

---

## 6. UE 层使用边界

允许在 UE Adapter/View 薄层使用：

- Enhanced Input；
- UMG；
- Niagara；
- Paper2D / Sprite / Quad / Mesh；
- Material；
- Actor/Component；
- UE JSON / 文件 IO；
- Editor Utility（仅开发工具）。

但必须满足：

1. 关掉 Niagara，规则仍完整运行；
2. 删除 UI，Headless/Core Harness 仍可跑；
3. 把 Actor 表现换成 DebugShape，不改变胜负/伤害/AI；
4. 保存数据可表示为 plain JSON-like DTO，而不是 UObject graph；
5. Blueprint 只做场景拼装/表现，不写业务规则。

未经 `docs/DECISIONS.md` 拍板，P0/P1 禁用：GAS、BT/StateTree 生产化、EQS、NavMesh、Chaos 伤害、MassEntity、Lumen/Nanite 依赖型玩法。

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

UE 可建立 `ContentId -> SoftObjectPath` 的**纯视觉映射表**。

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

- “先在 Blueprint 写，迁移时再说”；
- “先让 Actor 自己打，之后再抽 Core”；
- “直接用 Projectile Collision 决定伤害”；
- “先用 Behavior Tree，Cocos 再重写”；
- “DataAsset 先当数据库”；
- “SaveGame 先随便序列化 UObject”；
- “每个单位 Tick 反正现在单位不多”；
- “Math.random/FMath::Rand 只是原型”；
- “设计还没定，先写个 placeholder 行为”；
- “只要 UE 里跑起来就算 P1 完成”。

这些路线都会把 UE 从“薄宿主”变成“隐性游戏规则”。
