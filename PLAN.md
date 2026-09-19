# PLAN — living experiment board

## Vision

hp is an integer-exact PAQ-lineage laboratory. Goal: find which 1% is
real, then land it on a winner fork (Track W). Official Hutter bar is
enwik9 **S < 109,685,197** vs **L = 110,793,128**. hp has not been run
on enwik9. 8 MB leftovers will not cash the cheque.

Three tracks, different machines:

| track | where | job |
|---|---|---|
| **H** | this repo, `hp/` | one compile flag; 8 MB mem 22; bytes-down, SHA, fx2 stack; 100 MB mem 26 champ only with `SLOT_MAX=31` |
| **W** | `harvest/fx2-cmix`, `harvest/cmix-lex` | fork SOTA |
| **C** | github.com/odin-loki/Cypha | `review-and-thinktank.md` only |

Never vendor Cypha, CUDA, Qt, or floats into hp. Do not treat Cypha’s
2.664 bpc (`n_eval = 2000`) as comparable to any hp SHA number.

---

## Lab truth

Leftover wave **closed** at **v78**. RECORD leads if a later run accepts.

| | bytes |
|---|---:|
| Official L (fx2-cmix, prize page 2026-09-13) | 110,793,128 |
| 1% claim vs L: S < | **109,685,197** |
| cmix-lex (pending; Intel binary) | 109,650,047 |
| cmix-lex AMD rebuild (claimed) | 109,671,639 |
| If lex awards: next 1% ≈ | 108,553,546 |
| cmix-obias (claimed, not on prize page) | 108,492,825 |
| fx3-cmix (unsubmitted, <1%) | 109,735,627 |

enwik8 SOTA: cmix v21 ~14.62 MB. hp ~18.53 MB is paq8f-era quality.

**Mixer rate (H33, 2026-09-18).** `HP_LR1_SCALE=40` halves the hardcoded
layer-1 per-mixer rates to `{1,1,1,2,1,2}`. Landed at `SLOT_MAX=35` as
**v84**. See RECORD.

**8 MB champ** (`data/enwik8.8mb`, mem 22): **v93** identity
**1,675,993** (−401 vs v92 1,676,394), **1.598 bpc**, RT PASS.
fx2-manual pending. `HP_MIXER_SKIP_L1=80` ON.
Stack v92 + sl80. Did not overwrite `hp_v83.exe`–`hp_v92.exe`.

**100 MB champ** (`data/enwik8.fx2man`, mem 26, `SLOT_MAX=31`): **v77**
**18,370,971** (−38,737 vs v75 18,409,708), **1.469 bpc**, RT PASS.
Encode binary `hp_v77_m26.exe` (pulled tree + TITLE). Did not overwrite
`hp_v75_m26.exe`. Write archives to `%LOCALAPPDATA%\hp_lab` then copy;
OneDrive ate a CYHP header once.

v75 100 MB 18,409,708 RT PASS; v73 18,434,740; v57 18,527,464 RT PASS; v45 encode-only 18,633,242; v37 18,671,091 RT PASS.

C++ profile v57 8 MB: wall 704 s; model redundancy mixer vs best-expert
**+1,513,148 B**; PR **3.02** of 77 experts. Mixer is the hole.
Dedicated wiki-domain ContextModels paid (`SENGRP_MOD`); mixer gates
and n-gram twins mostly did not.

---

## Axes (from AXES.md)

hp already has: o1–o6 + PY, sparse/skip + discovery, word + wbi, case /
first-char / sentence streams, column, wiki states, table/link/template,
byte match 3/4/6/10/16, word-keyed match ×3, Hebbian, bracket nest,
pattern class, CTW (off), per-mixer lr / NCL / Hedge-L1.

Still thin vs winners: wiki transform stacks (`fccxt` / `brcxt` /
`ColumnContext`), LSTM mixer, WRT / `payload_lex`, POS / stem streams,
pronoun and partial-sentence rings.

Saturated: extra o1–o6 twins, match-order pile-ons, mixer-gate dilution.
Do not add more without a new axis.

---

## Models — the meta-pattern

Every paying expert is the same bilinear slice, not a new algorithm:

```
logit_i = StateMap( hash( axis_i  ⊗  recent history ) )
p       = squash( v_ctx2ᵀ  W_ctx  x )     x = stretch(logit)
```

`HP_SENGRP_MOD`, `HP_NEST_MOD`, `HP_PARA_MOD`, `HP_LINE_MOD` paid
because each added a **new axis** (sen-group, nest-markup, paragraph
bit, line-kind) into that outer product. Extra o1–o6 twins and match
orders failed because they added a second row on a **saturated** axis.
Mixer *gates* (`HP_SEN_GROUP`, word/hebb/link folds) failed because they
switch which `W` is trained and split the data.

hp is already a system of matrices: sparse hashed tables (not dense
GEMM) plus `MixerNet` — layer-1 `W_j` is `(n_ctx × n_experts)`, layer-2
`v` is `(n_ctx2 × k)`. The v57 profile is the hole in that product:
77 experts, participation **3.02**, mixer-vs-best **+1,513,148 B**.
Rewriting the codec as BLAS / Eigen / floats / GPU cannot pay (illegal
and the lookups are sparse). The matrix move that *can* pay is a
**low-rank factor of W** (H6): `W ≈ A B` with small integer rank,
same Q16 dots as `mixer.hpp` today.

