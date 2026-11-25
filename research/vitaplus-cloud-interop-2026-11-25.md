# PS5 cloud interoperability investigation — 2026-11-25

References: Pylux issues #63/#67 (allocation failures), Con-app v0.5.2 and issue #4 (HTTP 401 on some platforms), PS5 Cloud Play, Chiaki NG Cloud, CloudPad Android, Asobi's November 17 status, OpenPS Stream (cloud work in progress).

Changes: consume validated client IDs from the authenticated session response where provided; accept a direct datacenter array or one recognized envelope; record bounded HTTP status and enumerated error reasons only. Preserve existing H.264 capability declaration and account authorization checks. No device-identity or entitlement changes, no fake codec support, no forced retry of authorization failures. No proof of PS5 cloud playback on Vita.

Diagnostic allocation.reason mapping: 0 unknown, 1 authorization refusal (002.0011), 2 ping eligibility (002.0019), 3 subscription required (002.2001), 4 missing game entitlement, 5 invalid game specification, 6 no available datacenter, 7 session not locked, 8 allocation failure, 9 device not activated, 10 not authorized, 11 no capacity. HTTP status is recorded separately; allocation.phase identifies the stage. Never save the response body, headers, session IDs or tokens.
