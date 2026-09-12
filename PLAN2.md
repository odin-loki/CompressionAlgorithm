<!--
ARCHIVED. Superseded by root PLAN.md. Do not add new experiments here.
Axis-purity notes remain historically useful; live queue is PLAN.md.
-->

# PLAN2 — beat 1.806 bpc on enwik8-1MB

PLAN.md was the spec. Session 1 landed a bundle and measured it.
This is the second plan: keep only what A.3 and the fx2 notes support.

## What the first bundle taught us

| keep | reject |
|---|---|
| wiki states on `tag_` | CTW (r=0.930 vs o2:py) — already off |
| word-match-1 (mean \|r\|=0.588) | word-match-2/3 as a pile-on (r=0.969) — retarget, don't stack |
| bracket | `pat_` (r=0.971 vs brk) |
| per-mixer lr, Hedge-L1, NCL tiny λ | case-preserving word stream (r=0.944 vs word:py) |
| skip-k discovery | PPMD, B.3 library |

Removing correlated experts is a compression win: the mixer was already
+18 KB redundant on a 256k toy file. Product-of-experts overconfidence
is the measured failure.

## What harvest/wiki_states_notes.md says we still lack

1. **`linkword` as its own ContextModel** — packed into `tag_` today; fx2
   gives it a dedicated map (`cmC2[9]`). Link-vs-body is the axis.
2. **`senword`** — same 2104-hash, but only while `isParagraph` and not
   in a link/template. Body prose, not markup.
3. **Numeric field** — `number0`, `numlen`, previous number. Partial via
   pattern class; not a real model.
4. **Word-match keys `{0}`, `{1,3}`, `{7,2}`** — not 1/2/3-word stacks
   of the same ring (those correlate).
5. **First-char / paragraph as a mixer gate**, not only a context nibble.

## Work

1. Drop `pat_` and `wstr_case_`. Keep skip-k search and `wstr_sp_`
   (first-char class + previous word).
2. Add `link_` (linkword / senword) and `num_` (numeric field).
3. Retarget the three word-match models to fx2's sparse keys.
4. Wiki gate = `state + 16*isParagraph` (32 sets).
5. `kVersion = 4`. Rebuild as `hp_v4` (do not touch the running enwik8
   `hp.exe`).
6. Gates, in order: no_float, 64k roundtrip, cache identity, **enwik8
   1 MB vs 1.806**, dump_experts rank, then the full-enwik8 job.

Accept a model only if 1 MB bpc falls or entropy rank rises. Target:
**below 1.806 bpc** on the same `data/enwik8.1mb` / `--mem 20` protocol.

## Results (same `data/enwik8.1mb`, round-trip PASS unless noted)

| build | mem | out B | bpc | vs 236,657 |
|---|---:|---:|---:|---:|
| session-1 bundle (`hp.exe`) | 20 | 236,657 | 1.806 | baseline |
| v3b (FIRSTUPPER / isParagraph) | 20 | 236,473 | 1.804 | −184 |
| **v3b** | **22** | **232,151** | **1.771** | **−4,506** |
| **v3b** | **24** | **230,266** | **1.757** | **−6,391** |
| **v3b** | **26** | **229,440** | **1.750** | **−7,217** |
| PLAN2 v4 (dropped twins, +link/num) | 20 | 237,136 | 1.809 | +479 |
| PLAN2 v4 | 22 | 232,446 | 1.773 | −4,211 |

**Record on this protocol: v3b `--mem 26` = 229,440 B / 1.750 bpc**
(mem 24 = 230,266 / 1.757 still a win; curve still falling at 26).
Memory scaling (UPGRADES 0.3) was the real lever. PLAN2's axis-purity cut
lost bits at equal memory — keep v3b as the record binary; keep v4 as the
A.3 experiment, not the submission.
