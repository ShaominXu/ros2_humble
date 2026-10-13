# 第 7 章 QoS 与通信发现

创建发布者和订阅者之后，程序可能仍然收不到消息。两端名称和类型看似一样，问题也不一定在用户回调中。本章把端点配置、发现、匹配与数据处理分开，解释 QoS 为什么会让某些组合无法通信。

分析以本书本地源码为基线，重点使用 Fast DDS 适配路径。实验尚未构建运行，预期现象不是实测结果；发现时延和具体事件支持也不能由源码目录存在直接确定。

## 7.1 四个需要分别确认的阶段

```mermaid
flowchart LR
    A[创建端点] --> B[发现对端]
    B --> C[类型和 QoS 等条件匹配]
    C --> D[数据传递与取出]
    D --> E[执行用户回调]
```

这是一张诊断地图，不是 DDS 的完整状态机。图查询可以提供发现的端点信息，但看到两个端点不能独立证明它们兼容；兼容也不能保证每个样本都已被回调处理。

本地 [`node_graph.cpp`](../../src/ros2/rclcpp/rclcpp/src/rclcpp/node_interfaces/node_graph.cpp) 将话题端点信息查询转交给 C 层相关接口。这类图信息是观测入口，可能受传播时机和缓存影响。将查询结果与程序端点配置、实际样本记录一起使用，证据才完整。

## 7.2 发布方提供，订阅方请求

QoS 兼容不是简单比较两个配置结构是否完全相等。可靠性等策略有提供与请求方向。对明确的 Reliable 和 BestEffort 配置，可以建立下表：

| 发布者 | 订阅者 | 可靠性策略是否兼容 |
| --- | --- | --- |
| Reliable | Reliable | 是 |
| Reliable | BestEffort | 是 |
| BestEffort | BestEffort | 是 |
| BestEffort | Reliable | 否 |

这里仅判断可靠性这一项。其他策略、类型和发现条件仍可能阻止通信。Reliable 的提供能力可以满足较弱请求，而 BestEffort 发布者不能满足可靠性请求；它不是一种发送速度模式。

[`rmw_dds_common/src/qos.cpp`](../../src/ros2/rmw_dds_common/rmw_dds_common/src/qos.cpp) 的兼容性检查明确识别 BestEffort 发布者与 Reliable 订阅者的冲突。相同实现还检查其他策略，并对无法确定的默认值等情况作相应处理。兼容性检查入口和实际 DDS 匹配机制不是同一个对象，不能把一次静态检查当成发现已经完成的证明。

## 7.3 持久性与晚加入的订阅者

Volatile 主要面向当前通信，不承诺向晚加入的订阅者提供此前样本。TransientLocal 则允许仍存活的发布者按配置保留历史，并提供给符合条件的晚加入订阅者。

| 发布者 | 订阅者 | 持久性策略是否兼容 |
| --- | --- | --- |
| Volatile | Volatile | 是 |
| TransientLocal | Volatile | 是 |
| TransientLocal | TransientLocal | 是 |
| Volatile | TransientLocal | 否 |

兼容与历史交付能力仍要区分。要稳定观察晚加入历史样本，实验让双方都使用 TransientLocal，并保持发布者存活。不能把“发布者进程已退出”与“仍存活的发布者停止发送但保留历史”混为一谈。

KeepLast 深度限制相关历史，KeepAll 也可能受资源限制。历史深度不是回调批次，不保证恢复时必然得到深度数量的样本。deadline、liveliness、lifespan 等策略涉及其他条件，本章先建立兼容方向，具体限制需要结合配置与实现检查。

## 7.4 默认值和实际配置

[`qos_profiles.h`](../../src/ros2/rmw/rmw/include/rmw/qos_profiles.h) 定义了常用策略配置。用户可以使用默认配置，也可以明确设置。若配置含系统默认或未知值，就不能在未观察实现解析结果时给出无条件兼容结论。

第 6 章已经解释 `get_actual_qos()` 的入口。调试时记录用户请求和实际端点信息，尤其不要把不同环境的默认值当成固定基线。

