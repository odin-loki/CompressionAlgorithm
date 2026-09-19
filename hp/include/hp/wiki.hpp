#pragma once
//
// hp/wiki.hpp — fx2-cmix / paq8 wiki-XML state machine.
//
// THE AXIS
// --------
// The existing tag model is a generic depth counter. `tag:py` is already the
// most decorrelated expert in the ensemble (mean |corr| 0.621), which means
// the *axis* is right and the *features* are weak. This file gives that
// expert the states fx2-cmix actually uses on enwik:
//
//   WIKITABLE     {| ... |}
//   SQUAREOPEN    [[ ... ]]          wiki links
//   HTLINK        http(s):// ...
//   VERTICALBAR   |  inside table or template
//   CURLY         {{ ... }}          templates / infoboxes
//   COMMENT       <!-- ... -->
//   AMP           &entity;
//   plus linkword = linkword * 2104 + j   (fx2-cmix literal)
//
// All of it is derived from bytes already coded. Nothing is transmitted.
//
// Port of the control flow in fx2-cmix src/models/fxcmv1.cpp (GPL), not a
// copy of its tables. States are a 4-bit enum so they fit a mixer gate.

#include <cstdint>

#include "hp/features.hpp"
#include "hp/models.hpp"

namespace hp {

enum WikiState : int {
    kWkText = 0,
    kWkTag,
    kWkTagEnd,
    kWkAmp,
    kWkComment,
    kWkSquareOpen,
    kWkCurly,
    kWkWikiTable,
    kWkVerticalBar,
    kWkHtLink,
    kWkQuote,
    kWkEntity,
    kWkFirstUpper,   // fx2 FIRSTUPPER — line/word starts with A-Z
    kWkHeader,       // fx2 WIKIHEADER — line starts with '>'
    kWkN = 16
};

class WikiMachine {
 public:
    void push(int byte) {
        const int c = byte;
        prev2_ = prev1_;
        prev1_ = last_;
        last_ = c;
#if HP_STATETRANS_MOD
        prev_state_ = state_;
#endif
#if HP_WIKISTACK_MOD
        stack_feed(c);
#endif
#if HP_RUNLEN_MOD
        if (c == prev1_ && prev1_ != 0) {
            if (runlen_ < 15) ++runlen_;
        } else {
            runlen_ = 1;
        }
#endif
#if HP_WPOS_MOD
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            if (wpos_ < 31) ++wpos_;
        } else {
            wpos_ = 0;
        }
#endif
#if HP_BLANK_MOD
        if (c == '\n') {
            if (blank_n_ < 3) ++blank_n_;
        } else if (c != ' ' && c != '\t') {
            blank_n_ = 0;
        }
#endif
#if HP_SPRUN_MOD
        if (c == ' ') {
            if (sprun_ < 31) ++sprun_;
        } else {
            sprun_ = 0;
        }
#endif
#if HP_LINELEN_MOD
        if (c == '\n') linelen_ = 0;
        else if (linelen_ < 255) ++linelen_;
#endif
#if HP_TAGDIST_MOD
        if (c == '<') tagdist_ = 0;
        else if (tagdist_ < 255) ++tagdist_;
#endif
#if HP_MARKDIST_MOD
        if (c == '[' || c == ']' || c == '{' || c == '}' || c == '|' ||
            c == '=' || c == '*' || c == '#' || c == '<' || c == '>')
            markdist_ = 0;
        else if (markdist_ < 255) ++markdist_;
#endif
#if HP_UPPERGAP_MOD
        if (c >= 'A' && c <= 'Z') uppergap_ = 0;
        else if (uppergap_ < 255) ++uppergap_;
#endif
#if HP_DIGITGAP_MOD
        if (c >= '0' && c <= '9') digitgap_ = 0;
        else if (digitgap_ < 255) ++digitgap_;
#endif
#if HP_DOTGAP_MOD
        if (c == '.') dotgap_ = 0;
        else if (dotgap_ < 255) ++dotgap_;
#endif
#if HP_COMMAGAP_MOD
        if (c == ',') commagap_ = 0;
        else if (commagap_ < 255) ++commagap_;
#endif
#if HP_WORDLEN_MOD
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            if (wl_run_ < 31) ++wl_run_;
        } else {
            if (wl_run_) wordlen_ = wl_run_;
            wl_run_ = 0;
        }
#endif
#if HP_SENTLEN_MOD
        if (c == '.' || c == '?' || c == '!') sentlen_ = 0;
        else if (sentlen_ < 255) ++sentlen_;
#endif
#if HP_LOWERGAP_MOD
        if (c >= 'a' && c <= 'z') lowergap_ = 0;
        else if (lowergap_ < 255) ++lowergap_;
#endif
#if HP_DIGITPOS_MOD
        if (c >= '0' && c <= '9') {
            if (digitpos_ < 15) ++digitpos_;
        } else {
            digitpos_ = 0;
        }
#endif
#if HP_SLASHGAP_MOD
        if (c == '/') slashgap_ = 0;
        else if (slashgap_ < 255) ++slashgap_;
#endif
#if HP_DIGLEN_MOD
        if (c >= '0' && c <= '9') {
            if (dl_run_ < 15) ++dl_run_;
        } else {
            if (dl_run_) diglen_ = dl_run_;
            dl_run_ = 0;
        }
#endif
#if HP_PREVLINE_MOD
        if (c == '\n') {
            if (ll_run_) prevline_ = ll_run_;
            ll_run_ = 0;
        } else if (ll_run_ < 255) {
            ++ll_run_;
        }
#endif
#if HP_PREVSENT_MOD
        if (c == '.' || c == '?' || c == '!') {
            if (ss_run_) prevsent_ = ss_run_ > 255 ? 255 : ss_run_;
            ss_run_ = 0;
        } else if (ss_run_ < 255) {
            ++ss_run_;
        }
#endif
#if HP_LINKLEN_MOD
        if (state_ == kWkSquareOpen) {
            if (lk_run_ < 255) ++lk_run_;
        } else if (lk_run_) {
            linklen_ = lk_run_;
            lk_run_ = 0;
        }
#endif
#if HP_TPLLEN_MOD
        if (state_ == kWkCurly) {
            if (tp_run_ < 255) ++tp_run_;
        } else if (tp_run_) {
            tpllen_ = tp_run_;
            tp_run_ = 0;
        }
#endif
#if HP_PARALEN_MOD
        if (c == '\n' && prev1_ == '\n') {
            if (pr_run_) paralen_ = pr_run_ > 255 ? 255 : pr_run_;
            pr_run_ = 0;
        } else if (pr_run_ < 255) {
            ++pr_run_;
        }
#endif
#if HP_ALNUMLEN_MOD
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z')) {
            if (al_run_ < 31) ++al_run_;
        } else {
            if (al_run_) alnumlen_ = al_run_;
            al_run_ = 0;
        }
#endif
#if HP_SPLEN_MOD
        if (c == ' ') {
            if (sp_run_ < 31) ++sp_run_;
        } else {
            if (sp_run_) splen_ = sp_run_;
            sp_run_ = 0;
        }
#endif
#if HP_CASEFLIP_MOD
        if (prev1_ >= 'a' && prev1_ <= 'z' && c >= 'A' && c <= 'Z')
            caseflip_ = 0;
        else if (caseflip_ < 255)
            ++caseflip_;
#endif
#if HP_LEAD_MOD
        if (first_of_line_ && c == '=') lead_ = 0;
#endif
#if HP_MAINART_MOD
        if (first_of_line_ && c == '=') ma_seen_head_ = 1;
#endif
#if HP_HEADIDX_MOD
        if (first_of_line_ && c == '=') {
            if (!hi_on_ && headidx_ < 15) ++headidx_;
            hi_on_ = 1;
        } else if (c == '\n') {
            hi_on_ = 0;
        }
#endif
#if HP_LINKTRAIL_MOD
        if (lt_on_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z')
                linktrail_ = 1;
            else {
                linktrail_ = 0;
                lt_on_ = 0;
            }
        }
#endif
#if HP_CELLKIND_MOD
        if (in_table_) {
            if (c == '\n') {
                ck_sol_ = 1;
                ck_bar_ = 0;
            } else if (ck_sol_) {
                if (c != ' ' && c != '\t') {
                    ck_sol_ = 0;
                    if (c == '!')
                        cellkind_ = 2;
                    else if (c == '|')
                        ck_bar_ = 1;
                    else
                        ck_bar_ = 0;
                }
            } else if (ck_bar_) {
                ck_bar_ = 0;
                if (c == '+')
                    cellkind_ = 1;
                else if (c == '-')
                    cellkind_ = 4;
                else
                    cellkind_ = 3;
            }
        } else {
            cellkind_ = 0;
            ck_sol_ = 0;
            ck_bar_ = 0;
        }
#endif
#if HP_TBLCOL_MOD
        if (in_table_) {
            if (c == '\n' || (prev1_ == '|' && c == '-'))
                tblcol_ = 0;
            else if (c == '|' || c == '!') {
                if (tblcol_ < 15) ++tblcol_;
            }
        } else {
            tblcol_ = 0;
        }
#endif
#if HP_INFOVAL_MOD
        if (prev1_ == '{' && c == '{') {
            if (iv_tpl_ < 7) ++iv_tpl_;
            infoval_ = 0;
            iv_set_ = 0;
        } else if (prev1_ == '}' && c == '}') {
            if (iv_tpl_ > 0) --iv_tpl_;
            if (iv_tpl_ == 0) {
                infoval_ = 0;
                iv_set_ = 0;
            }
        } else if (iv_tpl_ > 0) {
            if (c == '|' || c == '=') {
                infoval_ = 0;
                iv_set_ = 0;
            } else if (!iv_set_ && c != ' ' && c != '\n' && c != '\t') {
                iv_set_ = 1;
                if (c >= '0' && c <= '9')
                    infoval_ = 1;
                else if (c == '[')
                    infoval_ = 2;
                else if (c == 'h' || c == 'H')
                    infoval_ = 3;
                else if ((c | 32) >= 'a' && (c | 32) <= 'z')
                    infoval_ = 4;
                else
                    infoval_ = 5;
            }
        }
#endif
#if HP_INFOBOX_MOD
        if (prev1_ == '{' && c == '{') {
            if (ib_depth_ < 7) ++ib_depth_;
            ib_n_ = 0;
            ib_col_ = 1;
        } else if (prev1_ == '}' && c == '}') {
            if (ib_depth_ > 0) --ib_depth_;
            if (ib_depth_ == 0) infobox_ = 0;
            ib_col_ = 0;
        } else if (ib_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && ib_n_ < 8) {
                ib_buf_[ib_n_++] = static_cast<char>(lc);
            } else {
                ib_col_ = 0;
                if (ib_n_ >= 7 && ib_buf_[0] == 'i' && ib_buf_[1] == 'n' &&
                    ib_buf_[2] == 'f' && ib_buf_[3] == 'o' &&
                    ib_buf_[4] == 'b' && ib_buf_[5] == 'o' &&
                    ib_buf_[6] == 'x')
                    infobox_ = 1;
            }
        }
#endif
#if HP_CONVERT_MOD || HP_CN_MOD || HP_REFLIST_MOD
        if (prev1_ == '{' && c == '{') {
            if (h39t_depth_ < 7) ++h39t_depth_;
            h39t_n_ = 0;
            h39t_col_ = 1;
        } else if (prev1_ == '}' && c == '}') {
            if (h39t_depth_ > 0) --h39t_depth_;
#if HP_CONVERT_MOD
            if (convert_ && h39t_depth_ < convert_d_) convert_ = 0;
#endif
#if HP_CN_MOD
            if (cn_ && h39t_depth_ < cn_d_) cn_ = 0;
#endif
#if HP_REFLIST_MOD
            if (reflist_ && rl_from_tpl_ && h39t_depth_ < rl_d_) {
                reflist_ = 0;
                rl_from_tpl_ = 0;
            }
#endif
            h39t_col_ = 0;
        } else if (h39t_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && h39t_n_ < 16) {
                h39t_buf_[h39t_n_++] = static_cast<char>(lc);
            } else if (c != ' ' && c != '_') {
                h39t_col_ = 0;
                auto teq = [&](const char* w, int n) {
                    if (h39t_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h39t_buf_[i] != w[i]) return 0;
                    return 1;
                };
#if HP_CONVERT_MOD
                if (teq("convert", 7)) {
                    convert_ = 1;
                    convert_d_ = h39t_depth_;
                }
#endif
#if HP_CN_MOD
                if (teq("cn", 2)) {
                    cn_ = 1;
                    cn_d_ = h39t_depth_;
                } else if (teq("citationneeded", 15)) {
                    cn_ = 2;
                    cn_d_ = h39t_depth_;
                } else if (teq("fact", 4)) {
                    cn_ = 3;
                    cn_d_ = h39t_depth_;
                } else if (teq("clarify", 7)) {
                    cn_ = 4;
                    cn_d_ = h39t_depth_;
                }
#endif
#if HP_REFLIST_MOD
                if (teq("reflist", 7)) {
                    reflist_ = 1;
                    rl_from_tpl_ = 1;
                    rl_d_ = h39t_depth_;
                }
#endif
            }
        }
#endif
#if HP_NOTES_MOD || HP_LANGTPL_MOD || HP_FRAC_MOD || HP_LISTEN_MOD || \
    HP_BIRTH_MOD || HP_HLIST_MOD || HP_MAINART_MOD
        if (prev1_ == '{' && c == '{') {
            if (h40t_depth_ < 7) ++h40t_depth_;
            h40t_n_ = 0;
            h40t_col_ = 1;
        } else if (prev1_ == '}' && c == '}') {
            if (h40t_depth_ > 0) --h40t_depth_;
#if HP_NOTES_MOD
            if (notes_ && h40t_depth_ < notes_d_) notes_ = 0;
#endif
#if HP_LANGTPL_MOD
            if (langtpl_ && h40t_depth_ < langtpl_d_) langtpl_ = 0;
#endif
#if HP_FRAC_MOD
            if (frac_ && h40t_depth_ < frac_d_) frac_ = 0;
#endif
#if HP_LISTEN_MOD
            if (listen_ && listen_ < 3 && h40t_depth_ < listen_d_) listen_ = 0;
#endif
#if HP_BIRTH_MOD
            if (birth_ && h40t_depth_ < birth_d_) birth_ = 0;
#endif
#if HP_HLIST_MOD
            if (hlist_ && h40t_depth_ < hlist_d_) hlist_ = 0;
#endif
#if HP_MAINART_MOD
            if (mainart_ && h40t_depth_ < mainart_d_) mainart_ = 0;
#endif
            h40t_col_ = 0;
        } else if (h40t_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && h40t_n_ < 16) {
                h40t_buf_[h40t_n_++] = static_cast<char>(lc);
            } else if (c != ' ' && c != '_' && c != '-') {
                h40t_col_ = 0;
                auto teq = [&](const char* w, int n) {
                    if (h40t_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h40t_buf_[i] != w[i]) return 0;
                    return 1;
                };
#if HP_LANGTPL_MOD || HP_BIRTH_MOD
                auto tpre = [&](const char* w, int n) {
                    if (h40t_n_ < n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h40t_buf_[i] != w[i]) return 0;
                    return 1;
                };
#endif
#if HP_NOTES_MOD
                if (teq("notelist", 8)) {
                    notes_ = 1;
                    notes_d_ = h40t_depth_;
                } else if (teq("notes", 5)) {
                    notes_ = 2;
                    notes_d_ = h40t_depth_;
                } else if (teq("notefoot", 8)) {
                    notes_ = 3;
                    notes_d_ = h40t_depth_;
                }
#endif
#if HP_LANGTPL_MOD
                if (teq("lang", 4)) {
                    langtpl_ = 1;
                    langtpl_d_ = h40t_depth_;
                } else if (h40t_n_ >= 6 && h40t_n_ <= 7 && tpre("lang", 4)) {
                    int rest_ok = 1;
                    for (int i = 4; i < h40t_n_; ++i) {
                        const int ch = h40t_buf_[i];
                        if (ch < 'a' || ch > 'z') rest_ok = 0;
                    }
                    if (rest_ok) {
                        langtpl_ = 2;
                        langtpl_d_ = h40t_depth_;
                    }
                }
#endif
#if HP_FRAC_MOD
                if (teq("frac", 4)) {
                    frac_ = 1;
                    frac_d_ = h40t_depth_;
                } else if (teq("sfrac", 5)) {
                    frac_ = 2;
                    frac_d_ = h40t_depth_;
                }
#endif
#if HP_LISTEN_MOD
                if (teq("listen", 6)) {
                    listen_ = 1;
                    listen_d_ = h40t_depth_;
                } else if (teq("audio", 5)) {
                    listen_ = 2;
                    listen_d_ = h40t_depth_;
                }
#endif
#if HP_BIRTH_MOD
                if (tpre("birthdate", 9)) {
                    birth_ = 1;
                    birth_d_ = h40t_depth_;
                } else if (tpre("deathdate", 9)) {
                    birth_ = 2;
                    birth_d_ = h40t_depth_;
                }
#endif
#if HP_HLIST_MOD
                if (teq("hlist", 5)) {
                    hlist_ = 1;
                    hlist_d_ = h40t_depth_;
                } else if (teq("plainlist", 9)) {
                    hlist_ = 2;
                    hlist_d_ = h40t_depth_;
                } else if (teq("unbulletedlist", 14)) {
                    hlist_ = 3;
                    hlist_d_ = h40t_depth_;
                }
#endif
#if HP_MAINART_MOD
                if (ma_seen_head_) {
                    if (teq("main", 4)) {
                        mainart_ = 1;
                        mainart_d_ = h40t_depth_;
                    } else if (teq("seealso", 7)) {
                        mainart_ = 2;
                        mainart_d_ = h40t_depth_;
                    } else if (teq("further", 7)) {
                        mainart_ = 3;
                        mainart_d_ = h40t_depth_;
                    }
                }
#endif
            }
        }
#endif
#if HP_SFN_MOD || HP_GEOTEMP_MOD || HP_EPIGRAPH_MOD || HP_TRACKLIST_MOD || \
    HP_SUCCESSION_MOD || HP_COLSTART_MOD || HP_REFBEGIN_MOD
        if (prev1_ == '{' && c == '{') {
            if (h41t_depth_ < 7) ++h41t_depth_;
            h41t_n_ = 0;
            h41t_col_ = 1;
        } else if (prev1_ == '}' && c == '}') {
            if (h41t_depth_ > 0) --h41t_depth_;
#if HP_SFN_MOD
            if (sfn_ && h41t_depth_ < sfn_d_) sfn_ = 0;
#endif
#if HP_GEOTEMP_MOD
            if (geotemp_ && h41t_depth_ < geotemp_d_) geotemp_ = 0;
#endif
#if HP_EPIGRAPH_MOD
            if (epigraph_ && h41t_depth_ < epigraph_d_) epigraph_ = 0;
#endif
#if HP_TRACKLIST_MOD
            if (tracklist_ && h41t_depth_ < tracklist_d_) tracklist_ = 0;
#endif
#if HP_SUCCESSION_MOD
            if (succession_ && h41t_depth_ < succession_d_) succession_ = 0;
#endif
#if HP_COLSTART_MOD
            if (colstart_ && h41t_depth_ < colstart_d_) colstart_ = 0;
#endif
            h41t_col_ = 0;
        } else if (h41t_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && h41t_n_ < 16) {
                h41t_buf_[h41t_n_++] = static_cast<char>(lc);
            } else if (c != ' ' && c != '_' && c != '-') {
                h41t_col_ = 0;
                auto teq = [&](const char* w, int n) {
                    if (h41t_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h41t_buf_[i] != w[i]) return 0;
                    return 1;
                };
#if HP_SFN_MOD
                if (teq("sfn", 3)) {
                    sfn_ = 1;
                    sfn_d_ = h41t_depth_;
                } else if (teq("harvnb", 6)) {
                    sfn_ = 2;
                    sfn_d_ = h41t_depth_;
                } else if (teq("harv", 4)) {
                    sfn_ = 3;
                    sfn_d_ = h41t_depth_;
                }
#endif
#if HP_GEOTEMP_MOD
                if (teq("coord", 5)) {
                    geotemp_ = 1;
                    geotemp_d_ = h41t_depth_;
                }
#endif
#if HP_EPIGRAPH_MOD
                if (teq("quotebox", 8)) {
                    epigraph_ = 1;
                    epigraph_d_ = h41t_depth_;
                } else if (teq("cquote", 6)) {
                    epigraph_ = 2;
                    epigraph_d_ = h41t_depth_;
                } else if (teq("blockquote", 10)) {
                    epigraph_ = 3;
                    epigraph_d_ = h41t_depth_;
                }
#endif
#if HP_TRACKLIST_MOD
                if (teq("tracklist", 9)) {
                    tracklist_ = 1;
                    tracklist_d_ = h41t_depth_;
                } else if (teq("tracklisting", 12)) {
                    tracklist_ = 2;
                    tracklist_d_ = h41t_depth_;
                }
#endif
#if HP_SUCCESSION_MOD
                if (teq("sstart", 6)) {
                    succession_ = 1;
                    succession_d_ = h41t_depth_;
                } else if (teq("successionbox", 14)) {
                    succession_ = 2;
                    succession_d_ = h41t_depth_;
                } else if (teq("sbef", 4)) {
                    succession_ = 3;
                    succession_d_ = h41t_depth_;
                } else if (teq("sttl", 4)) {
                    succession_ = 4;
                    succession_d_ = h41t_depth_;
                }
#endif
#if HP_COLSTART_MOD
                if (teq("colbegin", 8)) {
                    colstart_ = 1;
                    colstart_d_ = h41t_depth_;
                } else if (teq("divcol", 6)) {
                    colstart_ = 2;
                    colstart_d_ = h41t_depth_;
                } else if (teq("columnslist", 11)) {
                    colstart_ = 3;
                    colstart_d_ = h41t_depth_;
                }
#endif
#if HP_REFBEGIN_MOD
                if (teq("refbegin", 8))
                    refbegin_ = 1;
                else if (teq("refend", 6))
                    refbegin_ = 0;
#endif
            }
        }
#endif
#if HP_TOC_MOD
        if (c == '_' && prev1_ == '_') {
            if (toc_col_) {
                auto teq = [&](const char* w, int n) {
                    if (toc_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (toc_buf_[i] != w[i]) return 0;
                    return 1;
                };
                if (teq("toc", 3)) toc_ = 1;
                else if (teq("notoc", 5)) toc_ = 2;
                else if (teq("forcetoc", 8)) toc_ = 3;
                toc_col_ = 0;
                toc_n_ = 0;
            } else {
                toc_col_ = 1;
                toc_n_ = 0;
            }
        } else if (toc_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && toc_n_ < 12)
                toc_buf_[toc_n_++] = static_cast<char>(lc);
            else if (c != '_')
                toc_col_ = 0;
        }
#endif
#if HP_SHORTDESC_MOD || HP_SEEALSO_MOD || HP_PORTAL_MOD || HP_AUTHCTL_MOD || \
    HP_USEDATE_MOD || HP_IPA_MOD || HP_GOODART_MOD || HP_CAPTION_MOD
        if (prev1_ == '{' && c == '{') {
            if (h42t_depth_ < 7) ++h42t_depth_;
            h42t_n_ = 0;
            h42t_col_ = 1;
#if HP_CAPTION_MOD
            cap_key_on_ = 0;
            cap_key_n_ = 0;
#endif
        } else if (prev1_ == '}' && c == '}') {
            if (h42t_depth_ > 0) --h42t_depth_;
#if HP_SHORTDESC_MOD
            if (shortdesc_ && h42t_depth_ < shortdesc_d_) shortdesc_ = 0;
#endif
#if HP_SEEALSO_MOD
            if (seealso_ && h42t_depth_ < seealso_d_) seealso_ = 0;
#endif
#if HP_PORTAL_MOD
            if (portal_ && h42t_depth_ < portal_d_) portal_ = 0;
#endif
#if HP_AUTHCTL_MOD
            if (authctl_ && h42t_depth_ < authctl_d_) authctl_ = 0;
#endif
#if HP_IPA_MOD
            if (ipa_ && h42t_depth_ < ipa_d_) ipa_ = 0;
#endif
#if HP_CAPTION_MOD
            if (caption_ && h42t_depth_ < caption_d_) caption_ = 0;
            cap_key_on_ = 0;
            cap_key_n_ = 0;
            if (h42t_depth_ == 0) cap_sq_ = 0;
#endif
            h42t_col_ = 0;
        } else if (h42t_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && h42t_n_ < 20) {
                h42t_buf_[h42t_n_++] = static_cast<char>(lc);
            } else if (c != ' ' && c != '_' && c != '-') {
                h42t_col_ = 0;
                auto teq = [&](const char* w, int n) {
                    if (h42t_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h42t_buf_[i] != w[i]) return 0;
                    return 1;
                };
                auto tpre = [&](const char* w, int n) {
                    if (h42t_n_ < n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h42t_buf_[i] != w[i]) return 0;
                    return 1;
                };
#if HP_SHORTDESC_MOD
                if (teq("shortdescription", 16)) {
                    shortdesc_ = 1;
                    shortdesc_d_ = h42t_depth_;
                } else if (teq("shortdesc", 9)) {
                    shortdesc_ = 2;
                    shortdesc_d_ = h42t_depth_;
                }
#endif
#if HP_SEEALSO_MOD
                if (teq("seealso", 7)) {
                    seealso_ = 1;
                    seealso_d_ = h42t_depth_;
                } else if (teq("further", 7)) {
                    seealso_ = 2;
                    seealso_d_ = h42t_depth_;
                }
#endif
#if HP_PORTAL_MOD
                if (teq("portalbar", 9)) {
                    portal_ = 2;
                    portal_d_ = h42t_depth_;
                } else if (teq("portal", 6)) {
                    portal_ = 1;
                    portal_d_ = h42t_depth_;
                }
#endif
#if HP_AUTHCTL_MOD
                if (teq("authoritycontrol", 16)) {
                    authctl_ = 1;
                    authctl_d_ = h42t_depth_;
                }
#endif
#if HP_USEDATE_MOD
                if (tpre("usedmy", 6))
                    usedate_ = 1;
                else if (tpre("usemdy", 6))
                    usedate_ = 2;
                else if (tpre("useymd", 6))
                    usedate_ = 3;
#endif
#if HP_IPA_MOD
                if (teq("ipa", 3)) {
                    ipa_ = 1;
                    ipa_d_ = h42t_depth_;
                } else if (teq("ipacen", 6)) {
                    ipa_ = 2;
                    ipa_d_ = h42t_depth_;
                } else if (teq("pronen", 6)) {
                    ipa_ = 3;
                    ipa_d_ = h42t_depth_;
                }
#endif
#if HP_GOODART_MOD
                if (teq("goodarticle", 11))
                    goodart_ = 1;
                else if (teq("ga", 2))
                    goodart_ = 2;
                else if (teq("featuredarticle", 15))
                    goodart_ = 3;
#endif
#if HP_CAPTION_MOD
                if (c == '|') {
                    if (caption_ && h42t_depth_ == caption_d_) caption_ = 0;
                    cap_key_on_ = 1;
                    cap_key_n_ = 0;
                }
#endif
            }
        }
#if HP_CAPTION_MOD
        else if (h42t_depth_ > 0) {
            if (prev1_ == '[' && c == '[') {
                if (cap_sq_ < 7) ++cap_sq_;
            } else if (prev1_ == ']' && c == ']') {
                if (cap_sq_ > 0) --cap_sq_;
            }
            if (cap_sq_ == 0 && c == '|') {
                if (caption_ && h42t_depth_ == caption_d_) caption_ = 0;
                cap_key_on_ = 1;
                cap_key_n_ = 0;
            } else if (cap_key_on_) {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && cap_key_n_ < 16) {
                    cap_key_[cap_key_n_++] = static_cast<char>(lc);
                } else if (c == '=') {
                    cap_key_on_ = 0;
                    auto keq = [&](const char* w, int n) {
                        if (cap_key_n_ != n) return 0;
                        for (int i = 0; i < n; ++i)
                            if (cap_key_[i] != w[i]) return 0;
                        return 1;
                    };
                    if (keq("caption", 7)) {
                        caption_ = 1;
                        caption_d_ = h42t_depth_;
                    } else if (keq("imagecaption", 13)) {
                        caption_ = 2;
                        caption_d_ = h42t_depth_;
                    }
                } else if (c != ' ' && c != '_' && c != '-') {
                    cap_key_on_ = 0;
                }
            }
        }
