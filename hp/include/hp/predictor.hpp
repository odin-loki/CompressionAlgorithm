#pragma once
//
// hp/predictor.hpp — the model stack.
//
// Per bit:
//   1. every expert emits a stretched opinion
//   2. the mixer combines them, gated by (GRIA alpha bucket, partial byte, …)
//   3. two/three APM stages recalibrate
//   4. the coder codes the bit with the final probability
//   5. everything updates on the observed bit
//
// Steps 1-3 and 5 are IDENTICAL in the compressor and the decompressor, and
// depend only on data both sides already have.

#include <algorithm>
#include <cstdint>
#include <vector>

#include "hp/bracket.hpp"
#include "hp/discover.hpp"
#include "hp/english.hpp"
#include "hp/features.hpp"
#include "hp/gria.hpp"
#include "hp/hedge.hpp"
#include "hp/int_math.hpp"
#include "hp/mixer.hpp"
#include "hp/models.hpp"
#include "hp/numeric.hpp"
#include "hp/pattern_cache.hpp"
#include "hp/stat_gates.hpp"
#include "hp/statemap.hpp"
#include "hp/wiki.hpp"
#include "hp/stemmer.hpp"
#include "hp/wordmatch.hpp"
#include "hp/wordstream.hpp"
#include "hp/sentmem.hpp"

#ifndef HP_W0
#define HP_W0 0
#endif
#ifndef HP_WA
#define HP_WA 1
#endif
#ifndef HP_WB
#define HP_WB 5
#endif
#ifndef HP_WG
#define HP_WG 2
#endif
#ifndef HP_APM_CTX
#define HP_APM_CTX 0
#endif
#ifndef HP_APM_NCTX
#define HP_APM_NCTX 256
#endif
#ifndef HP_USE_ENT_GATE
#define HP_USE_ENT_GATE 1
#endif

namespace hp {

struct Config {
    int table_bits = 22;
    int buf_bits = 26;
    int match_bits = 22;
    int mixer_lr = 2;
    bool gria = true;

    // Encoder and decoder must agree. match/buf sizes are a function of
    // table_bits (the only size the archive header carries).
    // table_bits + 4 restores the historical default (22 -> 26).
    void normalize() {
        if (table_bits < 16) table_bits = 16;
        if (table_bits > 28) table_bits = 28;
        match_bits = table_bits;
        buf_bits = table_bits + 4;
        if (buf_bits > 28) buf_bits = 28;
    }
};

class Predictor {
 public:
    static constexpr int kExtraCtx =
        (HP_WORD_STREAMS ? 1 : 0) +
        (HP_BRACKET ? 1 : 0) +
        (HP_LINKWORD ? 1 : 0) +
        (HP_NUMERIC ? 1 : 0) +
        (HP_PAT_MODEL ? 1 : 0) +
        (HP_PPMD ? 1 : 0) +
        (HP_STEMMER ? HP_STEMMER_N : 0) +
        (HP_SENWORD ? 1 : 0) +
        (HP_SENT_STREAM ? 1 : 0) +
        (HP_SENT_MEM ? 1 : 0) +
        (HP_SENGRP_MOD ? 1 : 0);
    static constexpr int kCtxModels = 11 + kExtraCtx;
    static constexpr int kMatchModels = 5 + (HP_MATCH_18 ? 1 : 0)
        + (HP_MATCH_13 ? 1 : 0) + (HP_MATCH_01 ? 1 : 0)
        + (HP_MATCH_02 ? 1 : 0) + (HP_MATCH_05 ? 1 : 0)
        + (HP_MATCH_07 ? 1 : 0) + (HP_MATCH_09 ? 1 : 0)
        + (HP_MATCH_12 ? 1 : 0) + (HP_MATCH_20 ? 1 : 0);
    static constexpr int kWordMatch = HP_WORD_MATCH
        ? (HP_WORD_MATCH_N + (HP_WMATCH_4 ? 1 : 0) + (HP_WMATCH_5 ? 1 : 0)) : 0;
    static constexpr int kCtw = HP_CTW ? 1 : 0;
    static constexpr int kDiscovered =
        DiscoveryPool::kSlots * ContextModel::kOutputs;
    static constexpr int kBaseExperts =
        kCtxModels * ContextModel::kOutputs + 1 /*bias*/ + kMatchModels
        + kWordMatch + (HP_SPARSE_UTF8 ? 1 : 0)
        + 1 /*hebb*/ + kCtw + kDiscovered;
#if HP_HEDGE_L1
    static constexpr int kHedgeInputs = 0;
#else
    static constexpr int kHedgeInputs = 2;
#endif
    static constexpr int kNumExperts = kBaseExperts + kHedgeInputs;

    enum Gate {
        kGateC0 = 0, kGateAlpha, kGatePrev, kGateMatch, kGateEntropy,
        kGateHebb,
#if HP_EXTRA_GATES
        kGateWiki, kGatePattern,
#endif
#if HP_POS_GATE
        kGatePos,
#endif
#if HP_GATE_SHAPE
        kGateShape,
#endif
#if HP_GATE_BRANCH
        kGateBranch,
#endif
#if HP_GATE_DISP
        kGateDisp,
#endif
#if HP_GATE_MLEN2
        kGateMlen2,
#endif
#if HP_GATE_ARGMAX
        kGateArgmax,
#endif
#if HP_SEN_GROUP
        kGateSenGroup,
#endif
#if HP_GATE_BREAK
        kGateBreak,
#endif
#if HP_GATE_WORDPOS
        kGateWordPos,
#endif
#if HP_GATE_HEDGE
        kGateHedge,
#endif
#if HP_GATE_FWORD
        kGateFword,
#endif
#if HP_GATE_UTF8
        kGateUtf8,
#endif
#if HP_GATE_NEST
        kGateNest,
#endif
#if HP_GATE_AGREE
        kGateAgree,
#endif
#if HP_GATE_FCLASS
        kGateFclass,
#endif
#if HP_GATE_WMLEN
        kGateWmLen,
#endif
        kNumGates
    };

