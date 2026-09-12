<!--
ARCHIVED. Superseded by repo-root PLAN.md. Do not add new experiments here.
This was the harvest-era spec; Track H now lives at the workspace root.
-->

# PLAN — Model Harvest and Meta-Patterns

Two work packages. This document is a specification of what will be done, not
a record of work done. Nothing here is implemented.

---

# PACKAGE A — Harvest every model from the prize lineage

## A.0 Sources to pull

| source | where | why |
|---|---|---|
| fx2-cmix | github.com/kaitz/fx2-cmix | current record base, GPL, already cloned once |
| cmix | github.com/byronknoll/cmix | the model zoo everything descends from |
| lpaq / paq8 family | github.com/kaitz, mattmahoney.net | canonical implementations |
| starlit | github.com/amirstarlit | article reordering preprocessor |
| nncp | bellard.org/nncp | transformer route, disqualified but SOTA |
| phda9 / zpaq | mattmahoney.net | Mahoney's own lineage |

All GPL or public. Reading source is the sanctioned path — it is what every
winner since 2006 has done.

## A.1 Build the model inventory

Produce `MODELS.md`: one row per distinct model across all sources.

Columns: name, source file, what context it reads, table size, update rule,
whether `hp` has an equivalent, estimated port effort (hours), and — the
column that matters — **which information axis it reads**.

The axis column is the point. `hp` measures effective rank 3.34 of 28, so
models that read an axis already covered are worthless no matter how well they
perform in their home compressor. Everything gets sorted by axis novelty
before effort.

Known axes so far: byte suffix, word, word-bigram, sparse/skip, 2-D column,
XML structure, match at various orders, Hebbian association. Expect the
harvest to surface at least: bracket/nesting depth, first-character class,
paragraph and line position, capitalisation state, numeric-field structure,
UTF-8 continuation state, link-vs-body state, and PPM-style unbounded order.

## A.2 Priority ports (expected, subject to A.1)

**a. fx2-cmix wiki states.** `WIKITABLE`, `SQUAREOPEN`, `HTLINK`,
`VERTICALBAR`, `linkword = linkword*2104 + j`. `tag:py` is already the most
decorrelated expert in `hp` (mean |corr| 0.621) while being only a generic XML
depth counter. Giving the most decorrelated expert real structure is the
highest-confidence item in this package.

**b. Word-keyed match models.** fx2-cmix runs five match models keyed on word
context, not byte context. `hp` has none. Match models already occupy four of
its top-six decorrelated slots, so a word-keyed variant should land well.

**c. Word streams.** fx2-cmix maintains four separate word streams with ten
sparse configurations over them. `hp` has one word model and one bigram.

**d. PPMD order-25.** Structurally different from context mixing, therefore
likely decorrelated. Large memory appetite; check against the 10 GB budget.

**e. Bracket / nesting model.** 256 contexts, depth 15.

**f. Per-mixer learning rates.** 24 mixers with individually tuned rates from
0.0003 to 0.005. `hp` has 6 sharing one rate. Mechanical, cheap.

## A.3 Acceptance gate — this is the whole method

No model enters `hp` on reputation. Each candidate must pass, in order:

1. Round-trip clean (`test/roundtrip.sh`)
2. Bit-exact across all 8 builds (`test/determinism.sh`)
3. **Raise effective rank** — `dump_experts` + `mp_rank.py` before and after
4. Reduce bpc on both the wiki proxy and real enwik8
5. Cost less than 5% throughput per 0.1% gain

Anything failing (3) is a duplicate axis and gets rejected even if it passes
(4), because a duplicate that helps today will be superseded by a cheaper
model later. Record every rejection with its rank delta — the rejections are
the map of which axes are saturated.

## A.4 Deliverables

- `MODELS.md` — the full inventory
- `AXES.md` — axis coverage matrix, `hp` vs fx2-cmix vs cmix
- ported models behind individual compile flags for clean ablation
- one line in `UPGRADES.md` findings per accepted or rejected model

