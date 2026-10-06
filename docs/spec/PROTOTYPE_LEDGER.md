# PROTOTYPE_LEDGER — UE 原型执行条目

> 📌 状态：**现行**
> 本表只管理 UE 原型的架构/验证裁决，不复制完整产品 GDD。产品玩法细节仍以上游设计 ledger 为准。
>
> 状态：`已定 / 待决 / 冲突 / 挂起 / 作废`。只有 `已定` 可以直接进入代码。

| ID | 条目 | 状态 | 验收/落点 |
|---|---|---|---|
| DEMO-006 | 连续累计地图、镜头先行；友军完全画外仅纵向重布，镜头停稳后从上边缘走入；敌人提前布置并寻找掩体，由镜头带入；正常波次保留Actor身份 | 已定 | 用户本轮指令及后续澄清；DEMO_RULES v3；DEMO-03 |
| DEMO-004 | 用户授权探索闭环：随机障碍、自动绕行/避险/射击位置、清场下行与波次；即时激光，瞄准最大转速；实验规则详见 DEMO_RULES v2 | 已定 | DEMO-02；覆盖 v1 横向纯表现及无探索范围 |
| DEMO-005 | 本轮及后续 Editor/Live Coding、Terminal/MCP 验收演示可替代重复打包；上一轮包仅证明 v1 | 已定 | 用户本轮指令；Editor 测试、PIE运行及截图证据 |
| DEMO-001 | UE 原生集中模拟 + 蓝图编排，Windows 可打包单场闭环；取代本轮 Portable-first 与 G6/P1.5 前置要求 | 已定 | 用户批准计划；AGENTS 顶部修订；DEMO_RULES.md |
| DEMO-002 | 即时命中，首版到 P3 单视口；实验规则以 DEMO_RULES.md 为本轮依据 | 已定 | 覆盖 BAL-003 待决及 D-UE-003；玩法/运行测试 |
| DEMO-003 | 使用本机已编译 UE 5.8.3，引擎源码只读、允许项目及所需产物编译；不自动改 EngineAssociation | 已定 | 用户已授权补编译并完成 MCP 握手；覆盖 ARC-011 本轮执行版本 |
| ARC-001 | UE5 是原型验证宿主、成熟架构研究对象及参考后端平台，不是最终生产技术栈；最终目标仍为 Cocos Creator 3.8.8 + TypeScript + 移动/小游戏 | 已定 | `AGENTS.md`；不得出现不可替代的 UE-only gameplay semantic |
| ARC-002 | gameplay Core 必须纯 ISO C++，禁止 Unreal headers/types/macros | 已定 | Core Boundary gate |
| ARC-003 | UE Adapter/Reference Backend → 中立 Contract/Core；Host 组合后端；Core 不依赖 UE/View/Platform | 已定 | dependency scan；AGENTS 6.1 |
| ARC-004 | Core 使用 fixed-step；首版 `SIM_STEP_MS=50`，渲染可插值 | 已定 | determinism test |
| ARC-005 | 随机采用项目自有可跨语言复刻 PRNG；同 seed + 输入必须同结果 | 已定 | replay/golden test |
| ARC-006 | 规则实体以稳定 ID 引用；View Actor 不是 gameplay identity | 已定 | destroy/rebuild view test |
| ARC-007 | UI/View 不直接写规则状态，只提交 Command/Application action | 已定 | API boundary |
| ARC-008 | 配置 schema 引擎中立；DataAsset 可作视觉映射或由中立配置生成的参考后端缓存，不做 gameplay truth | 已定 | content loader review；禁止两个可写真相源 |
| ARC-009 | Save 规范为可映射 plain DTO；USaveGame 不作为 schema truth | 已定 | roundtrip test |
| ARC-010 | C++ 业务结构优先可机械重写成 TypeScript，避免以模板/继承技巧表达玩法语义 | 已定 | P1.5 TS port review |
| ARC-011 | 保持现有 UE 5.8.1 源码版工程及原 EngineAssociation，不自动升级/降级 | 已定 | P0-00 仓库事实确认：本机 Engine/Build/Build.version、已有 Editor 日志、Target 的 Unreal5_8 一致；见 ARCHITECTURE 第 0 节 |
| ARC-012 | P0/P1 允许 GAS、BT/StateTree、EQS、NavMesh、Chaos、Mass 等隔离参考后端；采用者须有中立契约、复刻预算、Portable 对照；不用者不强制双后端 | 已定 | 用户本次政策修订；AGENTS 6.1 / G6；DOC-01 |
| ARC-013 | UE 源码用于学习架构，按项目最小需求独立重实现；禁止复制或逐行翻译 Engine Code 到 Portable/Cocos 实现 | 已定 | 研究记录保留来源/版本/采用思想；AGENTS 6.1 |
| BAT-UE-001 | BattleSimulation 集中 Step 全体单位；Actor 不拥有独立规则 Tick | 已定 | headless battle test |
| BAT-UE-002 | 基础空间仍以“格/射程公共刻度”为规则真相；UE 世界坐标只属 View | 已定 | layout mapping test |
| BAT-UE-003 | 开局按上游现行 `CYC-010` 使用 2 人，不使用旧文档“1 人”历史口径 | 已定 | fixture |
| BAT-UE-004 | AI 契约采用 Condition/Selector/Action；P0/P1 可用 BT/StateTree/EQS 参考后端探索，Portable evaluator 承担无 UE 路径；UE 资产不做真相源 | 已定 | deterministic AI tests；采用参考后端时 G6 |
| BAL-001 | 弹道拆分 Rule Semantic 与 Visual Trajectory；UE Projectile collision 不决定命中 | 已定 | collision-independent test |
| BAL-002 | `ShotPlan` 至少具备 source/target/fireTick/impactTick/semantic/visualProfileId | 已定 | unit tests |
| BAL-003 | 本轮即时命中，仅视觉飞行；不引入在途规则弹丸 | 已定 | 用户批准计划；DEMO-002 / DEMO_RULES |
| VIEW-001 | UE 竖屏原型以 720×1280 为设计基准，逻辑布局经 mapper 映射 | 已定 | viewport screenshot |
| VIEW-002 | 左侧角色栏、竖井/战场、下方上涌方向遵循上游现行布局语义 | 已定 | visual evidence |
| VIEW-003 | P0/P1 不使用 Lumen/Nanite/Chaos/NavMesh 形成玩法依赖 | 已定 | architecture review |
| TEST-001 | 每个 Core 关键规则必须能在无正式 View 的 harness/test 中执行 | 已定 | CI/local gate |
| TEST-002 | P1 完成后必须做一次 C++ → TS 代表切片迁移并比对 Golden Scenario | 已定 | P1.5 gate |
| TEST-003 | View/FX 任务除 build 外必须给可复现运行证据 | 已定 | TASK_BOARD evidence |
| TEST-004 | 采用 UE 规则参考后端的卡必须通过 G6 才完成；UE/Portable C++ 对照不能替代 P1.5 C++/TS Golden 门 | 已定 | AGENTS G6；逐 tick 行为/事件及最终结果对照，关闭参考后端独立运行 |

## 使用规则

1. 新的架构/原型裁决先入本表，再实现。
2. 产品玩法条目不要擅自复制进本表；只引用上游 ID。
3. `待决/冲突` 不得落 gameplay 行为；可以搭接口，但禁止 placeholder outcome。
4. 条目状态变化要同步 `DECISIONS.md`：已定后从待决视图移除。
