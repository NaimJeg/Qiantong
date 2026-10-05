# 执行记录

## 2026-10-05 — [P0-00][ARC-011] UE 工程与 Git 基线

- 首次检查 `git status --short` 返回“not a git repository”；已有工程与地图均在位，不重新生成工程。
- 已读取 AGENTS、REQUIREMENTS、PROTOTYPE_LEDGER、ARCHITECTURE、TASK_BOARD、INITIALIZATION 和 P0-P1。原先三份文档平铺在 docs，按既定契约移到 `docs/spec/` 与 `docs/plan/`，内容保持不变（ledger 另补 ARC-011 版本事实）。
- 确认 UE 5.8.1：本机引擎 `Engine/Build/Build.version` 为 5/8/1，现有日志报告 `5.8.1-0+UE5`，两份 Target 使用 `Unreal5_8`。保持原 `.uproject`、插件、地图和 C++ 模板。
- 初始化 Git，新增缓存/生成物/本机配置排除，以及 UE 二进制资产属性。共享配置中的 Android 文件服务令牌留空，原配置完整备份到忽略目录 `Saved/LocalBaseline/`。另清理四份模板文本的行尾空白/多余末尾空行，不改变代码或配置语义。
- 验收：规范文档入口存在；配置语义修改仅限令牌；地图、源码、工程描述均入基线；生成物与本机备份排除；`git diff --cached --check` 通过。提交后检查 clean working tree。
- 门状态：本卡不修改规则或表现，未执行新的 UE 构建/运行验收；G0 明确为模板依赖基线红项，G1–G5 尚未建立或验收。已有 Editor 日志只用于版本事实确认，不作为本次运行证据。
- 下一卡为 P0-01：将 UE 模块注册与宿主依赖移出 Portable Core，建立真实双层结构及构建入口。P0/P1 整体尚未完成。
