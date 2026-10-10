# Qiantong — DEMO-04 及后续更新计划（提案）

> 审查基线：`NaimJeg/Qiantong` `master@4bc4986f8cb5836ba9b914c3722ecfeab3ddcc92`（2026-10-07 审查）  
> 当前交付：DEMO-03 / `demo-3`。  
> 文档性质：**计划提案，不是已定 gameplay spec**。在用户确认前，不应把下列新玩法语义写入 `docs/spec/PROTOTYPE_LEDGER.md` 的“已定”项，也不应直接改 Golden 结果。

---

## 1. 当前状态判断

DEMO-03 已经证明了以下闭环：

- 9×65 的有限连续竖井；
- 三波敌人预生成并提前寻找掩体；
- 镜头先行、友军画外保 X 重布、停稳后重新入场；
- 波次之间保留单位 Actor 身份；
- 50 ms / 20 Hz 集中规则模拟；
- 即时激光、限速瞄准、障碍遮挡、BFS 站位、危险区自动撤离；
- 2/5 人模式、三选一、Golden 与 PIE 验收。

当前最应该优先补的不是更多规则，而是 **Render/View 与固定步长规则之间的平滑层，以及性能测量基线**。否则继续增加地图长度、单位、弹道和 AI，会把现在的卡顿/跳帧问题放大。

---

# 2. 性能审查结论

## 2.1 P0 — 很可能直接造成“看起来卡”的问题：单位 View 没有帧级插值

当前 `FBattle` 每 50 ms 更新一次，即规则频率为 20 Hz。`ADemoDirector::Tick()` 虽然每渲染帧都会调用 `SyncViews()`，但 `SyncViews()` 使用的是当前 `Battle.Units` 的离散坐标和离散 Aim：

```text
20 Hz Battle state
    ↓
SetActorLocation(current logical position)
SetActorRotation(current logical aim)
    ↓
60/120/144 Hz 重复显示同一个值，下一 sim tick 再突然跳变
```

摄像机使用 `FInterpConstantTo()` 做了帧级平滑，单位位置和朝向却没有。因此在 60 FPS 下，单位通常会每 3 帧跳一次；在 120/144 Hz 显示器上，20 Hz 与刷新率的 cadence 会更明显。

这会产生一种典型现象：

> `stat unit` 看起来没有明显掉帧，但移动和转向仍然像低帧率。

### DEMO-04 必修

建立独立的 View interpolation：

```text
PreviousSimSnapshot
CurrentSimSnapshot
RenderAlpha = accumulator / SIM_STEP
        ↓
InterpolatedPosition / InterpolatedAim
        ↓
Actor View
```

Aim 必须沿最短角差插值，不能直接 lerp 0°/360°。

规则层仍保持 20 Hz，不因表现平滑改变 EventHash / Golden。

---

## 2.2 P0 — `SyncViews()` 存在重复提交

当前逻辑：

```text
Tick
  for every catch-up Step:
      Step()
          Battle.Step()
          SyncViews()
  camera interpolation
  SyncViews() again
```

也就是说，发生模拟步的帧至少同步两次；在 5×/10× 时，一个渲染帧可能执行多个模拟步，从而重复提交所有 Actor Transform。

当前单位总数最多约 23/26，因此尚不至于灾难性，但它是纯冗余，而且会随单位量和倍速线性放大。

### DEMO-04 必修

- `Step()` 只更新规则状态和 View event buffer；
- 一帧所有 fixed-step 完成后，只调用一次 `SyncViews(RenderAlpha)`；
- `SingleStep()` 单独同步一次；
- `Camera->SetRelativeLocation(0,0,1500)`、`Deck->SetRelativeLocation(0)` 等常量不要每帧重复设置。

---

## 2.3 P1 — 每个 Unit View 每帧直接更新 Actor Transform

`SyncViews()` 对每个 View 分别调用：

- `SetActorHiddenInGame()`；
- `SetActorLocation()`；
- `SetActorRotation()`。