#endif
#endif
#if HP_NAVBOX_MOD || HP_EFOOT_MOD || HP_RSHORT_MOD || HP_ASOF_MOD || \
    HP_CLARIFY_MOD || HP_CURRENCY_MOD || HP_DISPLAYTITLE_MOD || HP_NOWRAP_MOD || \
    HP_STUB_MOD || HP_PERSONDATA_MOD || HP_FLAG_MOD || HP_QUOTEBOX_MOD || \
    HP_CLEAR_MOD || HP_IMDB_MOD || HP_RP_MOD || HP_FN_MOD || \
    HP_TAXOBOX_MOD || HP_NIHONGO_MOD || HP_DEADLINK_MOD || HP_WAYBACK_MOD || \
    HP_UNREF_MOD || HP_CLEANUP_MOD || HP_NPOV_MOD || HP_RFROM_MOD || \
    HP_DOI_MOD || HP_PMID_MOD || HP_MEDAL_MOD || \
    HP_FURTHER_MOD || HP_DEATH_MOD || HP_HARV_MOD
        if (prev1_ == '{' && c == '{') {
            if (h43t_depth_ < 7) ++h43t_depth_;
            h43t_n_ = 0;
            h43t_col_ = 1;
#if HP_CLEAR_MOD
            h43t_dash_ = 0;
#endif
        } else if (prev1_ == '}' && c == '}') {
            if (h43t_depth_ > 0) --h43t_depth_;
#if HP_NAVBOX_MOD
            if (navbox_ && h43t_depth_ < navbox_d_) navbox_ = 0;
#endif
#if HP_EFOOT_MOD
            if (efoot_ && h43t_depth_ < efoot_d_) efoot_ = 0;
#endif
#if HP_RSHORT_MOD
            if (rshort_ && h43t_depth_ < rshort_d_) rshort_ = 0;
#endif
#if HP_ASOF_MOD
            if (asof_ && h43t_depth_ < asof_d_) asof_ = 0;
#endif
#if HP_CLARIFY_MOD
            if (clarify_ && h43t_depth_ < clarify_d_) clarify_ = 0;
#endif
#if HP_CURRENCY_MOD
            if (currency_ && h43t_depth_ < currency_d_) currency_ = 0;
#endif
#if HP_NOWRAP_MOD
            if (nowrap_ && h43t_depth_ < nowrap_d_) nowrap_ = 0;
#endif
#if HP_PERSONDATA_MOD
            if (persondata_ && h43t_depth_ < persondata_d_) persondata_ = 0;
#endif
#if HP_FLAG_MOD
            if (flag_ && h43t_depth_ < flag_d_) flag_ = 0;
#endif
#if HP_QUOTEBOX_MOD
            if (quotebox_ && h43t_depth_ < quotebox_d_) quotebox_ = 0;
#endif
#if HP_IMDB_MOD
            if (imdb_ && h43t_depth_ < imdb_d_) imdb_ = 0;
#endif
#if HP_RP_MOD
            if (rp_ && h43t_depth_ < rp_d_) rp_ = 0;
#endif
#if HP_FN_MOD
            if (fn_ && h43t_depth_ < fn_d_) fn_ = 0;
#endif
#if HP_TAXOBOX_MOD
            if (taxobox_ && h43t_depth_ < taxobox_d_) taxobox_ = 0;
#endif
#if HP_NIHONGO_MOD
            if (nihongo_ && h43t_depth_ < nihongo_d_) nihongo_ = 0;
#endif
#if HP_DEADLINK_MOD
            if (deadlink_ && h43t_depth_ < deadlink_d_) deadlink_ = 0;
#endif
#if HP_WAYBACK_MOD
            if (wayback_ && h43t_depth_ < wayback_d_) wayback_ = 0;
#endif
#if HP_UNREF_MOD
            if (unref_ && h43t_depth_ < unref_d_) unref_ = 0;
#endif
#if HP_CLEANUP_MOD
            if (cleanup_ && h43t_depth_ < cleanup_d_) cleanup_ = 0;
#endif
#if HP_NPOV_MOD
            if (npov_ && h43t_depth_ < npov_d_) npov_ = 0;
#endif
#if HP_RFROM_MOD
            if (rfrom_ && h43t_depth_ < rfrom_d_) rfrom_ = 0;
#endif
#if HP_DOI_MOD
            if (doi_ && doi_d_ && h43t_depth_ < doi_d_) doi_ = 0;
#endif
#if HP_PMID_MOD
            if (pmid_ && pmid_d_ && h43t_depth_ < pmid_d_) pmid_ = 0;
#endif
#if HP_MEDAL_MOD
            if (medal_ && h43t_depth_ < medal_d_) medal_ = 0;
#endif
#if HP_FURTHER_MOD
            if (further_ && h43t_depth_ < further_d_) further_ = 0;
#endif
#if HP_DEATH_MOD
            if (death_ && h43t_depth_ < death_d_) death_ = 0;
#endif
#if HP_HARV_MOD
            if (harv_ && h43t_depth_ < harv_d_) harv_ = 0;
#endif
            h43t_col_ = 0;
        } else if (h43t_col_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && h43t_n_ < 20) {
                h43t_buf_[h43t_n_++] = static_cast<char>(lc);
#if HP_CLEAR_MOD
            } else if (c == '-' && h43t_n_ == 0) {
                h43t_dash_ = 1;
#endif
            } else if (c != ' ' && c != '_' && c != '-') {
                h43t_col_ = 0;
                auto teq = [&](const char* w, int n) {
                    if (h43t_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h43t_buf_[i] != w[i]) return 0;
                    return 1;
                };
                auto tpre = [&](const char* w, int n) {
                    if (h43t_n_ < n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (h43t_buf_[i] != w[i]) return 0;
                    return 1;
                };
#if HP_NAVBOX_MOD
                if (tpre("navbox", 6)) {
                    navbox_ = 1;
                    navbox_d_ = h43t_depth_;
                } else if (tpre("sidebar", 7)) {
                    navbox_ = 2;
                    navbox_d_ = h43t_depth_;
                }
#endif
#if HP_EFOOT_MOD
                if (tpre("efn", 3)) {
                    efoot_ = 1;
                    efoot_d_ = h43t_depth_;
                } else if (teq("notelist", 8)) {
                    efoot_ = 2;
                    efoot_d_ = h43t_depth_;
                } else if (teq("notefoot", 8)) {
                    efoot_ = 3;
                    efoot_d_ = h43t_depth_;
                }
#endif
#if HP_RSHORT_MOD
                if (teq("r", 1) && (c == '|' || c == '}')) {
                    rshort_ = 1;
                    rshort_d_ = h43t_depth_;
                }
#endif
#if HP_ASOF_MOD
                if (teq("asof", 4)) {
                    asof_ = 1;
                    asof_d_ = h43t_depth_;
                }
#endif
#if HP_CLARIFY_MOD
                if (teq("clarify", 7)) {
                    clarify_ = 1;
                    clarify_d_ = h43t_depth_;
                } else if (teq("who", 3)) {
                    clarify_ = 2;
                    clarify_d_ = h43t_depth_;
                } else if (teq("which", 5)) {
                    clarify_ = 3;
                    clarify_d_ = h43t_depth_;
                } else if (teq("when", 4)) {
                    clarify_ = 4;
                    clarify_d_ = h43t_depth_;
                }
#endif
#if HP_CURRENCY_MOD
                if (teq("usd", 3)) {
                    currency_ = 1;
                    currency_d_ = h43t_depth_;
                } else if (teq("gbp", 3)) {
                    currency_ = 2;
                    currency_d_ = h43t_depth_;
                } else if (teq("eur", 3)) {
                    currency_ = 3;
                    currency_d_ = h43t_depth_;
                } else if (tpre("currency", 8)) {
                    currency_ = 4;
                    currency_d_ = h43t_depth_;
                }
#endif
#if HP_DISPLAYTITLE_MOD
                if (teq("displaytitle", 12))
                    displaytitle_ = 1;
                else if (tpre("italictitle", 11))
                    displaytitle_ = 2;
                else if (tpre("lowercase", 9))
                    displaytitle_ = 3;
#endif
#if HP_NOWRAP_MOD
                if (teq("nowrap", 6)) {
                    nowrap_ = 1;
                    nowrap_d_ = h43t_depth_;
                } else if (teq("nobold", 6)) {
                    nowrap_ = 2;
                    nowrap_d_ = h43t_depth_;
                } else if (teq("noitalic", 8)) {
                    nowrap_ = 3;
                    nowrap_d_ = h43t_depth_;
                }
#endif
#if HP_STUB_MOD
                if (teq("stub", 4)) {
                    stub_ = 1;
                } else if (h43t_n_ >= 4) {
                    int ok = 1;
                    const char* sw = "stub";
                    for (int i = 0; i < 4; ++i)
                        if (h43t_buf_[h43t_n_ - 4 + i] != sw[i]) ok = 0;
                    if (ok) stub_ = 2;
                }
#endif
#if HP_PERSONDATA_MOD
                if (tpre("persondata", 10)) {
                    persondata_ = 1;
                    persondata_d_ = h43t_depth_;
                }
#endif
#if HP_FLAG_MOD
                if (tpre("flagcountry", 11)) {
                    flag_ = 4;
                    flag_d_ = h43t_depth_;
                } else if (tpre("flagicon", 8)) {
                    flag_ = 2;
                    flag_d_ = h43t_depth_;
                } else if (tpre("flagu", 5)) {
                    flag_ = 3;
                    flag_d_ = h43t_depth_;
                } else if (tpre("flag", 4)) {
                    flag_ = 1;
                    flag_d_ = h43t_depth_;
                }
#endif
#if HP_QUOTEBOX_MOD
                if (tpre("quotebox", 8)) {
                    quotebox_ = 4;
                    quotebox_d_ = h43t_depth_;
                } else if (tpre("quotation", 9)) {
                    quotebox_ = 3;
                    quotebox_d_ = h43t_depth_;
                } else if (teq("cquote", 6)) {
                    quotebox_ = 2;
                    quotebox_d_ = h43t_depth_;
                } else if (tpre("quote", 5)) {
                    quotebox_ = 1;
                    quotebox_d_ = h43t_depth_;
                }
#endif
#if HP_CLEAR_MOD
                if ((h43t_dash_ && h43t_n_ == 0) ||
                    (h43t_n_ == 1 && h43t_buf_[0] == '-')) {
                    clear_ = 3;
                } else if (tpre("clear", 5)) {
                    clear_ = 1;
                } else if (teq("clr", 3)) {
                    clear_ = 2;
                }
#endif
#if HP_IMDB_MOD
                if (tpre("imdbtitle", 9)) {
                    imdb_ = 2;
                    imdb_d_ = h43t_depth_;
                } else if (tpre("imdbname", 8)) {
                    imdb_ = 3;
                    imdb_d_ = h43t_depth_;
                } else if (tpre("imdb", 4)) {
                    imdb_ = 1;
                    imdb_d_ = h43t_depth_;
                }
#endif
#if HP_RP_MOD
                if (teq("rp", 2) && (c == '|' || c == '}')) {
                    rp_ = 1;
                    rp_d_ = h43t_depth_;
                }
#endif
#if HP_FN_MOD
                if (teq("fnb", 3)) {
                    fn_ = 2;
                    fn_d_ = h43t_depth_;
                } else if (teq("fn", 2)) {
                    fn_ = 1;
                    fn_d_ = h43t_depth_;
                } else if (teq("reflabel", 8)) {
                    fn_ = 3;
                    fn_d_ = h43t_depth_;
                } else if (teq("notelabel", 9)) {
                    fn_ = 4;
                    fn_d_ = h43t_depth_;
                }
#endif
#if HP_TAXOBOX_MOD
                if (tpre("taxobox", 7)) {
                    taxobox_ = 1;
                    taxobox_d_ = h43t_depth_;
                }
#endif
#if HP_NIHONGO_MOD
                if (tpre("nihongo", 7)) {
                    nihongo_ = 1;
                    nihongo_d_ = h43t_depth_;
                } else if (tpre("korean", 6)) {
                    nihongo_ = 2;
                    nihongo_d_ = h43t_depth_;
                } else if (tpre("chinese", 7)) {
                    nihongo_ = 3;
                    nihongo_d_ = h43t_depth_;
                }
#endif
#if HP_DEADLINK_MOD
                if (teq("deadlink", 8)) {
                    deadlink_ = 1;
                    deadlink_d_ = h43t_depth_;
                } else if (teq("brokenlink", 10)) {
                    deadlink_ = 2;
                    deadlink_d_ = h43t_depth_;
                }
#endif
#if HP_WAYBACK_MOD
                if (tpre("wayback", 7)) {
                    wayback_ = 1;
                    wayback_d_ = h43t_depth_;
                } else if (tpre("webarchive", 10)) {
                    wayback_ = 2;
                    wayback_d_ = h43t_depth_;
                } else if (tpre("dmoz", 4)) {
                    wayback_ = 3;
                    wayback_d_ = h43t_depth_;
                }
#endif
#if HP_UNREF_MOD
                if (tpre("unreferenced", 12)) {
                    unref_ = 1;
                    unref_d_ = h43t_depth_;
                } else if (teq("unref", 5)) {
                    unref_ = 2;
                    unref_d_ = h43t_depth_;
                } else if (tpre("refimprove", 10)) {
                    unref_ = 3;
                    unref_d_ = h43t_depth_;
                }
#endif
#if HP_CLEANUP_MOD
                if (tpre("cleanup", 7)) {
                    cleanup_ = 1;
                    cleanup_d_ = h43t_depth_;
                } else if (tpre("wikify", 6)) {
                    cleanup_ = 2;
                    cleanup_d_ = h43t_depth_;
                } else if (tpre("orphan", 6)) {
                    cleanup_ = 3;
                    cleanup_d_ = h43t_depth_;
                }
#endif
#if HP_NPOV_MOD
                if (tpre("npov", 4)) {
                    npov_ = 1;
                    npov_d_ = h43t_depth_;
                } else if (tpre("pov", 3)) {
                    npov_ = 2;
                    npov_d_ = h43t_depth_;
                } else if (teq("coi", 3)) {
                    npov_ = 3;
                    npov_d_ = h43t_depth_;
                } else if (tpre("advert", 6)) {
                    npov_ = 4;
                    npov_d_ = h43t_depth_;
                }
#endif
#if HP_RFROM_MOD
                if (tpre("rfrom", 5)) {
                    rfrom_ = 1;
                    rfrom_d_ = h43t_depth_;
                } else if (tpre("rto", 3)) {
                    rfrom_ = 2;
                    rfrom_d_ = h43t_depth_;
                } else if (tpre("softredirect", 12)) {
                    rfrom_ = 3;
                    rfrom_d_ = h43t_depth_;
                }
#endif
#if HP_DOI_MOD
                if (teq("doi", 3)) {
                    doi_ = 2;
                    doi_d_ = h43t_depth_;
                } else if (tpre("citedoi", 7)) {
                    doi_ = 3;
                    doi_d_ = h43t_depth_;
                }
#endif
#if HP_PMID_MOD
                if (teq("pmid", 4)) {
                    pmid_ = 3;
                    pmid_d_ = h43t_depth_;
                } else if (teq("pmc", 3)) {
                    pmid_ = 4;
                    pmid_d_ = h43t_depth_;
                }
#endif
#if HP_MEDAL_MOD
                if (tpre("medal", 5)) {
                    medal_ = 1;
                    medal_d_ = h43t_depth_;
                } else if (tpre("gold", 4)) {
                    medal_ = 2;
                    medal_d_ = h43t_depth_;
                } else if (tpre("silver", 6)) {
                    medal_ = 3;
                    medal_d_ = h43t_depth_;
                } else if (tpre("bronze", 6)) {
                    medal_ = 4;
                    medal_d_ = h43t_depth_;
                }
#endif
#if HP_FURTHER_MOD
                if (tpre("further", 7)) {
                    further_ = 1;
                    further_d_ = h43t_depth_;
                } else if (tpre("details", 7)) {
                    further_ = 2;
                    further_d_ = h43t_depth_;
                } else if (teq("more", 4) && (c == '|' || c == '}')) {
                    further_ = 3;
                    further_d_ = h43t_depth_;
                }
#endif
#if HP_DEATH_MOD
                if (tpre("deathdate", 9)) {
                    death_ = 1;
                    death_d_ = h43t_depth_;
                } else if (tpre("deathyear", 9)) {
                    death_ = 2;
                    death_d_ = h43t_depth_;
                } else if (teq("dda", 3)) {
                    death_ = 3;
                    death_d_ = h43t_depth_;
                }
#endif
#if HP_HARV_MOD
                if (tpre("harvnb", 6)) {
                    harv_ = 2;
                    harv_d_ = h43t_depth_;
                } else if (tpre("harvtxt", 7)) {
                    harv_ = 3;
                    harv_d_ = h43t_depth_;
                } else if (tpre("harvp", 5)) {
                    harv_ = 4;
                    harv_d_ = h43t_depth_;
                } else if (tpre("harv", 4)) {
                    harv_ = 1;
                    harv_d_ = h43t_depth_;
                }
#endif
            }
        }
#endif
#if HP_SECLEVEL_MOD
        if (c == '\n') {
            sl_run_ = 0;
            sl_at_ = 0;
        } else if (first_of_line_ && c == '=') {
            sl_run_ = 1;
            sl_at_ = 1;
        } else if (sl_at_ && c == '=') {
            if (sl_run_ < 6) ++sl_run_;
        } else if (sl_at_) {
            sl_at_ = 0;
            seclevel_ = sl_run_;
        }
#endif
#if HP_BRACE3_MOD
        if (c == '{') {
            b3_cls_ = 0;
            if (b3_run_ < 3) ++b3_run_;
            if (b3_run_ == 3) {
                if (brace3_ < 3) ++brace3_;
                b3_run_ = 0;
            }
        } else if (c == '}') {
            b3_run_ = 0;
            if (b3_cls_ < 3) ++b3_cls_;
            if (b3_cls_ == 3) {
                if (brace3_ > 0) --brace3_;
                b3_cls_ = 0;
            }
        } else {
            b3_run_ = 0;
            b3_cls_ = 0;
        }
#endif
#if HP_NAMEDARG_MOD
        if (state_ == kWkCurly || state_ == kWkVerticalBar) {
            if (c == '|')
                namedarg_ = 0;
            else if (c == '=')
                namedarg_ = 1;
        } else {
            namedarg_ = 0;
        }
#endif
#if HP_SIG_MOD
        if (c == '~') {
            if (sig_ < 4) ++sig_;
        } else {
            sig_ = 0;
        }
#endif
#if HP_WIKIBOLD_MOD
        if (c == '\n') {
            wikibold_ = 0;
            wb_run_ = 0;
        } else if (c == '\'') {
            if (wb_run_ < 5) ++wb_run_;
        } else {
            if (wb_run_ == 2)
                wikibold_ ^= 1;
            else if (wb_run_ == 3)
                wikibold_ ^= 2;
            else if (wb_run_ == 4 || wb_run_ == 5)
                wikibold_ ^= 3;
            wb_run_ = 0;
        }
#endif
#if HP_URLPART_MOD
        if (state_ == kWkHtLink) {
            if (c == ' ' || c == '\n' || c == ']' || c == '"' || c == '<')
                urlpart_ = 0;
            else if (c == '#')
                urlpart_ = 4;
            else if (c == '?')
                urlpart_ = 3;
            else if (c == '/' && urlpart_ <= 1)
                urlpart_ = 2;
            else if (urlpart_ == 0)
                urlpart_ = 1;
        } else {
            urlpart_ = 0;
        }
#endif
#if HP_PRESPACE_MOD
        if (c == '\n')
            prespace_ = 0;
        else if (first_of_line_ && (c == ' ' || c == '\t'))
            prespace_ = 1;
#endif
#if HP_EXTDISP_MOD
        if (ed_pend_) {
            ed_pend_ = 0;
            if (c != '[') {
                ed_on_ = 1;
                extdisp_ = 0;
                ed_slash_ = 0;
            }
        }
        if (ed_on_) {
            if (c == ']' || c == '\n') {
                ed_on_ = 0;
                extdisp_ = 0;
                ed_slash_ = 0;
            } else if (c == '/' && prev1_ == '/') {
                ed_slash_ = 1;
            } else if (c == ' ' && ed_slash_) {
                extdisp_ = 1;
            }
        }
        if (c == '[' && prev1_ != '[') ed_pend_ = 1;
#endif
#if HP_PXSIZE_MOD
        if (c >= '0' && c <= '9') {
            if (px_n_ < 4) {
                px_acc_ = px_acc_ * 10 + (c - '0');
                ++px_n_;
            }
            px_p_ = 0;
        } else if (px_n_ && (c | 32) == 'p') {
            px_p_ = 1;
        } else if (px_p_ && (c | 32) == 'x') {
            int v = px_acc_;
            int b = 8;
            if (v <= 16) b = 1;
            else if (v <= 32) b = 2;
            else if (v <= 64) b = 3;
            else if (v <= 120) b = 4;
            else if (v <= 180) b = 5;
            else if (v <= 250) b = 6;
            else if (v <= 400) b = 7;
            pxsize_ = b;
            px_n_ = 0;
            px_p_ = 0;
            px_acc_ = 0;
        } else {
            px_n_ = 0;
            px_p_ = 0;
            px_acc_ = 0;
        }
#endif
#if HP_ENTNUM_MOD
        if (c == '&') {
            en_st_ = 1;
            entnum_ = 0;
        } else if (en_st_ == 1) {
            en_st_ = (c == '#') ? 2 : 0;
        } else if (en_st_ == 2) {
            if (c == 'x' || c == 'X') {
                en_st_ = 3;
                entnum_ = 2;
            } else if (c >= '0' && c <= '9') {
                en_st_ = 3;
                entnum_ = 1;
            } else {
                en_st_ = 0;
                entnum_ = 0;
            }
        } else if (en_st_ == 3) {
            if (c == ';') {
                en_st_ = 0;
            } else if (!((c >= '0' && c <= '9') ||
                         (c >= 'a' && c <= 'f') ||
                         (c >= 'A' && c <= 'F'))) {
                en_st_ = 0;
                entnum_ = 0;
            }
        }
#endif
#if HP_WIKIHR_MOD
        if (c == '\n') {
            hr_run_ = 0;
            wikihr_ = 0;
        } else if (hr_run_) {
            if (c == '-') {
                if (hr_run_ < 8) ++hr_run_;
                if (hr_run_ >= 4) wikihr_ = 1;
            } else {
                hr_run_ = 0;
            }
        } else if (first_of_line_ && c == '-') {
            hr_run_ = 1;
        }
#endif
#if HP_FONTCOL_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            fc_win_ = (fc_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if (c == '<' || c == '>') {
                fontcol_ = 0;
                fc_eat_ = 0;
            } else if (fc_eat_) {
                if (c == '"' || c == '\'' || c == ' ' || c == '|' || c == '\n')
                    fc_eat_ = 0;
            } else if ((fc_win_ & 0xffffffffffffull) == 0x636f6c6f723dull) {
                fontcol_ = 1;
                fc_eat_ = 1;
            } else if ((fc_win_ & 0xffffffffffull) == 0x73697a653dull) {
                fontcol_ = 2;
                fc_eat_ = 1;
            }
        }
#endif
#if HP_UTF8ST_MOD
        {
            const unsigned u = static_cast<unsigned>(c) & 255u;
            if (u < 0x80u) {
                utf8st_ = 0;
                utf8left_ = 0;
            } else if (u >= 0xC0u) {
                utf8st_ = static_cast<int>(u >> 4);
                if (u < 0xE0u) utf8left_ = 1;
                else if (u < 0xF0u) utf8left_ = 2;
                else utf8left_ = 3;
            } else {
                if (utf8left_ > 0) --utf8left_;
                utf8st_ = 16 + utf8left_;
            }
        }
#endif
#if HP_DLTERM_MOD
        if (c == '\n') {
            dlterm_ = 0;
            dl_on_ = 0;
        } else if (first_of_line_ && c == ';') {
            dlterm_ = 1;
            dl_on_ = 1;
        } else if (dl_on_ && c == ':') {
            dlterm_ = 2;
        }
#endif
#if HP_HEADCLOSE_MOD
        if (c == '\n') {
            hc_sol_ = 1;
            hc_eq_ = 0;
            hc_text_ = 0;
            headclose_ = 0;
        } else if (hc_sol_ && c == '=') {
            hc_eq_ = 1;
        } else if (hc_eq_) {
            if (c == '=') {
                if (hc_text_ && headclose_ < 6) ++headclose_;
            } else if (c != ' ' && c != '\t') {
                hc_text_ = 1;
            }
            hc_sol_ = 0;
        } else {
            hc_sol_ = 0;
        }
#endif
#if HP_WIKITIME_MOD
        if (c >= '0' && c <= '9') {
            if (tm_st_ == 2) {
                if (tm_n2_ < 2) ++tm_n2_;
                if (tm_n2_ >= 2) wikitime_ = 1;
            } else {
                tm_st_ = 1;
                if (tm_n1_ < 2) ++tm_n1_;
            }
        } else if (c == ':' && tm_st_ == 1 && tm_n1_ >= 1) {
            tm_st_ = 2;
            tm_n2_ = 0;
        } else {
            tm_st_ = 0;
            tm_n1_ = 0;
            tm_n2_ = 0;
            if (c != ':') wikitime_ = 0;
        }
#endif
#if HP_BR_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            br_win_ = (br_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((br_win_ & 0xffffffull) == 0x3c6272ull) {
                br_ = 1;
            } else if (c == '>' || c == '\n') {
                br_ = 0;
            }
        }
#endif
#if HP_AMPNBSP_MOD
        {
            ampnbsp_win_ = (ampnbsp_win_ << 8) | static_cast<std::uint64_t>(c & 255);
            if ((ampnbsp_win_ & 0xffffffffffffull) == 0x266e6273703bull)
                ampnbsp_ = 1;
            else if (c == ';' || c == ' ' || c == '\n')
                ampnbsp_ = 0;
        }
#endif
#if HP_MDASH_MOD
        {
            const unsigned u = static_cast<unsigned>(c) & 255u;
            if (u == 0xE2u) {
                md_st_ = 1;
            } else if (md_st_ == 1 && u == 0x80u) {
                md_st_ = 2;
            } else if (md_st_ == 2 && (u == 0x93u || u == 0x94u || u == 0x92u)) {
                mdash_ = (u == 0x94u) ? 2 : (u == 0x93u) ? 1 : 3;
                md_st_ = 0;
            } else {
                md_st_ = 0;
                if (u < 0x80u) mdash_ = 0;
            }
        }
#endif
#if HP_LISTMIX_MOD
        if (c == '\n') {
            listmix_ = 0;
            lm_run_ = 1;
        } else if (lm_run_) {
            if (c == '*') listmix_ |= 1;
            else if (c == '#') listmix_ |= 2;
            else if (c == ':') listmix_ |= 4;
            else if (c == ';') listmix_ |= 8;
            else
                lm_run_ = 0;
        }
#endif
#if HP_MATH_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            mh_win_ = (mh_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((mh_win_ & 0xffffffffffffull) == 0x3c2f6d617468ull) {
                in_math_ = 0;
                mh_open_ = 0;
            } else if ((mh_win_ & 0xffffffffffull) == 0x3c6d617468ull) {
                mh_open_ = 1;
            } else if (c == '>' && mh_open_) {
                in_math_ = 1;
                mh_open_ = 0;
            }
        }
#endif
#if HP_CHEM_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            ch_win_ = (ch_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((ch_win_ & 0xffffffffffffull) == 0x3c2f6368656dull) {
                in_chem_ = 0;
                ch_open_ = 0;
            } else if ((ch_win_ & 0xffffffffffull) == 0x3c6368656dull) {
                ch_open_ = 1;
            } else if (c == '>' && ch_open_) {
                in_chem_ = 1;
                ch_open_ = 0;
            }
        }
#endif
#if HP_SMALL_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            sm_win_ = (sm_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((sm_win_ & 0xffffffffffffffull) == 0x3c2f736d616c6cull) {
                in_small_ = 0;
                sm_open_ = 0;
            } else if ((sm_win_ & 0xffffffffffffull) == 0x3c736d616c6cull) {
                sm_open_ = 1;
            } else if (c == '>' && sm_open_) {
                in_small_ = 1;
                sm_open_ = 0;
            } else if (sm_open_ && lc >= 'a' && lc <= 'z') {
                sm_open_ = 0;
            }
        }
#endif
#if HP_SUPSUB_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            ss_win_ = (ss_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((ss_win_ & 0xffffffffffull) == 0x3c2f737570ull) {
                if (supsub_ == 1) supsub_ = 0;
                ss_open_ = 0;
            } else if ((ss_win_ & 0xffffffffffull) == 0x3c2f737562ull) {
                if (supsub_ == 2) supsub_ = 0;
                ss_open_ = 0;
            } else if ((ss_win_ & 0xffffffffull) == 0x3c737570ull) {
                ss_open_ = 1;
            } else if ((ss_win_ & 0xffffffffull) == 0x3c737562ull) {
                ss_open_ = 2;
            } else if (c == '>' && ss_open_) {
                supsub_ = ss_open_;
                ss_open_ = 0;
            } else if (ss_open_ && lc >= 'a' && lc <= 'z') {
                ss_open_ = 0;
            }
        }
#endif
#if HP_PRECODE_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            pc_win_ = (pc_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((pc_win_ & 0xffffffffffull) == 0x3c2f707265ull) {
                if (precode_ == 1) precode_ = 0;
                pc_open_ = 0;
            } else if ((pc_win_ & 0xffffffffffffull) == 0x3c2f636f6465ull) {
                if (precode_ == 2) precode_ = 0;
                pc_open_ = 0;
            } else if ((pc_win_ & 0xffffffffull) == 0x3c2f7474ull) {
                if (precode_ == 3) precode_ = 0;
                pc_open_ = 0;
            } else if ((pc_win_ & 0xffffffffull) == 0x3c707265ull) {
                pc_open_ = 1;
            } else if ((pc_win_ & 0xffffffffffull) == 0x3c636f6465ull) {
                pc_open_ = 2;
            } else if ((pc_win_ & 0xffffffull) == 0x3c7474ull) {
                pc_open_ = 3;
            } else if (c == '>' && pc_open_) {
                precode_ = pc_open_;
                pc_open_ = 0;
            } else if (pc_open_ && lc >= 'a' && lc <= 'z') {
                pc_open_ = 0;
            }
        }
#endif
#if HP_SYNTAX_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            sx_win_ = (sx_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if (sx_win_ == 0x3c2f73796e746178ull) {
                if (syntax_ == 1 || syntax_ == 3) syntax_ = 0;
                sx_open_ = 0;
            } else if (sx_win_ == 0x3c2f736f75726365ull) {
                if (syntax_ == 2) syntax_ = 0;
                sx_open_ = 0;
            } else if ((sx_win_ & 0xffffffffffffffull) == 0x3c73796e746178ull) {
                sx_open_ = 3;
            } else if ((sx_win_ & 0xffffffffffffffull) == 0x3c736f75726365ull) {
                sx_open_ = 2;
            } else if (sx_open_ == 3 && lc == 'h') {
                sx_open_ = 1;
            } else if (sx_open_ == 2 && lc >= 'a' && lc <= 'z') {
                sx_open_ = 0;
            } else if (c == '>' && sx_open_) {
                syntax_ = sx_open_;
                sx_open_ = 0;
            } else if (sx_open_ && (c == ' ' || c == '\n' || c == '/')) {
                ;
            } else if (sx_open_ && sx_open_ != 1 && lc >= 'a' && lc <= 'z') {
                sx_open_ = 0;
            }
        }
#endif
#if HP_PROTOCOL_MOD
        {
            const int lc = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                               ? (c | 32) : 0;
            if (lc) {
                if (proto_n_ < 8) proto_buf_[proto_n_++] = static_cast<char>(lc);
            } else if (c != ':' && c != '/') {
                proto_n_ = 0;
            }
            if (c == '/' && prev1_ == '/') {
                auto eq = [&](const char* s, int m) {
                    if (proto_n_ != m) return 0;
                    for (int i = 0; i < m; ++i)
                        if (proto_buf_[i] != s[i]) return 0;
                    return 1;
                };
                if (eq("https", 5)) protocol_ = 2;
                else if (eq("http", 4)) protocol_ = 1;
                else if (eq("ftp", 3)) protocol_ = 3;
                else if (eq("mailto", 6)) protocol_ = 4;
                else if (eq("irc", 3)) protocol_ = 5;
                else if (eq("file", 4)) protocol_ = 6;
                else if (proto_n_ >= 2) protocol_ = 7;
                proto_n_ = 0;
            }
            if (c == ' ' || c == '\n' || c == ']' || c == '<') protocol_ = 0;
        }
