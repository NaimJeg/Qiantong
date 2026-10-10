# 千瞳从 UE 到 Cocos 的迁移计划

目标为 Cocos Creator 3.8.8、TypeScript 与移动端或小游戏。先冻结已验证的实验规则，抽出纯 C++ 最小实现，再完成不依赖 Cocos 的 TypeScript 对照，最后替换宿主与表现。当前仍处于 UE 原型验证阶段；本计划不启动迁移、不新增玩法，也不将尚未验收的 DEMO-04 标为完成。

现状见 [ARCHITECTURE](../ARCHITECTURE.md)，字段归属与契约见 [抽象边界](../spec/ABSTRACTION_BOUNDARIES.md)。执行优先级遵循 AGENTS 顶部修订与 [PROTOTYPE_LEDGER](../spec/PROTOTYPE_LEDGER.md)，旧 P1.5 前置表述不阻塞当前 UE 原型。

## 迁移范围与文件映射

| 当前代码或资源 | 迁移时的最小处理 | 目标位置示意 |
|---|---|---|
| DemoBattle 的单位、地图、危险、选择与波次 | 移除 UE 类型后形成值状态与集中 Step | Portable Core；随后 `assets/scripts/rules/battle/` |
| XorShift32、角度、距离、射线、BFS | 锁定数值与遍历顺序，独立移植项目算法 | `rules/base/`、`rules/spatial/` |
| FClock、Director 调度 | 保留固定步、输入队列、暂停和积压 | `assets/scripts/host/` |
| ResultJson、未来配置与保存 IO | 规则只提供 DTO，由宿主编码和校验 | `assets/scripts/platform/` |
| DemoRender | 值快照、位置与角度插值 | `assets/scripts/view/` |
| UnitView、Canvas HUD、Camera、激光 | 按可读性需求重建节点、界面和效果 | Cocos 场景、资源与 `view/` |
| Blueprint、umap、材质、Editor 工具 | 作为参考和证据保留，表现资源另行制作或转换 | 不作为规则输入直接移植 |
| Tests/Golden | 保留版本和已批准输入输出 | 共享 fixtures 和对照测试 |

目录为迁移目标，当前仓库尚无这些 TypeScript 模块。不预建双后端框架、通用 ECS 或通用技能系统。现有 DEMO 不依赖 GAS、BT、EQS 等权威后端；若后续采用，再按 AGENTS 6.1 记录具体能力子集、复刻预算和 G6 对照。

## 阶段一 冻结可复现基线

开始实际迁移时选择明确的 commit，并记录规则版本、工具链、fixture、seed、命令及证据文件摘要。当前工作区有未提交的 DEMO-04，不能仅用 HEAD 代表它。版本 `demo-3` 的基线为 `Tests/Golden/continuous-{2,5}-option{0,1,2,3}.json` 共八份；demo-1 与 demo-2 样本保留历史用途。

保留既有自动化中多 seed、障碍连通、即时命中、转向、跨波生命周期及无 View 的覆盖。新增逐 tick 诊断输出时，包含完整单位字段、障碍、危险、随机状态、阶段状态和有序事件，不能只导出赢家或最终 hash。诊断输出保持只读，不引入额外随机调用。

退出条件：选定基线可重复生成相同结果；自动输入与人工输入的接受 tick 能明确重放；DEMO-04 的视觉证据缺口仍由原任务独立收尾。

## 阶段二 抽出纯 C++ 最小规则

按“先边界测试、再最小抽离、再对照”推进：移出 JSON 和性能宏，用标准整数、数组和项目值类型替换 UE 类型；保留原算法、单位顺序、阶段分支和事件调用顺序。Host 承担配置解析、时间累积和文件写入。障碍 mask 从唯一列表重建，不新增第二份地图状态。

先补齐 G0 检查，确保引入 UE include/type 会失败；再在独立 CMake harness 中执行同一 fixture。抽离期间不同时调整平衡、路径策略或数值算法；如果跨平台问题要求改变规则，须先形成新的版本裁决及新旧对照。

退出条件：不加载 UE 的 harness 跑通全部选定场景，与 UE 基线逐 tick 状态、事件顺序及完整结算结果一致。旧 UE 路径与 Portable 对照使用独立状态副本，不得双重结算。仅 CoreSmoke 和 ModuleLayout 通过不足以证明战斗已迁移。

## 阶段三 纯 TypeScript 对照

先实现 RNG、事件 hash 与空间边界，再接 targeting、移动、瞄准、即时伤害、危险及三波生命周期。规则模块不得 import Cocos；使用同一 fixtures 和命令重放，首先找第一处状态分歧，再检查最终结果。

