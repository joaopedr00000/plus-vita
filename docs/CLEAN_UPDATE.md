# VitaPlus clean development candidate

Direct parent: published v0.0.5-2 (ccce864310dc6694a243915be7c4e2edcc99fc1e). No storage migration, preset theme collection, decorative icons, wallpaper worker or changes to published VPKs.

- Appearance: persistent independent RGB primary/secondary colors, native hex entry, preview swatches, restore original gold. The original logo remains. No bundled themes or wallpaper support in this candidate.
- Menus: native catalogue button surface, focus colors, original header, live RTC/battery/network indicators, account/setup panel and QR frame. Existing library layout, cover aspect ratio, filters and game actions retained. System dialogs remain native.
- Latency: independently adapted GreenVita v0.3.3 ideas: 40/40/60 ms initial audio buffer, bounded adaptive growth, small backlog-only PCM shortening, 150 ms cover focus and 600 ms visible prefetch, reduced redundant handheld controller enumeration, 60 s local CPU-side timing benchmark after warmup. Existing 4 ms polling, reference ordering, display synchronization and native decoder protections remain. No claimed reduction in physical input lag; validate on hardware. GreenVita uses WebRTC/SDL, not VitaPlus' native Takion/AVC pipeline.
- PS5: Con v0.5.2 (https://github.com/houqai/Con-app/releases/tag/v0.5.2) fixed 002.0011 using PC session envelope; independent adaptation of its public protocol fields only for the PS5 configuration. Existing PS5 OAuth, protocol 12/cronos, measured RTT, region and authorization rules retained. The Vita still requests AVC and rejects unsupported HEVC. Con's HEVC playback does not establish Vita playback compatibility. PS5 remains experimental/unavailable for confirmed gameplay.

Test branch only. GitHub Actions must pass; physical device validation remains required.