    static int slot_bits(int base, int delta) {
#if HP_SLOT_GROW
        if (delta < 0) delta = 0;
        int b = base + delta;
#if HP_SLOT_GROW_EXTRA
        if (delta >= 2) b += 1;
#endif
#if HP_SLOT_WORD2
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD3
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD4
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD5
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD6
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD7
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD8
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD9
        if (delta == 1) b += 1;
#endif
#if HP_SLOT_WORD10
        if (delta == 1) b += 1;
#endif
#elif HP_SLOT_SIZES
        int b = base + delta;
#else
        int b = base;
        (void)delta;
#endif
        if (b < 16) b = 16;
        if (b > HP_SLOT_MAX) b = HP_SLOT_MAX;
        return b;
    }

    static int match_bits(int b) {
        b += (HP_MATCH_GROW ? 1 : 0) + (HP_MATCH_GROW2 ? 1 : 0);
        if (b < 16) b = 16;
        if (b > 28) b = 28;
        return b;
    }

    static int add_bits(int b, int extra) {
        b += extra;
        if (b < 16) b = 16;
        if (b > HP_SLOT_MAX) b = HP_SLOT_MAX;
        return b;
    }

    explicit Predictor(const Config& cfg)
        : cfg_(cfg),
          o1_(add_bits(slot_bits(cfg.table_bits, -2), HP_SLOT_O12 ? 1 : 0), 1023),
          o2_(add_bits(slot_bits(cfg.table_bits, -2), HP_SLOT_O12 ? 1 : 0), 1023),
          o3_(add_bits(slot_bits(cfg.table_bits, 0),
                       (HP_SLOT_O34 ? 1 : 0) + (HP_SLOT_O34B ? 1 : 0)
                       + (HP_SLOT_O34C ? 1 : 0) + (HP_SLOT_O34D ? 1 : 0)
                       + (HP_SLOT_O34E ? 1 : 0) + (HP_SLOT_O34F ? 1 : 0)
                       + (HP_SLOT_O34G ? 1 : 0)), 511),
          o4_(add_bits(slot_bits(cfg.table_bits, 0),
                       (HP_SLOT_O34 ? 1 : 0) + (HP_SLOT_O34B ? 1 : 0)
                       + (HP_SLOT_O34C ? 1 : 0) + (HP_SLOT_O34D ? 1 : 0)
                       + (HP_SLOT_O34E ? 1 : 0) + (HP_SLOT_O34F ? 1 : 0)
                       + (HP_SLOT_O34G ? 1 : 0)), 255),
          o6_(add_bits(slot_bits(cfg.table_bits, 0),
                       (HP_SLOT_O6 ? 1 : 0) + (HP_SLOT_O6B ? 1 : 0)
                       + (HP_SLOT_O6C ? 1 : 0) + (HP_SLOT_O6D ? 1 : 0)
                       + (HP_SLOT_O6E ? 1 : 0) + (HP_SLOT_O6F ? 1 : 0)
                       + (HP_SLOT_O6G ? 1 : 0) + (HP_SLOT_O6H ? 1 : 0)
                       + (HP_SLOT_O6I ? 1 : 0) + (HP_SLOT_O6J ? 1 : 0)), 127),
          word_(slot_bits(cfg.table_bits, 1), 255),
          col_(slot_bits(cfg.table_bits, HP_SLOT_COL2 ? 2 : 0), 255),
          tag_(slot_bits(cfg.table_bits, 2), 255),
          wbi_(slot_bits(cfg.table_bits, 1), 255),
          sp13_(add_bits(slot_bits(cfg.table_bits, 0), HP_SLOT_SP ? 1 : 0), 255),
          sp24_(add_bits(slot_bits(cfg.table_bits, 0), HP_SLOT_SP ? 1 : 0), 255),
#if HP_WORD_STREAMS
          wstr_sp_(slot_bits(cfg.table_bits, HP_SLOT_WSTR2 ? 2 : 0), 255),
#endif
#if HP_BRACKET
          brk_(slot_bits(cfg.table_bits, HP_SLOT_BRK2 ? 2 : 0), 255),
#endif
#if HP_LINKWORD
          link_(slot_bits(cfg.table_bits, 2), 255),
#endif
#if HP_NUMERIC
          num_(slot_bits(cfg.table_bits, HP_SLOT_NUM2 ? 2 : 0), 255),
#endif
#if HP_PAT_MODEL
          pat_(cfg.table_bits, 255),
#endif
#if HP_PPMD
          ppm_(cfg.table_bits, 127),
#endif
#if HP_STEMMER
          stem0_(cfg.table_bits, 255),
#if HP_STEMMER_N >= 2
          stem1_(cfg.table_bits, 255),
#endif
#endif
#if HP_SENWORD
          sen_(slot_bits(cfg.table_bits, 2), 255),
#endif
#if HP_SENT_STREAM
          sentst_(slot_bits(cfg.table_bits, HP_SLOT_S3 ? (HP_SLOT_S4 ? 3 : 2) : 0), 255),
#endif
#if HP_SENT_MEM
          sentmem_cm_(slot_bits(cfg.table_bits, HP_SLOT_SMEM ? 2 : 0), 255),
#endif
#if HP_SENGRP_MOD
          sengrp_(slot_bits(cfg.table_bits, HP_SLOT_SGRP ? 2 : 0), 255),
#endif
          match_{ {cfg.buf_bits, match_bits(cfg.match_bits), 3},
                  {cfg.buf_bits, match_bits(cfg.match_bits), 4},
                  {cfg.buf_bits, match_bits(cfg.match_bits), 6},
                  {cfg.buf_bits, match_bits(cfg.match_bits), 10},
                  {cfg.buf_bits, match_bits(cfg.match_bits), 16}
#if HP_MATCH_18
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 8}
#endif
#if HP_MATCH_13
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 13}
#endif
#if HP_MATCH_01
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 1}
#endif
#if HP_MATCH_02
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 2}
#endif
#if HP_MATCH_05
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 5}
#endif
#if HP_MATCH_07
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 7}
#endif
#if HP_MATCH_09
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 9}
#endif
#if HP_MATCH_12
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 12}
#endif
#if HP_MATCH_20
                  , {cfg.buf_bits, match_bits(cfg.match_bits), 20}
