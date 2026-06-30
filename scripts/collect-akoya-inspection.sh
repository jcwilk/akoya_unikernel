#!/usr/bin/env bash
# Collect hardware inspection logs from Akoya live USB via SSH.
# Usage: AKOYA_SSH_HOST=root@192.168.1.124 AKOYA_SSH_PASS=temppass ./scripts/collect-akoya-inspection.sh

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/target/akoya_ex/raw_inspection_logs"
HOST="${AKOYA_SSH_HOST:-root@192.168.1.124}"
PASS="${AKOYA_SSH_PASS:-temppass}"

mkdir -p "${OUT}"

python3 - "${HOST}" "${PASS}" "${OUT}" <<'PY'
import sys
import os
import shlex
import pexpect

host, password, out_dir = sys.argv[1:4]

# (filename, command) — skip gracefully if tool missing
commands = [
    # --- already useful; refresh / extend ---
    ("lspci_nnv_wifi_01_01.0.txt", "lspci -nnv -s 01:01.0"),
    ("lspci_nnv_audio_00_1f.5.txt", "lspci -nnv -s 00:1f.5"),
    ("lspci_nnv_ide_00_1f.1.txt", "lspci -nnv -s 00:1f.1"),
    ("lspci_nnv_usb_ehci.txt", "lspci -nnv -s 00:1d.7"),
    ("lspci_nnv_cardbus.txt", "lspci -nnv -s 01:04.0"),
    ("lspci_nnv_firewire.txt", "lspci -nnv -s 01:05.0"),
    ("lspci_xxx_01_01.0.txt", "lspci -xxxs 01:01.0"),
    ("setpci_01_01.0.txt", "setpci -s 01:01.0 0x00.l 0x04.l 0x10.l 0x3c.b"),
    # WiFi
    ("wifi_iwconfig.txt", "iwconfig 2>&1"),
    ("wifi_ip_link.txt", "ip link show 2>&1"),
    ("wifi_modules_ipw2200.txt", "modinfo ipw2200 2>&1 | head -80"),
    ("wifi_dmesg_ipw.txt", "dmesg 2>/dev/null | grep -iE 'ipw|2200|wlan|wireless' || true"),
    ("sys_class_net_all.txt", "for d in /sys/class/net/*; do echo \"=== $d ===\"; ls -la \"$d\"; cat \"$d/address\" 2>/dev/null; cat \"$d/device/uevent\" 2>/dev/null; echo; done"),
    # Audio / speakers
    ("proc_asound_cards.txt", "cat /proc/asound/cards 2>&1"),
    ("proc_asound_devices.txt", "cat /proc/asound/devices 2>&1"),
    ("proc_asound_pcm.txt", "cat /proc/asound/pcm 2>&1"),
    ("aplay_l.txt", "aplay -l 2>&1"),
    ("amixer_scontrols.txt", "amixer scontrols 2>&1"),
    ("amixer_scontents.txt", "amixer scontents 2>&1 | head -120"),
    ("dmesg_audio.txt", "dmesg 2>/dev/null | grep -iE 'snd|ac97|intel8x0|audio|sound' || true"),
    # Storage / IDE / disk
    ("hdparm_sda.txt", "hdparm -I /dev/sda 2>&1"),
    ("hdparm_sdb.txt", "hdparm -I /dev/sdb 2>&1"),
    ("cat_proc_partitions.txt", "cat /proc/partitions"),
    ("sys_block_detail.txt", "for b in /sys/block/sd* /sys/block/hd*; do [ -e \"$b\" ] || continue; echo \"=== $b ===\"; ls -la \"$b\"; cat \"$b/device/model\" 2>/dev/null; cat \"$b/device/vendor\" 2>/dev/null; cat \"$b/queue/rotational\" 2>/dev/null; cat \"$b/size\" 2>/dev/null; echo; done"),
    ("dmesg_ata_ide.txt", "dmesg 2>/dev/null | grep -iE 'ata|ide|piix|disk|scsi|usb-storage' || true"),
    ("mount.txt", "mount"),
    ("df_h.txt", "df -h"),
    ("blkid.txt", "blkid 2>&1"),
    # USB
    ("lsusb.txt", "lsusb 2>&1"),
    ("lsusb_v.txt", "lsusb -v 2>&1 | head -400"),
    ("dmesg_usb.txt", "dmesg 2>/dev/null | grep -iE 'usb|ehci|uhci' || true"),
    ("sys_kernel_debug_usb_devices.txt", "cat /sys/kernel/debug/usb/devices 2>&1 | head -300"),
    # Keyboard / i8042 / PS/2
    ("dmesg_i8042_ps2.txt", "dmesg 2>/dev/null | grep -iE 'i8042|ps/2|keyboard|serio' || true"),
    ("sys_bus_serio_devices.txt", "for d in /sys/bus/serio/devices/*; do echo \"=== $d ===\"; for f in \"$d\"/*; do [ -f \"$f\" ] && echo \"$(basename $f)=$(cat $f 2>/dev/null)\"; done; echo; done"),
    ("ioports_i8042_grep.txt", "grep -E '0060|0064|0061' /proc/ioports 2>/dev/null || cat /proc/ioports"),
    ("input_event_caps.txt", "for ev in /sys/class/input/event*; do echo \"=== $ev ===\"; cat \"$ev/device/name\" 2>/dev/null; cat \"$ev/device/capabilities/ev\" 2>/dev/null; echo; done"),
    # PCI / IRQ / ACPI / power
    ("proc_acpi_wakeup.txt", "cat /proc/acpi/wakeup 2>&1"),
    ("sys_class_power_supply.txt", "for d in /sys/class/power_supply/*; do echo \"=== $d ===\"; for f in \"$d\"/*; do [ -f \"$f\" ] && echo \"$(basename $f)=$(cat $f 2>/dev/null)\"; done; echo; done 2>&1"),
    ("battery_uevent.txt", "cat /sys/class/power_supply/*/uevent 2>&1"),
    ("acpi_dump_tables.txt", "acpidump 2>&1 | head -5; ls /sys/firmware/acpi/tables/ 2>&1; for t in /sys/firmware/acpi/tables/*; do echo \"=== $t ===\"; wc -c \"$t\" 2>/dev/null; done"),
    ("proc_cpuinfo_flags.txt", "grep -E '^(model name|flags|cpu MHz|cache)' /proc/cpuinfo"),
    ("lscpu.txt", "lscpu 2>&1"),
    ("free_h.txt", "free -h"),
    ("uptime.txt", "uptime"),
    ("lsmod.txt", "lsmod"),
    ("modules_builtin_i8042.txt", "find /lib/modules/$(uname -r) -name '*i8042*' -o -name '*8139*' -o -name '*ipw2200*' 2>/dev/null | head -50"),
    # Display / framebuffer
    ("fb_info.txt", "cat /sys/class/graphics/fb0/name /sys/class/graphics/fb0/virtual_size /sys/class/graphics/fb0/bits_per_pixel 2>&1"),
    ("dmesg_fb_i915.txt", "dmesg 2>/dev/null | grep -iE 'fb|i915|drm|vga' | head -80 || true"),
    # Modem (present on this machine)
    ("lspci_nnv_modem.txt", "lspci -nnv -s 00:1f.6"),
    # SMBus / sensors
    ("sensors.txt", "sensors 2>&1"),
    ("i2cdetect_l.txt", "i2cdetect -l 2>&1"),
    # Full dmidecode types we might have missed
    ("dmidecode_baseboard.txt", "dmidecode -t baseboard 2>&1"),
    ("dmidecode_chassis.txt", "dmidecode -t chassis 2>&1"),
    ("dmidecode_slot.txt", "dmidecode -t slot 2>&1"),
    ("dmidecode_connector.txt", "dmidecode -t connector 2>&1"),
    # Boot / firmware
    ("efibootmgr.txt", "efibootmgr 2>&1"),
    ("dmi_id.txt", "cat /sys/class/dmi/id/* 2>&1"),
    ("vmlinuz_version.txt", "uname -a; cat /etc/os-release 2>/dev/null; cat /etc/puppyversion 2>/dev/null"),
]

