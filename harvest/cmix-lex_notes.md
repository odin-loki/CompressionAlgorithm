# cmix-lex harvest notes

Cloned 2026-08-22 from https://github.com/blahem/cmix-lex (Hutter
submission 29 May 2026, 30-day comment). Not Orav's unpublished fx3-cmix.

## Claimed S

| piece | bytes |
|---|---:|
| archive9 | 109,190,109 |
| packed cmix | 459,938 |
| **S** | **109,650,047** |
| previous L | 110,793,128 |
| 1 − S/L | 1.0317% |

If awarded, next 1% bar ≈ 108,553,546.

## Pipeline (from `changes.md`)

```
enwik9
  → split PHDA9 intro / main / coda
  → reorder article bodies (packaged order file; decoder sorts by page ID)
  → PHDA9 Wikipedia preprocess
  → WRT dictionary
  → payload_lex on structured tail regime 1
  → cmix (fxcm_v26 + disk PPM)
  → archive9
```

Post-WRT stream 586,459,321 B. Tail 45,332,670 B starting at 541,126,651.
Regime 1 starts +13,599,801 into the tail, 243,425 metadata blocks.
`payload_lex` lex-sorts D99 blocks by payload after D99 + first D86a.
R1ORD3 side data = D86a predictor + Lehmer-rank correction, 679,489 B at EOF
(cmix spent 346,948 B coding that tail 0.12%).

## fxcm_v26 vs our ports (still missing in hp)

From `src/models/fxcmv1.cpp` / changes.md:

- dictionary codeword-aware word comparisons
- decoded-word + codeword contexts
- pronoun word type
- sentence memory; separate groups for prose / list / table / wikilink
- partial sentence contexts
- extra stationary + context-map predictors
- **gates that mute** Category / See also / References / Bibliography /
  External links

hp has wiki states, one linkword, thin streams. It does not have POS
streams, sentence memory, section muting, or codeword contexts.

## 2026-09-12 H9 — remaining W gaps (do not rewrite archive9)

August list above is stale. hp v54 now has wiki states, linkword+senword
experts, sentence memory, sen-group folds + dedicated `HP_SENGRP_MOD`,
word-keyed match, SPARSE_UTF8. H2.2 mute / H2.1 stem-experts / H2.7
pred-gate / `HP_SEN_GROUP` mixer gate **rejected** on leftover.

Still missing vs this tree (cmix-lex), by axis:

- **preprocess:** PHDA9 intro/main/coda + wiki transform + packaged
  article-order; WRT; `payload_lex` + `R1ORD3`. This is the cheque
  path. hp does not run it.
- **wiki transform (in-model):** `fccxt`/`brcxt`/`qocxt` stacks;
  `ColumnContext`; FIRSTUPPER/charSwap; `sets()` skip on References /
  See also / Category / math (not dummy mute).
- **mixer:** LSTM ByteMixer (`lstmpr`/`lstmex` into layers); dict-index
  mixer ctx; 18 POS/stream `Mixer1`s. lex `fxcmv1.cpp` does not consume
  `lstmex` as `mxA` ctx (dead extern).
- **match:** word-keyed `{1}`+run-map/DirectHash; `MatchModel2` word
  into byte match; 8 of 10 outer sparse word configs; cmix extra match
  orders.
- **LSTM:** fork-owned ByteMixer. Not an hp keep to land. After
  preprocess.
- **NLP still in fxcm_v26, not in hp:** pronoun type; partial sentence
  `wt3`/`wt4` codeword hashes; four POS `WordsContext` streams; four
  `SentenceContext` rings; codeword-aware compares (needs WRT).

W5: land `HP_SENGRP_MOD` (dedicated sen-group CM) on the fork if it
still wins on their pipeline. Sen-group **folds** (`TAG_GRP`,
`SENWORD_GRP`, `WSTR_GRP`, …) are hp-only. Do not port the rejected
mixer gate.

## Systems

Stable mmap + `MADV_DONTNEED` every 5,000 bytes (not munmap/remap).
`MADV_RANDOM`, `O_NOATIME`, `ftruncate` for `ppm.temp` (~14.68 GB).
RSS 9.7 GB, disk ~20.8 GB, CPU 43.5 h.

## What this means for PLAN3

- P5 (payload_lex) is the last published 1% and only exists *after* P4 WRT.
- X3/X4 (stemmer + sentence groups + section mute) is the in-model gap.
- H2.2 should mute more than math/pre/nowiki — also References / See also.
- Track W is this tree. Do not reimplement archive9.
