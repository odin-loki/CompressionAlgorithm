<!--
Strategic review of Cypha (github.com/odin-loki/Cypha), not hp.
Decisions that apply here landed in PLAN.md Track C.
Do not compare Cypha 2.664 bpc (n_eval=2000) to hp 8 MB / 100 MB SHA gates.
-->

# Cypha — Technical Review, Critique, and Feature Think Tank

**Author:** Odin Loch
**Reviewer notes prepared:** 2 September 2026
**Subject:** `github.com/odin-loki/Cypha` @ `b686658` (v2.4.0)
**Scope:** whole-framework review, deep critique of the compression path, forward feature programme

---

## 0. Method and scope

The repository was cloned at commit `b686658` and surveyed in full: 168,913 lines of C++ across `native/`, plus docs, benchmarks, fixtures, and the paper bundle. The `hp/` Hutter Prize working directory referenced in project notes is **not present in the public repo**, so the compression critique below targets the mixer that *is* pushed: `native/src/cyphalm/adaptive_predictor_mixer.cpp` and `native/src/cyphalm/arithmetic_coder.cpp`.

Where I make a claim about performance, I measured it. Two harnesses were built for this review:

- `mixdemo.cpp` — a faithful port of `AdaptivePredictorMixer` alongside a PAQ-geometry mixer using the **same expert set**, so that mixing geometry is isolated from model count.
- `se.cpp` — an empirical standard-error and selection-bias estimator driven by a real per-character surprisal distribution.

Both are reproduced in §6. Corpus: `bench/data/gutenberg/moby_dick.txt` + `bench/data/canterbury/lcet10.txt` + `plrabn12.txt`, i.e. data already in the repo.

---

## 1. Executive summary

Cypha is an unusually ambitious piece of solo engineering. It is a genuinely from-scratch C++23 ML runtime with a coherent theoretical spine, real CI, 220 registered CTest cases, REST and Qt front-ends, and a shipped release. Very few individuals produce anything at this scale and keep it building on two platforms.

That said, the review surfaced three problems that I think are currently load-bearing, in descending order of severity:

1. **The evaluation methodology cannot support the claims built on it.** Every headline BPC number in `bench/BASELINE_LOCK.json` — including the 2.664 in the README — is measured on `n_eval = 2000` characters. The 95% confidence interval on that measurement is **±0.103 bpc**. A 25-variant sweep selecting the best result on that eval set has an expected *spurious* improvement of **0.104 bpc**, which is the entire apparent margin of the cell-sweep winner. Some of the architecture may have been selected on noise.

2. **The mixer optimises the wrong loss function.** The Hedge score update is linear in probability while the codec pays logarithmic cost. Measured consequence: the *worst* expert in the ensemble captures 72% of the mixing weight. A one-line correction recovers 38.7%.

3. **The mixing geometry caps achievable compression.** Linear probability mixing is an arithmetic mean and can never be sharper than its sharpest component. This is the structural reason a mature CM design will beat this one regardless of model quality.

None of these are fatal. All three are fixable, and (2) is fixable this afternoon. The rest of this document works through the full picture and then proposes a forward programme.

---

## 2. What is genuinely strong

Being clear about this first, because the critique that follows is long.

**Scale and completion.** 168k lines, building green on Linux and Windows MSVC, with a release tagged and shipped. The distance between "research prototype" and "thing that builds on someone else's machine" is where most solo projects die. Cypha crossed it.

**Vendored dependency discipline.** Four third-party headers total — `httplib`, `nlohmann/json`, `numpy_rng`, `stb_image_write`. No PyTorch, no ONNX Runtime, no BLAS dependency chain. For a defence-adjacent product this is a real asset: the supply-chain attack surface is close to zero and the whole thing can be audited by one person in finite time. This is a selling point you are underusing.

**Test infrastructure exists.** 220 `add_test` registrations, 48 regression test sources, golden fixtures, parity checks against stored `.npz` artifacts, a REST schema contract test. Most people at this stage have none of this.

**Theoretical coherence.** The four-programme derivation in the README (AIXI/MDL prior, natural-gradient updates on the diagonal-Gaussian manifold, free-energy decomposition into `θ₀ ⊕ Δk`, Information Bottleneck encoder) is internally consistent and actually maps onto the code. `WorldPrior` and `ClassDifferential` exist as described. The theory is not decoration.

**The epistemic uncertainty machinery is a real differentiator.** NIG posteriors giving per-prediction epistemic and aleatoric variance is genuinely rare in compression work and in most deployed ML. §7 argues you are not exploiting it.

**The arithmetic coder is correct.** I read `ArithmeticEncoder`/`ArithmeticDecoder` closely. The carry handling via `pending_` is right, renormalisation is right, the CDF builder guarantees every symbol is encodable. It is a textbook-clean implementation. Nothing below is a criticism of the coder itself.

---

## 3. Critique

### 3.1 Measurement and evidence — the most important section

This is the finding I would act on first, because it potentially invalidates architectural decisions already made.

**The numbers.** Extracting every eval configuration from `bench/BASELINE_LOCK.json`:

