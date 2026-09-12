# Wiki state machine, word streams, word-keyed match

Port notes extracted from cloned `harvest/fx2-cmix` (kaitz/fx2-cmix, shallow clone succeeded).
Primary file: `src/models/fxcmv1.cpp`. Outer word/match wiring: `src/predictor.cpp` + `src/context-manager.cpp`.

`hp` already has a generic XML tag model (`tag_`: depth + in-tag + tag-name hash). That expert is the most decorrelated in the ensemble. This file is the upgrade spec: replace/augment it with the fx2 first-char / table / link state machine. Do not invent a second XML-depth counter.

All bytes below are **after WRT / charSwap**. Punctuation is remapped:

```
COLON='J'  SEMICOLON='K'  LESSTHAN='L'  EQUALS='M'  GREATERTHAN='N'
QUESTION='O'  CURLYOPENING='P'  VERTICALBAR='Q'  CURLYCLOSE='R'
FIRSTUPPER=64 '@'   SQUAREOPEN=91   SQUARECLOSE=93
HTLINK=31   HTML=30   LF=10   ESCAPE=12   UPPER=7   TEXTDATA=96
```

`charSwap()` (fxcmv1.cpp ~2204) swaps `{|}~` with `PQRS` and xors some punctuation with `0x70`. If `hp` does not run WRT, keep the **logical** names (`|`, `{`, `<`, `>`) and map at the detector, not the raw enwik bytes.

---

## 1. Wiki-state symbol list (exact)

From `fxcmv1.cpp` 1772–1925:

```cpp
#define COLON         'J' // :
#define SEMICOLON     'K' // ;
#define LESSTHAN      'L' // <
#define EQUALS        'M' // =
#define GREATERTHAN   'N' // >
#define QUESTION      'O' // ?
#define FIRSTUPPER     64 // @ - wrt first char in word is in upper case
#define SQUAREOPEN     91 // [
#define BACKSLASH      92 // '\'
#define SQUARECLOSE    93 // ]
#define CURLYOPENING  'P' // {
#define VERTICALBAR   'Q' // |
#define CURLYCLOSE    'R' // }
#define APOSTROPHE    39  // '
#define QUOTATION     34  // "
#define SPACE         32
#define HTLINK        31  // http link
#define HTML          30
#define LF            10
#define ESCAPE        12
#define UPPER          7  // Upper case word
#define TEXTDATA      96  // Any other char, probably text

#define WIKIHEADER GREATERTHAN   // '>'  — article-header / filtered-wiki newline
#define WIKITABLE  '-'           // table mode; newline char becomes '-'
```

There is **no C enum**. States live in three stacks plus flags:

| object | type | what it holds |
|---|---|---|
| `fccxt` | `BracketContext<U8>` over `fchar[]` | first-char / wiki-mode stack |
| `brcxt` | `BracketContext<U8>` over `brackets[]` | `() {} [] <>` nesting |
| `qocxt` | `BracketContext<U8>` over `quotes[]` | `'` and `"` |
| `htcxt` | `BracketContext<U16>` over `html[]` | `&L` / `&N` HTML-entity |
| `colcxt` | `ColumnContext` | line, column, table cells |
| `isParagraph` | int | 1 after `FIRSTUPPER` line start |
| `isTemp` | bool | inside `{{template}}` |
| `isMath` / `isNowiki` / `isPre` / `isText` | bool | XML tag regions |
| `nlChar` | U8 | `LF`, `WIKITABLE`, or `WIKIHEADER` |
| `fc` | int | first char of current line |
| `linkword` / `senword` / `firstWord` | U32 | hashes, see §4 |

Tracked first-char / bracket alphabet (used as `fccxt` elements):

```cpp
const U8 brackets[8] = {'(',')', CURLYOPENING,CURLYCLOSE, '[',']', LESSTHAN,GREATERTHAN};
const U8 quotes[4]   = {APOSTROPHE,APOSTROPHE, QUOTATION,QUOTATION};
const U8 fchar[20]   = {
  FIRSTUPPER,LF, TEXTDATA,LF, COLON,LF, LESSTHAN,GREATERTHAN, EQUALS,LF,
  SQUAREOPEN,SQUARECLOSE, CURLYOPENING,CURLYCLOSE, '*',LF, VERTICALBAR,LF,
  HTLINK,LF
};
const U16 html[2]    = {'&'*256+'L', '&'*256+'N'};
```

