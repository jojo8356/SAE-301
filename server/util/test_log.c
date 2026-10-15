/*
 * Copyright (c) AnimeTCG contributors.
 * SPDX-License-Identifier: MIT
 *
 * Small project-owned test harness; deliberately no external C test framework.
 */
#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define TEST_THREAD_COUNT 4u
#define TEST_RECORDS_PER_THREAD 250u
#define ROTATION_BATCH_SIZE 1000u
#define ROTATION_MAX_BATCHES 40u
#define SATURATION_ATTEMPTS 7000u

static int TEST_FAILURES;
static bool LOGGER_TEST_INITIALIZED;
static char TEST_DIRECTORY[] = "/tmp/animetcg-log-test-XXXXXX";

static void test_fail(const char *file, int line, const char *format, ...)
{
    va_list arguments;
    (void)fprintf(stderr, "FAIL %s:%d: ", file, line);
    va_start(arguments, format);
    (void)vfprintf(stderr, format, arguments);
    va_end(arguments);
    (void)fputc('\n', stderr);
    ++TEST_FAILURES;
}

#define CHECK(condition_, ...) \
    do { \
        if (!(condition_)) { \
            test_fail(__FILE__, __LINE__, __VA_ARGS__); \
            return false; \
        } \
    } while (0)

#define ASSERT_TEST(condition_, ...) \
    do { \
        if (!(condition_)) { \
            test_fail(__FILE__, __LINE__, __VA_ARGS__); \
            goto cleanup; \
        } \
    } while (0)

static bool check_sanitizer(void)
{
    static const unsigned char hostile[] = {
        'a', '\n', 'b', '\r', '\t', '|', 0x01u, 0x00u
    };
    static const unsigned char invalid_utf8[] = {'o', 'k', 0xffu, 'x'};
    static const char expected[] = "a\\nb\\r\\t\\u007C\\x01\\x00";
    char output[512];
    size_t output_length = 0u;
    char too_long[66];
    size_t index;

    CHECK(animetcg_log_sanitize(hostile, sizeof(hostile), output,
                                sizeof(output), 64u, &output_length) ==
          ANIMETCG_LOG_OK, "sanitize hostile input returned an error");
    CHECK(output_length == sizeof(expected) - 1u &&
          memcmp(output, expected, output_length + 1u) == 0,
          "control/separator escaping differs: '%s'", output);

    CHECK(animetcg_log_sanitize(invalid_utf8, sizeof(invalid_utf8), output,
                                sizeof(output), 64u, &output_length) ==
          ANIMETCG_LOG_OK, "sanitize invalid UTF-8 returned an error");
    CHECK(strcmp(output, "ok?x") == 0, "invalid UTF-8 was not replaced by '?'");

    memset(too_long, 'x', sizeof(too_long) - 1u);
    too_long[sizeof(too_long) - 1u] = '\0';
    CHECK(animetcg_log_sanitize((const unsigned char *)too_long,
                                sizeof(too_long) - 1u, output, sizeof(output),
                                64u, &output_length) == ANIMETCG_LOG_OK,
          "sanitize long value returned an error");
    CHECK(output_length == 65u && output[64] == '~',
          "64-character value was not capped with '~'");

    for (index = 0u; index < sizeof(output); ++index) {
        output[index] = 'X';
    }
    CHECK(animetcg_log_sanitize((const unsigned char *)"|", 1u, output, 2u,
                                64u, &output_length) ==
          ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL,
          "small sanitizer buffer was not rejected");
    CHECK(output[0] == '\0', "small-buffer result was not terminated safely");
    return true;
}