| Result | BPC | `n_eval` | `n_train` |
|---|---|---|---|
| `d17_hybrid_baseline` (README headline) | 2.664 | **2000** | 300000 |
| `overnight_results` | 2.6643 | **2000** | 300000 |
| `cell_sweep_results` best variant H19 | 2.9214 | **2000** | 300000 |
| `cell_sweep_results` b2 baseline | 3.0730 | **2000** | 300000 |
| `math_integration_results` baseline | 2.8641 | **2000** | 300000 |
| `math_integration_results` with integration | 3.0730 | **2000** | 300000 |
| `rpsm_results` | 3.8490 | **2000** | 300000 |
| `native_d17_wikitext_overnight_smoke` | — | **64** | 500 |

**What that eval size actually buys you.** I measured the empirical per-character surprisal distribution from a real text model at a comparable operating point (mean 2.652 bpc — close enough to your 2.664 that the variance estimate transfers):

```
per-char surprisal: mean = 2.6521 bits, sd = 2.3540 bits, n = 60000

  n_eval = 64        SE = 0.2942   95% CI = ±0.5767 bpc
  n_eval = 2000      SE = 0.0526   95% CI = ±0.1032 bpc
  n_eval = 10000     SE = 0.0235   95% CI = ±0.0461 bpc
  n_eval = 100000    SE = 0.0074   95% CI = ±0.0146 bpc
```

So the README's headline is properly written as **2.664 ± 0.103 bpc**, and the CTest smoke number at `n_eval = 64` is **±0.577 bpc** — wide enough to be uninformative as a number, though still fine as a wiring check, which is what the JSON note correctly says it is.

**Selection bias is the real damage.** The cell sweep evaluated 25 variants and took the best. Monte Carlo over 200,000 trials, drawing K iid noise samples at SE = 0.0526 and taking the minimum:

```
  best-of-5  variants at n_eval=2000: expected spurious gain = 0.0611 bpc
  best-of-25 variants at n_eval=2000: expected spurious gain = 0.1035 bpc
```

The cell sweep's apparent margin — 3.073 baseline to 2.921 for H19 — is **0.152 bpc**. The expected gain from pure noise when picking best-of-25 is **0.104 bpc**. Roughly two thirds of that headline improvement is explicable as selection on noise. H19 may be a real winner; the data as collected cannot distinguish it from a lucky draw.

The `math_integration` ablation fares better: 2.864 vs 3.073 is 0.209 bpc, about 4.0 SE, and only two arms, so no meaningful selection bias. That result is probably real. Worth noting it says the math integration *hurt*, which is a credible and useful negative result.

**No seeds, no intervals, no repeats.** Nothing in `BASELINE_LOCK.json` records a seed count, a variance, or a confidence interval. Every number is a point estimate from a single run. For a project whose entire thesis is about information and uncertainty quantification — one that ships an `NIGField` and an `EpistemicThreshold` class — reporting bare point estimates is an unforced irony.

**Why this matters commercially, not just scientifically.** You sell to Five Eyes and NATO-aligned agencies. Evaluation rigour is exactly what a government technical assessor is trained to attack. A reviewer who opens `BASELINE_LOCK.json`, sees `n_eval: 2000`, and computes the CI in their head will discount everything else in the submission. Conversely, a repo that reports `2.664 ± 0.015 (n_eval = 100k, 5 seeds)` reads as *professionally competent* in a way that the current numbers do not, and it costs you one afternoon of compute.

**Fix:**
- Raise `n_eval` to ≥100k characters. At ±0.015 bpc you can actually resolve the effects you care about.
- Run ≥5 seeds. Report mean ± 95% CI everywhere, including in the README.
- Adopt a hard rule: **no BPC claim ships without an interval and a seed count.**
- Re-run the cell sweep at the new eval size before trusting any variant ranking derived from it. Apply a Bonferroni or Benjamini–Hochberg correction across variants.
- Split eval into a dev set (for sweeps) and a locked test set (touched once, at release). Right now the sweep and the headline appear to use the same data.

### 3.2 The compression path

Full experimental detail is in §6. Summary of findings.

#### 3.2.1 The mixer optimises the wrong loss

In `adaptive_predictor_mixer.cpp`:

```cpp
scores_[i] += cfg_.mixer_lr * (py - p_mix);
```

This is a Hedge-style update with **linear** reward. The codec pays **logarithmic** cost. Hedge with linear loss has good regret bounds — for linear loss. It has no useful guarantee for log loss, and in practice it systematically misranks experts.

The mechanism: an expert that fires at 0.85 confidence collects +0.85 when correct and loses only ~0.0006 when wrong, because linear loss barely punishes confident errors. Under log loss a confidently-wrong prediction costs ~10.7 bits and the expert would be annihilated. Under this rule, it thrives.

Instrumented run of your mixer as written, orders 0–3 plus match, 200KB:

```
expert    mean weight   % steps active   solo bpc
order0        0.004         100.0%          4.612
order1        0.250          99.3%          3.723
order2        0.024          93.1%          3.746
order3        0.004          69.9%          4.566
match         0.718          71.8%          6.578
```

**The worst expert in the ensemble holds 72% of the weight.** Overall: 5.411 bpc — worse than the order-0 entropy of English, which is the signature of an ensemble actively harmed by its own weighting.

