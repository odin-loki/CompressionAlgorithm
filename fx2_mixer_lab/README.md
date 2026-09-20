# fx2_mixer_lab — fx2-cmix mixer / Boolean / DSP screens

Sandbox for Track W experiments. Does **not** touch `hp/` champ
binaries or the live prize tree under `~/fx2_build` except the shared
PPMD remap fix (skip munmap/remap every 20k bytes under WSL).

## Lab protocol

- Source copy: `/home/odin/hp_tmp/fx2exp/src_base` (patched prize tree)
- Screen slice: first 32768 B of `enwik8.8mb`, `cmix -n`
- Pin slice: first 65536 B
- Lab PPM: 512 MB, mmap off (prize default is 14000 mmap)
- Results: `fx2_mixer_lab/results/screen32.csv`, `screen64.csv`

Baseline (32 KB): **9165** bytes / 2.2375 bpc.

## What paid (32 KB)

| name | bytes | Δ | knob |
|---|---:|---:|---|
| c200_lr08_skip01 | 9146 | −19 | LSTM LR 0.08 + skip err 0.01 |
| lstm_lr08 | 9147 | −18 | LSTM LR 0.08 |
| h32_c256_lr05 | 9148 | −17 | horizon 32 + cells 256 + LR 0.05 |
| l1x15 / l1x20 | 9151 / 9152 | −14 / −13 | linear L1 scale 1.5–2.0 |
| lstm_h32 | 9155 | −10 | horizon 32 |

Faster LSTM learning and a **shorter** horizon beat more cells.
Width 256/300 alone lost or was flat.

## 64 KB pin (same lab PPM)

| name | bytes | Δ vs baseline64 |
|---|---:|---:|
| baseline64 | 17032 | 0 |
| **lr08_h32_64** | **16964** | **−68** |
| lr08_64 | 16994 | −38 |
| lr08_skip01_64 | 16996 | −36 |
| h32_64 | 17019 | −13 |

`lr=0.08` + horizon 32 stacks on 64 KB (−68). Skip-on-top of lr08 is noise.

## What died

- **Delete twins to fund LSTM:** nothing beat baseline; `tiny_lstm` +16.
- **Boolean extras** (XOR/AND/OR of expert signs): all worse; `xor_k6` +517.
- **Walsh-4 / parity / deltas:** worse; stacking made it worse.
- **Natural gradient / sign-sign LMS:** catastrophic (+20k…+27k).
- Kitchen-sink drop+scale combos: lost.

Reed–Muller / Walsh on this linear mixer did **not** open useful
feature space on 32 KB. The hole is already covered by the ByteMixer
LSTM; hand-coded XOR diluted training.

## 8 MiB side-by-side (patched PPMD remap)

| engine | bytes | bpc | notes |
|---|---:|---:|---|
| fx2-cmix (dict, patched) | **1,395,442** | **1.331** | wall ~100 min, RSS ~5.6 GB |
| hp v93 SLOT_MAX=35 | 1,675,993 | 1.598 | published champ |
| hp hard SLOT_MAX=22 | 1,684,990 | 1.607 | this-tree cap |

Unpatched fx2 SIGSEGV’d at ~20 KB on WSL (PPMD remapped VA). Patch:
`if (false && mmap_to_disk && counter_ % 20000 == 0)`.

## Scripts

| file | role |
|---|---|
| `exp_config.h` | compile-time knobs |
| `scripts/patch_src.py` | inject knobs into predictor/mixer/ppmd |
| `scripts/patch_bool.py` | Boolean/Walsh/DSP derived mixer features |
| `scripts/run_one.sh` | build + 32 KB encode, append CSV |
| `scripts/pin64_lstm.sh` | 64 KB pin of LSTM winners |
