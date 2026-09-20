#pragma once
//
// Integer LSTM (Q15). No float, no <cmath>. Encoder == decoder.
// Byte path mirrors cmix ByteMixer: softmax over 256, bit p from [bot,top]
// binary split, expected byte = argmax in range. Train truncated BPTT-1
// at byte boundary. Bit path is a 1-bit head (not used by cmix).

#include <cstdint>
#include <cstring>

#include "hp/features.hpp"
#include "hp/int_math.hpp"

namespace hp {

#ifndef HP_LSTM_H
#define HP_LSTM_H 16
#endif
#ifndef HP_LSTM_I
#define HP_LSTM_I 16
#endif
#ifndef HP_LSTM_LR
#define HP_LSTM_LR 6
#endif
#ifndef HP_LSTM_LAYERS
#define HP_LSTM_LAYERS 1
#endif
#ifndef HP_LSTM_MIXIN
#define HP_LSTM_MIXIN 0
#endif

class IntLstm {
 public:
    static constexpr int kH = HP_LSTM_H;
    static constexpr int kI = HP_LSTM_I;

    IntLstm() {
        std::uint32_t s = 0x4C53544Du;
        auto rnd = [&]() -> std::int16_t {
            s = s * 1664525u + 1013904223u;
            return static_cast<std::int16_t>(static_cast<int>(s >> 20) - 8);
        };
        for (int v = 0; v < 256; ++v)
            for (int i = 0; i < kI; ++i) E_[v][i] = rnd();
        for (int g = 0; g < 4; ++g) {
            for (int h = 0; h < kH; ++h) {
                b_[g][h] = (g == 1) ? 8000 : 0;
                for (int i = 0; i < kI; ++i) Wx_[g][h][i] = rnd();
                for (int j = 0; j < kH; ++j) Wh_[g][h][j] = rnd();
            }
        }
        for (int v = 0; v < 256; ++v) {
            by_[v] = 0;
            for (int h = 0; h < kH; ++h) Wy_[v][h] = rnd();
        }
        std::memset(h_, 0, sizeof(h_));
        std::memset(c_, 0, sizeof(c_));
        reset_range();
        for (int v = 0; v < 256; ++v) mass_[v] = 16;
        last_p12_ = 2048;
        expected_ = 0;
        mixin_ = 0;
#if HP_LSTM_LAYERS > 1
        std::memset(h2_, 0, sizeof(h2_));
        std::memset(c2_, 0, sizeof(c2_));
        for (int g = 0; g < 4; ++g) {
            for (int h = 0; h < kH; ++h) {
                b2_[g][h] = (g == 1) ? 8000 : 0;
                for (int j = 0; j < kH; ++j) {
                    Wx2_[g][h][j] = rnd();
                    Wh2_[g][h][j] = rnd();
                }
            }
        }
#endif
    }

    void set_mixin(int p12) {
        mixin_ = sat16((p12 - 2048) * 16);
    }

    void reset_range() { bot_ = 0; top_ = 255; }

    int predict_p12() {
        const int mid = bot_ + ((top_ - bot_) / 2);
        std::int64_t num = 0, den = 0;
        std::int64_t best = -1;
        expected_ = bot_;
        for (int i = bot_; i <= top_; ++i) {
            den += mass_[i];
            if (i > mid) num += mass_[i];
            if (mass_[i] > best) { best = mass_[i]; expected_ = i; }
        }
        if (den <= 0) { last_p12_ = 2048; return last_p12_; }
        last_p12_ = clamp_int(static_cast<int>((num * 4096 + den / 2) / den), 1, 4094);
        return last_p12_;
    }

    void perceive_bit(int y) {
        const int mid = bot_ + ((top_ - bot_) / 2);
        if (y) bot_ = mid + 1;
        else top_ = mid;
    }

    void perceive_byte(int byte) {
        train_output_(byte & 255);
        std::memcpy(hprev_, h_, sizeof(h_));
        std::memcpy(cprev_, c_, sizeof(c_));
        for (int i = 0; i < kI; ++i) x_[i] = E_[byte & 255][i];
#if HP_LSTM_MIXIN
        x_[kI - 1] = mixin_;
#endif
        forward_();
#if HP_LSTM_LAYERS > 1
        forward2_();
#endif
        softmax_();
        reset_range();
    }

