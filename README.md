# ROS 2 Humble 源码解读

以 ROS 2 Humble 源码为基础，沿节点创建、消息通信和回调执行逐步解读框架，最终整理为可阅读、可复现的书稿。

## 阅读入口

- [导读：源码全景与阅读路线](docs/book/introduction.md)
- [写作项目与成书路线](docs/book/README.md)
- [全书目录草案](docs/book/outline.md)
- [第 1 章：工作区、包与版本基线](docs/book/chapter-01-workspace-and-baseline.md)
- [第 2 章：从 talker/listener 追踪一条消息](docs/book/chapter-02-message-journey.md)
- [第 3 章：ROS 2 分层与资源生命周期](docs/book/chapter-03-layers-and-lifetime.md)
- [第 4 章：初始化、Context 与 Node](docs/book/chapter-04-initialization-context-node.md)
- [第 5 章：消息定义如何变成代码](docs/book/chapter-05-interface-generation.md)
- [第 6 章：Publisher 与 Subscription](docs/book/chapter-06-publisher-subscription.md)
- [第 7 章：QoS 与通信发现](docs/book/chapter-07-qos-discovery.md)
- [第 8 章：WaitSet、GuardCondition 与 Timer](docs/book/chapter-08-waitset-guard-timer.md)
- [第 9 章：Executor 与 CallbackGroup](docs/book/chapter-09-executor-callback-groups.md)
- [第 10 章：进程内通信与消息所有权](docs/book/chapter-10-intra-process-ownership.md)
- [第 11 章：借用消息与内存管理](docs/book/chapter-11-loaned-message-memory.md)
- [章节模板](docs/book/chapter-template.md)

当前章节已按本地源码核验关键调用，构建与实验尚未验证。

## 从源码安装

请按操作系统参考 ROS 2 Humble 官方源码构建指南，完成环境准备、依赖安装、编译和环境加载：

- [Ubuntu 源码安装](https://docs.ros.org/en/humble/Installation/Alternatives/Ubuntu-Development-Setup.html)
- [Windows 源码安装](https://docs.ros.org/en/humble/Installation/Alternatives/Windows-Development-Setup.html)

## 本书对应的源码版本

按官方源码构建指南准备环境时，使用本仓库的 [sources.repos](sources.repos) 获取源码，替代指南中的上游源码清单：本清单固定了 104 个上游仓库的具体提交，避免上游更新后代码与书中分析不一致。`src/` 为本地源码目录，不提交到本仓库。

在安装 Git 和 vcstool 后，从仓库根目录执行：

```text
mkdir src
vcs import src < sources.repos
```

上面的输入重定向适用于 Bash 或 cmd；Windows PowerShell 可使用：

```powershell
New-Item -ItemType Directory -Force src
Get-Content -Raw sources.repos | vcs import src
```

以上命令仅获取源码，依赖安装、编译与环境加载按对应系统的官方源码构建指南继续完成。章节中的源码链接指向本地 `src/`，建议获取源码后使用本地 Markdown 阅读器。

## 项目组织

```text
docs/                  工作区说明与书稿
  book/                成书路线、目录、章节与模板
sources.repos          上游源码地址和精确提交号
src/                   本地源码，由版本清单恢复
```

引用的上游源码版权与许可证以对应项目原文件为准。
