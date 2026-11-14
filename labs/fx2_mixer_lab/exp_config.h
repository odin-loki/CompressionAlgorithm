#pragma once
// Lab knobs for fx2-cmix mixer/axis screens. Defaults match prize fx2
// except PPM mem 512 / mmap off so 32 KiB encodes do not spawn 14 GB heaps.

#ifndef FX2_LSTM_CELLS
#define FX2_LSTM_CELLS 200
#endif
#ifndef FX2_LSTM_LAYERS
#define FX2_LSTM_LAYERS 1
#endif
#ifndef FX2_LSTM_HORIZON
#define FX2_LSTM_HORIZON 128
#endif
#ifndef FX2_LSTM_LR
#define FX2_LSTM_LR 0.03f
#endif
#ifndef FX2_LSTM_CLIP
#define FX2_LSTM_CLIP 10
#endif
#ifndef FX2_PPM_ORDER
#define FX2_PPM_ORDER 25
#endif
#ifndef FX2_PPM_MEM
#define FX2_PPM_MEM 512
#endif
#ifndef FX2_MMAP
#define FX2_MMAP 0
#endif
#ifndef FX2_SKIP_ERR
#define FX2_SKIP_ERR 0
#endif
#ifndef FX2_SKIP_UPD
#define FX2_SKIP_UPD 5e-12f
#endif
#ifndef FX2_L2_LR
#define FX2_L2_LR 0.0003f
#endif
#ifndef FX2_L1_SCALE
#define FX2_L1_SCALE 1.0f
#endif
#ifndef FX2_DROP_WORD
#define FX2_DROP_WORD 0
#endif
#ifndef FX2_DROP_MATCH
#define FX2_DROP_MATCH 0
#endif
#ifndef FX2_DROP_PPM
#define FX2_DROP_PPM 0
#endif
#ifndef FX2_DROP_HALF_MIX
#define FX2_DROP_HALF_MIX 0
#endif
#ifndef FX2_NO_DECAY
#define FX2_NO_DECAY 0
#endif
#ifndef FX2_FLAT_LR
#define FX2_FLAT_LR 0
#endif
#ifndef FX2_TINY_LSTM
#define FX2_TINY_LSTM 0
#endif

// Boolean / DSP extras on the linear mixer. Degree-1 mix cannot represent XOR.
// Reed-Muller: {1, a, b, ab} spans every 2-input Boolean function.
#ifndef FX2_BOOL_K
#define FX2_BOOL_K 6
#endif
#ifndef FX2_BOOL_XOR
#define FX2_BOOL_XOR 0
#endif
#ifndef FX2_BOOL_AND
#define FX2_BOOL_AND 0
#endif
#ifndef FX2_BOOL_OR
#define FX2_BOOL_OR 0
#endif
#ifndef FX2_BOOL_PROD
#define FX2_BOOL_PROD 0
#endif
#ifndef FX2_BOOL_MAJ
#define FX2_BOOL_MAJ 0
#endif
#ifndef FX2_DSP_DELTA
#define FX2_DSP_DELTA 0
#endif
#ifndef FX2_PARITY8
#define FX2_PARITY8 0
#endif
#ifndef FX2_WALSH4
#define FX2_WALSH4 0
#endif
#ifndef FX2_NATGRAD
#define FX2_NATGRAD 0
#endif
#ifndef FX2_SIGNLMS
#define FX2_SIGNLMS 0
#endif
