/*
 * Copyright (c) AnimeTCG contributors.
 * SPDX-License-Identifier: MIT
 *
 * Bounded, asynchronous C logger for the AnimeTCG server.
 */
#ifndef ANIMETCG_UTIL_LOG_H
#define ANIMETCG_UTIL_LOG_H

#include "log_events.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ANIMETCG_LOG_QUEUE_CAPACITY 4096u
#define ANIMETCG_LOG_LINE_MAX_BYTES 512u
#define ANIMETCG_LOG_MAX_FIELDS 8u
#define ANIMETCG_LOG_ROTATE_BYTES (10u * 1024u * 1024u)
#define ANIMETCG_LOG_ROTATE_FILES 5u /* Active file included. */
#define ANIMETCG_LOG_NO_CLIENT_ID UINT32_MAX

typedef enum {
    ANIMETCG_LOG_OK = 0,
    ANIMETCG_LOG_FILTERED = 1,
    ANIMETCG_LOG_DROPPED = 2,
    ANIMETCG_LOG_ERR_ARGUMENT = -1,
    ANIMETCG_LOG_ERR_NOT_RUNNING = -2,
    ANIMETCG_LOG_ERR_STATE = -3,
    ANIMETCG_LOG_ERR_IO = -4,
    ANIMETCG_LOG_ERR_TIMEOUT = -5,
    ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL = -6,
    ANIMETCG_LOG_ERR_PRIVACY_POLICY = -7
} animetcg_log_status_t;

typedef struct {
    const char *directory; /* Created with mode 0750 if it does not exist. */
    const char *filename;  /* A basename, e.g. "server.log". */
    animetcg_log_level_t minimum_level;
} animetcg_log_config_t;

typedef enum {
    ANIMETCG_LOG_KEY_ACTION = 0,
    ANIMETCG_LOG_KEY_STATUS,
    ANIMETCG_LOG_KEY_CODE,
    ANIMETCG_LOG_KEY_FIELD,
    ANIMETCG_LOG_KEY_REASON,
    ANIMETCG_LOG_KEY_EVENT,
    ANIMETCG_LOG_KEY_USER_ID,
    ANIMETCG_LOG_KEY_CARD_ID,
    ANIMETCG_LOG_KEY_OTHER_CARD_ID,
    ANIMETCG_LOG_KEY_WINNER_CARD_ID,
    ANIMETCG_LOG_KEY_BATTLE_ID,
    ANIMETCG_LOG_KEY_CLAIM_ID,
    ANIMETCG_LOG_KEY_LEGITIMATION_ID,
    ANIMETCG_LOG_KEY_AUTHENTICATION_ID,
    ANIMETCG_LOG_KEY_BLOCK_INDEX,
    ANIMETCG_LOG_KEY_HASH_HEAD,
    ANIMETCG_LOG_KEY_DIFFICULTY,
    ANIMETCG_LOG_KEY_DIFFICULTY_BEFORE,
    ANIMETCG_LOG_KEY_DIFFICULTY_AFTER,
    ANIMETCG_LOG_KEY_NONCE,
    ANIMETCG_LOG_KEY_HASHES,
    ANIMETCG_LOG_KEY_HASH_RATE,
    ANIMETCG_LOG_KEY_MS,
    ANIMETCG_LOG_KEY_BYTES,
    ANIMETCG_LOG_KEY_PAYLOAD_BYTES,
    ANIMETCG_LOG_KEY_TITLE_LEN,
    ANIMETCG_LOG_KEY_SYNOPSIS_LEN,
    ANIMETCG_LOG_KEY_JUSTIFICATION_LEN,
    ANIMETCG_LOG_KEY_CARDS,
    ANIMETCG_LOG_KEY_BLOCKS,
    ANIMETCG_LOG_KEY_VOTES,
    ANIMETCG_LOG_KEY_APPROVALS,
    ANIMETCG_LOG_KEY_REJECTIONS,
    ANIMETCG_LOG_KEY_RECIPIENTS,
    ANIMETCG_LOG_KEY_CHECKED,
    ANIMETCG_LOG_KEY_OP,
    ANIMETCG_LOG_KEY_PATTERN,
    ANIMETCG_LOG_KEY_POSITION,
    ANIMETCG_LOG_KEY_DEPTH,
    ANIMETCG_LOG_KEY_QUEUE_DEPTH,
    ANIMETCG_LOG_KEY_IN_FLIGHT,
    ANIMETCG_LOG_KEY_BACKLOG,
    ANIMETCG_LOG_KEY_DROPPED,
    ANIMETCG_LOG_KEY_EXTRA,
    ANIMETCG_LOG_KEY_ATTEMPT,
    ANIMETCG_LOG_KEY_DAYS,
    ANIMETCG_LOG_KEY_VALUE_BEFORE,
    ANIMETCG_LOG_KEY_VALUE_AFTER,
    ANIMETCG_LOG_KEY_CAUSE,
    ANIMETCG_LOG_KEY_OUTCOME,
    ANIMETCG_LOG_KEY_VERSION,
    ANIMETCG_LOG_KEY_IP,
    ANIMETCG_LOG_KEY_PORT,
    ANIMETCG_LOG_KEY_COUNT
} animetcg_log_key_t;