Paying family, remaining headroom: unsaturated table RAM under the
MAX cap, then a new wiki-domain axis as its own CM, then mixer rank.
PPMD mmap, Sequence Memoizer, n-gram twins, and LSTM-in-hp are not
this lab’s next 1% — LSTM / obias live on Track W.

---

## Upgrades still open (from UPGRADES.md)

Proxy-era numbers in that file are dead. Mixer redundancy is the hole
(+1.51 MB at 8 MB). LSTM mixer / bitlstm32 / obias belong on Track W
(cmix-obias claim). Article reorder is fx2-manual *test input*, not an
hp preprocessor. Dictionary / WRT is Track W. GPU / nncp is illegal.
Do not re-run the discovery slot/eval sweep (RECORD H0.2). Do not
rewrite hashed CMs as a dense matrix multiply.

---

## Closed from PLAN2 / PLAN3

Those files are gone. Measurements live in `RECORD.md`. What still binds:

- Axis-purity cuts that drop correlated twins can lose bits at equal
  memory (PLAN2 v4 +479 B on 1 MB; same v4 **−1,531** at 8 MB — H0.4).
  Rank is a compass, not an accept gate.
- 1 MB (`data/enwik8.1mb`) is 84% redirects (H0.5). Never accept or
  reject on it alone. Memory scaling on that slice plateaued at mem 28
  (H0.1, −3 B vs mem 26).
- First bundle keeps: wiki states, word-match-1, bracket, per-mixer lr /
  Hedge-L1 / NCL. First bundle rejects: CTW, stacked word-match-2/3,
  `pat_`+bracket, case-preserving word stream, PPMD, B.3 library.
- Prize-era H0–H2 queue is closed (discovery sweep, mute-in-math, stem
  as extra experts, dict/WRT on 8 MB, pred-gate). Do not reopen from
  memory; pointer is RECORD / the table below.
- Article reorder is accepted as fx2-manual *test input* (H1.2), not an
  hp preprocessor. W1–W3 inventory is written; W4/W5 remain in Track W.

---

## Patterns (from PATTERNS.md)

B.2 online class taxonomy (table / cite / time / infobox / link) stays.
B.1 pattern cache stays. **B.3 shipped pattern library / dict: NO-GO**
— transform win 1,498 B vs storage 10,121 B. Prefer zero-transmitted
Hebbian / wiki-state / class. Revisit only if stored size < 30% of an
enwik8 transform gain.

---

## Protocol, RAM, binaries

One compile flag per binary. Same binary encodes and decodes. No
`float` / `double` / `<cmath>` in `hp/include` or `hp/src`.

**Build line (the one in README was broken — it omits both -I paths):**

```
g++ -O3 -std=c++17 -msse4.1 -I hp/include -I hp/third_party/xsimd/include \
    $(grep -oE '\-DHP_[A-Z0-9_]+=[0-9]+' hp/tools/v78_flags.ps1 | tr '\n' ' ') \
    hp/src/main.cpp -o hp/build/hp_vNN.exe
```

A default-flag build is 1,804,979 (v7-era). The champ is the 79-flag set.

**Gate (in order):** — H34 calibrated this against 12 known deltas.

0. **Screen** on `data/enwik8.2mb` at `--mem 22 -DHP_SLOT_MAX=24`
   (92 s, 1.5 GB). Spearman **+0.84** vs the 8 MiB gate, **13/13**
   correct accept/reject signs. Kill losers here.
1. Build a new-named exe (never overwrite a binary a long job is using).
2. Compress first 8 MB of `data/enwik8` at `--mem 22`. Use
   `-DHP_SLOT_MAX=24` (1.6 GB, 8.5 min): Pearson **+0.96** vs
   `SLOT_MAX=35` and only +895 B on the base, for 8x less RAM.
3. **Accept if archive bytes drop** vs the current 8 MB champ.
4. Round-trip SHA of the decompressed file vs the input slice.
5. If accepted: stack-check on `data/enwik8.8mb.fx2man` (same mem 22).
6. Champ only: full enwik8 fx2-manual at `--mem 26`, `SLOT_MAX=31`.

| job | typical RSS | rule |
|---|---|---|
| 8 MB leftover, mem 22, `SLOT_MAX` ≤ 35 | ~15 GB | **two at a time** (user 2026-09-13) |
| 100 MB mem 26, `SLOT_MAX=31` | ~38–40 GB | champ confirmation only |
| 100 MB mem 26, `SLOT_MAX=35` | OOM | **do not run** |
| 8 MB leftover **plus** 100 MB | ~15 GB + ~38 GB | **never together** |

**Protect:** `hp_v37.exe`, `hp_v45_m26.exe`, `hp_v55_m26.exe`,
`hp_v57.exe`, `hp_v57_m26.exe`, `hp_v58.exe`, `hp_v59.exe`,
`hp_v60.exe`, `hp_v61.exe`, `hp_v61_m26.exe`, `hp_v62.exe`,
`hp_v63.exe`, `hp_v64.exe`, `hp_v65.exe`, `hp_v66.exe`, `hp_v67.exe`,
`hp_v68.exe`, `hp_v69.exe`, `hp_v70.exe`, `hp_v70_m26.exe`,
`hp_v71.exe`, `hp_v72.exe`, `hp_v73.exe`, `hp_v73_m26.exe`,
`hp_v74.exe`, `hp_v75.exe`, `hp_v75_m26.exe`, `hp_v76.exe`,
`hp_v77.exe`, `hp_v78.exe`, `hp_v79.exe`, `hp_v80.exe`, `hp_v81.exe`, `hp_v82.exe`, `hp_v83.exe`, `hp_v84.exe`.

