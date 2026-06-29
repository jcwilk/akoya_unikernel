# akoya_unikernel

Bare-metal unikernel project targeting the **Medion Akoya EX** notebook (32-bit x86, Pentium M class, no VT-x). The repository provides an agent-friendly build pipeline, a bootstrap diagnostic kernel with wired-network bring-up, and a QEMU smoke-test path for pre-hardware validation.

## Target hardware

| Property | Value |
|----------|-------|
| CPU | Intel Pentium M 735 (i686, single core) |
| RAM | ~2 GB |
| Virtualization | Not required (no VT-x dependency) |
| Boot | Multiboot1 via GRUB on prepared boot media (automated ISO) |
| Ethernet | 10/100 Mbit RJ-45 (wired LAN only for bootstrap diagnostics) |

Machine-readable constants live in `target/akoya.profile`.

## Prerequisites

Development workstation (Linux):

- **Cross toolchain:** `i686-elf-gcc` and `i686-elf-binutils` on `PATH`, or `gcc -m32` / multilib as a fallback (see `scripts/build.sh`). The vendored toolchain under `tools/bin/` is prepended automatically when present.

- **QEMU:** `qemu-system-i386` for emulation (Debian/Ubuntu package `qemu-system-x86`):

  ```bash
  sudo apt install qemu-system-x86
  ```

- **Macvtap LAN access:** Every QEMU run attaches an emulated RTL8139 NIC to the workstation LAN via ephemeral macvtap on the designated **USB wired** interface (`enx00e04c6801e8` by default). WiFi is never modified.

- **Node.js 20.19+** for OpenSpec Flow (`npx @fission-ai/openspec@latest …`).

- **ISO packaging (required for `make iso`):** `grub-mkrescue` and `xorriso`. **`make iso` exits immediately if either is missing** — install before packaging:

  ```bash
  sudo apt install grub-pc-bin xorriso
  make iso-deps   # optional: verify tools without building
  ```

  Output artifact: `build/akoya-boot.iso` (~2–3 MB).

- **USB/HDD disk image (required for `make usb` / `make etcher`):** `grub-install`, `genext2fs`, `sfdisk`, and `dd`. **`make usb` exits immediately if any are missing** — install before packaging:

  ```bash
  apt install grub-pc-bin genext2fs util-linux
  make usb
  ```

  Output artifact: `build/akoya-boot.img` (64 MiB MBR + ext2 + GRUB; symlink `build/akoya-etcher.img` for Etcher).

## Build and test

```bash
make build    # cross-compile bootstrap kernel → build/kernel.elf, build/kernel.bin, build/transport-test.*
make test     # build (if needed) + headless macvtap QEMU smoke test
make run      # build (if needed) + headful interactive macvtap QEMU session
make iso          # BIOS/Legacy ISO → build/akoya-boot.iso (QEMU optical verify; not for USB dd on legacy BIOS)
make verify-iso   # QEMU boot-from-ISO smoke
make usb          # MBR+ext2 disk image → build/akoya-boot.img (+ akoya-etcher.img symlink for Etcher)
make etcher       # alias for make usb
make verify-usb   # QEMU boot-from-disk smoke (builds image if missing)
make clean    # remove build/ artifacts
```

`make test` and `make run` automatically bring macvtap up before QEMU and tear it down afterward (including on failure).

### Agent verification entry points

Automated verification **requires** the configured inference endpoint (`AKOYA_CHAT_HOST_IP` / `AKOYA_CHAT_PORT`, default `192.168.1.110:11435`) to be reachable from the workstation. All entry points below abort before starting emulation when pre-flight fails.

