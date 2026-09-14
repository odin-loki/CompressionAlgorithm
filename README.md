# hp — Hutter Prize lab

Integer-exact context-mixing compressor in `hp/`. Encoder and decoder are
the same binary. This is **not** Cypha
([github.com/odin-loki/Cypha](https://github.com/odin-loki/Cypha)); do not
mix Cypha BPC with hp archive sizes. No CUDA, Qt, or `float`/`double` in
`hp/include` or `hp/src`.

**Read first:** [PLAN.md](PLAN.md) is the experiment board (protocol, champs,
next tests). [RECORD.md](RECORD.md) is the measurement log.

## Prize bar and lab champs (2026-09-13)

Hutter 1% claim on enwik9 vs official L (fx2-cmix): **S < 109,685,197**.
`hp` has **not** been run on enwik9. Closing that gap is Track W (fork
SOTA), not another 8 MB n-gram here. As of 2026-09-13 the prize page
still lists fx2-cmix; cmix-lex is pending; cmix-obias is a **claim**
(108,492,825). If this page and PLAN/RECORD disagree, **PLAN and RECORD
lead**.

| corpus | champ | bytes | notes |
|---|---|---:|---|
| 8 MB leftover (`data/enwik8.8mb`, `--mem 22`) | v75 | **1,694,607** | fx2-manual **1,690,955**; both RT PASS |
| 100 MB (`data/enwik8.fx2man`, `--mem 26`) | v73 | **18,434,740** | RT PASS, `SLOT_MAX=31`; v75 100 MB running |

v75 8 MB identity + fx2 both RT PASS. v73 100 MB still champ until v75 100 MB. PLAN.md / RECORD.md lead.

## Build

```
g++ -O3 -std=c++17 -I hp/include hp/src/main.cpp -o hp/build/hp.exe
```

## Compress / decompress

8 MB leftover (`--mem 22`):

```
hp/build/hp.exe c --mem 22 data/enwik8.8mb hp/build/out.hp
hp/build/hp.exe d hp/build/out.hp hp/build/out.bin
```

100 MB champ confirmation (`--mem 26`). Write archives off OneDrive —
OneDrive can zero the CYHP header. Use `%LOCALAPPDATA%\hp_lab`, then copy:

```
mkdir %LOCALAPPDATA%\hp_lab
hp/build/hp.exe c --mem 26 data/enwik8.fx2man %LOCALAPPDATA%\hp_lab\e8.hp
hp/build/hp.exe d %LOCALAPPDATA%\hp_lab\e8.hp %LOCALAPPDATA%\hp_lab\e8.out
```

Same exe for `c` and `d`. Do not overwrite a binary a long job is using.
RAM and naming: PLAN.md (protocol / RAM). Corpora and `hp/build` binaries
are local artifacts, not source.

## Profile (no Python)

`--profile` is C++ (`hp/include/hp/profile.hpp`). Spectral rank is
`hp/tools/rank.cpp` (floats allowed in tools only). `dump_experts` dumps
expert opinions. Codec profile does not need Python, CUDA, or Qt.

## Doc map

| file | role |
|---|---|
| [PLAN.md](PLAN.md) | living experiment board |
| [RECORD.md](RECORD.md) | measurement log |
| [HUTTER_RESEARCH.md](HUTTER_RESEARCH.md) | prize rules and winner literature |
| [review-and-thinktank.md](review-and-thinktank.md) | Cypha only → Track C |
| harvest notes | Track W: `harvest/cmix-lex_notes.md`, `harvest/fxcm_v26_vs_hp.md`, `harvest/wiki_states_notes.md` |

Everything else (`MODELS.md`, `AXES.md`, `PATTERNS.md`, `UPGRADES.md`,
`hp/UPGRADES.md`, `hp/README.md`, `Stats for Compression/`) is archived.
Architecture narrative in `hp/README.md` is historical (proxy-era bpc
tables). There is one plan file: `PLAN.md`.