Name new builds `hp_vNN.exe` or `hp_<flag>.exe`. 100 MB at mem 26 with
a cap other than the 8 MB default: `hp_vNN_m26.exe` and compile
`-DHP_SLOT_MAX=31` (not 35).

```
g++ -O3 -std=c++17 -I hp/include hp/src/main.cpp -o hp/build/hp_vNN.exe
hp/build/hp_vNN.exe c --mem 22 data/enwik8.8mb hp/build/e8_8mb_vNN.hp
hp/build/hp_vNN.exe d hp/build/e8_8mb_vNN.hp hp/build/e8_8mb_vNN.out
```

Write 100 MB archives to `%LOCALAPPDATA%\hp_lab` then copy into
`hp/build`. Harvest clones stay in `harvest/`; do not rewrite them.

---

## Next tests

**Priority after H33/H36.** The layer-1 rate is a *shared* parameter, so
correcting it changes every prior verdict. Mean per-feature dilution cost
fell 87.5 → 22.2 B (−75%) at scale 60. Work this order:

1. **Done.** `HP_LR1_SCALE=40` at `SLOT_MAX=35` → **v84** 1,680,397 / fx2 1,677,124 both RT PASS. 100 MB mem 26 not run.
2. Re-screen the 116 historical rejects at the corrected rate (2 MiB
   screen, 92 s each ≈ 3 h for the whole pile). They were all scored
   against an over-adapted mixer.
3. **Done (cheap protocol).** Per-mixer `HP_LR1_Ri` vs scale-40
   `{1,1,1,2,1,2}` at `SLOT_MAX=24`. 2 MiB base **439,496**. Paid:
   R0=2 **−123**, R0=3 **−35**, R5=1 **−22**. R3=4 **+219** (1/2/3 all
   worse; R5=4 skipped). 8 MiB s24 vs **1,681,094**: R0=2 **−27**,
   R0=3 **+566** (2 MiB false positive), R5=1 **−152**, sibling combo
   R0=2+R5=1 **−204**. None bytes-down vs v85-35 **1,680,191**; no
   `SLOT_MAX=35`. v89 retest: 2 MiB **439,146 (−107 vs 439,253)**;
   8 MiB s24 **1,679,749 (−13 vs 1,679,762)**; **no 35-cap**. Do not
   land `{2,1,1,2,1,1}`. See RECORD H40 / v89 combo.
4. **Sign-fix retest REJECT on 2 MiB.** `HP_MIXER_RANK=8` after the
   `l1k` sign clamp + `ufac_` init-to-w0 fix: **599,778 vs s_base
   439,496 (+160,282)** at `SLOT_MAX=24` / mem 22. Worse than +20 k;
   8 MB not run. Rank stays default-off. Do not claim champ accept
   (no `SLOT_MAX=35` bytes-down vs **1,680,191**). Do not sweep 4/16.
5. **Done (cheap protocol).** Null expert `HP_NULL_EXPERT` (default 0)
   duplicates always-on o1 `h2(1, hist_ & 0xffull)` as a second mixer
   input (separate StateMap, zero new information). 2 MiB s24
   **439,319 vs 439,253 (+66)**. Mixer dilution tax **+66 B**. Reject-pile
   re-score as (measured delta − 66) is a paper adjustment, not a
   reopen of H11–H43. Not in `v78_flags.ps1`. No 8 MiB. See RECORD.

One flag. Log every call in `RECORD.md`. After an 8 MB accept, recompile
remaining leftovers on the new champ.