| Entry point | Purpose |
|-------------|---------|
| `bash scripts/build-boot-iso.sh` / `make iso` | Package `build/akoya-boot.iso` (Multiboot1 + GRUB, BIOS/Legacy) |
| `bash scripts/verify-boot-iso.sh` / `make verify-iso` | Boot packaged ISO under QEMU; pass on bootstrap + connectivity probe (no inference pre-flight) |
| `bash scripts/run-transport-test.sh` | Build (if needed), run `transport-test` image headlessly, exit 0 on `transport-test: ALL PASS` |
| `bash scripts/run-qemu.sh --headless --logical kernel --script FILE` | Full-app scripted chat with output assertions (`*.akoya-script`) |
| `bash scripts/run-qemu.sh --headless --logical kernel` | Default multi-turn scripted chat regression (`scripts/fixtures/multi-turn-pong.akoya-script`) |
| `bash scripts/run-qemu.sh --headless --boot-iso build/akoya-boot.iso` | ISO boot smoke (bootstrap + connectivity probe only) |
| `bash scripts/run-qemu.sh --headless --image build/transport-test.elf` | Explicit transport-test image selection when both images exist |

`make verify-iso` and `run-qemu.sh --boot-iso` **do not** require the inference endpoint to be reachable from the workstation. `make test` and default headless kernel runs still require inference pre-flight.

When both `kernel.*` and `transport-test.*` exist under `build/`, omit `--image` / `--logical` and the runner errors with an actionable list—use `--logical kernel` or `--logical transport-test`.

### Transport-test image

`scripts/build.sh` emits two logical boot images sharing the production network stack (`link`, IPv4, DHCP, TCP):

- `build/kernel.bin` / `build/kernel.elf` — interactive chat unikernel
- `build/transport-test.bin` / `build/transport-test.elf` — non-interactive transport scenario suite

The transport-test image prints per-scenario `PASS`/`FAIL` lines and a final aggregate `transport-test: ALL PASS` or `transport-test: FAILED`. The refused-connection scenario targets port **19999** on the configured chat host (synthetic closed port, not inference downtime).

### Scripted chat interaction (`*.akoya-script`)

Headless-only. Parsed by `scripts/chat-script-harness.sh` and invoked via `run-qemu.sh --script`.

```
# comments allowed
delay 2000                 # ms before next action
expect " reachable"        # substring or /regex/ match on serial log (excludes echoed input lines)
type "reply only with PONG"
expect "PONG"
expect_line reply          # Nth assistant reply line after reachability (N = chat turns typed so far)
expect_not "chat failed: connect"
type "quit"
```

Keystrokes use the existing QEMU monitor `sendkey` path. Assertions poll captured serial output and fail fast with step index and log snippet.

Sample fixture: `scripts/fixtures/multi-turn-pong.akoya-script`.

**Headful script injection** is not supported; use `--headless --script`.

### Expected console output (smoke test)

The bootstrap kernel always prints the initial message, then network diagnostics with stable tokens for automation. After a successful ping, an interactive chat session starts on the integrated PS/2 keyboard path:

```
akoya_unikernel bootstrap ok
build_id=<git-id>
net_link=ok
net_ip=192.168.1.42
net_ping=google.com status=ok rtt_ms=12
chat>hi
chat_completion=ok reply=Hello!
chat>quit
chat_session=exit
```

When the configured chat endpoint (`192.168.1.110:11435` by default) is unreachable, you may see `chat_completion=fail reason=connect|timeout|http|parse|overflow` instead.

### Interactive chat session

After network diagnostics succeed, the kernel enters a REPL-style chat loop:

- **Input:** US-layout integrated keyboard via the i8042 PS/2 controller (the standard built-in keyboard path on Pentium M / ICH-era notebooks including the Akoya EX). In QEMU, headful SDL typing and headless `sendkey` injection both use this same emulated PS/2 path—not serial stdin.
- **Line editing:** Printable ASCII is echoed; Backspace removes the last character; Enter submits the line. Empty lines are ignored.
- **Exit:** Type `quit` or `exit` (case-insensitive) to end the session without sending inference.
- **History:** Up to 16 turns (~8 KB JSON budget) of user and assistant messages are retained in memory and included in each chat-completion request.
- **Headless automation:** `make test` runs the default multi-turn scripted regression (`scripts/fixtures/multi-turn-pong.akoya-script`): two inference turns with per-turn reply assertions and rejection of connection-failure lines between turns. Set `AKOYA_USE_KEYBOARD_SCRIPT=1` to restore legacy QEMU `sendkey` injection via `AKOYA_CHAT_SCRIPT` instead.

Headless `make test` asserts:

