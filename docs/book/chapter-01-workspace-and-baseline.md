# 第 1 章 工作区、包与版本基线

开始阅读 ROS 2 源码时，最先遇到的困难往往不是某个复杂算法，而是目录太多。同一个名字可能同时出现在仓库、包、头文件路径和命令参数中；一份源码又可能生成多个库与可执行文件。如果没有先建立地图，阅读很容易变成在文件之间反复跳转。

本章从当前工作区出发，回答三个问题：我们手中有哪些源码，构建工具如何把它们组织起来，怎样保证运行和阅读针对同一份实现。后续章节会沿具体功能深入，本章先确定定位文件和判断证据的方法。

本书以当前 `ros2_humble` 目录为解读对象，并用根目录的 `sources.repos` 保存上游仓库地址及提交号。本章已检查目录、包清单和相关构建文件；构建、环境加载与运行命令为待验证步骤，不代表当前工作区已经编译成功。

## 1.1 先区分工作区、仓库和包

这三个概念经常出现在同一条路径中，却承担不同职责。

**工作区**是组织源码和构建产物的目录。本项目根目录保存书稿和版本清单，源码放在 `src/` 中。执行构建后，通常还会出现构建、安装与日志目录；它们与原始源码的用途不同。

**仓库**是版本管理单位。当前源码按上游组织分组，各组中包含独立 Git 仓库。顶层书稿仓库与这些源码仓库各自管理历史。顶层 `.gitignore` 忽略 `src/`，书稿仓库通过清单记录源码版本，而不是重复提交全部上游文件。

**包**是 ROS 生态中的依赖与构建组织单位。包名来自 `package.xml`，一个源码仓库可以包含多个包，包还可以构建多个目标。

以 `rclcpp` 为例：

```text
ros2_humble/                         工作区与书稿仓库
  src/
    ros2/                            上游组织分组
      rclcpp/                        上游 Git 仓库
        rclcpp/                      C++ 客户端库包
          package.xml
          CMakeLists.txt
          include/rclcpp/
          src/rclcpp/
        rclcpp_components/           组件支持包
        rclcpp_action/               动作客户端接口包
        rclcpp_lifecycle/            生命周期节点包
```

路径里两次出现 `rclcpp`，前一次表示仓库，后一次表示包。因此，`src/ros2/rclcpp/rclcpp/include/rclcpp/publisher.hpp` 是仓库中的包头文件，不能因为目录名重复就省掉其中一层。

另一个例子是 `src/ros2/demos/demo_nodes_cpp`：`demos` 是仓库，`demo_nodes_cpp` 是包，`talker` 和 `listener` 是其中的可执行目标。命令 `ros2 run demo_nodes_cpp talker` 使用的是包名和可执行文件名，不是仓库名。

## 1.2 当前源码的阅读地图

工作区包含 104 个由版本清单记录的上游仓库。扫描 `src/` 找到 353 个 `package.xml`，但这个数量不是实际参与构建的包数：有些目录被忽略，构建工具也需要依据自身发现规则识别包。

先按职责建立索引，比逐个目录阅读更有用：

| 阅读问题 | 主要目录 |
| --- | --- |
| C++ 应用如何创建节点与通信对象 | `src/ros2/rclcpp` |
| Python 客户端接口如何实现 | `src/ros2/rclpy` |
| 公共 C 接口怎样连接客户端与中间件 | `src/ros2/rcl` |
| 通信抽象和具体适配如何组织 | `src/ros2/rmw`、`rmw_implementation`、`rmw_fastrtps` 等 |
| 消息定义如何生成语言代码 | `src/ros2/rosidl*` |
| 从哪里寻找最小运行示例 | `src/ros2/examples`、`src/ros2/demos` |
| 构建支持来自哪里 | `src/ament`、`src/ros2/ament_cmake_ros` |
| 启动、命令和录制工具如何工作 | `src/ros2/launch*`、`ros2cli`、`rosbag2` |
| 具体 DDS 实现在哪里 | `src/eProsima`、`src/eclipse-cyclonedds` |

目录按上游组织分组，不是框架架构图。比如 `src/ros` 中也有本工作区使用的组件，不能仅凭父目录名判断它属于哪一代运行接口。需要结合包配置、实际代码和构建状态确认。

同样，导航消息存在不等于包含完整导航算法，某些仿真相关依赖存在也不等于完整仿真平台已就绪。本书第一阶段集中分析核心客户端、通信和执行机制，其他目录按主题进入。

## 1.3 用 package.xml 判断包的身份和依赖

进入一个陌生目录，先看它的 `package.xml`。以 [`rclcpp/package.xml`](../../src/ros2/rclcpp/rclcpp/package.xml) 为例，本地文件声明包名为 `rclcpp`，包版本为 `16.0.21`，并导出构建类型 `ament_cmake`。

其中还有以下依赖声明，代码块节选自该文件：

```xml
<depend>rcl</depend>
<depend>rcpputils</depend>
<depend>rcutils</depend>
<depend>rmw</depend>
```

