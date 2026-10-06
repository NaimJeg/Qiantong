# 执行记录

## 2026-10-06 — [DEMO-02][DEMO-004/005] 激光与竖井探索（Editor 验收通过）

- 用户授权完成上轮建议的障碍、自动绕行/避险/射击位置和下行波次，并明确瞬时命中、激光表现与最大转速；随后明确本轮及以后可经 Editor/Live Coding、Terminal/MCP 验收，不重复打包。
- DemoBattle v2：9×13逻辑网格，XorShift32随机连通障碍；射线闭合格遮挡；固定邻接BFS找射击格及安全格、同队射击站位分散；每50ms瞄准最多9°，3°容差；即时直击/副射线/爆裂不穿墙。范围危险提前24tick预警，单位主动撤离，伤害由集中模拟结算。
- 三段6/7/8敌人，清场40tick下行，友军HP/阵亡/冷却/协议保留，三段结束或全灭/超时结算。HUD显示地图障碍、预警圈/倒计时、枪口朝向、瞬时光束、动作、深度/波次和新seed按钮。蓝图未增加逻辑，引擎/插件源码保持只读。
- 先增加测试，再实现；初轮空间、探索和时钟通过。新增副射线测试发现夹具中两敌人过近会主动分散，改为相隔一格的静止瞄准夹具后验证实际主/副伤害与瞄准锥。补充下行阶段总时限边界，未削弱断言。
- 首次 Editor 构建、多个 Live Coding 增量更新及最后固化 Editor 构建全部成功。最终重启二进制测试 UE Automation 4/4，零失败/零警告；CTest 2/2；24 seed全流程、逐tick转速/遮挡、地图连通、30/60/120FPS与暂停倍速、八份 demo-2 Golden 全部通过。旧 demo-1 Golden 保留，不据此宣称迁移门通过。
- MCP initialize/StartPIE/StopPIE/截图实测。终端输入早期因焦点和空格传递未执行，修正本机输入辅助脚本后日志确认执行。Python远程执行的启用请求被自动审批拒绝，未开启该服务；改用现有编辑器本机控制台运行资产审计，5个蓝图均编译通过且 IsDataOnly=True。
- PIE 2人选择3：tick394、hash73d268a7，含tick65表现销毁重建；5人选择3：tick223、hash92480aa6。两份完整JSON逐字段等于Golden。鼠标 NEW SEED 到seed2、N到seed3、Space暂停实际输入通过；下行tick120到155由第1段过渡至第2段，选择保留。真实截图 explore-laser/hazard/descent/complete.png 已查看并留档。
- 固化后重新打开编辑器并握手MCP，PIE停在seed1/tick65的危险预警场景，点击游戏后Space继续。汇总证据 docs/exec/evidence/explore-acceptance.json；完整日志 Saved/Logs/Explore-FinalBuild.log、Explore-FinalTests.log、Explore-Editor.log、Explore-Ready.log；自动化报告 Saved/Automation/Demo/index.json。
- 本轮未打包，out/Demo/Windows仍为v1；PackageDemo脚本可供以后重建，SmokeDemo已改为v2 Golden校验，但本轮未运行打包脚本。手册、需求、ledger、AGENTS和架构同步；未提交commit，保留既有未提交工作。

## 2026-10-06 — [DEMO-01] 可打包演示（验收通过）

- 用户要求优先可玩且可打包，并明确蓝图极薄：仅资产管理与实例化，全部逻辑由 C++ 完成。同步 AGENTS 顶部现行修订、DEMO_RULES、ledger、计划和架构；Portable/TS 对照后置到进入 Cocos 前。
- 增加 UE 原生 FBattle/FClock 与 Director/UnitView/HUD/Controller/GameMode；50ms、稳定 ID、即时命中、同 tick 汇总伤害、2/5v6、三选一、结算重开及结果 JSON。单位 View 无 Tick 与权威 HP。
- 先写规则/时钟测试并确认实现文件缺失，再补实现；Development Editor 编译通过，UE Automation 两组全部通过（Saved/Automation/Demo/index.json），原 CTest 2/2 通过。
- 编辑器脚本生成 /Game/Demo 下独立地图、4 材质和 5 个纯数据蓝图；原 Lvl_Main 保留。MCP/编辑工具/Python 插件限制 Editor target；打包入口 Tools/PackageDemo.ps1，运行说明 DEMO_HANDOFF.md。
- 首次游戏窗口 smoke 验证 option3：2v6 tick200、hash b62b1367；重开 5v6 tick140、hash 2367f902。截图发现几何体曝光过暗，修正原生相机曝光后，独立包截图确认几何体与竖井清晰可见；爆裂的溅射轨迹从主目标发出。
- Windows Development Game target 首次构建、Cook、Stage、Archive 成功（Demo-Package.log）；独立包 smoke 在 tick90 重建 View，完成 2v6、重开和 5v6，所有 JSON 字段与 Tests/Golden 两份记录一致。证据 Saved/Logs/Demo-PackagedSmoke.log；截图 docs/exec/evidence/demo-combat.png。
- 修复结算“Deploy again”与底部“Restart”点击区重名，增加交互日志；最终增量打包再次成功（Demo-FinalPackage.log，ExitCode=0）。真实窗口输入检查通过：暂停 tick5 保持不变，单步到 tick6；鼠标在 tick135 选择 option2；切为 11 个单位（5v6），键盘重开后暂停。证据 Saved/Logs/Demo-InputEvidence.json；用户随后确认“可用”。首次输入测试早于首帧就绪，等待窗口就绪后复验通过。
- 5 个 Blueprint 的 IsDataOnly=True，全部编译通过且资产引用完整（Saved/Logs/Demo-AssetAudit.json）。规则测试无 World/UI，独立包不需要 Editor/MCP/Python。原 Core 保持纯 C++；Portable/TS 迁移仍未执行，不在本轮完成范围。
- 交付目录 out/Demo/Windows，启动 QiantongCore.exe；测试窗口留在暂停状态。未提交 commit，未覆盖原地图及既有模块拆分，未修改引擎源码或 EngineAssociation。
- 更正早先对话中的错误交付报告：当时磁盘并无完整玩法与 SOURCE_HANDOFF.md。本记录对应本次真实写入及实际执行结果，不沿用虚报“6/6”。

