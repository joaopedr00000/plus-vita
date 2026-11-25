# Regional security review — VitaPlus v0.0.3

Scope: client regional discovery and selection policy; not a full Pylux, Sony or kernel audit.

- Prior risk: authenticated discovery could supply user-selected US/CA/PT locale despite a different authenticated account country. v0.0.3 removes that override and validates the actual account country and language.
- A legacy HTTP 404 does not grant entitlement. Same-account fallback is opt-in after successful authorization/session/transport only; no fallback on 401, 403, 429 or other errors. All streaming remains subject to server authorization.
- Account region does not prove physical presence. Future failover must default to manual if actual regional eligibility is not established. No automatic cross-region probing.
- Remaining risk: unofficial client, protocol/capability mismatch, regional restrictions and unusual requests may be rejected. No guarantee against restrictions or suspension; never falsify account/device identity or declared capabilities. Local HTTP login is unencrypted; use trusted LAN only.
- Validate Remember after reboot, cleanup, region, authorization denial, manual datacenter selection and PS3/PS4 regression on physical hardware. Never collect tokens, IPs or raw service replies in diagnostics.

Official documentation requires a supported account and location plus an eligible subscription, and accurate account information:
https://www.playstation.com/en-cz/ps5-game-cloud-streaming/
https://www.playstation.com/en-us/legal/terms-of-service/