#endif
#if HP_HEXRUN_MOD
        {
            const int hx = (c >= '0' && c <= '9') ||
                           ((c | 32) >= 'a' && (c | 32) <= 'f');
            if (c == '#') {
                hex_on_ = 1;
                hexrun_ = 0;
            } else if (hex_on_ && hx) {
                if (hexrun_ < 8) ++hexrun_;
                hexval_ = hexrun_;
            } else {
                hex_on_ = 0;
            }
        }
#endif
#if HP_SQDEPTH_MOD
        if (prev1_ == '[' && c == '[') {
            if (sqdepth_ < 7) ++sqdepth_;
        } else if (prev1_ == ']' && c == ']') {
            if (sqdepth_ > 0) --sqdepth_;
        }
#endif
#if HP_PIPEROLE_MOD
        if (c == '|') {
            if (state_ == kWkSquareOpen)
                piperole_ = 3;
            else if (in_table_ || state_ == kWkWikiTable)
                piperole_ = 1;
            else if (state_ == kWkCurly || state_ == kWkVerticalBar)
                piperole_ = 2;
        }
#endif
#if HP_AFTERREF_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            ar_win_ = (ar_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((ar_win_ & 0xffffffffffffull) == 0x3c2f7265663eull)
                afterref_ = 1;
            else if (afterref_ &&
                     ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
                afterref_ = 0;
        }
#endif
#if HP_SENTPOS_MOD
        {
            const int letter =
                (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
            if (c == '.' || c == '?' || c == '!' || c == '\n') {
                sentpos_ = 0;
                sp_inword_ = 0;
            } else if (letter) {
                if (!sp_inword_) {
                    if (sentpos_ < 31) ++sentpos_;
                    sp_inword_ = 1;
                }
            } else {
                sp_inword_ = 0;
            }
        }
#endif
#if HP_ABBREV_MOD
        {
            const int lc = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                               ? (c | 32) : 0;
            if (lc) {
                if (ab_n_ < 8) {
                    ab_w_ = (ab_w_ << 8) | static_cast<std::uint64_t>(lc);
                    ++ab_n_;
                }
            } else if (c == '.') {
                abbrev_ = 2;
                if (ab_n_ == 1) {
                    abbrev_ = 1;
                } else if (ab_n_ >= 2 && ab_n_ <= 6) {
                    auto eq = [&](const char* s, int n) {
                        if (ab_n_ != n) return 0;
                        for (int i = 0; i < n; ++i) {
                            const int sh = 8 * (n - 1 - i);
                            if (((ab_w_ >> sh) & 255u) !=
                                static_cast<unsigned char>(s[i]))
                                return 0;
                        }
                        return 1;
                    };
                    if (eq("mr", 2) || eq("ms", 2) || eq("dr", 2) ||
                        eq("st", 2) || eq("vs", 2) || eq("eg", 2) ||
                        eq("ie", 2) || eq("al", 2) || eq("ca", 2) ||
                        eq("pp", 2) || eq("jr", 2) || eq("sr", 2) ||
                        eq("no", 2) || eq("ed", 2) || eq("ft", 2) ||
                        eq("lb", 2) || eq("oz", 2) || eq("kg", 2) ||
                        eq("km", 2) || eq("cm", 2) || eq("mm", 2) ||
                        eq("mrs", 3) || eq("etc", 3) || eq("vol", 3) ||
                        eq("fig", 3) || eq("inc", 3) || eq("ltd", 3) ||
                        eq("rev", 3) || eq("gen", 3) || eq("col", 3) ||
                        eq("prof", 4) || eq("dept", 4) || eq("univ", 4) ||
                        eq("approx", 6))
                        abbrev_ = 1;
                }
                ab_n_ = 0;
                ab_w_ = 0;
            } else {
                ab_n_ = 0;
                ab_w_ = 0;
                if (c == '\n') abbrev_ = 0;
            }
        }
#endif
#if HP_THOUSAND_MOD
        if (c >= '0' && c <= '9') {
            if (th_st_ == 2) {
                th_st_ = 3;
                th_n_ = 1;
            } else if (th_st_ == 3) {
                if (th_n_ < 3) ++th_n_;
                if (th_n_ == 3) thousand_ = 1;
            } else {
                th_st_ = 1;
                if (th_n_ < 15) ++th_n_;
            }
        } else if (c == ',' && th_st_ == 1) {
            th_st_ = 2;
            th_n_ = 0;
        } else if (c == ',' && th_st_ == 3 && th_n_ == 3) {
            th_st_ = 2;
            th_n_ = 0;
        } else {
            th_st_ = 0;
            th_n_ = 0;
            if (c != ' ') thousand_ = 0;
        }
#endif
#if HP_REFPUNCT_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            rp_win_ = (rp_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((rp_win_ & 0xffffffffffull) == 0x2e3c726566ull)
                refpunct_ = 1;
            else if ((rp_win_ & 0xffffffffffffull) == 0x2f7265663e2eull)
                refpunct_ = 2;
            else if (c == '\n')
                refpunct_ = 0;
        }
#endif
#if HP_QPERIOD_MOD
        if (c == '.' && (prev1_ == '"' || prev1_ == '\''))
            qperiod_ = 1;
        else if ((c == '"' || c == '\'') && prev1_ == '.')
            qperiod_ = 2;
        else if (c == '\n')
            qperiod_ = 0;
#endif
#if HP_ELLIPSIS_MOD
        if (c == '.') {
            if (ellip_n_ < 3) ++ellip_n_;
            ellipsis_ = (ellip_n_ >= 2) ? ellip_n_ : 0;
        } else {
            ellip_n_ = 0;
            if (c != ' ') ellipsis_ = 0;
        }
#endif
#if HP_NUMRANGE_MOD
        if (c >= '0' && c <= '9') {
            if (nr_st_ == 2) {
                nr_st_ = 3;
                numrange_ = 1;
            } else if (nr_st_ != 3) {
                nr_st_ = 1;
            }
        } else if ((c == '-' || c == 0x96) && nr_st_ == 1) {
            nr_st_ = 2;
        } else if (nr_st_ == 1 && static_cast<unsigned>(c) == 0xE2u) {
            nr_utf_ = 1;
        } else if (nr_utf_ == 1 && static_cast<unsigned>(c) == 0x80u) {
            nr_utf_ = 2;
        } else if (nr_utf_ == 2 &&
                   (static_cast<unsigned>(c) == 0x93u ||
                    static_cast<unsigned>(c) == 0x94u) &&
                   nr_st_ == 1) {
            nr_st_ = 2;
            nr_utf_ = 0;
        } else {
            nr_utf_ = 0;
            if (nr_st_ == 3 && (c == ' ' || c == ',' || c == '.')) {
            } else {
                nr_st_ = 0;
                numrange_ = 0;
            }
        }
#endif
#if HP_DEG_MOD
        {
            const unsigned u = static_cast<unsigned>(c) & 255u;
            if (c >= '0' && c <= '9') {
                deg_d_ = 1;
                deg_st_ = 0;
            } else if (deg_d_ && u == 0xC2u) {
                deg_st_ = 1;
            } else if (deg_st_ == 1 && u == 0xB0u) {
                deg_ = 1;
                deg_st_ = 0;
                deg_d_ = 0;
            } else {
                deg_st_ = 0;
                if (c != ' ' && c != 'C' && c != 'F' && c != 'c' && c != 'f') {
                    deg_d_ = 0;
                    deg_ = 0;
                }
            }
            deg_win_ = (deg_win_ << 8) | static_cast<std::uint64_t>(c & 255);
            if ((deg_win_ & 0xffffffffffull) == 0x266465673bull)
                deg_ = 1;
        }
#endif
#if HP_PERCENT_MOD
        if (c >= '0' && c <= '9') {
            pct_d_ = 1;
        } else if (pct_d_ && c == '%') {
            percent_ = 1;
            pct_d_ = 0;
        } else {
            pct_d_ = 0;
            if (c != ' ') percent_ = 0;
        }
#endif
#if HP_CATBLOCK_MOD
        if (catblock_ && state_ != kWkSquareOpen &&
            c != '[' && c != ']' && c != ' ' && c != '\n' && c != '\t')
            catblock_ = 0;
#endif
#if HP_INIT_MOD
        {
            const int lc = c | 32;
            if (init_st_ == 0) {
                init_ = 0;
                if (lc >= 'a' && lc <= 'z') init_st_ = 1;
            } else if (init_st_ == 1) {
                if (c == '.') init_st_ = 2;
                else if (lc < 'a' || lc > 'z') {
                    init_ = 0;
                    init_st_ = 0;
                }
            } else {
                if (lc >= 'a' && lc <= 'z') {
                    init_ = 1;
                    init_st_ = 1;
                } else if (c == ' ') {
                    init_ = 1;
                } else {
                    init_ = 0;
                    init_st_ = 0;
                }
            }
        }
#endif
#if HP_DECIMAL_MOD
        if (c >= '0' && c <= '9') {
            if (dec_pend_) decimal_ = 1;
            dec_saw_ = 1;
            dec_pend_ = 0;
        } else if (c == '.' && dec_saw_) {
            dec_pend_ = 1;
            dec_saw_ = 0;
        } else {
            decimal_ = 0;
            dec_saw_ = 0;
            dec_pend_ = 0;
        }
#endif
#if HP_ORDINAL_MOD
        {
            const int lc = c | 32;
            if (c >= '0' && c <= '9') {
                ord_pend_ = 1;
                ord_n_ = 0;
            } else if (ord_pend_ && lc >= 'a' && lc <= 'z' && ord_n_ < 2) {
                ord_buf_[ord_n_++] = static_cast<char>(lc);
                if (ord_n_ == 2) {
                    int hit = 0;
                    if (ord_buf_[0] == 's' && ord_buf_[1] == 't') hit = 1;
                    else if (ord_buf_[0] == 'n' && ord_buf_[1] == 'd') hit = 2;
                    else if (ord_buf_[0] == 'r' && ord_buf_[1] == 'd') hit = 3;
                    else if (ord_buf_[0] == 't' && ord_buf_[1] == 'h') hit = 4;
                    if (hit) ordinal_ = hit;
                    ord_pend_ = 0;
                }
            } else if (ord_pend_ && (c == ' ' || c == '\t')) {
            } else {
                ord_pend_ = 0;
                ord_n_ = 0;
            }
        }
#endif
#if HP_UNIT_MOD
        {
            const int lc = c | 32;
            if (c >= '0' && c <= '9') {
                un_pend_ = 1;
                un_n_ = 0;
            } else if (un_pend_ && (c == ' ' || c == '\t')) {
            } else if (un_pend_ && lc >= 'a' && lc <= 'z' && un_n_ < 4) {
                un_buf_[un_n_++] = static_cast<char>(lc);
            } else {
                if (un_pend_ && un_n_ > 0) {
                    auto ueq = [&](const char* w, int n) {
                        if (un_n_ != n) return 0;
                        for (int i = 0; i < n; ++i)
                            if (un_buf_[i] != w[i]) return 0;
                        return 1;
                    };
                    int u = 0;
                    if (ueq("km", 2) || ueq("m", 1) || ueq("cm", 2) ||
                        ueq("mm", 2))
                        u = 1;
                    else if (ueq("mi", 2) || ueq("ft", 2) || ueq("in", 2) ||
                             ueq("yd", 2))
                        u = 2;
                    else if (ueq("kg", 2) || ueq("g", 1) || ueq("lb", 2) ||
                             ueq("oz", 2))
                        u = 3;
                    else if (ueq("hz", 2) || ueq("mph", 3) || ueq("kph", 3))
                        u = 4;
                    if (u) unit_ = u;
                }
                un_pend_ = 0;
                un_n_ = 0;
            }
        }
#endif
#if HP_TITLEWORD_MOD || HP_HEADWORD_MOD || HP_REPEAT_MOD
        {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z') {
                cw_acc_ = mix64(cw_acc_ * 31ull + static_cast<std::uint64_t>(lc));
                cw_on_ = 1;
            } else if (cw_on_) {
#if HP_TITLEWORD_MOD
#if HP_TITLE_MOD
                if (in_title_) {
#else
                if (0) {
#endif
                    if (tw_n_ < 8) tw_ring_[tw_n_++] = cw_acc_;
                    else {
                        for (int i = 0; i < 7; ++i) tw_ring_[i] = tw_ring_[i + 1];
                        tw_ring_[7] = cw_acc_;
                    }
                } else {
                    int hit = 0;
                    for (int i = 0; i < tw_n_; ++i)
                        if (tw_ring_[i] == cw_acc_) hit = 1;
                    titleword_ = hit;
                }
#endif
#if HP_HEADWORD_MOD
                if (first_of_line_ && c == '=') {
                    hw_n_ = 0;
                    headword_ = 0;
                }
                if (heading_level() > 0) {
                    if (hw_n_ < 8) hw_ring_[hw_n_++] = cw_acc_;
                    else {
                        for (int i = 0; i < 7; ++i) hw_ring_[i] = hw_ring_[i + 1];
                        hw_ring_[7] = cw_acc_;
                    }
                } else {
                    int hit = 0;
                    for (int i = 0; i < hw_n_; ++i)
                        if (hw_ring_[i] == cw_acc_) hit = 1;
                    headword_ = hit;
                }
#endif
#if HP_REPEAT_MOD
                repeat_ = (cw_acc_ != 0 && cw_acc_ == cw_prev_) ? 1 : 0;
                cw_prev_ = cw_acc_;
#endif
                cw_acc_ = 0;
                cw_on_ = 0;
            }
#if HP_HEADWORD_MOD
            else if (first_of_line_ && c == '=') {
                hw_n_ = 0;
            }
#endif
        }
#endif
#if HP_MONTH_MOD
        {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z') {
                if (mo_n_ < 9) mo_buf_[mo_n_++] = static_cast<char>(lc);
                else {
                    for (int i = 0; i < 8; ++i) mo_buf_[i] = mo_buf_[i + 1];
                    mo_buf_[8] = static_cast<char>(lc);
                    mo_n_ = 9;
                }
            } else {
                if (mo_n_ >= 3) {
                    auto meq = [&](const char* w, int n) {
                        if (mo_n_ != n) return 0;
                        for (int i = 0; i < n; ++i)
                            if (mo_buf_[i] != w[i]) return 0;
                        return 1;
                    };
                    int m = 0;
                    if (meq("january", 7)) m = 1;
                    else if (meq("february", 8)) m = 2;
                    else if (meq("march", 5)) m = 3;
                    else if (meq("april", 5)) m = 4;
                    else if (meq("may", 3)) m = 5;
                    else if (meq("june", 4)) m = 6;
                    else if (meq("july", 4)) m = 7;
                    else if (meq("august", 6)) m = 8;
                    else if (meq("september", 9)) m = 9;
                    else if (meq("october", 7)) m = 10;
                    else if (meq("november", 8)) m = 11;
                    else if (meq("december", 8)) m = 12;
                    if (m) month_ = m;
                }
                mo_n_ = 0;
            }
        }
#endif
#if HP_COLSPAN_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            cs_win_ = (cs_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if (cs_eat_) {
                if (c >= '0' && c <= '9') {
                    int v = cs_val_ * 10 + (c - '0');
                    cs_val_ = v > 32 ? 32 : v;
                } else {
                    cs_eat_ = 0;
                    if (cs_val_) colspan_ = cs_val_;
                }
            } else if (cs_win_ == 0x636f6c7370616e3dull ||
                       cs_win_ == 0x726f777370616e3dull) {
                cs_eat_ = 1;
                cs_val_ = 0;
            }
        }
#endif
#if HP_ROWSPAN_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            const int pre = static_cast<int>((rs_win_ >> 56) & 255);
            rs_win_ = (rs_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if (rs_win_ == 0x726f777370616e3dull) {
                if (pre == '|') rowspan_ = 1;
                else if (pre == '!') rowspan_ = 2;
            } else if (rowspan_) {
                if (prev1_ == '\n' && (c == '|' || c == '!'))
                    rowspan_ = 0;
                else if (prev1_ == '|' && (c == '|' || c == '}'))
                    rowspan_ = 0;
            }
        }
#endif
#if HP_DOI_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            doi_win_ = (doi_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((doi_win_ & 0xffffffffffull) == 0x7c646f693dull) {
                doi_ = 1;
                doi_d_ = 0;
            } else if (doi_ == 1 && (c == '|' || c == '}' || c == '\n')) {
                doi_ = 0;
            }
        }
#endif
#if HP_PMID_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            pmid_win_ = (pmid_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((pmid_win_ & 0xffffffffffffull) == 0x7c706d69643dull) {
                pmid_ = 1;
                pmid_d_ = 0;
            } else if ((pmid_win_ & 0xffffffffffull) == 0x7c706d633dull) {
                pmid_ = 2;
                pmid_d_ = 0;
            } else if ((pmid_ == 1 || pmid_ == 2) &&
                       (c == '|' || c == '}' || c == '\n')) {
                pmid_ = 0;
            }
        }
#endif
#if HP_ISBN_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            isbn_win_ = (isbn_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            const int pre = static_cast<int>((isbn_win_ >> 32) & 255);
            if ((isbn_win_ & 0xffffffffffffull) == 0x7c6973626e3dull) {
                isbn_ = 1;
            } else if ((isbn_win_ & 0xffffffffull) == 0x6973626eull) {
                if (!(pre == '|' || (pre >= 'a' && pre <= 'z')))
                    isbn_ = 2;
            } else if (isbn_) {
                if (isbn_ == 1 && (c == '|' || c == '}' || c == '\n'))
                    isbn_ = 0;
                else if (isbn_ == 2 && c == '\n')
                    isbn_ = 0;
            }
        }
#endif
#if HP_THUMB_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            thumb_win_ = (thumb_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if ((thumb_win_ & 0xffffffffffffull) == 0x7c7468756d62ull) {
                thumb_ = 1;
            } else if ((thumb_win_ & 0xffffffffffffull) == 0x7c7269676874ull) {
                thumb_ = 2;
            } else if ((thumb_win_ & 0xffffffffffull) == 0x7c6c656674ull) {
                thumb_ = 3;
            } else if (thumb_win_ == 0x7c75707269676874ull) {
                thumb_ = 4;
            } else if (thumb_win_ == 0x7c6672616d656c65ull) {
                thumb_ = 5;
            } else if (thumb_) {
                if (c == '|' || c == '\n' || c == '}')
                    thumb_ = 0;
                else if (prev1_ == ']' && c == ']')
                    thumb_ = 0;
            }
        }
#endif
#if HP_ISSN_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            issn_win_ = (issn_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            const int pre = static_cast<int>((issn_win_ >> 32) & 255);
            if ((issn_win_ & 0xffffffffffffull) == 0x7c6973736e3dull) {
                issn_ = 1;
            } else if ((issn_win_ & 0xffffffffull) == 0x6973736eull) {
                if (!(pre == '|' || (pre >= 'a' && pre <= 'z')))
                    issn_ = 2;
            } else if (issn_) {
                if (issn_ == 1 && (c == '|' || c == '}' || c == '\n'))
                    issn_ = 0;
                else if (issn_ == 2 && c == '\n')
                    issn_ = 0;
            }
        }
#endif
#if HP_OCLC_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            oclc_win_ = (oclc_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            const int pre = static_cast<int>((oclc_win_ >> 32) & 255);
            if ((oclc_win_ & 0xffffffffffffull) == 0x7c6f636c633dull) {
                oclc_ = 1;
            } else if ((oclc_win_ & 0xffffffffull) == 0x6f636c63ull) {
                if (!(pre == '|' || (pre >= 'a' && pre <= 'z')))
                    oclc_ = 2;
            } else if (oclc_) {
                if (oclc_ == 1 && (c == '|' || c == '}' || c == '\n'))
                    oclc_ = 0;
                else if (oclc_ == 2 && c == '\n')
                    oclc_ = 0;
            }
        }
#endif
#if HP_ALIGN_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            align_win_ = (align_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if (align_win_ == 0x7c76616c69676e3dull) {
                align_ = 2;
            } else if ((align_win_ & 0xffffffffffffffull) == 0x7c616c69676e3dull) {
                align_ = 1;
            } else if ((align_win_ & 0xffffffffffffffull) == 0x616c69676e3d22ull ||
                       (align_win_ & 0xffffffffffffffull) == 0x616c69676e3d27ull) {
                align_ = 3;
            } else if (align_ &&
                       (c == '|' || c == '"' || c == '\'' || c == '\n' ||
                        c == '>' || c == '}')) {
                align_ = 0;
            }
        }
#endif
#if HP_STYLE_MOD
        {
            int lc = c;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) lc = c | 32;
            st_win_ = (st_win_ << 8) | static_cast<std::uint64_t>(lc & 255);
            if (style_eat_) {
                if (c == '"' || c == '\'' || c == '|' || c == '\n' || c == '>')
                    style_eat_ = 0;
                else
                    style_hash_ = mix64(style_hash_ * 31ull +
                                        static_cast<std::uint64_t>(lc & 255));
            } else if ((st_win_ & 0xffffffffffffull) == 0x7374796c653dull) {
                style_eat_ = 1;
                style_hash_ = 0;
            }
        }
#endif

#if HP_EXTLINK_MOD
        if (in_ext_ && (c == ']' || c == '\n')) in_ext_ = 0;
        if (ext_pend_) {
            ext_pend_ = 0;
            if (c != '[') in_ext_ = 1;
        }
#endif
#if HP_ENTITY_MOD
        if (c == '&') {
            ent_collect_ = 1;
            entity_ = 0;
        } else if (ent_collect_) {
            if (c == ';' || c == ' ' || c == '\n' || c == '<')
                ent_collect_ = 0;
            else
                entity_ = mix64(entity_ * 31ull + static_cast<std::uint64_t>(c));
        }
#endif
#if HP_MAGIC_MOD
        if (c == '_' && prev1_ == '_') {
            if (magic_collect_) magic_collect_ = 0;
            else {
                magic_collect_ = 1;
                magic_hash_ = 0;
            }
        } else if (magic_collect_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z')
                magic_hash_ = mix64(magic_hash_ * 31ull +
                                    static_cast<std::uint64_t>(lc));
            else if (c != '_')
                magic_collect_ = 0;
        }
#endif
#if HP_TITLE_MOD
        if (in_title_) {
            if (c == '<') in_title_ = 0;
            else
                title_hash_ = mix64(title_hash_ * 31ull +
                                    static_cast<std::uint64_t>(c));
        }
#endif
#if HP_FWORD_MOD
        {
            const int lc = c | 32;
            if (c == '.' || c == '!' || c == '?' || c == '\n') {
                fw_pending_ = 1;
                fw_acc_ = 0;
                fw_n_ = 0;
            } else if (fw_pending_ && is_paragraph_) {
                if (lc >= 'a' && lc <= 'z') {
                    if (fw_n_ < 16) {
                        fw_acc_ = mix64(fw_acc_ * 31ull +
                                        static_cast<std::uint64_t>(lc));
                        ++fw_n_;
                    }
                } else if (fw_n_ > 0) {
                    fword_ = fw_acc_;
                    fw_pending_ = 0;
                }
            }
        }
#endif
#if HP_YEAR_MOD
        if (is_paragraph_ || in_table_ || state_ == kWkSquareOpen) {
            if (c >= '0' && c <= '9') {
                if (yr_n_ < 8) {
                    yr_val_ = yr_val_ * 10 + (c - '0');
                    ++yr_n_;
                }
            } else {
                if (yr_n_ == 4 && yr_val_ >= 1000 && yr_val_ <= 2099)
                    year_ = static_cast<std::uint64_t>(yr_val_);
                yr_n_ = 0;
                yr_val_ = 0;
            }
        }
#endif
#if HP_CAPMASK_MOD
        if (is_paragraph_ || in_table_) {
            const int up = (c >= 'A' && c <= 'Z');
            const int low = (c >= 'a' && c <= 'z');
            if (up || low) {
                if (cap_n_ == 0) cap_bits_ = up ? 1 : 2;
                else {
                    if (up) cap_bits_ |= 4;
                    if (low) cap_bits_ |= 8;
                }
                if (cap_n_ < 15) ++cap_n_;
                cap_mask_ = cap_bits_ | (cap_n_ << 4);
            } else if (cap_n_) {
                cap_mask_ = cap_bits_ | (cap_n_ << 4);
                cap_n_ = 0;
                cap_bits_ = 0;
            }
        }
#endif
#if HP_CHARCLS_MOD
        {
            int k = 0;
            if (c >= 'A' && c <= 'Z') k = 1;
            else if (c >= 'a' && c <= 'z') k = 2;
            else if (c >= '0' && c <= '9') k = 3;
            else if (c == ' ' || c == '\t') k = 4;
            else if (c == '\n') k = 5;
            else if (c == '<' || c == '>' || c == '{' || c == '}' ||
                     c == '[' || c == ']' || c == '|')
                k = 6;
            else if (c == '\'' || c == '"') k = 7;
            charcls_ = ((charcls_ << 3) | k) & 0xfff;
        }
#endif
#if HP_SHAPE_MOD || HP_SUFFIX_MOD || HP_PREFIX_MOD || HP_VOWEL_MOD || \
    HP_CONTR_MOD || HP_HYPHEN_MOD || HP_TOKENCLS_MOD
        if (is_paragraph_ || in_table_) {
            const int up = (c >= 'A' && c <= 'Z');
            const int low = (c >= 'a' && c <= 'z');
            const int dig = (c >= '0' && c <= '9');
            const int let = up || low;
            const int lc = c | 32;
            const int vow = let && (lc == 'a' || lc == 'e' || lc == 'i' ||
                                    lc == 'o' || lc == 'u' || lc == 'y');
            if (let || dig) {
#if HP_SHAPE_MOD
                {
                    const int t = up ? 1 : (low ? 2 : 3);
                    if (!shape_on_) {
                        shape_ = static_cast<std::uint64_t>(t);
                        shape_on_ = 1;
                    } else {
                        shape_ = ((shape_ << 2) |
                                  static_cast<std::uint64_t>(t)) &
                                 0xffffull;
                    }
                }
#endif
#if HP_PREFIX_MOD
                if (let && pre_n_ < 3) {
                    prefix_ = (prefix_ << 5) |
                              static_cast<std::uint64_t>(lc - 'a' + 1);
                    ++pre_n_;
                }
#endif
#if HP_SUFFIX_MOD
                if (let) {
                    suffix_acc_ = ((suffix_acc_ << 5) |
                                   static_cast<std::uint64_t>(lc - 'a' + 1)) &
                                  0x7fffull;
                    if (suf_n_ < 3) ++suf_n_;
                }
#endif
#if HP_VOWEL_MOD
                if (let)
                    vowel_ = ((vowel_ << 1) | (vow ? 1 : 0)) & 255;
#endif
#if HP_CONTR_MOD
                if (let) {
                    contr_acc_ = mix64(contr_acc_ * 31ull +
                                       static_cast<std::uint64_t>(lc));
                }
#endif
#if HP_HYPHEN_MOD
                if (let)
                    hy_acc_ = mix64(hy_acc_ * 31ull +
                                    static_cast<std::uint64_t>(lc));
#endif
#if HP_TOKENCLS_MOD
                if (tok_cls_ <= 0) tok_cls_ = let ? 1 : 2;
                else if ((tok_cls_ == 1 && dig) || (tok_cls_ == 2 && let))
                    tok_cls_ = 3;
#endif
            } else {
#if HP_SHAPE_MOD
                shape_on_ = 0;
#endif
#if HP_PREFIX_MOD
                pre_n_ = 0;
#endif
#if HP_SUFFIX_MOD
                if (suf_n_) {
                    suffix_ = suffix_acc_;
                    suffix_acc_ = 0;
                    suf_n_ = 0;
                }
#endif
#if HP_CONTR_MOD
                if (c == '\'' && contr_acc_) {
                    contr_apos_ = 1;
                } else if (c != '\'') {
                    if (contr_apos_ && contr_acc_) contr_ = contr_acc_;
                    contr_acc_ = 0;
                    contr_apos_ = 0;
                }
#endif
#if HP_HYPHEN_MOD
                if (c == '-' && hy_acc_) {
                    hy_seen_ = 1;
                } else if (c != '-') {
                    if (hy_seen_ && hy_acc_) hyphen_ = hy_acc_;
                    hy_acc_ = 0;
                    hy_seen_ = 0;
                }
#endif
#if HP_TOKENCLS_MOD
                if (in_tag_) tok_cls_ = 5;
                else if (state_ == kWkSquareOpen || state_ == kWkCurly ||
                         in_table_)
                    tok_cls_ = 6;
                else if (c != ' ' && c != '\n' && c != '\t')
                    tok_cls_ = 4;
                else
                    tok_cls_ = 0;
#endif
            }
        }
#endif
#if HP_PAREN_MOD
        if (is_paragraph_) {
            if (c == '(') {
                ++paren_d_;
                if (paren_d_ == 1) paren_acc_ = 0;
            } else if (c == ')' && paren_d_ > 0) {
                if (paren_d_ == 1 && paren_acc_) lastparen_ = paren_acc_;
                --paren_d_;
            } else if (paren_d_ > 0) {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z')
                    paren_acc_ = mix64(paren_acc_ * 31ull +
                                       static_cast<std::uint64_t>(lc));
            }
        }
#endif
#if HP_USER_MOD
        if (in_user_) {
            if (c == '<') in_user_ = 0;
            else
                user_hash_ = mix64(user_hash_ * 31ull +
                                   static_cast<std::uint64_t>(c));
        }
#endif
#if HP_PAGEID_MOD
        if (in_id_) {
            if (c >= '0' && c <= '9') {
                if (page_id_ < 100000000ull)
                    page_id_ = page_id_ * 10ull + static_cast<std::uint64_t>(c - '0');
            } else {
                in_id_ = 0;
                pageid_seen_ = 1;
            }
        }
#endif
#if HP_NS_MOD
        if (in_ns_) {
            if (c >= '0' && c <= '9') {
                if (ns_id_ < 1000)
                    ns_id_ = ns_id_ * 10 + (c - '0');
            } else {
                in_ns_ = 0;
            }
        }
