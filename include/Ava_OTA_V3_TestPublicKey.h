#pragma once

// Staging OTA trust anchor intentionally matches the existing production key.
// Both staging and production manifests are signed by AVA_OTA_SIGNING_PRIVATE_KEY.
#define AVA_OTA_V3_TEST_PUBLIC_KEY_CONFIGURED 1
#define AVA_OTA_V3_TEST_PUBLIC_KEY_PEM R"AVAKEY(
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEzWv2GXT9Tzqio+C4h32h6CNWRGSP
8q3XBEO7ep1aH2nBsOfpAr4aXo2wCSDkyejOMb6wBqXbfsDlJfzPKPUrYA==
-----END PUBLIC KEY-----
)AVAKEY"
