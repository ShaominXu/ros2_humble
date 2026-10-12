# 第 2 章介绍视频

内容：第 2 章非实验部分，25 个讲解段落，普通话合成配音，逐句硬字幕，源码摘录与流程图逐步高亮。

成片目标：1920 × 1080，30 fps，H.264 视频，AAC 音频，约 12 分 08 秒。

## 输出文件

生成文件保存在 `output/`，不提交到 GitHub：

- `ros2-humble-chapter02.mp4`：上传用成片，已包含字幕。
- `cover.png`：视频封面。
- `chapter02.zh-CN.srt`：独立中文字幕。
- `narration-script.md`：带段落时间的口播稿。
- `publication.md`：建议标题、简介、章节时间和来源说明。
- `narration.wav`：独立配音音轨。

源文件 `scenes.json` 可修改画面和口播内容。画面里的日志为示意，代码局部简写有注明。本视频不包含实验录屏，也不把书稿中待执行的步骤作为实测结果。

## 本机重新生成

本机制作使用 Windows 中文语音 Microsoft Huihui、Pillow 和 FFmpeg。脚本只读取系统语音，不修改语音注册表。

```powershell
New-Item -ItemType Directory -Force video/chapter02/assets
& ./video/chapter02/synthesize.ps1
python video/chapter02/render.py
```

Python 环境需提供 Pillow，PATH 中需有 FFmpeg，字体路径和语音标识按脚本配置。配音生成脚本使用 Windows COM，不能直接在 Linux 上执行；替换音频索引与对应 WAV 文件后，画面合成仍可适配其他平台。

上游代码来源为 ROS 2 对应仓库，原文件版权和 Apache-2.0 许可证不变，具体路径随画面列出。发布简介中也应保留源码与许可证说明。合成配音会把部分英文术语改读为中文解释，字幕保持讲稿原文。
