/*
 * Copyright (c) AnimeTCG contributors.
 * SPDX-License-Identifier: MIT
 *
 * A single-writer, bounded asynchronous logger. Producers build a bounded
 * record on their stack and use trylock() to enqueue it; disk I/O, rotation,
 * timestamps and loss summaries are owned by the writer thread.
 */
#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOG_DIRECTORY_MAX 768u
#define LOG_FILENAME_MAX 127u
#define LOG_PATH_MAX 1024u
#define LOG_FIELD_TEXT_MAX 64u
#define LOG_FIELD_BUFFER_BYTES 512u
#define LOG_SANITIZED_BUFFER_BYTES (LOG_FIELD_TEXT_MAX * 6u + 2u)
#define LOG_TIMESTAMP_BYTES 24u
#define LOG_CONTENT_RESERVE_BYTES 2u /* Optional '~' plus final LF. */
#define LOG_SUMMARY_INTERVAL_NS UINT64_C(5000000000)

typedef struct {
    uint16_t length;
    char bytes[ANIMETCG_LOG_LINE_MAX_BYTES];
} log_record_t;

_Static_assert(sizeof(log_record_t) == ANIMETCG_LOG_LINE_MAX_BYTES + sizeof(uint16_t),
               "queue record layout changed; review the memory bound");

_Static_assert(ANIMETCG_LOG_QUEUE_CAPACITY <= UINT16_MAX,
               "queue indices must fit in uint16_t");

typedef struct {
    const char *name;
    unsigned int allowed_types;
} log_key_info_t;

typedef struct {
    char *data;
    size_t length;
    size_t content_limit;
    bool truncated;
} line_builder_t;

typedef struct {
    uint32_t client_id;
    bool has_client;
    char request_id[13];
} log_context_t;

typedef enum {
    LOGGER_UNINITIALIZED = 0,
    LOGGER_RUNNING,
    LOGGER_STOPPING,
    LOGGER_CLOSED
} logger_state_t;

#define TYPE_MASK(type_) (1u << (unsigned int)(type_))
#define TEXT_ONLY TYPE_MASK(ANIMETCG_LOG_VALUE_TEXT)
#define INTEGER_TYPES (TYPE_MASK(ANIMETCG_LOG_VALUE_I64) | \
                      TYPE_MASK(ANIMETCG_LOG_VALUE_U64))
#define U64_ONLY TYPE_MASK(ANIMETCG_LOG_VALUE_U64)
#define DECIMAL_ONLY TYPE_MASK(ANIMETCG_LOG_VALUE_DECIMAL_1)

