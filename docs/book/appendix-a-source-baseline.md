# 附录 A 源码基线与构建环境

本书分析对象是 [sources.repos](../../sources.repos) 指定的源码快照。Humble 名称不替代逐仓库版本；章节引用通过仓库、相对路径、符号与下表提交关联，行号只辅助定位。

## A.1 核心仓库

2026-10-13 核对以下仓库：本地 HEAD 均与清单一致，`git status --porcelain` 均为空。未列出的仓库以清单为准；清单共固定 104 个上游仓库。来源 URL 也保存在清单中。

| 仓库 | 固定提交 | 相关章节 |
| --- | --- | --- |
| `ros2/rclcpp` | `8c09a31671534249faef346b629dea4e64bd570c` | 节点、通信、执行器、Action、生命周期、组件 |
| `ros2/rcl` | `cbaee7c905e3276bb7a629eb1e18d8d3c781f194` | C 层接口、Action 和生命周期 |
| `ros2/rclpy` | `eb711390d1fd8d76f6fc700d137fd370c4cdf625` | 第 17 章 |
| `ros2/demos` | `1fc7a62ee591e4c4a7f84e72aa4b33de11f6fbf3` | 各章最小演示 |
| `ros2/rmw` | `566d17a97e87bc5336ce33fb025ce8e952a3326a` | RMW 接口 |
| `ros2/rmw_fastrtps` | `da0c2d3120e9b5ea85cca251ea1f48ca8d0a93d7` | 具体端点与通信实现 |
| `ros2/rosidl` | `5621b26eff54596a356ad8abd72c06e7650d21f7` | 第 5 章接口生成 |
| `ros2/rosidl_typesupport` | `94c7b6d6db353cf26adc22cb7023833fd6cd18a1` | 类型支持分派 |
| `ros2/rosidl_typesupport_fastrtps` | `f04626533f5dec25d79eb334097881ba6fc3181b` | Fast RTPS 类型支持 |
| `ros2/rcl_interfaces` | `82776fc9068d1e0cd4af11c2e2700195994b2eb1` | 参数、Action、生命周期接口 |
| `ros2/common_interfaces` | `08434490d3e4abd37ccc5462cdde69b88853b5b8` | 标准消息 |
| `ament/ament_index` | `ae673e815cae9413cfbf4cab6e91a9c1c73c84ef` | 组件与包资源索引 |
| `ros2/launch` | `02900477fa6f4e21ffdf6a84618cda4e3cde5237` | 第 18 章 |
| `ros2/launch_ros` | `74064df445744dc96d08debf4b90d445e07385f5` | 第 18 章 ROS 动作 |
| `ros2/ros2cli` | `1e9f841df8d3fd10f3987ab0b4319cd3a94f6c52` | 第 19 章 |
| `ros2/rosbag2` | `bc63926c2e06efa480a957549be88b614fa96c82` | 第 20 章 |
| `ros2/geometry2` | `404b7224d623d614f18fa9738dbf1716403d857e` | 第 21 章 |

## A.2 环境与验证边界

本轮写作与静态检查环境为 Windows、PowerShell 和 Python 3.12；没有可调用的 `ros2`、`colcon`、`cmake`。尚未建立本书源码的可运行构建环境。各章 CLI、GDB、通信与录制实验均待执行，不能把预期输出视为实测。

新增章节默认实验环境为 Ubuntu、Bash、本书源码构建后的 `install/setup.bash`、域 42 和 rmw_fastrtps_cpp。章内 `/path/to/ros2_humble` 需替换为实际构建工作区。使用其他发行包环境时应记录差异，不视为本书固定源码的运行证据。

实验前在工作区根目录保存：

```bash
mkdir -p experiment-results
uname -a > experiment-results/system.txt
python3 --version > experiment-results/python.txt
cmake --version > experiment-results/cmake.txt
printenv ROS_DISTRO ROS_DOMAIN_ID RMW_IMPLEMENTATION AMENT_PREFIX_PATH > experiment-results/ros-env.txt
vcs export --exact src > experiment-results/sources-actual.repos
```

调试实验另记编译器、构建配置、符号位置与实际动态库来源。源码与运行库不一致时，断点和代码判断可能失去对应关系。实际构建是否成功、哪个包失败、运行环境来源均需保存原始记录。

## A.3 如何定位一个引用

例如第 12 章 `Client::handle_response` 的版本引用是：`ros2/rclcpp`、`rclcpp/include/rclcpp/client.hpp`、该符号和上表 rclcpp 提交。章内相对链接负责跳转，本表负责版本定位。上游源码版权与许可证以各仓库原文件为准。
