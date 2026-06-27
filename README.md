# akoya_unikernel

Bare-metal unikernel project targeting the **Medion Akoya EX** notebook (32-bit x86, Pentium M class, no VT-x). The repository provides an agent-friendly build pipeline, a bootstrap diagnostic kernel with wired-network bring-up, and a QEMU smoke-test path for pre-hardware validation.

## Target hardware

| Property | Value |
|----------|-------|
| CPU | Intel Pentium M 735 (i686, single core) |
| RAM | ~2 GB |
| Virtualization | Not required (no VT-x dependency) |
| Boot | Multiboot1 via GRUB on USB (manual setup) |
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

## Build and test

```bash
make build    # cross-compile bootstrap kernel → build/kernel.elf and build/kernel.bin
make test     # build (if needed) + headless macvtap QEMU smoke test
make run      # build (if needed) + headful interactive macvtap QEMU session
make clean    # remove build/ artifacts
```

`make test` and `make run` automatically bring macvtap up before QEMU and tear it down afterward (including on failure).

### Expected console output (smoke test)

The bootstrap kernel always prints the initial message, then network diagnostics with stable tokens for automation:

```
akoya_unikernel bootstrap ok
build_id=<git-id>
net_link=ok
net_ip=192.168.1.42
net_ping=google.com status=ok rtt_ms=12
```

Headless `make test` asserts:

- Bootstrap message (`akoya_unikernel bootstrap ok`)
- `net_ip=<dotted-quad>` with a leased IPv4 address
- `net_ping=<label> status=ok rtt_ms=<n>` (ICMP to the build-time probe target must succeed)

On failure you may see `net_ip=fail reason=…`, `net_link=fail reason=nic`, or `net_ping=… status=fail reason=timeout|unreachable`.

### Macvtap QEMU networking

**Mandatory on every run path** (headless and headful): the guest RTL8139 uses project MAC `52:54:00:12:34:56` when libexec scripts pin the macvtap interface MAC. Ephemeral setup creates `akoya-qemu0` on the wired parent only; the host keeps its IP on the physical NIC.

Install helpers and passwordless sudo (once per workstation):

```bash
sudo bash scripts/install-bridge-libexec.sh
sudo visudo -f /etc/sudoers.d/akoya-qemu-bridge
# paste from scripts/sudoers.d/akoya-qemu-bridge.example
```

| Script | Purpose |
|--------|---------|
| `scripts/qemu-bridge-up.sh` | Creates macvtap on wired LAN only; sets guest MAC; writes `/tmp/akoya-qemu-bridge.state` |
| `scripts/qemu-bridge-down.sh` | Deletes macvtap; removes state file |
| `scripts/install-bridge-libexec.sh` | Copies the above into `/usr/local/libexec/akoya/` |

Manual cycle (same as `make test` internally):

```bash
printf 'GUEST_MAC=52:54:00:12:34:56\n' >/tmp/akoya-qemu-guest-mac
sudo -n /usr/local/libexec/akoya/qemu-bridge-up.sh
bash scripts/run-qemu.sh --headless
sudo -n /usr/local/libexec/akoya/qemu-bridge-down.sh   # always, including on failure
```

Set `AKOYA_AUTO_LAN=0` if macvtap is already configured.

**Safety:** Only `AKOYA_QEMU_LAN_IF` (default `enx00e04c6801e8`) is used as the macvtap parent. WiFi (`wlp82s0`) is never touched. If installed libexec is stale, `run-qemu.sh` falls back to the macvtap interface MAC for the current run and logs a reinstall hint.

### Run entry point (`scripts/run-qemu.sh`)

The canonical way to run a built boot image under emulation. **Display mode is mandatory** — specify exactly one of `--headful` or `--headless`.

```bash
bash scripts/run-qemu.sh --headless
bash scripts/run-qemu.sh --headful
bash scripts/run-qemu.sh --headful --exit-on-guest-done
bash scripts/run-qemu.sh --headless --image build/kernel.elf
```

**Auto-selection:** When `--image` is omitted, the script scans `build/` for runnable `*.elf` / `*.bin` stems and prefers `.elf`.

`make test` invokes `--headless` (exits when the guest finishes). `make run` invokes `--headful` (holds the window open after the guest halts).

### Build environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_BUILD_MEM_LIMIT_MB` | `4096` | Virtual-memory ceiling for compilation |
| `AKOYA_CROSS_PREFIX` | `i686-elf-` | Toolchain command prefix (empty → `gcc -m32` fallback) |
| `AKOYA_PROBE_HOST` | `google.com` | Hostname resolved at build time for ICMP probe (no DNS in guest) |

Successful builds print an `AKOYA_BUILD_RESULT=...` summary line and write `build/build.log`.

### QEMU test environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_QEMU_TIMEOUT_SEC` | `90` | Max seconds before headless smoke test fails |
| `AKOYA_BOOTSTRAP_MESSAGE` | `akoya_unikernel bootstrap ok` | Expected bootstrap console line |
| `AKOYA_QEMU_LAN_IF` | `enx00e04c6801e8` | Wired interface for macvtap parent |
| `AKOYA_QEMU_GUEST_MAC` | `52:54:00:12:34:56` | Fixed guest MAC (when libexec pins macvtap) |
| `AKOYA_QEMU_TAP_IF` | `akoya-qemu0` | Macvtap interface name |
| `AKOYA_AUTO_LAN` | `1` | Ephemeral macvtap up/down around each run |
| `AKOYA_LAN_LIBEXEC` | `/usr/local/libexec/akoya` | Installed helper scripts for passwordless sudo |

## Bare-metal boot (manual)

Automated USB/GRUB image creation is deferred. To boot on the Akoya:

1. Build the kernel: `make build` → `build/kernel.bin`.
2. Prepare a bootable USB with GRUB (BIOS/Legacy mode).
3. Add a `multiboot` entry pointing at `kernel.bin` on the USB.
4. Connect the Akoya RJ-45 port to a LAN with DHCP.
5. Boot from USB; use USB-serial on COM1 and/or the laptop panel for VGA text output.
6. Confirm console output matches the **Expected console output** section above.

### Bare-metal Ethernet checklist

Before expecting `net_ip=` / `net_ping=` success on hardware:

- [ ] RJ-45 cable connected to a live switch or router port
- [ ] LAN provides DHCP (same as a normal wired workstation on that network)
- [ ] Outbound ICMP echo to the build-time probe target is allowed (some networks block ping)
- [ ] Rebuild after probe-target DNS changes so `AKOYA_PROBE_TARGET_*` constants stay current

The in-tree NIC driver targets QEMU's RTL8139 emulation first. A bare-metal driver for the Akoya EX controller is deferred until the exact chip is confirmed on hardware.

## OpenSpec Flow

This repo uses **OpenSpec Flow (OSF)** for structured propose → apply → finish workflows.

| Path | Role |
|------|------|
| **`OPENSPEC_FLOW.md`** | Workflow overview and bundle version. |
| **`AGENTS.md`** | Agent rules and `openspec/` discipline. |
| **`openspec/`** | Living specs and active changes. |

Use **`/osf-explore`**, **`/osf-propose`**, and **`/osf-apply-changes`** with Cursor Task subagents.
