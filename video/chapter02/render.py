"""Build 1080p narrated source walkthrough using local assets and FFmpeg."""
import json, math, pathlib, subprocess, wave, textwrap
from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parent
ASSETS = ROOT / 'assets'
OUT = ROOT / 'output'
OUT.mkdir(exist_ok=True)
scenes = json.loads((ROOT / 'scenes.json').read_text(encoding='utf-8-sig'))
records = json.loads((ASSETS / 'audio-index.json').read_text(encoding='utf-8-sig'))
FONT = 'C:/Windows/Fonts/msyh.ttc'
BOLD = 'C:/Windows/Fonts/msyhbd.ttc'
MONO = 'C:/Windows/Fonts/consola.ttf'
BG, PANEL, WHITE, MUTED, CYAN, GREEN = '#0c1424', '#16243a', '#edf4fc', '#a8b8ce', '#54d6ef', '#a0e7b0'

def font(size, bold=False, mono=False):
    return ImageFont.truetype(MONO if mono else BOLD if bold else FONT, size)

def wrap(draw, text, f, width):
    lines, line = [], ''
    for char in text:
        if draw.textlength(line + char, font=f) > width:
            lines.append(line)
            line = char
        else:
            line += char
    if line:
        lines.append(line)
    return lines

def fit(draw, text, size, width, bold=False, mono=False):
    while draw.textlength(text, font=font(size,bold,mono)) > width and size > 18:
        size -= 1
    return font(size,bold,mono)

