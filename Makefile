.PHONY: build clean test run iso verify-iso

build:
	@bash scripts/build.sh

clean:
	@rm -rf build

test: build
	@bash scripts/run-qemu.sh --headless --logical kernel

run: build
	@bash scripts/run-qemu.sh --headful

iso:
	@bash scripts/build-boot-iso.sh

verify-iso:
	@bash scripts/verify-boot-iso.sh
