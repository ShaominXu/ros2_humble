# ROS 2 Humble 源码解读

以 ROS 2 Humble 源码为基础，沿节点创建、消息通信和回调执行逐步解读框架，最终整理为可阅读、可复现的书稿。

## 阅读入口

- [工作区整体解读](docs/project-overview.md)
- [写作项目与成书路线](docs/book/README.md)
- [全书目录草案](docs/book/outline.md)
- [第 1 章：工作区、包与版本基线](docs/book/chapter-01-workspace-and-baseline.md)
- [第 2 章：从 talker/listener 追踪一条消息](docs/book/chapter-02-message-journey.md)
- [第 3 章：ROS 2 分层与资源生命周期](docs/book/chapter-03-layers-and-lifetime.md)
- [第 4 章：初始化、Context 与 Node](docs/book/chapter-04-initialization-context-node.md)
- [第 5 章：消息定义如何变成代码](docs/book/chapter-05-interface-generation.md)
- [第 6 章：Publisher 与 Subscription](docs/book/chapter-06-publisher-subscription.md)
- [章节模板](docs/book/chapter-template.md)
- [第 2、3 章实验操作与结果记录](docs/book/experiment-guide.md)

当前章节已按本地源码核验关键调用，构建与实验尚未验证。

## 安装环境

请参考 [ROS 2 Humble 官方安装指南](https://docs.ros.org/en/humble/Installation.html)，选择适合操作系统的安装方式。

## 本书对应的源码版本

安装与构建请参考上面的官方指南。若需对照本书阅读源码，请使用 [sources.repos](sources.repos) 获取对应版本：清单固定了 104 个上游仓库的具体提交，避免上游更新后代码与书中分析不一致。`src/` 为本地阅读目录，不提交到本仓库。

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

以上命令仅获取源码，依赖安装与编译按官方指南继续完成。章节中的源码链接指向本地 `src/`，建议获取源码后使用本地 Markdown 阅读器。

## 项目组织

```text
docs/                  工作区说明与书稿
  book/                成书路线、目录、章节与模板
sources.repos          上游源码地址和精确提交号
src/                   本地源码，由版本清单恢复
```

引用的上游源码版权与许可证以对应项目原文件为准。