static const log_key_info_t LOG_KEYS[ANIMETCG_LOG_KEY_COUNT] = {
    [ANIMETCG_LOG_KEY_ACTION] = {"action", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_STATUS] = {"status", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_CODE] = {"code", TEXT_ONLY | INTEGER_TYPES},
    [ANIMETCG_LOG_KEY_FIELD] = {"field", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_REASON] = {"reason", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_EVENT] = {"event", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_USER_ID] = {"user_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_CARD_ID] = {"card_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_OTHER_CARD_ID] = {"other_card_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_WINNER_CARD_ID] = {"winner_card_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_BATTLE_ID] = {"battle_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_CLAIM_ID] = {"claim_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_LEGITIMATION_ID] = {"legitimation_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_AUTHENTICATION_ID] = {"authentication_id", U64_ONLY},
    [ANIMETCG_LOG_KEY_BLOCK_INDEX] = {"block_index", U64_ONLY},
    [ANIMETCG_LOG_KEY_HASH_HEAD] = {"hash_head", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_DIFFICULTY] = {"difficulty", U64_ONLY},
    [ANIMETCG_LOG_KEY_DIFFICULTY_BEFORE] = {"difficulty_before", U64_ONLY},
    [ANIMETCG_LOG_KEY_DIFFICULTY_AFTER] = {"difficulty_after", U64_ONLY},
    [ANIMETCG_LOG_KEY_NONCE] = {"nonce", U64_ONLY},
    [ANIMETCG_LOG_KEY_HASHES] = {"hashes", U64_ONLY},
    [ANIMETCG_LOG_KEY_HASH_RATE] = {"hash_rate", U64_ONLY},
    [ANIMETCG_LOG_KEY_MS] = {"ms", U64_ONLY},
    [ANIMETCG_LOG_KEY_BYTES] = {"bytes", U64_ONLY},
    [ANIMETCG_LOG_KEY_PAYLOAD_BYTES] = {"payload_bytes", U64_ONLY},
    [ANIMETCG_LOG_KEY_TITLE_LEN] = {"title_len", U64_ONLY},
    [ANIMETCG_LOG_KEY_SYNOPSIS_LEN] = {"synopsis_len", U64_ONLY},
    [ANIMETCG_LOG_KEY_JUSTIFICATION_LEN] = {"justification_len", U64_ONLY},
    [ANIMETCG_LOG_KEY_CARDS] = {"cards", U64_ONLY},
    [ANIMETCG_LOG_KEY_BLOCKS] = {"blocks", U64_ONLY},
    [ANIMETCG_LOG_KEY_VOTES] = {"votes", U64_ONLY},
    [ANIMETCG_LOG_KEY_APPROVALS] = {"approvals", U64_ONLY},
    [ANIMETCG_LOG_KEY_REJECTIONS] = {"rejections", U64_ONLY},
    [ANIMETCG_LOG_KEY_RECIPIENTS] = {"recipients", U64_ONLY},
    [ANIMETCG_LOG_KEY_CHECKED] = {"checked", U64_ONLY},
    [ANIMETCG_LOG_KEY_OP] = {"op", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_PATTERN] = {"pattern", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_POSITION] = {"position", U64_ONLY},
    [ANIMETCG_LOG_KEY_DEPTH] = {"depth", U64_ONLY},
    [ANIMETCG_LOG_KEY_QUEUE_DEPTH] = {"queue_depth", U64_ONLY},
    [ANIMETCG_LOG_KEY_IN_FLIGHT] = {"in_flight", U64_ONLY},
    [ANIMETCG_LOG_KEY_BACKLOG] = {"backlog", U64_ONLY},
    [ANIMETCG_LOG_KEY_DROPPED] = {"dropped", U64_ONLY},
    [ANIMETCG_LOG_KEY_EXTRA] = {"extra", U64_ONLY},
    [ANIMETCG_LOG_KEY_ATTEMPT] = {"attempt", U64_ONLY},
    [ANIMETCG_LOG_KEY_DAYS] = {"days", U64_ONLY},
    [ANIMETCG_LOG_KEY_VALUE_BEFORE] = {"value_before", DECIMAL_ONLY},
    [ANIMETCG_LOG_KEY_VALUE_AFTER] = {"value_after", DECIMAL_ONLY},
    [ANIMETCG_LOG_KEY_CAUSE] = {"cause", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_OUTCOME] = {"outcome", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_VERSION] = {"version", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_IP] = {"ip", TEXT_ONLY},
    [ANIMETCG_LOG_KEY_PORT] = {"port", U64_ONLY}
};

static pthread_mutex_t LIFECYCLE_MUTEX = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t QUEUE_MUTEX = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t QUEUE_CONDITION;
static bool QUEUE_CONDITION_INITIALIZED;
static pthread_t WRITER_THREAD;
static bool WRITER_STARTED;
static bool WRITER_STOP;
static bool WRITER_PAUSED;
static bool FORCE_DROP_SUMMARY;
static log_record_t *QUEUE;
static size_t QUEUE_HEAD;
static size_t QUEUE_TAIL;
static size_t QUEUE_COUNT;
static size_t WRITER_IN_FLIGHT;
static int LOG_FD = -1;
static uint64_t LOG_FILE_BYTES;
static char LOG_PATH[LOG_PATH_MAX];
static _Atomic int LOGGER_STATE = LOGGER_UNINITIALIZED;
static _Atomic int LOGGER_LEVEL = ANIMETCG_LOG_LEVEL_INFO;
static _Atomic size_t ACTIVE_PRODUCERS = 0;
static _Atomic uint64_t STAT_ACCEPTED = 0;
static _Atomic uint64_t STAT_WRITTEN = 0;
static _Atomic uint64_t STAT_DROPPED = 0;
static _Atomic uint64_t STAT_PENDING_DROPPED = 0;
static _Atomic uint64_t STAT_IO_ERRORS = 0;
static _Atomic uint64_t STAT_INVALID_CALLS = 0;
static _Atomic uint64_t STAT_ROTATIONS = 0;
static _Atomic size_t STAT_QUEUE_DEPTH = 0;
static _Atomic size_t STAT_QUEUE_HIGH_WATER = 0;
static _Atomic size_t STAT_IN_FLIGHT = 0;
static _Atomic bool EMERGENCY_REPORTED = false;
#ifdef ANIMETCG_LOG_TESTING
static _Atomic size_t TEST_FAIL_AFTER_BYTES = SIZE_MAX;
#endif
static int64_t LAST_WRITTEN_TIMESTAMP_MS = INT64_MIN;
static _Thread_local log_context_t THREAD_CONTEXT = {
    .client_id = ANIMETCG_LOG_NO_CLIENT_ID,
    .has_client = false,
    .request_id = "-"
};

static void write_emergency_message(const char *message, size_t length)
{
    size_t offset = 0u;

    while (offset < length) {
        ssize_t written = write(STDERR_FILENO, message + offset, length - offset);
        if (written > 0) {
            offset += (size_t)written;
        } else if (written < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
}

static void report_output_failure_once(void)
{
    static const char message[] = "animetcg logger: file output or rotation failed\n";
    bool already_reported = atomic_exchange_explicit(&EMERGENCY_REPORTED, true,
                                                      memory_order_relaxed);
    if (!already_reported) {
        write_emergency_message(message, sizeof(message) - 1u);
    }
}

static void report_init_failure(void)
{
    static const char message[] = "animetcg logger: initialization failed\n";
    write_emergency_message(message, sizeof(message) - 1u);
}

static bool level_is_valid(animetcg_log_level_t level)
{
    return level >= ANIMETCG_LOG_LEVEL_TRACE && level <= ANIMETCG_LOG_LEVEL_OFF;
}

static const char *level_name(animetcg_log_level_t level)
{
    static const char *const names[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"
    };
    if (!level_is_valid(level)) {
        return "INFO";
    }
    return names[(size_t)level];
}

static bool parse_level(const char *text, animetcg_log_level_t *out_level)
{
    static const struct {
        const char *text;
        animetcg_log_level_t level;
    } names[] = {
        {"TRACE", ANIMETCG_LOG_LEVEL_TRACE},
        {"DEBUG", ANIMETCG_LOG_LEVEL_DEBUG},
        {"INFO", ANIMETCG_LOG_LEVEL_INFO},
        {"WARN", ANIMETCG_LOG_LEVEL_WARN},
        {"ERROR", ANIMETCG_LOG_LEVEL_ERROR},
        {"FATAL", ANIMETCG_LOG_LEVEL_FATAL},
        {"OFF", ANIMETCG_LOG_LEVEL_OFF}
    };
    size_t index;

    if (text == NULL || out_level == NULL) {
        return false;
    }
    for (index = 0u; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (strcmp(text, names[index].text) == 0) {
            *out_level = names[index].level;
            return true;
        }
    }
    return false;
}

static bool request_id_is_valid(const char *request_id)
{
    size_t index;
    if (request_id == NULL) {
        return true;
    }
    if (strnlen(request_id, 13u) != 12u) {
        return false;
    }
    for (index = 0u; index < 12u; ++index) {
        char ch = request_id[index];
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) {
            return false;
        }
    }
    return true;
}

static bool safe_filename(const char *filename)
{
    size_t index;
    size_t length;

    if (filename == NULL || filename[0] == '\0') {
        return false;
    }
    length = strnlen(filename, LOG_FILENAME_MAX + 1u);
    if (length == 0u || length > LOG_FILENAME_MAX ||
        strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        return false;
    }
    for (index = 0u; index < length; ++index) {
        char ch = filename[index];
        bool allowed = (ch >= 'A' && ch <= 'Z') ||
                       (ch >= 'a' && ch <= 'z') ||
                       (ch >= '0' && ch <= '9') ||
                       ch == '.' || ch == '_' || ch == '-';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

static int build_log_path(const char *directory, const char *filename,
                          char *out_path, size_t out_capacity)
{
    int length;
    if (directory == NULL || directory[0] == '\0' || out_path == NULL ||
        out_capacity == 0u || !safe_filename(filename) ||
        strnlen(directory, LOG_DIRECTORY_MAX + 1u) > LOG_DIRECTORY_MAX) {
        return -1;
    }
    length = snprintf(out_path, out_capacity, "%s/%s", directory, filename);
    if (length < 0 || (size_t)length >= out_capacity) {
        return -1;
    }
    return 0;
}

static int ensure_log_directory(const char *directory)
{
    struct stat info;
    if (mkdir(directory, (mode_t)0750) != 0 && errno != EEXIST) {
        return -1;
    }
    if (stat(directory, &info) != 0 || !S_ISDIR(info.st_mode)) {
        return -1;
    }
    if (chmod(directory, (mode_t)0750) != 0) {
        return -1;
    }
    return 0;
}

static int repair_incomplete_tail(int descriptor, off_t file_size,
                                  off_t *out_file_size)
{
    unsigned char buffer[4096];
    unsigned char last_byte;
    ssize_t last_result;
    off_t position;

    if (descriptor < 0 || file_size < (off_t)0 || out_file_size == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (file_size == (off_t)0) {
        *out_file_size = (off_t)0;
        return 0;
    }

    do {
        last_result = pread(descriptor, &last_byte, 1u, file_size - (off_t)1);
    } while (last_result < 0 && errno == EINTR);
    if (last_result != 1) {
        if (last_result == 0) {
            errno = EIO;
        }
        return -1;
    }
    if (last_byte == (unsigned char)'\n') {
        *out_file_size = file_size;
        return 0;
    }

    /* A crash/short write can leave only the final record incomplete. */
    position = file_size - (off_t)1;
    while (position > (off_t)0) {
        off_t block_length = position < (off_t)sizeof(buffer) ?
                             position : (off_t)sizeof(buffer);
        off_t block_start = position - block_length;
        size_t requested = (size_t)block_length;
        size_t received = 0u;

        while (received < requested) {
            ssize_t amount = pread(descriptor, buffer + received,
                                   requested - received,
                                   block_start + (off_t)received);
            if (amount > 0) {
                received += (size_t)amount;
            } else if (amount < 0 && errno == EINTR) {
                continue;
            } else {
                if (amount == 0) {
                    errno = EIO;
                }
                return -1;
            }
        }

        while (received > 0u) {
            if (buffer[received - 1u] == (unsigned char)'\n') {
                off_t repaired_size = block_start + (off_t)received;
                if (ftruncate(descriptor, repaired_size) != 0) {
                    return -1;
                }
                *out_file_size = repaired_size;
                return 0;
            }
            --received;
        }
        position = block_start;
    }

    /* No complete records exist in the file; discard its invalid tail. */
    if (ftruncate(descriptor, (off_t)0) != 0) {
        return -1;
    }
    *out_file_size = (off_t)0;
    return 0;
}

static int open_active_file(void)
{
    int flags = O_RDWR | O_APPEND | O_CREAT;
    struct stat info;
    off_t repaired_size;
    int descriptor;

#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    descriptor = open(LOG_PATH, flags, (mode_t)0640);
    if (descriptor < 0) {
        return -1;
    }
    if (fchmod(descriptor, (mode_t)0640) != 0) {
        int saved_errno = errno;
        (void)close(descriptor);
        errno = saved_errno;
        return -1;
    }
    if (fstat(descriptor, &info) != 0) {
        int saved_errno = errno;
        (void)close(descriptor);
        errno = saved_errno;
        return -1;
    }
    if (!S_ISREG(info.st_mode) || info.st_size < (off_t)0) {
        (void)close(descriptor);
        errno = EINVAL;
        return -1;
    }
    if (repair_incomplete_tail(descriptor, info.st_size, &repaired_size) != 0) {
        int saved_errno = errno;
        (void)close(descriptor);
        errno = saved_errno;
        return -1;
    }
    LOG_FD = descriptor;
    LOG_FILE_BYTES = (uint64_t)repaired_size;
    return 0;
}

static int build_backup_path(unsigned int suffix, char *out_path,
                             size_t out_capacity)
{
    int length;
    if (suffix == 0u) {
        return -1;
    }
    length = snprintf(out_path, out_capacity, "%s.%u", LOG_PATH, suffix);
    if (length < 0 || (size_t)length >= out_capacity) {
        return -1;
    }
    return 0;
}

static bool append_token(line_builder_t *builder, const char *text, size_t length)
{
    if (builder->truncated) {
        return false;
    }
    if (length > builder->content_limit - builder->length) {
        builder->truncated = true;
        return false;
    }
    memcpy(builder->data + builder->length, text, length);
    builder->length += length;
    return true;
}

static bool append_literal(line_builder_t *builder, const char *text)
{
    return append_token(builder, text, strlen(text));
}

static bool append_u32(line_builder_t *builder, uint32_t value)
{
    char number[16];
    int length = snprintf(number, sizeof(number), "%" PRIu32, value);
    if (length < 0 || (size_t)length >= sizeof(number)) {
        builder->truncated = true;
        return false;
    }
    return append_token(builder, number, (size_t)length);
}

static bool is_unquoted_character(unsigned char ch)
{
    return (ch >= (unsigned char)'A' && ch <= (unsigned char)'Z') ||
           (ch >= (unsigned char)'a' && ch <= (unsigned char)'z') ||
           (ch >= (unsigned char)'0' && ch <= (unsigned char)'9') ||
           ch == (unsigned char)'.' || ch == (unsigned char)'_' ||
           ch == (unsigned char)'-' || ch == (unsigned char)':' ||
           ch == (unsigned char)'/' || ch == (unsigned char)'@' ||
           ch == (unsigned char)'+';
}

/* Returns the valid UTF-8 sequence length, or one byte for an invalid octet. */
static size_t utf8_sequence_length(const unsigned char *text, size_t remaining)
{
    unsigned char first;
    unsigned char second;
    unsigned char third;
    unsigned char fourth;

    if (remaining == 0u) {
        return 0u;
    }
    first = text[0];
    if (first <= 0x7fu) {
        return 1u;
    }
    if (first >= 0xc2u && first <= 0xdfu && remaining >= 2u) {
        second = text[1];
        return (second >= 0x80u && second <= 0xbfu) ? 2u : 1u;
    }
    if (first >= 0xe0u && first <= 0xefu && remaining >= 3u) {
        second = text[1];
        third = text[2];
        if (third < 0x80u || third > 0xbfu) {
            return 1u;
        }
        if (first == 0xe0u) {
            return (second >= 0xa0u && second <= 0xbfu) ? 3u : 1u;
        }
        if (first == 0xedu) {
            return (second >= 0x80u && second <= 0x9fu) ? 3u : 1u;
        }
        return (second >= 0x80u && second <= 0xbfu) ? 3u : 1u;
    }
    if (first >= 0xf0u && first <= 0xf4u && remaining >= 4u) {
        second = text[1];
        third = text[2];
        fourth = text[3];
        if (third < 0x80u || third > 0xbfu ||
            fourth < 0x80u || fourth > 0xbfu) {
            return 1u;
        }
        if (first == 0xf0u) {
            return (second >= 0x90u && second <= 0xbfu) ? 4u : 1u;
        }
        if (first == 0xf4u) {
            return (second >= 0x80u && second <= 0x8fu) ? 4u : 1u;
        }
        return (second >= 0x80u && second <= 0xbfu) ? 4u : 1u;
    }
    return 1u;
}

static animetcg_log_status_t sanitize_internal(const unsigned char *input,
                                               size_t input_length,
                                               char *output,
                                               size_t output_capacity,
                                               size_t max_characters,
                                               size_t *output_length,
                                               bool *must_quote)
{
    static const char HEX[] = "0123456789ABCDEF";
    size_t input_offset = 0u;
    size_t output_offset = 0u;
    size_t characters = 0u;
    bool quote = false;

    if ((input == NULL && input_length != 0u) || output == NULL ||
        output_capacity == 0u || output_length == NULL || must_quote == NULL) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }

    while (input_offset < input_length && characters < max_characters) {
        unsigned char current = input[input_offset];
        size_t sequence_length = utf8_sequence_length(input + input_offset,
                                                       input_length - input_offset);
        char token[6];
        size_t token_length = 0u;

        if (sequence_length == 0u) {
            break;
        }
        if (sequence_length == 1u && current >= 0x80u) {
            token[0] = '?';
            token_length = 1u;
            quote = true;
        } else if (sequence_length == 1u && current == (unsigned char)'\n') {
            token[0] = '\\'; token[1] = 'n'; token_length = 2u; quote = true;
        } else if (sequence_length == 1u && current == (unsigned char)'\r') {
            token[0] = '\\'; token[1] = 'r'; token_length = 2u; quote = true;
        } else if (sequence_length == 1u && current == (unsigned char)'\t') {
            token[0] = '\\'; token[1] = 't'; token_length = 2u; quote = true;
        } else if (sequence_length == 1u && current == (unsigned char)'|') {
            memcpy(token, "\\u007C", 6u); token_length = 6u; quote = true;
        } else if (sequence_length == 1u &&
                   (current < 0x20u || current == 0x7fu)) {
            token[0] = '\\'; token[1] = 'x';
            token[2] = HEX[(current >> 4u) & 0x0fu];
            token[3] = HEX[current & 0x0fu];
            token_length = 4u; quote = true;
        } else if (sequence_length == 1u && current == (unsigned char)'"') {
            token[0] = '\\'; token[1] = '"'; token_length = 2u; quote = true;
        } else if (sequence_length == 1u && current == (unsigned char)'\\') {
            token[0] = '\\'; token[1] = '\\'; token_length = 2u; quote = true;
        } else {
            if (sequence_length == 1u && !is_unquoted_character(current)) {
                quote = true;
            } else if (sequence_length > 1u) {
                quote = true;
            }
            memcpy(token, input + input_offset, sequence_length);
            token_length = sequence_length;
        }

        if (output_offset >= output_capacity ||
            token_length > output_capacity - output_offset - 1u) {
            if (output_offset < output_capacity) {
                output[output_offset] = '\0';
            }
            *output_length = output_offset;
            *must_quote = quote;
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
        memcpy(output + output_offset, token, token_length);
        output_offset += token_length;
        input_offset += sequence_length;
        ++characters;
    }

    if (input_offset < input_length) {
        if (output_offset >= output_capacity - 1u) {
            if (output_offset < output_capacity) {
                output[output_offset] = '\0';
            }
            *output_length = output_offset;
            *must_quote = true;
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
        output[output_offset++] = '~';
        quote = true;
    }
    output[output_offset] = '\0';
    *output_length = output_offset;
    *must_quote = quote;
    return ANIMETCG_LOG_OK;
}

animetcg_log_status_t animetcg_log_sanitize(const unsigned char *input,
                                             size_t input_length,
                                             char *output,
                                             size_t output_capacity,
                                             size_t max_characters,
                                             size_t *output_length)
{
    bool ignored_quote = false;
    return sanitize_internal(input, input_length, output, output_capacity,
                             max_characters, output_length, &ignored_quote);
}

static bool key_is_valid(animetcg_log_key_t key)
{
    return (int)key >= 0 && (size_t)key < (size_t)ANIMETCG_LOG_KEY_COUNT &&
           LOG_KEYS[(size_t)key].name != NULL;
}

static unsigned int value_type_bit(animetcg_log_value_type_t type)
{
    if ((int)type < 0 || type > ANIMETCG_LOG_VALUE_DECIMAL_1) {
        return 0u;
    }
    return TYPE_MASK(type);
}

static animetcg_log_status_t format_decimal_1(int64_t tenths,
                                               char *output,
                                               size_t output_capacity,
                                               size_t *output_length)
{
    uint64_t magnitude;
    uint64_t whole;
    uint64_t fraction;
    int length;
    bool negative = tenths < 0;

    if (negative) {
        magnitude = (uint64_t)(-(tenths + 1)) + UINT64_C(1);
    } else {
        magnitude = (uint64_t)tenths;
    }
    whole = magnitude / UINT64_C(10);
    fraction = magnitude % UINT64_C(10);
    length = snprintf(output, output_capacity, "%s%" PRIu64 ".%" PRIu64,
                      negative ? "-" : "", whole, fraction);
    if (length < 0 || (size_t)length >= output_capacity) {
        return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
    }
    *output_length = (size_t)length;
    return ANIMETCG_LOG_OK;
}

static animetcg_log_status_t format_field(const animetcg_log_field_t *field,
                                          animetcg_log_level_t event_level,
                                          char *output,
                                          size_t output_capacity,
                                          size_t *output_length)
{
    char value[LOG_FIELD_BUFFER_BYTES];
    size_t value_length = 0u;
    size_t key_length;
    unsigned int type_bit;
    bool quote = false;
    int length;
    animetcg_log_status_t status;

    if (field == NULL || output == NULL || output_length == NULL ||
        !key_is_valid(field->key) || field->key == ANIMETCG_LOG_KEY_EXTRA) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    type_bit = value_type_bit(field->type);
    if (type_bit == 0u ||
        (LOG_KEYS[(size_t)field->key].allowed_types & type_bit) == 0u) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    if (field->key == ANIMETCG_LOG_KEY_IP &&
        event_level != ANIMETCG_LOG_LEVEL_DEBUG) {
        return ANIMETCG_LOG_ERR_PRIVACY_POLICY;
    }

    switch (field->type) {
    case ANIMETCG_LOG_VALUE_TEXT:
        if (field->value.text.data == NULL && field->value.text.length != 0u) {
            return ANIMETCG_LOG_ERR_ARGUMENT;
        }
        if (field->value.text.length == 0u) {
            value[0] = '-';
            value[1] = '\0';
            value_length = 1u;
        } else {
            status = sanitize_internal(field->value.text.data,
                                       field->value.text.length,
                                       value, sizeof(value), LOG_FIELD_TEXT_MAX,
                                       &value_length, &quote);
            if (status != ANIMETCG_LOG_OK) {
                return status;
            }
            if (quote) {
                char quoted[LOG_FIELD_BUFFER_BYTES];
                if (value_length + 2u > sizeof(quoted)) {
                    return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
                }
                quoted[0] = '"';
                memcpy(quoted + 1u, value, value_length);
                quoted[value_length + 1u] = '"';
                value_length += 2u;
                memcpy(value, quoted, value_length);
                value[value_length] = '\0';
            }
        }
        break;
    case ANIMETCG_LOG_VALUE_I64:
        length = snprintf(value, sizeof(value), "%" PRId64, field->value.i64);
        if (length < 0 || (size_t)length >= sizeof(value)) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
        value_length = (size_t)length;
        break;
    case ANIMETCG_LOG_VALUE_U64:
        length = snprintf(value, sizeof(value), "%" PRIu64, field->value.u64);
        if (length < 0 || (size_t)length >= sizeof(value)) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
        value_length = (size_t)length;
        break;
    case ANIMETCG_LOG_VALUE_DECIMAL_1:
        status = format_decimal_1(field->value.decimal_1_tenths,
                                  value, sizeof(value), &value_length);
        if (status != ANIMETCG_LOG_OK) {
            return status;
        }
        break;
    default:
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }

    key_length = strlen(LOG_KEYS[(size_t)field->key].name);
    if (key_length + 1u + value_length > output_capacity) {
        return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(output, LOG_KEYS[(size_t)field->key].name, key_length);
    output[key_length] = '=';
    memcpy(output + key_length + 1u, value, value_length);
    *output_length = key_length + 1u + value_length;
    return ANIMETCG_LOG_OK;
}

static animetcg_log_status_t build_record(animetcg_log_event_t event,
                                          const log_context_t *context,
                                          const animetcg_log_field_t *fields,
                                          size_t field_count,
                                          log_record_t *out_record)
{
    static const char timestamp_placeholder[] = "0000-00-00T00:00:00.000Z";
    static const char separator[] = " | ";
    const animetcg_log_event_info_t *event_info;
    line_builder_t builder;
    char formatted_fields[ANIMETCG_LOG_MAX_FIELDS][LOG_FIELD_BUFFER_BYTES];
    size_t formatted_lengths[ANIMETCG_LOG_MAX_FIELDS];
    size_t included_fields;
    size_t field_count_to_write;
    size_t index;
    bool line_truncated = false;
    animetcg_log_status_t status;

    if (context == NULL || out_record == NULL ||
        (fields == NULL && field_count != 0u)) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    event_info = animetcg_log_event_get_info(event);
    if (event_info == NULL || event_info->client_only || event_info->deprecated ||
        !level_is_valid(event_info->level) ||
        event_info->level == ANIMETCG_LOG_LEVEL_OFF) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }

    memset(out_record, 0, sizeof(*out_record));
    builder.data = out_record->bytes;
    builder.length = 0u;
    builder.content_limit = ANIMETCG_LOG_LINE_MAX_BYTES - LOG_CONTENT_RESERVE_BYTES;
    builder.truncated = false;

    if (!append_token(&builder, timestamp_placeholder,
                      sizeof(timestamp_placeholder) - 1u) ||
        !append_literal(&builder, separator) ||
        !append_literal(&builder, level_name(event_info->level)) ||
        !append_literal(&builder, separator) ||
        !append_literal(&builder, "srv") ||
        !append_literal(&builder, separator) ||
        !append_literal(&builder, "c=")) {
        return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
    }
    if (context->has_client) {
        if (!append_u32(&builder, context->client_id)) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
    } else if (!append_literal(&builder, "-")) {
        return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
    }
    if (!append_literal(&builder, separator) || !append_literal(&builder, "r=") ||
        !append_literal(&builder, context->request_id) ||
        !append_literal(&builder, separator) ||
        !append_literal(&builder, event_info->module) ||
        !append_literal(&builder, separator) ||
        !append_literal(&builder, event_info->message)) {
        return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
    }

    included_fields = field_count > ANIMETCG_LOG_MAX_FIELDS ?
                      ANIMETCG_LOG_MAX_FIELDS - 1u : field_count;
    for (index = 0u; index < included_fields; ++index) {
        size_t previous;
        for (previous = 0u; previous < index; ++previous) {
            if (fields[previous].key == fields[index].key) {
                return ANIMETCG_LOG_ERR_ARGUMENT;
            }
        }
        if (fields[index].key == ANIMETCG_LOG_KEY_IP &&
            event != ANIMETCG_LOG_EVENT_CLIENT_ADDRESS) {
            return ANIMETCG_LOG_ERR_PRIVACY_POLICY;
        }
        status = format_field(&fields[index], event_info->level,
                              formatted_fields[index],
                              sizeof(formatted_fields[index]),
                              &formatted_lengths[index]);
        if (status != ANIMETCG_LOG_OK) {
            return status;
        }
    }

    field_count_to_write = included_fields;
    for (;;) {
        size_t candidate_length = builder.length;
        size_t omitted_fields = field_count - field_count_to_write;
        size_t extra_length = 0u;
        size_t extra_prefix_length = 0u;
        size_t field_bytes = 0u;
        char extra_field[48];
        int extra_result;

        for (index = 0u; index < field_count_to_write; ++index) {
            size_t prefix_length = index == 0u ? sizeof(separator) - 1u : 1u;
            if (formatted_lengths[index] > SIZE_MAX - field_bytes - prefix_length) {
                return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
            }
            field_bytes += prefix_length + formatted_lengths[index];
        }
        candidate_length += field_bytes;
        if (omitted_fields != 0u) {
            extra_result = snprintf(extra_field, sizeof(extra_field),
                                    "extra=%zu", omitted_fields);
            if (extra_result < 0 || (size_t)extra_result >= sizeof(extra_field)) {
                return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
            }
            extra_length = (size_t)extra_result;
            extra_prefix_length = field_count_to_write == 0u ?
                                  sizeof(separator) - 1u : 1u;
            candidate_length += extra_prefix_length + extra_length;
        }
        if (candidate_length <= builder.content_limit) {
            line_truncated = field_count_to_write < included_fields;
            break;
        }
        if (field_count_to_write == 0u) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
        --field_count_to_write;
    }

    for (index = 0u; index < field_count_to_write; ++index) {
        const char *field_prefix = index == 0u ? separator : " ";
        size_t prefix_length = index == 0u ? sizeof(separator) - 1u : 1u;
        if (!append_token(&builder, field_prefix, prefix_length) ||
            !append_token(&builder, formatted_fields[index],
                          formatted_lengths[index])) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
    }
    if (field_count > field_count_to_write) {
        char extra_field[48];
        const char *field_prefix = field_count_to_write == 0u ? separator : " ";
        size_t prefix_length = field_count_to_write == 0u ?
                               sizeof(separator) - 1u : 1u;
        int extra_result = snprintf(extra_field, sizeof(extra_field),
                                    "extra=%zu", field_count - field_count_to_write);
        if (extra_result < 0 || (size_t)extra_result >= sizeof(extra_field) ||
            !append_token(&builder, field_prefix, prefix_length) ||
            !append_token(&builder, extra_field, (size_t)extra_result)) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
    }

    if (line_truncated) {
        if (builder.length >= ANIMETCG_LOG_LINE_MAX_BYTES - 1u) {
            return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
        }
        builder.data[builder.length++] = '~';
    }
    if (builder.length >= ANIMETCG_LOG_LINE_MAX_BYTES) {
        return ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL;
    }
    builder.data[builder.length++] = '\n';
    out_record->length = (uint16_t)builder.length;
    return ANIMETCG_LOG_OK;
}

static uint64_t monotonic_nanoseconds(void)
{
    struct timespec now;
    uint64_t seconds;
    uint64_t nanoseconds;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0 ||
        now.tv_nsec < 0) {
        return 0u;
    }
    seconds = (uint64_t)now.tv_sec;
    nanoseconds = (uint64_t)now.tv_nsec;
    if (seconds > (UINT64_MAX - nanoseconds) / UINT64_C(1000000000)) {
        return UINT64_MAX;
    }
    return seconds * UINT64_C(1000000000) + nanoseconds;
}

static struct timespec timespec_from_nanoseconds(uint64_t value)
{
    struct timespec result;
    uint64_t seconds = value / UINT64_C(1000000000);
    uint64_t nanoseconds = value % UINT64_C(1000000000);
    result.tv_sec = (time_t)seconds;
    result.tv_nsec = (long)nanoseconds;
    return result;
}

static bool format_monotone_utc_timestamp(char output[LOG_TIMESTAMP_BYTES + 1u])
{
    struct timespec now;
    struct tm utc;
    int64_t milliseconds;
    time_t seconds;
    int millisecond_part;
    int year;
    int length;

    if (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0 ||
        now.tv_nsec < 0 || now.tv_sec > (time_t)(INT64_MAX / INT64_C(1000))) {
        return false;
    }
    milliseconds = (int64_t)now.tv_sec * INT64_C(1000) +
                   (int64_t)(now.tv_nsec / 1000000L);
    if (milliseconds <= LAST_WRITTEN_TIMESTAMP_MS) {
        if (LAST_WRITTEN_TIMESTAMP_MS == INT64_MAX) {
            return false;
        }
        milliseconds = LAST_WRITTEN_TIMESTAMP_MS + INT64_C(1);
    }
    seconds = (time_t)(milliseconds / INT64_C(1000));
    millisecond_part = (int)(milliseconds % INT64_C(1000));
    if (gmtime_r(&seconds, &utc) == NULL) {
        return false;
    }
    year = utc.tm_year + 1900;
    if (year < 0 || year > 9999) {
        return false;
    }
    length = snprintf(output, LOG_TIMESTAMP_BYTES + 1u,
                      "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                      year, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour,
                      utc.tm_min, utc.tm_sec, millisecond_part);
    if (length != (int)LOG_TIMESTAMP_BYTES) {
        return false;
    }
    LAST_WRITTEN_TIMESTAMP_MS = milliseconds;
    return true;
}

static int write_all(int descriptor, const char *data, size_t length,
                     size_t *written_out)
{
    size_t offset = 0u;
#ifdef ANIMETCG_LOG_TESTING
    {
        size_t fail_after = atomic_exchange_explicit(&TEST_FAIL_AFTER_BYTES,
                                                      SIZE_MAX,
                                                      memory_order_acq_rel);
        if (fail_after != SIZE_MAX) {
            ssize_t written;
            if (length == 0u) {
                *written_out = 0u;
                errno = EIO;
                return -1;
            }
            if (fail_after >= length) {
                fail_after = length - 1u;
            }
            if (fail_after == 0u) {
                *written_out = 0u;
                errno = EIO;
                return -1;
            }
            do {
                written = write(descriptor, data, fail_after);
            } while (written < 0 && errno == EINTR);
            if (written > 0) {
                *written_out = (size_t)written;
                errno = EIO;
            } else {
                *written_out = 0u;
                if (written == 0) {
                    errno = EIO;
                }
            }
            return -1;
        }
    }
#endif
    while (offset < length) {
        ssize_t written = write(descriptor, data + offset, length - offset);
        if (written > 0) {
            offset += (size_t)written;
        } else if (written < 0 && errno == EINTR) {
            continue;
        } else {
            *written_out = offset;
            return -1;
        }
    }
    *written_out = offset;
    return 0;
}

static void record_io_error(void)
{
    (void)atomic_fetch_add_explicit(&STAT_IO_ERRORS, UINT64_C(1),
                                    memory_order_relaxed);
    report_output_failure_once();
}

static void disable_file_output(void)
{
    int failed_descriptor = LOG_FD;
    LOG_FD = -1;
    if (failed_descriptor >= 0) {
        (void)close(failed_descriptor);
    }
}

static bool write_materialized_record(log_record_t *record, bool allow_rotation);

static int rotate_active_file(void)
{
    char source[LOG_PATH_MAX];
    char destination[LOG_PATH_MAX];
    uint64_t previous_size = LOG_FILE_BYTES;
    unsigned int suffix;
    int rotation_error = 0;
    int saved_errno = 0;

    if (LOG_FD >= 0) {
        if (close(LOG_FD) != 0) {
            rotation_error = 1;
            saved_errno = errno;
        }
        LOG_FD = -1;
    }

    if (build_backup_path(ANIMETCG_LOG_ROTATE_FILES - 1u,
                          destination, sizeof(destination)) != 0) {
        rotation_error = 1;
        saved_errno = ENAMETOOLONG;
    } else if (unlink(destination) != 0 && errno != ENOENT) {
        rotation_error = 1;
        saved_errno = errno;
    }

    for (suffix = ANIMETCG_LOG_ROTATE_FILES - 1u; suffix > 1u; --suffix) {
        if (build_backup_path(suffix - 1u, source, sizeof(source)) != 0 ||
            build_backup_path(suffix, destination, sizeof(destination)) != 0) {
            rotation_error = 1;
            saved_errno = ENAMETOOLONG;
            break;
        }
        if (rename(source, destination) != 0 && errno != ENOENT) {
            rotation_error = 1;
            saved_errno = errno;
            break;
        }
    }

    if (rotation_error == 0 &&
        build_backup_path(1u, destination, sizeof(destination)) != 0) {
        rotation_error = 1;
        saved_errno = ENAMETOOLONG;
    }
    if (rotation_error == 0 && rename(LOG_PATH, destination) != 0) {
        rotation_error = 1;
        saved_errno = errno;
    }

    if (open_active_file() != 0) {
        if (saved_errno == 0) {
            saved_errno = errno;
        }
        errno = saved_errno;
        record_io_error();
        return -1;
    }
    if (rotation_error != 0) {
        errno = saved_errno;
        record_io_error();
        return -1;
    }

    LOG_FILE_BYTES = 0u;
    (void)atomic_fetch_add_explicit(&STAT_ROTATIONS, UINT64_C(1),
                                    memory_order_relaxed);
    {
        animetcg_log_field_t field = animetcg_log_u64(
            ANIMETCG_LOG_KEY_BYTES, previous_size);
        log_record_t notice;
        const log_context_t no_context = {
            .client_id = ANIMETCG_LOG_NO_CLIENT_ID,
            .has_client = false,
            .request_id = "-"
        };
        if (build_record(ANIMETCG_LOG_EVENT_LOG_ROTATED, &no_context,
                         &field, 1u, &notice) == ANIMETCG_LOG_OK) {
            (void)write_materialized_record(&notice, false);
        }
    }
    return 0;
}

static bool write_materialized_record(log_record_t *record, bool allow_rotation)
{
    size_t written = 0u;
    char timestamp[LOG_TIMESTAMP_BYTES + 1u];
    uint64_t record_length;
    uint64_t file_bytes_before_write;

    if (record == NULL || record->length == 0u ||
        (size_t)record->length > sizeof(record->bytes) || LOG_FD < 0) {
        record_io_error();
        return false;
    }
    record_length = (uint64_t)record->length;
    if (allow_rotation &&
        (LOG_FILE_BYTES > (uint64_t)ANIMETCG_LOG_ROTATE_BYTES ||
         record_length > (uint64_t)ANIMETCG_LOG_ROTATE_BYTES - LOG_FILE_BYTES)) {
        if (rotate_active_file() != 0) {
            return false;
        }
    }
    if (!format_monotone_utc_timestamp(timestamp)) {
        record_io_error();
        return false;
    }
    memcpy(record->bytes, timestamp, LOG_TIMESTAMP_BYTES);
    file_bytes_before_write = LOG_FILE_BYTES;
    if (write_all(LOG_FD, record->bytes, (size_t)record->length, &written) != 0) {
        if (written != 0u) {
            off_t rollback_size = (off_t)file_bytes_before_write;
            if (rollback_size >= (off_t)0 &&
                (uint64_t)rollback_size == file_bytes_before_write &&
                ftruncate(LOG_FD, rollback_size) == 0) {
                /* Remove only the incomplete tail; completed records stay append-only. */
                LOG_FILE_BYTES = file_bytes_before_write;
            } else {
                /* Never append a valid-looking record after a partial one. */
                LOG_FILE_BYTES += (uint64_t)written;
                disable_file_output();
            }
        }
        record_io_error();
        return false;
    }
    LOG_FILE_BYTES += (uint64_t)written;
    (void)atomic_fetch_add_explicit(&STAT_WRITTEN, UINT64_C(1),
                                    memory_order_relaxed);
    return true;
}

static void write_drop_summary(void)
{
    uint64_t dropped = atomic_exchange_explicit(&STAT_PENDING_DROPPED, 0u,
                                                memory_order_acq_rel);
    if (dropped != 0u) {
        animetcg_log_field_t field = animetcg_log_u64(
            ANIMETCG_LOG_KEY_DROPPED, dropped);
        log_record_t summary;
        const log_context_t no_context = {
            .client_id = ANIMETCG_LOG_NO_CLIENT_ID,
            .has_client = false,
            .request_id = "-"
        };
        if (build_record(ANIMETCG_LOG_EVENT_LOG_DROPPED, &no_context,
                         &field, 1u, &summary) != ANIMETCG_LOG_OK ||
            !write_materialized_record(&summary, true)) {
            (void)atomic_fetch_add_explicit(&STAT_PENDING_DROPPED, dropped,
                                            memory_order_release);
        }
    }
}

static void update_queue_stats_locked(void)
{
    size_t previous_high = atomic_load_explicit(&STAT_QUEUE_HIGH_WATER,
                                                 memory_order_relaxed);
    atomic_store_explicit(&STAT_QUEUE_DEPTH, QUEUE_COUNT, memory_order_relaxed);
    atomic_store_explicit(&STAT_IN_FLIGHT, WRITER_IN_FLIGHT,
                          memory_order_relaxed);
    while (QUEUE_COUNT > previous_high &&
           !atomic_compare_exchange_weak_explicit(&STAT_QUEUE_HIGH_WATER,
                                                  &previous_high, QUEUE_COUNT,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
        /* Retry with the value written by the competing producer. */
    }
}

static void drop_one_record(void)
{
    (void)atomic_fetch_add_explicit(&STAT_DROPPED, UINT64_C(1),
                                    memory_order_relaxed);
    (void)atomic_fetch_add_explicit(&STAT_PENDING_DROPPED, UINT64_C(1),
                                    memory_order_release);
}

static bool producer_enter(void)
{
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_seq_cst) != LOGGER_RUNNING) {
        return false;
    }
    (void)atomic_fetch_add_explicit(&ACTIVE_PRODUCERS, 1u, memory_order_seq_cst);
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_seq_cst) != LOGGER_RUNNING) {
        (void)atomic_fetch_sub_explicit(&ACTIVE_PRODUCERS, 1u, memory_order_seq_cst);
        return false;
    }
    return true;
}

static void producer_leave(void)
{
    (void)atomic_fetch_sub_explicit(&ACTIVE_PRODUCERS, 1u, memory_order_seq_cst);
}

static animetcg_log_status_t enqueue_record(const log_record_t *record)
{
    int lock_result = pthread_mutex_trylock(&QUEUE_MUTEX);
    if (lock_result != 0) {
        drop_one_record();
        return ANIMETCG_LOG_DROPPED;
    }
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_seq_cst) != LOGGER_RUNNING ||
        QUEUE == NULL) {
        (void)pthread_mutex_unlock(&QUEUE_MUTEX);
        return ANIMETCG_LOG_ERR_NOT_RUNNING;
    }
    if (QUEUE_COUNT == (size_t)ANIMETCG_LOG_QUEUE_CAPACITY) {
        drop_one_record();
        (void)pthread_mutex_unlock(&QUEUE_MUTEX);
        return ANIMETCG_LOG_DROPPED;
    }

    QUEUE[QUEUE_TAIL] = *record;
    QUEUE_TAIL = (QUEUE_TAIL + 1u) % (size_t)ANIMETCG_LOG_QUEUE_CAPACITY;
    ++QUEUE_COUNT;
    update_queue_stats_locked();
    (void)atomic_fetch_add_explicit(&STAT_ACCEPTED, UINT64_C(1),
                                    memory_order_relaxed);
    if (pthread_cond_signal(&QUEUE_CONDITION) != 0) {
        (void)atomic_fetch_add_explicit(&STAT_IO_ERRORS, UINT64_C(1),
                                        memory_order_relaxed);
    }
    (void)pthread_mutex_unlock(&QUEUE_MUTEX);
    return ANIMETCG_LOG_OK;
}

