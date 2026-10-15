#include "crypto.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int checks_run;
static unsigned int checks_failed;

#define CHECK(condition)                                                        \
    do {                                                                         \
        ++checks_run;                                                            \
        if (!(condition)) {                                                       \
            (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,      \
                          #condition);                                            \
            ++checks_failed;                                                     \
        }                                                                        \
    } while (0)

#define CHECK_STATUS(expression, expected_status)                               \
    do {                                                                         \
        const crypto_status_t actual_status = (expression);                      \
        ++checks_run;                                                            \
        if (actual_status != (expected_status)) {                                \
            (void)fprintf(stderr,                                                \
                          "FAIL %s:%d: status %s (%d), expected %s (%d)\n",   \
                          __FILE__, __LINE__,                                    \
                          crypto_status_string(actual_status),                   \
                          (int)actual_status,                                    \
                          crypto_status_string(expected_status),                 \
                          (int)(expected_status));                               \
            ++checks_failed;                                                     \
        }                                                                        \
    } while (0)

static int hex_nibble(char character)
{
    if (character >= '0' && character <= '9') {
        return (int)(character - '0');
    }
    if (character >= 'a' && character <= 'f') {
        return (int)(character - 'a') + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return (int)(character - 'A') + 10;
    }
    return -1;
}

static bool decode_hex(const char *text, uint8_t *out, size_t out_len)
{
    size_t i;

    if (text == NULL || (out == NULL && out_len != 0u) ||
        strlen(text) != out_len * 2u) {
        return false;
    }
    for (i = 0u; i < out_len; ++i) {
        const int high = hex_nibble(text[2u * i]);
        const int low = hex_nibble(text[(2u * i) + 1u]);

        if (high < 0 || low < 0) {
            return false;
        }
        out[i] = (uint8_t)(((unsigned int)high << 4) | (unsigned int)low);
    }
    return true;
}

static void encode_hex(const uint8_t *bytes, size_t length,
                       char *out, size_t out_capacity)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    if (out == NULL || out_capacity < (length * 2u) + 1u ||
        (bytes == NULL && length != 0u)) {
        return;
    }
    for (i = 0u; i < length; ++i) {
        out[2u * i] = digits[(unsigned int)(bytes[i] >> 4)];
        out[(2u * i) + 1u] = digits[(unsigned int)(bytes[i] & 0x0fu)];
    }
    out[length * 2u] = '\0';
}

static bool all_zero(const uint8_t *bytes, size_t length)
{
    size_t i;

    for (i = 0u; i < length; ++i) {
        if (bytes[i] != 0u) {
            return false;
        }
    }
    return true;
}

static void test_sha256_vectors(void)
{
    static const uint8_t abc[] = "abc";
    uint8_t digest[CRYPTO_SHA256_BYTES];
    uint8_t in_place[CRYPTO_SHA256_BYTES] = {'a', 'b', 'c'};
    char digest_hex[CRYPTO_SHA256_HEX_BYTES + 1u];

    CHECK_STATUS(crypto_sha256(NULL, 0u, digest), CRYPTO_OK);
    encode_hex(digest, sizeof(digest), digest_hex, sizeof(digest_hex));
    CHECK(strcmp(digest_hex,
                 "e3b0c44298fc1c149afbf4c8996fb924"
                 "27ae41e4649b934ca495991b7852b855") == 0);

    CHECK_STATUS(crypto_sha256(abc, sizeof(abc) - 1u, digest), CRYPTO_OK);
    encode_hex(digest, sizeof(digest), digest_hex, sizeof(digest_hex));
    CHECK(strcmp(digest_hex,
                 "ba7816bf8f01cfea414140de5dae2223"
                 "b00361a396177a9cb410ff61f20015ad") == 0);

    CHECK_STATUS(crypto_sha256(in_place, 3u, in_place), CRYPTO_OK);
    CHECK(memcmp(in_place, digest, sizeof(in_place)) == 0);

    CHECK_STATUS(crypto_sha256(NULL, 1u, digest), CRYPTO_ERR_ARGUMENT);
    CHECK(all_zero(digest, sizeof(digest)));
}