#endif
#if HP_IP_MOD
        if (in_ip_) {
            if (c == '<') in_ip_ = 0;
            else
                ip_hash_ = mix64(ip_hash_ * 31ull +
                                 static_cast<std::uint64_t>(c));
        }
#endif
#if HP_REVCOMMENT_MOD
        if (in_comment_) {
            if (c == '<') in_comment_ = 0;
            else
                comment_hash_ = mix64(comment_hash_ * 31ull +
                                      static_cast<std::uint64_t>(c));
        }
#endif
#if HP_WIKIMODEL_MOD
        if (in_model_) {
            if (c == '<') in_model_ = 0;
            else
                model_hash_ = mix64(model_hash_ * 31ull +
                                    static_cast<std::uint64_t>(c));
        }
#endif
#if HP_PARSERFN_MOD
        if (pfn_wait_) {
            pfn_wait_ = 0;
            if (c == '#') pfn_collect_ = 1;
        } else if (pfn_collect_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z')
                pfn_hash_ = mix64(pfn_hash_ * 31ull +
                                  static_cast<std::uint64_t>(lc));
            else
                pfn_collect_ = 0;
        }
#endif
#if HP_TABLECLASS_MOD
        if (tc_collect_) {
            if (c == '\n') tc_collect_ = 0;
            else {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z')
                    tc_hash_ = mix64(tc_hash_ * 31ull +
                                     static_cast<std::uint64_t>(lc));
            }
        }
#endif
#if HP_PUBID_MOD
        {
            const unsigned lc = static_cast<unsigned>((c | 32) & 255);
            pub_win_ = (pub_win_ << 8) | lc;
            if ((pub_win_ & 0xffffffffu) == 0x6973626eu /*isbn*/ ||
                (pub_win_ & 0xffffffffu) == 0x706d6964u /*pmid*/) {
                pub_collect_ = 1;
                pub_hash_ = 0;
            } else if (pub_collect_) {
                if (c >= '0' && c <= '9')
                    pub_hash_ = mix64(pub_hash_ * 31ull +
                                      static_cast<std::uint64_t>(c - '0'));
                else if (c != '-' && c != ' ')
                    pub_collect_ = 0;
            }
        }
#endif
#if HP_TEMPPOS_MOD
        if (tp_collect_) {
            if (c == '=' || c == '|' || c == '}') {
                if (c == '=') tp_hash_ = 0;
                tp_collect_ = 0;
                tp_done_ = 1;
            } else {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z')
                    tp_hash_ = mix64(tp_hash_ * 31ull +
                                     static_cast<std::uint64_t>(lc));
            }
        }
#endif

        // Close states that have a terminator.
        if (state_ == kWkAmp || state_ == kWkEntity) {
            if (c == ';' || c == ' ' || c == '\n' || c == '<') state_ = kWkText;
        }
        if (state_ == kWkHtLink) {
            if (c == ' ' || c == '\n' || c == '<' || c == ']' || c == '"')
                state_ = kWkText;
        }
        if (state_ == kWkQuote && c != '\'') state_ = kWkText;

        if (state_ == kWkComment) {
            if (prev2_ == '-' && prev1_ == '-' && c == '>') {
                state_ = kWkText;
                if (depth_ > 0) --depth_;
            }
            return;
        }

        if (c == '<') {
            in_tag_ = 1;
            tag_name_ = 0;
            if (depth_ < 15) ++depth_;
            state_ = kWkTag;
#if HP_CITE_MOD
            cite_slash_ = 0;
            cite_buf_ = 0;
#endif
#if HP_GALLERY_MOD
            gal_slash_ = 0;
            gal_n_ = 0;
#endif
#if HP_REFNAME_MOD
            refn_collect_ = 0;
            refn_win_ = 0;
            refn_skipq_ = 0;
#endif
#if HP_NOWIKI_MOD
            nw_slash_ = 0;
            nw_n_ = 0;
            nw_name_ = 0;
#endif
#if HP_DUMP_XML
            xml_slash_ = 0;
            xml_n_ = 0;
            xml_name_ = 0;
            xml_done_ = 0;
#endif
#if HP_HTMLFMT_MOD
            hf_slash_ = 0;
            hf_n_ = 0;
#endif
#if HP_INCLUDE_MOD
            ic_slash_ = 0;
            ic_n_ = 0;
#endif
#if HP_REFIDX_MOD
            ri_slash_ = 0;
            ri_n_ = 0;
#endif
#if HP_REFGROUP_MOD
            rg_slash_ = 0;
            rg_n_ = 0;
            rg_win_ = 0;
            rg_name_ = 0;
            rg_group_ = 0;
#endif
#if HP_REFLIST_MOD
            rl_slash_ = 0;
            rl_n_ = 0;
#endif
#if HP_BLOCK_MOD
            bk_slash_ = 0;
            bk_n_ = 0;
#endif
            return;
        }
        if (c == '>' && in_tag_) {
#if HP_CITE_MOD
            if ((cite_buf_ & 0xffffffu) == 0x726566u)
                in_ref_ = cite_slash_ ? 0 : 1;
#endif
#if HP_GALLERY_MOD
            if (gal_n_ == 7) {
                static const char kGal[] = "gallery";
                int ok = 1;
                for (int i = 0; i < 7; ++i)
                    if (gal_buf_[i] != kGal[i]) ok = 0;
                if (ok) in_gallery_ = gal_slash_ ? 0 : 1;
            }
#endif
#if HP_REFNAME_MOD
            refn_collect_ = 0;
#endif
#if HP_NOWIKI_MOD
            nowiki_apply_();
#endif
#if HP_DUMP_XML
            dump_apply_();
#endif
#if HP_HTMLFMT_MOD
            if (hf_n_ > 0) {
                auto heq = [&](const char* w, int n) {
                    if (hf_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (hf_buf_[i] != w[i]) return 0;
                    return 1;
                };
                int bit = 0;
                if (heq("small", 5)) bit = 1;
                else if (heq("sup", 3)) bit = 2;
                else if (heq("sub", 3)) bit = 4;
                else if (heq("b", 1) || heq("strong", 6)) bit = 8;
                else if (heq("i", 1) || heq("em", 2)) bit = 16;
                else if (heq("big", 3)) bit = 32;
                else if (heq("s", 1) || heq("u", 1) || heq("strike", 6))
                    bit = 64;
                if (bit) {
                    if (hf_slash_) htmlfmt_ &= ~bit;
                    else htmlfmt_ |= bit;
                }
            }
#endif
#if HP_INCLUDE_MOD
            if (ic_n_ > 0) {
                auto ieq = [&](const char* w, int n) {
                    if (ic_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (ic_buf_[i] != w[i]) return 0;
                    return 1;
                };
                int bit = 0;
                if (ieq("includeonly", 11)) bit = 1;
                else if (ieq("noinclude", 9)) bit = 2;
                else if (ieq("onlyinclude", 11)) bit = 4;
                if (bit) {
                    if (ic_slash_) include_ &= ~bit;
                    else include_ |= bit;
                }
            }
#endif
#if HP_REFIDX_MOD
            if (!ri_slash_ && ri_n_ == 3 && ri_buf_[0] == 'r' &&
                ri_buf_[1] == 'e' && ri_buf_[2] == 'f') {
                if (refidx_ < 15) ++refidx_;
            }
#endif
#if HP_REFGROUP_MOD
            if (rg_n_ == 3 && rg_buf_[0] == 'r' && rg_buf_[1] == 'e' &&
                rg_buf_[2] == 'f') {
                if (rg_slash_)
                    refgroup_ = 0;
                else if (rg_group_)
                    refgroup_ = 3;
                else if (rg_name_)
                    refgroup_ = 2;
                else
                    refgroup_ = 1;
            }
#endif
#if HP_REFLIST_MOD
            if (rl_n_ == 10) {
                static const char kRl[] = "references";
                int ok = 1;
                for (int i = 0; i < 10; ++i)
                    if (rl_buf_[i] != kRl[i]) ok = 0;
                if (ok) {
                    if (rl_slash_) {
                        reflist_ = 0;
                        rl_from_tpl_ = 0;
                    } else {
                        reflist_ = 1;
                        rl_from_tpl_ = 0;
                    }
                }
            }
#endif
#if HP_BLOCK_MOD
            if (bk_n_ > 0) {
                auto beq = [&](const char* w, int n) {
                    if (bk_n_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (bk_buf_[i] != w[i]) return 0;
                    return 1;
                };
                int kind = 0;
                if (beq("blockquote", 10)) kind = 1;
                else if (beq("center", 6)) kind = 2;
                else if (beq("div", 3)) kind = 3;
                else if (beq("span", 4)) kind = 4;
                if (kind) {
                    if (bk_slash_) {
                        if (block_ == kind) block_ = 0;
                    } else {
                        block_ = kind;
                    }
                }
            }
#endif
            in_tag_ = 0;
            state_ = kWkText;
            return;
        }
        if (c == '/' && in_tag_) {
#if HP_CITE_MOD
            if (cite_buf_ == 0) cite_slash_ = 1;
#endif
#if HP_GALLERY_MOD
            if (gal_n_ == 0) gal_slash_ = 1;
#endif
#if HP_NOWIKI_MOD
            if (nw_n_ == 0) nw_slash_ = 1;
#endif
#if HP_DUMP_XML
            if (xml_n_ == 0) xml_slash_ = 1;
#endif
#if HP_HTMLFMT_MOD
            if (hf_n_ == 0) hf_slash_ = 1;
#endif
#if HP_INCLUDE_MOD
            if (ic_n_ == 0) ic_slash_ = 1;
#endif
#if HP_REFIDX_MOD
            if (ri_n_ == 0) ri_slash_ = 1;
#endif
#if HP_REFGROUP_MOD
            if (rg_n_ == 0) rg_slash_ = 1;
#endif
#if HP_REFLIST_MOD
            if (rl_n_ == 0) rl_slash_ = 1;
#endif
#if HP_BLOCK_MOD
            if (bk_n_ == 0) bk_slash_ = 1;
#endif
            if (depth_ > 0) --depth_;
            state_ = kWkTagEnd;
            return;
        }
        if (in_tag_) {
            tag_name_ = mix64(tag_name_ * 31 + static_cast<std::uint64_t>(c));
#if HP_CITE_MOD
            {
                const unsigned ch = static_cast<unsigned>((c | 32) & 255);
                if (ch >= 'a' && ch <= 'z' && (cite_buf_ & 0xff0000u) == 0)
                    cite_buf_ = (cite_buf_ << 8) | ch;
            }
#endif
#if HP_GALLERY_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && gal_n_ < 8)
                    gal_buf_[gal_n_++] = static_cast<char>(lc);
            }
#endif
#if HP_REFNAME_MOD
            {
                const unsigned ch = static_cast<unsigned>(c & 255);
                const unsigned lc = ch | 32u;
                const unsigned packed = (ch == '=') ? '=' : lc;
                refn_win_ = (refn_win_ << 8) | packed;
                if ((refn_win_ & 0xffffffffffull) == 0x6e616d653dull) {
                    refn_collect_ = 1;
                    refname_ = 0;
                    refn_skipq_ = 1;
                } else if (refn_collect_) {
                    if (ch == '"' || ch == '\'') {
                        if (refn_skipq_) refn_skipq_ = 0;
                        else refn_collect_ = 0;
                    } else if (ch == ' ' && refn_skipq_) {
                    } else if (ch == ' ' || ch == '>' || ch == '/') {
                        refn_collect_ = 0;
                    } else {
                        refn_skipq_ = 0;
                        refname_ = mix64(refname_ * 31ull + ch);
                    }
                }
            }
#endif
#if HP_NOWIKI_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && nw_n_ < 8) {
                    nw_name_ = nw_name_ * 31ull + static_cast<std::uint64_t>(lc);
                    ++nw_n_;
                }
            }
#endif
#if HP_DUMP_XML
            if (!xml_done_) {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && xml_n_ < 12) {
                    xml_name_ = xml_name_ * 31ull + static_cast<std::uint64_t>(lc);
                    ++xml_n_;
                } else {
                    xml_done_ = 1;
                }
            }
#endif
#if HP_HTMLFMT_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && hf_n_ < 8)
                    hf_buf_[hf_n_++] = static_cast<char>(lc);
            }
#endif
#if HP_INCLUDE_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && ic_n_ < 12)
                    ic_buf_[ic_n_++] = static_cast<char>(lc);
            }
#endif
#if HP_REFIDX_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && ri_n_ < 4)
                    ri_buf_[ri_n_++] = static_cast<char>(lc);
            }
#endif
#if HP_REFGROUP_MOD
            {
                const unsigned ch = static_cast<unsigned>(c & 255);
                const unsigned lc = ch | 32u;
                const unsigned packed = (ch == '=') ? '=' : lc;
                if (lc >= 'a' && lc <= 'z' && rg_n_ < 4)
                    rg_buf_[rg_n_++] = static_cast<char>(lc);
                rg_win_ = (rg_win_ << 8) | packed;
                if ((rg_win_ & 0xffffffffffull) == 0x6e616d653dull)
                    rg_name_ = 1;
                if ((rg_win_ & 0xffffffffffffull) == 0x67726f75703dull)
                    rg_group_ = 1;
            }
#endif
#if HP_REFLIST_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && rl_n_ < 12)
                    rl_buf_[rl_n_++] = static_cast<char>(lc);
            }
#endif
#if HP_BLOCK_MOD
            {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z' && bk_n_ < 12)
                    bk_buf_[bk_n_++] = static_cast<char>(lc);
            }
#endif
            if (prev1_ == '!' && c == '-') state_ = kWkComment;
            return;
        }

        // Wiki link [[
        if (prev1_ == '[' && c == '[') {
            state_ = kWkSquareOpen;
            sq_ = 1;
            linkword_ = 0;
#if HP_CAT_MOD
            ns_collect_ = 1;
            ns_hash_ = 0;
#endif
#if HP_LINKCOMMA_MOD
            linkcomma_ = 0;
            lc_pipe_ = 0;
#endif
#if HP_CATBLOCK_MOD
            cb_n_ = 0;
            cb_hit_ = 0;
#endif
#if HP_ANCHOR_MOD
            an_collect_ = 0;
            an_hash_ = 0;
#endif
#if HP_LANG_MOD
            lang_n_ = 0;
            lang_acc_ = 0;
            lang_hash_ = 0;
#endif
#if HP_CATSORT_MOD || HP_FILEOPT_MOD
            pre_i_ = 0;
            pre_kind_ = 0;
            pipe_col_ = 0;
            extra_hash_ = 0;
#endif
#if HP_LASTLINK_MOD
            ll_acc_ = 0;
            ll_stop_ = 0;
#endif
#if HP_LINKNS_MOD
            ln_n_ = 0;
            ln_on_ = 1;
            linkns_ = 0;
#endif
#if HP_SECFRAG_MOD
            secfrag_ = 0;
#endif
#if HP_SUBPAGE_MOD
            subpage_ = 0;
#endif
#if HP_SISTER_MOD
            sister_ = 0;
            sis_on_ = 1;
            sis_n_ = 0;
#endif
#if HP_PIPETRICK_MOD
            pipetrick_ = 0;
            pt_pipe_ = 0;
#endif
#if HP_LISTEN_MOD
            ls_pre_n_ = 0;
            ls_pre_on_ = 1;
            ls_in_file_ = 0;
            ls_win_ = 0;
            if (listen_ == 3) listen_ = 0;
#endif
            return;
        }
        if (state_ == kWkSquareOpen) {
            if (c == ']') {
                if (sq_ > 0) --sq_;
                if (sq_ == 0) {
                    state_ = kWkText;
                    linkword_ = 0;
#if HP_LINKPIPE_MOD
                    link_pipe_ = 0;
                    disp_ = 0;
#endif
#if HP_CAT_MOD
                    ns_collect_ = 0;
                    ns_hash_ = 0;
#endif
#if HP_LINKCOMMA_MOD
                    linkcomma_ = 0;
                    lc_pipe_ = 0;
#endif
#if HP_CATBLOCK_MOD
                    if (cb_hit_) {
                        if (catblock_ < 15) ++catblock_;
                    } else {
                        catblock_ = 0;
                    }
                    cb_n_ = 0;
                    cb_hit_ = 0;
#endif
#if HP_ANCHOR_MOD
                    an_collect_ = 0;
#endif
#if HP_LANG_MOD
                    lang_n_ = 0;
#endif
#if HP_CATSORT_MOD || HP_FILEOPT_MOD
                    pipe_col_ = 0;
#endif
#if HP_LASTLINK_MOD
                    if (ll_acc_) lastlink_ = ll_acc_;
#endif
#if HP_LINKTRAIL_MOD
                    lt_on_ = 1;
#endif
#if HP_LINKNS_MOD
                    linkns_ = 0;
                    ln_on_ = 0;
                    ln_n_ = 0;
#endif
#if HP_SECFRAG_MOD
                    secfrag_ = 0;
#endif
#if HP_SUBPAGE_MOD
                    subpage_ = 0;
#endif
#if HP_SISTER_MOD
                    sister_ = 0;
                    sis_on_ = 0;
                    sis_n_ = 0;
#endif
#if HP_PIPETRICK_MOD
                    pipetrick_ = 0;
                    pt_pipe_ = 0;
#endif
#if HP_LISTEN_MOD
                    ls_pre_on_ = 0;
                    ls_in_file_ = 0;
                    if (listen_ == 3) listen_ = 0;
#endif
                }
            } else if (c == ':') {
                linkword_ = 0;   // fx2: [category:...] drops the hash
#if HP_LASTLINK_MOD
                ll_acc_ = 0;
#endif
#if HP_CAT_MOD
                if (ns_hash_ != 0) ns_collect_ = 0;
#endif
#if HP_LANG_MOD
                if (lang_n_ >= 2 && lang_n_ <= 3) lang_hash_ = lang_acc_;
                lang_n_ = 8;
#endif
#if HP_CATSORT_MOD || HP_FILEOPT_MOD
                pre_kind_ = 0;
                if (pre_i_ == 8) {
                    static const char kCat[] = "category";
                    int ok = 1;
                    for (int i = 0; i < 8; ++i)
                        if (pre_buf_[i] != kCat[i]) ok = 0;
                    if (ok) pre_kind_ = 1;
                } else if (pre_i_ == 4) {
                    static const char kFile[] = "file";
                    int ok = 1;
                    for (int i = 0; i < 4; ++i)
                        if (pre_buf_[i] != kFile[i]) ok = 0;
                    if (ok) pre_kind_ = 2;
                } else if (pre_i_ == 5) {
                    static const char kImg[] = "image";
                    int ok = 1;
                    for (int i = 0; i < 5; ++i)
                        if (pre_buf_[i] != kImg[i]) ok = 0;
                    if (ok) pre_kind_ = 2;
                }
#endif
#if HP_LINKNS_MOD
                if (ln_on_) {
                    linkns_ = link_ns_id_(ln_buf_, ln_n_);
                    ln_on_ = 0;
                }
#endif
#if HP_SISTER_MOD
                if (sis_on_) {
                    sister_ = sister_id_(sis_buf_, sis_n_);
                    sis_on_ = 0;
                }
#endif
#if HP_LISTEN_MOD
                if (ls_pre_on_) {
                    ls_pre_on_ = 0;
                    if (ls_pre_n_ == 4 && ls_pre_[0] == 'f' && ls_pre_[1] == 'i' &&
                        ls_pre_[2] == 'l' && ls_pre_[3] == 'e')
                        ls_in_file_ = 1;
                    else if (ls_pre_n_ == 5 && ls_pre_[0] == 'i' &&
                             ls_pre_[1] == 'm' && ls_pre_[2] == 'a' &&
                             ls_pre_[3] == 'g' && ls_pre_[4] == 'e')
                        ls_in_file_ = 1;
                }
#endif
            } else {
                // fx2-cmix: linkword = linkword * 2104 + j
                linkword_ = linkword_ * 2104ull + static_cast<std::uint64_t>(c);
#if HP_LINKPIPE_MOD
                if (c == '|') {
                    link_pipe_ = 1;
                    disp_ = 0;
                } else if (link_pipe_) {
                    disp_ = disp_ * 2104ull + static_cast<std::uint64_t>(c);
                }
#endif
#if HP_CAT_MOD
                if (ns_collect_) {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z')
                        ns_hash_ = mix64(ns_hash_ * 31ull +
                                         static_cast<std::uint64_t>(lc));
                    else if (c != ' ' && c != '_')
                        ns_collect_ = 0;
                }
#endif
#if HP_LINKNS_MOD
                if (ln_on_) {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z' && ln_n_ < 16)
                        ln_buf_[ln_n_++] = static_cast<char>(lc);
                    else if ((c == ' ' || c == '_') && ln_n_ > 0 && ln_n_ < 16)
                        ln_buf_[ln_n_++] = ' ';
                    else if (c != ' ')
                        ln_on_ = 0;
                }
#endif
#if HP_SECFRAG_MOD
                if (c == '#') secfrag_ = 1;
                else if (c == '|') secfrag_ = 0;
#endif
#if HP_SUBPAGE_MOD
                if (c == '/') subpage_ = 1;
                else if (c == '|') subpage_ = 0;
#endif
#if HP_SISTER_MOD
                if (sis_on_) {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z' && sis_n_ < 12)
                        sis_buf_[sis_n_++] = static_cast<char>(lc);
                    else if (c != ' ' && c != '_')
                        sis_on_ = 0;
                }
#endif
#if HP_PIPETRICK_MOD
                if (c == '|') {
                    pt_pipe_ = 1;
                    pipetrick_ = 1;
                } else if (pt_pipe_ && c != ']' && c != ' ' && c != '\t' &&
                           c != '\n') {
                    pipetrick_ = 0;
                    pt_pipe_ = 0;
                }
#endif
#if HP_LISTEN_MOD
                if (ls_pre_on_) {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z' && ls_pre_n_ < 8)
                        ls_pre_[ls_pre_n_++] = static_cast<char>(lc);
                    else if (c != ' ' && c != '_')
                        ls_pre_on_ = 0;
                }
                if (ls_in_file_) {
                    if (c == '|') {
                        ls_in_file_ = 0;
                    } else {
                        int lc = 0;
                        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                            lc = c | 32;
                        else if (c == '.')
                            lc = '.';
                        if (lc) {
                            ls_win_ = (ls_win_ << 8) |
                                      static_cast<std::uint64_t>(lc & 255);
                            const std::uint32_t ext = static_cast<std::uint32_t>(
                                ls_win_ & 0xffffffffull);
                            if ((ext == 0x2e6f6767u || ext == 0x2e6f6761u ||
                                 ext == 0x2e6d7033u) &&
                                listen_ == 0)
                                listen_ = 3;
                        }
                    }
                }
#endif
#if HP_LINKCOMMA_MOD
                if (c == '|')
                    lc_pipe_ = 1;
                else if (c == ',' && !lc_pipe_)
                    linkcomma_ = 1;
#endif
#if HP_CATBLOCK_MOD
                {
                    const int lc = c | 32;
                    static const char kCb[] = "category";
                    if (cb_n_ < 8 && lc == kCb[cb_n_]) {
                        ++cb_n_;
                        if (cb_n_ == 8) cb_hit_ = 1;
                    } else if (cb_n_ < 8 && c != ':' && c != ' ' && c != '_') {
                        cb_n_ = 0;
                    }
                }
#endif
#if HP_ANCHOR_MOD
                if (c == '#') {
                    an_collect_ = 1;
                    an_hash_ = 0;
                } else if (an_collect_) {
                    if (c == '|') {
                        an_collect_ = 0;
                    } else {
                        const int lc = c | 32;
                        if (lc >= 'a' && lc <= 'z')
                            an_hash_ = mix64(an_hash_ * 31ull +
                                             static_cast<std::uint64_t>(lc));
                    }
                }
#endif
#if HP_LANG_MOD
                {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z' && lang_n_ < 4) {
                        lang_acc_ = mix64(lang_acc_ * 31ull +
                                          static_cast<std::uint64_t>(lc));
                        ++lang_n_;
                    } else if (lang_n_ < 4 && c != '_' && c != ' ') {
                        lang_n_ = 0;
                    }
                }
#endif
#if HP_CATSORT_MOD || HP_FILEOPT_MOD
                {
                    const int lc = c | 32;
                    if (pre_kind_ == 0 && pre_i_ < 12 &&
                        lc >= 'a' && lc <= 'z') {
                        pre_buf_[pre_i_++] = static_cast<char>(lc);
                    }
                    if (c == '|') {
                        pipe_col_ = 1;
                        extra_hash_ = 0;
                    } else if (pipe_col_) {
#if HP_CATSORT_MOD
                        if (pre_kind_ == 1) {
                            extra_hash_ = mix64(extra_hash_ * 31ull +
                                                static_cast<std::uint64_t>(lc));
                        }
#endif
#if HP_FILEOPT_MOD
                        if (pre_kind_ == 2) {
                            extra_hash_ = mix64(extra_hash_ * 31ull +
                                                static_cast<std::uint64_t>(lc));
                        }
#endif
                    }
                }
#endif
#if HP_LASTLINK_MOD
                if (!ll_stop_) {
                    if (c == '|' || c == '#') {
                        ll_stop_ = 1;
                    } else {
                        const int lc = c | 32;
                        if (lc >= 'a' && lc <= 'z')
                            ll_acc_ = mix64(ll_acc_ * 31ull +
                                            static_cast<std::uint64_t>(lc));
                    }
                }
#endif
            }
            return;
        }

        // Template / infobox {{
        if (prev1_ == '{' && c == '{') {
            state_ = kWkCurly;
            if (depth_ < 15) ++depth_;
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
            tpl_collect_ = 1;
            tpl_name_ = 0;
            bar_idx_ = 0;
            key_ = 0;
            key_collect_ = 0;
#endif
#if HP_PARSERFN_MOD
            pfn_wait_ = 1;
            pfn_collect_ = 0;
            pfn_hash_ = 0;
#endif
#if HP_TEMPPOS_MOD
            tp_collect_ = 0;
            tp_done_ = 0;
            tp_hash_ = 0;
#endif
#if HP_DEFAULTSORT_MOD || HP_DAB_MOD || HP_HATNOTE_MOD
            tnm_i_ = 0;
            tnm_done_ = 0;
            tnm_mode_ = 0;
#endif
#if HP_CITEKIND_MOD || HP_COORD_MOD
            ck_i_ = 0;
            ck_done_ = 0;
#endif
#if HP_WIKIVAR_MOD
            wv_n_ = 0;
            wv_on_ = 1;
            wikivar_ = 0;
#endif
            return;
        }
        if (prev1_ == '{' && c == '|') {
            state_ = kWkWikiTable;
            in_table_ = 1;
#if HP_TBLDEPTH_MOD
            if (tbldepth_ < 7) ++tbldepth_;
#endif
#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
            table_reset();
#endif
#if HP_TABLECLASS_MOD
            tc_collect_ = 1;
            tc_hash_ = 0;
#endif
#if HP_TBLROW_MOD
            tbl_kind_ = 0;
            tbl_rowpend_ = 1;
            tbl_bar_ = 0;
#endif
            return;
        }
#if HP_WIKI_TEMP
        if (prev1_ == '{' && c != '{' && c != '|') is_temp_ = 1;
        if (c == '}') is_temp_ = 0;
#endif
        if (prev1_ == '|' && c == '}') {
            state_ = kWkText;
            in_table_ = 0;
#if HP_TBLDEPTH_MOD
            if (tbldepth_ > 0) --tbldepth_;
#endif
#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
            table_reset();
#endif
#if HP_TABLECLASS_MOD
            tc_collect_ = 0;
            tc_hash_ = 0;
#endif
#if HP_TBLROW_MOD
            tbl_kind_ = 0;
            tbl_rowpend_ = 0;
            tbl_bar_ = 0;
#endif
            return;
        }
        if (prev1_ == '}' && c == '}') {
            if (depth_ > 0) --depth_;
            state_ = kWkText;
#if HP_WIKIVAR_MOD
            if (wv_on_ && wv_n_ >= 4)
                wikivar_ = wiki_var_id_(wv_buf_, wv_n_);
            wv_on_ = 0;
            wikivar_ = 0;
#endif
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
            tpl_collect_ = 0;
            key_collect_ = 0;
#endif
#if HP_DEFAULTSORT_MOD || HP_DAB_MOD || HP_HATNOTE_MOD
            tnm_done_ = 1;
#endif
#if HP_CITEKIND_MOD || HP_COORD_MOD
            ck_done_ = 1;
#endif
            return;
        }

        if (c == '|' && (in_table_ || state_ == kWkCurly || state_ == kWkWikiTable)) {
            state_ = kWkVerticalBar;
#if HP_WIKIVAR_MOD
            if (wv_on_) {
                wv_on_ = 0;
                if (wv_n_ >= 4) wikivar_ = wiki_var_id_(wv_buf_, wv_n_);
            }
#endif
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
            tpl_collect_ = 0;
            if (bar_idx_ < 31) ++bar_idx_;
            key_ = 0;
            key_collect_ = 1;
#endif
#if HP_TEMPPOS_MOD
            if (state_ == kWkCurly && !tp_done_) {
                if (tp_collect_) {
                    tp_collect_ = 0;
                    tp_done_ = 1;
                } else {
                    tp_collect_ = 1;
                }
            }
#endif
            return;
        }
        if (state_ == kWkVerticalBar && c != '|') {
            // stay in table, leave the bar-cell marker after first payload byte
            if (c == '\n') state_ = kWkWikiTable;
            else if (in_table_) state_ = kWkWikiTable;
            else state_ = kWkCurly;
        }

        // http(s)://
        if ((prev2_ == 't' || prev2_ == 'T') &&
            (prev1_ == 't' || prev1_ == 'T') &&
            (c == 'p' || c == 'P')) {
            http_run_ = 3;
        } else if (http_run_ > 0) {
            ++http_run_;
            if (http_run_ >= 4 && prev1_ == '/' && c == '/') {
                state_ = kWkHtLink;
                linkword_ = 0;
                http_run_ = 0;
#if HP_HTTPHOST_MOD
                ht_acc_ = 0;
                ht_done_ = 0;
#endif
            }
            if (c == ' ' || c == '\n') http_run_ = 0;
        }
        if (state_ == kWkHtLink && c != ' ' && c != '\n') {
            linkword_ = linkword_ * 2104ull + static_cast<std::uint64_t>(c);
        }
