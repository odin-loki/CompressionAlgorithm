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

### v82wikibold → v83 - **1,687,899 / 1.609 bpc, −1,258 vs v82 (RT pending)**

`HP_WIKIBOLD_MOD` wiki `''`/`'''`/`'''''` bold-italic state.

### H22 preprocess / richer stacks on v78

Flags: `HP_WIKISTACK_MOD` (fccxt+bracket+cell-above packed CM), `HP_REORDER` (sort `<page>` by title), `HP_PAYLOAD_LEX` (sort `<page>` by `<text>`), `--dict` on `hp_v78.exe`. One at a time after H21.


