.PHONY: test test-logger test-crypto sanitize sanitize-logger clean

test: test-logger test-crypto

test-logger:
	$(MAKE) -C util test

test-crypto:
	$(MAKE) -C chain test

sanitize: sanitize-logger

sanitize-logger:
	$(MAKE) -C util sanitize

clean:
	$(MAKE) -C util clean
	$(MAKE) -C chain clean
