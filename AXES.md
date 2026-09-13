<!--
ARCHIVED. Superseded by repo-root PLAN.md (board) and README.md (how-to).
Coverage matrix remains historically useful; live champs and protocol live there.
-->

# AXES — coverage matrix

An axis is an information direction, not a model. Adding a second model on a
saturated axis is how effective rank stalls at 3.34 / 28.

| axis | hp (this tree) | fx2-cmix | cmix | notes |
|---|---|---|---|---|
| byte suffix o1–o6 | yes, + PY + CTW | yes, many more orders | yes | **saturated** in hp |
| sparse / skip | {1,3}{2,4} + 12 discovered masks + skip-k templates | 10+ hand masks | many | hp now searches; still fewer than fx2 |
| word | folded unigram | 4 streams | yes | hp stream 0 |
| word-bigram | `wbi_` | yes | yes | have |
| capitalisation / first-char | **new** `wordstream.hpp` | yes | some | landed |
| sentence / paragraph | stream 3 after `.!?\\n` | para flag in gates | some | landed, weak |
| 2-D column | `col_` | yes | some | have |
| XML / tag depth | was generic; **now wiki machine** | hardcoded enwik states | XML | landed a/A.2.a |
| wiki table / link / template | **WIKITABLE, SQUARE, HTLINK, BAR, CURLY, linkword** | yes | no | landed |
| byte-keyed match | 5 orders (3,4,6,10,16) | 5+ | yes | have; top-decorrelated |
| word-keyed match | **3 orders** | 5 | some | landed a/A.2.b |
| Hebbian association | yes | no | no | hp-original; keep |
| bracket / nest | **yes** | 256 ctx depth 15 | some | landed a/A.2.e |
| pattern class (table/cite/time/infobox) | **yes** `pat_` + cache taxonomy | implicit in wiki states | no | landed B.2 |
| PPM unbounded order | PY backoff + CTW; PPMD off | order-25 PPMD | no | likely overlap |
| UTF-8 state | no | some paq8px | no | skip (enwik is ASCII-heavy) |
| numeric / timestamp | pattern class only | some | some | partial |
| stored dictionary | `dict.hpp` off | 412 KB english.dic | some | **B.3 NO-GO** pending enwik8 |
| LSTM mix | no | no | **yes — cmix edge** | last item |
| article reorder | no | starlit in some entries | no | 4.2 later |

## Saturation map (from prior hp measurement + this design)

Already dense, do not add more without a rank *increase*: byte suffix, generic
match-at-order-N, a second XML depth counter.

Still thin after this session (must *measure*): wiki structure vs generic tag
(we replaced, not added — rank should rise if the axis got sharper),
word-keyed match, first-char/case, bracket, pattern class.

Mixer work (NCL, per-mixer lr, Hedge-L1) can raise or lower rank without
adding an axis. Validate on **both** rank and bpc (arXiv 2301.11323).
