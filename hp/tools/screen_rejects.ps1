# Re-screen H11–H38 leftover rejects at HP_LR1_SCALE=40 on a 2 MiB slice.
# Champ flags from v78_flags.ps1. HP_SLOT_MAX is hard-capped at 22 in features.hpp.
# Default: compile hp_s_base.exe and run ONLY that 2 MiB encode if no hp_g_/hp_v job
# is already encoding. Does not overwrite hp_v83.exe / hp_v84.exe.
# Does not start the leftover marathon unless -RunQueue is passed.
#
# Usage (from repo root):
#   powershell -File hp/tools/screen_rejects.ps1              # baseline compile + encode if idle
#   powershell -File hp/tools/screen_rejects.ps1 -List
#   powershell -File hp/tools/screen_rejects.ps1 -Name sentpos
#   powershell -File hp/tools/screen_rejects.ps1 -Name sentpos -CompileOnly
#   powershell -File hp/tools/screen_rejects.ps1 -RunQueue -Limit 8
param(
  [switch]$List,
  [string]$Name = "",
  [switch]$CompileOnly,
  [switch]$RunQueue,
  [switch]$ForceEncode,
  [int]$Limit = 0
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not (Test-Path (Join-Path $root "hp\src\main.cpp"))) {
  $root = "C:\Users\odinl\OneDrive\Desktop\Compression Algorithm"
}
Set-Location $root
. (Join-Path $PSScriptRoot "v78_flags.ps1")

$InSlice = Join-Path $root "data\enwik8.2mb"
$BuildDir = Join-Path $root "hp\build"
$Src = Join-Path $root "hp\src\main.cpp"
$LabDir = Join-Path $env:LOCALAPPDATA "hp_lab"
$LogCsv = Join-Path $LabDir "screen_rejects.csv"

