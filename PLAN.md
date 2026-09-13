# PLAN — living experiment board

## Vision

hp is an integer-exact PAQ-lineage laboratory. Goal: find which 1% is
real, then land it on a winner fork (Track W). Official Hutter bar is
enwik9 **S < 109,685,197** vs **L = 110,793,128**. hp has not been run
on enwik9. 8 MB leftovers will not cash the cheque.

Three tracks, different machines:

| track | where | job |
|---|---|---|
| **H** | this repo, `hp/` | one compile flag; 8 MB mem 22; bytes-down, SHA, fx2 stack; 100 MB mem 26 champ only with `SLOT_MAX=31` |
| **W** | `harvest/fx2-cmix`, `harvest/cmix-lex` | fork SOTA |
| **C** | github.com/odin-loki/Cypha | `review-and-thinktank.md` only |

Never vendor Cypha, CUDA, Qt, or floats into hp. Do not treat Cypha’s
2.664 bpc (`n_eval = 2000`) as comparable to any hp SHA number.

---

## Lab truth

Leftover wave closed at **v61**. RECORD leads if a later run accepts.

| | bytes |
|---|---:|
| Official L (fx2-cmix, prize page 2026-09-13) | 110,793,128 |
| 1% claim vs L: S < | **109,685,197** |
| cmix-lex (pending; Intel binary) | 109,650,047 |
| cmix-lex AMD rebuild (claimed) | 109,671,639 |
| If lex awards: next 1% ≈ | 108,553,546 |
| cmix-obias (claimed, not on prize page) | 108,492,825 |
| fx3-cmix (unsubmitted, <1%) | 109,735,627 |

enwik8 SOTA: cmix v21 ~14.62 MB. hp ~18.53 MB is paq8f-era quality.

**8 MB champ** (`data/enwik8.8mb`, mem 22): **v61** (`NEST_MOD` +
`PARA_MOD` + `LINE_MOD` + `MIXER_SKIP=32`) identity **1,705,939**, fx2
**1,701,530**, RT PASS. Path from v57: nest −560, para −98, line
−1,070, skip32 −29.

**100 MB champ** (`data/enwik8.fx2man`, mem 26, `SLOT_MAX=31`): **v57**
**18,527,464**, RT PASS. v61 has not been run at 100 MB. Write archives
to `%LOCALAPPDATA%\hp_lab` then copy; OneDrive ate a CYHP header once.

v45 100 MB encode-only 18,633,242; v37 18,671,091 RT PASS.

C++ profile v57 8 MB: wall 704 s; model redundancy mixer vs best-expert
**+1,513,148 B**; PR **3.02** of 77 experts. Mixer is the hole.
Dedicated wiki-domain ContextModels paid (`SENGRP_MOD`); mixer gates
and n-gram twins mostly did not.

---

## Axes (from AXES.md)

hp already has: o1–o6 + PY, sparse/skip + discovery, word + wbi, case /
first-char / sentence streams, column, wiki states, table/link/template,
byte match 3/4/6/10/16, word-keyed match ×3, Hebbian, bracket nest,
pattern class, CTW (off), per-mixer lr / NCL / Hedge-L1.

Still thin vs winners: wiki transform stacks (`fccxt` / `brcxt` /
`ColumnContext`), LSTM mixer, WRT / `payload_lex`, POS / stem streams,
pronoun and partial-sentence rings.

Saturated: extra o1–o6 twins, match-order pile-ons, mixer-gate dilution.
Do not add more without a new axis.

---

## Models — the meta-pattern

Every paying expert is the same bilinear slice, not a new algorithm:

```
logit_i = StateMap( hash( axis_i  ⊗  recent history ) )
p       = squash( v_ctx2ᵀ  W_ctx  x )     x = stretch(logit)
```

`HP_SENGRP_MOD`, `HP_NEST_MOD`, `HP_PARA_MOD`, `HP_LINE_MOD` paid
because each added a **new axis** (sen-group, nest-markup, paragraph
bit, line-kind) into that outer product. Extra o1–o6 twins and match
orders failed because they added a second row on a **saturated** axis.
Mixer *gates* (`HP_SEN_GROUP`, word/hebb/link folds) failed because they
switch which `W` is trained and split the data.

hp is already a system of matrices: sparse hashed tables (not dense
GEMM) plus `MixerNet` — layer-1 `W_j` is `(n_ctx × n_experts)`, layer-2
`v` is `(n_ctx2 × k)`. The v57 profile is the hole in that product:
77 experts, participation **3.02**, mixer-vs-best **+1,513,148 B**.
Rewriting the codec as BLAS / Eigen / floats / GPU cannot pay (illegal
and the lookups are sparse). The matrix move that *can* pay is a
**low-rank factor of W** (H6): `W ≈ A B` with small integer rank,
same Q16 dots as `mixer.hpp` today.

Paying family, remaining headroom: unsaturated table RAM under the
MAX cap, then a new wiki-domain axis as its own CM, then mixer rank.
PPMD mmap, Sequence Memoizer, n-gram twins, and LSTM-in-hp are not
this lab’s next 1% — LSTM / obias live on Track W.

