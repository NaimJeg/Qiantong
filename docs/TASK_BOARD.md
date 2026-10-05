# TASK_BOARD — UE5 Prototype

> 📌 状态：**现行**
> 状态：`TODO / DOING / BLOCKED / DONE`。
> “DONE”必须附验收证据，不接受“代码已写”作为唯一证据。

## P0 — Portable Core / 工程初始化

| ID | 状态 | 任务 | 完成定义/证据 |
|---|---|---|---|
| P0-00 | DONE | 确认/创建 UE 项目版本与 git 基线 | UE 5.8.1 由 Build.version/已有日志确认，保留原工程关联；本卡提交建立 Git 基线，规范文档入口与入库清单已核验。证据见 `docs/exec/CHANGELOG.md` 的 P0-00 记录；G0 模板依赖为基线红项，未宣称构建/运行验收。 |
| P0-01 | TODO | 建立目录与 `QiantongCore` / `QiantongUE` 模块 | Build 成功；ARCHITECTURE 刷新为真实目录 |
| P0-02 | TODO | 建 Core Boundary 检查 | 人工造一个 UE include 能让检查失败；撤销后通过 |
| P0-03 | TODO | 建 `Types + EntityId + deterministic RNG + SimClock` | 单测；固定 seed 序列锁定；SIM_STEP 测试 |
| P0-04 | TODO | 建 `BattleState/UnitState/Config/Command/Event/Snapshot` 最小 DTO | Core 无 UE 类型；可构造 fixture |
| P0-05 | TODO | 实现最小 `BattleSimulation::Step` | 2vN fixture 自动收束；重复执行 hash 一致 |
| P0-06 | TODO | 建 Golden Scenario 输出格式 | 同 scenario 重跑逐字段一致 |
| P0-07 | TODO | 建 UE BattleHost fixed-step Bridge | 改渲染帧率不改变 Golden 结果 |
| P0-08 | TODO | 建 Debug ViewProxy（无正式美术） | Snapshot 能重建单位；删除全部 View 后 Core 继续推进 |

## P1 — Battle Sandbox

| ID | 状态 | 任务 | 完成定义/证据 |
|---|---|---|---|
| P1-01 | TODO | 射程轴/接近/交战/撤位的基础空间逻辑 | 自动测试覆盖 range 1–4；无 UE world coord |
| P1-02 | TODO | Target policy + TargetChanged event | 确定性；同 seed 同目标序列 |
| P1-03 | TODO | 可移植 AI 六态 evaluator | idle/approach/engage/reposition/flee/dead 覆盖 |
| P1-04 | TODO | 基础攻击/伤害/死亡 | 2vN 收束；无 Actor collision 参与 |
| P1-05 | TODO | `ShotPlan` + visual profile 契约 | Hit 结果不依赖视觉弹丸 |
| P1-06 | TODO | 能量/热量最小战斗资源 | 规则测试；UI 尚可缺省 |
| P1-07 | TODO | 技能 Host 最小链路 | 至少角色/机甲/敌方之一跑通，结构可扩展三宿主 |
| P1-08 | TODO | 即时三选一 Command/Choice 状态 | 战斗模拟不停；选择改变后续规则结果 |
| P1-09 | TODO | Battle Lab 基础控制 | seed、重开、暂停 host、单步、倍率、双方配置 |
| P1-10 | TODO | 5vN Debug 战斗完整收束 | 屏上能辨认阵营/目标/主要事件；有截图/录屏证据 |

## P1.5 — Migration Proof（P2 前硬门）

| ID | 状态 | 任务 | 完成定义/证据 |
|---|---|---|---|
| P15-01 | TODO | 将 RNG + 一段 Targeting/BattleStep 重写为纯 TS | 不引入 Cocos API |
| P15-02 | TODO | 同一 Golden fixture 比对 C++ 与 TS | 关键结果逐字段等价；差异有明确数值规则 |
| P15-03 | TODO | 复核 C++ API 是否过度依赖 C++ 技巧 | 发现迁移痛点先重构再进入 P2 |

## P2 — Ballistic Readability（暂不展开）

P1.5 通过后才开始正式 Niagara/Sprite 弹道实验。目标是验证 Build 差异的视觉可读性，不是堆 FX。

## 卡片执行模板

开始一张卡前：

```text
[ ] 对应 ledger 条目已定
[ ] git status 已检查
[ ] 知道会改哪些文件
[ ] 先写失败测试/反证
```

收尾：

```text
[ ] 自动测试通过
[ ] Core Boundary 通过
[ ] UE build（若环境可用）通过
[ ] 表现任务有运行证据
[ ] TASK_BOARD 写证据
[ ] ARCHITECTURE 如有结构变化已同步
[ ] CHANGELOG 已写
```
