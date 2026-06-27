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

- **Accept:** manufacturer publications for this product; direct observation of the deployment machine; professional or editorial review of an identified Akoya EX unit; engineering documentation for a subsystem already confirmed on the Akoya EX through a separate retained source.
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
| [`ryanbetz-notebookreview-review-2009-03-01.md`](ryanbetz-notebookreview-review-2009-03-01.md) | 2026-06-26 | https://ryanbetz.wordpress.com/2009/03/01/medion-akoya-ex-specs/ | Marketing name; 14.1" lineup context; CPU (Pentium M 735 Dothan); chipset 855GM / Extreme Graphics 2; RAM default 512 MB DDR; display 1280×768 glossy; storage 80 GB 4200 RPM (Samsung MP0804H in tested unit); optical drive; card reader; audio; weight/dimensions; wireless 2200BG; ports (USB 2.0, VGA, DVI-D, FireWire, Ethernet, modem, IrDA, PCMCIA, audio); no Bluetooth; OS/software bundle (Windows XP Home SP2); battery-life observations |
| [`intel-855gm-gmch-memory-capacity.md`](intel-855gm-gmch-memory-capacity.md) | 2026-06-26 | https://www.intel.com/content/dam/doc/datasheet/855gm-gme-chipset-graphics-memory-controller-hub-datasheet.pdf | GMCH memory limit **2 GB** with high-density SO-DIMMs; two SO-DIMM slots; 1 GB per module — used only with review confirming **855GM on Akoya EX** |

---

## Provenance audit (2026-06-26)

- Every YAML fact maps to at least one row above.
- Every file in this directory (except this README) has an index row.
- Memory-vendor compatibility pages excluded (`offtek-md95335-memory-specs.md` removed).
- Forum / Q&A sources excluded (`howtomendit-mim2060-ram-2010.md` removed).
- No variant-only sources retained.
