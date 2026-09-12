# PATTERNS — detected classes and B.3 decision

## Taxonomy (B.2)

Detected online from already-coded bytes (`PatternCache::observe_byte`).
Counts are filled at runtime; `--profile` does not yet print them. A later
hook can dump `class_count[] / bytes`.

| class | id | detector | intended use |
|---|---:|---|---|
| plain | 0 | default | prose |
| table row | 1 | `in_table` / `\|` / WIKITABLE / VERTICALBAR | wiki tables |
| citation | 2 | rolling window `ref` | `<ref>`, {{cite}} |
| timestamp | 3 | digits + colon, line length < 24 | `12:30`, ISO dates |
| infobox | 4 | CURLY `{{` state | templates |
| link | 5 | SQUAREOPEN or HTLINK | `[[...]]`, `http://` |
| markup | 6 | inside XML tag | `<...>` |
| numeric | 7 | current word first-class = digit | fields, years |

`pat_` hashes `(class, last byte)`. Discovery pool, when `HP_META_PATTERNS=1`,
spends 1/4 of replacements on skip-k templates (every 2nd–5th byte) so the
search space includes *layout* patterns, not only random masks.

## B.1 pattern cache

Implementation: last-value memo of `hash2(salt,key)` plus the class taxonomy.
**Not** a probability cache — that cannot be bit-identical across
predict/update. Identity test: `test/pattern_cache.sh` and the Windows
on/off pair. **PASS** on proxy64k (2026-08-22).

Throughput not yet measured on enwik8. Keep the flag; the lookup is 32-line
direct-map and is cheaper than a missed `mix64`.

## B.3 cross-run pattern library — NO-GO (this session)

Same trap as the dictionary experiment already run in this repo:

```
transform won     1,498 B
stored table cost 10,121 B
net               −1.28 % at 3 MB
```

A shipped pattern library is S1. fx2-cmix's 412 KB `english.dic` only pays
at ~0.37% of a 110 MB archive. Until an enwik8 (and then enwik9) number
exists, **do not embed a library**. Prefer Hebbian / wiki-state / pattern
*class* (zero transmitted bytes).

**Decision: NO-GO.** Revisit only after enwik8 baseline and a compressed
dictionary re-test (UPGRADES 4.1). The go condition is: stored size < 30% of
the transform gain on enwik8, and effective rank does not drop.