typedef enum {
    ANIMETCG_LOG_VALUE_TEXT = 0,
    ANIMETCG_LOG_VALUE_I64,
    ANIMETCG_LOG_VALUE_U64,
    ANIMETCG_LOG_VALUE_DECIMAL_1
} animetcg_log_value_type_t;

typedef struct {
    animetcg_log_key_t key;
    animetcg_log_value_type_t type;
    union {
        struct {
            const unsigned char *data;
            size_t length;
        } text;
        int64_t i64;
        uint64_t u64;
        int64_t decimal_1_tenths;
    } value;
} animetcg_log_field_t;

typedef struct {
    uint64_t accepted;
    uint64_t written;
    uint64_t dropped;
    uint64_t pending_dropped;
    uint64_t io_errors;
    uint64_t invalid_calls;
    uint64_t rotations;
    size_t queue_depth;
    size_t queue_high_water;
    size_t in_flight;
} animetcg_log_stats_t;

/* Initialize exactly once per process; no logging call allocates memory. */
animetcg_log_status_t animetcg_log_init(const animetcg_log_config_t *config);

/* Drain queued records, sync and close the output, then release the queue. */
animetcg_log_status_t animetcg_log_shutdown(void);

/* Change the threshold at runtime; OFF suppresses every event. */
animetcg_log_status_t animetcg_log_set_level(animetcg_log_level_t level);
animetcg_log_level_t animetcg_log_get_level(void);
bool animetcg_log_is_enabled(animetcg_log_event_t event);

/* Context is thread-local and copied into each queued record. request_id is
 * NULL for no request, or exactly 12 lowercase hexadecimal characters. */
animetcg_log_status_t animetcg_log_set_context(uint32_t client_id,
                                               const char *request_id);
void animetcg_log_clear_context(void);

/*
 * Enqueue one catalogue event. This function never waits for disk I/O and
 * never blocks on the queue mutex: contention or a full queue drops the record
 * and increments the observable loss counters.
 */
animetcg_log_status_t animetcg_log_emit(animetcg_log_event_t event,
                                         const animetcg_log_field_t *fields,
                                         size_t field_count);

/* Wait for the queue and current write to drain; do not call on an event loop. */
animetcg_log_status_t animetcg_log_flush(uint32_t timeout_ms);
void animetcg_log_get_stats(animetcg_log_stats_t *out_stats);

/* Escape controls, line breaks and separators without splitting UTF-8. */
animetcg_log_status_t animetcg_log_sanitize(const unsigned char *input,
                                             size_t input_length,
                                             char *output,
                                             size_t output_capacity,
                                             size_t max_characters,
                                             size_t *output_length);

/* Typed field constructors. _n accepts a byte slice and is safe for network
 * data; the non-_n constructor expects a NUL-terminated string. */
animetcg_log_field_t animetcg_log_text(animetcg_log_key_t key,
                                        const char *value);
animetcg_log_field_t animetcg_log_text_n(animetcg_log_key_t key,
                                          const void *value,
                                          size_t length);
animetcg_log_field_t animetcg_log_i64(animetcg_log_key_t key, int64_t value);
animetcg_log_field_t animetcg_log_u64(animetcg_log_key_t key, uint64_t value);
animetcg_log_field_t animetcg_log_decimal_1(animetcg_log_key_t key,
                                             int64_t tenths);

#ifdef ANIMETCG_LOG_TESTING
/* Test-only fault injection. Not present in production builds. */
animetcg_log_status_t animetcg_log_test_pause_writer(bool pause);
animetcg_log_status_t animetcg_log_test_fail_next_write(size_t bytes_before_failure);
#endif

#ifdef __cplusplus
}
#endif

#endif
