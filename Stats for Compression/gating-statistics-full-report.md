# Gating Statistics for Context-Mixing Compression

## A measurement study on enwik8

**Odin Loch**

---

## Abstract

This study tests a single hypothesis: that in a PAQ/cmix-class compressor, a statistic
pays more when used as a **gate** — selecting which mixer weight set or SSE table runs —
than as an **input**, one more vote into the mixer.

The hypothesis is confirmed, and the effect is large. Every statistic tested gained
**2–4× more as a mixer weight-set selector than as an additional SSE stage**. The best
combination improved a competitive baseline by **+0.0171 bpc** at no time cost and
negligible code cost.

Thirty-nine candidate statistics were screened, drawn both from first principles and
from four mathematical literatures. Two survived as genuinely useful; most did not.
The negative results and the methodological failures are more transferable than the
winners, and are reported in full.

---

## 1. Setup

### 1.1 Corpus

enwik8 — the canonical 100,000,000-byte Wikipedia extract, md5
`a1fa5ffddb56f4953e226637dabbb36a`. This is the actual prize corpus for the pre-2020
track, not a proxy. Screening used the first 10 MB; scale validation used the first
30 MB.

Using the real corpus removes the main threat to this kind of measurement: a gain
measured on a weak proxy is usually slack, not signal.

### 1.2 Baseline

A purpose-built lpaq-class context-mixing model:

- 7 context models — orders 1, 2, 3, 4, 6, a word model, and order 0
- nibble-bucket hash tables: one 64-byte cache line per nibble holding a 15-node
  tree, checksummed, with reset on collision
- each model contributes **both** a direct adaptive counter and a StateMap-indirect
  prediction — 17 mixer inputs total
- logistic mixer, 2048 weight sets selected by `c0 × match-length`
- two-stage APM/SSE chain
- two match models at different minimum orders

Cost is the ideal code length `−log2 p` summed over all bits. Arithmetic coder overhead
is below 1e−4 bpc and is ignored.

| | bpc |
|---|---|
| baseline, 10 MB | **1.785846** |
| baseline, 30 MB | **1.751038** |

This is a genuinely competitive substrate. It is not frontier — 7 models against
cmix's hundreds — but the measured deltas are not slack in a broken model.

### 1.3 The screen

One instrumented run accumulates, over all 80 million bits, the joint histogram
`count[p̂-bin][S-bin][bit]` for all 39 candidate statistics simultaneously
(33 × 16 × 2 cells each). From these, the conditional mutual information
`I(bit ; S | p̂)` is computed with **Miller–Madow bias correction**.

Two pseudo-random statistics, independent of the coded bit by construction, provide an
empirical noise floor. Both corrected to **0.00000**, confirming the estimator is
unbiased.

Cost: one 34-second run covers all candidates — roughly 25× cheaper than wiring each
one individually.

### 1.4 Confirmation by real wiring

Each surviving candidate was then wired in for real, two ways, and the actual bpc
measured:

- **SSE gate** — an extra APM stage indexed by `S × c0`
- **Mixer gate** — the statistic joins the mixer weight-set selector

Both wirings carry an intrinsic overhead regardless of whether the statistic carries
information, so both were also run with a random statistic.

**All realised gains below are quoted against the matching null control, not against
the baseline.** This is load-bearing, not pedantry — see §3.2.

| control | bpc | overhead |
|---|---|---|
| null as SSE stage | 1.792894 | +0.00705 |
| null as mixer gate (16-way) | 1.792226 | +0.00638 |
| null as mixer gate (64-way) | 1.789854 | +0.00401 |

---

## 2. Results

### 2.1 Full ranking

Screen figures are CMI in bpc-equivalent. Gate figures are measured bpc, with gain
quoted against the matching null.

