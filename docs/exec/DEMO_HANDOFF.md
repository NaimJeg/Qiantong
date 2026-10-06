# 千瞳连续竖井演示 v3

DEMO-03 使用连续9×65地图，三波敌人开局提前布置并寻找掩体，由镜头带入。清场时镜头快于友军；友军完全画外后只调整纵向位置，X不变，镜头停稳后从上边缘走入。正常波次不创建、销毁或重置单位Actor。保留即时激光、限速瞄准、自动避险及三选一。

默认seed=1、两人、选择3的复现点：`qt.demo seek 65 3` 首波战斗；`qt.demo seek 169 3` 镜头带入提前找好掩体的敌人；`qt.demo seek 182 3` 镜头停稳后友军走入。先restart，从小到大推进；seek不会倒退。镜头实时下行时可用Space暂停继续。

结果导出为 `Saved/DemoResults/continuous-人数-seed种子-option选择.json`；旁边 `continuous-view-lifecycle.json` 提供Actor路径/ObjectId及创建销毁计数。自动化5/5、Core 2/2、5纯数据蓝图审计通过；两人/五人PIE全字段对齐Golden，哈希 `8e46c5cc` / `a664f106`，23/26个Actor跨三波身份不变、创建销毁增量均0。证据：[验收](evidence/continuous-acceptance.json)、[镜头带入敌人](evidence/continuous-enemies.png)、[友军走入](evidence/continuous-entry.png)。地图仍是有限三波，不是无限生成。

当前已编译Editor原生模块，重启可用；不依赖Live Coding补丁。引擎源码只读，未重新打包，旧Windows包仍为v1。浮动PIE的截图使用MCP `CaptureEditorImage`；`qt.demo capture`可能被主编辑器视口消费，不作为浮动PIE证据。

## 历史 v2（生命周期由v3覆盖）

当前版本为 DEMO-02：随机障碍、自动绕行/避险/射击站位、限速瞄准的即时激光、三段清场下行。全部逻辑 C++，蓝图仍仅资产。Editor/Live Coding、Terminal/MCP 与 PIE 验收；不重复打包。**out/Demo/Windows 当前仍是上一轮 demo-1，不含本轮探索功能。**

## v2 打开与操作

打开 `/Game/Demo/Maps/L_Demo`，点击 Play（建议独立 PIE 窗口，竖屏720×1280）。先点击游戏窗口获得输入焦点。

- 两人默认开局，可切换五人。小队自动找敌、绕障碍、限速瞄准和瞬时开火；朝向短线是当前枪口，完整闪光射线是实际攻击。
- 障碍同时挡移动和激光。红圈为即将触发的范围危险，单位自动撤离；即时激光本身没有飞行时间。
- 清场自动下行，依次6/7/8敌人；三段清场胜利。HP、阵亡、冷却与一次三选一效果跨段保留。
- 1/2/3 或鼠标选协议（5秒后开放）；Space暂停，句点单步，Tab倍速，R按原种子重开，N换种子。NEW SEED、UNITS等也可鼠标操作。
- 导出路径 `Saved/DemoResults/explore-人数-seed种子-option选择.json`。结果含规则版本、RNG、波次、危险/避险计数、坐标、朝向及事件hash。能量/热量/技能字段仍为null。

## Editor 开发与复现

从 Terminal 修改项目 C++，Ctrl+Alt+F11 Live Coding；改变 UCLASS/反射布局等需要关闭编辑器再运行 `Tools/Build.ps1`。引擎源码保持只读。本轮结束前另行编译了 Editor 目标，重启也可载入新版。

编辑器控制台命令：

```text
Automation RunTests Qiantong.Demo
qt.demo restart
qt.demo seek 65
qt.demo rebuild
qt.demo finish 3
qt.demo capture
```

`seek <tick> [choice]` 暂停并推进到指定绝对tick（不倒退）；`finish [choice]` 完整推进；`capture` 保存真实PIE画面到 `Saved/Screenshots/ExplorePIE.png`。还支持 pause/step/seed/squad/choose/export。这些命令使用同一个Director，没有另一套规则。手动单步只保留最近一步的射线。