static void test_payload_hash_bounds(void)
{
    static const uint8_t empty_object[] = "{}";
    static const uint8_t one_byte[] = "{";
    static const uint8_t alias_payload[] = "{\"n\":0}";
    uint8_t digest[CRYPTO_SHA256_BYTES];
    uint8_t expected[CRYPTO_SHA256_BYTES];
    uint8_t in_place[CRYPTO_SHA256_BYTES] = {0u};

    CHECK_STATUS(crypto_payload_sha256(empty_object,
                                      sizeof(empty_object) - 1u, digest),
                 CRYPTO_OK);
    CHECK_STATUS(crypto_payload_sha256(alias_payload,
                                      sizeof(alias_payload) - 1u, expected),
                 CRYPTO_OK);
    (void)memcpy(in_place, alias_payload, sizeof(alias_payload) - 1u);
    CHECK_STATUS(crypto_payload_sha256(in_place,
                                      sizeof(alias_payload) - 1u, in_place),
                 CRYPTO_OK);
    CHECK(memcmp(in_place, expected, sizeof(expected)) == 0);
    CHECK_STATUS(crypto_payload_sha256(one_byte, sizeof(one_byte) - 1u,
                                      digest),
                 CRYPTO_ERR_PAYLOAD_LENGTH);
    CHECK(all_zero(digest, sizeof(digest)));
    CHECK_STATUS(crypto_payload_sha256(empty_object,
                                      (size_t)CRYPTO_PAYLOAD_MAX_BYTES + 1u,
                                      digest),
                 CRYPTO_ERR_PAYLOAD_LENGTH);
    CHECK_STATUS(crypto_payload_sha256(NULL, 2u, digest),
                 CRYPTO_ERR_ARGUMENT);
}

static void test_difficulty_policy(void)
{
    uint32_t difficulty = 0u;
    bool below_target = false;
    uint64_t budget = 0u;

    CHECK_STATUS(crypto_difficulty_for_rate(0u, &difficulty, &below_target),
                 CRYPTO_OK);
    CHECK(difficulty == 3u && below_target);
    CHECK_STATUS(crypto_difficulty_for_rate(UINT64_C(2047), &difficulty,
                                            &below_target),
                 CRYPTO_OK);
    CHECK(difficulty == 3u && below_target);
    CHECK_STATUS(crypto_difficulty_for_rate(UINT64_C(2048), &difficulty,
                                            &below_target),
                 CRYPTO_OK);
    CHECK(difficulty == 3u && !below_target);
    CHECK_STATUS(crypto_difficulty_for_rate(UINT64_C(32767), &difficulty,
                                            &below_target),
                 CRYPTO_OK);
    CHECK(difficulty == 3u && !below_target);
    CHECK_STATUS(crypto_difficulty_for_rate(UINT64_C(32768), &difficulty,
                                            &below_target),
                 CRYPTO_OK);
    CHECK(difficulty == 4u && !below_target);
    CHECK_STATUS(crypto_difficulty_for_rate(1u, NULL, &below_target),
                 CRYPTO_ERR_ARGUMENT);

    CHECK_STATUS(crypto_default_attempt_budget(3u, &budget), CRYPTO_OK);
    CHECK(budget == UINT64_C(65536));
    CHECK_STATUS(crypto_default_attempt_budget(4u, &budget), CRYPTO_OK);
    CHECK(budget == UINT64_C(1048576));
    CHECK_STATUS(crypto_default_attempt_budget(2u, &budget),
                 CRYPTO_ERR_DIFFICULTY);
    CHECK(budget == 0u);
}

static void test_proof_of_work_nibbles(void)
{
    uint8_t hash[CRYPTO_SHA256_BYTES] = {0u};
    bool meets = false;

    hash[1] = 0x0fu;
    CHECK_STATUS(crypto_hash_meets_difficulty(hash, 3u, &meets), CRYPTO_OK);
    CHECK(meets);
    CHECK_STATUS(crypto_hash_meets_difficulty(hash, 4u, &meets), CRYPTO_OK);
    CHECK(!meets);

    hash[1] = 0x10u;
    CHECK_STATUS(crypto_hash_meets_difficulty(hash, 3u, &meets), CRYPTO_OK);
    CHECK(!meets);
    hash[1] = 0u;
    CHECK_STATUS(crypto_hash_meets_difficulty(hash, 4u, &meets), CRYPTO_OK);
    CHECK(meets);
    CHECK_STATUS(crypto_hash_meets_difficulty(hash, 2u, &meets),
                 CRYPTO_ERR_DIFFICULTY);
    CHECK(!meets);
}

