$ErrorActionPreference = 'Stop'
$videoRoot = $PSScriptRoot
$scenes = Get-Content -Raw -Encoding utf8 (Join-Path $videoRoot 'scenes.json') | ConvertFrom-Json
$voiceToken = New-Object -ComObject SAPI.SpObjectToken
$voiceToken.SetId('HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech_OneCore\Voices\Tokens\MSTTS_V110_zhCN_HuihuiM')
$voice = New-Object -ComObject SAPI.SpVoice
$voice.Voice = $voiceToken
$voice.Rate = 2
$voice.Volume = 100
$records = @()
for ($i = 0; $i -lt $scenes.Count; $i++) {
  $sentences = [regex]::Matches($scenes[$i].narration, '[^。！？]+[。！？]?')
  $j = 0
  foreach ($sentence in $sentences) {
    $caption = $sentence.Value.Trim()
    if (-not $caption) { continue }
    $speech = $caption.Replace('Talker','发布节点').Replace('Listener','订阅节点').Replace('Humble','汉波').Replace('Publishing','正在发布').Replace('I heard','我听到了').Replace('QoS','服务质量').Replace('Fast DDS','Fast D D S').Replace('DDS','D D S').Replace('SerializedData','序列化数据').Replace('CDR','C D R').Replace('unique','独占').Replace('move','移动').Replace('publish','发布').Replace('main','主函数').Replace('spin','循环执行').Replace('String','字符串').Replace('Node','节点').Replace('chatter','聊天话题').Replace('data','数据').Replace('hpp','头文件').Replace('msg','消息定义').Replace('rcl','R C L').Replace('rmw','R M W').Replace('wait','等待').Replace('work','工作').Replace('taken','取出成功').Replace('take','取出').Replace('lambda','匿名函数')
    $fileName = '{0:D2}-{1:D2}.wav' -f $i,$j
    $audioPath = Join-Path $videoRoot "assets/$fileName"
    $stream = New-Object -ComObject SAPI.SpFileStream
    $stream.Open($audioPath,3,$false)
    $voice.AudioOutputStream = $stream
    $voice.Speak($speech) | Out-Null
    $stream.Close()
    $records += [ordered]@{scene=$i; sentence=$j; text=$caption; file=$fileName}
    $j++
  }
  Write-Output ('Narration {0}/{1}' -f ($i+1),$scenes.Count)
}
$records | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $videoRoot 'assets/audio-index.json')