| statistic | what it measures | screen | SSE gate | mixer gate |
|---|---|---|---|---|
| `st_shape6` | (p, n) shape of the order-6 bit-history slot | 0.00891 | +0.00951 | **+0.02054** |
| `rec_branch3` | distinct successors seen for the order-3 context | 0.00229 | +0.00452 | **+0.01742** |
| `st_cnt6` | confidence count of the order-6 slot | 0.00091 | – | +0.01642 |
| `disp_var` | variance of model predictions before mixing | 0.00254 | +0.00566 | +0.01551 |
| `mm_len2` | secondary match model length | 0.00207 | +0.00335 | +0.01496 |
| `xo_argmaxord` | which order is currently most confident | 0.00120 | +0.00390 | +0.01467 |
| `kt_gap6` | KT/Jeffreys estimate vs adaptive counter | **0.00534** | – | +0.01445 |
| `disp_range` | max − min of model predictions | 0.00195 | – | +0.01428 |
| `gt_hapax4` | Good–Turing hapax fraction N₁/N | 0.00145 | – | +0.01321 |
| `disp_signsum` | signed vote count across models | 0.00165 | – | +0.01256 |
| `disp_maxabs` | largest single model confidence | 0.00281 | +0.00489 | +0.01077 |
| `fs_entropy` | Fixed-Share posterior entropy | 0.00181 | – | +0.00847 |
| `sur_delta` | fast minus slow EWMA surprisal | 0.00028 | – | +0.00747 |
| `sur_switch` | EWMA of argmax-model identity change | 0.00014 | – | +0.00554 |
| `fs_switch` | Fixed-Share switch posterior | 0.00104 | – | – |
| `ctl_matchlen` | match length *(already used by baseline)* | 0.00184 | +0.00217 | – |
| `ctl_xmldepth` | XML nesting depth | 0.00077 | – | – |
| `gt_hapax3` | Good–Turing hapax, order 3 | 0.00062 | – | – |
| `cal_resid` | running calibration residual per p̂-bin | 0.00013 | – | – |
| `ada_gap` | AdaHedge mixability gap | 0.00010 | – | – |
| `bocpd_rl` | BOCPD MAP run length | 0.00008 | – | – |
| `bocpd_cp` | BOCPD changepoint probability | 0.00000 | – | – |
| *null control* | pseudo-random | 0.00000 | 0.00000 | 0.00000 |

### 2.2 Combinations

Each statistic quantised to 4 bins in the pairs and triples, so weight-set count is
held constant across rows — a fair comparison against the 16-bin singles.

| combination | bpc | vs baseline |
|---|---|---|
| `branch3 × dispvar × shape6` | **1.768769** | **+0.01708** |
| `branch3 × dispvar × mm_len2` | 1.769673 | +0.01617 |
| `branch3 × dispvar × kt_gap6` | 1.770033 | +0.01582 |
| `branch3 × dispvar × fs_switch` | 1.770644 | +0.01520 |
| `branch3 × dispvar` | 1.770638 | +0.01521 |
| `shape6 × branch3` | 1.770685 | +0.01516 |
| `shape6 × cnt6` *(same family)* | 1.772961 | +0.01289 |
| `hapax4 × dispvar × shape6` | 1.773828 | +0.01202 |
| `shape6 × dispvar` | 1.774556 | +0.01129 |
| `shape6` alone (16 bins) | 1.771683 | +0.01416 |

### 2.3 Scale

| 30 MB | bpc | vs baseline |
|---|---|---|
| baseline | 1.751038 | – |
| null (16-way) | 1.750542 | +0.00050 |
| `branch3 × dispvar` | 1.734908 | **+0.01613** |

The gain **grows** with data: +0.01521 at 10 MB, +0.01613 at 30 MB. Gating benefits
from more data because each gated weight set gets more training. Note the null control
turns marginally positive at 30 MB — the dilution cost of extra weight sets amortises
away.

---

## 3. Findings

### 3.1 Gate position dominates statistic choice

The same statistic is worth 2–4× more as a mixer selector than as an SSE stage:

| statistic | SSE | mixer | ratio |
|---|---|---|---|
| `rec_branch3` | +0.00452 | +0.01742 | 3.9× |
| `xo_argmaxord` | +0.00390 | +0.01467 | 3.8× |
| `mm_len2` | +0.00335 | +0.01496 | 4.5× |
| `st_shape6` | +0.00951 | +0.02054 | 2.2× |

