# Native persisted diagnostics

This contract applies to the native Vita app. The desktop reference recorder
remains memory-only under [diagnostics-policy.md](diagnostics-policy.md).

VitaPlus creates one log per graphical app launch at `ux0:data/VitaPlus-Diagnostic-DD-MM-YYYY-HH-MM.log`.
The name uses the console RTC in UTC (day-month-year-hour-minute). A second launch within the same minute overwrites the same name. If the clock cannot be formatted, the timestamp is all zeroes.
Opening the app replaces the previous log; Play and retry actions append to the
same bounded session log. The offline PS3 replay retains its separate path.
There is no archive or automatic upload. Copy the log before reopening the app.

After the new header is persisted, normal app startup attempts to unlink these
exact historical filenames in `ux0:data/` (missing files are harmless):

- `plus-vita-allocation.log` (original, beta1/2 and early beta3)
- `plus-vita-v0.0.1.log` (v0.0.1, v0.0.2 and PS5 experimental release)
- `plus-vita-catalog-r2.log` (catalogue r2)
- `plus-vita-catalog-r3.log` (catalogue r3)
- `plus-vita-ps3-replay-53.log` (offline PS3 replay diagnostic)

The former `VitaPlus.log` and previous timestamped `VitaPlus-Diagnostic-DD-MM-YYYY-HH-MM.log` files are also removed after successful initialization. The current file is excluded. Only the exact prefix, numeric timestamp and `.log` suffix are matched. No recursive cleanup, account-file removal or unrelated log removal is performed. Failed filesystem operations cannot guarantee deletion. The standalone offline replay build does not perform this cleanup.

The current development identity is `recovery-diagnostics-1`, with the accepted
720p independent-input/deferred-writer profile. This is an experimental
candidate; [the stable graphical baseline](STABLE_GRAPHICAL_BASELINE.md) remains
the hardware comparison. It is not the public beta or a passed #22 gate.

## Persisted schema

Only locally generated records reach the writer:

- Fixed public build/profile constants, a local sequence and relative elapsed
  milliseconds. Sequence and elapsed origin restart for each app launch.
  The header records console RTC UTC at launch; records include UTC derived
  from that origin and monotonic elapsed milliseconds.
- Exact known allocation, stream, recovery, account-completion and UI-stage
  event names. Unknown names are dropped. Numeric result codes are limited to
  the native error range `-1..21`; unknown codes map to `-1`. Allocation stages
  are `0..5`; UI stages are the public screen enum `0..17` in `app.h`.
- Typed `stream.startup.*` events identify the transport, handshake, STREAMINFO
  and decoder stages, followed by `ready` when the client starts waiting for
  media. Failure events distinguish invalid provision data, transport or
  handshake failure, an interrupted STREAMINFO exchange,
  invalid STREAMINFO, unavailable 720p H.264 profile, invalid video header,
  audited SPS/PPS rejection, decoder or audio initialization, and reply send
  failure. An early disconnect, decryption error, media validation failure or
  first-frame timeout also gets a code before the first decoded picture.
  These numeric values are startup status codes from
  `PlusVitaStreamStartup`. The terminal `stream_probe.complete` retains the
  native error code.
- Fixed library severity categories (`library.info`, `library.warning`,
  `library.error`). The library callback discards its entire text parameter,
  including purported summaries, addresses and remote disconnect reasons.
  Dynamic progress maps to `working`; the Play adapter supplies fixed stage
  numbers independently.
- Exact approved metric/counter names from `diagnostic_log.c`, aggregate
  durations/counts, UI draw/present/sample-to-present/poll aggregates and fixed
  artwork-result counters. Unknown metric/counter names and enum values are
  dropped. These are local observations, not physical input response latency.

No account/game text, payload, URL, address, port, identifier, secret, remote
reason or arbitrary key/value field is accepted. Only locally generated UTC timestamps are permitted. Numeric
APIs are for their documented aggregate quantities only. New fields require
explicit schema review and persistent-boundary canary tests.

## Bounds and ownership

Each record has a 1024-byte bound. Deferred queues hold 128 ordinary and 64
critical records. Ordinary overflow drops new records; critical overflow keeps
the newest records. The whole file is at most **1 MiB**, including header,
terminal records and loss summaries. Once the body quota is exhausted, a
64-record memory reserve keeps the newest known terminal/error records for
shutdown. Final typed stream metrics and counters use the critical reserve.
Sink summaries account for queue, quota, truncation and drain losses.
`persisted` counts numbered records actually written, including the retained
tail; `tail_retained` identifies that reserve, `tail_evicted` counts its replaced
records, and `budget_dropped` counts actual quota losses (including evictions).

Completed, failed and cancelled Play attempts retain their diagnostic records
in the same session log. The writer flushes each persisted record, and
`attempt.begin` / `attempt.end` delimit attempts. The file closes on app exit.
UI timing snapshots remain bounded over the whole launch.

The deferred worker owns open, header, writes, flush and close. Producers do no
file I/O. Availability remains false until initialization actually persists;
Settings updates from that status. Failed/full storage disables successful
persistence without falling back to raw error output. Shutdown/reset waits at
most two seconds for the writer. A stuck device call retains one bounded worker,
file and buffers; reset fails rather than reopening or reusing them. Playback
can proceed with diagnostics unavailable. This does not guarantee that the
device's own filesystem syscall can be interrupted.

