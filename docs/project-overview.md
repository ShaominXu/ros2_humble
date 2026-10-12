# ROS 2 Humble 项目解读

本文基于当前工作区的目录、包配置和关键源码整理，说明项目定位、模块职责、消息流转方式及阅读入口。此次分析未执行编译或运行验证。

## 1. 项目定位

本项目是一套 ROS 2 Humble 源码工作区，包含机器人软件框架的通信层、C++/Python 开发接口、构建工具、命令行工具、可视化组件和示例。

当前扫描范围内未发现独立的机器人业务应用，例如完整导航系统或机械臂控制程序。项目的主要价值是提供机器人应用的基础设施，以及学习、调试和修改 ROS 2 的源码环境。

- 源码位于 `src/`，按上游组织和仓库归类，一个仓库可能包含多个 ROS 包。
- 共找到 353 个 `package.xml`。部分目录带有 `COLCON_IGNORE` 或 `AMENT_IGNORE`，该数量不等于实际参与构建的包数。
- [ros_environment/CMakeLists.txt](../src/ros/ros_environment/CMakeLists.txt) 将发行版默认设置为 `humble`，并允许通过 `ROS_DISTRO_OVERRIDE` 覆盖。
- 当前根目录未发现顶层统一构建脚本、源码版本清单或 `build/`、`install/` 等构建产物。

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

本地 [get_default_rmw_implementation.cmake](../src/ros2/rmw/rmw_implementation_cmake/cmake/get_default_rmw_implementation.cmake) 在未指定中间件时，优先选择可用的 `rmw_fastrtps_cpp`；具体使用哪种实现仍取决于构建结果和配置。

ROS 2 应用中的几个基本概念：

- **Node（节点）**：组织通信、参数、定时器等功能的基本单位。
- **Topic（话题）**：通过发布者和订阅者传递消息。
- **Service（服务）**：请求与响应通信。
- **Action（动作）**：适合需要反馈和取消能力的长时间任务。
- **QoS（服务质量）**：控制可靠性、历史缓存等通信行为，发布与订阅双方需要兼容。
- **Executor（执行器）**：等待就绪事件并调度订阅、定时器等回调。

## 4. 示例：一条消息如何流转

示例入口：

- [talker.cpp](../src/ros2/demos/demo_nodes_cpp/src/topics/talker.cpp)
- [listener.cpp](../src/ros2/demos/demo_nodes_cpp/src/topics/listener.cpp)

### 4.1 发送端

`Talker` 继承 `rclcpp::Node`，其构造过程为：

1. 创建名为 `talker` 的节点。
2. 在 `chatter` 话题上创建 `std_msgs::msg::String` 发布者。
3. 使用 `KeepLast{7}` 配置历史缓存深度。
4. 创建每秒触发一次的定时器。
5. 在定时器回调中发布 `Hello World: 1`、`Hello World: 2` 等消息。

普通跨进程发布路径可以追踪为：

```text
pub_->publish(...)
  → rcl_publish(...)
  → rmw_publish(...)
  → 具体通信中间件
```

源码入口为 [publisher.hpp](../src/ros2/rclcpp/rclcpp/include/rclcpp/publisher.hpp) 中的发布逻辑，以及 [publisher.c](../src/ros2/rcl/rcl/src/rcl/publisher.c) 中的 `rcl_publish`。

### 4.2 接收端

`Listener` 创建名为 `listener` 的节点，订阅同一话题 `chatter`，使用同一消息类型，并设置深度为 10 的 QoS。

执行器发现订阅可读后，取出消息并调用用户回调，打印 `I heard: [...]`。相关实现位于：

- [executor.cpp](../src/ros2/rclcpp/rclcpp/src/rclcpp/executor.cpp)：`Executor::execute_subscription`。
- [subscription_base.cpp](../src/ros2/rclcpp/rclcpp/src/rclcpp/subscription_base.cpp)：`SubscriptionBase::take_type_erased`，调用 `rcl_take`。

