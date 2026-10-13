# 第 8 章 WaitSet、GuardCondition 与 Timer

消息传递到中间件之后，应用并不会凭空知道有工作可做。执行器需要等待订阅数据、定时器到期和框架状态变化。本章分析它们怎样进入等待体系，以及等待为什么不同于执行回调。

源码采用本书固定基线。探针提供有限时长的操作，预期输出均为源码推导，尚未编译运行。

## 8.1 WaitSet 组织等待对象

第 2 章的执行器调用 `wait_for_work()`。[`executor.cpp`](../../src/ros2/rclcpp/rclcpp/src/rclcpp/executor.cpp) 收集需要处理的对象，把它们组织到 C 层等待集合，再调用 `rcl_wait()`。

等待集合可以包含订阅、定时器、服务、客户端、guard condition 和事件等类别。它表达“等待哪些东西就绪”，不是一份保存所有应用消息的队列，也不是用户回调列表的简单复制。

C++ 还提供 [`WaitSet`](../../src/ros2/rclcpp/rclcpp/include/rclcpp/wait_set.hpp) 接口，供应用显式管理等待；执行器不必直接使用这层公共封装才能进行 C 层等待。公共等待集合有不同存储和同步策略，不能把默认版本的修改规则推广到所有变体。

## 8.2 从 rcl_wait 到 rmw_wait

[`rcl/wait.c`](../../src/ros2/rcl/rcl/src/rcl/wait.c) 将 C 层实体与 RMW 等待对象关联，并调用 `rmw_wait()`。返回后，没有就绪的相关项会以空指针等形式被过滤，C 层再整理结果。

```mermaid
flowchart LR
    A[收集实体] --> B[组织 rcl_wait_set_t]
    B --> C[rcl_wait / rmw_wait]
    C --> D[整理就绪与超时状态]
    D --> E[选择工作并执行]
```

最后一步由使用等待结果的调度逻辑完成，而不是 `rmw_wait()` 自动调用所有用户函数。等待返回只是把控制权和就绪信息交回上层。

订阅就绪后仍需 take，并检查是否实际取得样本。定时器就绪后仍需更新调用状态并执行回调。GuardCondition 唤醒也不必携带一条业务消息。

## 8.3 定时器怎样影响等待时间

计时不必对应一个“每个定时器专属的睡眠线程”。C 层等待实现检查定时器下一次到期时间，并用它约束底层等待的超时；返回后再用 `rcl_timer_is_ready()` 检查到期状态。

因此，一个只含定时器的等待过程，也可以因为时间到了而返回 Ready，而不只是底层订阅收到了数据。等待超时与定时器就绪应区别处理。

[`timer.c`](../../src/ros2/rcl/rcl/src/rcl/timer.c) 管理下一次调用时间、周期、取消等状态；[`timer.hpp`](../../src/ros2/rclcpp/rclcpp/include/rclcpp/timer.hpp) 中的普通 GenericTimer 使用 `call()` 更新调用状态，再通过 `execute_callback()` 执行函数。

定时器就绪不是硬实时保证。若回调执行太久或调度受阻，实际开始时刻可能延迟；源码会处理下一次调用时间，但不能由此推断每个错过的周期都生成一份独立回调任务。

## 8.4 GuardCondition：框架主动通知等待者

[`guard_condition.cpp`](../../src/ros2/rclcpp/rclcpp/src/rclcpp/guard_condition.cpp) 初始化底层条件并持有 Context，`trigger()` 经对应分支通知等待体系。它适合表达结构变化或停止等待等需要主动唤醒的事件。

创建新订阅时 NodeTopics 会触发通知，第 6 章已经看到这一入口。执行器取消和上下文关闭也需要打断等待。只改一个停止标志，却让线程无限阻塞在等待里，不足以确保循环及时退出。

GuardCondition 不是消息计数器，也没有自动等价于一个用户订阅回调。多次触发与等待返回次数之间不能无条件按一比一解释，应检查具体实现与使用方式。

