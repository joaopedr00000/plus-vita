# Library and cover preview — VitaPlus v0.0.5

Included in v0.0.5, based on v0.0.4. Earlier release packages remain unchanged.

- Compact title library on the left; search and platform filter stay above it.
- Right panel: cover, title, platform, catalogue identifier (when valid), availability and PS5 warning.
- Single background worker; 350 ms focus dwell for uncached covers, immediate reuse of ready RAM images; visible thumbnail prefetch after 1.2 seconds; 12 decoded RAM slots; cancels obsolete selections and suspends during drag/active operation. Legacy menu retains its existing 3-second dwell.
- Uses existing Sony CDN HTTPS allowlist, no redirects, no credentials, bounded DNS and transfer, verified JPEG/PNG decoder. No search service, title guess or third-party artwork API.
- Persistent local decoded artwork cache: ux0:data/VitaPlus-Covers, 64 fixed slots (~4 MiB) and one temporary file. Full URL fingerprints and pixel checksum; no raw URL, title list, account identifier, token or trophy data stored. Local artwork can reveal games browsed; folder can be deleted when app is closed. This is not encrypted storage. Cache corruption or storage errors fall back to download. Hashes are noncryptographic cache keys, not security checks.
- Hours and trophies marked unavailable, not zero. Pylux unified schema provides artwork/identifier but not playtime or trophies. Pylux has a separate authenticated trophy API (psn-api/test_trophy_api.py and GUI TrophyListDialog.qml); the VitaPlus token scopes are different and the title/communication-ID mapping requires additional validation. Do not silently broaden OAuth permissions or match account progress by title text.
- No changes to streaming settings, input, authentication or entitlement checks. Physical Vita validation remains pending.

Reference: Pylux pinned ca87a49a12bab071948f565ca53ffdad790775d3, lib/include/chiaki/cloudcatalog.h and psn-api/test_trophy_api.py. The exact GreenVita fork was not accessible in this review; independent adaptation of VitaPlus's existing downloader.