| # | test | how | accept |
|---|---|---|---|
| **H1** | v58 leftover wave | nest / para / line / skip32 | **closed** — all accepted as v58–v61 |
| **H2** | Skip `HP_SENGRP_C0` | `#elif` after `HP_SENGRP_WORD` (already on) | no-op |
| **H3** | 100 MB off OneDrive, `MAX=31` | v61 **18,490,445** RT PASS | **closed** — new 100 MB champ |
| **H6** | Integer low-rank mixer | unfixed `SLOT_MAX=35` **REJECT +702,383** (2,407,863). Sign-fix 2 MiB s24 **REJECT +160,282** (599,778 vs 439,496); 8 MB not run | no — do not sweep 4/16 |
| **H7** | Wiki-axis 4-CM bundle | `HP_WIKI_AXES` **REJECT +432** (1,706,371) | split into singles |
| **H7a** | `HP_DOM_MOD` | sent_domain CM **REJECT +276** (1,706,215) | no |
| **H7b** | `HP_STATE_MOD` | wiki.state **1,705,480 / fx2 1,701,090 both RT PASS** | new 8 MB champ |
| **H7c** | `HP_HDR_MOD` | wiki_header CM **REJECT +288** (1,705,768) | no |
| **H7d** | `HP_DEPTH_MOD` | wiki.depth CM **REJECT +97** (1,705,577) | no |
| **H4** | Track W inventory | harvest notes 2026-09-12; **add cmix-obias / fx3** | written gaps only |
| **H8** | Drop PY mixer input | `HP_PY_EXPERT=0` **REJECT +18,224** (1,723,704) | keep PY |
| **H9** | Mixer precision / estimation | GPU sweep on frozen v57 dump | compass — Q4 x / Q6 W free; <32 experts not free |
| **H10** | Cluster-MoE of experts | GPU hierarchical mix **sizeable** (best 0.781 vs 0.254) | no leftover — do not flag |
| **H11** | v62 first-wave leftovers | skip40 **REJECT +52**; fccxt **REJECT +146 / +241 vs v69**; period **REJECT +6**; SR **REJECT +143**; baridx **REJECT +266**; pron **REJECT +122** | **closed** |
| **H12** | Dual-stage hash `HP_HASH_CHK` | **1,699,746 / 1.621 bpc RT PASS −5,734** / fx2 **1,695,486** | v64 rung |
| **H13** | Restack paying leftovers on HASH_CHK | TPL −3,179; DMC −110; LZP −283; skip-k −511; infokey −125; HASH2 −23 | **v70 1,695,515** |
| **H14** | New axes on v70 | skip-3 **−152**; link-pipe **−141**; skip-4 **−81**; cite **REJECT +187**; HASH_P5 **REJECT +423**; DMC_GROW **REJECT +1**; skip-5 **REJECT +8** | **v73 1,695,141** |
| **H15** | Wiki-domain CMs on v73 | CAT **−434** → v74; HEADING **−100** → v75 **1,694,607** / fx2 **1,690,955**; REDIR **+234**; EXTLINK **+521**; REFNAME **+300**; QOCXT **+363**; ENTITY **+309** | **v75 1,694,607** |
| **H16** | Indent / list / ISSE / magic / nowiki | vs pulled v75 **1,694,590**: indent **+613**; list **+488**; ISSE **+265**; magic **+480**; nowiki **+292** | all reject |
| **H17** | Dump XML CMs on v76 | title **v77 1,693,559 / fx2 1,689,873 both RT PASS −1,031/−1,055**; pageid restack **+311**; user **+267**; text **+473** | **v77** |
| **H18** | More dump XML on v77 | ns **+282**; dumpredir **+281**; ip **+390**; revcomment **+249** | all reject |
| **H19** | Dump minor / model on v77 | minor **+353**; model **+292** | all reject |
| **H20** | Wikitext CMs on v77 | sectitle **v78 1,692,024 RT PASS −1,535**; parserfn **+264/+286**; tableclass **+517** | **v78** |
| **H21** | Anchor / pub-id / temp-pos on v78 | anchor **+461**; pubid **+391**; temppos **+266** | all reject |
| **H22** | Richer wiki stacks / reorder / payload_lex / dict on v78 | wikistack **v79 1,690,064 RT PASS −1,960**; reorder **+3,022**; payload_lex **+3,735**; dict **+62,667** | **v79** |
| **H23** | Wiki-domain CMs on v79 | lang **+478**; catsort **+434**; tblrow **+478**; fileopt **+508**; defaultsort **+271**; redirtarget **+290**; dab **+557**; hatnote **+309** | all reject |
| **H24** | Sticky/dense wiki CMs on v79 | capmask **v80 1,689,846 RT PASS −218**; lastlink **+392**; fword **+304**; year **+168**; celltxt **+639**; httphost **+554**; paren **+517**; listpos **+589** | **v80** |
| **H25** | Word-shape CMs on v80 | shape **+655**; suffix **+230**; prefix **+711**; charcls **+501**; vowel **+145**; contr **+542**; hyphen **+555**; tokencls **+627** | all reject |
| **H26** | Layout/run CMs on v80 | runlen **+432**; wpos **+466**; blank **+335**; sprun **+414**; linelen **+277**; tagdist **+634**; markdist **+233**; uppergap **v81 1,689,247 RT PASS −599** | **v81** |
| **H27** | Wiki-domain CMs on v81 | month **+254**; gallery **+292**; seckind **+276**; citekind **+676**; tagname **+625**; colspan **+688**; style **+532**; coord **+271** | all reject |
| **H28** | Recency/length CMs on v81 | digitgap **+274**; dotgap **+149**; commagap **+256**; wordlen **v82 1,689,157 RT PASS −90**; sentlen **+187**; lowergap **+333**; digitpos **+322**; slashgap **+390** | **v82** |
| **H29** | Sticky last-run lengths on v82 | diglen **+324**; prevline **+326**; prevsent **+442**; linklen **+525**; tpllen **+479**; paralen **+511**; alnumlen **+672**; splen **+651** | all reject |
| **H30** | Title/heading word hits + initials/ordinal/unit/decimal/repeat/caseflip on v82 | titleword **+645**; headword **+633**; init **+579**; ordinal **+452**; unit **+476**; decimal **+340**; repeat **+684**; caseflip **+682** | all reject |
| **H31** | Wiki-domain layout on v82 | lead **+524**; infoval **+601**; linktrail **+280**; cellkind **+534**; tblcol **+480**; headidx **+246**; htmlfmt **+297**; infobox **+535** | all reject |
| **H32** | Wiki markup states on v82 | seclevel **+379**; brace3 **+285**; namedarg **+470**; include **+283**; sig **+275**; wikibold **v83 1,687,899 RT PASS −1,258**; urlpart **+286**; refidx **+145** | **v83** |
| **H33** | Wiki layout/markup on v83 | prespace **+612**; extdisp **+581**; pxsize **+478**; entnum **+149**; wikihr **+168**; fontcol **+554**; tbldepth **+503**; utf8st **+126** | all reject |
| **H34** | Wiki splits/punctuation on v83 | dlterm **+264**; headclose **+330**; wikitime **+152**; linkcomma **+456**; catblock **+467**; br **+161**; ampnbsp **+139**; mdash **+140** | all reject |
| **H35** | Wiki-domain / layout on v83 | math **+129**; listmix **+467**; protocol **+462**; hexrun **+457**; sqdepth **+220**; piperole **+615**; afterref **+134**; sentpos **+32** | all reject |
| **H36** | Punctuation/number FSMs on v83 | abbrev **+641**; thousand **+193**; refpunct **+155**; qperiod **+616**; ellipsis **+141**; numrange **+262**; deg **+151**; percent **+142** | all reject |
| **H37** | Wiki-domain / layout on v83 | statetrans flipped at scale 40 → **v88 1,679,112**; colring **+486**; listpara **+593**; secfrag **+249**; wikivar **+118**; subpage **+186**; linkns **+597**; fccur **+242** (fccur later 2 MiB −43 / s24 −187, no 35) | **v88** |
| **H38** | Paying-axis joints on v83/v84 | parast **+299**; boldst **+178**; headbold **+252**; boldline **+222**; cappara **+109**; nestpara **+51**; catpipe **+98**; headpara **+107** | all reject |
| **H39** | Wiki-domain leftovers on v84 | expectcl **+102**; refgroup **v85 1,680,191 RT PASS −206**; reflist / sister / convert / cn / block / pipetrick all 2 MiB kill | **v85** |
| **H40** | Wiki-domain leftovers on v85 | notes / langtpl / frac / listen / birth / hlist / mainart / chem | **closed** — all 2 MiB kill |
| **H41** | Wiki-domain leftovers on v87 | sfn / geotemp / epigraph / tracklist / succession / colstart / toc / refbegin | **closed** — all 2 MiB kill |
| **H42** | Wiki-domain leftovers on v88 | shortdesc **+64**; seealso **+62**; portal **+61**; authctl **+63**; usedate **+61**; ipa **+60**; goodart **+69**; caption **+59** | **closed** — all 2 MiB kill |
| **H43** | Wiki-domain leftovers on v89 | navbox **+59**; efoot **+61**; rshort **+56**; asof **+62**; clarify **+60**; currency **+56**; displaytitle **+52**; nowrap **+62** | **closed** — all 2 MiB kill |
| **H44** | Wiki-domain leftovers on v89 | stub **+64**; persondata **+54**; flag **+56**; quotebox **+65**; clear **+58**; imdb **+61**; rp **+60**; fn **+59** | **closed** — all 2 MiB kill |
| **H45** | Wiki-domain leftovers on v89 | small **+63**; supsub **+58**; precode **+64**; taxobox **+74**; nihongo **+57**; deadlink **+58**; wayback **+62**; rowspan **+62** | **closed** — all 2 MiB kill |
| **H46** | Wiki-domain leftovers on v89 | unref **+66**; cleanup **+55**; npov **+59**; rfrom **+56**; doi **+56**; pmid **+60**; isbnmod **+72**; medal **+58** | **closed** — all 2 MiB kill |
| **H47** | Wiki-domain leftovers on v89 | thumb **+62**; further **+64**; death **+57**; harv **+60**; issn **+64**; oclc **+59**; alignmod **+62**; syntax **+60** | **closed** — all 2 MiB kill |
| **H49** | Mixer/APM knobs on v89 | skip56 **v90** 1,678,417; skip24 **+172**; skip32 **+69**; skip48 s24 **−308**; lr30/35 **−39** (v89); lr45 **0**; lr50 **+374**; w0one **+50,270**; wb4 **+139,363** | **v90** |
| **H50** | Mixer knobs on v90 | msc75 **v91** 1,676,677; lr30 35-cap **−356**; skipl1 s24 **−327**; skip64/72/80 / apm6/8 / msc112 reject | **v91** |
| **H51** | Mixer-scale neighbors + lr30/skipl1 on v91 | msc* all reject; lr30 −4 reject; skipl1 **v92** 1,676,394 | **v92** |
| **H52** | SKIP_L1 neighbors + lr30/skip48 on v92 | sl80 **v93** 1,675,993; sl16/24/32/48 reject; sl56/64/72 s24 only | **v93** |
| **H53** | SKIP_L1 neighbors of 80 on v93 | sl88/96/104/112/120/128/144/160; maybe sl72 confirm | 2 MiB |

