# 导读：源码全景与阅读路线

阅读具体函数之前，先建立一张源码地图。本导读说明项目范围、模块职责和核心分层，并提供后续章节的阅读入口。当前分析基于目录、包配置和关键源码，构建与运行实验尚未验证。

## 1. 项目定位

本项目是一套 ROS 2 Humble 源码工作区，包含机器人软件框架的通信层、C++/Python 开发接口、构建工具、命令行工具、可视化组件和示例。

当前扫描范围内未发现独立的机器人业务应用，例如完整导航系统或机械臂控制程序。项目的主要价值是提供机器人应用的基础设施，以及学习、调试和修改 ROS 2 的源码环境。

- 源码位于 `src/`，按上游组织和仓库归类，一个仓库可能包含多个 ROS 包。
- 共找到 353 个 `package.xml`。部分目录带有 `COLCON_IGNORE` 或 `AMENT_IGNORE`，该数量不等于实际参与构建的包数。
- [ros_environment/CMakeLists.txt](../../src/ros/ros_environment/CMakeLists.txt) 将发行版默认设置为 `humble`，并允许通过 `ROS_DISTRO_OVERRIDE` 覆盖。
- 根目录的 [sources.repos](../../sources.repos) 保存上游仓库地址与精确提交号。源码获取、构建和运行来源的确认方法见[第 1 章](chapter-01-workspace-and-baseline.md)。

发行版默认值不能单独证明所有子仓库版本完全一致；完整构建状态仍需实际验证。

## 2. 目录与模块职责

| 目录或模块 | 主要职责 |
| --- | --- |
| `src/ament` | 包构建支持、依赖导出、测试及代码规范检查 |
| `src/ros2/rclcpp`、`rclpy` | C++、Python 客户端接口，用于创建节点、发布者、订阅者等 |
| `src/ros2/rcl` | 客户端库共用的底层 C 接口，以及动作、生命周期和参数解析支持 |
| `src/ros2/rmw`、`rmw_implementation` | 通信中间件抽象接口与实现选择、加载支持 |
| `src/ros2/rmw_fastrtps`、`rmw_cyclonedds`、`rmw_connextdds` | 面向不同 DDS 实现的适配层 |
| `src/eProsima` | Fast DDS、Fast CDR 及相关内存分配依赖 |
| `src/eclipse-cyclonedds` | Cyclone DDS 实现 |
| `src/eclipse-iceoryx` | 共享内存通信相关基础设施；是否使用取决于构建和运行配置 |
| `src/ros2/rosidl*` | 接口定义解析、代码生成、运行时类型与类型支持 |
| `src/ros2/common_interfaces`、`rcl_interfaces`、`example_interfaces` | 常用消息、框架接口和示例接口定义 |
| `src/ros2/launch`、`launch_ros` | 进程与多节点启动编排 |
| `src/ros2/ros2cli` | `ros2` 命令行工具及扩展机制 |
| `src/ros2/rosbag2` | 消息录制、存储与回放 |
| `src/ros2/geometry2` | TF2 坐标变换 |
| `src/ros2/rviz` | 机器人状态与传感器数据可视化 |
| `src/ros-visualization` | rqt、交互标记及相关 GUI 工具 |
| `src/ros` | URDF、机器人状态发布、插件加载等基础组件 |
| `src/ros-perception` | 图像传输、相机信息、激光数据处理 |
| `src/ros-planning/navigation_msgs` | 地图等导航相关消息定义 |
| `src/gazebo-release` | Gazebo 相关构建与数学依赖包 |
| `src/ros2/demos`、`examples` | 节点通信、参数、组件等示例 |
| `src/ros2/system_tests` | 跨模块系统测试 |
| `src/ros2-rust/rosidl_rust` | Rust 接口生成相关源码，本文未深入分析 |

`navigation_msgs` 的存在不代表包含完整导航算法；`gazebo-release` 中的依赖包也不代表包含完整仿真平台。

## 3. 核心分层

普通跨进程消息通信的主要层次如下：

```text
业务节点 / 示例节点
        ↓
rclcpp（C++）或 rclpy（Python）
        ↓
rcl：公共 C 接口
        ↓
rmw：统一中间件接口
        ↓
rmw_fastrtps / rmw_cyclonedds 等适配实现
        ↓
DDS：发现与数据传输
```

这套分层将应用 API 和具体通信实现隔离。应用通常通过客户端库创建节点和通信对象，无需直接调用某一家 DDS 的 API。

本地 [get_default_rmw_implementation.cmake](../../src/ros2/rmw/rmw_implementation_cmake/cmake/get_default_rmw_implementation.cmake) 在未指定中间件时，优先选择可用的 `rmw_fastrtps_cpp`；具体使用哪种实现仍取决于构建结果和配置。

ROS 2 应用中的几个基本概念：

- **Node（节点）**：组织通信、参数、定时器等功能的基本单位。
- **Topic（话题）**：通过发布者和订阅者传递消息。
- **Service（服务）**：请求与响应通信。
- **Action（动作）**：适合需要反馈和取消能力的长时间任务。
- **QoS（服务质量）**：控制可靠性、历史缓存等通信行为，发布与订阅双方需要兼容。
- **Executor（执行器）**：等待就绪事件并调度订阅、定时器等回调。

## 4. 用一条消息连接源码地图

本书从 talker/listener 示例出发：talker 在定时器回调中发布字符串，listener 由执行器调度订阅回调并打印消息。普通跨进程通信沿 rclcpp → rcl → rmw → DDS 进入中间件，接收端再取出消息并执行用户回调。进程内通信和借用消息存在不同分支。

阅读示例还会遇到两个问题：`talker.cpp` 没有 `main()`，程序入口在哪里？`std_msgs/msg/string.hpp` 如何从 `String.msg` 生成？这些问题连接了组件注册、构建规则、接口生成与类型支持，分别在第 2、5 章展开。组件加载机制将在第 16 章进一步分析。

## 5. 本书阅读路线

建议先顺序阅读第一篇，建立目录、通信路径和对象关系，再按问题进入后续章节。

| 阅读目标 | 章节入口 |
| --- | --- |
| 区分工作区、仓库与包，确认源码版本及构建、运行来源 | [第 1 章：工作区、包与版本基线](chapter-01-workspace-and-baseline.md) |
| 串联程序入口、发布、接收与回调 | [第 2 章：从 talker/listener 追踪一条消息](chapter-02-message-journey.md) |
| 理解各层职责与资源创建、持有、释放 | [第 3 章：ROS 2 分层与资源生命周期](chapter-03-layers-and-lifetime.md) |
| 追踪上下文与节点的初始化 | [第 4 章：初始化、Context 与 Node](chapter-04-initialization-context-node.md) |
| 理解消息定义、生成代码和类型支持 | [第 5 章：消息定义如何变成代码](chapter-05-interface-generation.md) |
| 深入端点创建和普通跨进程 publish/take 路径 | [第 6 章：Publisher 与 Subscription](chapter-06-publisher-subscription.md) |

后续沿 QoS 与发现、等待与执行器、进程内通信和内存管理深入，再进入服务、动作、参数、生命周期与组件。工程应用部分结合启动、命令行、录制与 TF2 展示机制如何协作，完整安排见[全书目录](outline.md)。

阅读时应把问题、源码证据和实验观察对应起来。依赖声明提供定位线索，具体行为需要追踪实现与适用分支；尚未运行的实验保留为待验证步骤。

