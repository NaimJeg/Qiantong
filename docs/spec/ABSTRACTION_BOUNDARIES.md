# 千瞳原型抽象边界

本文说明当前 UE 原型中哪些数据决定玩法、哪些代码负责调度与显示，以及正式迁移前需要抽出的最小契约。依据为 [现行执行条目](PROTOTYPE_LEDGER.md)、[演示规则](DEMO_RULES.md) 与 [当前架构](../ARCHITECTURE.md)。截至 2026-10-10，玩法仍在 QiantongUE，QiantongCore 仅有独立库基础；本文不表示 Portable 战斗实现已经完成。

## 当前职责和数据流

```text
Controller / HUD 输入
        ↓ Choose → PendingChoice
ADemoDirector：帧时间、暂停、倍速、固定步调度
        ↓ 每步 50ms，FBattle::Step(Choose)
FBattle：单位、地图、随机、选择、波次、命中和胜负
        ↓ 只读状态 / LastShots
FRenderSnapshot：前后样本、位置与最短角差插值
        ↓ 每帧统一同步
UnitView / Camera / HUD / 激光表现
```

| 现有落点 | 所有权与职责 | 抽象边界 |
|---|---|---|
| `DemoBattle.h/.cpp` 的 `FBattle` | 集中推进所有单位，拥有规则状态 | 不读取 Actor、World、物理碰撞或墙钟；当前仍依赖 UE 容器、数学、JSON、性能宏 |
| `FClock` 与 `ADemoDirector` | 将帧时间转换为固定模拟步；管理暂停、倍速、待处理选择 | 帧率影响显示与积压，不能变成可变规则步长 |
| `DemoRender.h` 的 `FRenderSnapshot` | 保留两个非权威表现样本 | 插值结果不能回写单位坐标、目标、瞄准或 HP |
| `ADemoUnitView`、HUD、相机 | 消费状态和激光端点；映射到世界或屏幕 | Actor 不独立推进规则，碰撞与动画回调不结算伤害 |
| 五个 Demo Blueprint | 原生类、资产和表现配置 | 无业务图表；启动、Tick 和交互由 C++ 实现 |
| `QiantongCore` | 纯 C++ 版本接口、独立构建及测试 | 不得因为迁移准备引入 UE 依赖 |

这里的 Host 和 View 是职责边界，当前主要合置于 `DemoRuntime`；不能将目标目录结构误写成已有模块。

## 需要保留的权威状态

迁移时以 `FBattle` 的实际字段为清单，按值表达，而不是序列化 UObject 图。

| 状态集合 | 当前字段或内容 | 迁移要求 |
|---|---|---|
| 单位 | `Id`、`Generation`、阵营、HP、伤害、间隔、X/Position、Target、NextShot、Aim、Waypoint、Engaged、Action | 保留稳定 ID、数组顺序和所有影响下一步的字段；指针仅能作局部查找结果 |
| 战斗进度 | Tick、Choice/ChoiceTick、Winner、AllyCount、Wave/WaveStartTick、WavesCleared、DescentTicks、Entering | 保留阶段边界与命令接收时机 |
| 连续空间 | 障碍列表、CameraY、Relocations | CameraY 参与画外判定和入场，属于规则进度；插值后的 RenderCameraY 才是表现状态 |
| 危险 | X、Y、ImpactTick | 到期伤害仍按模拟 tick 处理，不由特效计时 |
| 随机及统计 | Seed、Rng、EventHash、Shots、TargetSwitches、避险与危险计数 | 随机状态、调用顺序和事件顺序需要完整保留 |
| 当步输出 | LastShots 的 Source/Target/Damage/Kind 与起终点 | 即时命中的表现依据；当前没有在途规则弹丸 |

障碍排序列表是地图真相源，585 格 `ObstacleMask` 是可重建缓存；只能通过生成地图或 `SetObstacles` 同步更新。缓存不另设可写配置。View registry、Actor 指针、Transform、插值样本、音画资源引用均不进入规则状态。

当前 `ResultJson()` 导出的是结算证据，缺少完整单位内部状态、地图和危险等恢复信息，**不能当作完整存档或中途恢复快照**。当前 `Event()` 更新摘要 hash，并未提供完整可回放的事件流。

## 最小迁移契约

以下是正式抽离时的接口职责，不要求当前立即建立同名类或通用框架。

| 契约 | 输入 | 输出与约束 |
|---|---|---|
| Reset | 队伍配置、seed、规则版本 | 独立初态；当前队伍只支持 2/5，seed 0 归一为 1 |
| Step | 一份权威状态、该步命令 | 下一状态、当步事实；固定 50ms，不读取外部时钟 |
| Command | 生效 tick、选择值；未来多命令才增加序号 | 明确合法性与稳定顺序；无效、重复、结束后命令保持现有语义 |
| Snapshot | 当前规则状态 | 不含引擎对象的只读值；完整恢复快照和用于显示的投影可分别定义 |
| Event | tick、顺序、事件类型、稳定 ID、数值 | 描述已发生的事实；View 可丢失或重建，不影响规则结果 |
| Result | 已结束状态 | 保留现有结果字段、null 语义、单位顺序和 hash 编码 |

现有 `FBattle::Step` 在增加 Tick 前检查 `CanChoose()`，随后记录接受命令的 `ChoiceTick`。重放一条生效于 tick T 的命令时，应在状态 tick T−1 上提交给 Step，不能把 UI 按键帧直接当作生效 tick。未来若增加队列，必须测试该边界，不能顺便改变现有行为。

当前数值来自 C++ fixture 与常量；Blueprint 不是第二份玩法配置。未来引入中立 JSON schema 后，由 Host 解析、校验再转换为规则 DTO，避免 JSON 与代码两份数值同时可写。

## 固定步与表现生命周期

`FClock` 以 0.05 秒为一步，倍速限制在 1–10，每次 Advance 最多返回 8 步，剩余时间留作积压；暂停及非法帧增量不推进。迁移应保留积压，不以丢 tick 消除卡顿。单步直接显示当前态，暂停保持已显示的插值时刻。

规则使用 milli-grid，1000 为一格；瞄准使用整数毫度。UE 世界坐标与屏幕像素仅属于映射层。DEMO-04 将单位、血条、Aim 和镜头放在同一插值时刻；重开、身份变化、调试重建及画外重布重置样本，避免显示穿越地图的插值轨迹。

连续三波开局已生成全部单位。正常波次保留 Actor 身份；正式 Cocos View 同样需要稳定 ID 到节点的映射。调试销毁再重建表现只能验证隔离，不得借此重置 HP、冷却、目标或波次。

## 当前边界的缺口

- `FBattle` 仍使用 `CoreMinimal.h`、`TArray`、`FMath`、`FString` 和 UE JSON。无 View 的 UE 自动化测试不能证明无 UE 可运行。
- Aim 的 atan2、取整及射线浮点边界只已有 Win64 证据。跨语言逐 tick 对齐尚未通过。
- 结算 hash 不能定位第一处状态分歧；迁移需要增加完整状态和事件的诊断输出。
- 能量、热量、技能、存档与养成仍后置。结果中的 null 表示未实现，不是数值 0。

按 [迁移执行计划](../plan/UE_TO_COCOS_MIGRATION.md) 逐项关闭这些缺口。Portable、G6 和 TypeScript 对照仍在正式进入 Cocos 前执行，不前移为当前 UE 演示卡的完成条件。