v62 wiki-axis singles closed (only STATE paid). H6 rank-8 **REJECT +702k**. H8 nopy **REJECT +18k**. Mixer width and low-rank W are closed.
H9: mixer dots are already more precise than they need; wall time is the 77 StateMaps, not Q16.
H10: clustering experts then mixing cluster opinions loses ~3× vs one linear W. Hard MoE is worse. Phase B dump-with-gates is not promoted. LSTM router stays Track W.
H11/H13 leftover wave **closed** on v70. H14 new-axis wave **closed** on v73 (skip-3/4 + link-pipe paid; cite / 5-probe / DMC grow / skip-5 rejected). Do not reopen rank-W / cluster-MoE / pairwise / mean-mix / skip40 / fccxt / period / SR / baridx / pronoun / HASH_P5 / skip-5.
H15 **closed** on v75: Category-namespace **−434** and heading-level **−100** paid; `#REDIRECT` / `[http` / ref-name / quote-CM / entity-name rejected. Do not reopen those five.
H16 **closed**: indent / list-level / ISSE / magic / nowiki all rejected vs pulled-tree v75 **1,694,590**. Pulled RAM/speed tree is **v76** 8 MB **1,694,590 RT PASS** (−17 vs old v75). Not bit-identical. Do not reopen H16 flags.
H17 **closed** on v77: `<title>` **−1,031** paid; first-`<id>` isolated −924 but **+311** on title champ; username **+267**; in-`<text>` **+473**. Do not reopen those three. Do not reopen H16.
H18 **closed**: dump `<ns>` / `<redirect/>` / `<ip>` / `<comment>` all rejected vs v77 **1,693,559**. Do not reopen. Do not reopen H17 pageid/user/text or `#REDIRECT`.
H19 **closed**: `<minor/>` **+353**; `<model>` **+292**. Dump-XML extras after title all dilute. Do not add more dump-tag CMs.
H20 **closed** on v78: heading-body text **−1,535** paid; `{{#` parser-fn **+264/+286**; `{| class=` **+517**. Do not reopen parserfn/tableclass. Do not add more dump-tag CMs.
H21 leftovers **closed**: anchor/pubid/temppos all reject vs v78. H22 **closed** on v79: wikistack **−1,960**; reorder **+3,022**; payload_lex **+3,735**; dict **+62,667**. Do not reopen those three. H23 **closed**: lang **+478**; catsort **+434**; tblrow **+478**; fileopt **+508**; defaultsort **+271**; redirtarget **+290**; dab **+557**; hatnote **+309**. Do not reopen. H24 **closed** on v80: capmask **−218**; lastlink **+392**; fword **+304**; year **+168**; celltxt **+639**; httphost **+554**; paren **+517**; listpos **+589**. Do not reopen those seven. H25 **closed**: shape **+655**; suffix **+230**; prefix **+711**; charcls **+501**; vowel **+145**; contr **+542**; hyphen **+555**; tokencls **+627**. Do not reopen.
H26 **closed** on v81: uppergap **−599**; runlen **+432**; wpos **+466**; blank **+335**; sprun **+414**; linelen **+277**; tagdist **+634**; markdist **+233**. Do not reopen those seven. H27 **closed**: month **+254**; gallery **+292**; seckind **+276**; citekind **+676**; tagname **+625**; colspan **+688**; style **+532**; coord **+271**. Do not reopen. H28 **closed** on v82: wordlen **−90**; digitgap **+274**; dotgap **+149**; commagap **+256**; sentlen **+187**; lowergap **+333**; digitpos **+322**; slashgap **+390**. Do not reopen those seven. H29 **closed**: diglen **+324**; prevline **+326**; prevsent **+442**; linklen **+525**; tpllen **+479**; paralen **+511**; alnumlen **+672**; splen **+651**. Do not reopen. H30 **closed**: titleword **+645**; headword **+633**; init **+579**; ordinal **+452**; unit **+476**; decimal **+340**; repeat **+684**; caseflip **+682**. Do not reopen. H31 **closed**: lead **+524**; infoval **+601**; linktrail **+280**; cellkind **+534**; tblcol **+480**; headidx **+246**; htmlfmt **+297**; infobox **+535**. Do not reopen. H32 **closed** on v83: wikibold **−1,258**; seclevel **+379**; brace3 **+285**; namedarg **+470**; include **+283**; sig **+275**; urlpart **+286**; refidx **+145**. Do not reopen those seven. H33 **closed**: prespace **+612**; extdisp **+581**; pxsize **+478**; entnum **+149**; wikihr **+168**; fontcol **+554**; tbldepth **+503**; utf8st **+126**. Do not reopen. H34 **closed**: dlterm **+264**; headclose **+330**; wikitime **+152**; linkcomma **+456**; catblock **+467**; br **+161**; ampnbsp **+139**; mdash **+140**. Do not reopen. H35 **closed**: math **+129**; listmix **+467**; protocol **+462**; hexrun **+457**; sqdepth **+220**; piperole **+615**; afterref **+134**; sentpos **+32**. Do not reopen. H36 **closed**: abbrev **+641**; thousand **+193**; refpunct **+155**; qperiod **+616**; ellipsis **+141**; numrange **+262**; deg **+151**; percent **+142**. Do not reopen. H37 **closed** then **reopened at scale 40**: statetrans **v88 1,679,112**; colring **+486**; listpara **+593**; secfrag **+249**; wikivar **+118**; subpage **+186**; linkns **+597**; fccur **+242** (later 2 MiB −43). Do not reopen the seven rejects. H38 **closed**: parast **+299**; boldst **+178**; headbold **+252**; boldline **+222**; cappara **+109**; nestpara **+51**; catpipe **+98**; headpara **+107**. Do not reopen joints of paying leftover CMs. No mem 26 (55 GB).
H39 **closed** on v85: expectcl **+102**; refgroup **v85 1,680,191 RT PASS −206**; reflist / sister / convert / cn / block / pipetrick all 2 MiB kill. Do not reopen. H40 leftover CMs **closed** on v86 2 MiB: notes **+56**; langtpl **+61**; frac **+57**; listen **+68**; birth **+59**; hlist **+51**; mainart **+60**; chem **+52**. No 8 MiB gate. Do not reopen. H40 per-mixer rates **closed** on cheap protocol: combo R0=2+R5=1 8 MiB s24 **1,680,890 (−204 / still +699 vs v85-35)**. No mem 26. No SLOT_MAX=35 until RAM is free for that encode. H41 leftover CMs **closed** on v87 2 MiB: sfn **+63**; geotemp **+58**; epigraph **+58**; tracklist **+52**; succession **+54**; colstart **+49**; toc **+72**; refbegin **+57**. No 8 MiB gate. Do not reopen. Not CITE / COORD / BLOCK / TABLECLASS / ANCHOR / MAGIC / REFLIST twins. Do not reopen H11–H41. H42 leftover CMs **closed** on v89 2 MiB: shortdesc **+64**; seealso **+62**; portal **+61**; authctl **+63**; usedate **+61**; ipa **+60**; goodart **+69**; caption **+59**. No 8 MiB gate. Do not reopen. H43 leftover CMs **closed** on v89 2 MiB: navbox **+59**; efoot **+61**; rshort **+56**; asof **+62**; clarify **+60**; currency **+56**; displaytitle **+52**; nowrap **+62**. No 8 MiB gate. Do not reopen. H44 leftover CMs **closed** on v89 2 MiB: stub **+64**; persondata **+54**; flag **+56**; quotebox **+65**; clear **+58**; imdb **+61**; rp **+60**; fn **+59**. No 8 MiB gate. Do not reopen. H45 leftover CMs **closed** on v89 2 MiB: small **+63**; supsub **+58**; precode **+64**; taxobox **+74**; nihongo **+57**; deadlink **+58**; wayback **+62**; rowspan **+62**. No 8 MiB gate. Do not reopen. H46 leftover CMs **closed** on v89 2 MiB: unref **+66**; cleanup **+55**; npov **+59**; rfrom **+56**; doi **+56**; pmid **+60**; isbnmod **+72**; medal **+58**. No 8 MiB gate. Do not reopen. Not twins of CN / DEADLINK / HATNOTE / UNREF / REDIR / DUMPREDIR / CITEKIND / PUBID / SUCCESSION / GEOTEMP. Do not reopen H11–H46.
H47 leftover CMs **closed** on v89 2 MiB: thumb **+62**; further **+64**; death **+57**; harv **+60**; issn **+64**; oclc **+59**; alignmod **+62**; syntax **+60**. No 8 MiB gate. Do not reopen. Not twins of FILEOPT / EXTDISP / PXSIZE / GALLERY / SEEALSO / MAINART / HATNOTE / BIRTH / MONTH / SFN / CITE / CITEKIND / ISBN / PUBID / DOI / STYLE / COLSPAN / CELLKIND / ROWSPAN / PRECODE / NOWIKI / MATH. Do not reopen H11–H47.
H49 mixer/APM knobs: skip56 **v90** 1,678,417 (−455); `HP_MIXER_SKIP=56` ON. skip24 **+172**; skip32 **+69**; skip48 s24 **−308** (no land). lr45 **0**; lr50 **+374**; w0one **+50,270**; wb4 **+139,363**. Did not overwrite `hp_v83.exe`–`hp_v89.exe`.
H50 mixer knobs: msc75 **v91** 1,676,677 (−1,740); `HP_MIXER_SCALE=49152` ON. lr30 35-cap **1,678,061 (−356)** (superseded). skipl1 s24 **−327** (no 35; re-screen on v91). skip64 **+3**; skip72 **+29**; skip80 **+51**; apm6 **+782**; apm8 **+6**; msc112 **+266**. Did not overwrite `hp_v83.exe`–`hp_v90.exe`.
H51 mixer knobs: skipl1 **v92** 1,676,394 (−283); `HP_MIXER_SKIP_L1=40` ON. msc50 **+12,033**; msc62 **+197**; msc69 **+29**; msc72 **+8**; msc78 **+9**; msc81 **+24**; msc88 **+89**; msc94 **+200**; lr30 **−4**. Did not overwrite `hp_v83.exe`–`hp_v91.exe`.
H52 mixer knobs: sl80 **v93** 1,675,993 (−401); `HP_MIXER_SKIP_L1=80` ON. sl16 **+41**; sl24 **+25**; sl32 **+15**; sl48 **−11**; lr30 **−15**; skip48 **+1**; sl56 s24 **−169**; sl64 s24 **−269**; sl72 s24 **−354**. Did not overwrite `hp_v83.exe`–`hp_v92.exe`.
H53 mixer knobs **open** on v93 2 MiB vs **438,789**: SKIP_L1 neighbors of 80 (sl88/96/104/112/120/128/144/160) plus maybe sl72 confirm. Ten 2 MiB at a time. No mem 26. Do not reopen skip>56 / APM rate 6/8 / msc* / W0 / WB4.
v75 100 MB **18,409,708 RT PASS** (−25,032 vs v73). v77-100 **18,370,971 RT PASS** (−38,737 vs v75). Skip-k saturates at 4. LSTM / WRT / `payload_lex` / POS / bitlstm32 / obias are Track W. v78-100 mem 26 `SLOT_MAX=31` in flight (8 MB idle).