## 8.5 等待集合与生命周期约束

[`wait_set_template.hpp`](../../src/ros2/rclcpp/rclcpp/include/rclcpp/wait_set_template.hpp) 的登记过程会检查实体使用状态，避免某些实体被不恰当地同时关联到不同等待集合。等待结果还与相应集合的使用周期有关。

应用应保持相关实体存活，在等待结果有效期间按规定读取它，结束结果使用后再进行下一轮操作。默认策略不支持任意线程同时修改集合；需要线程安全变体时，应检查它对应的同步规则。

本章探针不用执行器，避免同一定时器既被执行器等待又被手动等待。它也不在另一线程中增删集合；guard 实验仅触发已经登记的条件。

## 8.6 实验：等待、唤醒与手动分发

程序见 [`experiments/waiting/main.cpp`](../../experiments/waiting/main.cpp)。它有 timer、timeout、guard 三个模式，没有执行器。以 Ubuntu、Bash 为例，从书稿根目录构建：

```bash
source /opt/ros/humble/setup.bash
mkdir -p experiment-results
cmake -S experiments/waiting -B build/waiting -DCMAKE_BUILD_TYPE=Debug
cmake --build build/waiting
```

需有编译器、CMake、rclcpp 与可用 RMW。apt 构建对应安装版本，核验本书实现时使用源码安装环境。

### 实验一：Ready 之前没有自动回调

```bash
./build/waiting/waiting_probe timer
```

程序登记五百毫秒周期的定时器，并最多等待一秒。预期关键输出为：

```text
wait=Ready callbacks_before_dispatch=0
callbacks_after_dispatch=1
```

等待返回时回调仍是零。程序随后检查 ready、调用 `call()`，再手动 `execute_callback()`，计数才增加。这检验的是显式等待与执行的分离，不代表所有应用都应该绕开执行器手动分发。

### 实验二：等待期限短于下一次到期

```bash
./build/waiting/waiting_probe timeout
```

程序复位同样周期的定时器，只等待一百毫秒。一般负载下预期为 Timeout，前后回调计数都是零。机器长时间暂停或调度延迟可能改变返回时刻，应记录真实状态，不将严格毫秒相等当作成功条件。

### 实验三：主动触发 GuardCondition

```bash
./build/waiting/waiting_probe guard
```

程序登记一个条件，另一个线程约两百毫秒后触发，主线程最多等待一秒。预期 `wait=Ready user_callbacks=0`。这里返回 Ready 是通知条件就绪，不是取出了订阅消息或执行了业务回调。

三个命令正常退出为 0，异常为 2；程序只是打印观察状态，退出码不代替预期状态核对。保存日志时可使用 `2>&1 | tee experiment-results/waiting-timer.txt` 等不同文件名。同时保存操作系统、编译器与 GDB 版本、rclcpp 安装前缀、RMW、源码提交及修改状态、构建命令和环境加载顺序；使用 apt 库时记录包版本。

需要核对调用关系时，用 GDB 启动 timer 模式，在 `rcl_wait`、`rmw_wait` 和 `rcl_timer_call` 设置函数断点；每次命中记录 `bt` 并继续。暂停会改变定时器状态和到期时间，该实验不用于测量正常延迟。

| 模式 | 状态 | 实际返回与计数 | 证据与限制 |
| --- | --- | --- | --- |
| timer | 待执行 | 待填写 | 待填写 |
| timeout | 待执行 | 待填写 | 待填写 |
| guard | 待执行 | 待填写 | 待填写 |

## 8.7 本章小结

WaitSet 组织实体，RMW 等待提供底层事件，定时器影响等待期限，GuardCondition 提供主动唤醒。返回就绪信息之后，取出、选择与用户函数执行仍然是后续工作。

下一章分析这些就绪工作怎样被不同执行器选择，以及回调组如何限制并发。已核验上述本地等待、条件和定时器入口，探针未编译运行；未展开全部等待集合策略与模拟时间跳变。
