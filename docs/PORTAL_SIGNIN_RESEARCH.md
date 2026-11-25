# Portal-style sign-in research
This experimental branch contains a transport-independent device authorization scheduler inspired by RFC 8628. It is not wired to production login, and it does not claim to support Sony's actual device authorization service.

Public references:
- https://www.playstation.com/en-us/support/hardware/psportal/system-software-info/
- https://www.rfc-editor.org/rfc/rfc8628

The official Portal supports mobile QR sign-in. A usable authorized third-party integration still needs verified provider documentation for device authorization and token exchange, registered client permissions, token audience and scopes accepted by the cloud catalogue/allocation services, verification URL validation, and renewal/revocation semantics. Existing NPSSO-based code cannot assume any returned access token is an NPSSO cookie. Do not convert or rename tokens to make them appear compatible.

The scheduler enforces single in-flight poll, expiration, minimum server-provided intervals (default 5 seconds), slow_down (+5 seconds), exponential timeout backoff, terminal cancellation/denial and late-reply rejection. The host test uses synthetic states only. No live credentials, session identifiers, private client secrets, QR captures or raw traffic are needed to develop this layer. No fake Sony endpoints or credential collection are included.

Only enable a user-facing mode after a documented/authorized provider and compatible authentication path are available and tested. Retain the existing Beta 3 login alternatives unchanged. A physical Portal is useful for observing the user-facing flow, not sufficient to prove an API contract. Never upload active sign-in QR codes, cookies, passwords, tokens or unredacted network captures.
