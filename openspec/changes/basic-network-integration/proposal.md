## Why

The Akoya EX hardware inventory now documents a 10/100 Mbit Ethernet interface, but the bootstrap unikernel still proves only console output. We need a minimal, real-network path on both the development workstation and bare metal so later features can build on DHCP, IPv4, and ICMP without reworking the foundation.

## What Changes

- Replace the console-only bootstrap with a single always-network bootstrap image that brings up Ethernet, obtains an IPv4 address via DHCP, prints the assigned address, and sends one ICMP echo request to a well-known public host, reporting success or failure and round-trip latency on the console.
- Introduce a modular in-kernel network stack layout (link layer, IPv4, DHCP client, ICMP) so drivers and protocols can evolve independently.
- Require the development-workstation QEMU runner—headless and headful—to attach an emulated NIC bridged to the workstation's physical LAN on every invocation, so the guest talks directly to the same router and DHCP server as the workstation. Use a fixed guest MAC address so repeated runs do not exhaust DHCP leases.
- Extend smoke-test expectations so all run paths assert network diagnostic console output (assigned address and connectivity probe result) in addition to the bootstrap message.
- Keep wireless, DNS resolution beyond what DHCP provides, TCP, and persistent storage out of scope for this iteration.

## Capabilities

### New Capabilities

- `network-unikernel`: Observable Ethernet bring-up, DHCP lease acquisition, IPv4 assignment reporting, and a single outbound connectivity probe with latency reporting on the console.

### Modified Capabilities

- `bootstrap-unikernel`: Bootstrap always includes the network diagnostic sequence after the initial console message; no console-only build flavor.
- `dev-test-runner`: Mandatory bridged Ethernet on every run mode (headless and headful), stable guest MAC, and automated assertions for network diagnostic output.
- `deployment-target`: Deployment profile assumes wired Ethernet and DHCP are required for bootstrap diagnostic completion on target-class hardware.

## Impact

- New kernel modules under a dedicated network source tree (driver, link layer, IPv4, DHCP, ICMP, orchestration).
- Bootstrap entry flow always runs the network sequence; console-only code path removed.
- QEMU runner always uses bridged networking to the workstation LAN; user-mode or NAT-only networking is not a supported run path.
- Documentation updates for bridge setup prerequisites (host bridge permissions, DHCP on the LAN).
- No changes to living hardware-inventory provenance rules; Ethernet speed facts already recorded remain the hardware basis for targeting wired LAN only.
