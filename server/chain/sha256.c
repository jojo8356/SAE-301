#include "crypto.h"

#include <openssl/sha.h>

#include <string.h>

crypto_status_t crypto_sha256(const uint8_t *input, size_t input_len,
                              uint8_t digest_out[CRYPTO_SHA256_BYTES])
{
    static const uint8_t empty_input = 0u;
    const uint8_t *bytes = input;
    uint8_t calculated[CRYPTO_SHA256_BYTES] = {0u};

    if (digest_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    if (input == NULL && input_len != 0u) {
        (void)memset(digest_out, 0, CRYPTO_SHA256_BYTES);
        return CRYPTO_ERR_ARGUMENT;
    }
    if (input == NULL) {
        bytes = &empty_input;
    }
    if (SHA256(bytes, input_len, calculated) == NULL) {
        (void)memset(digest_out, 0, CRYPTO_SHA256_BYTES);
        return CRYPTO_ERR_OPENSSL;
    }
    (void)memcpy(digest_out, calculated, sizeof(calculated));
    return CRYPTO_OK;
}

crypto_status_t crypto_payload_sha256(
    const uint8_t *payload_canon, size_t payload_canon_len,
    uint8_t digest_out[CRYPTO_SHA256_BYTES])
{
    if (digest_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    if (payload_canon == NULL) {
        (void)memset(digest_out, 0, CRYPTO_SHA256_BYTES);
        return CRYPTO_ERR_ARGUMENT;
    }
    if (payload_canon_len < CRYPTO_PAYLOAD_MIN_BYTES ||
        payload_canon_len > CRYPTO_PAYLOAD_MAX_BYTES) {
        (void)memset(digest_out, 0, CRYPTO_SHA256_BYTES);
        return CRYPTO_ERR_PAYLOAD_LENGTH;
    }
    return crypto_sha256(payload_canon, payload_canon_len, digest_out);
}
