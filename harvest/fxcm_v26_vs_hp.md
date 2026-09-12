# W3 — fxcm_v26 vs hp (unported list)

Source: `harvest/cmix-lex/src/models/fxcmv1.cpp` (Orav fxcm_v26 inside
cmix-lex). hp column is this tree after the 2026-08-22 ports.

| fxcm_v26 piece | hp | axis | port? |
|---|---|---|---|
| Wiki states (table/link/template/bar) | `wiki.hpp` | markup | have |
| FIRSTUPPER / isParagraph / header `>` | `wiki.hpp` | layout | have |
| linkword × 2104 | packed in `tag_` / `link_` | link vs body | partial |
| senword (body-only) | packed in `link_` when no link | prose | partial |
| 4 stemmed word streams + POS types | 1 sparse stream | morphology | **no — H2.1** |
| Pronoun word type | no | syntax | no |
| Sentence memory (64 sentences, similar-sentence match) | no | long-range prose | **no — big** |
| Separate sentence groups: prose / list / table / wikilink | no | domain | no |
| Partial sentence contexts | no | syntax | no |
| Dictionary codeword-aware compares | no (dict off) | lexicon | after H1.4 |
| skipSeeExternal on ==References/See also/Bibliography/External links== | `HP_SECTION_MUTE` (new) | section gate | **testing** |
| isMath / isNowiki / isPre (escaped `&lt;math`) | `HP_SECTION_MUTE` | domain gate | **testing** |
| isCategory after `[[Category:` | mute until `]]` in SECTION_MUTE | wiki | testing |
| Predictor on/off by table/link/bracket | no | mixer | H2.7 |
| 3 ContextMaps 32/64/128-byte slots | one map | estimator | H2.9 |
| Multiple state tables | one | estimator | H2.10 |
| Sparse UTF-8 match gap 1–2 | no | UTF-8 | H2.3 |
| LSTM expected-byte mixer ctx | no | neural mix | H3.3 |
| Dict-index mixer context | no | decode state | needs WRT |
| Online reverse dictionary | no | lexicon | P3 |
| payload_lex tail permute | no | metadata order | P5 / Track W |

Priority if we stay on Track H: H2.1 stemmer streams, sentence memory,
then H2.7 on/off. If the cheque is the goal, land SECTION_MUTE only after
A.3, then put accepted ideas on cmix-lex.

---

## 2026-09-12 H9 remaining W gaps (hp v54 vs fxcm_v26 / cmix-lex)

Table above is 2026-08-22. Since then hp landed linkword/senword experts,
sentence memory, sen-group folds + `HP_SENGRP_MOD`, word-keyed match ×3+wm4,
SPARSE_UTF8, and rejected SECTION_MUTE / stem-as-experts / pred-gate /
`HP_SEN_GROUP` mixer gate / `HP_SENT_DOM`. Remaining **unported vs the
fork**, tagged by axis. Do not vendor Cypha.

### preprocess

- PHDA9 split + wiki transform + packaged article-order (P1/P4). hp only
  *tests* `data/enwik8.fx2man`; it does not run their pipeline.
- WRT dictionary + online reverse dict + codeword-aware compares (P2/P3).
  `dict.hpp` off; fxcm `loaddict` / `codeword2sym` / decoded-word CMs
  need this stream.
- payload_lex regime-1 D99 sort + R1ORD3 Lehmer side data (P5). Last
  published 1%. Exists only after WRT.

### wiki transform (in-model markup; still thin vs `fxcmv1.cpp`)

- `fccxt` / `brcxt` / `qocxt` / `htcxt` stacks + `ColumnContext` 4-row
  cell ring. hp `wiki.hpp` is a nibble machine; `HP_TABLE_ABOVE` /
  `HP_NLCHAR` / `HP_WIKI_TEMP` rejected or no-op on leftover.
