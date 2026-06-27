.PHONY: build clean test run

build:
	@bash scripts/build.sh

clean:
	@rm -rf build

test: build
	@bash scripts/run-qemu.sh --headless

run: build
	@bash scripts/run-qemu.sh --headful
