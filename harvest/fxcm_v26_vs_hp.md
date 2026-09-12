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