This also explains `neural_weight_floor = 0.55` and the comment "warm-model safety: keep a floor so cold/noisy experts cannot dominate." That floor is a tourniquet applied to a wound the mixer is inflicting on itself. It works, in the sense that it stops the bleeding — at the cost of hard-capping how much the mixer can ever route around a bad neural prediction.

**The fix is one line.** For a linear mixture under softmax weights, the true gradient of log loss with respect to score `s_i` is `w_i · (p_i(y)/p_mix(y) − 1)`:

```cpp
const double pi = py / s;
scores_[i] += cfg_.mixer_lr * (pi / (p_mix + 1e-12) - 1.0);
```

The missing division by `p_mix` is the whole thing. Measured effect, same corpus, same experts, nothing else changed:

| | BPC | Δ |
|---|---|---|
| Repo rule `(p_i − p_mix)` | 5.411 | — |
| Corrected `(p_i/p_mix − 1)` | **3.316** | **−38.7%** |

Resulting weights become sane: `order0=0.021 order1=0.340 order2=0.427 order3=0.000 match=0.211`. Order-2 becomes the dominant expert, which is what you would expect on 200KB of English.

Once this lands, `neural_weight_floor` should be removable. Verify that empirically rather than assuming — but if the floor is still load-bearing after the fix, something else is wrong.

#### 3.2.2 Linear mixing caps you structurally

`mix_p_[k] += w * p_i[k]` is an arithmetic mean over expert distributions. **An arithmetic mean can never be sharper than its sharpest component.** If two independent models each assign p = 0.9 to the truth, linear mixing gives you 0.9. The correct combination is closer to 0.99.

This is the core insight PAQ is built on. Logistic mixing —

```
p = squash( Σ wᵢ · stretch(pᵢ) ),  stretch(p) = ln(p/(1−p))
```

— combines *evidence* rather than *opinions*, and can produce a mixture more confident than any member. Measured effect, identical experts (orders 0–3 + match), identical corpus:

| | BPC |
|---|---|
| Linear symbol mixing, corrected gradient | 3.316 |
| **Logistic binary mixing, same experts** | **2.574** |

A further **−22.4%** from geometry alone. No new information, no new models.

This requires binary decomposition: encode each byte as 8 binary decisions down a bit tree, use a binary arithmetic coder, mix in logit space. That is a real rewrite, but it also unlocks two things with no symbol-level analogue:

- **Context-selected weight sets** — a separate weight vector per (bit position × order-1 context) bucket instead of one global vector. Currently you have a single global weight vector shared across all 256 symbols and all contexts.
- **SSE / APM chains** — take the mixed probability, quantise `stretch(p)` into buckets crossed with a small context, interpolate a learned refinement. Reliably worth 1–3% in PAQ-family compressors. Your `use_bias_expert` is a static per-symbol bias vector, which is not the same thing and is why it "diluted the neural" as the comment records.

Adding the models a mature design would carry — orders 4/6/8/12/16/24, a word model, a two-stage APM — took it to **2.393 bpc** in my harness.

#### 3.2.3 `find_match` is quadratic and will not scale

```cpp
for (int len = maxl; len >= cfg_.match_min_len; --len)
    for (int i = 0; i + len < n; i++)
        for (int j = 0; j < len; j++) ...
```

Lengths 32 down to 3, over all 4096 history positions, with inner comparisons. That is up to ~10⁶ byte comparisons **per symbol**. It is also called two or three times per prediction, since `expert_active()` calls `find_match(nullptr)` and then `fill_match_log_probs()` calls it again.

Concretely: my 200KB run took minutes. 600KB exceeded a 10-minute execution limit and I could not obtain a clean scaling curve before hitting tool limits. At enwik9's 10⁹ bytes this is not slow, it is non-terminating. The Hutter Prize allows 100 hours on one core.

**Fix:** hash the last 6 bytes into a table of last-seen positions. Maintain a match pointer and a match length; extend on agreement, invalidate on mismatch. O(1) amortised. Feed the prediction through a StateMap keyed on `min(match_len, 31)` so the model learns its own reliability as a function of match length rather than using a hardcoded `peak = 0.85`.

The same pattern kills the kNN expert: `knn_nearest_dist2()` scans all 2048 stored entries × 32 dims, is called from `expert_active()` **and** from `fill_knn_log_probs()`, and `knn_features()` recomputes the query vector from scratch each time. That is ~200k redundant flops per symbol. Cache the query features once per prediction at minimum.

#### 3.2.4 Memory model will not fit enwik9

```cpp
std::vector<std::unordered_map<std::uint64_t, std::vector<float>>> tables_;
```

Each context stores a dense `vector<float>` of size V. At V = 256 that is **1 KB per context**, plus `unordered_map` node overhead of ~50 bytes. Order-3 over enwik9 yields tens of millions of distinct contexts. That is hundreds of gigabytes. The Hutter Prize limit is 10 GB.

**Fix:** direct-mapped hash tables of fixed size with 1-byte bit-history states, nibble-bucketed (hash once per nibble, index within a 16-slot cache line), with a checksum byte for collision detection and LRU-ish replacement within the bucket. This is what every CM compressor does and it is the difference between "fits in RAM" and "does not exist."

#### 3.2.5 Other compression-path issues

