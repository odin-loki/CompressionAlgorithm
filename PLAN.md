# PLAN — living experiment board

This is the one execution plan for this workspace. `RECORD.md` is the
measurement log; do not copy it here. Historical boards (`PLAN2.md`,
`PLAN3.md`, `hp/PLAN.md`) are archived. `HUTTER_RESEARCH.md` is literature.
`review-and-thinktank.md` is a **Cypha** review (Track C only).

Three tracks, different machines of work:

| track | where | job |
|---|---|---|
| **H** | this repo, `hp/` | integer-exact lab; one flag at a time; SHA gates |
| **W** | `harvest/fx2-cmix`, `harvest/cmix-lex` | fork SOTA if we want a prize cheque |
| **C** | github.com/odin-loki/Cypha | mixer / eval remediations; **not** this tree |

Do not vendor Cypha into `hp`. Do not add CUDA, Qt, or floats to `hp`.
Do not treat Cypha’s 2.664 bpc (`n_eval = 2000`) as comparable to any hp
8 MB / 100 MB SHA number.

---

## 1. Lab truth (2026-09-12)

Live numbers may lead `RECORD.md` by a run or two. Use this table, then
append new results to RECORD.

### Prize bar (enwik9)

| | bytes |
|---|---:|
| Official L (fx2-cmix) | 110,793,128 |
| 1% claim: S < | **109,685,197** |
| If cmix-lex awards: next 1% ≈ | 108,553,546 |

`hp` has **not** been run on enwik9. Closing the prize gap is Track W
plus preprocess, not another 8 MB n-gram on this tree.

### enwik8 SOTA vs this lab

| | bytes | notes |
|---|---:|---|
| cmix v21 (LTCB, not Hutter-legal RAM) | ~14.62 MB | published SOTA-ish |
| **hp 100 MB champ (encode)** | **18,633,242** | v45 mem 26, `SLOT_MAX=31` |
| hp 100 MB RT-confirmed | 18,671,091 | v37 mem 26 |

### 8 MB champ (`data/enwik8.8mb`, `--mem 22`)

| build | identity B | fx2-manual B | status |
|---|---:|---:|---|
| v55 (`+ BRK_GRP`) | 1,708,329 | 1,704,368 | RT PASS |
| v56 (`+ SP_GRP`) | 1,707,742 | 1,703,460 | RT PASS |
| **v57 (`+ SLOT_COL3`)** | **1,707,696** | **1,703,430** | **RT PASS** champ |

### 100 MB (`data/enwik8.fx2man`, `--mem 26`)

| build | bytes | status | binary |
|---|---:|---|---|
| v37 | 18,671,091 | **RT PASS** | `hp/build/hp_v37.exe` |
| v45 | 18,633,242 | encode only | `hp/build/hp_v45_m26.exe` (`SLOT_MAX=31`) |
| **v55** | **18,528,992** | **RT PASS** | `hp/build/hp_v55_m26.exe` (`SLOT_MAX=31`) |

---

## 2. Track H protocol

One compile flag per binary. Same binary encodes and decodes. No
`float` / `double` / `<cmath>` in `hp/include` or `hp/src`.

**Gate (in order):**

1. Build a new-named exe (never overwrite a binary a long job is using).
2. Compress first 8 MB of `data/enwik8` at `--mem 22`.
3. **Accept if archive bytes drop** vs the current 8 MB champ.
4. Round-trip SHA of the decompressed file vs the input slice.
5. If accepted: stack-check on `data/enwik8.8mb.fx2man` (same mem 22).
6. Champ only: full enwik8 fx2-manual at `--mem 26`.

1 MB (`data/enwik8.1mb`) is hostile (84% redirects). It may lie. Never
accept or reject on 1 MB alone. See RECORD “why 1 MB lied” / H0.5.

---

## 3. RAM and binary naming

| job | typical RSS | rule |
|---|---|---|
| 8 MB leftover, mem 22, `SLOT_MAX` ≤ 35 | ~ high-single to ~15 GB | one at a time is fine |
| 100 MB mem 26, `SLOT_MAX=31` | ~15 GB | champ confirmation only |
| 100 MB mem 26, `SLOT_MAX=35` | OOM | **do not run** |
| 8 MB leftover **plus** 100 MB | ~38 GB + 15 GB | **do not run together** |

**Binaries — do not overwrite while in use:**

