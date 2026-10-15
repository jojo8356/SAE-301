/*
 * AnimeTCG block-chain cryptographic primitives.
 *
 * This module hashes caller-supplied RFC 8785 canonical JSON bytes; it does
 * not parse or canonicalize JSON. No function in this module allocates heap
 * memory. Hashes are exposed as 32 raw bytes; hex formatting belongs to the
 * persistence / presentation boundary.
 */
#ifndef ANIMETCG_CRYPTO_H
#define ANIMETCG_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CRYPTO_SHA256_BYTES       32u
#define CRYPTO_SHA256_HEX_BYTES   64u
#define CRYPTO_BLOCK_HEADER_BYTES 96u
#define CRYPTO_PAYLOAD_MIN_BYTES  2u
#define CRYPTO_PAYLOAD_MAX_BYTES  (64u * 1024u)
#define CRYPTO_HEADER_VERSION     1u
#define CRYPTO_DIFFICULTY_MIN     3u
#define CRYPTO_DIFFICULTY_MAX     4u
#define CRYPTO_MAX_POW_ATTEMPTS   (UINT64_C(1) << 20)
#define CRYPTO_DIFFICULTY_3_RATE  UINT64_C(2048)
#define CRYPTO_DIFFICULTY_4_RATE  UINT64_C(32768)

typedef enum {
    CRYPTO_OK = 0,
    CRYPTO_ERR_ARGUMENT = -1,
    CRYPTO_ERR_PAYLOAD_LENGTH = -2,
    CRYPTO_ERR_HEADER_VERSION = -3,
    CRYPTO_ERR_TIMESTAMP = -4,
    CRYPTO_ERR_DIFFICULTY = -5,
    CRYPTO_ERR_ATTEMPT_BUDGET = -6,
    CRYPTO_ERR_NONCE_OVERFLOW = -7,
    CRYPTO_ERR_OPENSSL = -8,
    CRYPTO_ERR_NO_WORK = -9,
    CRYPTO_ERR_PAYLOAD_HASH_MISMATCH = -10,
    CRYPTO_ERR_BLOCK_HASH_MISMATCH = -11,
    CRYPTO_ERR_POW_INVALID = -12,
    CRYPTO_ERR_CHAIN_EMPTY = -13,
    CRYPTO_ERR_CHAIN_LINK = -14,
    CRYPTO_ERR_BLOCK_INDEX = -15
} crypto_status_t;

typedef struct {
    uint32_t version;
    uint32_t block_index;
    int64_t timestamp_ms;
    uint8_t previous_hash[CRYPTO_SHA256_BYTES];
    uint32_t action_id;
    uint32_t difficulty;
    uint64_t nonce;
    uint8_t payload_hash[CRYPTO_SHA256_BYTES];
} crypto_header_fields_t;

typedef struct {
    crypto_header_fields_t header;
    /* Borrowed, immutable RFC 8785 canonical JSON bytes; never owned here. */
    const uint8_t *payload_canon;
    size_t payload_canon_len;
    uint8_t hash[CRYPTO_SHA256_BYTES];
} crypto_block_t;

typedef struct {
    uint64_t nonce;
    uint64_t attempts;
    uint8_t hash[CRYPTO_SHA256_BYTES];
} crypto_pow_result_t;

typedef struct {
    crypto_status_t status;
    size_t block_position;
    uint32_t block_index;
} crypto_verify_report_t;

/*
 * Hashes any byte string with OpenSSL SHA-256. A NULL input is accepted only
 * when input_len is zero (the empty-message test vector).
 */
crypto_status_t crypto_sha256(const uint8_t *input, size_t input_len,
                              uint8_t digest_out[CRYPTO_SHA256_BYTES]);

/*
 * Hashes a previously canonicalized RFC 8785 JSON payload. This function
 * checks only the documented 2..65536-byte boundary; it does not parse JSON
 * or prove that the bytes are canonical.
 */
crypto_status_t crypto_payload_sha256(
    const uint8_t *payload_canon, size_t payload_canon_len,
    uint8_t digest_out[CRYPTO_SHA256_BYTES]);

/* Selects difficulty 4 at >=32768 hashes/s, otherwise 3. Rates below 2048
 * hashes/s are reported through out_below_target (the policy floor is 3). */
crypto_status_t crypto_difficulty_for_rate(uint64_t hashes_per_second,
                                             uint32_t *difficulty_out,
                                             bool *below_target_out);

/* Returns 16^(difficulty+1), the bounded search budget in the specification. */
crypto_status_t crypto_default_attempt_budget(uint32_t difficulty,
                                               uint64_t *attempts_out);

/* Serializes the canonical version-1 block header into exactly 96 bytes. */
crypto_status_t crypto_header_encode(
    const crypto_header_fields_t *fields,
    uint8_t header_out[CRYPTO_BLOCK_HEADER_BYTES]);

/* Computes SHA-256 over the 96-byte canonical header (single SHA-256). */
crypto_status_t crypto_header_sha256(
    const crypto_header_fields_t *fields,
    uint8_t digest_out[CRYPTO_SHA256_BYTES]);

/* Tests a hexadecimal leading-zero proof-of-work target (difficulty 3 or 4). */
crypto_status_t crypto_hash_meets_difficulty(
    const uint8_t hash[CRYPTO_SHA256_BYTES], uint32_t difficulty,
    bool *meets_out);

/*
 * Mines a header without heap allocations. The caller supplies the first
 * nonce and a positive attempt budget no larger than the specification's
 * 16^(difficulty+1) limit. out is written only on success and is zeroed on
 * every failure. The caller is responsible for any wall-clock deadline.
 */
crypto_status_t crypto_mine_header(
    const crypto_header_fields_t *fields, uint64_t first_nonce,
    uint64_t max_attempts, crypto_pow_result_t *out);

/* Hashes the borrowed canonical payload, mines from nonce zero, then updates
 * the block atomically on success. attempts_out may be NULL. */
crypto_status_t crypto_block_mine(crypto_block_t *block,
                                  uint64_t *attempts_out);

/* Recomputes the payload digest, header digest, and that block's own PoW. */
crypto_status_t crypto_verify_block(const crypto_block_t *block);

/* Verifies genesis, contiguous indices, previous-hash links, each block hash,
 * payload digest, and each block's own difficulty. An empty chain is invalid.
 * report_out is optional; the input blocks and payloads are never modified. */
crypto_status_t crypto_verify_chain(const crypto_block_t *blocks, size_t count,
                                    crypto_verify_report_t *report_out);

/* Returns a stable, static English description; no allocation is performed. */
const char *crypto_status_string(crypto_status_t status);

#endif /* ANIMETCG_CRYPTO_H */