    int expected() const { return expected_; }
    int last_p12() const { return last_p12_; }

    std::uint32_t hidden_sig() const {
        std::uint32_t u = 0x4C53544Du;
        for (int h = 0; h < kH; ++h)
            u = u * 1664525u + static_cast<std::uint32_t>(h_[h] + 32768);
        return u;
    }

    int hidden_bin(int n) const {
        int v = (static_cast<int>(h_[0]) + 32768) >> (16 - n);
        const int m = (1 << n) - 1;
        return v < 0 ? 0 : (v > m ? m : v);
    }

#if HP_LSTM_BIT
    int predict_bit_p12(int last_bit) {
        x_[0] = last_bit ? 16384 : -16384;
        for (int i = 1; i < kI; ++i) x_[i] = 0;
        std::memcpy(hprev_, h_, sizeof(h_));
        std::memcpy(cprev_, c_, sizeof(c_));
        forward_();
        std::int32_t z = by_[0];
        for (int h = 0; h < kH; ++h)
            z += static_cast<std::int32_t>(Wy_[0][h]) * h_[h];
        last_p12_ = squash(clamp_int(z >> 16, -2047, 2047));
        if (last_p12_ < 1) last_p12_ = 1;
        if (last_p12_ > 4094) last_p12_ = 4094;
        return last_p12_;
    }

    void update_bit(int y) {
        const int p = last_p12_;
        const int err = y ? (p - 4096) : p;
        const int lr = HP_LSTM_LR;
        std::int32_t dh[kH];
        std::memset(dh, 0, sizeof(dh));
        for (int h = 0; h < kH; ++h) {
            const std::int32_t d = (static_cast<std::int32_t>(h_[h]) * err) >> (8 + lr);
            Wy_[0][h] = sat16(Wy_[0][h] - d);
            dh[h] += (static_cast<std::int32_t>(Wy_[0][h]) * err) >> 12;
        }
        by_[0] = sat16(by_[0] - (err >> lr));
        backward_cell_(dh);
    }
#endif

 private:
    static std::int16_t sat16(std::int32_t v) {
        if (v > 32767) return 32767;
        if (v < -32767) return -32767;
        return static_cast<std::int16_t>(v);
    }

    static std::int16_t mul15(std::int16_t a, std::int16_t b) {
        return sat16((static_cast<std::int32_t>(a) * b) >> 15);
    }

    static std::int16_t sig15(std::int32_t z) {
        if (z > 8192) return 32767;
        if (z < -8192) return 0;
        const std::int32_t a = z < 0 ? -z : z;
        std::int32_t s = (z * 16384) / (256 + a);
        s += 16384;
        return sat16(s);
    }

    static std::int16_t tanh15(std::int32_t z) {
        const std::int16_t s = sig15(z * 2);
        return sat16((static_cast<std::int32_t>(s) * 2) - 32767);
    }

    static std::int16_t dsig15(std::int16_t s) {
        const std::int32_t t = (32767 - s);
        return sat16((static_cast<std::int32_t>(s) * t) >> 15);
    }

    static std::int16_t dtanh15(std::int16_t t) {
        const std::int32_t u = 32767 - ((static_cast<std::int32_t>(t) * t) >> 15);
        return sat16(u);
    }

    void forward_() {
        for (int g = 0; g < 4; ++g) {
            for (int h = 0; h < kH; ++h) {
                std::int32_t acc = b_[g][h];
                for (int i = 0; i < kI; ++i)
                    acc += static_cast<std::int32_t>(Wx_[g][h][i]) * x_[i];
                for (int j = 0; j < kH; ++j)
                    acc += static_cast<std::int32_t>(Wh_[g][h][j]) * hprev_[j];
                pre_[g][h] = acc >> 8;
                gate_[g][h] = (g == 3) ? tanh15(pre_[g][h]) : sig15(pre_[g][h]);
            }
        }
        for (int h = 0; h < kH; ++h) {
            const std::int16_t nc = sat16(
                static_cast<std::int32_t>(mul15(gate_[1][h], cprev_[h])) +
                mul15(gate_[0][h], gate_[3][h]));
            c_[h] = nc;
            h_[h] = mul15(gate_[2][h], tanh15(nc));
        }
    }

