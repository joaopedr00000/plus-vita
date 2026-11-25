<p align="center">
  <img src="docs/readme/vitaplus-logo.jpg" alt="VitaPlus README logo" width="128">
</p>

<h1 align="center">VitaPlus</h1>

<p align="center">
  PlayStation Plus cloud gaming on PlayStation Vita<br>
  An experimental community fork of <a href="https://github.com/Calligula87/plus-vita">Plus-Vita</a>
</p>

<p align="center">
  <a href="https://github.com/joaopedr00000/plus-vita/releases"><img src="https://img.shields.io/badge/Releases-Download-gold" alt="Downloads"></a>
  <a href="https://github.com/joaopedr00000/plus-vita/tree/release/v0.0.3"><img src="https://img.shields.io/badge/v0.0.3-Experimental-orange" alt="Experimental v0.0.3"></a>
  <a href="https://github.com/joaopedr00000/plus-vita/issues"><img src="https://img.shields.io/badge/GitHub-Issues-blue" alt="Report issues"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-AGPL--3.0-blue" alt="AGPL-3.0"></a>
</p>

## Introduction

VitaPlus brings eligible PlayStation Plus cloud games directly to a homebrew-enabled PS Vita, without a PC relay during gameplay. It builds on the original Plus-Vita project and introduces a redesigned interface, local QR-assisted sign-in, encrypted account storage, improved streaming controls and an experimental PS5 Cloud route.

The project started with the ambition of playing GTA VI through cloud streaming on the PS Vita. **PS5 Cloud playback and GTA VI are not confirmed to work.** This is an unofficial homebrew project and is not affiliated with or endorsed by Sony Interactive Entertainment.

## Downloads and version status

