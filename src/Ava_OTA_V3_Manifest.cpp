#include "Ava_OTA_V3_Manifest.h"
#if defined(AVA_OTA_V3_TEST_CHANNEL)
#include "Ava_OTA_V3_TestPublicKey.h"
#define AVA_OTA_V3_PUBLIC_KEY_CONFIGURED AVA_OTA_V3_TEST_PUBLIC_KEY_CONFIGURED
#define AVA_OTA_V3_PUBLIC_KEY_PEM AVA_OTA_V3_TEST_PUBLIC_KEY_PEM
#else
#include "Ava_OTA_V3_PublicKey.h"
#endif

#include <ctype.h>
#include <string.h>
#include <mbedtls/base64.h>
#include <mbedtls/ecp.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>

namespace {
struct JsonCursor { const char* p; const char* end; };

void skipWhitespace(JsonCursor& c) {
    while (c.p < c.end && (*c.p == ' ' || *c.p == '\t' ||
           *c.p == '\r' || *c.p == '\n')) ++c.p;
}

bool consume(JsonCursor& c, char expected) {
    skipWhitespace(c);
    if (c.p >= c.end || *c.p != expected) return false;
    ++c.p;
    return true;
}

// The signed release schema is deliberately flat and ASCII-only.
// Escapes, nested values and unknown keys are rejected.
bool parsePlainString(JsonCursor& c, char* out, size_t capacity) {
    if (!consume(c, '"') || capacity == 0) return false;
    size_t used = 0;
    while (c.p < c.end && *c.p != '"') {
        const unsigned char ch = static_cast<unsigned char>(*c.p++);
        if (ch < 0x20 || ch > 0x7e || ch == '\\' || used + 1 >= capacity) return false;
        out[used++] = static_cast<char>(ch);
    }
    if (c.p >= c.end || *c.p != '"') return false;
    ++c.p;
    out[used] = '\0';
    return true;
}

bool parseUint32(JsonCursor& c, uint32_t& value) {
    skipWhitespace(c);
    if (c.p >= c.end || !isdigit(static_cast<unsigned char>(*c.p))) return false;
    uint64_t number = 0;
    while (c.p < c.end && isdigit(static_cast<unsigned char>(*c.p))) {
        number = number * 10U + static_cast<unsigned>(*c.p - '0');
        if (number > UINT32_MAX) return false;
        ++c.p;
    }
    value = static_cast<uint32_t>(number);
    return true;
}

bool isHex(const char* value, size_t length) {
    if (strlen(value) != length) return false;
    for (size_t i = 0; i < length; ++i)
        if (!isxdigit(static_cast<unsigned char>(value[i]))) return false;
    return true;
}

bool isVersion(const char* value) {
    const size_t n = strlen(value);
    if (n < 5 || n > 32) return false;
    unsigned dots = 0;
    bool digitInComponent = false;
    for (size_t i = 0; i < n; ++i) {
        if (value[i] == '.') {
            if (!digitInComponent) return false;
            ++dots;
            digitInComponent = false;
        } else if (isdigit(static_cast<unsigned char>(value[i]))) {
            digitInComponent = true;
        } else return false;
    }
    return dots == 2 && digitInComponent;
}

[[maybe_unused]] bool parseManifestJson(const uint8_t* bytes, size_t length, AvaOTAV3Manifest& m) {
    if (!bytes || length == 0 || length > 2048) return false;
    JsonCursor c {
        reinterpret_cast<const char*>(bytes),
        reinterpret_cast<const char*>(bytes) + length
    };
    memset(&m, 0, sizeof(m));
    if (!consume(c, '{')) return false;

    // schema, build, version, commit, firmware_url, size, sha256
    uint8_t seen = 0;
    for (unsigned fields = 0; fields < 7; ++fields) {
        char key[24] = {};
        if (!parsePlainString(c, key, sizeof(key)) || !consume(c, ':')) return false;

        uint8_t bit = 0;
        if (strcmp(key, "schema") == 0) {
            bit = 1U << 0;
            if (!parseUint32(c, m.schema)) return false;
        } else if (strcmp(key, "build") == 0) {
            bit = 1U << 1;
            if (!parseUint32(c, m.build)) return false;
        } else if (strcmp(key, "version") == 0) {
            bit = 1U << 2;
            if (!parsePlainString(c, m.version, sizeof(m.version))) return false;
        } else if (strcmp(key, "commit") == 0) {
            bit = 1U << 3;
            if (!parsePlainString(c, m.commit, sizeof(m.commit))) return false;
        } else if (strcmp(key, "firmware_url") == 0) {
            bit = 1U << 4;
            if (!parsePlainString(c, m.firmwareUrl, sizeof(m.firmwareUrl))) return false;
        } else if (strcmp(key, "size") == 0) {
            bit = 1U << 5;
            if (!parseUint32(c, m.size)) return false;
        } else if (strcmp(key, "sha256") == 0) {
            bit = 1U << 6;
            if (!parsePlainString(c, m.sha256, sizeof(m.sha256))) return false;
        } else return false;

        if (seen & bit) return false;
        seen |= bit;
        skipWhitespace(c);
        if (fields == 6) break;
        if (consume(c, '}') || !consume(c, ',')) return false;
    }

    if (!consume(c, '}')) return false;
    skipWhitespace(c);
    if (c.p != c.end || seen != 0x7f) return false;
    if (m.schema != 3 || m.build == 0 || m.size == 0 ||
        !isVersion(m.version) || !isHex(m.commit, 40) || !isHex(m.sha256, 64)) return false;

    for (size_t i = 0; m.sha256[i]; ++i)
        m.sha256[i] = static_cast<char>(tolower(static_cast<unsigned char>(m.sha256[i])));
    return true;
}
} // namespace