**History capped at 4096 symbols.** `history_.erase(history_.begin(), history_.end() - 2048)` — an O(n) memmove, and more importantly a 4KB window. enwik9's exploitable redundancy is measured in megabytes; article-level repetition is the single largest structure in the file. Use a ring buffer over the whole input.

**`max_order = 3`.** Nothing below roughly 2 bpc has ever been built on order-3 contexts alone.

**No word model.** For English text, a context keyed on the hash of the current partial word plus the previous word is among the two strongest single models available. Absent entirely.

**Pure frequency counting with no aging.** `counts[symbol] += 1.0f` is optimal for a stationary source. Text is not stationary. Use adaptive-rate counters — `p += (target − p)/(n+2)` with `n` capped — so recent evidence outweighs stale evidence.

**`min_context_count = 16` gates out exactly the models that carry information.** On a few hundred KB of text, an order-3 context appearing ≥16 times is rare — measured at 69.9% active, and given how few contexts reach the threshold, the high-order experts are silent when they would be most valuable. The right answer is not a count gate; it is to let the counter's own confidence and the mixer's context-selected weights handle low-count contexts.

**`cdf_from_log_probs` forces ≥1 count per symbol.** Harmless at V = 256 (256/2²⁰ ≈ 0.024% of mass). If you move to a BPE vocabulary of 32k it costs ~0.045 bits/token for nothing. Also, all leftover mass from the flooring pass is dumped on the mode, which is a small but free-to-fix bias.

### 3.3 Determinism and reproducibility — a correctness landmine

`predictive_codec.cpp` runs the encoder and decoder through `std::exp` / `std::log` on `double`, and calls `model.adapt_after_predict(nxt, opt.online_adapt_lr_scale)` **mid-stream on both sides**. The decoder must reproduce the encoder's probabilities bit-exactly or the archive is garbage from the first divergence onward.

Things that break this silently:

- Different libm implementations (glibc vs MSVC vs musl) — `exp` and `log` are not correctly-rounded and differ in the last ulp across platforms.
- `-ffast-math`, `-Ofast`, or MSVC `/fp:fast`.
- FMA contraction — GCC will fuse `a*b+c` into an FMA at `-O2`, changing rounding, and whether it does depends on target arch flags.
- x87 80-bit intermediates on 32-bit targets.
- Any change to summation order from vectorisation.

The current tests pass because compression and decompression run in the same process with the same binary. The failure mode is: you ship a self-extracting archive to Marcus Hutter, it was built with your compiler, and it does not decompress on his.

Your recorded instinct to go integer-only for the Hutter attempt is correct and should be treated as non-negotiable. Beyond that, add a CI job that compresses on one toolchain and decompresses on another, and a round-trip fuzz test over random and adversarial inputs. Right now there is no test I can find that would catch cross-platform divergence.

The comment in `compress_tokens` — "Checkpoints omit Adam moments; post-train warm Adam on the encoder would diverge from a freshly loaded decoder under online_adapt. Both sides start cold and stay locked." — shows you have already been bitten by exactly this class of bug once. That is the tip of it.

### 3.4 Architecture and complexity

**`cyphalm_model.hpp` includes 25 Cypha subsystem headers** before declaring anything: BPE tokenizer, alpha spectrum, CellAI SSM, char LSTM, compressive memory, context bank, DIF, EWC regulariser, embed table, GRIA low-rank, Hebbian stack, hierarchical SSM, n-gram fusion, algebraic fingerprint, AXIOM activation, CA state cell, GRIA gated mixture, MDL forget, NIG state cell, PGM cell, reversible SSM cell, selective SSM, view embedding, intelligence profiler, profile-guided loss, RPSM sequence layer, SOM discriminative feedback, GNG expert, GRIA controller.

`cyphalm_model.cpp` is **2,821 lines defining 65 methods on one class.** `CyphaLMConfig` has **148 configuration fields.**

This is a god object, and the coupling has a specific cost beyond aesthetics: **you cannot ablate.** With 148 interacting knobs and one class owning everything, you cannot cheaply answer "does the Hebbian stack earn its keep?" or "what does the CA state cell contribute?" And because §3.1 shows the eval harness cannot resolve differences below ~0.1 bpc, you also could not measure the answer if you tried. The two problems compound: too many knobs to test, and a test that is too noisy to distinguish them.

I would guess — and it is a guess, which is the point — that a substantial fraction of those 25 subsystems contribute nothing measurable. Some may be actively harmful, as the `math_integration` result (2.864 → 3.073, a real regression) already demonstrates for one of them.

**Related structural issues:**

- `bench_domains.cpp` at 10,466 lines and `shell_main.cpp` at 8,014 lines are both well past the point where anyone can hold them in working memory.
- `bessel_table_data.cpp` at 49,167 lines is a generated table checked into source. Fine, but it should be generated at build time or stored as data, not compiled — it is inflating your build times and your LOC count in a way that flatters the latter and hurts the former.
- Everything is one CMake target family. Someone who wants the compressor gets the Qt shell, the REST server, the GDELT forecasting stream, and the SQL layer.

