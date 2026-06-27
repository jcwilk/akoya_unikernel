.PHONY: build clean test

build:
	@bash scripts/build.sh

clean:
	@rm -rf build

test: build
	@bash scripts/run-qemu.sh
