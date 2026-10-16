# 附录 B 类、函数与源码入口索引

本索引帮助按问题进入章节。每章的源码表提供具体相对链接，版本由 [附录 A](appendix-a-source-baseline.md) 和固定清单确定。它不是所有函数的自动生成 API 索引。

| 要查的问题 | 主要符号与仓库内路径 | 章节 |
| --- | --- | --- |
| 包、版本与构建来源 | `package.xml`、`CMakeLists.txt`、`sources.repos` | [1](chapter-01-workspace-and-baseline.md) |
| 一条 String 消息的全程 | `Talker`、`Listener`，demos 的 `demo_nodes_cpp/src/topics/` | [2](chapter-02-message-journey.md) |
| 分层与对象存活 | `NodeBase`、`PublisherBase`、`SubscriptionBase` | [3](chapter-03-layers-and-lifetime.md) |
| 初始化与关闭 | `Context::init`、`Context::shutdown`，rclcpp 的 `rclcpp/src/rclcpp/context.cpp` | [4](chapter-04-initialization-context-node.md) |
| 接口生成与类型支持 | rosidl 解析器、C/C++ 生成器与 typesupport 分派 | [5](chapter-05-interface-generation.md) |
| 普通发布和取出 | `Publisher::publish`、`rcl_publish`、`rcl_take` | [6](chapter-06-publisher-subscription.md) |
| 发现与 QoS | `QoS`、RMW profile、Fast DDS 端点创建 | [7](chapter-07-qos-discovery.md) |
| 等待与唤醒 | `rcl_wait`、GuardCondition、Timer | [8](chapter-08-waitset-guard-timer.md) |
| 回调选择与并发 | `Executor`、`MultiThreadedExecutor`、`CallbackGroup` | [9](chapter-09-executor-callback-groups.md) |
| 进程内交付 | `IntraProcessManager`、`Publisher::publish` 的分支 | [10](chapter-10-intra-process-ownership.md) |
| 借用与归还 | `LoanedMessage`、`can_loan_messages`、`rcl_borrow_loaned_message` | [11](chapter-11-loaned-message-memory.md) |
| 请求与 Future | `Client::async_send_request_impl`、`handle_response`，rclcpp 的 `rclcpp/include/rclcpp/client.hpp` | [12](chapter-12-service-client.md) |
| Action 协议事件 | `ServerBase::take_data`、`execute`，rclcpp 的 `rclcpp_action/src/server.cpp` | [13](chapter-13-action.md) |
| 参数校验与提交 | `NodeParameters::set_parameters_atomically`，rclcpp 的 `rclcpp/src/rclcpp/node_interfaces/node_parameters.cpp` | [14](chapter-14-parameters.md) |
| 生命周期转换 | `LifecycleNodeInterfaceImpl::change_state`，rclcpp 的 `rclcpp_lifecycle/src/lifecycle_node_interface_impl.hpp` | [15](chapter-15-lifecycle.md) |
| 工厂与动态加载 | `ComponentManager::on_load_node`，rclcpp 的 `rclcpp_components/src/component_manager.cpp` | [16](chapter-16-components.md) |
| Python 调度与完成 | `_make_handler`、`_execute_client`，rclpy 的 `rclpy/rclpy/executors.py` | [17](chapter-17-rclpy.md) |
| 启动事件循环 | `LaunchService.run_async`，launch 的 `launch/launch/launch_service.py` | [18](chapter-18-launch.md) |
| 命令扩展与查询 | `cli.main`、`NodeStrategy`，ros2cli 的 `ros2cli/ros2cli/` | [19](chapter-19-ros2cli.md) |
| 录制与回放 | `Recorder`、`Player`、`SequentialWriter`，rosbag2 的 transport 与 cpp 包 | [20](chapter-20-rosbag2.md) |
| 时间变换查询 | `BufferCore::lookupTransform`、`walkToTopParent`，geometry2 的 `tf2/src/buffer_core.cpp` | [21](chapter-21-tf2.md) |
| 逐层取证 | 图快照、QoS 探针、take 与业务回调断点 | [22](chapter-22-troubleshooting.md) |

查找时先确认章节的适用条件，再进入符号。重载、模板、动态分派和生成入口可能使文本搜索结果对应不同路径；不能只根据同名函数判断实际执行链。