---

## GPU trial queue (3090, 24 GB, compass only)

Full board: canvases/gpu-trial-queue.canvas.tsx. Codec stays integer.
Do not estimate the mix. Do not retry H6 / H10 / pairwise / mean-mix.

80k-record dump is ~12 MB — the GPU is starved. First job after H8
(RAM free): stride-1 v62 dump into `%LOCALAPPDATA%\hp_lab` (int16 ≈ 10.5 GB,
fits). Then run Wave 1 on that dump, not another 80k toy.

| wave | job | dump | kill if |
|---|---|---|---|
| **G0** | stride-1 v62 dump + gate ids + layer-1 logits | hp, mem 22, off OneDrive | write fails |
| **G1** | residual mix, C(77,3), integer W, Q-MoE by \|x\|, APM knots, online Q16 replay | G0 or current | sizeable vs 0.254 |
| **G2** | context-conditional W (few buckets), skip-table classifier | G0 with gates | dilution / sizeable |
| **G3** | function skip: predict quiet StateMap from c0/wiki/match | G0 with n0/n1 | cannot skip without the table |
| **G4** | LSTM / 1-layer transformer ceiling on logits | G0 stride-1 | cannot beat 0.254 → Track W only |

Wide experimental net (Mandelbrot/fractal, SM, DMC, grammar, LLM, fxcm
gaps): canvases/wide-trial-net.canvas.tsx. Most items are GPU compass or
Track W. Promote at most one integer leftover if compass is free/visible.

