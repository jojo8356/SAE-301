#include "crypto.h"
#include "crypto_internal.h"

#include <string.h>

bool crypto_difficulty_is_supported(uint32_t difficulty)
{
    return difficulty == CRYPTO_DIFFICULTY_MIN ||
           difficulty == CRYPTO_DIFFICULTY_MAX;
}

crypto_status_t crypto_validate_header_fields(
    const crypto_header_fields_t *fields)
{
    if (fields == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    if (fields->version != CRYPTO_HEADER_VERSION) {
        return CRYPTO_ERR_HEADER_VERSION;
    }
    if (fields->timestamp_ms < 0) {
        return CRYPTO_ERR_TIMESTAMP;
    }
    if (!crypto_difficulty_is_supported(fields->difficulty)) {
        return CRYPTO_ERR_DIFFICULTY;
    }
    return CRYPTO_OK;
}

void crypto_store_u32_be(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)(value >> 24);
    destination[1] = (uint8_t)(value >> 16);
    destination[2] = (uint8_t)(value >> 8);
    destination[3] = (uint8_t)value;
}

void crypto_store_u64_be(uint8_t *destination, uint64_t value)
{
    destination[0] = (uint8_t)(value >> 56);
    destination[1] = (uint8_t)(value >> 48);
    destination[2] = (uint8_t)(value >> 40);
    destination[3] = (uint8_t)(value >> 32);
    destination[4] = (uint8_t)(value >> 24);
    destination[5] = (uint8_t)(value >> 16);
    destination[6] = (uint8_t)(value >> 8);
    destination[7] = (uint8_t)value;
}

crypto_status_t crypto_header_encode(
    const crypto_header_fields_t *fields,
    uint8_t header_out[CRYPTO_BLOCK_HEADER_BYTES])
{
    uint8_t encoded[CRYPTO_BLOCK_HEADER_BYTES] = {0u};
    uint64_t timestamp_bits;
    crypto_status_t status;

    if (header_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    status = crypto_validate_header_fields(fields);
    if (status != CRYPTO_OK) {
        (void)memset(header_out, 0, CRYPTO_BLOCK_HEADER_BYTES);
        return status;
    }

    timestamp_bits = (uint64_t)fields->timestamp_ms;
    crypto_store_u32_be(&encoded[0], fields->version);
    crypto_store_u32_be(&encoded[4], fields->block_index);
    crypto_store_u64_be(&encoded[8], timestamp_bits);
    (void)memcpy(&encoded[16], fields->previous_hash,
                 CRYPTO_SHA256_BYTES);
    crypto_store_u32_be(&encoded[48], fields->action_id);
    crypto_store_u32_be(&encoded[52], fields->difficulty);
    crypto_store_u64_be(&encoded[56], fields->nonce);
    (void)memcpy(&encoded[64], fields->payload_hash,
                 CRYPTO_SHA256_BYTES);
    (void)memcpy(header_out, encoded, sizeof(encoded));
    return CRYPTO_OK;
}

crypto_status_t crypto_header_sha256(
    const crypto_header_fields_t *fields,
    uint8_t digest_out[CRYPTO_SHA256_BYTES])
{
    uint8_t encoded[CRYPTO_BLOCK_HEADER_BYTES];
    uint8_t calculated[CRYPTO_SHA256_BYTES];
    crypto_status_t status;

    if (digest_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    status = crypto_header_encode(fields, encoded);
    if (status != CRYPTO_OK) {
        (void)memset(digest_out, 0, CRYPTO_SHA256_BYTES);
        return status;
    }
    status = crypto_sha256(encoded, sizeof(encoded), calculated);
    if (status != CRYPTO_OK) {
        (void)memset(digest_out, 0, CRYPTO_SHA256_BYTES);
        return status;
    }
    (void)memcpy(digest_out, calculated, sizeof(calculated));
    return CRYPTO_OK;
}
