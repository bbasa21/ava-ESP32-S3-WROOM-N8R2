#pragma once

// Staging-only key. Generate with:
//   .\tools\Setup-OTA-V3-Key.ps1 -Channel Test -SetGitHubSecret
// This public key is intentionally distinct from the production trust anchor.
#define AVA_OTA_V3_TEST_PUBLIC_KEY_CONFIGURED 0
#define AVA_OTA_V3_TEST_PUBLIC_KEY_PEM R"AVAKEY(
-----BEGIN PUBLIC KEY-----
REPLACE_WITH_YOUR_TEST_P256_PUBLIC_KEY
-----END PUBLIC KEY-----
)AVAKEY"
