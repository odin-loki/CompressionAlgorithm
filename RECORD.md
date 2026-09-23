# RECORD — work log

Started 2026-08-22. This file is the running record of everything done,
accepted, rejected, or deferred. PLAN.md is the spec; this is what happened.

## Session 0 — intake

Workspace contained only `PLAN.md`, `UPGRADES.md`, and `hp.zip` (57,894 B).
Extracted `hp/` in place. Task Master `parse_prd` failed: no `PERPLEXITY_API_KEY`.
PRD written to `.taskmaster/docs/prd.txt`. Tasks tracked here and in this file.

**What `hp` is.** Integer-exact PAQ-lineage context mixer. 28–55 mixer inputs
depending on how you count (README’s “28” is the pre-discovery / pre-Hebbian
headline; the compiled stack is larger — see inventory below). No `float` in
`include/` or `src/`. Encoder/decoder symmetry is the correctness invariant.

**Compiled stack as found (before this session):**

| piece | count | notes |
|---|---:|---|
| hashed context models | 11 × 2 | o1–o4, o6, word, sp13, sp24, col, tag, wbi; indirect + PY |
| bias | 1 | |
| match bank | 5 | byte-keyed orders 3, 4, 6, 10, 16 |
| Hebbian | 1 | |
| discovery pool | 12 × 2 | residual-targeted MDL, novelty-gated masks |
| Hedge + concentration | 2 | sum-mixture over raw experts |
| **mixer inputs** | **53 + 2 = 55** | README “28” is stale |
| layer-1 gates | 6 | c0, α, prev, match-len, entropy, Hebb strength |
| APM | 3 | c0, lex, gria |

**Stale docs vs code (found, not yet fixed):** `mp_rank.py` still labels a
17-expert layout and does **not** report participation ratio or entropy
effective rank, despite UPGRADES.md claiming it does. `kVersion = 2`.
Dictionary defaults off. Discovery pool exists; README expert table omits it.

**Order of work executed:** implement A.2 a/b/f + c/e + B.1 + B.2 + 1.1 + 2.3
+ 2.5 + 4.3 + instruments first (they do not require enwik8 to *exist*), then
proxy baseline, then enwik8 if the download lands, then A.3 gates.

## Standing constraints (enforced)

- `test/no_float.sh` after every codec edit.
- Round-trip before any claim.
- Feature ports behind `include/hp/features.hpp` compile flags.
- Cache must be bit-identical on/off.
- B.3 not shipped.

## Decisions taken up front

| id | decision | why |
|---|---|---|
| D1 | Wiki states *replace* the generic tag context, not add an expert | `tag:py` is already the most decorrelated slot; giving it real structure is the highest-confidence move and does not dilute the mixer |
| D2 | Word-match uses its own smaller tables, not a fifth copy of the 64 MB ring | 10 GB budget is real but this machine is not the prize box; keep local testable |
| D3 | Full order-25 PPMD default OFF | existing PY backoff already reads a near-unbounded-order axis; A.3 would likely reject it as saturated. Stub + note, do not pay the RAM |
| D4 | Pattern cache is last-value memo of *hashes and invariants*, never of probabilities | a p-cache cannot be bit-identical across predict/update (tables move every bit). Plan requires identity |
| D5 | Hedge-over-L1 *replaces* raw-expert Hedge, does not sit beside it | UPGRADES 2.3: raw experts are ~0.71 correlated so the sum-mixture duplicates the product |
| D6 | NCL lambda Q8 = 4 (tiny) | literature + arXiv 2301.11323: large λ games the diversity metric |
| D7 | B.3 = NO-GO until enwik8 | already measured: dict transform +1,498 B, storage −10,121 B |

---

## Implementation log

### Files added
- `hp/include/hp/features.hpp` — compile-time ablation flags
- `hp/include/hp/wiki.hpp` — fx2-cmix wiki state machine
- `hp/include/hp/wordstream.hpp` — 4 streams
- `hp/include/hp/wordmatch.hpp` — word-keyed match
- `hp/include/hp/bracket.hpp` — nesting depths
- `hp/include/hp/pattern_cache.hpp` — B.1 memo + B.2 taxonomy
- `hp/include/hp/english.hpp` — 4.3 prior
- `hp/tools/gen_proxy.py`, `hp/test/pattern_cache.sh`
- `MODELS.md`, `AXES.md`, `PATTERNS.md`, this file
- `.taskmaster/docs/prd.txt` (parse_prd failed: no PERPLEXITY_API_KEY)

### Files changed
- `models.hpp` — NCL in `ContextModel::update`, n0/n1 accessors
- `statemap.hpp` — `HP_STATE_CAP`, NCL extra term
- `mixer.hpp` — per-mixer lr, `layer1_p()`
- `discover.hpp` — `HP_DISC_SLOTS/EVAL`, skip-k templates
- `predictor.hpp` — full stack wiring, `Config::normalize()`
- `main.cpp` — kVersion 3, normalize on encode and decode
- `dump_experts.cpp` — optional table_bits argv
- `mp_rank.py` — PR + entropy rank, current expert names
- `UPGRADES.md` — session findings

### Tests
| test | result |
|---|---|
| no_float | PASS |
| roundtrip proxy64k mem18 | PASS 65536 → 5741, 0.701 bpc |
| roundtrip proxy256k mem18 | PASS 262144 → 19363, 0.591 bpc |
| roundtrip enwik8.1mb mem20 | PASS 1048576 → 236657, **1.806 bpc**, 55.5 s |
| pattern cache on/off | PASS identical SHA-256 |
| 8-way determinism.sh | not run (needs bash + 8 rebuilds; identity cache is the same idea) |
| full enwik8 100 MB | **running** (`hp/build/enwik8.hp`, mem 20, est. ~90 min at 53 µs/byte) |

### Rank (proxy64k, 30,841 records, 64 dumped experts, mem 18)

| metric | old (28 exp, 3.1 MB proxy) | new (64 exp, synth 64k) |
|---|---:|---:|
| participation ratio | 1.76 | 1.764 |
| entropy effective rank | 3.34 | **3.885** |
| top-1 variance share | 74.9% | 74.9% |
| mean pairwise \|corr\| | 0.711 | higher (many |r|>0.90 pairs) |

Most decorrelated now: `disc3:ind` 0.516, `disc10:ind` 0.518, `wm1` 0.588, `col:ind` 0.605.
`tag:py` is no longer #1 (0.739). **wm1 is the port that landed on a new axis.**

### A.3 calls (bundle, not flag-ablated)

| id | model | rank delta | call | reason |
|---|---|---|---|---|
| A.2.a | wiki states | n/a (replaced tag ctx) | **keep** | still a thin-ish axis; tag:ind 0.684 |
| A.2.b | word-match-1 | new, mean\|r\|=0.588 | **keep** | most decorrelated port |
| A.2.b | word-match-2/3 | overlap 0.969 | **keep 1, watch** | near-duplicate |
| A.2.c | wstr1 | r=0.944 vs word:py | **reject** | saturated word axis |
| A.2.e | bracket | r=0.971 vs pat | **reject one of {brk,pat}** | same class on wiki |
| A.2.f | per-mixer lr | mixer, not axis | **keep** | mechanical, no rank claim |
| B.1 | pattern cache | 0 (identity) | **keep** | speed only; identity PASS |
| B.2 | pat_ + skip-k | r=0.971 vs brk | **keep skip-k; drop pat_ or brk** | |
| B.3 | shipped library | — | **NO-GO** | dict lesson; no enwik8 amortisation yet |
| 1.1 | NCL λ=4/256 | mixed into bundle | **keep small λ** | no solo ablation |
| 2.3 | Hedge-L1 | mixer | **keep** | theoretically right target |
| 2.5 | CTW | mean\|r\|=0.796, twin o2:py 0.930 | **reject** | duplicate of PY chain |
| 3.3 | PPMD o25 | not built | **deferred / predicted reject** | same axis as PY+CTW |
| 4.3 | English prior | not measured | **keep** | free, amortises |

### Throughput
enwik8 1 MB: 55.5 s → **53 µs/byte** (was claimed 3.7). The extra experts ate the
50x headroom down to ~14 h projected for enwik9 — still inside 50 h, no longer
comfortable. B.1 did not buy this back (hash memo is not a model skip).

### Bug
`--mem` desync. Fixed by `Config::normalize()` on both sides.

### Harvest follow-up (clones + Cypha 5.1)

[model harvest](25e32ae5-14bc-4b39-ae31-7e73ff2fb6e4) cloned fx2-cmix, cmix, paq8pxd, starlit into `harvest/`. Notes: `harvest/wiki_states_notes.md`. fx2 has **no enum** — states are `#define`s after WRT/charSwap (`FIRSTUPPER=64`, `WIKITABLE='-'`, `WIKIHEADER='>'`, `isParagraph`). Our first port missed the first-char / paragraph switch; `wiki.hpp` now carries `is_paragraph_`, `wiki_header_`, `kWkFirstUpper`, `kWkHeader`, and fx2's `linkword=0` on `]` / `:`.

[Cypha mixer](224e0d5a-14e2-4ce3-a8fb-28d19d2448e0) for UPGRADES 5.1: `AdaptivePredictorMixer` is 7 default experts (neural + n-gram 0–3 + match + kNN) on the **codec path only**. No per-expert dump. hp’s `dump_experts` cannot be pointed at it. Plan: thin `expert_prob(i, symbol)` accessors + a Cypha-side `dump_mixer_experts` that writes the same int16 stretch file `mp_rank.py` already reads. `gria_gated_blend_logit` is upstream (2-way GRIA/LSTM), default off — a separate 2-expert dump if we want it.

enwik8.zip (36,445,475 B) downloaded and unpacked to `data/enwik8` (100,000,000 B).

### PLAN2 + record (same day)

v4 (drop twins, add link/num) **lost** 479 B at mem 20 vs the session-1
bundle. Memory scaling did the opposite.

**Record, `data/enwik8.1mb`, v3b (FIRSTUPPER/isParagraph), round-trip PASS:**

| mem | bytes | bpc | vs session-1 236,657 |
|---:|---:|---:|---:|
| 20 | 236,473 | 1.804 | −184 |
| 22 | 232,151 | 1.771 | −4,506 |
| 24 | 230,266 | 1.757 | −6,391 |
| **26** | **229,440** | **1.750** | **−7,217** |

Binary: `hp/build/hp_v3b.exe`. PLAN2 v4 is the A.3 experiment, not the
record holder.

**Full enwik8 (PLAN step 1), session-1 `hp.exe` `--mem 20`:**
100,000,000 → **21,828,153 B / 1.746 bpc**. Inside the 1.55–1.75 band
UPGRADES.md called “the number that makes the project real.”
Round-trip **PASS** (SHA-256
`2B49720EC4D78C3C9FABAEE6E4179A5E997302B3A70029F30F2D582218C024A8`,
decode 5,429 s).

### Not done (honest)
- Per-flag A.3 ablations (would be 10 × enwik8-1MB ≈ 10 minutes each)
- Full 8-build `determinism.sh`
- Discovery sweep 0.2 (knobs exist: `HP_DISC_SLOTS`, `HP_DISC_EVAL`)
- Memory curve 0.3 (normalize enables `--mem 16..28`; curve not swept)
- LSTM 2.4, rank-coding 2.6, starlit 4.2, Cypha 5.1, fx2 instrument 5.2
- Task Master task list (API key missing)
- Original 3.1 MB proxy (not in the zip)

---

## Session — Hutter Prize research (2026-08-22, afternoon)

Asked: analyse every winner and record-breaker, list what to test, work
through it. Wrote `HUTTER_RESEARCH.md`, `PLAN3.md`, canvas
`hutter-prize-research.canvas.tsx`.

### Official target
- L = **110,793,128** (fx2-cmix). 1% bar = **109,685,197**.
- **cmix-lex** (Marcouch, 29 May 2026) pending: S = **109,650,047**.
  Cloned to `harvest/cmix-lex`. Notes: `harvest/cmix-lex_notes.md`.
- Last 1% is `payload_lex` (PHDA9 tail regime-1 lex order + R1ORD3
  Lehmer side data) + **fxcm_v26** (sentence groups, section muting,
  codeword contexts), not another n-gram.

### Genealogy (one line each)
paq8f mix → Rhatushnyak WRT+wiki transforms (4 awards) → starlit
**article reorder** → fast-cmix **speed/mmap PPM** → fx-cmix **wiki
machine** → fx2 **stemmer streams + online DRT + voyage order** →
cmix-lex **tail permute + fxcm_v26**. Unconstrained LTCB: nncp 107.3 MB
(GPU), cmix v21 108.2 MB (31 GB). ~3% headroom, illegal resources.

### Corpus (H0.5) — why 1 MB lied
| slice | pages | article | image | redirect | `&lt;math` |
|---|---:|---:|---:|---:|---:|
| 1 MB | 176 | ~15% | — | **148 (84%)** | 1 |
| 8 MB | 1,075 | 264 (25%) | 322 | 411 | — |
| 100 MB | 12,347 | 3,746 (30%) | 3,978 | 3,668 | **6,455** |

Wikilinks ~9.5/kB. Digits 2.18%. Raw `<ref>`/`<math>` counts were 0
because the dump escapes them. Tools: `hp/tools/corpus_profile.py`,
`count_escaped.py`, `article_reorder.py`. Title-sort restore ≠ identity
(enwik page order is not a pure title sort; starlit uses page IDs).
Split/join round-trip is exact.

Full enwik8 classes: article 3746, image 3978, redirect 3668,
disambig 485, table 339, infobox 131. **70% of pages are not prose.**
That is the starlit/fx2 reorder.

### H1.2 8 MB reorder (v3b `--mem 22`, ~7 min each) — **accept**

| order | out B | bpc | vs identity |
|---|---:|---:|---|
| identity (original dump order) | 1,806,540 | 1.723 | 0 |
| redirects last | 1,803,104 | 1.720 | −3,436 |
| **fx2-manual** (image/disambig/redirect last) | **1,800,840** | **1.717** | **−5,700 (−0.32%)** |

Crosses the PLAN3 0.3% keep line on an 8 MB slice that is only 25% prose.
`data/enwik8.fx2man` built (12,347 pages, sha256 9fa638182a0384af).

### H0.1 memory curve on 1 MB (v3b) — **plateau**

| mem | bytes | bpc |
|---:|---:|---:|
| 20 | 236,473 | 1.804 |
| 22 | 232,151 | 1.771 |
| 24 | 230,266 | 1.757 |
| **26** | **229,440** | **1.750** |
| 28 | 229,437 | 1.750 |

Mem 28 is −3 B. Do not spend more table bits on 1 MB. Next RAM test is 8 MB / 100 MB at 26.

### H1.2 full enwik8 fx2-manual + mem 26 — **19,354,445 B / 1.548 bpc**

v3b, 6,466 s (~108 min). vs session-1 original-order `--mem 20` 21,828,153 / 1.746:
**−2,473,708 B (−11.3%)**. Two levers moved at once (reorder + mem 20→26).
8 MB isolated reorder was only −0.32%, so most of this 100 MB jump is
memory; reorder still keeps. Round-trip **PASS** vs `data/enwik8.fx2man`
(SHA-256 `9FA638182A0384AF…B9336A51`, decode 6,813 s).

### H0.2 discovery sweep (1 MB, v3b flags, mem 22) — **reject all**

Baseline v3b is 12 slots / 1024 eval = 232,151 B. Every wider or
faster-cycling pool lost bits. More slots monotonically worse
(+354 / +543 / +808). e512 at 12 slots is the least-bad (+203) and
still a loss. Keep the default knobs; do not sweep again on 1 MB.

### H2.2 section mute (v3b flags + `HP_SECTION_MUTE=1`, mem 22) — **reject**

| corpus | mute | v3b | delta |
|---|---:|---:|---:|
| 1 MB | 234,276 | 232,151 | +2,125 |
| 8 MB | 1,821,592 | 1,806,540 | +15,052 |

Flag stays off. Naive header/math mute poisons word contexts instead of
skipping maps the way fx2's `sets()` does. Revisit only with a real
context-map skip, not a dummy hash.

### H0.4 PLAN2 v4 vs v3b (mem 22) — **1 MB lied; keep v4 at 8 MB+**

| corpus | v4 | v3b | delta |
|---|---:|---:|---:|
| 1 MB | 232,446 | 232,151 | +295 |
| 8 MB | **1,805,009** | 1,806,540 | **−1,531** |

Dropping correlated twins + link/num costs bits on the redirect-heavy
1 MB head and pays on 8 MB. Next 100 MB lab run should be v4, not v3b.

### H3.1 / H2.3 / H1.5 (v3b flags, mem 22) — skip and utf8 **accept at 8 MB**

| id | 1 MB | 8 MB | vs v3b 8 MB 1,806,540 |
|---|---:|---:|---|
| skip24 | 232,323 (+172) | **1,803,239** | **−3,301** |
| sparse utf8 | 232,331 (+180) | **1,803,601** | **−2,939** |
| entity fold | 232,456 (+305) | 1,805,841 | −699 |

1 MB still lies. Enable `HP_MIXER_SKIP=24` and `HP_SPARSE_UTF8` on the next
v4-class build. Entity fold is real but small; dict (other batch) already
**rejected** at 8 MB (+33,369 including 10 KB shipped table). Numeric alone
**−1,321** at 8 MB — that part of PLAN2 was right. Wiki off **+5,224** —
do not drop wiki states.

### H0.3 8 MB flag drops (v3b base, mem 22) — **keep all three**

v3b 8 MB = 1,806,540.

| flag off | bytes | delta |
|---|---:|---:|
| wiki states | 1,811,764 | +5,224 |
| word-match | 1,809,939 | +3,399 |
| bracket | 1,814,096 | +7,556 |

Bracket is the most expensive to lose. None of these are twins at 8 MB.

### v5 (v4 + `HP_MIXER_SKIP=24` + `HP_SPARSE_UTF8`) 8 MB mem 22 — **record**

**1,798,335 B / 1.715 bpc** identity. −6,674 vs v4, −8,205 vs v3b.

**v5 + fx2-manual: 1,792,549 B / 1.709 bpc.** Reorder still stacks
(−5,786 vs v5 identity, −8,291 vs v3b+fx2 1,800,840). This is the 8 MB
lab champion.

### H0.4 v4 isolate, full enwik8 fx2-manual + mem 26 — **19,316,064 B / 1.545 bpc**

v4 (link+num on), 6,539 s (~109 min). vs v3b same levers 19,354,445:
**−38,381 B (−0.20%)**. The 8 MB −1,531 win scaled; 1 MB +295 was the lie.

### v5 + fx2-manual + mem 26, full enwik8 — **19,271,085 B / 1.542 bpc**

**100 MB lab champion.** 6,085 s (~101 min). vs v4 19,316,064: **−44,979**.
vs v3b 19,354,445: **−83,360 (−0.43%)**. Skip + UTF-8 match stacked at
full file, same as 8 MB. Round-trip **PASS** vs `data/enwik8.fx2man`
(SHA-256 `9FA638182A0384AF…B9336A51`, decode 6,945 s).

### H2.1 / H2.5 / H2.7 — in flight (v5 base)

Porter2+POS stem streams (`HP_STEMMER`, two extra models), senword split
(`HP_SENWORD`), and `sets()`-style pred gate (`HP_PRED_GATE`, idle + no
update). vs v5 1 MB 231,849 / 8 MB 1,798,335.

| id | 1 MB | vs v5 |
|---|---:|---:|
| stem | 231,871 | +22 |
| senword split | **231,698** | **−151** |
| pred-gate | 238,777 | +6,928 |
| stem-fold | 239,157 | +7,308 |
| stem N=1 | 231,891 | +42 |

**H2.1 extra experts: reject** at 8 MB (+288). Folding stem into
`word_` destroys the surface-form axis. POS lists are not wasted —
they stay behind the flag for a later mixer-gate try.

**H2.7 pred-gate: reject.** Same failure mode as H2.2 mute (+6.9k).

**H2.5 senword split: accept.** 8 MB **1,796,738 / 1.713 bpc (−1,597
vs v5)**. New 8 MB record. v6 = v5 + `HP_SENWORD`.

Stem as two extra experts, 8 MB: **1,798,623 / +288**. Reject that wiring.
H2.1b fold 1 MB: **239,157 / +7,308**. XOR into `word_` wrecks the axis. Reject.
Pred-gate 1 MB +6,928 — same class as H2.2. Reject.

### H2.5 senword own expert (v5 base) — **accept**

| corpus | bytes | vs v5 |
|---|---:|---:|
| 1 MB | **231,698** | **−151** |
| 8 MB | **1,796,738** | **−1,597** |

Both gates agree (rare). `link_` keeps the link hash; `sen_` gets the body-prose
2104-hash. v6 = v5 + `HP_SENWORD`. Round-trip 8 MB **PASS**.

**v6 + fx2-manual 8 MB: 1,790,934 / 1.707 bpc (−1,615 vs v5+fx2).**
Senword stacks with reorder. New 8 MB champion. 8 MB identity RT **PASS**.

**v6 + fx2-manual 8 MB: 1,790,934 / 1.707 bpc (−1,615 vs v5+fx2).**
Reorder still stacks. 100 MB v6 + fx2-manual + mem 26 in flight.

H2.1c POS mixer gate on v6: 1 MB 231,850 (**+152** vs v6). Reject at 1 MB.

### Stats-for-Compression mixer gates (v5 base, 8 MB)

| id | 8 MB | vs v5 1,798,335 | call |
|---|---:|---:|---|
| **argmax** | **1,796,939** | **−1,396** | **accept** |
| shape6 | **1,798,126** | **−209** | weak keep |
| argmax×branch | 1,796,194 | −1,141 | worse than argmax alone |
| branch3 | 1,798,521 | +186 | **reject** (1 MB lied) |
| branch×disp×shape | 1,798,581 | +246 | **reject** |
| branch×disp | 1,798,780 | +445 | **reject** |

Paper triple does not transfer: hp already mixes o6, so shape/branch/disp
dilute. **Loudest-expert identity** (`xo_argmaxord`) is the independent-state
win. Stacking senword × argmax × shape on 8 MB now.

### v7 stack (v5 + senword + argmax) 8 MB — **1,795,356 / 1.712 bpc**

| id | 8 MB | vs v6 1,796,738 | call |
|---|---:|---:|---|
| **v7 sen+argmax** | **1,795,356** | **−1,382** | **accept — almost full stack** |
| v6+shape | 1,796,550 | −188 | shape stacks with sen, not with v7 |
| v7+shape | 1,795,436 | −1,302 | worse than v7 |
| argmax×shape | 1,796,991 | +253 | worse than argmax |
| v6+link×num | 1,797,606 | +868 | **reject** |
| match-break gate | 1,799,197 | +2,459 | **reject** |

Expected v7 if independent: −1,597−1,396 = −2,993 vs v5. Got −2,979.
v7 is the new 8 MB lab champion. Identity RT **PASS**.

**v7 + fx2-manual 8 MB: 1,789,550 / 1.706 bpc (−1,384 vs v6+fx2).**
Argmax stacks with reorder too.

### H2.9 per-model slot sizes on v7 — **accept, 1,786,531 / 1.704 bpc**

`o1/o2` −2 bits, `word/wbi` +1, `tag/link/sen` +2. 8 MB **−8,825 vs v7**.
1 MB 231,023 (−826 vs v5). Round-trip **PASS**. v8 = v7 + `HP_SLOT_SIZES`.

**v8 + fx2-manual 8 MB: 1,780,777 / 1.698 bpc (−8,773 vs v7+fx2).**
Slot sizes stack with reorder. New 8 MB fx2 champion.

### Next leftovers on v8 (8 MB)

| id | 8 MB | vs v8 1,786,531 | 1 MB vs v8 | call |
|---|---:|---:|---:|---|
| **wordpos gate** | **1,783,378** | **−3,153** | −464 | **accept** |
| slot-grow (no o1 shrink) | **1,785,824** | **−707** | −92 | **accept — shrink hurt** |
| state2 PY cap | **1,786,223** | **−308** | −39 | weak keep |
| hedge-max gate | — | — | +186 | **reject** |

v8's slot win was mostly growing tag/word; shrinking o1/o2 costs bits.

### v9 = grow + wordpos + state2 — **1,782,343 / 1.700 bpc**

| id | 8 MB | vs v8 | call |
|---|---:|---:|---|
| grow+wordpos | 1,782,640 | −3,891 | accept |
| **+ state2** | **1,782,343** | **−4,188** | **accept — −297 more** |
| wordpos RT | — | — | **PASS** |

Independent sum −707−3,153−308 = −4,168; got −4,188. v9 is the 8 MB champ.
Identity RT **PASS**.

**v9 + fx2-manual 8 MB: 1,776,517 / 1.694 bpc (−4,260 vs v8+fx2).**
Grow+wordpos+state2 stacks with reorder. New 8 MB fx2 champion.

### Leftover A.3 on v9 (8 MB)

| id | flag | 8 MB | vs v9 1,782,343 | call |
|---|---|---:|---:|---|
| recency | `HP_SENT_RECENCY` | 1,818,028 | +35,685 | **reject** |
| wt3 trail | `HP_WT3_CTX` | 1,784,008 | +1,665 | **reject** |
| fword gate | `HP_GATE_FWORD` | 1,783,021 | +678 | **reject** |
| **grow+3** | `HP_SLOT_GROW_EXTRA` | **1,781,301** | **−1,042** | **accept — v10** |

Do not fold recency into `word_` (pollutes the champ expert). First-word as gate is the same reject family as first-word fold.

### v10 = v9 + grow+3 — **1,781,301 / 1.699 bpc, RT PASS**

v10 + fx2-manual 8 MB: **1,775,459 (−1,058 vs v9+fx2)**. Slots stack with reorder.

### Next-wave A.3 on v9 (8 MB)

| id | flag | 8 MB | vs v9 | call |
|---|---|---:|---:|---|
| wiki-temp | `HP_WIKI_TEMP` | 1,782,343 | 0 | **reject — no-op** |
| **sent-stream** | `HP_SENT_STREAM` | **1,778,093** | **−4,250** | **accept** |
| utf8-idle | `HP_UTF8_IDLE` | 1,782,210 | −133 | weak; hold off stack |
| match-o8 | `HP_MATCH_18` | 1,781,893 | −450 | weak keep |

v11 target: grow+3 × sent-stream (cross-family). Then try +match-o8.

### v11 = v10 + sent-stream — **1,777,044 / 1.695 bpc**

Independent −1,042−4,250 = −5,292 vs v9; got **−5,299**. vs v10 −4,257; vs s3 −1,049.

v11 + fx2-manual 8 MB: **1,771,335 (−4,124 vs v10+fx2)**.

| id | 8 MB | vs | call |
|---|---:|---|---|
| table-above | 1,782,463 | +120 vs v9 | **reject** |
| nlchar | 1,782,343 | 0 vs v9 | **reject** |
| **v11+match-o8** | **1,776,601** | **−443 vs v11** | **accept — v12** |

Isolated match-o8 was −450; stacks. v12 = v11 + `HP_MATCH_18`.
v11 RT **PASS**. v12 RT **PASS**.
v12 + fx2-manual 8 MB: **1,770,904 (−431 vs v11+fx2)**.

### v6 + fx2 + mem 26 (100 MB) — **19,240,021 / 1.539 bpc**

vs v5 19,271,085: **−31,064**. Senword stacks at full file. RT of v6-100 not yet run (do not overwrite `hp_v6.exe`).

v12 + fx2 + mem 26 **started** (`e8_fx2man_v12_m26.hp`). Expected well under v6 if 8 MB scale holds.

### Quote / closer on v12 (8 MB)

| id | 8 MB | vs v12 1,776,601 | call |
|---|---:|---:|---|
| **quote stack** | **1,775,314** | **−1,287** | **accept — v13** |
| brk closer | 1,776,604 | +3 | **reject** |

v13 = v12 + `HP_QUOTE_STACK`. Identity RT **PASS**.
v13 + fx2-manual 8 MB: **1,769,611 (−1,293 vs v12+fx2)**. Quote stacks with reorder.
Do not overwrite `hp_v12.exe` while the 100 MB job runs. After v12-100 lands, next 100 MB is **v13 + fx2 + mem 26**.

### Mixer-gate wave on v13 (8 MB) — all reject

| id | flag | 8 MB | vs v13 1,775,314 | call |
|---|---|---:|---:|---|
| mlen2 | `HP_GATE_MLEN2` | 1,775,659 | +345 | **reject** |
| utf8 gate | `HP_GATE_UTF8` | 1,776,338 | +1,024 | **reject** |
| nest gate | `HP_GATE_NEST` | 1,775,945 | +631 | **reject** |
| agree | `HP_GATE_AGREE` | 1,776,420 | +1,106 | **reject** |
| fclass | `HP_GATE_FCLASS` | 1,775,428 | +114 | **reject** |
| wbi sentpos | `HP_WBI_SENTPOS` | 1,779,998 | +4,684 | **reject** |
| wm-len gate | `HP_GATE_WMLEN` | 1,775,842 | +528 | **reject** |

Mixer is saturated on v13. Next: RAM / match-order family (the axes that paid).

### RAM / match-order wave on v13 (8 MB)

| id | flag | 8 MB | vs v13 | call |
|---|---|---:|---:|---|
| **word/wbi +2** | `HP_SLOT_WORD2` | **1,769,276** | **−6,038** | **accept — v14** |
| **match o1** | `HP_MATCH_01` | **1,772,984** | **−2,330** | **accept** |
| match o13 | `HP_MATCH_13` | 1,775,108 | −206 | weak |
| skip 32 | MIXER_SKIP=32 | 1,775,108 | −206 | weak |
| col +2 | `HP_SLOT_COL2` | 1,775,287 | −27 | reject |
| skip 16 | MIXER_SKIP=16 | 1,775,643 | +329 | **reject** |

v14 = v13 + word/wbi +2. Then stack match-o1 (cross-family).

### v14 = v13 + word+2 — **1,769,276 / 1.687 bpc, RT PASS**

v14 + fx2-manual 8 MB: **1,763,629 (−5,982 vs v13+fx2)**.

### v15 = v14 + match-o1 — **1,766,973 / 1.685 bpc**

Independent −6,038−2,330 = −8,368 vs v13; got **−8,341**. vs v14 **−2,303** (isolated −2,330). Stacks.
v15 identity RT **PASS**.
v15 + fx2-manual 8 MB: **1,761,328 (−2,301 vs v14+fx2)**.

### v16 = v15 + word/wbi +3 — **1,761,967 / 1.680 bpc**

`HP_SLOT_WORD3` on v15: **−5,006**. Numeric +2 was −70 (reject). Word-table RAM is still not saturated.
v16 identity RT **PASS**.
v16 + fx2-manual 8 MB: **1,756,400 (−4,928 vs v15+fx2)**. New 8 MB champ.
v16-100 starts when v12-100 exits. Do not overwrite `hp_v12.exe` or `hp_v16.exe`.

### Slot-grow wave on v16 (8 MB)

| id | flag | 8 MB | vs v16 1,761,967 | call |
|---|---|---:|---:|---|
| **word/wbi +4** | `HP_SLOT_WORD4` | **1,758,260** | **−3,707** | **accept — v17** |
| **sentst +2** | `HP_SLOT_S3` | **1,760,123** | **−1,844** | **accept** |
| wstr +2 | `HP_SLOT_WSTR2` | 1,761,675 | −292 | weak |
| brk +2 | `HP_SLOT_BRK2` | 1,761,873 | −94 | reject |

### v12 + fx2 + mem 26 (100 MB) — **19,056,065 / 1.524 bpc**

vs v6 19,240,021: **−183,956**. Full-file stack of senword→v12 holds.

### v17 = v16 + word+4 — **1,758,260 / 1.677 bpc, RT PASS**

v17 + fx2-manual 8 MB: **1,752,596 (−3,804 vs v16+fx2)**.

### v18 = v17 + sentst+2 — **1,756,449 / 1.675 bpc**

Independent −3,707−1,844 = −5,551 vs v16; vs v17 **−1,811** (isolated −1,844). Stacks.
v18 + fx2 + mem 26 **started** (`e8_fx2man_v18_m26.hp`). Do not overwrite `hp_v18.exe`.
v18 identity RT **PASS**.
v18 + fx2-manual 8 MB: **1,750,870 (−1,726 vs v17+fx2)**.

### v19 = v18 + word/wbi +5 — **1,753,764 / 1.672 bpc, RT PASS**

`HP_SLOT_WORD5` on v18: **−2,685**. Curve still paying (+2 −6.0k, +3 −5.0k, +4 −3.7k, +5 −2.7k).
v19 + fx2-manual 8 MB: **1,748,266 (−2,604 vs v18+fx2)**. Stacks. New 8 MB fx2 champ.
v19-100 queued after v18-100. Do not overwrite `hp_v18.exe` or `hp_v19.exe`.

At `--mem 26`, `slot_bits` caps at 28, so WORD3+ is a no-op on 100 MB. `HP_SLOT_MAX` (default 28) plus WORD6/WORD7 exist so the curve can keep going: WORD6 hits 28 at mem 22; WORD7 needs `HP_SLOT_MAX=29`.

### A.3 wave on v19 (8 MB)