`fchar` pairs are (open, close). `LF` as closer means “this first-char lasts until newline”. `LESSTHAN` closes on `GREATERTHAN` (XML tag). `SQUAREOPEN` closes on `SQUARECLOSE` (wiki link). `CURLYOPENING` closes on `CURLYCLOSE` (template/table).

Index maps (byte → 1..7, 0 if unseen):

```
fcy[] bracket/quote → BrFcIdx:  "→5  '→6  L(<)→4  P({)→2  [→3  (→1  Q(|) unused here
fcq[] first-char    → FcIdx:    @→1  P({)→2  J(:)→3  L(<)→4  M(=)→5  *→6  Q(|)→7  [→2
```

---

## 2. First-char state machine (`fccxt` + `fc` + `isParagraph`)

Update order, once per byte, `bpos==0` (`modelPrediction`, ~3730–4192):

1. `colcxt.Update(c1, c4&0xffffff)` — may set `nlChar`, `isTemp`, `NL`, `fc` of the new line.
2. `brcxt.Update(c1)` only if `c1 < 'a'` (letters do not pop brackets).
3. On new line (`colcxt.isNewLine()`):
   - two near-empty lines → reset `fccxt`, `brcxt`, `qocxt`, `htcxt`
   - `fc = colcxt.lastfc()`
   - if `fc == WIKIHEADER` (`>`) → `fccxt.Reset()`
   - `isParagraph = (fc == FIRSTUPPER)`
   - `fccxt.Update(fc)`
4. If `col>2 && c1>FIRSTUPPER && !isMath` and `c1<128`: `fccxt.Update(c1)` after these pops:
   - `[xx|xx]` / `{xx|xx}` ended: while `fccxt.cxt==VERTICALBAR` push `LF` (pop bars)
   - http/category link ended: while `COLON` or `HTLINK` push `LF`
5. `category:` / `wikipedia:` after `COLON` → `fccxt.Update(LF)`, drop that word.
6. `//` after `COLON` → pop colon, push `HTLINK` (switch `[word://` to `[http://`).
7. Line started `[` then `] ` → treat rest as paragraph: `fc=FIRSTUPPER`, `isParagraph=1`, reset `fccxt`, push `FIRSTUPPER`.
8. Leading spaces: first non-space becomes `fc`; `FIRSTUPPER` ⇒ paragraph.
9. `*` list: first non-space after `*` becomes `fc`.
10. `&` + `<` → `fc=HTML`.
11. Bold/italic `''` / `'''` then space, not in a list → paragraph start (`FIRSTUPPER`).
12. Last three bytes `J//` (`://`) → `fc=HTLINK`.
13. List-to-paragraph: `-` after `*` first-char, or `] ` / `],` after `*` → `isParagraph=1`, `fc=FIRSTUPPER`.

`isParagraph` is the **link-vs-body** switch used everywhere below: paragraph mode uses word hashes; non-paragraph uses column/table cells.

---

## 3. Table / header column machine (`ColumnContext`)

```cpp
// Table open/close — expects WRT-swapped {{|  and  |}}
if ( b2 == ((CURLYOPENING<<16)+(CURLYOPENING<<8)+VERTICALBAR) ) nlChar = WIKITABLE;
else if ( b2 == ((VERTICALBAR<<16)+(CURLYCLOSE<<8)+CURLYCLOSE) ) nlChar = LF, resetCells();

// Template (single { not {{)
if (byte!=CURLYOPENING && (b2&0xff00)==(CURLYOPENING<<8) && (b2&0xff0000)!=(CURLYOPENING<<16))
    isTemp = true;
else if (isTemp && byte==CURLYCLOSE) isTemp = false;

// Newline
if (byte==LF) { rotate row ring of 4; reset current row; fc=0; }
else {
    push byte;
    if (collen()==2) {  // first char of line
        fc = min(byte, TEXTDATA);
        NL = true;
        if (fc==GREATERTHAN && !isPre) nlChar = WIKIHEADER;
        if (fc==SQUAREOPEN && nlChar==WIKIHEADER) nlChar = LF;
    }
}
```

Wiki table syntax recognised (comment in source):

