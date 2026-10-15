CC ?= cc
BUILD_DIR ?= build
CFLAGS ?= -O2 -g
LDFLAGS ?=
THREAD_FLAGS ?= -pthread

STRICT_CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic \
                -Werror -Wconversion -Wshadow -Wstrict-prototypes \
                -Wmissing-prototypes -Wformat=2 -Wundef

TEST_BINARY = $(BUILD_DIR)/test_log
LOGGER_SOURCES = log.c log_events.c
LOGGER_HEADERS = log.h log_events.h

.PHONY: all test sanitize memcheck clean

all: $(TEST_BINARY)

$(TEST_BINARY): $(LOGGER_SOURCES) $(LOGGER_HEADERS) test_log.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_CFLAGS) -DANIMETCG_LOG_TESTING \
		$(LOGGER_SOURCES) test_log.c $(LDFLAGS) $(THREAD_FLAGS) -o $@

test: $(TEST_BINARY)
	./$(TEST_BINARY)

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined' \
		LDFLAGS='-fsanitize=address,undefined' test

memcheck: $(TEST_BINARY)
	@if ! command -v valgrind >/dev/null 2>&1; then \
		echo 'valgrind is not installed; run make sanitize instead'; \
		exit 2; \
	fi
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
		--errors-for-leak-kinds=definite,possible --error-exitcode=1 ./$(TEST_BINARY)

clean:
	rm -rf $(BUILD_DIR)
