# PushBox 程序文档

> 推荐使用AI快速获取程序结构。

## 1. 运行时模块关系

- `UPushBoxLevelData`：关卡静态数据（二维网格 + 默认格子类）。
- `ALevelProcessController`：单流程节点控制器，管理本节点关卡序列 `LevelSequence`。
- `APushBoxFlowDirector`：全局流程调度（Node 与关卡索引推进）。
- `APushBoxLevelRuntime`：真正执行关卡逻辑的运行时 Actor。
- `APushBoxPlayerController`：输入到网格移动请求转换。

推荐理解顺序：**FlowDirector -> ProcessController -> LevelRuntime -> Cell/Box/Player 行为**。

## 2. Cell 体系结构与职责

### 2.1 基类与子类
- `AGridCellBase`：所有格子基类，提供网格坐标、通行/进出判定、可视组件与事件扩展点。
- `APlayerSpawnCell`：玩家出生语义格。
- `ABoxCell`：箱子出生/配置格（与可移动 BoxActor 配合）。
- `ABoxTargetCell`：目标格基类，负责目标匹配语义。
- 其他地形格（如默认地板、墙）通过 `AGridCellBase` 派生扩展。

## 3. Runtime 工作流（关键）

### 3.1 关卡加载
1. 接收目标 `UPushBoxLevelData`。
2. 遍历二维网格生成静态 Cell（`None` 使用 `DefaultCellClass` 兜底）。
3. 根据 Cell 类型生成玩家出生与箱子实体（BoxActor）。
4. 建立运行时索引（如坐标到 BoxActor 映射）。
5. 初始状态缓存用于重开。

### 3.2 输入与移动
- PlayerController 把方向输入转为 Runtime 的网格移动请求。
- Runtime 先做边界与 Cell 事件判定，再更新玩家/箱子坐标。
- 视觉层按网格步进更新位置。

### 3.3 通关与流程回调
- Runtime 判定当前关卡是否达成目标条件。
- 完成后通知当前活动的 ProcessController。
- Controller 进入 pending，Director 再决定 Next/Replay/流程结束。

### 3.4 重开
- 使用初始快照恢复玩家、箱子、关卡状态。
- 保持当前流程索引不变。

## 4. Flow 结构与推进机制

- `FlowDataAsset.Nodes[]`：流程节点数组。
- 每个 Node 指向一个 `ProcessController`，并可配置展示参数（如 CellSize）。
- Director 按 Node 线性推进；Node 内按 `LevelSequence` 线性推进。
- pending 状态下由 UI 触发 `ConfirmAdvance` / `ConfirmReplay`。

## 5. 编辑器工具实现要点

- 地图编辑主控件：`ULevelDataGridEditorWidget`。
- 固定 `64x64` 池化格，不重建控件，仅重绘有效区。
- 选择模型：Replace/Add/Remove（普通/Shift/Ctrl）。
- Undo/Redo：双栈快照（回退地图数据，样式始终取最新 CellList）。
- 复制粘贴：稀疏 Pattern（保留空洞）、越界自动裁剪、Q/E 旋转。