#endif
          },
#if HP_SPARSE_UTF8
          smatch_(cfg.buf_bits, match_bits(cfg.match_bits), 4),
#endif
#if HP_WORD_MATCH
          wmatch_{
              WordMatchModel(cfg.buf_bits > 2 ? cfg.buf_bits - 2 : cfg.buf_bits,
                             cfg.match_bits > 2 ? cfg.match_bits - 2 : cfg.match_bits, 1),
              WordMatchModel(cfg.buf_bits > 2 ? cfg.buf_bits - 2 : cfg.buf_bits,
                             cfg.match_bits > 2 ? cfg.match_bits - 2 : cfg.match_bits, 2),
              WordMatchModel(cfg.buf_bits > 2 ? cfg.buf_bits - 2 : cfg.buf_bits,
                             cfg.match_bits > 2 ? cfg.match_bits - 2 : cfg.match_bits, 3)
#if HP_WMATCH_4
              , WordMatchModel(cfg.buf_bits > 2 ? cfg.buf_bits - 2 : cfg.buf_bits,
                               cfg.match_bits > 2 ? cfg.match_bits - 2 : cfg.match_bits, 3)
#endif
#if HP_WMATCH_5
              , WordMatchModel(cfg.buf_bits > 2 ? cfg.buf_bits - 2 : cfg.buf_bits,
                               cfg.match_bits > 2 ? cfg.match_bits - 2 : cfg.match_bits, 1)
#endif
          },
#endif
          hebb_(cfg.table_bits, 255),
          pool_(cfg.table_bits, 0xC0FFEEull),
          mixer_(kNumExperts, gate_sizes(), 256, cfg.mixer_lr, gate_rates(cfg.mixer_lr)),
          apm_c0_(256),
          apm_lex_(256 * 256),
          apm_gria_(GriaGate::kBuckets * 256),
#if HP_HEDGE_L1
          hedge_(kNumGates),
#else
          hedge_(kBaseExperts),