Gating the mixer multiplies across the whole ensemble. An extra SSE stage only
re-calibrates a scalar that has already collapsed the ensemble into one number.
**Where you put a statistic matters more than which statistic you pick.**

### 3.2 The null control is load-bearing

An empty SSE stage costs +0.0070 bpc by itself — larger than most statistics being
measured. Against the raw baseline, only `st_shape6` improved as an SSE gate; every
other statistic appeared to make things *worse*. The information was real but smaller
than the stage's overhead.

Any experiment comparing against the baseline instead of a matched null will mis-rank
the entire field and discard working statistics.

### 3.3 Bit-history shape is the strongest single gate

`st_shape6` is derived from a slot that is *already a mixer input*. The predicted
probability is used; the shape of the evidence behind it is discarded. Recovering that
shape as a selector was the largest single win.

### 3.4 Cross-family statistics stack; same-family ones do not

`rec_branch3 × disp_var` beats either alone despite each being quantised more coarsely
— genuine independent information. `st_shape6 × st_cnt6`, both read off the same
order-6 slot, is *worse* than `st_shape6` alone: pure redundancy, paid for in weight
dilution.

Choose combination candidates across families, never by individual rank.

### 3.5 The screen filters reliably but ranks poorly

Spearman ρ between CMI and realised mixer-gate gain is **0.56**. It cleanly separates
carries-information from carries-nothing — nulls, `sur_switch`, and the BOCPD family
all sort to the bottom — but does not order the middle. `st_cnt6` ranked 21st on the
screen and 3rd in reality.

Use it to discard, then wire the survivors.

### 3.6 The screen over-credits statistics that duplicate existing mixer inputs

`kt_gap6` scored **second** on CMI (0.00534, nearly double the previous runner-up) and
came seventh in realised gain. The cause is structural: `I(bit ; S | p̂)` conditions on
the mixer's *output* but not its *input vector*. `kt_gap6` is computed from the order-6
slot, which already feeds the mixer directly. The information is genuinely present —
the mixer already had access to it.

Confirmed in the combinations: `kt_gap6` adds only +0.0006 on top of
`branch3 × dispvar`, while `st_shape6` adds +0.0019 to the same pair.

**Correction:** condition on `p̂` *and* on the strongest existing gate, not `p̂` alone.

### 3.7 The switching-rate idea works — but only in its Bayesian form

| formulation | CMI |
|---|---|
| EWMA of argmax-model identity change | 0.00014 |
| Fixed-Share (Herbster–Warmuth) switch posterior | 0.00104 |

A factor of 7 between two operationalisations of the same idea. The posterior mass of
the winning expert that arrived via the share term carries real information; the moving
average carries essentially none.

The signal remains modest and did not improve the best combination. But the earlier
conclusion — "switching rate is at the noise floor" — applies only to the crude version.
**A statistic's family does not determine its value; its exact formulation does.**

### 3.8 Normalising destroyed information

Good–Turing's `N₁/N` is the principled escape estimator; a raw successor popcount is
not. The crude one won: 0.00229 against 0.00145 on the screen, +0.01742 against
+0.01321 as a gate.

Dividing by the total discards *how much evidence exists* — which is exactly what a
gate needs. A context with 2 successors out of 2 and one with 40 out of 40 have
identical hapax fractions and should route to completely different weight sets.

**Ratios are the wrong shape for gating.** Keep numerator and denominator as separate
dimensions.

### 3.9 BOCPD and calibration statistics do not pay here

Run-length posterior, changepoint probability, AdaHedge mixability gap, and calibration
residual all sat at or near the noise floor. Run-length information is largely redundant
with match length and recency, which the mixer already uses.

### 3.10 Code size is not the constraint; memory is

These statistics are tens of bytes of compiled logic each. The three-way gate ran in
**28 s** against the baseline's **29 s** — no measurable time cost, because the work is
table indexing, not computation.

The real cost is RAM: the three-way gate multiplies the weight table 64×. Under the
prize's 10 GB ceiling, **weight-set count is the scarce resource.** This reframes gate
design as a memory-budget allocation problem, and makes the binding question *which
statistics to leave out*. Adding gates indiscriminately dilutes every weight set and
loses — exactly what `shape6 × cnt6` demonstrated.