static void *writer_main(void *unused)
{
    uint64_t next_summary_ns;
    (void)unused;
    next_summary_ns = monotonic_nanoseconds() + LOG_SUMMARY_INTERVAL_NS;

    for (;;) {
        log_record_t record;
        bool have_record = false;
        bool stop = false;
        bool force_summary = false;
        int wait_result = 0;

        if (pthread_mutex_lock(&QUEUE_MUTEX) != 0) {
            report_output_failure_once();
            return NULL;
        }
        while (QUEUE_COUNT == 0u && !WRITER_STOP && !WRITER_PAUSED) {
            struct timespec deadline = timespec_from_nanoseconds(next_summary_ns);
            wait_result = pthread_cond_timedwait(&QUEUE_CONDITION, &QUEUE_MUTEX,
                                                 &deadline);
            if (wait_result == ETIMEDOUT) {
                break;
            }
            if (wait_result != 0) {
                break;
            }
        }
        while (WRITER_PAUSED && !WRITER_STOP) {
            wait_result = pthread_cond_wait(&QUEUE_CONDITION, &QUEUE_MUTEX);
            if (wait_result != 0) {
                break;
            }
        }
        if (QUEUE_COUNT > 0u && (!WRITER_PAUSED || WRITER_STOP)) {
            record = QUEUE[QUEUE_HEAD];
            QUEUE_HEAD = (QUEUE_HEAD + 1u) % (size_t)ANIMETCG_LOG_QUEUE_CAPACITY;
            --QUEUE_COUNT;
            ++WRITER_IN_FLIGHT;
            update_queue_stats_locked();
            have_record = true;
        } else if (WRITER_STOP && QUEUE_COUNT == 0u) {
            stop = true;
        }
        force_summary = FORCE_DROP_SUMMARY;
        FORCE_DROP_SUMMARY = false;
        (void)pthread_mutex_unlock(&QUEUE_MUTEX);

        if (have_record) {
            (void)write_materialized_record(&record, true);
            if (pthread_mutex_lock(&QUEUE_MUTEX) == 0) {
                if (WRITER_IN_FLIGHT > 0u) {
                    --WRITER_IN_FLIGHT;
                }
                update_queue_stats_locked();
                if (QUEUE_COUNT == 0u && WRITER_IN_FLIGHT == 0u) {
                    (void)pthread_cond_broadcast(&QUEUE_CONDITION);
                }
                (void)pthread_mutex_unlock(&QUEUE_MUTEX);
            } else {
                report_output_failure_once();
            }
        }

        {
            uint64_t now = monotonic_nanoseconds();
            if (force_summary || now >= next_summary_ns) {
                write_drop_summary();
                next_summary_ns = monotonic_nanoseconds() + LOG_SUMMARY_INTERVAL_NS;
            }
        }

        if (stop) {
            if (atomic_load_explicit(&STAT_PENDING_DROPPED,
                                     memory_order_acquire) != 0u) {
                uint64_t now = monotonic_nanoseconds();
                if (now < next_summary_ns) {
                    struct timespec deadline = timespec_from_nanoseconds(next_summary_ns);
                    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                           &deadline, NULL) == EINTR) {
                        /* Keep waiting until the summary interval is reached. */
                    }
                }
                write_drop_summary();
            }
            break;
        }
    }
    return NULL;
}