---

# PACKAGE B — Meta-patterns

The idea: common structures recur across the corpus, so recognise them once
and reuse the recognition instead of re-learning it in every context.

There are three genuinely different things this could mean. They have very
different cost profiles and B.1 is the only one that is clearly free.

## B.1 Pattern cache — speed, not ratio

**What.** A small cache mapping a recently-seen context signature to its
resolved prediction, checked before the full model stack runs. On a hit, skip
the hashing and table walks for the models that would have agreed anyway.

**Why it is safe.** The cache is *derived state* — both encoder and decoder
build it identically from bytes already coded, so nothing is transmitted and
determinism holds. It changes only which code path runs, never the probability
produced. Verified by requiring bit-identical archives with the cache on and
off.

**Payoff.** Throughput, not compression. `hp` currently runs ~3.7 µs/byte with
roughly 50x headroom against the 50-hour budget — so speed converts directly
into affording more models. This is the item that funds Package A.

**Risk.** Cache lookup may cost more than the models it skips. Measure before
keeping.

## B.2 Meta-pattern models — ratio

**What.** Models whose context is *which pattern class we are in* rather than
which bytes preceded. Detect a repeating structure — a table row, a citation
template, a timestamp field, an infobox — and make the detected class a
context.

**Why it should work.** This is a genuinely new axis, and the diagnostic says
new axes are the binding constraint. It is also a generalisation of what the
tag and column models already do successfully.

**How to detect classes without transmitting anything.** Extend
`discover.hpp`. It already runs deterministic MDL-gated search over context
masks with an identical PRNG on both sides. The same machinery can search over
*pattern templates* rather than byte masks — the mechanism is proven, only the
candidate space changes.

**Watch for.** A pattern class that duplicates an existing axis. Gate it
through A.3.

## B.3 Cross-run pattern library — DO NOT SHIP

**What it would be.** Mine enwik8 for common structures, store the library,
ship it with the compressor.

**Why it is listed as a warning.** S = S1 + S2. This is exactly the dictionary
experiment already run here: the transform won 1,498 B and 10,121 B of stored
table destroyed the gain. A pattern library is the same trade at larger scale.

It may still pay at enwik9 — fx2-cmix ships a 412 KB `english.dic` and it pays
for itself at 0.37% of a 110 MB archive. But that must be *measured on
enwik9*, not assumed, and the library must be compressed before embedding.
Any version of B.3 is blocked on the enwik8 baseline existing first.

**The Hebbian result is the guide here.** `HebbianModel` gets dictionary-like
behaviour for zero transmitted bytes because both sides potentiate identical
synapses from data they already hold. Prefer learned over stored every time.
B.3 only becomes interesting where learning is too slow to converge and
amortisation is genuinely large.

## B.4 Deliverables

- `PATTERNS.md` — taxonomy of detected classes with frequency counts
- pattern cache behind a flag, with a determinism test proving archives are
  identical with it on and off
- meta-pattern candidate generator in `discover.hpp`
- explicit go/no-go on B.3, decided by measurement on enwik8

---

# ORDER OF WORK

1. **enwik8 baseline.** Nothing in either package is decidable without it.
   Every number in this repo is from a 3 MB synthetic proxy.
2. **B.1 pattern cache** — buys the throughput that funds everything else.
3. **A.1 inventory** — cheap, and it determines what is worth porting.
4. **A.2 a/b/f** — highest-confidence ports.
5. **B.2 meta-pattern models.**
6. **A.2 c/d/e** — larger ports, only if rank is still low.
7. **B.3 decision** — measurement only, no build until it passes.

# STANDING CONSTRAINTS

- No `float`, `double`, `<cmath>` in `include/` or `src/`.
- Both sides derive all state from already-coded data.
- `no_float.sh`, `roundtrip.sh`, `determinism.sh` pass before anything is
  accepted.
- Effective rank measured before and after every model addition.
- Every rejection recorded with its rank delta.
