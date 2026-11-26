# Themes and latency development build

Based on published v0.0.5-2; no existing release replacement. The current test package uses the same application version/title ID and should be treated as a test build, not a new final release.

## GreenVita 0.3.3 comparison

Reviewed joaopedr00000/green-vita-xconsole at b7a671282bdf7cb3c0a501953443411b19794ab1: src/streaming/policy.rs, audio.rs, input.rs, src/app/stream_session/playback.rs and src/api/catalog/worker.rs, plus the v0.3.3 release notes. Independent C adaptations, no copied Rust code. The two clients use different transport and display architectures; GreenVita's WebRTC NACK/RAW dispatch and SDL texture claim must not be copied blindly into Takion/native AVC scanout. Existing VitaPlus 4 ms independent native input and signaled feedback state, H.264 references, double-buffer safety, decoder recovery, Wi-Fi recovery and account validation are retained.

- Initial native Opus queue targets 40/40/60 ms, adaptive increase capped at 110 ms. Local PCM shortening 1–2% when compressed backlog exceeds target, never during startup/fade. Native 960-sample output period unchanged. Small pitch changes possible. Real-device listening required.
- Cover focus delay 150 ms instead of 350 ms; visible thumbnail prefetch after 600 ms instead of 1200 ms; protect visible entries from RAM cache eviction. Still one cancellable worker and bounded disk cache. No network access during streaming.
- First 10 s warmup, then 60 s CPU-side benchmark of submitted compressed AU enqueue to presentation return; histogram upper-bound p50/p95/p99, mean/min/max and frame interval P95. Not physical button-to-screen latency, nor guaranteed age of decoded picture. Logging depends on existing diagnostic policy. No claims of GreenVita's measured 4.99 ms applying to VitaPlus.
- Appearance settings accept arbitrary 24-bit RGB colors (# optional), separate primary and secondary, visual swatches, persistent separate theme preferences and restore original. Logo and original default colors unchanged. New labels localized; detailed color-entry help English fallback outside EN/PT.
- Native handheld input avoids repeated controller-port enumeration for up to 24 additional 4 ms samples (approximately 96 ms), while sampling the actual pad each time. Failed reads fall back immediately. External/TV paths unchanged. Typed benchmark metrics are persisted through the diagnostic allowlist.
- No claim of general PS5 support based on a single user report; request diagnostic log to verify selected platform, actual stream codec and path. Keep experimental warning.

Pending: hardware A/B, analog latency trace, separate decoder/presentation scheduling design and protocol-specific network tuning. The user's joystick plugin workaround is not proof of the root cause. Do not remove display synchronization or increase clocks without testing.