def make_frame(scene_id, sentence_id, caption, progress):
    scene = scenes[scene_id]
    img = Image.new('RGB', (1920,1080), BG)
    d = ImageDraw.Draw(img)
    d.rectangle((0,0,1920,7), fill=PANEL)
    d.rectangle((0,0,int(1920*progress),7), fill=CYAN)
    d.text((90,44),'ROS 2 HUMBLE / SOURCE WALKTHROUGH',font=font(24,True),fill=CYAN)
    d.text((90,90),scene['title'],font=fit(d,scene['title'],59,1700,True),fill=WHITE)
    d.text((1740,45),f'{scene_id+1:02d} / 25',font=font(25),fill=MUTED)
    d.line((90,185,1830,185),fill='#32435e',width=2)
    has_code = bool(scene.get('code'))
    has_flow = bool(scene.get('flow'))
    y = 220
    if has_flow:
        flow = scene['flow']
        w = (1660 - 90*(len(flow)-1))/len(flow)
        highlight = min(sentence_id//2,len(flow)-1)
        for i, label in enumerate(flow):
            x = 130+i*(w+90)
            d.rounded_rectangle((x,y,x+w,y+135),radius=18,fill=PANEL,outline=CYAN if i==highlight else '#334863',width=3)
            lines=wrap(d,label,font(31,True),w-45)
            for k,line in enumerate(lines):
                d.text((x+(w-d.textlength(line,font=font(31,True)))/2,y+35+k*39),line,font=font(31,True),fill=WHITE)
            if i < len(flow)-1:
                ax=x+w+21
                d.line((ax,y+68,ax+47,y+68),fill=CYAN,width=4)
                d.polygon([(ax+48,y+68),(ax+34,y+59),(ax+34,y+77)],fill=CYAN)
        y += 165
    if has_code:
        code=scene['code']
        code_size=31
        for line in code:
            code_size=min(code_size,fit(d,line,code_size,1590,mono=True).size)
        height=len(code)*49+45
        d.rounded_rectangle((100,y,1820,y+height),radius=18,fill='#101c2f',outline='#334863',width=2)
        for k,line in enumerate(code):
            active=min(sentence_id,len(code)-1)==k
            if active:
                d.rounded_rectangle((119,y+15+k*49,1800,y+60+k*49),radius=5,fill='#223c4a')
            d.text((128,y+21+k*49),f'{k+1:02d}',font=font(23,mono=True),fill=MUTED)
            d.text((190,y+17+k*49),line,font=font(code_size,mono=True),fill=GREEN if active else WHITE)
        y+=height+25
    points=scene.get('points',[])
    for i,point in enumerate(points):
        if not has_code and not has_flow:
            d.rounded_rectangle((105,y,1815,y+110),radius=18,fill=PANEL)
            d.text((133,y+26),f'0{i+1}',font=font(35,True),fill=CYAN)
            f=fit(d,point,38,1480)
            d.text((220,y+30),point,font=f,fill=WHITE)
            y+=139
        else:
            d.ellipse((115,y+13,129,y+27),fill=CYAN)
            d.text((155,y),point,font=fit(d,point,29,1620),fill=MUTED)
            y+=49
    # Source references remain above the two-line subtitle area.
    d.text((100,819),'SOURCE  '+scene.get('source','第 2 章 · 源码讲解 / 画面为示意'),font=fit(d,'SOURCE  '+scene.get('source','第 2 章 · 源码讲解 / 画面为示意'),21,1720),fill=MUTED)
    d.rectangle((0,872,1920,1080),fill='#080e19')
    subtitle_font=font(39)
    lines=wrap(d,caption,subtitle_font,1710)
    if len(lines)>3:
        subtitle_font=font(33)
        lines=wrap(d,caption,subtitle_font,1710)
    start=905+(3-len(lines))*15
    for k,line in enumerate(lines):
        d.text(((1920-d.textlength(line,font=subtitle_font))/2,start+k*50),line,font=subtitle_font,fill=WHITE)
    d.text((90,1044),'SHAOMIN XU  /  ros2_humble',font=font(16),fill='#72849f')
    return img

def srt_time(t):
    ms=round(t*1000)
    return f'{ms//3600000:02d}:{ms//60000%60:02d}:{ms//1000%60:02d},{ms%1000:03d}'

audio_chunks=[]
durations=[]
params=None
for record in records:
    with wave.open(str(ASSETS/record['file']),'rb') as w:
        params=params or w.getparams()
        if (w.getnchannels(),w.getsampwidth(),w.getframerate()) != (params.nchannels,params.sampwidth,params.framerate):
            raise ValueError('Inconsistent audio format')
        raw=w.readframes(w.getnframes())
        speech_duration=w.getnframes()/w.getframerate()
    gap=.25
    audio_chunks.append(raw+b'\x00'*(round(params.framerate*gap)*params.nchannels*params.sampwidth))
    durations.append(speech_duration+gap)
total=sum(durations)
with wave.open(str(OUT/'narration.wav'),'wb') as w:
    w.setparams(params)
    for chunk in audio_chunks:
        w.writeframes(chunk)

concat=['ffconcat version 1.0']
srt=[]
timing=[]
elapsed=0
for k,(record,duration) in enumerate(zip(records,durations)):
    image_name=f'frame-{k:03d}.png'
    frame=make_frame(record['scene'],record['sentence'],record['text'],elapsed/total)
    frame.save(ASSETS/image_name)
    if record['sentence']==0:
        frame.save(ASSETS/f'scene-{record["scene"]:02d}.png')
    concat.extend([f"file '{image_name}'",f'duration {duration:.6f}'])
    srt.append(f'{k+1}\n{srt_time(elapsed)} --> {srt_time(elapsed+duration-.1)}\n{record["text"]}\n')
    timing.append(dict(record,start=elapsed,end=elapsed+duration))
    elapsed+=duration
concat.append(f"file 'frame-{len(records)-1:03d}.png'")
(ASSETS/'frames.ffconcat').write_text('\n'.join(concat),encoding='utf-8')
(OUT/'chapter02.zh-CN.srt').write_text('\n'.join(srt),encoding='utf-8-sig')
(OUT/'timing.json').write_text(json.dumps(timing,ensure_ascii=False,indent=2),encoding='utf-8')
script=[]
for i,scene in enumerate(scenes):
    start=next(x['start'] for x in timing if x['scene']==i)
    script.append(f'## {i+1:02d} {scene["title"]} / {int(start)//60:02d}:{int(start)%60:02d}\n\n{scene["narration"]}\n')
(OUT/'narration-script.md').write_text('# 第 2 章视频口播稿\n\n'+'\n'.join(script),encoding='utf-8')
chapter_lines=[]
for i in [0,2,3,6,7,9,12,16,19,22,23,24]:
    start=int(next(x['start'] for x in timing if x['scene']==i))
    chapter_lines.append(f'{start//60:02d}:{start%60:02d} {scenes[i]["title"]}')
publication='''# 发布信息

## 标题

ROS 2 Humble 源码解读：一条消息如何从 Talker 到 Listener？

## 简介

从 talker/listener 示例出发，沿 rclcpp、rcl、rmw 追踪发布路径，再看执行器如何等待、取出消息并分发到用户回调。涵盖消息生成、组件入口、定时器、消息所有权与普通跨进程通信。

本视频采用普通话合成配音。画面为源码摘录与流程示意，不包含实测日志或实验录屏。

书稿与源码版本清单：https://github.com/ShaominXu/ros2_humble

源码来自 ROS 2 demos、rclcpp、rcl、rmw_fastrtps、common_interfaces 等上游项目；版权归原作者，所引代码遵循原文件 Apache-2.0 许可证。

## 时间目录

'''+ '\n'.join(chapter_lines) + '\n\n## 标签建议\n\nROS2、Humble、源码解读、机器人、C++、DDS\n'
(OUT/'publication.md').write_text(publication,encoding='utf-8')
# Dedicated clean thumbnail; no narration subtitle.
cover=Image.new('RGB',(1920,1080),BG)
d=ImageDraw.Draw(cover)
d.text((120,115),'ROS 2 HUMBLE  /  源码解读',font=font(38,True),fill=CYAN)
d.text((120,240),'一条消息的旅程',font=font(110,True),fill=WHITE)
d.text((127,410),'Talker → Listener',font=font(78,True),fill=GREEN)
for i,label in enumerate(['发布','中间件','回调']):
    x=130+i*570
    d.rounded_rectangle((x,640,x+460,785),radius=24,fill=PANEL,outline=CYAN,width=3)
    d.text((x+110,676),label,font=font(49,True),fill=WHITE)
    if i<2:
        d.text((x+475,678),'→',font=font(57),fill=CYAN)
d.text((130,940),'第二章  ·  SHAOMIN XU',font=font(29),fill=MUTED)
cover.save(OUT/'cover.png')
sheet=Image.new('RGB',(1920,1080),'#1e2738')
for idx,scene_id in enumerate([0,4,5,6,9,13,14,17,22]):
    thumb=Image.open(ASSETS/f'scene-{scene_id:02d}.png').resize((640,360))
    sheet.paste(thumb,((idx%3)*640,(idx//3)*360))
sheet.save(OUT/'contact-sheet.jpg',quality=92)
print(json.dumps({'duration_seconds':round(total,3),'sentences':len(records),'scenes':len(scenes)},ensure_ascii=False),flush=True)
cmd=['ffmpeg','-hide_banner','-y','-f','concat','-safe','0','-i',str(ASSETS/'frames.ffconcat'),'-i',str(OUT/'narration.wav'),'-vf','fps=30,fade=t=in:st=0:d=0.7',
     '-c:v','libx264','-preset','veryfast','-crf','21','-threads','4','-pix_fmt','yuv420p',
     '-af','loudnorm=I=-16:TP=-1.5:LRA=11','-c:a','aac','-ar','48000','-b:a','192k','-t',f'{total:.6f}',
     '-movflags','+faststart',str(OUT/'ros2-humble-chapter02.mp4')]
with (ASSETS/'encode.log').open('w',encoding='utf-8') as log:
    subprocess.run(cmd,stdout=log,stderr=log,check=True)
print('Video rendered successfully',flush=True)
