# 第 20 章 rosbag2：订阅、存储插件、录制与回放

录制一个话题并不是把发布者内存直接写入文件。Recorder 是另一个订阅者，它需要发现话题、选择 QoS、接收序列化数据，再交给存储层。回放则沿相反方向组织数据，却无法自动重建原系统的全部运行条件。

> 状态：初稿，关键路径按本地 `ros2/rosbag2` 源码核验；实验待执行。固定提交见 [附录 A](appendix-a-source-baseline.md)。前置知识为第 5–7、9 章。

## 20.1 transport、cpp 与 storage 的分工

```mermaid
flowchart LR
    T[ROS 话题] --> G[GenericSubscription]
    G --> R[Recorder]
    R --> W[Writer 与 SequentialWriter]
    W --> S[存储插件]
    S --> D[bag 数据与元数据]
    D --> Q[Reader]
    Q --> P[Player 与时间控制]
    P --> U[GenericPublisher]
    U --> T
```

GenericSubscription 根据运行时类型信息接收 SerializedMessage。这样 Recorder 不必为每一种业务消息编译一个模板订阅。类型支持仍需要可用，运行时知道类型名也不等于机器已经安装该接口。

| 文件与符号 | 职责 |
| --- | --- |
| [`recorder.cpp`](../../src/ros2/rosbag2/rosbag2_transport/src/rosbag2_transport/recorder.cpp)，`topics_discovery`、`subscribe_topics`、`create_subscription` | 发现、QoS 选择与通用订阅 |
| [`writer.cpp`](../../src/ros2/rosbag2/rosbag2_cpp/src/rosbag2_cpp/writer.cpp)，`Writer` | 写入的公共包装接口 |
| [`sequential_writer.cpp`](../../src/ros2/rosbag2/rosbag2_cpp/src/rosbag2_cpp/writers/sequential_writer.cpp) | 主题登记、缓存、文件切分与存储写入 |
| [`sequential_reader.cpp`](../../src/ros2/rosbag2/rosbag2_cpp/src/rosbag2_cpp/readers/sequential_reader.cpp) | 读元数据与逐条数据 |
| [`player.cpp`](../../src/ros2/rosbag2/rosbag2_transport/src/rosbag2_transport/player.cpp)，`prepare_publishers`、`publish_message` | 通用发布者与回放控制 |

## 20.2 订阅之前先登记话题

Recorder 轮询图并找出需要录制而尚未订阅的话题，为它们创建通用订阅。在创建订阅之前登记 writer 的话题信息，是为了避免订阅回调已经到来而存储层还不知道该话题。

订阅回调检查是否暂停，再调用 writer 写入序列化消息、话题名、类型和时间。本地实现使用 `this->get_clock()->now()` 作为该路径的时间参数。这个时间是 Recorder 回调处理时读取的时钟，不应等同于消息 header.stamp，也不能自动解释为发布者采样时刻。

写入接口返回与数据已经持久化到磁盘也需要区分。启用消息缓存时，写入还经过缓存与消费线程；正常关闭要刷新剩余数据。突然终止进程的损坏和恢复边界，需要按具体插件与文件状态验证。

## 20.3 录制 QoS 和回放 QoS

Recorder 可以根据已发现发布者的 offered QoS 适配订阅，也允许显式覆盖。若可靠订阅无法匹配 best-effort 发布者，图里有话题也不能保证记录到消息。动态新增端点的 QoS 变化同样值得核对。

回放根据存储话题信息创建发布者，按记录时间和播放速率控制发布节奏。原业务系统还有执行器延迟、设备时钟、外部服务和节点状态，这些条件不会由 bag 自动复原。改变播放速率也不会保证每个下游订阅处理得完。

发布 `/clock` 与接收节点启用 `use_sim_time` 是两个配置动作。只开其中一个不能完成全系统仿真时间切换；动态 TF 与消息时间不一致时，还会影响下一章的变换查询。

## 20.4 实验：录制后停止源节点，再回放

Ubuntu、Bash，所有终端加载本书工作区，固定域和 RMW：

```bash
source /path/to/ros2_humble/install/setup.bash
export ROS_DOMAIN_ID=42
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
mkdir -p experiment-results
```

终端 A 运行 talker。终端 B 为本次记录选择不存在的目录：

```bash
ros2 run demo_nodes_cpp talker
```

```bash
ros2 bag record -s sqlite3 -o experiment-results/ch20-bag /chatter
```

等待录制开始并记录数秒消息，在 B 按 Ctrl+C 正常关闭，再在 A 停止 talker。目录已存在时应另选名字，不覆盖旧证据。

```bash
ros2 bag info experiment-results/ch20-bag
```

启动 listener 后，在另一个终端回放：

```bash
ros2 run demo_nodes_cpp listener 2>&1 | tee experiment-results/ch20-listener.log
```

```bash
ros2 bag play experiment-results/ch20-bag
```

预期而非实测：bag info 列出 String 话题及非零消息数，listener 在源 talker 已停止时收到回放消息。若插件不可用，先处理 sqlite3 插件构建与安装问题，不把打开失败归因于订阅 QoS。

记录 bag info 消息数与 listener 实际收到条数。少量回放开始阶段消息可能受发现时机影响，不要求二者无条件相等，也不能由相等推断所有录制期间的原始发布均已保存。

| 实验 | 状态 | 实际观察与证据 |
| --- | --- | --- |
| 基本录制、info 与回放 | 待执行 | 待填写；`experiment-results/ch20-bag/` 与 listener 日志 |

本轮不对写入吞吐、丢失率和掉电恢复给出实测结论。性能实验还需固定消息大小、缓存、存储介质、压缩与下游负载。

## 20.5 本章小结

rosbag2 把通用端点、类型支持、存储插件和回放时钟连接起来。bag 保留消息和必要的描述信息，系统复现仍需保留环境与配置。下一章用带时间的 TF 查询说明为什么消息本身不足以重建空间关系。

审校记录：订阅创建顺序、记录时间与缓存入口已核对；插件运行、回放计数和仿真时间实验待验证。