static bool parse_timestamp(const char *line, char timestamp[25])
{
    static bool regex_ready;
    static regex_t compiled;
    static const char pattern[] =
        "^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}"
        "\\.[0-9]{3}Z [|] (TRACE|DEBUG|INFO|WARN|ERROR|FATAL)"
        " [|] srv [|] c=([0-9]+|-) [|] r=([0-9a-f]{12}|-)"
        " [|] (session|net|protocol|business|chain|persistence|util|view)"
        " [|] [a-z]+( [a-z]+){1,3}( [|] .*)?$";
    size_t pipe_count = 0u;
    size_t index;
    int result;
    size_t line_length = strlen(line);

    if (!regex_ready) {
        result = regcomp(&compiled, pattern, REG_EXTENDED | REG_NOSUB);
        if (result != 0) {
            return false;
        }
        regex_ready = true;
    }
    result = regexec(&compiled, line, 0u, NULL, 0);
    if (result != 0 || line_length < 24u) {
        return false;
    }
    memcpy(timestamp, line, 24u);
    timestamp[24] = '\0';
    for (index = 0u; index < line_length; ++index) {
        if (line[index] == '|') {
            ++pipe_count;
        }
        if (line[index] == '\r') {
            return false;
        }
    }
    return pipe_count == 6u || pipe_count == 7u;
}

static bool scan_log_file(const char *path, uint64_t *line_count,
                          char previous_timestamp[25], bool *has_previous,
                          bool *found_injected_message,
                          bool *found_drop_summary,
                          bool *found_extra_summary,
                          bool *found_decimal)
{
    FILE *file = fopen(path, "rb");
    char line[ANIMETCG_LOG_LINE_MAX_BYTES + 2u];

    if (file == NULL) {
        return false;
    }
    while (fgets(line, (int)sizeof(line), file) != NULL) {
        size_t length = strlen(line);
        char timestamp[25];
        char *newline;
        if (length == 0u || line[length - 1u] != '\n' ||
            length > ANIMETCG_LOG_LINE_MAX_BYTES) {
            (void)fclose(file);
            return false;
        }
        line[length - 1u] = '\0';
        newline = strchr(line, '\n');
        if (newline != NULL || !parse_timestamp(line, timestamp)) {
            (void)fclose(file);
            return false;
        }
        if (*has_previous && strcmp(timestamp, previous_timestamp) <= 0) {
            (void)fclose(file);
            return false;
        }
        memcpy(previous_timestamp, timestamp, sizeof(timestamp));
        *has_previous = true;
        ++*line_count;
        if (strstr(line, "reason=\"safe\\n2026-01-01T00:00:00.000Z \\u007C") != NULL) {
            *found_injected_message = true;
        }
        if (strstr(line, "util | log dropped | dropped=") != NULL) {
            *found_drop_summary = true;
        }
        if (strstr(line, "extra=2") != NULL) {
            *found_extra_summary = true;
        }
        if (strstr(line, "value_after=8.0") != NULL) {
            *found_decimal = true;
        }
        if (strstr(line, "2026-01-01T00:00:00.000Z | FATAL") == line) {
            (void)fclose(file);
            return false;
        }
    }
    if (ferror(file) != 0) {
        (void)fclose(file);
        return false;
    }
    return fclose(file) == 0;
}

