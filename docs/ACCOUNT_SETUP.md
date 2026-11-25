# Connect your Sony account (Beta 3)

1. Put the Vita and phone on the same trusted private IPv4 Wi-Fi network.
2. On Vita select Get started, Continue, Sign in with QR code.
3. Scan the unique QR using your phone's camera. A temporary HTTP page served by the Vita opens in the phone browser. No downloaded helper, QR generator or transferred file is necessary.
4. Open the official PlayStation sign-in link. Enter email/password, passkey and verification codes only on Sony's website. Open Sony's token-response link in the same browser, copy the response and paste it on the local page. Choose Connect to Vita.
5. Confirm on Vita. The server closes after a valid submission or cancellation/expiry. Sony validates the token and authorizes the account and streaming separately.

This is AstraPlay-inspired embedded local relay authentication, not Sony's native device-code API or automatic cookie capture. The local HTTP hop is **not encrypted**: anyone able to inspect the LAN may obtain the token. Use a trusted private network, never public Wi-Fi; do not forward the port, publish the QR, paste passwords or share the token. A one-time 128-bit session secret, same-subnet restrictions, bounded requests, Host/Origin checks and 3-minute expiry reduce accidental or unauthorized submissions but do not encrypt the connection. Clear your clipboard. No central relay, telemetry or password collection. Browser local-network access may need permission; HTTPS-only mode may require allowing this local HTTP page. Hardware/browser compatibility is not yet verified.

Remember remains optional. Saved credentials remain locally readable; Forget removes app-managed account data. The Beta 3 build no longer offers manual NPSSO entry, optical pairing, reverse QR or JSON import; legacy files are not automatically imported or deleted. Previously saved app accounts can still restore. Older builds retain their previous behavior.

## Account controls and recovery

- Circle returns to connected home. Open catalogue, About and cancelled Exit
  retain the runtime account without rereading the file. Development builds use
  D-pad/left stick, Cross/Circle or touch. The released v000 uses left-stick/touch
  menus. See the [development control guide](CONTROLS.md).
- Account → Remember Off removes the app-managed saved copy and pauses automatic
  sign-in, . The current runtime account
  remains usable. Remember On resumes persistence; the JSON stays in place.
- Forget clears runtime sign-in, saved account/label and catalogue state, then
  returns to setup.  Choose QR sign-in to connect again; enable Remember if you want automatic sign-in.
- Confirmed rejection/expiry also blocks automatic reuse. Sign in again by QR. No fixed token lifetime
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
