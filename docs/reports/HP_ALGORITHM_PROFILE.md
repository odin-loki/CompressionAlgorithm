# HP Algorithm Profile

**Subject:** `hp/` in [CompressionAlgorithm](https://github.com/odin-loki/CompressionAlgorithm) — integer-exact context-mixing compressor (~1.610 BPC on 8 MiB enwik8 at v82).

**Scope:** Architecture (experts → mixer → APM → arithmetic coder), profiling hooks, time/RAM breakdown, v82 reproduction, Cypha integration footguns.

**Measurement status (2026-09-19):** Live runs on this agent VM (15 GB RAM, 4 cores, Linux) with downloaded `data/enwik8.8mb` (SHA256 `09f6dd7241a8ae21…`, matches RECORD). `SLOT_MAX=35` champ encodes were **not** run here (RECORD/H34: peaks ~13–27 GB RSS; OOM risk on ≤15 GB). Those rows cite RECORD/PLAN only.

---

## 1. Architecture and file map

### 1.1 Per-bit pipeline

Each input byte is coded MSB-first as eight binary decisions. Encode and decode share the same path (`hp/src/main.cpp` → `encode_buffer()`).

```
┌──────────────────────────────────────────────────────────────────────────┐
│ EXPERTS  (hp/include/hp/predictor.hpp :: Predictor::predict)             │
│   ContextModel chain (o1–o6, word, sp*, col, tag, wbi, brk, pat, …)      │
│   + domain CMs enabled via HP_*_MOD compile flags                       │
│   + MatchModel×K, word-match, skip/lzp/dmc/sr, Hebbian, DiscoveryPool   │
│   + optional CTW / hedge inputs                                            │
│   → each expert: stretched logit via stretch(squash(p))                  │
└───────────────────────────────┬──────────────────────────────────────────┘
                                │ MixerNet::add(stretched)
                                ▼
┌──────────────────────────────────────────────────────────────────────────┐
│ MIXER  (hp/include/hp/mixer.hpp :: MixerNet)                             │
│   Layer 1: k gated dot-products W_j[ctx_j]·x → squash → pr_[j]           │
│   Layer 2: v[ctx2]·dot_ → squash → final probability                   │
│   Gates: c0, GRIA α-bucket, prev byte, match-len, entropy, hebb, …       │
│   Optional Hedge-L1 blend (hp/include/hp/hedge.hpp)                      │
└───────────────────────────────┬──────────────────────────────────────────┘
                                │ mixed_p_
                                ▼
┌──────────────────────────────────────────────────────────────────────────┐
│ APM ×3  (hp/include/hp/mixer.hpp :: APM)                                 │
│   apm_c0_.refine(pr, c0)                                                 │
│   apm_lex_.refine(pr, prev_byte*256 + c0)                                │
│   apm_gria_.refine(pr, gria_bucket*256 + c0)                             │
│   pr_final = (HP_W0·pr + HP_WA·a + HP_WB·b + HP_WG·g) >> 3              │
└───────────────────────────────┬──────────────────────────────────────────┘
                                │ p << 4  (12-bit effective prob)
                                ▼
┌──────────────────────────────────────────────────────────────────────────┐
│ CODER  (hp/include/hp/coder.hpp :: Encoder/Decoder)                    │
│   32-bit interval split, renormalize, emit/consume bytes                 │
│   One bit per step; no float softmax / full-alphabet CDF                 │
└──────────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
                         Predictor::update(bit)  (all tables + mixer + APM)
```

Canonical comment block: `hp/include/hp/predictor.hpp` lines 5–13. `predict()` expert loop starts ~L1393; mixer gates ~L1560–L1624; APM blend ~L1635–L1639; coder call in `main.cpp` ~L132–L136.

### 1.2 Source file map

| Stage | Primary files | Key types / symbols |
|-------|---------------|---------------------|
| **CLI & archive** | `hp/src/main.cpp` | `compress()`, `decompress()`, CYHP magic, `kVersion=4`, `--profile` |
| **Orchestration** | `hp/include/hp/predictor.hpp` | `struct Config`, `class Predictor`, `predict()`, `update()`, `slot_bits()` |
| **Compile flags** | `hp/include/hp/features.hpp` | 568 `HP_*` switches; `HP_SLOT_MAX` (default 28; champ 35) |
| **Experts — context** | `hp/include/hp/models.hpp`, `statemap.hpp` | `ContextModel` (indirect StateMap + optional PY direct) |
| **Experts — match** | `hp/include/hp/models.hpp`, `wordmatch.hpp` | `MatchModel`, word-keyed matches |
| **Experts — discovery** | `hp/include/hp/discover.hpp` | `DiscoveryPool` (online MDL masks) |
| **Experts — wiki/domain** | `wiki.hpp`, `bracket.hpp`, `numeric.hpp`, `sentmem.hpp`, … | Dozens of `HP_*_MOD` axis models |
| **GRIA / gates** | `hp/include/hp/gria.hpp`, `stat_gates.hpp` | `GriaGate`, `argmax_bin`, `agree_bin`, … |
| **Mixer weights** | `hp/include/hp/mixer_weights.hpp`, `simd_dot.hpp` | Q16 packed weights, SSE dot products |
| **APM** | `hp/include/hp/mixer.hpp` | `APM::refine()`, `APM::update()` — 33-entry SSE tables per context |
| **Coder** | `hp/include/hp/coder.hpp` | `split()`, `Encoder`, `Decoder` |
| **Math** | `hp/include/hp/int_math.hpp` | `squash`/`stretch`, `log2_q16` — no float on codec path |
| **Redundancy profiler** | `hp/include/hp/profile.hpp` | `Profiler::account()`, `report()` |
| **Expert dump (tools)** | `hp/tools/dump_experts.cpp`, `dump_profile.cpp` | Side-file diagnostics; not linked into codec |
| **Rank analysis** | `hp/tools/mp_rank.py`, `rank.cpp` | Participation ratio, entropy effective rank |
| **Wall-time / RSS** | `hp/tools/bench.cpp`, `hp/tools/hp_harness.sh` | In-process encode timing; `/proc/$pid` VmHWM polling |
| **Champ flag bundle** | `hp/tools/v78_flags.ps1` | Canonical `-DHP_*=…` list (name is historical; file tracks current champ) |

### 1.3 Expert inventory (v82-era champ build)

Expert count is **compile-time derived** (`kNumExperts` in `predictor.hpp`). With v82 flag set (see §4) and `HP_WORDLEN_MOD=1`:

- **114 experts** counted by `dump_experts` on this build (80k bit-records, stride 37).
- RECORD v57 reference: **77 experts** (smaller leftover set).

Expert categories in `predict()`: context chain (11+ models × 2 outputs if `HP_PY_EXPERT`), matches, sparse UTF-8 match, skip/lzp/dmc/sr mods, word-matches, Hebbian, discovery pool (12×2 default), domain `HP_*_MOD` context models, hedge inputs.

Table sizing: `--mem N` → `Config.table_bits` (default 22, header-clamped 16–28). Per-model caps via `slot_bits(base, delta)` and **`HP_SLOT_MAX`** (`features.hpp`; champ overrides to 35 in `v78_flags.ps1`).

---

## 2. Profiling: `--profile`, `dump_experts`, timing

### 2.1 `--profile` (bit-cost / redundancy decomposition)

**Not wall-clock CPU profiling.** `--profile` enables `hp::Profiler` (`profile.hpp`) on the shared encode path (`main.cpp` L125–144, L230–231, L249, L384).

CTW-style decomposition per bit (integer Q16 bit-costs):

| Report row | Meaning | Fix lever |
|------------|---------|-----------|
| `total spent` | Actual coded bits (final `pr_final`) | — |
| `best-expert` | Oracle single-expert hindsight minimum | Expert quality |
| `after mixer` | Cost at mixer output (pre-APM) | Mixing, gates, more experts |
| `sparse ctx` / `dense ctx` | Bit mass where contexts are sparse vs dense | Table size, pooling |
| **model redundancy** | `after mixer − best-expert` | Mixer / gates / expert set |
| **coding redundancy** | `total − after mixer` | APM + 12-bit quantisation + coder |
| **parameter share** | `sparse / (sparse+dense)` | Indirect estimation, `HP_SLOT_*` |

**Enable:**

```bash
hp/build/hp.exe c --mem 22 --profile data/enwik8.8mb hp/build/out.hp
```

Profile prints to **stderr** after the `bpc` line. Archive still written to `<out>`.

**There is no built-in per-stage CPU timer** (experts vs mixer vs APM vs coder). “Stage” in code refers to the **three APM refine tables**, not CPU stages.

### 2.2 `dump_experts` / `dump_profile` / `mp_rank.py`

Build via `make -C hp` (or champ `g++` line with same `-D` flags).

```bash
# Expert opinions + true bit (int16 LE, stride samples bit positions)
hp/build/dump_experts data/enwik8.8mb /tmp/e.i16 37 400000 22
#                                                      ^stride ^max records ^mem

python3 hp/tools/mp_rank.py /tmp/e.i16 <n_experts>
```

Defaults (`dump_experts.cpp` L26–30): stride **37**, maxrec **400000**, mem **22**.

`dump_profile.cpp` adds layer-1 mixer dots + wiki/mlen/entropy metadata + JSON sidecar (same champ flags required).

**Do not** point `dump_experts` at Cypha’s mixer without per-expert accessors (UPGRADES.md §5.1).

### 2.3 Wall time and peak RSS

| Tool | Measures | Notes |
|------|----------|-------|
| `hp/tools/bench.cpp` | `input_bytes coded_bytes peak_rss_kb encode_ms` | Coder body only (no CYHP header); uses `VmHWM` |
| `hp/tools/hp_harness.sh` | `hp_run` → `bytes rss_kb ms` | Polls child `/proc/$pid/status`; safe parallel temps |
| `main.cpp` progress | `\r<N> MB` on stderr | Coarse throughput indicator |

Harness calibration (RECORD H34, `hp_harness.sh` L9–14):

| Screen | Wall | RSS | vs 8 MiB gate |
|--------|------|-----|---------------|
| `enwik8.2mb`, `SLOT_MAX=24` | 92 s | 1.5 GB | Spearman +0.84, 13/13 signs |
| `enwik8.8mb`, `SLOT_MAX=24` | 500 s | 1.6 GB | Pearson +0.96 vs `SLOT_MAX=35` |
| `enwik8.8mb`, `SLOT_MAX=35` | ~1676 s (v85) | ~13–27 GB peak | Champ accept gate; needs ≥16 GB RAM |

**Recommended harness workflow:**

```bash
source hp/tools/hp_harness.sh
hp_build v82_s24 -DHP_SLOT_MAX=24 -DHP_WORDLEN_MOD=1   # plus other champ -D from v78_flags
hp_run v82_s24 data/enwik8.8mb 22
# → "1685481 1579008 502370"  (example: bytes rss_kb ms)
```

---

## 3. Where time and RAM go

### 3.1 Measured on this VM (2026-09-19)

**Build:** v82-era flags = `v78_flags.ps1` **minus** post-v82 accepts (`HP_LR1_SCALE`, `HP_REFGROUP_MOD`, `HP_SENTPOS_MOD`, `HP_STATETRANS_MOD`, `HP_CAPPARA_MOD`, `HP_WIKIBOLD_MOD`). `HP_WORDLEN_MOD=1` retained. **`HP_SLOT_MAX=24`** (screen config; fits 15 GB VM).

**Corpus:** `data/enwik8.8mb` (8,388,608 B; SHA256 matches RECORD v82 decode check).

#### Compression size & wall time

| Metric | Measured (s24) | RECORD v82 (`SLOT_MAX=35`) |
|--------|----------------|----------------------------|
| Archive bytes | **1,685,481** | **1,689,157** |
| BPC (header in numerator) | **1.607** | **1.610** |
| Δ vs v82 champ | +3,676 B (+903 B class per H40 s24 screen) | — |
| Encode wall (`bench`) | **502 s** (~8.4 min) | *(unmeasured here)* |
| Peak RSS (`bench`) | **1.54 GB** (1,579,008 KiB) | ~15–27 GB per PLAN/RECORD |

2 MiB screen (same build): **440,523 B** archive, **125 s**, **1.53 GB** RSS — consistent with RECORD H40 baseline ~439,496 B at v85.

#### `--profile` redundancy (8 MiB, s24) — **MEASURED**

```
--- redundancy profile (67108864 bits over 8388608 B) ---
  total spent       1685533 B   1.607 bpc
    best-expert      185598 B   0.177 bpc
    after mixer     1724512 B   1.644 bpc
    sparse ctx       802030 B   0.764 bpc
    dense ctx        883502 B   0.842 bpc
  model redundancy (mixer vs best expert): +1538914 B
  coding redundancy (APM+coder vs mixer):  -38979 B
  parameter share (sparse contexts):       47%
```

**Interpretation:**

- **~91% of the “gap” to oracle** is **model redundancy** (mixer vs best single expert): ~1.54 MB of the ~1.50 MB excess over the 0.177 bpc oracle floor.
- **APM + quantisation help**: coding redundancy **negative** (~−39 KB) — final chain beats raw mixer on average.
- **Parameter / sparsity**: **47%** of bits land in sparse contexts (same ballpark as v57).

#### Expert ensemble (`dump_experts` 80k records, stride 37) — **MEASURED**

| Metric | Value |
|--------|------:|
| Experts | 114 |
| Participation ratio | **2.061** / 114 |
| Entropy effective rank | **5.594** / 114 |
| Top-1 variance share | 69.1% |
| Dump wall | 29 s |

High pairwise correlation among discovery/PY experts (see `mp_rank.py` output) — mixer must combine ~114 correlated opinions; PR ≈ 2 confirms effective dimensionality ≪ raw count.

### 3.2 RECORD / PLAN citations (not re-run here)

#### v57 `--profile` on 8 MiB mem 22 (`SLOT_MAX=35`) — RECORD L1069–L1081

| Row | Bytes | BPC |
|-----|------:|----:|
| total spent | 1,707,748 | 1.628 |
| best-expert | 217,853 | 0.207 |
| after mixer | 1,731,001 | 1.650 |
| model redundancy | **+1,513,148** | — |
| coding redundancy | **−23,253** | — |
| parameter share | **47%** | — |

Wall **703.7 s**; archive **1,707,696 B**; **77 experts**; PR **3.02**.

#### `SLOT_MAX=24` vs `35` (v85 flags) — RECORD L2298–L2303

| Config | 8 MiB bytes | Δ vs s35 | RSS | Wall | BPC |
|--------|------------:|---------:|----:|-----:|----:|
| v85 `SLOT_MAX=35` | 1,680,191 | — | ~27 GB | ~1676 s | 1.602 |
| v85 `SLOT_MAX=24` | 1,681,094 | +903 | 1.53 GB | 1440 s | 1.603 |

PLAN L170–176: s24 screen is **Pearson +0.96** vs s35 gate with **~8× less RAM**.

#### RAM planning — PLAN L182–187

| Job | Typical RSS | Rule |
|-----|-------------|------|
| 8 MB mem 22, `SLOT_MAX ≤ 35` | ~15 GB | max **two** parallel encodes |
| 100 MB mem 26, `SLOT_MAX=31` | ~38–40 GB | champ confirmation only |
| 100 MB mem 26, `SLOT_MAX=35` | **OOM** | do not run |

### 3.3 Where CPU time goes (inference, uninstrumented)

No in-code stage timers exist. From structure and PAQ-lineage behaviour:

| Phase | Expected cost driver | Evidence |
|-------|---------------------|----------|
| **Experts** | Hash + table lookups across 11+ context orders, matches, discovery, domain mods; dominates | Large `predict()` loop; table bits scale with `HP_SLOT_*` |
| **Mixer** | k gated dot-products over ~100+ stretched inputs; SIMD (`HP_XSIMD`) | `mixer.hpp` L64–108; `mixer_weights.hpp` |
| **APM** | 3× 33-entry interpolations + updates | Cheap vs experts/mixer |
| **Coder** | Interval split + renormalize | `coder.hpp` — negligible vs prediction |
| **Updates** | Mirror predict path on `update()` | Same order as predict |

**RAM** scales with **`HP_SLOT_MAX`** and `HP_SLOT_GROW` / `HP_SLOT_WORD*` / `HP_SLOT_O6*` flags far more than bare `--mem 22` (2²² base per model, grown per-slot up to `SLOT_MAX`).

---

## 4. Exact reproduce: v82 @ 1.610 BPC

### 4.1 Recorded result (RECORD L1852–L1860)

| Field | Value |
|-------|-------|
| Version | **v82** = v81 + `HP_WORDLEN_MOD=1` |
| Corpus | `data/enwik8.8mb` |
| Archive | **1,689,157 B** |
| BPC | **1.610** (uses **full archive size** including header) |
| Decode SHA | `09F6DD7241A8AE21EDFD6762F3C6712A1FD02F7F322C5E77CAB8BB88F292EE8E` |
| fx2-manual | 1,685,642 B; SHA `563B4429789311B3E6E6DD71E5C6C58424B6BDD5E0382962161E78F0FCAA446E` |

v82 stack includes v81 accept `HP_UPPERGAP_MOD=1` (RECORD L1786–L1790).

### 4.2 Critical flag note

`hp/tools/v78_flags.ps1` is named for v78 but its header says **“v93 leftover compile flags (8 MB champ)”**. It includes flags accepted **after v82** (`HP_WIKIBOLD_MOD`, `HP_LR1_SCALE=40`, `HP_REFGROUP_MOD`, …). For **historical v82**, strip post-v82 `-D` lines.

### 4.3 Build & run (`SLOT_MAX=35`, mem 22 — champ gate)

Requires **≥16 GB free RAM** (RECORD H34: champ peaks 13.3 GB; v85 logged ~27 GB).

```bash
# From repo root
FLAGS=$(grep -oE '\-DHP_[A-Z0-9_]+=[0-9]+' hp/tools/v78_flags.ps1 \
  | grep -vE 'HP_LR1_SCALE|HP_REFGROUP|HP_SENTPOS|HP_STATETRANS|HP_CAPPARA|HP_WIKIBOLD' \
  | tr '\n' ' ')

g++ -O3 -std=c++17 -msse4.1 \
  -I hp/include -I hp/third_party/xsimd/include \
  $FLAGS \
  hp/src/main.cpp -o hp/build/hp_v82.exe

hp/build/hp_v82.exe c --mem 22 data/enwik8.8mb hp/build/e8_v82.hp
hp/build/hp_v82.exe d hp/build/e8_v82.hp hp/build/e8_v82.out
sha256sum hp/build/e8_v82.out data/enwik8.8mb
# Expect: 1,689,157 B archive, SHA above, bpc line ending in 1.610
```

Optional redundancy profile:

```bash
hp/build/hp_v82.exe c --mem 22 --profile data/enwik8.8mb hp/build/e8_v82.hp
```

### 4.4 Screen build (`SLOT_MAX=24`, mem 22 — cheap protocol)

```bash
FLAGS_S24=$(grep -oE '\-DHP_[A-Z0-9_]+=[0-9]+' hp/tools/v78_flags.ps1 \
  | grep -vE 'HP_LR1_SCALE|HP_REFGROUP|HP_SENTPOS|HP_STATETRANS|HP_CAPPARA|HP_WIKIBOLD' \
  | sed 's/-DHP_SLOT_MAX=35/-DHP_SLOT_MAX=24/' | tr '\n' ' ')

g++ -O3 -std=c++17 -msse4.1 \
  -I hp/include -I hp/third_party/xsimd/include \
  $FLAGS_S24 hp/src/main.cpp -o hp/build/hp_v82_s24.exe

hp/build/hp_v82_s24.exe c --mem 22 data/enwik8.8mb hp/build/e8_v82_s24.hp
# Agent measurement: 1,685,481 B (1.607 bpc), +3,676 B vs v82 champ — within H40 s24 band
```

### 4.5 Data prep

```bash
curl -L -o data/enwik8.zip http://mattmahoney.net/dc/enwik8.zip
unzip -o data/enwik8.zip -d data/
head -c 8388608 data/enwik8 > data/enwik8.8mb
```

---

## 5. Cypha integration footguns

Sources: `PLAN.md`, `README.md`, `UPGRADES.md`, `review-and-thinktank.md`, `hp/include/hp/coder.hpp` header comment.

### 5.1 Do not mix metrics or codebases

| Footgun | Detail |
|---------|--------|
| **Incomparable BPC** | Cypha README **2.664 bpc** uses `n_eval=2000` chars; CI ±0.103 bpc. hp uses **full-corpus archive bytes** + SHA round-trip on enwik slices. |
| **Different tasks** | hp = online lossless binary codec; Cypha lock = char-window eval harness. |
| **Separate repos** | Do not vendor Cypha CUDA/Qt/float paths into `hp/`. Public Cypha tree may not include `hp/`. |

### 5.2 Flag defaults (hp-specific)

| Footgun | Detail |
|---------|--------|
| **Bare `make` ≠ champ** | Only ~10/568 `HP_*` default ON. Default build ≈ **1,804,979 B / 1.721 bpc** on 8 MiB (README L37–38). |
| **`v78_flags.ps1` drift** | Tracks **current** champ (v93-era), not frozen v78/v82. Reproducing historical versions requires subtracting post-vN flags. |
| **`--mem` vs `SLOT_MAX`** | `--mem 22` sets base `table_bits`; per-model growth capped by `HP_SLOT_MAX` + `HP_SLOT_GROW*` — RAM planning needs **both**. |
| **Leftover mods default OFF** | Each `HP_*_MOD` is `#define … 0` in `features.hpp`; champ enables via compile `-D`. |

### 5.3 Header included in reported BPC

`main.cpp` L236–242: `bpc = ftell(out) * 8 / raw_len` — **entire CYHP archive** (magic, version, gria flag, table_bits, mixer_lr, dict flag, lengths, optional dictionary, page-perm tables, **plus** coded body).

Decompressor reads config from header — **`hp d` takes no modelling flags** (`main.cpp` L13–14).

Comparing to “coded body only” metrics (e.g. `bench.cpp` output) will disagree by ~29+ bytes header plus dict overhead.

### 5.4 Cypha mixer / tooling mismatches

| Footgun | Detail |
|---------|--------|
| **`dump_experts` on Cypha** | `AdaptivePredictorMixer` lacks per-expert dump; need `expert_prob(i,symbol)` accessors + sibling tool (UPGRADES L255–262). |
| **GRIA integration** | Cypha: optional 2-way `gria_gated_blend_logit` (default off). hp: GRIA as mixer gate + APM context + hedge switch rate. |
| **Arithmetic coder** | Cypha: float softmax + full-alphabet CDF per symbol. hp: integer binary decomposition (`coder.hpp` L5–14). |
| **Mixer loss mismatch** | Cypha hedge updates linear in *p*; codec pays −log *p* (review L37, L113). |
| **Config complexity** | Cypha `CyphaLMConfig`: 148 fields / 25 subsystems vs hp compile-flag ablation (`review-and-thinktank.md` L252–254). |

### 5.5 Operational footguns (hp lab)

| Footgun | Detail |
|---------|--------|
| **`SLOT_MAX=35` at mem 26** | OOM on 100 MiB — use **`SLOT_MAX=31`** (PLAN L186). |
| **Parallel jobs** | Max 2× 8 MiB champ; never 8 MiB + 100 MiB together (~53 GB). |
| **OneDrive / sync** | Can corrupt CYHP headers; write archives to local path (PLAN L52, L208). |

---

## 6. Quick command reference

```bash
# Default build (NOT competitive)
make -C hp

# Current champ build (v93-era v78_flags.ps1)
g++ -O3 -std=c++17 -msse4.1 -I hp/include -I hp/third_party/xsimd/include \
  $(grep -oE '\-DHP_[A-Z0-9_]+=[0-9]+' hp/tools/v78_flags.ps1 | tr '\n' ' ') \
  hp/src/main.cpp -o hp/build/hp.exe

# Redundancy profile
hp/build/hp.exe c --mem 22 --profile data/enwik8.8mb hp/build/out.hp

# Expert dump + rank
hp/build/dump_experts data/enwik8.8mb /tmp/e.i16 37 400000 22
python3 hp/tools/mp_rank.py /tmp/e.i16 114

# Wall + RSS (body only)
hp/build/bench data/enwik8.8mb --mem 22

# Float check on codec path
bash hp/test/no_float.sh
```

---

## 7. References

| Source | Relevant sections |
|--------|-------------------|
| `RECORD.md` | L1069–L1081 (v57 profile), L1852–L1860 (v82 champ), L2298–L2303 (s24 vs s35) |
| `PLAN.md` | L170–187 (gate protocol, RAM table) |
| `UPGRADES.md` | L255–262 (Cypha dump), L318–319 (task mismatch) |
| `review-and-thinktank.md` | L31–35, L71–79 (eval methodology), L252–254 (config surface) |
| `hp/tools/hp_harness.sh` | H34 calibration comments |
| `hp/include/hp/profile.hpp` | Redundancy definitions |

**Agent measurement log:** `hp/build/profile_s24.log`, `hp/build/bench_8mb_s24.log`, `hp/build/mp_rank_v82_s24.log` (local artifacts; not committed).
