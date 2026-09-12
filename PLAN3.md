<!--
ARCHIVED. Superseded by root PLAN.md. Do not add new experiments here.
Scoring-board rows are historical; append new results to RECORD.md only.
-->

# PLAN3 — tests that can win the Hutter Prize

Generated from `HUTTER_RESEARCH.md`. This is the experiment list.
`PLAN.md` was harvest. `PLAN2.md` was axis purity. This is the prize.

**Official bar:** S < 109,685,197 vs L = 110,793,128 (1%).
**If cmix-lex awards:** S < ~108,553,546 vs new L ≈ 109,650,047.

**hp lab protocol (until Track W is built):**

- corpus A: `data/enwik8.1mb` (fast, lies sometimes — PLAN2)
- corpus B: first 8 MB of `data/enwik8`
- corpus C: full `data/enwik8` (100 MB)
- gate: no_float, round-trip SHA, then bpc **or** entropy rank
- record binary: `hp_v3b.exe` on 1 MB; **v4 wins at 8 MB** (H0.4)

Two tracks run in parallel. Track W is how you get a cheque.
Track H is how we know *which* 1% to steal.

---

## Track W — winner fork (the cheque)

| id | test | why | how | accept |
|---|---|---|---|---|
| W1 | Clone + build `cmix-lex` and `fx2-cmix` | current / pending SOTA | `harvest/cmix-lex`, existing `harvest/fx2-cmix` | binaries run, README numbers recorded |
| W2 | Diff fx2 vs lex (`payload_lex`, `R1ORD3`, fxcm_v26) | last 1.03% lives here | `changes.md` + source diff | written inventory in harvest notes |
| W3 | Diff fxcm_v26 vs our `wiki.hpp` / streams | what we still lack in-model | read fxcm, list unported ctx | port list with axis tags |
| W4 | Time/RAM envelope on this machine | can we even run a 50 h job | 1 MB / 8 MB dry run if Linux avail | go / no-go for full enwik9 |
| W5 | After a keep from Track H, land it on the fork | Hutter: combine with SOTA | one change, one enwik8 first | enwik8 bytes down, then enwik9 |

Do not invent a third self-extracting pipeline. Use theirs.

---

## Track H — hp laboratory

### Tier 0 — do immediately (hours, already unlocked)

| id | test | files | protocol | accept / reject |
|---|---|---|---|---|
| H0.1 | Memory curve 20…28 (30 if RSS < 10 GB) | `Config::normalize` | v3b on 1 MB, then 8 MB at best mem | keep the largest mem that still round-trips; **this was the only 1 MB win** |
| H0.2 | Discovery sweep `kSlots` × `kEvalBytes` | `discover.hpp` | {12,16,24,32} × {256,512,1024,2048} on 1 MB mem 22 | keep iff 1 MB **and** 8 MB both improve |
| H0.3 | Per-flag A.3 of the v3b bundle | `features.hpp` | 8 rebuilds × 1 MB | drop any flag that costs bits at mem 22 |
| H0.4 | 8 MB / 32 MB truth check of PLAN2 v4 | `hp_v4.exe` | same mem as v3b | if v4 wins at 8 MB, PLAN2 was right and 1 MB lied |
| H0.5 | Corpus profile of enwik8/9 | new `tools/corpus_profile.py` | counts: articles, tables, links, digits, templates, `<math>` | drives P1/P4/X7 priority |

### Tier 1 — preprocess (largest published prize deltas)