#endif
          bias_(256) {
        counter_init(bias_.data(), bias_.size());
#if HP_ENGLISH_PRIOR
        for (int i = 0; i < 256; ++i) {
            bias_[static_cast<std::size_t>(i)].p =
                static_cast<std::uint16_t>(english_bit_prior16(i));
        }
#endif
        gria_.set_enabled(cfg.gria);
        set_byte_contexts();
    }

    int predict() {
        mixer_.reset_inputs();
        mixer_.add(counter_predict(bias_[c0_]));
        exp_p_[0] = counter_predict_p(bias_[c0_]);

        n_exp_ = 0;
        int out[ContextModel::kOutputs];
        int backoff = counter_predict_p(bias_[c0_]);
        ContextModel* chain[kCtxModels];
        int nchain = 0;
        chain[nchain++] = &o1_;
        chain[nchain++] = &o2_;
        chain[nchain++] = &o3_;
        chain[nchain++] = &o4_;
        chain[nchain++] = &o6_;
        chain[nchain++] = &word_;
        chain[nchain++] = &sp13_;
        chain[nchain++] = &sp24_;
        chain[nchain++] = &col_;
        chain[nchain++] = &tag_;
        chain[nchain++] = &wbi_;
#if HP_WORD_STREAMS
        chain[nchain++] = &wstr_sp_;
#endif
#if HP_BRACKET
        chain[nchain++] = &brk_;
#endif
#if HP_LINKWORD
        chain[nchain++] = &link_;
#endif
#if HP_NUMERIC
        chain[nchain++] = &num_;
#endif
#if HP_PAT_MODEL
        chain[nchain++] = &pat_;
#endif
#if HP_PPMD
        chain[nchain++] = &ppm_;
#endif
#if HP_STEMMER
        chain[nchain++] = &stem0_;
#if HP_STEMMER_N >= 2
        chain[nchain++] = &stem1_;
#endif
#endif
#if HP_SENWORD
        chain[nchain++] = &sen_;
#endif
#if HP_SENT_STREAM
        chain[nchain++] = &sentst_;
#endif
#if HP_SENT_MEM
        chain[nchain++] = &sentmem_cm_;
#endif
#if HP_SENGRP_MOD
        chain[nchain++] = &sengrp_;
#endif
        for (int i = 0; i < nchain; ++i) {
            chain[i]->predict(c0_, backoff, out);
            for (int j = 0; j < ContextModel::kOutputs; ++j) {
                mixer_.add(out[j]);
                exp_p_[n_exp_++] = squash(out[j]);
            }
            if (i < 4) backoff = chain[i]->last_p();
            else if (i == 4) backoff = o1_.last_p();
        }
        for (int i = 0; i < kMatchModels; ++i) {
            const int ms = match_[i].predict(c0_, bitpos_);
            mixer_.add(ms);
            exp_p_[n_exp_++] = squash(ms);
        }
#if HP_SPARSE_UTF8
        {
            const int ms = smatch_.predict(c0_, bitpos_);
            mixer_.add(ms);
            exp_p_[n_exp_++] = squash(ms);
        }
#endif
#if HP_WORD_MATCH
        for (int i = 0; i < kWordMatch; ++i) {
            const int ms = wmatch_[i].predict(c0_, bitpos_);
            mixer_.add(ms);
            exp_p_[n_exp_++] = squash(ms);
        }
#endif
        {
            const int hs = hebb_.predict(c0_);
            mixer_.add(hs);
            exp_p_[n_exp_++] = squash(hs);
        }
#if HP_CTW
        {
            // Recursive KT weighting over the contiguous order chain.
            // P_o = (KT_o + P_{o-1}) / 2, which is CTW with β = 1/2.
            int p = backoff_kt_(bias_[c0_]);
            ContextModel* ord[5] = {&o1_, &o2_, &o3_, &o4_, &o6_};
            for (int i = 0; i < 5; ++i) {
                const int kt = py_estimate(ord[i]->n0(), ord[i]->n1(), p);
                p = (kt + p) >> 1;
            }
            mixer_.add(stretch(p));
            exp_p_[n_exp_++] = p;
        }
#endif
        {
            int dout[kDiscovered];
            pool_.predict(c0_, o1_.last_p(), dout);
            for (int i = 0; i < kDiscovered; ++i) {
                mixer_.add(dout[i]);
                exp_p_[n_exp_++] = squash(dout[i]);
            }
        }
        sparse_ = o6_.sparsity();

#if !HP_HEDGE_L1
        for (int e = 0; e < n_exp_ && e < kBaseExperts; ++e) hedge_.set(e, exp_p_[e]);
        const int ph = hedge_.mix();
        mixer_.add(stretch(ph));
        int wmax = 0;
        for (int e = 0; e < kBaseExperts; ++e) wmax = std::max(wmax, hedge_.weight(e));
        mixer_.add(clamp_int((wmax * kBaseExperts - 65536) >> 5, -2047, 2047));
#endif

        mixer_.set_ctx(kGateC0, c0_);
        mixer_.set_ctx(kGateAlpha, gria_.bucket());
        mixer_.set_ctx(kGatePrev, static_cast<int>(hist_ & 0xff));
        int mlen = 0;
        for (int i = 0; i < kMatchModels; ++i)
            if (match_[i].match_len() > mlen) mlen = match_[i].match_len();
#if HP_WORD_MATCH
        for (int i = 0; i < kWordMatch; ++i)
            if (wmatch_[i].match_len() > mlen) mlen = wmatch_[i].match_len();
#endif
        mixer_.set_ctx(kGateMatch, mlen > 31 ? 31 : mlen);
        mixer_.set_ctx(kGateHebb, hebb_.strength() > 15 ? 15 : hebb_.strength());
        mixer_.set_ctx(kGateEntropy, gria_.entropy_bucket());
#if HP_EXTRA_GATES
        mixer_.set_ctx(kGateWiki, wiki_.state() + (wiki_.is_paragraph() << 4));
        mixer_.set_ctx(kGatePattern, cache_.cls());
#endif
#if HP_POS_GATE
        mixer_.set_ctx(kGatePos, static_cast<int>(stems_.type() & 31u));
#endif
#if HP_GATE_SHAPE
        mixer_.set_ctx(kGateShape, shape6_bin(o6_.n0(), o6_.n1(), o6_.last_p()));
#endif
#if HP_GATE_DISP
        mixer_.set_ctx(kGateDisp, disp_var_bin(exp_p_, n_exp_));
#endif
#if HP_GATE_MLEN2
        {
            int l0 = 0, l1 = 0;
            for (int i = 0; i < kMatchModels; ++i) {
                const int l = match_[i].match_len();
                if (l > l0) { l1 = l0; l0 = l; }
                else if (l > l1) l1 = l;
            }
            mixer_.set_ctx(kGateMlen2, mlen2_bin(l0, l1));
        }
#endif
#if HP_GATE_ARGMAX
        mixer_.set_ctx(kGateArgmax, argmax_bin(exp_p_, n_exp_));
#endif
#if HP_GATE_BRANCH
        mixer_.set_ctx(kGateBranch, branch3_.bin());
#endif
#if HP_SEN_GROUP
        mixer_.set_ctx(kGateSenGroup, wiki_.sen_group());
#endif
#if HP_GATE_BREAK
        mixer_.set_ctx(kGateBreak, qlog_u32(static_cast<std::uint32_t>(break_age_ + 1), 16));
#endif
#if HP_GATE_WORDPOS
        mixer_.set_ctx(kGateWordPos, streams_.word_len() > 15 ? 15 : streams_.word_len());
#endif
#if HP_GATE_HEDGE
        {
            int wmax = 0;
            for (int d = 0; d < hedge_.size(); ++d)
                wmax = std::max(wmax, hedge_.weight(d));
            mixer_.set_ctx(kGateHedge, wmax >> 12 > 15 ? 15 : (wmax >> 12));
        }
#endif
#if HP_GATE_FWORD
        mixer_.set_ctx(kGateFword, streams_.first_word_bin());
#endif
#if HP_GATE_UTF8
        mixer_.set_ctx(kGateUtf8, utf8left_ > 3 ? 3 : utf8left_);
#endif
#if HP_GATE_NEST
        mixer_.set_ctx(kGateNest, wiki_.nest_markup() ? 1 : 0);
#endif
#if HP_GATE_AGREE
        mixer_.set_ctx(kGateAgree, agree_bin(exp_p_, n_exp_));
#endif
#if HP_GATE_FCLASS
        mixer_.set_ctx(kGateFclass, streams_.first_class());
#endif
#if HP_GATE_WMLEN
        {
            int wml = 0;
#if HP_WORD_MATCH
            for (int i = 0; i < kWordMatch; ++i)
                if (wmatch_[i].match_len() > wml) wml = wmatch_[i].match_len();
#endif
            mixer_.set_ctx(kGateWmLen, wml > 15 ? 15 : wml);
        }
#endif
        mixer_.set_ctx2(c0_);
        int pr = mixer_.mix();

#if HP_HEDGE_L1
        for (int j = 0; j < mixer_.num_layer1(); ++j)
            hedge_.set(j, mixer_.layer1_p(j));
        const int ph = hedge_.mix();
        pr = (pr + ph) >> 1;
#endif
        mixed_p_ = pr;

        const int a = apm_c0_.refine(pr, c0_);
        const int b = apm_lex_.refine(pr, static_cast<int>(hist_ & 0xff) * 256 + c0_);
        const int g = apm_gria_.refine(pr, gria_.bucket() * 256 + c0_);
        pr_final_ = clamp_int((HP_W0 * pr + HP_WA * a + HP_WB * b + HP_WG * g) >> 3, 1, 4094);
        return pr_final_;
    }

    void update(int y) {
        gria_.account_bit(y ? pr_final_ : 4096 - pr_final_);

        mixer_.update(y);
        hedge_.update(y, gria_.switch_rate_q16());
        apm_c0_.update(y);
        apm_lex_.update(y);
        apm_gria_.update(y);

        counter_update(bias_[c0_], y, 1023);
        const int ens = pr_final_;
        o1_.update(y, ens); o2_.update(y, ens); o3_.update(y, ens);
        o4_.update(y, ens); o6_.update(y, ens); word_.update(y, ens);
        sp13_.update(y, ens); sp24_.update(y, ens);
        col_.update(y, ens); tag_.update(y, ens); wbi_.update(y, ens);
#if HP_WORD_STREAMS
        wstr_sp_.update(y, ens);
#endif
#if HP_BRACKET
        brk_.update(y, ens);
#endif
#if HP_LINKWORD
        link_.update(y, ens);
#endif
#if HP_NUMERIC
        num_.update(y, ens);
#endif
#if HP_PAT_MODEL
        pat_.update(y, ens);
#endif
#if HP_PPMD
        ppm_.update(y, ens);
#endif
#if HP_STEMMER
        stem0_.update(y, ens);
#if HP_STEMMER_N >= 2
        stem1_.update(y, ens);
#endif
#endif
#if HP_SENWORD
        sen_.update(y, ens);
#endif
#if HP_SENT_STREAM
        sentst_.update(y, ens);
#endif
#if HP_SENT_MEM
        sentmem_cm_.update(y, ens);
#endif
#if HP_SENGRP_MOD
        sengrp_.update(y, ens);
#endif
        for (int i = 0; i < kMatchModels; ++i) match_[i].update(y);
#if HP_SPARSE_UTF8
        smatch_.update(y);
#endif
#if HP_WORD_MATCH
        for (int i = 0; i < kWordMatch; ++i) wmatch_[i].update(y);
#endif
        hebb_.update(y);
        pool_.update(y, y ? (4096 - pr_final_) >> 4 : pr_final_ >> 4);

        c0_ = (c0_ << 1) | y;
        ++bitpos_;
        if (bitpos_ == 8) {
            const int byte = c0_ & 0xff;
            c0_ = 1;
            bitpos_ = 0;
            end_of_byte(byte);
        }
    }

    const GriaGate& gria() const { return gria_; }
    int discovery_replacements() const { return pool_.replaced(); }
    int cache_hits() const { return cache_.hits(); }
    int cache_lookups() const { return cache_.lookups(); }
    const PatternCache& patterns() const { return cache_; }

    int expert_count() const { return n_exp_; }
    int expert_p(int i) const { return exp_p_[i]; }
    int mixed_p() const { return mixed_p_; }
    int sparse_fraction() const { return sparse_; }

 private:
    static std::vector<int> gate_sizes() {
        std::vector<int> s = {256, GriaGate::kBuckets, 256, 32,
                              GriaGate::kEntBuckets, 16};
#if HP_EXTRA_GATES
        s.push_back(32);   // wiki state × isParagraph
        s.push_back(PatternCache::kNClass);
#endif
#if HP_POS_GATE
        s.push_back(32);
#endif
#if HP_GATE_SHAPE
        s.push_back(16);
#endif
#if HP_GATE_BRANCH
        s.push_back(16);
#endif
#if HP_GATE_DISP
        s.push_back(16);
#endif
#if HP_GATE_MLEN2
        s.push_back(16);
#endif
#if HP_GATE_ARGMAX
        s.push_back(16);
#endif
#if HP_SEN_GROUP
        s.push_back(4);
#endif
#if HP_GATE_BREAK
        s.push_back(16);
#endif
#if HP_GATE_WORDPOS
        s.push_back(16);
#endif
#if HP_GATE_HEDGE
        s.push_back(16);
#endif
#if HP_GATE_FWORD
        s.push_back(16);
#endif
#if HP_GATE_UTF8
        s.push_back(4);
#endif
#if HP_GATE_NEST
        s.push_back(2);
#endif
#if HP_GATE_AGREE
        s.push_back(16);
#endif
#if HP_GATE_FCLASS
        s.push_back(4);
#endif
#if HP_GATE_WMLEN
        s.push_back(16);
#endif
        return s;
    }

    static std::vector<int> gate_rates(int base) {
#if HP_PER_MIXER_LR
        // Spread inspired by fx2-cmix's 0.0003–0.005, mapped onto hp's
        // integer lr where 2 is the historical shared default.
        (void)base;
        std::vector<int> r = {2, 3, 2, 4, 3, 4};
#if HP_EXTRA_GATES
        r.push_back(3);
        r.push_back(3);
#endif
#if HP_POS_GATE
        r.push_back(3);
#endif
#if HP_GATE_SHAPE
        r.push_back(3);
#endif
#if HP_GATE_BRANCH
        r.push_back(3);
#endif
#if HP_GATE_DISP
        r.push_back(3);
#endif
#if HP_GATE_MLEN2
        r.push_back(3);
#endif
#if HP_GATE_ARGMAX
        r.push_back(3);
#endif
#if HP_SEN_GROUP
        r.push_back(3);
#endif
#if HP_GATE_BREAK
        r.push_back(3);
#endif
#if HP_GATE_WORDPOS
        r.push_back(3);
#endif
#if HP_GATE_HEDGE
        r.push_back(3);
#endif
#if HP_GATE_FWORD
        r.push_back(3);
#endif
#if HP_GATE_UTF8
        r.push_back(3);
#endif
#if HP_GATE_NEST
        r.push_back(3);
#endif
#if HP_GATE_AGREE
        r.push_back(3);
#endif
#if HP_GATE_FCLASS
        r.push_back(3);
#endif
#if HP_GATE_WMLEN
        r.push_back(3);
#endif
        return r;
#else
        return std::vector<int>(static_cast<std::size_t>(kNumGates), base);
#endif
    }

    static int backoff_kt_(const Counter& c) {
        return counter_predict_p(c);
    }

