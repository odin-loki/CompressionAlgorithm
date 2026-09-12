# MODELS — prize-lineage inventory

Sorted by **axis novelty versus current hp**, then port effort.
`hp` column is what this tree actually has after the 2026-08-22 ports.

Provenance: published source layout of fx2-cmix (`src/models/fxcmv1.cpp`),
cmix, paq8/lpaq, starlit, nncp, phda9/zpaq, plus the ports landed here.
A harvest clone was launched in parallel; rows marked *exact* were checked
against known fx2-cmix identifiers in PLAN.md.

| name | source | context | table | update | hp equivalent | hours | axis | novelty |
|---|---|---|---|---|---|---:|---|---|
| wiki states WIKITABLE/SQUAREOPEN/HTLINK/VERTICALBAR/CURLY + `linkword*2104+j` | fx2-cmix fxcmv1.cpp | markup machine state | reuses tag table | state-map | **ported** `wiki.hpp` → `tag_` | 4 | wiki structure | **new** (was generic XML depth) |
| word-keyed match ×3 | fx2-cmix match models | last 1/2/3 words | smaller ring | length-conditioned counter | **ported** `wordmatch.hpp` | 4 | word-keyed match | **new** |
| word streams ×4 + sparse | fx2-cmix | case / first-char / sentence streams | 2^table | state-map | **ported** `wordstream.hpp` (2 of 10 configs) | 6 | capitalisation, first-char, sentence | **new** |
| bracket / nesting depth 15 | paq8 / fx2 | `()[]{}<>` depths | 256 ctx | state-map | **ported** `bracket.hpp` | 2 | bracket nest | **new** |
| pattern-class model | this repo B.2 | table/cite/time/infobox/link | 2^table | state-map + MDL templates | **ported** `pat_` + `discover.hpp` skip-k | 4 | pattern class | **new** |
| per-mixer learning rates | fx2-cmix 24 mixers 0.0003–0.005 | n/a (mixer) | n/a | per-gate lr | **ported** `mixer.hpp` | 1 | (not an axis) | mixer |
| NCL (Liu & Yao 1999) | literature | n/a | n/a | `err + λ(p_i−p_ens)` | **ported** `ContextModel::update` λ=4/256 | 3 | (anti-collapse) | mixer |
| Hedge over layer-1 | this repo 2.3 | 8 L1 opinions | n/a | fixed-share | **ported** `HP_HEDGE_L1` | 2 | (mixing) | mixer |
| CTW β=1/2 over o1–o6 | CTW / paq | order chain (n0,n1) | none extra | recursive KT | **ported** `HP_CTW` | 2 | tree-source mix | overlap with PY chain |
| English bit prior | paq8hp precedent | c0 unigram | 256 × u16 | init only | **ported** `english.hpp` | 1 | cold-start | tiny / amortises |
| byte match 3,4,6,10,16 | hp already | byte suffix | 2^buf | length counter | `MatchModel` | 0 | match | have |
| orders 1–4,6 + PY | hp already | byte suffix | 2^table | SM + PY | `o1_…o6_` | 0 | byte suffix | have / **saturated** |
| word + word-bigram | hp already | folded word | 2^table | SM | `word_`, `wbi_` | 0 | word, word-bigram | have |
| sparse {1,3}{2,4} | hp already | skip bytes | 2^table | SM | `sp13_`, `sp24_` | 0 | sparse/skip | have |
| column | hp already | byte above | 2^table | SM | `col_` | 0 | 2-D column | have |
| Hebbian A→B | hp already | prev word synapse | 2^table | potentiate/decay | `hebb_` | 0 | association | have |
| discovery 12 masks | hp already | last-16 bitmask | 12 × 2^table | residual MDL | `discover.hpp` | 0 | discovered sparse | have |
| PPMD order-25 | fx2-cmix | unbounded byte | ~14 GB | PPMd | **OFF** `HP_PPMD=0` | 20 | unbounded order | overlap — PY+CTW cover it |
| Sequence Memoizer | Teh | infinite depth PY | large | coagulation | deferred (discount already in `statemap.hpp`) | 16 | unbounded order | overlap |
| nibble buckets + checksum LRU | paq8 | same contexts, fewer collisions | +chk byte | LRU | **OFF** `HP_NIBBLE_BUCKETS=0` | 8 | (estimator) | not an axis |
| richer StateTable cap>20 | paq8/cmix | bit history | 16-bit still | discount | knob `HP_STATE_CAP` default 20 | 2 | (estimator) | not an axis |
| LSTM mixer | cmix | all experts | ~100 MB | BPTT | **last** (2.4) | 40 | neural mix | new but expensive |
| starlit article reorder | starlit | two-pass article cluster | n/a | preprocess | not started (4.2) | 12 | document adjacency | new, two-pass |
| english.dic 412 KB | fx2-cmix | whole stream | 412 KB S1 | subst | `dict.hpp` exists, default off | 2 | stored lexicon | **B.3 / 4.1 NO-GO** until enwik8 |
| nncp transformer | bellard.org/nncp | char LM | 199 M params | SGD | out of rules (GPU / time) | — | neural | disqualified |
| zpaq / phda9 | mattmahoney.net | context + journaling | various | ZPAQ | not a model, an arch | — | journalled ctx | different product |
| UTF-8 continuation | paq8px | lead/cont bits | tiny | SM | not ported | 3 | UTF-8 state | new, low value on enwik (ASCII) |
| numeric-field | paq8 | digit runs, dates | small | SM | partial via `kNumeric` / `kTimestamp` class | 4 | numeric structure | partial |
| paragraph / line pos | paq8 | col, line#, para | small | SM | column have; line# in wiki machine unused as expert | 2 | layout | partial |

Exact fx2-cmix transitions (no C enum; WRT-swapped `#define`s, `fccxt`/`brcxt` stacks, `isParagraph`): `harvest/wiki_states_notes.md`. Clones: `harvest/fx2-cmix`, `harvest/cmix`, `harvest/paq8pxd`, `harvest/starlit`. lpaq/nncp/zpaq are literature-only.

## Top ports by novelty (this session)

1. Wiki states on the existing `tag_` expert  
2. Word-keyed match  
3. Word streams (case / first-char / sentence)  
4. Bracket nesting  
5. Pattern-class expert + skip-k discovery templates  
6. Per-mixer rates / NCL / Hedge-L1 (mixer, not axes)  
7. CTW (likely overlap — A.3 must decide)  
8. PPMD — not built, predicted reject  
9. LSTM — last  
10. starlit reorder — two-pass, later
