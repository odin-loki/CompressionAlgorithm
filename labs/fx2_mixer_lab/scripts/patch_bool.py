#!/usr/bin/env python3
"""Add Boolean/DSP derived mixer features to src_base mixer.{h,cpp}."""
from pathlib import Path

root = Path("/home/odin/hp_tmp/fx2exp/src_base")
win_hdr = Path("/mnt/c/Users/odinl/OneDrive/Desktop/Compression Algorithm/fx2_mixer_lab/exp_config.h")
text = win_hdr.read_text(encoding="utf-8").replace("\r\n", "\n")
Path("/home/odin/hp_tmp/exp_config.h").write_text(text, encoding="utf-8")
(root / "src" / "exp_config.h").write_text(text, encoding="utf-8")

mixer_h = r'''#ifndef MIXER_H
#define MIXER_H

#include <vector>
#include <valarray>
#include "../ds/emhash_map.hpp"
#include <memory>
#include "../exp_config.h"

struct ContextData {
  ContextData(unsigned long long input_size,
      unsigned long long extra_input_size) : /*steps(0), */weights(input_size),
      extra_weights(extra_input_size) {};
  ContextData& operator=(const ContextData&) = default;
  ContextData(const ContextData&) = default;
  std::valarray<float> weights, extra_weights;
};

class Mixer {
 public:
  Mixer(const std::valarray<float>& inputs,
      const std::valarray<float>& extra_inputs, const unsigned long long& context,
      float learning_rate, unsigned int extra_input_size);
  float Mix();
  void Perceive(int bit);

 private:
  ContextData* GetContextData();
  void FillDerived();
  const std::valarray<float>& inputs_;
  const std::valarray<float>& extra_inputs_vec_;
  uint16_t extra_inputs_size_;
  float p_, learning_rate_;
  const unsigned long long& context_;
  unsigned long long steps_;
  emhash6::HashMap<unsigned int, ContextData> context_map_;
  ContextData context_base_;
  int k_bool_;
  int n_derived_;
  std::valarray<float> derived_;
  std::valarray<float> derived_w_;
  std::valarray<float> last_in_;
  unsigned int bit_hist_;
};

#endif
'''
(root / "src" / "mixer" / "mixer.h").write_text(mixer_h, encoding="utf-8")

cpp = (root / "src" / "mixer" / "mixer.cpp").read_text(encoding="utf-8").replace("\r\n", "\n")

old_ctor = '''Mixer::Mixer(const std::valarray<float>& inputs,
    const std::valarray<float>& extra_inputs,
    const unsigned long long& context, float learning_rate,
    unsigned int extra_input_size) : inputs_(inputs),
    extra_inputs_vec_(extra_inputs), extra_inputs_size_(extra_input_size),/*extra_inputs_(extra_input_size),*/ p_(0.5),
    learning_rate_(learning_rate), context_(context), /*max_steps_(1),*/ steps_(0),
    context_base_(inputs.size(), extra_inputs_size_)
    {}'''

new_ctor = '''static int CountDerived(int k) {
  int np = k * (k - 1) / 2;
  int n = 0;
#if FX2_BOOL_XOR
  n += np;
#endif
#if FX2_BOOL_AND
  n += np;
#endif
#if FX2_BOOL_OR
  n += np;
#endif
#if FX2_BOOL_PROD
  n += np;
#endif
#if FX2_BOOL_MAJ
  n += 1;
#endif
#if FX2_DSP_DELTA
  n += k;
#endif
#if FX2_PARITY8
  n += 1;
#endif
#if FX2_WALSH4
  n += 15;
#endif
  return n;
}

Mixer::Mixer(const std::valarray<float>& inputs,
    const std::valarray<float>& extra_inputs,
    const unsigned long long& context, float learning_rate,
    unsigned int extra_input_size) : inputs_(inputs),
    extra_inputs_vec_(extra_inputs), extra_inputs_size_(extra_input_size), p_(0.5),
    learning_rate_(learning_rate), context_(context), steps_(0),
    context_base_(inputs.size(), extra_input_size),
    k_bool_(0), n_derived_(0), bit_hist_(0)
{
  k_bool_ = FX2_BOOL_K;
  if (k_bool_ > (int)inputs_.size()) k_bool_ = (int)inputs_.size();
  n_derived_ = CountDerived(k_bool_);
  derived_.resize(n_derived_ > 0 ? n_derived_ : 1, 0.f);
  derived_w_.resize(n_derived_ > 0 ? n_derived_ : 1, 0.f);
  last_in_.resize(k_bool_ > 0 ? k_bool_ : 1, 0.f);
}'''

if old_ctor not in cpp:
    raise SystemExit("ctor not found")
cpp = cpp.replace(old_ctor, new_ctor, 1)