`hp_v37.exe`, `hp_v45.exe`, `hp_v45_m26.exe`, `hp_v55.exe`, `hp_v55_m26.exe`,
`hp_v56.exe`, `hp_v57.exe`.

Name new builds `hp_vNN.exe` or `hp_<flag>.exe`. 100 MB at mem 26 with
a cap other than the 8 MB default: `hp_vNN_m26.exe` and compile
`-DHP_SLOT_MAX=31` (not 35).

Build (Windows / g++):

```
g++ -O3 -std=c++17 -I hp/include hp/src/main.cpp -o hp/build/hp_vNN.exe
```

Compress / decompress:

```
hp/build/hp_vNN.exe c --mem 22 data/enwik8.8mb hp/build/e8_8mb_vNN.hp
hp/build/hp_vNN.exe d hp/build/e8_8mb_vNN.hp hp/build/e8_8mb_vNN.out
```

---

## 4. Track H — next tests

Run in this order. One flag. Log every call in `RECORD.md`.

| # | test | how | accept |
|---|---|---|---|
| H1 | Champ **v57** 100 MB mem 26 | `hp_v57_m26.exe`, `-DHP_SLOT_MAX=31` only; no 8 MB job | bytes < 18,528,992 |
| H2 | Skip `HP_SENGRP_C0` on champ | `#elif` after `HP_SENGRP_WORD` (already on) | no-op |
| H3 | Do **not** 100 MB at `SLOT_MAX=35` | — | RAM / OOM |
| H4 | Do **not** pair an 8 MB leftover with a 100 MB job | — | RAM |
| H7 | Do **not** 100 MB at `SLOT_MAX=35` | — | RAM / OOM |
| H8 | Do **not** pair an 8 MB leftover with a 100 MB job | — | RAM |
| H9 | Track W inventory pass | read `harvest/cmix-lex_notes.md`, `harvest/fxcm_v26_vs_hp.md` | written gaps only |
| H10 | Land a **proven** H keep on the fork | only after H accept + fx2 stack | enwik8 bytes down on their pipeline |

Still thin vs winners (do **not** start until H1–H6 are idle): article
reorder already accepted (fx2-manual); wiki transform / payload_lex;
stemmer streams (H2.1 rejected as extra experts — fold variants only
with new evidence); LSTM mixer last; PPMD mmap only if RSS < 10 GB.

---

## 5. Track W — winner fork (the cheque)

Hutter’s FAQ: combine with SOTA. Do not invent a third self-extracting
pipeline. Use theirs.

| id | test | accept |
|---|---|---|
| W1 | `harvest/cmix-lex` and `harvest/fx2-cmix` already cloned | binaries run; README S recorded |
| W2 | Diff fx2 vs lex (`payload_lex`, `R1ORD3`, fxcm_v26) | inventory in harvest notes |
| W3 | Diff fxcm_v26 vs `hp/include/hp/wiki.hpp` / streams | port list with axis tags |
| W4 | Time/RAM envelope on this machine | go / no-go for enwik9 (≤10 GB, ~50 h) |
| W5 | After an H keep, land **one** change on the fork | enwik8 down, then enwik9 |

Track H will not hit S < 109,685,197 this month. Use H to pick the 1%,
then W to cash it.

---

## 6. Track C — Cypha programme

Source: `review-and-thinktank.md` (Cypha @ `b686658`, not this repo).
Work happens in the Cypha tree. **hp already has** binary decomposition,
logistic mixing, integer tables, and SHA round-trips. The think-tank
mixer rewrite is a Cypha fix, not an hp port.

**Do (in Cypha):**

| # | action | why |
|---|---|---|
| C1 | `n_eval` ≥ 100k, ≥5 seeds, report 95% CI | 2000-char lock is ±0.103 bpc; best-of-25 expected spurious gain ≈ 0.104 bpc |
| C2 | Log-loss mixer gradient: `w_i (p_i/p_mix − 1)` | Hedge linear reward vs codec log loss; measured −38.7% on their harness |
| C3 | After C2, try removing `neural_weight_floor` | floor was a tourniquet |
| C4 | Binary decomposition + logistic (PAQ) mix | linear p-mix cannot beat its sharpest expert; harness −22.4% vs C2 |
| C5 | Hash-pointer match; drop quadratic `find_match` | enwik9-infeasible as written |
| C6 | Fixed-budget nibble-bucketed tables | `unordered_map<uint64_t, vector<float>>` will not fit 10 GB |
| C7 | Integer-only codec path + cross-toolchain RT | `exp`/`log` + FMA desync on Hutter’s machine |
| C8 | External baselines (xz / zpaq / lpaq / ppmd) | 2.664 is unanchored |
| C9 | Ablation registry; delete knobs that do not pay | 148-field god object |