static bool scan_all_rotated_logs(const char *directory, uint64_t *line_count,
                                  bool *found_injected_message,
                                  bool *found_drop_summary,
                                  bool *found_extra_summary,
                                  bool *found_decimal)
{
    char previous_timestamp[25] = "";
    bool has_previous = false;
    char path[1024];
    int suffix;
    struct stat info;
    uint64_t total_bytes = 0u;
    unsigned int file_count = 0u;

    *line_count = 0u;
    for (suffix = (int)ANIMETCG_LOG_ROTATE_FILES - 1; suffix >= 1; --suffix) {
        int length = snprintf(path, sizeof(path), "%s/server.log.%d", directory,
                              suffix);
        if (length < 0 || (size_t)length >= sizeof(path)) {
            return false;
        }
        if (stat(path, &info) == 0) {
            if (!S_ISREG(info.st_mode) || info.st_size < 0 ||
                (uint64_t)info.st_size > (uint64_t)ANIMETCG_LOG_ROTATE_BYTES) {
                return false;
            }
            total_bytes += (uint64_t)info.st_size;
            ++file_count;
            if (!scan_log_file(path, line_count, previous_timestamp,
                               &has_previous, found_injected_message,
                               found_drop_summary, found_extra_summary,
                               found_decimal)) {
                return false;
            }
        } else if (errno != ENOENT) {
            return false;
        }
    }

    {
        int length = snprintf(path, sizeof(path), "%s/server.log", directory);
        if (length < 0 || (size_t)length >= sizeof(path) || stat(path, &info) != 0 ||
            !S_ISREG(info.st_mode) || info.st_size < 0 ||
            (uint64_t)info.st_size > (uint64_t)ANIMETCG_LOG_ROTATE_BYTES) {
            return false;
        }
        total_bytes += (uint64_t)info.st_size;
        ++file_count;
        if (!scan_log_file(path, line_count, previous_timestamp, &has_previous,
                           found_injected_message, found_drop_summary,
                           found_extra_summary, found_decimal)) {
            return false;
        }
    }

    return file_count <= ANIMETCG_LOG_ROTATE_FILES &&
           total_bytes <= (uint64_t)ANIMETCG_LOG_ROTATE_FILES *
                          (uint64_t)ANIMETCG_LOG_ROTATE_BYTES;
}

static void *concurrent_writer(void *argument)
{
    uintptr_t thread_number = (uintptr_t)argument;
    size_t index;
    char request_id[13];
    (void)snprintf(request_id, sizeof(request_id), "%012" PRIxPTR,
                   thread_number + 1u);
    (void)animetcg_log_set_context((uint32_t)(thread_number + 1u), request_id);
    for (index = 0u; index < TEST_RECORDS_PER_THREAD; ++index) {
        animetcg_log_field_t fields[2] = {
            animetcg_log_u64(ANIMETCG_LOG_KEY_POSITION, (uint64_t)index),
            animetcg_log_u64(ANIMETCG_LOG_KEY_DEPTH, UINT64_C(4))
        };
        animetcg_log_status_t status = animetcg_log_emit(
            ANIMETCG_LOG_EVENT_QUEUE_ENQUEUE, fields, 2u);
        if (status != ANIMETCG_LOG_OK && status != ANIMETCG_LOG_DROPPED) {
            return (void *)(uintptr_t)1u;
        }
    }
    animetcg_log_clear_context();
    return NULL;
}