#if HP_HTTPHOST_MOD
        if (state_ == kWkHtLink && !ht_done_) {
            if (c == '/' || c == ':' || c == ']' || c == ' ' || c == '\n' ||
                c == '"') {
                if (ht_acc_) {
                    httphost_ = ht_acc_;
                    ht_done_ = 1;
                }
            } else {
                const int lc = c | 32;
                ht_acc_ = mix64(ht_acc_ * 31ull +
                                static_cast<std::uint64_t>(lc));
            }
        }
#endif

        if (c == '&') state_ = kWkAmp;
        if (c == '\'' && prev1_ == '\'') state_ = kWkQuote;

        // senword: body-prose 2104-hash, only in paragraph, not in link/template.
        const int letter = ((c | 32) >= 'a' && (c | 32) <= 'z');
        if (letter && is_paragraph_ && state_ != kWkSquareOpen &&
            state_ != kWkHtLink && state_ != kWkCurly && !in_table_) {
            senword_ = senword_ * 2104ull + static_cast<std::uint64_t>(c);
        }
        if (c == '.' || c == ',' || c == ':' || c == '(' || c == ')' || c == '\n')
            senword_ = 0;

#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
        if (in_table_) table_push(c);
#endif
#if HP_TBLROW_MOD
        if (in_table_) {
            if (c == '\n') {
                tbl_rowpend_ = 1;
                tbl_bar_ = 0;
            } else if (tbl_rowpend_) {
                if (c == '|') {
                    tbl_bar_ = 1;
                } else if (c == '!') {
                    tbl_kind_ = 3;
                    tbl_rowpend_ = 0;
                    tbl_bar_ = 0;
                } else if (tbl_bar_ && c == '-') {
                    tbl_kind_ = 1;
                    tbl_rowpend_ = 0;
                    tbl_bar_ = 0;
                } else if (tbl_bar_ && c == '+') {
                    tbl_kind_ = 2;
                    tbl_rowpend_ = 0;
                    tbl_bar_ = 0;
                } else if (tbl_bar_) {
                    tbl_kind_ = 4;
                    tbl_rowpend_ = 0;
                    tbl_bar_ = 0;
                } else if (c != ' ' && c != '\t') {
                    tbl_rowpend_ = 0;
                }
            }
        }
#endif
#if HP_CELLTXT_MOD
        if (in_table_) {
            if (c == '|' || c == '!' || c == '\n') {
                if (cell_acc_) celltxt_ = cell_acc_;
                cell_acc_ = 0;
            } else {
                const int lc = c | 32;
                if (lc >= 'a' && lc <= 'z')
                    cell_acc_ = mix64(cell_acc_ * 31ull +
                                      static_cast<std::uint64_t>(lc));
            }
        }
#endif
#if HP_DEFAULTSORT_MOD || HP_DAB_MOD || HP_HATNOTE_MOD
        if (state_ == kWkCurly) {
            const int lc = c | 32;
            if (!tnm_done_) {
                if (lc >= 'a' && lc <= 'z' && tnm_i_ < 16) {
                    tnm_buf_[tnm_i_++] = static_cast<char>(lc);
                } else if (c == ':' || c == '|' || c == '}' || c == ' ' ||
                           c == '\n') {
                    tnm_done_ = 1;
                    tnm_mode_ = 0;
                    auto eq = [&](const char* w, int n) {
                        if (tnm_i_ != n) return 0;
                        for (int i = 0; i < n; ++i)
                            if (tnm_buf_[i] != w[i]) return 0;
                        return 1;
                    };
                    std::uint64_t nh = 0;
                    for (int i = 0; i < tnm_i_; ++i)
                        nh = mix64(nh * 31ull +
                                   static_cast<std::uint64_t>(tnm_buf_[i]));
                    if (eq("defaultsort", 11)) {
                        tnm_mode_ = 1;
#if HP_DEFAULTSORT_MOD
                        ds_hash_ = nh;
#endif
                    } else if (eq("disambig", 8) || eq("hndis", 5) ||
                               eq("dab", 3) || eq("disambiguation", 14)) {
                        tnm_mode_ = 2;
#if HP_DAB_MOD
                        dab_hash_ = nh;
#endif
                    } else if (eq("for", 3) || eq("about", 5) ||
                               eq("main", 4) || eq("further", 7)) {
                        tnm_mode_ = 3;
#if HP_HATNOTE_MOD
                        hat_hash_ = nh;
#endif
                    }
                }
            } else if (tnm_mode_ == 1 && c != '}' && c != '\n') {
#if HP_DEFAULTSORT_MOD
                const int lc2 = c | 32;
                if (lc2 >= 'a' && lc2 <= 'z')
                    ds_hash_ = mix64(ds_hash_ * 31ull +
                                     static_cast<std::uint64_t>(lc2));
#endif
            }
        }
#endif
#if HP_CITEKIND_MOD || HP_COORD_MOD
        if (state_ == kWkCurly && !ck_done_) {
            const int lc = c | 32;
            if (lc >= 'a' && lc <= 'z' && ck_i_ < 16) {
                ck_buf_[ck_i_++] = static_cast<char>(lc);
            } else if (c == ':' || c == '|' || c == '}' || c == ' ' ||
                       c == '\n') {
                ck_done_ = 1;
                auto eq = [&](const char* w, int n) {
                    if (ck_i_ != n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (ck_buf_[i] != w[i]) return 0;
                    return 1;
                };
                auto haspre = [&](const char* w, int n) {
                    if (ck_i_ < n) return 0;
                    for (int i = 0; i < n; ++i)
                        if (ck_buf_[i] != w[i]) return 0;
                    return 1;
                };
#if HP_CITEKIND_MOD
                int ck = 0;
                if (eq("citation", 8)) ck = 5;
                else if (haspre("cite", 4)) {
                    if (eq("citeweb", 7)) ck = 1;
                    else if (eq("citejournal", 11)) ck = 2;
                    else if (eq("citebook", 8)) ck = 3;
                    else if (eq("citenews", 8)) ck = 4;
                    else ck = 6;
                }
                if (ck) citekind_ = ck;
#endif
#if HP_COORD_MOD
                if (eq("coord", 5) || eq("coordinates", 11) ||
                    eq("coordlink", 9))
                    coord_ = 1;
#endif
            }
        }
#endif
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
        if (tpl_collect_) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9'))
                tpl_name_ = mix64(tpl_name_ * 31ull + static_cast<std::uint64_t>(c));
            else if (c != '{')
                tpl_collect_ = 0;
        }
        if (key_collect_) {
            if (c == '=') key_collect_ = 0;
            else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '_')
                key_ = mix64(key_ * 31ull + static_cast<std::uint64_t>(c));
        }
#endif
#if HP_SECTION_MUTE
        push_mute(c);
#endif

        if (c == '\n') {
            ++line_;
            first_of_line_ = 1;
            line_kind_ = 0;
            if (state_ == kWkText) ++para_;
#if HP_REDIR_MOD
            in_redir_ = 0;
#endif
#if HP_REDIR_MOD || HP_REDIRTARGET_MOD
            redir_i_ = 0;
#endif
#if HP_HEADING_MOD
            heading_ = 0;
            heading_run_ = 0;
#endif
#if HP_SECTITLE_MOD
            st_collect_ = 0;
            st_skip_ = 0;
#endif
#if HP_SECKIND_MOD
            if (sk_collect_ || sk_n_ > 0) seckind_apply_();
            sk_skip_ = 0;
            sk_collect_ = 0;
            sk_n_ = 0;
#endif
#if HP_INDENT_MOD
            indent_ = 0;
            indent_run_ = 0;
#endif
#if HP_LISTLEVEL_MOD
            list_level_ = 0;
            list_run_ = 0;
            list_kind_ = 0;
#endif
        } else if (first_of_line_ && c != ' ' && c != '\t') {
            first_of_line_ = 0;
            line_kind_ = c;
            // fx2: isParagraph = (fc == FIRSTUPPER); WIKIHEADER = line-start '>'
            is_paragraph_ = (c >= 'A' && c <= 'Z') ? 1 : 0;
            wiki_header_ = (c == '>') ? 1 : 0;
            if (is_paragraph_ && state_ == kWkText) state_ = kWkFirstUpper;
            if (wiki_header_ && state_ == kWkText) state_ = kWkHeader;
#if HP_REDIR_MOD || HP_REDIRTARGET_MOD
            if (c == '#') redir_i_ = 1;
#endif
#if HP_HEADING_MOD
            if (c == '=') {
                heading_ = 1;
                heading_run_ = 1;
            }
#endif
#if HP_SECTITLE_MOD
            if (c == '=') {
                st_skip_ = 1;
                st_collect_ = 0;
                st_hash_ = 0;
            }
#endif
#if HP_SECKIND_MOD
            if (c == '=') {
                sk_skip_ = 1;
                sk_collect_ = 0;
                sk_n_ = 0;
            }
#endif
#if HP_INDENT_MOD
            if (c == ':') {
                indent_ = 1;
                indent_run_ = 1;
            }
#endif
#if HP_LISTLEVEL_MOD
            if (c == '*' || c == '#') {
                list_level_ = 1;
                list_run_ = 1;
                list_kind_ = c;
            }
#endif
#if HP_LISTPOS_MOD
            if (c == '*' || c == '#') {
                if (prev_line_list_ && list_kind_line_ == c) ++list_pos_;
                else list_pos_ = 1;
                prev_line_list_ = 1;
                list_kind_line_ = c;
            } else {
                prev_line_list_ = 0;
                list_pos_ = 0;
            }
#endif
        } else {
#if HP_REDIR_MOD || HP_REDIRTARGET_MOD
            if (redir_i_ > 0 && redir_i_ < 9
#if HP_REDIR_MOD
                && !in_redir_
#endif
            ) {
                static const char rd[] = "redirect";
                if ((c | 32) == rd[redir_i_ - 1]) {
                    ++redir_i_;
                    if (redir_i_ == 9) {
#if HP_REDIR_MOD
                        in_redir_ = 1;
#endif
#if HP_REDIRTARGET_MOD
                        rtgt_phase_ = 1;
                        rtgt_hash_ = 0;
#endif
                    }
                } else {
                    redir_i_ = 0;
                }
            }
#endif
#if HP_REDIRTARGET_MOD
            if (rtgt_phase_ == 1) {
                if (c == '[') rtgt_phase_ = 2;
            } else if (rtgt_phase_ == 2) {
                if (c == '[') {
                    rtgt_phase_ = 3;
                    rtgt_hash_ = 0;
                } else if (c != ' ' && c != '\t') {
                    rtgt_phase_ = 0;
                }
            } else if (rtgt_phase_ == 3) {
                if (c == ']' || c == '|' || c == '\n') {
                    rtgt_phase_ = 0;
                } else {
                    const int lc = c | 32;
                    rtgt_hash_ = mix64(rtgt_hash_ * 31ull +
                                       static_cast<std::uint64_t>(lc));
                }
            }
#endif
#if HP_HEADING_MOD
            if (heading_run_) {
                if (c == '=') {
                    if (heading_ < 6) ++heading_;
                } else {
                    heading_run_ = 0;
                }
            }
#endif
#if HP_SECTITLE_MOD
            if (st_skip_) {
                if (c != '=') {
                    st_skip_ = 0;
                    st_collect_ = 1;
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z')
                        st_hash_ = mix64(st_hash_ * 31ull +
                                         static_cast<std::uint64_t>(lc));
                }
            } else if (st_collect_) {
                if (c == '=') {
                    st_collect_ = 0;
                } else {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z')
                        st_hash_ = mix64(st_hash_ * 31ull +
                                         static_cast<std::uint64_t>(lc));
                }
            }
#endif
#if HP_SECKIND_MOD
            if (sk_skip_) {
                if (c != '=') {
                    sk_skip_ = 0;
                    sk_collect_ = 1;
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z' && sk_n_ < 16)
                        sk_buf_[sk_n_++] = static_cast<char>(lc);
                }
            } else if (sk_collect_) {
                if (c == '=') {
                    seckind_apply_();
                    sk_collect_ = 0;
                    sk_n_ = 0;
                } else {
                    const int lc = c | 32;
                    if (lc >= 'a' && lc <= 'z' && sk_n_ < 16)
                        sk_buf_[sk_n_++] = static_cast<char>(lc);
                }
            }
#endif
#if HP_INDENT_MOD
            if (indent_run_) {
                if (c == ':') {
                    if (indent_ < 8) ++indent_;
                } else {
                    indent_run_ = 0;
                }
            }
#endif
#if HP_LISTLEVEL_MOD
            if (list_run_) {
                if (c == list_kind_) {
                    if (list_level_ < 8) ++list_level_;
                } else {
                    list_run_ = 0;
                }
            }
#endif
        }
#if HP_EXTLINK_MOD
        if (c == '[' && prev1_ != '[') ext_pend_ = 1;
#endif
#if HP_LISTPARA_MOD
        if (c == '\n') {
            listpara_ = 0;
            lp_apos_ = 0;
        } else {
            if (c == '\'') {
                if (lp_apos_ < 5) ++lp_apos_;
            } else {
                if (lp_apos_ >= 2 && c == ' ' &&
                    line_kind_ != '*' && line_kind_ != '#' &&
                    line_kind_ != ':' && line_kind_ != ';')
                    listpara_ = 1;
                lp_apos_ = 0;
            }
            if ((line_kind_ == '*' || line_kind_ == '#') &&
                prev1_ == ']' && (c == ' ' || c == ','))
                listpara_ = 1;
            if (line_kind_ == '[' && prev1_ == ']' && c == ' ')
                listpara_ = 1;
            if (line_kind_ == '*' && prev1_ == '*' && c == '-')
                listpara_ = 1;
        }
#endif
#if HP_FCCUR_MOD
        if (c == '\n') {
            fccur_ = 0;
            fccur_set_ = 0;
        } else if (!fccur_set_ && c != ' ' && c != '\t') {
            fccur_ = fccur_token_(c);
            fccur_set_ = 1;
        }
        if (prev2_ == ':' && prev1_ == '/' && c == '/')
            fccur_ = 14;
        if (c == ' ' && prev1_ == ']' &&
            (line_kind_ == '*' || line_kind_ == '#' || line_kind_ == '['))
            fccur_ = 1;
        if (c == ' ' && prev1_ == '\'' && prev2_ == '\'')
            fccur_ = 1;
#endif
#if HP_WIKIVAR_MOD
        if (wv_on_) {
            int u = c;
            if (c >= 'a' && c <= 'z') u = c - 32;
            if (u >= 'A' && u <= 'Z' && wv_n_ < 20) {
                wv_buf_[wv_n_++] = static_cast<char>(u);
                if (wv_n_ >= 4) wikivar_ = wiki_var_id_(wv_buf_, wv_n_);
            } else if (c != '{') {
                wv_on_ = 0;
            }
        }
        if (c == '\n') wikivar_ = 0;
