# AVA OTA v2 — Architecture and Safety Plan

## Scope

This document defines a new OTA engine for AVA. Work is confined to `feature/petting-sleepy-eyes`.

**Safety boundary:** do not change `main`, the public `ota/manifest.json`, published firmware releases, or the release workflow as part of OTA v2 development. Do not flash hardware automatically.

The current OTA implementation remains the reference until v2 is implemented, builds successfully, and is explicitly selected for device testing.

## Design goals

1. A failed download must never activate an incomplete image.
2. The image must fit the actual inactive OTA partition.
3. The downloaded byte count and SHA-256 must match the manifest before activation.
4. Network interruption, HTTP errors, timeout, flash write errors, malformed metadata, and hash mismatch must produce a clear failure and leave the running image selected.
5. The updater must not compete with other TLS/network work; use the existing AVA network lock with a bounded wait.
6. A check-only operation must never write flash or change the boot partition.
7. The engine must have a testable boundary between manifest parsing, transport, image validation, and activation.
8. The public production manifest must not be used to test experimental firmware. The v2 manifest URL must be explicitly configured for a later test, and production publishing remains out of scope.

## Proposed state machine

`IDLE → WAITING_FOR_NETWORK → FETCHING_MANIFEST → VALIDATING_MANIFEST → DOWNLOADING_IMAGE → VERIFYING_IMAGE → FINALIZING_IMAGE → SELECTING_BOOT_PARTITION → REBOOT_PENDING`

Any failure transitions to `FAILED`, closes HTTP/TLS resources, releases the network lock, and aborts an in-progress flash update where possible. No failure path selects the candidate partition.

## Manifest contract

The first implementation will retain the small existing manifest contract for compatibility:

- `build`: unsigned integer, strictly greater than the running build to install.
- `version`: non-empty string for diagnostics.
- `commit`: commit identifier string for diagnostics.
- `firmware_url`: HTTPS URL only; reject empty, malformed, or non-HTTPS URLs.
- `sha256`: exactly 64 hexadecimal characters.

Parsing must be structural rather than substring-based. Enforce a small maximum manifest size and reject missing fields, wrong types, trailing/invalid JSON, and out-of-range build values. Never print secrets or full response bodies into Serial.

## Download and flash invariants

- Require HTTP 200 and a positive, bounded content length.
- Reject an image larger than the inactive OTA partition before starting the write.
- Stream in fixed-size chunks; handle short reads, disconnection, and no-progress timeout.
- Hash exactly the bytes successfully received and written.
- Require bytes written == declared content length and the final SHA-256 == manifest SHA-256.
- Require the framework's update finalization checks to succeed.
- Only after every validation succeeds may the candidate boot partition be selected.
- Verify the selected boot partition before requesting restart.
- Keep the current running partition selected on every failure.

## Transport security

The current code uses `WiFiClientSecure::setInsecure()`, which encrypts traffic but does not authenticate the server certificate. This is not sufficient as the final security posture for an updater. Before enabling v2 for production, choose and test a maintainable certificate-validation strategy (trusted root certificate or ESP-IDF certificate bundle) and define how certificate updates will be handled. Do not silently fall back to insecure TLS if verification fails.

## Integration and rollout

1. Add v2 as an isolated engine and keep the current entry points unchanged.
2. Add compile-time/build checks and tests for manifest validation and state transitions where feasible.
3. Build the feature branch in CI and review its logs.
4. Add an explicit, opt-in test configuration that cannot accidentally point at the public production manifest.
5. Review partition layout and the exact ESP32 Arduino core APIs before device testing.
6. Only after build and review, ask Ali before integrating v2 into the normal boot flow or flashing AVA.

## Explicitly deferred

- Automatic rollback / first-boot health confirmation.
- Publishing releases or changing the public manifest.
- Automatic flashing of physical AVA hardware.
- Switching the production boot flow to v2 before explicit approval.