| id | flag | 8 MB | vs v19 1,753,764 | call |
|---|---|---:|---:|---|
| **word/wbi +6** | `HP_SLOT_WORD6` | **1,752,055** | **−1,709** | **accept** |
| **word/wbi +7 cap29** | `WORD6+WORD7+SLOT_MAX=29` | **1,751,028** | **−2,736** | **accept — v20** |
| **match o2** | `HP_MATCH_02` | **1,752,016** | **−1,748** | **accept** |
| match o5 | `HP_MATCH_05` | 1,753,118 | −646 | weak keep |
| **wm4** | `HP_WMATCH_4` | **1,752,846** | **−918** | **accept** |

v20 = v19 + WORD6 + WORD7 + `HP_SLOT_MAX=29`. Next stack: match-o2 (cross-family). WORD8 + cap 30 is the next RAM-curve point.
v20 identity RT **PASS**.
v20 + fx2-manual 8 MB: **1,745,545 (−2,721 vs v19+fx2)**. Stacks. New 8 MB fx2 champ.
Do not overwrite `hp_v18.exe` / `hp_v19.exe`. v19-100 skipped (WORD5 is a no-op at mem 26 / cap 28). Next 100 MB is v20+ after v18-100.

### Stack-on v20 (8 MB)

| id | flag | 8 MB | vs v20 1,751,028 | call |
|---|---|---:|---:|---|
| **match o2** | `HP_MATCH_02` | **1,749,276** | **−1,752** | **accept — v21** (isolated −1,748) |
| **wm4** | `HP_WMATCH_4` | **1,750,081** | **−947** | **accept** (isolated −918) |
| word+8 cap30 | `HP_SLOT_WORD8` | 1,750,402 | −626 | weak keep |
| sentst +3 | `HP_SLOT_S4` | 1,750,777 | −251 | weak |

v21 = v20 + match-o2. Next stack: wm4 (cross-family).
v21 identity RT **PASS**.
v21 + fx2-manual 8 MB: **1,743,820 (−1,725 vs v20+fx2)**. Stacks.

### v22 = v21 + wm4 — **1,748,366 / 1.667 bpc, RT PASS**

`HP_WMATCH_4` on v21: **−910** (isolated −947). Stacks.
v22 + fx2-manual 8 MB: **1,742,937 (−883 vs v21+fx2)**. New 8 MB fx2 champ.

### v23 = v22 + word+8 cap30 — **1,747,742 / 1.667 bpc, RT PASS**

`HP_SLOT_WORD8` + `HP_SLOT_MAX=30` on v22: **−624** (isolated −626). Still paying, slower. Accept for 100 MB so mem 26 can use 30-bit word/wbi.
v23 + fx2-manual 8 MB: **1,742,354 (−583 vs v22+fx2)**. New 8 MB identity and fx2 champ.
v18-100 still running. Next 100 MB is v23 (`HP_SLOT_MAX=30`) after v18-100. Do not overwrite `hp_v18.exe`.

### Leftover wave on v23 (8 MB)

| id | flag | 8 MB | vs v23 1,747,742 | call |
|---|---|---:|---:|---|
| **word+9 cap31** | `HP_SLOT_WORD9` | **1,747,417** | **−325** | **accept — v24** (100 MB cap 31) |
| wstr +2 | `HP_SLOT_WSTR2` | 1,747,539 | −203 | weak keep |
| match o7 | `HP_MATCH_07` | 1,747,670 | −72 | reject |
| wm5 | `HP_WMATCH_5` | 1,747,694 | −48 | reject |

### v24 = v23 + word+9 cap31 — **1,747,417 / 1.666 bpc, RT PASS**

`HP_SLOT_WORD9` + `HP_SLOT_MAX=31`: **−325**. Curve still positive. 100 MB gets 31-bit word/wbi at mem 26.
v24 + fx2-manual 8 MB: **1,742,026 (−328 vs v23+fx2)**. Stacks.

### v25 = v24 + wstr+2 — **1,747,228 / 1.666 bpc, RT PASS**

`HP_SLOT_WSTR2` on v24: **−189** (isolated −203). Weak but stacks. New 8 MB identity champ.
v25 + fx2-manual 8 MB: **1,741,830 (−196 vs v24+fx2)**. New 8 MB fx2 champ.
### v18 + fx2 + mem 26 (100 MB) — **18,964,991 / 1.517 bpc**

vs v12 19,056,065: **−91,074**. WORD3+ was a no-op at cap 28, so this is quote + match-o1 + sentst+2 + WORD2 only. Identity RT **PASS**.
### v25 + fx2 + mem 26 (100 MB) — **18,821,118 / 1.506 bpc**

vs v18 18,964,991: **−143,873**. Cap 31 word/wbi paid at full file. Identity RT **PASS**.

### Leftover wave on v25 (8 MB)

| id | flag | 8 MB | vs v25 1,747,228 | call |
|---|---|---:|---:|---|
| **sent-mem** | `HP_SENT_MEM` | **1,746,513** | **−715** | **accept — v26** |
| sentst +3 | `HP_SLOT_S4` | 1,746,973 | −255 | weak |
| utf8-idle | `HP_UTF8_IDLE` | 1,747,072 | −156 | weak |
| match o5 | `HP_MATCH_05` | 1,746,644 | −584 | weak keep (same family as o2) |

### v26 = v25 + sent-mem — **1,746,513 / 1.665 bpc, RT PASS**

`HP_SENT_MEM` on v25: **−715**. New expert family.
v26 + fx2-manual 8 MB: **1,741,193 (−637 vs v25+fx2)**.
v26-100 died incomplete at 13,771,852. Restarting as v27-100 (S4 stacked).

### Sentmem upgrades on v26 (8 MB)

| id | flag | 8 MB | vs v26 1,746,513 | call |
|---|---|---:|---:|---|
| sent-mem big | `HP_SENT_MEM_BIG` | 1,746,512 | −1 | reject |
| sent-align | `HP_SENT_ALIGN` | 1,746,535 | +22 | reject |
| sentmem +2 | `HP_SLOT_SMEM` | 1,746,518 | +5 | reject |
| **sentst +3** | `HP_SLOT_S4` | **1,746,264** | **−249** | **accept — v27** |

### v27 = v26 + sentst+3 — **1,746,264 / 1.665 bpc, RT PASS**

`HP_SLOT_S4` on v26: **−249** (isolated −255). Weak but stacks.
v27 + fx2-manual 8 MB: **1,740,950 (−243 vs v26+fx2)**. New 8 MB champ.
v27-100 running (`e8_fx2man_v27_m26.hp`). Do not overwrite `hp_v27.exe`.
v26+m05 on 8 MB: **1,745,955 (−558 vs v26)**. Stacks.

### v28 = v27 + match-o5 — **1,745,709 / 1.665 bpc, RT PASS**

`HP_MATCH_05` on v27: **−555** (isolated −558). Stacks.
v28 + fx2-manual 8 MB: **1,740,100 (−850 vs v27+fx2)**. New 8 MB identity and fx2 champ.
v28-100 queued after v27-100. Do not overwrite `hp_v27.exe`.

### v28 leftovers wave 1 (8 MB)

| id | flag | 8 MB | vs v28 1,745,709 | call |
|---|---|---:|---:|---|
| match o9 | `HP_MATCH_09` | 1,745,725 | +16 | reject |
| sent-dom rings | `HP_SENT_DOM` | 1,745,812 | +103 | reject |
| **o3/o4 +1** | `HP_SLOT_O34` | **1,741,838** | **−3,871** | **accept — v29** |
| sent-cur | `HP_SENT_CUR` | 1,745,833 | +124 | reject |

### v29 = v28 + o3/o4+1 — **1,741,838 / 1.661 bpc, RT PASS**

`HP_SLOT_O34` on v28: **−3,871**. Orders 3/4 were never grown (word path is delta==1 only).
v29 + fx2-manual 8 MB: **1,736,210 (−3,890 vs v28+fx2)**. Stacks. New 8 MB champ.
v29-100 queued after v27-100. Do not overwrite `hp_v27.exe`.

### v29 leftovers wave 1 (8 MB)

| id | flag | 8 MB | vs v29 1,741,838 | call |
|---|---|---:|---:|---|
| **o6 +1** | `HP_SLOT_O6` | **1,738,930** | **−2,908** | **accept — v30** |
| **o3/o4 +2** | `HP_SLOT_O34B` | **1,739,111** | **−2,727** | **accept, stack next** |

### v30 = v29 + o6+1 — **1,738,930 / 1.658 bpc, RT PASS**

`HP_SLOT_O6` on v29: **−2,908**. Same unsaturated-order RAM story.
v30 + fx2-manual 8 MB: **1,733,365 (−2,845 vs v29+fx2)**. Stacks.
o34b on v30: **1,736,303 (−2,627)**. Isolated −2,727. Stacks — v31.
v27-100 finished **18,814,116**. RT in flight (`hp_v27.exe`). Do not overwrite it.

### v31 = v30 + o3/o4+2 — **1,736,303 / 1.656 bpc, RT PASS**

`HP_SLOT_O34B` on v30: **−2,627**. Isolated −2,727.
v31 + fx2-manual 8 MB: **1,731,055 (−2,310 vs v30+fx2)**. Stacks. New 8 MB champ.
v31-100 starts after this leftover o1/o2 grow test. Do not overwrite `hp_v27.exe`.

### v31 leftovers (8 MB)

| id | flag | 8 MB | vs v31 1,736,303 | call |
|---|---|---:|---:|---|
| o1/o2 +1 | `HP_SLOT_O12` | 1,736,197 | −106 | weak keep |

v31-100 started (`e8_fx2man_v31_m26.hp`). Do not overwrite `hp_v27.exe` or `hp_v31.exe`.
v27-100 RT deferred to free RAM for leftover 8 MB tests; restart after the 8 MB wave.

### v31 leftovers wave 2 (8 MB)

| id | flag | 8 MB | vs v31 1,736,303 | call |
|---|---|---:|---:|---|
| **o6 +2** | `HP_SLOT_O6B` | **1,733,103** | **−3,200** | **accept — v32** |
| **o3/o4 +3** | `HP_SLOT_O34C` | **1,734,691** | **−1,612** | **accept, stack next** |

### v32 = v31 + o6+2 — **1,733,103 / 1.653 bpc, RT PASS**

