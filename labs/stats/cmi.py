import numpy as np, sys

NSTAT, NPBIN, NBINS = 39, 33, 16
names = ["ctl_bitpos","ctl_c0hi","ctl_prevclass","ctl_xmldepth","ctl_wordpos","ctl_matchlen",
 "disp_var","disp_range","disp_maxabs","disp_agreefrac","disp_signsum","disp_top2gap",
 "xo_agree26","xo_diff62","xo_argmaxord",
 "rec_last4","rec_branch3","st_cnt6","st_shape6",
 "sur_fast","sur_slow","sur_delta","sur_switch","sur_prevbyte",
 "mm_break","mm_agree2","mm_len2","NULL_rand","NULL_rand2",
 "gt_hapax3","gt_hapax4","kt_gap6","fs_entropy","fs_maxw",
 "fs_switch","bocpd_rl","bocpd_cp","ada_gap","cal_resid"]

fn = sys.argv[1] if len(sys.argv)>1 else "hist10.bin"
h = np.fromfile(fn, dtype=np.uint32).reshape(NSTAT,NPBIN,NBINS,2).astype(np.float64)

LN2 = np.log(2.0)
def ent_bits(c0, c1):
    n = c0+c1
    out = np.zeros_like(n)
    m = n>0
    p0 = np.where(m, c0/np.maximum(n,1), 0.5)
    p1 = np.where(m, c1/np.maximum(n,1), 0.5)
    t = np.zeros_like(n)
    for p in (p0,p1):
        t = t + np.where(p>0, -p*np.log2(np.maximum(p,1e-300)), 0.0)
    return np.where(m, t, 0.0)

rows=[]
for s in range(NSTAT):
    H = h[s]                      # [pbin][sbin][bit]
    N = H.sum()
    # marginal over S, per pbin
    per_p = H.sum(axis=1)         # [pbin][bit]
    n_p   = per_p.sum(axis=1)     # [pbin]
    Hp    = ent_bits(per_p[:,0], per_p[:,1])
    H_given_p = float((n_p*Hp).sum()/N)

    n_ps = H.sum(axis=2)          # [pbin][sbin]
    Hps  = ent_bits(H[:,:,0], H[:,:,1])
    H_given_ps = float((n_ps*Hps).sum()/N)

    cmi_plug = H_given_p - H_given_ps

    # Miller-Madow: nonempty-cell counts
    m_p  = ((per_p>0).sum(axis=1) * (n_p>0)).sum()
    m_ps = ((H>0).sum(axis=2) * (n_ps>0)).sum()
    n_pbins  = int((n_p>0).sum())
    n_pscell = int((n_ps>0).sum())
    corr = ((m_p - n_pbins) - (m_ps - n_pscell)) / (2*N*LN2)
    cmi_mm = cmi_plug + corr

    rows.append((names[s], cmi_plug, cmi_mm, cmi_mm*8, n_pscell))

null_floor = max(r[3] for r in rows if r[0].startswith("NULL"))

print(f"N bits = {h[0].sum():,.0f}   baseline = 1.78585 bpc")
print(f"empirical null floor (bpc-equiv) = {null_floor:.5f}\n")
print(f"{'statistic':<16}{'CMI(bits/bit)':>14}{'bpc-equiv':>12}{'x null':>9}  cells")
print("-"*62)
for name,cp,cm,bpc,cells in sorted(rows,key=lambda r:-r[3]):
    ratio = bpc/null_floor if null_floor>0 else float('inf')
    tag = "  <-- NULL" if name.startswith("NULL") else ""
    print(f"{name:<16}{cm:>14.6f}{bpc:>12.5f}{ratio:>9.1f}  {cells:>5}{tag}")
