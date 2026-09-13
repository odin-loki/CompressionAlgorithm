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

### v62meta stack (compiled, 8 MB not started)

v61-100 (`hp_v61_m26`) still live — did not start a second hp. Compiled `hp_g_v62meta.exe` = v61 flags + `HP_WIKI_AXES=1` (dedicated state / sent_domain / wiki_header / depth CMs) + `HP_MIXER_RANK=8` (U(ctx)×V shared layer-1) + `HP_XSIMD=1 -msse4.1`. Did not overwrite live champs. 8 MB mem 22 vs 1,705,939 when RAM is free.