    void softmax_() {
        std::int32_t logit[256];
        std::int32_t mx = -2147483647 - 1;
        for (int v = 0; v < 256; ++v) {
            std::int32_t z = by_[v];
            for (int h = 0; h < kH; ++h)
#if HP_LSTM_LAYERS > 1
                z += static_cast<std::int32_t>(Wy_[v][h]) * h2_[h];
#else
                z += static_cast<std::int32_t>(Wy_[v][h]) * h_[h];
#endif
            z >>= 8;
            logit[v] = z;
            if (z > mx) mx = z;
        }
        for (int v = 0; v < 256; ++v) {
            int d = mx - logit[v];
            if (d < 0) d = 0;
            if (d > 8191) d = 8191;
            mass_[v] = (4096 * 2048) / (2048 + d);
            if (mass_[v] < 1) mass_[v] = 1;
        }
    }

    void train_output_(int y) {
        std::int64_t tot = 0;
        for (int v = 0; v < 256; ++v) tot += mass_[v];
        if (tot <= 0) tot = 1;
        std::int32_t dh[kH];
        std::memset(dh, 0, sizeof(dh));
        const int lr = HP_LSTM_LR;
        for (int v = 0; v < 256; ++v) {
            const std::int32_t p = static_cast<std::int32_t>((mass_[v] * 4096) / tot);
            const std::int32_t err = p - (v == y ? 4096 : 0);
            for (int h = 0; h < kH; ++h) {
                const std::int32_t dw = (static_cast<std::int32_t>(htop_(h)) * err) >> (8 + lr);
                Wy_[v][h] = sat16(Wy_[v][h] - dw);
                dh[h] += (static_cast<std::int32_t>(Wy_[v][h]) * err) >> 12;
            }
            by_[v] = sat16(by_[v] - (err >> lr));
        }
#if HP_LSTM_LAYERS > 1
        backward_cell2_(dh);
#else
        backward_cell_(dh);
#endif
        for (int i = 0; i < kI; ++i) {
            std::int32_t dx = 0;
            for (int g = 0; g < 4; ++g)
                for (int h = 0; h < kH; ++h)
                    dx += (static_cast<std::int32_t>(Wx_[g][h][i]) * dpre_[g][h]) >> 12;
            E_[y][i] = sat16(E_[y][i] - (dx >> lr));
        }
    }

    void backward_cell_(const std::int32_t* dh_in) {
        std::int32_t dc[kH];
        std::memset(dpre_, 0, sizeof(dpre_));
        for (int h = 0; h < kH; ++h) {
            const std::int16_t th = tanh15(c_[h]);
            const std::int32_t do_ = (dh_in[h] * th) >> 15;
            dc[h] = (dh_in[h] * gate_[2][h]) >> 15;
            dc[h] = (dc[h] * dtanh15(th)) >> 15;
            const std::int32_t di = (dc[h] * gate_[3][h]) >> 15;
            const std::int32_t dg = (dc[h] * gate_[0][h]) >> 15;
            const std::int32_t df = (dc[h] * cprev_[h]) >> 15;
            dpre_[0][h] = (di * dsig15(gate_[0][h])) >> 15;
            dpre_[1][h] = (df * dsig15(gate_[1][h])) >> 15;
            dpre_[2][h] = (do_ * dsig15(gate_[2][h])) >> 15;
            dpre_[3][h] = (dg * dtanh15(gate_[3][h])) >> 15;
        }
        const int lr = HP_LSTM_LR;
        for (int g = 0; g < 4; ++g) {
            for (int h = 0; h < kH; ++h) {
                const std::int32_t dp = dpre_[g][h];
                b_[g][h] = sat16(b_[g][h] - (dp >> lr));
                for (int i = 0; i < kI; ++i) {
                    const std::int32_t dw = (static_cast<std::int32_t>(x_[i]) * dp) >> (8 + lr);
                    Wx_[g][h][i] = sat16(Wx_[g][h][i] - dw);
                }
                for (int j = 0; j < kH; ++j) {
                    const std::int32_t dw = (static_cast<std::int32_t>(hprev_[j]) * dp) >> (8 + lr);
                    Wh_[g][h][j] = sat16(Wh_[g][h][j] - dw);
                }
            }
        }
    }

