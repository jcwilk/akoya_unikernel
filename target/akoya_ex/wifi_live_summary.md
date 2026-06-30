# WiFi live inspection — Medion Akoya EX (Puppy live USB)

Collected after `ipw2200` firmware install and driver load. Radio confirmed **on** (not rfkill-blocked).

## Identity

| Field | Value |
|-------|-------|
| Chip | Intel PRO/Wireless 2200BG (`8086:4220`, rev 05) |
| PCI slot | `01:01.0` (Mini-PCI) |
| Interface | `wlp1s1` |
| **MAC address** | **`00:0e:35:ae:a2:be`** |
| Driver | `ipw2200` 1.2.2kmprq |
| Firmware | `/lib/firmware/ipw2200-bss.fw`, `ipw2200-ibss.fw` |
| Wired MAC (RTL8139) | `00:40:d0:6f:2c:74` on `enp1s2` @ `01:02.0` |

## Radio state (at collection time)

- rfkill: soft blocked **no**, hard blocked **no** (`state=1`)
- Interface: UP, not associated to any AP
- Geography: ZZM (11 802.11bg channels)

## Visible networks (scan snapshot)

- NeepsHouse
- NETGEAR22
- TP-Link_Extender
- ATTKafMJ42
- CasaRod

## Raw logs

See `raw_inspection_logs/wifi_*_live.txt` in this directory's sibling folder.

## Notes for unikernel

WiFi is **not** used by the Akoya unikernel build (wired RTL8139 only). This record is hardware inventory / live-USB diagnostics only.