static void test_golden_header_vector(void)
{
    static const uint8_t payload[] =
        "{\"episodes\":26,\"title_romaji\":\"Cowboy Bebop\",\"year\":1998}";
    static const char expected_payload_hash[] =
        "a79a3d87a822afbd43f4191ae858bf03525779cc2f092362ec83953e3a792a23";
    static const char expected_header_hex[] =
        "0000000100000076000001a0c03b8d54"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "000000010000000300000000000005cc"
        "a79a3d87a822afbd43f4191ae858bf03525779cc2f092362ec83953e3a792a23";
    static const char expected_block_hash[] =
        "000e857b87a46fb7617d396f1a60c941e09318da4ba5ea9d3c37ac64e30daf54";
    crypto_header_fields_t fields;
    uint8_t header[CRYPTO_BLOCK_HEADER_BYTES];
    uint8_t expected_header[CRYPTO_BLOCK_HEADER_BYTES];
    uint8_t digest[CRYPTO_SHA256_BYTES];
    uint8_t expected_payload_digest[CRYPTO_SHA256_BYTES];
    char digest_hex[CRYPTO_SHA256_HEX_BYTES + 1u];

    (void)memset(&fields, 0, sizeof(fields));
    fields.version = CRYPTO_HEADER_VERSION;
    fields.block_index = 118u;
    fields.timestamp_ms = INT64_C(1789931523412);
    fields.action_id = 1u;
    fields.difficulty = 3u;
    fields.nonce = UINT64_C(1484);

    CHECK(decode_hex(expected_payload_hash, expected_payload_digest,
                     sizeof(expected_payload_digest)));
    CHECK_STATUS(crypto_payload_sha256(payload, sizeof(payload) - 1u,
                                       fields.payload_hash),
                 CRYPTO_OK);
    CHECK(memcmp(fields.payload_hash, expected_payload_digest,
                 sizeof(fields.payload_hash)) == 0);
    CHECK(decode_hex(expected_header_hex, expected_header,
                     sizeof(expected_header)));
    CHECK_STATUS(crypto_header_encode(&fields, header), CRYPTO_OK);
    CHECK(memcmp(header, expected_header, sizeof(header)) == 0);
    CHECK_STATUS(crypto_header_sha256(&fields, digest), CRYPTO_OK);
    encode_hex(digest, sizeof(digest), digest_hex, sizeof(digest_hex));
    CHECK(strcmp(digest_hex, expected_block_hash) == 0);
}

static void test_mining_and_block_verification(void)
{
    static const uint8_t payload[] =
        "{\"episodes\":26,\"title_romaji\":\"Cowboy Bebop\",\"year\":1998}";
    static const char expected_hash[] =
        "000e857b87a46fb7617d396f1a60c941e09318da4ba5ea9d3c37ac64e30daf54";
    crypto_block_t block;
    uint64_t attempts = UINT64_MAX;
    char hash_hex[CRYPTO_SHA256_HEX_BYTES + 1u];
    uint8_t saved_hash[CRYPTO_SHA256_BYTES];

    (void)memset(&block, 0, sizeof(block));
    block.header.version = CRYPTO_HEADER_VERSION;
    block.header.block_index = 118u;
    block.header.timestamp_ms = INT64_C(1789931523412);
    block.header.action_id = 1u;
    block.header.difficulty = 3u;
    block.payload_canon = payload;
    block.payload_canon_len = sizeof(payload) - 1u;

    CHECK_STATUS(crypto_block_mine(&block, &attempts), CRYPTO_OK);
    CHECK(block.header.nonce == UINT64_C(1484));
    CHECK(attempts == UINT64_C(1485));
    encode_hex(block.hash, sizeof(block.hash), hash_hex, sizeof(hash_hex));
    CHECK(strcmp(hash_hex, expected_hash) == 0);
    CHECK_STATUS(crypto_verify_block(&block), CRYPTO_OK);

    (void)memcpy(saved_hash, block.hash, sizeof(saved_hash));
    block.hash[CRYPTO_SHA256_BYTES - 1u] ^= 1u;
    CHECK_STATUS(crypto_verify_block(&block), CRYPTO_ERR_BLOCK_HASH_MISMATCH);
    (void)memcpy(block.hash, saved_hash, sizeof(block.hash));

    block.payload_canon_len = 1u;
    attempts = UINT64_MAX;
    CHECK_STATUS(crypto_block_mine(&block, &attempts),
                 CRYPTO_ERR_PAYLOAD_LENGTH);
    CHECK(attempts == 0u);
    CHECK(block.header.nonce == UINT64_C(1484));
    CHECK(memcmp(block.hash, saved_hash, sizeof(block.hash)) == 0);
}

