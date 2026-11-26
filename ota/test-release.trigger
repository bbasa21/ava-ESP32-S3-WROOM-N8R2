AVA OTA V2 staging release trigger

Source branch: work/ota-v3-hardening
Build environment: ava_ota_v2_test

Purpose:
Build and publish an unsigned OTA V2 staging release.

The release must include:
- ava-firmware.bin
- v2-test-manifest.json
- SHA256 integrity information

OTA V2 uses HTTPS certificate validation and SHA256 integrity verification.
No ECDSA signature or private signing key is required.

This trigger is for staging tests only. Production main branch must remain unchanged.