bool avaOTAV3VerifyAndParseManifest(
    const uint8_t* manifestBytes,
    size_t manifestLength,
    const char* signatureBase64,
    AvaOTAV3Manifest& output,
    AvaOTAV3Error& error
) {
    error = AVA_OTA_V3_ERR_SIGNATURE_INVALID;
#if !AVA_OTA_V3_PUBLIC_KEY_CONFIGURED
    (void)manifestBytes;
    (void)manifestLength;
    (void)signatureBase64;
    (void)output;
    error = AVA_OTA_V3_ERR_SIGNING_KEY_NOT_CONFIGURED;
    return false;
#else
    if (!manifestBytes || manifestLength == 0 || manifestLength > 2048 ||
        !signatureBase64 || !signatureBase64[0]) {
        error = AVA_OTA_V3_ERR_MANIFEST_SCHEMA;
        return false;
    }

    unsigned char signature[128] = {};
    size_t signatureLength = 0;
    const size_t encodedLength = strlen(signatureBase64);
    if (encodedLength < 80 || encodedLength > 180 ||
        mbedtls_base64_decode(
            signature, sizeof(signature), &signatureLength,
            reinterpret_cast<const unsigned char*>(signatureBase64),
            encodedLength
        ) != 0 || signatureLength == 0) {
        error = AVA_OTA_V3_ERR_SIGNATURE_INVALID;
        return false;
    }

    unsigned char digest[32] = {};
    if (mbedtls_sha256(manifestBytes, manifestLength, digest, 0) != 0) {
        error = AVA_OTA_V3_ERR_SIGNATURE_INVALID;
        return false;
    }

    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    const int parsed = mbedtls_pk_parse_public_key(
        &key,
        reinterpret_cast<const unsigned char*>(AVA_OTA_V3_PUBLIC_KEY_PEM),
        strlen(AVA_OTA_V3_PUBLIC_KEY_PEM) + 1
    );

    bool verified = false;
    if (parsed == 0 && mbedtls_pk_can_do(&key, MBEDTLS_PK_ECDSA)) {
        const mbedtls_ecp_keypair* ec = mbedtls_pk_ec(key);
        if (ec && mbedtls_ecp_keypair_get_group_id(ec) == MBEDTLS_ECP_DP_SECP256R1) {
            verified = mbedtls_pk_verify(
                &key, MBEDTLS_MD_SHA256, digest, sizeof(digest),
                signature, signatureLength
            ) == 0;
        }
    }
    mbedtls_pk_free(&key);

    if (!verified) {
        error = AVA_OTA_V3_ERR_SIGNATURE_INVALID;
        return false;
    }
    if (!parseManifestJson(manifestBytes, manifestLength, output)) {
        error = AVA_OTA_V3_ERR_MANIFEST_SCHEMA;
        return false;
    }
    error = AVA_OTA_V3_ERR_NONE;
    return true;
#endif
}
