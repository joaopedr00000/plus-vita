# VitaPlus — PS5 Cloud Server Fix (Experimental 1)

Experimental test release based on the validated VitaPlus v0.0.2 build (commit faf5ac6). This is a possible fix for PS5 Cloud allocation and missing datacenter lists, not a confirmed PS5 playback solution.

- Improved handling of server-issued session identifiers and datacenter response formats.
- Recovery to manual datacenter selection when an attempted server fails.
- More precise connection and HTTP error diagnostics without logging account credentials.
- Includes updated About, controls, exit translations and three streaming profiles.

The existing PS5 client uses the PlayStation Portal protocol profile (qlite). The PS3/PS4 playback paths and H.264 video support are retained. No new device spoof or fake HEVC support is included. PS5 playback has not been validated on a physical Vita using Sony's live service.

The VPK retains the existing application ID and internal v0.0.2 version and installs over the current app. The stable v0.0.2 release remains unchanged.

If allocation fails, report the failing stage and sanitized error codes. Never share NPSSO, account JSON, passwords or other credentials.

[Successful build and automated tests](https://github.com/joaopedr00000/plus-vita/actions/runs/45635069629)