- **[Download VitaPlus — GitHub Releases](https://github.com/joaopedr00000/plus-vita/releases)**
- **[v0.0.3 — Experimental source and release notes](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/release/v0.0.3.md)**
- [v0.0.2 — Visual Update](https://github.com/joaopedr00000/plus-vita/releases/tag/v0.0.2)

v0.0.3 is an experimental development version. Download its VPK from Releases when the experimental package is available; an available source branch alone does not mean a binary has passed CI. Physical Vita validation of the newest account and streaming changes remains pending. v0.0.2 is the previous release.

The VitaPlus v0.0.3 package is named `VitaPlus-v0.0.3.vpk` (APP_VER `00.19`, Title ID `PLVTB0001`). Verify the published SHA-256 checksum when available. The same VitaPlus Title ID is retained for an in-place upgrade; original upstream Plus-Vita installations may use a different ID. Back up your account data privately before upgrading.

## Features

- Native cloud streaming on PS Vita; original PS3/PS4 streaming routes retained.
- Experimental PS5 Cloud catalogue discovery and service-provided manual datacenter selection.
- 720p H.264 video profile, with selectable bitrate targets: **6 Mbps (low latency), 10 Mbps (balanced), 15 Mbps (maximum quality)**.
- Adaptive packet reordering and audio buffering, decoder-buffer reuse and bounded network recovery; a permitted recovery may reconnect at a reduced bitrate to the same server.
- Optional performance HUD with local FPS, received bitrate, allocation RTT and stream/decoder diagnostics. These are not measurements of end-to-end input latency.
- Lightweight catalogue with search, PS3/PS4/PS5 platform filters and a text-based list.
- New VitaPlus bubble icon, LiveArea, splash screen and black/graphite/gold/white interface.
- QR-assisted account connection using a phone browser on the local network; no external relay or JSON file transfer required for normal sign-in.
- Encrypted Remember account storage (AES-256-GCM), optional startup password and verified cleanup of matching legacy plaintext imports in v0.0.3.
- Remappable gameplay controls, saved layout and experimental PS TV DualShock 3/4 support.
- Localized interface with 19 language/region variants.
- Privacy-conscious local diagnostic logging and detailed stream failure messages.

Features describe the current experimental source; availability and reliability may differ by version, service response and hardware. PS5 Cloud normally uses HEVC, whereas VitaPlus requires a compatible H.264 stream. A catalogue entry or datacenter list does not guarantee playback.

## Requirements

- A homebrew-enabled PlayStation Vita with VitaShell and working Wi-Fi.
- Your own Sony account and an eligible PlayStation Plus subscription that includes cloud streaming for your actual supported region and account.
- A phone or compatible browser on the same trusted private IPv4 Wi-Fi network for initial QR-assisted sign-in.
- A stable internet connection. A PC relay, external pairing helper and Wi-Fi plugin are not required.

Check [Sony's official cloud streaming requirements](https://www.playstation.com/en-us/support/subscriptions/psportal-cloud-game-streaming/). Service availability, games and subscriptions are controlled by Sony.

## Setup

1. Download the appropriate VitaPlus VPK from [Releases](https://github.com/joaopedr00000/plus-vita/releases), transfer it to your Vita and install it in VitaShell.
2. Connect your Vita and phone to the same trusted private Wi-Fi network.
3. Launch VitaPlus and select **Get started → Continue → Sign in with QR code**.
4. Scan the displayed QR code. It opens a temporary local page served by your Vita.
5. Follow the official PlayStation sign-in link. Complete any password, passkey or verification prompts only on Sony's website.
6. In the same browser, open the Sony sign-in response using the link provided on the local page. Copy the response containing your NPSSO token, paste it into the local page and select **Connect to Vita**.
7. Confirm the connection on the Vita. Once authenticated, browse the catalogue, select an eligible title and choose **Play**. If the app presents a datacenter list, choose a server manually.

**Important:** the local browser-to-Vita HTTP connection is not encrypted. Use only a trusted private network, never public Wi-Fi. Do not share the QR, token, Sony response or clipboard contents. Clear the clipboard after connecting. The app does not need your Sony password.

See the [account setup and recovery guide](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/ACCOUNT_SETUP.md).

## Account security and privacy

Remember is optional. In the current v0.0.3 source:

- Remembered credentials are encrypted using AES-256-GCM with a fresh nonce.
- Automatic unlock uses a random local key kept separately from the encrypted account file. It does not require a password on every launch.
- An optional local startup password can be configured in **Settings and help → Security and privacy**.
- A matching legacy `ux0:data/plus-vita-auth.json` is removed only after the newly saved encrypted vault is reopened and authenticated. Failed verification preserves the import; a file belonging to a different account is not deleted.
- The encrypted `ux0:data/plus-vita/account.json` and its separate key (or startup password) remain necessary for restoring the saved account.
- Remember Off or Forget disables automatic restoration. If Sony rejects an expired credential, sign in again using the QR flow.

Encryption at rest does not protect a compromised Vita, full filesystem access, runtime memory or the local HTTP sign-in hop. Never share saved account files, key files, NPSSO tokens, browser responses or private backups.

[Token vault documentation](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/TOKEN_VAULT.md)

## Regions and server selection

VitaPlus v0.0.3 uses the authenticated account country and language for regional discovery. The legacy foreign-region hints have been replaced by an opt-in same-account discovery setting. It does not change the account country or IP address, bypass subscriptions, or grant streaming eligibility.

Manual datacenter selection remains available where the service provides a list. Users in unsupported or unknown regions are not guaranteed access. Any permitted reconnect stays with the selected server. **Automatic server switching is not implemented yet.**

Planned automatic failover will be restricted to users with verified eligibility in supported countries; unsupported or unknown regions will remain manual. Authorization, region, subscription and rate-limit refusals must not trigger automatic failover.

See the [regional security review](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/REGIONAL_SECURITY_REVIEW.md) and [roadmap](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/ROADMAP.md).

## Controls

| Action | Default control |
| --- | --- |
| Navigate menus | D-pad, left stick or touch |
| Select / go back | Cross / Circle |
| Cycle catalogue platform filter | Triangle or Filter button |
| Search titles | Search, where available |
| In-game buttons and sticks | Corresponding PlayStation controls |
| L1 / R1 | Vita L / R |
| L2 / R2 | Rear touchpad top-left / top-right |
| L3 / R3 | Rear touchpad bottom-left / bottom-right |
| Options / Share | START / SELECT |
| Controller touchpad movement / click | Front touch / two front touches |
| Stop streaming | Hold **START + SELECT for two seconds** |
| Skip splash | Cross or touch |

Open **Settings and help** to configure gameplay bindings, streaming profiles, the performance HUD, language and account security. PS TV controller support remains experimental; DualShock 4 uses OPTIONS + SHARE for the exit shortcut.

[Control guide (upstream baseline and remapping)](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/CONTROLS.md)

## Languages

English (US/UK), Portuguese (Brazil/Portugal), Spanish (Spain/Mexico), German, French, Italian, Dutch, Polish, Swedish, Turkish, Russian, Japanese, Korean, Simplified Chinese, Traditional Chinese and Arabic.

Translation coverage and font/complex-script rendering may vary on physical hardware.

## Optional network optimization

The optional [Wi-Fi Turbo plugin](https://github.com/juanjsebgarcia/vita-turbo-wifi) may improve responsiveness on compatible systems. Results depend on firmware, router and network congestion. Follow its own installation and compatibility instructions; 40 MHz is not always beneficial. VitaPlus does not require the plugin.

After Wi-Fi loss or sleep, Retry starts a fresh cloud connection; it does not resume the previous remote session.

## Report an issue

Open a ticket in the **[VitaPlus GitHub Issues](https://github.com/joaopedr00000/plus-vita/issues/new/choose)**, not the upstream project, for issues specific to this fork.

Include the VitaPlus version, Vita model, firmware, game/platform, reproduction steps and the exact symptoms or displayed error codes.

**Attach a reviewed session log for priority investigation.** In current v0.0.3 builds:

1. Reproduce the problem.
2. Open VitaShell and locate `ux0:data/VitaPlus-Diagnostic-DD-MM-YYYY-HH-MM.log`.
3. Copy the log **before reopening VitaPlus**, which replaces the previous launch's log. Multiple Play attempts in one launch append to the same bounded log.
4. Review it privately, then attach only the relevant reviewed log to the issue.

Older versions may use different names, including `ux0:data/plus-vita-v0.0.1.log`. If a log is unavailable, describe the symptoms in the ticket.

The app does not automatically upload logs. Never attach tokens, Sony responses, account JSON, encrypted vaults, unlock keys, private screenshots or entire app-data directories.

[Diagnostic log documentation](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/NATIVE_DIAGNOSTICS.md)

## Roadmap

- Automatic server failover for verified supported-region users only, while retaining manual selection elsewhere.
- Additional PS5 Cloud interoperability and hardware testing, without bypassing service restrictions.
- Regression testing of encrypted account restoration, regional policy, PS3/PS4 playback and recovery.
- Continued streaming performance, stability and localization improvements.

[Full roadmap](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/docs/ROADMAP.md)

## Source, credits and license

The fork's active experimental source is on [release/v0.0.3](https://github.com/joaopedr00000/plus-vita/tree/release/v0.0.3). The default branch preserves the upstream baseline except for this project overview. Use the experimental branch when inspecting or building VitaPlus-specific functionality.

- [Calligula87 / Plus-Vita](https://github.com/Calligula87/plus-vita) — original project and native Vita streaming foundation.
- [Pylux / libchiaki](https://github.com/ForWard-Technologies-LLC/Pylux) and Chiaki contributors — cloud and streaming library foundation.
- [tertiumndatur / PS5 Cloud Play](https://github.com/tertiumndatur/ps5-cloud-play) — PS5 cloud interoperability reference.
- [VitaSDK](https://vitasdk.org/) — PlayStation Vita homebrew toolchain.
- All upstream contributors and community testers.

[Build instructions](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/release/README.md) · [CI build workflow](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/.github/workflows/vita-smoke.yml) · [Third-party notices](https://github.com/joaopedr00000/plus-vita/blob/release/v0.0.3/release/THIRD_PARTY_NOTICES.md) · [AGPL-3.0](LICENSE)

Third-party components retain their own licenses.

### Support the original developer

If you appreciate the original Plus-Vita work, consider supporting Calligula87:

[![Buy Me a Coffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://www.buymeacoffee.com/caligolare6)