- Bootstrap message (`akoya_unikernel bootstrap ok`)
- `net_ip=<dotted-quad>` with a leased IPv4 address
- `net_ping=<label> status=ok rtt_ms=<n>` (ICMP to the build-time probe target must succeed)
- Plain assistant reply text for at least two scripted chat turns when the host can reach `192.168.1.110:11435` at test time (pre-flight check in `run-qemu.sh`)
- No `chat failed:` lines between successful multi-turn exchanges

**llama.cpp prerequisite:** For full chat-completion acceptance, run a llama.cpp OpenAI-compatible server on the workstation LAN at `http://192.168.1.110:11435/v1/chat/completions`. Headless verification aborts before emulation when the endpoint is unreachable from the workstation.

On failure you may see `net_ip=fail reason=…`, `net_link=fail reason=nic`, `net_ping=… status=fail reason=timeout|unreachable`, or `chat_completion=fail reason=…`.

### Macvtap QEMU networking

**Mandatory on every run path** (headless and headful): the guest RTL8139 uses project MAC `52:54:00:12:34:56` when libexec scripts pin the macvtap interface MAC. Ephemeral setup creates `akoya-qemu0` on the wired parent only; the host keeps its IP on the physical NIC.

One-time admin install (once per workstation). After this, `make test` and `make run` work as your normal user:

```bash
sudo bash scripts/install-bridge-libexec.sh
```

This copies macvtap helpers to `/usr/local/libexec/akoya/` and applies `cap_net_admin` via `bridge-cap-exec` so unprivileged runs can create ephemeral macvtap devices.

| Script | Purpose |
|--------|---------|
| `scripts/qemu-bridge-up.sh` | Creates macvtap on wired LAN only; sets guest MAC; writes `/tmp/akoya-qemu-bridge.state` |
| `scripts/qemu-bridge-down.sh` | Deletes macvtap; removes state file |
| `scripts/install-bridge-libexec.sh` | One-time admin install: copies helpers + `setcap cap_net_admin+ep` on `bridge-cap-exec` |

Manual cycle (same as `make test` internally):

```bash
printf 'GUEST_MAC=52:54:00:12:34:56\n' >/tmp/akoya-qemu-guest-mac
/usr/local/libexec/akoya/qemu-bridge-up
bash scripts/run-qemu.sh --headless
/usr/local/libexec/akoya/qemu-bridge-down   # always, including on failure
```

**Legacy optional path:** passwordless sudoers via `scripts/sudoers.d/akoya-qemu-bridge.example` — not required when the setcap install succeeds.

Set `AKOYA_AUTO_LAN=0` if macvtap is already configured.

**Safety:** Only `AKOYA_QEMU_LAN_IF` (default `enx00e04c6801e8`) is used as the macvtap parent. WiFi (`wlp82s0`) is never touched. If installed libexec is stale, `run-qemu.sh` falls back to the macvtap interface MAC for the current run and logs a reinstall hint.

### Run entry point (`scripts/run-qemu.sh`)

The canonical way to run a built boot image under emulation. **Display mode is mandatory** — specify exactly one of `--headful` or `--headless`.

```bash
bash scripts/run-qemu.sh --headless
bash scripts/run-qemu.sh --headful
bash scripts/run-qemu.sh --headful --exit-on-guest-done
bash scripts/run-qemu.sh --headless --image build/kernel.elf
bash scripts/run-qemu.sh --headless --boot-iso build/akoya-boot.iso
```

**Boot modes:** `--image` / `--logical` load a flat kernel image with `-kernel`. `--boot-iso PATH` boots from virtual optical media (`-cdrom`, BIOS/Legacy `order=d`) and, in headless mode, uses a lighter assertion profile (bootstrap + connectivity probe only).

**Auto-selection:** When `--image` is omitted and `--boot-iso` is not set, the script scans `build/` for runnable `*.elf` / `*.bin` stems and prefers `.elf`.

`make test` invokes `--headless` (exits when the guest finishes). `make run` invokes `--headful` (holds the window open after the guest halts).