Host checks: `tools/test-deferred-log.sh` exercises real file output with
synthetic private-data canaries, oversized/unknown inputs, whole-file bounds,
final retention, overflow, reset/sequence, blocked startup/writes, full/unavailable
storage, failed thread creation and writer ownership. `tools/test-latency.sh`
and `tools/test-input-worker.sh` retain the accepted timing/input checks.
Vita retrieval, sleep/Wi-Fi cleanup and playback comparison remain hardware
checks in [RECOVERY_DIAGNOSTICS_TEST.md](RECOVERY_DIAGNOSTICS_TEST.md).

The v001 diagnostic VPK for issue #52 uses the `ps3-startup-52` log identity.
Copy the log after each title before selecting Play again. Compare the affected
PS3 title with a known-good PS4 title.

The PS3 media startup experiment is built with
`-DPLUS_VITA_PS3_STARTUP_SAMPLE=ON`. It installs as **Plus-Vita PS3 Test**
(`PLVTPS352`) with log identity `ps3-media-start-52`. Only Play sessions
resolved by provisioning as PS3 collect video instead of displaying it.
PS4 sessions use the normal playback path.
The sample VPK follows the release build's startup path without requiring
the development replay fixture. The live PS4 SPS/PPS and slice checks still
run during Play.

The PS3 experiment uses the normal neutral controller feedback sender and
Pylux's one-second heartbeat after STREAMINFO, with up to 60 seconds to
receive a sample. It saves the unmodified headers and, if received, up to
five seconds of contiguous video from an IDR in
`ux0:data/plus-vita-ps3-52-media-*.h264`. The log records decrypted media
packet, captured frame, feedback send and heartbeat counts. A partial file can
contain headers alone. Save the log and sample before another Play attempt.

Issue #53 uses `-DPLUS_VITA_PS3_REPLAY=ON` to build **Plus-Vita PS3 Replay**
(`PLVTPS353`, log identity `ps3-replay-53`). Place the private framed capture
at `ux0:data/plus-vita-ps3-replay-53.bin` and launch the app. The replay
checks the file digest, verifies the original PS3 SPS/PPS and checks every
slice before decoding. It uses a five-slot SPS that changes only the original
nine reference and buffering declarations. Software decode produced identical
output across all 300 pictures. The fixture stays in private local storage.

Read `ux0:data/plus-vita-ps3-replay-53.log` after the result screen. A pass
records `replay.complete` code 0, 300 each in `replay_submitted` and
`replay_displayed`, and `replay_drain_empty` equal to 1.
`replay_drained` counts pictures displayed during the drain.
`tools/test-h264-config.sh` also accepts the private replay file as an
optional local regression fixture.

On physical Vita, replay build `ps3-replay-53` submitted and displayed all
300 pictures, including six displayed during the drain. The final drain was
empty, the result was code 0 in 5.632 seconds, and all 35 diagnostic records
persisted. The maintainer confirmed the picture.

Issue #54 uses `-DPLUS_VITA_PS3_LIVE=ON` to build **Plus-Vita PS3 Play**
(`PLVTPS354`, log identity `ps3-live-54-2`). In normal catalogue Play, PS3
sessions use the audited header and slice guard and start controller feedback
and one-second heartbeats after STREAMINFO. They allow up to 60 seconds for
the first picture. A rejected slice records `stream.video.slice_guard_rejected`
with a fixed invalid-data code and an allowlisted parser reason. The original
`ps3-live-54` build recorded the event without a reason. Existing packet and heartbeat counters cover
the PS3 media handoff. PS4 uses its existing decoder and startup policy.

On physical Vita, launch the original PS3 title and verify picture, sound,
controls, stop and catalogue return. Relaunch it and repeat on a known-good
PS4 title in the same VPK. Copy `ux0:data/plus-vita-allocation.log` after each
run, before the next Play or app startup replaces it.

The maintainer reported that both the original PS3 title and the known-good
PS4 title passed the playback, stop, catalogue return and relaunch checks on
`ps3-live-54-2` (commit `77afd68`, package version `00.02`). The PS4 result
agrees with the accepted public v001 playback baseline by tester observation.
The earlier `ps3-live-54` PS3 run displayed 9,162 pictures and processed
16,368 audio frames before a slice guard rejection ended the session.
The maintainer supplied one `ps3-live-54-2` log. Its startup and final
counters indicate the PS4 path because the PS3 feedback event and heartbeat
counter are absent and all 1,509 records persisted without drops. The run
displayed 7,632 pictures and processed 16,305 audio frames in 2 minutes
43 seconds from stream probe start to completion. It consumed neutral input
on exit, recorded zero feedback send failures and completed with code 0.
The updated PS3 run remains supported by the maintainer's report.

## On-screen streaming diagnostics

A failed streaming attempt can display the last observed allocation phase (0–11) or video startup stage, most recent HTTP status and allowlisted service reason, native error code, and requested client/video profiles. `N/A` means unavailable. These fields are also available without file persistence; they are reset before each attempt. Only service codes 002.0011, 002.0019 and 002.2001 are shown verbatim from the fixed mapping; R4–R11 identify other bounded reason categories from cloud_response.h. The requested H.264 profile is not proof of the actual server codec. No raw payload, token, session ID or URL is rendered. A screenshot of the technical details can help when a log cannot be attached.
