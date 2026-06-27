## Why

The Akoya EX hardware inventory now documents a 10/100 Mbit Ethernet interface, but the bootstrap unikernel still proves only console output. We need a minimal, real-network path on both the development workstation and bare metal so later features can build on DHCP, IPv4, and ICMP without reworking the foundation.

## What Changes

- Extend the bootstrap unikernel to bring up Ethernet, obtain an IPv4 address via DHCP, print the assigned address, and send one ICMP echo request to a well-known public host, reporting success or failure and round-trip latency on the console.
- Introduce a modular in-kernel network stack layout (link layer, IPv4, DHCP client, ICMP) so drivers and protocols can evolve independently.
- Update the development-workstation QEMU runner to attach an emulated NIC bridged to the workstation's LAN, using a fixed guest MAC address so repeated runs do not exhaust DHCP leases.
- Extend headless smoke-test expectations to cover the new network diagnostic console output when network mode is enabled.
- Keep wireless, DNS resolution beyond what DHCP provides, TCP, and persistent storage out of scope for this iteration.

## Capabilities

### New Capabilities

- `network-unikernel`: Observable Ethernet bring-up, DHCP lease acquisition, IPv4 assignment reporting, and a single outbound connectivity probe with latency reporting on the console.

### Modified Capabilities

- `bootstrap-unikernel`: Expand bootstrap scope beyond console-only diagnostics to include the network sequence while preserving build identity and orderly completion behavior.
- `dev-test-runner`: Add bridged Ethernet emulation with a stable guest MAC and automated assertions for network diagnostic output in headless mode.
- `deployment-target`: Relax the "no network required" conservative assumption so the deployment profile explicitly allows Ethernet-based bootstrap behavior on target-class hardware.

## Impact

- New kernel modules under a dedicated network source tree (driver, link layer, IPv4, DHCP, ICMP, orchestration).
- Changes to the main bootstrap entry flow and build wiring.
- QEMU runner script gains bridged networking, fixed MAC configuration, and extended smoke-test patterns.
- Documentation updates for network smoke testing prerequisites (host bridge permissions, DHCP on the LAN).
- No changes to living hardware-inventory provenance rules; Ethernet speed facts already recorded remain the hardware basis for targeting wired LAN only.
