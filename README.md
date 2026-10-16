# ROS 2 Humble 源码解读

围绕节点创建、消息通信和回调执行，解读 ROS 2 Humble 源码。

## 获取源码

[sources.repos](sources.repos) 固定了 104 个上游仓库的提交版本，与书稿分析保持一致。安装 Git 和 vcstool 后，在仓库根目录执行：

```bash
mkdir src
vcs import src < sources.repos
```

PowerShell 使用：

```powershell
New-Item -ItemType Directory -Force src
Get-Content -Raw sources.repos | vcs import src
```

## 编译与阅读

按照 [官方源码安装指南](https://docs.ros.org/en/humble/Installation.html) 完成对应系统的环境准备、依赖安装、编译与环境加载；源码获取步骤使用上述清单。

`src/` 不提交到本仓库。章节中的源码链接指向该目录，建议获取源码后使用本地 Markdown 阅读器。

引用的上游源码版权与许可证以对应项目原文件为准。