**Fix, in order:**
1. Split into independent targets with explicit interfaces: `cypha-core`, `cypha-lm`, `cypha-compress`, `cypha-forecast`, `cypha-serve`. The compressor in particular should build standalone with zero dependencies — that is a prerequisite for a Hutter submission anyway, since submission size counts.
2. Tier the config: a small stable surface, everything else behind an `experimental` struct that is explicitly documented as unvalidated.
3. Build an ablation registry (see §7.2) and **delete anything that cannot demonstrate it earns its cost.** This is the highest-leverage refactor available to you and it will feel bad, which is how you know it is the right one.

### 3.5 Claims versus evidence

The README is doing a lot of work that the benchmarks do not support, and I think this is costing you credibility with exactly the audience you are targeting.

**"2.664 BPC" stated without an interval** — covered above. Should be `2.664 ± 0.103 (n_eval=2000, 1 seed)` as it stands, or re-measured.

**No external baselines anywhere in the repo.** There is no comparison against `xz`, `zpaq`, `ppmd`, `lpaq1`, `cmix`, or a small character-level LSTM. Without those, "2.664 BPC" is unanchored — a reader cannot tell whether it is good. For calibration: on WikiText-2 character-level, `xz -9` lands around 2.2–2.4 bpc, and a well-tuned lpaq-class compressor is comfortably under 2.0. This means the headline number for a custom-architecture neural LM is **currently worse than a general-purpose compressor from 2009.** That is a fixable engineering gap, not an indictment of the ideas — but stating the number without the comparison invites a reader to discover it themselves, which is worse than stating it yourself.

**"Built from first principles (AIXI/MDL, information geometry, free-energy world prior…)"** — the derivation is real, but the phrasing implies the theory *produced* the performance. The evidence does not show that. Where the theory has been ablated (`math_integration`), it made things worse. I would recommend describing the theory as the design rationale and reporting separately, and honestly, what it buys. The `math_integration` regression is a genuinely interesting negative result and it is currently buried in a JSON file.

**The `intelligence/` module** — `IntelligenceProfiler`, `LandscapeSystemClass::{SimpleFfn, LargeTransformer, CyphaAugmented}`, `EpistemicThreshold`, `CriticalityVector`, `causal_graph`. The naming makes claims that the mechanism does not obviously cash out. `LandscapeSystemClass` enumerating `CyphaAugmented` alongside `LargeTransformer` as reference classes, with profiles "estimated from Paper III §3", is the kind of thing a sceptical assessor will read as self-grading. If the profiler measures something real, name it for what it measures. If it is exploratory, mark it exploratory.

**The GRIA α ≈ 0.5 "universal order parameter."** `gria_gated_blend_logit` implements this as an 18-line header with three hardcoded constants (`kAlphaCenter = 0.5`, `kMeanScale = 1.2`, `kDeltaScale = 0.6`). For a claim positioned as a Grand Unified Law spanning compression, neural networks, cryptography, and circuit optimisation, the implementation is three magic numbers and an addition. Either the constants should be learned and shown to converge near their theoretical values — which would be a genuinely strong result — or the claim should be scoped down. §7.3 proposes a way to make α actually earn its keep.

### 3.6 Where 1.5 bpc actually sits

Current landscape, verified against the Large Text Compression Benchmark and Hutter Prize records:

- **Hutter Prize record:** fx2-cmix (Kaido Orav & Byron Knoll, Sept 2024) — 110,793,128 bytes on enwik9 ≈ **0.886 bpc**.
- **cmix-obias** (David Freelan, derived from cmix-lex, July 2026) — 108,009,834 bytes ≈ **0.864 bpc**.
- **altxs 1.0.0** (Conrad Lippert-Zajaczkowski, Aug 2026) — 96,996,198 bytes ≈ **0.776 bpc**. Requires a CUDA GPU, 63 hours to compress and 330 hours to decompress, so LTCB-only, not Hutter-eligible.
- **Independent entrant, Feb 2026** — 1.588 bpc = 189.3 MB via sparse contexts + match + Kneser-Ney order-6, self-reported as 1.79× fx2-cmix.

Your 1.5 bpc puts you **marginally ahead of that last data point**, which is a serious hand-built CM without dictionary preprocessing. That is a real result and worth saying plainly. It also puts you about **41% away from the prize record**, and that last 41% is mostly unglamorous engineering: dictionary preprocessing (~10% on enwik alone), 20+ context models, SSE chains, article reordering, and relentless tuning.

**A candid word on ROI.** The Hutter Prize pays €5,000 per 1% improvement. Beating the record means reaching ~0.877 bpc. From 1.5 that is a 41% reduction, and each subsequent 1% gets harder — Hutter himself projects roughly 1% per year industry-wide. Realistically this is a multi-year project for a first payout of €5,000. Treat the prize as a forcing function for engineering quality and a credential, not as a revenue line. The techniques transfer directly to things that *are* commercially interesting (§7.5), and that is the actual return.

---

## 4. Cross-cutting: what this means for the defence-sales strategy

You are cold-submitting to government and defence agencies and pointing them at this GitHub. That changes what "good" means for this repo in ways worth stating explicitly.

**What helps you:**
- Zero-dependency C++ with four vendored headers. Auditable by one cleared person. Say this louder and put it near the top of the README.
- Deterministic, integer-only compute paths. A defence buyer cares about bit-exact reproducibility far more than a research buyer does.
- Real CI on two platforms.