这提供了一条阅读线索：C++ 客户端库依赖公共 C 接口及相关基础设施。但依赖声明不等于运行调用栈。一个包依赖另一个包，可能是因为使用头文件、链接库或其他资源；要证明某个函数实际被调用，还要追踪代码。

常见依赖标签可以这样阅读：

| 标签 | 阅读时应关注的含义 |
| --- | --- |
| `buildtool_depend` | 构建该包需要的工具或构建支持 |
| `build_depend` | 构建阶段所需依赖 |
| `build_export_depend` | 下游构建使用本包时需要导出的依赖 |
| `exec_depend` | 运行阶段所需依赖 |
| `depend` | 合并表达构建、构建导出与运行依赖 |
| `test_depend` | 测试阶段所需依赖 |

[`demo_nodes_cpp/package.xml`](../../src/ros2/demos/demo_nodes_cpp/package.xml) 声明了对 `rclcpp`、`rclcpp_components` 和 `std_msgs` 等包的依赖，正好对应下一章的节点接口、组件注册与字符串消息。

还要区分包版本和源码提交。`16.0.21` 是包清单中的版本值；Git 提交号定位仓库中的具体状态。一个仓库有多个包时，它们分别声明包版本，而仓库提交同时固定那次提交中的文件集合。为书稿建立基线时，需要保留精确提交，不能只记录“使用 Humble”。

## 1.4 构建文件回答“代码如何变成程序”

`package.xml` 声明元信息，具体构建目标还要看构建文件。

在 [`demo_nodes_cpp/CMakeLists.txt`](../../src/ros2/demos/demo_nodes_cpp/CMakeLists.txt) 中，可以看到 `find_package(rclcpp REQUIRED)`、库和可执行目标的创建、目标依赖以及安装规则。同一文件还通过组件注册规则生成 talker 和 listener 的启动入口。

这解释了一个常见现象：节点源文件没有 `main()`，却能运行同名可执行文件。入口可能由构建模板生成，不能只在该源文件中搜索 `main` 就断言程序没有入口。第 2 章将具体连接组件注册、构建宏和入口模板。

Python 包则有另一种入口组织方式。[`ros2cli/package.xml`](../../src/ros2/ros2cli/ros2cli/package.xml) 导出 `ament_python` 构建类型；[`setup.py`](../../src/ros2/ros2cli/ros2cli/setup.py) 的 `console_scripts` 包含：

```text
ros2 = ros2cli.cli:main
```

这把 `ros2` 命令关联到 Python 模块中的函数。寻找 Python 命令入口时，应检查安装配置和扩展注册，而不是只搜索与命令同名的脚本。

构建工具在这里有两种不同层次的工作：ament 提供包内构建支持，colcon 在工作区范围发现包、选择包并组织构建。它们不是两个可以简单互换的同义名称。

## 1.5 被忽略的目录与生成文件

本工作区中有明确的忽略标记，例如 `src/ros/ros_tutorials/roscpp_tutorials/COLCON_IGNORE`。因此，看到目录和 `package.xml`，不意味着该包一定会被当前构建流程处理。

同样，不能把顶层 `.gitignore` 与构建忽略标记混为一谈：前者决定顶层 Git 如何处理未跟踪文件，后者参与相应构建工具的目录发现。源码没有提交到书稿仓库，不会因此自动从本地构建范围消失。

生成文件是另一类需要辨认的对象。下一章将看到 `std_msgs/msg/string.hpp`：其原始输入是 [`String.msg`](../../src/ros2/common_interfaces/std_msgs/msg/String.msg)，生成后的头文件要经过接口处理与构建才出现。

如果在 `src/` 中找不到某个头文件，应按下面的顺序检查：先确认 include 名称和包，再查看是否属于生成接口或模板输出，随后检查生成规则及构建目录。不要立即把“原始源码树没有该文件”解释为源码缺失。

## 1.6 固定书稿对应的源码

官方指南解决安装与构建的问题，项目的 [`sources.repos`](../../sources.repos) 解决本书分析具体对应哪份源码的问题。两者配合使用。

清单的每个条目给出相对于 `src/` 的仓库路径、Git 地址和精确提交号。获取源码后，可以用其中的路径直接对应正文链接。书稿默认以清单固定的版本解释行为；如果读者使用其他 Humble 更新版本，需要自行核对差异。

安装 Git 和 vcstool 后，从书稿仓库根目录执行以下源码获取步骤。Bash 或 cmd 中可使用：

```text
mkdir src
vcs import src < sources.repos
```

PowerShell 中可使用：

```powershell
New-Item -ItemType Directory -Force src
Get-Content -Raw sources.repos | vcs import src
```

这些命令适用于准备源码阅读目录，不包含依赖安装、编译与运行环境加载。已有源码时应先检查其状态，不应把重复导入当作无条件替换版本的操作。

对于具体仓库，可以从工作区根目录检查：