def run_remote(cmd, timeout=120):
    ssh_cmd = (
        f"ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "
        f"{host} {shlex.quote(cmd)}"
    )
    child = pexpect.spawn("/bin/bash", ["-c", ssh_cmd], encoding="utf-8", timeout=timeout)
    idx = child.expect(["password:", pexpect.EOF], timeout=30)
    if idx == 0:
        child.sendline(password)
        child.expect(pexpect.EOF, timeout=timeout)
    out = child.before or ""
    exitcode = child.exitstatus if child.exitstatus is not None else 0
    return out, exitcode

ok = 0
fail = 0
for fname, cmd in commands:
    path = os.path.join(out_dir, fname)
    try:
        out, code = run_remote(cmd)
        with open(path, "w") as f:
            f.write(f"# {cmd}\n\n")
            f.write(out)
            if not out.endswith("\n"):
                f.write("\n")
            f.write(f"\n# exit_code={code}\n")
        print(f"OK  {fname} (exit {code})")
        ok += 1
    except Exception as e:
        with open(path, "w") as f:
            f.write(f"# {cmd}\n\n# COLLECTION ERROR: {e}\n")
        print(f"ERR {fname}: {e}")
        fail += 1

print(f"\nDone: {ok} ok, {fail} err -> {out_dir}")
PY

echo "Total files: $(ls -1 "${OUT}"/*.txt 2>/dev/null | wc -l)"
