.PHONY: test test-logger test-crypto lint sanitize sanitize-logger clean

test: test-logger test-crypto

test-logger: lint
	$(MAKE) -C util test

lint:
	python3 ../scripts/check_no_goto.py
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy util/log.c util/log_events.c util/test_log.c -- \
			-std=c11 -D_POSIX_C_SOURCE=200809L -DANIMETCG_LOG_TESTING \
			-pthread -Iutil; \
	else \
		echo 'clang-tidy not installed; strict no-goto scan still ran'; \
	fi

test-crypto:
	$(MAKE) -C chain test

sanitize: sanitize-logger

sanitize-logger:
	$(MAKE) -C util sanitize

clean:
	$(MAKE) -C util clean
	$(MAKE) -C chain clean