static bool seed_test_log_with_incomplete_tail(const char *directory)
{
    static const char contents[] =
        "2000-01-01T00:00:00.000Z | INFO | srv | c=- | r=- | session | "
        "server started | hash_rate=1 difficulty=1\nINCOMPLETE";
    char path[1024];
    int length = snprintf(path, sizeof(path), "%s/server.log", directory);
    FILE *file;
    size_t written;
    int close_result;

    if (length < 0 || (size_t)length >= sizeof(path)) {
        return false;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    written = fwrite(contents, 1u, sizeof(contents) - 1u, file);
    close_result = fclose(file);
    return written == sizeof(contents) - 1u && close_result == 0;
}

static bool remove_test_directory(const char *directory)
{
    char path[1024];
    int suffix;
    for (suffix = 1; suffix < (int)ANIMETCG_LOG_ROTATE_FILES; ++suffix) {
        int length = snprintf(path, sizeof(path), "%s/server.log.%d", directory,
                              suffix);
        if (length < 0 || (size_t)length >= sizeof(path)) {
            return false;
        }
        if (unlink(path) != 0 && errno != ENOENT) {
            return false;
        }
    }
    if (snprintf(path, sizeof(path), "%s/server.log", directory) < 0 ||
        unlink(path) != 0 || rmdir(directory) != 0) {
        return false;
    }
    return true;
}

int main(void)
{
    animetcg_log_config_t config;
    animetcg_log_stats_t stats_before;
    animetcg_log_stats_t stats_after;
    animetcg_log_field_t hostile_field;
    animetcg_log_field_t many_fields[9];
    animetcg_log_field_t rotation_fields[8];
    pthread_t threads[TEST_THREAD_COUNT];
    char hostile_value[] = "safe\n2026-01-01T00:00:00.000Z | FATAL | srv";
    char short_value[] = "x";
    char long_value[65];
    uint64_t line_count;
    bool found_injected_message = false;
    bool found_drop_summary = false;
    bool found_extra_summary = false;
    bool found_decimal = false;
    size_t index;
    size_t batches;
    bool ok = false;
    struct stat directory_info;

    if (unsetenv("ANIMETCG_LOG_LEVEL") != 0) {
        test_fail(__FILE__, __LINE__, "unsetenv failed");
        goto cleanup;
    }
    if (!check_sanitizer()) {
        goto cleanup;
    }
    ASSERT_TEST(animetcg_log_event_count() == 70u,
          "catalogue contains %zu events, expected 70",
          animetcg_log_event_count());
    ASSERT_TEST(animetcg_log_event_get_info(ANIMETCG_LOG_EVENT_CLAIM_CLOSED_DEPRECATED)
              ->deprecated,
          "deprecated claim.closed entry is not marked deprecated");

    if (mkdtemp(TEST_DIRECTORY) == NULL) {
        test_fail(__FILE__, __LINE__, "mkdtemp failed: %s", strerror(errno));
        goto cleanup;
    }
    ASSERT_TEST(seed_test_log_with_incomplete_tail(TEST_DIRECTORY),
          "could not seed active log with a valid record and partial tail");
    config.directory = TEST_DIRECTORY;
    config.filename = "server.log";
    config.minimum_level = ANIMETCG_LOG_LEVEL_INFO;
    ASSERT_TEST(animetcg_log_init(&config) == ANIMETCG_LOG_OK,
          "logger initialization failed");
    LOGGER_TEST_INITIALIZED = true;
    ASSERT_TEST(stat(TEST_DIRECTORY, &directory_info) == 0 &&
          (directory_info.st_mode & 0777) == 0750,
          "log directory permissions are not 0750");

    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_REQUEST_RECEIVED, NULL, 0u) ==
          ANIMETCG_LOG_FILTERED,
          "DEBUG event was not filtered at the default INFO threshold");
    ASSERT_TEST(animetcg_log_set_level(ANIMETCG_LOG_LEVEL_DEBUG) == ANIMETCG_LOG_OK,
          "runtime level change to DEBUG failed");
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_REQUEST_RECEIVED, NULL, 0u) ==
          ANIMETCG_LOG_OK,
          "DEBUG event did not pass after runtime level change");
    {
        animetcg_log_field_t ip = animetcg_log_text(ANIMETCG_LOG_KEY_IP,
                                                     "192.0.2.7");
        ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_CLIENT_ADDRESS,
                                      &ip, 1u) == ANIMETCG_LOG_OK,
              "dedicated DEBUG client-address event was rejected");
        ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_REQUEST_RECEIVED,
                                      &ip, 1u) == ANIMETCG_LOG_ERR_PRIVACY_POLICY,
              "IP address was accepted outside its dedicated event");
    }
    ASSERT_TEST(animetcg_log_set_level(ANIMETCG_LOG_LEVEL_INFO) == ANIMETCG_LOG_OK,
          "runtime level change back to INFO failed");

    ASSERT_TEST(animetcg_log_set_context(42u, "ab12f3c4d5e6") == ANIMETCG_LOG_OK,
          "valid MDC context was rejected");
    ASSERT_TEST(animetcg_log_set_context(99u, "not-a-request") ==
          ANIMETCG_LOG_ERR_ARGUMENT,
          "invalid request id was accepted");
    {
        animetcg_log_field_t fields[3] = {
            animetcg_log_u64(ANIMETCG_LOG_KEY_CARD_ID, UINT64_C(17)),
            animetcg_log_text(ANIMETCG_LOG_KEY_REASON, "card_limit"),
            animetcg_log_decimal_1(ANIMETCG_LOG_KEY_VALUE_AFTER, INT64_C(80))
        };
        ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_RULE_REFUSED,
                                fields, 2u) == ANIMETCG_LOG_OK,
              "structured rule-refused event was not accepted");
        ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_VALUE_UPDATED,
                                &fields[2], 1u) == ANIMETCG_LOG_OK,
              "one-decimal field was not accepted");
    }
    ASSERT_TEST(animetcg_log_flush(5000u) == ANIMETCG_LOG_OK,
          "initial logger flush timed out");
    animetcg_log_get_stats(&stats_before);

    hostile_field = animetcg_log_text(ANIMETCG_LOG_KEY_REASON, hostile_value);
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_RULE_REFUSED,
                            &hostile_field, 1u) == ANIMETCG_LOG_OK,
          "anti-injection record was rejected");
    ASSERT_TEST(animetcg_log_flush(5000u) == ANIMETCG_LOG_OK,
          "anti-injection logger flush timed out");
    animetcg_log_get_stats(&stats_after);
    ASSERT_TEST(stats_after.written == stats_before.written + 1u,
          "anti-injection call produced %" PRIu64 " physical records, expected 1",
          stats_after.written - stats_before.written);

    {
        animetcg_log_field_t ip = animetcg_log_text(ANIMETCG_LOG_KEY_IP,
                                                     "192.0.2.7");
        ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_LISTEN_READY, &ip, 1u) ==
              ANIMETCG_LOG_ERR_PRIVACY_POLICY,
              "IP address was accepted on an INFO event");
    }
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_SESSION_WELCOMED, NULL, 0u) ==
          ANIMETCG_LOG_ERR_ARGUMENT,
          "client-only event was accepted by the server logger");
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_CLAIM_CLOSED_DEPRECATED,
                            NULL, 0u) == ANIMETCG_LOG_ERR_ARGUMENT,
          "deprecated event was emitted");

    memset(long_value, 'x', sizeof(long_value) - 1u);
    long_value[sizeof(long_value) - 1u] = '\0';
    many_fields[0] = animetcg_log_text(ANIMETCG_LOG_KEY_ACTION, short_value);
    many_fields[1] = animetcg_log_text(ANIMETCG_LOG_KEY_STATUS, short_value);
    many_fields[2] = animetcg_log_text(ANIMETCG_LOG_KEY_CODE, short_value);
    many_fields[3] = animetcg_log_text(ANIMETCG_LOG_KEY_FIELD, short_value);
    many_fields[4] = animetcg_log_text(ANIMETCG_LOG_KEY_REASON, short_value);
    many_fields[5] = animetcg_log_text(ANIMETCG_LOG_KEY_EVENT, short_value);
    many_fields[6] = animetcg_log_text(ANIMETCG_LOG_KEY_OP, short_value);
    many_fields[7] = animetcg_log_text(ANIMETCG_LOG_KEY_PATTERN, short_value);
    many_fields[8] = animetcg_log_text(ANIMETCG_LOG_KEY_VERSION, short_value);
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_SERVER_STARTED,
                            many_fields, 9u) == ANIMETCG_LOG_OK,
          "too-many-fields event was not accepted");
    ASSERT_TEST(animetcg_log_flush(5000u) == ANIMETCG_LOG_OK,
          "field-cap logger flush timed out");

    for (index = 0u; index < TEST_THREAD_COUNT; ++index) {
        int result = pthread_create(&threads[index], NULL, concurrent_writer,
                                    (void *)(uintptr_t)index);
        ASSERT_TEST(result == 0, "could not create producer thread %zu", index);
    }
    for (index = 0u; index < TEST_THREAD_COUNT; ++index) {
        void *thread_result = NULL;
        int result = pthread_join(threads[index], &thread_result);
        ASSERT_TEST(result == 0 && thread_result == NULL,
              "producer thread %zu failed", index);
    }
    ASSERT_TEST(animetcg_log_flush(10000u) == ANIMETCG_LOG_OK,
          "concurrent producer logger flush timed out");

    for (index = 0u; index < 8u; ++index) {
        static const animetcg_log_key_t keys[] = {
            ANIMETCG_LOG_KEY_ACTION, ANIMETCG_LOG_KEY_STATUS,
            ANIMETCG_LOG_KEY_CODE, ANIMETCG_LOG_KEY_FIELD,
            ANIMETCG_LOG_KEY_REASON, ANIMETCG_LOG_KEY_EVENT,
            ANIMETCG_LOG_KEY_OP, ANIMETCG_LOG_KEY_PATTERN
        };
        rotation_fields[index] = animetcg_log_text(keys[index], long_value);
    }
    animetcg_log_clear_context();

    for (batches = 0u; batches < ROTATION_MAX_BATCHES; ++batches) {
        for (index = 0u; index < ROTATION_BATCH_SIZE; ++index) {
            animetcg_log_status_t status = animetcg_log_emit(
                ANIMETCG_LOG_EVENT_SERVER_STARTED, rotation_fields, 8u);
            ASSERT_TEST(status == ANIMETCG_LOG_OK || status == ANIMETCG_LOG_DROPPED,
                  "rotation producer returned unexpected status %d", (int)status);
        }
        ASSERT_TEST(animetcg_log_flush(10000u) == ANIMETCG_LOG_OK,
              "rotation batch %zu did not drain", batches);
        animetcg_log_get_stats(&stats_after);
        if (stats_after.rotations > 0u) {
            break;
        }
    }
    ASSERT_TEST(stats_after.rotations > 0u,
          "writing more than 10 MiB did not rotate the active file");

    ASSERT_TEST(animetcg_log_test_pause_writer(true) == ANIMETCG_LOG_OK,
          "could not pause writer for saturation test");
    {
        struct timespec start;
        struct timespec finish;
        int64_t elapsed_ms;
        (void)clock_gettime(CLOCK_MONOTONIC, &start);
        for (index = 0u; index < SATURATION_ATTEMPTS; ++index) {
            animetcg_log_status_t status = animetcg_log_emit(
                ANIMETCG_LOG_EVENT_SERVER_STARTED, NULL, 0u);
            ASSERT_TEST(status == ANIMETCG_LOG_OK || status == ANIMETCG_LOG_DROPPED,
                  "saturation producer returned unexpected status %d", (int)status);
        }
        (void)clock_gettime(CLOCK_MONOTONIC, &finish);
        elapsed_ms = (int64_t)(finish.tv_sec - start.tv_sec) * INT64_C(1000) +
                     (int64_t)(finish.tv_nsec - start.tv_nsec) / INT64_C(1000000);
        ASSERT_TEST(elapsed_ms < INT64_C(500),
              "saturated non-blocking producer took %" PRId64 " ms", elapsed_ms);
    }
    animetcg_log_get_stats(&stats_before);
    ASSERT_TEST(stats_before.dropped > 0u && stats_before.queue_depth > 0u,
          "queue saturation did not produce observable drops");
    ASSERT_TEST(animetcg_log_test_pause_writer(false) == ANIMETCG_LOG_OK,
          "could not resume writer after saturation test");
    ASSERT_TEST(animetcg_log_flush(20000u) == ANIMETCG_LOG_OK,
          "saturated queue did not drain");
    animetcg_log_get_stats(&stats_after);
    ASSERT_TEST(stats_after.pending_dropped == 0u,
          "dropped-count summary remained pending after writer resumed");

    /* Force a short write, verify its incomplete tail is rolled back, then
     * confirm the same descriptor can still append a valid complete record. */
    stats_before = stats_after;
    ASSERT_TEST(animetcg_log_test_fail_next_write(8u) == ANIMETCG_LOG_OK,
          "could not arm the partial-write fault injection");
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_SERVER_STOPPED, NULL, 0u) ==
          ANIMETCG_LOG_OK,
          "partial-write fault test record was not enqueued");
    ASSERT_TEST(animetcg_log_flush(5000u) == ANIMETCG_LOG_OK,
          "logger did not finish the partial-write recovery");
    animetcg_log_get_stats(&stats_after);
    ASSERT_TEST(stats_after.io_errors == stats_before.io_errors + 1u &&
          stats_after.written == stats_before.written,
          "short write did not produce exactly one I/O error without counting a record");
    ASSERT_TEST(animetcg_log_emit(ANIMETCG_LOG_EVENT_SERVER_STOPPED, NULL, 0u) ==
          ANIMETCG_LOG_OK,
          "record after partial-write recovery was not enqueued");
    ASSERT_TEST(animetcg_log_flush(5000u) == ANIMETCG_LOG_OK,
          "record after partial-write recovery did not drain");
    animetcg_log_get_stats(&stats_after);
    ASSERT_TEST(stats_after.io_errors == stats_before.io_errors + 1u &&
          stats_after.written == stats_before.written + 1u,
          "logger did not resume cleanly after truncating an incomplete tail");

    ASSERT_TEST(animetcg_log_shutdown() == ANIMETCG_LOG_OK,
          "logger shutdown failed");
    LOGGER_TEST_INITIALIZED = false;
    ASSERT_TEST(animetcg_log_shutdown() == ANIMETCG_LOG_OK,
          "logger shutdown is not idempotent");
    ASSERT_TEST(animetcg_log_get_level() == ANIMETCG_LOG_LEVEL_INFO,
          "logger level changed unexpectedly");

    ASSERT_TEST(scan_all_rotated_logs(TEST_DIRECTORY, &line_count,
                                &found_injected_message, &found_drop_summary,
                                &found_extra_summary, &found_decimal),
          "log files failed format, size, permission, or monotonic-time checks");
    ASSERT_TEST(line_count > 10000u, "only %" PRIu64 " log lines were written", line_count);
    ASSERT_TEST(found_injected_message, "escaped anti-injection record was not found");
    ASSERT_TEST(found_drop_summary, "dropped-entry summary was not written");
    ASSERT_TEST(found_extra_summary, "excess fields were not summarized as extra=N");
    ASSERT_TEST(found_decimal, "decimal value was not serialized with one decimal place");
    animetcg_log_get_stats(&stats_after);
    ASSERT_TEST(stats_after.io_errors == 1u,
          "logger reported %" PRIu64 " I/O errors; expected the injected short write only",
          stats_after.io_errors);

    ok = true;

cleanup:
    if (LOGGER_TEST_INITIALIZED) {
        (void)animetcg_log_test_pause_writer(false);
        if (animetcg_log_shutdown() != ANIMETCG_LOG_OK) {
            test_fail(__FILE__, __LINE__, "cleanup logger shutdown failed");
            ok = false;
        }
        LOGGER_TEST_INITIALIZED = false;
    }
    if (access(TEST_DIRECTORY, F_OK) == 0) {
        if (ok) {
            if (!remove_test_directory(TEST_DIRECTORY)) {
                test_fail(__FILE__, __LINE__, "could not remove temporary log directory");
                ok = false;
            }
        } else {
            (void)fprintf(stderr, "retaining failed-test logs at %s\\n", TEST_DIRECTORY);
        }
    }
    if (!ok || TEST_FAILURES != 0) {
        (void)fprintf(stderr, "%d logger test failure(s)\n", TEST_FAILURES);
        return EXIT_FAILURE;
    }
    (void)printf("logger tests passed: catalogue, sanitizer, queue, rotation, context, short-write recovery, format\n");
    return EXIT_SUCCESS;
}