static void reset_statistics(void)
{
    atomic_store_explicit(&STAT_ACCEPTED, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_WRITTEN, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_DROPPED, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_PENDING_DROPPED, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_IO_ERRORS, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_INVALID_CALLS, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_ROTATIONS, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_QUEUE_DEPTH, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_QUEUE_HIGH_WATER, 0u, memory_order_relaxed);
    atomic_store_explicit(&STAT_IN_FLIGHT, 0u, memory_order_relaxed);
}

static int initialize_condition_variable(void)
{
    pthread_condattr_t attributes;
    int result = pthread_condattr_init(&attributes);
    if (result != 0) {
        return result;
    }
    result = pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    if (result == 0) {
        result = pthread_cond_init(&QUEUE_CONDITION, &attributes);
    }
    (void)pthread_condattr_destroy(&attributes);
    if (result == 0) {
        QUEUE_CONDITION_INITIALIZED = true;
    }
    return result;
}

animetcg_log_status_t animetcg_log_init(const animetcg_log_config_t *config)
{
    animetcg_log_level_t initial_level;
    const char *environment_level;
    pthread_attr_t thread_attributes;
    int result;

    if (config == NULL || config->directory == NULL || config->filename == NULL ||
        !level_is_valid(config->minimum_level)) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    if (pthread_mutex_lock(&LIFECYCLE_MUTEX) != 0) {
        return ANIMETCG_LOG_ERR_STATE;
    }
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) !=
        LOGGER_UNINITIALIZED) {
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_STATE;
    }
    if (strnlen(config->directory, LOG_DIRECTORY_MAX + 1u) > LOG_DIRECTORY_MAX ||
        build_log_path(config->directory, config->filename,
                       LOG_PATH, sizeof(LOG_PATH)) != 0 ||
        ensure_log_directory(config->directory) != 0 || open_active_file() != 0) {
        report_init_failure();
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_IO;
    }

    result = initialize_condition_variable();
    if (result != 0) {
        (void)close(LOG_FD);
        LOG_FD = -1;
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_STATE;
    }
    QUEUE = calloc((size_t)ANIMETCG_LOG_QUEUE_CAPACITY, sizeof(*QUEUE));
    if (QUEUE == NULL) {
        (void)pthread_cond_destroy(&QUEUE_CONDITION);
        QUEUE_CONDITION_INITIALIZED = false;
        (void)close(LOG_FD);
        LOG_FD = -1;
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_IO;
    }

    QUEUE_HEAD = 0u;
    QUEUE_TAIL = 0u;
    QUEUE_COUNT = 0u;
    WRITER_IN_FLIGHT = 0u;
    WRITER_STOP = false;
    WRITER_PAUSED = false;
    FORCE_DROP_SUMMARY = false;
    LAST_WRITTEN_TIMESTAMP_MS = INT64_MIN;
    reset_statistics();

    initial_level = config->minimum_level;
    environment_level = getenv("ANIMETCG_LOG_LEVEL");
    if (environment_level != NULL && environment_level[0] != '\0') {
        animetcg_log_level_t parsed_level;
        if (parse_level(environment_level, &parsed_level)) {
            initial_level = parsed_level;
        } else {
            static const char warning[] =
                "animetcg logger: invalid ANIMETCG_LOG_LEVEL; using configured level\n";
            write_emergency_message(warning, sizeof(warning) - 1u);
        }
    }
    atomic_store_explicit(&LOGGER_LEVEL, (int)initial_level, memory_order_release);

    result = pthread_attr_init(&thread_attributes);
    if (result == 0) {
        result = pthread_attr_setdetachstate(&thread_attributes,
                                             PTHREAD_CREATE_JOINABLE);
        if (result == 0) {
            result = pthread_create(&WRITER_THREAD, &thread_attributes,
                                   writer_main, NULL);
        }
        (void)pthread_attr_destroy(&thread_attributes);
    }
    if (result != 0) {
        free(QUEUE);
        QUEUE = NULL;
        (void)pthread_cond_destroy(&QUEUE_CONDITION);
        QUEUE_CONDITION_INITIALIZED = false;
        (void)close(LOG_FD);
        LOG_FD = -1;
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_STATE;
    }
    WRITER_STARTED = true;
    atomic_store_explicit(&LOGGER_STATE, LOGGER_RUNNING, memory_order_release);
    (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
    return ANIMETCG_LOG_OK;
}