**What hurts you:**
- `n_eval: 2000` with no confidence intervals. This is the single thing most likely to get you dismissed by a technical assessor, because it is the exact failure mode they are trained to look for.
- No external baselines. An assessor's first question is "compared to what?"
- Claims phrased more strongly than the evidence supports. In this domain, one over-claim discounts everything else in the package.
- 148 config knobs with no ablation evidence. Reads as unvalidated surface area.
- Absolute Windows paths embedded in shipped artifacts (`bench/BASELINE_LOCK.json` contains `C:\Users\odinl\OneDrive\Desktop\Cypha\...`). Small, but it signals the artifacts were not produced by a reproducible pipeline. Strip these.

The rigour fixes in §3.1 are not academic hygiene for you — they are directly commercial.

---

## 5. Prioritised remediation list

Ordered by measured-benefit per hour of work.

| # | Action | Effort | Expected effect | Confidence |
|---|---|---|---|---|
| 1 | One-line gradient fix in `observe()` | 15 min | −38.7% bpc on mixer path | Measured |
| 2 | Raise `n_eval` to 100k, add 5 seeds, report CIs | 1 day, mostly compute | Claims become defensible | Certain |
| 3 | Remove `neural_weight_floor` after (1); verify | 30 min | Removes a hard cap | High |
| 4 | Replace `find_match` with hash-pointer match model | 1 day | Correctness at scale, large speedup | Certain |
| 5 | Strip absolute Windows paths from artifacts | 15 min | Credibility | Certain |
| 6 | Add xz / zpaq / lpaq / ppmd baselines to bench | 1 day | Anchors every claim | Certain |
| 7 | Binary decomposition + logistic mixing | 1–2 weeks | −22.4% beyond (1) | Measured |
| 8 | Context-selected mixer weight sets | 2 days | 3–8% typical | High |
| 9 | Two-stage SSE/APM chain | 2 days | 1–3% typical | High |
| 10 | Orders 4/6/8/12/16/24 + word model | 3 days | Large; cheap once (7) exists | Measured |
| 11 | Fixed-budget nibble-bucketed hash tables | 1 week | Makes enwik9 possible at all | Certain |
| 12 | Integer-only end-to-end + cross-toolchain CI | 1 week | Prevents shipping a broken archive | Certain |
| 13 | Ablation registry; delete what does not earn its keep | 2 weeks | Large complexity reduction | High |
| 14 | Split into independent CMake targets | 1 week | Enables standalone compressor | Certain |
| 15 | Dictionary preprocessing for enwik | 2 weeks | ~10% on enwik specifically | High |
| 16 | Re-run cell sweep at proper eval size | 1 day compute | May invalidate H19 | — |

Items 1–6 are roughly three days of work and address the majority of the credibility gap.

---

## 6. The experiment, in full

### 6.1 Design

Three arms, plus a variant, all on the same corpus (Moby Dick + lcet10 + plrabn12, from `bench/data/`), all reporting ideal code length `−log₂ p(y)` averaged per byte. Real arithmetic coder output lands within a few bytes of this.

- **Arm A** — faithful port of `AdaptivePredictorMixer`: symbol-level 256-way, linear probability mixing, softmax-over-Hedge-scores weighting, experts = Laplace n-grams order 0–3 + the peaked match expert. The neural slot is absent (your LM is not loadable in a standalone harness), so absolute numbers run higher than yours; the *relative deltas* are the finding, since the expert set is held fixed across arms.
- **Arm A2** — identical to A, with only the score update changed to the correct log-loss gradient.
- **Arm B** — same experts, same information, PAQ geometry: binary decomposition, logistic mixing, adaptive-rate counters, hash-pointer match model.
- **Arm C** — Arm B plus orders 4/6/8/12/16/24, a word model, and a two-stage APM.

### 6.2 Results

At 200,000 bytes:

```
ARM A   Cypha linear symbol mix, orders 0-3 + match      5.4114 bpc
ARM A2  same, correct log-loss gradient (one line)       3.3164 bpc   -38.7%
ARM B   logistic binary mix, identical experts           2.5743 bpc   -22.4% vs A2
ARM C   + orders 4-24, word model, 2-stage SSE           2.3934 bpc    -7.0% vs B
```

At 300,000 bytes (Arm A omitted — too slow, see §3.2.3):

```
ARM B   2.5056 bpc
ARM C   2.3169 bpc
```

Expert diagnostics for Arm A at 200KB:

```
expert    mean weight   % steps active   solo bpc
order0        0.004         100.0%          4.612
order1        0.250          99.3%          3.723
order2        0.024          93.1%          3.746
order3        0.004          69.9%          4.566
match         0.718          71.8%          6.578
```

Expert weights for Arm A2 after the gradient fix:

```
order0=0.021  order1=0.340  order2=0.427  order3=0.000  match=0.211
```

### 6.3 Standard error and selection bias

Driven by the empirical per-character surprisal distribution from Arm C (n = 60,000, mean 2.652 bpc, sd 2.354 bits):

```
  n_eval = 64        SE = 0.2942   95% CI = ±0.5767 bpc
  n_eval = 2000      SE = 0.0526   95% CI = ±0.1032 bpc
  n_eval = 10000     SE = 0.0235   95% CI = ±0.0461 bpc
  n_eval = 100000    SE = 0.0074   95% CI = ±0.0146 bpc

  best-of-5  variants at n_eval=2000: expected spurious gain = 0.0611 bpc
  best-of-25 variants at n_eval=2000: expected spurious gain = 0.1035 bpc
```