---

## 4. Implementation notes

Three bugs surfaced during construction. All three produced *plausible* output rather
than obvious failure, which is the reason to record them:

1. **StateMap update omitted the shift into the probability field.** The delta was added
   to the packed word without being scaled, so every indirect prediction adapted ~1000×
   too slowly. All seven indirect inputs were effectively frozen at 0.5. Cost: ~1.2 bpc.

2. **APM interpolation used the wrong shifts.** The index ran off the end of each
   33-entry row into neighbouring contexts. The mixer alone was already at 1.90 bpc
   while the final output was 3.08 — the SSE chain was destroying a working prediction.

3. **Successor tracking recorded a byte belonging to the context as its own successor.**
   This made `rec_branch3` score exactly zero. It is in fact the second-best statistic
   found.

Two further statistics (`fs_switch`, `bocpd_cp`) initially scored exactly zero due to
degenerate bin scaling — the values were pinned in a single bin. The tell was the
occupied-cell count in the screen output (32 cells where 500 were expected). Both
carried real signal once rescaled to log bins.

**Diagnostics worth keeping permanently:**

- a per-input standalone-cost readout, which localised bugs 1 and 2 in a single run
- the occupied-cell count in the screen output, which catches degenerate binning
- null-control statistics in every batch, which calibrate both the estimator and the
  wiring overhead

---

## 5. Caveats

**Gains will not transfer 1:1 to a frontier baseline.** These were measured against a
1.75 bpc model. fx2-cmix operates near 0.886 bpc on enwik9 and already exploits match
length and `c0` in its mixer selection. Scaling the +0.0171 gain by residual redundancy
gives a crude estimate near 0.009 bpc on enwik9 — roughly 1.1 MB, right at the 1% claim
threshold — and that estimate is soft in both directions.

`rec_branch3` and the dispersion family **as mixer selectors** are the components least
likely to already be present.

**Absolute bpc here is not competitive** and is not intended to be. Only the deltas are
the result.

**Screen figures in §2.1 are uncorrected for §3.6.** The over-crediting of
input-redundant statistics affects `kt_gap6`, `st_cnt6`, `st_shape6`, and the surprisal
family, all of which derive from state the mixer already sees. The wiring results are
unaffected.

---

## 6. Where the next gain is

The pattern across all 39 candidates: **every statistic that read off an existing slot
under-delivered relative to its screen score; the two winners are both external to the
slot values.**

The productive direction is therefore not more mathematics but more **independent
state** — quantities no current mixer input can reconstruct. Successor structure and
pre-mixing ensemble dispersion both qualify. Slot-derived and surprisal-derived
quantities mostly do not.

That points away from formula-hunting and toward document structure: position within an
article, article identity, cross-article similarity, section type. This is the region
starlit exploited to win in 2021 — by reordering enwik9 articles by similarity so the
online learner sees related content adjacently, with an unmodified cmix core. It remains
the cheapest percent anyone has won on this prize, and it is exactly the kind of state
that no bit-level statistic can reconstruct.

---

## 7. Reproduction

`cm2.cpp` — single file, no dependencies. Build with `g++ -O3 -march=native`.

```
./cm2 <file> <bytes> <collect> <gate_idx> <gate_mode>

  collect = 1     write joint histograms for all 39 statistics
  gate_mode       1 = SSE gate
                  2 = mixer gate
                  3 = 2-way composite gate
                  4 = 3-way composite gate
  env             BBITS   log2 buckets per model (default 21)
                  MIXLR   mixer learning rate (default 4)
                  GATE2   second statistic index (modes 3, 4)
                  GATE3   third statistic index (mode 4)
                  DIAG    per-input standalone cost readout
                  FS_ALPHA, BO_HAZ   Fixed-Share and BOCPD parameters
```

`cmi.py <histfile>` computes the bias-corrected ranking, the empirical null floor, and
the occupied-cell counts.

Runtimes on a single core: baseline 29 s / 10 MB, instrumented collection 55 s / 10 MB,
30 MB run 82 s.
