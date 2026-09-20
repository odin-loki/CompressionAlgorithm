#!/usr/bin/env python3
"""Inject exp_config.h knobs into a copied fx2-cmix tree."""
from pathlib import Path
import shutil
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else "/home/odin/hp_tmp/fx2exp/src_base")
hdr_src = Path(__file__).resolve().parent.parent / "exp_config.h"
if not hdr_src.exists():
    hdr_src = Path("/mnt/c/Users/odinl/OneDrive/Desktop/Compression Algorithm/fx2_mixer_lab/exp_config.h")
shutil.copy(hdr_src, root / "src" / "exp_config.h")


def once(path: Path, old: str, new: str, label: str) -> None:
    t = path.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n")
    if new.strip() in t and old not in t:
        print(f"skip {label} (already patched)")
        return
    if old not in t:
        raise SystemExit(f"FAIL {label}: needle not found in {path}")
    path.write_text(t.replace(old, new, 1), encoding="utf-8")
    print(f"ok {label}")


pred = root / "src" / "predictor.cpp"
mix = root / "src" / "mixer" / "mixer.cpp"
ppmd = root / "src" / "models" / "ppmd.cpp"

once(
    pred,
    '#include "predictor.h"\n',
    '#include "predictor.h"\n#include "exp_config.h"\n',
    "predictor include",
)

once(
    pred,
    "  byte_model_.emplace(25, 14000, manager_.bit_context_, vocab_);",
    """#if FX2_DROP_PPM
  byte_model_.emplace(2, 16, manager_.bit_context_, vocab_);
#else
  byte_model_.emplace(FX2_PPM_ORDER, FX2_PPM_MEM, manager_.bit_context_, vocab_);
#endif""",
    "ppm mem",
)

once(
    pred,
    """void Predictor::AddWord() {
  float delta = 200;""",
    """void Predictor::AddWord() {
#if FX2_DROP_WORD
  return;
#endif
  float delta = 200;""",
    "drop word",
)

once(
    pred,
    "void Predictor::AddMatch() {",
    """void Predictor::AddMatch() {
#if FX2_DROP_MATCH
  return;
#endif""",
    "drop match",
)

once(
    pred,
    """void Predictor::AddMixer(int layer, const unsigned long long& context,
    float learning_rate) {""",
    """void Predictor::AddMixer(int layer, const unsigned long long& context,
    float learning_rate) {
#if FX2_DROP_HALF_MIX
  if (layer == 0 && learning_rate < 0.002f) return;
#endif
#if FX2_FLAT_LR
  if (layer == 0) learning_rate = 0.002f;
#endif
  learning_rate *= FX2_L1_SCALE;""",
    "mixer lr knobs",
)

once(
    pred,
    "      vocab_size, new Lstm(vocab_size, vocab_size, 200, 1, 128, 0.03, 10));",
    """#if FX2_TINY_LSTM
      vocab_size, new Lstm(vocab_size, vocab_size, 1, 1, 8, 0.03f, 10));
#else
      vocab_size, new Lstm(vocab_size, vocab_size, FX2_LSTM_CELLS, FX2_LSTM_LAYERS,
          FX2_LSTM_HORIZON, FX2_LSTM_LR, FX2_LSTM_CLIP));
#endif""",
    "lstm ctor",
)

once(
    pred,
    "  AddMixer(1,manager_.zero_context_, 0.0003);",
    "  AddMixer(1,manager_.zero_context_, FX2_L2_LR);",
    "l2 lr",
)

once(
    mix,
    '#include "mixer.h"\n',
    '#include "mixer.h"\n#include "../exp_config.h"\n',
    "mixer include",
)

once(
    mix,
    """  float decay=0.2f;
  if ( steps_ < 25000000) {
      decay = 0.3f;
      if ( steps_ < 5000000) { 
          decay = 0.7f;
          if ( steps_ < 1000000)  
              decay = 1.0f;
      }
  }
  ++steps_;
   
  float update =   learning_rate_ * (Sigmoid::Logistic(p_) - bit);
  if(fabs(update)<0.000000000005f && extra_inputs_size_>0) {
      return;
  }""",
    """  float decay=0.2f;
#if FX2_NO_DECAY
  decay = 1.0f;
#else
  if ( steps_ < 25000000) {
      decay = 0.3f;
      if ( steps_ < 5000000) { 
          decay = 0.7f;
          if ( steps_ < 1000000)  
              decay = 1.0f;
      }
  }
#endif
  ++steps_;
   
  float update =   learning_rate_ * (Sigmoid::Logistic(p_) - bit);
  if (FX2_SKIP_ERR > 0.0f && fabs(Sigmoid::Logistic(p_) - bit) < FX2_SKIP_ERR) {
      return;
  }
  if(fabs(update)<FX2_SKIP_UPD && extra_inputs_size_>0) {
      return;
  }""",
    "mixer skip/decay",
)

once(
    ppmd,
    '#include "ppmd.h"\n',
    '#include "ppmd.h"\n#include "../exp_config.h"\n',
    "ppmd include",
)

once(
    ppmd,
    "bool mmap_to_disk = true;",
    "bool mmap_to_disk = FX2_MMAP;",
    "mmap flag",
)

print("PATCH_ALL_OK")