创建订阅本身不会自动完成应用回调的持续调度，需要执行器运行。上述路径描述普通跨进程通信，进程内通信、借用消息等机制存在不同分支。

## 5. 组件入口与消息生成

### 5.1 为什么 talker.cpp 没有 main()

该示例采用组件形式，通过 `RCLCPP_COMPONENTS_REGISTER_NODE` 注册节点类。

[demo_nodes_cpp/CMakeLists.txt](../src/ros2/demos/demo_nodes_cpp/CMakeLists.txt) 将节点编译进共享库，并通过 `rclcpp_components_register_node` 声明组件及对应可执行文件。

入口由 [node_main.cpp.in](../src/ros2/rclcpp/rclcpp_components/src/node_main.cpp.in) 模板生成，主要流程是：

```text
初始化 ROS
  → 加载组件库
  → 创建节点实例
  → 将节点加入执行器
  → exec.spin()
  → 移除节点并关闭 ROS
```

因此，阅读这一示例需要将节点源码、组件注册宏和构建文件联系起来。

### 5.2 std_msgs/msg/string.hpp 从哪里来

该头文件由接口生成工具产生，其原始定义为 [String.msg](../src/ros2/common_interfaces/std_msgs/msg/String.msg)：

```text
string data
```

`rosidl` 负责接口解析和代码生成，相关生成器与类型支持包提供各语言可用的数据类型及通信所需的类型支持。整体关系为：

```text
.msg / .srv / .action
        ↓
接口解析与转换
        ↓
语言代码生成器、类型支持生成器
        ↓
C / C++ / Python 数据类型及类型支持代码
```

可以从 [rosidl/README.md](../src/ros2/rosidl/README.md) 查看该仓库内部各包职责。

## 6. 构建组织方式

该工作区按包组织，不存在一个负责所有功能的总 `main()`。不同节点、工具和库拥有各自的构建目标。

| 文件或工具 | 作用 |
| --- | --- |
| `package.xml` | 声明包名、版本、依赖、构建类型等元数据 |
| `CMakeLists.txt` | 定义 C/C++ 等包的构建、链接和安装规则 |
| `setup.py` | 定义 Python 包的安装信息及命令入口 |
| `ament` | 提供 ROS 包构建、测试及依赖导出支持 |
| `colcon` | 在工作区层面发现包、解析依赖并组织构建 |
| `COLCON_IGNORE` | 告诉 colcon 忽略相应目录子树 |

例如 [rclcpp/package.xml](../src/ros2/rclcpp/rclcpp/package.xml) 声明了对 `rcl`、`rmw`、`rcutils` 等包的依赖，可以用来验证架构中的依赖方向。

当前未检查完整工具链、系统依赖及子仓库版本一致性，也未执行构建，因此本文不将该源码快照描述为已验证可运行的环境。

## 7. 推荐源码阅读顺序

1. **基础发布与订阅**：从 [minimal_publisher](../src/ros2/examples/rclcpp/topics/minimal_publisher) 和 [minimal_subscriber](../src/ros2/examples/rclcpp/topics/minimal_subscriber) 理解节点与回调。
2. **较完整的示例**：阅读 [demo_nodes_cpp](../src/ros2/demos/demo_nodes_cpp)，了解定时器、参数、服务与组件。
3. **C++ 客户端接口**：阅读 `rclcpp` 中的 `Node`、`Publisher`、`Subscription`。
4. **执行与并发**：阅读 `Executor`、定时器和回调组，理解事件何时被处理。
5. **通信实现**：沿 `rcl → rmw → DDS` 追踪节点创建、发布、接收和资源释放。
6. **接口生成**：结合 `common_interfaces` 和 `rosidl` 理解消息定义如何变成代码。
7. **按需阅读工具**：根据任务进入 `launch_ros`、`rosbag2`、`geometry2` 或 `rviz`。

若目标是开发机器人应用，可先掌握示例、客户端 API 和启动配置；若目标是研究 ROS 2 内部机制，则应重点追踪执行器、内存管理和中间件调用链。