# H11–H38 leftover rejects that already exist in features.hpp.
# Ordered: H38 still-open joints, then closest historical 8 MB deltas, then the rest.
# skip40 replaces champ HP_MIXER_SKIP=32 rather than adding a CM.
# H22 reorder / payload_lex / dict omitted (preprocess, not leftover CMs).
$RejectRows = @(
  # H54 new axis / table RAM on v93. Baseline this-tree 439,192.
  @{ n = "matchword";   d = "-DHP_MATCH_WORD=1";      w = "H54"; old = $null },
  @{ n = "o12";         d = "-DHP_SLOT_O12=1";        w = "H54"; old = $null },
  # H53 SKIP_L1 neighbors of 80 on v93 (sl80 champ). Baseline 438,789.
  @{ n = "sl72";        d = "-DHP_MIXER_SKIP_L1=72";   w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl84";        d = "-DHP_MIXER_SKIP_L1=84";   w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl88";        d = "-DHP_MIXER_SKIP_L1=88";   w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl96";        d = "-DHP_MIXER_SKIP_L1=96";   w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl104";       d = "-DHP_MIXER_SKIP_L1=104";  w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl112";       d = "-DHP_MIXER_SKIP_L1=112";  w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl120";       d = "-DHP_MIXER_SKIP_L1=120";  w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl128";       d = "-DHP_MIXER_SKIP_L1=128";  w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl144";       d = "-DHP_MIXER_SKIP_L1=144";  w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  @{ n = "sl160";       d = "-DHP_MIXER_SKIP_L1=160";  w = "H53"; old = $null; r = "-DHP_MIXER_SKIP_L1=80" },
  # H52 SKIP_L1 neighbors + restack on v92 (skipl1 champ). Baseline 438,841.
  @{ n = "sl16";        d = "-DHP_MIXER_SKIP_L1=16";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl24";        d = "-DHP_MIXER_SKIP_L1=24";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl32";        d = "-DHP_MIXER_SKIP_L1=32";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl48";        d = "-DHP_MIXER_SKIP_L1=48";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl56";        d = "-DHP_MIXER_SKIP_L1=56";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl64";        d = "-DHP_MIXER_SKIP_L1=64";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl72";        d = "-DHP_MIXER_SKIP_L1=72";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "sl80";        d = "-DHP_MIXER_SKIP_L1=80";   w = "H52"; old = $null; r = "-DHP_MIXER_SKIP_L1=40" },
  @{ n = "lr30";        d = "-DHP_LR1_SCALE=30";       w = "H52"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "skip48";      d = "-DHP_MIXER_SKIP=48";      w = "H52"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  # H51 mixer-scale neighbors + restack on v91 (msc75 champ). Baseline 438,897.
  @{ n = "msc50";       d = "-DHP_MIXER_SCALE=32768";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc62";       d = "-DHP_MIXER_SCALE=40960";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc69";       d = "-DHP_MIXER_SCALE=45056";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc72";       d = "-DHP_MIXER_SCALE=47104";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc78";       d = "-DHP_MIXER_SCALE=51200";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc81";       d = "-DHP_MIXER_SCALE=53248";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc88";       d = "-DHP_MIXER_SCALE=57344";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "msc94";       d = "-DHP_MIXER_SCALE=61440";  w = "H51"; old = $null; r = "-DHP_MIXER_SCALE=49152" },
  @{ n = "lr30";        d = "-DHP_LR1_SCALE=30";       w = "H51"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "skipl1";      d = "-DHP_MIXER_SKIP_L1=40";   w = "H51"; old = $null },
  # H50 mixer knobs on v90 (skip56 champ). Baseline s_skip56.hp 439,210.
  @{ n = "lr30";        d = "-DHP_LR1_SCALE=30";       w = "H50"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "lr35";        d = "-DHP_LR1_SCALE=35";       w = "H50"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "skip64";      d = "-DHP_MIXER_SKIP=64";      w = "H50"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "skip72";      d = "-DHP_MIXER_SKIP=72";      w = "H50"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "skip80";      d = "-DHP_MIXER_SKIP=80";      w = "H50"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "apm6";        d = "-DHP_APM_RATE=6";         w = "H50"; old = $null },
  @{ n = "apm8";        d = "-DHP_APM_RATE=8";         w = "H50"; old = $null },
  @{ n = "msc75";       d = "-DHP_MIXER_SCALE=49152";  w = "H50"; old = $null },
  @{ n = "msc112";      d = "-DHP_MIXER_SCALE=73728";  w = "H50"; old = $null },
  @{ n = "skipl1";      d = "-DHP_MIXER_SKIP_L1=40";   w = "H50"; old = $null },
  # H49 mixer/APM knobs on v89 (replace skip/scale or extra APM blend; not new CMs)
  @{ n = "skip24";      d = "-DHP_MIXER_SKIP=24";      w = "H49"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "skip32";      d = "-DHP_MIXER_SKIP=32";      w = "H49"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "skip48";      d = "-DHP_MIXER_SKIP=48";      w = "H49"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "skip56";      d = "-DHP_MIXER_SKIP=56";      w = "H49"; old = $null; r = "-DHP_MIXER_SKIP=56" },
  @{ n = "lr30";        d = "-DHP_LR1_SCALE=30";       w = "H49"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "lr35";        d = "-DHP_LR1_SCALE=35";       w = "H49"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "lr45";        d = "-DHP_LR1_SCALE=45";       w = "H49"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "lr50";        d = "-DHP_LR1_SCALE=50";       w = "H49"; old = $null; r = "-DHP_LR1_SCALE=40" },
  @{ n = "w0one";       d = "-DHP_W0=1";               w = "H49"; old = $null },
  @{ n = "wb4";         d = "-DHP_WB=4";               w = "H49"; old = $null },
  # H47 leftover CMs on v89 (new wiki-domain / identifier / layout, default-off)
  @{ n = "thumb";       d = "-DHP_THUMB_MOD=1";        w = "H47"; old = $null },
  @{ n = "further";     d = "-DHP_FURTHER_MOD=1";      w = "H47"; old = $null },
  @{ n = "death";       d = "-DHP_DEATH_MOD=1";        w = "H47"; old = $null },
  @{ n = "harv";        d = "-DHP_HARV_MOD=1";         w = "H47"; old = $null },
  @{ n = "issn";        d = "-DHP_ISSN_MOD=1";         w = "H47"; old = $null },
  @{ n = "oclc";        d = "-DHP_OCLC_MOD=1";         w = "H47"; old = $null },
  @{ n = "alignmod";    d = "-DHP_ALIGN_MOD=1";        w = "H47"; old = $null },
  @{ n = "syntax";      d = "-DHP_SYNTAX_MOD=1";       w = "H47"; old = $null },
  # H46 leftover CMs on v89 (new wiki-domain / identifier, default-off)
  @{ n = "unref";       d = "-DHP_UNREF_MOD=1";        w = "H46"; old = $null },
  @{ n = "cleanup";     d = "-DHP_CLEANUP_MOD=1";      w = "H46"; old = $null },
  @{ n = "npov";        d = "-DHP_NPOV_MOD=1";         w = "H46"; old = $null },
  @{ n = "rfrom";       d = "-DHP_RFROM_MOD=1";        w = "H46"; old = $null },
  @{ n = "doi";         d = "-DHP_DOI_MOD=1";          w = "H46"; old = $null },
  @{ n = "pmid";        d = "-DHP_PMID_MOD=1";         w = "H46"; old = $null },
  @{ n = "isbnmod";     d = "-DHP_ISBN_MOD=1";         w = "H46"; old = $null },
  @{ n = "medal";       d = "-DHP_MEDAL_MOD=1";        w = "H46"; old = $null },
  # H45 leftover CMs on v89 (new wiki-domain / layout, default-off)
  @{ n = "small";       d = "-DHP_SMALL_MOD=1";        w = "H45"; old = $null },
  @{ n = "supsub";      d = "-DHP_SUPSUB_MOD=1";       w = "H45"; old = $null },
  @{ n = "precode";     d = "-DHP_PRECODE_MOD=1";      w = "H45"; old = $null },
  @{ n = "taxobox";     d = "-DHP_TAXOBOX_MOD=1";      w = "H45"; old = $null },
  @{ n = "nihongo";     d = "-DHP_NIHONGO_MOD=1";      w = "H45"; old = $null },
  @{ n = "deadlink";    d = "-DHP_DEADLINK_MOD=1";     w = "H45"; old = $null },
  @{ n = "wayback";     d = "-DHP_WAYBACK_MOD=1";      w = "H45"; old = $null },
  @{ n = "rowspan";     d = "-DHP_ROWSPAN_MOD=1";      w = "H45"; old = $null },
  # H44 leftover CMs on v89 (new wiki-domain / dense-layout, default-off)
  @{ n = "stub";        d = "-DHP_STUB_MOD=1";         w = "H44"; old = $null },
  @{ n = "persondata";  d = "-DHP_PERSONDATA_MOD=1";   w = "H44"; old = $null },
  @{ n = "flag";        d = "-DHP_FLAG_MOD=1";         w = "H44"; old = $null },
  @{ n = "quotebox";    d = "-DHP_QUOTEBOX_MOD=1";     w = "H44"; old = $null },
  @{ n = "clear";       d = "-DHP_CLEAR_MOD=1";        w = "H44"; old = $null },
  @{ n = "imdb";        d = "-DHP_IMDB_MOD=1";         w = "H44"; old = $null },
  @{ n = "rp";          d = "-DHP_RP_MOD=1";           w = "H44"; old = $null },
  @{ n = "fn";          d = "-DHP_FN_MOD=1";           w = "H44"; old = $null },
  # H43 leftover CMs on v89 (new wiki-domain / layout, default-off)
  @{ n = "navbox";       d = "-DHP_NAVBOX_MOD=1";       w = "H43"; old = $null },
  @{ n = "efoot";        d = "-DHP_EFOOT_MOD=1";        w = "H43"; old = $null },
  @{ n = "rshort";       d = "-DHP_RSHORT_MOD=1";       w = "H43"; old = $null },
  @{ n = "asof";         d = "-DHP_ASOF_MOD=1";         w = "H43"; old = $null },
  @{ n = "clarify";      d = "-DHP_CLARIFY_MOD=1";      w = "H43"; old = $null },
  @{ n = "currency";     d = "-DHP_CURRENCY_MOD=1";     w = "H43"; old = $null },
  @{ n = "displaytitle"; d = "-DHP_DISPLAYTITLE_MOD=1"; w = "H43"; old = $null },
  @{ n = "nowrap";       d = "-DHP_NOWRAP_MOD=1";       w = "H43"; old = $null },
  # H42 leftover CMs on v88 (new wiki-domain / layout, default-off)
  @{ n = "shortdesc";   d = "-DHP_SHORTDESC_MOD=1";    w = "H42"; old = $null },
  @{ n = "seealso";     d = "-DHP_SEEALSO_MOD=1";      w = "H42"; old = $null },
  @{ n = "portal";      d = "-DHP_PORTAL_MOD=1";       w = "H42"; old = $null },
  @{ n = "authctl";     d = "-DHP_AUTHCTL_MOD=1";      w = "H42"; old = $null },
  @{ n = "usedate";     d = "-DHP_USEDATE_MOD=1";      w = "H42"; old = $null },
  @{ n = "ipa";         d = "-DHP_IPA_MOD=1";          w = "H42"; old = $null },
  @{ n = "goodart";     d = "-DHP_GOODART_MOD=1";      w = "H42"; old = $null },
  @{ n = "caption";     d = "-DHP_CAPTION_MOD=1";      w = "H42"; old = $null },
  # H41 leftover CMs on v87 (new wiki-domain / layout, default-off)
  @{ n = "sfn";         d = "-DHP_SFN_MOD=1";          w = "H41"; old = $null },
  @{ n = "geotemp";     d = "-DHP_GEOTEMP_MOD=1";      w = "H41"; old = $null },
  @{ n = "epigraph";    d = "-DHP_EPIGRAPH_MOD=1";     w = "H41"; old = $null },
  @{ n = "tracklist";   d = "-DHP_TRACKLIST_MOD=1";    w = "H41"; old = $null },
  @{ n = "succession";  d = "-DHP_SUCCESSION_MOD=1";   w = "H41"; old = $null },
  @{ n = "colstart";    d = "-DHP_COLSTART_MOD=1";     w = "H41"; old = $null },
  @{ n = "toc";         d = "-DHP_TOC_MOD=1";          w = "H41"; old = $null },
  @{ n = "refbegin";    d = "-DHP_REFBEGIN_MOD=1";     w = "H41"; old = $null },
  # H40 leftover CMs on v85 (new wiki-domain / layout, default-off)
  @{ n = "notes";       d = "-DHP_NOTES_MOD=1";        w = "H40"; old = $null },
  @{ n = "langtpl";     d = "-DHP_LANGTPL_MOD=1";      w = "H40"; old = $null },
  @{ n = "frac";        d = "-DHP_FRAC_MOD=1";         w = "H40"; old = $null },
  @{ n = "listen";      d = "-DHP_LISTEN_MOD=1";       w = "H40"; old = $null },
  @{ n = "birth";       d = "-DHP_BIRTH_MOD=1";        w = "H40"; old = $null },
  @{ n = "hlist";       d = "-DHP_HLIST_MOD=1";        w = "H40"; old = $null },
  @{ n = "mainart";     d = "-DHP_MAINART_MOD=1";      w = "H40"; old = $null },
  @{ n = "chem";        d = "-DHP_CHEM_MOD=1";         w = "H40"; old = $null },
  # H39 remaining on v85 (8 MB identity not yet run at SLOT_MAX=24)
  @{ n = "reflist";     d = "-DHP_REFLIST_MOD=1";      w = "H39"; old = $null },
  @{ n = "sister";      d = "-DHP_SISTER_MOD=1";       w = "H39"; old = $null },
  @{ n = "convert";     d = "-DHP_CONVERT_MOD=1";      w = "H39"; old = $null },
  @{ n = "cn";          d = "-DHP_CN_MOD=1";           w = "H39"; old = $null },
  @{ n = "block";       d = "-DHP_BLOCK_MOD=1";        w = "H39"; old = $null },
  @{ n = "pipetrick";   d = "-DHP_PIPETRICK_MOD=1";    w = "H39"; old = $null },
  @{ n = "expectcl";    d = "-DHP_EXPECTCL_MOD=1";     w = "H39"; old = 102 },
  # closest historical 8 MB deltas (most likely to flip at scale 40)
  @{ n = "dmcgrow";     d = "-DHP_DMC_GROW=1";         w = "H14"; old = 1 },
  @{ n = "period";      d = "-DHP_PERIOD_MOD=1";       w = "H11"; old = 6 },
  @{ n = "skip5";       d = "-DHP_SKIP5_MOD=1";        w = "H14"; old = 8 },
  @{ n = "sentpos";     d = "-DHP_SENTPOS_MOD=1";      w = "H35"; old = 32 },
  @{ n = "skip40";      d = "-DHP_MIXER_SKIP=40";      w = "H11"; old = 52; r = "-DHP_MIXER_SKIP=32" },
  @{ n = "cappara";     d = "-DHP_CAPPARA_MOD=1";      w = "H38"; old = 109 },
  @{ n = "wikivar";     d = "-DHP_WIKIVAR_MOD=1";      w = "H37"; old = 118 },
  @{ n = "pron";        d = "-DHP_PRONOUN_MOD=1";      w = "H11"; old = 122 },
  @{ n = "utf8st";      d = "-DHP_UTF8ST_MOD=1";       w = "H33"; old = 126 },
  @{ n = "math";        d = "-DHP_MATH_MOD=1";         w = "H35"; old = 129 },
  @{ n = "afterref";    d = "-DHP_AFTERREF_MOD=1";     w = "H35"; old = 134 },
  @{ n = "ampnbsp";     d = "-DHP_AMPNBSP_MOD=1";      w = "H34"; old = 139 },
  @{ n = "mdash";       d = "-DHP_MDASH_MOD=1";        w = "H34"; old = 140 },
  @{ n = "ellipsis";    d = "-DHP_ELLIPSIS_MOD=1";     w = "H36"; old = 141 },
  @{ n = "percent";     d = "-DHP_PERCENT_MOD=1";      w = "H36"; old = 142 },
  @{ n = "sr";          d = "-DHP_SR_MOD=1";           w = "H11"; old = 143 },
  @{ n = "refidx";      d = "-DHP_REFIDX_MOD=1";       w = "H32"; old = 145 },
  @{ n = "vowel";       d = "-DHP_VOWEL_MOD=1";        w = "H25"; old = 145 },
  @{ n = "fccxt";       d = "-DHP_FCCXT_MOD=1";        w = "H11"; old = 146 },
  @{ n = "dotgap";      d = "-DHP_DOTGAP_MOD=1";       w = "H28"; old = 149 },
  @{ n = "entnum";      d = "-DHP_ENTNUM_MOD=1";       w = "H33"; old = 149 },
  @{ n = "deg";         d = "-DHP_DEG_MOD=1";          w = "H36"; old = 151 },
  @{ n = "wikitime";    d = "-DHP_WIKITIME_MOD=1";     w = "H34"; old = 152 },
  @{ n = "refpunct";    d = "-DHP_REFPUNCT_MOD=1";     w = "H36"; old = 155 },
  @{ n = "br";          d = "-DHP_BR_MOD=1";           w = "H34"; old = 161 },
  @{ n = "year";        d = "-DHP_YEAR_MOD=1";         w = "H24"; old = 168 },
  @{ n = "wikihr";      d = "-DHP_WIKIHR_MOD=1";       w = "H33"; old = 168 },
  @{ n = "statetrans";  d = "-DHP_STATETRANS_MOD=1";   w = "H37"; old = 175 },
  @{ n = "boldst";      d = "-DHP_BOLDST_MOD=1";       w = "H38"; old = 178 },
  @{ n = "subpage";     d = "-DHP_SUBPAGE_MOD=1";      w = "H37"; old = 186 },
  @{ n = "cite";        d = "-DHP_CITE_MOD=1";         w = "H14"; old = 187 },
  @{ n = "sentlen";     d = "-DHP_SENTLEN_MOD=1";      w = "H28"; old = 187 },
  @{ n = "thousand";    d = "-DHP_THOUSAND_MOD=1";     w = "H36"; old = 193 },
  @{ n = "sqdepth";     d = "-DHP_SQDEPTH_MOD=1";      w = "H35"; old = 220 },
  @{ n = "boldline";    d = "-DHP_BOLDLINE_MOD=1";     w = "H38"; old = 222 },
  @{ n = "suffix";      d = "-DHP_SUFFIX_MOD=1";       w = "H25"; old = 230 },
  @{ n = "markdist";    d = "-DHP_MARKDIST_MOD=1";     w = "H26"; old = 233 },
  @{ n = "redir";       d = "-DHP_REDIR_MOD=1";        w = "H15"; old = 234 },
  @{ n = "fccur";       d = "-DHP_FCCUR_MOD=1";        w = "H37"; old = 242 },
  @{ n = "headidx";     d = "-DHP_HEADIDX_MOD=1";      w = "H31"; old = 246 },
  @{ n = "revcomment";  d = "-DHP_REVCOMMENT_MOD=1";   w = "H18"; old = 249 },
  @{ n = "secfrag";     d = "-DHP_SECFRAG_MOD=1";      w = "H37"; old = 249 },
  @{ n = "headbold";    d = "-DHP_HEADBOLD_MOD=1";     w = "H38"; old = 252 },
  @{ n = "month";       d = "-DHP_MONTH_MOD=1";        w = "H27"; old = 254 },
  @{ n = "commagap";    d = "-DHP_COMMAGAP_MOD=1";     w = "H28"; old = 256 },
  @{ n = "numrange";    d = "-DHP_NUMRANGE_MOD=1";     w = "H36"; old = 262 },
  @{ n = "dlterm";      d = "-DHP_DLTERM_MOD=1";       w = "H34"; old = 264 },
  @{ n = "isse";        d = "-DHP_ISSE_MOD=1";         w = "H16"; old = 265 },
  @{ n = "baridx";      d = "-DHP_BARIDX_MOD=1";       w = "H11"; old = 266 },
  @{ n = "temppos";     d = "-DHP_TEMPPOS_MOD=1";      w = "H21"; old = 266 },
  @{ n = "user";        d = "-DHP_USER_MOD=1";         w = "H17"; old = 267 },
  @{ n = "defaultsort"; d = "-DHP_DEFAULTSORT_MOD=1";  w = "H23"; old = 271 },
  @{ n = "coord";       d = "-DHP_COORD_MOD=1";        w = "H27"; old = 271 },
  @{ n = "digitgap";    d = "-DHP_DIGITGAP_MOD=1";     w = "H28"; old = 274 },
  @{ n = "sig";         d = "-DHP_SIG_MOD=1";          w = "H32"; old = 275 },
  @{ n = "seckind";     d = "-DHP_SECKIND_MOD=1";      w = "H27"; old = 276 },
  @{ n = "linelen";     d = "-DHP_LINELEN_MOD=1";      w = "H26"; old = 277 },
  @{ n = "linktrail";   d = "-DHP_LINKTRAIL_MOD=1";    w = "H31"; old = 280 },
  @{ n = "dumpredir";   d = "-DHP_DUMPREDIR_MOD=1";    w = "H18"; old = 281 },
  @{ n = "ns";          d = "-DHP_NS_MOD=1";           w = "H18"; old = 282 },
  @{ n = "include";     d = "-DHP_INCLUDE_MOD=1";      w = "H32"; old = 283 },
  @{ n = "brace3";      d = "-DHP_BRACE3_MOD=1";       w = "H32"; old = 285 },
  @{ n = "parserfn";    d = "-DHP_PARSERFN_MOD=1";     w = "H20"; old = 286 },
  @{ n = "urlpart";     d = "-DHP_URLPART_MOD=1";      w = "H32"; old = 286 },
  @{ n = "redirtarget"; d = "-DHP_REDIRTARGET_MOD=1";  w = "H23"; old = 290 },
  @{ n = "nowiki";      d = "-DHP_NOWIKI_MOD=1";       w = "H16"; old = 292 },
  @{ n = "gallery";     d = "-DHP_GALLERY_MOD=1";      w = "H27"; old = 292 },
  @{ n = "wikimodel";   d = "-DHP_WIKIMODEL_MOD=1";    w = "H19"; old = 292 },
  @{ n = "htmlfmt";     d = "-DHP_HTMLFMT_MOD=1";      w = "H31"; old = 297 },
  @{ n = "parast";      d = "-DHP_PARAST_MOD=1";       w = "H38"; old = 299 },
  @{ n = "refname";     d = "-DHP_REFNAME_MOD=1";      w = "H15"; old = 300 },
  @{ n = "fword";       d = "-DHP_FWORD_MOD=1";        w = "H24"; old = 304 },
  @{ n = "entity";      d = "-DHP_ENTITY_MOD=1";       w = "H15"; old = 309 },
  @{ n = "hatnote";     d = "-DHP_HATNOTE_MOD=1";      w = "H23"; old = 309 },
  @{ n = "pageid";      d = "-DHP_PAGEID_MOD=1";       w = "H17"; old = 311 },
  @{ n = "digitpos";    d = "-DHP_DIGITPOS_MOD=1";     w = "H28"; old = 322 },
  @{ n = "diglen";      d = "-DHP_DIGLEN_MOD=1";       w = "H29"; old = 324 },
  @{ n = "prevline";    d = "-DHP_PREVLINE_MOD=1";     w = "H29"; old = 326 },
  @{ n = "headclose";   d = "-DHP_HEADCLOSE_MOD=1";    w = "H34"; old = 330 },
  @{ n = "lowergap";    d = "-DHP_LOWERGAP_MOD=1";     w = "H28"; old = 333 },
  @{ n = "blank";       d = "-DHP_BLANK_MOD=1";        w = "H26"; old = 335 },
  @{ n = "decimal";     d = "-DHP_DECIMAL_MOD=1";      w = "H30"; old = 340 },
  @{ n = "minor";       d = "-DHP_MINOR_MOD=1";        w = "H19"; old = 353 },
  @{ n = "qocxt";       d = "-DHP_QOCXT_MOD=1";        w = "H15"; old = 363 },
  @{ n = "seclevel";    d = "-DHP_SECLEVEL_MOD=1";     w = "H32"; old = 379 },
  @{ n = "ip";          d = "-DHP_IP_MOD=1";           w = "H18"; old = 390 },
  @{ n = "slashgap";    d = "-DHP_SLASHGAP_MOD=1";     w = "H28"; old = 390 },
  @{ n = "pubid";       d = "-DHP_PUBID_MOD=1";        w = "H21"; old = 391 },
  @{ n = "lastlink";    d = "-DHP_LASTLINK_MOD=1";     w = "H24"; old = 392 },
  @{ n = "sprun";       d = "-DHP_SPRUN_MOD=1";        w = "H26"; old = 414 },
  @{ n = "hashp5";      d = "-DHP_HASH_P5=1";          w = "H14"; old = 423 },
  @{ n = "runlen";      d = "-DHP_RUNLEN_MOD=1";       w = "H26"; old = 432 },
  @{ n = "catsort";     d = "-DHP_CATSORT_MOD=1";      w = "H23"; old = 434 },
  @{ n = "prevsent";    d = "-DHP_PREVSENT_MOD=1";     w = "H29"; old = 442 },
  @{ n = "ordinal";     d = "-DHP_ORDINAL_MOD=1";      w = "H30"; old = 452 },
  @{ n = "linkcomma";   d = "-DHP_LINKCOMMA_MOD=1";    w = "H34"; old = 456 },
  @{ n = "hexrun";      d = "-DHP_HEXRUN_MOD=1";       w = "H35"; old = 457 },
  @{ n = "anchor";      d = "-DHP_ANCHOR_MOD=1";       w = "H21"; old = 461 },
  @{ n = "protocol";    d = "-DHP_PROTOCOL_MOD=1";     w = "H35"; old = 462 },
  @{ n = "wpos";        d = "-DHP_WPOS_MOD=1";         w = "H26"; old = 466 },
  @{ n = "catblock";    d = "-DHP_CATBLOCK_MOD=1";     w = "H34"; old = 467 },
  @{ n = "listmix";     d = "-DHP_LISTMIX_MOD=1";      w = "H35"; old = 467 },
  @{ n = "namedarg";    d = "-DHP_NAMEDARG_MOD=1";     w = "H32"; old = 470 },
  @{ n = "text";        d = "-DHP_TEXT_MOD=1";         w = "H17"; old = 473 },
  @{ n = "unit";        d = "-DHP_UNIT_MOD=1";         w = "H30"; old = 476 },
  @{ n = "lang";        d = "-DHP_LANG_MOD=1";         w = "H23"; old = 478 },
  @{ n = "pxsize";      d = "-DHP_PXSIZE_MOD=1";       w = "H33"; old = 478 },
  @{ n = "tblrow";      d = "-DHP_TBLROW_MOD=1";       w = "H23"; old = 478 },
  @{ n = "tpllen";      d = "-DHP_TPLLEN_MOD=1";       w = "H29"; old = 479 },
  @{ n = "magic";       d = "-DHP_MAGIC_MOD=1";        w = "H16"; old = 480 },
  @{ n = "tblcol";      d = "-DHP_TBLCOL_MOD=1";       w = "H31"; old = 480 },
  @{ n = "colring";     d = "-DHP_COLRING_MOD=1";      w = "H37"; old = 486 },
  @{ n = "listlevel";   d = "-DHP_LISTLEVEL_MOD=1";    w = "H16"; old = 488 },
  @{ n = "charcls";     d = "-DHP_CHARCLS_MOD=1";      w = "H25"; old = 501 },
  @{ n = "tbldepth";    d = "-DHP_TBLDEPTH_MOD=1";     w = "H33"; old = 503 },
  @{ n = "fileopt";     d = "-DHP_FILEOPT_MOD=1";      w = "H23"; old = 508 },
  @{ n = "paralen";     d = "-DHP_PARALEN_MOD=1";      w = "H29"; old = 511 },
  @{ n = "paren";       d = "-DHP_PAREN_MOD=1";        w = "H24"; old = 517 },
  @{ n = "tableclass";  d = "-DHP_TABLECLASS_MOD=1";   w = "H20"; old = 517 },
  @{ n = "extlink";     d = "-DHP_EXTLINK_MOD=1";      w = "H15"; old = 521 },
  @{ n = "lead";        d = "-DHP_LEAD_MOD=1";         w = "H31"; old = 524 },
  @{ n = "linklen";     d = "-DHP_LINKLEN_MOD=1";      w = "H29"; old = 525 },
  @{ n = "style";       d = "-DHP_STYLE_MOD=1";        w = "H27"; old = 532 },
  @{ n = "cellkind";    d = "-DHP_CELLKIND_MOD=1";     w = "H31"; old = 534 },
  @{ n = "infobox";     d = "-DHP_INFOBOX_MOD=1";      w = "H31"; old = 535 },
  @{ n = "contr";       d = "-DHP_CONTR_MOD=1";        w = "H25"; old = 542 },
  @{ n = "fontcol";     d = "-DHP_FONTCOL_MOD=1";      w = "H33"; old = 554 },
  @{ n = "httphost";    d = "-DHP_HTTPHOST_MOD=1";     w = "H24"; old = 554 },
  @{ n = "hyphen";      d = "-DHP_HYPHEN_MOD=1";       w = "H25"; old = 555 },
  @{ n = "dab";         d = "-DHP_DAB_MOD=1";          w = "H23"; old = 557 },
  @{ n = "init";        d = "-DHP_INIT_MOD=1";         w = "H30"; old = 579 },
  @{ n = "extdisp";     d = "-DHP_EXTDISP_MOD=1";      w = "H33"; old = 581 },
  @{ n = "listpos";     d = "-DHP_LISTPOS_MOD=1";      w = "H24"; old = 589 },
  @{ n = "listpara";    d = "-DHP_LISTPARA_MOD=1";     w = "H37"; old = 593 },
  @{ n = "linkns";      d = "-DHP_LINKNS_MOD=1";       w = "H37"; old = 597 },
  @{ n = "infoval";     d = "-DHP_INFOVAL_MOD=1";      w = "H31"; old = 601 },
  @{ n = "prespace";    d = "-DHP_PRESPACE_MOD=1";     w = "H33"; old = 612 },
  @{ n = "indent";      d = "-DHP_INDENT_MOD=1";       w = "H16"; old = 613 },
  @{ n = "piperole";    d = "-DHP_PIPEROLE_MOD=1";     w = "H35"; old = 615 },
  @{ n = "qperiod";     d = "-DHP_QPERIOD_MOD=1";      w = "H36"; old = 616 },
  @{ n = "tagname";     d = "-DHP_TAGNAME_MOD=1";      w = "H27"; old = 625 },
  @{ n = "tokencls";    d = "-DHP_TOKENCLS_MOD=1";     w = "H25"; old = 627 },
  @{ n = "headword";    d = "-DHP_HEADWORD_MOD=1";     w = "H30"; old = 633 },
  @{ n = "tagdist";     d = "-DHP_TAGDIST_MOD=1";      w = "H26"; old = 634 },
  @{ n = "celltxt";     d = "-DHP_CELLTXT_MOD=1";      w = "H24"; old = 639 },
  @{ n = "abbrev";      d = "-DHP_ABBREV_MOD=1";       w = "H36"; old = 641 },
  @{ n = "titleword";   d = "-DHP_TITLEWORD_MOD=1";    w = "H30"; old = 645 },
  @{ n = "splen";       d = "-DHP_SPLEN_MOD=1";        w = "H29"; old = 651 },
  @{ n = "shape";       d = "-DHP_SHAPE_MOD=1";        w = "H25"; old = 655 },
  @{ n = "alnumlen";    d = "-DHP_ALNUMLEN_MOD=1";     w = "H29"; old = 672 },
  @{ n = "citekind";    d = "-DHP_CITEKIND_MOD=1";     w = "H27"; old = 676 },
  @{ n = "caseflip";    d = "-DHP_CASEFLIP_MOD=1";     w = "H30"; old = 682 },
  @{ n = "repeat";      d = "-DHP_REPEAT_MOD=1";       w = "H30"; old = 684 },
  @{ n = "colspan";     d = "-DHP_COLSPAN_MOD=1";      w = "H27"; old = 688 },
  @{ n = "prefix";      d = "-DHP_PREFIX_MOD=1";       w = "H25"; old = 711 }
)

function Get-HpBusy {
  Get-Process -ErrorAction SilentlyContinue | Where-Object {
    $_.ProcessName -like "hp_g_*" -or $_.ProcessName -like "hp_v*"
  }
}

function Ensure-Slice {
  $dst = $InSlice
  if ((Test-Path $dst) -and ((Get-Item $dst).Length -eq 2097152)) { return }
  $src8 = Join-Path $root "data\enwik8.8mb"
  $srcFull = Join-Path $root "data\enwik8"
  $from = $null
  if (Test-Path $src8) { $from = $src8 }
  elseif (Test-Path $srcFull) { $from = $srcFull }
  else { throw "missing data/enwik8.8mb and data/enwik8" }
  $fs = [IO.File]::OpenRead($from)
  try {
    $out = [IO.File]::Create($dst)
    try {
      $buf = New-Object byte[] 2097152
      $n = $fs.Read($buf, 0, 2097152)
      $out.Write($buf, 0, $n)
    } finally { $out.Dispose() }
  } finally { $fs.Dispose() }
  Write-Host "CREATED $dst from $from"
}

function Get-ScreenFlags {
  param($extraFlag, $replaceFlag)
  $flags = New-Object System.Collections.Generic.List[string]
  foreach ($f in $script:V78Flags) { [void]$flags.Add([string]$f) }
  $replaced = $false
  $drop = New-Object System.Collections.Generic.List[int]
  for ($i = 0; $i -lt $flags.Count; $i++) {
    if ($flags[$i] -match '^-DHP_SLOT_MAX=') { [void]$drop.Add($i); continue }
    if ($replaceFlag -and $flags[$i] -eq $replaceFlag) {
      $flags[$i] = $extraFlag
      $replaced = $true
    }
  }
  for ($j = $drop.Count - 1; $j -ge 0; $j--) { $flags.RemoveAt($drop[$j]) }
  if ($extraFlag) {
    if ($replaceFlag) {
      if (-not $replaced) { [void]$flags.Add($extraFlag) }
    } else {
      [void]$flags.Add($extraFlag)
    }
  }
  return , $flags.ToArray()
}

function Get-ScreenExe([string]$jobName) {
  if ($jobName -in @("v83", "v84", "v85", "v86", "v87", "v88", "v89", "v90", "v91", "v92", "v93")) { throw "refusing champ name hp_s_$jobName.exe" }
  return (Join-Path $BuildDir "hp_s_$jobName.exe")
}

function Compile-Screen {
  param($jobName, $extraFlag, $replaceFlag)
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  $exe = Get-ScreenExe $jobName
  $protected = @(
    (Join-Path $BuildDir "hp_v83.exe"),
    (Join-Path $BuildDir "hp_v84.exe"),
    (Join-Path $BuildDir "hp_v85.exe"),
    (Join-Path $BuildDir "hp_v86.exe"),
    (Join-Path $BuildDir "hp_v87.exe"),
    (Join-Path $BuildDir "hp_v88.exe"),
    (Join-Path $BuildDir "hp_v89.exe"),
    (Join-Path $BuildDir "hp_v90.exe"),
    (Join-Path $BuildDir "hp_v91.exe"),
    (Join-Path $BuildDir "hp_v92.exe"),
    (Join-Path $BuildDir "hp_v93.exe")
  )
  foreach ($p in $protected) {
    if ([IO.Path]::GetFullPath($exe) -eq [IO.Path]::GetFullPath($p)) {
      throw "refusing to overwrite $p"
    }
  }
  $flags = Get-ScreenFlags $extraFlag $replaceFlag
  $hasLr = $false
  $lrVal = "?"
  foreach ($f in $flags) {
    if ($f -match '^-DHP_LR1_SCALE=(.+)$') { $hasLr = $true; $lrVal = $Matches[1] }
  }
  if (-not $hasLr) { throw "HP_LR1_SCALE missing from flags" }
  Write-Host "COMPILE hp_s_$jobName.exe extra=$extraFlag SLOT_MAX=22(hard) LR1=$lrVal"
  & g++ @flags $Src -o $exe
  if ($LASTEXITCODE -ne 0) { throw "compile failed $jobName" }
  return $exe
}

function Write-ScreenLog {
  param($jobName, $bytes, $delta, $wallS, $note)
  New-Item -ItemType Directory -Force -Path $LabDir | Out-Null
  if (-not (Test-Path $LogCsv)) {
    "name,bytes,delta_vs_base,wall_s,note,utc" | Set-Content -Encoding utf8 $LogCsv
  }
  $line = "{0},{1},{2},{3:N1},{4},{5}" -f $jobName, $bytes, $delta, $wallS, $note, ([DateTime]::UtcNow.ToString("o"))
  Add-Content -Encoding utf8 $LogCsv $line
}

function Encode-Screen {
  param($jobName)
  Ensure-Slice
  New-Item -ItemType Directory -Force -Path $LabDir | Out-Null
  $exe = Get-ScreenExe $jobName
  if (-not (Test-Path $exe)) { throw "missing $exe" }
  $arc = Join-Path $LabDir "s_$jobName.hp"
  Write-Host "ENCODE $exe c --mem 22 $InSlice $arc"
  $sw = [Diagnostics.Stopwatch]::StartNew()
  & $exe c --mem 22 $InSlice $arc
  $sw.Stop()
  if ($LASTEXITCODE -ne 0) { throw "encode failed $jobName" }
  $bytes = (Get-Item $arc).Length
  Write-Host ("BYTES {0} {1}  {2:N1}s" -f $jobName, $bytes, $sw.Elapsed.TotalSeconds)
  return @{ bytes = $bytes; wall = $sw.Elapsed.TotalSeconds; arc = $arc }
}

function Test-EncodeAllowed {
  if ($ForceEncode) { return $true }
  $busy = @(Get-HpBusy)
  if ($busy.Count -eq 0) { return $true }
  Write-Host "SKIP encode: hp leftover already running:"
  $busy | ForEach-Object {
    Write-Host ("  {0} pid={1} rss={2:N1} GB" -f $_.ProcessName, $_.Id, ($_.WorkingSet64 / 1GB))
  }
  return $false
}

function Show-Queue {
  Write-Host ("QUEUE {0} leftover flags  SLOT_MAX=22(hard)  LR1_SCALE=40  slice={1}" -f $RejectRows.Count, $InSlice)
  $i = 0
  foreach ($j in $RejectRows) {
    $i++
    $old = if ($null -eq $j.old) { "queued" } else { "+$($j.old)" }
    $rep = if ($j.r) { " replace=$($j.r)" } else { "" }
    Write-Host ("  {0,3} {1,-12} {2,-8} {3,-8} {4}{5}" -f $i, $j.n, $j.w, $old, $j.d, $rep)
  }
}

if ($List) {
  Show-Queue
  exit 0
}

Ensure-Slice
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $LabDir | Out-Null

$doEncode = -not $CompileOnly
if ($doEncode -and -not (Test-EncodeAllowed)) { $doEncode = $false }

if ($RunQueue) {
  Write-Host "BASELINE compile"
  Compile-Screen "base" $null $null | Out-Null
  $baseBytes = $null
  if ($doEncode) {
    $br = Encode-Screen "base"
    $baseBytes = $br.bytes
    Write-ScreenLog "base" $baseBytes 0 $br.wall "baseline"
  }
  $n = 0
  foreach ($j in $RejectRows) {
    if ($Limit -gt 0 -and $n -ge $Limit) { break }
    Compile-Screen $j.n $j.d $j.r | Out-Null
    if ($doEncode) {
      $er = Encode-Screen $j.n
      $delta = if ($null -ne $baseBytes) { $er.bytes - $baseBytes } else { "" }
      Write-Host ("DELTA {0} {1}" -f $j.n, $delta)
      Write-ScreenLog $j.n $er.bytes $delta $er.wall $j.w
    }
    $n++
  }
  Write-Host ("DONE queue jobs={0} encoded={1}" -f $n, $doEncode)
  exit 0
}

if ($Name) {
  if ($Name -eq "base") {
    Compile-Screen "base" $null $null | Out-Null
    if ($doEncode) {
      $br = Encode-Screen "base"
      Write-ScreenLog "base" $br.bytes 0 $br.wall "baseline"
    }
  } else {
    $job = $RejectRows | Where-Object { $_.n -eq $Name } | Select-Object -First 1
    if (-not $job) { throw "unknown leftover name '$Name' (use -List)" }
    Compile-Screen $job.n $job.d $job.r | Out-Null
    if ($doEncode) {
      $er = Encode-Screen $job.n
      Write-ScreenLog $job.n $er.bytes "" $er.wall $job.w
    }
  }
  exit 0
}

# Default harness: baseline only. Never a leftover marathon.
Write-Host ("DEFAULT baseline  queue={0}  SLOT_MAX=22(hard)" -f $RejectRows.Count)
Compile-Screen "base" $null $null | Out-Null
if ($doEncode) {
  $br = Encode-Screen "base"
  Write-ScreenLog "base" $br.bytes 0 $br.wall "baseline"
  Write-Host ("BASELINE {0} bytes" -f $br.bytes)
} else {
  Write-Host "BASELINE compiled; encode skipped"
}
Write-Host ("QUEUE {0} leftover flags listed in this script" -f $RejectRows.Count)
