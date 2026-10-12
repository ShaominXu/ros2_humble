# 第 2、3 章实验操作与结果记录

本指南以 Ubuntu、Bash 和 GDB 为操作环境。命令中的工作区是本书仓库所在目录，由读者按实际路径进入。当前作者环境没有可运行的 ROS 工具链，以下所有实验均为**待执行**；预期输出来自源码分析，不是实测日志。

## 一、统一准备与记录

### 1. 选择运行版本

已通过 apt 安装 ROS 2 Humble 的读者可以先执行行为实验和函数入口断点实验，不必先重编译整个 ROS。每个终端先加载环境：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=42
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 pkg prefix demo_nodes_cpp
ros2 pkg prefix rclcpp
```

本指南固定选择 Fast DDS 适配实现，使实验三的中间件断点有明确对象。若该实现没有安装，应安装对应包或选择已安装的实现，并调整断点。domain 42 用于隔离本实验；仍应确保没有其他实验使用相同 domain。

若提示找不到示例包，可在 apt 环境安装所需工具与包：

```bash
sudo apt update
sudo apt install gdb cmake build-essential ros-humble-demo-nodes-cpp \
  ros-humble-rmw-fastrtps-cpp
```

apt 环境的实验结论仅对应安装版本。要核对本书固定源码，需要按项目清单获取源码、准备平台依赖并完成构建，再在每个终端加载本地 `install/setup.bash`。

只有需要源码行断点、变量和清晰的逐行追踪时，才优先采用带调试信息的源码构建。构建步骤示例：

```bash
colcon build --packages-up-to demo_nodes_cpp \
  --cmake-args -DCMAKE_BUILD_TYPE=Debug