`HP_SLOT_O6B` on v31: **−3,200**. Order-6 still unsaturated.
v32 + fx2-manual 8 MB: **1,727,635 (−3,420 vs v31+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v31.exe` while v31-100 runs.

### v32 leftovers (8 MB)

| id | flag | 8 MB | vs v32 1,733,103 | call |
|---|---|---:|---:|---|
| **o3/o4 +3 stack** | `HP_SLOT_O34C` | **1,731,587** | **−1,516** | **accept — v33** (isolated −1,612) |
| sparse +1 | `HP_SLOT_SP` | 1,733,058 | −45 | reject / noise |

### v31 + fx2 + mem 26 (100 MB) — **18,752,700 / 1.500 bpc**

vs v27 18,814,116: **−61,416**. Order-table growth transferred. RT started. Do not overwrite `hp_v31.exe`.

### v33 = v32 + o3/o4+3 — **1,731,587 / 1.651 bpc, RT PASS**

`HP_SLOT_O34C` on v32: **−1,516** (isolated −1,612). Stacks.
v33 + fx2-manual 8 MB: **1,726,421 (−1,214 vs v32+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v31.exe`.

### v33 leftovers (8 MB)

| id | flag | 8 MB | vs v33 1,731,587 | call |
|---|---|---:|---:|---|
| match tables +1 | `HP_MATCH_GROW` | 1,731,251 | −336 | weak keep |
| **o6 +3** | `HP_SLOT_O6C` | **1,728,615** | **−2,972** | **accept — v34** |

### v34 = v33 + o6+3 — **1,728,615 / 1.649 bpc, RT PASS**

`HP_SLOT_O6C` on v33: **−2,972**. Order-6 still paying.
v34 + fx2-manual 8 MB: **1,723,212 (−3,209 vs v33+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v31.exe`.

### v34 leftovers (8 MB)

| id | flag | 8 MB | vs v34 1,728,615 | call |
|---|---|---:|---:|---|
| **match tables +1** | `HP_MATCH_GROW` | **1,728,317** | **−298** | **accept — v35** (isolated −336) |

### v35 = v34 + match-grow — **1,728,317 / 1.648 bpc, RT PASS**

`HP_MATCH_GROW` on v34: **−298**. Weak but stacks.
v35 + fx2-manual 8 MB: **1,722,958 (−254 vs v34+fx2)**. Stacks. New 8 MB champ.
Next 100 MB is v35 after v31-100 RT. Do not overwrite `hp_v31.exe`.

### v27 + fx2 + mem 26 (100 MB) RT — **PASS**

Decode SHA matches `data/enwik8.fx2man`. Encode **18,814,116** confirmed.

### v35 leftovers (8 MB)

| id | flag | 8 MB | vs v35 1,728,317 | call |
|---|---|---:|---:|---|
| **o6 +4** | `HP_SLOT_O6D` | **1,725,795** | **−2,522** | **accept — v36** |
| **o3/o4 +4** | `HP_SLOT_O34D` | **1,727,437** | **−880** | **accept, stack next** |

### v36 = v35 + o6+4 — **1,725,795 / 1.646 bpc, RT PASS**

`HP_SLOT_O6D` on v35: **−2,522**. Order-6 still paying.
v36 + fx2-manual 8 MB: **1,720,515 (−2,443 vs v35+fx2)**. Stacks.
o34d on v36: **1,724,976 (−819)**. Isolated −880. Stacks — v37.

### v31 + fx2 + mem 26 (100 MB) RT — **PASS**

Decode SHA matches. Encode **18,752,700** confirmed. Do not overwrite `hp_v31.exe`.

### v37 = v36 + o3/o4+4 — **1,724,976 / 1.645 bpc, RT PASS**

`HP_SLOT_O34D` on v36: **−819**. Stacks.
v37 + fx2-manual 8 MB: **1,719,644 (−871 vs v36+fx2)**. Stacks. New 8 MB champ.
v37 + fx2 + mem 26 (100 MB) in flight. Do not overwrite `hp_v37.exe`.

### v37 leftovers (8 MB)

| id | flag | 8 MB | vs v37 1,724,976 | call |
|---|---|---:|---:|---|
| match tables +2 | `HP_MATCH_GROW2` | 1,724,889 | −87 | reject / noise |
| **o6 +5** | `HP_SLOT_O6E` | **1,723,137** | **−1,839** | **accept — v38** |

### v38 = v37 + o6+5 — **1,723,137 / 1.643 bpc, RT PASS**

`HP_SLOT_O6E` on v37: **−1,839**. Order-6 still paying.
v38 + fx2-manual 8 MB: **1,717,842 (−1,802 vs v37+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v37.exe`.

### v38 leftovers (8 MB)

| id | flag | 8 MB | vs v38 1,723,137 | call |
|---|---|---:|---:|---|
| **o3/o4 +5** | `HP_SLOT_O34E` | **1,722,701** | **−436** | **accept, stack next** |
| **o6 +6 cap32** | `HP_SLOT_O6F` + `HP_SLOT_MAX=32` | **1,721,877** | **−1,260** | **accept — v39** |

### v39 = v38 + o6+6 cap32 — **1,721,877 / 1.642 bpc, RT PASS**

`HP_SLOT_O6F` + `HP_SLOT_MAX=32` on v38: **−1,260**. At mem 22 this is only o6+1 (28 < 31). Cap 32 also lets word/wbi grow at mem 26.
v39 + fx2-manual 8 MB: **1,716,607 (−1,235 vs v38+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v37.exe`.

### v39 leftovers (8 MB)

| id | flag | 8 MB | vs v39 1,721,877 | call |
|---|---|---:|---:|---|
| **o3/o4 +5** | `HP_SLOT_O34E` | **1,721,462** | **−415** | **accept — v40** (isolated −436) |

### v40 = v39 + o3/o4+5 — **1,721,462 / 1.641 bpc, RT PASS**

`HP_SLOT_O34E` on v39: **−415**.
v40 + fx2-manual 8 MB: **1,716,197 (−410 vs v39+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v37.exe`.

### v40 leftovers (8 MB)

| id | flag | 8 MB | vs v40 1,721,462 | call |
|---|---|---:|---:|---|
| **o6 +7 cap33** | `HP_SLOT_O6G` + `HP_SLOT_MAX=33` | **1,720,680** | **−782** | **accept — v41** |

### v41 = v40 + o6+7 cap33 — **1,720,680 / 1.640 bpc, RT PASS**

`HP_SLOT_O6G` + `HP_SLOT_MAX=33` on v40: **−782**.
v41 + fx2-manual 8 MB: **1,715,498 (−699 vs v40+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v37.exe`.

### v41 leftovers (8 MB)

| id | flag | 8 MB | vs v41 1,720,680 | call |
|---|---|---:|---:|---|
| **o6 +8 cap34** | `HP_SLOT_O6H` + `HP_SLOT_MAX=34` | **1,720,250** | **−430** | **accept — v42** |
| o3/o4 +6 | `HP_SLOT_O34F` | 1,720,444 | −236 | accept, stack after v42 |

### v42 = v41 + o6+8 cap34 — **1,720,250 / 1.640 bpc, RT PASS**

`HP_SLOT_O6H` + `HP_SLOT_MAX=34` on v41: **−430**.
v42 + fx2-manual 8 MB: **1,715,088 (−410 vs v41+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v37.exe`.

### v42 leftovers (8 MB)

| id | flag | 8 MB | vs v42 1,720,250 | call |
|---|---|---:|---:|---|
| **o3/o4 +6** | `HP_SLOT_O34F` | **1,720,014** | **−236** | **accept — v43** (isolated −236) |

### v43 = v42 + o3/o4+6 — **1,720,014 / 1.640 bpc, RT PASS**

`HP_SLOT_O34F` on v42: **−236**. Do not overwrite `hp_v37.exe`.

### v43 leftovers (8 MB)

| id | flag | 8 MB | vs v43 1,720,014 | call |
|---|---|---:|---:|---|
| match o12 | `HP_MATCH_12` | 1,720,114 | +100 | reject |
| **o6 +9 cap35** | `HP_SLOT_O6I` + `HP_SLOT_MAX=35` | **1,719,796** | **−218** | **accept — v44** |
| o3/o4 +7 | `HP_SLOT_O34G` | 1,719,709 | −87 | reject / noise |

### v44 = v43 + o6+9 cap35 — **1,719,796 / 1.640 bpc, RT PASS**

`HP_SLOT_O6I` + `HP_SLOT_MAX=35` on v43: **−218**.
v44 + fx2-manual 8 MB: **1,714,651 (−437 vs v42+fx2; v43 fx2 skipped)**. Stacks.
Do not overwrite `hp_v37.exe`.

### v44 leftovers (8 MB)

| id | flag | 8 MB | vs v44 1,719,796 | call |
|---|---|---:|---:|---|
| **sent-grp ctx** | `HP_SENT_GRP_CTX` | **1,717,483** | **−2,313** | **accept — v45** |
| word +10 | `HP_SLOT_WORD10` | 1,760,662 | +43,179 | reject — 2^32 tables too sparse on 8 MB |

### v45 = v44 + sent-grp ctx — **1,717,483 / 1.637 bpc, RT PASS**

`HP_SENT_GRP_CTX` on v44: **−2,313**. New context family.
v45 + fx2-manual 8 MB: **1,712,736 (−1,915 vs v44+fx2)**. Stacks. New 8 MB champ.
Do not overwrite `hp_v37.exe`.

### v45 + fx2 + mem 26 (100 MB) — **18,633,242 / 1.490 bpc** (encode only)

vs v37 18,671,091: **−37,849**. `hp_v45_m26.exe` (`HP_SLOT_MAX=31`). RT not done. Do not overwrite `hp_v45_m26.exe` until RT.

### v45 leftovers (8 MB)

| id | flag | 8 MB | vs v45 1,717,483 | call |
|---|---|---:|---:|---|
| **wstr sen-group** | `HP_WSTR_GRP` | **1,716,826** | **−657** | **accept — v46** |

### v46 = v45 + wstr-grp — **1,716,826 / 1.637 bpc, RT PASS**

### v46 leftovers (8 MB)

| id | flag | 8 MB | vs v46 1,716,826 | call |
|---|---|---:|---:|---|
| word sen-group | `HP_WORD_GRP` | 1,718,711 | +1,885 | reject — splits word table |
| sen-group mixer gate | `HP_SEN_GROUP` | 1,717,506 | +680 | reject — mixer dilution |
| **wbi sen-group** | `HP_WBI_GRP` | **1,716,568** | **−258** | **accept — v47** |

### v47 = v46 + wbi-grp — **1,716,568 / 1.637 bpc, RT PASS**

`HP_WBI_GRP` on v46: **−258**.

### v47 leftovers (8 MB)

| id | flag | 8 MB | vs v47 1,716,568 | call |
|---|---|---:|---:|---|
| **sentmem sen-group** | `HP_SMEM_GRP` | **1,715,657** | **−911** | **accept — v48** |

### v48 = v47 + smem-grp — **1,715,657 / 1.636 bpc, RT PASS**

`HP_SMEM_GRP` on v47: **−911**.
v48 + fx2-manual 8 MB: **1,711,027 (−1,709 vs v45+fx2)**. Stacks. New 8 MB champ.

### v48 leftovers (8 MB)

| id | flag | 8 MB | vs v48 1,715,657 | call |
|---|---|---:|---:|---|
| **sen-group expert** | `HP_SENGRP_MOD` | **1,712,541** | **−3,116** | **accept — v49** |

### v49 = v48 + sengrp model — **1,712,541 / 1.633 bpc, RT PASS**

`HP_SENGRP_MOD` on v48: **−3,116**. Own table, not a fold. Decode SHA matches `data/enwik8.8mb`.
v49 + fx2-manual 8 MB: **1,707,965 (-3,062 vs v48+fx2)**. Stacks. New 8 MB champ.

### v49 leftovers (8 MB)

| id | flag | 8 MB | vs v49 1,712,541 | call |
|---|---|---:|---:|---|
| **slot sen-group** | `HP_SLOT_SGRP` | **1,711,345** | **-1,196** | **accept - v50** |

### v50 = v49 + slot-sgrp - **1,711,345 / 1.632 bpc, RT PASS**

`HP_SLOT_SGRP` on v49: **-1,196**. Decode SHA matches `data/enwik8.8mb`.
v50 + fx2-manual 8 MB: **1,706,837 (-1,128 vs v49+fx2)**. Stacks. New 8 MB champ.

### v50 leftovers (8 MB)

| id | flag | 8 MB | vs v50 1,711,345 | call |
|---|---|---:|---:|---|
| **sengrp word fold** | `HP_SENGRP_WORD` | **1,709,366** | **-1,979** | **accept - v51** |

### v51 = v50 + sengrp-word - **1,709,366 / 1.630 bpc, RT PASS**

`HP_SENGRP_WORD` on v50: **-1,979**. Decode SHA matches `data/enwik8.8mb`.
v51 + fx2-manual 8 MB: **1,705,183 (-1,654 vs v50+fx2)**. Stacks. New 8 MB champ.

### v51 leftovers (8 MB)

| id | flag | 8 MB | vs v51 1,709,366 | call |
|---|---|---:|---:|---|
| **col sen-group** | `HP_COL_GRP` | **1,708,738** | **-628** | **accept - v52** |

### v52 = v51 + col-grp - **1,708,738 / 1.629 bpc, RT PASS**

`HP_COL_GRP` on v51: **-628**. Decode SHA matches `data/enwik8.8mb`.
v52 + fx2-manual 8 MB: **1,704,635 (-548 vs v51+fx2)**. Stacks. New 8 MB champ.

### v52 leftovers (8 MB)

| id | flag | 8 MB | vs v52 1,708,738 | call |
|---|---|---:|---:|---|
| **tag sen-group** | `HP_TAG_GRP` | **1,708,708** | **-30** | **accept - v53** |

### v53 = v52 + tag-grp - **1,708,708 / 1.629 bpc, RT PASS**

`HP_TAG_GRP` on v52: **-30**. Decode SHA matches `data/enwik8.8mb`.
v53 + fx2-manual 8 MB: **1,704,578 (-57 vs v52+fx2)**. Stacks. New 8 MB champ.

### v53 leftovers (8 MB)

| id | flag | 8 MB | vs v53 1,708,708 | call |
|---|---|---:|---:|---|
| **senword sen-group** | `HP_SENWORD_GRP` | **1,708,542** | **-166** | **accept - v54** |

### v54 = v53 + senword-grp - **1,708,542 / 1.629 bpc, RT PASS**

`HP_SENWORD_GRP` on v53: **-166**. Decode SHA matches `data/enwik8.8mb`.
v54 + fx2-manual 8 MB: **1,704,388 (-190 vs v53+fx2)**. Stacks. New 8 MB champ.

### v54 leftovers (8 MB)

| id | flag | 8 MB | vs v54 1,708,542 | call |
|---|---|---:|---:|---|
| link sen-group | `HP_LINK_GRP` | 1,708,620 | +78 | reject |
| **bracket sen-group** | `HP_BRK_GRP` | **1,708,329** | **-213** | **accept - v55** |

### v55 = v54 + brk-grp - **1,708,329 / 1.629 bpc, RT PASS**

`HP_BRK_GRP` on v54: **-213**. Decode SHA matches `data/enwik8.8mb`.
v55 + fx2-manual 8 MB: **1,704,368 (-20 vs v54+fx2)**. Stacks. New 8 MB champ.

### v55 leftovers (8 MB)

| id | flag | 8 MB | vs v55 1,708,329 | call |
|---|---|---:|---:|---|
| sengrp position | `HP_SENGRP_POS` | 1,708,329 | 0 | reject |
| num sen-group | `HP_NUM_GRP` | 1,708,398 | +69 | reject |
| **space sen-group** | `HP_SP_GRP` | **1,707,742** | **-587** | **accept - v56** |

Skipped `HP_SENGRP_C0` (no-op while `HP_SENGRP_WORD` is on).

### v56 = v55 + sp-grp - **1,707,742 / 1.628 bpc, RT PASS**

`HP_SP_GRP` on v55: **-587**. Decode SHA matches `data/enwik8.8mb`.
v56 + fx2-manual 8 MB: **1,703,460 (-908 vs v55+fx2)**. Stacks. New 8 MB champ.

### v56 leftovers (8 MB)

| id | flag | 8 MB | vs v56 1,707,742 | call |
|---|---|---:|---:|---|
| **col3 extra bit** | `HP_SLOT_COL3` | **1,707,696** | **-46** | **accept - v57** |

### v57 = v56 + slot-col3 - **1,707,696 / 1.628 bpc, RT PASS**

`HP_SLOT_COL3` on v56: **-46**. Decode SHA matches `data/enwik8.8mb`.
v57 + fx2-manual 8 MB: **1,703,430 (-30 vs v56+fx2)**. Stacks. New 8 MB champ.

v55 leftover wave complete. Skipped `HP_SENGRP_C0` (no-op). Did not start 100 MB.

### v55 + fx2 + mem 26 (100 MB) — **18,528,992 / 1.482 bpc, RT PASS**

vs v45 18,633,242: **−104,250**. Decode SHA matches `data/enwik8.fx2man`.
`hp_v55_m26.exe` with `HP_SLOT_MAX=31` (8 MB champ is MAX=35). New 100 MB champ.

### v37 + fx2 + mem 26 (100 MB) — **18,671,091 / 1.494 bpc, RT PASS**

vs v31 18,752,700: **−81,609**. Decode SHA matches `data/enwik8.fx2man`.
Next 100 MB after a later champ is mem 26 with `HP_SLOT_MAX=31` (8 MB champ is MAX=35; at mem 26 that is 2^35 tables and will OOM).

### v57 + fx2 + mem 26 (100 MB) - **18,527,464 / 1.482 bpc** (encode only)

vs v55 18,528,992: **-1,528**. `hp_v57_m26.exe` (`HP_SLOT_MAX=31`). RT pending. Do not overwrite `hp_v57_m26.exe` until RT.

### v57 + fx2 + mem 26 (100 MB) RT - **FAIL**

Decode aborted immediately: `not a CYHP archive`. Encode length **18,527,464** (**-1,528** vs v55 18,528,992) is not a valid archive: no `CYHP` magic anywhere; first 6,348,800 bytes are zeros. `.out` not produced. Did not start 8 MB leftovers. Did not overwrite `hp_v37.exe` / `hp_v45_m26.exe` / `hp_v55_m26.exe`.

### v57 + fx2 + mem 26 (100 MB) retry (non-OneDrive) - **18,527,464 / 1.482 bpc, RT PASS**

Invalid OneDrive archive renamed to `hp/build/e8_fx2man_v57_m26.BAD.hp` (18,527,464, leading zeros, no CYHP). Re-encoded with `hp_v57_m26.exe c --mem 26` to `%LOCALAPPDATA%\hp_lab\e8_fx2man_v57_m26.hp`. Magic `CYHP` (43 59 48 50) at offset 0. Length **18,527,464** (**-1,528** vs v55 18,528,992). Copied to `hp/build/e8_fx2man_v57b_m26.hp`. Decode SHA matches `data/enwik8.fx2man`. New 100 MB champ.

Did not start 8 MB. Did not overwrite `hp_v37.exe` / `hp_v45_m26.exe` / `hp_v55_m26.exe` / `hp_v57.exe`.

### v57 leftovers (8 MB)

| id | flag | 8 MB | vs v57 1,707,696 | call |
|---|---|---:|---:|---|
| match order 20 | `HP_MATCH_20` | 1,707,793 | +97 | reject |
| hebb sen-group | `HP_HEBB_GRP` | 1,707,786 | +90 | reject |

v57 leftover wave complete. Champ remains v57 (1,707,696 identity / 1,703,430 fx2). Did not start 100 MB. Did not run --profile.

### v57 C++ profile (8 MB mem 22)

`hp_v57.exe c --mem 22 --profile` on `data/enwik8.8mb`. Wall **703.7 s**. Archive **1,707,696**.

| row | bytes | bpc |
|---|---:|---:|
| total spent | 1,707,748 | 1.628 |
| best-expert | 217,853 | 0.207 |
| after mixer | 1,731,001 | 1.650 |
| sparse ctx | 811,972 | 0.774 |
| dense ctx | 895,775 | 0.854 |

Model redundancy (mixer vs best expert): **+1,513,148 B**. Coding redundancy (APM+coder vs mixer): **-23,253 B**. Parameter share (sparse contexts): **47%**. Profiled over 67,108,864 bits / 8,388,608 B.

Dump: `dump_experts_v57.exe` (same v57 -D flags, `HP_SLOT_MAX=35`), stride 37, 80,000 records, mem 22. **77 experts**.

| metric | v57 (80k rec, 77 exp, mem 22) |
|---|---:|
| participation ratio | **3.021** of 77 |
| entropy effective rank | **11.874** of 77 |
| top-1 variance share | 57.3% |

gamma=0.000963 (Marchenko-Pastur does not apply). Did not overwrite `hp_v57.exe` / `hp_v57_m26.exe`.

### v57 leftovers (8 MB) continued

| id | flag | 8 MB | vs v57 1,707,696 | call |
|---|---|---:|---:|---|
| **nest-mod** | `HP_NEST_MOD` | **1,707,136** | **-560** | **accept - v58** |

### v58 = v57 + nest-mod - **1,707,136 / 1.628 bpc, RT PASS**

`HP_NEST_MOD` on v57: **-560**. Decode SHA matches `data/enwik8.8mb`.
v58 + fx2-manual 8 MB: **1,702,818 (-612 vs v57+fx2)**. Stacks. New 8 MB champ.
Did not overwrite `hp_v37.exe` / `hp_v45_m26.exe` / `hp_v55_m26.exe` / `hp_v57.exe` / `hp_v57_m26.exe`.

### v58 leftovers (8 MB)

| id | flag | 8 MB | vs v58 1,707,136 | call |
|---|---|---:|---:|---|
| para-mod (no nest, independent) | `HP_PARA_MOD` | 1,707,038 | -98 vs v58 / **-658 vs v57** | restack on v58 (no copy; lacks NEST) |
| **para-mod on v58** | `HP_PARA_MOD` | **1,707,038** | **-98** | **accept - v59** |

### v59 = v58 + para-mod - **1,707,038 / 1.628 bpc, RT PASS**

`HP_PARA_MOD` on v58: **-98**. Decode SHA matches `data/enwik8.8mb`.
Independent PARA (no nest) was also 1,707,038 (**-658 vs v57**); restack matched. Did not copy the no-nest binary.
v59 + fx2-manual 8 MB: **1,702,703 (-115 vs v58+fx2)**. Stacks. New 8 MB champ.

### v59 leftovers (8 MB)

| id | flag | 8 MB | vs v59 1,707,038 | call |
|---|---|---:|---:|---|
| **line-mod** | `HP_LINE_MOD` | **1,705,968** | **-1,070** | **accept - v60** |

### v60 = v59 + line-mod - **1,705,968 / 1.626 bpc, RT PASS**

`HP_LINE_MOD` on v59: **-1,070**. Decode SHA matches `data/enwik8.8mb`.
v60 + fx2-manual 8 MB: **1,701,566 (-1,137 vs v59+fx2)**. Stacks. New 8 MB champ.

### v60 leftovers (8 MB)

| id | flag | 8 MB | vs v60 1,705,968 | call |
|---|---|---:|---:|---|
| **mixer skip 32** | `HP_MIXER_SKIP=32` | **1,705,939** | **-29** | **accept - v61** |

### v61 = v60 + mixer-skip 32 - **1,705,939 / 1.626 bpc, RT PASS**

`HP_MIXER_SKIP=32` on v60: **-29**. Decode SHA matches `data/enwik8.8mb`.
v61 + fx2-manual 8 MB: **1,701,530 (-36 vs v60+fx2)**. Stacks. New 8 MB champ.
Did not overwrite `hp_v37.exe` / `hp_v45_m26.exe` / `hp_v55_m26.exe` / `hp_v57.exe` / `hp_v57_m26.exe` / `hp_v58.exe` / `hp_v59.exe` / `hp_v60.exe`.

v58 leftover wave complete (PARA restack, LINE, SKIP=32 all accepted). Champ is v61 (1,705,939 identity / 1,701,530 fx2). Did not start 100 MB.
Did not overwrite `hp_v37.exe` / `hp_v45_m26.exe` / `hp_v55_m26.exe` / `hp_v57.exe` / `hp_v57_m26.exe` / `hp_v58.exe` / `hp_v59.exe`.

### 2026-09-13 prize frontier (literature, not a lab run)

Official prize table still L = fx2-cmix **110,793,128**. cmix-lex still pending (Intel 109,650,047; claimed AMD rebuild 109,671,639). fx3-cmix unsubmitted 109,735,627 (<1%). cmix-obias claimed S **108,492,825** (256-cell LSTM + bitlstm32 + PPMd obias prior); not on prize.hutter1.net; do not vendor into hp. Meta-pattern: paying CMs are `StateMap(hash(axis ⊗ hist))`; mixer is already `squash(vᵀ W x)` in Q16.

### v61 + fx2 mem26 - **18,490,445 / 1.479 bpc, RT PASS**

`hp_v61_m26.exe` on `data/enwik8.fx2man` `--mem 26` `SLOT_MAX=31`.
Archive **18,490,445** (−**37,019** vs v57 18,527,464). CYHP magic OK.
Decode SHA matches `data/enwik8.fx2man`
`9FA638182A0384AF0040762CBD3C67DC1613B90085BAE37F6E567350B9336A51`.
Wrote `%LOCALAPPDATA%\hp_lab\e8_fx2man_v61_m26.hp` (copy in `hp/build`).
Did not overwrite `hp_v57_m26.exe`. New 100 MB champ.

### v62meta stack - **2,403,533 / 2.292 bpc, REJECT**

v61 + `HP_WIKI_AXES` + `HP_MIXER_RANK=8` + `HP_XSIMD` mem 22: **2,403,533** (**+697,594** vs v61 1,705,939). Bundle loses. Do not blame SIMD. Splitting: wiki-axis CMs alone first (`hp_g_v62wiki.exe`), then rank mixer.

### v62wiki - **1,706,371 / 1.627 bpc, REJECT +432**

`HP_WIKI_AXES` four-CM bundle on v61: **1,706,371** vs 1,705,939. Small loss; a paying axis may be hidden. Split into `HP_STATE_MOD` / `HP_DOM_MOD` / `HP_HDR_MOD` / `HP_DEPTH_MOD`. Rank mixer deferred (v62meta already showed it as the +697k).

### v62dom - **1,706,215 / 1.627 bpc, REJECT +276**

`HP_DOM_MOD` on v61: **1,706,215** vs 1,705,939. sent_domain CM does not pay alone.

### v62state - **1,705,480 / 1.626 bpc, RT PASS −459**

`HP_STATE_MOD` on v61: **1,705,480**. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v62.exe`. Did not overwrite `hp_v61.exe`. fx2-manual **1,701,090 (−440 vs v61 fx2 1,701,530)**. Stacks.
fx2 decode SHA matches `data/enwik8.8mb.fx2man`. New 8 MB champ identity+fx2.

### CUDA mixer search (compass, not a gate)

`hp/tools/gpu_mixer_search.cu` cuBLAS v2 on RTX 3090 (8,203 mixers, 16k symbolic). Wall **15.1 s**. Linear **0.254**; Q16 snap **0.254** (no loss); prefix-16 **0.263**; 8192×8-sparse best **0.271**; pairwise top-12 products **0.392** (lose); symbolic add **0.266**. Q16 is enough for this mixer; extra bilinear features are not. Codec untouched. JSON: `%LOCALAPPDATA%\hp_lab\gpu_mixer_search.json`.

### CUDA simple-algorithm search (subsets of top-12)

`hp/tools/gpu_simple_search.cu` — all 4095 nonempty subsets of the 12 best v57 experts, plus 27k integer 3-term mixes. Knee at **k=4–5** (0.261 bits). Full top-12 **0.270** (extra elite experts hurt). Integer `(x37+x7+x11)/3` **0.275**. Full 77-expert linear still **0.254**. Simpler mixer exists; it is fewer tables, not a new formula.

### v62hdr - **1,705,768 / 1.626 bpc, REJECT +288**

`HP_HDR_MOD` on v62: **1,705,768** vs 1,705,480. wiki_header CM does not pay.

### v62depth - **1,705,577 / 1.626 bpc, REJECT +97**

`HP_DEPTH_MOD` on v62: **1,705,577** vs 1,705,480. depth CM does not pay. Wiki-axis singles: state −459, dom +276, hdr +288, depth +97.

### v62rank - **2,407,863 / 2.296 bpc, REJECT +702,383**

`HP_MIXER_RANK=8` isolate on v62: **2,407,863** vs 1,705,480. Same class of loss as v62meta (+697,594). Low-rank W is the catastrophe, not SIMD or the wiki bundle. Do not sweep rank 4/16.

### v62nopy - **1,723,704 / 1.643 bpc, REJECT +18,224**

`HP_PY_EXPERT=0` on v62: **1,723,704** vs 1,705,480. Dropping the Pitman-Yor mixer slot loses. Half-width CM inputs do not pay. Do not default PY off.

### CUDA precision / estimation sweep (compass, not a gate)

`hp/tools/gpu_precision_search.cu` on RTX 3090, frozen v57 dump, 80 ms. Offline linear mix **0.2540**. Visible = +0.001 val bits; sizeable = +0.010. Codec untouched.

| approx | val | delta | call |
|---|---:|---:|---|
| Q16 / Q12 / Q8 logits | 0.2540 | ~0 | free |
| Q4 logits | 0.2537 | −0.0002 | free |
| Q3 logits | 0.2553 | +0.0013 | visible |
| Q6 weights | 0.2542 | +0.0003 | free |
| Q4 weights | 0.2610 | +0.007 | visible |
| Q3 weights | 0.3068 | +0.053 | sizeable |
| static 32 / 48 / 77 experts | 0.2588 / 0.2555 / 0.2540 | +0.0048 / +0.0015 / 0 | visible / visible / free |
| static ≤24 experts | ≥0.264 | ≥+0.010 | sizeable |
| dyn \|x\| top-32 | 0.2562 | +0.0022 | visible |
| zero \|x\| < 256 | 0.2532 | −0.0008 | free (still computes them) |
| equal-weight mean | 0.356 | +0.102 | sizeable |
| best expert | 0.313 | +0.059 | sizeable |
| logit noise ±256 | 0.2542 | +0.0002 | free |

Knee: mixer math can drop to ~4-bit x and ~6-bit W. Cheap estimators cannot replace W. Cutting below ~32 of 77 experts is not free even as a mix-time skip. Layer-1 gate count not in this dump (stride 37). JSON: `%LOCALAPPDATA%\hp_lab\gpu_precision_search.json`.

### CUDA cluster-MoE (compass, not a gate)

`hp/tools/gpu_moe_cluster.cu` on RTX 3090, frozen v57 dump, wall **15.3 s**. k-means on 77 expert time series, mix inside cluster (`W·x`), squash, uneven `v` across clusters. Mix is never replaced by a mean.

| setup | val bits | delta vs linear 0.2540 | call |
|---|---:|---:|---|
| linear all 77 | 0.2540 | 0 | baseline |
| Q4 logits + Q6 W | 0.2541 | +0.00008 | free |
| prefix-48 retrain | 0.2555 | +0.0015 | visible |
| prefix-32 retrain | 0.2584 | +0.0044 | visible |
| **K=4 soft MoE** (best cluster) | **0.7814** | **+0.527** | sizeable |
| K=4 hard top-1 | 0.8633 | +0.609 | sizeable |
| K=8..24 soft/hard | 0.85–0.99 | +0.60–0.73 | sizeable |

Clustering then mixing cluster opinions loses ~3×. Hard routing is worse. No leftover flag. JSON: `%LOCALAPPDATA%\hp_lab\gpu_moe_cluster.json`.

### Wide experimental net (Mandelbrot and friends)

Searched Mandelbrot/Mandelbrotz, IFS, Sequence Memoizer, DMC, GLZA/RePair, fxcm, PMC, paq8px models, STARLIT, Nacrith, FineZip, L3TC/RWKV, diffusion LMs, NanoZip/BWT, LTCB oddballs. There is **no** working lossless Mandelbrot text compressor (lossy image IFS / Chi 1993 vapor). hp-legal slice = skip-k match, table period, cheap affine residual.

Board: `canvases/wide-trial-net.canvas.tsx` (**240** trials). Tags: H leftover / G GPU / W Track W / X skip. First wave waits on `dump_profile_v62` (M9 mute gate, M3 residual, S2 skip tables) then one-at-a-time DMC / LZP / SR / fccxt / template-name / infobox key. Do not vendor LSTM/fractal/LLM into `hp/`.

### v62 dump + GPU importance (compass)

`dump_profile_v62` on 8 MB mem 22, stride 16: **4,194,304** rec, **85** experts, **10** gates → `%LOCALAPPDATA%\hp_lab\v62_profile.i16`. Names match n_exp.

`gpu_importance` 500k rec (400k/100k), RTX 3090, 325 ms. Offline linear mix of stretched experts **0.020** val bits (not comparable to v57 dump 0.254 — different scale/subsample). Residual mix **4.77** (sizeable fail). Softmax **0.025** worse than linear. Q4 x / Q6 W free vs that linear. Prefix-k looks “free” because 85-d W is undertrained in 80 SGD steps — do not mute disc slots from this. Gate mute is a linear mix of layer-1 *dots*, misspecified; all 10 mutes slightly “help”; **do not leftover-mute a gate**. No mixer leftover from this dump.

### H11 leftover wave (in flight)

Flags default off: `HP_FCCXT_MOD`, `HP_TPLNAME_MOD`, `HP_INFOKEY_MOD`, `HP_BARIDX_MOD`, `HP_PERIOD_MOD`, `HP_PRONOUN_MOD`, `HP_DMC_MOD`, `HP_LZP_MOD`, `HP_SR_MOD`, `HP_SKIPK_MOD`. One compile flag each vs v62 **1,705,480**.

`HP_MIXER_SKIP=40` 8 MB mem 22: **1,705,532 (+52)** REJECT. Keep skip32.

### v62dmc - **1,705,421 / 1.626 bpc, RT PASS −59**

`HP_DMC_MOD` on v62: **1,705,421**. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v63.exe`. Did not overwrite `hp_v62.exe`. fx2-manual stack after the dual 8 MB pair.

Two 8 MB leftovers at once (user). fccxt **1,705,626 (+146 vs v62)** REJECT.

### v62hashchk - **1,699,746 / 1.621 bpc, RT PASS −5,734**

`HP_HASH_CHK` dual-stage index+checksum 3-probe on all CMs: **1,699,746** vs v62 **1,705,480**. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v64.exe`. Did not overwrite `hp_v62.exe` / `hp_v63.exe`. New 8 MB champ. fx2-manual stack in flight.

### v63tpl - **1,704,861 / 1.625 bpc, −560 vs v63, stale vs v64**

`HP_TPLNAME_MOD` on v63 (DMC on): **1,704,861**. Pays on DMC, loses to HASH_CHK by **+5,115**. Restack tpl on v64.

HASH2_O6 encoding vs v62 (one flag). Two 8 MB jobs: hash2 + v64 fx2.

### v62hash2 - **1,705,176 / 1.626 bpc, −304 vs v62, stale vs v64**

`HP_HASH2_O6` on v62 (no HASH_CHK): **1,705,176**. Pays vs v62, loses to HASH_CHK. Restacked on later champs.

### v64 fx2 - **1,695,486 / 1.616 bpc, RT PASS −5,604 vs v62 fx2**

`hp_v64.exe` on `data/enwik8.8mb.fx2man`: **1,695,486**. Decode SHA matches
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`. HASH_CHK stacks.

### v64dmc - **1,699,684 / 1.620 bpc, RT PASS −62 vs v64, superseded**

`HP_DMC_MOD` on v64: **1,699,684**. Decode SHA matches `data/enwik8.8mb`.
Did not copy as champ — TPL paid more on the same rung.

### v64tpl → v65 - **1,696,567 / 1.617 bpc, RT PASS −3,179**

`HP_TPLNAME_MOD` on v64: **1,696,567**. Copied `hp_v65.exe`. Did not overwrite `hp_v64.exe`.

### v64period - **1,699,752 / 1.621 bpc, REJECT +6**

`HP_PERIOD_MOD` on v64. No RT.

### v65dmc → v66 - **1,696,457 / 1.617 bpc, RT PASS −110**

`HP_DMC_MOD` on v65: **1,696,457**. Copied `hp_v66.exe`.
v65 fx2 **1,692,565** RT PASS (−2,921 vs v64 fx2). v66 fx2 **1,692,536** RT PASS (−29).

### v66lzp → v67 - **1,696,174 / 1.617 bpc, RT PASS −283**

`HP_LZP_MOD` on v66: **1,696,174**. Copied `hp_v67.exe`.
v67 fx2 **1,692,228** RT PASS (−308 vs v66 fx2).

### v67sr - **1,696,317 / 1.617 bpc, REJECT +143**

`HP_SR_MOD` on v67. No RT.

### v67skipk → v68 - **1,695,663 / 1.617 bpc, RT PASS −511**

`HP_SKIPK_MOD` on v67: **1,695,663**. Copied `hp_v68.exe`.
v68 fx2 **1,691,956** RT PASS (−272 vs v67 fx2).
infokey vs v67 **1,696,031 (−143)** stale; restacked on v68.

### v68infokey → v69 - **1,695,538 / 1.616 bpc, RT PASS −125**

`HP_INFOKEY_MOD` on v68: **1,695,538**. Copied `hp_v69.exe`.
v69 fx2 **1,691,835** RT PASS (−121 vs v68 fx2).

### v69baridx / pron / fccxt - REJECT

baridx **1,695,804 (+266)**. pronoun **1,695,660 (+122)**. fccxt **1,695,779 (+241)** vs v69. No RT.

### v69hash2 → v70 - **1,695,515 / 1.616 bpc, RT PASS −23**

`HP_HASH2_O6` on v69: **1,695,515**. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v70.exe`. Did not overwrite `hp_v64.exe`…`hp_v69.exe`.
New 8 MB champ. fx2-manual **1,691,740 / 1.613 bpc, RT PASS −95 vs v69 fx2
1,691,835** (−9,350 vs v62 fx2 1,701,090). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

H11/H13 leftover queue empty. Rejected vs champ: skip40, fccxt, period, SR, baridx, pronoun.

### v70 100 MB - **18,443,405 / 1.475 bpc, RT PASS −47,040 vs v61**

`hp_v70_m26.exe` `--mem 26` `SLOT_MAX=31` on `data/enwik8.fx2man`: **18,443,405**.
Wrote `%LOCALAPPDATA%\hp_lab\e8_v70_m26.hp` then copied to `hp/build`.
Decode SHA matches `data/enwik8.fx2man`
`9FA638182A0384AF0040762CBD3C67DC1613B90085BAE37F6E567350B9336A51`.
Did not overwrite `hp_v61_m26.exe`. New 100 MB champ. 8 MB wave idle.

### H14 new-axis wave on v70

Flags default off: `HP_SKIP3_MOD`, `HP_LINKPIPE_MOD`, `HP_CITE_MOD`, `HP_HASH_P5`, `HP_DMC_GROW`, `HP_SKIP4_MOD`, `HP_SKIP5_MOD`.

### v70skip3 → v71 - **1,695,363 / 1.616 bpc, RT PASS −152**

`HP_SKIP3_MOD` skip-3 MatchModel on v70: **1,695,363**. Copied `hp_v71.exe`.
v71 fx2 **1,691,599** RT PASS (−141 vs v70 fx2).

### v71linkpipe → v72 - **1,695,222 / 1.616 bpc, RT PASS −141**

`HP_LINKPIPE_MOD` `[[target|display]]` after-pipe CM on v71: **1,695,222**.
Copied `hp_v72.exe`. v72 fx2 **1,691,482** RT PASS (−117 vs v71 fx2).

### v72cite / hashp5 / dmcgrow - REJECT

cite **1,695,409 (+187)**. HASH_P5 **1,695,645 (+423)** — keep 3-probe. DMC_GROW **1,695,223 (+1)**.

### v72skip4 → v73 - **1,695,141 / 1.616 bpc, RT PASS −81**

`HP_SKIP4_MOD` on v72: **1,695,141**. Copied `hp_v73.exe`.
v73 fx2 **1,691,386** RT PASS (−96 vs v72 fx2). skip-5 **1,695,149 (+8)** REJECT. Skip-k saturates at 4.

### v73 100 MB - **18,434,740 / 1.474 bpc, RT PASS −8,665 vs v70**

`hp_v73_m26.exe` `--mem 26` `SLOT_MAX=31` on `data/enwik8.fx2man`: **18,434,740**.
Wrote `%LOCALAPPDATA%\hp_lab\e8_v73_m26.hp` then copied to `hp/build`.
Decode SHA matches `data/enwik8.fx2man`
`9FA638182A0384AF0040762CBD3C67DC1613B90085BAE37F6E567350B9336A51`.
Did not overwrite `hp_v70_m26.exe`. New 100 MB champ. 8 MB wave idle.

### H15 wiki-domain CMs on v73

Flags default off: `HP_CAT_MOD` (`[[Category:` / `File:` / `Image:` namespace), `HP_REDIR_MOD` (`#REDIRECT`), `HP_HEADING_MOD` (leading `=` count), `HP_EXTLINK_MOD` (`[http…]`), `HP_REFNAME_MOD` (`<ref name="…">`), `HP_QOCXT_MOD` (dedicated quote CM), `HP_ENTITY_MOD` (`&nbsp;` name). One flag per leftover vs v73 **1,695,141**. Two 8 MB at a time.

### v73cat → v74 - **1,694,707 / 1.616 bpc, RT PASS −434**

`HP_CAT_MOD` namespace hash of `[[Category:` / `File:` / `Image:` on v73: **1,694,707**.
Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v74.exe`. Did not overwrite `hp_v73.exe`. fx2 + remaining leftovers on v74.

### v74 fx2 - **1,691,045 / 1.612 bpc, RT PASS −341 vs v73 fx2**

`hp_v74.exe` on `data/enwik8.8mb.fx2man`: **1,691,045**. Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v74heading → v75 - **1,694,607 / 1.616 bpc, RT PASS −100**

`HP_HEADING_MOD` leading `=` count on v74: **1,694,607**.
Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v75.exe`. Did not overwrite `hp_v74.exe`. fx2 + remaining leftovers on v75.

### v75 fx2 - **1,690,955 / 1.612 bpc, RT PASS −90 vs v74 fx2**

`hp_v75.exe` on `data/enwik8.8mb.fx2man`: **1,690,955**. Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v75extlink - **1,695,128 / 1.616 bpc, REJECT +521**

`HP_EXTLINK_MOD` `[http…]` on v75. Twin of `kWkHtLink` / STATE. No RT. Do not restack.

### v75refname - **1,694,907 / 1.616 bpc, REJECT +300**

`HP_REFNAME_MOD` `<ref name="…">` on v75. Cite-adjacent. No RT.

### v75qocxt - **1,694,970 / 1.616 bpc, REJECT +363**

`HP_QOCXT_MOD` dedicated quote CM on v75. Twin of `QUOTE_STACK` fold into `brk_`. No RT.

### v75entity - **1,694,916 / 1.616 bpc, REJECT +309**

`HP_ENTITY_MOD` `&nbsp;` / `&lt;` name on v75. Twin of `kWkAmp` / STATE. No RT.

### v73redir - **1,695,375 / 1.616 bpc, REJECT +234**

`HP_REDIR_MOD` `#REDIRECT` domain. No RT. Do not restack.

H15 leftover queue empty. 8 MB champ is v75. v75 100 MB mem 26 `SLOT_MAX=31` encoded
**18,409,708** (−25,032 vs v73 18,434,740). CYHP header. Did not overwrite
`hp_v73_m26.exe`. Copied `%LOCALAPPDATA%\hp_lab\e8_v75_m26.hp` → `hp/build`.

### v75 100 MB - **18,409,708 / 1.473 bpc, RT PASS −25,032 vs v73**

`hp_v75_m26.exe` `--mem 26` `SLOT_MAX=31` on `data/enwik8.fx2man`: **18,409,708**.
Decode SHA matches `data/enwik8.fx2man`
`9FA638182A0384AF0040762CBD3C67DC1613B90085BAE37F6E567350B9336A51`.
New 100 MB champ. First RT attempt was killed; retry succeeded (~3.15 h).

### H16 leftovers on pulled v75 tree

Pulled-tree v75 8 MB baseline (`hp_g_v75base.exe`, XSIMD + BUF_DELTA=3):
**1,694,590** (−17 vs old v75 1,694,607). Not bit-identical; leftover deltas vs 1,694,590.
Indent + list-level encoding.

### v75indent - **1,695,203 / 1.616 bpc, REJECT +613**

`HP_INDENT_MOD` leading `:` count vs pulled baseline 1,694,590. Twin of LINE/HEADING. No RT.

### v75list - **1,695,078 / 1.616 bpc, REJECT +488**

`HP_LISTLEVEL_MOD` leading `*`/`#` count. Twin of LINE. No RT.

### v75isse - **1,694,855 / 1.616 bpc, REJECT +265**

`HP_ISSE_MOD` hist + o6 p-bucket. Dilution. No RT.

### v75magic - **1,695,070 / 1.616 bpc, REJECT +480**

`HP_MAGIC_MOD` `__TOC__` / `__NOTOC__`. Sparse. No RT.

### v75nowiki - **1,694,882 / 1.616 bpc, REJECT +292**

`HP_NOWIKI_MOD` `<nowiki>`/`<math>`/`<pre>`/`<code>`. Tag-domain twin. No RT.

H16 leftover queue empty. All five rejected vs **1,694,590**.

### v76 = pulled v75 tree - **1,694,590 / 1.616 bpc, RT PASS −17 vs v75**

XSIMD + BUF_DELTA=3 + shared rings on the v75 flag set. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v76.exe`. Did not overwrite `hp_v75.exe`. fx2 **1,690,928 RT PASS** (−27 vs v75 fx2).

### H17 leftovers on v76

Flags default off: `HP_TITLE_MOD` (`<title>` hash), `HP_PAGEID_MOD` (first `<id>` after `<page>`), `HP_USER_MOD` (`<username>`), `HP_TEXT_MOD` (in-`<text>` domain). One flag vs v76 **1,694,590**. Two 8 MB at a time.

### v76title - **1,693,559 / 1.615 bpc, RT PASS −1,031 vs v76**

`HP_TITLE_MOD` page-title hash on v76: **1,693,559**. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v77.exe`. Did not overwrite `hp_v76.exe`. fx2 not yet.

### v76pageid - **1,693,666 / 1.615 bpc, −924 vs v76**

### v77pageid - **1,693,870 / 1.615 bpc, REJECT +311 vs v77**

`HP_PAGEID_MOD` on v77 (title on). Isolated −924 vs v76; title already carries dump-page identity. No RT.

### v77user - **1,693,826 / 1.615 bpc, REJECT +267 vs v77**

`HP_USER_MOD` `<username>` hash on v77. Dilution. No RT.

`HP_TEXT_MOD` in-`<text>` + v77 fx2 in flight.

### v77text - **1,694,032 / 1.615 bpc, REJECT +473 vs v77**

`HP_TEXT_MOD` in-`<text>` domain on v77. Tag-domain twin of STATE. No RT.

H17 leftover queue empty. pageid isolated −924 but **+311** on title champ; user **+267**; text **+473**.

### v77 = v76 + TITLE - **1,693,559 / 1.615 bpc, RT PASS −1,031 vs v76**

`HP_TITLE_MOD`. Copied `hp_v77.exe`. Did not overwrite `hp_v76.exe`.
fx2-manual **1,689,873** (−1,055 vs v76 fx2 1,690,928). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### H18 leftovers on v77

Flags default off: `HP_NS_MOD` (dump `<ns>`), `HP_DUMPREDIR_MOD` (`<redirect/>`, not `#REDIRECT`), `HP_IP_MOD` (`<ip>`), `HP_REVCOMMENT_MOD` (`<comment>`). One flag vs v77 **1,693,559**. Two 8 MB at a time.

### v77ns - **1,693,841 / 1.615 bpc, REJECT +282 vs v77**

`HP_NS_MOD` dump `<ns>` id. Sparse vs title. No RT.

### v77dumpredir - **1,693,840 / 1.615 bpc, REJECT +281 vs v77**

`HP_DUMPREDIR_MOD` dump `<redirect/>`. Sparse; not `#REDIRECT`. No RT.

`HP_IP_MOD` + `HP_REVCOMMENT_MOD` in flight.

### v77ip - **1,693,949 / 1.615 bpc, REJECT +390 vs v77**

`HP_IP_MOD` dump `<ip>`. Sparse. No RT.

### v77revcomment - **1,693,808 / 1.615 bpc, REJECT +249 vs v77**

`HP_REVCOMMENT_MOD` dump `<comment>`. No RT.

H18 leftover queue empty. ns **+282**; dumpredir **+281**; ip **+390**; revcomment **+249**. Do not reopen. 8 MB champ stays v77.

### v77-100 mem 26 - **18,370,971 / 1.469 bpc, RT PASS −38,737 vs v75**

`hp_v77_m26.exe` `--mem 26` `SLOT_MAX=31` + `HP_TITLE_MOD` on `data/enwik8.fx2man`: **18,370,971**.
Decode SHA matches `data/enwik8.fx2man`
`9FA638182A0384AF0040762CBD3C67DC1613B90085BAE37F6E567350B9336A51`.
New 100 MB champ. Did not overwrite `hp_v75_m26.exe`. Copied
`%LOCALAPPDATA%\hp_lab\e8_v77_m26.hp` → `hp/build`.

### H19 leftovers on v77

Flags default off: `HP_MINOR_MOD` (`<minor/>`), `HP_WIKIMODEL_MOD` (`<model>`). One flag vs v77 **1,693,559**. Two 8 MB at a time.

### v77minor - **1,693,912 / 1.615 bpc, REJECT +353 vs v77**

`HP_MINOR_MOD` dump `<minor/>`. Sparse. No RT.

### v77model - **1,693,851 / 1.615 bpc, REJECT +292 vs v77**

`HP_WIKIMODEL_MOD` dump `<model>`. Almost always `wikitext`. No RT.

H19 leftover queue empty. Dump-XML extras after title all dilute. 8 MB champ stays v77. 100 MB champ is v77.

### H20 leftovers on v77

Flags default off: `HP_SECTITLE_MOD` (heading-body text, not `=` count), `HP_PARSERFN_MOD` (`{{#if` / `#switch`), `HP_TABLECLASS_MOD` (`{| class=`). One flag vs v77 **1,693,559**. Two 8 MB at a time.

### v77sectitle - **1,692,024 / 1.613 bpc, RT PASS −1,535 vs v77**

`HP_SECTITLE_MOD` heading-body text hash on v77: **1,692,024**. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v78.exe`. Did not overwrite `hp_v77.exe`.

### v77parserfn - **1,693,823 / 1.615 bpc, REJECT +264 vs v77**

`HP_PARSERFN_MOD` `{{#` name. TPLNAME already covers most templates. Restack on v78. No RT.

### v78parserfn - **1,692,310 / 1.613 bpc, REJECT +286 vs v78**

`HP_PARSERFN_MOD` on v78. Dilution. No RT.

### v78tableclass - **1,692,541 / 1.614 bpc, REJECT +517 vs v78**

`HP_TABLECLASS_MOD` `{| class=` first-line tokens. Twin of table STATE. No RT.

H20 leftover queue empty. parserfn **+264/+286**; tableclass **+517**. 8 MB champ is v78.

### v78 = v77 + SECTITLE - **1,692,024 / 1.613 bpc, RT PASS −1,535 vs v77**

`HP_SECTITLE_MOD`. Copied `hp_v78.exe`. Did not overwrite `hp_v77.exe`.
fx2-manual **1,688,500** (−1,373 vs v77 fx2 1,689,873). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v78-100 mem 26 (in flight)

`hp_v78_m26.exe` `--mem 26` `SLOT_MAX=31` + TITLE + SECTITLE on `data/enwik8.fx2man` → `%LOCALAPPDATA%\hp_lab\e8_v78_m26.hp`. Did not overwrite `hp_v77_m26.exe`.

### H21 leftovers on v78 (queued)

Flags default off: `HP_ANCHOR_MOD` (`[[#section`), `HP_PUBID_MOD` (ISBN/PMID digits), `HP_TEMPPOS_MOD` (first positional `{{tpl|arg`). One at a time, mem 22, RSS cap 30 GB.

### v78anchor - **1,692,485 / 1.614 bpc, REJECT +461 vs v78**

`HP_ANCHOR_MOD` `[[target#section`. Twin of link/CAT. No RT.

### v78pubid - **1,692,415 / 1.614 bpc, REJECT +391 vs v78**

`HP_PUBID_MOD` ISBN/PMID digits. Sparse. No RT.

### v78temppos - **1,692,290 / 1.613 bpc, REJECT +266 vs v78**

`HP_TEMPPOS_MOD` first positional `{{tpl|arg`. Twin of TPLNAME/INFOKEY. No RT.

H21 leftover queue empty. anchor **+461**; pubid **+391**; temppos **+266**. Do not reopen.

### v78wikistack → v79 - **1,690,064 / 1.611 bpc, RT PASS −1,960 vs v78**

`HP_WIKISTACK_MOD` packed first-char stack + bracket nest + cell-above. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v79.exe`. Did not overwrite `hp_v78.exe`.
fx2-manual **1,686,585** (−1,915 vs v78 fx2 1,688,500). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v79reorder - **1,693,086 / 1.614 bpc, REJECT +3,022 vs v79**

`HP_REORDER` sort `<page>` by title. 1075 pages, perm header 4,333 B. Clustering does not cover the sidecar on 8 MB. No RT. Keep testing fx2-manual as input, not an hp preprocessor.

### v79payloadlex - **1,693,799 / 1.615 bpc, REJECT +3,735 vs v79**

`HP_PAYLOAD_LEX` sort `<page>` by `<text>`. Same sidecar, worse clustering. No RT. Winners' payload_lex is PHDA9-tail after WRT, not raw dump pages.

### v79dict - **1,752,731 / 1.671 bpc, REJECT +62,667 vs v79**

`--dict` on `hp_v79.exe`. 1,280 words, dict 10,877 B, body 82% of raw. Token stream hurts the mixer more than the transform saves. Same B.3 lesson. No RT.

H22 leftover queue empty. wikistack **v79 −1,960**. reorder **+3,022**; payload_lex **+3,735**; dict **+62,667**. Do not reopen those three. Do not run mem 26 (55 GB).

### H23 leftovers on v79 (queued)

Flags default off: `HP_LANG_MOD` (`[[xx:` interwiki), `HP_CATSORT_MOD` (`[[Category:Name|sortkey`), `HP_TBLROW_MOD` (`{|` row/caption/header/cell), `HP_FILEOPT_MOD` (`[[File:` options), `HP_DEFAULTSORT_MOD` (`{{DEFAULTSORT:`), `HP_REDIRTARGET_MOD` (`#REDIRECT [[title]]`), `HP_DAB_MOD` (`{{disambig`), `HP_HATNOTE_MOD` (`{{for|` / `{{about|`). One at a time, mem 22, RSS cap 30 GB.

### v79lang - **1,690,542 / 1.612 bpc, REJECT +478 vs v79**

`HP_LANG_MOD` `[[xx:` interwiki prefix. Twin of CAT. No RT.

### v79catsort - **1,690,498 / 1.612 bpc, REJECT +434 vs v79**

`HP_CATSORT_MOD` `[[Category:Name|sortkey`. Twin of CAT/linkpipe. No RT.

### v79tblrow - **1,690,542 / 1.612 bpc, REJECT +478 vs v79**

`HP_TBLROW_MOD` `{|` row/caption/header/cell kind. Twin of tableclass. No RT.

### v79fileopt - **1,690,572 / 1.612 bpc, REJECT +508 vs v79**

`HP_FILEOPT_MOD` `[[File:` thumb/px/right options. Twin of CAT. No RT.

### v79defaultsort - **1,690,335 / 1.612 bpc, REJECT +271 vs v79**

`HP_DEFAULTSORT_MOD` `{{DEFAULTSORT:` key. Twin of TPLNAME. No RT.

### v79redirtarget - **1,690,354 / 1.612 bpc, REJECT +290 vs v79**

`HP_REDIRTARGET_MOD` `#REDIRECT [[title]]` target hash. Twin of REDIR. No RT.

### v79dab - **1,690,621 / 1.612 bpc, REJECT +557 vs v79**

`HP_DAB_MOD` `{{disambig` / hndis / dab. Twin of TPLNAME. No RT.

### v79hatnote - **1,690,373 / 1.612 bpc, REJECT +309 vs v79**

`HP_HATNOTE_MOD` `{{for|` / `{{about|` / `{{main|`. Twin of TPLNAME. No RT.

H23 leftover queue empty. lang **+478**; catsort **+434**; tblrow **+478**; fileopt **+508**; defaultsort **+271**; redirtarget **+290**; dab **+557**; hatnote **+309**. Do not reopen. 8 MB champ stays v79. Do not run mem 26.

### H24 leftovers on v79 (queued)

Flags default off: `HP_LASTLINK_MOD` (sticky last `[[target]]`), `HP_FWORD_MOD` (sentence first-word), `HP_YEAR_MOD` (last 4-digit year), `HP_CAPMASK_MOD` (word capitalisation mask), `HP_CELLTXT_MOD` (table-cell text), `HP_HTTPHOST_MOD` (URL hostname), `HP_PAREN_MOD` (last `(...)` group), `HP_LISTPOS_MOD` (nth `*`/`#` item). One at a time, mem 22, RSS cap 30 GB. Not dump-tag CMs. Do not reopen H23.

### v79lastlink - **1,690,456 / 1.612 bpc, REJECT +392 vs v79**

`HP_LASTLINK_MOD` sticky last `[[target]]` after `]]`. Twin of link/CAT. No RT.

### v79fword - **1,690,368 / 1.612 bpc, REJECT +304 vs v79**

`HP_FWORD_MOD` sentence first-word (fxcm fword) as own CM. Twin of senword/PARA. No RT.

### v79year - **1,690,232 / 1.611 bpc, REJECT +168 vs v79**

`HP_YEAR_MOD` last 4-digit year 1000–2099. Twin of NUMERIC. No RT.

### v79capmask → v80 - **1,689,846 / 1.611 bpc, RT PASS −218 vs v79**

`HP_CAPMASK_MOD` current-word capitalisation mask. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v80.exe`. Did not overwrite `hp_v79.exe`.
fx2-manual **1,686,392** (−193 vs v79 fx2 1,686,585). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v80celltxt - **1,690,485 / 1.612 bpc, REJECT +639 vs v80**

`HP_CELLTXT_MOD` table-cell text hash. Twin of wikistack/tableclass. No RT.

### v80httphost - **1,690,400 / 1.612 bpc, REJECT +554 vs v80**

`HP_HTTPHOST_MOD` URL hostname hash. Twin of EXTLINK. No RT.

### v80paren - **1,690,363 / 1.612 bpc, REJECT +517 vs v80**

`HP_PAREN_MOD` sticky last `(...)` group. Twin of NEST. No RT.

### v80listpos - **1,690,435 / 1.612 bpc, REJECT +589 vs v80**

`HP_LISTPOS_MOD` nth `*`/`#` list item. Twin of LISTLEVEL. No RT.

H24 leftover queue empty. capmask **v80 −218**. lastlink **+392**; fword **+304**; year **+168**; celltxt **+639**; httphost **+554**; paren **+517**; listpos **+589**. Do not reopen the seven rejects. 8 MB champ is v80. Do not run mem 26.

### H25 leftovers on v80 (queued)

Flags default off: `HP_SHAPE_MOD` (2-bit-per-char word shape), `HP_SUFFIX_MOD` (last 3 letters), `HP_PREFIX_MOD` (first 3 letters), `HP_CHARCLS_MOD` (rolling byte-class), `HP_VOWEL_MOD` (vowel/consonant ring), `HP_CONTR_MOD` (internal `'`), `HP_HYPHEN_MOD` (hyphenated compound), `HP_TOKENCLS_MOD` (token kind). One at a time, mem 22, RSS cap 30 GB. Do not reopen H24 rejects.

### v80shape - **1,690,501 / 1.612 bpc, REJECT +655 vs v80**

`HP_SHAPE_MOD` 2-bit-per-char word shape sequence. Twin of capmask. No RT.

### v80suffix - **1,690,076 / 1.611 bpc, REJECT +230 vs v80**

`HP_SUFFIX_MOD` last 3 letters of word. Twin of word_/stem. No RT.

### v80prefix - **1,690,557 / 1.612 bpc, REJECT +711 vs v80**

`HP_PREFIX_MOD` first 3 letters of word. Twin of word_/capmask. No RT.

### v80charcls - **1,690,347 / 1.612 bpc, REJECT +501 vs v80**

`HP_CHARCLS_MOD` rolling byte-class stream. Twin of capmask/o1. No RT.

### v80vowel - **1,689,991 / 1.611 bpc, REJECT +145 vs v80**

`HP_VOWEL_MOD` vowel/consonant bit-ring of current word. Twin of capmask. No RT.

### v80contr - **1,690,388 / 1.612 bpc, REJECT +542 vs v80**

`HP_CONTR_MOD` internal-apostrophe contraction. Twin of QOCXT/word. No RT.

### v80hyphen - **1,690,401 / 1.612 bpc, REJECT +555 vs v80**

`HP_HYPHEN_MOD` hyphenated-compound hash. Twin of word_. No RT.

### v80tokencls - **1,690,473 / 1.612 bpc, REJECT +627 vs v80**

`HP_TOKENCLS_MOD` alpha/digit/mixed/punct/xml/wiki token kind. Twin of STATE/capmask. No RT.

H25 leftover queue empty. shape **+655**; suffix **+230**; prefix **+711**; charcls **+501**; vowel **+145**; contr **+542**; hyphen **+555**; tokencls **+627**. Do not reopen. 8 MB champ stays v80. Do not run mem 26.

### H26 leftovers on v80 (queued)

Flags default off: `HP_RUNLEN_MOD` (identical-byte run), `HP_WPOS_MOD` (letter index in word), `HP_BLANK_MOD` (consecutive newlines), `HP_SPRUN_MOD` (space-run), `HP_LINELEN_MOD` (line length), `HP_TAGDIST_MOD` (bytes since `<`), `HP_MARKDIST_MOD` (bytes since `[]{}|=*#<>`), `HP_UPPERGAP_MOD` (bytes since last `A–Z`). One at a time, mem 22, RSS cap 30 GB. Do not reopen H25.

### v80runlen - **1,690,278 / 1.611 bpc, REJECT +432 vs v80**

`HP_RUNLEN_MOD` identical-byte run length. Twin of o1/HEADING. No RT.

### v80wpos - **1,690,312 / 1.612 bpc, REJECT +466 vs v80**

`HP_WPOS_MOD` letter index in word. Twin of GATE_WORDPOS. No RT.

### v80blank - **1,690,181 / 1.612 bpc, REJECT +335 vs v80**

`HP_BLANK_MOD` consecutive newlines. Twin of PARA/LINE. No RT.

### v80sprun - **1,690,260 / 1.611 bpc, REJECT +414 vs v80**

`HP_SPRUN_MOD` space-run length. Twin of o1/SP_GRP. No RT.

### v80linelen - **1,690,123 / 1.611 bpc, REJECT +277 vs v80**

`HP_LINELEN_MOD` current line length. Twin of LINE/HEADING. No RT.

### v80tagdist - **1,690,480 / 1.612 bpc, REJECT +634 vs v80**

`HP_TAGDIST_MOD` bytes since last `<`. Twin of in_tag/STATE. No RT.

### v80markdist - **1,690,079 / 1.611 bpc, REJECT +233 vs v80**

`HP_MARKDIST_MOD` bytes since last `[]{}|=*#<>`. Twin of STATE/WIKISTACK. No RT.

### v80uppergap → v81 - **1,689,247 / 1.610 bpc, RT PASS −599 vs v80**

`HP_UPPERGAP_MOD` bytes since last `A–Z`. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v81.exe`. Did not overwrite `hp_v80.exe`.
fx2-manual **1,685,768** (−624 vs v80 fx2 1,686,392). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

H26 leftover queue empty. uppergap **v81 −599**. runlen **+432**; wpos **+466**; blank **+335**; sprun **+414**; linelen **+277**; tagdist **+634**; markdist **+233**. Do not reopen the seven rejects. 8 MB champ is v81. Do not run mem 26.

### H27 leftovers on v81 (queued)

Flags default off: `HP_MONTH_MOD` (last English month name), `HP_GALLERY_MOD` (`<gallery>` domain), `HP_SECKIND_MOD` (classified heading kind), `HP_CITEKIND_MOD` (`{{cite web/journal/book/news`), `HP_TAGNAME_MOD` (HTML/XML tag-name hash), `HP_COLSPAN_MOD` (`colspan=`/`rowspan=`), `HP_STYLE_MOD` (`style=` CSS), `HP_COORD_MOD` (`{{coord`). One at a time, mem 22, RSS cap 30 GB. Do not reopen H26 rejects.

### v81month - **1,689,501 / 1.611 bpc, REJECT +254 vs v81**

`HP_MONTH_MOD` last English month name. Twin of YEAR. No RT.

### v81gallery - **1,689,539 / 1.611 bpc, REJECT +292 vs v81**

`HP_GALLERY_MOD` `<gallery>` domain. Sparse vs NOWIKI/tag. No RT.

### v81seckind - **1,689,523 / 1.611 bpc, REJECT +276 vs v81**

`HP_SECKIND_MOD` classified heading kind. Twin of SECTITLE. No RT.

### v81citekind - **1,689,923 / 1.611 bpc, REJECT +676 vs v81**

`HP_CITEKIND_MOD` `{{cite web/journal/book/news`. Twin of TPLNAME/CITE. No RT.

### v81tagname - **1,689,872 / 1.611 bpc, REJECT +625 vs v81**

`HP_TAGNAME_MOD` HTML/XML tag-name hash. Twin of STATE/dump tags. No RT.

### v81colspan - **1,689,935 / 1.611 bpc, REJECT +688 vs v81**

`HP_COLSPAN_MOD` `colspan=`/`rowspan=` value. Twin of TBLROW. No RT.

### v81style - **1,689,779 / 1.611 bpc, REJECT +532 vs v81**

`HP_STYLE_MOD` `style=` CSS fragment. Twin of TABLECLASS. No RT.

### v81coord - **1,689,518 / 1.611 bpc, REJECT +271 vs v81**

`HP_COORD_MOD` `{{coord` / `{{coordinates`. Twin of TPLNAME. No RT.

H27 leftover queue empty. month **+254**; gallery **+292**; seckind **+276**; citekind **+676**; tagname **+625**; colspan **+688**; style **+532**; coord **+271**. Do not reopen. 8 MB champ stays v81. Do not run mem 26.

### H28 leftovers on v81 (queued)

Flags default off: `HP_DIGITGAP_MOD` (bytes since last digit), `HP_DOTGAP_MOD` (bytes since `.`), `HP_COMMAGAP_MOD` (bytes since `,`), `HP_WORDLEN_MOD` (last completed letter-word length), `HP_SENTLEN_MOD` (bytes since `.?!`), `HP_LOWERGAP_MOD` (bytes since `a–z`), `HP_DIGITPOS_MOD` (index in current digit run), `HP_SLASHGAP_MOD` (bytes since `/`). One at a time, mem 22, RSS cap 30 GB. Do not reopen H27.

### v81digitgap - **1,689,521 / 1.611 bpc, REJECT +274 vs v81**

`HP_DIGITGAP_MOD` bytes since last digit. Twin of UPPERGAP/NUMERIC. No RT.

### v81dotgap - **1,689,396 / 1.611 bpc, REJECT +149 vs v81**

`HP_DOTGAP_MOD` bytes since `.`. Twin of PERIOD/sent. No RT.

### v81commagap - **1,689,503 / 1.611 bpc, REJECT +256 vs v81**

`HP_COMMAGAP_MOD` bytes since `,`. Twin of o1/layout. No RT.

### v81wordlen → v82 - **1,689,157 / 1.610 bpc, RT PASS −90 vs v81**

`HP_WORDLEN_MOD` last completed letter-word length. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v82.exe`. Did not overwrite `hp_v81.exe`.
fx2-manual **1,685,642** (−126 vs v81 fx2 1,685,768). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v82sentlen - **1,689,344 / 1.611 bpc, REJECT +187 vs v82**

`HP_SENTLEN_MOD` bytes since `.?!`. Twin of DOTGAP/PERIOD. No RT.

### v82lowergap - **1,689,490 / 1.611 bpc, REJECT +333 vs v82**

`HP_LOWERGAP_MOD` bytes since `a–z`. Twin of UPPERGAP. No RT.

### v82digitpos - **1,689,479 / 1.611 bpc, REJECT +322 vs v82**

`HP_DIGITPOS_MOD` index in current digit run. Twin of WPOS/NUMERIC. No RT.

### v82slashgap - **1,689,547 / 1.611 bpc, REJECT +390 vs v82**

`HP_SLASHGAP_MOD` bytes since `/`. Twin of EXTLINK/HTTP. No RT.

H28 leftover queue empty. wordlen **v82 −90**. digitgap **+274**; dotgap **+149**; commagap **+256**; sentlen **+187**; lowergap **+333**; digitpos **+322**; slashgap **+390**. Do not reopen the seven rejects. 8 MB champ is v82. Do not run mem 26.

### H29 leftovers on v82 (queued)

Flags default off: `HP_DIGLEN_MOD` (last completed digit-run length), `HP_PREVLINE_MOD` (last completed line length), `HP_PREVSENT_MOD` (last completed sentence length), `HP_LINKLEN_MOD` (last completed `[[link]]` length), `HP_TPLLEN_MOD` (last completed `{{template}}` length), `HP_PARALEN_MOD` (last completed paragraph length), `HP_ALNUMLEN_MOD` (last completed alnum-token length), `HP_SPLEN_MOD` (last completed space-run length). One at a time, mem 22, RSS cap 30 GB. Do not reopen H28.

### v82diglen - **1,689,481 / 1.611 bpc, REJECT +324 vs v82**

`HP_DIGLEN_MOD` last completed digit-run length. Twin of DIGITPOS/NUMERIC. No RT.

### v82prevline - **1,689,483 / 1.611 bpc, REJECT +326 vs v82**

`HP_PREVLINE_MOD` last completed line length. Twin of LINELEN. No RT.

### v82prevsent - **1,689,599 / 1.611 bpc, REJECT +442 vs v82**

`HP_PREVSENT_MOD` last completed sentence length. Twin of SENTLEN. No RT.

### v82linklen - **1,689,682 / 1.611 bpc, REJECT +525 vs v82**

`HP_LINKLEN_MOD` last completed `[[link]]` length. Twin of LASTLINK/STATE. No RT.

### v82tpllen - **1,689,636 / 1.611 bpc, REJECT +479 vs v82**

`HP_TPLLEN_MOD` last completed `{{template}}` length. Twin of TPLNAME. No RT.

### v82paralen - **1,689,668 / 1.611 bpc, REJECT +511 vs v82**

`HP_PARALEN_MOD` last completed paragraph length. Twin of PARA. No RT.

### v82alnumlen - **1,689,829 / 1.611 bpc, REJECT +672 vs v82**

`HP_ALNUMLEN_MOD` last completed alnum-token length. Twin of WORDLEN. No RT.

### v82splen - **1,689,808 / 1.611 bpc, REJECT +651 vs v82**

`HP_SPLEN_MOD` last completed space-run length. Twin of SPRUN. No RT.

H29 leftover queue empty. diglen **+324**; prevline **+326**; prevsent **+442**; linklen **+525**; tpllen **+479**; paralen **+511**; alnumlen **+672**; splen **+651**. Do not reopen. 8 MB champ stays v82. Do not run mem 26.

### H30 leftovers on v82 (queued)

Flags default off: `HP_TITLEWORD_MOD` (current word hits page-title token), `HP_HEADWORD_MOD` (current word hits last-heading token), `HP_INIT_MOD` (letter-dot initials), `HP_ORDINAL_MOD` (1st/2nd/3rd/4th after digits), `HP_UNIT_MOD` (km/mi/kg after number), `HP_DECIMAL_MOD` (digit.digit), `HP_REPEAT_MOD` (current word repeats previous), `HP_CASEFLIP_MOD` (bytes since lower-to-upper flip). One at a time, mem 22, RSS cap 30 GB. Do not reopen H29.

### v82titleword - **1,689,802 / 1.611 bpc, REJECT +645 vs v82**

`HP_TITLEWORD_MOD` current word hits page-title token. Twin of TITLE hash. No RT.

### v82headword - **1,689,790 / 1.611 bpc, REJECT +633 vs v82**

`HP_HEADWORD_MOD` current word hits last-heading token. Twin of HEADING/SECTITLE. No RT.

### v82init - **1,689,736 / 1.611 bpc, REJECT +579 vs v82**

`HP_INIT_MOD` letter-dot initials. Twin of capmask/word. No RT.

### v82ordinal - **1,689,609 / 1.611 bpc, REJECT +452 vs v82**

`HP_ORDINAL_MOD` 1st/2nd/3rd/4th after digits. Twin of YEAR/NUMERIC. No RT.

### v82unit - **1,689,633 / 1.611 bpc, REJECT +476 vs v82**

`HP_UNIT_MOD` km/mi/kg after number. Twin of NUMERIC. No RT.

### v82decimal - **1,689,497 / 1.611 bpc, REJECT +340 vs v82**

`HP_DECIMAL_MOD` digit.digit. Twin of NUMERIC/DOTGAP. No RT.

### v82repeat - **1,689,841 / 1.611 bpc, REJECT +684 vs v82**

`HP_REPEAT_MOD` current word repeats previous. Twin of WORD. No RT.

### v82caseflip - **1,689,839 / 1.611 bpc, REJECT +682 vs v82**

`HP_CASEFLIP_MOD` bytes since lower-to-upper flip. Twin of UPPERGAP/CAPMASK. No RT.

H30 leftover queue empty. titleword **+645**; headword **+633**; init **+579**; ordinal **+452**; unit **+476**; decimal **+340**; repeat **+684**; caseflip **+682**. Do not reopen. 8 MB champ stays v82. Do not run mem 26.

### H31 leftovers on v82 (queued)

Flags default off: `HP_LEAD_MOD` (before first heading), `HP_INFOVAL_MOD` (template value class), `HP_LINKTRAIL_MOD` (letters after `]]`), `HP_CELLKIND_MOD` (table caption/header/data/rowsep), `HP_TBLCOL_MOD` (column in current table row), `HP_HEADIDX_MOD` (nth heading on the page), `HP_HTMLFMT_MOD` (open inline HTML bitmask), `HP_INFOBOX_MOD` (inside `{{Infobox}}`). One at a time, mem 22, RSS cap 30 GB. Do not reopen H30.

### v82lead - **1,689,681 / 1.611 bpc, REJECT +524 vs v82**

`HP_LEAD_MOD` before first heading. Twin of HEADING/PARA/LINE. No RT.

### v82infoval - **1,689,758 / 1.611 bpc, REJECT +601 vs v82**

`HP_INFOVAL_MOD` template/infobox value class. Twin of INFOKEY/NUMERIC. No RT.

### v82linktrail - **1,689,437 / 1.611 bpc, REJECT +280 vs v82**

`HP_LINKTRAIL_MOD` letters after `]]`. Twin of LINKPIPE/WORD. No RT.

### v82cellkind - **1,689,691 / 1.611 bpc, REJECT +534 vs v82**

`HP_CELLKIND_MOD` table caption/header/data/rowsep. Twin of TBLROW/STATE. No RT.

### v82tblcol - **1,689,637 / 1.611 bpc, REJECT +480 vs v82**

`HP_TBLCOL_MOD` column in current table row. Twin of BARIDX/COL. No RT.

### v82headidx - **1,689,403 / 1.611 bpc, REJECT +246 vs v82**

`HP_HEADIDX_MOD` nth heading on the page. Twin of HEADING/PARA. No RT.

### v82htmlfmt - **1,689,454 / 1.611 bpc, REJECT +297 vs v82**

`HP_HTMLFMT_MOD` open inline HTML bitmask. Twin of TAGNAME/NEST. No RT.

### v82infobox - **1,689,692 / 1.611 bpc, REJECT +535 vs v82**

`HP_INFOBOX_MOD` inside `{{Infobox}}`. Twin of TPLNAME. No RT.

H31 leftover queue empty. lead **+524**; infoval **+601**; linktrail **+280**; cellkind **+534**; tblcol **+480**; headidx **+246**; htmlfmt **+297**; infobox **+535**. Do not reopen. 8 MB champ stays v82. Do not run mem 26.

### H32 leftovers on v82 (queued)

Flags default off: `HP_SECLEVEL_MOD` (sticky heading level of section body), `HP_BRACE3_MOD` (`{{{` param depth), `HP_NAMEDARG_MOD` (named template arg after `=`), `HP_INCLUDE_MOD` (includeonly/noinclude/onlyinclude), `HP_SIG_MOD` (`~~~~` tilde run), `HP_WIKIBOLD_MOD` (wiki `''`/`'''`/`'''''`), `HP_URLPART_MOD` (URL host/path/query/fragment), `HP_REFIDX_MOD` (nth `<ref>`). One at a time, mem 22, RSS cap 30 GB. Do not reopen H31.

### v82seclevel - **1,689,536 / 1.611 bpc, REJECT +379 vs v82**

`HP_SECLEVEL_MOD` sticky heading level of section body. Twin of HEADING/LEAD/HEADIDX. No RT.

### v82brace3 - **1,689,442 / 1.611 bpc, REJECT +285 vs v82**

`HP_BRACE3_MOD` `{{{` param depth. Twin of NEST/TPLNAME. No RT.

### v82namedarg - **1,689,627 / 1.611 bpc, REJECT +470 vs v82**

`HP_NAMEDARG_MOD` named template arg after `=`. Twin of INFOKEY/INFOVAL. No RT.

### v82include - **1,689,440 / 1.611 bpc, REJECT +283 vs v82**

`HP_INCLUDE_MOD` includeonly/noinclude/onlyinclude. Twin of NOWIKI/TAGNAME. No RT.

### v82sig - **1,689,432 / 1.611 bpc, REJECT +275 vs v82**

`HP_SIG_MOD` `~~~~` tilde run. Twin of RUNLEN/MAGIC. No RT.

### v82wikibold → v83 - **1,687,899 / 1.609 bpc, RT PASS −1,258 vs v82**

`HP_WIKIBOLD_MOD` wiki `''`/`'''`/`'''''` bold-italic state. Decode SHA matches
`data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v83.exe`. Did not overwrite `hp_v82.exe`.
fx2-manual **1,684,337** (−1,305 vs v82 fx2 1,685,642). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.

### v83urlpart - **1,688,185 / 1.609 bpc, REJECT +286 vs v83**

`HP_URLPART_MOD` URL host/path/query/fragment. Twin of HTLINK/EXTLINK. No RT.

### v83refidx - **1,688,044 / 1.609 bpc, REJECT +145 vs v83**

`HP_REFIDX_MOD` nth `<ref>` on the page. Twin of CITE/REFNAME. No RT.

H32 leftover queue empty. seclevel **+379**; brace3 **+285**; namedarg **+470**; include **+283**; sig **+275**; wikibold **−1,258**; urlpart **+286**; refidx **+145**. Do not reopen those seven (wikibold stays on champ). 8 MB champ stays v83. Do not run mem 26.

### H33 leftovers on v83 (queued)

Flags default off: `HP_PRESPACE_MOD` (leading-space preformatted line), `HP_EXTDISP_MOD` (`[http url display]` display text), `HP_PXSIZE_MOD` (`NNpx` size bucket), `HP_ENTNUM_MOD` (`&#` / `&#x` numeric entity), `HP_WIKIHR_MOD` (`----` line-start rule), `HP_FONTCOL_MOD` (`<font color=` / `size=` region), `HP_TBLDEPTH_MOD` (nested `{|` depth), `HP_UTF8ST_MOD` (UTF-8 lead/continuation). One at a time, mem 22, RSS cap 30 GB. Do not reopen H32.

### v83prespace - **1,688,511 / 1.610 bpc, REJECT +612 vs v83**

`HP_PRESPACE_MOD` leading-space preformatted line. Twin of LINE. No RT.

### v83extdisp - **1,688,480 / 1.610 bpc, REJECT +581 vs v83**

`HP_EXTDISP_MOD` `[http url display]` display text. Twin of EXTLINK/HTLINK. No RT.

### v83pxsize - **1,688,377 / 1.610 bpc, REJECT +478 vs v83**

`HP_PXSIZE_MOD` `NNpx` size bucket. Twin of FILEOPT/UNIT. No RT.

### v83entnum - **1,688,048 / 1.609 bpc, REJECT +149 vs v83**

`HP_ENTNUM_MOD` `&#` / `&#x` numeric entity. Twin of ENTITY/AMP. No RT.

### v83wikihr - **1,688,067 / 1.609 bpc, REJECT +168 vs v83**

`HP_WIKIHR_MOD` `----` line-start rule. Twin of LINE/RUNLEN. No RT.

### v83fontcol - **1,688,453 / 1.610 bpc, REJECT +554 vs v83**

`HP_FONTCOL_MOD` `<font color=` / `size=` region. Twin of STYLE/HTMLFMT. No RT.

### v83tbldepth - **1,688,402 / 1.610 bpc, REJECT +503 vs v83**

`HP_TBLDEPTH_MOD` nested `{|` depth. Twin of NEST/DEPTH. No RT.

### v83utf8st - **1,688,025 / 1.609 bpc, REJECT +126 vs v83**

`HP_UTF8ST_MOD` UTF-8 lead/continuation. Twin of SPARSE_UTF8. No RT.

H33 leftover queue empty. prespace **+612**; extdisp **+581**; pxsize **+478**; entnum **+149**; wikihr **+168**; fontcol **+554**; tbldepth **+503**; utf8st **+126**. Do not reopen. 8 MB champ stays v83. Do not run mem 26.

### H34 leftovers on v83 (queued)

Flags default off: `HP_DLTERM_MOD` (`;term : def` split), `HP_HEADCLOSE_MOD` (trailing heading `=`), `HP_WIKITIME_MOD` (`HH:MM` clock), `HP_LINKCOMMA_MOD` (comma in `[[` target), `HP_CATBLOCK_MOD` (consecutive `[[Category:` burst), `HP_BR_MOD` (`<br` tag), `HP_AMPNBSP_MOD` (`&nbsp;`), `HP_MDASH_MOD` (UTF-8 en/em dash). One at a time, mem 22, RSS cap 30 GB. Do not reopen H33.

### v83dlterm - **1,688,163 / 1.609 bpc, REJECT +264 vs v83**

`HP_DLTERM_MOD` `;term : def` split. Twin of LINE/INDENT. No RT.

### v83headclose - **1,688,229 / 1.610 bpc, REJECT +330 vs v83**

`HP_HEADCLOSE_MOD` trailing heading `=`. Twin of HEADING. No RT.

### v83wikitime - **1,688,051 / 1.609 bpc, REJECT +152 vs v83**

`HP_WIKITIME_MOD` `HH:MM` clock. Twin of DECIMAL/DIGITPOS. No RT.

### v83linkcomma - **1,688,355 / 1.610 bpc, REJECT +456 vs v83**

`HP_LINKCOMMA_MOD` comma in `[[` target. Twin of DAB/COMMAGAP. No RT.

### v83catblock - **1,688,366 / 1.610 bpc, REJECT +467 vs v83**

`HP_CATBLOCK_MOD` consecutive `[[Category:` burst. Twin of CAT. No RT.

### v83br - **1,688,060 / 1.609 bpc, REJECT +161 vs v83**

`HP_BR_MOD` `<br` tag. Twin of TAGNAME. No RT.

### v83ampnbsp - **1,688,038 / 1.609 bpc, REJECT +139 vs v83**

`HP_AMPNBSP_MOD` `&nbsp;`. Twin of ENTITY. No RT.

### v83mdash - **1,688,039 / 1.609 bpc, REJECT +140 vs v83**

`HP_MDASH_MOD` UTF-8 en/em dash. Twin of HYPHEN/UTF8ST. No RT.

H34 leftover queue empty. dlterm **+264**; headclose **+330**; wikitime **+152**; linkcomma **+456**; catblock **+467**; br **+161**; ampnbsp **+139**; mdash **+140**. Do not reopen. 8 MB champ stays v83. Do not run mem 26.

### H35 leftovers on v83 (queued)

Flags default off: `HP_MATH_MOD` (`<math>` TeX region), `HP_LISTMIX_MOD` (mixed `*#:;` line-prefix bitmask), `HP_PROTOCOL_MOD` (URL scheme class), `HP_HEXRUN_MOD` (`#` hex color/fragment), `HP_SQDEPTH_MOD` (nested `[[` depth), `HP_PIPEROLE_MOD` (`|` table/template/link role), `HP_AFTERREF_MOD` (just-closed `</ref>`), `HP_SENTPOS_MOD` (nth word in sentence). One at a time, mem 22, RSS cap 30 GB. Do not reopen H34.

### v83math - **1,688,028 / 1.609 bpc, REJECT +129 vs v83**

`HP_MATH_MOD` `<math>` TeX region. Twin of NOWIKI. No RT.

### v83listmix - **1,688,366 / 1.610 bpc, REJECT +467 vs v83**

`HP_LISTMIX_MOD` mixed `*#:;` line-prefix bitmask. Twin of LINE/INDENT/LISTLEVEL. No RT.

### v83protocol - **1,688,361 / 1.610 bpc, REJECT +462 vs v83**

`HP_PROTOCOL_MOD` URL scheme class. Twin of HTLINK. No RT.

### v83hexrun - **1,688,356 / 1.610 bpc, REJECT +457 vs v83**

`HP_HEXRUN_MOD` `#` hex color/fragment. Twin of ANCHOR/DIGITPOS. No RT.

### v83sqdepth - **1,688,119 / 1.609 bpc, REJECT +220 vs v83**

`HP_SQDEPTH_MOD` nested `[[` depth. Twin of NEST/STATE. No RT.

### v83piperole - **1,688,514 / 1.610 bpc, REJECT +615 vs v83**

`HP_PIPEROLE_MOD` `|` table/template/link role. Twin of STATE/BARIDX. No RT.

### v83afterref - **1,688,033 / 1.609 bpc, REJECT +134 vs v83**

`HP_AFTERREF_MOD` just-closed `</ref>`. Twin of CITE. No RT.

### v83sentpos - **1,687,931 / 1.609 bpc, REJECT +32 vs v83**

`HP_SENTPOS_MOD` nth word in sentence. Twin of WPOS/SENTLEN. No RT.

H35 leftover queue empty. math **+129**; listmix **+467**; protocol **+462**; hexrun **+457**; sqdepth **+220**; piperole **+615**; afterref **+134**; sentpos **+32**. Do not reopen. 8 MB champ stays v83. Do not run mem 26.

### H36 leftovers on v83 (queued)

Flags default off: `HP_ABBREV_MOD` (abbrev vs sentence-end after `.`), `HP_THOUSAND_MOD` (`1,234` grouping), `HP_REFPUNCT_MOD` (`.` glued to `<ref>`), `HP_QPERIOD_MOD` (`."` vs `".`), `HP_ELLIPSIS_MOD` (`...`), `HP_NUMRANGE_MOD` (digit-dash-digit), `HP_DEG_MOD` (degree after number), `HP_PERCENT_MOD` (digits then `%`). One at a time, mem 22, RSS cap 30 GB. Do not reopen H35.

### v83abbrev - **1,688,540 / 1.610 bpc, REJECT +641 vs v83**

`HP_ABBREV_MOD` abbrev vs sentence-end after `.`. Twin of INIT/DOTGAP. No RT.

### v83thousand - **1,688,092 / 1.609 bpc, REJECT +193 vs v83**

`HP_THOUSAND_MOD` `1,234` grouping. Twin of COMMAGAP/DECIMAL. No RT.

### v83refpunct - **1,688,054 / 1.609 bpc, REJECT +155 vs v83**

`HP_REFPUNCT_MOD` `.` glued to `<ref>`. Twin of CITE/AFTERREF. No RT.

### v83qperiod - **1,688,515 / 1.610 bpc, REJECT +616 vs v83**

`HP_QPERIOD_MOD` `."` vs `".`. Twin of QOCXT/WIKIBOLD. No RT.

### v83ellipsis - **1,688,040 / 1.609 bpc, REJECT +141 vs v83**

`HP_ELLIPSIS_MOD` `...`. Twin of DOTGAP/RUNLEN. No RT.

### v83numrange - **1,688,161 / 1.609 bpc, REJECT +262 vs v83**

`HP_NUMRANGE_MOD` digit-dash-digit. Twin of YEAR/HYPHEN. No RT.

### v83deg - **1,688,050 / 1.609 bpc, REJECT +151 vs v83**

`HP_DEG_MOD` degree after number. Twin of UNIT/COORD. No RT.

### v83percent - **1,688,041 / 1.609 bpc, REJECT +142 vs v83**

`HP_PERCENT_MOD` digits then `%`. Twin of UNIT. No RT.

H36 leftover queue empty. abbrev **+641**; thousand **+193**; refpunct **+155**; qperiod **+616**; ellipsis **+141**; numrange **+262**; deg **+151**; percent **+142**. Do not reopen. 8 MB champ stays v83. Do not run mem 26.

### H37 leftovers on v83 (queued)

Flags default off: `HP_STATETRANS_MOD` (prev wiki.state × current), `HP_COLRING_MOD` (4-row cell ring), `HP_LISTPARA_MOD` (list/quote-to-paragraph), `HP_SECFRAG_MOD` (`[[page#section]]`), `HP_WIKIVAR_MOD` (`{{PAGENAME}}` / `{{CURRENTYEAR}}`), `HP_SUBPAGE_MOD` (`[[Foo/Bar]]`), `HP_LINKNS_MOD` (`[[Namespace:` class), `HP_FCCUR_MOD` (mid-line fccxt first-char). One at a time, mem 22, RSS cap 30 GB. Do not reopen H36.

### v83statetrans - **1,688,074 / 1.609 bpc, REJECT +175 vs v83**

`HP_STATETRANS_MOD` prev wiki.state × current. Twin of STATE. No RT.

### v83colring - **1,688,385 / 1.610 bpc, REJECT +486 vs v83**

`HP_COLRING_MOD` 4-row cell ring. Twin of WIKISTACK/FCCXT. No RT.

### v83listpara - **1,688,492 / 1.610 bpc, REJECT +593 vs v83**

`HP_LISTPARA_MOD` list/quote-to-paragraph. Twin of PARA/LINE. No RT.

### v83secfrag - **1,688,148 / 1.609 bpc, REJECT +249 vs v83**

`HP_SECFRAG_MOD` `[[page#section]]`. Twin of ANCHOR/HEXRUN. No RT.

### v83wikivar - **1,688,017 / 1.609 bpc, REJECT +118 vs v83**

`HP_WIKIVAR_MOD` `{{PAGENAME}}` / `{{CURRENTYEAR}}`. Twin of MAGIC/TPLNAME. No RT.

### v83subpage - **1,688,085 / 1.609 bpc, REJECT +186 vs v83**

`HP_SUBPAGE_MOD` `[[Foo/Bar]]`. Twin of SLASHGAP. No RT.

### v83linkns - **1,688,496 / 1.610 bpc, REJECT +597 vs v83**

`HP_LINKNS_MOD` `[[Namespace:` class. Twin of CAT/LANG. No RT.

### v83fccur - **1,688,141 / 1.609 bpc, REJECT +242 vs v83**

`HP_FCCUR_MOD` mid-line fccxt first-char. Twin of LINE/WIKISTACK. No RT.

H37 leftover queue empty. statetrans **+175**; colring **+486**; listpara **+593**; secfrag **+249**; wikivar **+118**; subpage **+186**; linkns **+597**; fccur **+242**. Do not reopen. 8 MB champ stays v83. Do not run mem 26.

### H38 leftovers on v83 (closed)

Flags default off: `HP_PARAST_MOD` (is_paragraph × wiki.state), `HP_BOLDST_MOD` (wikibold × state), `HP_HEADBOLD_MOD` (heading × wikibold), `HP_BOLDLINE_MOD` (wikibold × line_kind), `HP_CAPPARA_MOD` (capmask × paragraph), `HP_NESTPARA_MOD` (nest × paragraph), `HP_CATPIPE_MOD` (in-Category × after-pipe), `HP_HEADPARA_MOD` (heading × paragraph). Joints of paying leftover CMs, not H11–H37 unaries. Five rejected vs v83; remaining three rejected vs v84. Do not reopen.

### v83parast - **1,688,198 / 1.609 bpc, REJECT +299 vs v83**

`HP_PARAST_MOD` is_paragraph × wiki.state. Twin of mixer wiki-gate / STATE/PARA. No RT.

### v83boldst - **1,688,077 / 1.609 bpc, REJECT +178 vs v83**

`HP_BOLDST_MOD` wikibold × wiki.state. Twin of WIKIBOLD/STATE. No RT.

### v83headbold - **1,688,151 / 1.609 bpc, REJECT +252 vs v83**

`HP_HEADBOLD_MOD` heading × wikibold. Twin of HEADING/WIKIBOLD. No RT.

### v83boldline - **1,688,121 / 1.609 bpc, REJECT +222 vs v83**

`HP_BOLDLINE_MOD` wikibold × line_kind. Twin of WIKIBOLD/LINE. No RT.

### v83cappara - **1,688,008 / 1.609 bpc, REJECT +109 vs v83**

`HP_CAPPARA_MOD` capmask × is_paragraph. Twin of CAPMASK/PARA. No RT.

### v83 SLOT_MAX=35 + HP_LR1_SCALE=40 → v84 - **1,680,397 / 1.602 bpc, RT PASS −7,502 vs v83**

Pulled mixer-rate work (`24bfcce`) onto the v83 leftover tree. Leftover CMs stay default-off. `hp_g_v83_lr40.exe` v83 flags + `HP_LR1_SCALE=40` `SLOT_MAX=35` mem 22: **1,680,397**. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v84.exe`. Did not overwrite `hp_v83.exe`.

### v84 fx2 - **1,677,124 / 1.599 bpc, RT PASS −7,213 vs v83 fx2**

`hp_g_v83_lr40.exe` on `data/enwik8.8mb.fx2man`: **1,677,124**. Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
New 8 MB champ is v84. Do not run mem 26.

### H38 remaining on v84

Recompile remaining joints on v84 flags (`HP_LR1_SCALE=40`). Champ **1,680,397**. One at a time, mem 22, RSS cap 30 GB. Do not reopen H37. Do not overwrite `hp_v84.exe` / `hp_v83.exe`.

| leftover | bytes | Δ vs v84 | RSS | wall | verdict |
|---|---:|---:|---:|---:|---|
| nestpara | 1,680,448 | **+51** | ~27.1 GB | 1093 s | **REJECT** |
| catpipe | 1,680,495 | **+98** | ~27 GB | 1115 s | **REJECT** |
| headpara | 1,680,504 | **+107** | ~27.1 GB | 1605 s | **REJECT** |

H38 **closed**. All eight joints reject (v83 five + v84 three). Do not reopen. Do not coarsen. Next: H39 on v84.

### H39 leftovers on v84

Flags default off: `HP_EXPECTCL_MOD` (dedicated `brackets_.closer()` CM, not BRK_CLOSE fold), `HP_REFGROUP_MOD` (`<ref>` anonymous / `name=` / `group=`), `HP_REFLIST_MOD` (`{{reflist}}` / `<references>` block), `HP_SISTER_MOD` (sister-project `[[wikt:` / `commons:` / `n:` / `s:` / `b:` / `v:` / `q:`), `HP_CONVERT_MOD` (`{{convert|` region), `HP_CN_MOD` (`{{cn}}` / `{{citation needed}}` / `{{fact}}` / `{{clarify}}`), `HP_BLOCK_MOD` (`<blockquote>` / `<center>` / `<div>` / `<span>` kind), `HP_PIPETRICK_MOD` (`[[target|]]` empty display). One at a time, mem 22, RSS cap 30 GB. Champ **1,680,397**. Do not reopen H11–H38. Do not overwrite `hp_v84.exe` / `hp_v83.exe`.

| leftover | bytes | Δ vs v84 | wall | verdict |
|---|---:|---:|---:|---|
| expectcl | 1,680,499 | **+102** | 1717 s | **REJECT** |
| refgroup | 1,680,191 | **−206** | 1676 s | **v85 RT PASS** |

### v85 identity - **1,680,191 / 1.602 bpc, RT PASS −206 vs v84**

`HP_REFGROUP_MOD` `<ref>` anonymous / `name=` / `group=`. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v85.exe`. Did not overwrite `hp_v84.exe`.
fx2-manual **1,676,908** (−216 vs v84 fx2 1,677,124). Decode SHA matches
`data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
New 8 MB champ is v85. Do not run mem 26.

### H40 Claude cheap protocol on v85

`SLOT_MAX=24` is the cheap compressor Claude calibrated: same v85 flags, smaller tables.

| cfg | 8 MiB bytes | Δ vs v85-35 | RSS | wall | bpc |
|---|---:|---:|---:|---:|---|
| v85 `SLOT_MAX=35` | 1,680,191 | — | ~27 GB | ~1676 s | 1.602 |
| v85 `SLOT_MAX=24` | **1,681,094** | **+903** | **1.53 GB** | 1440 s (CPU-shared) | 1.603 |

### H40 2 MiB screens on v85

Recompiled `hp_s_base.exe` from `v78_flags.ps1` (v85: `HP_LR1_SCALE=40`, `HP_REFGROUP_MOD=1`) with `SLOT_MAX=24`. Encoded `data/enwik8.2mb` `--mem 22` one at a time via `hp/tools/screen_rejects.ps1 -Name`. Did not overwrite `hp_v83.exe` / `hp_v84.exe` / `hp_v85.exe`. No mem 26. No `SLOT_MAX=35`. `hp_c_*` 8 MiB s24 jobs were allowed to share CPU; Get-HpBusy did not skip.

**Baseline** `%LOCALAPPDATA%\hp_lab\s_base.hp` (copied `hp/build/s_base.hp`): **439,496** / 1.676 bpc, 272 s.

H39 remaining leftover CMs (default-off, already in tree):

| leftover | flag | 2 MiB | Δ vs 439,496 | wall | verdict |
|---|---|---:|---:|---:|---|
| reflist | `HP_REFLIST_MOD` | 439,557 | **+61** | 358 s | kill |
| sister | `HP_SISTER_MOD` | 439,553 | **+57** | 252 s | kill |
| convert | `HP_CONVERT_MOD` | 439,553 | **+57** | 315 s | kill |
| cn | `HP_CN_MOD` | 439,558 | **+62** | 213 s | kill |
| block | `HP_BLOCK_MOD` | 439,552 | **+56** | 299 s | kill |
| pipetrick | `HP_PIPETRICK_MOD` | 439,558 | **+62** | 215 s | kill |
| expectcl | `HP_EXPECTCL_MOD` | 439,527 | **+31** | 321 s | kill |

None bytes-down. Closest is expectcl +31. No 8 MiB gate.

### H40 2 MiB historical re-screen

Closest historical 8 MB rejects at `HP_LR1_SCALE=40`, `SLOT_MAX=24`, `data/enwik8.2mb`. Archives stay in `%LOCALAPPDATA%\hp_lab\s_*.hp`. No mem 26. This pass did not run `SLOT_MAX=35`.

| leftover | 2 MiB bytes | Δ vs 439,496 | verdict |
|---|---:|---:|---|
| dmcgrow | 439,496 | **0** | tie — no 8 MiB |
| period | 439,570 | **+74** | kill |
| skip5 | 439,490 | **−6** | bytes-down (marginal) |
| sentpos | 439,378 | **−118** | bytes-down |
| skip40 | 439,433 | **−63** | bytes-down |
| cappara | 439,461 | **−35** | bytes-down |
| wikivar | 439,551 | **+55** | kill |
| pron | 439,438 | **−58** | bytes-down |
| utf8st | 439,547 | **+51** | kill |
| math | 439,555 | **+59** | kill |
| afterref | 439,551 | **+55** | kill |
| ampnbsp | 439,555 | **+59** | kill |
| mdash | 439,556 | **+60** | kill |
| ellipsis | 439,558 | **+62** | kill |
| percent | 439,435 | **−61** | bytes-down |

| leftover | flag | old 8 MB Δ | wall |
|---|---|---:|---:|
| dmcgrow | `HP_DMC_GROW` | +1 | 306 s |
| period | `HP_PERIOD_MOD` | +6 | 314 s |
| skip5 | `HP_SKIP5_MOD` | +8 | 211 s |
| sentpos | `HP_SENTPOS_MOD` | +32 | 308 s |
| skip40 | `HP_MIXER_SKIP=40` (replaces 32) | +52 | 323 s |
| cappara | `HP_CAPPARA_MOD` | +109 | 208 s |
| wikivar | `HP_WIKIVAR_MOD` | +118 | 346 s |
| pron | `HP_PRONOUN_MOD` | +122 | 495 s |
| utf8st | `HP_UTF8ST_MOD` | +126 | 509 s |
| math | `HP_MATH_MOD` | +129 | 222 s |
| afterref | `HP_AFTERREF_MOD` | +134 | 533 s |
| ampnbsp | `HP_AMPNBSP_MOD` | +139 | 232 s |
| mdash | `HP_MDASH_MOD` | +140 | 489 s |
| ellipsis | `HP_ELLIPSIS_MOD` | +141 | 536 s |
| percent | `HP_PERCENT_MOD` | +142 | 492 s |

6/15 flipped bytes-down on the 2 MiB screen (sentpos −118, skip40 −63, percent −61, pron −58, cappara −35, skip5 −6). dmcgrow identity. v86 continuation (SENTPOS in champ) screened sr…boldst below.

v86 2 MiB `SLOT_MAX=24` baseline **439,378** (SENTPOS in champ; old v85 baseline 439,496 kept as `s_base_v85.hp`). skip40 on that screen **439,311 (−67)**; 8 MiB s24 **1,680,231 (−402 vs v86 s24 1,680,633)**. Champ confirm `HP_MIXER_SKIP=40` at `SLOT_MAX=35`: **1,679,351 (−401 vs v86)**, identity RT PASS. Copied `hp_v87.exe`. Did not overwrite `hp_v86.exe`. fx2-manual **1,676,003** (−378 vs v86 fx2 1,676,381), RT PASS.

### H40 2 MiB historical re-screen on v86

Recompiled leftovers from current `v78_flags.ps1` (v86: `HP_LR1_SCALE=40`, `HP_REFGROUP_MOD=1`, `HP_SENTPOS_MOD=1`) with `SLOT_MAX=24`. Encoded `data/enwik8.2mb` `--mem 22` via `hp/tools/screen_rejects.ps1 -Name`. Did not overwrite `hp_v83.exe` / `hp_v84.exe` / `hp_v85.exe` / `hp_v86.exe`. No mem 26. No `SLOT_MAX=35`. One historical at a time while skip40 35-cap / `hp_v87` and H40 CM screens ran.

**Baseline** at encode time: **439,378** (v86 identity s24). Names after percent that were not yet on the v85 `screen_rejects.csv`.

| leftover | flag | old 8 MB Δ | 2 MiB | Δ vs 439,378 | wall | verdict |
|---|---|---:|---:|---:|---:|---|
| sr | `HP_SR_MOD` | +143 | 439,434 | **+56** | 202 s | kill |
| refidx | `HP_REFIDX_MOD` | +145 | 439,435 | **+57** | 192 s | kill |
| vowel | `HP_VOWEL_MOD` | +145 | 439,349 | **−29** | 202 s | bytes-down — no 8 MiB |
| fccxt | `HP_FCCXT_MOD` | +146 | 439,400 | **+22** | 201 s | kill |
| dotgap | `HP_DOTGAP_MOD` | +149 | 439,388 | **+10** | 212 s | kill |
| entnum | `HP_ENTNUM_MOD` | +149 | 439,440 | **+62** | 201 s | kill |
| deg | `HP_DEG_MOD` | +151 | 439,438 | **+60** | 212 s | kill |
| wikitime | `HP_WIKITIME_MOD` | +152 | 439,436 | **+58** | 212 s | kill |
| refpunct | `HP_REFPUNCT_MOD` | +155 | 439,433 | **+55** | 201 s | kill |
| br | `HP_BR_MOD` | +161 | 439,373 | **−5** | 201 s | bytes-down — no 8 MiB |
| year | `HP_YEAR_MOD` | +168 | 439,339 | **−39** | 201 s | bytes-down — no 8 MiB (−39 not < −40) |
| wikihr | `HP_WIKIHR_MOD` | +168 | 439,372 | **−6** | 202 s | bytes-down — no 8 MiB |
| statetrans | `HP_STATETRANS_MOD` | +175 | 439,288 | **−90** | 202 s | bytes-down → 8 MiB s24 |
| boldst | `HP_BOLDST_MOD` | +178 | 439,301 | **−77** | ~200 s | bytes-down → 8 MiB s24 |

8 MiB `SLOT_MAX=24` leftover gates vs v86 s24 **1,680,633** (`e8_8mb_v86s24.hp`). Archives `%LOCALAPPDATA%\hp_lab\e8_8mb_s24_{statetrans,boldst}.hp`. Binaries `hp_c_v86s24_statetrans.exe` / `hp_c_v86s24_boldst.exe`. Did not overwrite `hp_v86.exe`.

| leftover | 8 MiB s24 | Δ vs 1,680,633 | Δ vs v86-35 1,679,752 | Δ vs v87 1,679,351 | wall |
|---|---:|---:|---:|---:|---:|
| statetrans | **1,680,018** | **−615** | +266 | +667 | 767 s |
| boldst | **1,680,191** | **−442** | +439 | +840 | 947 s |

Both pay at s24 vs v86 s24. Neither is bytes-down vs v86-35 or v87. No `SLOT_MAX=35`. Next unscreened: subpage, cite, sentlen, thousand, …

Rank-8 sign-fix 2 MiB **599,778 (+160,282)** — still dead. Do not overwrite `hp_v85.exe`.

### H40 per-mixer rates (2 MiB SLOT_MAX=24)

Compile overrides `HP_LR1_R0`…`HP_LR1_R5` (default 0 = keep scaled value) applied **after** `HP_LR1_SCALE` to the first six mixer rates only. Extra-gate rates stay the scaled defaults. Not added to `v78_flags.ps1`. Integer-exact. Did not overwrite `hp_v83.exe` / `hp_v84.exe` / `hp_v85.exe`. Did not run `SLOT_MAX=35` or mem 26.

Cheap protocol: v85 flags (`HP_LR1_SCALE=40`, `HP_REFGROUP_MOD=1`), `SLOT_MAX=24`, `--mem 22`. Baseline `hp_s_base.exe` → `%LOCALAPPDATA%\hp_lab\s_base.hp` **439,496**. Scale-40 current `{1,1,1,2,1,2}` skipped. Trials `{1,2,3}` per mixer (12 jobs) plus R3=4 because 1/2/3 were all worse. R5=4 skipped (R5=1 paid). Archives `s_r{i}v{v}.hp`.

| i | current | v | 2 MiB | Δ | wall |
|---:|---:|---:|---:|---:|---:|
| 0 | 1 | 2 | 439,373 | **−123** | 248 s |
| 0 | 1 | 3 | 439,461 | **−35** | 221 s |
| 1 | 1 | 2 | 439,532 | +36 | 304 s |
| 1 | 1 | 3 | 439,563 | +67 | 292 s |
| 2 | 1 | 2 | 439,530 | +34 | 214 s |
| 2 | 1 | 3 | 439,708 | +212 | 208 s |
| 3 | 2 | 1 | 439,531 | +35 | 216 s |
| 3 | 2 | 3 | 439,590 | +94 | 213 s |
| 3 | 2 | 4 | 439,715 | +219 | 235 s |
| 4 | 1 | 2 | 439,571 | +75 | 319 s |
| 4 | 1 | 3 | 439,575 | +79 | 313 s |
| 5 | 2 | 1 | 439,474 | **−22** | 211 s |
| 5 | 2 | 3 | 439,515 | +19 | 220 s |

2 MiB paid: **R0=2 −123**, **R0=3 −35**, **R5=1 −22**. Those three compiled at `SLOT_MAX=24` and encoded `data/enwik8.8mb` → `%LOCALAPPDATA%\hp_lab\e8_8mb_s24_r*.hp`. Compare vs s24 v85 **1,681,094** (`e8_8mb_v85s24.hp`), not vs unmatched 35-cap champ unless an s24 job is bytes-down vs **1,680,191**.

| cfg | 8 MiB s24 | Δ vs 1,681,094 | Δ vs v85-35 | wall |
|---|---:|---:|---:|---:|
| R0=2 | **1,681,067** | **−27** | +876 | (sibling job) |
| R0=3 | 1,681,660 | **+566** | +1,469 | 890 s |
| R5=1 | **1,680,942** | **−152** | +751 | 1717 s |
| R0=2+R5=1 (sibling stack) | **1,680,890** | **−204** | +699 | — |

**Call.** A single scalar is not elementwise-optimal: 2 MiB wants R0=2 and R5=1. Transfer to 8 MiB s24 is noisy — R0=3 −35 on 2 MiB flipped to **+566**; R5=1 −22 grew to **−152** (best single at s24); R0=2 −123 shrank to **−27**. None of the s24 archives beat v85-35 **1,680,191**, so `SLOT_MAX=35` was not run. Do not land `{2,1,1,2,1,1}` on the champ without a 35-cap encode. Overrides stay default-0.

### H22 preprocess / richer stacks on v78

Flags: `HP_WIKISTACK_MOD` (fccxt+bracket+cell-above packed CM), `HP_REORDER` (sort `<page>` by title), `HP_PAYLOAD_LEX` (sort `<page>` by `<text>`), `--dict` on `hp_v78.exe`. One at a time after H21.



---

## H33 — mixer layer-1 learning rate (never swept in 191 trials)

Environment: Linux, g++ 13.3, 4 cores / 15 GB. The champ 79-flag set
(`hp/tools/v78_flags.ps1`) at `SLOT_MAX=35` peaks **13.3 GB RSS** and
OOMs on this box, so all runs below use `-DHP_SLOT_MAX=24` (1.6 GB).
That rung is validated against RECORD in H34.

**Finding.** `gate_rates()` (`hp/include/hp/predictor.hpp:1717`) does
`(void)base;` and returns a hardcoded `{2, 3, 2, 4, 3, 4}` whenever
`HP_PER_MIXER_LR` is on — which is the default and the champ. So `--lr`
never reaches layer 1; it only sets the layer-2 rate. The rates that
actually combine the 77 experts are source constants, unreachable from
the command line, and `grep -i 'lr|learning rate|mixer_lr'` over this
file returns **zero** sweeps in 191 logged trials.

New flag `HP_LR1_SCALE` (percent, 100 = identity, byte-identical at 100).

| `HP_LR1_SCALE` | effective rates | 8 MiB, mem 22, SLOT_MAX=24 | vs 100 |
|---:|---|---:|---:|
| 100 (current) | {2,3,2,4,3,4} | 1,690,052 | — |
| 75 | {2,2,2,3,2,3} | 1,685,761 | −4,291 |
| 60 | {1,2,1,2,2,2} | 1,683,710 | −6,342 |
| 50 | {1,2,1,2,2,2} | 1,683,710 | −6,342 |
| **40** | **{1,1,1,2,1,2}** | **1,683,014** | **−7,038** |
| 30 | {1,1,1,1,1,1} | 1,683,114 | −6,938 |

Minimum fully bracketed at **scale 40**, rates `{1,1,1,2,1,2}`. 50/60
collide (same rounding); at scale <=35 everything floors to all-ones and
gets worse again. **The layer-1 rates should be roughly halved, and the
optimum is not a uniform scale** — {1,1,1,2,1,2} is not 0.4x{2,3,2,4,3,4}
elementwise, so a per-rate sweep should do better still.

Both `HP_LR1_SCALE=60` and the best stack below **round-trip PASS**
(SHA256 `09f6dd72…f292ee8e` on both sides).

| cfg (SLOT_MAX=24) | 8 MiB | bpc | RT |
|---|---:|---:|---|
| champ flags, scale 100 | 1,690,052 | 1.6122 | — |
| + `HP_LR1_SCALE=60` | 1,683,710 | 1.6062 | PASS |
| + `HP_LR1_SCALE=40` | 1,683,014 | 1.6055 | — |
| + `HP_LR1_SCALE=60` + `HP_WIKIBOLD_MOD` | 1,682,129 | 1.6043 | — |
| + `HP_LR1_SCALE=40` + `HP_WIKIBOLD_MOD` | **1,681,311** | **1.6035** | PASS |

**Mechanism.** The layer-1 update is unnormalised LMS
(`simd_dot.hpp:110`): `dw_i = (st_i * err * l1) >> 14`, a *fixed* step.
The gradient scales with input energy `||st||^2`, which grows with the
number of mixer inputs. hp went from ~28 inputs to 77 without retuning,
so the effective step size rose ~3x and the mixer has been running
over-adapted ever since. Greedy single-flag search cannot find this:
it is not a flag, it is a constant shared by every expert.

## H34 — proxy calibration: what a cheap screen is actually worth

12 flags from H28–H32 with known 8 MiB deltas against the identical v82
base were re-measured on cheap proxies, plus `HP_WIKIBOLD_MOD` (the one
known **accept**, −1,258).

| screen | cost / RAM | Spearman vs RECORD | sign accuracy |
|---|---|---:|---:|
| 12 x 256 KB spread windows | 190 s / 1.5 GB | +0.50 | 13/13 |
| 2 MiB contiguous head | 92 s / 1.5 GB | **+0.84** | 13/13 |
| 8 MiB, SLOT_MAX=24 | 500 s / 1.6 GB | **+0.96** (Pearson) | 4/4 |

- Both cheap screens classify accept-vs-reject **perfectly (13/13)**,
  including the true accept. They are sound go/no-go gates.
- Spread 256 KB windows **rank worse** than one contiguous 2 MiB slice,
  despite better corpus composition: 12 independent cold starts inject
  warm-up variance that swamps deltas of a few hundred bytes.
- `SLOT_MAX=24` vs the champ's 35 costs **+895 B (+0.05%)** on the base
  and reproduces per-flag deltas at ratio 0.94–1.07. **8x the RAM buys
  0.05% of the bytes at 8 MiB.**

**Revised protocol** (≈10x faster loop, 8x less RAM):
screen on 2 MiB head @ SLOT_MAX=24 → gate on 8 MiB @ SLOT_MAX=24 →
champ confirmation only at SLOT_MAX=35.

Corpus note: the 8 MiB gate slice is **43.8% `#REDIRECT` stubs** vs
33.7% for full enwik8 and 84.1% for the 1 MB slice. The gate is
over-weighted toward repetitive boilerplate.

## H35 — pairwise interaction: rejects are additive, never synergistic

Six near-miss rejects on six different axes, 2 MiB head, SLOT_MAX=24.
interaction = d(A+B) − d(A) − d(B).

| pair | interaction |
|---|---:|
| HEADIDX + {SIG, LINKTRAIL, BRACE3, HTMLFMT, DECIMAL} | −6 … +1 (**additive**) |
| SIG + {LINKTRAIL, BRACE3, HTMLFMT, DECIMAL} | +98 … +120 (**interference**) |

**Zero synergistic pairs.** Combining rejected features does not rescue
them: their costs sum, or worse. Do not reopen pair/group search over
the reject pile on this axis.

## H36 — the dilution tax is a function of the learning rate

Same six flags, measured as marginal cost on top of two different
layer-1 rates (2 MiB head):

| flag | cost @ scale 100 | cost @ scale 60 |
|---|---:|---:|
| HP_HEADIDX_MOD | +79 | **+2** |
| HP_BRACE3_MOD | +91 | +19 |
| HP_LINKTRAIL_MOD | +87 | +22 |
| HP_SIG_MOD | +88 | +26 |
| HP_HTMLFMT_MOD | +81 | +28 |
| HP_DECIMAL_MOD | +99 | +36 |
| HP_WIKIBOLD_MOD (true accept) | −211 | **−275** |

Mean reject cost **87.5 → 22.2 B, a 75% reduction**, while the genuine
win got *better*. The flat +145…+700 reject band of H29–H32 was largely
a **mis-tuned mixer**, not 24 independently bad ideas. The 116 historical
rejects were scored against an over-adapted mixer and are not safely
closed; they deserve re-screening at the corrected rate.

## H33–H36 negative results (closed)

- **Layer-2 `--lr`**: default 2 is optimal; monotone worse above
  (12-window totals: lr1 786,383 / lr2 786,143 / lr3 786,394 /
  lr10 789,607 / lr32 806,179). Do not retest.
- **`HP_MIXER_CLAMP_BITS`**: 18/20/22/24 are **byte-identical**
  (786,145); 14 costs +1,571. Weights never reach ±2^18, so the ±1.0
  clamp is not binding. Do not retest.
- **`HP_MIXER_BACKPROP`** (new): train layer-1 on the backpropagated
  final error instead of its own local error → 890,133 vs 441,538 on
  2 MiB, catastrophic. Local-error training is correct for this
  architecture. Do not retest.
- **`HP_MIXER_NLMS`** (new): normalise the layer-1 step by per-bit input
  energy. Best `HP_NLMS_ETYP=10^7` → 443,021 vs 441,538 baseline; worse
  at every setting tried (2.5e6 → 447,733; 8e7 → 471,954). The
  diagnosis (fixed step, varying energy) is right but this
  normalisation is not the fix — `HP_LR1_SCALE` is. Flag left in, off.

## Repo hygiene found while reproducing

1. The build command in `README.md` and `PLAN.md` **does not compile**:
   it omits `-msse4.1` and `-I hp/third_party/xsimd/include`.
2. A default-flag build is **1,804,979 / 1.721 bpc** on 8 MiB — v7-era.
   Only 10 of 568 flags are default-ON; the champ is the 79-flag `-D`
   set recorded *only* in `hp/tools/v78_flags.ps1`. Every accepted
   feature from v45 to v83 is default-OFF. The documented build does
   not build the champion.
3. `HP_MIXER_RANK`'s V-factor update folds `U[f]` into the learning
   *rate* (`mixer.hpp:131-136`) and then clamps to `[1, 4095]`, which
   **discards the sign** when `U[f] < 0`; `ufac_` is also initialised
   exactly at `kMixerClamp`. Unfixed bug — likely why rank never paid.

## H33 hygiene #3

Code-fixed in `mixer.hpp` (rank path still default-off, `HP_MIXER_RANK=0`).
V-factor `l1k` keeps the sign of `U[f]`: magnitude is the old
`(l1 * (|Uk| >> 8) + 128) >> 8` with floor 1 and cap 4095, then negated
if `Uk < 0` — a negative factor no longer forces `l1k = +1`. `ufac_`
init is the same packed `w0 = (1<<16)/n_inputs` as full-rank `W`, not
`kMixerClamp` / `(1<<16)`. Sign-fix 2 MiB retest: H40 below, REJECT
+160,282; 8 MB not run.

## Next-test #2 — 2 MiB leftover-reject screen harness (2026-09-18)

Added `hp/tools/screen_rejects.ps1`. Dotsources `v78_flags.ps1` (already
`HP_LR1_SCALE=40`), forces `SLOT_MAX=24`, compiles `hp/build/hp_s_<name>.exe`
one flag at a time, encodes `data/enwik8.2mb` → `%LOCALAPPDATA%\hp_lab\s_<name>.hp`.
Does not overwrite `hp_v83.exe` / `hp_v84.exe`. Default path is baseline only;
`-RunQueue` is the explicit leftover marathon.

`data/enwik8.2mb` created as the first **2,097,152** bytes of `data/enwik8.8mb`.

Baseline binary `hp/build/hp_s_base.exe` recompiled on v85 flags (scale 40,
`HP_REFGROUP_MOD`, `SLOT_MAX=24`) and encoded: **439,496**. See **H40 2 MiB
screens on v85**.

Queue **158** leftover flags (H11–H38, all already in `features.hpp`).
Priority: H38 remaining joints (`nestpara` / `catpipe` / `headpara`), then
closest historical 8 MB misses (`dmcgrow +1`, `period +6`, `skip5 +8`,
`sentpos +32`, `skip40 +52`, `cappara +109`, …). H22 reorder / payload_lex /
dict omitted (preprocess). Closest after the joints: dmcgrow, period, skip5,
sentpos, skip40, cappara, wikivar, pron, utf8st, math, afterref, ampnbsp,
mdash, ellipsis, percent, sr, refidx, vowel, fccxt, dotgap, entnum, deg,
wikitime, refpunct, br, year, wikihr, statetrans, boldst, subpage, cite,
sentlen, thousand. Full list via `screen_rejects.ps1 -List`.

## H40 MIXER_RANK=8 sign-fix retest (SLOT_MAX=24)

Cheap protocol only. Did not overwrite `hp_v83.exe` / `hp_v84.exe` /
`hp_v85.exe`. Did not run `SLOT_MAX=35` or mem 26. Rank remains default-off.

Compiled `hp/build/hp_c_v85rank8s24.exe` from `v78_flags.ps1` (v85:
`HP_LR1_SCALE=40`, `HP_REFGROUP_MOD=1`) with `-DHP_SLOT_MAX=24`
`-DHP_MIXER_RANK=8`. Compile OK (268,299 B, 7.5 s). Champ binaries
untouched (`hp_v83.exe` 265,457 / `hp_v84.exe` 266,575 / `hp_v85.exe`
266,993).

**2 MiB** `data/enwik8.2mb` `--mem 22` → `%LOCALAPPDATA%\hp_lab\s_rank8.hp`:
**599,778** / 2.287 bpc, 632 s, RSS 1.52 GB.

vs `s_base.hp` **439,496**: **+160,282**. Worse than the +20 k blow-up
stop. **REJECT.** 8 MiB not run (`e8_8mb_v85rank8s24.hp` not written).
`hp_c_v85s24` 8 MB identity was already in flight; rank-8 8 MB would
have waited anyway.

This is not the unfixed H6 result (`+702,383` at `SLOT_MAX=35`,
2,407,863). Sign-preserving `l1k` + `ufac_` init-to-w0 did not recover
rank-8 on the 2 MiB screen. Do not claim champ accept (no 8 MB @35
bytes-down vs **1,680,191**). Do not sweep rank 4/16.

### H40 leftover CMs on v86 (closed)

Eight new default-off wiki-domain ContextModels. Not in `v78_flags.ps1`.
Recompiled after `HP_SENTPOS_MOD` landed in champ flags (old `hp_s_notes.exe`
etc. were pre-SENTPOS). Did not overwrite `hp_v83.exe` / `hp_v84.exe` /
`hp_v85.exe` / `hp_v86.exe`. Did not run `SLOT_MAX=35` or mem 26. Screen:
v86 flags + `SLOT_MAX=24` + one `-D`, `hp/tools/screen_rejects.ps1 -Name`,
`data/enwik8.2mb` vs v86 base **439,378** (`s_base.hp` rewritten 14:26 UTC,
not 439,496).

| leftover | flag | salt | scanner | 2 MiB bytes | Δ vs 439,378 | verdict |
|---|---|---:|---|---:|---:|---|
| notes | `HP_NOTES_MOD` | 218 | `{{notelist}}` / `{{notes}}` / `{{notefoot}}` region (not reflist) | 439,434 | **+56** | kill |
| langtpl | `HP_LANGTPL_MOD` | 219 | `{{lang\|` / `{{lang-xx\|` (not LANG 2-letter link prefix) | 439,439 | **+61** | kill |
| frac | `HP_FRAC_MOD` | 220 | `{{frac\|` / `{{sfrac\|` | 439,435 | **+57** | kill |
| listen | `HP_LISTEN_MOD` | 221 | `{{listen\|` / `{{audio\|` / `[[File:… .ogg/.oga/.mp3]]` | 439,446 | **+68** | kill |
| birth | `HP_BIRTH_MOD` | 222 | `{{birth date` / `{{death date` / `{{birth-date` / `{{death-date` | 439,437 | **+59** | kill |
| hlist | `HP_HLIST_MOD` | 223 | `{{hlist` / `{{plainlist` / `{{unbulleted list` | 439,429 | **+51** | kill |
| mainart | `HP_MAINART_MOD` | 224 | `{{main\|` / `{{see also\|` / `{{further\|` after a heading | 439,438 | **+60** | kill |
| chem | `HP_CHEM_MOD` | 225 | `<chem>` region (not MATH `<math>`) | 439,430 | **+52** | kill |

ISBN13 skipped (PUBID reject). WIKITABLE skipped (TABLECLASS reject).
Replaced slot 8 with CHEM.

Wall: notes 189 s, langtpl 211 s, frac 226 s, listen 219 s, birth 188 s,
hlist 191 s, mainart 192 s, chem 186 s. Archives `%LOCALAPPDATA%\hp_lab\s_*.hp`.
All eight bytes-up. Closest: hlist +51, chem +52. **No 8 MiB gate** (v86
s24 8 MiB `e8_8mb_v86s24.hp` **1,680,633** exists; unused). Champ binaries
untouched (`hp_v83.exe` 265,457 / `hp_v84.exe` 266,575 / `hp_v85.exe`
266,993 / `hp_v86.exe` 267,599).

### H40 v86 re-screen

v85 2 MiB bytes-down leftovers re-screened on **v86** (`HP_SENTPOS_MOD` +
`HP_REFGROUP_MOD` + `HP_LR1_SCALE=40`). Cheap protocol: `SLOT_MAX=24`,
`--mem 22`, `hp/tools/screen_rejects.ps1 -Name`. Recompiled `hp_s_*`
(pre-SENTPOS binaries were stale). Did not overwrite `hp_v83.exe` /
`hp_v84.exe` / `hp_v85.exe` / `hp_v86.exe`. No mem 26.

**2 MiB baseline** `%LOCALAPPDATA%\hp_lab\s_base.hp` (v86 flags, rewritten
after 14:20 UTC): **439,378** (old v85-no-SENTPOS 439,496 saved as
`s_base_v85.hp`). Matches the v85 sentpos leftover.

| leftover | 2 MiB | Δ vs 439,378 | v85 Δ | 8 MiB s24 | Δ vs 1,680,633 | Δ vs v86-35 | call |
|---|---:|---:|---:|---:|---:|---:|---|
| skip40 | 439,311 | **−67** | −63 | **1,680,231** | **−402** | +479 vs v86-35; **−401 at 35-cap** | **v87** |
| percent | 439,435 | **+57** | −61 | — | | | **kill** (flipped) |
| pron | 439,317 | **−61** | −58 | **1,680,412** | **−221** | +660 | s24 paid; no 35 |
| cappara | 439,343 | **−35** | −35 | **1,680,391** | **−242** | +639 | s24 paid; no 35 |
| skip5 | 439,370 | **−8** | −6 | **1,680,603** | **−30** | +851 | s24 paid; no 35 (<200 vs s24) |

v86 s24 8 MiB champ compiled `hp_c_v86s24.exe` (v78 flags, `SLOT_MAX=24`,
no extra leftover) → `%LOCALAPPDATA%\hp_lab\e8_8mb_v86s24.hp`:
**1,680,633** (same as v85s24+sentpos), 867 s, RSS 1.54 GB. Leftover
archives `e8_8mb_v86s24_<n>.hp`. skip40 s24 was already in flight when
the champ encode started.

skip40 35-cap later paid **1,679,351 (−401)** and landed **v87**. pron /
cappara / skip5 beat v86 s24 but stayed above the 35-cap champ. No
`SLOT_MAX=35` for those.

### H40 v87 2 MiB re-screen

Champ is **v87** (`HP_MIXER_SKIP=40` + SENTPOS + REFGROUP + LR1_SCALE=40).
Rewrote `%LOCALAPPDATA%\hp_lab\s_base.hp` from current `v78_flags.ps1`
`SLOT_MAX=24`: **439,311** (198 s). Same bytes as the v86 skip40 2 MiB
leftover. Copy `s_base_v87.hp`. `hp_s_base.exe` rebuilt 16:08 UTC. Did
not overwrite `hp_v83.exe`–`hp_v87.exe`. No mem 26.

v86-flag 8 MiB s24 leftover gates (SKIP still 32) vs v86 s24 **1,680,633**:
statetrans **1,680,018 (−615)**; boldst **1,680,191 (−442)**. Both still
above v87 35-cap **1,679,351**.

v87 s24 8 MiB champ `hp_c_v87s24.exe` → `e8_8mb_v87s24.hp` **1,680,231**
(matches v86 skip40 s24). Copied to `hp/build`. 35-cap only if a v87
leftover s24 is **>200 B** under that archive.

8 MiB leftover s24 vs **1,680,231** (`e8_8mb_v87s24_{n}.hp`):

| leftover | 8 MiB s24 | Δ vs 1,680,231 | vs v87-35 1,679,351 | verdict |
|---|---:|---:|---:|---|
| cappara | **1,679,989** | **−242** | +638 | s24 paid; 35-cap bytes-ok but RSS abort |
| statetrans | **1,680,018** | **−213** | +667 | s24 paid; 35-cap bytes-ok but RSS abort |
| fccur | **1,680,044** | **−187** | +693 | s24 paid; no 35 (<200) |
| pron | **1,680,049** | **−182** | +698 | s24 paid; no 35 (<200) |

suffix 8 MiB s24 in flight (`hp_c_v87s24_suffix.exe`). No third concurrent
8 MiB. Two 35-cap attempts (`hp_c_v87s35_statetrans` / `hp_c_v87_statetrans`)
**aborted**: RSS **27 GB > 20 GB**. No mem 26. Did not overwrite
`hp_v83.exe`–`hp_v87.exe`.

2 MiB leftover screens vs `s_base.hp` **439,311**. Cheap protocol:
`SLOT_MAX=24`, `--mem 22`, `hp/tools/screen_rejects.ps1 -Name`. Max two
2 MiB. Did not overwrite `hp_v83.exe`–`hp_v87.exe`. Gate 8 MiB s24 only
if Δ < **−20**.

| leftover | 2 MiB | Δ vs 439,311 | v86 Δ | verdict |
|---|---:|---:|---:|---|
| base | 439,311 | 0 | — | baseline |
| statetrans | 439,288 | **−23** | −90 | bytes-down → 8 MiB s24 **1,680,018 (−213)**; 35-cap RSS abort |
| boldst | 439,301 | **−10** | −77 | bytes-down; no 8 MiB |
| cite | 439,372 | **+61** | — | **kill** |
| subpage | 439,376 | **+65** | — | **kill** |
| sentlen | 439,324 | **+13** | — | **kill** |
| thousand | 439,369 | **+58** | — | **kill** |
| pron | 439,273 | **−38** | −61 | bytes-down → 8 MiB s24 **1,680,049 (−182)**; no 35 |
| year | 439,339 | **+28** | −39 | **kill** (flipped) |
| sqdepth | 439,315 | **+4** | — | **kill** |
| boldline | 439,322 | **+11** | — | **kill** |
| suffix | 439,283 | **−28** | +230 | bytes-down → 8 MiB s24 in flight |
| markdist | 439,319 | **+8** | — | **kill** |
| sfn | 439,374 | **+63** | — | **kill** |
| geotemp | 439,369 | **+58** | — | **kill** |
| epigraph | 439,369 | **+58** | — | **kill** |
| redir | 439,378 | **+67** | — | **kill** |
| fccur | 439,268 | **−43** | +242 | bytes-down → 8 MiB s24 **1,680,044 (−187)**; no 35 |
| cappara | 439,272 | **−39** | −35 | bytes-down → 8 MiB s24 **1,679,989 (−242)**; 35-cap RSS abort |
| vowel | 439,292 | **−19** | −29 | bytes-down; no 8 MiB |
| skip5 | 439,321 | **+10** | −8 | **kill** (flipped) |
| br | 439,373 | **+62** | −5 | **kill** (flipped) |
| wikihr | 439,372 | **+61** | −6 | **kill** (flipped) |
| tracklist | 439,363 | **+52** | — | **kill** |
| succession | 439,365 | **+54** | — | **kill** |
| colstart | 439,360 | **+49** | — | **kill** |
| toc | 439,383 | **+72** | — | **kill** |
| headbold | 439,338 | **+27** | — | **kill** |
| month | 439,348 | **+37** | — | **kill** |
| refbegin | 439,368 | **+57** | — | **kill** |
| headidx | 439,316 | **+5** | — | **kill** |
| secfrag | 439,377 | **+66** | — | **kill** |
| commagap | 439,344 | **+33** | — | **kill** |
| dlterm | 439,371 | **+60** | — | **kill** |

H41 leftover CMs implemented default-off salts 226–233: `HP_SFN_MOD`,
`HP_GEOTEMP_MOD`, `HP_EPIGRAPH_MOD`, `HP_TRACKLIST_MOD`,
`HP_SUCCESSION_MOD`, `HP_COLSTART_MOD`, `HP_TOC_MOD`, `HP_REFBEGIN_MOD`.
Not in `v78_flags.ps1`. Not CITE / COORD / BLOCK / TABLECLASS / ANCHOR /
MAGIC / REFLIST twins.

### H41 leftover CMs on v87 (closed)

Eight new default-off wiki-domain ContextModels. Not in `v78_flags.ps1`.
Did not overwrite `hp_v83.exe` / `hp_v84.exe` / `hp_v85.exe` / `hp_v86.exe`
/ `hp_v87.exe`. Did not run a completed `SLOT_MAX=35` leftover (35-cap
statetrans aborted at 27 GB RSS). Not CITE / COORD / BLOCK / TABLECLASS /
ANCHOR / MAGIC / REFLIST twins. Do not reopen H11–H40. All eight 2 MiB
screens **kill**.

Compiled `hp_s_{sfn,geotemp,epigraph,tracklist,succession,colstart,toc,refbegin}.exe`
from v87 flags + `SLOT_MAX=24` + one `-D`. Screen vs rewritten
`s_base.hp` **439,311** (v87 / `HP_MIXER_SKIP=40`, also `s_base_v87.hp`).

| leftover | flag | salt | scanner | 2 MiB bytes | Δ vs 439,311 | verdict |
|---|---|---:|---|---:|---:|---|
| sfn | `HP_SFN_MOD` | 226 | `{{sfn\|` / `{{harvnb\|` / `{{harv\|` (not CITE `<ref>`, not REFGROUP) | 439,374 | **+63** | **kill** |
| geotemp | `HP_GEOTEMP_MOD` | 227 | `{{coord\|` 1-bit IN-template (not COORD lat/lon unary) | 439,369 | **+58** | **kill** |
| epigraph | `HP_EPIGRAPH_MOD` | 228 | `{{quote box` / `{{quotebox` / `{{cquote` / `{{blockquote` (not BLOCK html) | 439,369 | **+58** | **kill** |
| tracklist | `HP_TRACKLIST_MOD` | 229 | `{{tracklist` / `{{Track listing` | 439,363 | **+52** | **kill** |
| succession | `HP_SUCCESSION_MOD` | 230 | `{{s-start` / `{{succession box` / `{{s-bef` / `{{s-ttl` | 439,365 | **+54** | **kill** |
| colstart | `HP_COLSTART_MOD` | 231 | `{{col-begin` / `{{div col` / `{{columns-list` (not TABLECLASS) | 439,360 | **+49** | **kill** |
| toc | `HP_TOC_MOD` | 232 | `__TOC__` / `__NOTOC__` / `__FORCETOC__` 2-bit sticky (not MAGIC hash) | 439,383 | **+72** | **kill** |
| refbegin | `HP_REFBEGIN_MOD` | 233 | `{{refbegin` / `{{refend}}` block (not REFLIST / `<references>`) | 439,368 | **+57** | **kill** |

CITEWEB / WIKITABLEID / ANCHORID skipped (CITE / TABLECLASS / ANCHOR rejects).
Replaced with SFN / COLSTART / TOC. Wall ~190–210 s. All eight bytes-up.
Closest: colstart +49, tracklist +52. **No 8 MiB gate.** Champ binaries
untouched (`hp_v83.exe` 265,457 / `hp_v84.exe` 266,575 / `hp_v85.exe`
266,993 / `hp_v86.exe` 267,599 / `hp_v87.exe` 267,599).

### H40 v87 historical leftovers after boldst

Queue after year/wikihr/statetrans/boldst on **v87** flags
(`HP_MIXER_SKIP=40` + SENTPOS), `SLOT_MAX=24`, `--mem 22`,
`data/enwik8.2mb`. Recompiled `hp_s_*` from current `v78_flags.ps1`.
Waited for parent `s_base.hp` rewrite after 16:04 UTC: **439,311**
(16:08:48 UTC; not v86 439,378). Did not overwrite `hp_v83.exe`–
`hp_v87.exe`. No mem 26. No `SLOT_MAX=35`. Did not rerun H40 CMs
notes…chem. 2 MiB only, two max. Gate 8 MiB s24 if Δ < **−40**.

| leftover | 2 MiB | Δ vs 439,311 | old 8 MB Δ | verdict |
|---|---:|---:|---:|---|
| subpage | 439,376 | **+65** | +186 | **kill** |
| cite | 439,372 | **+61** | +187 | **kill** |
| sentlen | 439,324 | **+13** | +187 | **kill** |
| thousand | 439,369 | **+58** | +193 | **kill** |
| sqdepth | 439,315 | **+4** | +220 | **kill** |
| boldline | 439,322 | **+11** | +222 | **kill** |
| suffix | 439,283 | **−28** | +230 | bytes-down; no 8 MiB (not >40) |
| markdist | 439,319 | **+8** | +233 | **kill** |
| redir | 439,378 | **+67** | +234 | **kill** |
| fccur | 439,268 | **−43** | +242 | bytes-down → 8 MiB s24 left to flip-candidate (`hp_c_v87s24_fccur` in flight) |
| headidx | 439,316 | **+5** | +246 | **kill** |
| secfrag | 439,377 | **+66** | +249 | **kill** |
| headbold | 439,338 | **+27** | +252 | **kill** |
| month | 439,348 | **+37** | +254 | **kill** |
| commagap | 439,344 | **+33** | +256 | **kill** |

revcomment not finished (slot contention; incomplete `s_revcomment.hp`).
Next unscreened after this wave: numrange, dlterm, isse, baridx, …

### H40 v87 re-screen

v86 2 MiB bytes-down leftovers re-screened on **v87** (`HP_MIXER_SKIP=40`
+ SENTPOS + REFGROUP + `HP_LR1_SCALE=40`). Waited for parent
`s_base.hp` rewrite after 16:04 UTC (`hp_s_base` idle). Cheap protocol:
`SLOT_MAX=24`, `--mem 22`, `hp/tools/screen_rejects.ps1 -Name`.
Recompiled each leftover (old `hp_s_pron.exe` was SKIP=32). Did not
overwrite `hp_v83.exe` / `hp_v84.exe` / `hp_v85.exe` / `hp_v86.exe` /
`hp_v87.exe`. No mem 26.

**2 MiB baseline** `%LOCALAPPDATA%\hp_lab\s_base.hp` (16:08:48 UTC):
**439,311** (copy `s_base_v87.hp`). Same bytes as the v86 skip40 leftover.

**8 MiB s24 champ** `hp_c_v87s24.exe` (v78 flags, `SLOT_MAX=24`, no extra)
→ `e8_8mb_v87s24.hp`: **1,680,231** (matches v86 skip40 s24), 766 s.

| leftover | 2 MiB | Δ vs 439,311 | v86 Δ | 8 MiB s24 | Δ vs 1,680,231 | call |
|---|---:|---:|---:|---:|---:|---|
| pron | 439,273 | **−38** | −61 | **1,680,049** | **−182** | s24 paid; no 35 (<200) |
| year | 439,339 | **+28** | −39 | — | | **kill** (flipped) |
| cappara | 439,272 | **−39** | −35 | **1,679,989** | **−242** | s24 paid; 35-cap bytes-ok |
| vowel | 439,292 | **−19** | −29 | — | | bytes-down; no 8 MiB (≤20) |
| skip5 | 439,321 | **+10** | −8 | — | | **kill** (flipped) |
| wikihr | 439,372 | **+61** | −6 | — | | **kill** (flipped) |
| br | 439,373 | **+62** | −5 | — | | **kill** (flipped) |
| statetrans | 439,288 | **−23** | −90 | **1,680,018** | **−213** | s24 paid; 35-cap bytes-ok |
| boldst | 439,301 | **−10** | −77 | — | | bytes-down; no 8 MiB |

Stopped on request: no further `hp_s_*` / `hp_c_*` encodes. Own leftovers
were already idle. **35-cap runs alone** (`hp_g_v87statetrans` →
`e8_8mb_v87statetrans.hp`). Did not start cappara 35.

### v87statetrans → v88 - **1,679,112 / 1.601 bpc, RT PASS −239 vs v87**

`HP_STATETRANS_MOD` on v87 flags, `SLOT_MAX=35`, mem 22. Waited for
existing `hp_g_v87statetrans` pid 12404 (did not start a second encode).
Archive `%LOCALAPPDATA%\hp_lab\e8_8mb_v87statetrans.hp` **1,679,112**
(−239 vs v87 1,679,351). Copied `hp/build/e8_8mb_v87statetrans.hp`.
Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v88.exe` from `hp_g_v87statetrans.exe`. Did not overwrite
`hp_v87.exe`.

### v88 fx2 - **1,675,790 / 1.598 bpc, RT PASS −213 vs v87 fx2**

`hp_g_v87statetrans.exe` on `data/enwik8.8mb.fx2man`: **1,675,790**.
Decode SHA matches `data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
Copied `hp/build/e8_8mb_v87statetrans_fx2.hp`. New 8 MB champ is v88.
Did not run mem 26. Did not overwrite `hp_v87.exe`.

v88 2 MiB `SLOT_MAX=24` baseline `s_base.hp` **439,288** (192 s). Matches the
v87 statetrans leftover. Old v87 baseline 439,311 saved as `s_base_v87.hp`.
`HP_STATETRANS_MOD=1` added to `v78_flags.ps1`. `hp_v88.exe` protected in
`screen_rejects.ps1`.

8 MiB leftover s24 vs **1,680,018**:

Closed numbers: **H42 v88 re-screen** below. In-flight stub kept only as a pointer.

| leftover | 2 MiB | Δ vs 439,288 | 8 MiB s24 | Δ vs 1,680,018 |
|---|---:|---:|---:|---:|
| cappara | 439,253 | **−35** | **1,679,762** | **−256** |
| fccur | 439,261 | **−27** | **1,679,691** | **−327** |
| pron | 439,247 | **−41** | **1,679,829** | **−189** |
| suffix | 439,269 | **−19** | — | no 8 |
| vowel | 439,268 | **−20** | — | no 8 |
| boldst | 439,294 | **+6** | — | kill |

### H40 v88 historical leftovers after isse

Queue after dlterm/isse/numrange/commagap/headbold/month (v87 decisions,
not recompiled). **v88** flags (`HP_STATETRANS_MOD` + SKIP=40 + SENTPOS),
`SLOT_MAX=24`, `--mem 22`, `data/enwik8.2mb`. Recompiled `hp_s_baridx.exe`
/ `hp_s_temppos.exe` from current `v78_flags.ps1`. Waited for parent
`s_base.hp` rewrite: **439,288** (4:11:32; not v87 439,311). Did not
overwrite `hp_v83.exe`–`hp_v88.exe`. No mem 26. No `SLOT_MAX=35`. Did
not rerun H40 CMs / H41 / cappara/fccur/pron/suffix. Two 2 MiB. Waited
out other-agent cappara/fccur slots. `hp_c_v88s24` still in flight;
no 8 MiB leftover (both bytes-up).

| leftover | 2 MiB | Δ vs 439,288 | old 8 MB Δ | verdict |
|---|---:|---:|---:|---|
| baridx | 439,336 | **+48** | +266 | **kill** |
| temppos | 439,349 | **+61** | +266 | **kill** |

Wall: baridx 206 s, temppos 208 s. Archives `%LOCALAPPDATA%\hp_lab\s_{baridx,temppos}.hp`.
Next unscreened: user, defaultsort, coord, …

### H40 v88 leftovers after temppos (stopped)

Queue after baridx/temppos on **v88** flags, `SLOT_MAX=24`, `--mem 22`,
`data/enwik8.2mb`. Recompiled via `hp/tools/screen_rejects.ps1 -Name`.
Baseline `s_base.hp` **439,288**. Did not overwrite `hp_v83.exe`–`hp_v88.exe`.
No mem 26. No `SLOT_MAX=35`. No leftover 8 MiB (all bytes-up; parent
35-cap cappara next). Stopped: no further `hp_s_*` / `hp_c_*`.

| leftover | 2 MiB | Δ vs 439,288 | old 8 MB Δ | verdict |
|---|---:|---:|---:|---|
| user | 439,333 | **+45** | +267 | **kill** |
| defaultsort | 439,352 | **+64** | +271 | **kill** |
| coord | 439,351 | **+63** | +271 | **kill** |
| digitgap | 439,339 | **+51** | +274 | **kill** |
| sig | 439,347 | **+59** | +275 | **kill** |
| seckind | 439,295 | **+7** | +276 | **kill** |

Wall ~184–203 s. Archives `%LOCALAPPDATA%\hp_lab\s_{user,defaultsort,coord,digitgap,sig,seckind}.hp`.
Closest: seckind +7. **No 8 MiB gate.** Next unscreened: linelen, linktrail, dumpredir, …

### H42 leftover CMs on v88 (compiled)

Eight new default-off wiki-domain ContextModels. Not in `v78_flags.ps1`.
Did not overwrite `hp_v83.exe`–`hp_v88.exe`. No mem 26. No `SLOT_MAX=35`.
No 8 MiB encode. Integer-exact scanners. Not CITE / COORD / BLOCK /
TABLECLASS / ANCHOR / MAGIC / REFLIST / SFN / GEOTEMP / EPIGRAPH /
TRACKLIST / SUCCESSION / COLSTART / TOC / REFBEGIN / NOTES / LANGTPL /
FRAC / LISTEN / BIRTH / HLIST / MAINART / CHEM. Do not reopen H11–H41.
SENTPOS / skip40 / STATETRANS / REFGROUP stay ON.

Compiled `hp_s_{shortdesc,seealso,portal,authctl,usedate,ipa,goodart,caption}.exe`
from v88 flags + `SLOT_MAX=24` + one `-D`. 2 MiB screen vs `s_base.hp`
**439,288** skipped: two `hp_s_*` already running (`hp_s_coord`,
`hp_s_digitgap`).

| leftover | flag | salt | scanner | 2 MiB | Δ vs 439,288 |
|---|---|---:|---|---:|---:|
| shortdesc | `HP_SHORTDESC_MOD` | 234 | `{{short description` / `{{shortdesc` in-template | 439,352 | **+64** | **kill** |
| seealso | `HP_SEEALSO_MOD` | 235 | `{{see also` / `{{further` (not MAINART `{{Main}}`) | 439,350 | **+62** | **kill** |
| portal | `HP_PORTAL_MOD` | 236 | `{{portal` / `{{portal bar` | — | not run |
| authctl | `HP_AUTHCTL_MOD` | 237 | `{{authority control` | — | not run |
| usedate | `HP_USEDATE_MOD` | 238 | `{{use dmy` / `{{use mdy` / `{{use ymd` page-sticky | — | not run |
| ipa | `HP_IPA_MOD` | 239 | `{{IPA` / `{{IPAc-en` / `{{pron-en` templates (not PRONOUN) | — | not run |
| goodart | `HP_GOODART_MOD` | 240 | `{{good article` / `{{GA` / `{{featured article` page class | — | not run |
| caption | `HP_CAPTION_MOD` | 241 | sticky after `\|caption=` / `\|image_caption=` (not INFOBOX) | — | not run |

### v88cappara → v89 - **1,678,872 / 1.601 bpc, RT PASS −240 vs v88**

`HP_CAPPARA_MOD` on v88 flags, `SLOT_MAX=35`, mem 22. Waited for
existing `hp_g_v88cappara` pid 40008 (did not start a second encode).
Killed leftover `hp_s_*` / `hp_c_*` (30 GB cap). Archive
`%LOCALAPPDATA%\hp_lab\e8_8mb_v88cappara.hp` **1,678,872**
(−240 vs v88 1,679,112). Copied `hp/build/e8_8mb_v88cappara.hp` and
`hp/build/e8_8mb_v89.hp`. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v89.exe` from `hp_g_v88cappara.exe`. Did not overwrite
`hp_v88.exe`.

### v89 fx2 - **1,675,542 / 1.598 bpc, RT PASS −248 vs v88 fx2**

`hp_v89.exe` on `data/enwik8.8mb.fx2man`: **1,675,542**.
Decode SHA matches `data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
Copied `hp/build/e8_8mb_v88cappara_fx2.hp` and `hp/build/e8_8mb_v89_fx2.hp`.
New 8 MB champ is v89. Did not run mem 26. Did not overwrite `hp_v88.exe`.

v89 2 MiB baseline **439,253**. fccur re-screen **439,229 (−24)** → 8 MiB s24
**1,679,691 (−71 vs 1,679,762)**. Not >200; **no 35-cap**. pron re-screen
**439,194 (−59)** → 8 MiB s24 **1,679,566 (−196 vs 1,679,762)**. Not >200;
**no 35-cap**. suffix re-screen **439,224 (−29)** → 8 MiB s24 **1,679,583
(−179 vs 1,679,762)**. Not >200; **no 35-cap**. entity **439,298 (+45)**
kill.

### H42 v88 re-screen

v87 2 MiB bytes-down leftovers recompiled on **v88** flags (`HP_STATETRANS_MOD`
in `v78_flags.ps1`, SKIP=40, SENTPOS, REFGROUP, `HP_LR1_SCALE=40`). Cheap
protocol: `SLOT_MAX=24`, `--mem 22`, `data/enwik8.2mb`. Did not overwrite
`hp_v83.exe`–`hp_v88.exe`. No mem 26. Did not rerun H41. Did not rerun
statetrans (now in champ). Two 2 MiB max. Gate 8 MiB s24 if Δ < **−20**.
35-cap only if leftover s24 is **>200** under the v88 s24 champ **and** no
process RSS >20 GB.

**2 MiB baseline** `%LOCALAPPDATA%\hp_lab\s_base.hp` (18:11:32 UTC, copy
`s_base_v88.hp`): **439,288** (not v87 439,311). Matches the v87
statetrans leftover.

**8 MiB s24 champ** `hp_c_v88s24.exe` (v78 flags, `SLOT_MAX=24`, no extra)
→ `e8_8mb_v88s24.hp`: **1,680,018** (matches v87 statetrans s24), 761 s.

| leftover | 2 MiB | Δ vs 439,288 | v87 Δ | 8 MiB s24 | Δ vs 1,680,018 | vs v88-35 1,679,112 | call |
|---|---:|---:|---:|---:|---:|---:|---|
| cappara | 439,253 | **−35** | −39 | **1,679,762** | **−256** | +650 | s24 paid; 35-cap bytes-ok |
| fccur | 439,261 | **−27** | −43 | **1,679,691** | **−327** | +579 | s24 paid; 35-cap bytes-ok |
| pron | 439,247 | **−41** | −38 | **1,679,829** | **−189** | +717 | s24 paid; no 35 (<200) |
| suffix | 439,269 | **−19** | −28 | — | | | bytes-down; no 8 MiB (not < −20) |
| vowel | 439,268 | **−20** | −19 | — | | | bytes-down; no 8 MiB (not < −20) |
| boldst | 439,294 | **+6** | −10 | — | | | **kill** (flipped) |

fccur s24 is the strongest leftover (−327). cappara s24 −256. Both >200
under 1,680,018. **Did not start `SLOT_MAX=35`**: leftover 35-cap RSS is
~27 GB (v87 abort; sibling `hp_g_v88cappara` later ran at **27.16 GB** and
landed **v89** 1,678,872 / fx2 1,675,542 both RT PASS). No process RSS
>20 GB on this agent's leftover jobs. No fccur 35-cap (champ already
moved to v89 cappara; RSS would exceed 20 GB).

Wall: 2 MiB 184–209 s; s24 champ 761 s; cappara s24 809 s; fccur s24
801 s (TEMP then copied). Archives
`%LOCALAPPDATA%\hp_lab\e8_8mb_v88s24_{cappara,fccur,pron}.hp` and
`hp/build`. Champ binaries untouched (`hp_v83.exe` 265,457 /
`hp_v84.exe` 266,575 / `hp_v85.exe` 266,993 / `hp_v86.exe` 267,599 /
`hp_v87.exe` 267,599 / `hp_v88.exe` 268,111).

### H42 leftover CMs on v89 (closed)

Waited out `hp_g_v88cappara` 35-cap and parent identity/fx2 RT. Champ is
**v89** (`hp_v89.exe` exists; `HP_CAPPARA_MOD` in `v78_flags.ps1`).
Saved `s_base.hp` as `s_base_v88.hp` (439,288). Rewrote 2 MiB
`SLOT_MAX=24` baseline via `hp_c_v88s24_cappara.exe` (v89 flags;
new `hp_s_base.exe` from `screen_rejects.ps1` crashed under `&` while
fx2 held 27 GB). New Length **439,253**. Did not overwrite
`hp_v83.exe`–`hp_v89.exe`. No mem 26. No leftover `SLOT_MAX=35`.
Did not rerun shortdesc/seealso (v88 kill +64/+62). Did not run fccur
8 MiB (champ no longer v88). Two 2 MiB max. Gate 8 MiB s24 if Δ < **−20**
vs 439,253; 35-cap only if leftover s24 is **>200** under v89 s24
`e8_8mb_v88s24_cappara.hp` **1,679,762**. `hp_v89.exe` protected in
`screen_rejects.ps1`.

| leftover | 2 MiB | Δ vs 439,253 | old 8 MB Δ | verdict |
|---|---:|---:|---:|---|
| shortdesc | 439,352 | **+64** | | **kill** (v88; not rerun) |
| seealso | 439,350 | **+62** | | **kill** (v88; not rerun) |
| portal | 439,314 | **+61** | | **kill** |
| authctl | 439,316 | **+63** | | **kill** |
| usedate | 439,314 | **+61** | | **kill** |
| ipa | 439,313 | **+60** | | **kill** |
| goodart | 439,322 | **+69** | | **kill** |
| caption | 439,312 | **+59** | | **kill** |

H42 closed. No 8 MiB. Wall ~200 s. Archives
`%LOCALAPPDATA%\hp_lab\s_{portal,authctl,usedate,ipa,goodart,caption}.hp`.

### Historical leftovers after seckind on v89

Queue after seckind on **v89** flags (`HP_CAPPARA_MOD` + STATETRANS +
SKIP=40 + SENTPOS), `SLOT_MAX=24`, `--mem 22`, `data/enwik8.2mb`.
Recompiled via `screen_rejects.ps1 -Name`. Baseline **439,253**.
Skipped session-decided: user/defaultsort/coord/digitgap/sig/seckind/
baridx/temppos/pron/cappara/fccur/suffix/vowel/boldst / H40 / H41 /
H42 shortdesc/seealso. Two 2 MiB. No leftover 8 MiB (no Δ < −20).
No 35-cap. No mem 26. Did not overwrite `hp_v83.exe`–`hp_v89.exe`.

| leftover | 2 MiB | Δ vs 439,253 | old 8 MB Δ | verdict |
|---|---:|---:|---:|---|
| linelen | 439,256 | **+3** | +277 | **kill** |
| linktrail | 439,313 | **+60** | +280 | **kill** |
| dumpredir | 439,314 | **+61** | +281 | **kill** |
| ns | 439,309 | **+56** | +282 | **kill** |
| include | 439,307 | **+54** | +283 | **kill** |
| brace3 | 439,313 | **+60** | +285 | **kill** |
| parserfn | 439,308 | **+55** | +286 | **kill** |
| urlpart | 439,247 | **−6** | +286 | bytes-down; no 8 MiB (not < −20) |
| redirtarget | 439,311 | **+58** | +290 | **kill** |
| nowiki | 439,320 | **+67** | +292 | **kill** |
| gallery | 439,314 | **+61** | +292 | **kill** |
| wikimodel | 439,311 | **+58** | +292 | **kill** |
| htmlfmt | 439,314 | **+61** | +297 | **kill** |
| parast | 439,266 | **+13** | +299 | **kill** |
| refname | 439,307 | **+54** | +300 | **kill** |
| fword | 439,296 | **+43** | +304 | **kill** |
| entity | 439,298 | **+45** | +305 | **kill** |
| hatnote | 439,320 | **+67** | +309 | **kill** |
| pageid | 439,265 | **+12** | +311 | **kill** |
| digitpos | 439,313 | **+60** | +322 | **kill** |
| diglen | 439,242 | **−11** | +324 | bytes-down; no 8 MiB (not < −20) |
| prevline | 439,281 | **+28** | +326 | **kill** |
| headclose | 439,312 | **+59** | +330 | **kill** |
| lowergap | 439,283 | **+30** | +333 | **kill** |
| blank | 439,317 | **+64** | +335 | **kill** |
| decimal | 439,318 | **+65** | +340 | **kill** |
| minor | 439,273 | **+20** | +353 | **kill** |
| qocxt | 439,310 | **+57** | +363 | **kill** |
| seclevel | 439,274 | **+21** | +379 | **kill** |
| ip | 439,284 | **+31** | +390 | **kill** |
| slashgap | 439,271 | **+18** | +390 | **kill** |
| pubid | 439,283 | **+30** | +391 | **kill** |
| lastlink | 439,422 | **+169** | +392 | **kill** |
| sprun | 439,320 | **+67** | +414 | **kill** |
| hashp5 | 439,342 | **+89** | +423 | **kill** |
| runlen | 439,319 | **+66** | +432 | **kill** |
| catsort | 439,319 | **+66** | +434 | **kill** |
| prevsent | 439,310 | **+57** | +442 | **kill** |
| ordinal | 439,271 | **+18** | +452 | **kill** |
| linkcomma | 439,329 | **+76** | +456 | **kill** |
| hexrun | 439,276 | **+23** | +457 | **kill** |
| anchor | 439,330 | **+77** | +461 | **kill** |
| protocol | 439,287 | **+34** | +462 | **kill** |
| wpos | 439,242 | **−11** | +466 | bytes-down; no 8 MiB |
| catblock | 439,321 | **+68** | +467 | **kill** |
| listmix | 439,297 | **+44** | +467 | **kill** |
| namedarg | 439,327 | **+74** | +470 | **kill** |
| text | 439,291 | **+38** | +473 | **kill** |
| unit | 439,272 | **+19** | +476 | **kill** |
| lang | 439,326 | **+73** | +478 | **kill** |
| pxsize | 439,256 | **+3** | +478 | **kill** |
| tblrow | 439,323 | **+70** | +478 | **kill** |
| tpllen | 439,255 | **+2** | +479 | **kill** |
| magic | 439,311 | **+58** | +480 | **kill** |
| tblcol | 439,321 | **+68** | +480 | **kill** |
| colring | 439,311 | **+58** | +486 | **kill** |
| listlevel | 439,291 | **+38** | +488 | **kill** |
| charcls | 439,259 | **+6** | +501 | **kill** |
| tbldepth | 439,324 | **+71** | +503 | **kill** |
| fileopt | 439,313 | **+60** | +508 | **kill** |
| paralen | 439,292 | **+39** | +511 | **kill** |
| paren | 439,296 | **+43** | +517 | **kill** |
| tableclass | 439,314 | **+61** | +517 | **kill** |
| extlink | 439,307 | **+54** | +521 | **kill** |
| lead | 439,282 | **+29** | +524 | **kill** |
| linklen | 439,286 | **+33** | +525 | **kill** |
| style | 439,309 | **+56** | +532 | **kill** |
| cellkind | 439,320 | **+67** | +534 | **kill** |
| infobox | 439,332 | **+79** | +535 | **kill** |
| contr | 439,299 | **+46** | +542 | **kill** |
| fontcol | 439,311 | **+58** | +554 | **kill** |
| httphost | 439,263 | **+10** | +554 | **kill** |
| hyphen | 439,316 | **+63** | +555 | **kill** |
| dab | 439,303 | **+50** | +557 | **kill** |
| init | 439,311 | **+58** | +579 | **kill** |
| extdisp | 439,324 | **+71** | +581 | **kill** |
| listpos | 439,294 | **+41** | +589 | **kill** |
| listpara | 439,319 | **+66** | +593 | **kill** |
| linkns | 439,325 | **+72** | +597 | **kill** |
| infoval | 439,315 | **+62** | +601 | **kill** |
| prespace | 439,319 | **+66** | +612 | **kill** |
| indent | 439,339 | **+86** | +613 | **kill** |
| piperole | 439,303 | **+50** | +615 | **kill** |
| qperiod | 439,340 | **+87** | +616 | **kill** |
| tagname | 439,275 | **+22** | +625 | **kill** |
| tokencls | 439,301 | **+48** | +627 | **kill** |
| headword | 439,326 | **+73** | +633 | **kill** |
| tagdist | 439,289 | **+36** | +634 | **kill** |
| celltxt | 439,306 | **+53** | +639 | **kill** |
| abbrev | 439,308 | **+55** | +641 | **kill** |
| titleword | 439,335 | **+82** | +645 | **kill** |
| splen | 439,301 | **+48** | +651 | **kill** |
| shape | 439,296 | **+43** | +655 | **kill** |
| alnumlen | 439,294 | **+41** | +672 | **kill** |
| citekind | 439,327 | **+74** | +676 | **kill** |
| caseflip | 439,315 | **+62** | +682 | **kill** |
| repeat | 439,324 | **+71** | +684 | **kill** |
| colspan | 439,313 | **+60** | +688 | **kill** |
| prefix | 439,506 | **+253** | +711 | **kill** |

Historical leftover re-screen on v89 `SLOT_MAX=24` **closed through prefix**. Queue empty.
Closest: tpllen +2 / linelen +3 / pxsize +3 / charcls +6. This tail closest: tagname +22. Best bytes-down: diglen −11 / wpos −11 / urlpart −6 (no gate).
No leftover 2 MiB Δ < **−20**; no 8 MiB s24 gate; **no 35-cap**. No mem 26.
pron s24 **1,679,566 (−196, no 35)**. suffix s24 **1,679,583 (−179, no 35)**. fccur s24 **1,679,691 (−71, no 35)**.
Wall ~188–215 s. Did not overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 / 267,599 / 267,599 / 268,111 / 268,111).
**Champ stays v89 1,678,872** / fx2 1,675,542.

Entity-start pass (entity → prefix) recompiled via `screen_rejects.ps1 -Name`.
`catsort` re-encoded **439,319 (+66)** (matches table; not a runlen copy).
No Δ < **−20** on this pass so no leftover 8 MiB; no 35-cap; no mem 26.
Failed extra restarts left lab `s_{prevsent,catblock,listmix,lang,pxsize,tblrow,tpllen,magic,tblcol,colring,listlevel}.hp`
missing or short; table bytes are from the completed encodes. `s_catsort.hp`
restored.

### H43 leftover CMs on v89 (closed)

Eight new default-off wiki-domain / dense-layout ContextModels. Not in
`v78_flags.ps1`. Did not overwrite `hp_v83.exe`–`hp_v89.exe`
(265,457 / 266,575 / 266,993 / 267,599 / 267,599 / 268,111 / 268,111).
No mem 26. No leftover `SLOT_MAX=35`. No leftover 8 MiB (no Δ < **−20**).
Integer-exact scanners already in the headers. Not twins of CITE /
COORD / BLOCK / TABLECLASS / ANCHOR / MAGIC / REFLIST / SFN / GEOTEMP /
EPIGRAPH / TRACKLIST / SUCCESSION / COLSTART / TOC / REFBEGIN / NOTES /
LANGTPL / FRAC / LISTEN / BIRTH / HLIST / MAINART / CHEM / SHORTDESC /
SEEALSO / PORTAL / AUTHCTL / USEDATE / IPA / GOODART / CAPTION. Do not
reopen H11–H42. SENTPOS / skip40 / STATETRANS / REFGROUP / CAPPARA stay
ON.

Compiled `hp_s_{navbox,efoot,rshort,asof,clarify,currency,displaytitle,nowrap}.exe`
from v89 flags + `SLOT_MAX=24` + one `-D` via `screen_rejects.ps1 -Name`.
2 MiB screen vs `s_base.hp` **439,253**. Waited out in-flight navbox
(did not duplicate). Two 2 MiB max vs historical leftover queue.
`hp_c_v89s24_fccur` / later suffix s24 not counted toward the cap. fccur
s24 had finished **1,679,691 (−71)**; suffix s24 was still running, so
even a gate would have stayed with that watcher. No gate fired.

| leftover | flag | salt | scanner | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---|---:|---:|---|
| navbox | `HP_NAVBOX_MOD` | 242 | `{{navbox` / `{{sidebar` in-template (not INFOBOX) | 439,312 | **+59** | **kill** |
| efoot | `HP_EFOOT_MOD` | 243 | `{{efn` / `{{notelist` / `{{notefoot` (not NOTES `{{notes}}`) | 439,314 | **+61** | **kill** |
| rshort | `HP_RSHORT_MOD` | 244 | `{{r\|` / `{{r}}` exact (not SFN, not CITE) | 439,309 | **+56** | **kill** |
| asof | `HP_ASOF_MOD` | 245 | `{{as of` / `{{As of` in-template | 439,315 | **+62** | **kill** |
| clarify | `HP_CLARIFY_MOD` | 246 | `{{clarify` / `{{who` / `{{which` / `{{when` (not CN) | 439,313 | **+60** | **kill** |
| currency | `HP_CURRENCY_MOD` | 247 | `{{USD` / `{{GBP` / `{{EUR` / `{{currency` | 439,309 | **+56** | **kill** |
| displaytitle | `HP_DISPLAYTITLE_MOD` | 248 | `{{DISPLAYTITLE` / `{{italic title` / `{{lowercase` page class | 439,305 | **+52** | **kill** |
| nowrap | `HP_NOWRAP_MOD` | 249 | `{{nowrap` / `{{nobold` / `{{noitalic` in-template | 439,315 | **+62** | **kill** |

H43 closed. All eight bytes-up. Closest: displaytitle +52. **No 8 MiB
gate.** Wall 192–203 s (navbox 198 s). Archives
`%LOCALAPPDATA%\hp_lab\s_{navbox,efoot,rshort,asof,clarify,currency,displaytitle,nowrap}.hp`.

### v89 fccur 8 MiB s24 (no 35-cap)

`hp_c_v89s24_fccur` → `%LOCALAPPDATA%\hp_lab\e8_8mb_v89s24_fccur.hp`
(copied `hp/build`): **1,679,691**. Δ vs v89 s24 champ **1,679,762**
(`e8_8mb_v89s24.hp` / `e8_8mb_v88s24_cappara.hp`): **−71**. Not >200.
**No `SLOT_MAX=35`.** Did not encode further. Did not kill `hp_s_*`.
Did not overwrite `hp_v83.exe`–`hp_v89.exe`. No mem 26. Champ stays
v89 1,678,872 / fx2 1,675,542. Same 1,679,691 as v88 fccur s24.

### v89 pron 8 MiB s24 (no 35-cap)

`hp_c_v89s24_pron` pid 6552 → `%LOCALAPPDATA%\hp_lab\e8_8mb_v89s24_pron.hp`
(copied `hp/build`): **1,679,566**. Δ vs v89 s24 champ **1,679,762**:
**−196**. Not >200. **No `SLOT_MAX=35`.** Did not compile
`hp_g_v89pron.exe`. Did not start a second 8 MiB. Did not kill `hp_s_*`.
Did not overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 /
267,599 / 267,599 / 268,111 / 268,111). No mem 26. Champ stays v89
1,678,872 / fx2 1,675,542. 2 MiB was **439,194 (−59 vs 439,253)**. Wall
790 s (06:48:53–07:02:03). Still **+694** vs identity champ. Stronger
than fccur s24 (−71) but under the 200 B 35-cap gate.

### v89 leftover re-screen (fccur / pron / suffix / vowel)

v88 2 MiB bytes-down leftovers recompiled on **v89** flags
(`HP_CAPPARA_MOD` already ON in `v78_flags.ps1`; SKIP=40, SENTPOS,
STATETRANS, REFGROUP, `HP_LR1_SCALE=40`). Cheap protocol: `SLOT_MAX=24`,
`--mem 22`, `data/enwik8.2mb` via `screen_rejects.ps1 -Name`. Did **not**
add cappara (in champ). Did not overwrite `hp_v83.exe`–`hp_v89.exe`
(265,457 / 266,575 / 266,993 / 267,599 / 267,599 / 268,111 / 268,111).
No mem 26. Two 2 MiB max (shared with historical `hp_s_*`). Gate 8 MiB
s24 if Δ < **−20**. 35-cap ALONE only if leftover s24 is **>200** under
v89 s24 champ `e8_8mb_v88s24_cappara.hp` **1,679,762** and no RSS >20 GB.
One 8 MiB at a time.

**2 MiB baseline** `s_base.hp` / `s_base_v89.hp`: **439,253**.

| leftover | 2 MiB | Δ vs 439,253 | v88 Δ | 8 MiB s24 | Δ vs 1,679,762 | vs v89-35 1,678,872 | call |
|---|---:|---:|---:|---:|---:|---:|---|
| fccur | 439,229 | **−24** | −27 | **1,679,691** | **−71** | +819 | s24 paid; no 35 (<200) |
| pron | 439,194 | **−59** | −41 | **1,679,566** | **−196** | +694 | s24 paid; no 35 (<200) |
| suffix | 439,224 | **−29** | −19 | **1,679,583** | **−179** | +711 | s24 paid; no 35 (<200) |
| vowel | 439,237 | **−16** | −20 | — | | | bytes-down; no 8 MiB (not < −20) |

### v89 suffix 8 MiB s24 (no 35-cap)

`hp_c_v89s24_suffix` pid 31460 → `%LOCALAPPDATA%\hp_lab\e8_8mb_v89s24_suffix.hp`
(copied `hp/build`): **1,679,583**. Δ vs v89 s24 champ **1,679,762**:
**−179**. Not >200. **No `SLOT_MAX=35`.** Did not compile
`hp_g_v89suffix.exe`. Did not start a second 8 MiB. Did not kill `hp_s_*`.
Did not overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 /
267,599 / 267,599 / 268,111 / 268,111). No mem 26. Champ stays v89
1,678,872 / fx2 1,675,542. 2 MiB was **439,224 (−29 vs 439,253)**. Wall
790 s (07:04:34–07:17:44). Still **+711** vs identity champ. Between
pron s24 (−196) and fccur s24 (−71); under the 200 B 35-cap gate.

pron s24 is the strongest leftover (−196). suffix −179. fccur −71.
None clear the 200 B 35-cap gate. vowel 2 MiB −16 does not gate 8 MiB.
Champ stays v89.

### v89 combo R0=2+R5=1 (2 MiB)

Per-mixer `{2,1,1,2,1,1}` on v89 flags (`SLOT_MAX=24`). Integer-exact
`HP_LR1_R0=2` / `HP_LR1_R5=1` already in headers (applied after
`HP_LR1_SCALE=40`). Compiled `hp_s_r0v2r5v1.exe` (268,111) from
`v78_flags.ps1` with `SLOT_MAX` forced to 24. Encoded `data/enwik8.2mb`
`--mem 22` → `%LOCALAPPDATA%\hp_lab\s_r0v2r5v1.hp`. 2 MiB
**439,146 (−107 vs 439,253)**. Wall 205 s. Old v85 combo was 439,341
(−155 vs 439,496). 8 MiB s24 **1,679,749 (−13 vs 1,679,762)**; not >200;
**no 35-cap**. Did not overwrite `hp_v83.exe`–`hp_v89.exe`. No mem 26.
Overrides not added to `v78_flags.ps1`.

### H44 leftover CMs on v89 (closed)

Eight new default-off wiki-domain / dense-layout ContextModels on **v89**.
Not in `v78_flags.ps1`. Did not overwrite `hp_v83.exe`–`hp_v89.exe`.
No mem 26. No leftover `SLOT_MAX=35`. No leftover 8 MiB unless a 2 MiB
Δ < **−20**. Integer-exact scanners reuse H43 `h43t_*` (dash first-byte
for `{{-}}`). Not twins of MAINART / HATNOTE / SHORTDESC / INFOBOX /
GEOTEMP / COORD / SISTER / BLOCK / BR / COLSTART / WIKIHR / CITE /
EXTLINK / HTTPHOST / SFN / RSHORT / EFOOT / NOTES / REFBEGIN. Do not
reopen H11–H43. SENTPOS / skip40 / STATETRANS / REFGROUP / CAPPARA stay
ON.

Salts **250–257**. Hash `h2(salt, val + ((hist_ & 0xffffffull) << 8))`.
Compiled `hp_s_{stub,persondata,flag,quotebox,clear,imdb,rp,fn}.exe`
from v89 flags + `SLOT_MAX=24` + one `-D` via `screen_rejects.ps1 -Name`.
2 MiB screen vs `s_base.hp` **439,253**. Two 2 MiB max. Skip archive
already ≥ 430000. Gate 8 MiB s24 only if Δ < **−20**. 35-cap ALONE only
if leftover s24 is >200 under **1,679,762** (fccur −71 / pron −196 /
suffix −179 already failed).

| leftover | flag | salt | scanner | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---|---:|---:|---|
| stub | `HP_STUB_MOD` | 250 | exact `stub` or name ends `stub` (not MAINART/HATNOTE/SHORTDESC) | 439,317 | **+64** | **kill** |
| persondata | `HP_PERSONDATA_MOD` | 251 | `{{persondata` (not INFOBOX) | 439,307 | **+54** | **kill** |
| flag | `HP_FLAG_MOD` | 252 | `{{flag` / `{{flagicon` / `{{flagu` / `{{flagcountry` (not GEOTEMP/COORD/SISTER) | 439,309 | **+56** | **kill** |
| quotebox | `HP_QUOTEBOX_MOD` | 253 | `{{quote` / `{{cquote` / `{{quotation` / `{{quotebox` (not BLOCK `<blockquote>`) | 439,318 | **+65** | **kill** |
| clear | `HP_CLEAR_MOD` | 254 | `{{clear}}` / `{{clr}}` / `{{-}}` (not BR/COLSTART/WIKIHR) | 439,311 | **+58** | **kill** |
| imdb | `HP_IMDB_MOD` | 255 | `{{imdb` / `{{IMDb` folded (not CITE/EXTLINK/HTTPHOST) | 439,314 | **+61** | **kill** |
| rp | `HP_RP_MOD` | 256 | `{{rp\|` / `{{rp}}` (not SFN/RSHORT/CITE) | 439,313 | **+60** | **kill** |
| fn | `HP_FN_MOD` | 257 | `{{fn}}` / `{{fnb}}` / `{{reflabel}}` / `{{notelabel}}` (not EFOOT/NOTES/REFBEGIN) | 439,312 | **+59** | **kill** |

H44 closed. All eight bytes-up. Closest: persondata +54. **No 8 MiB
gate.** Did not start 8 MiB (combo s24 was running; no Δ < **−20**).
No 35-cap. Did not overwrite `hp_v83.exe`–`hp_v89.exe`. Archives
`%LOCALAPPDATA%\hp_lab\s_{stub,persondata,flag,quotebox,clear,imdb,rp,fn}.hp`.
Wall ~206–216 s.

### PLAN item 5 null-expert dilution tax (v89 2 MiB)

`HP_NULL_EXPERT` default 0. When 1, leftover-style ContextModel
`nullexpert_` copies always-on o1 `set_context(h2(1, hist_ & 0xffull))`
into a **separate** StateMap — second mixer input, zero new axis.
Not added to `v78_flags.ps1`. Compiled v89 flags with `SLOT_MAX`
forced to 24 + `-DHP_NULL_EXPERT=1` → `hp/build/hp_s_null.exe`
(268,017). Waited out `hp_s_prevsent` + `hp_s_r0v2r5v1`. Encoded
`data/enwik8.2mb` `--mem 22` → `%LOCALAPPDATA%\hp_lab\s_null.hp`.
Did not start 8 MiB. Did not overwrite `hp_v83.exe`–`hp_v89.exe`
(265,457 / 266,575 / 266,993 / 267,599 / 267,599 / 268,111 / 268,111).
No mem 26. No `SLOT_MAX=35`. Champ stays v89 1,678,872 / fx2 1,675,542.

| | |
|---|---:|
| bytes | **439,319** |
| Δ vs 439,253 | **+66** |
| wall | 236 s |

Tax **+66 B** is the cost of one leftover-sized mixer slot with no
new information. Reject-pile re-score as (measured delta − 66) is a
**paper adjustment**, not a reopen of H11–H43. Flag stays default-off.
This is a diagnostic, not a leftover CM accept.

### v89 combo R0=2+R5=1 8 MiB s24 (no 35-cap)

Re-score of v85 `{2,1,1,2,1,1}` on v89 cheap protocol. 2 MiB gated
(439,146, **−107 vs 439,253**). `hp_c_v89s24_r0v2r5v1` pid 37796 →
`%LOCALAPPDATA%\hp_lab\e8_8mb_v89s24_r0v2r5v1.hp` (copied `hp/build`):
**1,679,749**. Δ vs v89 s24 champ **1,679,762**: **−13**. Not >200.
**No `SLOT_MAX=35`.** Did not compile `hp_g_v89r0v2r5v1.exe`. Did not
start a second 8 MiB (waited out in-flight pid 37796). Did not kill
`hp_s_*`. Did not overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 /
266,575 / 266,993 / 267,599 / 267,599 / 268,111 / 268,111). No mem 26.
Did not add `-DHP_LR1_R0=2` / `-DHP_LR1_R5=1` to `v78_flags.ps1`. Did
not copy to `hp_v90.exe`. Champ stays v89 1,678,872 / fx2 1,675,542.
Wall 815 s (09:29:04–09:42:39). Still **+877** vs identity champ.

| rung | bytes | Δ vs v89 | old v85 | old v85 Δ |
|---|---:|---:|---:|---:|
| 2 MiB s24 | **439,146** | **−107** vs 439,253 | 439,341 | −155 vs 439,496 |
| 8 MiB s24 | **1,679,749** | **−13** vs 1,679,762 | 1,680,890 | −204 vs 1,681,094 |

Transfer collapsed: 2 MiB −107 did not become an 8 MiB −200. Weaker
than pron s24 (−196) / suffix (−179) / fccur (−71). Under the 200 B
35-cap gate. Overrides stay default-0.

### H45 leftover CMs on v89 (closed)

Eight new default-off leftover ContextModels on **v89**. Not twins of
HTMLFMT / FONTCOL / SMALL / MATH / PRESPACE / NOWIKI / INFOBOX /
PERSONDATA / LANGTPL / LANG / EXTLINK / CN / IMDB / SISTER / COLSPAN /
TBLROW. Not in `v78_flags.ps1`. Did not overwrite `hp_v83.exe`–`hp_v89.exe`.
No mem 26. No leftover `SLOT_MAX=35`. No leftover 8 MiB unless a 2 MiB
Δ < **−20**. Integer-exact scanners: HTML regions (small / supsub /
precode) are MATH/CHEM-style windows; taxobox / nihongo / deadlink /
wayback reuse H43 `h43t_*` (spaces skipped so `dead link` → `deadlink`);
rowspan is sticky `|rowspan=` / `!rowspan=` (not COLSPAN value). Do not
reopen H11–H44. SENTPOS / skip40 / STATETRANS / REFGROUP / CAPPARA stay
ON.

Salts **258–265**. Hash `h2(salt, val + ((hist_ & 0xffffffull) << 8))`.
Compiled `hp_s_{small,supsub,precode,taxobox,nihongo,deadlink,wayback,rowspan}.exe`
from v89 flags + `SLOT_MAX=24` + one `-D` via `screen_rejects.ps1 -Name`.
2 MiB screen vs `s_base.hp` **439,253**. Two 2 MiB max. Skip archive
already ≥ 430000. Gate 8 MiB s24 only if Δ < **−20**. 35-cap ALONE only
if leftover s24 is >200 under **1,679,762** (combo R0=2+R5=1 s24 already
**1,679,749 (−13)** — no 35; fccur −71 / pron −196 / suffix −179 also
failed).

| leftover | flag | salt | scanner | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---|---:|---:|---|
| small | `HP_SMALL_MOD` | 258 | `<small>` region (not HTMLFMT bitmask, not FONTCOL) | 439,316 | **+63** | **kill** |
| supsub | `HP_SUPSUB_MOD` | 259 | `<sup>` / `<sub>` kind (not SMALL, not MATH) | 439,311 | **+58** | **kill** |
| precode | `HP_PRECODE_MOD` | 260 | `<pre>` / `<code>` / `<tt>` region (not PRESPACE, not NOWIKI) | 439,317 | **+64** | **kill** |
| taxobox | `HP_TAXOBOX_MOD` | 261 | `{{taxobox` (not INFOBOX, not PERSONDATA) | 439,327 | **+74** | **kill** |
| nihongo | `HP_NIHONGO_MOD` | 262 | `{{nihongo` / `{{korean` / `{{chinese` (not LANGTPL, not LANG) | 439,310 | **+57** | **kill** |
| deadlink | `HP_DEADLINK_MOD` | 263 | `{{dead link}}` / `{{deadlink}}` / `{{broken link}}` (not EXTLINK, not CN) | 439,311 | **+58** | **kill** |
| wayback | `HP_WAYBACK_MOD` | 264 | `{{wayback` / `{{webarchive` / `{{dmoz` (not EXTLINK, not IMDB, not SISTER) | 439,315 | **+62** | **kill** |
| rowspan | `HP_ROWSPAN_MOD` | 265 | sticky `\|rowspan=` / `!rowspan=` (not COLSPAN, not TBLROW) | 439,315 | **+62** | **kill** |

H45 closed. All eight bytes-up. Closest: nihongo +57. **No 8 MiB
gate.** Did not start 8 MiB (no Δ < **−20**). No 35-cap. Did not
overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 /
267,599 / 267,599 / 268,111 / 268,111). Archives
`%LOCALAPPDATA%\hp_lab\s_{small,supsub,precode,taxobox,nihongo,deadlink,wayback,rowspan}.hp`.
Wall ~196–277 s.

### H45 leftover CMs on v89 (closed)

Eight new default-off wiki-domain / dense-layout ContextModels. Not in
`v78_flags.ps1`. Did not overwrite `hp_v83.exe`–`hp_v89.exe`. No mem 26.
No leftover `SLOT_MAX=35`. No leftover 8 MiB (no Δ < **−20**). Integer-exact
scanners. Not twins of HTMLFMT / FONTCOL / MATH / PRESPACE / NOWIKI /
INFOBOX / PERSONDATA / LANGTPL / LANG / EXTLINK / CN / IMDB / SISTER /
COLSPAN / TBLROW. Do not reopen H11–H44. SENTPOS / skip40 / STATETRANS /
REFGROUP / CAPPARA stay ON.

2 MiB vs `s_base.hp` **439,253**. Two 2 MiB max.

| leftover | flag | salt | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---:|---:|---|
| small | `HP_SMALL_MOD` | 258 | 439,316 | **+63** | **kill** |
| supsub | `HP_SUPSUB_MOD` | 259 | 439,311 | **+58** | **kill** |
| precode | `HP_PRECODE_MOD` | 260 | 439,317 | **+64** | **kill** |
| taxobox | `HP_TAXOBOX_MOD` | 261 | 439,327 | **+74** | **kill** |
| nihongo | `HP_NIHONGO_MOD` | 262 | 439,310 | **+57** | **kill** |
| deadlink | `HP_DEADLINK_MOD` | 263 | 439,311 | **+58** | **kill** |
| wayback | `HP_WAYBACK_MOD` | 264 | 439,315 | **+62** | **kill** |
| rowspan | `HP_ROWSPAN_MOD` | 265 | 439,315 | **+62** | **kill** |

H45 closed. All eight bytes-up. Closest: nihongo +57 (null-expert tax is +66).
**No 8 MiB gate.** Champ stays v89 1,678,872 / fx2 1,675,542.

### H46 leftover CMs on v89 (closed)

Eight new default-off leftover ContextModels on **v89**. Not twins of CN /
DEADLINK / HATNOTE / UNREF / CLEANUP / REDIR / DUMPREDIR / CITEKIND / PUBID /
DOI / SUCCESSION / GEOTEMP. Not in `v78_flags.ps1`. Did not overwrite
`hp_v83.exe`–`hp_v89.exe`. No mem 26. No leftover `SLOT_MAX=35`. No leftover
8 MiB unless a 2 MiB Δ < **−20**. Integer-exact scanners: unref / cleanup /
npov / rfrom / medal reuse H43 `h43t_*` (spaces skipped so `R from` → `rfrom`,
`soft redirect` → `softredirect`); doi / pmid add sticky `|doi=` / `|pmid=` /
`|pmc=` plus template kinds; isbnmod is `ISBN` token / `|isbn=` (not PUBID
digit hash). Do not reopen H11–H45. SENTPOS / skip40 / STATETRANS / REFGROUP /
CAPPARA stay ON.

Salts **266–273**. Hash `h2(salt, val + ((hist_ & 0xffffffull) << 8))`.
Compiled `hp_s_{unref,cleanup,npov,rfrom,doi,pmid,isbnmod,medal}.exe`
from v89 flags + `SLOT_MAX=24` + one `-D` via `screen_rejects.ps1 -Name`.
2 MiB screen vs `s_base.hp` **439,253**. Two 2 MiB max. Skip archive
already ≥ 430000. Gate 8 MiB s24 only if Δ < **−20**. 35-cap ALONE only
if leftover s24 is >200 under **1,679,762** (combo R0=2+R5=1 s24 already
**1,679,749 (−13)** — no 35; fccur −71 / pron −196 / suffix −179 also
failed).

| leftover | flag | salt | scanner | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---|---:|---:|---|
| unref | `HP_UNREF_MOD` | 266 | `{{unreferenced` / `{{unref}}` / `{{refimprove` (not CN, not DEADLINK) | 439,319 | **+66** | **kill** |
| cleanup | `HP_CLEANUP_MOD` | 267 | `{{cleanup` / `{{wikify` / `{{orphan` (not UNREF, not HATNOTE) | 439,308 | **+55** | **kill** |
| npov | `HP_NPOV_MOD` | 268 | `{{npov` / `{{pov` / `{{coi}}` / `{{advert` (not CLEANUP) | 439,312 | **+59** | **kill** |
| rfrom | `HP_RFROM_MOD` | 269 | `{{R from` / `{{R to` / `{{soft redirect` (not REDIR, not DUMPREDIR) | 439,309 | **+56** | **kill** |
| doi | `HP_DOI_MOD` | 270 | sticky `\|doi=` / `{{doi}}` / `{{cite doi` (not CITEKIND, not PUBID) | 439,309 | **+56** | **kill** |
| pmid | `HP_PMID_MOD` | 271 | sticky `\|pmid=` / `\|pmc=` / `{{pmid}}` / `{{pmc}}` (not DOI, not PUBID) | 439,313 | **+60** | **kill** |
| isbnmod | `HP_ISBN_MOD` | 272 | `ISBN` token / `\|isbn=` (not PUBID numeric id, not DOI) | 439,325 | **+72** | **kill** |
| medal | `HP_MEDAL_MOD` | 273 | `{{medal` / `{{Medal` / `{{gold` / `{{silver` / `{{bronze` sports (not SUCCESSION, not GEOTEMP) | 439,311 | **+58** | **kill** |

H46 closed. All eight bytes-up. Closest: cleanup +55 (null-expert tax is +66).
**No 8 MiB gate.** Did not start 8 MiB (no Δ < **−20**). No 35-cap. Did not
overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 /
267,599 / 267,599 / 268,111 / 268,111). Archives
`%LOCALAPPDATA%\hp_lab\s_{unref,cleanup,npov,rfrom,doi,pmid,isbnmod,medal}.hp`.
Wall ~194–216 s. Champ stays v89 1,678,872 / fx2 1,675,542.

### H47 leftover CMs on v89 (closed)

Eight new default-off leftover ContextModels on **v89**. Not twins of FILEOPT /
EXTDISP / PXSIZE / GALLERY / SEEALSO / MAINART / HATNOTE / BIRTH / MONTH /
SFN / CITE / CITEKIND / ISBN / PUBID / DOI / STYLE / COLSPAN / CELLKIND /
ROWSPAN / PRECODE / NOWIKI / MATH. Not in `v78_flags.ps1`. Did not overwrite
`hp_v83.exe`–`hp_v89.exe`. No mem 26. No leftover `SLOT_MAX=35`. No leftover
8 MiB unless a 2 MiB Δ < **−20**. Integer-exact scanners: further / death /
harv reuse H43 `h43t_*` (spaces skipped so `death date` → `deathdate`);
thumb / issn / oclc / alignmod are rolling windows; syntax is an HTML-tag
region. Do not reopen H11–H46. SENTPOS / skip40 / STATETRANS / REFGROUP /
CAPPARA stay ON.

Salts **274–281**. Hash `h2(salt, val + ((hist_ & 0xffffffull) << 8))`.
Compiled `hp_s_{thumb,further,death,harv,issn,oclc,alignmod,syntax}.exe`
from v89 flags + `SLOT_MAX=24` + one `-D` via `screen_rejects.ps1 -Name`.
2 MiB screen vs `s_base.hp` **439,253**. Two 2 MiB max. Skip archive
already ≥ 430000. Gate 8 MiB s24 only if Δ < **−20**. 35-cap ALONE only
if leftover s24 is >200 under **1,679,762**.

| leftover | flag | salt | scanner | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---|---:|---:|---|
| thumb | `HP_THUMB_MOD` | 274 | sticky `\|thumb` / `\|right` / `\|left` / `\|upright` / `\|frameless` (not FILEOPT, not EXTDISP, not PXSIZE, not GALLERY) | 439,315 | **+62** | **kill** |
| further | `HP_FURTHER_MOD` | 275 | `{{further` / `{{details` / `{{more}}` (not SEEALSO, not MAINART, not HATNOTE) | 439,317 | **+64** | **kill** |
| death | `HP_DEATH_MOD` | 276 | `{{death date` / `{{death year` / `{{dda}}` / `{{death-date` (not BIRTH, not MONTH) | 439,310 | **+57** | **kill** |
| harv | `HP_HARV_MOD` | 277 | `{{harv` / `{{harvnb` / `{{harvtxt` / `{{harvp` (not SFN, not CITE, not CITEKIND) | 439,313 | **+60** | **kill** |
| issn | `HP_ISSN_MOD` | 278 | `ISSN` token / `\|issn=` (not ISBN, not PUBID, not DOI) | 439,317 | **+64** | **kill** |
| oclc | `HP_OCLC_MOD` | 279 | `OCLC` token / `\|oclc=` (not ISSN, not PUBID, not ISBN) | 439,312 | **+59** | **kill** |
| alignmod | `HP_ALIGN_MOD` | 280 | sticky `\|align=` / `\|valign=` / `align="` in tables (not STYLE, not COLSPAN, not CELLKIND, not ROWSPAN) | 439,315 | **+62** | **kill** |
| syntax | `HP_SYNTAX_MOD` | 281 | `<syntaxhighlight` / `<source` / `<syntax` region (not PRECODE, not NOWIKI, not MATH) | 439,313 | **+60** | **kill** |

H47 closed. All eight bytes-up. Closest: death +57 (null-expert tax is +66).
**No 8 MiB gate.** Did not start 8 MiB (no Δ < **−20**). No 35-cap. Did not
overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 /
267,599 / 267,599 / 268,111 / 268,111). Archives
`%LOCALAPPDATA%\hp_lab\s_{thumb,further,death,harv,issn,oclc,alignmod,syntax}.hp`.
Wall ~214–383 s. Champ stays v89 1,678,872 / fx2 1,675,542.

### H49 mixer/APM knobs on v89

Leftover CMs H42–H47 all sit on the null-expert dilution floor (~+66 B).
This wave tunes mixer skip, layer-1 scale, and APM blend — not new CMs.
Ten 2 MiB `SLOT_MAX=24` encodes at once (~1.5 GB each). Baseline
`s_base.hp` **439,253**. Gate 8 MiB s24 only if Δ < **−20**. 35-cap ALONE
only if leftover s24 is >200 under **1,679,762**. Did not overwrite
`hp_v83.exe`–`hp_v89.exe`. Integer-exact. `HP_MIXER_SKIP=56` ON (was 40).
`HP_LR1_SCALE=40` stays on champ unless a leftover replace pays.

| leftover | flag | 2 MiB | Δ vs 439,253 | verdict |
|---|---|---:|---:|---|
| skip24 | `HP_MIXER_SKIP=24` (replace 40) | 439,425 | **+172** | reject |
| skip32 | `HP_MIXER_SKIP=32` (replace 40) | 439,322 | **+69** | reject |
| skip48 | `HP_MIXER_SKIP=48` (replace 40) | 439,224 | **−29** | 8 MiB s24 **1,679,454 (−308 vs 1,679,762)** |
| skip56 | `HP_MIXER_SKIP=56` (replace 40) | 439,210 | **−43** | **v90** 8 MiB s24 **1,679,323 (−439)**; 35-cap **1,678,417 (−455 vs v89 1,678,872)** identity RT PASS |
| lr30 | `HP_LR1_SCALE=30` (replace 40) | 439,214 | **−39** | 2 MiB gate (SHA=lr35) |
| lr35 | `HP_LR1_SCALE=35` (replace 40) | 439,214 | **−39** | 2 MiB gate (byte-identical to lr30) |
| lr45 | `HP_LR1_SCALE=45` (replace 40) | 439,253 | 0 | reject (identity) |
| lr50 | `HP_LR1_SCALE=50` (replace 40) | 439,627 | +374 | reject |
| w0one | `HP_W0=1` (mixer leak into APM blend) | 489,523 | **+50,270** | **kill** |
| wb4 | `HP_WB=4` (lex APM weight 5→4) | 578,616 | **+139,363** | **kill** |

### v89 skip56 → v90 - **1,678,417 / 1.601 bpc, identity RT PASS −455 vs v89**

`HP_MIXER_SKIP=56` (replace 40) on v89 flags, `SLOT_MAX=35`, mem 22.
Identity archive `%LOCALAPPDATA%\hp_lab\e8_8mb_s35_skip56.hp` **1,678,417**
(−455 vs v89 1,678,872). Copied `hp/build/e8_8mb_v90.hp`; lab copy kept.
Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v90.exe` from `hp_c_v89s35_skip56.exe` (268,111). Did not
overwrite `hp_v83.exe`–`hp_v89.exe` (265,457 / 266,575 / 266,993 /
267,599 / 267,599 / 268,111 / 268,111). `HP_MIXER_SKIP=56` ON in
`v78_flags.ps1`. `screen_rejects.ps1` Get-ScreenExe refuses v90;
protected list includes `hp_v90.exe`. No mem 26. No second 35-cap
identity encode.

### v90 fx2 - **1,675,088 / 1.598 bpc, RT PASS −454 vs v89 fx2**

`hp_v90.exe` on `data/enwik8.8mb.fx2man`, `SLOT_MAX=35`, mem 22:
**1,675,088**. Decode SHA matches `data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
Copied `hp/build/e8_8mb_v90_fx2.hp`; lab copy kept. New 8 MB champ is
v90. Did not run mem 26. Did not overwrite `hp_v83.exe`–`hp_v89.exe`.

### H50 mixer knobs on v90

Champ is **v90** skip56. 2 MiB baseline `s_base.hp` **439,210** (copy of
`s_skip56.hp`). Ten 2 MiB at a time. Gate 8 MiB s24 only if Δ < **−20**.
35-cap ALONE only if leftover s24 is >200 under **1,679,323**. Did not
overwrite `hp_v83.exe`–`hp_v90.exe`. Integer-exact. New flags default-off
except `HP_APM_RATE=7` (identity with old default).

| leftover | flag | 2 MiB | Δ vs 439,210 | verdict |
|---|---|---:|---:|---|
| lr30 | `HP_LR1_SCALE=30` (replace 40) | 439,121 | **−89** | 8 MiB s24 **1,678,978 (−345 vs 1,679,323)**; 35-cap **1,678,061 (−356 vs v90 1,678,417)** identity RT PASS |
| lr35 | `HP_LR1_SCALE=35` (replace 40) | 439,121 | **−89** | SHA=lr30; same 8 MiB s24 / 35-cap |
| skip64 | `HP_MIXER_SKIP=64` (replace 56) | 439,213 | **+3** | reject |
| skip72 | `HP_MIXER_SKIP=72` (replace 56) | 439,239 | **+29** | reject |
| skip80 | `HP_MIXER_SKIP=80` (replace 56) | 439,261 | **+51** | reject |
| apm6 | `HP_APM_RATE=6` | 439,992 | **+782** | reject |
| apm8 | `HP_APM_RATE=8` | 439,216 | +6 | reject |
| msc75 | `HP_MIXER_SCALE=49152` (0.75) | 438,897 | **−313** | 8 MiB s24 **1,677,559 (−1,764)**; 35-cap **1,676,677 (−1,740 vs v90 1,678,417)** identity RT PASS; fx2 **1,673,312 (−1,776 vs 1,675,088)** RT PASS. Binary `hp_c_v90s35_msc75.exe`. Did not overwrite `hp_v90.exe` / `v78_flags.ps1` |
| msc112 | `HP_MIXER_SCALE=73728` (1.125) | 439,476 | +266 | reject |
| skipl1 | `HP_MIXER_SKIP_L1=40` | 439,145 | **−65** | 8 MiB s24 **1,678,996 (−327 vs 1,679,323)**; no 35-cap (msc75 used ALONE) |

### v90 msc75 → v91 - **1,676,677 / 1.599 bpc, identity RT PASS −1,740 vs v90**

`HP_MIXER_SCALE=49152` (Q16 0.75 on layer-1 dots) on v90 flags,
`SLOT_MAX=35`, mem 22: **1,676,677** (−1,740 vs v90 1,678,417). Identity
RT PASS (agent). Copied `hp_v91.exe` from `hp_c_v90s35_msc75.exe`. Did not
overwrite `hp_v83.exe`–`hp_v90.exe`. `HP_MIXER_SCALE=49152` in
`v78_flags.ps1`. Get-ScreenExe refuses v91. fx2-manual **1,673,312**
(−1,776 vs v90 fx2 1,675,088) RT PASS. Archives `hp/build/e8_8mb_v91.hp`
and `e8_8mb_v91_fx2.hp`. 2 MiB baseline `s_base.hp` **438,897**. No mem 26.

### H51 mixer knobs on v91

Scale neighbors of 49152 plus lr30 / skipl1 restack on **v91**. Baseline
**438,897**. Ten 2 MiB at a time. Gate 8 MiB s24 only if Δ < **−20**.
35-cap ALONE only if leftover s24 is >200 under **1,677,559**. Did not
overwrite `hp_v83.exe`–`hp_v91.exe`.

| leftover | flag | 2 MiB | Δ vs 438,897 | verdict |
|---|---|---:|---:|---|
| msc50 | `HP_MIXER_SCALE=32768` (0.50) | 450,930 | **+12,033** | reject |
| msc62 | `HP_MIXER_SCALE=40960` (0.625) | 439,094 | **+197** | reject |
| msc69 | `HP_MIXER_SCALE=45056` (0.6875) | 438,926 | +29 | reject |
| msc72 | `HP_MIXER_SCALE=47104` (0.719) | 438,905 | +8 | reject |
| msc78 | `HP_MIXER_SCALE=51200` (0.781) | 438,906 | +9 | reject |
| msc81 | `HP_MIXER_SCALE=53248` (0.813) | 438,921 | +24 | reject |
| msc88 | `HP_MIXER_SCALE=57344` (0.875) | 438,986 | +89 | reject |
| msc94 | `HP_MIXER_SCALE=61440` (0.9375) | 439,097 | +200 | reject |
| lr30 | `HP_LR1_SCALE=30` (replace 40) | 438,893 | **−4** | reject |
| skipl1 | `HP_MIXER_SKIP_L1=40` | 438,841 | **−56** | 8 MiB s24 **1,677,287 (−272 vs 1,677,559)**; 35-cap **1,676,394 (−283 vs v91 1,676,677)** identity RT PASS; fx2 **1,673,051 (−261 vs 1,673,312)** RT PASS. Binary `hp_c_v91s35_skipl1.exe`. Did not overwrite `hp_v91.exe` |

### v91 skipl1 → v92 - **1,676,394 / 1.599 bpc, identity RT PASS −283 vs v91**

`HP_MIXER_SKIP_L1=40` (skip layer-1 axpy if |local err| < 40; layer-2
still trains) on v91 flags, `SLOT_MAX=35`, mem 22: **1,676,394** (−283
vs v91 1,676,677). Identity RT PASS. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v92.exe` from `hp_c_v91s35_skipl1.exe`. Did not overwrite
`hp_v83.exe`–`hp_v91.exe`. `HP_MIXER_SKIP_L1=40` in `v78_flags.ps1`.
Get-ScreenExe refuses v92. Archives `hp/build/e8_8mb_v92.hp` and
`e8_8mb_v92_fx2.hp`. 2 MiB baseline `s_base.hp` **438,841**. fx2-manual
**1,673,051** (−261 vs v91 fx2 1,673,312) RT PASS. No mem 26.

### v92 fx2 - **1,673,051 / 1.595 bpc, RT PASS −261 vs v91 fx2**

`hp_v92.exe` on `data/enwik8.8mb.fx2man`, `SLOT_MAX=35`, mem 22:
**1,673,051**. Decode SHA matches `data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
Copied `hp/build/e8_8mb_v92_fx2.hp`; lab copy kept. New 8 MB champ is
v92. Did not run mem 26. Did not overwrite `hp_v83.exe`–`hp_v91.exe`.

### H52 mixer knobs on v92

SKIP_L1 neighbors of 40 plus lr30 / skip48 restack on **v92**. Baseline
**438,841**. Ten 2 MiB at a time. Gate 8 MiB s24 only if Δ < **−20**.
35-cap ALONE only if leftover s24 is >200 under **1,677,287**. Did not
overwrite `hp_v83.exe`–`hp_v92.exe`.

| leftover | flag | 2 MiB | Δ vs 438,841 | verdict |
|---|---|---:|---:|---|
| sl16 | `HP_MIXER_SKIP_L1=16` (replace 40) | 438,882 | **+41** | reject |
| sl24 | `HP_MIXER_SKIP_L1=24` (replace 40) | 438,866 | **+25** | reject |
| sl32 | `HP_MIXER_SKIP_L1=32` (replace 40) | 438,856 | **+15** | reject |
| sl48 | `HP_MIXER_SKIP_L1=48` (replace 40) | 438,830 | **−11** | reject |
| lr30 | `HP_LR1_SCALE=30` (replace 40) | 438,826 | **−15** | reject |
| skip48 | `HP_MIXER_SKIP=48` (replace 56) | 438,842 | **+1** | reject |
| sl56 | `HP_MIXER_SKIP_L1=56` (replace 40) | 438,815 | **−26** | 8 MiB s24 **1,677,118 (−169 vs 1,677,287)**; no 35-cap (−169 not >200). No mem 26. Binary `hp_s_sl56.exe`. Did not overwrite `hp_v83.exe`–`hp_v92.exe` |
| sl64 | `HP_MIXER_SKIP_L1=64` (replace 40) | 438,798 | **−43** | 8 MiB s24 **1,677,018 (−269 vs 1,677,287)**; no 35-cap (wave). No mem 26. Binary `hp_s_sl64.exe`. Did not overwrite `hp_v83.exe`–`hp_v92.exe` |
| sl72 | `HP_MIXER_SKIP_L1=72` (replace 40) | 438,791 | **−50** | 8 MiB s24 **1,676,933 (−354 vs 1,677,287)**; no 35-cap (wave). No mem 26. Binary `hp_s_sl72.exe`. Did not overwrite `hp_v83.exe`–`hp_v92.exe` |
| sl80 | `HP_MIXER_SKIP_L1=80` (replace 40) | 438,789 | **−52** | 8 MiB s24 **1,676,873 (−414 vs 1,677,287)**; 35-cap **1,675,993 (−401 vs v92 1,676,394)** identity RT PASS. Binary `hp_c_v92s35_sl80.exe`. Did not overwrite `hp_v83.exe`–`hp_v92.exe` |

### v92 sl80 → v93 - **1,675,993 / 1.598 bpc, identity RT PASS −401 vs v92**

`HP_MIXER_SKIP_L1=80` (skip layer-1 axpy if |local err| < 80; layer-2
still trains) on v92 flags, `SLOT_MAX=35`, mem 22: **1,675,993** (−401
vs v92 1,676,394). Identity RT PASS. Decode SHA matches `data/enwik8.8mb`
`09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E`.
Copied `hp_v93.exe` from `hp_c_v92s35_sl80.exe`. Did not overwrite
`hp_v83.exe`–`hp_v92.exe`. `HP_MIXER_SKIP_L1=80` in `v78_flags.ps1`.
Get-ScreenExe refuses v93. Archives `hp/build/e8_8mb_v93.hp` and
`e8_8mb_v93_fx2.hp`. 2 MiB baseline `s_base.hp` **438,789** (old saved
`s_base_v92.hp` **438,841**). fx2-manual **1,672,628** (−423 vs v92 fx2
1,673,051) RT PASS. No mem 26.

### v93 fx2 - **1,672,628 / 1.595 bpc, RT PASS −423 vs v92 fx2**

`hp_v93.exe` on `data/enwik8.8mb.fx2man`, `SLOT_MAX=35`, mem 22:
**1,672,628**. Decode SHA matches `data/enwik8.8mb.fx2man`
`563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E`.
Copied `hp/build/e8_8mb_v93_fx2.hp`; lab copy kept. New 8 MB champ is
v93. Did not run mem 26. Did not overwrite `hp_v83.exe`–`hp_v92.exe`.

### H53 mixer knobs on v93

SKIP_L1 neighbors of 80 on **v93**. Baseline **438,789**. Ten 2 MiB at a
time. Gate 8 MiB s24 only if Δ < **−20**. 35-cap ALONE only if leftover
s24 is >200 under **1,676,873**. Did not overwrite `hp_v83.exe`–`hp_v93.exe`.

This-tree 2 MiB `SLOT_MAX=24` baseline `hp_s_base.exe` **439,192**
(231.5 s). Recorded v93 `s_base.hp` **438,789** was overwritten; deltas
below are vs **439,192** (same compile line as the leftovers). Did not
overwrite `hp_v83.exe`–`hp_v93.exe`. No mem 26.

| leftover | flag | 2 MiB | Δ vs 439,192 | verdict |
|---|---|---:|---:|---|
| sl72 | `HP_MIXER_SKIP_L1=72` (replace 80) | 439,193 | **+1** | reject |
| sl84 | `HP_MIXER_SKIP_L1=84` (replace 80) | 439,184 | **−8** | reject |
| sl88 | `HP_MIXER_SKIP_L1=88` (replace 80) | 439,184 | **−8** | reject |
| sl96 | `HP_MIXER_SKIP_L1=96` (replace 80) | 439,183 | **−9** | reject |
| sl104 | `HP_MIXER_SKIP_L1=104` (replace 80) | 439,184 | **−8** | reject |
| sl112 | `HP_MIXER_SKIP_L1=112` (replace 80) | 439,168 | **−24** | 8 MiB s24 **1,677,930 (−249 vs this-tree 1,678,179; +1,057 vs v93 s24 1,676,873)**; no 35-cap (not >200 under 1,676,873). No mem 26. Binary `hp_s_sl112.exe`. Did not overwrite `hp_v83.exe`–`hp_v93.exe` |
| sl120 | `HP_MIXER_SKIP_L1=120` (replace 80) | 439,169 | **−23** | 8 MiB s24 **1,677,899 (−280 vs this-tree 1,678,179; +1,026 vs v93 s24 1,676,873)**; no 35-cap. No mem 26. Binary `hp_s_sl120.exe`. Did not overwrite `hp_v83.exe`–`hp_v93.exe` |
| sl128 | `HP_MIXER_SKIP_L1=128` (replace 80) | 439,174 | **−18** | reject (not &lt; −20) |
| sl144 | `HP_MIXER_SKIP_L1=144` (replace 80) | 439,178 | **−14** | reject |
| sl160 | `HP_MIXER_SKIP_L1=160` (replace 80) | 439,190 | **−2** | reject |

This-tree 8 MiB s24 base **1,678,179**. sl112/sl120 beat that screen
(−249/−280) but miss the published v93 s24 bar **1,676,873**, so no
35-cap. Champ stays **v93 1,675,993**. H53 **closed**. Did not overwrite
`hp_v83.exe`–`hp_v93.exe`. Archives `%LOCALAPPDATA%\hp_lab\e8_s24_h53base.hp`,
`e8_s24_sl112.hp`, `e8_s24_sl120.hp`; 2 MiB copy `s_base_h53.hp` **439,192**.

### Throughput / RAM (kVersion 5, identity-safe)

v93 champ flags are now the default model set. `HP_SLOT_MAX` is
**hard-capped at 22**. `-DHP_SLOT_MAX` is ignored. Do not overwrite
`hp_v83.exe`–`hp_v93.exe`.

Structural squeeze (one g++ recipe, no attributes / PGO / prefetch):
demand-zero `HashTable` (VirtualAlloc/mmap) for CM tables, HASH_CHK
tags, byte ring, match/LZP/Hebbian; AVX2 mixer `dot_i32_i16`;
`-flto -march=x86-64-v3 -mavx2 -fno-exceptions -fno-rtti`; PY
`(num<<4)/total` identical to `(num<<12)/(total<<8)`. `HP_MATCH_WORD`
stays 0.

2 MiB mem 22 `SLOT_MAX=24` vs `hp_s_base.exe`: both **439,192**
(1.675 bpc). `hp_opt_s24.exe` **196.2 s / 1502 MB WS** vs s_base
**198.2 s / 1595 MB WS** (−93 MB, −2 s). proxy256k mem 18 both
**18,702**. Roundtrip proxy64k/256k PASS. Binary `hp/build/hp_opt.exe`
(default cap 28) and `hp_opt_s24.exe`. Did not overwrite
`hp_v83.exe`–`hp_v93.exe`. No mem 26.

I/O follow-up (still identity **439,192**): Windows `MapViewOfFile` for
the input (same as Linux mmap — no extra copy of the file); 1 MiB
coder / stdio buffers; decode writes 64 KiB chunks instead of
`fputc` per byte. Hand-vectorized mixer axpy was slower than g++'s
own schedule and was reverted. Peak WS still **~1503 MB** on 2 MiB.
Did not overwrite `hp_v83.exe`–`hp_v93.exe`. No mem 26.

### Cross-context CM screen (256 KB all-pairs/triples, then 2/8 MiB)

One extra CM (`-DHP_CROSS_CM=1`, `--cross i,j[,k]` hashes live
context keys). 64-way mem 16 `SLOT_MAX=24` on `proxy256k.xml`:
C(40,2)=780 pairs + C(40,3)=9880 triples + idle/125 baselines.
10,664 ok, 1 zero (`t_25_29_31`). Idle extra CM **18,737** vs
125-expert **18,733** (+4 dilution). 2,400 combos beat 125 on this
proxy (noise-heavy). Best 256 KB: `word*brk*sentmem` **18,598**
(−135). Condenses vs 125: nohash2 **18,735**, nomatchx **18,735**,
notwin **18,732** (−1, noise).

2 MiB mem 22 s24 (base **439,192** matches identity):

| combo | bytes | Δ vs 125 |
| o2×sentmem (`1,17`) | 438,897 | **−295** |
| o1×word×sentmem (`0,5,17`) | 438,920 | **−272** |
| word×brk×sentmem (`5,12,17`) | 438,973 | **−219** |
| word×sentmem (`5,17`) | 439,015 | **−177** |
| word×sen×sentmem (`5,15,17`) | 439,047 | **−145** |
| sentmem×sengrp (`17,18`) | 439,087 | **−105** |
| sen×sentpos (`15,36`) | 439,177 | −15 reject |
| idle extra | 439,216 | +24 |

8 MiB mem 22 s24 (this-tree base **1,678,179**):

| combo | bytes | Δ vs base |
| o2×sentmem | **1,676,998** | **−1,181** |
| o1×word×sentmem | 1,677,215 | **−964** |

Published v93 s24 bar **1,676,873**. Single o2×sentmem s24 is **+125**
over that bar.

Stacked two extra CMs (`-DHP_CROSS_CM=2`). 256 KB pin o2×sentmem then
all pairs/triples as `--cross2` (10,678 jobs). 2 MiB cartesian of
2 MiB winners + `o2×sentmem + word×brk`:

| combo | 2 MiB s24 | Δ vs 439,192 |
| o2×sentmem + word×brk (`1,17`+`5,12`) | **438,528** | **−664** |
| o2×sentmem + o1×word×sentmem | 438,744 | −448 |
| o2×sentmem idle-2nd | 438,920 | −272 |
| base125 | 439,192 | 0 |

8 MiB mem 22 s24 vs **1,678,179**:

| combo | bytes | Δ |
| o2×sentmem + word×brk | **1,675,990** | **−2,189** |
| o2×sentmem + o1×word×sentmem | 1,676,419 | −1,760 |
| o2×sentmem alone | 1,676,998 | −1,181 |

s24 **1,675,990** is **−883** vs published s24 bar **1,676,873** (>200),
so 35-cap. `hp_cross2_s35.exe` 8 MiB mem 22 SLOT_MAX=35
`--cross 1,17 --cross2 5,12`: **1,675,128** (−865 vs v93 champ
**1,675,993**; 1.597 bpc). Did **not** overwrite `hp_v93.exe`.
`HP_CROSS_CM` default still 0 until baked. ~24 GB WS at s35.

### Integer LSTM insertion screen (256 KB + 2 MiB s24)

Q15 ByteMixer-style cell in `hp/include/hp/lstm.hpp` (no float).
Winner shape from harvest cmix-lex: byte softmax → bit p from
`[bot,top]` split; `lstmpr` = mixer expert; `lstmex` = extra CM.
Default `HP_LSTM=0`. One leftover binary per path. `SLOT_MAX=24`.

256 KB mem 16 vs base **18,733**:

| path | bytes | Δ |
| exp16 / exp8 / exp32 / exp_ctx | 18,733 | 0 (different SHA) |
| ctx only (lstmex CM) | 18,735 | +2 |
| bit16 | 18,744 | +11 |
| exp+gate / all / gate | 18,749–18,751 | +16..+18 |

2 MiB mem 22 s24 vs identity base **439,192**:

| path | bytes | Δ |
| ctx only | 439,198 | +6 |
| bit16 | 439,570 | +378 |
| exp32 | 439,856 | +664 |
| exp_ctx | 439,918 | +726 |
| exp16 | 439,923 | +731 |

All reject (need Δ &lt; −20). Tiny integer LSTM is not cmix’s 170–200
cell float ByteMixer; at this size it is mixer dilution.

Wave 2 (H=64/128, mixin, LR=3/8, 2-layer) 256 KB vs **18,733**:
lr3 **18,730 (−3)**; others −1..+2. Noise; not promoted. Default
`HP_LSTM=0`. Did not overwrite `hp_v83.exe`–`hp_v93.exe`. No mem 26.

### SLOT_MAX 1..30 sweep → hard cap 22 (2026-09-20)

v93 flags, `proxy256k.xml` (262,144 B), `--mem 16`. Thirty binaries
`hp_slot1.exe`…`hp_slot30.exe`. Archives `%LOCALAPPDATA%\hp_lab\slot256\`.
CSV `slot256_sweep.csv`. Did not overwrite `hp_v83.exe`–`hp_v93.exe`.
No mem 26. No 8 MiB (24–28 8 MiB jobs killed). fx2-cmix 8 MiB still
SIGSEGV at 0.38% (not OOM).

| cap | bytes | bpc | peak WS |
|---:|---:|---:|---:|
| 1 | 19,977 | 0.6096 | 29 MB |
| 16 | 18,742 | 0.5720 | 33 MB |
| 20 | 18,734 | 0.5717 | 59 MB |
| **22** | **18,732** | **0.5717** | **113 MB** |
| 24 | 18,733 | 0.5717 | 241 MB |
| 25 | 18,735 | 0.5717 | 395 MB |
| 26–30 | 18,735 | 0.5717 | 395 MB |

BPC is **0.5717 from 20 through 30**. Caps 25–30 are byte-identical.
22 is the lowest cap on the plateau that also wins size (18,732).
Raising 22→30 is 3.5× RAM for three bytes of noise, and those three
are worse. At mem 16, unclipped o6/word request 25 so ≥25 is a no-op.

**Landed:** `HP_SLOT_MAX` hard-capped at **22** in `features.hpp`
(`#undef` then `#define 22`; `-DHP_SLOT_MAX` ignored). Dropped from
`v78_flags.ps1`. Screen tools no longer force 24/35. Archive
`kVersion` **5→6** so 35-cap v93 archives fail fast. Published v93
8 MiB **1,675,993** remains the 35-cap identity; `hp_v93.exe` not
overwritten. Do not pass `-DHP_SLOT_MAX=35`.

### fx2-cmix WSL remap fix + 8 MiB encode (2026-09-20)

Unpatched fx2 SIGSEGV at ~20 KB on WSL: PPMD munmap/remap every
20,000 bytes moved the 14 GB heap VA while MaxContext held absolute
pointers. Lab patch: skip that remap (`if (false && …)`); MAP_SHARED
already writes through. Patched `cmix -c dictionary` on
`data/enwik8.8mb`: **1,395,442** bytes / **1.331 bpc**, wall ~100 min,
peak RSS ~5.6 GB. vs hp v93 1,675,993 / 1.598 (−280,551). vs hard
SLOT_MAX=22 encode 1,684,990 / 1.607.

### fx2_mixer_lab screens (32 KB, lab PPM 512)

~100 variants. Baseline **9165**. Paid: LSTM LR 0.08 (−18), horizon
32 (−10), L1 scale 1.5–2.0 (−13..−14), LR0.08+skip0.01 (−19). Died:
Boolean XOR/AND/OR/Walsh/parity/deltas (xor_k6 +517; worse with k);
natural gradient / sign-LMS (catastrophic); drop twins to fund LSTM
(none beat baseline; tiny_lstm +16). CSVs under
`fx2_mixer_lab/results/`. 64 KB pin: baseline64 **17032**; lr08_64
**16994 (−38)**; lr08+skip 16996; h32 17019 (−13); **lr08+h32 16964
(−68)** best pin. Did not land into prize binary. Did not overwrite
`hp_v83.exe`–`hp_v93.exe`.



### CyphaLM gate24 lossy screen (Cypha, 2026-09-23)

Run in odin-loki/Cypha (branch `claude/llm-profiling-optimization-y5cdrr`)
on the vendored gate24 tree: v78 set, `HP_SLOT_MAX=24`, mem 22. This is
not this lab's `SLOT_MAX=22` build, so the numbers are not v93 archive
bytes. Metric: observe bpc (= archive bpc at the same flags). Full write-up:
Cypha `docs/reports/CYPHALM_LOSSY_MIXER_REPORT.md`.

**8 MiB (`enwik8.8mb`), gate24 = 1.611729, peak RSS 1,538 MB:**

| variant | bpc | Δ | peak RSS |
|---|---:|---:|---:|
| drop 8 wiki CMs (para, nest, infokey, linkpipe, tpl, o6b, heading, capmask) | 1.610643 | −0.0011 | 1,406 MB |
| discovery pool 8 slots (of 12) | 1.610558 | −0.0012 | 1,490 MB |
| both + pool tables ≤2^20 + match tables ≤2^22 (`lean`) | 1.609866 | −0.0019 | 1,078 MB |
| lean + CM tables ≤2^23 | 1.612457 | +0.0007 | 814 MB |
| lean + CM tables ≤2^22 | 1.617400 | +0.0057 | 670 MB |
| lean + CM/match ≤2^21 | 1.629798 | +0.0181 | 404 MB |
| lean + CM/match ≤2^20, pool ≤2^18 | 1.652317 | +0.0406 | 253 MB |

**1 MiB drop-one ablation** of all 35 context models: eight are negative
(listed above), four are neutral (line, cat, state, title, each within
+0.0001), and the rest pay (word +0.025, o4 +0.009, o2 +0.006, brk +0.006,
o3 +0.005 …). Stacking the eight negatives gave −0.0023; adding the four
neutrals gave +0.0006.

**Mixer:** every one of the 10 layer-1 weight sets pays on 1 MiB
(+0.0035 to +0.0177 to drop). Raising the update skip threshold 32→64/128/256
costs +0.0023/+0.0068/+0.0149. Consistent with H33 (the mixer is tuned);
the lossy room is in its inputs, not its weights.

**New evidence against "do not retest" rows:** `discovery slot/eval sweep`
(H0.2, all reject) predates the v78 stack. At gate24, 8 slots beat 12 on
both 1 MiB (−0.0017) and 8 MiB (−0.0012). The eight dropped wiki CMs were
each accepted one at a time on the 8 MiB identity in the `SLOT_MAX=35`
leftover wave (e.g. nest −560 B v58, para −98 B v59). Stacked on gate24 at
SLOT 24 and without dict preprocessing, they cost more than they return. Worth a re-screen at this lab's `SLOT_MAX=22` before
trusting either direction.

**Speed / RAM notes that apply here too:** `perf` puts 62% of observe time
in `ContextModel::predict` (2 cache misses per model per bit). Prefetching
each model's next slot as soon as the bit is known is bit-identical;
the speed A/B against this lab's huge-page tables is pending (Cypha report). The lab's `alloc_zero` + `MADV_HUGEPAGE` tables had been
lost in the Cypha vendoring (eager `std::vector`, ~8 s construct at mem 22).
Cypha has them back.
