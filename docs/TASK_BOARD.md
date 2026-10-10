# TASK_BOARD — UE5 Prototype

> 📌 状态：**现行**
> 状态：`TODO / DOING / BLOCKED / DONE`。
> “DONE”必须附验收证据，不接受“代码已写”作为唯一证据。

## 仓库发布

| ID | 状态 | 任务 | 验收 |
|---|---|---|---|
| REPO-01 | DONE | 保留本地历史，将当前原型初始化到 GitHub NaimJeg/Qiantong | 原型提交 `44af531` 已推送至 origin/master，ls-remote 核对一致；README、源码、资产及证据在位；Core 构建及 CTest 2/2、暂存差异空白检查通过 |

## 当前闭环：竖井探索演示

| ID | 状态 | 任务 | 验收 |
|---|---|---|---|
| DEMO-04 | DOING | 位置/Aim/镜头帧级插值、单次View提交、障碍查询等价优化、HUD裁剪及性能基线 | DEMO-007；Editor构建、Core2/2、UE自动化7/7、八份demo-3 Golden一致、五纯数据蓝图编译；四场PIE完整Golden/26 Actor身份通过。smooth-performance.json及smooth-view-samples.json：60/120FPS、10×无backlog、每帧同步1次。60/120FPS视频和目视复核未完成：全编辑器截图被自动审批拒绝，已询问仅限PIE本地录制许可，待答复；未标DONE |
| DEMO-03 | DONE | 连续竖井、镜头先行、友军画外保X重布/停稳后走入；敌人提前布置找掩体，由镜头带入；Actor跨波生命周期 | DEMO-006；Editor最终构建成功、Core 2/2、UE自动化5/5；5纯数据蓝图审计；两人/五人PIE完整结果与Golden一致，23/26 Actor身份不变，波次间创建/销毁0。docs/exec/evidence/continuous-acceptance.json、continuous-*.png及result.json；Saved/Logs/Continuous-FinalBuild.log、Continuous-FinalTests.log、Continuous-Editor.log。未重复打包，旧包仍v1 |
| DEMO-02 | DONE | 即时激光限速瞄准、随机障碍/自动避险及三段下行探索 | Editor 构建及 Live Coding 成功；CTest 2/2、UE Automation 4/4 无警告，24 seed/8 Golden；5 个纯数据蓝图编译审计；MCP PIE 两人/五人结果逐字段对齐，View重建、鼠标/键盘换seed与暂停通过；四张运行截图。证据 docs/exec/evidence/explore-acceptance.json、explore-*.png；Saved/Logs/Explore-FinalBuild.log、Explore-FinalTests.log、Explore-Editor.log；重启后 MCP/PIE 再次通过。按 DEMO-005 不重复打包，旧包仍为 v1 |
| DEMO-01 | DONE | 集中模拟、演示关卡/蓝图、交互 HUD、Windows 独立包 | UE Automation 2/2、CTest 2/2、5 个纯数据蓝图审计通过；最终 BuildCookRun ExitCode=0；独立包 2v6/5v6 完整 JSON 与 Tests/Golden 一致；鼠标/键盘输入实测通过，用户确认“可用”。见 CHANGELOG、DEMO_HANDOFF、docs/exec/evidence/demo-combat.png；日志 Saved/Logs/Demo-FinalPackage.log、Demo-PackagedSmoke.log、Demo-InputEvidence.json |

本轮由 DEMO-01 承接旧 P0-04..08、P1-01/02/04/05/08/09/10 的最小子集；其余规则后置，不将旧卡误标 DONE。P1.5/G6 改为进入 Cocos 前执行。旧表保留历史，不作为当前优先顺序。2026-10-06 项目 Editor 补编译与 CTest 2/2、MCP 握手已通过，证据 Saved/Logs/ProjectBuild-MCP.log 和 McpHandshake.json。

## 文档契约修订

| ID | 状态 | 任务 | 完成定义/证据 |
|---|---|---|---|
| DOC-02 | DONE | 项目文件权限修复，补齐当前抽象边界与 UE→Cocos 迁移计划 | 两个依赖目录恢复继承，.git 及其余沙箱所有者恢复 RNaim；全树 ACL 校验 21116 项、0失败；Core CTest 2/2，4份文档/19个本地链接及8份 demo-3 fixture 检查通过，diff 空白检查通过。见 spec/ABSTRACTION_BOUNDARIES.md、plan/UE_TO_COCOS_MIGRATION.md、exec/evidence/permissions-docs-20261010.json；仅权限与文档验收，保留 DEMO-04 状态和既有改动 |
| DOC-01 | DONE | 以语义隔离替代 UE 高级系统笼统禁令 | AGENTS 6.1/G6、ARC-012/013、TEST-004 及需求/架构/计划同步；旧禁令检索与 diff 检查通过；详见 CHANGELOG。本卡仅文档验收，不代表后端实现或运行门通过。 |

规则卡统一附加条件：若采用 UE Reference Backend，按 `AGENTS.md` G6 补齐 Portable 实现与双后端对照后才能 DONE；实验先行阶段记 DOING。未采用者不要求双后端；P1.5 的 Portable C++/TS 对照不豁免。

## P0 — Portable Core / 工程初始化

2026-10-05 审查：开发前回退提交为 `436cd8f`，当时工作区干净，故使用空提交。源码核对仅存在 UE 模板，P0 原有完成度为 1/9（P0-00），P1 为 0/10；没有可支持“P0 已完成”的规则或运行证据。用户随后明确本轮先完成 P0-01。仓库缺少上游 `03-设计交接/spec/ledger.md`，P1 玩法细节不能由计划中的建议自行补定；P0-02/03 工程基础不受此缺口阻塞。

| ID | 状态 | 任务 | 完成定义/证据 |
|---|---|---|---|
| P0-00 | DONE | 确认/创建 UE 项目版本与 git 基线 | UE 5.8.1 由 Build.version/已有日志确认，保留原工程关联；本卡提交建立 Git 基线，规范文档入口与入库清单已核验。证据见 `docs/exec/CHANGELOG.md` 的 P0-00 记录；G0 模板依赖为基线红项，未宣称构建/运行验收。 |
| P0-01 | DONE | 建立目录与 `QiantongCore` / `QiantongUE` 模块 | 2026-10-06 在用户授权的 UE 5.8.3 下补齐完整 Editor 构建、项目启动、MCP 握手及 CTest 2/2；模块清单由 UBT 生成，后续 DEMO-01 继续通过。此前 5.8.1 阻塞记录保留于 CHANGELOG。 |
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

P1.5 通过后进入系统性的弹道读感验证。P0/P1 已可使用 Niagara/Sprite 等表现工具；阶段门限制的是扩展 P2 工作，不是工具使用。目标是验证 Build 差异的视觉可读性。

## 卡片执行模板

开始一张卡前：

```text
[ ] 对应 ledger 条目已定
[ ] 若引入参考后端，已记录中立契约、复刻预算与对照场景
[ ] git status 已检查
[ ] 知道会改哪些文件
[ ] 先写失败测试/反证
```

收尾：

```text
[ ] 自动测试通过
[ ] Core Boundary 通过
[ ] 若采用 UE 规则参考后端，G6 对照与关闭后端的 Portable 路径通过
[ ] UE build（若环境可用）通过
[ ] 表现任务有运行证据
[ ] TASK_BOARD 写证据
[ ] ARCHITECTURE 如有结构变化已同步
[ ] CHANGELOG 已写
```