对当前 26 个 Actor 尚可，但如果以后进入 50+ 敌人、弹丸代理、更多特效代理，这类 Game Thread → Scene Primitive 更新会成为实际成本。

### 建议

- 用 `SetActorLocationAndRotation()` 合并；
- 保存上次提交的 View Transform，只有发生可见变化时才提交；
- 屏外单位不需要每帧更新 Render Transform；
- 大规模后再考虑 ISM/Mass/自定义批 View，目前不要为了 26 个单位过度架构。

---

## 2.4 P1 — HUD 是每帧完全即时重绘

`ADemoHUD::DrawHUD()` 每帧重新绘制：

- 全部固定 UI；
- 网格；
- `B.Obstacles`；
- Hazard 圆（每个 Hazard 32 条线）；
- 所有可见单位 Aim 线；
- 所有激光 Trace；
- HP 条、ID、按钮、文字。

当前地图只有约 50 个障碍，因此还不算巨大，但它已经是一个随地图/特效增长的线性路径。若 DEMO-05 真正变为无限/流式竖井，不能继续每帧遍历整个累计障碍集合再靠屏幕坐标裁掉。

### 建议

- DEMO-04：只遍历当前可见 Row/Segment；固定 UI 可继续 Canvas，不必立即重写 UMG；
- Hazard 圆的 32 个 sin/cos 单位向量预计算；
- 对静态网格/边框考虑缓存或至少减少重复字符串格式化；
- `ADemoHUD::Director()` 不要每帧 `TActorIterator`，BeginPlay 后缓存 `TWeakObjectPtr<ADemoDirector>`；
- 当前没有 Hover 逻辑，可关闭 `bEnableMouseOverEvents`。

---

## 2.5 P1 — 障碍查询使用线性 `TArray::Contains`

`Obstacles` 是排序 `TArray<int32>`，但运行时大量使用：

```cpp
Obstacles.Contains(Cell)
```

它出现在：

- `Blocked()`；
- Cover 检查；
- BFS 邻接搜索；
- 地图连通性检查；
- 路径/危险相关逻辑。

当前约 50 个障碍时问题不大，但 `NextCell()` 的 BFS 会放大这个 O(M) 查询。

### 建议

保留：

```text
SortedObstacleList      # 稳定序列、导出、Golden
```

同时增加：

```text
ObstacleOccupancy       # GridWidth × active rows，O(1) lookup
```

例如固定原型可使用 `TArray<uint8>` / `TBitArray<>`。进入 Portable Core 时用普通 vector/bitset 风格结构。

这不会改变规则语义，适合在 DEMO-04 做等价优化并用 Golden 锁住结果。

---

## 2.6 P1 — `ClearRay()` 每次扫描全部障碍

当前每条射线都遍历完整 `Obstacles`，并对每个格做 slab intersection。

同时 `ClearRay()` 被以下路径频繁调用：

- 判断是否可以射击；
- BFS 评估候选射击格；
- 多目标/爆裂筛选；
- 下行移动检查。

在当前小图还能工作；无限地图后复杂度会随累计地图增长，这是必须提前切断的增长路径。

### 建议

改成 **grid DDA / supercover traversal**，只检查射线实际穿过的格子：

```text
ray endpoints
   ↓
traverse crossed grid cells
   ↓
ObstacleOccupancy[cell]
```

需要保留现在的“擦边/碰角视为阻挡”语义，并加回归测试。

---

## 2.7 P2 — `Find()` 和目标搜索是线性扫描

`Find(Id)` 使用 `Units.FindByPredicate()`；目标选择和伤害汇总中还存在多次全单位扫描。

当前 N≤26，没有必要立即优化成复杂 ECS。可以在 DEMO-04 之后按 profiler 决定是否增加：

```text
EntityId -> UnitIndex
```

该索引不能成为迭代顺序真相源，仅用于查找。

---

## 2.8 P2 — 高倍速时 catch-up 上限会形成 backlog

`FClock::Advance()` 每帧最多执行 8 个 sim step。