---

## Upgrades still open (from UPGRADES.md)

Proxy-era numbers in that file are dead. Mixer redundancy is the hole
(+1.51 MB at 8 MB). LSTM mixer / bitlstm32 / obias belong on Track W
(cmix-obias claim). Article reorder is fx2-manual *test input*, not an
hp preprocessor. Dictionary / WRT is Track W. GPU / nncp is illegal.
Do not re-run the discovery slot/eval sweep (RECORD H0.2). Do not
rewrite hashed CMs as a dense matrix multiply.

---

## Closed from PLAN2 / PLAN3

Those files are gone. Measurements live in `RECORD.md`. What still binds:

- Axis-purity cuts that drop correlated twins can lose bits at equal
  memory (PLAN2 v4 +479 B on 1 MB; same v4 **−1,531** at 8 MB — H0.4).
  Rank is a compass, not an accept gate.
- 1 MB (`data/enwik8.1mb`) is 84% redirects (H0.5). Never accept or
  reject on it alone. Memory scaling on that slice plateaued at mem 28
  (H0.1, −3 B vs mem 26).
- First bundle keeps: wiki states, word-match-1, bracket, per-mixer lr /
  Hedge-L1 / NCL. First bundle rejects: CTW, stacked word-match-2/3,
  `pat_`+bracket, case-preserving word stream, PPMD, B.3 library.
- Prize-era H0–H2 queue is closed (discovery sweep, mute-in-math, stem
  as extra experts, dict/WRT on 8 MB, pred-gate). Do not reopen from
  memory; pointer is RECORD / the table below.
- Article reorder is accepted as fx2-manual *test input* (H1.2), not an
  hp preprocessor. W1–W3 inventory is written; W4/W5 remain in Track W.

---

## Patterns (from PATTERNS.md)

B.2 online class taxonomy (table / cite / time / infobox / link) stays.
B.1 pattern cache stays. **B.3 shipped pattern library / dict: NO-GO**
— transform win 1,498 B vs storage 10,121 B. Prefer zero-transmitted
Hebbian / wiki-state / class. Revisit only if stored size < 30% of an
enwik8 transform gain.

---

## Protocol, RAM, binaries

One compile flag per binary. Same binary encodes and decodes. No
`float` / `double` / `<cmath>` in `hp/include` or `hp/src`.

**Gate (in order):**

1. Build a new-named exe (never overwrite a binary a long job is using).
2. Compress first 8 MB of `data/enwik8` at `--mem 22`.
3. **Accept if archive bytes drop** vs the current 8 MB champ.
4. Round-trip SHA of the decompressed file vs the input slice.
5. If accepted: stack-check on `data/enwik8.8mb.fx2man` (same mem 22).
6. Champ only: full enwik8 fx2-manual at `--mem 26`, `SLOT_MAX=31`.

| job | typical RSS | rule |
|---|---|---|
| 8 MB leftover, mem 22, `SLOT_MAX` ≤ 35 | ~15 GB | one at a time |
| 100 MB mem 26, `SLOT_MAX=31` | ~38–40 GB | champ confirmation only |
| 100 MB mem 26, `SLOT_MAX=35` | OOM | **do not run** |
| 8 MB leftover **plus** 100 MB | ~15 GB + ~38 GB | **never together** |

**Protect:** `hp_v37.exe`, `hp_v45_m26.exe`, `hp_v55_m26.exe`,
`hp_v57.exe`, `hp_v57_m26.exe`, `hp_v58.exe`, `hp_v59.exe`,
`hp_v60.exe`, `hp_v61.exe`.

Name new builds `hp_vNN.exe` or `hp_<flag>.exe`. 100 MB at mem 26 with
a cap other than the 8 MB default: `hp_vNN_m26.exe` and compile
`-DHP_SLOT_MAX=31` (not 35).

```
g++ -O3 -std=c++17 -I hp/include hp/src/main.cpp -o hp/build/hp_vNN.exe
hp/build/hp_vNN.exe c --mem 22 data/enwik8.8mb hp/build/e8_8mb_vNN.hp
hp/build/hp_vNN.exe d hp/build/e8_8mb_vNN.hp hp/build/e8_8mb_vNN.out
```

Write 100 MB archives to `%LOCALAPPDATA%\hp_lab` then copy into
`hp/build`. Harvest clones stay in `harvest/`; do not rewrite them.

---

## Next tests

One flag. Log every call in `RECORD.md`. After an 8 MB accept, recompile
remaining leftovers on the new champ.