同时，domain、发现范围、网络接口和实现配置都会影响发现。更换 QoS 不是对所有通信故障的通用修复。需要依次核对名称、类型、domain、端点状态、兼容配置以及取出和调度。

## 7.5 不兼容事件能提供什么证据

[`node_topics.cpp`](../../src/ros2/rclcpp/rclcpp/src/rclcpp/node_interfaces/node_topics.cpp) 将端点的事件处理对象加入等待体系。某些不兼容状态可以通过对应事件和日志观察，但回调是否执行仍与执行器相关，事件支持也依赖 RMW。

没有警告日志不等于一定兼容。反过来，出现不兼容警告应结合端点身份判断，避免把第三个进程的事件当成本实验两端的结果。

## 7.6 实验程序和环境

探针见 [`experiments/qos/main.cpp`](../../experiments/qos/main.cpp)。发布模式连续发送约十秒，再打印 `phase=holding_no_new_messages` 并保持端点存活至约二十五秒。订阅模式运行十秒，记录样本和计数。两端显式使用 KeepLast(5)，关闭进程内通信。

以 Ubuntu、Bash 为例，从书稿根目录构建：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=42
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
mkdir -p experiment-results
cmake -S experiments/qos -B build/qos -DCMAKE_BUILD_TYPE=Debug
cmake --build build/qos
```

需已安装编译器、CMake、rclcpp、std_msgs 和对应 RMW。其他终端进入相同目录并加载相同环境。apt 实验只验证安装版本；本书源码核验应使用对应构建。程序正常结束返回 0，无论是否收到消息；异常或用法错误返回 2。因此“退出码 0”不是本实验通信成功的判据。

### 实验一：可靠性兼容矩阵

终端 A 先运行：

```bash
./build/qos/qos_probe sub reliable volatile
```

终端 B 在两秒内运行：

```bash
./build/qos/qos_probe pub reliable volatile
```

预期订阅端在发现后收到部分或多条样本，不保证起始编号为 1。接收端结束后可停止仍在保持阶段的发布进程。每次重做先结束上次两端，再分别测试表中的其他组合。

负例使用 A 的 `sub reliable volatile` 与 B 的 `pub best-effort volatile`。预期可见端点但不收到该发布者样本，可能出现不兼容事件。正例包括 Reliable 提供给 BestEffort 请求。

可在第三终端查询 `ros2 topic info /qos_probe --verbose`，核对类型和 QoS。查询不会代替样本记录，不要运行额外 echo 订阅干扰端点数量。没有消息的负例必须配合同环境正例验证，否则无法排除发现失败。

### 实验二：晚加入的历史数据

终端 B 先运行 `qos_probe pub reliable transient-local`。等到它明确打印保持阶段标记，再在三秒内于终端 A 运行 `qos_probe sub reliable transient-local`。

此时发布者仍存活但不产生新消息。预期订阅端可以收到保留的近期样本。记录真实编号、数量和发现时间，不把五条写成无条件保证。

对照组双方改为 `reliable volatile`，仍然等到保持阶段再启动订阅者。预期没有历史交付。若此时收到样本，先确认确已停止新发布、没有其他进程，且 QoS 与记录一致。

### 实验三：持久性不兼容

先启动 `sub reliable transient-local`，再启动 `pub reliable volatile`。预期因持久性请求不被满足而无法正常匹配。与双方 TransientLocal 的正例对照，并记录端点查询和可能的警告。

| 实验 | 状态 | 实际样本与端点配置 | 证据与结论 |
| --- | --- | --- | --- |
| 可靠性矩阵 | 待执行 | 待填写 | 待填写 |
| 晚加入历史 | 待执行 | 待填写 | 待填写 |
| 持久性负例 | 待执行 | 待填写 | 待填写 |

## 7.7 本章小结

QoS 描述端点要求与能力，并非两端结构完全相等才可通信。发现与匹配又是不同阶段，历史缓存与当前样本也提供不同观察证据。

下一章进入等待体系，解释订阅、定时器与主动唤醒怎样形成可执行工作的就绪信息。本章已核验上述兼容检查、图查询和事件登记入口，实验未运行；未展开完整 DDS 发现协议与所有 QoS 策略。
