# Roadmap

## VitaPlus — next implementations (v0.0.3 onward)

### Automatic datacenter failover (planned, not enabled)

- Offer automatic failover only after actual current country AND authenticated PSN account country are verified to be in Sony-supported cloud regions, the subscription/title/session are eligible, and Sony authorizes the candidate datacenters. Unknown or mismatched eligibility defaults to manual.
- A supported account locale alone is not proof of physical location. Do not use UI locale, regional discovery, VPN/proxy, a selected server or self-reported country alone as proof; no external IP lookup or location telemetry without a separately reviewed, consented design.
- Unsupported or unknown countries: retain the manually chosen datacenter. Any permitted reconnect remains on that server.
- For eligible users who opt in: use only service-authorized datacenters, at most one alternate after a recoverable transport error, with backoff and cancellation. A reconnect starts a fresh session.
- Never retry authorization, regional, entitlement, subscription, policy or rate-limit refusals; no identity/region spoofing, forced eligibility, or invented servers.
- Add tests for supported/unsupported/unknown and mismatched regions, denial, retry caps and cancellation, then verify physical Vita playback.

Official requirements: https://www.playstation.com/en-us/support/subscriptions/psportal-cloud-game-streaming/ and https://www.playstation.com/en-us/legal/terms-of-service/.


## Next version - v001 (unreleased)

The maintainer accepted the current Vita-facing development result on 2026-11-04.
Controls, menu interaction, scrolling lists and focused catalogue artwork are
saved locally for [v001](V001.md). Pushing and another VPK release are explicitly
on hold while further changes are considered. PS TV DS3/DS4 hardware validation
remains open in #38. Published v000 and its preserved artifact are unchanged.

## v000 acceptance — 2026-10-27

The maintainer accepts the first v000 version and confirms its 45-minute play,
stop/relaunch, Wi-Fi/sleep recovery, account persistence/Forget and catalogue
search/back-navigation checks passed. Software privacy and package checks pass.
See [v000 identity and acceptance](V000.md). The maintainer authorized public
distribution of the project and VPK with personal account data excluded. Direct Sony
sign-in research (#24) is separate future work, outside v000 completion.

## Current milestone — 2026-10-16

The native Vita streaming proof of concept has reached a maintainer-accepted
**playable pre-release milestone**. The combined input-worker/deferred-log VPK
preserves picture, sound, controls and restart while retaining the perceived
input-response improvement. The scoped input/logging work in issues #1–#12 is
complete. This accepted build is the baseline for release preparation; a public
release is still a subsequent step. See [package identity and scope](PLAYABLE_MILESTONE.md)
and [hardware results](LATENCY_COMBINED_RESULT.md).

The phase descriptions below retain the original feasibility gates and audit
history; their older pending audit items are not a claim that live Vita
streaming remains unimplemented.

Development is gated so Vita-specific work starts only after the service can be
used by an independent desktop reference client.

## Phase 1: PC client feasibility audit

1. [x] Verify and inventory the official installer.
2. [x] Inspect the installed application, native libraries, drivers, codecs,
   and configuration without collecting account data.
3. [ ] Complete protocol feasibility observation:
   - [x] Observe process lifecycle, endpoint allocation, UDP traffic profile,
     IPC/input command surface, and shutdown behavior.
   - [ ] Establish a documented, legitimate independent authentication,
     catalog, and session-allocation contract.
   - [ ] Establish the Takion handshake, cryptographic key lifecycle, message
     schemas, media framing, and controller wire format without bypassing an
     access control.
4. [x] Make the initial go/no-go decision. The original
   [Phase 1 decision](PHASE1_DECISION.md) was no-go based on official-client
   evidence alone. It has now been **reopened** after confirming that Pylux
   publishes a native direct-cloud implementation. See the
   [Pylux source audit](../research/pylux-source-audit.md).

Proceed only if the authenticated protocol can be implemented without bypassing
DRM, authentication, encryption, attestation, or other access controls.

## Phase 2: Desktop reference client

**Reopened using Pylux/libchiaki as the implementation basis.** The existing
local implementation remains an offline simulator; it is not itself a Sony
service client.

Implement the smallest independent client that can authenticate legitimately,
list available titles, allocate a session, receive audio/video, and send
controller input.

## Phase 3: Vita proof of concept

Port the proven protocol client to VitaSDK. Target hardware H.264 decoding,
stereo audio, one controller, touch mappings for missing DS4 inputs, and a
conservative 720p stream profile.

## Phase 4: Productization

Add token lifecycle handling, diagnostics, reconnect support, network recovery,
controller remapping, compatibility testing, packaging, and release
documentation.
