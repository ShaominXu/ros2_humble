# ROS 2 Humble 源码解读

以 ROS 2 Humble 源码为基础，沿节点创建、消息通信和回调执行逐步解读框架，最终整理为可阅读、可复现的书稿。

## 阅读入口

- [工作区整体解读](docs/project-overview.md)
- [写作项目与成书路线](docs/book/README.md)
- [全书目录草案](docs/book/outline.md)
- [第 2 章：从 talker/listener 追踪一条消息](docs/book/chapter-02-message-journey.md)
- [章节模板](docs/book/chapter-template.md)

当前章节已按本地源码核验关键调用，构建与实验尚未验证。

## 安装环境

请参考 [ROS 2 Humble 官方安装指南](https://docs.ros.org/en/humble/Installation.html)，选择适合操作系统的安装方式。

## 源码基线与恢复

本仓库保存书稿和源码版本清单。`src/` 为本地阅读目录，不提交到本仓库。[sources.repos](sources.repos) 固定了写作基线中的 104 个上游仓库及其提交号；[源码状态记录](docs/source-state.json) 保存清单采集时的状态，当时所有仓库均无本地修改。

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

清单采用 JSON 表示，也是有效的 YAML。恢复需要能够访问上游地址，且上游仍提供对应提交。此步骤恢复源码，不负责安装构建依赖或完成编译；具体环境与实验记录会随章节补充。

章节中的源码链接指向恢复后的本地 `src/`，在 GitHub 网页上直接点击这些链接不会显示上游源码。建议克隆并恢复源码后使用本地 Markdown 阅读器。

## 项目组织

```text
docs/                  工作区说明与书稿
  book/                成书路线、目录、章节与模板
sources.repos          上游源码地址和精确提交号
src/                   本地源码，由版本清单恢复
```

引用的上游源码版权与许可证以对应项目原文件为准。
