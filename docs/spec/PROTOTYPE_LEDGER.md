# PROTOTYPE_LEDGER — UE 原型执行条目

> 📌 状态：**现行**
> 本表只管理 UE 原型的架构/验证裁决，不复制完整产品 GDD。产品玩法细节仍以上游设计 ledger 为准。
>
> 状态：`已定 / 待决 / 冲突 / 挂起 / 作废`。只有 `已定` 可以直接进入代码。

| ID | 条目 | 状态 | 验收/落点 |
|---|---|---|---|
| ARC-001 | UE5 是原型验证宿主，不是最终生产技术栈；最终迁移目标为 Cocos Creator 3.8.8 + TypeScript + 移动/小游戏 | 已定 | `AGENTS.md`；不得出现 UE-only gameplay semantic |
| ARC-002 | gameplay Core 必须纯 ISO C++，禁止 Unreal headers/types/macros | 已定 | Core Boundary gate |
| ARC-003 | 依赖单向：UE Adapter/View → Core；Core 不依赖 UE/View/Platform | 已定 | dependency scan |
| ARC-004 | Core 使用 fixed-step；首版 `SIM_STEP_MS=50`，渲染可插值 | 已定 | determinism test |
| ARC-005 | 随机采用项目自有可跨语言复刻 PRNG；同 seed + 输入必须同结果 | 已定 | replay/golden test |
| ARC-006 | 规则实体以稳定 ID 引用；View Actor 不是 gameplay identity | 已定 | destroy/rebuild view test |
| ARC-007 | UI/View 不直接写规则状态，只提交 Command/Application action | 已定 | API boundary |
| ARC-008 | 配置数据 schema 引擎中立；DataAsset 只能做 UE 视觉映射，不做 gameplay truth | 已定 | content loader review |
| ARC-009 | Save 规范为可映射 plain DTO；USaveGame 不作为 schema truth | 已定 | roundtrip test |
| ARC-010 | C++ 业务结构优先可机械重写成 TypeScript，避免以模板/继承技巧表达玩法语义 | 已定 | P1.5 TS port review |
| ARC-011 | 保持现有 UE 5.8.1 源码版工程及原 EngineAssociation，不自动升级/降级 | 已定 | P0-00 仓库事实确认：本机 Engine/Build/Build.version、已有 Editor 日志、Target 的 Unreal5_8 一致；见 ARCHITECTURE 第 0 节 |
| BAT-UE-001 | BattleSimulation 集中 Step 全体单位；Actor 不拥有独立规则 Tick | 已定 | headless battle test |
| BAT-UE-002 | 基础空间仍以“格/射程公共刻度”为规则真相；UE 世界坐标只属 View | 已定 | layout mapping test |
| BAT-UE-003 | 开局按上游现行 `CYC-010` 使用 2 人，不使用旧文档“1 人”历史口径 | 已定 | fixture |
| BAT-UE-004 | AI 首版为可移植 Condition/Selector/Action evaluator；不使用 UE BT/StateTree 做真相源 | 已定 | deterministic AI tests |
| BAL-001 | 弹道拆分 Rule Semantic 与 Visual Trajectory；UE Projectile collision 不决定命中 | 已定 | collision-independent test |
| BAL-002 | `ShotPlan` 至少具备 source/target/fireTick/impactTick/semantic/visualProfileId | 已定 | unit tests |
| BAL-003 | 是否需要“真实飞行时间影响战术”必须由实验决定，P0 不默认引入复杂弹丸实体 | 待决 | 在 P2 Ballistic Lab 比较后裁决 |
| VIEW-001 | UE 竖屏原型以 720×1280 为设计基准，逻辑布局经 mapper 映射 | 已定 | viewport screenshot |
| VIEW-002 | 左侧角色栏、竖井/战场、下方上涌方向遵循上游现行布局语义 | 已定 | visual evidence |
| VIEW-003 | P0/P1 不使用 Lumen/Nanite/Chaos/NavMesh 形成玩法依赖 | 已定 | architecture review |
| TEST-001 | 每个 Core 关键规则必须能在无正式 View 的 harness/test 中执行 | 已定 | CI/local gate |
| TEST-002 | P1 完成后必须做一次 C++ → TS 代表切片迁移并比对 Golden Scenario | 已定 | P1.5 gate |
| TEST-003 | View/FX 任务除 build 外必须给可复现运行证据 | 已定 | TASK_BOARD evidence |

## 使用规则

1. 新的架构/原型裁决先入本表，再实现。
2. 产品玩法条目不要擅自复制进本表；只引用上游 ID。
3. `待决/冲突` 不得落 gameplay 行为；可以搭接口，但禁止 placeholder outcome。
4. 条目状态变化要同步 `DECISIONS.md`：已定后从待决视图移除。