`Tools/Invoke-UnrealMcp.ps1` 连接本机8000端口，可发现/调用 StartPIE、StopPIE、CaptureEditorImage等工具。`Tools/Send-EditorInput.ps1` 可向经过归属/前台检查的项目窗口输入本机控制台命令或测试鼠标/键盘；不会开启Python远程执行。

自动化覆盖24个seed、地图连通、阻挡、路径、逐步最大转角、同tick伤害、危险预警/伤害/逃离、波次状态保留、30/60/120FPS、暂停倍速及八份Golden。2人/5人 PIE 选择3的最终结果分别为 `73d268a7` / `92480aa6`；两人测试包含View重建。

运行截图：[激光](evidence/explore-laser.png)、[避险](evidence/explore-hazard.png)、[下行](evidence/explore-descent.png)、[结算](evidence/explore-complete.png)。详细证据以 TASK_BOARD / CHANGELOG 为准。这是实验参数及有限场景验证，不代表完整产品、无限地图或Cocos迁移验收。

## 历史 v1 交付（以下描述旧 Windows 包）

2026-10-06 验收通过：Windows Development 打包及独立运行、两场 Golden 对照、UE Automation 2/2、Core CTest 2/2、5 个纯数据蓝图审计和鼠标/键盘实测均通过。用户确认可用。运行画面见 [demo-combat.png](evidence/demo-combat.png)。

## 打开与操作

编辑器打开 `/Game/Demo/Maps/L_Demo` 并 Play。Windows 包生成后从 `out/Demo/Windows/QiantongCore.exe` 启动；整个 Windows 目录一起分发，不能只复制 exe。

- 自动 2 对 6，5 秒出现三选一；1/2/3 或鼠标点击选择，战斗不停。
- Space 暂停，Tab 切换 1/2/5/10 倍速，句点单步，R 重开，Esc 退出。
- 底部 UNITS 切换 2/5 人并重新开始，EXPORT 导出当前结果；结算自动导出。
- 结果在运行目录对应 `Saved/DemoResults`；能量/热量/技能字段为 null（未实现），seed 为保留值且本版无随机。

## 编辑入口

`Content/Demo/Blueprints` 的五个蓝图是纯数据子类，仅资产、表现参数和实例化。所有逻辑在 `Source/QiantongUE` 的 DemoBattle/DemoRuntime 中。数值 fixture 只有 C++ 一份。不要往蓝图 EventGraph 添加 Tick、规则或 UI 分支。

Director 的 UnitViewClass、StageMaterial、AllyMaterial、EnemyMaterial 是主要资产入口。L_Demo 可调整竖井静态几何；相机与 HUD 使用 720×1280 设计坐标。旧 Lvl_Main 没有覆盖。

## 重建

```powershell
./Tools/Build.ps1 -EngineRoot 'D:/UE/Source/UnrealEngine-5.8.3-release'
./Tools/PackageDemo.ps1 -EngineRoot 'D:/UE/Source/UnrealEngine-5.8.3-release'
./Tools/TestDemo.ps1 -EngineRoot 'D:/UE/Source/UnrealEngine-5.8.3-release'
./Tools/SmokeDemo.ps1
```

蓝图和材质已保存，无需再运行 CreateDemoAssets.py；该脚本检测到已有地图会拒绝覆盖。编辑器插件仅 Editor target 启用，独立包不依赖 MCP/Python。

可重复验收：UE Automation `Qiantong.Demo`；运行游戏加 `-DemoSmoke -DemoOption=3` 会依次运行 2v6 和 5v6、导出结果并退出。`-DemoCaptureTick=121` 请求战斗截图。自动模式仅显式传参时启用，正常玩家模式不会自动选项。

构建、打包和运行是否通过，以 TASK_BOARD / CHANGELOG 的实际证据为准。此演示没有存档、养成、移动端打包或最终美术。