正常 1× 没问题；10× 且渲染掉到约 20 FPS 以下时，一帧需要的逻辑步可能超过 8，`Remainder` 会继续积累。这样不会破坏确定性，但会形成“游戏追不上目标倍速”的积压，并可能让操作感觉延迟。

### 建议

- 暴露 `SimBacklogSeconds` 调试指标；
- 10× 只作为测试/快进模式，不作为性能验收主模式；
- 若 backlog 连续超过阈值，UI 显示 `SIM BEHIND`，而不是静默积压；
- 不建议为了追赶而取消 fixed-step 或无限放开 catch-up。

---

## 2.9 GPU 配置不是当前第一嫌疑

`DefaultEngine.ini` 中：

```text
r.DynamicGlobalIlluminationMethod=0
r.ReflectionMethod=0
r.RayTracing=False
```

单位和 Deck 也显式 `SetCastShadow(false)`。因此当前简单 DEMO 的“卡”不应优先归因于 Lumen / RT。

项目仍启用了 Nanite、Substrate、Virtual Shadow Map 等工程能力，但在当前极简场景里，更可能影响 shader/Editor 成本，而不是解释 20 Hz 的单位跳动。

最终仍应通过 Unreal Insights / `stat unit` / `stat game` / `stat gpu` 证实，而不是只靠静态推断。

---

# 3. 后续版本建议

| 版本 | 主题 | 主要目的 | 是否应改变 Golden gameplay |
|---|---|---|---|
| **DEMO-04** | Smooth View + Performance Baseline | 消除视觉卡顿，建立 profiling 基线，切断明显 O(N/M) 热路径 | **不应** |
| **DEMO-05** | Endless Shaft Streaming | 从有限 9×65 转为确定性分段生成、窗口化地图与 Actor/View 生命周期 | 会，新增 `demo-5` |
| **DEMO-06** | Ballistic Readability | 建立武器/词缀的肉眼可辨识弹道语言和 FX pooling | 规则按 Contract 决定 |
| **DEMO-07** | Squad Coordination | 5 人以内的目标/站位分工、减少扎堆与过度集火，提高自动战斗可读性 | 会 |
| **DEMO-08** | Resource + Ability Loop | 能量/热量/技能与三选一真正形成 Build 行为差异 | 会 |
| **DEMO-09** | Run Persistence + Migration Proof | Run 状态保存，并完成 C++ → TypeScript 代表切片 Golden 对照 | 不应改变既有规则结果 |

---

# 4. DEMO-04 — Smooth View & Performance Baseline

## 4.1 目标

1. 规则仍固定 50 ms / 20 Hz；
2. 60/120/144 Hz 渲染时单位位置、Aim、镜头运动视觉连续；
3. 不改变 `demo-3` 的任何规则结果和 Golden；
4. 建立可重复的 CPU/GPU profiling 基线；
5. 先处理已确认的冗余，再决定是否需要更重的架构优化。

## 4.2 推荐实现

### View snapshot

新增非权威 View 数据：

```text
FRenderUnitState
  id
  previousX / previousY / previousAim
  currentX  / currentY  / currentAim
  visible
```

每次 sim step 后只更新 snapshot；每个 render frame：

```text
alpha = clamp(Clock.Remainder / StepSeconds)
position = lerp(previous, current, alpha)
aim = previous + shortestAngleDelta(previous, current) * alpha
```

### Sync once per frame

```text
Tick()
  Advance fixed simulation 0..N steps
  Update RenderCamera
  SyncViews(alpha)      # exactly once
```

### O(1) obstacle occupancy

```text
ObstacleList       # stable/sorted
ObstacleMask       # O(1) lookup
```

### Ray traversal

将 `ClearRay()` 从“扫描全部障碍”改为“遍历射线经过的格”。

### HUD visible-window culling

```text
firstVisibleRow
lastVisibleRow
segment index
```

只遍历对应段的障碍/场景元素。

### Profiling scopes

