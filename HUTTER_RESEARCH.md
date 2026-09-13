# Hutter Prize — research archive

Compiled 2026-08-22; **frontier refresh 2026-09-13**. Sources:
[prize.hutter1.net](http://prize.hutter1.net/),
[hrules.htm](http://prize.hutter1.net/hrules.htm),
[hfaq.htm](http://prize.hutter1.net/hfaq.htm),
[Large Text Compression Benchmark](http://mattmahoney.net/dc/text.html)
(Mahoney, last update 8 Jul 2026), winner READMEs (starlit, fast-cmix,
fx-cmix, fx2-cmix, cmix-lex), [fx3-cmix](https://github.com/kaitz/fx3-cmix),
[cmix-obias](https://huggingface.co/dfreelan/cmix-obias), encode.su,
Bellard NNCP.

This file is the literature. `PLAN.md` is the only living board.
`RECORD.md` is what we measured.

---

## 1. What the prize actually scores

Losslessly compress **enwik9** (first 10^9 bytes of
`enwiki-20060303-pages-articles.xml`) so that

```
S  =  size(comp.exe) + size(archive.exe)     <  L
```

and `archive.exe`, run with no other input, reproduces a 10^9-byte file
identical to enwik9.

| quantity | official value (prize page, as fetched) |
|---|---|
| Current record **L** | **110,793,128** (fx2-cmix, 3 Sep 2024 / awarded 8 Oct 2024) |
| 1% claim threshold | **109,685,197** |
| Award | `500,000€ × (1 − S/L)`, floor 5,000€ (1%) |
| Implied €/byte | ~1€ per 230 B |
| Time | ≲ `70,000 / Geekbench5` hours, one CPU core, no GPU |
| RAM / disk | < 10 GB RAM, < 100 GB HDD |
| Platforms | Linux or Windows x86 32/64, no extra installs |
| Source | OSI license, documented, 30-day public comment |

Relaxation: compressor + decompressor + archive, with
`S = zip(comp) + 2×zip(decomp) + archive` (1× if they are the same
binary). Command-line option length is added to S. Self-extracting
archive is the usual form: S1 ≈ 0.4–0.5 MB packed compressor, S2 ≈
the payload.

**cmix-lex (Ibrahim Marcouch, 29 May 2026)** is still **pending** as of
2026-09-13: the official prize table still lists fx2-cmix as L. Claimed
Intel-binary S = **109,650,047** (1.0317% under L). The Intel-built
binary reportedly failed to self-extract on the AMD judging machine;
an AMD source rebuild is claimed at **109,671,639** (+21,592 B). If
lex awards, the next 1% bar becomes ≈ **108,553,546**.

**2026-09-13 frontier (not on the prize page):**

| entry | status | S | vs L | what changed |
|---|---|---:|---:|---|
| **fx2-cmix** | **official L** | **110,793,128** | — | awarded 8 Oct 2024 |
| cmix-lex | pending | 109,650,047 (Intel) / 109,671,639 (AMD rebuild) | 1.03% / 1.01% | `payload_lex` + fxcm_v26 |
| fx3-cmix | **unsubmitted** | 109,735,627 | 0.95% (below 1%) | smaller disk PPM; Orav not actively working (encode.su) |
| cmix-obias | **claimed** (Freelan, fork of lex) | **108,492,825** | 2.08% | 256-cell LSTM, bitlstm32 fp16 head (23,002 B, counted twice in S), PPMd logit prior `+0.15 ln p_PPMd`; ~9.95 GiB, ~40 h, no GPU |

Do not treat claimed obias or unsubmitted fx3 as L. Do not vendor obias
(fp16 / `rcpps`) into `hp/`. Track W forks lex (or obias if it awards);
Track H tests whether a dedicated wiki-axis CM or a low-rank integer
mixer is the 1% worth landing.

Hutter's own start advice (FAQ): you cannot win with a new idea
alone. You have to combine it with the current messy SOTA. Every
winner since 2021 did exactly that: fork the previous winner, keep the
self-extracting pipeline, change one or two axes, spend the 50-hour
budget.

---

## 2. The number we have to beat

| compressor | date | S (bytes) | factor | RAM | time | prize? |
|---|---|---:|---:|---|---|---|
| You (1% vs fx2) | — | **< 109,685,197** | > 9.12 | < 10 GB | < 50 h | 5,000€+ |
| cmix-obias (claimed) | 2026-09 | 108,492,825 | 9.22 | 9.95 GB | ~40 h | not on prize page |
| cmix-lex (pending) | 2026-05 | 109,650,047 | 9.12 | 9.4 GB | ~44 h CPU | if awarded |
| fx3-cmix (unsubmitted) | 2026 | 109,735,627 | 9.10 | ~10 GB | ~47 h | <1%, not submitted |
| **fx2-cmix** (L) | 2024-09 | **110,793,128** | 9.03 | 8.8 GB | ~47 h eq. | 7,950€ |
| fx-cmix | 2024-02 | 112,578,322 | 8.88 | 8.9 GB | ~50 h | 6,911€ |
| fast-cmix | 2023-07 | 114,156,155 | 8.76 | 8.4 GB | 43 h | 5,187€ |
| starlit | 2021-05 | 115,352,938 | 8.67 | 10 GB | ~50 h | 9,000€ |
| phda9 v1.8 | 2019-07 | 116,673,681 | 8.58 | 6.3 GB | ~23 h | pre-prize baseline |

enwik8 era (100 MB, 2006–2017):

| compressor | date | S | bpc | award |
|---|---|---:|---:|---|
| phda9 | 2017-11 | 15,284,944 | 1.225 | 2,085€ |
| decomp8 | 2009-05 | 15,949,688 | 1.278 | 1,614€ |
| paq8hp12 -7 | 2007-05 | 16,481,655 | 1.319 | 1,732€ |
| paq8hp5 -7 | 2006-09 | 17,073,018 | 1.366 | 3,416€ |
| paq8f -7 (baseline) | 2006-03 | 18,324,887 | 1.466 | — |

From paq8f to fx2-cmix on the *same* knowledge dump, S dropped ~40%
on enwik8-equivalent and the enwik9 factor went from ~5.5 to 9.03.
Almost all of that is context mixing + Wikipedia-specific
preprocessing, not a new coding theorem.

### 2.1 Our hp vs the same numbers

| corpus | hp | winner / SOTA | gap |
|---|---|---|---|
| enwik8 1 MB, mem 26 (v3b) | 229,440 B / **1.750 bpc** | (no public 1 MB table) | — |
| enwik8 100 MB, mem 20 | 21,828,153 B / **1.746 bpc** | phda9 **1.225 bpc** | **+42% bits** |
| enwik9 1 GB (not run) | projected ~218 MB @ 1.75 bpc | fx2 **110.4 MB** payload | **~2× too big** |

hp is a real compressor (round-trip holds). It is not yet in the
prize band. Closing 1.746 → ~0.88 bpc on enwik9 is a *preprocess +
model-zoo + 10 GB* problem, not another sparse n-gram.

---

## 3. Winner genealogy — what each one actually changed

Every award after 2006 is a fork. The stack that won is:

```
enwik9
  → article reorder          (starlit 2021, refined 2024/2026)
  → Wikipedia / HTML transform (phda9 2017, single-pass in fx2)
  → WRT / reverse dictionary (Rhatushnyak 2006–2017, online in fx2)
  → context-mix predictor    (PAQ → cmix → fxcm → fxcm_v26)
  → packed self-extracting archive (UPX + embedded dict + order file)
```

### 3.1 Matt Mahoney — paq8f (2006, pre-prize baseline)

**Idea.** Context mixing: many independent models emit P(next bit = 1);
a neural mixer (logistic, online SGD) combines them; arithmetic code
the bit. Models include order-n, word, sparse, match, and indirect
(StateMap) predictors.

**Why it matters.** This *is* the modern prize architecture. hp is
this idea with integer-exact tables.

### 3.2 Alexander Rhatushnyak — paq8hp5 … phda9 (2006–2019, four awards)

**Ideas that paid, in order of lasting value:**

1. **Static WRT / DRT dictionary** with *semantic clustering*. Frequent
   words become short codes that still look like words to the word
   model. Similar meanings get similar codes, so context prediction
   transfers. Built and hand-tuned over years; size counts against S.
   encode.su (Sheosi / Shelwien): this is more "AI-like" than a
   generic LZ78 dictionary.
2. **enwik-specific transforms** (open-sourced 2017 with phda9):
   protect patterns that a naïve rewriter would smash, rewrite
   MediaWiki / XML so the mixer sees cleaner text. ~5% size drop
   *before* DRT on enwik8.
3. **Manual experiment loops.** His own note on paq8hp6–12: most of
   the time went into planning experiments and reading the results,
   not inventing new math.
4. phda9 added an **LSTM** path and a ~188 k-word external dictionary.

**Lesson for us.** Dictionary + wiki rewrite is not a side quest. It
is how the first four prizes were won. Our B.3 "NO-GO" was correct
on a 3 MB proxy (dict +1,498 B payload, −10,121 B storage). It is
**not** settled on enwik8/9. Re-test at 100 MB before discarding.

### 3.3 Dmitry Shkarin — durilca (2006, rejected)

PPMd / ppmonstr + filters (text, exe, fixed-length records).
**1.5% better than paq8f** but **1.65 GB RAM** vs the then-reasonable
limit. Still #10-ish on LTCB in 2026 at 127.4 MB enwik9 with 13 GB.

**Lesson.** PPM unbounded-order is real (order-41). The prize killed
it on RAM, then fast-cmix brought it back via **mmap-to-disk**. hp's
`HP_PPMD=0` prediction (overlap with PY) may be wrong once we have
10 GB + disk.

### 3.4 Artemiy Margaritov — starlit (2021, first enwik9 award)

**The idea that reset the contest.**

enwik9 is articles sorted by *title*. Context-mix tables are a
bounded buffer. Similar articles share vocabulary, templates, and
infobox shape. If you place similar articles adjacent, that shared
state is used before eviction.

Search (not in the decompressor):

1. Doc2Vec embedding per article.
2. Treat embeddings as points; solve a TSP for a short path.
3. Ship only the **permutation** (row *n* = original article index).
4. Restore by sorting titles (tiny code, alphabetical is free).

Plus: disable PAQ8 and layer-1 mixers, shrink PPMD to 850 MB, LSTM
to 1×200, floats instead of doubles, PGO, UPX, embed compressed
dictionary + order file. HP-2017 transforms ported to enwik9.

**Lesson.** Reordering is the largest single documented win of the
enwik9 era and **hp does not have it**. Search complexity is free
(PLAN.md 4.2). Only the order file + a sort costs S1.

### 3.5 Byron Knoll — cmix / cmix-hp (lineage, not always awarded)

cmix is the model zoo: PAQ8-family predictors + PPM + LSTM byte
mixer, 20–30 GB unconstrained. cmix-hp (Jun 2021) is starlit with a
**huge PPMD mapped to 21 GB virtual / 10 GB RSS**. LTCB cmix v21
(Sep 2024, includes fx2 ideas): **107,963,380** enwik9, **30.9 GB**
— better than fx2, illegal for the prize.

**Lesson.** Unconstrained SOTA is ~3% ahead of the prize record
(encode.su "scaling law" thread). The prize is a *resource-shaped*
slice of cmix, not a different algorithm family.

### 3.6 Saurabh Kumar — fast-cmix (2023)

No new model class. **Speed is a compression tool.**

- Kill virtual calls on the predict path (enables inlining).
- AoS / no pointer-chasing for cache locality.
- Faster hash maps.
- Huge PPM, swapped to disk.
- Export *fewer* PAQ8HP predictions into the cmix mixer; drop some
  mixers.

Result: same architecture, finishes in 43 h instead of 50, which is
what made a 1.04% claim possible on the official box.

**Lesson.** hp's 53 µs/byte on 1 MB projects ~14 h for enwik9 — we
still have time budget. Spend it on memory and models, not on more
correlated twins. When we add LSTM / PPM, we will need his tricks.

### 3.7 Kaido Orav — fx-cmix (Feb 2024)

Replace paq8hp with **fxcmv1**, a Wikipedia-native context machine:

- ~30 main predictors, contexts split for memory efficiency.
- **Bracket, quote, first-char, paragraph, column, table, template,
  word-stream** contexts. Parse wiki links, HTTP, tables, lists.
- Swap / disable predictors by last char, link, current bracket.
- Match model (paq8hp lacked it).
- Multiple state tables.
- New dictionary; small phda9 preprocessor tweaks.
- PPM still mmap-to-disk for the 10 GB cap.

**Lesson.** This is the model we have been porting piecemeal
(`wiki.hpp`, `bracket.hpp`, `wordstream.hpp`). We ported states, not
the *swap/disable* logic or the 30-way split. fx-cmix's gain over
fast-cmix is almost entirely "the mixer sees wiki structure."

### 3.8 Orav + Knoll — fx2-cmix (Sep 2024, current official L)

On top of fx-cmix:

**NLP / stemmer (from paq8pxd), new word types:** Article,
Conjunction, Adposition, ConjunctiveAdverb. Four word streams:

1. undecoded words
2. stemmed sentence stream (reset on `.?!`)
3. stemmed paragraph stream (drops Conjunction, Article, gender,
   ConjunctiveAdverb)
4. stemmed content stream (also drops Adposition, AdverbOfManner)

Word limit 64 → 256. Some types *skip* stream updates. Words between
`=|`, `<>`, `[|`, `()` stripped from streams 2/3.

**Online reverse dictionary.** Dictionary appears in the stream;
after it is decoded, later text is looked up. Separate text buffer
from the coded byte buffer.

**Single-pass Wikipedia transform.** HTML entities → UTF-8. Disk
18 GB → 7 GB, time 7 min → 3 min.

**Article order v2.** voyage-large-2-instruct 1024-d embeddings →
t-SNE to 1-d → sort → k-means, original order inside cluster →
reverse → manual (images / disambiguation to the end).

**Mixer:** skip weight update when error < threshold (speed); drop
regularizer; drop 7 indirect NS predictors, 6 match models, 3
mixers so fxcm can be *heavier*. Three ContextMaps (32 / 64 / 128
byte slots by table size). Sparse match, gap 1–2, minlen 3–6, for
escaped UTF-8. Detect `<math>`, `<pre>`, `<nowiki>` and skip word
contexts inside them. LSTM expected-byte as a mixer context.
Dictionary index of the partially decoded word as mixer context.

**S breakdown:** S1 = 441,463; S2 = 110,351,665; S = 110,793,128.

**Lesson.** They *deleted* models to make room. Our PLAN2 instinct
(drop twins) matches this — but we dropped the wrong ones relative
to 1 MB bpc. Their deletions were timed (CPU) and then reinvested.

### 3.9 Ibrahim Marcouch — cmix-lex (May 2026, pending)

Fork of fx2-cmix, **not** Orav's unpublished fx3-cmix.

- **payload_lex:** reorder a structured PHDA9 *tail* region; compact
  EOF side-data for exact restore.
- **R1ORD3:** visible `D86a` fields + Lehmer ranks for reversible
  regime-1 order.
- **fxcm_v26** (Orav standalone) into the cmix stack.
- Article order from the fxcm_v26 archive.
- PPM heap still disk-backed; `MADV_DONTNEED` / `MADV_RANDOM` /
  `O_NOATIME` / `ftruncate`.
- clang-17, PGO, UPX 5.1.1 (older UPX can produce a binary that
  dies on new kernels).

Claimed S = 109,650,047. LTCB lists archive9 = 109,190,109.

**Lesson.** The remaining 1% is now in *structured-tail permutation*
and a newer fxcm, not another n-gram. If this is awarded, we inherit
a new L and a new open source tree (`blahem/cmix-lex`).

---

## 4. Record-breakers that did **not** win (still steal from them)

LTCB ranks `enwik9 + decompressor`. No RAM/time cap. Mahoney update
8 Jul 2026:

| program | enwik9 | +prog | mem | alg | why not prize |
|---|---:|---:|---:|---|---|
| **nncp v3.2** (Bellard) | 106,632,363 | 107,261,318 | 7.6 GB | Transformer | GPU / multi-thread / 67 h-class; spirit + rules |
| **cmix v21 -t** | 107,963,380 | 108,244,767 | **31 GB** | CM+LSTM | RAM |
| **cmix-lex** | 109,190,109 | 109,190,109 | 9.4 GB | CM | pending award |
| fx2-cmix | 110,351,665 | 110,351,665 | 8.8 GB | CM | current L |
| jax-compress (Knoll, TPU) | 113,393,442 | 113,393,442 | 42 GB | LSTM | TPU |
| tensorflow-compress v4 | 113,542,413 | 113,597,696 | 45 GB | LSTM | GPU |
| paq8px_v206 -12L | 124,696,410 | 125,099,359 | 28 GB | CM | RAM/time |
| durilca kingsize | 127,376,595 | 127,783,295 | 13 GB | PPM | RAM |
| zpaq 6.42 | 142,252,605 | 142,257,365 | 14 GB | CM | ratio |
| gzip | 322,591,995 | — | — | LZ | baseline |

### 4.1 nncp (Fabrice Bellard) — Transformer LM as compressor

Predict next token with a Transformer, arithmetic-code the residual.
v3.2: 7 layers, hidden 384-class, ~199 M params historically; C +
LibNC; CUDA optional. **0.853 bpc** on enwik9 vs cmix v19 0.892.

FAQ + rules: GPU banned because it "stifles non-vectorizable ideas"
(Hooker's hardware lottery, inverted). A CPU Transformer at 199 M
params will not finish in 50 h / 10 GB.

**What we can steal without a GPU:**

- Dictionary / symbol preprocessor (cmix and starlit already did).
- The observation that **long-range semantic mixing beats another
  order-n**. Our Hebbian + Hedge are the cheap version; LSTM mixer
  (UPGRADES 2.4) is the legal version.
- Two-part MDL: model size counts. nncp's zip is 629 KB. A huge
  weight file would lose the prize even if bpc won.

### 4.2 paq8px / paq8pxd (Orav and others)

The living PAQ. Stemmer, UTF-8, JPEG/EXE/WAV detectors, record
models, more state tables. fx2's NLP is a paq8pxd stemmer with extra
POS-like types. Harvest clone: `harvest/paq8pxd`.

### 4.3 zpaq (Mahoney)

Journalled context mixing, tiny decompressor (4.7 KB). Different
product (archiver). Not a prize threat. Useful as a reminder that
**S1 can be tiny** if you generate tables at runtime (fx2 already
does this for state tables).

### 4.4 BWT / PPM / GLZA / kanzi

bsc-m03, ppmonstr, glza grammar coder: 157–162 MB. They lose ~40%
to CM on this corpus. Do not pivot hp to BWT.

### 4.5 Shannon / Hutter on the floor

FAQ: Shannon 0.6–1.3 bpc for English; humans might take enwik9 to
~75 MB; machines should do better. We are at 110 MB official, 107
unconstrained. **~30 MB of structure is still on the table** before
anyone is near a human language model, and Kolmogorov complexity
has no computable lower bound. The 1% bar is 1.1 MB, not 30 MB.

enwik9 is ~75% prose, ~25% tables / markup / XML. Both are human
knowledge. Filtering the "artificial" part never won.

---

## 5. Technique taxonomy (cast wide, then score)

An item is only useful to hp if it (a) reads a **new information
axis**, (b) spends unused RAM/time, or (c) shrinks S1 without
hurting S2. Adding a second model on a saturated axis is how we
stalled at rank ~3.9 / 64.

### 5.1 Preprocess (largest documented prize deltas)

| # | technique | who | axis | hp | expected | test |
|---|---|---|---|---|---|---|
| P1 | Article reorder (embed → 1-d / TSP / k-means) | starlit, fx2, lex | document adjacency | **missing** | 0.5–1.5% on enwik9 | 4.2 |
| P2 | WRT/DRT static clustered dictionary | phda9, all later | stored lexicon | `dict.hpp` **off** | large on 100 MB+, negative on 3 MB | B.3 retest |
| P3 | Online reverse dictionary | fx2 | same, no S1 dump | missing | medium | after P2 |
| P4 | Single-pass wiki/HTML transform | phda9, fx2 | markup canonicalization | missing | medium | after P1 |
| P5 | payload_lex / PHDA9 tail permute | cmix-lex | structured metadata order | missing | the last 1% | after P4 |
| P6 | Entity → UTF-8, protect-sed | starlit/phda9 | rewrite safety | missing | small, prevents loss | with P4 |
| P7 | Manual class sort (images/disambig last) | fx2 | cluster hygiene | missing | small | with P1 |

### 5.2 Predictor axes (what the mixer sees)

| # | technique | who | axis | hp | note |
|---|---|---|---|---|---|
| X1 | Wiki state machine + linkword | fx2 | markup | **ported** (v3b) | keep; v3b record |
| X2 | isParagraph / FIRSTUPPER as **gate** | fx2 | layout | partial (state nibble) | PLAN2 item 4 |
| X3 | Four stemmed word streams + POS types | fx2 | syntax | 2 of 10 configs | **biggest unported NLP** |
| X4 | Stemmer (paq8pxd) | fx2 | morphology | missing | X3 depends on this |
| X5 | Word-keyed match, sparse keys | fx2 | word match | 1/2/3 stack (correlated) | retarget {0},{1,3},{7,2} |
| X6 | Bracket / quote stacks | fx2 | nesting | ported | twin of `pat_` |
| X7 | Numeric field (n0, n1, len, dec) | paq8/fx2 | numbers | v4 added, lost bits | re-ablate alone |
| X8 | Column / table / template swap | fx-cmix | 2-d + wiki | column only | predictor on/off |
| X9 | Sparse match gap 1–2 (escaped UTF-8) | fx2 | UTF-8 | missing | cheap |
| X10 | `<math>`/`<pre>`/`<nowiki>` mute | fx2 | domain gate | missing | speed + bits |
| X11 | PPMD o25 mmap | cmix-hp, fast | unbounded order | flag off | re-test at 10 GB |
| X12 | LSTM mixer / expected-byte ctx | cmix, fx2 | neural mix | missing | last; 2.4 |
| X13 | Discovery / skip-k masks | hp | searched sparse | **have** | **unswept** |
| X14 | Hebbian A→B | hp | association | have | keep; unique |
| X15 | Sequence memoizer / PY infinite | Teh / literature | unbounded | discount only | after PPMD |
| X16 | CTW | Willems | tree mix | rejected r=0.930 | stay off |
| X17 | Three ContextMaps 32/64/128 | fx2 | estimator | one map | collision vs RAM |
| X18 | Multiple state tables | fx-cmix | estimator | one table | after X17 |
| X19 | Dict-index mixer context | fx2 | decode state | missing | needs P3 |
| X20 | senword (body-only 2104-hash) | fx2 | prose vs link | packed in tag | PLAN2 |

### 5.3 Mixer / systems (not axes, but they bought awards)

| # | technique | who | hp | why test |
|---|---|---|---|---|
| M1 | Skip mixer update if error < θ | fx2 | **have** (`HP_MIXER_SKIP=32`, v61) | keep |
| M13 | Low-rank integer mixer `W ≈ AB` | this lab (v57 profile) | **H6** | mixer hole +1.51 MB, PR 3.02 |
| M14 | bitlstm32 + PPMd logit prior | cmix-obias | Track W only | floats / SSE rcpps; counted in S |
| M2 | Drop mixer L2 regularizer | fx2 | n/a | speed |
| M3 | Delete redundant experts to fund a heavier one | fx2 | PLAN2 tried, lost | delete only after 1 MB A.3 |
| M4 | Per-mixer learning rates | fx2 | have | keep |
| M5 | NCL small λ | literature | have | keep tiny |
| M6 | Hedge / fixed-share over L1 | hp | have | keep |
| M7 | PGO + clang + UPX 5.1.1 | starlit→lex | MSVC/g++ | S1 + time |
| M8 | mmap PPM + madvise | lex | no | if X11 |
| M9 | No virtual calls / AoS | fast-cmix | already header | profile before LSTM |
| M10 | `--mem` to 10 GB | all winners | curve to 26, still falling | **do now** |
| M11 | Config::normalize both sides | hp bug | fixed | never regress |
| M12 | Runtime-generated state tables | fx2 | compiled | S1 |

### 5.4 Ideas that look smart and lose

| idea | why it dies |
|---|---|
| Recursively compress the archive | counting argument; FAQ |
| GPU / TPU Transformer | rules + hardware lottery inversion |
| Huge shipped NN weights | L(M) is in S |
| More order-n twins | our rank is ~4 of 64; PLAN2 v4 lost 479 B |
| Generic XML depth next to wiki machine | saturated |
| Secret test-set / pretrained closed corpus | "outside information" banned |
| Lossy + ignore markup | lossless required; 25% of the file *is* markup |
| BWT / gzip-family | 40% behind CM on this file |

---

## 6. Information axes still thin in hp

From `AXES.md` + winner READMEs + our A.3:

**Thin (test first):**

1. Document adjacency (P1) — not an online expert; a permutation.
2. Morphology / POS-gated word streams (X3–X4).
3. Stored / online lexicon at enwik8 scale (P2–P3).
4. Markup canonicalization (P4–P6).
5. Structured tail order (P5).
6. Unbounded order with real RAM (X11), not another PY twin.
7. Neural mix (X12) once time is measured.
8. Discovery slot/eval sweep (X13) — already built, never swept.
9. Memory (M10) — **largest measured lever this project has**.

**Saturated (do not add):**

byte suffix o1–o6, CTW, generic match-at-order-N, second XML depth,
case-preserving word stream, stacked word-match-2/3, `pat_` +
bracket together.

---

## 7. Strategy that can actually cash the cheque

Hutter: combine with SOTA. Every post-2020 winner forked the
previous tree. Two parallel tracks:

**Track W (winner fork).** Clone `fx2-cmix` and `cmix-lex`. This is
the only path with a published 110 MB file. Improvements are P5-class
and fxcm_v26 deltas. Compliance work (PGO, UPX, mmap, 10 GB) is
already done upstream.

**Track H (this repo).** Keep hp integer-exact as a laboratory:

1. Spend the 10 GB (M10) and sweep discovery (X13).
2. Port preprocess (P1, then P2 at 100 MB, then P4).
3. Port stemmer streams (X3–X4) behind flags; A.3 on 1 MB **and**
   8 MB / 32 MB slices (1 MB lied to us on PLAN2).
4. Only then LSTM / PPMD.

Do not expect Track H alone to hit 109 MB this month. Use it to
*measure axes*, then land the winners on Track W.

Acceptance for any change (unchanged from PLAN):

- no `float` in `hp/include`, `hp/src`
- encoder/decoder derive all state from shared history
- round-trip SHA
- on `data/enwik8.1mb`: bpc down **or** entropy rank up, not just
  "looks diverse"
- re-check on a ≥ 8 MB slice before trusting a 1 MB reject/keep

---

## 8. Source map

| tree | where |
|---|---|
| fx2-cmix | `harvest/fx2-cmix`, github.com/kaitz/fx2-cmix |
| fx-cmix | github.com/kaitz/fx-cmix |
| fxcm / fxcm_v26 | github.com/kaitz/fxcm |
| cmix | `harvest/cmix`, github.com/byronknoll/cmix |
| fast-cmix | github.com/saurabhk/fast-cmix |
| starlit | `harvest/starlit`, github.com/amargaritov/starlit |
| cmix-lex | github.com/blahem/cmix-lex (clone into harvest) |
| paq8pxd | `harvest/paq8pxd` |
| nncp | bellard.org/nncp |
| LTCB | mattmahoney.net/dc/text.html |
| prize / rules / FAQ | prize.hutter1.net , hrules.htm , hfaq.htm |
| discussion | groups.google.com/g/Hutter-Prize , encode.su |

---

## 9. Bibliography (prize-adjacent)

- Mahoney, *Data Compression Explained* (2011); LTCB rationale (2009).
- Hutter, *Universal Artificial Intelligence* (2005 / 2024).
- Shannon, prediction and entropy of printed English.
- Willems, Shtarkov, Tjalkens — CTW.
- Teh, *A hierarchical Bayesian language model based on Pitman-Yor*.
- Liu & Yao, Negative Correlation Learning (1999).
- Bellard, NNCP papers (v2.1, v3).
- Hooker, *The Hardware Lottery* (why the prize bans GPUs).
- Franz et al., incremental compression (2021).
- Winner READMEs listed in §3.

---

## 10. One-page steal list

If we only remember ten things:

1. **Reorder articles.** Free search, tiny decoder, every win since 2021.
2. **Dictionary is scale-dependent.** Retest WRT on enwik8, not the proxy.
3. **Wiki transform** before more n-grams.
4. **Stemmer + four streams + POS gates** — fx2's actual NLP.
5. **Delete to reinvest** (time and mixer capacity), don't stack twins.
6. **10 GB is the cheapest 1% we have already seen.**
7. **PPM via mmap** is how they used "unbounded order" legally.
8. **LSTM is a mixer, not a replacement for preprocess.**
9. **Fork the winner** if the goal is the cheque; use hp to pick axes.
10. **cmix-lex / fxcm_v26 / payload_lex** are the 2026 frontier.
