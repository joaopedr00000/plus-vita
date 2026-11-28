import urllib.request, re, json, hashlib, statistics
urls=[
"https://github.com/user-attachments/files/35531319/VitaPlus-Diagnostic-27-11-2026-10-28.log",
"https://github.com/user-attachments/files/35531147/VitaPlus-Diagnostic-27-11-2026-10-28.log"]
patterns={
"audio_timing":r"audio timing (?:periodic|final)",
"audio_summary":r"audio summary",
"audio_pcm":r"audio PCM levels",
"audio_port":r"audio playback started",
"video_timing":r"Stream probe timing (?:submitted|summary)",
"video_display":r"Stream probe display timing",
"video_summary":r"Stream probe preview summary",
"video_recovery":r"Stream probe recovery summary",
"network":r"(?:packet loss|packet_loss|network stats|connection stats|server quality|rtt|bitrate)",
"audio_error":r"audio (?:output failed|decode failed|worker failed)",
"options":r"Stream probe options mask",
"feedback":r"Stream probe feedback summary",
"reorder":r"(?:reorder summary|reorder stats|reorder grace|reorder adjusted)"}
allowed=set("decoded arrived arrival_gap_max_us arrival_gt20ms arrival_gt40ms empty_waits empty_wait_max_us empty_gt20ms output_gap_max_us output_gap_stalls output_call_max_us output_call_stalls stall_budget_us queue_age_avg_us queue_age_max_us queue_now seq_gaps seq_resets max_missing skipped_local outputs concealed stale dropped queue_peak failed drain release peak_L peak_R rms_L rms_R queue block port_len port_freq port_mode submitted displayed skipped prepare_us decode_us queue_wait_us elapsed_ms complete incomplete audio result episodes dropped_dependent awaiting_idr validated_idrs prepare_total_us decode_total_us decode_max_us cpu_scale_us submit_total_us vblank_total_us vblank_max_us queue_wait_max_us rtt_ms rtt bitrate_kbps bitrate packet_loss fps lost received sent server_quality reorder_initial_us mask".split())
for idx,url in enumerate(urls):
 try:
  req=urllib.request.Request(url,headers={"User-Agent":"VitaPlus diagnostic review","Accept":"application/octet-stream"})
  with urllib.request.urlopen(req,timeout=25) as response: data=response.read(8*1024*1024+1)
  if len(data)>8*1024*1024: raise ValueError("size")
  lines=data.decode("utf-8",errors="replace").splitlines()
  print(json.dumps({"attachment":idx+1,"bytes":len(data),"lines":len(lines),"sha256":hashlib.sha256(data).hexdigest()}))
  for kind,pat in patterns.items():
   records=[]
   for line in lines:
    if re.search(pat,line,re.I):
     record={k:v for k,v in re.findall(r"\b([A-Za-z_][A-Za-z_0-9]*)=(0x[0-9a-fA-F]+|-?\d+(?:\.\d+)?)\b",line) if k in allowed}
     records.append(record)
   if records:
    print(json.dumps({"kind":kind,"count":len(records),"first":records[0],"last":records[-1],"sample":records[-4:] if kind in ("audio_summary","audio_timing","video_summary","video_recovery","audio_error") else records[:3]}))
 except Exception as e: print(json.dumps({"attachment":idx+1,"fetch_failed":type(e).__name__}))