建议加：

```cpp
TRACE_CPUPROFILER_EVENT_SCOPE(QT_BattleStep);
TRACE_CPUPROFILER_EVENT_SCOPE(QT_AdvanceCover);
TRACE_CPUPROFILER_EVENT_SCOPE(QT_NextCell);
TRACE_CPUPROFILER_EVENT_SCOPE(QT_ClearRay);
TRACE_CPUPROFILER_EVENT_SCOPE(QT_SyncViews);
TRACE_CPUPROFILER_EVENT_SCOPE(QT_DrawHUD);
```

必要时再加 CSV stats；不要先写自制 profiler。

## 4.3 验收

### 规则

- UE Automation 现有 5/5 继续通过；
- CTest 2/2；
- 8 份 `continuous-*` Golden **逐字段不变**；
- 30/60/120 FPS `EventHash` 不变；
- View interpolation 开关 on/off 不改变任何 `FBattle` 状态。

### 表现

- 录制 60 FPS 和 120/144 FPS 运动片段；
- 单位移动不再以 50 ms 阶梯跳变；
- Aim 转向不再以每 tick 9°离散跳变；
- Camera 与 Unit 的相对运动无明显抖动。

### 性能

在 720×1280、5 人、完整三波 Actor 全预生成情况下记录：

```text
1×：Game Thread / GPU / Frame p50, p95, max
10×：QT_BattleStep、QT_NextCell、QT_ClearRay、QT_SyncViews p95
DrawHUD p95
Sim backlog max
```

推荐暂定门：

- 1× 渲染稳定达到 60 FPS；
- warm-up 后项目自身逻辑不产生周期性 >33 ms spike；
- 1× 不出现持续 sim backlog；
- 10× 若达不到真实 10 倍速，必须明确显示 backlog，而不能表现为无解释卡顿。

硬件相关的绝对毫秒预算待第一次 Insights 数据后再锁定。

---

# 5. DEMO-05 — Endless Shaft Streaming

DEMO-03 已验证“连续坐标 + 不按波重建 Actor”，下一步才适合做真正的无限/长程竖井。

## 5.1 地图

从：

```text
one 9×65 map
```

改为：

```text
SegmentIndex -> deterministic segment seed
              -> Segment DTO
              -> obstacle mask / spawn anchors / hazards
```

只保留：

```text
camera 前方 K 段
当前段
camera 后方 H 段
```

超出保留窗口的 Segment 可卸载 View；需要保存的规则结果压缩成 plain DTO。

## 5.2 确定性

段生成不能依赖“玩家什么时候走到这里”改变随机序列。建议：

```text
segmentSeed = Hash(runSeed, segmentIndex, generatorVersion)
```

相同 seed + segmentIndex 必须生成相同地图。

## 5.3 View 生命周期

- Unit gameplay identity 与 View Actor 分离；
- 普通远端敌人的 View 可回收/重建；
- 活跃窗口内使用对象池，避免 Spawn/Destroy spike；
- DEMO-03 的“正常波次不重建”验证应升级为“重建 View 不改变 gameplay entity”。

---

# 6. DEMO-06 — Ballistic Readability

对应 REQUIREMENTS V-02。

重点不是先增加大量武器，而是建立最小视觉语法：

```text
direct
pierce
multishot
bounce
blast
```

规则输出保持 `ShotPlan/ShotSemantic`，View 仅消费：

```text
visualProfileId
source/target
fireTick/impactTick
semantic hints
```

实现：

- pooled beam / projectile / impact proxy；
- 同一种规则 semantic 可换不同视觉 Profile；
- 关闭全部 FX 后 Golden 不变；
- 不用 Actor projectile collision 决定伤害。

---

# 7. DEMO-07 — Squad Coordination

对应 REQUIREMENTS V-01；规模限制在 2–5 人，因此不需要重型群体 AI。

建议引入中立的 Squad Coordinator：

```text
Battle facts
  ↓
Generate candidate intents
  ↓
score per agent
  ↓
reserve target / tactical cell
  ↓
individual movement/action
```

