# Intel 855GM GMCH — system memory capacity (datasheet excerpt)

**Source URL:** https://www.intel.com/content/dam/doc/datasheet/855gm-gme-chipset-graphics-memory-controller-hub-datasheet.pdf  
**Retrieved:** 2026-06-26  
**Akoya EX confirmation:** Not a machine-specific document. Used only together with [`ryanbetz-notebookreview-review-2009-03-01.md`](ryanbetz-notebookreview-review-2009-03-01.md), which names **Intel 855GM** as the chipset on the Medion Akoya EX.

## Relevant GMCH memory interface limits

From Intel 855GM/855GME GMCH datasheet, section 2.3 (GMCH System Memory Interface):

- One channel of PC1600/2100 SO-DIMM DDR SDRAM (855GM) or PC1600/2100/2700 (855GME)
- DDR SDRAM device densities: 128-Mb, 256-Mb, and 512-Mb technology
- **Up to 1 GB** (512-Mb technology) with two SO-DIMMs using standard density
- **Up to 2 GB** (512-Mb technology) using **high-density** devices with two SO-DIMMs

Memory capacity table (512-Mb technology, 8-bit width): **1 GB per SO-DIMM**, **2 GB system maximum** with stacked/high-density pairing.

## Use in this inventory

Supports **2048 MB maximum system RAM**, **2 slots**, and **1024 MB maximum per module** on the Akoya EX because the review-confirmed 855GM memory controller can address 2 GB when populated with appropriate high-density SO-DIMMs (subject to BIOS/module compatibility).