#if HP_PRED_GATE
    bool pred_gate_mute() const {
        return wiki_.nest_markup() ||
               brackets_.square_depth() > 0 ||
               brackets_.curly_depth() > 0;
    }
#endif

    std::uint32_t h2(std::uint64_t salt, std::uint64_t key) {
#if HP_PATTERN_CACHE
        return cache_.hash_memo(salt, key);
#else
        return hash2(salt, key);
#endif
    }

    void end_of_byte(int byte) {
        hist_ = (hist_ << 8) | static_cast<std::uint64_t>(byte);

        const bool alnum = (byte >= 'a' && byte <= 'z') ||
                           (byte >= 'A' && byte <= 'Z') ||
                           (byte >= '0' && byte <= '9');
        const int at_boundary = (!alnum && word_hash_ != 0) ? 1 : 0;
        const bool letter = (byte >= 'a' && byte <= 'z') ||
                            (byte >= 'A' && byte <= 'Z');
        if (alnum) {
            word_hash_ = mix64(word_hash_ * 0x100000001B3ull +
                               static_cast<std::uint64_t>(byte | 0x20));
        } else {
            word_hash_ = 0;
        }
        if (letter) {
            letter_hash_ = mix64(letter_hash_ * 0x100000001B3ull +
                                 static_cast<std::uint64_t>(byte | 0x20));
        } else {
            letter_hash_ = 0;
        }

        if (byte == '\n') {
            for (int i = 0; i < kLineMax; ++i)
                prev_line_[i] = (i < col_pos_) ? cur_line_[i] : 0;
            col_pos_ = 0;
        } else {
            if (col_pos_ < kLineMax) cur_line_[col_pos_] = static_cast<std::uint8_t>(byte);
            if (col_pos_ < kLineMax - 1) ++col_pos_;
        }

#if HP_WIKI_STATES
        wiki_.push(byte);
#else
        if (byte == '<') { in_tag_ = 1; tag_name_ = 0; if (tag_depth_ < 15) ++tag_depth_; }
        else if (byte == '>') { in_tag_ = 0; }
        else if (byte == '/' && in_tag_) { if (tag_depth_ > 0) --tag_depth_; }
        else if (in_tag_) tag_name_ = mix64(tag_name_ * 31 + byte);
#endif

#if HP_BRACKET
        brackets_.push(byte);
#endif
        streams_.push(byte, alnum);
#if HP_SENT_MEM
#if HP_SENT_DOM
        sentmem_.set_domain(wiki_.sen_group());
#endif
        if (at_boundary) sentmem_.push_word(word_hash_prev_);
        if (byte == '.' || byte == '!' || byte == '?' || byte == '\n')
            sentmem_.end_sentence();
#endif
#if HP_STEMMER || HP_STEM_FOLD || HP_POS_GATE || HP_WT3_CTX
        {
            const int nest = wiki_.nest_markup() ||
                             brackets_.square_depth() > 0 ||
                             brackets_.curly_depth() > 0;
            stems_.push(byte, nest);
        }
#endif
#if HP_NUMERIC
        numbers_.push(byte);
#endif

        if (!alnum && word_hash_prev_ != 0) {
            hebb_.potentiate(prev_word_, word_hash_prev_);
            prev_word_ = word_hash_prev_;
            word_ring_[3] = word_ring_[2];
            word_ring_[2] = word_ring_[1];
            word_ring_[1] = word_ring_[0];
            word_ring_[0] = prev_word_;
        }
        word_hash_prev_ = word_hash_;
        hebb_.set_context(prev_word_);
#if HP_GATE_BRANCH
        branch3_.push_byte(byte, hist_);
#endif
        hist2_ = (hist2_ << 8) | ((hist_ >> 56) & 0xffull);
        pool_.end_byte();
        pool_.set_contexts(hist_, hist2_);

        for (int i = 0; i < kMatchModels; ++i) match_[i].push_byte(byte, hist_);
#if HP_GATE_BREAK
        {
            int ml = 0;
            for (int i = 0; i < kMatchModels; ++i)
                if (match_[i].match_len() > ml) ml = match_[i].match_len();
            break_age_ = ml > 0 ? (break_age_ < 255 ? break_age_ + 1 : 255) : 0;
        }
#endif
#if HP_SPARSE_UTF8
        {
            std::uint64_t sh = 0;
            for (int i = 0; i < 4; ++i)
                sh = (sh << 8) | ((hist_ >> (16 * i)) & 0xffull);
            smatch_.push_byte(byte, sh);
        }
#endif
#if HP_WORD_MATCH
        // fx2 keys: {0} current, {1,3} current-alt + word-before-prev, {7,2} letters + last
        const std::uint64_t w0 = word_hash_ ? word_hash_ : word_ring_[0];
        const std::uint64_t w13 = mix64(word_hash_ * 263ull + word_ring_[1]);
        const std::uint64_t w72 = mix64(letter_hash_ * 997ull + word_ring_[0]);
#if HP_WMATCH_4
        const std::uint64_t w123 = mix64(word_hash_ * 31ull + word_ring_[0] * 17ull +
                                         word_ring_[1]);
#endif
        std::uint64_t whist[5] = {w0, w13, w72, 0, 0};
        int nw = 3;
#if HP_WMATCH_4
        whist[nw++] = w123;
#endif
#if HP_WMATCH_5
        whist[nw++] = streams_.stream(1);
#endif
        (void)nw;
        for (int i = 0; i < kWordMatch; ++i)
            wmatch_[i].push_byte(byte, whist[i], at_boundary);
#endif

#if HP_UTF8_IDLE || HP_GATE_UTF8
        if (utf8left_ > 0) {
            if ((byte & 0xC0) == 0x80) --utf8left_;
            else utf8left_ = 0;
        }
        if (byte >= 0xC0 && byte < 0xE0) utf8left_ = 1;
        else if (byte >= 0xE0 && byte < 0xF0) utf8left_ = 2;
        else if (byte >= 0xF0 && byte < 0xF8) utf8left_ = 3;
#endif
        cache_.observe_byte(byte, wiki_.state(), wiki_.in_table(),
                            streams_.first_class());

        gria_.account_byte(byte);
        set_byte_contexts();
    }

    void set_byte_contexts() {
        const int col = (col_pos_ < kLineMax) ? col_pos_ : kLineMax - 1;
#if HP_TABLE_ABOVE
        if (wiki_.in_table())
            col_.set_context(h2(21, (static_cast<std::uint64_t>(wiki_.above_cell()) << 16) |
                                        static_cast<std::uint64_t>(col & 63)));
        else
#endif
        col_.set_context(h2(21, (static_cast<std::uint64_t>(prev_line_[col]) << 16) |
                                    static_cast<std::uint64_t>(col & 63)));
#if HP_WIKI_STATES
        tag_.set_context(h2(22, wiki_.context_key()));
#else
        tag_.set_context(h2(22, (static_cast<std::uint64_t>(tag_depth_ & 15) << 40) |
                                    (static_cast<std::uint64_t>(in_tag_) << 39) |
                                    (tag_name_ & 0x7FFFFFFFFFull)));
#endif
#if HP_WBI_SENTPOS
        wbi_.set_context(h2(23, prev_word_ * 0x9E3779B97F4A7C15ull + word_hash_ +
                               static_cast<std::uint64_t>(streams_.sent_pos()) * 17ull));
#elif HP_WBI_GRP
        wbi_.set_context(h2(23, prev_word_ * 0x9E3779B97F4A7C15ull + word_hash_ +
                               static_cast<std::uint64_t>(wiki_.sen_group()) * 131ull));
#else
        wbi_.set_context(h2(23, prev_word_ * 0x9E3779B97F4A7C15ull + word_hash_));
#endif

        o1_.set_context(h2(1, hist_ & 0xffull));
        o2_.set_context(h2(2, hist_ & 0xffffull));
        o3_.set_context(h2(3, hist_ & 0xffffffull));
        o4_.set_context(h2(4, hist_ & 0xffffffffull));
        o6_.set_context(h2(6, hist_ & 0xffffffffffffull));
#if HP_SECTION_MUTE
        if (wiki_.mute_words())
            word_.set_idle();
        else
#endif
#if HP_PRED_GATE
        if (pred_gate_mute())
            word_.set_idle();
        else
#endif
#if HP_UTF8_IDLE
        if (utf8left_ > 0)
            word_.set_idle();
        else
#endif
#if HP_STEM_FOLD
        word_.set_context(h2(7, word_hash_ ^ (stems_.sentence() * 0x9E3779B97F4A7C15ull)));
#elif HP_SENT_RECENCY
        word_.set_context(h2(7, word_hash_ * 1471ull + streams_.recency_at() +
                               (hist_ & 0xffull)));
#elif HP_WORD_GRP
        word_.set_context(h2(7, word_hash_ +
                               static_cast<std::uint64_t>(wiki_.sen_group()) * 131ull));
#else
        word_.set_context(h2(7, word_hash_));
#endif
        sp13_.set_context(h2(8, ((hist_ >> 0) & 0xffull) |
                                   (((hist_ >> 16) & 0xffull) << 8)));
        sp24_.set_context(h2(9, ((hist_ >> 8) & 0xffull) |
                                   (((hist_ >> 24) & 0xffull) << 8)));
#if HP_WORD_STREAMS
#if HP_SECTION_MUTE
        if (wiki_.mute_words())
            wstr_sp_.set_idle();
        else
#endif
#if HP_PRED_GATE
        if (pred_gate_mute())
            wstr_sp_.set_idle();
        else
#endif
#if HP_UTF8_IDLE
        if (utf8left_ > 0)
            wstr_sp_.set_idle();
        else
#endif
#if HP_STEM_FOLD
        wstr_sp_.set_context(h2(25, streams_.prev0() * 131ull + streams_.stream(2) +
                                   stems_.typed() * 17ull + stems_.paragraph()));
#elif HP_FIRST_WORD
        wstr_sp_.set_context(h2(25, streams_.prev0() * 131ull + streams_.stream(2) +
                                   streams_.first_word() * 89ull));
#elif HP_WT3_CTX
        wstr_sp_.set_context(h2(25, streams_.prev0() * 131ull + streams_.stream(2) +
                                   stems_.wt3() * 17ull));
#else
#if HP_WSTR_GRP
        wstr_sp_.set_context(h2(25, streams_.prev0() * 131ull + streams_.stream(2) +
                               static_cast<std::uint64_t>(wiki_.sen_group()) * 131ull));
#else
        wstr_sp_.set_context(h2(25, streams_.prev0() * 131ull + streams_.stream(2)));
#endif
#endif
#endif
#if HP_BRACKET
        {
            std::uint64_t bk = brackets_.context_key();
#if HP_BRK_CLOSE
            bk += static_cast<std::uint64_t>(brackets_.closer()) << 16;
#endif
#if HP_QUOTE_STACK
            bk += static_cast<std::uint64_t>(brackets_.quote()) << 24;
#endif
            brk_.set_context(h2(26, bk));
        }
#endif
#if HP_LINKWORD
#if HP_UTF8_IDLE
        if (utf8left_ > 0) {
            link_.set_idle();
#if HP_SENWORD
            sen_.set_idle();
#endif
        } else
#endif
        {
            const std::uint64_t lw = wiki_.linkword();
#if HP_SENWORD
#if HP_LINK_NUM
            link_.set_context(h2(29, (lw ? lw : word_hash_) * 3301ull +
                                   numbers_.previous() * 3191ull));
#else
            link_.set_context(h2(29, lw));
#endif
            {
                const std::uint64_t sw = wiki_.senword();
                sen_.set_context(h2(31, sw ? sw * 1471ull + (hist_ & 0xffull) : 0));
            }
#else
            const std::uint64_t sw = wiki_.senword();
            const std::uint64_t key = lw ? lw : (sw ? sw * 1471ull + (hist_ & 0xffull) : 0);
#if HP_LINK_NUM
            link_.set_context(h2(29, key * 3301ull + numbers_.previous() * 3191ull));
#else
            link_.set_context(h2(29, key));
#endif
#endif
        }
#endif
#if HP_STEMMER
#if HP_PRED_GATE
        if (pred_gate_mute()) {
            stem0_.set_idle();
#if HP_STEMMER_N >= 2
            stem1_.set_idle();
#endif
        } else
#endif
        {
            stem0_.set_context(h2(32, stems_.ctx0() + stems_.ctx1()));
#if HP_STEMMER_N >= 2
            stem1_.set_context(h2(33, stems_.ctx1()));
#endif
        }
#endif
#if HP_SENT_STREAM
#if HP_SENT_GRP_CTX
        sentst_.set_context(h2(34, streams_.stream(3) * 83ull + (hist_ & 0xffull) +
                               static_cast<std::uint64_t>(wiki_.sen_group()) * 131ull));
#else
        sentst_.set_context(h2(34, streams_.stream(3) * 83ull + (hist_ & 0xffull)));
#endif
#endif
#if HP_SENT_MEM
#if HP_SENT_CUR
        sentmem_cm_.set_context(h2(35, sentmem_.match_hash() * 53ull +
                                       sentmem_.cur_hash() * 17ull +
                                       static_cast<std::uint64_t>(sentmem_.word_pos()) +
                                       (hist_ & 0xffull)));
#elif HP_SENT_ALIGN
        sentmem_cm_.set_context(h2(35, sentmem_.aligned_word() * 53ull +
                                       static_cast<std::uint64_t>(sentmem_.word_pos()) +
                                       (hist_ & 0xffull)));
#elif HP_SMEM_GRP
        sentmem_cm_.set_context(h2(35, sentmem_.match_hash() * 53ull +
                                       static_cast<std::uint64_t>(sentmem_.word_pos()) +
                                       (hist_ & 0xffull) +
                                       static_cast<std::uint64_t>(wiki_.sen_group()) * 131ull));
#else
        sentmem_cm_.set_context(h2(35, sentmem_.match_hash() * 53ull +
                                       static_cast<std::uint64_t>(sentmem_.word_pos()) +
                                       (hist_ & 0xffull)));
#endif
#endif
#if HP_NUMERIC
        num_.set_context(h2(30, numbers_.context_key()));
#endif
#if HP_PAT_MODEL
        pat_.set_context(h2(27, (static_cast<std::uint64_t>(cache_.cls()) << 16) |
                                   (hist_ & 0xffull)));
#endif
#if HP_PPMD
        ppm_.set_context(h2(28, hist_));
#endif
#if HP_SENGRP_MOD
#if HP_SENGRP_WORD
        sengrp_.set_context(h2(36, static_cast<std::uint64_t>(wiki_.sen_group()) +
                                   ((hist_ & 0xffffffull) << 8) + word_hash_ * 17ull));
#else
        sengrp_.set_context(h2(36, static_cast<std::uint64_t>(wiki_.sen_group()) +
                                   ((hist_ & 0xffffffull) << 8)));
#endif
#endif
    }

    Config cfg_;
    ContextModel o1_, o2_, o3_, o4_, o6_, word_;
    ContextModel col_, tag_, wbi_;
    ContextModel sp13_, sp24_;