首版只解决：

- 多名友军不要无意义扎在同一区域；
- 不要所有人长期过度集火同一目标；
- 攻击位/避险位有 reservation；
- 同一危险出现时队员分散到不同安全格；
- 角色当前行为在 HUD 上可解释。

不要一开始引入通用 BT 编辑器或完整 EQS。若用 StateTree/EQS 做 UE 参考实验，必须继续遵守现有 ARC-012/G6：中立 Contract 是真相源。

---

# 8. DEMO-08 — Resource & Ability Loop

当前 JSON 的：

```text
energy = null
heat = null
skillCasts = null
```

在该阶段变为真实规则。

目标：让协议/装备的差异不仅是“伤害数字变大”，而是改变：

- 射击节奏；
- 目标数量；
- 风险偏好；
- 热量/能量管理；
- 技能触发窗口；
- 可观察的弹道行为。

三选一继续不暂停模拟。

---

# 9. DEMO-09 — Save / Migration Proof

完成一次真实 Run 状态序列化：

```text
schemaVersion
runSeed
segmentIndex
squad state
selected protocols
resource state
persistent unit state
```

随后执行现有 P1.5 要求：选一个代表切片，例如：

```text
RNG + Target Selection + one BattleStep
```

独立重写为 TypeScript，用同 fixture / seed / command sequence 对比 Golden。

这一步通过后，UE 原型才算真正证明“可迁移”，而不是只在文档上保持解耦。

---

# 10. 推荐实际执行顺序

```text
DEMO-04A  Render interpolation
    ↓
DEMO-04B  Insights baseline + redundant Sync cleanup
    ↓
DEMO-04C  obstacle mask + grid ray traversal
    ↓
重新测量
    ↓
DEMO-05   streaming shaft
    ↓
DEMO-06   ballistic readability
    ↓
DEMO-07   squad coordination
    ↓
DEMO-08   energy / heat / ability
    ↓
DEMO-09   save + TS migration proof
```

不要在 DEMO-04A 前增加更多屏幕单位或无限地图，否则会把“固定步长视觉抖动”和“真实性能瓶颈”混在一起，后续很难测量。

---

# 11. DEMO-04 建议修改文件

```text
Source/QiantongUE/Public/DemoRuntime.h
Source/QiantongUE/Private/DemoRuntime.cpp
Source/QiantongUE/Public/DemoBattle.h
Source/QiantongUE/Private/DemoBattle.cpp
Source/QiantongUE/Private/Tests/DemoBattleTests.cpp

docs/spec/DEMO_RULES.md        # 仅记录“规则未变；View interpolation 非权威”
docs/ARCHITECTURE.md           # 增加 sim snapshot -> render interpolation
docs/TASK_BOARD.md             # DEMO-04 卡和 profiling evidence
docs/exec/CHANGELOG.md
docs/exec/DEMO_HANDOFF.md
```

如果 DEMO-04 保证 `FBattle` 规则结果完全不变，则不要创建新的 gameplay Golden version；只增加 View/perf evidence。

---

# 12. 最低 profiling 证据模板

```text
Hardware:
  CPU:
  GPU:
  Monitor refresh:
  PIE/Standalone:
  Resolution: 720x1280

Scenario:
  allies: 5
  preplaced enemies: 21
  seed: 1
  protocol: 3
  speed: 1x / 10x

Frame:
  FPS p50 / p95
  Frame ms p50 / p95 / max
  GameThread ms p50 / p95 / max
  GPU ms p50 / p95 / max

Scopes:
  QT_BattleStep p95
  QT_AdvanceCover p95
  QT_NextCell p95
  QT_ClearRay p95
  QT_SyncViews p95
  QT_DrawHUD p95

Simulation:
  sim steps/frame max
  backlog seconds max
  eventHash
```

只有拿到这份数据以后，才值得讨论是否需要 Mass、ISM、异步路径搜索或更重的并行化。
