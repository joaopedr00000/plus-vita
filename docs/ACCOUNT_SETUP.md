# Connect your own Sony account

## Beta 3: sign in on Sony without transferring files

Choose **Sign in on Sony (email / passkey)** to open the real PlayStation website on your phone with a unique QR. Complete all sign-in prompts on Sony's website. The second QR opens Sony's own token-response endpoint in the same browser. Choose Cross and type only the 64-character NPSSO in the masked Vita keyboard. If already signed in, choose **QR login / enter token (no files)** to skip the first page. No HTML download, VitaShell transfer, password or verification code is required by the Vita app in this mode. The token is validated by the existing account workflow and the temporary UI buffers are cleared. Turn Remember off if you do not want the app-managed locally readable token saved. A fresh QR navigation nonce is generated on each page; it does not grant authorization or change Sony's regional policy. This flow still requires manual token entry and is not official Sony device-code authorization.

Never enter your Sony email or password in a third-party login form. The Plus-Vita screens are not Sony screens. Only enter them on the actual official Sony site.

## Beta 2: experimental encrypted optical QR pairing

1. Download the [offline pairing helper](../release/qr-pairing.html) as a local HTML file on your phone or PC. Open it in a browser supporting local Web Crypto. It makes no network requests itself.
2. On Vita choose Get started → Continue → Pair with QR code. Sign in at playstation.com on your phone and open the official Sony response link in the downloaded helper. Copy the response privately. The static Sony link is no longer displayed as a shared QR.
3. Press Cross to generate the temporary key QR. Copy the key or type its 64 hexadecimal characters (both lines together) into the offline helper. Paste the Sony response and generate an encrypted QR. The helper uses AES-256-GCM, an authenticated 256-bit session identifier and a fresh nonce, and clears the fields after generation.
4. Press Cross on Vita and aim its rear camera at the generated QR. Only an authenticated response for this session is accepted. A new key mixes 256 bits of kernel-generated cryptographic entropy with a local hardware network discriminator; the raw MAC address is never displayed or transmitted. No static pairing QR or predictable fallback is used. A different device/session is rejected even if it uses the same Sony account. Absolute mathematical collision-free operation cannot be guaranteed offline; the cryptographic collision risk is negligible. The key expires after three minutes. The result uses the existing sign-in validator and does not write the original JSON import file.
5. Keep both screens private, clear the clipboard, and never screenshot or share either code. Remember still governs locally readable saved credentials. This is not Sony's official device-code login.

Camera operation and browser compatibility need hardware testing. If either fails, use the no-file token keyboard flow or Import JSON (including PS TV). Never paste a credential into an online QR generator.

## Save your sign-in JSON on a phone or PC

1. Sign in to your own account at [PlayStation](https://www.playstation.com/).
   Complete Sony's password, passkey or verification prompts on that device.
2. In the same browser/profile, open the
   [Sony sign-in response](https://ca.account.sony.com/api/v1/ssocookie).
   It should show JSON containing an `npsso` field. If not, sign in again and retry.
3. Save the response itself as **plus-vita-auth.json**, using the browser's
   save/download action (usually Ctrl+S on a PC). Save JSON, not an HTML page or
   screenshot. You do not need to extract or type the token or edit the response.
4. Transfer it with VitaShell to **ux0:data/plus-vita-auth.json**.

Keep this file private: it grants account access. Never post it, the browser
response or screenshots of its contents to GitHub. Plus Vita does not ask for
your Sony password, passkey or verification code; Sony handles sign-in.

## Open the app

The app automatically checks for an existing saved account, then uses your
JSON if no saved account exists. It validates the credential before opening
the catalogue. **The original JSON stays in place**, unchanged, including after
manual import, cancellation or a malformed response. Successive launches can
read it again while Sony accepts the credential.

If the app is already open, choose Get started → Continue → Import JSON.
Remember defaults On for fresh setup: placing the JSON in the folder enables
automatic sign-in. It accepts UTF-8 JSON up to 4096 bytes, including a byte-order
mark; unrelated response fields are ignored and invalid/ambiguous credentials
are rejected without showing their contents.

Account data is locally readable with file access. The original browser response
remains your file. The app may separately save only the needed NPSSO and optional
user-entered label; it does not claim an encrypted credential vault. The label
is not a Sony-verified identity.

Account headings use the authenticated Sony `onlineId` when available. A local
label entered in Account overrides it and is shown as a user-entered label.
Before authorization completes, a saved local label may appear; otherwise the
heading stays generic. The Sony name is runtime-only and is not saved alongside
the credential or written to diagnostics.

## Account controls and recovery

- Circle returns to connected home. Open catalogue, About and cancelled Exit
  retain the runtime account without rereading the file. Development builds use
  D-pad/left stick, Cross/Circle or touch. The released v000 uses left-stick/touch
  menus. See the [development control guide](CONTROLS.md).
- Account → Remember Off removes the app-managed saved copy and pauses automatic
  sign-in, including reuse of the retained JSON. The current runtime account
  remains usable. Remember On resumes persistence; the JSON stays in place.
- Forget clears runtime sign-in, saved account/label and catalogue state, then
  returns to setup. It keeps your JSON but blocks automatic reuse. Choose Import
  JSON explicitly to connect again; enable Remember if you want automatic sign-in.
- Confirmed rejection/expiry also blocks automatic reuse. Replace the JSON with
  a fresh browser response, then explicitly import it. No fixed token lifetime
  or automatic renewal is promised. Wi-Fi, rate-limit and service failures retain
  the credential and offer Retry without requiring another transfer.
- Missing, unreadable or malformed files produce fixed readable errors. Replace
  malformed files using VitaShell; the app does not delete them. Storage write
  and cleanup failures offer recovery and do not claim success.
- Cancelled or late account results cannot reconnect a forgotten account.
  Network cancellation is cooperative; allow an existing worker to finish before
  retrying if it is still busy.

The catalogue shows alphabetical cover tiles. Browse with the left
stick (left/right or up/down), open details with Cross or touch, and use Triangle
or the Search button to enter a title using the Vita keyboard. Search ignores
ASCII case and accepts supported non-ASCII text. Circle returns without losing
the current search and position. Missing or failed covers use placeholders.
A listed game does not guarantee streaming eligibility. Open a title's details
and choose Play to connect. Hold START+SELECT for two seconds to stop streaming
and return to the catalogue. After Wi-Fi loss or sleep, retry starts a fresh
connection rather than resuming the previous cloud session.
Report steps and symptoms using the [issue-reporting guide](REPORTING_ISSUES.md), never
credential files or a full app-data folder.