### Build environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_BUILD_MEM_LIMIT_MB` | `4096` | Virtual-memory ceiling for compilation |
| `AKOYA_CROSS_PREFIX` | `i686-elf-` | Toolchain command prefix (empty → `gcc -m32` fallback) |
| `AKOYA_PROBE_HOST` | `google.com` | Hostname resolved at build time for ICMP probe (no DNS in guest) |
| `AKOYA_CHAT_HOST_IP` | `192.168.1.110` | IPv4 address for chat-completion HTTP probe |
| `AKOYA_CHAT_PATH` | `/v1/chat/completions` | HTTP path for chat-completion probe |
| `AKOYA_CHAT_USER_MSG` | `hi` | Legacy build-time default (interactive session uses keyboard input) |
| `AKOYA_CHAT_MODEL` | `fast-text-qwen3-8b` | Model string in chat-completion JSON body |
| `AKOYA_CHAT_PORT` | `11435` | TCP port for chat-completion HTTP probe |
| `AKOYA_CHAT_TIMEOUT_MS` | `60000` | Total timeout for chat HTTP exchange (ms) |

Successful builds print an `AKOYA_BUILD_RESULT=...` summary line and write `build/build.log`.

### QEMU test environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_QEMU_TIMEOUT_SEC` | `300` | Max seconds before headless smoke test fails |
| `AKOYA_BOOTSTRAP_MESSAGE` | `akoya_unikernel bootstrap ok` | Expected bootstrap console line |
| `AKOYA_QEMU_LAN_IF` | `enx00e04c6801e8` | Wired interface for macvtap parent |
| `AKOYA_QEMU_GUEST_MAC` | `52:54:00:12:34:56` | Fixed guest MAC (when libexec pins macvtap) |
| `AKOYA_QEMU_TAP_IF` | `akoya-qemu0` | Macvtap interface name |
| `AKOYA_AUTO_LAN` | `1` | Ephemeral macvtap up/down around each run |
| `AKOYA_LAN_LIBEXEC` | `/usr/local/libexec/akoya` | Installed macvtap helpers (capability-bound after one-time install) |
| `AKOYA_CHAT_HOST_IP` | `192.168.1.110` | Chat endpoint host for pre-flight reachability check |
| `AKOYA_CHAT_PORT` | `11435` | Chat endpoint port for pre-flight reachability check |
| `AKOYA_CHAT_SCRIPT` | `h i ret w h a t ret q u i t ret` | Legacy headless sendkey sequence when `AKOYA_USE_KEYBOARD_SCRIPT=1` |
| `AKOYA_USE_KEYBOARD_SCRIPT` | `0` | `1` selects `AKOYA_CHAT_SCRIPT` instead of the default multi-turn `*.akoya-script` |
| `AKOYA_SKIP_INFERENCE_PREFLIGHT` | `0` | `1` skips chat-endpoint pre-flight (set by `verify-boot-iso.sh` / `--boot-iso` smoke) |

## Bare-metal boot

### USB flash drive (primary — use this on the Akoya)

#### Balena Etcher (recommended)

Etcher is **compatible** with this project. It writes the selected file byte-for-byte to the USB device—the same operation as `dd`. Use the **`.img` disk image**, not the `.iso`:

| File | Etcher | Akoya USB boot |
|------|--------|----------------|
| `build/akoya-boot.img` (or `akoya-etcher.img`) | Yes — select this | Works (MBR + ext2 + GRUB) |
| `build/akoya-boot.iso` | Yes — Etcher will flash it | **Broken** on legacy BIOS (GRUB `hd0` geometry errors → rescue mode) |

The ISO failure is **not** an Etcher limitation. You can verify independently: flash the ISO with Etcher, observe the GRUB rescue errors; flash the `.img` with Etcher on the same stick, boot succeeds. Etcher and `dd` produce identical USB contents for a given input file (`cmp` the first N MiB of the device after each write if you want to prove it).

**Etcher steps:**

1. Build the disk image on your workstation:

   ```bash
   apt install grub-pc-bin genext2fs util-linux
   make usb
   ```

2. Open Balena Etcher → **Flash from file** → choose `build/akoya-etcher.img` (or `build/akoya-boot.img`).

3. Select the USB target drive (not a partition).

4. Flash. Etcher validates the write; no extra steps.

