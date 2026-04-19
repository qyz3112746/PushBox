# PushBox

一个基于 **Unreal Engine 5.5** 开发的推箱子玩法原型，包含：
- 可游玩的推箱子关卡流程（FlowDirector + ProcessController）
- 可在编辑器内使用的关卡编辑工具（MapEditor / EUW）

本仓库用于技术策划测试题交付，目标是展示“完整的关卡体验流程+关卡编辑器”。

## 环境要求

- Unreal Engine `5.5.4`
- Visual Studio 2022（参考：[虚幻设置Visual Studio](https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.5)）
- Windows 10/11

## 本地运行

1. 克隆项目
```bash
git clone https://github.com/qyz3112746/PushBox.git
```
2. 生成工程文件
- 右键 `PushBox.uproject` -> `Generate Visual Studio project files`
3. 打开项目
- 双击 `PushBox.uproject`，或从 VS 启动 `PushBoxEditor`
4. 在编辑器中打开测试地图并进入 PIE 运行

## 功能总览

### 运行时
- 网格推箱子移动与交互判定（通关条件为所有目标点上都有对应类型的格子则判定通过本关）
- 关卡数据驱动生成（二维网格）
- 流程控制（通关后pending、Next/Replay、多节点线性流程）

### 编辑器工具
- 地图可视化编辑（选择、批量 Apply）
- 复制粘贴（Ctrl+C、悬浮预览、左键连续盖章、Esc 取消、Q/E 旋转）
- 撤回/前进（Ctrl+Z / Ctrl+Y）
- 关卡预览与测试入口（Flow 侧）

## 文档导航

- 程序结构简单说明：[程序结构简单说明](docs/programming.md)
- 策划使用文档：[策划使用手册](docs/design.md#planner-manual)
- 美术使用文档：[美术文档](docs/art.md)

## 已知问题与说明

- 不要让编辑器中流程中两个引用指向同一个ProcessActor，如果这样做Test按钮启动关卡时始终只能启动第一个。

## 提交说明

- 仓库提交内容为：`Config/`、`Content/`、`Source/`、`*.uproject` 及必要脚本/文档
- 已忽略中间产物与本地缓存（如 `Binaries/`、`Intermediate/`、`Saved/`、IDE 临时文件）
