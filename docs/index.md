---
title: Home
layout: default
---

# CompressionAlgorithm

Context-mixing compressors and the research behind them. Source code lives
in the repository beside this site; the links below point at it on GitHub.

Repository: [odin-loki/CompressionAlgorithm](https://github.com/odin-loki/CompressionAlgorithm)

## The two projects

| | what it is | start here |
|---|---|---|
| **hp** | Hutter Prize lab: integer-exact context mixing, tuned against enwik | [hp README](https://github.com/odin-loki/CompressionAlgorithm/blob/master/hp/README.md) · [lab board](lab/PLAN.md) |
| **hpd** | everyday compressor built from the HP40 study, 1.8 bpc on general files | [hpd README](https://github.com/odin-loki/CompressionAlgorithm/blob/master/hpd/README.md) |

## hp: the lab compressor

Integer-exact context-mixing compressor. Encoder and decoder are the same
binary. Not related to Cypha's BPC, so do not compare hp archive sizes with
Cypha numbers.

**Status (2026-09-13):** Track W is the open problem. `hp` has not yet
been run on enwik9. The 1% claim bar on enwik9 is **S < 109,685,197**
(against fx2-cmix). If this page and the lab notes disagree, the lab notes lead.

| corpus | champ | bytes | notes |
|---|---|---:|---|
| 8 MB leftover (`--mem 22`) | v78 | **1,692,024** | fx2-manual 1,688,500; both round-trip PASS |
| 100 MB (`--mem 26`) | v77 | **18,370,971** | round-trip PASS, `SLOT_MAX=31` |

A bare build is **not** the champion. Only 10 of 568 flags are on by
default, so a bare build gives 1,804,979 bytes (1.721 bpc) on the 8 MiB
slice. The champion flag set is in
[`hp/tools/v78_flags.ps1`](https://github.com/odin-loki/CompressionAlgorithm/blob/master/hp/tools/v78_flags.ps1).

## hpd: the everyday compressor

`hpd` is HP40-Lite rebuilt for day-to-day use. It keeps the integer-exact
model and fixes what the HP40 study measured as wrong for general use.

| | GP set: 42 files, 27 MB | speed, 1 thread | peak RAM |
|---|---:|---:|---:|
| **hpd** | **1.823 bpc** (−21.3% vs xz -9e) | 78.6 KiB/s | 0.6 GiB per thread |
| HP40, `--mem 22` | 1.851 bpc (−20.1%) | 14.2 KiB/s | 10.6 GiB |
| HP40-Lite, `--mem 22` | 1.874 bpc (−19.1%) | 30.3 KiB/s | 6.2 GiB |
| xz -9e | 2.316 bpc | 1,924 KiB/s | 76 MiB |

hpd has a GPU build (`hpd -G`) that writes the same archive bytes as the CPU
build. Details are in the [hpd README](https://github.com/odin-loki/CompressionAlgorithm/blob/master/hpd/README.md).

## Paper and release

- The paper, *HP40 and hpd*, Parts I and II as one, 25 pages:
  [`paper/`](https://github.com/odin-loki/CompressionAlgorithm/tree/master/paper).
- Source package for hpd 1.2.0:
  [`release/dist/hpd-1.2.0-src.zip`](https://github.com/odin-loki/CompressionAlgorithm/blob/master/release/dist/hpd-1.2.0-src.zip).
- Zenodo upload package and how to make it:
  [`release/zenodo/`](https://github.com/odin-loki/CompressionAlgorithm/tree/master/release/zenodo).

## Map of the documents

**Lab notes** (living, in [`lab/`](lab/)):

- [PLAN.md](lab/PLAN.md): the experiment board: protocol, champs, next tests.
- [RECORD.md](lab/RECORD.md): the measurement log.
- [HUTTER_RESEARCH.md](lab/HUTTER_RESEARCH.md): prize rules and winner literature.
- [review-and-thinktank.md](lab/review-and-thinktank.md): Cypha-only notes, Track C.
- [MODELS.md](lab/MODELS.md), [AXES.md](lab/AXES.md), [PATTERNS.md](lab/PATTERNS.md),
  [UPGRADES.md](lab/UPGRADES.md): earlier notes, kept for reference.

**Reports** (in [`reports/`](reports/)):

- [HP_ALGORITHM_PROFILE.md](reports/HP_ALGORITHM_PROFILE.md): architecture, profiling and the v82 reproduction.

**Experiments** (in the repository under `labs/`):

- `labs/fx2_mixer_lab/`: screens of LSTM, Boolean and DSP knobs on patched fx2-cmix.
- `labs/stats/`: the statistics study (`gating-statistics-full-report.md`) and `cm2.cpp`.
- `labs/harvest/`: Track W inventory notes on cmix-lex, fx2-cmix v26 and wiki states.

## Working on the code

Build and test commands are in the [repository README](https://github.com/odin-loki/CompressionAlgorithm#quick-start).
The codec sources are checked against two rules: round-trip exactly, and
contain no floating point (`hp/test/no_float.sh`).
