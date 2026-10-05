# INITIALIZATION — Codex 第一次进入 UE 原型仓库

> 📌 状态：**现行计划**
> 目标：建立“能安全继续长”的工程骨架，不追求第一天看起来像游戏。

---

## 0. 绝对原则

第一次执行只做 P0 基础设施。不要：

- 先画完整 HUD；
- 用大规模特效制作替代规则验收；
- 绕过中立契约，把 Behavior Tree 等 UE 资产作为唯一规则定义；
- 先搬上游 Cocos 代码；
- 先批量创建 DataAssets；
- 先做母舰经营；
- 先做 2×2 大地图；
- 先引入第三方 ECS/AI/序列化框架。

初始化的产物应该让后续“难以走错”，而不是“截图很多”。

---

## 1. 仓库事实侦察

Codex 先执行并记录：

```text
git status
find/list repo top-level
读取 *.uproject（若有）
读取 Source/**.Build.cs（若有）
读取 Config/Default*.ini（只查事实，不批量改）
确认 UE minor version
确认现有 module/target 名
```

若已有项目：在现有结构上最小改造，不重建工程。

若没有 `.uproject`：只在 UE 版本已经确认后生成。

**不要依据本计划猜项目名/UE minor。**

---

## 2. 建立最小目录

目标：

```text
Source/QiantongCore/
Source/QiantongUE/
Tests/Core/
Tests/Scenarios/
Tests/Golden/
Tools/CoreBoundary/
Data/
Content/Prototype/
```

UE 自动生成目录遵循 UE 规则；文档中目标结构以实际创建后的仓库为准刷新。

---

## 3. Core module 初始化

要求：

- `QiantongCore` 的业务 `.h/.cpp` 不 include UE headers；
- Build.cs 只是 UBT 宿主，不等于允许业务代码使用 UE；
- 关闭不必要 PCH 依赖或确保不会通过 PCH 偷渡 UE 类型；
- 先建立 `EntityId`、tick、RNG，再建立 Battle。

第一条测试应该故意 include 一个 UE 头，然后确认 Boundary Gate 会红；随后删掉恢复绿色。

这一步比“成功 new 一个 Actor”优先级高。

---

## 4. UE module 初始化

`QiantongUE` 可以依赖 UE modules + `QiantongCore`。

第一批只需要：

```text
BattleHost
DebugBattleStage
DebugUnitView
```

表现使用基础 shape/text 即可。

P0/P1 可使用 Niagara、UMG、Animation Blueprint、Sequencer 等表现能力，也可按 `AGENTS.md` 第 6.1 节接入 GAS、BT/StateTree、EQS、Navigation、Chaos、Mass 等参考后端。按当前闭环需求引入依赖，不预装无关系统。

采用规则参考后端时，先写中立 Contract、最小能力/复刻预算与对照 fixture，再在 `QiantongUE/ReferenceBackends/` 隔离实现。允许先做 UE 探索；Portable 实现及 G6 未通过时不得标记相应规则卡完成。Core 不引入 UE 头、类型或链接依赖；视觉碰撞和动画回调不结算伤害。

---

## 5. Boundary Gate

Codex 实现 `Tools/CoreBoundary` 时至少检查：

1. `Source/QiantongCore/**/*.h|cpp` 不得包含形如：
   - `CoreMinimal.h`
   - `Engine/`
   - `GameFramework/`
   - `UObject/`
   - `Components/`
   - `Navigation`
   - `GameplayAbility`
2. 禁止 UE 反射宏：`UCLASS/USTRUCT/UENUM/UFUNCTION/UPROPERTY/GENERATED_BODY`。
3. 禁止典型 UE 容器/类型前缀清单（保守白名单/黑名单，避免仅靠字符串误报）。
4. 脚本必须支持 CI exit code。
5. 引入参考后端后，检查其 UE 类型/句柄不外泄到中立 Contract、规范配置、存档或 UI 业务 ViewModel；不是禁止 UE Adapter 内出现 UE 类型。

必须做一正一反两个自测：

```text
违规 fixture -> exit != 0
合法 fixture -> exit == 0
```

门不自证，就不算门。

---

## 6. Determinism 基础设施

首批固定：

```text
SimTick: uint64
SIM_STEP_MS: 50
EntityId: uint32/uint64（选定后不随便变）
DeterministicRng: 明确算法 + 明确 32-bit overflow 语义
```

测试：

```text
seed=1 -> 前 N 个输出固定
seed=2 -> 与 seed=1 不同
重跑 -> 完全一致
```

若选择算法涉及 JS 32-bit 位运算，文档写清 `>>> 0` 等未来 TS 语义。

---

## 7. 最小 Battle fixture

第一版只做：

```text
2 allies
3 enemies
HP
attack
range
logical distance
fixed target policy
cooldown
win/lose
```

没有技能、词缀、热量、母舰都可以。

成功条件：BattleSimulation 在无 UE View 的测试里自动收束，并输出稳定结果。

---

## 8. 第一版 UE Bridge

Bridge 做：

```text
Frame delta -> accumulator -> Core Step
Core Snapshot -> DebugUnitView positions/text
SimEvent -> debug log/temporary glyph
```

第一轮测试特意在不同渲染帧率条件下运行同一 scenario，结果应相同。

---

## 9. 初始化完成的退出条件

满足以下条件才将 `P0-01..P0-08` 相应项标 Done：

- Core boundary 真能拦 UE 依赖；
- deterministic RNG 测试锁定；
- 最小 battle 可 headless 收束；
- UE Host 驱动同一 battle Contract；如采用参考后端，Portable 路径仍能独立运行，且对应 G6 对照通过；
- 可以删除全部 Debug View 后继续模拟；
- `ARCHITECTURE.md` 已更新为**真实**目录；
- 无未说明的新第三方依赖。

---

## 10. 给 Codex 的首次执行指令

可直接使用：

> 阅读根 `AGENTS.md`、`docs/REQUIREMENTS.md`、`docs/spec/PROTOTYPE_LEDGER.md`、`docs/ARCHITECTURE.md`、`docs/TASK_BOARD.md` 与本文件。先侦察仓库与 UE 版本，不假定工程为空。只执行 P0：优先建立 Portable Core 边界门、确定性 RNG/SimClock、最小 BattleState/Step/Golden fixture，再建立 UE fixed-step Bridge 与 Debug View。允许按 AGENTS 6.1 使用高级表现系统或隔离的 UE Reference Backend；采用后者须提取中立契约、记录复刻预算并通过 G6。禁止 UE 类型/资产成为规范规则、配置、存档或 UI 业务模型，禁止 Actor 自主结算与视觉碰撞决定伤害。每张规则卡先造失败测试/反证，完成后跑门并把证据写回 TASK_BOARD；结构变化同 commit 更新 ARCHITECTURE。若运行环境无法启动 UE Editor，如实停在静态/编译可验证层，不把整卡标 Done。
