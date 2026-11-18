# AVA OTA V3 — hardware validation plan

**Status:** Not yet hardware-tested. Do not merge to `main`, publish a production manifest, or run this against a production-only device until every required test below passes.

## Preconditions

- [ ] Build from the exact `feature/ota-v3` commit intended for test; record the commit SHA.
- [ ] Confirm the generated firmware is smaller than the inactive app partition (`0x330000` bytes).
- [ ] Confirm the selected ESP32-S3 SDK configuration has `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=1`.
- [ ] Generate the permanent P-256 keypair; embed only the public key in firmware.
- [ ] Store the private key only in GitHub Actions secret `AVA_OTA_SIGNING_PRIVATE_KEY`.
- [ ] Provision the key and use the isolated `ava_ota_v3_test` environment.
- [ ] Publish a signed staging manifest and matching `ota-v3-test-*` release on `feature/ota-v3`. Never point test firmware at the production manifest.
- [ ] Keep a known-good firmware build and USB recovery path available.
- [ ] Use stable power and do not disconnect power during flash writes.

## User interface / command checks

| Test | Action | Expected result |
|---|---|---|
| Idle | Boot AVA | Normal OLED behavior; serial reports OTA V3 worker ready |
| Status | Send `ota status` on COM3 | Current V3 state, error, build and progress printed |
| Check | Send `ota check` | OLED shows check state, then up-to-date / update available / failure |
| Duplicate check | Send `ota check` repeatedly during a check | Additional request rejected; one worker owns the transaction |
| Install without check | Send `ota install` before a verified update is available | Request rejected; no flash write or reboot |
| Download UI | Start a valid test install | OLED shows V3 header, build transition, progress bar and byte progress |
| Terminal UI | Cause a controlled check failure | Error state remains visible briefly, then normal OLED ownership resumes |

## Security and failure-injection tests

Perform these using signed test fixtures and a non-production release:

| Test | Injection | Pass condition |
|---|---|---|
| Wrong signature | Replace `manifest.sig` with a signature from another key | Signature rejected; no install starts |
| Modified manifest | Change one byte after signing | Signature rejected before trusting any field |
| Unsupported schema | Sign a manifest with an unsupported schema | Manifest rejected |
| Unapproved firmware URL | Sign a manifest pointing outside the exact repository release URL | URL rejected |
| Wrong declared size | Sign a manifest with a size different from HTTP Content-Length | Download rejected; running slot unchanged |
| Wrong SHA-256 | Sign a manifest with a wrong image hash | Image rejected; running slot unchanged |
| Truncated response | Interrupt the firmware body | OTA write aborted; running slot remains selected |
| Wi-Fi loss | Disable Wi-Fi during transfer | Download fails safely; no boot-slot switch |
| Network contention | Hold the shared network lock from another service | OTA check/install fails or defers cleanly; no watchdog reset |
| Duplicate install | Send install repeatedly during a transfer | Only one installer transaction runs |

For every failure test, verify that AVA does not enter `REBOOT_PENDING`, does not call `esp_ota_set_boot_partition`, and continues booting the previous valid image.

## Successful update and rollback tests

1. Send `ota check`; verify the signed build number and size.
2. Send `ota install`; watch the OLED progress screen and serial state transitions.
3. Confirm the installer verifies the streamed SHA-256, validates the ESP image, reads the inactive slot back, and only then selects it.
4. After restart, verify the running image is `ESP_OTA_IMG_PENDING_VERIFY` and OLED shows the first-boot health screen.
5. Keep the device running beyond the 60-second health window. Confirm the OLED/adapter and free-heap conditions pass and `esp_ota_mark_app_valid_cancel_rollback()` succeeds.
6. Verify the new build boots again after a normal power cycle and is no longer pending verification.
7. On a dedicated test-only firmware, deliberately fail the boot-health condition. Confirm the 180-second failsafe marks the image invalid and the bootloader returns to the previous OTA slot.
8. After both successful and rollback runs, verify saved Wi-Fi credentials, AI settings, and other NVS data remain intact.

## Release gate

All checks must pass on the physical ESP32-S3 board. A green GitHub Actions compile is necessary but does not prove TLS redirects, flash behavior, first-boot validation, or hardware rollback. Keep the V2 production implementation on `main` until the complete hardware matrix passes.