| id | test | files | protocol | accept / reject |
|---|---|---|---|---|
| H1.1 | Article extract + title sort restore | new `preproc/reorder.*` | split enwik8 on `<page>`, shuffle, restore by title | restore bit-identical |
| H1.2 | Cheap reorder: cluster by first heading / category / infobox class | same | 8 MB and 100 MB through **v3b** | keep if C bytes fall > 0.3% |
| H1.3 | Import starlit / fx2 order file and apply to enwik9 later | `harvest/starlit/.../new_article_order` | identity of permutation only | file ships; decoder is sort |
| H1.4 | WRT/DRT **retest at 100 MB** | `dict.hpp` | enwik8 mem 22, dict on vs off | keep only if S2 drop > dict storage; proxy result is void |
| H1.5 | phda9 / fx2 wiki transform (single pass) | port from harvest | 8 MB round-trip, then 100 MB | keep if C bytes fall |
| H1.6 | payload_lex-style tail permute | after W2 | only once H1.5 exists | keep if C bytes fall |

### Tier 2 — unported winner models (flags, A.3)

| id | test | files | protocol | accept / reject |
|---|---|---|---|---|
| H2.1 | paq8pxd stemmer + 4 POS types | new `stemmer.hpp` | streams 2–4 as in fx2 README | rank + bpc on 1 MB **and** 8 MB |
| H2.2 | Mute word models in `<math>`/`<pre>`/`<nowiki>` | `wiki.hpp` | 1 MB + 8 MB | keep if bits or µs/byte fall |
| H2.3 | Sparse UTF-8 match (gap 1–2, min 3–6) | `wordmatch.hpp` or new | 1 MB | keep if rank up or bpc down |
| H2.4 | Word-match keys `{0}`, `{1,3}`, `{7,2}` not 1/2/3 | `wordmatch.hpp` | replace stack | keep if mean\|r\| of extras < 0.85 |
| H2.5 | `senword` own expert; linkword stays | `predictor.hpp` | A.3 | keep if not twin of `tag_` |
| H2.6 | Wiki gate = `state + 16*isParagraph` | mixer ctx | 1 MB vs v3b | keep if bpc down |
| H2.7 | Predictor on/off by table/link/bracket | `wiki.hpp`+models | 1 MB | keep if bpc down |
| H2.8 | Numeric model **alone** on v3b (not in the v4 drop bundle) | `numeric.hpp` | 1 MB + 8 MB | PLAN2 bundled it with deletions; isolate |
| H2.9 | Three hash slot sizes 32/64/128 | `statemap.hpp` | mem 22 vs 26 | keep if 8 MB falls |
| H2.10 | Second state table | `statemap.hpp` | after H2.9 | reject if r > 0.9 vs first |

### Tier 3 — mixer / time / S1

| id | test | files | protocol | accept / reject |
|---|---|---|---|---|
| H3.1 | Skip mixer update if \|error\| < θ | `mixer.hpp` | θ sweep, 1 MB time + bpc | keep if time ↓ and bpc ≤ +0.001 |
| H3.2 | PPMD o25 mmap at `--mem` high | new / flag | 8 MB first | keep if not twin of PY (A.3) **and** RSS < 10 GB |
| H3.3 | LSTM mixer (UPGRADES 2.4) | last | only after H0.1 time known | legal neural mix; integer or fixed-point |
| H3.4 | UPX + runtime state tables | S1 | when we have a submission binary | S1 down, decode identical |
| H3.5 | clang PGO build | build | vs g++ -O2 1 MB | keep if time ↓ |

### Tier 4 — do not test again unless new evidence

| id | item | evidence |
|---|---|---|
| Z1 | CTW | r = 0.930 vs o2:py |
| Z2 | case-preserving word stream | r = 0.944 vs word:py |
| Z3 | stacked word-match-2/3 | r = 0.969 |
| Z4 | `pat_` + bracket together | r = 0.971 |
| Z5 | B.3 dictionary on 3 MB proxy | +1,498 B |
| Z6 | GPU / nncp port | illegal |
| Z7 | recursive compression | FAQ counting argument |
| Z8 | more o1–o6 twins | saturated |

---

## Execution order (this session and next)