animetcg_log_status_t animetcg_log_shutdown(void)
{
    animetcg_log_status_t result = ANIMETCG_LOG_OK;
    int current_state;

    if (pthread_mutex_lock(&LIFECYCLE_MUTEX) != 0) {
        return ANIMETCG_LOG_ERR_STATE;
    }
    current_state = atomic_load_explicit(&LOGGER_STATE, memory_order_acquire);
    if (current_state == LOGGER_UNINITIALIZED || current_state == LOGGER_CLOSED) {
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_OK;
    }
    if (current_state != LOGGER_RUNNING) {
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_STATE;
    }

    atomic_store_explicit(&LOGGER_STATE, LOGGER_STOPPING, memory_order_seq_cst);
    while (atomic_load_explicit(&ACTIVE_PRODUCERS, memory_order_seq_cst) != 0u) {
        (void)sched_yield();
    }
    if (pthread_mutex_lock(&QUEUE_MUTEX) != 0) {
        (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
        return ANIMETCG_LOG_ERR_STATE;
    }
    WRITER_STOP = true;
    if (pthread_cond_broadcast(&QUEUE_CONDITION) != 0) {
        result = ANIMETCG_LOG_ERR_STATE;
    }
    (void)pthread_mutex_unlock(&QUEUE_MUTEX);

    if (WRITER_STARTED) {
        int join_result = pthread_join(WRITER_THREAD, NULL);
        if (join_result != 0) {
            report_output_failure_once();
            (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
            return ANIMETCG_LOG_ERR_STATE;
        }
        WRITER_STARTED = false;
    }
    if (LOG_FD >= 0) {
        if (fdatasync(LOG_FD) != 0) {
            record_io_error();
            result = ANIMETCG_LOG_ERR_IO;
        }
        if (close(LOG_FD) != 0) {
            record_io_error();
            result = ANIMETCG_LOG_ERR_IO;
        }
        LOG_FD = -1;
    }
    free(QUEUE);
    QUEUE = NULL;
    if (QUEUE_CONDITION_INITIALIZED) {
        if (pthread_cond_destroy(&QUEUE_CONDITION) != 0) {
            result = ANIMETCG_LOG_ERR_STATE;
        }
        QUEUE_CONDITION_INITIALIZED = false;
    }
    atomic_store_explicit(&LOGGER_STATE, LOGGER_CLOSED, memory_order_release);
    (void)pthread_mutex_unlock(&LIFECYCLE_MUTEX);
    return result;
}

animetcg_log_status_t animetcg_log_set_level(animetcg_log_level_t level)
{
    if (!level_is_valid(level)) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) != LOGGER_RUNNING) {
        return ANIMETCG_LOG_ERR_NOT_RUNNING;
    }
    atomic_store_explicit(&LOGGER_LEVEL, (int)level, memory_order_release);
    return ANIMETCG_LOG_OK;
}