static void test_chain_verification_and_tampering(void)
{
    static uint8_t genesis_payload[] = "{\"n\":0}";
    static const uint8_t next_payload[] = "{\"n\":1}";
    crypto_block_t blocks[2];
    crypto_verify_report_t report;
    uint8_t saved_previous_hash[CRYPTO_SHA256_BYTES];
    char saved_character;

    (void)memset(blocks, 0, sizeof(blocks));
    blocks[0].header.version = CRYPTO_HEADER_VERSION;
    blocks[0].header.block_index = 0u;
    blocks[0].header.timestamp_ms = INT64_C(1000);
    blocks[0].header.action_id = 1u;
    blocks[0].header.difficulty = 3u;
    blocks[0].payload_canon = genesis_payload;
    blocks[0].payload_canon_len = sizeof(genesis_payload) - 1u;
    CHECK_STATUS(crypto_block_mine(&blocks[0], NULL), CRYPTO_OK);

    blocks[1].header.version = CRYPTO_HEADER_VERSION;
    blocks[1].header.block_index = 1u;
    blocks[1].header.timestamp_ms = INT64_C(1001);
    (void)memcpy(blocks[1].header.previous_hash, blocks[0].hash,
                 sizeof(blocks[1].header.previous_hash));
    blocks[1].header.action_id = 1u;
    blocks[1].header.difficulty = 3u;
    blocks[1].payload_canon = next_payload;
    blocks[1].payload_canon_len = sizeof(next_payload) - 1u;
    CHECK_STATUS(crypto_block_mine(&blocks[1], NULL), CRYPTO_OK);

    CHECK_STATUS(crypto_verify_chain(blocks, 2u, &report), CRYPTO_OK);
    CHECK(report.status == CRYPTO_OK && report.block_position == SIZE_MAX);

    (void)memcpy(saved_previous_hash, blocks[1].header.previous_hash,
                 sizeof(saved_previous_hash));
    blocks[1].header.previous_hash[0] ^= 1u;
    CHECK_STATUS(crypto_verify_chain(blocks, 2u, &report),
                 CRYPTO_ERR_CHAIN_LINK);
    CHECK(report.block_position == 1u && report.block_index == 1u);
    (void)memcpy(blocks[1].header.previous_hash, saved_previous_hash,
                 sizeof(saved_previous_hash));

    blocks[1].header.block_index = 2u;
    CHECK_STATUS(crypto_verify_chain(blocks, 2u, &report),
                 CRYPTO_ERR_BLOCK_INDEX);
    CHECK(report.block_position == 1u);
    blocks[1].header.block_index = 1u;

    blocks[0].header.difficulty = 4u;
    CHECK_STATUS(crypto_verify_chain(blocks, 2u, &report),
                 CRYPTO_ERR_BLOCK_HASH_MISMATCH);
    CHECK(report.block_position == 0u);
    blocks[0].header.difficulty = 3u;

    saved_character = (char)genesis_payload[5];
    genesis_payload[5] = (uint8_t)'2';
    CHECK_STATUS(crypto_verify_chain(blocks, 2u, &report),
                 CRYPTO_ERR_PAYLOAD_HASH_MISMATCH);
    CHECK(report.block_position == 0u);
    genesis_payload[5] = (uint8_t)saved_character;

    CHECK_STATUS(crypto_verify_chain(NULL, 1u, &report),
                 CRYPTO_ERR_ARGUMENT);
    CHECK_STATUS(crypto_verify_chain(blocks, 0u, &report),
                 CRYPTO_ERR_CHAIN_EMPTY);
    CHECK(report.block_position == SIZE_MAX);
}