```
{|  table start     → nlChar = WIKITABLE
|+  caption         (not implemented)
|-  new row         → (b2&0xffff)==(WIKITABLE + VERTICALBAR*256)  i.e. "-|"
! / !!  header      (not implemented)
| / ||  data cell   → "||"  or  "\n|"
|}  table end       → "|}}"  → nlChar=LF, reset cells
```

Cell tracking: 4-row ring, up to 32 cell start positions per row. `abovecellpos` walks the cell **above** the current one (2-D, but cell-aligned, not just byte-above). That is a different axis from `hp`’s `col_` (byte at same column of previous line).

`WIKIHEADER` mode (`>` as newline, filtered wiki): new row on `>\n`; new cell on each `>`.

---

## 4. `linkword` and `senword` (exact hash)

```cpp
// while accumulating a letter in a word:
word0 = word0*2104 + j;          // j = current letter
if (brcxt.cxt==SQUAREOPEN && fccxt.cxt!=HTLINK && fc!=HTML)
    linkword = linkword*2104 + j;
if (isParagraph && fccxt.cxt!=HTLINK && colcxt.isTemp==false)
    senword = senword*2104 + j;

// resets
if (linkword && c1==COLON) linkword = 0;          // [category:...]
if (c1==SQUARECLOSE)       linkword = 0;          // wiki link ended
if (c1=='.' || c1==',')    senword  = 0;
if (c1=='(' || c1==')')    senword  = 0;
if (c1==COLON)             senword  = 0;
if (Conjunction POS)       senword  = 0;
```

Context use (`cmC2[9]` / `cmC[3]`):

```cpp
if (linkword)      cmC2[9].set(linkword);
else if (isMath)   cmC2[9].sets();          // skip
else if (senword)  cmC2[9].set(senword*1471 + c1);
else               cmC2[9].set(0);          // unless HTML / '<'

cmC[3].set( ((linkword ? linkword : word0)*3301 + number0*3191) );
```

`2104 = 263*8`. This is a **gapless hash of the whole `[wiki link]`** as one word. `hp` has no equivalent.

Sentence-end dot is ignored (does **not** reset `worcxt`) when inside `[]`, `()`, a table, or a `*` list.

---

## 5. Three `WordsContext` streams (inside fxcm)

Not the same as the outer `words_[0..7]` array. These are POS-filtered word rings (256 slots).

| stream | object | what is pushed | reset |
|---|---|---|---|
| sentence | `worcxt` | every stemmed word | LF, `;`, `.` (unless in `[]`/`()`/table/list) |
| paragraph | `worcxt1` | word unless type is Conjunction/Article/Male/Female/Number/ConjunctiveAdverb; skip if inside `<` | LF, `" - "` |
| typed | `worcxt2` | only words that have a POS type, excluding the `worcxt1` types plus Adposition/AdverbOfManner | (not reset on LF in the same way) |

Each slot stores: surrounding bytes, POS `Type`, stem hash, capitalisation flag. `fword` = first word of the sentence.

Gap removal (do not let link/template internals pollute the sentence):

```cpp
worcxt.removeWordsL(8, '(', ')');
worcxt.removeWordsL(8, SQUAREOPEN, VERTICALBAR);   // [word|display]
worcxt.removeWordsL(8, LESSTHAN, COLON);
if (colcxt.isTemp) worcxt.removeWordsR(10, EQUALS, VERTICALBAR);  // {{a=b|c}}
worcxt.removeWordsL(8, LESSTHAN, GREATERTHAN);
```

POS flags (`EngWordTypeFlags`): Verb, Noun, Adjective, Plural, PastTense, PresentParticiple, AdverbOfManner, Suffix, Prefix, Male, Female, Article, Conjunction, Adposition, Number, ConjunctiveAdverb. Stemmer is Porter2 English (`EnglishStemmer`), from paq8px.

---

## 6. Outer word streams + word-keyed match (predictor.cpp)

`ContextManager::words_` has **8 slots**. Update (`context-manager.cpp`):

```
words_[7]  letter-only (a–z or ≥0x80), hash *997*16; reset on non-letter
words_[0]  alnum + {8,6,≥0x80}, hash *997*16, 28-bit
words_[1]  same charset, hash *263*32
on word end: words_[6]=words_[5] … words_[2]=words_[1]; words_[1]=0
```