animetcg_log_level_t animetcg_log_get_level(void)
{
    int value = atomic_load_explicit(&LOGGER_LEVEL, memory_order_acquire);
    if (value < (int)ANIMETCG_LOG_LEVEL_TRACE ||
        value > (int)ANIMETCG_LOG_LEVEL_OFF) {
        return ANIMETCG_LOG_LEVEL_INFO;
    }
    return (animetcg_log_level_t)value;
}

bool animetcg_log_is_enabled(animetcg_log_event_t event)
{
    const animetcg_log_event_info_t *info = animetcg_log_event_get_info(event);
    if (info == NULL || info->client_only || info->deprecated ||
        atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) != LOGGER_RUNNING) {
        return false;
    }
    return (int)info->level >= atomic_load_explicit(&LOGGER_LEVEL,
                                                     memory_order_acquire);
}

animetcg_log_status_t animetcg_log_set_context(uint32_t client_id,
                                               const char *request_id)
{
    if (!request_id_is_valid(request_id)) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    THREAD_CONTEXT.client_id = client_id;
    THREAD_CONTEXT.has_client = client_id != ANIMETCG_LOG_NO_CLIENT_ID;
    if (request_id == NULL) {
        memcpy(THREAD_CONTEXT.request_id, "-", 2u);
    } else {
        memcpy(THREAD_CONTEXT.request_id, request_id, 13u);
    }
    return ANIMETCG_LOG_OK;
}