#endif
    }

    int state() const { return state_; }
    int depth() const { return depth_; }
    int in_tag() const { return in_tag_; }
    int in_table() const { return in_table_; }
    int is_paragraph() const { return is_paragraph_; }
    int wiki_header() const { return wiki_header_; }
    int line() const { return line_; }
    std::uint64_t tag_name() const { return tag_name_; }
    std::uint64_t linkword() const { return linkword_; }
    std::uint64_t senword() const { return senword_; }
    int line_kind() const { return line_kind_; }
    int sen_group() const {
        if (in_table_ || state_ == kWkWikiTable || state_ == kWkVerticalBar)
            return 2;
        if (state_ == kWkSquareOpen) return 3;
        if (line_kind_ == '*') return 1;
        return 0;
    }
    int nest_markup() const {
        return in_table_ || state_ == kWkSquareOpen || state_ == kWkCurly ||
               state_ == kWkWikiTable || state_ == kWkVerticalBar ||
               state_ == kWkHtLink;
    }
    int mute_words() const {
#if HP_SECTION_MUTE
        return mute_;
#else
        return 0;
#endif
    }

    // Context packed for the existing tag_ ContextModel.
    std::uint64_t context_key() const {
        return (static_cast<std::uint64_t>(state_ & 15) << 48) |
               (static_cast<std::uint64_t>(depth_ & 15) << 44) |
               (static_cast<std::uint64_t>(in_tag_ & 1) << 43) |
               (static_cast<std::uint64_t>(in_table_ & 1) << 42) |
               (static_cast<std::uint64_t>(is_paragraph_ & 1) << 41) |
               (static_cast<std::uint64_t>(wiki_header_ & 1) << 40) |
               ((tag_name_ & 0x1fffffull) << 21) |
               (linkword_ & 0x1fffffull)
#if HP_WIKI_TEMP
               | (static_cast<std::uint64_t>(is_temp_ & 1) << 39)
#endif
#if HP_NLCHAR
               | (static_cast<std::uint64_t>(nl_mode() & 3) << 37)
#endif
               ;
    }
    int sent_domain() const {
        if (in_table_ || state_ == kWkWikiTable || state_ == kWkVerticalBar)
            return 1;
        if (state_ == kWkSquareOpen || state_ == kWkHtLink) return 2;
        if (state_ == kWkCurly) return 3;
        return 0;
    }
    int nl_mode() const {
        if (in_table_ || state_ == kWkWikiTable) return 1;
        if (wiki_header_) return 2;
        return 0;
    }
    int above_cell() const {
#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
        const int r = (tbl_row_ + 3) & 3;
        const int c = tbl_cell_ > 31 ? 31 : tbl_cell_;
        return tbl_cells_[r][c];
#else
        return 0;
#endif
    }
    int cell_first() const {
#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
        const int c = tbl_cell_ > 31 ? 31 : tbl_cell_;
        return tbl_cells_[tbl_row_][c];
#else
        return 0;
#endif
    }
    int tbl_cell() const {
#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
        return tbl_cell_;
#else
        return 0;
#endif
    }
    int state_trans() const {
#if HP_STATETRANS_MOD
        return (prev_state_ & 15) | ((state_ & 15) << 4);
#else
        return 0;
#endif
    }
    std::uint64_t col_ring() const {
#if HP_COLRING_MOD
        const int c = tbl_cell_ > 31 ? 31 : tbl_cell_;
        return static_cast<std::uint64_t>(tbl_cells_[0][c]) |
               (static_cast<std::uint64_t>(tbl_cells_[1][c]) << 8) |
               (static_cast<std::uint64_t>(tbl_cells_[2][c]) << 16) |
               (static_cast<std::uint64_t>(tbl_cells_[3][c]) << 24) |
               (static_cast<std::uint64_t>(tbl_row_ & 3) << 32) |
               (static_cast<std::uint64_t>(c & 31) << 34);
#else
        return 0;
#endif
    }
    int list_para() const {
#if HP_LISTPARA_MOD
        return listpara_;
#else
        return 0;
#endif
    }
    int sec_frag() const {
#if HP_SECFRAG_MOD
        return secfrag_;
#else
        return 0;
#endif
    }
    int wiki_var() const {
#if HP_WIKIVAR_MOD
        return wikivar_;
#else
        return 0;
#endif
    }
    int sub_page() const {
#if HP_SUBPAGE_MOD
        return subpage_;
#else
        return 0;
#endif
    }
    int link_ns() const {
#if HP_LINKNS_MOD
        return linkns_;
#else
        return 0;
#endif
    }
    int fc_cur() const {
#if HP_FCCUR_MOD
        return fccur_;
#else
        return 0;
#endif
    }
    int ref_group() const {
#if HP_REFGROUP_MOD
        return refgroup_;
#else
        return 0;
#endif
    }
    int in_reflist() const {
#if HP_REFLIST_MOD
        return reflist_;
#else
        return 0;
#endif
    }
    int sister() const {
#if HP_SISTER_MOD
        return sister_;
#else
        return 0;
#endif
    }
    int in_convert() const {
#if HP_CONVERT_MOD
        return convert_;
#else
        return 0;
#endif
    }
    int cn_kind() const {
#if HP_CN_MOD
        return cn_;
#else
        return 0;
#endif
    }
    int block_kind() const {
#if HP_BLOCK_MOD
        return block_;
#else
        return 0;
#endif
    }
    int pipe_trick() const {
#if HP_PIPETRICK_MOD
        return pipetrick_;
#else
        return 0;
#endif
    }
    int notes_kind() const {
#if HP_NOTES_MOD
        return notes_;
#else
        return 0;
#endif
    }
    int lang_tpl() const {
#if HP_LANGTPL_MOD
        return langtpl_;
#else
        return 0;
#endif
    }
    int frac_kind() const {
#if HP_FRAC_MOD
        return frac_;
#else
        return 0;
#endif
    }
    int listen_kind() const {
#if HP_LISTEN_MOD
        return listen_;
#else
        return 0;
#endif
    }
    int birth_kind() const {
#if HP_BIRTH_MOD
        return birth_;
#else
        return 0;
#endif
    }
    int hlist_kind() const {
#if HP_HLIST_MOD
        return hlist_;
#else
        return 0;
#endif
    }
    int mainart_kind() const {
#if HP_MAINART_MOD
        return mainart_;
#else
        return 0;
#endif
    }
    int in_chem() const {
#if HP_CHEM_MOD
        return in_chem_;
#else
        return 0;
#endif
    }
    int sfn_kind() const {
#if HP_SFN_MOD
        return sfn_;
#else
        return 0;
#endif
    }
    int in_geotemp() const {
#if HP_GEOTEMP_MOD
        return geotemp_;
#else
        return 0;
#endif
    }
    int epigraph_kind() const {
#if HP_EPIGRAPH_MOD
        return epigraph_;
#else
        return 0;
#endif
    }
    int tracklist_kind() const {
#if HP_TRACKLIST_MOD
        return tracklist_;
#else
        return 0;
#endif
    }
    int succession_kind() const {
#if HP_SUCCESSION_MOD
        return succession_;
#else
        return 0;
#endif
    }
    int colstart_kind() const {
#if HP_COLSTART_MOD
        return colstart_;
#else
        return 0;
#endif
    }
    int toc_mode() const {
#if HP_TOC_MOD
        return toc_;
#else
        return 0;
#endif
    }
    int in_refbegin() const {
#if HP_REFBEGIN_MOD
        return refbegin_;
#else
        return 0;
#endif
    }
    int shortdesc_kind() const {
#if HP_SHORTDESC_MOD
        return shortdesc_;
#else
        return 0;
#endif
    }
    int seealso_kind() const {
#if HP_SEEALSO_MOD
        return seealso_;
#else
        return 0;
#endif
    }
    int portal_kind() const {
#if HP_PORTAL_MOD
        return portal_;
#else
        return 0;
#endif
    }
    int authctl_kind() const {
#if HP_AUTHCTL_MOD
        return authctl_;
#else
        return 0;
#endif
    }
    int usedate_kind() const {
#if HP_USEDATE_MOD
        return usedate_;
#else
        return 0;
#endif
    }
    int ipa_kind() const {
#if HP_IPA_MOD
        return ipa_;
#else
        return 0;
#endif
    }
    int goodart_kind() const {
#if HP_GOODART_MOD
        return goodart_;
#else
        return 0;
#endif
    }
    int caption_kind() const {
#if HP_CAPTION_MOD
        return caption_;
#else
        return 0;
#endif
    }
    int navbox_kind() const {
#if HP_NAVBOX_MOD
        return navbox_;
#else
        return 0;
#endif
    }
    int efoot_kind() const {
#if HP_EFOOT_MOD
        return efoot_;
#else
        return 0;
#endif
    }
    int rshort_kind() const {
#if HP_RSHORT_MOD
        return rshort_;
#else
        return 0;
#endif
    }
    int asof_kind() const {
#if HP_ASOF_MOD
        return asof_;
#else
        return 0;
#endif
    }
    int clarify_kind() const {
#if HP_CLARIFY_MOD
        return clarify_;
#else
        return 0;
#endif
    }
    int currency_kind() const {
#if HP_CURRENCY_MOD
        return currency_;
#else
        return 0;
#endif
    }
    int displaytitle_kind() const {
#if HP_DISPLAYTITLE_MOD
        return displaytitle_;
#else
        return 0;
#endif
    }
    int nowrap_kind() const {
#if HP_NOWRAP_MOD
        return nowrap_;
#else
        return 0;
#endif
    }
    int stub_kind() const {
#if HP_STUB_MOD
        return stub_;
#else
        return 0;
#endif
    }
    int persondata_kind() const {
#if HP_PERSONDATA_MOD
        return persondata_;
#else
        return 0;
#endif
    }
    int flag_kind() const {
#if HP_FLAG_MOD
        return flag_;
#else
        return 0;
#endif
    }
    int quotebox_kind() const {
#if HP_QUOTEBOX_MOD
        return quotebox_;
#else
        return 0;
#endif
    }
    int clear_kind() const {
#if HP_CLEAR_MOD
        return clear_;
#else
        return 0;
#endif
    }
    int imdb_kind() const {
#if HP_IMDB_MOD
        return imdb_;
#else
        return 0;
#endif
    }
    int rp_kind() const {
#if HP_RP_MOD
        return rp_;
#else
        return 0;
#endif
    }
    int fn_kind() const {
#if HP_FN_MOD
        return fn_;
#else
        return 0;
#endif
    }
    int small_kind() const {
#if HP_SMALL_MOD
        return in_small_;
#else
        return 0;
#endif
    }
    int supsub_kind() const {
#if HP_SUPSUB_MOD
        return supsub_;
#else
        return 0;
#endif
    }
    int precode_kind() const {
#if HP_PRECODE_MOD
        return precode_;
#else
        return 0;
#endif
    }
    int taxobox_kind() const {
#if HP_TAXOBOX_MOD
        return taxobox_;
#else
        return 0;
#endif
    }
    int nihongo_kind() const {
#if HP_NIHONGO_MOD
        return nihongo_;
#else
        return 0;
#endif
    }
    int deadlink_kind() const {
#if HP_DEADLINK_MOD
        return deadlink_;
#else
        return 0;
#endif
    }
    int wayback_kind() const {
#if HP_WAYBACK_MOD
        return wayback_;
#else
        return 0;
#endif
    }
    int rowspan_kind() const {
#if HP_ROWSPAN_MOD
        return rowspan_;
#else
        return 0;
#endif
    }
    int unref_kind() const {
#if HP_UNREF_MOD
        return unref_;
#else
        return 0;
#endif
    }
    int cleanup_kind() const {
#if HP_CLEANUP_MOD
        return cleanup_;
#else
        return 0;
#endif
    }
    int npov_kind() const {
#if HP_NPOV_MOD
        return npov_;
#else
        return 0;
#endif
    }
    int rfrom_kind() const {
#if HP_RFROM_MOD
        return rfrom_;
#else
        return 0;
#endif
    }
    int doi_kind() const {
#if HP_DOI_MOD
        return doi_;
#else
        return 0;
#endif
    }
    int pmid_kind() const {
#if HP_PMID_MOD
        return pmid_;
#else
        return 0;
#endif
    }
    int isbn_kind() const {
#if HP_ISBN_MOD
        return isbn_;
#else
        return 0;
#endif
    }
    int medal_kind() const {
#if HP_MEDAL_MOD
        return medal_;
#else
        return 0;
#endif
    }
    int thumb_kind() const {
#if HP_THUMB_MOD
        return thumb_;
#else
        return 0;
#endif
    }
    int further_kind() const {
#if HP_FURTHER_MOD
        return further_;
#else
        return 0;
#endif
    }
    int death_kind() const {
#if HP_DEATH_MOD
        return death_;
#else
        return 0;
#endif
    }
    int harv_kind() const {
#if HP_HARV_MOD
        return harv_;
#else
        return 0;
#endif
    }
    int issn_kind() const {
#if HP_ISSN_MOD
        return issn_;
#else
        return 0;
#endif
    }
    int oclc_kind() const {
#if HP_OCLC_MOD
        return oclc_;
#else
        return 0;
#endif
    }
    int align_kind() const {
#if HP_ALIGN_MOD
        return align_;
#else
        return 0;
#endif
    }
    int syntax_kind() const {
#if HP_SYNTAX_MOD
        return syntax_;
#else
        return 0;
#endif
    }
    std::uint64_t tpl_name() const {
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
        return tpl_name_;
#else
        return 0;
#endif
    }
    std::uint64_t infokey() const {
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
        return key_;
#else
        return 0;
#endif
    }
    int bar_idx() const {
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
        return bar_idx_;
#else
        return 0;
#endif
    }
    int in_ref() const {
#if HP_CITE_MOD
        return in_ref_;
#else
        return 0;
#endif
    }
    int after_pipe() const {
#if HP_LINKPIPE_MOD
        return link_pipe_;
#else
        return 0;
#endif
    }
    std::uint64_t link_disp() const {
#if HP_LINKPIPE_MOD
        return disp_;
#else
        return 0;
#endif
    }
    std::uint64_t cat_ns() const {
#if HP_CAT_MOD
        return ns_hash_;
#else
        return 0;
#endif
    }
    int in_redir() const {
#if HP_REDIR_MOD
        return in_redir_;
#else
        return 0;
#endif
    }
    int heading_level() const {
#if HP_HEADING_MOD
        return heading_;
#else
        return 0;
#endif
    }
    int in_ext() const {
#if HP_EXTLINK_MOD
        return in_ext_;
#else
        return 0;
#endif
    }
    std::uint64_t refname() const {
#if HP_REFNAME_MOD
        return refname_;
#else
        return 0;
#endif
    }
    std::uint64_t entity() const {
#if HP_ENTITY_MOD
        return entity_;
#else
        return 0;
#endif
    }
    int indent_level() const {
#if HP_INDENT_MOD
        return indent_;
#else
        return 0;
#endif
    }
    int list_level() const {
#if HP_LISTLEVEL_MOD
        return list_level_;
#else
        return 0;
#endif
    }
    std::uint64_t magic() const {
#if HP_MAGIC_MOD
        return magic_hash_;
#else
        return 0;
#endif
    }
    int in_nowiki() const {
#if HP_NOWIKI_MOD
        return nw_bits_;
#else
        return 0;
#endif
    }
    std::uint64_t page_title() const {
#if HP_TITLE_MOD
        return title_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t page_id() const {
#if HP_PAGEID_MOD
        return page_id_;
#else
        return 0;
#endif
    }
    std::uint64_t username() const {
#if HP_USER_MOD
        return user_hash_;
#else
        return 0;
#endif
    }
    int in_text() const {
#if HP_TEXT_MOD
        return in_text_;
#else
        return 0;
#endif
    }
    int ns_id() const {
#if HP_NS_MOD
        return ns_id_;
#else
        return 0;
#endif
    }
    int dump_redir() const {
#if HP_DUMPREDIR_MOD
        return dump_redir_;
#else
        return 0;
#endif
    }
    std::uint64_t ip_hash() const {
#if HP_IP_MOD
        return ip_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t rev_comment() const {
#if HP_REVCOMMENT_MOD
        return comment_hash_;
#else
        return 0;
#endif
    }
    int minor_edit() const {
#if HP_MINOR_MOD
        return minor_;
#else
        return 0;
#endif
    }
    std::uint64_t wiki_model() const {
#if HP_WIKIMODEL_MOD
        return model_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t sectitle() const {
#if HP_SECTITLE_MOD
        return st_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t parser_fn() const {
#if HP_PARSERFN_MOD
        return pfn_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t table_class() const {
#if HP_TABLECLASS_MOD
        return tc_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t anchor() const {
#if HP_ANCHOR_MOD
        return an_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t pub_id() const {
#if HP_PUBID_MOD
        return pub_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t temp_pos() const {
#if HP_TEMPPOS_MOD
        return tp_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t wiki_stack() const {
#if HP_WIKISTACK_MOD
        return (static_cast<std::uint64_t>(fc_stk_[0])) |
               (static_cast<std::uint64_t>(fc_stk_[1]) << 8) |
               (static_cast<std::uint64_t>(fc_stk_[2]) << 16) |
               (static_cast<std::uint64_t>(fc_stk_[3]) << 24) |
               (static_cast<std::uint64_t>(br_stk_[0] & 15) << 32) |
               (static_cast<std::uint64_t>(br_stk_[1] & 15) << 36) |
               (static_cast<std::uint64_t>(br_stk_[2] & 15) << 40) |
               (static_cast<std::uint64_t>(above_cell() & 255) << 44) |
               (static_cast<std::uint64_t>(tbl_cell() & 31) << 52) |
               (static_cast<std::uint64_t>(nl_mode() & 3) << 57) |
               (static_cast<std::uint64_t>(is_paragraph_ & 1) << 59);
#else
        return 0;
#endif
    }
    std::uint64_t lang_prefix() const {
#if HP_LANG_MOD
        return lang_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t cat_sort() const {
#if HP_CATSORT_MOD
        return extra_hash_;
#else
        return 0;
#endif
    }
    int tbl_row() const {
#if HP_TBLROW_MOD
        return tbl_kind_;
#else
        return 0;
#endif
    }
    std::uint64_t file_opt() const {
#if HP_FILEOPT_MOD
        return extra_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t default_sort() const {
#if HP_DEFAULTSORT_MOD
        return ds_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t redir_target() const {
#if HP_REDIRTARGET_MOD
        return rtgt_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t dab() const {
#if HP_DAB_MOD
        return dab_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t hatnote() const {
#if HP_HATNOTE_MOD
        return hat_hash_;
#else
        return 0;
#endif
    }
    std::uint64_t last_link() const {
#if HP_LASTLINK_MOD
        return lastlink_;
#else
        return 0;
#endif
    }
    std::uint64_t first_word() const {
#if HP_FWORD_MOD
        return fword_;
#else
        return 0;
#endif
    }
    std::uint64_t year() const {
#if HP_YEAR_MOD
        return year_;
#else
        return 0;
#endif
    }
    int cap_mask() const {
#if HP_CAPMASK_MOD
        return cap_mask_;
#else
        return 0;
#endif
    }
    std::uint64_t cell_text() const {
#if HP_CELLTXT_MOD
        return celltxt_;
#else
        return 0;
#endif
    }
    std::uint64_t http_host() const {
#if HP_HTTPHOST_MOD
        return httphost_;
#else
        return 0;
#endif
    }
    std::uint64_t last_paren() const {
#if HP_PAREN_MOD
        return lastparen_;
#else
        return 0;
#endif
    }
    int list_pos() const {
#if HP_LISTPOS_MOD
        return list_pos_;
#else
        return 0;
#endif
    }
    std::uint64_t word_shape() const {
#if HP_SHAPE_MOD
        return shape_;
#else
        return 0;
#endif
    }
    std::uint64_t word_suffix() const {
#if HP_SUFFIX_MOD
        return suffix_;
#else
        return 0;
#endif
    }
    std::uint64_t word_prefix() const {
#if HP_PREFIX_MOD
        return prefix_;
#else
        return 0;
#endif
    }
    int char_cls() const {
#if HP_CHARCLS_MOD
        return charcls_;
#else
        return 0;
#endif
    }
    int vowel_mask() const {
#if HP_VOWEL_MOD
        return vowel_;
#else
        return 0;
#endif
    }
    std::uint64_t contraction() const {
#if HP_CONTR_MOD
        return contr_;
#else
        return 0;
#endif
    }
    std::uint64_t hyphen_word() const {
#if HP_HYPHEN_MOD
        return hyphen_;
#else
        return 0;
#endif
    }
    int token_cls() const {
#if HP_TOKENCLS_MOD
        return tok_cls_;
#else
        return 0;
#endif
    }
    int run_len() const {
#if HP_RUNLEN_MOD
        return runlen_;
#else
        return 0;
#endif
    }
    int word_pos() const {
#if HP_WPOS_MOD
        return wpos_;
#else
        return 0;
#endif
    }
    int blank_n() const {
#if HP_BLANK_MOD
        return blank_n_;
#else
        return 0;
#endif
    }
    int space_run() const {
#if HP_SPRUN_MOD
        return sprun_;
#else
        return 0;
#endif
    }
    int line_len() const {
#if HP_LINELEN_MOD
        return linelen_;
#else
        return 0;
#endif
    }
    int tag_dist() const {
#if HP_TAGDIST_MOD
        return tagdist_;
#else
        return 0;
#endif
    }
    int mark_dist() const {
#if HP_MARKDIST_MOD
        return markdist_;
#else
        return 0;
#endif
    }
    int upper_gap() const {
#if HP_UPPERGAP_MOD
        return uppergap_;
#else
        return 0;
#endif
    }
    int month() const {
#if HP_MONTH_MOD
        return month_;
#else
        return 0;
#endif
    }
    int in_gallery() const {
#if HP_GALLERY_MOD
        return in_gallery_;
#else
        return 0;
#endif
    }
    int sec_kind() const {
#if HP_SECKIND_MOD
        return seckind_;
#else
        return 0;
#endif
    }
    int cite_kind() const {
#if HP_CITEKIND_MOD
        return citekind_;
#else
        return 0;
#endif
    }
    std::uint64_t tag_name_cm() const {
#if HP_TAGNAME_MOD
        return tag_name_;
#else
        return 0;
#endif
    }
    int col_span() const {
#if HP_COLSPAN_MOD
        return colspan_;
#else
        return 0;
#endif
    }
    std::uint64_t style_hash() const {
#if HP_STYLE_MOD
        return style_hash_;
#else
        return 0;
#endif
    }
    int in_coord() const {
#if HP_COORD_MOD
        return coord_;
#else
        return 0;
#endif
    }
    int digit_gap() const {
#if HP_DIGITGAP_MOD
        return digitgap_;
#else
        return 0;
#endif
    }
    int dot_gap() const {
#if HP_DOTGAP_MOD
        return dotgap_;
#else
        return 0;
#endif
    }
    int comma_gap() const {
#if HP_COMMAGAP_MOD
        return commagap_;
#else
        return 0;
#endif
    }
    int word_len() const {
#if HP_WORDLEN_MOD
        return wordlen_;
#else
        return 0;
#endif
    }
    int sent_len() const {
#if HP_SENTLEN_MOD
        return sentlen_;
#else
        return 0;
#endif
    }
    int lower_gap() const {
#if HP_LOWERGAP_MOD
        return lowergap_;
#else
        return 0;
#endif
    }
    int digit_pos() const {
#if HP_DIGITPOS_MOD
        return digitpos_;
#else
        return 0;
#endif
    }
    int slash_gap() const {
#if HP_SLASHGAP_MOD
        return slashgap_;
#else
        return 0;
#endif
    }
    int dig_len() const {
#if HP_DIGLEN_MOD
        return diglen_;
#else
        return 0;
#endif
    }
    int prev_line() const {
#if HP_PREVLINE_MOD
        return prevline_;
#else
        return 0;
#endif
    }
    int prev_sent() const {
#if HP_PREVSENT_MOD
        return prevsent_;
#else
        return 0;
#endif
    }
    int link_len() const {
#if HP_LINKLEN_MOD
        return linklen_;
#else
        return 0;
#endif
    }
    int tpl_len() const {
#if HP_TPLLEN_MOD
        return tpllen_;
#else
        return 0;
#endif
    }
    int para_len() const {
#if HP_PARALEN_MOD
        return paralen_;
#else
        return 0;
#endif
    }
    int alnum_len() const {
#if HP_ALNUMLEN_MOD
        return alnumlen_;
#else
        return 0;
#endif
    }
    int sp_len() const {
#if HP_SPLEN_MOD
        return splen_;
#else
        return 0;
#endif
    }
    int title_word() const {
#if HP_TITLEWORD_MOD
        return titleword_;
#else
        return 0;
#endif
    }
    int head_word() const {
#if HP_HEADWORD_MOD
        return headword_;
#else
        return 0;
#endif
    }
    int in_init() const {
#if HP_INIT_MOD
        return init_;
#else
        return 0;
#endif
    }
    int ordinal() const {
#if HP_ORDINAL_MOD
        return ordinal_;
#else
        return 0;
#endif
    }
    int unit() const {
#if HP_UNIT_MOD
        return unit_;
#else
        return 0;
#endif
    }
    int in_decimal() const {
#if HP_DECIMAL_MOD
        return decimal_;
#else
        return 0;
#endif
    }
    int word_repeat() const {
#if HP_REPEAT_MOD
        return repeat_;
#else
        return 0;
#endif
    }
    int case_flip() const {
#if HP_CASEFLIP_MOD
        return caseflip_;
#else
        return 0;
#endif
    }
    int in_lead() const {
#if HP_LEAD_MOD
        return lead_;
#else
        return 0;
#endif
    }
    int info_val() const {
#if HP_INFOVAL_MOD
        return infoval_;
#else
        return 0;
#endif
    }
    int link_trail() const {
#if HP_LINKTRAIL_MOD
        return linktrail_;
#else
        return 0;
#endif
    }
    int cell_kind() const {
#if HP_CELLKIND_MOD
        return cellkind_;
#else
        return 0;
#endif
    }
    int tbl_col() const {
#if HP_TBLCOL_MOD
        return tblcol_;
#else
        return 0;
#endif
    }
    int head_idx() const {
#if HP_HEADIDX_MOD
        return headidx_;
#else
        return 0;
#endif
    }
    int html_fmt() const {
#if HP_HTMLFMT_MOD
        return htmlfmt_;
#else
        return 0;
#endif
    }
    int in_infobox() const {
#if HP_INFOBOX_MOD
        return infobox_;
#else
        return 0;
#endif
    }
    int sec_level() const {
#if HP_SECLEVEL_MOD
        return seclevel_;
#else
        return 0;
#endif
    }
    int brace3() const {
#if HP_BRACE3_MOD
        return brace3_;
#else
        return 0;
#endif
    }
    int named_arg() const {
#if HP_NAMEDARG_MOD
        return namedarg_;
#else
        return 0;
#endif
    }
    int include_bits() const {
#if HP_INCLUDE_MOD
        return include_;
#else
        return 0;
#endif
    }
    int sig_run() const {
#if HP_SIG_MOD
        return sig_;
#else
        return 0;
#endif
    }
    int wiki_bold() const {
#if HP_WIKIBOLD_MOD
        return wikibold_;
#else
        return 0;
#endif
    }
    int url_part() const {
#if HP_URLPART_MOD
        return urlpart_;
#else
        return 0;
#endif
    }
    int ref_idx() const {
#if HP_REFIDX_MOD
        return refidx_;
#else
        return 0;
#endif
    }
    int prespace() const {
#if HP_PRESPACE_MOD
        return prespace_;
#else
        return 0;
#endif
    }
    int ext_disp() const {
#if HP_EXTDISP_MOD
        return extdisp_;
#else
        return 0;
#endif
    }
    int px_size() const {
#if HP_PXSIZE_MOD
        return pxsize_;
#else
        return 0;
#endif
    }
    int ent_num() const {
#if HP_ENTNUM_MOD
        return entnum_;
#else
        return 0;
#endif
    }
    int wiki_hr() const {
#if HP_WIKIHR_MOD
        return wikihr_;
#else
        return 0;
#endif
    }
    int font_col() const {
#if HP_FONTCOL_MOD
        return fontcol_;
#else
        return 0;
#endif
    }
    int tbl_depth() const {
#if HP_TBLDEPTH_MOD
        return tbldepth_;
#else
        return 0;
#endif
    }
    int utf8_st() const {
#if HP_UTF8ST_MOD
        return utf8st_;
#else
        return 0;
#endif
    }
    int dl_term() const {
#if HP_DLTERM_MOD
        return dlterm_;
#else
        return 0;
#endif
    }
    int head_close() const {
#if HP_HEADCLOSE_MOD
        return headclose_;
#else
        return 0;
#endif
    }
    int wiki_time() const {
#if HP_WIKITIME_MOD
        return wikitime_;
#else
        return 0;
#endif
    }
    int link_comma() const {
#if HP_LINKCOMMA_MOD
        return linkcomma_;
#else
        return 0;
#endif
    }
    int cat_block() const {
#if HP_CATBLOCK_MOD
        return catblock_;
#else
        return 0;
#endif
    }
    int br_tag() const {
#if HP_BR_MOD
        return br_;
#else
        return 0;
#endif
    }
    int amp_nbsp() const {
#if HP_AMPNBSP_MOD
        return ampnbsp_;
#else
        return 0;
#endif
    }
    int mdash() const {
#if HP_MDASH_MOD
        return mdash_;
#else
        return 0;
#endif
    }
    int in_math() const {
#if HP_MATH_MOD
        return in_math_;
#else
        return 0;
#endif
    }
    int list_mix() const {
#if HP_LISTMIX_MOD
        return listmix_;
#else
        return 0;
#endif
    }
    int protocol() const {
#if HP_PROTOCOL_MOD
        return protocol_;
#else
        return 0;
#endif
    }
    int hex_run() const {
#if HP_HEXRUN_MOD
        return hexval_;
#else
        return 0;
#endif
    }
    int sq_depth() const {
#if HP_SQDEPTH_MOD
        return sqdepth_;
#else
        return 0;
#endif
    }
    int pipe_role() const {
#if HP_PIPEROLE_MOD
        return piperole_;
#else
        return 0;
#endif
    }
    int after_ref() const {
#if HP_AFTERREF_MOD
        return afterref_;
#else
        return 0;
#endif
    }
    int sent_pos() const {
#if HP_SENTPOS_MOD
        return sentpos_;
#else
        return 0;
#endif
    }
    int abbrev() const {
#if HP_ABBREV_MOD
        return abbrev_;
#else
        return 0;
#endif
    }
    int thousand() const {
#if HP_THOUSAND_MOD
        return thousand_;
#else
        return 0;
#endif
    }
    int ref_punct() const {
#if HP_REFPUNCT_MOD
        return refpunct_;
#else
        return 0;
#endif
    }
    int q_period() const {
#if HP_QPERIOD_MOD
        return qperiod_;
#else
        return 0;
#endif
    }
    int ellipsis() const {
#if HP_ELLIPSIS_MOD
        return ellipsis_;
#else
        return 0;
#endif
    }
    int num_range() const {
#if HP_NUMRANGE_MOD
        return numrange_;
#else
        return 0;
#endif
    }
    int deg() const {
#if HP_DEG_MOD
        return deg_;
#else
        return 0;
#endif
    }
    int percent() const {
#if HP_PERCENT_MOD
        return percent_;
#else
        return 0;
#endif
    }
    int is_temp() const {
#if HP_WIKI_TEMP
        return is_temp_;
#else
        return 0;
#endif
    }

 private:
    int state_ = kWkText;
    int depth_ = 0;
    int in_tag_ = 0;
    int in_table_ = 0;
    int sq_ = 0;
    int http_run_ = 0;
    int last_ = 0, prev1_ = 0, prev2_ = 0;
    int line_ = 0;
    int para_ = 0;
    int first_of_line_ = 1;
    int line_kind_ = 0;
    int is_paragraph_ = 0;
    int wiki_header_ = 0;
    std::uint64_t tag_name_ = 0;
    std::uint64_t linkword_ = 0;
    std::uint64_t senword_ = 0;
#if HP_WIKI_TEMP
    int is_temp_ = 0;
#endif
#if HP_TPLNAME_MOD || HP_INFOKEY_MOD || HP_BARIDX_MOD
    std::uint64_t tpl_name_ = 0;
    std::uint64_t key_ = 0;
    int bar_idx_ = 0;
    int tpl_collect_ = 0;
    int key_collect_ = 0;
#endif
#if HP_CITE_MOD
    int in_ref_ = 0;
    int cite_slash_ = 0;
    std::uint32_t cite_buf_ = 0;
#endif
#if HP_LINKPIPE_MOD
    int link_pipe_ = 0;
    std::uint64_t disp_ = 0;
#endif
#if HP_CAT_MOD
    int ns_collect_ = 0;
    std::uint64_t ns_hash_ = 0;
#endif
#if HP_REDIR_MOD
    int in_redir_ = 0;
#endif
#if HP_REDIR_MOD || HP_REDIRTARGET_MOD
    int redir_i_ = 0;
#endif
#if HP_REDIRTARGET_MOD
    int rtgt_phase_ = 0;
    std::uint64_t rtgt_hash_ = 0;
#endif
#if HP_LANG_MOD
    int lang_n_ = 0;
    std::uint64_t lang_acc_ = 0;
    std::uint64_t lang_hash_ = 0;
#endif
#if HP_CATSORT_MOD || HP_FILEOPT_MOD
    char pre_buf_[12] = {};
    int pre_i_ = 0;
    int pre_kind_ = 0;
    int pipe_col_ = 0;
    std::uint64_t extra_hash_ = 0;
#endif
#if HP_TBLROW_MOD
    int tbl_kind_ = 0;
    int tbl_rowpend_ = 0;
    int tbl_bar_ = 0;
#endif
#if HP_DEFAULTSORT_MOD || HP_DAB_MOD || HP_HATNOTE_MOD
    char tnm_buf_[16] = {};
    int tnm_i_ = 0;
    int tnm_done_ = 1;
    int tnm_mode_ = 0;
#endif
#if HP_DEFAULTSORT_MOD
    std::uint64_t ds_hash_ = 0;
#endif
#if HP_DAB_MOD
    std::uint64_t dab_hash_ = 0;
#endif
#if HP_HATNOTE_MOD
    std::uint64_t hat_hash_ = 0;
#endif
#if HP_LASTLINK_MOD
    std::uint64_t lastlink_ = 0;
    std::uint64_t ll_acc_ = 0;
    int ll_stop_ = 0;
#endif
#if HP_FWORD_MOD
    std::uint64_t fword_ = 0;
    std::uint64_t fw_acc_ = 0;
    int fw_pending_ = 1;
    int fw_n_ = 0;
#endif
#if HP_YEAR_MOD
    std::uint64_t year_ = 0;
    int yr_n_ = 0;
    int yr_val_ = 0;
#endif
#if HP_CAPMASK_MOD
    int cap_mask_ = 0;
    int cap_bits_ = 0;
    int cap_n_ = 0;
#endif
#if HP_CELLTXT_MOD
    std::uint64_t celltxt_ = 0;
    std::uint64_t cell_acc_ = 0;
#endif
#if HP_HTTPHOST_MOD
    std::uint64_t httphost_ = 0;
    std::uint64_t ht_acc_ = 0;
    int ht_done_ = 1;
#endif
#if HP_PAREN_MOD
    std::uint64_t lastparen_ = 0;
    std::uint64_t paren_acc_ = 0;
    int paren_d_ = 0;
#endif
#if HP_LISTPOS_MOD
    int list_pos_ = 0;
    int prev_line_list_ = 0;
    int list_kind_line_ = 0;
#endif
#if HP_SHAPE_MOD
    std::uint64_t shape_ = 0;
    int shape_on_ = 0;
#endif
#if HP_SUFFIX_MOD
    std::uint64_t suffix_ = 0;
    std::uint64_t suffix_acc_ = 0;
    int suf_n_ = 0;
#endif
#if HP_PREFIX_MOD
    std::uint64_t prefix_ = 0;
    int pre_n_ = 0;
#endif
#if HP_CHARCLS_MOD
    int charcls_ = 0;
#endif
#if HP_VOWEL_MOD
    int vowel_ = 0;
#endif
#if HP_CONTR_MOD
    std::uint64_t contr_ = 0;
    std::uint64_t contr_acc_ = 0;
    int contr_apos_ = 0;
#endif
#if HP_HYPHEN_MOD
    std::uint64_t hyphen_ = 0;
    std::uint64_t hy_acc_ = 0;
    int hy_seen_ = 0;
#endif
#if HP_TOKENCLS_MOD
    int tok_cls_ = 0;
#endif
#if HP_RUNLEN_MOD
    int runlen_ = 0;
#endif
#if HP_WPOS_MOD
    int wpos_ = 0;
#endif
#if HP_BLANK_MOD
    int blank_n_ = 0;
#endif
#if HP_SPRUN_MOD
    int sprun_ = 0;
#endif
#if HP_LINELEN_MOD
    int linelen_ = 0;
#endif
#if HP_TAGDIST_MOD
    int tagdist_ = 0;
#endif
#if HP_MARKDIST_MOD
    int markdist_ = 0;
#endif
#if HP_UPPERGAP_MOD
    int uppergap_ = 0;
#endif
#if HP_MONTH_MOD
    int month_ = 0;
    int mo_n_ = 0;
    char mo_buf_[9] = {};
#endif
#if HP_GALLERY_MOD
    int in_gallery_ = 0;
    int gal_slash_ = 0;
    int gal_n_ = 0;
    char gal_buf_[8] = {};
#endif
#if HP_SECKIND_MOD
    void seckind_apply_() {
        auto eq = [&](const char* w, int n) {
            if (sk_n_ != n) return 0;
            for (int i = 0; i < n; ++i)
                if (sk_buf_[i] != w[i]) return 0;
            return 1;
        };
        int k = 7;
        if (sk_n_ == 0) k = 0;
        else if (eq("references", 10) || eq("refs", 4) || eq("notes", 5) ||
                 eq("footnotes", 9))
            k = 1;
        else if (eq("seealso", 7) || eq("see", 3))
            k = 2;
        else if (eq("externallinks", 13) || eq("external", 8))
            k = 3;
        else if (eq("bibliography", 12) || eq("sources", 7) ||
                 eq("furtherreading", 15))
            k = 4;
        else if (eq("history", 7) || eq("career", 6) || eq("biography", 9) ||
                 eq("life", 4))
            k = 5;
        seckind_ = k;
    }
    int seckind_ = 0;
    int sk_skip_ = 0;
    int sk_collect_ = 0;
    int sk_n_ = 0;
    char sk_buf_[16] = {};
#endif
#if HP_CITEKIND_MOD
    int citekind_ = 0;
#endif
#if HP_CITEKIND_MOD || HP_COORD_MOD
    int ck_i_ = 0;
    int ck_done_ = 0;
    char ck_buf_[16] = {};
#endif
#if HP_COLSPAN_MOD
    int colspan_ = 0;
    int cs_eat_ = 0;
    int cs_val_ = 0;
    std::uint64_t cs_win_ = 0;
#endif
#if HP_ROWSPAN_MOD
    int rowspan_ = 0;
    std::uint64_t rs_win_ = 0;
#endif
#if HP_STYLE_MOD
    std::uint64_t style_hash_ = 0;
    int style_eat_ = 0;
    std::uint64_t st_win_ = 0;
#endif
#if HP_COORD_MOD
    int coord_ = 0;
#endif
#if HP_DIGITGAP_MOD
    int digitgap_ = 0;
#endif
#if HP_DOTGAP_MOD
    int dotgap_ = 0;
#endif
#if HP_COMMAGAP_MOD
    int commagap_ = 0;
#endif
#if HP_WORDLEN_MOD
    int wordlen_ = 0;
    int wl_run_ = 0;
#endif
#if HP_SENTLEN_MOD
    int sentlen_ = 0;
#endif
#if HP_LOWERGAP_MOD
    int lowergap_ = 0;
#endif
#if HP_DIGITPOS_MOD
    int digitpos_ = 0;
#endif
#if HP_SLASHGAP_MOD
    int slashgap_ = 0;
#endif
#if HP_DIGLEN_MOD
    int diglen_ = 0;
    int dl_run_ = 0;
#endif
#if HP_PREVLINE_MOD
    int prevline_ = 0;
    int ll_run_ = 0;
#endif
#if HP_PREVSENT_MOD
    int prevsent_ = 0;
    int ss_run_ = 0;
#endif
#if HP_LINKLEN_MOD
    int linklen_ = 0;
    int lk_run_ = 0;
#endif
#if HP_TPLLEN_MOD
    int tpllen_ = 0;
    int tp_run_ = 0;
#endif
#if HP_PARALEN_MOD
    int paralen_ = 0;
    int pr_run_ = 0;
#endif
#if HP_ALNUMLEN_MOD
    int alnumlen_ = 0;
    int al_run_ = 0;
#endif
#if HP_SPLEN_MOD
    int splen_ = 0;
    int sp_run_ = 0;
#endif
#if HP_TITLEWORD_MOD || HP_HEADWORD_MOD || HP_REPEAT_MOD
    std::uint64_t cw_acc_ = 0;
    int cw_on_ = 0;
#endif
#if HP_TITLEWORD_MOD
    std::uint64_t tw_ring_[8] = {};
    int tw_n_ = 0;
    int titleword_ = 0;
#endif
#if HP_HEADWORD_MOD
    std::uint64_t hw_ring_[8] = {};
    int hw_n_ = 0;
    int headword_ = 0;
#endif
#if HP_INIT_MOD
    int init_ = 0;
    int init_st_ = 0;
#endif
#if HP_ORDINAL_MOD
    int ordinal_ = 0;
    int ord_pend_ = 0;
    int ord_n_ = 0;
    char ord_buf_[2] = {};
#endif
#if HP_UNIT_MOD
    int unit_ = 0;
    int un_pend_ = 0;
    int un_n_ = 0;
    char un_buf_[4] = {};
#endif
#if HP_DECIMAL_MOD
    int decimal_ = 0;
    int dec_saw_ = 0;
    int dec_pend_ = 0;
#endif
#if HP_REPEAT_MOD
    std::uint64_t cw_prev_ = 0;
    int repeat_ = 0;
#endif
#if HP_CASEFLIP_MOD
    int caseflip_ = 0;
#endif
#if HP_LEAD_MOD
    int lead_ = 1;
#endif
#if HP_INFOVAL_MOD
    int infoval_ = 0;
    int iv_tpl_ = 0;
    int iv_set_ = 0;
#endif
#if HP_LINKTRAIL_MOD
    int linktrail_ = 0;
    int lt_on_ = 0;
#endif
#if HP_CELLKIND_MOD
    int cellkind_ = 0;
    int ck_sol_ = 0;
    int ck_bar_ = 0;
#endif
#if HP_TBLCOL_MOD
    int tblcol_ = 0;
#endif
#if HP_HEADIDX_MOD
    int headidx_ = 0;
    int hi_on_ = 0;
#endif
#if HP_HTMLFMT_MOD
    int htmlfmt_ = 0;
    int hf_n_ = 0;
    int hf_slash_ = 0;
    char hf_buf_[8] = {};
#endif
#if HP_INFOBOX_MOD
    int infobox_ = 0;
    int ib_depth_ = 0;
    int ib_n_ = 0;
    int ib_col_ = 0;
    char ib_buf_[8] = {};
#endif
#if HP_SECLEVEL_MOD
    int seclevel_ = 0;
    int sl_run_ = 0;
    int sl_at_ = 0;
#endif
#if HP_BRACE3_MOD
    int brace3_ = 0;
    int b3_run_ = 0;
    int b3_cls_ = 0;
#endif
#if HP_NAMEDARG_MOD
    int namedarg_ = 0;
#endif
#if HP_INCLUDE_MOD
    int include_ = 0;
    int ic_n_ = 0;
    int ic_slash_ = 0;
    char ic_buf_[12] = {};
#endif
#if HP_SIG_MOD
    int sig_ = 0;
#endif
#if HP_WIKIBOLD_MOD
    int wikibold_ = 0;
    int wb_run_ = 0;
#endif
#if HP_URLPART_MOD
    int urlpart_ = 0;
#endif
#if HP_REFIDX_MOD
    int refidx_ = 0;
    int ri_n_ = 0;
    int ri_slash_ = 0;
    char ri_buf_[4] = {};
#endif
#if HP_PRESPACE_MOD
    int prespace_ = 0;
#endif
#if HP_EXTDISP_MOD
    int extdisp_ = 0;
    int ed_pend_ = 0;
    int ed_on_ = 0;
    int ed_slash_ = 0;
#endif
#if HP_PXSIZE_MOD
    int pxsize_ = 0;
    int px_n_ = 0;
    int px_p_ = 0;
    int px_acc_ = 0;
#endif
#if HP_ENTNUM_MOD
    int entnum_ = 0;
    int en_st_ = 0;
#endif
#if HP_WIKIHR_MOD
    int wikihr_ = 0;
    int hr_run_ = 0;
#endif
#if HP_FONTCOL_MOD
    int fontcol_ = 0;
    int fc_eat_ = 0;
    std::uint64_t fc_win_ = 0;
#endif
#if HP_TBLDEPTH_MOD
    int tbldepth_ = 0;
#endif
#if HP_UTF8ST_MOD
    int utf8st_ = 0;
    int utf8left_ = 0;
#endif
#if HP_DLTERM_MOD
    int dlterm_ = 0;
    int dl_on_ = 0;
#endif
#if HP_HEADCLOSE_MOD
    int headclose_ = 0;
    int hc_sol_ = 1;
    int hc_eq_ = 0;
    int hc_text_ = 0;
#endif
#if HP_WIKITIME_MOD
    int wikitime_ = 0;
    int tm_st_ = 0;
    int tm_n1_ = 0;
    int tm_n2_ = 0;
#endif
#if HP_LINKCOMMA_MOD
    int linkcomma_ = 0;
    int lc_pipe_ = 0;
#endif
#if HP_CATBLOCK_MOD
    int catblock_ = 0;
    int cb_n_ = 0;
    int cb_hit_ = 0;
#endif
#if HP_BR_MOD
    int br_ = 0;
    std::uint64_t br_win_ = 0;
#endif
#if HP_AMPNBSP_MOD
    int ampnbsp_ = 0;
    std::uint64_t ampnbsp_win_ = 0;
#endif
#if HP_MDASH_MOD
    int mdash_ = 0;
    int md_st_ = 0;
#endif
#if HP_LISTMIX_MOD
    int listmix_ = 0;
    int lm_run_ = 1;
#endif
#if HP_MATH_MOD
    int in_math_ = 0;
    int mh_open_ = 0;
    std::uint64_t mh_win_ = 0;
#endif
#if HP_CHEM_MOD
    int in_chem_ = 0;
    int ch_open_ = 0;
    std::uint64_t ch_win_ = 0;
#endif
#if HP_SMALL_MOD
    int in_small_ = 0;
    int sm_open_ = 0;
    std::uint64_t sm_win_ = 0;
#endif
#if HP_SUPSUB_MOD
    int supsub_ = 0;
    int ss_open_ = 0;
    std::uint64_t ss_win_ = 0;
#endif
#if HP_PRECODE_MOD
    int precode_ = 0;
    int pc_open_ = 0;
    std::uint64_t pc_win_ = 0;
#endif
#if HP_PROTOCOL_MOD
    int protocol_ = 0;
    int proto_n_ = 0;
    char proto_buf_[8] = {};
#endif
#if HP_HEXRUN_MOD
    int hexval_ = 0;
    int hexrun_ = 0;
    int hex_on_ = 0;
#endif
#if HP_SQDEPTH_MOD
    int sqdepth_ = 0;
#endif
#if HP_PIPEROLE_MOD
    int piperole_ = 0;
#endif
#if HP_AFTERREF_MOD
    int afterref_ = 0;
    std::uint64_t ar_win_ = 0;
#endif
#if HP_SENTPOS_MOD
    int sentpos_ = 0;
    int sp_inword_ = 0;
#endif
#if HP_ABBREV_MOD
    int abbrev_ = 0;
    int ab_n_ = 0;
    std::uint64_t ab_w_ = 0;
#endif
#if HP_THOUSAND_MOD
    int thousand_ = 0;
    int th_st_ = 0;
    int th_n_ = 0;
#endif
#if HP_REFPUNCT_MOD
    int refpunct_ = 0;
    std::uint64_t rp_win_ = 0;
#endif
#if HP_QPERIOD_MOD
    int qperiod_ = 0;
#endif
#if HP_ELLIPSIS_MOD
    int ellipsis_ = 0;
    int ellip_n_ = 0;
#endif
#if HP_NUMRANGE_MOD
    int numrange_ = 0;
    int nr_st_ = 0;
    int nr_utf_ = 0;
#endif
#if HP_DEG_MOD
    int deg_ = 0;
    int deg_d_ = 0;
    int deg_st_ = 0;
    std::uint64_t deg_win_ = 0;
#endif
#if HP_PERCENT_MOD
    int percent_ = 0;
    int pct_d_ = 0;
#endif
#if HP_STATETRANS_MOD
    int prev_state_ = 0;
#endif
#if HP_LISTPARA_MOD
    int listpara_ = 0;
    int lp_apos_ = 0;
#endif
#if HP_SECFRAG_MOD
    int secfrag_ = 0;
#endif
#if HP_WIKIVAR_MOD
    int wikivar_ = 0;
    int wv_on_ = 0;
    int wv_n_ = 0;
    char wv_buf_[20] = {};
#endif
#if HP_SUBPAGE_MOD
    int subpage_ = 0;
#endif
#if HP_LINKNS_MOD
    int linkns_ = 0;
    int ln_on_ = 0;
    int ln_n_ = 0;
    char ln_buf_[16] = {};
#endif
#if HP_FCCUR_MOD
    int fccur_ = 0;
    int fccur_set_ = 0;
#endif
#if HP_REFGROUP_MOD
    int refgroup_ = 0;
    int rg_slash_ = 0;
    int rg_n_ = 0;
    int rg_name_ = 0;
    int rg_group_ = 0;
    std::uint64_t rg_win_ = 0;
    char rg_buf_[4] = {};
#endif
#if HP_REFLIST_MOD
    int reflist_ = 0;
    int rl_from_tpl_ = 0;
    int rl_d_ = 0;
    int rl_slash_ = 0;
    int rl_n_ = 0;
    char rl_buf_[12] = {};
#endif
#if HP_SISTER_MOD
    int sister_ = 0;
    int sis_on_ = 0;
    int sis_n_ = 0;
    char sis_buf_[12] = {};
#endif
#if HP_CONVERT_MOD
    int convert_ = 0;
    int convert_d_ = 0;
#endif
#if HP_CN_MOD
    int cn_ = 0;
    int cn_d_ = 0;
#endif
#if HP_CONVERT_MOD || HP_CN_MOD || HP_REFLIST_MOD
    int h39t_depth_ = 0;
    int h39t_n_ = 0;
    int h39t_col_ = 0;
    char h39t_buf_[16] = {};
#endif
#if HP_BLOCK_MOD
    int block_ = 0;
    int bk_slash_ = 0;
    int bk_n_ = 0;
    char bk_buf_[12] = {};
#endif
#if HP_PIPETRICK_MOD
    int pipetrick_ = 0;
    int pt_pipe_ = 0;
#endif
#if HP_NOTES_MOD
    int notes_ = 0;
    int notes_d_ = 0;
#endif
#if HP_LANGTPL_MOD
    int langtpl_ = 0;
    int langtpl_d_ = 0;
#endif
#if HP_FRAC_MOD
    int frac_ = 0;
    int frac_d_ = 0;
#endif
#if HP_LISTEN_MOD
    int listen_ = 0;
    int listen_d_ = 0;
    int ls_pre_n_ = 0;
    int ls_pre_on_ = 0;
    int ls_in_file_ = 0;
    std::uint64_t ls_win_ = 0;
    char ls_pre_[8] = {};
#endif
#if HP_BIRTH_MOD
    int birth_ = 0;
    int birth_d_ = 0;
#endif
#if HP_HLIST_MOD
    int hlist_ = 0;
    int hlist_d_ = 0;
#endif
#if HP_MAINART_MOD
    int mainart_ = 0;
    int mainart_d_ = 0;
    int ma_seen_head_ = 0;
#endif
#if HP_NOTES_MOD || HP_LANGTPL_MOD || HP_FRAC_MOD || HP_LISTEN_MOD || \
    HP_BIRTH_MOD || HP_HLIST_MOD || HP_MAINART_MOD
    int h40t_depth_ = 0;
    int h40t_n_ = 0;
    int h40t_col_ = 0;
    char h40t_buf_[16] = {};
#endif
#if HP_SFN_MOD
    int sfn_ = 0;
    int sfn_d_ = 0;
#endif
#if HP_GEOTEMP_MOD
    int geotemp_ = 0;
    int geotemp_d_ = 0;
#endif
#if HP_EPIGRAPH_MOD
    int epigraph_ = 0;
    int epigraph_d_ = 0;
#endif
#if HP_TRACKLIST_MOD
    int tracklist_ = 0;
    int tracklist_d_ = 0;
#endif
#if HP_SUCCESSION_MOD
    int succession_ = 0;
    int succession_d_ = 0;
#endif
#if HP_COLSTART_MOD
    int colstart_ = 0;
    int colstart_d_ = 0;
#endif
#if HP_REFBEGIN_MOD
    int refbegin_ = 0;
#endif
#if HP_SFN_MOD || HP_GEOTEMP_MOD || HP_EPIGRAPH_MOD || HP_TRACKLIST_MOD || \
    HP_SUCCESSION_MOD || HP_COLSTART_MOD || HP_REFBEGIN_MOD
    int h41t_depth_ = 0;
    int h41t_n_ = 0;
    int h41t_col_ = 0;
    char h41t_buf_[16] = {};
#endif
#if HP_TOC_MOD
    int toc_ = 0;
    int toc_col_ = 0;
    int toc_n_ = 0;
    char toc_buf_[12] = {};
#endif
#if HP_SHORTDESC_MOD
    int shortdesc_ = 0;
    int shortdesc_d_ = 0;
#endif
#if HP_SEEALSO_MOD
    int seealso_ = 0;
    int seealso_d_ = 0;
#endif
#if HP_PORTAL_MOD
    int portal_ = 0;
    int portal_d_ = 0;
#endif
#if HP_AUTHCTL_MOD
    int authctl_ = 0;
    int authctl_d_ = 0;
#endif
#if HP_USEDATE_MOD
    int usedate_ = 0;
#endif
#if HP_IPA_MOD
    int ipa_ = 0;
    int ipa_d_ = 0;
#endif
#if HP_GOODART_MOD
    int goodart_ = 0;
#endif
#if HP_CAPTION_MOD
    int caption_ = 0;
    int caption_d_ = 0;
    int cap_key_on_ = 0;
    int cap_key_n_ = 0;
    int cap_sq_ = 0;
    char cap_key_[16] = {};
#endif
#if HP_SHORTDESC_MOD || HP_SEEALSO_MOD || HP_PORTAL_MOD || HP_AUTHCTL_MOD || \
    HP_USEDATE_MOD || HP_IPA_MOD || HP_GOODART_MOD || HP_CAPTION_MOD
    int h42t_depth_ = 0;
    int h42t_n_ = 0;
    int h42t_col_ = 0;
    char h42t_buf_[20] = {};
#endif
#if HP_NAVBOX_MOD
    int navbox_ = 0;
    int navbox_d_ = 0;
#endif
#if HP_EFOOT_MOD
    int efoot_ = 0;
    int efoot_d_ = 0;
#endif
#if HP_RSHORT_MOD
    int rshort_ = 0;
    int rshort_d_ = 0;
#endif
#if HP_ASOF_MOD
    int asof_ = 0;
    int asof_d_ = 0;
#endif
#if HP_CLARIFY_MOD
    int clarify_ = 0;
    int clarify_d_ = 0;
#endif
#if HP_CURRENCY_MOD
    int currency_ = 0;
    int currency_d_ = 0;
#endif
#if HP_DISPLAYTITLE_MOD
    int displaytitle_ = 0;
#endif
#if HP_NOWRAP_MOD
    int nowrap_ = 0;
    int nowrap_d_ = 0;
#endif
#if HP_STUB_MOD
    int stub_ = 0;
#endif
#if HP_PERSONDATA_MOD
    int persondata_ = 0;
    int persondata_d_ = 0;
#endif
#if HP_FLAG_MOD
    int flag_ = 0;
    int flag_d_ = 0;
#endif
#if HP_QUOTEBOX_MOD
    int quotebox_ = 0;
    int quotebox_d_ = 0;
#endif
#if HP_CLEAR_MOD
    int clear_ = 0;
    int h43t_dash_ = 0;
#endif
#if HP_IMDB_MOD
    int imdb_ = 0;
    int imdb_d_ = 0;
#endif
#if HP_RP_MOD
    int rp_ = 0;
    int rp_d_ = 0;
#endif
#if HP_FN_MOD
    int fn_ = 0;
    int fn_d_ = 0;
#endif
#if HP_TAXOBOX_MOD
    int taxobox_ = 0;
    int taxobox_d_ = 0;
#endif
#if HP_NIHONGO_MOD
    int nihongo_ = 0;
    int nihongo_d_ = 0;
#endif
#if HP_DEADLINK_MOD
    int deadlink_ = 0;
    int deadlink_d_ = 0;
#endif
#if HP_WAYBACK_MOD
    int wayback_ = 0;
    int wayback_d_ = 0;
#endif
#if HP_UNREF_MOD
    int unref_ = 0;
    int unref_d_ = 0;
#endif
#if HP_CLEANUP_MOD
    int cleanup_ = 0;
    int cleanup_d_ = 0;
#endif
#if HP_NPOV_MOD
    int npov_ = 0;
    int npov_d_ = 0;
#endif
#if HP_RFROM_MOD
    int rfrom_ = 0;
    int rfrom_d_ = 0;
#endif
#if HP_DOI_MOD
    int doi_ = 0;
    int doi_d_ = 0;
    std::uint64_t doi_win_ = 0;
#endif
#if HP_PMID_MOD
    int pmid_ = 0;
    int pmid_d_ = 0;
    std::uint64_t pmid_win_ = 0;
#endif
#if HP_ISBN_MOD
    int isbn_ = 0;
    std::uint64_t isbn_win_ = 0;
#endif
#if HP_MEDAL_MOD
    int medal_ = 0;
    int medal_d_ = 0;
#endif
#if HP_THUMB_MOD
    int thumb_ = 0;
    std::uint64_t thumb_win_ = 0;
#endif
#if HP_FURTHER_MOD
    int further_ = 0;
    int further_d_ = 0;
#endif
#if HP_DEATH_MOD
    int death_ = 0;
    int death_d_ = 0;
#endif
#if HP_HARV_MOD
    int harv_ = 0;
    int harv_d_ = 0;
#endif
#if HP_ISSN_MOD
    int issn_ = 0;
    std::uint64_t issn_win_ = 0;
#endif
#if HP_OCLC_MOD
    int oclc_ = 0;
    std::uint64_t oclc_win_ = 0;
#endif
#if HP_ALIGN_MOD
    int align_ = 0;
    std::uint64_t align_win_ = 0;
#endif
#if HP_SYNTAX_MOD
    int syntax_ = 0;
    int sx_open_ = 0;
    std::uint64_t sx_win_ = 0;
#endif
#if HP_NAVBOX_MOD || HP_EFOOT_MOD || HP_RSHORT_MOD || HP_ASOF_MOD || \
    HP_CLARIFY_MOD || HP_CURRENCY_MOD || HP_DISPLAYTITLE_MOD || HP_NOWRAP_MOD || \
    HP_STUB_MOD || HP_PERSONDATA_MOD || HP_FLAG_MOD || HP_QUOTEBOX_MOD || \
    HP_CLEAR_MOD || HP_IMDB_MOD || HP_RP_MOD || HP_FN_MOD || \
    HP_TAXOBOX_MOD || HP_NIHONGO_MOD || HP_DEADLINK_MOD || HP_WAYBACK_MOD || \
    HP_UNREF_MOD || HP_CLEANUP_MOD || HP_NPOV_MOD || HP_RFROM_MOD || \
    HP_DOI_MOD || HP_PMID_MOD || HP_MEDAL_MOD || \
    HP_FURTHER_MOD || HP_DEATH_MOD || HP_HARV_MOD
    int h43t_depth_ = 0;
    int h43t_n_ = 0;
    int h43t_col_ = 0;
    char h43t_buf_[20] = {};
#endif
#if HP_HEADING_MOD
    int heading_ = 0;
    int heading_run_ = 0;
#endif
#if HP_EXTLINK_MOD
    int in_ext_ = 0;
    int ext_pend_ = 0;
#endif
#if HP_REFNAME_MOD
    std::uint64_t refname_ = 0;
    std::uint64_t refn_win_ = 0;
    int refn_collect_ = 0;
    int refn_skipq_ = 0;
#endif
#if HP_ENTITY_MOD
    std::uint64_t entity_ = 0;
    int ent_collect_ = 0;
#endif
#if HP_INDENT_MOD
    int indent_ = 0;
    int indent_run_ = 0;
#endif
#if HP_LISTLEVEL_MOD
    int list_level_ = 0;
    int list_run_ = 0;
    int list_kind_ = 0;
#endif
#if HP_MAGIC_MOD
    int magic_collect_ = 0;
    std::uint64_t magic_hash_ = 0;
#endif
#if HP_NOWIKI_MOD
    void nowiki_apply_() {
        int bit = 0;
        auto eq = [&](const char* s) {
            std::uint64_t w = 0;
            int m = 0;
            while (s[m]) {
                w = w * 31ull + static_cast<std::uint64_t>(
                                    static_cast<unsigned char>(s[m]));
                ++m;
            }
            return m == nw_n_ && w == nw_name_;
        };
        if (eq("nowiki")) bit = 1;
        else if (eq("math")) bit = 2;
        else if (eq("pre")) bit = 4;
        else if (eq("code")) bit = 8;
        if (bit) {
            if (nw_slash_) nw_bits_ &= ~bit;
            else nw_bits_ |= bit;
        }
    }
    int nw_slash_ = 0;
    int nw_n_ = 0;
    int nw_bits_ = 0;
    std::uint64_t nw_name_ = 0;
#endif
#if HP_DUMP_XML
    void dump_apply_() {
        auto eq = [&](const char* s) {
            std::uint64_t w = 0;
            int m = 0;
            while (s[m]) {
                w = w * 31ull + static_cast<std::uint64_t>(
                                    static_cast<unsigned char>(s[m]));
                ++m;
            }
            return m == xml_n_ && w == xml_name_;
        };
#if HP_TITLE_MOD
        if (eq("title")) {
            in_title_ = xml_slash_ ? 0 : 1;
            if (in_title_) title_hash_ = 0;
        }
#endif
#if HP_TITLEWORD_MOD
        if (eq("title") && !xml_slash_) {
            tw_n_ = 0;
            titleword_ = 0;
            cw_acc_ = 0;
            cw_on_ = 0;
        }
#endif
#if HP_USER_MOD
        if (eq("username")) {
            in_user_ = xml_slash_ ? 0 : 1;
            if (in_user_) user_hash_ = 0;
        }
#endif
#if HP_TEXT_MOD
        if (eq("text")) in_text_ = xml_slash_ ? 0 : 1;
#endif
#if HP_PAGEID_MOD
        if (eq("page") && !xml_slash_) {
            pageid_seen_ = 0;
            page_id_ = 0;
        }
        if (eq("id") && !xml_slash_ && !pageid_seen_) in_id_ = 1;
#endif
#if HP_NS_MOD
        if (eq("ns") && !xml_slash_) {
            in_ns_ = 1;
            ns_id_ = 0;
        }
#endif
#if HP_DUMPREDIR_MOD
        if (eq("page") && !xml_slash_) dump_redir_ = 0;
        if (eq("redirect")) dump_redir_ = 1;
#endif
#if HP_IP_MOD
        if (eq("ip")) {
            in_ip_ = xml_slash_ ? 0 : 1;
            if (in_ip_) ip_hash_ = 0;
        }
#endif
#if HP_REVCOMMENT_MOD
        if (eq("comment")) {
            in_comment_ = xml_slash_ ? 0 : 1;
            if (in_comment_) comment_hash_ = 0;
        }
#endif
#if HP_MINOR_MOD
        if (eq("page") && !xml_slash_) minor_ = 0;
        if (eq("minor")) minor_ = 1;
#endif
#if HP_WIKIMODEL_MOD
        if (eq("model")) {
            in_model_ = xml_slash_ ? 0 : 1;
            if (in_model_) model_hash_ = 0;
        }
#endif
#if HP_LASTLINK_MOD || HP_FWORD_MOD || HP_YEAR_MOD || HP_PAREN_MOD || \
    HP_LISTPOS_MOD || HP_HTTPHOST_MOD || HP_CELLTXT_MOD || HP_CAPMASK_MOD || \
    HP_SHAPE_MOD || HP_SUFFIX_MOD || HP_PREFIX_MOD || HP_CHARCLS_MOD || \
    HP_VOWEL_MOD || HP_CONTR_MOD || HP_HYPHEN_MOD || HP_TOKENCLS_MOD || \
    HP_RUNLEN_MOD || HP_WPOS_MOD || HP_BLANK_MOD || HP_SPRUN_MOD || \
    HP_LINELEN_MOD || HP_TAGDIST_MOD || HP_MARKDIST_MOD || HP_UPPERGAP_MOD || \
    HP_MONTH_MOD || HP_GALLERY_MOD || HP_SECKIND_MOD || HP_CITEKIND_MOD || \
    HP_TAGNAME_MOD || HP_COLSPAN_MOD || HP_STYLE_MOD || HP_COORD_MOD || \
    HP_DIGITGAP_MOD || HP_DOTGAP_MOD || HP_COMMAGAP_MOD || HP_WORDLEN_MOD || \
    HP_SENTLEN_MOD || HP_LOWERGAP_MOD || HP_DIGITPOS_MOD || HP_SLASHGAP_MOD || \
    HP_DIGLEN_MOD || HP_PREVLINE_MOD || HP_PREVSENT_MOD || HP_LINKLEN_MOD || \
    HP_TPLLEN_MOD || HP_PARALEN_MOD || HP_ALNUMLEN_MOD || HP_SPLEN_MOD || \
    HP_TITLEWORD_MOD || HP_HEADWORD_MOD || HP_INIT_MOD || HP_ORDINAL_MOD || \
    HP_UNIT_MOD || HP_DECIMAL_MOD || HP_REPEAT_MOD || HP_CASEFLIP_MOD || \
    HP_LEAD_MOD || HP_INFOVAL_MOD || HP_LINKTRAIL_MOD || HP_CELLKIND_MOD || \
    HP_TBLCOL_MOD || HP_HEADIDX_MOD || HP_HTMLFMT_MOD || HP_INFOBOX_MOD || \
    HP_SECLEVEL_MOD || HP_BRACE3_MOD || HP_NAMEDARG_MOD || HP_INCLUDE_MOD || \
    HP_SIG_MOD || HP_WIKIBOLD_MOD || HP_URLPART_MOD || HP_REFIDX_MOD || \
    HP_PRESPACE_MOD || HP_EXTDISP_MOD || HP_PXSIZE_MOD || HP_ENTNUM_MOD || \
    HP_WIKIHR_MOD || HP_FONTCOL_MOD || HP_TBLDEPTH_MOD || HP_UTF8ST_MOD || \
    HP_DLTERM_MOD || HP_HEADCLOSE_MOD || HP_WIKITIME_MOD || HP_LINKCOMMA_MOD || \
    HP_CATBLOCK_MOD || HP_BR_MOD || HP_AMPNBSP_MOD || HP_MDASH_MOD || \
    HP_MATH_MOD || HP_LISTMIX_MOD || HP_PROTOCOL_MOD || HP_HEXRUN_MOD || \
    HP_SQDEPTH_MOD || HP_PIPEROLE_MOD || HP_AFTERREF_MOD || HP_SENTPOS_MOD || \
    HP_ABBREV_MOD || HP_THOUSAND_MOD || HP_REFPUNCT_MOD || HP_QPERIOD_MOD || \
    HP_ELLIPSIS_MOD || HP_NUMRANGE_MOD || HP_DEG_MOD || HP_PERCENT_MOD || \
    HP_STATETRANS_MOD || HP_COLRING_MOD || HP_LISTPARA_MOD || HP_SECFRAG_MOD || \
    HP_WIKIVAR_MOD || HP_SUBPAGE_MOD || HP_LINKNS_MOD || HP_FCCUR_MOD || \
    HP_REFGROUP_MOD || HP_REFLIST_MOD || HP_SISTER_MOD || HP_CONVERT_MOD || \
    HP_CN_MOD || HP_BLOCK_MOD || HP_PIPETRICK_MOD || HP_NOTES_MOD || \
    HP_LANGTPL_MOD || HP_FRAC_MOD || HP_LISTEN_MOD || HP_BIRTH_MOD || \
    HP_HLIST_MOD || HP_MAINART_MOD || HP_CHEM_MOD || HP_SFN_MOD || \
    HP_GEOTEMP_MOD || HP_EPIGRAPH_MOD || HP_TRACKLIST_MOD || \
    HP_SUCCESSION_MOD || HP_COLSTART_MOD || HP_TOC_MOD || HP_REFBEGIN_MOD || \
    HP_SHORTDESC_MOD || HP_SEEALSO_MOD || HP_PORTAL_MOD || HP_AUTHCTL_MOD || \
    HP_USEDATE_MOD || HP_IPA_MOD || HP_GOODART_MOD || HP_CAPTION_MOD || \
    HP_NAVBOX_MOD || HP_EFOOT_MOD || HP_RSHORT_MOD || HP_ASOF_MOD || \
    HP_CLARIFY_MOD || HP_CURRENCY_MOD || HP_DISPLAYTITLE_MOD || HP_NOWRAP_MOD || \
    HP_STUB_MOD || HP_PERSONDATA_MOD || HP_FLAG_MOD || HP_QUOTEBOX_MOD || \
    HP_CLEAR_MOD || HP_IMDB_MOD || HP_RP_MOD || HP_FN_MOD || \
    HP_SMALL_MOD || HP_SUPSUB_MOD || HP_PRECODE_MOD || HP_TAXOBOX_MOD || \
    HP_NIHONGO_MOD || HP_DEADLINK_MOD || HP_WAYBACK_MOD || HP_ROWSPAN_MOD || \
    HP_UNREF_MOD || HP_CLEANUP_MOD || HP_NPOV_MOD || HP_RFROM_MOD || \
    HP_DOI_MOD || HP_PMID_MOD || HP_ISBN_MOD || HP_MEDAL_MOD || \
    HP_THUMB_MOD || HP_FURTHER_MOD || HP_DEATH_MOD || HP_HARV_MOD || \
    HP_ISSN_MOD || HP_OCLC_MOD || HP_ALIGN_MOD || HP_SYNTAX_MOD
        if (eq("page") && !xml_slash_) {
#if HP_LASTLINK_MOD
            lastlink_ = 0;
            ll_acc_ = 0;
#endif
#if HP_FWORD_MOD
            fword_ = 0;
            fw_acc_ = 0;
            fw_pending_ = 1;
            fw_n_ = 0;
#endif
#if HP_YEAR_MOD
            year_ = 0;
            yr_n_ = 0;
            yr_val_ = 0;
#endif
#if HP_CAPMASK_MOD
            cap_mask_ = 0;
            cap_bits_ = 0;
            cap_n_ = 0;
#endif
#if HP_CELLTXT_MOD
            celltxt_ = 0;
            cell_acc_ = 0;
#endif
#if HP_HTTPHOST_MOD
            httphost_ = 0;
            ht_acc_ = 0;
            ht_done_ = 1;
#endif
#if HP_PAREN_MOD
            lastparen_ = 0;
            paren_acc_ = 0;
            paren_d_ = 0;
#endif
#if HP_LISTPOS_MOD
            list_pos_ = 0;
            prev_line_list_ = 0;
            list_kind_line_ = 0;
#endif
#if HP_SHAPE_MOD
            shape_ = 0;
            shape_on_ = 0;
#endif
#if HP_SUFFIX_MOD
            suffix_ = 0;
            suffix_acc_ = 0;
            suf_n_ = 0;
#endif
#if HP_PREFIX_MOD
            prefix_ = 0;
            pre_n_ = 0;
#endif
#if HP_CHARCLS_MOD
            charcls_ = 0;
#endif
#if HP_VOWEL_MOD
            vowel_ = 0;
#endif
#if HP_CONTR_MOD
            contr_ = 0;
            contr_acc_ = 0;
            contr_apos_ = 0;
#endif
#if HP_HYPHEN_MOD
            hyphen_ = 0;
            hy_acc_ = 0;
            hy_seen_ = 0;
#endif
#if HP_TOKENCLS_MOD
            tok_cls_ = 0;
#endif
#if HP_RUNLEN_MOD
            runlen_ = 0;
#endif
#if HP_WPOS_MOD
            wpos_ = 0;
#endif
#if HP_BLANK_MOD
            blank_n_ = 0;
#endif
#if HP_SPRUN_MOD
            sprun_ = 0;
#endif
#if HP_LINELEN_MOD
            linelen_ = 0;
#endif
#if HP_TAGDIST_MOD
            tagdist_ = 0;
#endif
#if HP_MARKDIST_MOD
            markdist_ = 0;
#endif
#if HP_UPPERGAP_MOD
            uppergap_ = 0;
#endif
#if HP_MONTH_MOD
            month_ = 0;
            mo_n_ = 0;
#endif
#if HP_GALLERY_MOD
            in_gallery_ = 0;
            gal_slash_ = 0;
            gal_n_ = 0;
#endif
#if HP_SECKIND_MOD
            seckind_ = 0;
            sk_skip_ = 0;
            sk_collect_ = 0;
            sk_n_ = 0;
#endif
#if HP_CITEKIND_MOD
            citekind_ = 0;
#endif
#if HP_CITEKIND_MOD || HP_COORD_MOD
            ck_i_ = 0;
            ck_done_ = 0;
#endif
#if HP_COLSPAN_MOD
            colspan_ = 0;
            cs_eat_ = 0;
            cs_val_ = 0;
            cs_win_ = 0;
#endif
#if HP_ROWSPAN_MOD
            rowspan_ = 0;
            rs_win_ = 0;
#endif
#if HP_STYLE_MOD
            style_hash_ = 0;
            style_eat_ = 0;
            st_win_ = 0;
#endif
#if HP_COORD_MOD
            coord_ = 0;
#endif
#if HP_DIGITGAP_MOD
            digitgap_ = 0;
#endif
#if HP_DOTGAP_MOD
            dotgap_ = 0;
#endif
#if HP_COMMAGAP_MOD
            commagap_ = 0;
#endif
#if HP_WORDLEN_MOD
            wordlen_ = 0;
            wl_run_ = 0;
#endif
#if HP_SENTLEN_MOD
            sentlen_ = 0;
#endif
#if HP_LOWERGAP_MOD
            lowergap_ = 0;
#endif
#if HP_DIGITPOS_MOD
            digitpos_ = 0;
#endif
#if HP_SLASHGAP_MOD
            slashgap_ = 0;
#endif
#if HP_DIGLEN_MOD
            diglen_ = 0;
            dl_run_ = 0;
#endif
#if HP_PREVLINE_MOD
            prevline_ = 0;
            ll_run_ = 0;
#endif
#if HP_PREVSENT_MOD
            prevsent_ = 0;
            ss_run_ = 0;
#endif
#if HP_LINKLEN_MOD
            linklen_ = 0;
            lk_run_ = 0;
#endif
#if HP_TPLLEN_MOD
            tpllen_ = 0;
            tp_run_ = 0;
#endif
#if HP_PARALEN_MOD
            paralen_ = 0;
            pr_run_ = 0;
#endif
#if HP_ALNUMLEN_MOD
            alnumlen_ = 0;
            al_run_ = 0;
#endif
#if HP_SPLEN_MOD
            splen_ = 0;
            sp_run_ = 0;
#endif
#if HP_TITLEWORD_MOD || HP_HEADWORD_MOD || HP_REPEAT_MOD
            cw_acc_ = 0;
            cw_on_ = 0;
#endif
#if HP_TITLEWORD_MOD
            tw_n_ = 0;
            titleword_ = 0;
#endif
#if HP_HEADWORD_MOD
            hw_n_ = 0;
            headword_ = 0;
#endif
#if HP_INIT_MOD
            init_ = 0;
            init_st_ = 0;
#endif
#if HP_ORDINAL_MOD
            ordinal_ = 0;
            ord_pend_ = 0;
            ord_n_ = 0;
#endif
#if HP_UNIT_MOD
            unit_ = 0;
            un_pend_ = 0;
            un_n_ = 0;
#endif
#if HP_DECIMAL_MOD
            decimal_ = 0;
            dec_saw_ = 0;
            dec_pend_ = 0;
#endif
#if HP_REPEAT_MOD
            cw_prev_ = 0;
            repeat_ = 0;
#endif
#if HP_CASEFLIP_MOD
            caseflip_ = 0;
#endif
#if HP_LEAD_MOD
            lead_ = 1;
#endif
#if HP_INFOVAL_MOD
            infoval_ = 0;
            iv_tpl_ = 0;
            iv_set_ = 0;
#endif
#if HP_LINKTRAIL_MOD
            linktrail_ = 0;
            lt_on_ = 0;
#endif
#if HP_CELLKIND_MOD
            cellkind_ = 0;
            ck_sol_ = 0;
            ck_bar_ = 0;
#endif
#if HP_TBLCOL_MOD
            tblcol_ = 0;
#endif
#if HP_HEADIDX_MOD
            headidx_ = 0;
            hi_on_ = 0;
#endif
#if HP_HTMLFMT_MOD
            htmlfmt_ = 0;
            hf_n_ = 0;
            hf_slash_ = 0;
#endif
#if HP_INFOBOX_MOD
            infobox_ = 0;
            ib_depth_ = 0;
            ib_n_ = 0;
            ib_col_ = 0;
#endif
#if HP_SECLEVEL_MOD
            seclevel_ = 0;
            sl_run_ = 0;
            sl_at_ = 0;
#endif
#if HP_BRACE3_MOD
            brace3_ = 0;
            b3_run_ = 0;
            b3_cls_ = 0;
#endif
#if HP_NAMEDARG_MOD
            namedarg_ = 0;
#endif
#if HP_INCLUDE_MOD
            include_ = 0;
            ic_n_ = 0;
            ic_slash_ = 0;
#endif
#if HP_SIG_MOD
            sig_ = 0;
#endif
#if HP_WIKIBOLD_MOD
            wikibold_ = 0;
            wb_run_ = 0;
#endif
#if HP_URLPART_MOD
            urlpart_ = 0;
#endif
#if HP_REFIDX_MOD
            refidx_ = 0;
            ri_n_ = 0;
            ri_slash_ = 0;
#endif
#if HP_PRESPACE_MOD
            prespace_ = 0;
#endif
#if HP_EXTDISP_MOD
            extdisp_ = 0;
            ed_pend_ = 0;
            ed_on_ = 0;
            ed_slash_ = 0;
#endif
#if HP_PXSIZE_MOD
            pxsize_ = 0;
            px_n_ = 0;
            px_p_ = 0;
            px_acc_ = 0;
#endif
#if HP_ENTNUM_MOD
            entnum_ = 0;
            en_st_ = 0;
#endif
#if HP_WIKIHR_MOD
            wikihr_ = 0;
            hr_run_ = 0;
#endif
#if HP_FONTCOL_MOD
            fontcol_ = 0;
            fc_eat_ = 0;
            fc_win_ = 0;
#endif
#if HP_TBLDEPTH_MOD
            tbldepth_ = 0;
#endif
#if HP_UTF8ST_MOD
            utf8st_ = 0;
            utf8left_ = 0;
#endif
#if HP_DLTERM_MOD
            dlterm_ = 0;
            dl_on_ = 0;
#endif
#if HP_HEADCLOSE_MOD
            headclose_ = 0;
            hc_sol_ = 1;
            hc_eq_ = 0;
            hc_text_ = 0;
#endif
#if HP_WIKITIME_MOD
            wikitime_ = 0;
            tm_st_ = 0;
            tm_n1_ = 0;
            tm_n2_ = 0;
#endif
#if HP_LINKCOMMA_MOD
            linkcomma_ = 0;
            lc_pipe_ = 0;
#endif
#if HP_CATBLOCK_MOD
            catblock_ = 0;
            cb_n_ = 0;
            cb_hit_ = 0;
#endif
#if HP_BR_MOD
            br_ = 0;
            br_win_ = 0;
#endif
#if HP_AMPNBSP_MOD
            ampnbsp_ = 0;
            ampnbsp_win_ = 0;
#endif
#if HP_MDASH_MOD
            mdash_ = 0;
            md_st_ = 0;
#endif
#if HP_LISTMIX_MOD
            listmix_ = 0;
            lm_run_ = 1;
#endif
#if HP_MATH_MOD
            in_math_ = 0;
            mh_open_ = 0;
            mh_win_ = 0;
#endif
#if HP_PROTOCOL_MOD
            protocol_ = 0;
            proto_n_ = 0;
#endif
#if HP_HEXRUN_MOD
            hexval_ = 0;
            hexrun_ = 0;
            hex_on_ = 0;
#endif
#if HP_SQDEPTH_MOD
            sqdepth_ = 0;
#endif
#if HP_PIPEROLE_MOD
            piperole_ = 0;
#endif
#if HP_AFTERREF_MOD
            afterref_ = 0;
            ar_win_ = 0;
#endif
#if HP_SENTPOS_MOD
            sentpos_ = 0;
            sp_inword_ = 0;
#endif
#if HP_ABBREV_MOD
            abbrev_ = 0;
            ab_n_ = 0;
            ab_w_ = 0;
#endif
#if HP_THOUSAND_MOD
            thousand_ = 0;
            th_st_ = 0;
            th_n_ = 0;
#endif
#if HP_REFPUNCT_MOD
            refpunct_ = 0;
            rp_win_ = 0;
#endif
#if HP_QPERIOD_MOD
            qperiod_ = 0;
#endif
#if HP_ELLIPSIS_MOD
            ellipsis_ = 0;
            ellip_n_ = 0;
#endif
#if HP_NUMRANGE_MOD
            numrange_ = 0;
            nr_st_ = 0;
            nr_utf_ = 0;
#endif
#if HP_DEG_MOD
            deg_ = 0;
            deg_d_ = 0;
            deg_st_ = 0;
            deg_win_ = 0;
#endif
#if HP_PERCENT_MOD
            percent_ = 0;
            pct_d_ = 0;
#endif
#if HP_STATETRANS_MOD
            prev_state_ = 0;
#endif
#if HP_LISTPARA_MOD
            listpara_ = 0;
            lp_apos_ = 0;
#endif
#if HP_SECFRAG_MOD
            secfrag_ = 0;
#endif
#if HP_WIKIVAR_MOD
            wikivar_ = 0;
            wv_on_ = 0;
            wv_n_ = 0;
#endif
#if HP_SUBPAGE_MOD
            subpage_ = 0;
#endif
#if HP_LINKNS_MOD
            linkns_ = 0;
            ln_on_ = 0;
            ln_n_ = 0;
#endif
#if HP_FCCUR_MOD
            fccur_ = 0;
            fccur_set_ = 0;
#endif
#if HP_REFGROUP_MOD
            refgroup_ = 0;
            rg_slash_ = 0;
            rg_n_ = 0;
            rg_name_ = 0;
            rg_group_ = 0;
            rg_win_ = 0;
#endif
#if HP_REFLIST_MOD
            reflist_ = 0;
            rl_from_tpl_ = 0;
            rl_d_ = 0;
            rl_slash_ = 0;
            rl_n_ = 0;
#endif
#if HP_SISTER_MOD
            sister_ = 0;
            sis_on_ = 0;
            sis_n_ = 0;
#endif
#if HP_CONVERT_MOD
            convert_ = 0;
            convert_d_ = 0;
#endif
#if HP_CN_MOD
            cn_ = 0;
            cn_d_ = 0;
#endif
#if HP_CONVERT_MOD || HP_CN_MOD || HP_REFLIST_MOD
            h39t_depth_ = 0;
            h39t_n_ = 0;
            h39t_col_ = 0;
#endif
#if HP_BLOCK_MOD
            block_ = 0;
            bk_slash_ = 0;
            bk_n_ = 0;
#endif
#if HP_PIPETRICK_MOD
            pipetrick_ = 0;
            pt_pipe_ = 0;
#endif
#if HP_NOTES_MOD
            notes_ = 0;
            notes_d_ = 0;
#endif
#if HP_LANGTPL_MOD
            langtpl_ = 0;
            langtpl_d_ = 0;
#endif
#if HP_FRAC_MOD
            frac_ = 0;
            frac_d_ = 0;
#endif
#if HP_LISTEN_MOD
            listen_ = 0;
            listen_d_ = 0;
            ls_pre_n_ = 0;
            ls_pre_on_ = 0;
            ls_in_file_ = 0;
            ls_win_ = 0;
#endif
#if HP_BIRTH_MOD
            birth_ = 0;
            birth_d_ = 0;
#endif
#if HP_HLIST_MOD
            hlist_ = 0;
            hlist_d_ = 0;
#endif
#if HP_MAINART_MOD
            mainart_ = 0;
            mainart_d_ = 0;
            ma_seen_head_ = 0;
#endif
#if HP_CHEM_MOD
            in_chem_ = 0;
            ch_open_ = 0;
            ch_win_ = 0;
#endif
#if HP_SMALL_MOD
            in_small_ = 0;
            sm_open_ = 0;
            sm_win_ = 0;
#endif
#if HP_SUPSUB_MOD
            supsub_ = 0;
            ss_open_ = 0;
            ss_win_ = 0;
#endif
#if HP_PRECODE_MOD
            precode_ = 0;
            pc_open_ = 0;
            pc_win_ = 0;
#endif
#if HP_NOTES_MOD || HP_LANGTPL_MOD || HP_FRAC_MOD || HP_LISTEN_MOD || \
    HP_BIRTH_MOD || HP_HLIST_MOD || HP_MAINART_MOD
            h40t_depth_ = 0;
            h40t_n_ = 0;
            h40t_col_ = 0;
#endif
#if HP_SFN_MOD
            sfn_ = 0;
            sfn_d_ = 0;
#endif
#if HP_GEOTEMP_MOD
            geotemp_ = 0;
            geotemp_d_ = 0;
#endif
#if HP_EPIGRAPH_MOD
            epigraph_ = 0;
            epigraph_d_ = 0;
#endif
#if HP_TRACKLIST_MOD
            tracklist_ = 0;
            tracklist_d_ = 0;
#endif
#if HP_SUCCESSION_MOD
            succession_ = 0;
            succession_d_ = 0;
#endif
#if HP_COLSTART_MOD
            colstart_ = 0;
            colstart_d_ = 0;
#endif
#if HP_REFBEGIN_MOD
            refbegin_ = 0;
#endif
#if HP_TOC_MOD
            toc_ = 0;
            toc_col_ = 0;
            toc_n_ = 0;
#endif
#if HP_SFN_MOD || HP_GEOTEMP_MOD || HP_EPIGRAPH_MOD || HP_TRACKLIST_MOD || \
    HP_SUCCESSION_MOD || HP_COLSTART_MOD || HP_REFBEGIN_MOD
            h41t_depth_ = 0;
            h41t_n_ = 0;
            h41t_col_ = 0;
#endif
#if HP_SHORTDESC_MOD
            shortdesc_ = 0;
            shortdesc_d_ = 0;
#endif
#if HP_SEEALSO_MOD
            seealso_ = 0;
            seealso_d_ = 0;
#endif
#if HP_PORTAL_MOD
            portal_ = 0;
            portal_d_ = 0;
#endif
#if HP_AUTHCTL_MOD
            authctl_ = 0;
            authctl_d_ = 0;
#endif
#if HP_USEDATE_MOD
            usedate_ = 0;
#endif
#if HP_IPA_MOD
            ipa_ = 0;
            ipa_d_ = 0;
#endif
#if HP_GOODART_MOD
            goodart_ = 0;
#endif
#if HP_CAPTION_MOD
            caption_ = 0;
            caption_d_ = 0;
            cap_key_on_ = 0;
            cap_key_n_ = 0;
            cap_sq_ = 0;
#endif
#if HP_SHORTDESC_MOD || HP_SEEALSO_MOD || HP_PORTAL_MOD || HP_AUTHCTL_MOD || \
    HP_USEDATE_MOD || HP_IPA_MOD || HP_GOODART_MOD || HP_CAPTION_MOD
            h42t_depth_ = 0;
            h42t_n_ = 0;
            h42t_col_ = 0;
#endif
#if HP_NAVBOX_MOD
            navbox_ = 0;
            navbox_d_ = 0;
#endif
#if HP_EFOOT_MOD
            efoot_ = 0;
            efoot_d_ = 0;
#endif
#if HP_RSHORT_MOD
            rshort_ = 0;
            rshort_d_ = 0;
#endif
#if HP_ASOF_MOD
            asof_ = 0;
            asof_d_ = 0;
#endif
#if HP_CLARIFY_MOD
            clarify_ = 0;
            clarify_d_ = 0;
#endif
#if HP_CURRENCY_MOD
            currency_ = 0;
            currency_d_ = 0;
#endif
#if HP_DISPLAYTITLE_MOD
            displaytitle_ = 0;
#endif
#if HP_NOWRAP_MOD
            nowrap_ = 0;
            nowrap_d_ = 0;
#endif
#if HP_STUB_MOD
            stub_ = 0;
#endif
#if HP_PERSONDATA_MOD
            persondata_ = 0;
            persondata_d_ = 0;
#endif
#if HP_FLAG_MOD
            flag_ = 0;
            flag_d_ = 0;
#endif
#if HP_QUOTEBOX_MOD
            quotebox_ = 0;
            quotebox_d_ = 0;
#endif
#if HP_CLEAR_MOD
            clear_ = 0;
            h43t_dash_ = 0;
#endif
#if HP_IMDB_MOD
            imdb_ = 0;
            imdb_d_ = 0;
#endif
#if HP_RP_MOD
            rp_ = 0;
            rp_d_ = 0;
#endif
#if HP_FN_MOD
            fn_ = 0;
            fn_d_ = 0;
#endif
#if HP_TAXOBOX_MOD
            taxobox_ = 0;
            taxobox_d_ = 0;
#endif
#if HP_NIHONGO_MOD
            nihongo_ = 0;
            nihongo_d_ = 0;
#endif
#if HP_DEADLINK_MOD
            deadlink_ = 0;
            deadlink_d_ = 0;
#endif
#if HP_WAYBACK_MOD
            wayback_ = 0;
            wayback_d_ = 0;
#endif
#if HP_UNREF_MOD
            unref_ = 0;
            unref_d_ = 0;
#endif
#if HP_CLEANUP_MOD
            cleanup_ = 0;
            cleanup_d_ = 0;
#endif
#if HP_NPOV_MOD
            npov_ = 0;
            npov_d_ = 0;
#endif
#if HP_RFROM_MOD
            rfrom_ = 0;
            rfrom_d_ = 0;
#endif
#if HP_DOI_MOD
            doi_ = 0;
            doi_d_ = 0;
            doi_win_ = 0;
#endif
#if HP_PMID_MOD
            pmid_ = 0;
            pmid_d_ = 0;
            pmid_win_ = 0;
#endif
#if HP_ISBN_MOD
            isbn_ = 0;
            isbn_win_ = 0;
#endif
#if HP_MEDAL_MOD
            medal_ = 0;
            medal_d_ = 0;
#endif
#if HP_THUMB_MOD
            thumb_ = 0;
            thumb_win_ = 0;
#endif
#if HP_FURTHER_MOD
            further_ = 0;
            further_d_ = 0;
#endif
#if HP_DEATH_MOD
            death_ = 0;
            death_d_ = 0;
#endif
#if HP_HARV_MOD
            harv_ = 0;
            harv_d_ = 0;
#endif
#if HP_ISSN_MOD
            issn_ = 0;
            issn_win_ = 0;
#endif
#if HP_OCLC_MOD
            oclc_ = 0;
            oclc_win_ = 0;
#endif
#if HP_ALIGN_MOD
            align_ = 0;
            align_win_ = 0;
#endif
#if HP_SYNTAX_MOD
            syntax_ = 0;
            sx_open_ = 0;
            sx_win_ = 0;
#endif
#if HP_NAVBOX_MOD || HP_EFOOT_MOD || HP_RSHORT_MOD || HP_ASOF_MOD || \
    HP_CLARIFY_MOD || HP_CURRENCY_MOD || HP_DISPLAYTITLE_MOD || HP_NOWRAP_MOD || \
    HP_STUB_MOD || HP_PERSONDATA_MOD || HP_FLAG_MOD || HP_QUOTEBOX_MOD || \
    HP_CLEAR_MOD || HP_IMDB_MOD || HP_RP_MOD || HP_FN_MOD || \
    HP_TAXOBOX_MOD || HP_NIHONGO_MOD || HP_DEADLINK_MOD || HP_WAYBACK_MOD || \
    HP_UNREF_MOD || HP_CLEANUP_MOD || HP_NPOV_MOD || HP_RFROM_MOD || \
    HP_DOI_MOD || HP_PMID_MOD || HP_MEDAL_MOD || \
    HP_FURTHER_MOD || HP_DEATH_MOD || HP_HARV_MOD
            h43t_depth_ = 0;
            h43t_n_ = 0;
            h43t_col_ = 0;
#endif
        }
#endif
    }
    int xml_slash_ = 0;
    int xml_n_ = 0;
    int xml_done_ = 0;
    std::uint64_t xml_name_ = 0;
#endif
#if HP_TITLE_MOD
    int in_title_ = 0;
    std::uint64_t title_hash_ = 0;
#endif
#if HP_PAGEID_MOD
    int in_id_ = 0;
    int pageid_seen_ = 0;
    std::uint64_t page_id_ = 0;
#endif
#if HP_USER_MOD
    int in_user_ = 0;
    std::uint64_t user_hash_ = 0;
#endif
#if HP_TEXT_MOD
    int in_text_ = 0;
#endif
#if HP_NS_MOD
    int in_ns_ = 0;
    int ns_id_ = 0;
#endif
#if HP_DUMPREDIR_MOD
    int dump_redir_ = 0;
#endif
#if HP_IP_MOD
    int in_ip_ = 0;
    std::uint64_t ip_hash_ = 0;
#endif
#if HP_REVCOMMENT_MOD
    int in_comment_ = 0;
    std::uint64_t comment_hash_ = 0;
#endif
#if HP_MINOR_MOD
    int minor_ = 0;
#endif
#if HP_WIKIMODEL_MOD
    int in_model_ = 0;
    std::uint64_t model_hash_ = 0;
#endif
#if HP_SECTITLE_MOD
    int st_skip_ = 0;
    int st_collect_ = 0;
    std::uint64_t st_hash_ = 0;
#endif
#if HP_PARSERFN_MOD
    int pfn_wait_ = 0;
    int pfn_collect_ = 0;
    std::uint64_t pfn_hash_ = 0;
#endif
#if HP_TABLECLASS_MOD
    int tc_collect_ = 0;
    std::uint64_t tc_hash_ = 0;
#endif
#if HP_ANCHOR_MOD
    int an_collect_ = 0;
    std::uint64_t an_hash_ = 0;
#endif
#if HP_PUBID_MOD
    int pub_collect_ = 0;
    std::uint32_t pub_win_ = 0;
    std::uint64_t pub_hash_ = 0;
#endif
#if HP_TEMPPOS_MOD
    int tp_collect_ = 0;
    int tp_done_ = 0;
    std::uint64_t tp_hash_ = 0;
#endif
#if HP_TABLE_ABOVE || HP_FCCXT_MOD || HP_WIKISTACK_MOD || HP_COLRING_MOD
    void table_reset() {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 32; ++c) tbl_cells_[r][c] = 0;
        tbl_row_ = 0;
        tbl_cell_ = 0;
        tbl_started_ = 0;
    }
    void table_push(int c) {
        if (c == '|' && prev1_ != '{') {
            if (tbl_cell_ < 31) ++tbl_cell_;
            tbl_started_ = 0;
        } else if (c == '\n') {
            tbl_row_ = (tbl_row_ + 1) & 3;
            tbl_cell_ = 0;
            tbl_started_ = 0;
        } else if (!tbl_started_ && c != '-' && c != ' ' && c != '\t') {
            tbl_cells_[tbl_row_][tbl_cell_ > 31 ? 31 : tbl_cell_] =
                static_cast<std::uint8_t>(c);
            tbl_started_ = 1;
        }
    }
    std::uint8_t tbl_cells_[4][32] = {};
    int tbl_row_ = 0;
    int tbl_cell_ = 0;
    int tbl_started_ = 0;
#endif
#if HP_FCCUR_MOD
    static int fccur_token_(int c) {
        if (c >= 'A' && c <= 'Z') return 1;
        if (c == ':') return 2;
        if (c == '<') return 3;
        if (c == '=') return 4;
        if (c == '*') return 5;
        if (c == '#') return 6;
        if (c == '|') return 7;
        if (c == '[') return 8;
        if (c == '{') return 9;
        if (c == '>') return 10;
        if (c == ';') return 11;
        if (c == '\'') return 12;
        return 13;
    }
#endif
#if HP_LINKNS_MOD
    static int link_ns_id_(const char* s, int n) {
        auto eq = [&](const char* w, int wn) {
            if (n != wn) return 0;
            for (int i = 0; i < wn; ++i)
                if (s[i] != w[i]) return 0;
            return 1;
        };
        auto st = [&](const char* w, int wn) {
            if (n < wn) return 0;
            for (int i = 0; i < wn; ++i)
                if (s[i] != w[i]) return 0;
            return 1;
        };
        if (n == 0) return 12;
        if (eq("file", 4) || eq("image", 5) || eq("media", 5)) return 1;
        if (eq("category", 8) || eq("cat", 3)) return 2;
        if (eq("template", 8)) return 3;
        if (eq("user", 4) || st("user ", 5)) return 4;
        if (eq("wikipedia", 9) || eq("wp", 2) || eq("project", 7)) return 5;
        if (eq("help", 4)) return 6;
        if (eq("portal", 6)) return 7;
        if (eq("talk", 4) || st("talk ", 5)) return 8;
        if (eq("wiktionary", 10) || eq("wikt", 4)) return 9;
        if (eq("commons", 7)) return 10;
        return 11;
    }
#endif
#if HP_SISTER_MOD
    static int sister_id_(const char* s, int n) {
        auto eq = [&](const char* w, int wn) {
            if (n != wn) return 0;
            for (int i = 0; i < wn; ++i)
                if (s[i] != w[i]) return 0;
            return 1;
        };
        if (eq("wikt", 4) || eq("wiktionary", 10)) return 1;
        if (eq("commons", 7) || eq("c", 1)) return 2;
        if (eq("n", 1) || eq("wikinews", 8)) return 3;
        if (eq("s", 1) || eq("wikisource", 10)) return 4;
        if (eq("b", 1) || eq("wikibooks", 9)) return 5;
        if (eq("v", 1) || eq("wikiversity", 11)) return 6;
        if (eq("q", 1) || eq("wikiquote", 9)) return 7;
        return 0;
    }
#endif
#if HP_WIKIVAR_MOD
    static int wiki_var_id_(const char* s, int n) {
        auto st = [&](const char* w, int wn) {
            if (n < wn) return 0;
            for (int i = 0; i < wn; ++i)
                if (s[i] != w[i]) return 0;
            return 1;
        };
        if (st("CURRENT", 7)) return 1;
        if (st("LOCAL", 5)) return 2;
        if (st("REVISION", 8)) return 3;
        if (st("PAGENAME", 8) || st("BASEPAGE", 8) || st("SUBPAGE", 7) ||
            st("FULLPAGE", 8) || st("PAGEID", 6) || st("PAGESIZE", 8))
            return 4;
        if (st("NAMESPACE", 9) || st("TALKSPACE", 9) || st("SUBJECT", 7) ||
            st("ARTICLE", 7) || st("TALKPAGE", 8))
            return 5;
        if (st("NUMBEROF", 8)) return 6;
        if (st("SITENAME", 8) || st("SERVER", 6)) return 7;
        if (st("DISPLAYT", 8) || st("DEFAULTS", 8) || st("DIRMARK", 7) ||
            st("CONTENTL", 8) || st("PROTECT", 7) || st("CASCADING", 9))
            return 8;
        return 0;
    }
#endif
#if HP_WIKISTACK_MOD
    static int fc_token(int c) {
        if (c >= 'A' && c <= 'Z') return 1;
        if (c == ':') return 2;
        if (c == '<') return 3;
        if (c == '=') return 4;
        if (c == '*') return 5;
        if (c == '#') return 6;
        if (c == '|') return 7;
        if (c == '[') return 8;
        if (c == '{') return 9;
        if (c == '>') return 10;
        if (c == ';') return 11;
        if (c == '\'') return 12;
        return 13;
    }
    static int br_open(int c) {
        if (c == '(') return 1;
        if (c == '{') return 2;
        if (c == '[') return 3;
        if (c == '<') return 4;
        return 0;
    }
    static int br_close(int c) {
        if (c == ')') return 1;
        if (c == '}') return 2;
        if (c == ']') return 3;
        if (c == '>') return 4;
        return 0;
    }
    void stack_feed(int c) {
        if (c == '\n') {
            if (stk_col_ <= 1) {
                if (blank_run_ < 3) ++blank_run_;
            } else {
                blank_run_ = 0;
            }
            if (blank_run_ >= 2) {
                fc_stk_[0] = fc_stk_[1] = fc_stk_[2] = fc_stk_[3] = 0;
                br_stk_[0] = br_stk_[1] = br_stk_[2] = 0;
                br_n_ = 0;
            } else {
                fc_stk_[3] = fc_stk_[2];
                fc_stk_[2] = fc_stk_[1];
                fc_stk_[1] = fc_stk_[0];
                fc_stk_[0] = static_cast<std::uint8_t>(stk_fc_);
            }
            stk_col_ = 0;
            stk_fc_ = 0;
            saw_fc_ = 0;
        } else {
            if (!saw_fc_ && c != ' ' && c != '\t') {
                stk_fc_ = fc_token(c);
                fc_stk_[0] = static_cast<std::uint8_t>(stk_fc_);
                saw_fc_ = 1;
            }
            if (stk_col_ < 255) ++stk_col_;
            const int op = br_open(c);
            if (op) {
                if (br_n_ < 3) {
                    br_stk_[br_n_] = static_cast<std::uint8_t>(op);
                    ++br_n_;
                } else {
                    br_stk_[0] = br_stk_[1];
                    br_stk_[1] = br_stk_[2];
                    br_stk_[2] = static_cast<std::uint8_t>(op);
                }
            }
            const int cl = br_close(c);
            if (cl && br_n_ > 0 && br_stk_[br_n_ - 1] == cl) {
                --br_n_;
                br_stk_[br_n_] = 0;
            }
        }
    }
    std::uint8_t fc_stk_[4] = {};
    std::uint8_t br_stk_[3] = {};
    int br_n_ = 0;
    int stk_col_ = 0;
    int stk_fc_ = 0;
    int saw_fc_ = 0;
    int blank_run_ = 0;
#endif

#if HP_SECTION_MUTE
    // fx2 skipSeeExternal / isMath: derived, both sides see the same bytes.
    void push_mute(int c) {
        ring_[ring_n_ & 31] = static_cast<std::uint8_t>(c);
        ++ring_n_;
        if (line_len_ < 63) linebuf_[line_len_++] = static_cast<std::uint8_t>(c);

        if (ends_with_ci("&lt;math") || ends_with_ci("<math")) mute_ = 1;
        if (ends_with_ci("&lt;pre") || ends_with_ci("<pre")) mute_ = 1;
        if (ends_with_ci("&lt;nowiki") || ends_with_ci("<nowiki")) mute_ = 1;
        if (ends_with_ci("&lt;/math") || ends_with_ci("</math")) mute_ = 0;
        if (ends_with_ci("&lt;/pre") || ends_with_ci("</pre")) mute_ = 0;
        if (ends_with_ci("&lt;/nowiki") || ends_with_ci("</nowiki")) mute_ = 0;
        if (ends_with_ci("[[category:")) mute_ = 1;
        if (ends_with_ci("<page>") || ends_with_ci("</page>")) mute_ = 0;

        if (c == '\n') {
            if (is_skip_header(linebuf_, line_len_)) mute_ = 1;
            else if (is_any_header(linebuf_, line_len_)) mute_ = 0;
            line_len_ = 0;
        }
    }

    static int ci_eq(int b, int t) {
        if (b >= 'A' && b <= 'Z') b += 32;
        if (t >= 'A' && t <= 'Z') t += 32;
        return b == t;
    }

    bool ends_with_ci(const char* s) const {
        int n = 0;
        while (s[n]) ++n;
        if (ring_n_ < n) return false;
        for (int i = 0; i < n; ++i) {
            if (!ci_eq(ring_[(ring_n_ - n + i) & 31], static_cast<unsigned char>(s[i])))
                return false;
        }
        return true;
    }

    static bool is_any_header(const std::uint8_t* s, int n) {
        int i = 0;
        while (i < n && (s[i] == ' ' || s[i] == '\t')) ++i;
        return i + 1 < n && s[i] == '=' && s[i + 1] == '=';
    }

    static bool contains_ci(const std::uint8_t* s, int n, const char* pat) {
        int m = 0;
        while (pat[m]) ++m;
        for (int i = 0; i + m <= n; ++i) {
            int ok = 1;
            for (int j = 0; j < m; ++j) {
                if (!ci_eq(s[i + j], static_cast<unsigned char>(pat[j]))) {
                    ok = 0;
                    break;
                }
            }
            if (ok) return true;
        }
        return false;
    }

    static bool is_skip_header(const std::uint8_t* s, int n) {
        if (!is_any_header(s, n)) return false;
        return contains_ci(s, n, "references") || contains_ci(s, n, "see also") ||
               contains_ci(s, n, "bibliography") || contains_ci(s, n, "external link");
    }

    std::uint8_t ring_[32] = {};
    std::uint8_t linebuf_[64] = {};
    int ring_n_ = 0;
    int line_len_ = 0;
    int mute_ = 0;
#endif
};

}  // namespace hp