- FIRSTUPPER / charSwap alphabet. hp uses raw `A–Z` for paragraph.
- `sets()` skip on `skipSeeExternal` / math / nowiki / pre / Category.
  H2.2 dummy-hash mute **rejected** (+15k/8 MB). Fork skips maps, not
  the word hash.

### mixer

- LSTM ByteMixer (`predictor.cpp` `byte_mixer_`; `lstmpr`/`lstmex`).
  cmix-lex consumes it in the outer mixer; lex `fxcmv1.cpp` declares
  `lstmex` but does not use it as `mxA` ctx (fx2-cmix did). hp: none.
- Dict-index mixer ctx (`mxA[8].cxt=deccode`). Needs WRT.
- Predictor on/off by table/link/bracket (`sets()`). H2.7 **rejected**.
- 18 `Mixer1` contexts from POS/stream bits (`stream2b`/`stream3b`/
  `FcIdx`/`BrFcIdx`). hp extra gates are a different family.

### match

- `{1}` word-keyed match with run-map + DirectHash (fx2 five-key set).
  hp has `{0}`, `{1,3}`, `{7,2}`, plus `HP_WMATCH_4` `{1,2,3}`.
- `MatchModel2` hashing `worcxt.Word(1)` into the **byte** match table.
- Outer 10 sparse word-context configs (hp `wordstream.hpp`: 2 of 10).
- cmix extra byte-match orders `{2,8}{15,2}{17,2}{20,1}{25,1}`.

### LSTM (neural mix; same as mixer LSTM above)

- Full LSTM mixer is fork-owned. Not an hp→W5 port. Last on Track W
  after preprocess.

### still missing NLP (syntax / morphology)

- Porter2 + four POS-filtered `WordsContext` streams (`worcxt` /
  `worcxt1` / `worcxt2` / `worcxt3` + undecoded `worcxt0`). H2.1 extra
  experts **rejected**; stem-fold into `word_` **rejected**.
- Pronoun word type (`Pronouns[14][5]`, flag `1<<20`).
- Partial sentence contexts: `wt3cxt` / `wt3cxtW` / `wt4cxtW` (POS-type
  hash and codeword hash of the open sentence, nouns excluded on one).
- Gap removal of `[word|display]` / `{{a=b|c}}` / `()` from sentence
  rings (`removeWordsL`/`R`).
- Four independent `SentenceContext` rings (`sencxt` / `sencxtL` /
  `sencxtT` / `sencxtCL`). hp `HP_SENT_DOM` **rejected**; hp uses one
  ring + sen-group **folds** instead.
- Extra stationary + ContextMap3/4 (32/64/128-byte slots); multiple
  state tables. Estimator, not an axis.

### W5: which H keeps map onto the fork vs hp-only

Fork already **has** grouped sentence memory (four rings) and wiki
state in CMs. It does **not** have a dedicated ContextModel whose
context is `sen_group + last-3-bytes`.

| H keep | flag | map? |
|---|---|---|
| dedicated sengrp expert | `HP_SENGRP_MOD` (−3,116) | **W5 candidate** — new CM on (domain, c4); not a mixer gate |
| sengrp × word fold | `HP_SENGRP_WORD` (−1,979) | second-wave after SENGRP_MOD on the fork |
| sen-group folds into existing experts | `HP_WSTR_GRP` `HP_WBI_GRP` `HP_SMEM_GRP` `HP_COL_GRP` `HP_TAG_GRP` `HP_SENWORD_GRP` | **hp-only** — compensates for one ring / generic tables; fork already routes by domain |
| sen-group mixer gate | `HP_SEN_GROUP` | **do not port** — rejected (dilution); fork already `sets()` |
| 4-ring sent-mem | `HP_SENT_DOM` | fork already has this; hp rejected the slim port |
| sent-mem / senword experts | `HP_SENT_MEM` `HP_SENWORD` | fork already richer; not hp→fork |
| slot growth | `HP_SLOT_SGRP` | hp-only estimator size |