void animetcg_log_clear_context(void)
{
    THREAD_CONTEXT.client_id = ANIMETCG_LOG_NO_CLIENT_ID;
    THREAD_CONTEXT.has_client = false;
    memcpy(THREAD_CONTEXT.request_id, "-", 2u);
}

animetcg_log_status_t animetcg_log_emit(animetcg_log_event_t event,
                                         const animetcg_log_field_t *fields,
                                         size_t field_count)
{
    const animetcg_log_event_info_t *info;
    log_record_t record;
    animetcg_log_status_t status;

    if (!producer_enter()) {
        return ANIMETCG_LOG_ERR_NOT_RUNNING;
    }
    info = animetcg_log_event_get_info(event);
    if (info == NULL || info->client_only || info->deprecated) {
        (void)atomic_fetch_add_explicit(&STAT_INVALID_CALLS, UINT64_C(1),
                                        memory_order_relaxed);
        producer_leave();
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    if ((int)info->level < atomic_load_explicit(&LOGGER_LEVEL,
                                                memory_order_acquire)) {
        producer_leave();
        return ANIMETCG_LOG_FILTERED;
    }
    if (field_count > 0u && fields == NULL) {
        (void)atomic_fetch_add_explicit(&STAT_INVALID_CALLS, UINT64_C(1),
                                        memory_order_relaxed);
        producer_leave();
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    if (field_count > 0u && field_count <= ANIMETCG_LOG_MAX_FIELDS) {
        size_t index;
        for (index = 0u; index < field_count; ++index) {
            if (fields[index].key == ANIMETCG_LOG_KEY_IP &&
                info->level != ANIMETCG_LOG_LEVEL_DEBUG) {
                (void)atomic_fetch_add_explicit(&STAT_INVALID_CALLS, UINT64_C(1),
                                                memory_order_relaxed);
                producer_leave();
                return ANIMETCG_LOG_ERR_PRIVACY_POLICY;
            }
        }
    }

    status = build_record(event, &THREAD_CONTEXT, fields, field_count, &record);
    if (status != ANIMETCG_LOG_OK) {
        (void)atomic_fetch_add_explicit(&STAT_INVALID_CALLS, UINT64_C(1),
                                        memory_order_relaxed);
        producer_leave();
        return status;
    }
    status = enqueue_record(&record);
    producer_leave();
    return status;
}

animetcg_log_status_t animetcg_log_flush(uint32_t timeout_ms)
{
    uint64_t now;
    uint64_t deadline_ns;
    int wait_result;

    if (atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) != LOGGER_RUNNING) {
        return ANIMETCG_LOG_ERR_NOT_RUNNING;
    }
    now = monotonic_nanoseconds();
    if ((uint64_t)timeout_ms > (UINT64_MAX - now) / UINT64_C(1000000)) {
        deadline_ns = UINT64_MAX;
    } else {
        deadline_ns = now + (uint64_t)timeout_ms * UINT64_C(1000000);
    }
    if (pthread_mutex_lock(&QUEUE_MUTEX) != 0) {
        return ANIMETCG_LOG_ERR_STATE;
    }
    while (QUEUE_COUNT != 0u || WRITER_IN_FLIGHT != 0u) {
        struct timespec deadline = timespec_from_nanoseconds(deadline_ns);
        wait_result = pthread_cond_timedwait(&QUEUE_CONDITION, &QUEUE_MUTEX,
                                             &deadline);
        if (wait_result == ETIMEDOUT) {
            (void)pthread_mutex_unlock(&QUEUE_MUTEX);
            return ANIMETCG_LOG_ERR_TIMEOUT;
        }
        if (wait_result != 0) {
            (void)pthread_mutex_unlock(&QUEUE_MUTEX);
            return ANIMETCG_LOG_ERR_STATE;
        }
        if (atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) !=
            LOGGER_RUNNING) {
            (void)pthread_mutex_unlock(&QUEUE_MUTEX);
            return ANIMETCG_LOG_ERR_NOT_RUNNING;
        }
    }
    (void)pthread_mutex_unlock(&QUEUE_MUTEX);
    return ANIMETCG_LOG_OK;
}

