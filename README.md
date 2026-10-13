# ROS 2 Humble 源码解读

以 ROS 2 Humble 源码为基础，沿节点创建、消息通信和回调执行逐步解读框架，最终整理为可阅读、可复现的书稿。

## 从源码安装

请参考 [ROS 2 Humble 官方安装指南](https://docs.ros.org/en/humble/Installation.html)，选择对应操作系统的源码安装流程，完成环境准备、依赖安装、编译和环境加载。源码使用本仓库的版本，获取方法见下文。

## 本书对应的源码版本

执行官方源码安装流程中的源码获取步骤时，使用本仓库的 [sources.repos](sources.repos) 替代官方指南中的源码清单。本清单固定了 104 个上游仓库的具体提交，确保源码与本书分析对应。`src/` 为本地源码目录，不提交到本仓库。

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

以上命令仅获取本仓库指定版本的源码。获取完成后，继续按照官方安装指南中对应系统的源码安装流程，完成依赖安装、编译与环境加载。章节中的源码链接指向本地 `src/`，建议获取源码后使用本地 Markdown 阅读器。

引用的上游源码版权与许可证以对应项目原文件为准。
