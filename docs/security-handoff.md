# Security Handoff

This note captures the current Web UI security posture and the next controls to
evaluate before using the lamp outside a trusted local setup.

## Current State

- Web UI routes are protected with HTTP Basic authentication through `WebAuth`.
- Multiple Web UI users can be stored and managed from the System page.
- Wi-Fi credentials, cloud credentials, and WebAuth users are persisted on the
  device filesystem.
- Settings changes now use authenticated `POST /settings` form submissions.
- OTA firmware upload is authenticated with the same WebAuth mechanism.
- Reset-to-default and reboot actions are authenticated Web UI actions.

## Risks To Review

- Password storage: WebAuth password material is persisted on-device. Confirm
  whether stored values are hashed, salted, and non-reversible before production
  use.
- CSRF: Authenticated POST endpoints do not currently require a CSRF token. A
  browser that already has Basic Auth credentials cached could be triggered by a
  malicious page on the same network.
- OTA protection: Firmware upload uses WebAuth, but it does not add signed
  firmware verification. A compromised WebAuth credential can replace firmware.
- AP password: Confirm the default access-point password policy, minimum length,
  and whether an unset password creates an open AP.
- Reset behavior: Reset removes credentials and settings. Confirm whether all
  sensitive files, including legacy files, are erased.
- Transport: Local HTTP has no TLS. Credentials are visible to any actor that can
  observe the local network or AP traffic.

## Recommended Next Controls

- Store WebAuth credentials as salted password hashes and never print secrets to
  serial logs.
- Add a per-session CSRF token for state-changing Web UI POST routes.
- Require signed firmware images or a release checksum before OTA accepts an
  upload.
- Enforce a non-empty WPA2 AP password with at least 8 characters.
- Document and test reset coverage for config, WebAuth users, Wi-Fi credentials,
  cloud tokens, and both legacy and current Lenta settings files.
- Consider disabling OTA and reset routes in normal operation, or gate them
  behind a physical-button confirmation window.