source install/setup.bash
```

`Debug` 不是所有断点实验的必要条件；函数入口断点取决于可识别的符号。`RelWithDebInfo` 也可以提供调试信息，但优化可能影响单步和变量观察。不要把 apt 环境中的源码行对应到任意本地提交。

### 2. 保存环境信息

在书稿仓库根目录创建记录目录，并保存基本信息：

```bash
mkdir -p experiment-results
{
  date -Is
  uname -a
  cat /etc/os-release
  gdb --version | head -n 1
  printenv ROS_DISTRO ROS_DOMAIN_ID RMW_IMPLEMENTATION
  ros2 pkg prefix demo_nodes_cpp
  ros2 pkg prefix rclcpp
  dpkg-query -W ros-humble-demo-nodes-cpp ros-humble-rclcpp
} > experiment-results/environment.txt 2>&1
```

若运行本地源码构建，另记录相关仓库提交、修改状态、构建命令和环境加载顺序；apt 包版本不能代替本地构建的版本记录。

## 二、第 2 章实验一：建立基本通信

**目的**：观察话题端点匹配和应用回调输出，不以此单独证明底层调用链或消息完全无丢失。

1. 在终端 A 加载上述环境，运行 Talker，保存日志：

   ```bash
   ros2 run demo_nodes_cpp talker 2>&1 | tee experiment-results/talker.log
   ```

2. 等待约 3 秒，在终端 B 加载相同环境，运行 Listener：

   ```bash
   ros2 run demo_nodes_cpp listener 2>&1 | tee experiment-results/listener.log
   ```

3. 在终端 C 加载相同环境，查询端点：

   ```bash
   ros2 topic info /chatter --verbose | tee experiment-results/chatter-info.txt
   ```

4. 两端运行约 10 秒，分别按 Ctrl+C 结束。

**预期结果（非实测）**：Talker 打印递增的 `Hello World: N`；Listener 在匹配后打印同样格式的字符串。接收起始编号可以大于 1。查询结果应包含一个示例发布端和一个示例订阅端，类型为 `std_msgs/msg/String`；记录实际 QoS，不以书稿值替代查询结果。

**结果判读**：两端出现相同编号，说明对应样本已进入 Listener 回调。只有发送日志时，先检查终端环境、domain、RMW 和端点查询结果。发现额外端点时先排除其他进程。不要仅凭发布日志认定消息已成功发送。

## 三、第 2 章实验二：暂停与恢复 Listener

**目的**：观察执行器进入调度前没有用户订阅回调，以及恢复后的消息行为。默认暂停方式会停止 Listener 全部线程，因此不能独立证明“接收线程持续工作，只有执行器暂停”。

### 1. 启动 Talker

终端 A 加载环境，运行 `ros2 run demo_nodes_cpp talker`，保持它独立运行。

### 2. 用 GDB 启动 Listener

终端 B 加载相同环境，然后执行：

```bash
gdb --args "$(ros2 pkg prefix demo_nodes_cpp)/lib/demo_nodes_cpp/listener"
```

在 GDB 中依次输入，命令不要包含 `(gdb)` 提示符：

```gdb
set pagination off
set logging file experiment-results/listener-gdb.txt
set logging on
set breakpoint pending on
break rclcpp::executors::SingleThreadedExecutor::spin
run
```

选择函数入口断点，是因为示例没有在 `listener.cpp` 中定义 `main()`；`exec.spin()` 来自生成的入口。断点命中 `spin()` 的函数入口时，节点及订阅已经创建，但执行器循环尚未开始。

程序运行前断点可能显示 `pending`，表示等待共享库加载后解析；这不是失败。必须等到 GDB 明确报告命中断点后，再进行下一步。若 Listener 已打印 `I heard` 而断点未命中，停止本次实验，检查实际执行器与符号。

### 3. 暂停期间记录

命中断点后输入：

```gdb
bt
info threads
```

记录此时 Talker 的消息编号，等待约 10 秒，再记录编号。不要输入 `next` 或 `step`：本步骤需要保持 Listener 尚未执行调度循环。

**预期结果（非实测）**：独立的 Talker 进程继续打印；Listener 不打印用户回调日志。GDB 默认 all-stop，Listener 的其他线程也会被暂停。

### 4. 恢复执行

在 GDB 输入：

```gdb
continue
```

记录恢复后收到的前几条编号，运行约 5 秒。按 Ctrl+C 返回 GDB，输入 `quit`，根据提示结束调试进程；终端 A 按 Ctrl+C 停止 Talker。

**结果判读**：若收到暂停区间内的编号，说明这些样本最终可供取出；不能仅据此定位它们暂停时缓存在发送端还是接收端。若没有收到这些编号，也不能推断底层没有接收能力。发现进度、QoS、缓存及暂停全部线程都会影响结果。深度为 10 不等于恢复时必然打印十条历史消息。

### 5. 可选：只暂停执行器线程

在一次新的 GDB 会话中、`run` 之前输入：

```gdb
set non-stop on
show non-stop
```

再设置同样断点并运行。命中后用 `info threads` 检查其他线程是否显示为运行状态。non-stop 需要目标支持；不能只根据设置命令就认定已生效。选择命中断点的线程再恢复，例如实际命中线程号为 1 时：

```gdb
thread 1
continue
```

此方式让其他线程有机会继续运行，但“线程正在运行”仍不是“消息已被接收”的证据。需要接收路径断点或其他观测手段进一步确认。

GDB 模式与断点规则见 [non-stop 官方说明](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Non_002dStop-Mode.html) 和 [断点官方说明](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Set-Breaks.html)。

## 四、第 2 章实验三：观察调用栈

**目的**：验证发布、取出与回调的连接。断点改变运行时序，不用于性能测量。

1. 终端 A 用 GDB 启动 Talker：

   ```bash
   gdb --args "$(ros2 pkg prefix demo_nodes_cpp)/lib/demo_nodes_cpp/talker"
   ```

   在 GDB 输入：

   ```gdb
   set breakpoint pending on
   break rcl_publish
   run
   bt
   ```

   只在断点命中后执行 `bt`，保存输出。预期栈中可找到发布路径、Talker 定时器回调和执行器调度；优化构建可能省略部分栈帧。

2. 输入 `disable 1` 禁用刚才的断点（若编号不同，以 `info breakpoints` 为准），再输入 `continue`，使 Talker 持续运行。

3. 终端 B 用 GDB 启动 Listener，设置 `break rcl_take`，执行 `run`，命中后执行 `bt`。预期看到 `SubscriptionBase::take_type_erased` 与执行器路径。命中取出入口本身不能证明本次取出成功。

4. 需要核对实际成功的取出时，在完整调试信息下使用 `finish` 返回并查看返回值；再观察 Listener 用户回调中的日志行断点。若使用本书本地源码调试构建，该位置为 `listener.cpp` 中 `RCLCPP_INFO` 所在行。用 GDB 的 `list` 确认实际文件和行号后设置 `break 文件:行号`，不要照搬不同版本行号。

5. 若要核对 Fast DDS 发送入口，在 Talker 的 GDB 会话中禁用其他断点，设置：

   ```gdb
   break rmw_fastrtps_shared_cpp::__rmw_publish
   continue
   bt
   ```

   必须实际选用了对应 RMW 且符号可识别。断点一直 pending 时检查库和符号，不把“未命中”写成“函数未调用”的结论。

**预期结果（非实测）**：发送栈和接收栈分别支持第 2 章的两段路径。记录实际出现的函数，并对缺失栈帧标注优化或符号限制；不能把书中完整调用链直接复制为实测栈。

## 五、第 3 章：可编译的生命周期探针

程序位于 [`experiments/lifetime/main.cpp`](../../experiments/lifetime/main.cpp)。它不创建执行器，也不注册捕获业务对象的回调；关闭参数服务、参数事件和 rosout，减少无关端点。每个阶段要求按 Enter 才继续，便于观察销毁边界。

加载 apt 或本地安装环境后，从书稿仓库根目录构建：

```bash
cmake -S experiments/lifetime -B build/lifetime -DCMAKE_BUILD_TYPE=Debug
cmake --build build/lifetime
```

这里仅编译探针，链接当前环境发现的 ROS 库。探针带调试信息不意味着依赖库自动带完整调试信息。可以先不用 GDB 验证输出；若观察函数清理边界，再使用 GDB。

### 实验一：释放 Node，保留 Publisher

执行：

```bash
./build/lifetime/lifetime_probe
```

按每个提示输入 Enter。A 阶段对象已创建；C 阶段最外层 Node 已释放，Publisher 保留；D 阶段 Publisher 已释放；最后退出 NodeOptions 作用域并释放 Context。

**按本地源码推导的预期输出（非实测）**：

```text
A: objects created [press Enter]
node_expired=1 base_expired=0
C: outer Node released; publisher retained [press Enter]
base_expired=1
D: publisher released [press Enter]
context_expired=1
```

`node_expired=1` 表示最外层 Node 不再存活，`base_expired=0` 表示发布者的共享句柄仍保持 NodeBase。最后释放 Publisher 后，若没有其他强引用，NodeBase 才释放。结果与预期不同，应记录实际输出并检查版本和持有关系。

如需观察 C 层清理，运行 `gdb --args ./build/lifetime/lifetime_probe`，然后输入：

```gdb
set breakpoint pending on
break rcl_publisher_fini
break rcl_node_fini
run
```

遇到程序的 Enter 提示时按 Enter；遇到 GDB 断点提示时输入 `bt`，保存栈，再输入 `continue`。预期相关清理集中在释放 Publisher 的步骤；不要假定不同配置下只有一次发布者清理。

### 实验二：先关闭 Context，保留对象

执行：

```bash
./build/lifetime/lifetime_probe shutdown
```

在 A 阶段按 Enter，程序调用显式 Context 的 shutdown；停在 B 阶段时，预期增加以下输出：

```text
context_valid=0 node_alive=1 publisher_alive=1
B: context shutdown; objects retained [press Enter]
```

这说明上下文已无效，而程序仍持有 Node 和 Publisher 对象。后续按 Enter 释放对象，预期 C、D 和最终输出与实验一相同。`publisher_alive=1` 只表示共享指针非空，不表示端点此时可正常通信。

进一步调试可使用 `gdb --args ./build/lifetime/lifetime_probe shutdown`，设置以下断点后运行：

```gdb
set breakpoint pending on
break rcl_shutdown
break rcl_publisher_fini
break rcl_node_fini
break rcl_context_fini
run
```

每次命中记录函数、阶段和调用栈，再继续。预期 shutdown 先于端点和节点清理，上下文最终清理在依赖释放之后。本程序专门让 NodeOptions 先退出作用域，因为它也持有 Context，否则 `context.reset()` 不一定是最后一份强引用。

## 六、实测结果记录表

运行完成后填入实际观察与日志位置。在执行前保持“待执行”，不得将预期结果改称实测。

| 实验 | 状态 | 实际观察 | 证据位置 | 结论与限制 |
| --- | --- | --- | --- | --- |
| 2-1 基本通信 | 待执行 | 接收起始编号、端点、QoS | 待填写 | 待填写 |
| 2-2 默认暂停 | 待执行 | 暂停区间、恢复编号、线程模式 | 待填写 | 待填写 |
| 2-2 non-stop | 待执行 | 目标支持、运行线程、恢复编号 | 待填写 | 待填写 |
| 2-3 调用栈 | 待执行 | 发送栈、取出返回值、回调栈 | 待填写 | 待填写 |
| 3-1 共享持有 | 待执行 | 弱引用状态、fini 阶段 | 待填写 | 待填写 |
| 3-2 关闭与清理 | 待执行 | 有效性、存活状态、清理顺序 | 待填写 | 待填写 |
