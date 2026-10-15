#include "crypto.h"
#include "crypto_internal.h"

#include <string.h>

crypto_status_t crypto_difficulty_for_rate(uint64_t hashes_per_second,
                                             uint32_t *difficulty_out,
                                             bool *below_target_out)
{
    if (difficulty_out == NULL || below_target_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    *difficulty_out = 0u;
    *below_target_out = false;
    *difficulty_out = hashes_per_second >= CRYPTO_DIFFICULTY_4_RATE
                          ? CRYPTO_DIFFICULTY_MAX
                          : CRYPTO_DIFFICULTY_MIN;
    *below_target_out = hashes_per_second < CRYPTO_DIFFICULTY_3_RATE;
    return CRYPTO_OK;
}

crypto_status_t crypto_default_attempt_budget(uint32_t difficulty,
                                               uint64_t *attempts_out)
{
    unsigned int exponent;

    if (attempts_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    *attempts_out = 0u;
    if (!crypto_difficulty_is_supported(difficulty)) {
        return CRYPTO_ERR_DIFFICULTY;
    }
    exponent = 4u * (difficulty + 1u);
    *attempts_out = UINT64_C(1) << exponent;
    return CRYPTO_OK;
}

crypto_status_t crypto_hash_meets_difficulty(
    const uint8_t hash[CRYPTO_SHA256_BYTES], uint32_t difficulty,
    bool *meets_out)
{
    size_t full_zero_bytes;
    size_t i;

    if (meets_out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    *meets_out = false;
    if (hash == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    if (!crypto_difficulty_is_supported(difficulty)) {
        return CRYPTO_ERR_DIFFICULTY;
    }

    full_zero_bytes = (size_t)(difficulty / 2u);
    for (i = 0u; i < full_zero_bytes; ++i) {
        if (hash[i] != 0u) {
            return CRYPTO_OK;
        }
    }
    if ((difficulty % 2u) != 0u &&
        (hash[full_zero_bytes] & UINT8_C(0xf0)) != 0u) {
        return CRYPTO_OK;
    }
    *meets_out = true;
    return CRYPTO_OK;
}

crypto_status_t crypto_mine_header(
    const crypto_header_fields_t *fields, uint64_t first_nonce,
    uint64_t max_attempts, crypto_pow_result_t *out)
{
    crypto_header_fields_t local_fields;
    crypto_pow_result_t result = {0u, 0u, {0u}};
    uint8_t header[CRYPTO_BLOCK_HEADER_BYTES];
    uint8_t digest[CRYPTO_SHA256_BYTES];
    uint64_t default_budget;
    uint64_t last_offset;
    uint64_t attempt;
    bool meets;
    crypto_status_t status;

    if (fields != NULL) {
        local_fields = *fields;
    }
    if (out == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    (void)memset(out, 0, sizeof(*out));
    if (fields == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    status = crypto_validate_header_fields(&local_fields);
    if (status != CRYPTO_OK) {
        return status;
    }
    status = crypto_default_attempt_budget(local_fields.difficulty,
                                           &default_budget);
    if (status != CRYPTO_OK) {
        return status;
    }
    if (max_attempts == 0u || max_attempts > default_budget) {
        return CRYPTO_ERR_ATTEMPT_BUDGET;
    }
    last_offset = max_attempts - 1u;
    if (last_offset > UINT64_MAX - first_nonce) {
        return CRYPTO_ERR_NONCE_OVERFLOW;
    }

    local_fields.nonce = first_nonce;
    status = crypto_header_encode(&local_fields, header);
    if (status != CRYPTO_OK) {
        return status;
    }

    for (attempt = 0u; attempt < max_attempts; ++attempt) {
        const uint64_t nonce = first_nonce + attempt;

        crypto_store_u64_be(&header[56], nonce);
        status = crypto_sha256(header, sizeof(header), digest);
        if (status != CRYPTO_OK) {
            return status;
        }
        status = crypto_hash_meets_difficulty(digest,
                                               local_fields.difficulty,
                                               &meets);
        if (status != CRYPTO_OK) {
            return status;
        }
        if (meets) {
            result.nonce = nonce;
            result.attempts = attempt + 1u;
            (void)memcpy(result.hash, digest, sizeof(result.hash));
            *out = result;
            return CRYPTO_OK;
        }
    }
    return CRYPTO_ERR_NO_WORK;
}

crypto_status_t crypto_block_mine(crypto_block_t *block,
                                  uint64_t *attempts_out)
{
    crypto_block_t mined;
    crypto_pow_result_t result;
    uint64_t budget;
    crypto_status_t status;

    if (attempts_out != NULL) {
        *attempts_out = 0u;
    }
    if (block == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    mined = *block;
    status = crypto_payload_sha256(mined.payload_canon,
                                   mined.payload_canon_len,
                                   mined.header.payload_hash);
    if (status != CRYPTO_OK) {
        return status;
    }
    status = crypto_default_attempt_budget(mined.header.difficulty, &budget);
    if (status != CRYPTO_OK) {
        return status;
    }
    status = crypto_mine_header(&mined.header, 0u, budget, &result);
    if (status != CRYPTO_OK) {
        return status;
    }
    mined.header.nonce = result.nonce;
    (void)memcpy(mined.hash, result.hash, sizeof(mined.hash));
    *block = mined;
    if (attempts_out != NULL) {
        *attempts_out = result.attempts;
    }
    return CRYPTO_OK;
}