old_mix = '''float Mixer::Mix() {
  ContextData* data = GetContextData();
  float p = 0;
  for (int i = 0; i < inputs_.size(); ++i) {
    p += inputs_[i] * data->weights[i];
  }
  p_ = p;
  // for (unsigned int i = 0; i < extra_inputs_.size(); ++i) {
  //   extra_inputs_[i] = extra_inputs_vec_[i];
  // }
  float e = 0;
  for (unsigned int i = 0; i < extra_inputs_size_; ++i) {
    e += extra_inputs_vec_[i] * data->extra_weights[i];
  }
  p_ += e;
  return p_;
}'''

new_mix = r'''void Mixer::FillDerived() {
  if (n_derived_ <= 0) return;
  const int k = k_bool_;
  int idx = 0;
  auto sgn = [](float x) { return x >= 0.f ? 1.f : -1.f; };
#if FX2_BOOL_XOR
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      derived_[idx++] = -8.f * sgn(inputs_[i]) * sgn(inputs_[j]);
#endif
#if FX2_BOOL_AND
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      derived_[idx++] = (inputs_[i] >= 0.f && inputs_[j] >= 0.f) ? 8.f : -8.f;
#endif
#if FX2_BOOL_OR
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      derived_[idx++] = (inputs_[i] >= 0.f || inputs_[j] >= 0.f) ? 8.f : -8.f;
#endif
#if FX2_BOOL_PROD
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      derived_[idx++] = 0.05f * inputs_[i] * inputs_[j];
#endif
#if FX2_BOOL_MAJ
  {
    int pos = 0;
    for (int i = 0; i < k; ++i) if (inputs_[i] >= 0.f) ++pos;
    derived_[idx++] = (pos * 2 > k) ? 8.f : -8.f;
  }
#endif
#if FX2_DSP_DELTA
  for (int i = 0; i < k; ++i)
    derived_[idx++] = inputs_[i] - last_in_[i];
#endif
#if FX2_PARITY8
  {
    unsigned int x = bit_hist_ & 255u;
    int par = 0;
    while (x) { par ^= (int)(x & 1u); x >>= 1; }
    derived_[idx++] = par ? 8.f : -8.f;
  }
#endif
#if FX2_WALSH4
  {
    float b[4];
    for (int i = 0; i < 4; ++i)
      b[i] = (bit_hist_ & (1u << i)) ? 1.f : -1.f;
    for (int mask = 1; mask < 16; ++mask) {
      float v = 8.f;
      for (int i = 0; i < 4; ++i)
        if (mask & (1 << i)) v *= b[i];
      derived_[idx++] = v;
    }
  }
#endif
  (void)idx;
}

float Mixer::Mix() {
  ContextData* data = GetContextData();
  float p = 0;
  for (int i = 0; i < inputs_.size(); ++i) {
    p += inputs_[i] * data->weights[i];
  }
  p_ = p;
  float e = 0;
  for (unsigned int i = 0; i < extra_inputs_size_; ++i) {
    e += extra_inputs_vec_[i] * data->extra_weights[i];
  }
  p_ += e;
  FillDerived();
  for (int i = 0; i < n_derived_; ++i)
    p_ += derived_w_[i] * derived_[i];
  return p_;
}'''

if old_mix not in cpp:
    raise SystemExit("Mix() not found")
cpp = cpp.replace(old_mix, new_mix, 1)

old_upd = '''  update = decay * update;
  ContextData* data = GetContextData();
  
  data->weights -= update * inputs_;
  data->extra_weights -= update * extra_inputs_vec_[std::slice(0,extra_inputs_size_,1)];'''

new_upd = '''  float pbit = Sigmoid::Logistic(p_);
#if FX2_NATGRAD
  update = learning_rate_ * (pbit - bit) / (pbit * (1.f - pbit) + 1e-4f);
#endif
#if FX2_SIGNLMS
  update = learning_rate_ * ((pbit > (float)bit) ? 1.f : -1.f);
#endif
  update = decay * update;
  ContextData* data = GetContextData();
  
#if FX2_SIGNLMS
  for (int i = 0; i < (int)inputs_.size(); ++i)
    data->weights[i] -= update * (inputs_[i] >= 0.f ? 1.f : -1.f);
  for (unsigned int i = 0; i < extra_inputs_size_; ++i)
    data->extra_weights[i] -= update * (extra_inputs_vec_[i] >= 0.f ? 1.f : -1.f);
#else
  data->weights -= update * inputs_;
  data->extra_weights -= update * extra_inputs_vec_[std::slice(0,extra_inputs_size_,1)];
#endif
  for (int i = 0; i < n_derived_; ++i)
    derived_w_[i] -= update * derived_[i];
#if FX2_DSP_DELTA
  for (int i = 0; i < k_bool_; ++i)
    last_in_[i] = inputs_[i];
#endif
  bit_hist_ = (bit_hist_ << 1) | (unsigned int)(bit & 1);'''

if old_upd not in cpp:
    raise SystemExit("Perceive update not found")
cpp = cpp.replace(old_upd, new_upd, 1)

(root / "src" / "mixer" / "mixer.cpp").write_text(cpp, encoding="utf-8")
print("BOOL_PATCH_OK")