#if HP_WORD_STREAMS
    ContextModel wstr_sp_;
#endif
#if HP_BRACKET
    ContextModel brk_;
#endif
#if HP_LINKWORD
    ContextModel link_;
#endif
#if HP_NUMERIC
    ContextModel num_;
#endif
#if HP_PAT_MODEL
    ContextModel pat_;
#endif
#if HP_PPMD
    ContextModel ppm_;
#endif
#if HP_STEMMER
    ContextModel stem0_;
#if HP_STEMMER_N >= 2
    ContextModel stem1_;
#endif
#endif
#if HP_SENWORD
    ContextModel sen_;
#endif
#if HP_SENT_STREAM
    ContextModel sentst_;
#endif
#if HP_SENT_MEM
    SentenceMemory sentmem_;
    ContextModel sentmem_cm_;
#endif
#if HP_SENGRP_MOD
    ContextModel sengrp_;
#endif
    MatchModel match_[kMatchModels];
#if HP_SPARSE_UTF8
    MatchModel smatch_;
#endif
#if HP_WORD_MATCH
    WordMatchModel wmatch_[3 + (HP_WMATCH_4 ? 1 : 0) + (HP_WMATCH_5 ? 1 : 0)];
#endif
    HebbianModel hebb_;
    DiscoveryPool pool_;
    MixerNet mixer_;
    APM apm_c0_, apm_lex_, apm_gria_;
    Hedge hedge_;
    std::vector<Counter> bias_;
    GriaGate gria_;
    WikiMachine wiki_;
    WordStreams streams_;
