# VitaPlus clean development candidate

Direct parent: published v0.0.5-2 (ccce864310dc6694a243915be7c4e2edcc99fc1e). No preset theme collection, decorative icons, wallpaper worker or changes to published VPKs.

- Appearance: persistent independent RGB primary/secondary colors, native hex entry, preview swatches, restore original gold. The original logo remains. No bundled themes or wallpaper support in this candidate.
- Menus: native catalogue button surface, focus colors, original header, live RTC/battery/network indicators, account/setup panel and QR frame. Existing library layout, cover aspect ratio, filters and game actions retained. System dialogs remain native.
- Latency: independently adapted GreenVita v0.3.3 ideas: 40/40/60 ms initial audio buffer, bounded adaptive growth, small backlog-only PCM shortening, 150 ms cover focus and 600 ms visible prefetch, reduced redundant handheld controller enumeration, 60 s local CPU-side timing benchmark after warmup. Existing 4 ms polling, reference ordering, display synchronization and native decoder protections remain. No claimed reduction in physical input lag; validate on hardware. GreenVita uses WebRTC/SDL, not VitaPlus' native Takion/AVC pipeline.
- PS5: Con v0.5.2 (https://github.com/houqai/Con-app/releases/tag/v0.5.2) fixed 002.0011 using PC session envelope; independent adaptation of its public protocol fields only for the PS5 configuration. Existing PS5 OAuth, protocol 12/cronos, measured RTT, region and authorization rules retained. The Vita still requests AVC and rejects unsupported HEVC. Con's HEVC playback does not establish Vita playback compatibility. PS5 remains experimental/unavailable for confirmed gameplay.

Development history for v0.1.0. GitHub Actions must pass before publication; physical-device validation of streaming and migration remains required.

## Storage organization

All native adaptive user data is organized under `ux0:data/VitaPlus`: Account, Settings, Imports, Cache/Covers, Cache/PS5-Catalog, Logs/Archive and Captures. Wallpapers and Cache/Themes are reserved and existing local files are migrated, but the wallpaper renderer is not part of this clean candidate. Only dedicated automatic vault key files are moved to `ur0:data/VitaPlus`, keeping the key on a separate partition from the encrypted vault. Startup migration is idempotent, uses same-volume renames, compares duplicates byte-for-byte before removing them, refuses conflicting files, and does not recurse through unrelated ur0 directories. Back up the old and new dedicated directories privately before testing. Downgrading may require manually restoring the original paths. No system or jailbreak files are touched. Current logs retain the existing single-active-log policy; legacy logs are archived. Host regression tests cover idempotency, conflicting files, key length validation, and unrelated file preservation.

## Shared controller glyphs

The catalogue's existing circular PlayStation button artwork is reused in native menu footers: Cross selects, Circle goes back or cancels, Square saves the diagnostic capture where supported, and a directional glyph indicates navigation. Only actual commands are displayed; notices stay textual. Localized labels derive from the existing locale tables. Catalogue geometry, pixels of the original four controller icons, button mappings, account state, streaming and storage are unchanged. QR camera preview stops above the footer. Hardware visual validation remains required.

## Publication hold and update channels

Official v0.1.0 is blocked until the maintainer confirms actual playable PS5 cloud streaming on physical Vita hardware. A successful CI build is insufficient. Stable accepts regular releases; Beta also accepts official GitHub prereleases tagged vX.Y.Z-bN, displayed X.Y.Z BN, where X.Y.Z is the originating stable version (0.0.6 B1, B2, B3, then 0.0.7 stable). No automatic downgrade. Verified VPKs use ux0:Downloads/. No release authorized by this change.
