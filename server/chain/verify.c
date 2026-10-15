#include "crypto.h"

#include <string.h>

static bool bytes_equal(const uint8_t *left, const uint8_t *right,
                        size_t length)
{
    unsigned int difference = 0u;
    size_t i;

    for (i = 0u; i < length; ++i) {
        difference |= (unsigned int)(left[i] ^ right[i]);
    }
    return difference == 0u;
}

static bool bytes_are_zero(const uint8_t *bytes, size_t length)
{
    uint8_t combined = 0u;
    size_t i;

    for (i = 0u; i < length; ++i) {
        combined = (uint8_t)(combined | bytes[i]);
    }
    return combined == 0u;
}

static void set_report(crypto_verify_report_t *report,
                       crypto_status_t status, size_t position,
                       uint32_t block_index)
{
    if (report != NULL) {
        report->status = status;
        report->block_position = position;
        report->block_index = block_index;
    }
}

crypto_status_t crypto_verify_block(const crypto_block_t *block)
{
    uint8_t calculated_payload_hash[CRYPTO_SHA256_BYTES];
    uint8_t calculated_hash[CRYPTO_SHA256_BYTES];
    bool meets;
    crypto_status_t status;

    if (block == NULL) {
        return CRYPTO_ERR_ARGUMENT;
    }
    status = crypto_payload_sha256(block->payload_canon,
                                   block->payload_canon_len,
                                   calculated_payload_hash);
    if (status != CRYPTO_OK) {
        return status;
    }
    if (!bytes_equal(calculated_payload_hash, block->header.payload_hash,
                     CRYPTO_SHA256_BYTES)) {
        return CRYPTO_ERR_PAYLOAD_HASH_MISMATCH;
    }
    status = crypto_header_sha256(&block->header, calculated_hash);
    if (status != CRYPTO_OK) {
        return status;
    }
    if (!bytes_equal(calculated_hash, block->hash, CRYPTO_SHA256_BYTES)) {
        return CRYPTO_ERR_BLOCK_HASH_MISMATCH;
    }
    status = crypto_hash_meets_difficulty(calculated_hash,
                                           block->header.difficulty,
                                           &meets);
    if (status != CRYPTO_OK) {
        return status;
    }
    return meets ? CRYPTO_OK : CRYPTO_ERR_POW_INVALID;
}

crypto_status_t crypto_verify_chain(const crypto_block_t *blocks, size_t count,
                                    crypto_verify_report_t *report_out)
{
    size_t i;
    crypto_status_t status;

    set_report(report_out, CRYPTO_OK, SIZE_MAX, 0u);
    if (blocks == NULL) {
        set_report(report_out, CRYPTO_ERR_ARGUMENT, SIZE_MAX, 0u);
        return CRYPTO_ERR_ARGUMENT;
    }
    if (count == 0u) {
        set_report(report_out, CRYPTO_ERR_CHAIN_EMPTY, SIZE_MAX, 0u);
        return CRYPTO_ERR_CHAIN_EMPTY;
    }

    for (i = 0u; i < count; ++i) {
        const crypto_block_t *block = &blocks[i];
        const uint32_t expected_index = (uint32_t)i;

        if (i > (size_t)UINT32_MAX || block->header.block_index != expected_index) {
            set_report(report_out, CRYPTO_ERR_BLOCK_INDEX, i,
                       block->header.block_index);
            return CRYPTO_ERR_BLOCK_INDEX;
        }
        if (i == 0u) {
            if (!bytes_are_zero(block->header.previous_hash,
                                CRYPTO_SHA256_BYTES)) {
                set_report(report_out, CRYPTO_ERR_CHAIN_LINK, i,
                           block->header.block_index);
                return CRYPTO_ERR_CHAIN_LINK;
            }
        } else if (!bytes_equal(block->header.previous_hash,
                                blocks[i - 1u].hash,
                                CRYPTO_SHA256_BYTES)) {
            set_report(report_out, CRYPTO_ERR_CHAIN_LINK, i,
                       block->header.block_index);
            return CRYPTO_ERR_CHAIN_LINK;
        }
        status = crypto_verify_block(block);
        if (status != CRYPTO_OK) {
            set_report(report_out, status, i, block->header.block_index);
            return status;
        }
    }
    return CRYPTO_OK;
}