| 风险 | 必须锁定的行为 | 验证样本 |
|---|---|---|
| XorShift32 | 13/17/5 位移、无符号右移、每步 uint32 截断；seed 0→1 | 高位 seed、长随机序列、地图生成后的 rngState |
| 事件 hash | 初值 2166136261、逐字节顺序、乘数 16777619、uint32 回绕 | 负数参数、高位字节；TS 用 `Math.imul` 实现低32位乘法，输出8位小写十六进制 |
| 角度 | 0/360°环绕、正好180°走正向、9000毫度步进、3000容差 | ±180°、0°边界、容差内外一单位 |
| atan2 与取整 | 明确负数及半整数的取整；不能默认不同语言数学库完全相同 | 轴线、象限和取整边界；若改定点或查表，单独版本化 |
| 整数除法与余数 | 核对 C++ 向零截断与负余数语义；逐表达式转换 | 画外负坐标、角度归一、比例伤害 |
| 距离和射线 | 平方距离中间值范围；闭合格擦边/碰角阻挡；浮点判断顺序 | 水平/垂直/反向/零长线段、角点、格边两侧 |
| 搜索和同分 | BFS 邻接下、左、右、上；稳定 ID 与原数组顺序 | 等距目标、多个可达格、死亡目标、未来波敌人 |
| 命令和同 tick 结算 | CanChoose 在 Tick 自增前检查；已生成攻击不因同 tick 死亡取消 | tick99/100/101提交、重复/非法选择、互杀、结束后命令 |
| 波次与镜头 | CameraY 仍为规则状态；画外保X重布、停稳后入场 | 障碍阻挡下行、死亡友军、连续三波、6000 tick边界 |

TypeScript 的 number 不能自动等价于任意 int64。当前有限地图的坐标可用于推导中间值上界；扩展无限地图前必须重新证明安全整数范围或选择显式整数方案。不要用宽松误差放过会改变目标、遮挡、转向或伤害的分歧。

退出条件：代表切片满足 P15-01/02/03，完整计划迁移的 DEMO 子集也逐 tick 对齐。最终比较 ruleVersion、fixture、seed/rngState、winner、simTicks、shots、targetSwitches、cameraY、entering、relocations、wave、wavesCleared、descentTicks、evasionSteps、hazardsCreated、hazardHits、eventHash、remainingHp 与 commands；energy、heat、skillCasts 保持 null。对象键顺序可忽略，数组顺序和数值不可忽略。

## 阶段四 接入 Cocos 宿主与表现

规则通过纯 TS 对照后，再接入帧回调、输入、节点映射、UI 与相机。宿主仍每50ms推进，最多补步数与积压处理按已冻结契约执行。用实际帧序列测试30/60/120FPS、暂停、单步及1–10倍速；比较相同模拟 tick 的权威状态，不把墙钟上的结束帧当作确定性要求。

按稳定 ID 保留三波单位节点；重建表现后能继续同一场战斗。相机和单位用相同插值系数，画外重布清空历史样本，激光使用规则命中端点。删除 UI、节点或特效后，纯规则仍能收束。

退出条件：无 View 对照通过、Cocos 中真实运行证据在位、输入命令和连续空间可读性通过验收。移动端性能预算应在选定真机上另测，不从 UE 桌面帧率推导。

## 保存和内容的后续接入

存档不在当前 DEMO 范围。正式接入时另定 SaveEnvelope/schemaVersion，保存所有影响下一步的状态及随机状态，重建派生缓存；加入中途保存恢复后与连续执行逐 tick 一致的测试。不能直接把 ResultJson 改名当 SaveGame。

内容 schema 使用稳定 ID、明确量纲和版本；资产路径经表现映射解析。编辑器资源不成为第二份可写规则数据库。能量、热量、技能及养成等待已定玩法条目，不因迁移计划预填行为。

## 复现入口与完成记录

现有入口为 `Tools/Build.ps1`、`Tools/TestDemo.ps1`，引擎根显式使用本机 UE 5.8.3；后者当前要求7项 UE 自动化成功且无警告。独立库测试可在已配置的 `build/core` 下通过 CTest 执行。UE 自动化及 PIE 证据说明原型行为，不能替代尚未建立的 Portable 战斗 harness 或 TypeScript 对照程序。

每个迁移闭环完成时，在 TASK_BOARD 附实际命令、版本、测试摘要、首差诊断及证据位置；结构变化同步 ARCHITECTURE，执行结果记 CHANGELOG。某一边界未过就保持未完成状态，保留旧 Golden 与旧入口供定位和回退，不修改历史提交来消除差异。
