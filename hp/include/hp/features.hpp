#pragma once
//
// hp/features.hpp — compile-time ablation flags.
//
// Each ported model / mixer change is a switch so A.3 can accept or reject
// one axis at a time. Defaults are the high-confidence set from PLAN.md.
// Override with -DHP_WIKI_STATES=0 etc.
//
// NOTHING here is transmitted. Encoder and decoder are the same binary, so
// they compile the same set. Changing a flag produces a different archive
// family (kVersion bumped when the default set changes).

#ifndef HP_WIKI_STATES
#define HP_WIKI_STATES 1
#endif
#ifndef HP_WORD_MATCH
#define HP_WORD_MATCH 1
#endif
#ifndef HP_WORD_MATCH_N
#define HP_WORD_MATCH_N 3          // 1-, 2-, 3-word keys
#endif
#ifndef HP_WMATCH_4
#define HP_WMATCH_4 0              // extra word-match {current, last two}
#endif
#ifndef HP_WMATCH_5
#define HP_WMATCH_5 0              // extra word-match case-preserving current
#endif
#ifndef HP_WORD_STREAMS
#define HP_WORD_STREAMS 1          // first-char + prev only; case stream dropped
#endif
#ifndef HP_LINKWORD
#define HP_LINKWORD 1              // dedicated linkword/senword model
#endif
#ifndef HP_NUMERIC
#define HP_NUMERIC 1
#endif
#ifndef HP_PAT_MODEL
#define HP_PAT_MODEL 0             // A.3 reject: r=0.971 vs bracket
#endif
#ifndef HP_BRACKET
#define HP_BRACKET 1
#endif
#ifndef HP_PER_MIXER_LR
#define HP_PER_MIXER_LR 1
#endif
#ifndef HP_PATTERN_CACHE
#define HP_PATTERN_CACHE 1
#endif
#ifndef HP_META_PATTERNS
#define HP_META_PATTERNS 1
#endif
#ifndef HP_NCL
#define HP_NCL 1
#endif
#ifndef HP_NCL_LAMBDA
#define HP_NCL_LAMBDA 4            // Q8; 4/256 ≈ 0.016
#endif
#ifndef HP_HEDGE_L1
#define HP_HEDGE_L1 1
#endif
#ifndef HP_CTW
#define HP_CTW 0                  // A.3 reject: mean|r|=0.796, twin o2:py 0.930
#endif
#ifndef HP_ENGLISH_PRIOR
#define HP_ENGLISH_PRIOR 1
#endif
#ifndef HP_PPMD
#define HP_PPMD 0                  // D3: default off, likely saturated
#endif
#ifndef HP_NIBBLE_BUCKETS
#define HP_NIBBLE_BUCKETS 0
#endif
#ifndef HP_STATE_CAP
#define HP_STATE_CAP 20
#endif
#ifndef HP_DISC_SLOTS
#define HP_DISC_SLOTS 12
#endif
#ifndef HP_DISC_EVAL
#define HP_DISC_EVAL 1024
#endif
#ifndef HP_EXTRA_GATES
#define HP_EXTRA_GATES 1           // wiki + pattern-class gates
#endif
#ifndef HP_SECTION_MUTE
#define HP_SECTION_MUTE 0          // H2.2: mute word models in refs/math
#endif
#ifndef HP_MIXER_SKIP
#define HP_MIXER_SKIP 0            // H3.1: skip mixer update if |err| < this
#endif
#ifndef HP_SPARSE_UTF8
#define HP_SPARSE_UTF8 0           // H2.3: gap-1 match for escaped UTF-8
#endif
#ifndef HP_STEMMER
#define HP_STEMMER 0               // H2.1: Porter2 + POS as extra experts
#endif
#ifndef HP_STEMMER_N
#define HP_STEMMER_N 2
#endif
#ifndef HP_STEM_FOLD
#define HP_STEM_FOLD 0             // H2.1b: fold stem hashes into word_/wstr_
#endif
#ifndef HP_SENWORD
#define HP_SENWORD 0               // H2.5: senword own expert (linkword stays)
#endif
#ifndef HP_PRED_GATE
#define HP_PRED_GATE 0             // H2.7: idle word models in table/link/{}
#endif
#ifndef HP_POS_GATE
#define HP_POS_GATE 0              // H2.1c: mixer gate from stem POS type
#endif
#ifndef HP_GATE_SHAPE
#define HP_GATE_SHAPE 0            // st_shape6 mixer gate
#endif
#ifndef HP_GATE_BRANCH
#define HP_GATE_BRANCH 0           // rec_branch3 mixer gate
#endif
#ifndef HP_GATE_DISP
#define HP_GATE_DISP 0             // disp_var mixer gate
#endif
#ifndef HP_GATE_MLEN2
#define HP_GATE_MLEN2 0            // second-longest match mixer gate
#endif
#ifndef HP_GATE_ARGMAX
#define HP_GATE_ARGMAX 0           // loudest-expert mixer gate
#endif
#ifndef HP_SEN_GROUP
#define HP_SEN_GROUP 0             // prose/list/table/link mixer gate
#endif
#ifndef HP_FIRST_WORD
#define HP_FIRST_WORD 0            // fold sentence first-word into wstr
#endif
#ifndef HP_LINK_NUM
#define HP_LINK_NUM 0              // fold number0 into linkword context
#endif
#ifndef HP_GATE_BREAK
#define HP_GATE_BREAK 0            // match-tenure mixer gate
#endif
#ifndef HP_SLOT_SIZES
#define HP_SLOT_SIZES 0            // H2.9: per-model table bit offsets
#endif
#ifndef HP_SLOT_GROW
#define HP_SLOT_GROW 0             // grow tag/word only; do not shrink o1/o2
#endif
#ifndef HP_STATE_TABLE2
#define HP_STATE_TABLE2 0          // H2.10: PY uses capped n0/n1
#endif
#ifndef HP_GATE_WORDPOS
#define HP_GATE_WORDPOS 0          // in-word position mixer gate
#endif
#ifndef HP_GATE_HEDGE
#define HP_GATE_HEDGE 0            // max hedge weight mixer gate
#endif
#ifndef HP_SENT_RECENCY
#define HP_SENT_RECENCY 0          // prev-sentence word at same offset into word_
#endif
#ifndef HP_WT3_CTX
#define HP_WT3_CTX 0               // rolling POS-bucket trail into wstr
#endif
#ifndef HP_GATE_FWORD
#define HP_GATE_FWORD 0            // sentence first-word mixer gate (not fold)
#endif
#ifndef HP_SLOT_GROW_EXTRA
#define HP_SLOT_GROW_EXTRA 0       // tag/link/sen +3 instead of +2
#endif
#ifndef HP_WIKI_TEMP
#define HP_WIKI_TEMP 0             // {name|...} template bit in tag_
#endif
#ifndef HP_SENT_STREAM
#define HP_SENT_STREAM 0           // sentence-boundary word as own expert
#endif
#ifndef HP_SENT_MEM
#define HP_SENT_MEM 0              // similar-sentence memory expert
#endif
#ifndef HP_SENT_MEM_BIG
#define HP_SENT_MEM_BIG 0          // 128-slot / 32-word sentence ring
#endif
#ifndef HP_SENT_ALIGN
#define HP_SENT_ALIGN 0            // ctx = matched sentence word at index
#endif
#ifndef HP_SLOT_SMEM
#define HP_SLOT_SMEM 0             // grow sentmem table +2
#endif
#ifndef HP_SENT_DOM
#define HP_SENT_DOM 0              // prose/table/link/template sentence rings
#endif
#ifndef HP_UTF8_IDLE
#define HP_UTF8_IDLE 0             // idle word models on UTF-8 continuation
#endif
#ifndef HP_MATCH_18
#define HP_MATCH_18 0              // extra byte-match order 8
#endif
#ifndef HP_TABLE_ABOVE
#define HP_TABLE_ABOVE 0           // cell-aligned byte from row above
#endif
#ifndef HP_NLCHAR
#define HP_NLCHAR 0                // LF / table / header line-mode nibble
#endif
#ifndef HP_QUOTE_STACK
#define HP_QUOTE_STACK 0           // '' / "" toggle into bracket ctx
#endif
#ifndef HP_BRK_CLOSE
#define HP_BRK_CLOSE 0             // expected closer byte into bracket ctx
#endif
#ifndef HP_GATE_UTF8
#define HP_GATE_UTF8 0             // UTF-8 leftover mixer gate (no idle)
#endif
#ifndef HP_GATE_NEST
#define HP_GATE_NEST 0             // markup-nest mixer gate
#endif
#ifndef HP_GATE_AGREE
#define HP_GATE_AGREE 0            // expert-agreement mixer gate
#endif
#ifndef HP_GATE_FCLASS
#define HP_GATE_FCLASS 0           // first-char-class mixer gate
#endif
#ifndef HP_WBI_SENTPOS
#define HP_WBI_SENTPOS 0           // sentence word index into wbi_
#endif
#ifndef HP_GATE_WMLEN
#define HP_GATE_WMLEN 0            // longest word-match mixer gate
#endif
#ifndef HP_SLOT_WORD2
#define HP_SLOT_WORD2 0            // word/wbi +2 instead of +1
#endif
#ifndef HP_SLOT_COL2
#define HP_SLOT_COL2 0             // grow col_ +2
#endif
#ifndef HP_MATCH_13
#define HP_MATCH_13 0              // extra byte-match order 13
#endif
#ifndef HP_MATCH_01
#define HP_MATCH_01 0              // extra byte-match order 1
#endif
#ifndef HP_MATCH_02
#define HP_MATCH_02 0              // extra byte-match order 2
#endif
#ifndef HP_MATCH_05
#define HP_MATCH_05 0              // extra byte-match order 5
#endif
#ifndef HP_MATCH_07
#define HP_MATCH_07 0              // extra byte-match order 7
#endif
#ifndef HP_MATCH_09
#define HP_MATCH_09 0              // extra byte-match order 9
#endif
#ifndef HP_MATCH_12
#define HP_MATCH_12 0              // extra byte-match order 12
#endif
#ifndef HP_MATCH_20
#define HP_MATCH_20 0              // extra byte-match order 20
#endif
#ifndef HP_SLOT_NUM2
#define HP_SLOT_NUM2 0             // grow numeric +2
#endif
#ifndef HP_SLOT_WORD3
#define HP_SLOT_WORD3 0            // word/wbi +3 (on top of WORD2)
#endif
#ifndef HP_SLOT_WORD4
#define HP_SLOT_WORD4 0            // word/wbi +4
#endif
#ifndef HP_SLOT_WORD5
#define HP_SLOT_WORD5 0            // word/wbi +5
#endif
#ifndef HP_SLOT_WORD6
#define HP_SLOT_WORD6 0            // word/wbi +6
#endif
#ifndef HP_SLOT_WORD7
#define HP_SLOT_WORD7 0            // word/wbi +7 (needs HP_SLOT_MAX>=29)
#endif
#ifndef HP_SLOT_WORD8
#define HP_SLOT_WORD8 0            // word/wbi +8 (needs HP_SLOT_MAX>=30)
#endif
#ifndef HP_SLOT_WORD9
#define HP_SLOT_WORD9 0            // word/wbi +9 (needs HP_SLOT_MAX>=31)
#endif
#ifndef HP_SLOT_WORD10
#define HP_SLOT_WORD10 0           // word/wbi +10 (needs HP_SLOT_MAX>=32)
#endif
#ifndef HP_SLOT_MAX
#define HP_SLOT_MAX 28             // per-model table-bit cap
#endif
#ifndef HP_SLOT_S3
#define HP_SLOT_S3 0               // grow sent-stream +2
#endif
#ifndef HP_SLOT_S4
#define HP_SLOT_S4 0               // grow sent-stream +3 (on top of S3)
#endif
#ifndef HP_SLOT_WSTR2
#define HP_SLOT_WSTR2 0            // grow wstr +2
#endif
#ifndef HP_SLOT_BRK2
#define HP_SLOT_BRK2 0             // grow bracket +2
#endif
#ifndef HP_SLOT_O34
#define HP_SLOT_O34 0              // grow o3/o4 +1 (not via delta==1 word path)
#endif
#ifndef HP_SLOT_O34B
#define HP_SLOT_O34B 0             // grow o3/o4 one more bit on top of O34
#endif
#ifndef HP_SLOT_O6
#define HP_SLOT_O6 0               // grow o6 +1
#endif
#ifndef HP_SLOT_O6B
#define HP_SLOT_O6B 0              // grow o6 one more bit on top of O6
#endif
#ifndef HP_SLOT_O6C
#define HP_SLOT_O6C 0              // grow o6 a third extra bit
#endif
#ifndef HP_SLOT_O6D
#define HP_SLOT_O6D 0              // grow o6 a fourth extra bit
#endif
#ifndef HP_SLOT_O6E
#define HP_SLOT_O6E 0              // grow o6 a fifth extra bit
#endif
#ifndef HP_SLOT_O6F
#define HP_SLOT_O6F 0              // grow o6 a sixth extra bit (needs HP_SLOT_MAX>=32 at mem 26)
#endif
#ifndef HP_SLOT_O6G
#define HP_SLOT_O6G 0              // grow o6 a seventh extra bit (needs HP_SLOT_MAX>=33 at mem 26)
#endif
#ifndef HP_SLOT_O6H
#define HP_SLOT_O6H 0              // grow o6 an eighth extra bit (needs HP_SLOT_MAX>=34 at mem 26)
#endif
#ifndef HP_SLOT_O6I
#define HP_SLOT_O6I 0              // grow o6 a ninth extra bit (needs HP_SLOT_MAX>=35 at mem 26)
#endif
#ifndef HP_SLOT_O6J
#define HP_SLOT_O6J 0              // grow o6 a tenth extra bit (needs HP_SLOT_MAX>=36 at mem 26)
#endif
#ifndef HP_SLOT_O34C
#define HP_SLOT_O34C 0             // grow o3/o4 a third bit
#endif
#ifndef HP_SLOT_O34D
#define HP_SLOT_O34D 0             // grow o3/o4 a fourth bit
#endif
#ifndef HP_SLOT_O34E
#define HP_SLOT_O34E 0             // grow o3/o4 a fifth bit
#endif
#ifndef HP_SLOT_O34F
#define HP_SLOT_O34F 0             // grow o3/o4 a sixth bit (needs HP_SLOT_MAX>=32 at mem 26)
#endif
#ifndef HP_SLOT_O34G
#define HP_SLOT_O34G 0             // grow o3/o4 a seventh bit
#endif
#ifndef HP_SLOT_O12
#define HP_SLOT_O12 0              // grow o1/o2 +1 (they sit at delta 0 under SLOT_GROW)
#endif
#ifndef HP_SLOT_SP
#define HP_SLOT_SP 0               // grow sparse 1-3 / 2-4 +1
#endif
#ifndef HP_MATCH_GROW
#define HP_MATCH_GROW 0            // match hash tables +1 bit
#endif
#ifndef HP_MATCH_GROW2
#define HP_MATCH_GROW2 0           // match hash tables +2 bits
#endif
#ifndef HP_SENT_CUR
#define HP_SENT_CUR 0              // fold current-sentence hash into sentmem ctx
#endif
#ifndef HP_SENT_GRP_CTX
#define HP_SENT_GRP_CTX 0          // fold wiki sen_group into sent-stream ctx
#endif
#ifndef HP_WSTR_GRP
#define HP_WSTR_GRP 0              // fold wiki sen_group into wstr ctx
#endif
#ifndef HP_WORD_GRP
#define HP_WORD_GRP 0              // fold wiki sen_group into word_ ctx
#endif
#ifndef HP_WBI_GRP
#define HP_WBI_GRP 0               // fold wiki sen_group into wbi_ ctx
#endif
#ifndef HP_SMEM_GRP
#define HP_SMEM_GRP 0              // fold wiki sen_group into sentmem ctx
#endif