void animetcg_log_get_stats(animetcg_log_stats_t *out_stats)
{
    if (out_stats == NULL) {
        return;
    }
    out_stats->accepted = atomic_load_explicit(&STAT_ACCEPTED, memory_order_relaxed);
    out_stats->written = atomic_load_explicit(&STAT_WRITTEN, memory_order_relaxed);
    out_stats->dropped = atomic_load_explicit(&STAT_DROPPED, memory_order_relaxed);
    out_stats->pending_dropped = atomic_load_explicit(&STAT_PENDING_DROPPED,
                                                       memory_order_relaxed);
    out_stats->io_errors = atomic_load_explicit(&STAT_IO_ERRORS, memory_order_relaxed);
    out_stats->invalid_calls = atomic_load_explicit(&STAT_INVALID_CALLS,
                                                     memory_order_relaxed);
    out_stats->rotations = atomic_load_explicit(&STAT_ROTATIONS, memory_order_relaxed);
    out_stats->queue_depth = atomic_load_explicit(&STAT_QUEUE_DEPTH,
                                                   memory_order_relaxed);
    out_stats->queue_high_water = atomic_load_explicit(&STAT_QUEUE_HIGH_WATER,
                                                        memory_order_relaxed);
    out_stats->in_flight = atomic_load_explicit(&STAT_IN_FLIGHT,
                                                 memory_order_relaxed);
}

animetcg_log_field_t animetcg_log_text_n(animetcg_log_key_t key,
                                          const void *value,
                                          size_t length)
{
    animetcg_log_field_t field;
    field.key = key;
    field.type = ANIMETCG_LOG_VALUE_TEXT;
    field.value.text.data = (const unsigned char *)value;
    field.value.text.length = length;
    return field;
}

animetcg_log_field_t animetcg_log_text(animetcg_log_key_t key,
                                        const char *value)
{
    size_t length = 0u;
    if (value != NULL) {
        length = strnlen(value, 4097u);
    }
    return animetcg_log_text_n(key, value, length);
}

animetcg_log_field_t animetcg_log_i64(animetcg_log_key_t key, int64_t value)
{
    animetcg_log_field_t field;
    field.key = key;
    field.type = ANIMETCG_LOG_VALUE_I64;
    field.value.i64 = value;
    return field;
}

animetcg_log_field_t animetcg_log_u64(animetcg_log_key_t key, uint64_t value)
{
    animetcg_log_field_t field;
    field.key = key;
    field.type = ANIMETCG_LOG_VALUE_U64;
    field.value.u64 = value;
    return field;
}

animetcg_log_field_t animetcg_log_decimal_1(animetcg_log_key_t key,
                                             int64_t tenths)
{
    animetcg_log_field_t field;
    field.key = key;
    field.type = ANIMETCG_LOG_VALUE_DECIMAL_1;
    field.value.decimal_1_tenths = tenths;
    return field;
}

#ifdef ANIMETCG_LOG_TESTING
animetcg_log_status_t animetcg_log_test_pause_writer(bool pause)
{
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) != LOGGER_RUNNING) {
        return ANIMETCG_LOG_ERR_NOT_RUNNING;
    }
    if (pthread_mutex_lock(&QUEUE_MUTEX) != 0) {
        return ANIMETCG_LOG_ERR_STATE;
    }
    WRITER_PAUSED = pause;
    if (!pause && atomic_load_explicit(&STAT_PENDING_DROPPED,
                                       memory_order_acquire) != 0u) {
        FORCE_DROP_SUMMARY = true;
    }
    (void)pthread_cond_broadcast(&QUEUE_CONDITION);
    (void)pthread_mutex_unlock(&QUEUE_MUTEX);
    return ANIMETCG_LOG_OK;
}

animetcg_log_status_t animetcg_log_test_fail_next_write(size_t bytes_before_failure)
{
    size_t expected = SIZE_MAX;
    if (bytes_before_failure == 0u ||
        bytes_before_failure >= (size_t)ANIMETCG_LOG_LINE_MAX_BYTES) {
        return ANIMETCG_LOG_ERR_ARGUMENT;
    }
    if (atomic_load_explicit(&LOGGER_STATE, memory_order_acquire) != LOGGER_RUNNING) {
        return ANIMETCG_LOG_ERR_NOT_RUNNING;
    }
    if (!atomic_compare_exchange_strong_explicit(&TEST_FAIL_AFTER_BYTES,
                                                 &expected,
                                                 bytes_before_failure,
                                                 memory_order_release,
                                                 memory_order_relaxed)) {
        return ANIMETCG_LOG_ERR_STATE;
    }
    return ANIMETCG_LOG_OK;
}
#endif