## 2026-10-05 — [P0-01][ARC-002][ARC-003] Portable Core / UE 模块拆分（验收中）

- 按用户请求先建立回退点 `436cd8f`；初始工作区干净，无待提交修改，因此创建空检查点。审查代码与任务板确认此前仅 P0-00 完成（P0 1/9、P1 0/10）。用户将本轮范围收敛至 P0-01，不推进缺失上游设计来源的玩法。
- 将模板游戏注册迁到 `QiantongUE`，更新原名 Game/Editor Target 与 `.uproject` 的 Runtime 模块引用。原引擎关联、项目名、插件、地图保留；旧脚本包名增加/更新重定向。
- 新建 ISO C++20 CMake 静态库 `QiantongCore`；其 Build.cs 仅作为 UBT External 接口，不添加 UE 依赖。CoreVersion 提供实际跨库链接入口，UE 启动日志消费该版本；没有创建战斗占位规则。
- 增加 `Tools/Build.ps1`：独立构建及 CTest 通过后，构建原 UE 5.8.1 Development Editor 目标。生成物继续忽略；UBT 跟踪 Core 输入并拒绝缺失/过期库。当前 UBT 库路径限 Win64，非默认 Debug CRT 不在本卡验收范围。
- 先写模块接线和链接 smoke 测试：原模板下接线检查出现 2 个失败、1 个缺文件错误；实现后 3 个断言测试通过，CTest 2/2 通过。CMake 故意 include `CoreMinimal.h` 的负向编译按预期失败；纯 Core 正常编译、链接、运行。源码 UE 头/类型/宏人工扫描无匹配，完整 G0 自动扫描仍留 P0-02。
- UE 单模块 Development Editor 编译/链接通过；启动检查发现单模块构建不更新旧模块清单，因此不能作为完整目标验收。完整 Editor 目标构建触发大量引擎编译，随后按用户要求停止进程树；未手工改模块清单。完整 G1 与成功启动验收尚未取得，P0-01 保持 DOING，不标记完成。
- 构建接入参考：阅读本机 UE 5.8.1 BuildSettings/uLangCore 的 Build.cs 与 UBT XML 接口说明，仅确认构建接口；未复制 Engine Code 到 Portable 库，无规则参考后端，G6 不适用。

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
## 2026-10-06 — [REPO-01] GitHub 仓库初始化

- 用户指定远程 `https://github.com/NaimJeg/Qiantong.git`；检查远程无分支，本地已有三条提交及尚未提交的 P0-01、DEMO-01..03 成果。保留历史和当前文件，将原型源码、资产、Golden 与验收证据纳入提交。
- 新增 README，说明 UE 5.8.3 环境、Editor 插件依赖、独立 Core 与项目构建、演示入口及现有验收范围。沿用缓存、构建输出和本机配置排除规则；当前文件与历史对象均无超大文件，共享 SecurityToken 为空，常见密钥模式检查无匹配。
- 本轮重跑 Core 配置、构建与 CTest，2/2 通过；清理三个文件末尾多余空行后，`git diff --cached --check` 通过。没有修改玩法逻辑或重新执行 UE/PIE 验收，保留此前 DEMO-03 证据。
- 提交 `44af531949f89782f676d81abf9070976075db83` 已推送至 `origin/master`，设置上游跟踪；`git ls-remote` 确认远程 master 指向同一提交。保留原有三条提交，无强制推送或历史重写。

# 2026-10-06 [DEMO-03][DEMO-006] 连续地图与跨波次生命周期

- 地图改为开局预生成9×65连续网格，随机障碍跨波保留；敌人三波6/7/8提前生成并移动寻找掩体，镜头下行自然带入。
- 清场镜头650/tick、友军至多200/tick；停稳且友军完全画外后仅调整Y，X/HP/冷却/死亡/选择保留，再从上边缘走入。敌人不随波次重布。单位Actor仅显式新局重建，正常波次不重建。
- Actor使用累计世界坐标，HUD/激光投影减去镜头深度；正交相机宿主随镜头移动，修正相机自动裁剪仍参考原点导致远处网格模型消失的问题。全部逻辑C++，蓝图保持资产用途。
- 验收：Continuity先补测试，UE5组自动化通过（含24seed、8Golden）；Core CTest2/2；Editor构建及相机修正Live Coding/最终构建成功；5个纯数据蓝图编译审计通过。两人/五人PIE结果与v3 Golden全字段一致，tick427/277，hash8e46c5cc/a664f106；跨波23/26Actor路径和ObjectId不变，创建/销毁增量均0。
- 证据：docs/exec/evidence/continuous-acceptance.json、continuous-2-result.json、continuous-5-result.json、continuous-before/enemies/entry.png；Saved/Logs/Continuous-FinalBuild.log、Continuous-FinalTests.log、Continuous-Actors-Before/After.json及Continuous-5-Actors-Before/After.json。MCP端口初次绑定失败，经本地控制台重启已有服务恢复；未开启Python远程执行。
- 限制：有限三波、非无限流式地图；Portable/TS仍后置。按用户授权Editor验收，不重复打包；out/Demo旧包仍demo-1。引擎及插件源码未修改，未提交Git。