1. H0.5 corpus profile (drives everything).
2. W1 clone cmix-lex.
3. H0.1 memory 28 on 1 MB (v3b).
4. H0.4 8 MB v3b vs v4 (does PLAN2 become true at scale?).
5. H1.1–H1.2 article split / cheap reorder on 8 MB.
6. H0.2 discovery sweep (compile-time, one flag pair at a time).
7. H2.2 mute-in-math (small patch, winner-documented).
8. W2–W3 written inventory.
9. H1.4 dictionary on full enwik8 (long).
10. Track W land only what H accepted.

---

## Scoring board (fill as we run)

| id | corpus | mem | bytes | bpc | vs v3b | call |
|---|---|---:|---:|---:|---|---|
| v3b record | 1 MB | 26 | 229,440 | 1.750 | 0 | keep |
| v3b | 100 MB | 20 | 21,828,153 | 1.746 | n/a | baseline C |
| H0.5 | 1 / 8 / 100 MB | — | see RECORD | — | 1 MB is 84% redirects | **1 MB gate is hostile** |
| H1.2 8 MB identity | 8 MB | 22 | 1,806,540 | 1.723 | 0 | baseline |
| H1.2 8 MB redir-last | 8 MB | 22 | 1,803,104 | 1.720 | **−3,436** | keep |
| H1.2 8 MB fx2-manual | 8 MB | 22 | **1,800,840** | **1.717** | **−5,700 (−0.32%)** | **accept** |
| H0.1 v3b mem 28 | 1 MB | 28 | 229,437 | 1.750 | −3 vs mem 26 | **plateau — stop** |
| H1.2 full fx2-manual | 100 MB | 26 | **19,354,445** | **1.548** | −2,473,708 vs mem20 original | **accept, RT PASS** |
| H0.4 v4 + fx2-man | 100 MB | 26 | running | | vs 19,354,445 v3b | |
| H0.2 d16/e1024 | 1 MB | 22 | 232,505 | 1.774 | +354 | reject |
| H0.2 d24/e1024 | 1 MB | 22 | 232,694 | 1.775 | +543 | reject |
| H0.2 d32/e1024 | 1 MB | 22 | 232,959 | 1.777 | +808 | reject |
| H0.2 d12/e512 | 1 MB | 22 | 232,354 | 1.773 | +203 | reject (least bad) |
| H0.2 d12/e2048 | 1 MB | 22 | 232,499 | 1.774 | +348 | reject |
| H2.2 mute | 1 MB | 22 | 234,276 | 1.787 | +2,125 | **reject** |
| H2.2 mute | 8 MB | 22 | 1,821,592 | 1.737 | +15,052 | **reject** |
| H0.4 v4 | 1 MB | 22 | 232,446 | 1.773 | +295 | 1 MB still says reject |
| H0.4 v4 | 8 MB | 22 | **1,805,009** | **1.721** | **−1,531** | **accept — 1 MB lied** |
| H3.1 skip24 | 1 MB | 22 | 232,323 | 1.773 | +172 | 1 MB noise |
| H3.1 skip24 | 8 MB | 22 | **1,803,239** | **1.720** | **−3,301** | **accept** |
| H2.3 utf8 match | 1 MB | 22 | 232,331 | 1.773 | +180 | 1 MB noise |
| H2.3 utf8 match | 8 MB | 22 | **1,803,601** | **1.720** | **−2,939** | **accept** |
| H1.5 entity fold | 8 MB | 22 | 1,805,841 | n/a | **−699** archive | weak keep |
| H1.4 dict | 8 MB | 22 | 1,839,909 | 1.755 | +33,369 | **reject** |
| H2.8 numeric alone | 8 MB | 22 | **1,805,219** | **1.722** | **−1,321** | **accept** |
| H0.3 wiki off | 8 MB | 22 | 1,811,764 | 1.728 | +5,224 | **keep wiki** |
| H0.3 word-match off | 8 MB | 22 | 1,809,939 | 1.726 | +3,399 | **keep wm** |
| H0.3 bracket off | 8 MB | 22 | 1,814,096 | 1.730 | +7,556 | **keep bracket** |
| v5 = v4+skip+utf8 | 8 MB | 22 | **1,798,335** | **1.715** | **−8,205 vs v3b / −6,674 vs v4** | keep |
| H2.1 stem experts | 8 MB | 22 | 1,798,623 | 1.715 | +288 vs v5 | **reject** |
| H2.7 pred-gate | 1 MB | 22 | 238,777 | 1.821 | +6,928 vs v5 | **reject** |
| H2.5 senword | 8 MB | 22 | **1,796,738** | **1.713** | **−1,597 vs v5** | **accept — v6** |
| v5 + fx2 + mem26 | 100 MB | 26 | **19,271,085** | **1.542** | −83,360 vs v3b | **100 MB champ, RT PASS** |
| H2.5 senword v6 | 8 MB | 22 | **1,796,738** | **1.713** | −1,597 vs v5 | **accept** |
| v6 + fx2 | 8 MB | 22 | **1,790,934** | **1.707** | −1,615 vs v5+fx2 | **stacks** |
| gate argmax | 8 MB | 22 | **1,796,939** | **1.714** | −1,396 vs v5 | **accept** |
| gate shape6 | 8 MB | 22 | 1,798,126 | 1.715 | −209 vs v5 | weak; does not stack on v7 |
| gate branch3 | 8 MB | 22 | 1,798,521 | 1.715 | +186 vs v5 | **reject** |
| **v7 = v6+argmax** | 8 MB | 22 | **1,795,356** | **1.712** | **−2,979 vs v5** | keep |
| **H2.9 slot sizes v8** | 8 MB | 22 | **1,786,531** | **1.704** | **−8,825 vs v7** | keep (shrink o1 hurts) |
| v8 + fx2 | 8 MB | 22 | **1,780,777** | **1.698** | −8,773 vs v7+fx2 | stacks |
| wordpos gate | 8 MB | 22 | 1,783,378 | 1.701 | −3,153 vs v8 | **accept** |
| slot-grow | 8 MB | 22 | 1,785,824 | 1.703 | −707 vs v8 | **accept** |
| state2 PY cap | 8 MB | 22 | 1,786,223 | 1.704 | −308 vs v8 | **accept** |
| **v9 grow+wpos+st2** | 8 MB | 22 | **1,782,343** | **1.700** | **−4,188 vs v8** | **new 8 MB champ** |
| v9 identity RT | 8 MB | 22 | — | — | SHA match | **PASS** |
| **v9 + fx2** | 8 MB | 22 | **1,776,517** | **1.694** | **−4,260 vs v8+fx2** | **stacks** |
| recency fold | 8 MB | 22 | 1,818,028 | n/a | +35,685 vs v9 | **reject** |
| wt3 trail | 8 MB | 22 | 1,784,008 | n/a | +1,665 vs v9 | **reject** |
| fword gate | 8 MB | 22 | 1,783,021 | n/a | +678 vs v9 | **reject** |
| **v10 grow+3** | 8 MB | 22 | **1,781,301** | **1.699** | **−1,042 vs v9** | **accept, RT PASS** |
| v10 + fx2 | 8 MB | 22 | **1,775,459** | **1.693** | **−1,058 vs v9+fx2** | **stacks** |
| wiki-temp | 8 MB | 22 | 1,782,343 | 1.700 | 0 vs v9 | **reject** |
| **sent-stream** | 8 MB | 22 | **1,778,093** | **1.696** | **−4,250 vs v9** | **accept** |
| utf8-idle | 8 MB | 22 | 1,782,210 | 1.700 | −133 vs v9 | weak |
| match-o8 | 8 MB | 22 | 1,781,893 | 1.699 | −450 vs v9 | weak keep |
| **v11 = v10+s3** | 8 MB | 22 | **1,777,044** | **1.695** | **−5,299 vs v9** | **accept — independent stack** |
| v11 + fx2 | 8 MB | 22 | **1,771,335** | **1.689** | **−4,124 vs v10+fx2** | **stacks** |
| table-above | 8 MB | 22 | 1,782,463 | n/a | +120 vs v9 | **reject** |
| nlchar | 8 MB | 22 | 1,782,343 | n/a | 0 vs v9 | **reject** |
| **v12 = v11+m8** | 8 MB | 22 | **1,776,601** | **1.694** | **−443 vs v11** | **accept, RT PASS** |
| v12 + fx2 | 8 MB | 22 | **1,770,904** | **1.689** | **−431 vs v11+fx2** | **stacks** |
| v6 + fx2 mem26 | 100 MB | 26 | **19,240,021** | **1.539** | **−31,064 vs v5** | **accept** |
| **v12 + fx2 mem26** | 100 MB | 26 | **19,056,065** | **1.524** | **−183,956 vs v6** | **accept** |
| **v13 = v12+quote** | 8 MB | 22 | **1,775,314** | **1.693** | **−1,287 vs v12** | **accept, RT PASS** |
| v13 + fx2 | 8 MB | 22 | **1,769,611** | **1.688** | **−1,293 vs v12+fx2** | **stacks** |
| brk closer | 8 MB | 22 | 1,776,604 | n/a | +3 vs v12 | **reject** |
| match o1 | 8 MB | 22 | **1,772,984** | **1.691** | **−2,330 vs v13** | **accept** |
| **v14 word+2** | 8 MB | 22 | **1,769,276** | **1.687** | **−6,038 vs v13** | **accept, RT PASS** |
| v14 + fx2 | 8 MB | 22 | **1,763,629** | **1.682** | **−5,982 vs v13+fx2** | **stacks** |
| **v15 = v14+o1** | 8 MB | 22 | **1,766,973** | **1.685** | **−2,303 vs v14 / −8,341 vs v13** | **accept, RT PASS** |
| v15 + fx2 | 8 MB | 22 | **1,761,328** | **1.680** | **−2,301 vs v14+fx2** | **stacks** |
| **v16 word+3** | 8 MB | 22 | **1,761,967** | **1.680** | **−5,006 vs v15** | **accept, RT PASS** |
| v16 + fx2 | 8 MB | 22 | **1,756,400** | **1.675** | **−4,928 vs v15+fx2** | **stacks** |
| sentst +2 | 8 MB | 22 | **1,760,123** | **1.678** | **−1,844 vs v16** | **accept** |
| **v17 word+4** | 8 MB | 22 | **1,758,260** | **1.677** | **−3,707 vs v16** | **accept, RT PASS** |
| v17 + fx2 | 8 MB | 22 | **1,752,596** | **1.671** | **−3,804 vs v16+fx2** | **stacks** |
| **v18 = v17+s3** | 8 MB | 22 | **1,756,449** | **1.675** | **−1,811 vs v17** | **accept** |
| v18 + fx2 | 8 MB | 22 | **1,750,870** | **1.670** | **−1,726 vs v17+fx2** | **stacks, RT PASS** |
| **v19 word+5** | 8 MB | 22 | **1,753,764** | **1.672** | **−2,685 vs v18** | **accept, RT PASS** |
| v19 + fx2 | 8 MB | 22 | **1,748,266** | **1.667** | **−2,604 vs v18+fx2** | **stacks** |
| **word+6** | 8 MB | 22 | **1,752,055** | **1.671** | **−1,709 vs v19** | **accept** |
| **v20 word+7 cap29** | 8 MB | 22 | **1,751,028** | **1.670** | **−2,736 vs v19** | **accept, RT PASS** |
| v20 + fx2 | 8 MB | 22 | **1,745,545** | **1.665** | **−2,721 vs v19+fx2** | **stacks** |
| **v21 = v20+o2** | 8 MB | 22 | **1,749,276** | **1.668** | **−1,752 vs v20** | **accept, RT PASS** |
| v21 + fx2 | 8 MB | 22 | **1,743,820** | **1.663** | **−1,725 vs v20+fx2** | **stacks** |
| **v22 = v21+wm4** | 8 MB | 22 | **1,748,366** | **1.667** | **−910 vs v21** | **accept, RT PASS** |
| v22 + fx2 | 8 MB | 22 | **1,742,937** | **1.662** | **−883 vs v21+fx2** | **stacks** |
| **v23 word+8 cap30** | 8 MB | 22 | **1,747,742** | **1.667** | **−624 vs v22** | **accept, RT PASS** |
| v23 + fx2 | 8 MB | 22 | **1,742,354** | **1.662** | **−583 vs v22+fx2** | **stacks** |
| **v24 word+9 cap31** | 8 MB | 22 | **1,747,417** | **1.666** | **−325 vs v23** | **accept, RT PASS** |
| v24 + fx2 | 8 MB | 22 | **1,742,026** | **1.661** | **−328 vs v23+fx2** | **stacks** |
| **v25 = v24+wstr2** | 8 MB | 22 | **1,747,228** | **1.666** | **−189 vs v24** | **accept, RT PASS** |
| v25 + fx2 | 8 MB | 22 | **1,741,830** | **1.661** | **−196 vs v24+fx2** | **stacks** |
| wstr +2 on v23 | 8 MB | 22 | 1,747,539 | 1.666 | −203 vs v23 | weak keep |
| match o7 | 8 MB | 22 | 1,747,670 | 1.666 | −72 vs v23 | reject |
| wm5 | 8 MB | 22 | 1,747,694 | 1.666 | −48 vs v23 | reject |
| wm4 on v20 | 8 MB | 22 | **1,750,081** | **1.669** | **−947 vs v20** | **accept** |
| word+8 cap30 | 8 MB | 22 | 1,750,402 | 1.669 | −626 vs v20 | weak keep |
| sentst +3 | 8 MB | 22 | 1,750,777 | 1.670 | −251 vs v20 | weak |
| **match o2** | 8 MB | 22 | **1,752,016** | **1.671** | **−1,748 vs v19** | **accept** |
| match o5 | 8 MB | 22 | 1,753,118 | 1.672 | −646 vs v19 | weak keep |
| **wm4** | 8 MB | 22 | **1,752,846** | **1.671** | **−918 vs v19** | **accept** |
| **v18 + fx2 mem26** | 100 MB | 26 | **18,964,991** | **1.517** | **−91,074 vs v12** | **accept, RT PASS** |
| **v25 + fx2 mem26** | 100 MB | 26 | **18,821,118** | **1.506** | **−143,873 vs v18** | **accept, RT PASS** |
| **v26 = v25+sentmem** | 8 MB | 22 | **1,746,513** | **1.665** | **−715 vs v25** | **accept, RT PASS** |
| v26 + fx2 | 8 MB | 22 | **1,741,193** | **1.660** | **−637 vs v25+fx2** | **stacks** |
| sent-mem big | 8 MB | 22 | 1,746,512 | 1.665 | −1 vs v26 | reject |
| sent-align | 8 MB | 22 | 1,746,535 | 1.665 | +22 vs v26 | reject |
| sentmem +2 | 8 MB | 22 | 1,746,518 | 1.665 | +5 vs v26 | reject |
| **v27 = v26+s4** | 8 MB | 22 | **1,746,264** | **1.665** | **−249 vs v26** | **accept, RT PASS** |
| v27 + fx2 | 8 MB | 22 | **1,740,950** | **1.660** | **−243 vs v26+fx2** | **stacks** |
| m05 on v26 | 8 MB | 22 | **1,745,955** | **1.665** | **−558 vs v26** | **accept** |
| **v28 = v27+o5** | 8 MB | 22 | **1,745,709** | **1.665** | **−555 vs v27** | **accept, RT PASS** |
| v28 + fx2 | 8 MB | 22 | **1,740,100** | **1.659** | **−850 vs v27+fx2** | **stacks** |
| match o9 | 8 MB | 22 | 1,745,725 | 1.665 | +16 vs v28 | reject |
| sent-dom | 8 MB | 22 | 1,745,812 | 1.665 | +103 vs v28 | reject |
| **v29 = v28+o34** | 8 MB | 22 | **1,741,838** | **1.661** | **−3,871 vs v28** | **accept, RT PASS** |
| v29 + fx2 | 8 MB | 22 | **1,736,210** | **1.656** | **−3,890 vs v28+fx2** | **stacks** |
| **v30 = v29+o6** | 8 MB | 22 | **1,738,930** | **1.658** | **−2,908 vs v29** | **accept, RT PASS** |
| v30 + fx2 | 8 MB | 22 | **1,733,365** | **1.653** | **−2,845 vs v29+fx2** | **stacks** |
| o34b on v29 | 8 MB | 22 | **1,739,111** | **1.658** | **−2,727 vs v29** | **accept** |
| **v31 = v30+o34b** | 8 MB | 22 | **1,736,303** | **1.656** | **−2,627 vs v30** | **accept, RT PASS** |
| v31 + fx2 | 8 MB | 22 | **1,731,055** | **1.651** | **−2,310 vs v30+fx2** | **stacks** |
| o1/o2 +1 | 8 MB | 22 | 1,736,197 | 1.656 | −106 vs v31 | weak keep |
| **v32 = v31+o6b** | 8 MB | 22 | **1,733,103** | **1.653** | **−3,200 vs v31** | **accept, RT PASS** |
| v32 + fx2 | 8 MB | 22 | **1,727,635** | **1.647** | **−3,420 vs v31+fx2** | **stacks** |
| o34c on v31 | 8 MB | 22 | **1,734,691** | **1.654** | **−1,612 vs v31** | **accept, stack** |
| **v33 = v32+o34c** | 8 MB | 22 | **1,731,587** | **1.651** | **−1,516 vs v32** | **accept, RT PASS** |
| v33 + fx2 | 8 MB | 22 | **1,726,421** | **1.646** | **−1,214 vs v32+fx2** | **stacks** |
| match grow | 8 MB | 22 | 1,731,251 | 1.651 | −336 vs v33 | weak keep |
| **v34 = v33+o6c** | 8 MB | 22 | **1,728,615** | **1.649** | **−2,972 vs v33** | **accept, RT PASS** |
| v34 + fx2 | 8 MB | 22 | **1,723,212** | **1.643** | **−3,209 vs v33+fx2** | **stacks** |
| **v35 = v34+mg** | 8 MB | 22 | **1,728,317** | **1.648** | **−298 vs v34** | **accept, RT PASS** |
| sparse +1 | 8 MB | 22 | 1,733,058 | 1.653 | −45 vs v32 | reject |
| **v31 + fx2 mem26** | 100 MB | 26 | **18,752,700** | **1.500** | **−61,416 vs v27** | **encode done, RT in flight** |
| v27 + fx2 mem26 | 100 MB | 26 | **18,814,116** | **1.505** | **−7,002 vs v25** | **accept, RT PASS** |
| sent-cur | 8 MB | 22 | 1,745,833 | 1.665 | +124 vs v28 | reject |
| sentst +3 on v25 | 8 MB | 22 | 1,746,973 | 1.666 | −255 vs v25 | weak |
| utf8-idle on v25 | 8 MB | 22 | 1,747,072 | 1.666 | −156 vs v25 | weak |
| num +2 | 8 MB | 22 | 1,766,903 | n/a | −70 vs v15 | reject |
| **v37 + fx2 mem26** | 100 MB | 26 | **18,671,091** | **1.494** | **−81,609 vs v31** | **accept, RT PASS** |
| **v45 = v44+sentgrp** | 8 MB | 22 | **1,717,483** | **1.637** | **−2,313 vs v44 / −7,493 vs v37** | **8 MB champ, RT PASS** |