#if HP_STEMMER || HP_STEM_FOLD || HP_POS_GATE || HP_WT3_CTX
    StemStreams stems_;
#endif
    BracketMachine brackets_;
    PatternCache cache_;
#if HP_NUMERIC
    NumericField numbers_;
#endif
#if HP_GATE_BRANCH
    Branch3 branch3_;
#endif
#if HP_GATE_BREAK
    int break_age_ = 0;
#endif

    std::uint64_t hist_ = 0;
    std::uint64_t word_hash_ = 0;
    std::uint64_t letter_hash_ = 0;
    std::uint64_t hist2_ = 0;
    static constexpr int kLineMax = 256;
    std::uint8_t prev_line_[kLineMax] = {0};
    std::uint8_t cur_line_[kLineMax] = {0};
    int col_pos_ = 0;
    int tag_depth_ = 0;
    int in_tag_ = 0;
    std::uint64_t tag_name_ = 0;
    std::uint64_t prev_word_ = 0;
    std::uint64_t word_hash_prev_ = 0;
    std::uint64_t word_ring_[4] = {0, 0, 0, 0};
    int c0_ = 1;
    int bitpos_ = 0;
    int pr_final_ = 2048;
    int exp_p_[kNumExperts + 8] = {0};
    int n_exp_ = 0;
    int mixed_p_ = 2048;
    int sparse_ = 0;
#if HP_UTF8_IDLE || HP_GATE_UTF8
    int utf8left_ = 0;
#endif
};

}  // namespace hp
