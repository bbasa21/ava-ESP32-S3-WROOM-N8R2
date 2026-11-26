#include <Arduino.h>
#include <unity.h>
#include <stdint.h>
#include <string.h>

#include "Ava_OTA_V3_Manifest.h"

// Exact signed staging fixture from ota/v3-test-manifest.json.
// The final newline is part of the signed byte sequence.
static const uint8_t kValidManifest[] =
    "{\n"
    "  \"schema\": 3,\n"
    "  \"build\": 20007,\n"
    "  \"version\": \"0.0.0\",\n"
    "  \"commit\": \"5b6ca84729e6d4342731c25bfce432551441c921\",\n"
    "  \"firmware_url\": \"https://github.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/releases/download/ota-v3-test-20007/ava-firmware.bin\",\n"
    "  \"size\": 1474304,\n"
    "  \"sha256\": \"a9a6c4f2e75ff5d7032a3589f6be8fe8f85d5a35038323b16aa209af8331a148\"\n"
    "}\n";

static const char kValidSignature[] =
    "MEYCIQD1ro5ft1eTg8wTk3JYPRyPap17O/hs+UojpYbfwX0zPQIhAP/Q8FdJAqTs3SZR8e0c2QE+GCVZsSDpIvNCWu3RTEoy";

static void test_valid_signed_manifest_is_accepted() {
    AvaOTAV3Manifest parsed = {};
    AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE;

    const bool accepted = avaOTAV3VerifyAndParseManifest(
        kValidManifest, sizeof(kValidManifest) - 1,
        kValidSignature, parsed, error
    );

    TEST_ASSERT_TRUE(accepted);
    TEST_ASSERT_EQUAL(AVA_OTA_V3_ERR_NONE, error);
    TEST_ASSERT_EQUAL_UINT32(3, parsed.schema);
    TEST_ASSERT_EQUAL_UINT32(20007, parsed.build);
    TEST_ASSERT_EQUAL_UINT32(1474304, parsed.size);
    TEST_ASSERT_EQUAL_STRING("0.0.0", parsed.version);
    TEST_ASSERT_EQUAL_STRING("5b6ca84729e6d4342731c25bfce432551441c921", parsed.commit);
    TEST_ASSERT_EQUAL_STRING(
        "a9a6c4f2e75ff5d7032a3589f6be8fe8f85d5a35038323b16aa209af8331a148",
        parsed.sha256
    );
}

static void test_modified_manifest_is_rejected() {
    uint8_t modified[sizeof(kValidManifest)];
    memcpy(modified, kValidManifest, sizeof(kValidManifest));
    // Change one signed byte without changing the detached signature.
    modified[modified[0] == '{' ? 25 : 0] ^= 1U;

    AvaOTAV3Manifest parsed = {};
    AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE;
    const bool accepted = avaOTAV3VerifyAndParseManifest(
        modified, sizeof(kValidManifest) - 1,
        kValidSignature, parsed, error
    );

    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_EQUAL(AVA_OTA_V3_ERR_SIGNATURE_INVALID, error);
}

static void test_invalid_signature_is_rejected() {
    AvaOTAV3Manifest parsed = {};
    AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE;
    const bool accepted = avaOTAV3VerifyAndParseManifest(
        kValidManifest, sizeof(kValidManifest) - 1,
        "not-a-valid-signature", parsed, error
    );

    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_EQUAL(AVA_OTA_V3_ERR_SIGNATURE_INVALID, error);
}

static void test_null_manifest_is_rejected() {
    AvaOTAV3Manifest parsed = {};
    AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE;
    const bool accepted = avaOTAV3VerifyAndParseManifest(
        nullptr, 0, kValidSignature, parsed, error
    );

    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_EQUAL(AVA_OTA_V3_ERR_MANIFEST_SCHEMA, error);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_valid_signed_manifest_is_accepted);
    RUN_TEST(test_modified_manifest_is_rejected);
    RUN_TEST(test_invalid_signature_is_rejected);
    RUN_TEST(test_null_manifest_is_rejected);
    UNITY_END();
}

void loop() {
    delay(1000);
}