That is four logical streams: current-alnum (`0`), current-alnum-alt (`1`), previous-word ring (`2..6`), letter-only (`7`).

**10 sparse word-context configs** (Indirect / nonstationary, delta=200):

```
{0}
{0, 1}
{1}
{1, 2}
{1, 3}
{2, 3}
{3, 4}
{1, 2, 4}
{2, 3, 4}
{2}
```

**5 word-keyed match models** (history 2e6, limit 200, delta 0.5):

```
{0}          current word
{1}          current word alt  + run-map + (cmix only) DirectHash
{1, 3}       current + word-before-previous
{1, 2, 3}    current + last two completed
{7, 2}       letter-stream + last completed word
```

`hp` has one word hash + one word-bigram, and **no** word-keyed match.

---

## 7. Byte-keyed match orders (for comparison)

fx2-cmix `AddMatch()` — `ContextHash(order, bits)`:

```
{0, 8}   {1, 8}   {7, 4}   {11, 3}   {13, 2}
```

cmix adds `{2,8} {15,2} {17,2} {20,1} {25,1}`.

Inside fxcm, `MatchModel2` also hashes `worcxt.Word(1)` into the same match table (word-keyed candidate). `SparseMatchModel` uses 4 sparse hashes, minLen {3,4,6,5}, one with stride 2 — mostly UTF-8.

`hp` match bank: orders **3, 4, 6, 10, 16**, BYTE-keyed only.

---

## 8. Bracket model (outer, not fxcm)

`src/models/bracket.cpp` + `AddBracket()`:

- Pairs (after WRT): `()` `PR` (`{}`) `[]` `LN` (`<>`) `''` `""`
- Stack limit 10, distance limit 200, stats 100000
- Predicts the **closing** byte; 256-context Direct + Indirect on `(bracket, distance)`
- `BracketContext`: 256 distance × 15 stack, size `257 * 256`

`hp` tag model is XML-depth only. This is the **bracket/nesting** axis.

---

## 9. UTF-8 continuation (fxcm)

```cpp
if (c2==ESCAPE) {          // WRT-escaped high byte
    if (utf8left==0) {
        if      ((c1>>5)==6)    utf8left=1;   // 2-byte
        else if ((c1>>4)==0xE)  utf8left=2;   // 3-byte
        else if ((c1>>3)==0x1E) utf8left=3;   // 4-byte
        else utf8left=0;
    } else {
        utf8left--;
        if ((c1>>6)!=2) utf8left=0;           // not a continuation
    }
}
```

When `utf8left != 0`, most word/sentence context maps are **skipped** (`sets()`). `hp` has no UTF-8 axis.

---

## 10. Numeric field (fxcm)

```
number0 / numlen0   current number (max 19 digits)
number1 / numlen1   previous number
mybenum             1 or 2  →  N.N  /  N,N  decimal
numbers             bit-ring of “was digit”
```

Comma between digits rotates current→previous. Used as `word00 + number0*191 + numlen0` and in `cmC[3]` with `linkword/word0`.

---

## 11. Capitalisation

- WRT emits `FIRSTUPPER` (`@`) before a capitalised word and `UPPER` (7) for ALL-CAPS.
- `WordsContext.capital[]` stores per-word “started with FIRSTUPPER”.
- `fc == FIRSTUPPER` is the paragraph detector.
- paq8 `TextModel` also tracks `lastUpper`, `maskUpper` (32-byte bitmask), `firstLetter`.

---

## 12. Minimum port into `hp` tag model

Replace `tag_` context in `predictor.hpp::set_byte_contexts()` with a hash of:

```
fccxt.cxt          (first-char / wiki mode)
isParagraph        (0/1)
nlChar             (LF / WIKITABLE / WIKIHEADER)
brcxt.cxt          (innermost bracket)
linkword?1:0       (inside a wiki link that is not http)
isTemp / isMath    (optional, cheap)
```

Keep the existing XML depth as a **secondary** nibble; do not drop it. Then add `linkword = linkword*2104 + j` as its own `ContextModel` (new axis). Word-keyed match is a separate port (`MatchModel` keyed on `word_hash_` instead of `hist_`).

Do not copy mixer rates or LSTM. Those are not axes.