5. Boot the Akoya from USB in **Legacy/BIOS** mode (not UEFI).

Optional pre-hardware check: `make verify-usb` (QEMU boots the same `.img` as a hard disk).

#### Command-line alternative (`dd`)

**Do not** use `build/akoya-boot.iso` for USB on legacy BIOS — see table above.

1. Install packaging tools on the development workstation:

   ```bash
   apt install grub-pc-bin genext2fs util-linux
   ```

2. Build the USB/HDD disk image (unprivileged — user-space ext2 assembly + GRUB into a plain file):

   ```bash
   make usb
   ```

   Success prints `AKOYA_USB_RESULT=status=success;img=build/akoya-boot.img;...`. Output is a 64 MiB MBR + ext2 image with GRUB in the MBR and `kernel.elf` on the partition. A symlink `build/akoya-etcher.img` points at the same file for Etcher workflows.

3. Write the **disk image** to the whole USB device (Etcher or `dd`):

   ```bash
   lsblk
   sudo dd if=build/akoya-boot.img of=/dev/sdX bs=4M status=progress conv=fsync
   sync
   ```

   Replace `/dev/sdX` with the USB stick (e.g. `/dev/sdb`), **not** a partition like `/dev/sdb1`.

4. Connect the Akoya RJ-45 port to a LAN with DHCP.

5. Boot from USB in the firmware menu (Legacy/BIOS mode, not UEFI).

6. Confirm console output matches **Expected console output** (bootstrap line, `net_ip=`, connectivity probe, then chat session).

Pre-hardware confidence: `make verify-usb` (QEMU boots `akoya-boot.img` as `-hda`; bootstrap + connectivity probe; no inference pre-flight).

### ISO (QEMU / optical media only)

For emulation or an internal optical drive, not recommended for USB sticks on legacy BIOS:

```bash
sudo apt install grub-pc-bin xorriso
make iso
make verify-iso
```

### Internal boot drive

Use the same `akoya-boot.img` with whole-disk `dd` when imaging a small internal boot device, or expand the image size with `AKOYA_USB_IMAGE_SIZE_MB=128 make usb` if your tooling requires more headroom.

### Bare-metal Ethernet checklist

Before expecting `net_ip=` and connectivity-probe success on hardware:

- [ ] RJ-45 cable connected to a live switch or router port
- [ ] LAN provides DHCP (same as a normal wired workstation on that network)
- [ ] Outbound ICMP echo to the configured probe/chat host is allowed (some networks block ping)
- [ ] Rebuild after probe-target DNS changes so build-time IP constants stay current

The in-tree NIC driver targets QEMU's RTL8139 emulation first. A bare-metal driver for the Akoya EX controller is deferred until the exact chip is confirmed on hardware.

## Appendix: manual GRUB/USB setup (fallback)

If automated ISO packaging is unavailable on your workstation, you can prepare boot media manually:

1. Build the kernel: `make build` → `build/kernel.bin`.
2. Prepare a bootable USB with GRUB (BIOS/Legacy mode).
3. Copy `build/kernel.elf` (or `build/kernel.bin` where your GRUB build supports flat Multiboot payloads) onto the USB.
4. Add a `multiboot` entry pointing at the kernel file on the USB.
5. Connect the Akoya RJ-45 port to a LAN with DHCP.
6. Boot from USB; use USB-serial on COM1 and/or the laptop panel for VGA text output.
7. Confirm console output matches the **Expected console output** section above.

Workstation packages for the automated path: `grub-pc-bin`, `genext2fs`, `util-linux` (USB/HDD disk image); `grub-pc-bin`, `xorriso` (ISO).

## OpenSpec Flow

This repo uses **OpenSpec Flow (OSF)** for structured propose → apply → finish workflows.

| Path | Role |
|------|------|
| **`OPENSPEC_FLOW.md`** | Workflow overview and bundle version. |
| **`AGENTS.md`** | Agent rules and `openspec/` discipline. |
| **`openspec/`** | Living specs and active changes. |

Use **`/osf-explore`**, **`/osf-propose`**, and **`/osf-apply-changes`** with Cursor Task subagents.