### 6.4 Caveats on the experiment

- Arm A has no neural expert. Absolute BPC is therefore higher than your production figures. The comparison is valid because the expert set is identical across arms — mixing geometry is the only variable.
- Corpus is ~200–300KB of literary English, not WikiText-2 or enwik. Relative effects of this magnitude are robust across text corpora, but re-run on your own data before committing to the numbers.
- Arm B/C use float mixer weights for clarity. Production requires integer arithmetic (§3.3).
- Arm C's hash tables are direct-mapped without checksums, so it carries some collision noise. A proper implementation would do slightly better.

Both harnesses (`mixdemo.cpp`, `se.cpp`) are self-contained single files and can be regenerated on request.

---

## 7. Feature think tank

### 7.1 Compression core — the road to SOTA

**Tier 1: table stakes for any serious CM.**

- **Binary decomposition + logistic mixing.** Non-negotiable. Everything else in this section assumes it.
- **Two-layer mixer.** PAQ8 runs a bank of mixers whose outputs are themselves mixed by a second-stage mixer. Worth several percent over a single layer.
- **Context-selected weight sets.** Select the weight vector by (bit position, order-1 byte, match state). Cheap, large effect.
- **SSE/APM chain.** Two or three stages, each keyed on a different small context, outputs averaged with learned weights.
- **Bit-history state machine counters.** Replace frequency counts with 1-byte state machine states (à la PAQ's `nex()` table) feeding a StateMap. Handles nonstationarity and deterministic contexts far better than counts.
- **Fixed-budget nibble-bucketed hash tables** with checksums. Prerequisite for enwik9.
- **Match model family** — not one match model but several, at different minimum orders, each with its own StateMap keyed on match length.
- **Word model, sparse contexts, skip contexts, indirect contexts.** Indirect contexts (context → bit history → StateMap) are one of the cheapest large wins available.
- **Run model and record model.** The record model (detecting fixed-stride table structure) is disproportionately effective on Wikipedia XML.

**Tier 2: enwik-specific.**

- **Dictionary preprocessing.** The phda9/cmix-family transform replaces common words with short codes before compression. Worth ~10% on enwik alone. This is the single largest remaining lever after the core rewrite.
- **Capitalisation modelling.** Transform `The` → `<cap>the` so the word model sees one token instead of two.
- **XML structure transform.** enwik9 is Wikipedia XML; tag structure is highly predictable and should be modelled separately from prose.
- **Article reordering/clustering.** fx2-cmix sorts articles by similarity so that related content is adjacent and the match models fire more often. This is where a chunk of recent record progress came from, and it plays directly to Cypha's existing similarity/kNN machinery.

**Tier 3: engineering.**

- SIMD (AVX2/AVX-512) mixer dot products and counter updates. The mixer is the hot loop; 4–8× is available.
- Memory-mapped model tables to control resident set under the 10GB limit.
- Integer-only arithmetic throughout, with a cross-toolchain round-trip CI job.
- A profiling harness that reports bits-per-model-per-second so you can make time/ratio tradeoffs quantitatively rather than by feel.

### 7.2 Framework rigour — the "measure or delete" programme

This is the feature I would build first if I were you, because everything else depends on it.

**An ablation registry.** A declarative table mapping every config flag and subsystem to: (a) the last measured ablation result, (b) the eval size and seed count, (c) the date. CI fails if a flag defaults to `true` and has no ablation newer than N months. Anything that cannot show a measured benefit gets deleted.

The effect on a 148-knob config surface will be dramatic. My expectation is you delete 40–60% of the experimental surface and the framework gets both faster and better.

**A proper eval library.** `cypha::eval` providing: seeded runs, bootstrap confidence intervals, paired comparison tests (the same eval sequence across two variants is far more powerful than independent runs — use a paired test and you can resolve smaller effects with the same compute), multiple-comparison correction for sweeps, and dev/test split enforcement.

**External baselines vendored into `bench/`.** xz, zpaq, ppmd, lpaq1, and a small char-LSTM reference. Every BPC claim reports alongside them. This costs a day and permanently anchors your numbers.

**A reproducibility bundle.** One command that regenerates every number in the README from scratch, with a manifest of toolchain versions and hashes. Defence buyers ask for this; almost nobody has it; having it is a differentiator.

**Round-trip fuzzing.** Property test: for random and adversarial inputs, `decompress(compress(x)) == x`, run under ASan/UBSan, across two toolchains.

### 7.3 Making GRIA α earn its keep

Currently α appears as three hardcoded constants in an 18-line header. Three ways to make it do real work, in ascending order of ambition:

**(a) α as mixer weight-set selector.** In a context-mixing compressor, the mixer selects its weight vector by some context. Quantised α — a measure of how ordered vs chaotic the local sequence is — is a *theoretically motivated* selector, and it is cheap. This is the single most natural place for your thesis to pay rent: if α is genuinely a universal order parameter, then routing mixer weights by α should measurably beat routing by an arbitrary hash. That is a clean, falsifiable, publishable experiment, and either outcome is informative.

**(b) α as an SSE context.** Same idea one stage later. Bucket the APM by quantised α alongside the usual contexts.

**(c) Learn the constants.** Make `kAlphaCenter`, `kMeanScale`, `kDeltaScale` learnable and show they converge near 0.5 / their theoretical values across different corpora and model families. If they do, that is a genuinely strong empirical result supporting the Grand Unified Law claim, and far more persuasive than asserting it. If they do not, you have learned something important and cheaply.

I would rank (a) as the highest-value experiment in this entire document from a research standpoint, because it converts a claim into a measurement.

### 7.4 Using the uncertainty machinery you already have

Cypha computes per-prediction epistemic and aleatoric variance via NIG posteriors. Almost no compressor has this. Ideas:

- **Epistemic variance as a mixer input.** Feed `stretch(f(epistemic_var))` in as an additional mixer input so the mixer can learn to discount the neural expert exactly when the neural expert is uncertain. This is a principled replacement for `neural_weight_floor` and it is a genuine differentiator — most CM designs have no calibrated confidence signal at all.
- **Uncertainty-gated compute.** Run the expensive neural model only when the cheap CM ensemble is uncertain. On text, the CM is confident on the large majority of characters. This could buy a large speedup at near-zero ratio cost, which matters enormously under a 100-hour budget.
- **Proper Bayesian model averaging.** Your NIG machinery is genuinely suited to maintaining a posterior over expert reliability rather than a Hedge score. This is a more defensible version of the mixer and it is closer to your stated theoretical commitments than the current heuristic.

### 7.5 Commercial and defence-adjacent directions

These matter more than the prize.

- **Compression as anomaly detection.** A well-calibrated CM gives you a surprisal score per byte for free. High surprisal = anomalous. This is a direct product for network telemetry, log monitoring, and RF/EW signal streams — all areas you already work in. It requires no new modelling, only a different output path, and it is far easier to sell than "we compress Wikipedia well."
- **Normalised Compression Distance classification.** `NCD(x,y) = (C(xy) − min(C(x),C(y))) / max(C(x),C(y))` gives a parameter-free similarity metric. Useful for malware family clustering, protocol identification, and document deduplication. Cheap to build on top of what you have.
- **Domain-specialised compressors.** A CM tuned for a specific telemetry format will beat general-purpose compressors substantially. Bandwidth-constrained links (tactical radio, satellite, UAV downlink) are a real market and the constraint is real.
- **Embedded/streaming profile.** A fixed-memory, integer-only, single-pass variant with bounded latency. This is what actually deploys on a platform, and it is a natural fallout of the Hutter work.
- **Reproducible auditable build as a product claim.** Zero dependencies, deterministic output, bit-identical across toolchains. Lead with this.

### 7.6 Speculative / research

- **Two-pass compression** where pass one collects global statistics (article clustering, dictionary derivation) and pass two uses them. Legal under Hutter rules provided the statistics are derivable by the decompressor or counted in the submission size.
- **Online distillation** — train a small fast model to imitate the slow ensemble, run the fast model, fall back to the ensemble on disagreement.
- **Cross-domain α validation.** If α is universal, measure it on RF captures, network traces, and executable binaries, and check whether the α ≈ 0.5 critical point predicts compressibility across all of them. This is the strongest form of the Grand Unified Law claim and it is testable with data you can obtain.
- **Compression-based model evaluation.** Use bits-per-character on held-out data as the *only* metric for any Cypha sequence model. It is the one metric that cannot be gamed and it aligns with your AIXI/MDL framing.

---

## 8. Open questions I could not answer from the repo

1. What corpus and configuration produced the 1.5 bpc figure? Without that, I cannot tell whether it is comparable to the 2.664 lock or to enwik.
2. Does `hp/` already implement binary decomposition and logistic mixing? If so, most of §3.2 applies only to the pushed mixer and the priority list changes substantially.
3. Has any ablation been run on the 25 subsystems in `cyphalm_model.hpp`, at any eval size?
4. Is there any cross-machine decompression test, anywhere?
5. What is the current memory ceiling of the `hp/` model tables on a full enwik9 pass?

---

## 9. Closing assessment

The engineering capacity on display here is real and rare. The theoretical work is coherent and, in places, genuinely interesting. The gap between where Cypha is and where it claims to be is almost entirely a **measurement and validation gap**, not a capability gap — and that is the good kind of gap, because it is cheap to close.

If I could persuade you to do only three things:

1. Apply the one-line gradient fix and measure it. Fifteen minutes for a 38.7% improvement on the mixer path.
2. Raise `n_eval` to 100k with five seeds and put confidence intervals on every claim in the README. One day of compute for a permanent step-change in how the project reads to a technical assessor.
3. Build the ablation registry and delete what cannot justify itself. Two weeks that will make everything after it faster.

The compression work is worth continuing. Not primarily for the prize — the ROI there is poor and honest arithmetic says so — but because context mixing done properly is a transferable capability that maps directly onto anomaly detection, similarity metrics, and bandwidth-constrained links, which are the things you can actually sell.

---

*Prepared under the byline Odin Loch. All performance figures in §6 were measured on the corpus shipped in `bench/data/`; harness sources available on request.*
