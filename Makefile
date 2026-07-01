.PHONY: build clean test run iso iso-deps verify-iso usb etcher verify-usb

build:
	@bash scripts/build.sh

clean:
	@rm -rf build

test: build
	@bash scripts/run-test.sh

run: build
	@bash scripts/run-qemu.sh --headful

iso-deps:
	@bash scripts/build-boot-iso.sh --check-deps-only

iso:
	@bash scripts/build-boot-iso.sh

verify-iso:
	@bash scripts/verify-boot-iso.sh

usb:
	@bash scripts/build-boot-usb.sh

# Same artifact as usb — raw .img for Balena Etcher (not the .iso).
etcher: usb

verify-usb:
	@bash scripts/verify-boot-usb.sh