| # | test | how | accept |
|---|---|---|---|
| **H1** | v58 leftover wave | nest / para / line / skip32 | **closed** — all accepted as v58–v61 |
| **H2** | Skip `HP_SENGRP_C0` | `#elif` after `HP_SENGRP_WORD` (already on) | no-op |
| **H3** | 100 MB off OneDrive, `MAX=31` | v61-100 is live (`hp_v61_m26`) | valid CYHP + RT vs 18,527,464 |
| **H6** | Integer low-rank mixer | **landed** `HP_MIXER_RANK=8` in `hp_g_v62meta.exe` with wiki-axis bundle; 8 MB pending until v61-100 frees RAM | bytes drop vs **1,705,939** |
| **H7** | Wiki-axis bundle | `HP_WIKI_AXES`: dedicated state / sent_domain / header / depth CMs (meta-pattern) | stacked in v62meta; same 8 MB gate |
| **H4** | Track W inventory | harvest notes 2026-09-12; **add cmix-obias / fx3** | written gaps only |
| **H5 / W5** | Land a proven H keep on the fork | dedicated wiki-axis CMs (`SENGRP` then `NEST`/`PARA`/`LINE`); not folds | enwik8 bytes down on their pipeline |

H6/H7 are stacked in `hp_g_v62meta.exe` (not run while v61-100 holds RAM).
LSTM / WRT / `payload_lex` / POS / bitlstm32 / obias are Track W.

---

## Track W — winner fork

Hutter’s FAQ: combine with SOTA. Do not invent a third self-extracting
pipeline. Use theirs. Track H will not hit S < 109,685,197 this month.

| id | status |
|---|---|
| W1 | clones already in `harvest/cmix-lex` and `harvest/fx2-cmix` |
| W2–W3 | inventory written 2026-09-12 (H4) |
| W4 | time/RAM envelope still open |
| W5 | after H accept: land dedicated wiki-axis CMs one at a time; not mixer gates or hp-only folds |
| W6 | **cmix-obias** (Freelan, claimed S **108,492,825**): 256-cell LSTM, bitlstm32 fp16 head, PPMd logit prior `g=0.15`. Fork of lex. Do **not** vendor into `hp/` (floats / SSE rcpps). Inventory only. |
| W7 | **fx3-cmix** (Orav, unsubmitted S **109,735,627**, 0.95%): not the fork target; encode.su says not actively worked. PPM shrunk 14 GB → 1.75 GB. |

---

## Track C — Cypha only

Work happens in the Cypha tree. hp already has binary decomposition,
logistic mixing, integer tables, and SHA round-trips. The think-tank
mixer rewrite is a Cypha fix, not an hp port.

**Do (in Cypha):** C1 `n_eval` ≥ 100k + 95% CI; C2 log-loss mixer
gradient `w_i (p_i/p_mix − 1)`; then C3 drop `neural_weight_floor`,
C4 binary logistic mix, C5 hash-pointer match, C6 nibble-bucketed
tables, C7 integer-only RT, C8 external baselines, C9 ablation.

**Do not:** merge Cypha LM / Qt / REST / CUDA into `hp/`; quote 2.664
bpc next to hp SHA numbers; treat the prize as a revenue line.

---

## Do not retest

Pointer is the RECORD heading unless noted. New evidence only.

| item | evidence |
|---|---|
| CTW / case-word / stacked word-match-2/3 / `pat_`+bracket | RECORD A.3 |
| B.3 pattern library / dict | PATTERNS.md; H1.4 +33,369 |
| discovery slot/eval sweep | RECORD H0.2: all reject |
| H2.2 section mute | +15,052 on 8 MB |
| H2.1 stem as extra experts | +288 vs v5 |
| H2.7 pred-gate | +6,928 on 1 MB |
| recency fold / wt3 / fword gate | RECORD v9 leftovers |
| `HP_WORD_GRP` | RECORD v46 leftovers: +1,885 |
| `HP_SEN_GROUP` mixer gate | v46 leftovers: +680 (dilution) |
| `HP_LINK_GRP` | RECORD v54 leftovers: +78 |
| `HP_SENGRP_POS` | RECORD v55 leftovers: 0 |
| `HP_NUM_GRP` | RECORD v55 leftovers: +69 |
| `HP_MATCH_20` | RECORD v57 leftovers: **+97** |
| `HP_HEBB_GRP` | RECORD v57 leftovers: **+90** |
| `HP_SLOT_WORD10` | +43,179 (2^32 sparse) |
| `HP_SLOT_O34G` / `HP_MATCH_GROW2` | −87 noise |
| `HP_MATCH_12` | +100 vs v43 |
| 100 MB at `SLOT_MAX=35` | OOM (this plan, RAM table) |
| GPU / nncp / Qt in hp | illegal for Hutter |
| recursive compression | Hutter FAQ |

---

## Doc map

This is the only plan file. Scoring-board history is `RECORD.md`, not
another PLAN.

| file | role |
|---|---|
| **PLAN.md** (this file) | living board |
| **RECORD.md** | measurement log only |
| **README.md** | build / run |
| HUTTER_RESEARCH.md | literature |
| review-and-thinktank.md | Cypha Track C only |
| harvest/*_notes.md | Track W gaps; not the hp queue |
| MODELS.md AXES.md PATTERNS.md UPGRADES.md hp/UPGRADES.md hp/README.md | archived inventories |
| Stats for Compression/gating-statistics-full-report.md | archived study; hp leftover mixer gates rejected |
