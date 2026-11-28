import urllib.request, re, json, hashlib, collections
url="https://github.com/user-attachments/files/35531319/VitaPlus-Diagnostic-27-11-2026-10-28.log"
with urllib.request.urlopen(urllib.request.Request(url,headers={"User-Agent":"VitaPlus diagnostic review"}),timeout=25) as r: data=r.read(8*1024*1024+1)
if len(data)>8*1024*1024: raise ValueError("size")
lines=data.decode("utf-8",errors="replace").splitlines()
metric=set("input_poll_gap input_publish_call main_mutex_wait callback_mutex_wait callback_work frame_assembly ready_to_enqueue queue_wait decode_prepare decode display_wait submitted_first_packet_to_present submitted_ready_to_present log_write_call log_mutex_wait feedback.sample_to_publish feedback.publish_to_wake feedback.sample_to_state_send feedback.sample_to_history_send feedback.state_send_call feedback.history_send_call benchmark.au_enqueue_to_present benchmark.presentation_interval".split())
counter=set("displayed submitted audio_frames fec_repaired fec_failed reorder_late reorder_missing input_updates input_poll_failures feedback_published feedback_consumed feedback_coalesced feedback_send_failures feedback_logs_suppressed input_neutral_consumed input_neutral_failed recovery_episodes decrypted_video_packets decrypted_audio_packets capture_frames heartbeats_sent replay_submitted replay_displayed replay_drained replay_drain_empty feedback_state_sends benchmark.min_us".split())
events=set("allocation.begin allocation.complete allocation.stage allocation.phase allocation.http allocation.reason allocation.network.init allocation.network.release stream_probe.begin stream_probe.complete stream.video.slice_guard_rejected replay.begin replay.complete recovery.wifi recovery.sleep app.start app.stop attempt.begin attempt.end ui.stage account.complete auth.phase https.phase https.curl catalog.phase stream.startup.transport stream.startup.handshake stream.startup.streaminfo stream.startup.decoder stream.startup.ready stream.startup.transport_failed stream.startup.handshake_failed stream.startup.streaminfo_interrupted stream.startup.streaminfo_invalid stream.startup.profile_unavailable stream.startup.audio_init_failed stream.startup.video_header_invalid stream.startup.header_guard_rejected stream.startup.decoder_init_failed stream.startup.streaminfo_reply_failed stream.startup.provision_invalid stream.startup.media_disconnected stream.startup.media_decrypt_failed stream.startup.media_invalid stream.startup.first_frame_timeout stream.startup.ps3_feedback_started".split())
allowed=set("count total_us max_us invalid p50_us p95_us p99_us budget_dropped tail_retained tail_evicted limit_bytes regular_dropped critical_dropped truncated drain_dropped persisted io_ok enqueue_count enqueue_total_us enqueue_max_us persistence_count persistence_total_us persistence_max_us".split())
def nums(s): return {k:int(v) for k,v in re.findall(r"\b([a-z_][a-z_0-9]*)=(-?\d+)\b",s) if k in allowed}
sessions=[];cur=None
kinds=collections.Counter(); unparsed=0
for line in lines:
 if line.startswith("sink "): print(json.dumps({"sink":nums(line)})); continue
 m=re.match(r"^\d+\s+elapsed_ms=(\d+)\s+utc=\S+\s+([A-Z]+)\s+code=(-?\d+)\s+(.*)$",line)
 if not m:
  unparsed+=1;continue
 elapsed,kind,code,msg=m.groups(); elapsed=int(elapsed);code=int(code);kinds[kind if kind in ("BUILD","EVENT","LIB","METRIC","COUNTER","UI","STAGE","ARTWORK") else "other"]+=1
 if kind=="BUILD":
  v=re.search(r"\bbuild=([0-9.]{3,12}(?:[- ]?[bB]\d{1,2})?)(?:\s|$)",msg)
  print(json.dumps({"build":v.group(1) if v else "unrecognized"}))
 if kind=="EVENT" and msg.split()[0] in ("stream_probe.begin","attempt.begin"):
  if cur is None or msg.split()[0]=="stream_probe.begin" and cur.get("stream_begin") is not None:
   cur={"start_ms":elapsed,"events":[],"metrics":{},"counters":{},"lib":{}}
   sessions.append(cur)
  if msg.split()[0]=="stream_probe.begin":cur["stream_begin"]=elapsed
 if cur is None:continue
 cur["last_ms"]=elapsed
 if kind=="EVENT" and msg.split()[0] in events and msg.split()[0] not in ("ui.stage","https.phase","auth.phase","catalog.phase","allocation.http","allocation.phase","allocation.stage"):
  if len(cur["events"])<80:cur["events"].append([elapsed,msg.split()[0],code])
 if kind=="METRIC":
  name=re.search(r"\bmetric=([\w.]+)",msg)
  if name and name.group(1) in metric:cur["metrics"][name.group(1)]=nums(msg)
 if kind=="COUNTER":
  name=re.search(r"\bcounter=([\w.]+)",msg)
  if name and name.group(1) in counter:cur["counters"][name.group(1)]=nums(msg).get("count")
 if kind=="LIB" and msg in ("library.info","library.warning","library.error"):cur["lib"][msg]=cur["lib"].get(msg,0)+1
 if kind=="EVENT" and msg=="stream_probe.complete":cur["stream_end"]=elapsed
print(json.dumps({"lines":len(lines),"kinds":dict(kinds),"unparsed":unparsed,"sessions":len(sessions)}))
for j,s in enumerate(sessions):print(json.dumps({"session":j+1,**s}))
