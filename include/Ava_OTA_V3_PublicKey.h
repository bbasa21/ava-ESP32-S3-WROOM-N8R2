#pragma once

// Generate the permanent P-256 keypair with tools/Setup-OTA-V3-Key.ps1.
// Only the public key belongs in firmware. The private key must remain in
// the GitHub Actions secret AVA_OTA_SIGNING_PRIVATE_KEY.
#define AVA_OTA_V3_PUBLIC_KEY_CONFIGURED 0
#define AVA_OTA_V3_PUBLIC_KEY_PEM R"AVAKEY(
-----BEGIN PUBLIC KEY-----
REPLACE_WITH_YOUR_P256_PUBLIC_KEY
-----END PUBLIC KEY-----
)AVAKEY"
