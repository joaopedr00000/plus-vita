<p align="center">
  <img src="vita-client/assets/branding/logo-source.png" alt="VitaPlus logo" width="200">
</p>

<h1 align="center">VitaPlus</h1>

[**VitaPlus v0.0.2 — Visual Update**](https://github.com/joaopedr00000/plus-vita/releases/tag/v0.0.2)

An experimental fork of Plus-Vita. The visual update does not introduce new streaming functionality.

## Beta 1 — PS5 Cloud experimental

[Release notes (English / Português)](release/v0.1.0-beta.1.md) · [Build instructions](release/BETA1_BUILD.md)

## Experimental fork: PS5 Cloud and manual servers

This branch adds an experimental PS5 Cloud route using the existing Pylux provisioning library, informed by [PS5 Cloud Play](https://github.com/tertiumndatur/ps5-cloud-play). PS3/PS4 remain on PS Now. The Vita still requires H.264; PS5 Cloud ordinarily uses HEVC, so AVC negotiation may be refused or produce no compatible stream. No successful PS5 stream or GTA VI test is claimed.

Select a title and Play to discover the service-provided datacenter list. Select a server manually; no automatic server fallback is used. The bitrate control cycles 4, 8, 12 and 15 Mbps before connecting. The selected server alone is probed for actual RTT and MTU; failed probes stop the attempt instead of substituting another server. The video profile stays at 720p (the original decoder validation limit). The existing audited H.264 parameter-set guard also remains enabled, so an unfamiliar PS5 bitstream may be rejected even if AVC is offered. Server availability and title entitlement are checked by Sony. The app does not bypass account, subscription or regional requirements.

Build with VitaSDK and initialized recursive submodules as in the beta build instructions. The experimental features are enabled explicitly for Beta 1; ordinary builds default to the original path. This branch is not a validated VPK release. Keep your account JSON and logs private; do not publish test credentials.

Credits: original Plus-Vita by Calligula87, Pylux/libchiaki, and tertiumndatur's PS5 Cloud Play for reference. Existing licenses remain in force.

## Introduction

Plus-Vita lets you play eligible **PlayStation Plus cloud games directly on your
PlayStation Vita**. You need your own Sony account and
**a PlayStation Plus subscription tier that includes cloud streaming in your
region**. 

[**Download Plus-Vita v002**](https://github.com/Calligula87/plus-vita/releases/tag/v002)

This is an unofficial homebrew project, not affiliated with Sony Interactive
Entertainment. No PC relay is needed during play; initial sign-in uses a phone
or PC browser.

## Requirements

- A homebrew-enabled PS Vita with **VitaShell** and working Wi-Fi.
- Your own Sony account with a subscription that supports PlayStation Plus cloud
  streaming in your region.
- A phone or PC browser to create your sign-in JSON, plus USB or FTP file transfer.
- Your private **plus-vita-auth.json** file at **ux0:data/plus-vita-auth.json**,
  created using the setup below.

## Setup

1. Download **plus-vita-v002.vpk**, copy it to your Vita and install it using
   VitaShell. Existing Plus-Vita installations are updated using the same app ID.
2. On your phone or PC, sign in to your own account at
   [PlayStation](https://www.playstation.com/). Complete Sony's verification or
   passkey prompts there.
3. In the **same browser and profile**, open the
   [Sony sign-in response](https://ca.account.sony.com/api/v1/ssocookie).
   It should display JSON containing an `npsso` field. If it does not, sign in
   again and retry.
4. Save that response itself as **plus-vita-auth.json**, using the browser's
   save/download action, usually **Ctrl+S** on a PC. Save JSON, not an HTML page,
   screenshot or a file ending in `.json.txt`. You do not need to extract or type
   the token.
5. Transfer the file through VitaShell to **ux0:data/plus-vita-auth.json**.
6. Launch **Plus-Vita**. The app validates your saved sign-in and opens the
   catalogue. If the app is already open, choose **Get started → Continue →
   Import JSON**. Select a game, open its details and choose **Play**.

**Keep the JSON private: it grants account access. Never upload it to GitHub or
share its contents.** It remains on your Vita. Remember defaults On; Remember Off
and Forget pause automatic reuse until you explicitly reconnect. Account files
are readable by someone with Vita file access. If Sony rejects an expired
credential, save a fresh response and explicitly import it again.

See the [full setup and account-recovery guide](docs/ACCOUNT_SETUP.md) for help.
After Wi-Fi loss or sleep, Retry starts a fresh cloud connection; it does not
resume the previous session.

For a possible small improvement in streaming responsiveness, you can try the
optional [Wi-Fi Turbo plugin](https://github.com/juanjsebgarcia/vita-turbo-wifi).
A slight improvement was reported with v0.4.1 on firmware 3.65 using **Max
performance** and **40 MHz**. Results depend on your network; 40 MHz needs router
support and may perform worse in congested areas. Follow the plugin's installation
and firmware compatibility guidance. Wi-Fi Turbo is not required to use Plus-Vita.

## Report an issue

If you encounter a problem, [submit a GitHub ticket](https://github.com/Calligula87/plus-vita/issues/new/choose).
Include the app version, the steps to reproduce it, what you expected and what happened.

**A session log is needed for priority investigation. Reports without a log will
not be treated as priority.** To provide one:

1. Reproduce the issue, then use VitaShell to copy
   **ux0:data/VitaPlus.log** to your phone or PC.
2. Copy it **before another app launch**, which replaces the log.
3. Review it privately and attach **only the reviewed log** to your ticket.

Never attach `plus-vita-auth.json`, saved account files, tokens, Sony browser
responses, private screenshots or an entire app-data folder. The app does not
upload logs automatically. If no log is available, explain that in the ticket
and describe the symptoms; the report can still be submitted without priority.
See the [reporting guide](docs/REPORTING_ISSUES.md).

## Controls

These are the default Vita controls. See the [control guide](docs/CONTROLS.md)
for saved gameplay mappings and experimental PS TV DualShock 3/4 input.

| Action | Control |
| --- | --- |
| Move through menus | D-pad, left stick, or touch |
| Select / go back | Cross / Circle |
| Search catalogue | Triangle or Search button |
| In-game buttons and sticks | Corresponding PlayStation controls |
| L2 / R2 | Rear touchpad top-left / top-right |
| L3 / R3 | Rear touchpad bottom-left / bottom-right |
| Move / press the controller touchpad | Front touch / two front touches |
| Stop streaming and return | Hold **START + SELECT for two seconds** |
| Skip opening splash | Cross or touch |

**Settings and help** contains gameplay control mapping, account controls,
the app version, and log status. The interface is English, supports one
account, and uses a fixed 720p stream profile.

---

Built on [Pylux/libchiaki](https://github.com/ForWard-Technologies-LLC/Pylux) and
[VitaSDK](https://vitasdk.org/). See [source/build instructions](release/README.md),
[licenses and credits](release/THIRD_PARTY_NOTICES.md), and the
[v000 identity and validation record](docs/V000.md). Source is provided under the
[AGPL-3.0](LICENSE); third-party components retain their own license terms.



if you appreciate the work

[![Buy Me a Coffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://www.buymeacoffee.com/caligolare6)