static void test_invalid_proof_and_search_limits(void)
{
    static const uint8_t payload[] = "{\"n\":0}";
    crypto_header_fields_t fields;
    crypto_pow_result_t result;
    crypto_block_t block;
    uint8_t digest[CRYPTO_SHA256_BYTES];
    uint64_t budget = 0u;
    uint64_t nonce;
    bool meets = true;

    (void)memset(&fields, 0, sizeof(fields));
    fields.version = CRYPTO_HEADER_VERSION;
    fields.timestamp_ms = INT64_C(1);
    fields.action_id = 1u;
    fields.difficulty = 3u;
    CHECK_STATUS(crypto_payload_sha256(payload, sizeof(payload) - 1u,
                                       fields.payload_hash),
                 CRYPTO_OK);

    CHECK_STATUS(crypto_default_attempt_budget(3u, &budget), CRYPTO_OK);
    (void)memset(&result, 0xa5, sizeof(result));
    CHECK_STATUS(crypto_mine_header(&fields, 0u, 1u, &result),
                 CRYPTO_ERR_NO_WORK);
    CHECK(result.nonce == 0u && result.attempts == 0u);
    CHECK(all_zero(result.hash, sizeof(result.hash)));

    CHECK_STATUS(crypto_mine_header(&fields, UINT64_MAX, 2u, &result),
                 CRYPTO_ERR_NONCE_OVERFLOW);
    CHECK_STATUS(crypto_mine_header(&fields, 0u, budget + 1u, &result),
                 CRYPTO_ERR_ATTEMPT_BUDGET);

    (void)memset(&block, 0, sizeof(block));
    block.header = fields;
    block.payload_canon = payload;
    block.payload_canon_len = sizeof(payload) - 1u;
    for (nonce = 0u; nonce < budget; ++nonce) {
        block.header.nonce = nonce;
        CHECK_STATUS(crypto_header_sha256(&block.header, digest), CRYPTO_OK);
        CHECK_STATUS(crypto_hash_meets_difficulty(digest, 3u, &meets),
                     CRYPTO_OK);
        if (!meets) {
            break;
        }
    }
    CHECK(nonce < budget);
    block.header.nonce = nonce;
    (void)memcpy(block.hash, digest, sizeof(block.hash));
    CHECK_STATUS(crypto_verify_block(&block), CRYPTO_ERR_POW_INVALID);
}

static void test_header_validation_and_status_text(void)
{
    crypto_header_fields_t fields;
    uint8_t encoded[CRYPTO_BLOCK_HEADER_BYTES];
    uint8_t digest[CRYPTO_SHA256_BYTES];

    (void)memset(&fields, 0, sizeof(fields));
    fields.version = 2u;
    fields.difficulty = 3u;
    (void)memset(encoded, 0xa5, sizeof(encoded));
    CHECK_STATUS(crypto_header_encode(&fields, encoded),
                 CRYPTO_ERR_HEADER_VERSION);
    CHECK(all_zero(encoded, sizeof(encoded)));

    fields.version = CRYPTO_HEADER_VERSION;
    fields.timestamp_ms = -1;
    CHECK_STATUS(crypto_header_encode(&fields, encoded),
                 CRYPTO_ERR_TIMESTAMP);
    fields.timestamp_ms = 0;
    fields.difficulty = 2u;
    CHECK_STATUS(crypto_header_sha256(&fields, digest),
                 CRYPTO_ERR_DIFFICULTY);
    CHECK(all_zero(digest, sizeof(digest)));
    CHECK_STATUS(crypto_header_encode(NULL, encoded), CRYPTO_ERR_ARGUMENT);
    CHECK(all_zero(encoded, sizeof(encoded)));
    CHECK(strcmp(crypto_status_string(CRYPTO_OK), "success") == 0);
    CHECK(strcmp(crypto_status_string((crypto_status_t)-999),
                 "unknown cryptographic status") == 0);
}

int main(void)
{
    test_sha256_vectors();
    test_payload_hash_bounds();
    test_difficulty_policy();
    test_proof_of_work_nibbles();
    test_golden_header_vector();
    test_mining_and_block_verification();
    test_chain_verification_and_tampering();
    test_invalid_proof_and_search_limits();
    test_header_validation_and_status_text();

    if (checks_failed != 0u) {
        (void)fprintf(stderr, "%u/%u checks failed\n", checks_failed,
                      checks_run);
        return EXIT_FAILURE;
    }
    (void)printf("OK: %u crypto checks passed\n", checks_run);
    return EXIT_SUCCESS;
}
