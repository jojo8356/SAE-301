CC ?= cc
BUILD_DIR ?= build
OPENSSL_CFLAGS ?= $(shell pkg-config --cflags libcrypto 2>/dev/null)
OPENSSL_LIBS ?= $(shell pkg-config --libs libcrypto 2>/dev/null || printf '%s' '-lcrypto')
CFLAGS ?= -O2 -g
LDFLAGS ?=

STRICT_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror \
                -Wconversion -Wshadow -Wstrict-prototypes \
                -Wmissing-prototypes -Wformat=2 -Wundef

TEST_BINARY = $(BUILD_DIR)/test_crypto
CRYPTO_SOURCES = crypto.c sha256.c header.c pow.c verify.c
CRYPTO_HEADERS = crypto.h crypto_internal.h

.PHONY: all test sanitize memcheck clean

all: $(TEST_BINARY)

$(TEST_BINARY): $(CRYPTO_SOURCES) $(CRYPTO_HEADERS) test_crypto.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(OPENSSL_CFLAGS) $(CFLAGS) $(STRICT_CFLAGS) \
		$(CRYPTO_SOURCES) test_crypto.c $(LDFLAGS) $(OPENSSL_LIBS) -o $@

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