```text
git -C src/ros2/rclcpp rev-parse HEAD
git -C src/ros2/rclcpp status --short
```

第一条确认提交，第二条检查本地修改。只有提交号相同却存在未提交修改时，当前文件仍可能与固定基线不同。正式记录实验时，两个维度都要保存。

## 1.7 构建前先确认范围，运行前再确认来源

安装与构建条件请按 [ROS 2 Humble 官方安装指南](https://docs.ros.org/en/humble/Installation.html) 准备。本章不假定 Windows、Linux 或其他环境已经配置完毕，也不将同一套系统依赖命令套用到所有平台。

在工具链准备好后，可以先查看包发现结果：

```text
colcon list --base-paths src
colcon list --base-paths src --topological-order
```

`list` 输出发现的包及其路径和类型，`--topological-order` 按依赖关系排序。具体参数说明见 [colcon list 官方文档](https://colcon.readthedocs.io/en/released/reference/verb/list.html)。这些结果比单纯统计 `package.xml` 更接近本次工具配置下的包发现情况，但仍不是成功构建记录。

若从 talker/listener 入手，可考虑先构建它们所在的包及工作区中的递归依赖：

```text
colcon build --packages-up-to demo_nodes_cpp
```

`--packages-up-to` 与 `--packages-select` 不同：前者选择指定包及其递归依赖，后者只选择指定包。参见 [colcon 包选择文档](https://colcon.readthedocs.io/en/released/reference/package-selection-arguments.html)。选择依赖不会自动安装缺少的系统软件，也不能保证这份源码在当前平台一定构建成功。

构建成功后，还要按平台加载生成的环境脚本。常见形式分别是 Bash 的 `source install/setup.bash`、cmd 的 `call install\setup.bat`，以及 PowerShell 的 `. .\install\setup.ps1`；应以本次安装目录中实际生成的脚本为准。

尤其当系统已经安装另一套 ROS 2 时，“能够执行 ros2 命令”并不能证明运行的是刚阅读的源码。可以查看：

```text
ros2 pkg prefix demo_nodes_cpp
ros2 pkg prefix rclcpp
```

检查输出是否指向预期安装位置，并结合环境加载顺序确认。包前缀是定位线索，不能单独证明进程加载的每个动态库都来自同一构建；更严格的调试还需检查进程实际加载的库。

### 实验记录应包含什么

为了使后续结论能够复现，每次运行至少记录操作系统、编译器与构建配置、源码提交及修改状态、构建命令、环境加载步骤、RMW 实现和 ROS domain 配置，以及实际输出。尚未记录的项目标为未知，不用默认值替代观察结果。

当前工作区尚未完成这组构建与运行验证。因此，本书现有章节中的源码调用分析和待执行实验分别标注，避免把设计预期写成实测结果。

## 1.8 建立一条可重复的阅读路线

遇到一个具体问题，例如“订阅回调是谁调用的”，可以采用相同的定位过程：

1. 找到最小示例，确定类型、话题、回调和运行条件。
2. 查看包清单和构建文件，找出依赖与程序入口。
3. 沿用户接口追到核心实现，记录关键对象、分支和句柄。
4. 区分源码明确表达的事实与尚待解释的设计原因。
5. 用断点、调用栈或行为实验验证，再记录适用条件。

源码搜索时，函数名和类名通常比笼统主题更有效。比如搜 `execute_subscription`，比搜索“接收消息”更容易找到关键实现；搜索结果也要区分声明、实现、测试、模板和历史变更记录。

当调用进入外部实现或生成代码时，先记录边界再展开，不必为了理解一行示例同时读完全部仓库。把问题和证据对应起来，才能让阅读结果成为可审查的章节。

## 1.9 本章小结

工作区组织源码与构建结果，仓库管理历史，包声明依赖并形成构建单位。理解这三个层次，才能把路径、命令与程序目标正确连接起来。

包清单告诉我们“它是谁、依赖谁”，构建文件说明“代码怎样成为目标”，版本清单固定“本书读的是哪份源码”，运行环境则决定“实际执行了哪份实现”。这些证据彼此补充，任何一个都不能独自替代其余部分。

下一章从 talker/listener 出发，沿定时器、发布接口、消息取出和回调分发追踪一条字符串。阅读地图将在这条具体路径中第一次连成完整流程。

## 源码与资料索引

- 本地 `rclcpp/package.xml`、`demo_nodes_cpp/package.xml`：包身份与依赖示例。
- 本地 `demo_nodes_cpp/CMakeLists.txt`、`ros2cli/setup.py`：构建和命令入口示例。
- 根目录 `sources.repos`、`.gitignore`：版本基线与顶层仓库边界。
- 正文链接的 ROS 2 安装指南和 colcon 官方文档：环境准备及工具参数参考。

本章中的 104 个仓库和 353 个包清单是当前工作区的检查结果，不作为其他 Humble 工作区的固定数量。工具命令尚未在本工作区完整执行，实际构建和运行记录将在验证后补充。