---

## Track W — winner fork

Hutter’s FAQ: combine with SOTA. Do not invent a third self-extracting
pipeline. Use theirs. Track H will not hit S < 109,685,197 this month.

| id | status |
|---|---|
| W1 | clones already in `harvest/cmix-lex` and `harvest/fx2-cmix` |
| W2–W3 | inventory written 2026-09-12 (H4) |
| W4 | time/RAM envelope still open |
| W5 | after H accept: land dedicated wiki-axis CMs one at a time; not mixer gates or hp-only folds |
| W6 | **cmix-obias** (Freelan, claimed S **108,492,825**): 256-cell LSTM, bitlstm32 fp16 head, PPMd logit prior `g=0.15`. Fork of lex. Do **not** vendor into `hp/` (floats / SSE rcpps). Inventory only. |
| W7 | **fx3-cmix** (Orav, unsubmitted S **109,735,627**, 0.95%): not the fork target; encode.su says not actively worked. PPM shrunk 14 GB → 1.75 GB. |

---

## Track C — Cypha only

Work happens in the Cypha tree. hp already has binary decomposition,
logistic mixing, integer tables, and SHA round-trips. The think-tank
mixer rewrite is a Cypha fix, not an hp port.

**Do (in Cypha):** C1 `n_eval` ≥ 100k + 95% CI; C2 log-loss mixer
gradient `w_i (p_i/p_mix − 1)`; then C3 drop `neural_weight_floor`,
C4 binary logistic mix, C5 hash-pointer match, C6 nibble-bucketed
tables, C7 integer-only RT, C8 external baselines, C9 ablation.

