# Medion Akoya EX — source material index

This directory holds **Akoya EX–confirmed** source material that supports facts in [`../akoya_ex.yaml`](../akoya_ex.yaml).

## How to use this directory

1. **Add sources** — Save markdown summaries, permitted downloads, or link archives here. Every file must be storable in-repo.
2. **Index here** — Add a row to the table below before (or when) any fact from that source enters the YAML.
3. **Update YAML** — Only add facts traceable to indexed Akoya EX–specific sources.
4. **Withdraw bad sources** — Remove the file, delete its index row, and remove any YAML facts that depended solely on it.

## Relationship to `akoya.profile`

[`../akoya.profile`](../akoya.profile) remains the **thin bash-sourcable build constants** file used by `scripts/build.sh`. It is **not** auto-generated from this inventory.

When build tooling needs a new constant, derive it manually from confirmed facts in `akoya_ex.yaml` (and document the change). The profile may intentionally contain only a subset (for example deployment/arch constraints) and can lag the fuller YAML record until someone syncs them.

## Source quality rules

Material enters this store **only** if it documents the Medion Akoya EX through **primary specification or verified observation**:

- **Accept:** manufacturer publications for this product; direct observation of the deployment machine; professional or editorial review of an identified Akoya EX unit; **retailer product specification listings** that publish structured specs for this exact model/SKU (not upgrade compatibility catalogs); engineering documentation for a subsystem already confirmed on the Akoya EX through a separate retained source.
- **Reject other Akoya variants** (P-series, E-series, LS, XL, etc.) unless a separate Akoya EX source corroborates the same fact.
- **Reject similar unnamed notebooks** even if hardware looks alike.
- **Reject commercially inferred claims** — memory upgrade shops, compatibility listings, and reseller catalogs whose limits reflect stocked SKUs rather than OEM engineering SHALL NOT support hardware capability claims.
- **Reject unverified user discourse** — forum threads, Q&A sites, and peer answers SHALL NOT be retained even when they name the Akoya EX.
- **Generic component datasheets alone are insufficient** — an Akoya EX–named source must tie the component to this machine (for example review naming 855GM on Akoya EX, plus Intel GMCH datasheet for memory limits).

If online Akoya EX material is scarce, keep `akoya_ex.yaml` sparse rather than infer from other models or derivative sources.

---

## Source index

| File | Retrieved | Original URL | Akoya EX facts supported |
|------|-----------|--------------|--------------------------|
| [`web-search-notes-2026-06-26.md`](web-search-notes-2026-06-26.md) | 2026-06-26 | (search log) | Documents queries, retained vs rejected URLs for provenance audit |
| [`bestbuy_medion_akoya_ex_specifications.md`](bestbuy_medion_akoya_ex_specifications.md) | 2026-06-26 | https://www.bestbuy.com/site/medion-akoya-ex-notebook-with-intel-centrino/7134941.p?skuId=7134941 | **MD95335**, SKU 7134941, UPC; Centrino 855GM / Pentium M 735 / 2200BG; **512 MB DDR, expandable to 2.0 GB**; 14.1" 1280×768 WXGA; 80 GB HDD; double-layer DVD±RW/CD-RW; card reader formats; Intel Extreme Graphics 2 (64 MB); DVI-out, IrDA, FireWire, 3× USB 2.0, 10/100 Ethernet, V.90 modem; PCMCIA Type II; 16-bit stereo; 5.1 lb / 1.3" thin; lithium-ion battery; touchpad; Windows XP Home SP2; bundled Works / PowerCinema / Nero |
| [`ryanbetz-notebookreview-review-2009-03-01.md`](ryanbetz-notebookreview-review-2009-03-01.md) | 2026-06-26 | https://ryanbetz.wordpress.com/2009/03/01/medion-akoya-ex-specs/ | Marketing name; 14.1" lineup context; CPU (Pentium M 735 Dothan); chipset 855GM / Extreme Graphics 2; RAM default 512 MB DDR; display 1280×768 glossy; storage 80 GB 4200 RPM (Samsung MP0804H in tested unit); optical drive; card reader; audio; weight/dimensions; wireless 2200BG; ports (USB 2.0, VGA, DVI-D, FireWire, Ethernet, modem, IrDA, PCMCIA, audio); no Bluetooth; OS/software bundle (Windows XP Home SP2); battery-life observations |
| [`intel-855gm-gmch-memory-capacity.md`](intel-855gm-gmch-memory-capacity.md) | 2026-06-26 | https://www.intel.com/content/dam/doc/datasheet/855gm-gme-chipset-graphics-memory-controller-hub-datasheet.pdf | GMCH memory limit **2 GB** with high-density SO-DIMMs; two SO-DIMM slots; 1 GB per module — used with Akoya EX sources confirming **855GM** |

---

## Source conflicts

Cross-check of **Best Buy product specifications** vs **ryanbetz review** on overlapping fields:

| Topic | Best Buy | Review | Result |
|-------|----------|--------|--------|
| CPU (Pentium M 735, 1.7 GHz, 400 MHz FSB, 2 MB L2) | ✓ | ✓ | **Agree** |
| Chipset / graphics (855GM, Extreme Graphics 2) | ✓ | ✓ | **Agree** |
| Default RAM 512 MB DDR | ✓ | ✓ | **Agree** |
| **Maximum RAM 2048 MB** | ✓ (“expandable to 2.0GB”) | (not stated) | **No conflict** — Best Buy confirms deployment target; Intel 855GM datasheet corroborates |
| Display 14.1" 1280×768 | ✓ | ✓ (glossy TFT) | **Agree** |
| Storage 80 GB | ✓ | ✓ (4200 RPM, Samsung model on tested unit) | **Agree** — RPM/model are review-only detail |
| Wireless 2200BG 802.11b/g | ✓ | ✓ | **Agree** |
| Ethernet 10/100, V.90 modem | ✓ | ✓ | **Agree** |
| USB 3× 2.0, FireWire, IrDA, DVI-out | ✓ | ✓ (+ VGA, port placement) | **Agree** — VGA/placement review-only |
| Weight 5.1 lb | ✓ | ✓ | **Agree** |
| OS / core bundled apps | ✓ | ✓ (+ Norton, recovery partition) | **Agree** — review adds items Best Buy lists as “and more” |

**No factual conflicts** on shared specification fields. Facts present in only one source (chassis color, USB placement, Samsung HDD model, Norton/recovery partition, brightness levels, Bluetooth absent) remain single-sourced without contradiction.

---

## Provenance audit (2026-06-26)

- Every YAML fact maps to at least one row above.
- Every file in this directory (except this README) has an index row.
- Memory-vendor compatibility pages excluded (`offtek-md95335-memory-specs.md` removed).
- Forum / Q&A sources excluded (`howtomendit-mim2060-ram-2010.md` removed).
- No variant-only sources retained.