    std::int16_t htop_(int h) const {
#if HP_LSTM_LAYERS > 1
        return h2_[h];
#else
        return h_[h];
#endif
    }

#if HP_LSTM_LAYERS > 1
    void forward2_() {
        std::memcpy(h2prev_, h2_, sizeof(h2_));
        std::memcpy(c2prev_, c2_, sizeof(c2_));
        for (int g = 0; g < 4; ++g) {
            for (int h = 0; h < kH; ++h) {
                std::int32_t acc = b2_[g][h];
                for (int i = 0; i < kH; ++i)
                    acc += static_cast<std::int32_t>(Wx2_[g][h][i]) * h_[i];
                for (int j = 0; j < kH; ++j)
                    acc += static_cast<std::int32_t>(Wh2_[g][h][j]) * h2prev_[j];
                pre2_[g][h] = acc >> 8;
                gate2_[g][h] = (g == 3) ? tanh15(pre2_[g][h]) : sig15(pre2_[g][h]);
            }
        }
        for (int h = 0; h < kH; ++h) {
            const std::int16_t nc = sat16(
                static_cast<std::int32_t>(mul15(gate2_[1][h], c2prev_[h])) +
                mul15(gate2_[0][h], gate2_[3][h]));
            c2_[h] = nc;
            h2_[h] = mul15(gate2_[2][h], tanh15(nc));
        }
    }

    void backward_cell2_(const std::int32_t* dh_in) {
        const int lr = HP_LSTM_LR;
        for (int h = 0; h < kH; ++h) {
            const std::int16_t th = tanh15(c2_[h]);
            const std::int32_t do_ = (dh_in[h] * th) >> 15;
            std::int32_t dc = (dh_in[h] * gate2_[2][h]) >> 15;
            dc = (dc * dtanh15(th)) >> 15;
            const std::int32_t di = (dc * gate2_[3][h]) >> 15;
            const std::int32_t dg = (dc * gate2_[0][h]) >> 15;
            const std::int32_t df = (dc * c2prev_[h]) >> 15;
            dpre2_[0][h] = (di * dsig15(gate2_[0][h])) >> 15;
            dpre2_[1][h] = (df * dsig15(gate2_[1][h])) >> 15;
            dpre2_[2][h] = (do_ * dsig15(gate2_[2][h])) >> 15;
            dpre2_[3][h] = (dg * dtanh15(gate2_[3][h])) >> 15;
        }
        for (int g = 0; g < 4; ++g) {
            for (int h = 0; h < kH; ++h) {
                const std::int32_t dp = dpre2_[g][h];
                b2_[g][h] = sat16(b2_[g][h] - (dp >> lr));
                for (int i = 0; i < kH; ++i) {
                    const std::int32_t dw = (static_cast<std::int32_t>(h_[i]) * dp) >> (8 + lr);
                    Wx2_[g][h][i] = sat16(Wx2_[g][h][i] - dw);
                }
                for (int j = 0; j < kH; ++j) {
                    const std::int32_t dw = (static_cast<std::int32_t>(h2prev_[j]) * dp) >> (8 + lr);
                    Wh2_[g][h][j] = sat16(Wh2_[g][h][j] - dw);
                }
            }
        }
    }
#endif

    std::int16_t E_[256][kI];
    std::int16_t Wx_[4][kH][kI];
    std::int16_t Wh_[4][kH][kH];
    std::int16_t b_[4][kH];
    std::int16_t Wy_[256][kH];
    std::int16_t by_[256];
    std::int16_t h_[kH], c_[kH], hprev_[kH], cprev_[kH], x_[kI];
    std::int16_t gate_[4][kH];
    std::int32_t pre_[4][kH];
    std::int32_t dpre_[4][kH];
    std::int32_t mass_[256];
    std::int16_t mixin_ = 0;
#if HP_LSTM_LAYERS > 1
    std::int16_t Wx2_[4][kH][kH];
    std::int16_t Wh2_[4][kH][kH];
    std::int16_t b2_[4][kH];
    std::int16_t h2_[kH], c2_[kH], h2prev_[kH], c2prev_[kH];
    std::int16_t gate2_[4][kH];
    std::int32_t pre2_[4][kH];
    std::int32_t dpre2_[4][kH];
#endif
    int bot_ = 0, top_ = 255, expected_ = 0, last_p12_ = 2048;
};

}  // namespace hp