**Do not:**

- Merge Cypha LM / Qt / REST / CUDA into `hp/`.
- Quote Cypha 2.664 bpc next to hp 1.637 / 1.490 bpc as a horse race.
- Treat the prize as a revenue line (think-tank §3.6). Track W is a
  credential and a forcing function.

Think-tank §8 Q2 is answered: **yes**, `hp` already implements binary
logistic mixing. Priority list in that review applies to Cypha’s
`AdaptivePredictorMixer`, not to Track H.

---

## 7. Do not retest

Pointer is the RECORD heading unless noted. New evidence only.

| item | evidence |
|---|---|
| CTW | RECORD A.3: r = 0.930 vs o2:py |
| case-preserving word stream | A.3: r = 0.944 vs word:py |
| stacked word-match-2/3 as twins | A.3: r = 0.969 |
| `pat_` + bracket together | A.3: r = 0.971 |
| B.3 shipped pattern library / dict on proxy | PATTERNS.md; +1,498 B transform vs 10,121 B storage |
| H1.4 dict on 8 MB | RECORD H1.4: +33,369 |
| discovery slot/eval sweep | RECORD H0.2: all reject |
| H2.2 section mute | +15,052 on 8 MB |
| H2.1 stem as extra experts | +288 vs v5 |
| H2.7 pred-gate | +6,928 on 1 MB |
| recency fold / wt3 / fword gate | RECORD v9 leftovers |
| `HP_WORD_GRP` | RECORD v46 leftovers: +1,885 (splits word table) |
| `HP_LINK_GRP` | RECORD v54 leftovers: +78 |
| `HP_SENGRP_POS` | RECORD v55 leftovers: 0 |
| `HP_NUM_GRP` | RECORD v55 leftovers: +69 |
| `HP_SEN_GROUP` mixer gate | v46 leftovers: +680 (dilution) |
| `HP_SLOT_WORD10` on 8 MB | +43,179 (2^32 sparse) |
| `HP_SLOT_O34G` | −87 noise |
| `HP_MATCH_12` | +100 vs v43 |
| `HP_MATCH_GROW2` | −87 noise vs v37 leftovers |
| 100 MB at `SLOT_MAX=35` | OOM (this plan §3) |
| GPU / nncp in hp | illegal for Hutter |
| recursive compression | Hutter FAQ |

---

## 8. Execution order

**This machine (Track H):** H1 v57-100 at `SLOT_MAX=31` (not 35). Never H7/H8.
No Cypha into `hp`.

**Track W:** W2–W3 notes any time; W4 before any enwik9 fantasy; W5
only after an H accept.

**Track C:** in the Cypha repo, C1–C2 first (eval + one-line gradient).
No Cypha work lands in `hp/src`.

---

## 9. Standing constraints

- No `float`, `double`, `<cmath>` in `hp/include` or `hp/src`.
- Encoder and decoder derive all state from already-coded bytes.
- One flag, 8 MB mem 22, bytes-down, then SHA, then fx2-manual stack.
- 100 MB mem 26 only on a champ, `SLOT_MAX=31`.
- Effective rank is a compass, not an accept gate by itself (1 MB lied;
  correlated twins can still drop bytes at 8 MB — still prefer new axes).
- Harvest clones stay in `harvest/`; do not rewrite them.

---

## 10. Doc map

| file | role |
|---|---|
| **PLAN.md** (this file) | what to run next |
| **RECORD.md** | what we measured |
| **README.md** | what the repo is; how to build |
| HUTTER_RESEARCH.md | prize literature |
| review-and-thinktank.md | Cypha critique → Track C |
| MODELS.md / AXES.md | inventory / saturation |
| UPGRADES.md / hp/UPGRADES.md | older roadmap; findings still true |
| PATTERNS.md | B.1/B.2 taxonomy; B.3 NO-GO |
| PLAN2.md / PLAN3.md / hp/PLAN.md | archived boards |
| hp/README.md | hp architecture (proxy-era numbers; use §1 above for champs) |
