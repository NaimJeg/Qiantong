# 执行记录

## 2026-10-05 — [DOC-01][ARC-012][ARC-013][TEST-004] UE 参考后端政策

- 根据用户明确修订，将 P0/P1 对 GAS、BT/StateTree、EQS、NavMesh、Mass、Chaos 等的笼统禁令改为中立接口隔离；保留纯 C++ Core、固定步长、项目随机数、单一权威状态和视觉碰撞不结算伤害等约束。
- AGENTS 新增第 6.1 节与 G6：允许 UE 实验先行、按需双后端；规则卡完成需 Portable 实现和逐 tick/Golden 对照，P1.5 的 C++/TS 门仍必需。参考结果不自动成为正确答案。
- 同步 REQUIREMENTS、ARCHITECTURE、PROTOTYPE_LEDGER、INITIALIZATION、P0-P1、TASK_BOARD 与 DECISIONS；取消 Niagara 的 P2 工具准入限制，保留 P1.5 → P2 的阶段顺序。未更改 BAL-003 等待决玩法。
- 加入复刻预算、按需能力映射与源码研究纪律。核验 Epic 官方源码 FAQ，并在 AGENTS 链接来源；要求独立实现项目子集，禁止复制/逐行翻译 UE Engine Code 到 Portable/Cocos。
- 文档验收：检索旧禁令及阶段工具限制，核对政策引用、Markdown 围栏、相对文档路径和 diff 空白检查；全部通过。只修改 AGENTS/docs，未启用插件、修改源码或配置，未运行 UE 构建或游戏测试。G0 模板依赖基线红项仍在，G6 尚未实现/执行。

## 2026-10-05 — [P0-00][ARC-011] UE 工程与 Git 基线

- 首次检查 `git status --short` 返回“not a git repository”；已有工程与地图均在位，不重新生成工程。
- 已读取 AGENTS、REQUIREMENTS、PROTOTYPE_LEDGER、ARCHITECTURE、TASK_BOARD、INITIALIZATION 和 P0-P1。原先三份文档平铺在 docs，按既定契约移到 `docs/spec/` 与 `docs/plan/`，内容保持不变（ledger 另补 ARC-011 版本事实）。
- 确认 UE 5.8.1：本机引擎 `Engine/Build/Build.version` 为 5/8/1，现有日志报告 `5.8.1-0+UE5`，两份 Target 使用 `Unreal5_8`。保持原 `.uproject`、插件、地图和 C++ 模板。
- 初始化 Git，新增缓存/生成物/本机配置排除，以及 UE 二进制资产属性。共享配置中的 Android 文件服务令牌留空，原配置完整备份到忽略目录 `Saved/LocalBaseline/`。另清理四份模板文本的行尾空白/多余末尾空行，不改变代码或配置语义。
- 验收：规范文档入口存在；配置语义修改仅限令牌；地图、源码、工程描述均入基线；生成物与本机备份排除；`git diff --cached --check` 通过。提交后检查 clean working tree。
- 门状态：本卡不修改规则或表现，未执行新的 UE 构建/运行验收；G0 明确为模板依赖基线红项，G1–G5 尚未建立或验收。已有 Editor 日志只用于版本事实确认，不作为本次运行证据。
- 下一卡为 P0-01：将 UE 模块注册与宿主依赖移出 Portable Core，建立真实双层结构及构建入口。P0/P1 整体尚未完成。
