#pragma once
#include "Ava_OTA_V3.h"
#include <stddef.h>
#include <stdint.h>

struct AvaOTAV3Manifest {
    uint32_t schema;
    uint32_t build;
    uint32_t size;
    char version[33];
    char commit[41];
    char firmwareUrl[256];
    char sha256[65];
};

// Verify the detached ECDSA P-256 signature over the exact byte sequence
// before parsing or trusting any manifest field.
bool avaOTAV3VerifyAndParseManifest(
    const uint8_t* manifestBytes,
    size_t manifestLength,
    const char* signatureBase64,
    AvaOTAV3Manifest& output,
    AvaOTAV3Error& error
);
