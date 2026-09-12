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
            return;
        }
        if (c == '>' && in_tag_) {
            in_tag_ = 0;
            state_ = kWkText;
            return;
        }
        if (c == '/' && in_tag_) {
            if (depth_ > 0) --depth_;
            state_ = kWkTagEnd;
            return;
        }
        if (in_tag_) {
            tag_name_ = mix64(tag_name_ * 31 + static_cast<std::uint64_t>(c));
            if (prev1_ == '!' && c == '-') state_ = kWkComment;
            return;
        }

        // Wiki link [[
        if (prev1_ == '[' && c == '[') {
            state_ = kWkSquareOpen;
            sq_ = 1;
            linkword_ = 0;
            return;
        }
        if (state_ == kWkSquareOpen) {
            if (c == ']') {
                if (sq_ > 0) --sq_;
                if (sq_ == 0) { state_ = kWkText; linkword_ = 0; }
            } else if (c == ':') {
                linkword_ = 0;   // fx2: [category:...] drops the hash
            } else {
                // fx2-cmix: linkword = linkword * 2104 + j
                linkword_ = linkword_ * 2104ull + static_cast<std::uint64_t>(c);
            }
            return;
        }

        // Template / infobox {{
        if (prev1_ == '{' && c == '{') {
            state_ = kWkCurly;
            if (depth_ < 15) ++depth_;
            return;
        }
        if (prev1_ == '{' && c == '|') {
            state_ = kWkWikiTable;
            in_table_ = 1;
#if HP_TABLE_ABOVE
            table_reset();
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
#if HP_TABLE_ABOVE
            table_reset();
#endif
            return;
        }
        if (prev1_ == '}' && c == '}') {
            if (depth_ > 0) --depth_;
            state_ = kWkText;
            return;
        }

        if (c == '|' && (in_table_ || state_ == kWkCurly || state_ == kWkWikiTable)) {
            state_ = kWkVerticalBar;
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
            }
            if (c == ' ' || c == '\n') http_run_ = 0;
        }
        if (state_ == kWkHtLink && c != ' ' && c != '\n') {
            linkword_ = linkword_ * 2104ull + static_cast<std::uint64_t>(c);
        }

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

#if HP_TABLE_ABOVE
        if (in_table_) table_push(c);
#endif
#if HP_SECTION_MUTE
        push_mute(c);
#endif

        if (c == '\n') {
            ++line_;
            first_of_line_ = 1;
            line_kind_ = 0;
            if (state_ == kWkText) ++para_;
        } else if (first_of_line_ && c != ' ' && c != '\t') {
            first_of_line_ = 0;
            line_kind_ = c;
            // fx2: isParagraph = (fc == FIRSTUPPER); WIKIHEADER = line-start '>'
            is_paragraph_ = (c >= 'A' && c <= 'Z') ? 1 : 0;
            wiki_header_ = (c == '>') ? 1 : 0;
            if (is_paragraph_ && state_ == kWkText) state_ = kWkFirstUpper;
            if (wiki_header_ && state_ == kWkText) state_ = kWkHeader;
        }
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
#if HP_TABLE_ABOVE
        const int r = (tbl_row_ + 3) & 3;
        const int c = tbl_cell_ > 31 ? 31 : tbl_cell_;
        return tbl_cells_[r][c];
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
#if HP_TABLE_ABOVE
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