**Do not:** merge Cypha LM / Qt / REST / CUDA into `hp/`; quote 2.664
bpc next to hp SHA numbers; treat the prize as a revenue line.

---

## Do not retest

Pointer is the RECORD heading unless noted. New evidence only.

| item | evidence |
|---|---|
| CTW / case-word / stacked word-match-2/3 / `pat_`+bracket | RECORD A.3 |
| B.3 pattern library / dict | PATTERNS.md; H1.4 +33,369 |
| discovery slot/eval sweep | RECORD H0.2: all reject |
| H2.2 section mute | +15,052 on 8 MB |
| H2.1 stem as extra experts | +288 vs v5 |
| H2.7 pred-gate | +6,928 on 1 MB |
| recency fold / wt3 / fword gate | RECORD v9 leftovers |
| `HP_WORD_GRP` | RECORD v46 leftovers: +1,885 |
| `HP_SEN_GROUP` mixer gate | v46 leftovers: +680 (dilution) |
| `HP_LINK_GRP` | RECORD v54 leftovers: +78 |
| `HP_SENGRP_POS` | RECORD v55 leftovers: 0 |
| `HP_NUM_GRP` | RECORD v55 leftovers: +69 |
| `HP_MATCH_20` | RECORD v57 leftovers: **+97** |
| `HP_HEBB_GRP` | RECORD v57 leftovers: **+90** |
| `HP_SLOT_WORD10` | +43,179 (2^32 sparse) |
| `HP_SLOT_O34G` / `HP_MATCH_GROW2` | −87 noise |
| `HP_MATCH_12` | +100 vs v43 |
| 100 MB at `SLOT_MAX=35` | OOM (this plan, RAM table) |
| layer-2 `--lr` (mixer_lr) | RECORD H33: default 2 optimal, monotone worse above |
| `HP_MIXER_CLAMP_BITS` 18/20/22/24 | RECORD H33: byte-identical; clamp not binding |
| `HP_MIXER_BACKPROP` | RECORD H33: 890,133 vs 441,538 — catastrophic |
| `HP_MIXER_NLMS` (energy-normalised step) | RECORD H33: worse at every ETYP |
| pair/group stacking of near-miss rejects | RECORD H35: 0 synergistic pairs, additive or interfering |
| GPU / nncp / Qt in hp | illegal for Hutter |
| recursive compression | Hutter FAQ |

---

## Doc map

This is the only plan file. Scoring-board history is `RECORD.md`, not
another PLAN.

| file | role |
|---|---|
| **PLAN.md** (this file) | living board |
| **RECORD.md** | measurement log only |
| **README.md** | build / run |
| HUTTER_RESEARCH.md | literature |
| review-and-thinktank.md | Cypha Track C only |
| harvest/*_notes.md | Track W gaps; not the hp queue |
| MODELS.md AXES.md PATTERNS.md UPGRADES.md hp/UPGRADES.md hp/README.md | archived inventories |
| Stats for Compression/gating-statistics-full-report.md | archived study; hp leftover mixer gates rejected |
