# Compression Algorithm — hp lab

Integer-exact context-mixing compressor (`hp/`) used as a Hutter Prize
laboratory. Encoder and decoder are the same binary. No floating point
in the coding path.

This is **not** Cypha. Cypha lives at
[github.com/odin-loki/Cypha](https://github.com/odin-loki/Cypha). A
review of that mixer is in `review-and-thinktank.md`; its action list
is **Track C** in `PLAN.md`. Do not mix those BPC numbers with hp
archives.

## What to read

| file | what it is |
|---|---|
| [PLAN.md](PLAN.md) | living experiment board — protocol, champs, next tests |
| [RECORD.md](RECORD.md) | measurement log (accepted / rejected runs) |
| [HUTTER_RESEARCH.md](HUTTER_RESEARCH.md) | prize rules and winner literature |
| [hp/README.md](hp/README.md) | architecture (proxy-era table; champs are in PLAN) |

**Prize bar:** S < 109,685,197 vs L = 110,793,128 on enwik9. hp has not
been run on enwik9. **Current lab champs** (2026-09-12): 8 MB v47
1,716,568 (RT in flight); 100 MB v45 encode 18,633,242 / v37 RT
18,671,091. Details and “what to run next” are in PLAN.md.

## Build

```
g++ -O3 -std=c++17 -I hp/include hp/src/main.cpp -o hp/build/hp.exe
```

Or `cmake -S hp -B hp/build && cmake --build hp/build`.

## Compress / decompress

```
hp/build/hp.exe c --mem 22 data/enwik8.8mb hp/build/out.hp
hp/build/hp.exe d hp/build/out.hp hp/build/out.bin
```

`--mem 22` for 8 MB leftover tests; `--mem 26` for full enwik8 champ
confirmation. Same exe for `c` and `d`. Do not overwrite
`hp_v37.exe` / `hp_v45.exe` / `hp_v45_m26.exe` / `hp_v46.exe` /
`hp_v47.exe` while a job is using them. RAM rules: PLAN.md §3.

Corpora (`data/enwik8*`) and `hp/build` binaries are local artifacts,
not source.
