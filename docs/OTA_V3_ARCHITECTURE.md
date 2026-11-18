# AVA OTA V3 Architecture

Status: design baseline; implementation has not started.

## Goals

- Replace the V2 collection of booleans and loosely coordinated tasks with one OTA owner and an explicit state machine.
- Keep the robot's main loop responsible for UI rendering and normal behavior.
- Keep HTTPS/TLS, manifest validation, firmware streaming, flash writes, and boot selection behind narrow interfaces.
- Preserve the currently running application until a complete image has passed all checks.
- Make every failure observable and leave the currently selected application bootable.
- Authenticate update metadata cryptographically; a SHA-256 value delivered by an unsigned manifest is not an authenticity guarantee.

## Proposed modules

- `Ava_OTA_Manager`: public API, command queue, single owner, state machine, status snapshot.
- `Ava_OTA_Manifest`: bounded manifest parser, schema/version/URL checks, signature verification.
- `Ava_OTA_Transport`: HTTPS client, trusted CA bundle, host/repository allowlist, bounded timeouts, shared network-lock integration.
- `Ava_OTA_Installer`: inactive-partition streaming, bounded buffers, progress reporting, SHA-256 calculation, abort/cleanup.
- `Ava_OTA_BootGuard`: pending-verify detection, first-boot health checks, mark-valid or rollback policy.
- `Ava_OTA_Display`: presentation only; reads a status snapshot and never owns OTA state.

These are logical boundaries. They do not imply one FreeRTOS task per module.

## Single-owner execution model

- Exactly one OTA worker may own an update transaction.
- Commands from Serial, BLE, or future app interfaces enqueue requests; they never perform network or flash operations inline.
- One worker performs the transaction sequentially. It is not pinned to Core 0. Its priority, stack, yielding, and watchdog behavior will be validated against the actual Arduino/ESP-IDF task configuration before hardware use.
- The main loop remains responsive and renders the OTA UI from a copied status snapshot.
- The network mutex is acquired with a bounded timeout and released on every exit path. No other OTA task may race the same HTTP client or OTA partition.
- Automatic checks are low-priority requests submitted to the same manager. They cannot start a second worker or overlap an install.

## State machine

`IDLE -> CHECKING -> UPDATE_AVAILABLE | UP_TO_DATE | FAILED`

`UPDATE_AVAILABLE -> DOWNLOADING -> VERIFYING_IMAGE -> READY_TO_BOOT -> REBOOT_PENDING`

Any transaction state may transition to `FAILED` or `CANCELLED` with a stable error code. An install request is accepted only from `UPDATE_AVAILABLE`. Duplicate requests are rejected deterministically.

## Trust and integrity

- Continue using HTTPS with the ESP32 CA bundle and strict redirect handling.
- Validate the manifest size, schema, build number, version, commit, exact release URL, and firmware size before beginning installation.
- Add a detached ECDSA P-256/SHA-256 signature over the exact manifest bytes. The verification public key is embedded in firmware; the private signing key exists only as a protected GitHub Actions secret and is never committed.
- Verify the manifest signature before trusting its firmware URL or SHA-256.
- Stream to the inactive OTA app partition while calculating SHA-256. On mismatch, abort and do not select the new partition.
- Validate the resulting ESP image and ensure the final image size is within the inactive partition.
- Do not treat TLS or an unsigned SHA-256 field alone as firmware authenticity.

## Boot safety and rollback

- The current application partition must remain untouched during download and installation.
- Select the new partition only after manifest authentication, full-image hash verification, and ESP image validation succeed.
- Automatic boot rollback is a release gate, not an assumption. First verify that the actual bootloader is built with ESP-IDF rollback support and that the partition table has two OTA app slots plus otadata.
- If supported, the new image boots in pending-verify state and calls the mark-valid API only after a defined health check. If it fails or reboots before validation, the bootloader must revert to the previous slot.
- If the existing Arduino bootloader cannot provide this behavior, do not claim automatic rollback. Decide on a tested custom-bootloader path before enabling production OTA.

## Release pipeline

- Build metadata must be generated once and embedded in the exact firmware artifact.
- CI validates the binary, computes its SHA-256, signs the manifest, and publishes the firmware, manifest, and detached signature together.
- Production manifest publication occurs only after all release artifacts have been produced successfully.
- CI must test manifest schema, URL policy, signature verification, image size, and partition compatibility.
- Branch builds are build-only and must never publish the production manifest.

## Implementation gates

1. Capture the actual local partition CSV, bootloader configuration, and installed image metadata.
2. Finalize the state machine, status/error contract, and signature format.
3. Implement and compile the modules on a feature branch; do not modify or replace the production OTA path yet.
4. Add CI tests and inspect the generated release artifact and signed manifest.
5. Perform controlled hardware tests: check only, successful install, wrong signature, wrong hash, truncated download, timeout, duplicate command, network-lock contention, and reboot/boot validation.
6. Merge and publish only after the hardware test matrix passes.

## Non-goals for the first V3 milestone

- No changes to AVA AI, gaze, OLED rendering outside the OTA display adapter, BLE protocol beyond routing OTA commands, or motor behavior.
- No partition-table or bootloader replacement until the rollback gate is verified.
- No production release from an experimental branch.
