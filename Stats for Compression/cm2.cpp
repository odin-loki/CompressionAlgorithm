// cm2.cpp - lpaq-class CM: nibble-bucket hash tables, logistic mixer, SSE chain,
//           match models, + instrumentation for candidate-statistic screening.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;
typedef uint8_t U8; typedef uint16_t U16; typedef uint32_t U32; typedef uint64_t U64;

static int squash(int d){
  static const int t[33]={1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,1546,2047,
    2549,2994,3348,3607,3785,3901,3975,4024,4050,4068,4079,4085,4089,4092,4093,4094};
  if(d>2047)return 4095; if(d<-2047)return 0;
  int w=d&127; d=(d>>7)+16;
  return (t[d]*(128-w)+t[d+1]*w+64)>>7;
}
static short stretch_tab[4096];
static void init_stretch(){
  int pi=0;
  for(int x=-2047;x<=2047;++x){ int v=squash(x); for(int j=pi;j<=v;++j) stretch_tab[j]=(short)x; pi=v+1; }
  for(int j=pi;j<4096;++j) stretch_tab[j]=2047;
}
static inline int stretch(int p){ return stretch_tab[p]; }

struct Slot { U16 p; U8 n; U8 chk; };
struct Bucket { Slot s[16]; };   // 64 bytes; s[0].chk = bucket checksum, nodes 1..15
static U32 ratemul[64];
static void init_rate(){ for(int i=0;i<64;++i) ratemul[i]=(U32)(65536.0/(i+1.6)); }
static inline void updSlot(Slot&s,int bit,int limit){
  int target=bit?65535:0;
  s.p=(U16)((int)s.p+(int)(((int64_t)(target-(int)s.p)*ratemul[s.n])>>16));
  if(s.n<limit) s.n++;
}

struct StateMap {
  vector<U32> t;
  StateMap(int n=0){ t.assign(n,1u<<31); }
  inline int pr(int cx){ return t[cx]>>20; }
  inline void upd(int cx,int bit,int lim=1023){
    U32 &v=t[cx]; int n=v&1023; int p=(int)(v>>10);
    if(n<lim) ++v;
    int delta=((bit<<22)-p)/(n+2);
    v += ((U32)delta)<<10;
  }
};

struct APM {
  vector<U16> t; int idx;
  APM(int n=0){ t.resize((size_t)n*33);
    for(int i=0;i<n;++i) for(int j=0;j<33;++j) t[(size_t)i*33+j]=(U16)(squash((j-16)*128)*16); }
  inline int pp(int pr,int cx){
    int st=(stretch(pr)+2048)*32; int w=st&4095;
    idx=(st>>12)+cx*33;
    return (t[idx]*(4096-w)+t[idx+1]*w)>>16;
  }
  inline void upd(int bit,int rate=7){
    int g=(bit<<16)+(bit<<rate)-bit-bit;
    t[idx]=(U16)(t[idx]+((g-t[idx])>>rate));
    t[idx+1]=(U16)(t[idx+1]+((g-t[idx+1])>>rate));
  }
};

static inline U64 hmix(U64 x){ x*=0x9E3779B97F4A7C15ull; x^=x>>29; x*=0xBF58476D1CE4E5B9ull; x^=x>>32; return x; }

static const int NMODEL=6;                 // o1,o2,o3,o4,o6,word
static const int NIN=2+NMODEL*2+2+1;       // 17
static int BBITS=21;                       // buckets per model (2^21 * 64B = 128MB)

static vector<Bucket> btab[NMODEL];
static vector<Slot>   tab0;
static StateMap *sm[NMODEL], *sm0, *smm1, *smm2;
static U64 ctxhash[NMODEL];
static U8 *buf; static U64 bufsz;
static vector<U32> mtab1,mtab2,lastseen; static vector<U64> succmask;
static U32 mptr1=0,mlen1=0,mptr2=0,mlen2=0;
static vector<int> W; static int mixin[NIN];
static APM *a1,*a2,*gapm=nullptr;

static int GATE=-1, GATE2=-1, GATE3=-1, GATEMODE=0;
static int MIXLR=6;

static const int NSTAT=39;
static const char* statname[NSTAT]={
 "ctl_bitpos","ctl_c0hi","ctl_prevclass","ctl_xmldepth","ctl_wordpos","ctl_matchlen",
 "disp_var","disp_range","disp_maxabs","disp_agreefrac","disp_signsum","disp_top2gap",
 "xo_agree26","xo_diff62","xo_argmaxord",
 "rec_last4","rec_branch3","st_cnt6","st_shape6",
 "sur_fast","sur_slow","sur_delta","sur_switch","sur_prevbyte",
 "mm_break","mm_agree2","mm_len2","NULL_rand","NULL_rand2",
 "gt_hapax3","gt_hapax4","kt_gap6","fs_entropy","fs_maxw",
 "fs_switch","bocpd_rl","bocpd_cp","ada_gap","cal_resid"};
static const int NBINS=16, NPBIN=33;
static U32 (*hist)[NPBIN][NBINS][2];
static int statbin[NSTAT];
static bool COLLECT=false;

static int bitpos=0,c0=1,prevbyte=0,xmldepth=0,wordpos=0;
static double surp_fast=1.0,surp_slow=1.0,prev_byte_surp=0,cur_byte_surp=0;
static int prev_argmax=-1; static double switch_rate=0;
static int mm_break_age=0, recency_bin=15, branch_bin=0;
// Good-Turing: per-context masks of successors seen once vs more than once
static vector<U64> gt_once3, gt_more3, gt_once4, gt_more4;
static int gt_hapax3_bin=0, gt_hapax4_bin=0;
// Fixed-Share over the indirect model predictions (Herbster-Warmuth)
static const int NFS=7;
static double fsw[NFS];
static double FS_ETA=1.0, FS_ALPHA=0.02;
static int fs_ent_bin=8, fs_max_bin=8, fs_sw_bin=0, ada_gap_bin=0;
// BOCPD run-length posterior over byte-level surprisal (Adams-MacKay)
static const int NRL=12;
static double rlp[NRL], rl_a[NRL], rl_b[NRL];
static double BO_HAZ=1.0/128.0;
static int bocpd_rl_bin=0, bocpd_cp_bin=0;
// calibration residual per p-bin
static double calres[33];
static int cal_bin=8;

static inline int qlog(U32 v,int nb){ int r=0; while(v&&r<nb-1){v>>=1;++r;} return r; }
static inline int clampb(int v,int nb){ return v<0?0:(v>=nb?nb-1:v); }

int main(int argc,char**argv){
  const char* fn=argv[1];
  U64 limit=argc>2?strtoull(argv[2],0,10):10000000ull;
  COLLECT=argc>3&&atoi(argv[3])==1;
  GATE=argc>4?atoi(argv[4]):-1;
  GATEMODE=argc>5?atoi(argv[5]):0;
  const char* outhist=argc>6?argv[6]:"hist.bin";
  if(getenv("MIXLR")) MIXLR=atoi(getenv("MIXLR"));
  if(getenv("BBITS")) BBITS=atoi(getenv("BBITS"));
  if(getenv("GATE2")) GATE2=atoi(getenv("GATE2"));
  if(getenv("GATE3")) GATE3=atoi(getenv("GATE3"));

  init_stretch(); init_rate();
  FILE*f=fopen(fn,"rb"); if(!f){fprintf(stderr,"no file\n");return 1;}
  buf=(U8*)calloc(limit+64,1); bufsz=fread(buf,1,limit,f); fclose(f);

  U64 BS=1ull<<BBITS, BM=BS-1;
  for(int i=0;i<NMODEL;++i){ btab[i].assign(BS,Bucket{}); sm[i]=new StateMap(64*256); }
  for(int i=0;i<NMODEL;++i) for(U64 j=0;j<BS;++j) for(int k=0;k<16;++k) btab[i][j].s[k].p=32768;
  tab0.assign(256,Slot{32768,0,0}); sm0=new StateMap(64*256);
  mtab1.assign(1<<22,0); mtab2.assign(1<<22,0);
  smm1=new StateMap(64*2); smm2=new StateMap(64*2);
  lastseen.assign(1<<22,0); succmask.assign(1<<21,0);
  int wsets=256*8;
  int wmul = (GATEMODE==2||GATEMODE==3)?NBINS:((GATEMODE==4)?64:1);
  W.assign((size_t)wsets*wmul*NIN, 65536/NIN);
  a1=new APM(1024); a2=new APM(65536);
  if(GATEMODE==1) gapm=new APM(NBINS*256);

  gt_once3.assign(1<<21,0); gt_more3.assign(1<<21,0);
  gt_once4.assign(1<<21,0); gt_more4.assign(1<<21,0);
  for(int i=0;i<NFS;++i) fsw[i]=1.0/NFS;
  for(int i=0;i<NRL;++i){ rlp[i]=0; rl_a[i]=0.5; rl_b[i]=0.5; }
  rlp[0]=1.0;
  for(int i=0;i<33;++i) calres[i]=0;
  if(getenv("FS_ALPHA")) FS_ALPHA=atof(getenv("FS_ALPHA"));
  if(getenv("BO_HAZ"))   BO_HAZ=1.0/atof(getenv("BO_HAZ"));
  hist=(U32(*)[NPBIN][NBINS][2])calloc(NSTAT,sizeof(U32)*NPBIN*NBINS*2);

  double total_bits=0; U64 wordhash=0; int mpred1=-1,mpred2=-1;
  double icost[NIN]; for(int i=0;i<NIN;++i) icost[i]=0;
  bool DIAG = getenv("DIAG")!=nullptr;
  double mixcost=0;
  Bucket *bk[NMODEL]; Slot *slot[NMODEL],*slot0; int smidx[NMODEL];
  int st_model[NMODEL]; int argmax=0;

  for(U64 pos=0;pos<bufsz;++pos){
    if(pos>0){
      U64 b1=buf[pos-1],b2=pos>1?buf[pos-2]:0,b3=pos>2?buf[pos-3]:0;
      U64 b4=pos>3?buf[pos-4]:0,b5=pos>4?buf[pos-5]:0,b6=pos>5?buf[pos-6]:0;
      ctxhash[0]=hmix(b1+0x100);
      ctxhash[1]=hmix(b1+(b2<<8)+0x20000);
      ctxhash[2]=hmix(b1+(b2<<8)+(b3<<16)+0x3000000);
      ctxhash[3]=hmix(b1+(b2<<8)+(b3<<16)+(b4<<24)+0x40000000ull);
      ctxhash[4]=hmix(b1+(b2<<8)+(b3<<16)+(b4<<24)+(b5<<32)+(b6<<40)+0x6000ull);
    } else for(int i=0;i<5;++i) ctxhash[i]=hmix(i+1);
    ctxhash[5]=hmix(wordhash*0x9E3779B9ull+0x777);

    if(pos>=8){
      U64 h1=hmix((*(U64*)(buf+pos-6))&0xffffffffffffull)&((1<<22)-1);
      U64 h2=hmix((*(U64*)(buf+pos-8))^0xABCDull)&((1<<22)-1);
      if(mlen1>0&&mptr1<pos&&buf[mptr1]==buf[pos-1]){ ++mptr1; if(mlen1<65535)++mlen1; }
      else { mlen1=0; U32 c=mtab1[h1]; if(c>0&&c<pos){ mptr1=c; U32 l=0; while(l<32&&c>l&&buf[c-l-1]==buf[pos-l-1])++l; mlen1=l; } }
      mtab1[h1]=(U32)pos;
      if(mlen2>0&&mptr2<pos&&buf[mptr2]==buf[pos-1]){ ++mptr2; if(mlen2<65535)++mlen2; }
      else { mlen2=0; U32 c=mtab2[h2]; if(c>0&&c<pos){ mptr2=c; U32 l=0; while(l<32&&c>l&&buf[c-l-1]==buf[pos-l-1])++l; mlen2=l; } }
      mtab2[h2]=(U32)pos;
      mpred1=mlen1>0?buf[mptr1]:-1; mpred2=mlen2>0?buf[mptr2]:-1;
      U64 hr=hmix((*(U64*)(buf+pos-4))&0xffffffffull)&((1<<22)-1);
      U32 ls=lastseen[hr]; recency_bin=ls?clampb(qlog((U32)(pos-ls),16),16):15;
      lastseen[hr]=(U32)pos;
      // query: distinct successors seen for the order-3 context preceding this byte
      U64 hbq=hmix(((*(U64*)(buf+pos-3))&0xffffffull)+9)&((1<<21)-1);
      branch_bin=clampb(__builtin_popcountll(succmask[hbq]),16);
      // update: the PREVIOUS order-3 context, whose true successor is buf[pos-1]
      U64 hbu=hmix(((*(U64*)(buf+pos-4))&0xffffffull)+9)&((1<<21)-1);
      succmask[hbu]|=(1ull<<(buf[pos-1]&63));
      // --- Good-Turing hapax fraction: N1/N over distinct successors ---
      {
        U64 q3=hmix(((*(U64*)(buf+pos-3))&0xffffffull)+77)&((1<<21)-1);
        int n1=__builtin_popcountll(gt_once3[q3]);
        int nm=__builtin_popcountll(gt_more3[q3]);
        gt_hapax3_bin = (n1+nm)? clampb(n1*15/(n1+nm),16) : 15;
        U64 q4=hmix(((*(U64*)(buf+pos-4))&0xffffffffull)+177)&((1<<21)-1);
        int m1=__builtin_popcountll(gt_once4[q4]);
        int mm=__builtin_popcountll(gt_more4[q4]);
        gt_hapax4_bin = (m1+mm)? clampb(m1*15/(m1+mm),16) : 15;
        // update the PREVIOUS context with its observed successor buf[pos-1]
        U64 u3=hmix(((*(U64*)(buf+pos-4))&0xffffffull)+77)&((1<<21)-1);
        U64 bit3=1ull<<(buf[pos-1]&63);
        if(gt_once3[u3]&bit3){ gt_once3[u3]&=~bit3; gt_more3[u3]|=bit3; }
        else if(!(gt_more3[u3]&bit3)) gt_once3[u3]|=bit3;
        U64 u4=hmix(((*(U64*)(buf+pos-5))&0xffffffffull)+177)&((1<<21)-1);
        U64 bit4=1ull<<(buf[pos-1]&63);
        if(gt_once4[u4]&bit4){ gt_once4[u4]&=~bit4; gt_more4[u4]|=bit4; }
        else if(!(gt_more4[u4]&bit4)) gt_once4[u4]|=bit4;
      }
    } else { recency_bin=15; branch_bin=0; }
    mm_break_age = mlen1>0? min(mm_break_age+1,255):0;

    c0=1; cur_byte_surp=0;
    int actual=buf[pos];
    int hi_nib=0;

    for(int bp=7;bp>=0;--bp){
      int bit=(actual>>bp)&1; bitpos=7-bp;

      // fetch buckets at nibble boundaries
      if(bp==7||bp==3){
        for(int m=0;m<NMODEL;++m){
          U64 h = (bp==7)? ctxhash[m] : hmix(ctxhash[m]*3+hi_nib+0x51ull);
          U64 bi = h & BM;
          Bucket *b=&btab[m][bi];
          U8 chk=(U8)(h>>56);
          if(b->s[0].chk!=chk){
            b->s[0].chk=chk;
            for(int k=1;k<16;++k){ b->s[k].p=32768; b->s[k].n=0; }
          }
          bk[m]=b;
        }
      }
      int node = (bp>=4)? (c0) : (c0>>4? (c0&15)|((c0>>4)?0:0) : c0);
      // node index within nibble tree: rebuild from partial bits of current nibble
      {
        int nb = (bp>=4)? (7-bp) : (3-bp);       // bits done in this nibble
        int val = (bp>=4)? (c0 & ((1<<nb)-1)) : (c0 & ((1<<nb)-1));
        node = (1<<nb) | val;
      }

      slot0=&tab0[c0&255];
      int p0=slot0->p>>4;
      int s0state=((slot0->p>>12)&15)|((min((int)slot0->n,15)/4)<<4);
      int i0=s0state*256+(c0&255);
      mixin[0]=stretch(p0); mixin[1]=stretch(sm0->pr(i0));

      int k=2;
      for(int m=0;m<NMODEL;++m){
        Slot*s=&bk[m]->s[node]; slot[m]=s;
        int pd=s->p>>4;
        int state=((s->p>>12)&15)|((min((int)s->n,15)/4)<<4);
        int ii=state*256+(c0&255); smidx[m]=ii;
        int si=stretch(sm[m]->pr(ii));
        st_model[m]=si;
        mixin[k++]=stretch(pd); mixin[k++]=si;
      }

      auto expbit=[&](int pred)->int{
        if(pred<0) return -1;
        int shifted=(pred>>(bp+1))|(1<<(7-bp));
        if(shifted!=c0) return -1;
        return (pred>>bp)&1;
      };
      int mexp1=expbit(mpred1), mexp2=expbit(mpred2);
      int mi1=mexp1<0?0:(min((int)mlen1,31)*2+mexp1);
      int mi2=mexp2<0?0:(min((int)mlen2,31)*2+mexp2);
      mixin[k++]=mexp1<0?0:stretch(smm1->pr(mi1));
      mixin[k++]=mexp2<0?0:stretch(smm2->pr(mi2));
      mixin[k++]=256;

      // dispersion / argmax (always computed; needed by gates and stats)
      int vals[NMODEL+1]; int nv=0;
      vals[nv++]=mixin[1];
      for(int m=0;m<NMODEL;++m) vals[nv++]=st_model[m];
      double mean=0; for(int i=0;i<nv;++i) mean+=vals[i]; mean/=nv;
      double var=0; for(int i=0;i<nv;++i){ double d=vals[i]-mean; var+=d*d; } var/=nv;
      int mn=vals[0],mx=vals[0],mxa=0,pos_=0,neg_=0,av1=0,av2=0;
      for(int i=0;i<nv;++i){ mn=min(mn,vals[i]); mx=max(mx,vals[i]);
        int av=abs(vals[i]); if(av>av1){av2=av1;av1=av;} else if(av>av2)av2=av;
        mxa=max(mxa,av); if(vals[i]>0)++pos_; else if(vals[i]<0)++neg_; }
      { int bestv=-1; argmax=0; for(int i=0;i<nv;++i) if(abs(vals[i])>bestv){bestv=abs(vals[i]);argmax=i;} }

      int mixsel=((c0&255)*8+min((int)mlen1,7));

      // statistics
      {
        statbin[0]=bitpos;
        statbin[1]=clampb((c0&255)>>4,16);
        int pc=prevbyte;
        int pcl=((pc>='a'&&pc<='z')||(pc>='A'&&pc<='Z'))?0:(pc>='0'&&pc<='9')?1:(pc==' ')?2:
                (pc=='\n')?3:(pc=='<'||pc=='>'||pc=='/')?4:(pc=='['||pc==']'||pc=='{'||pc=='}')?5:
                (pc=='='||pc=='|')?6:7;
        statbin[2]=pcl;
        statbin[3]=clampb(xmldepth,16);
        statbin[4]=clampb(wordpos,16);
        statbin[5]=clampb(qlog(mlen1+1,16),16);
        statbin[6]=clampb(qlog((U32)(var/64),16),16);
        statbin[7]=clampb(qlog((U32)((mx-mn)/16),16),16);
        statbin[8]=clampb(qlog((U32)(mxa/8),16),16);
        int sgn=0; // sign of provisional consensus = sign of mean
        sgn = mean>0?1:(mean<0?-1:0);
        int agree=0; for(int i=0;i<nv;++i){ int s2=vals[i]>0?1:(vals[i]<0?-1:0); if(s2==sgn)++agree; }
        statbin[9]=clampb(agree*16/(nv+1),16);
        statbin[10]=clampb(pos_-neg_+8,16);
        statbin[11]=clampb(qlog((U32)((av1-av2)/8),16),16);
        int o2=st_model[1],o6=st_model[4];
        statbin[12]=((o2>0)==(o6>0)?8:0)+clampb(qlog((U32)(abs(o2-o6)/32),8),8);
        statbin[13]=clampb((o6-o2)/128+8,16);
        statbin[14]=clampb(argmax,16);
        statbin[15]=recency_bin;
        statbin[16]=branch_bin;
        statbin[17]=clampb(min((int)slot[4]->n,63)/4,16);
        statbin[18]=clampb(((slot[4]->p>>13)&7)*2+(slot[4]->n>8?1:0),16);
        statbin[19]=clampb((int)(surp_fast*4),16);
        statbin[20]=clampb((int)(surp_slow*4),16);
        statbin[21]=clampb((int)((surp_fast-surp_slow)*4)+8,16);
        statbin[22]=clampb((int)(switch_rate*16),16);
        statbin[23]=clampb((int)prev_byte_surp,16);
        statbin[24]=clampb(qlog(mm_break_age+1,16),16);
        statbin[25]=(mexp1>=0&&mexp2>=0)?(mexp1==mexp2?2:1):(mexp1>=0?3:(mexp2>=0?4:0));
        statbin[26]=clampb(qlog(mlen2+1,16),16);
        // null controls: independent of the bit by construction
        statbin[27]=(int)((hmix(pos*8+bitpos+0xF00Dull)>>13)&15);
        statbin[28]=(int)((hmix(pos*8+bitpos+0xBEEFull)>>21)&7);
        statbin[29]=gt_hapax3_bin;
        statbin[30]=gt_hapax4_bin;
        // KT (Jeffreys) estimate from the order-6 slot vs its adaptive value
        {
          Slot*s6=slot[4];
          double pv=s6->p/65536.0; int nn=s6->n;
          double bb=pv*nn, aa=nn-bb;
          double kt=(bb+0.5)/(nn+1.0);
          int gap=stretch(clampb((int)(kt*4096),4096))-stretch(s6->p>>4);
          statbin[31]=clampb(gap/64+8,16);
          (void)aa;
        }
        statbin[32]=fs_ent_bin;
        statbin[33]=fs_max_bin;
        statbin[34]=fs_sw_bin;
        statbin[35]=bocpd_rl_bin;
        statbin[36]=bocpd_cp_bin;
        statbin[37]=ada_gap_bin;
        statbin[38]=cal_bin;
      }

      int gsel=0;
      if(GATEMODE==2&&GATE>=0) gsel=statbin[GATE];
      else if(GATEMODE==3&&GATE>=0&&GATE2>=0) gsel=((statbin[GATE]>>2)<<2)|(statbin[GATE2]>>2);
      else if(GATEMODE==4&&GATE>=0&&GATE2>=0&&GATE3>=0)
        gsel=((statbin[GATE]>>2)<<4)|((statbin[GATE2]>>2)<<2)|(statbin[GATE3]>>2);
      size_t woff=(size_t)((GATEMODE==2||GATEMODE==3)?(mixsel*NBINS+gsel):
                           (GATEMODE==4?(mixsel*64+gsel):mixsel))*NIN;
      int64_t dot=0; for(int i=0;i<NIN;++i) dot+=(int64_t)W[woff+i]*mixin[i];
      int mst=(int)(dot>>16); if(mst>2047)mst=2047; if(mst<-2047)mst=-2047;
      int mixp=squash(mst);

      int pa=a1->pp(mixp,(c0&255)*4+min((int)mlen1,3));
      int pb=a2->pp(mixp,(c0&255)*256+prevbyte);
      int pr=(mixp+pa+2*pb+2)>>2;
      if(pr<1)pr=1; if(pr>4094)pr=4094;

      if(GATEMODE==1&&GATE>=0){
        int pg=gapm->pp(pr,statbin[GATE]*256+(c0&255));
        pr=(pr+3*pg+2)>>2;
        if(pr<1)pr=1; if(pr>4094)pr=4094;
      }

      if(DIAG){
        for(int i=0;i<NIN;++i){ int q=squash(mixin[i]); if(q<1)q=1; if(q>4095)q=4095;
          double pv=q/4096.0; icost[i]+=-log2(bit?pv:1.0-pv); }
        { double pv=mixp/4096.0; if(pv<1e-4)pv=1e-4; if(pv>1-1e-4)pv=1-1e-4;
          mixcost+=-log2(bit?pv:1.0-pv); }
      }
      double ppv=pr/4096.0;
      double cost=-log2(bit?ppv:1.0-ppv);
      total_bits+=cost; cur_byte_surp+=cost;

      if(COLLECT){
        int pbin=clampb((stretch(pr)+2048)>>7,NPBIN);
        for(int s=0;s<NSTAT;++s) hist[s][pbin][clampb(statbin[s],NBINS)][bit]++;
      }

      if(GATEMODE==1&&GATE>=0) gapm->upd(bit);
      a1->upd(bit); a2->upd(bit);
      int err=((bit<<12)-mixp)*MIXLR;
      for(int i=0;i<NIN;++i){
        int nw=W[woff+i]+((mixin[i]*err)>>14);
        if(nw>(1<<22))nw=(1<<22); if(nw<-(1<<22))nw=-(1<<22);
        W[woff+i]=nw;
      }
      sm0->upd(i0,bit); updSlot(*slot0,bit,60);
      for(int m=0;m<NMODEL;++m){ sm[m]->upd(smidx[m],bit); updSlot(*slot[m],bit,m>=3?30:60); }
      if(mexp1>=0) smm1->upd(mi1,bit);
      if(mexp2>=0) smm2->upd(mi2,bit);

      // ---- Fixed-Share (Herbster-Warmuth) over indirect model predictions ----
      {
        double pi_[NFS]; double mixloss=0, exploss=0, Z=0;
        pi_[0]=squash(mixin[1])/4096.0;
        for(int m=0;m<NMODEL;++m) pi_[m+1]=squash(st_model[m])/4096.0;
        for(int i=0;i<NFS;++i){
          double q=bit?pi_[i]:1.0-pi_[i];
          if(q<1e-6)q=1e-6; if(q>1-1e-6)q=1-1e-6;
          Z += fsw[i]*q;
          exploss += fsw[i]*(-log2(q));
        }
        mixloss = -log2(Z>1e-300?Z:1e-300);
        ada_gap_bin = clampb((int)((exploss-mixloss)*8),16);
        // multiplicative update
        double tot=0;
        for(int i=0;i<NFS;++i){
          double q=bit?pi_[i]:1.0-pi_[i];
          if(q<1e-6)q=1e-6; if(q>1-1e-6)q=1-1e-6;
          fsw[i]*=pow(q,FS_ETA);
          tot+=fsw[i];
        }
        if(tot<=0||!(tot==tot)){ for(int i=0;i<NFS;++i) fsw[i]=1.0/NFS; tot=1.0; }
        double share=FS_ALPHA*tot/NFS;
        double newtot=0;
        double best=0; int bi=0;
        for(int i=0;i<NFS;++i){
          fsw[i]=(1.0-FS_ALPHA)*fsw[i]+share;
          newtot+=fsw[i];
        }
        double H=0;
        for(int i=0;i<NFS;++i){
          fsw[i]/=newtot;
          if(fsw[i]>best){best=fsw[i];bi=i;}
          if(fsw[i]>1e-12) H+=-fsw[i]*log2(fsw[i]);
        }
        fs_ent_bin=clampb((int)(H*5),16);
        fs_max_bin=clampb((int)(best*16),16);
        // posterior mass of the winning expert that arrived via the share term
        double sw=(share/newtot)/(fsw[bi]>1e-12?fsw[bi]:1e-12);
        fs_sw_bin=clampb((int)(-log2(sw>1e-12?sw:1e-12)*1.5),16);
      }
      // ---- calibration residual for this p-bin ----
      {
        int pb=clampb((stretch(pr)+2048)>>7,33);
        cal_bin=clampb((int)(calres[pb]*40)+8,16);
        calres[pb]+=((bit?1.0:0.0)-pr/4096.0-calres[pb])*0.002;
      }
      surp_fast+=(cost-surp_fast)*0.06;
      surp_slow+=(cost-surp_slow)*0.004;
      switch_rate+=(((prev_argmax>=0&&argmax!=prev_argmax)?1.0:0.0)-switch_rate)*0.03;
      prev_argmax=argmax;

      c0=(c0<<1)|bit;
      if(bp==4) hi_nib=c0&15;
    }
    // ---- BOCPD (Adams-MacKay) run-length posterior over byte predictability ----
    {
      int good = (cur_byte_surp < 2.0) ? 1 : 0;   // binary observation
      double np[NRL], na[NRL], nb[NRL];
      for(int i=0;i<NRL;++i){ np[i]=0; na[i]=0.5; nb[i]=0.5; }
      double cp=0;
      for(int r=0;r<NRL;++r){
        if(rlp[r]<=0) continue;
        double pred=(good? (rl_a[r]+0.5) : (rl_b[r]+0.5))/(rl_a[r]+rl_b[r]+1.0);
        double growth=rlp[r]*pred*(1.0-BO_HAZ);
        double chg   =rlp[r]*pred*BO_HAZ;
        cp+=chg;
        int rn=(r+1<NRL)?r+1:NRL-1;
        double aa=rl_a[r]+(good?1:0), bb=rl_b[r]+(good?0:1);
        if(np[rn]+growth>0){
          na[rn]=(na[rn]*np[rn]+aa*growth)/(np[rn]+growth);
          nb[rn]=(nb[rn]*np[rn]+bb*growth)/(np[rn]+growth);
        }
        np[rn]+=growth;
      }
      np[0]+=cp; na[0]=0.5; nb[0]=0.5;
      double tot=0; for(int i=0;i<NRL;++i) tot+=np[i];
      if(tot<=0||!(tot==tot)){ for(int i=0;i<NRL;++i){np[i]=0;na[i]=0.5;nb[i]=0.5;} np[0]=1; tot=1; }
      int map_r=0; double bestp=-1;
      for(int i=0;i<NRL;++i){ rlp[i]=np[i]/tot; rl_a[i]=na[i]; rl_b[i]=nb[i];
        if(rlp[i]>bestp){bestp=rlp[i];map_r=i;} }
      bocpd_rl_bin=clampb(map_r,16);
      { double cpn=cp/tot; bocpd_cp_bin=clampb((int)(-log2(cpn>1e-12?cpn:1e-12)*2.0),16); }
    }
    prev_byte_surp=cur_byte_surp;
    int b=actual;
    if((b>='a'&&b<='z')||(b>='A'&&b<='Z')){ wordhash=wordhash*0x2F0FD693ull+(b|32); if(wordpos<15)++wordpos; }
    else { wordhash=0; wordpos=0; }
    if(b=='<')++xmldepth; else if(b=='>'){ if(xmldepth>0)--xmldepth; }
    prevbyte=b;
  }

  double bpc=total_bits/bufsz;
  fprintf(stderr,"bytes=%llu bpc=%.5f size=%.0f\n",(unsigned long long)bufsz,bpc,total_bits/8);
  printf("%.6f\n",bpc);
  if(DIAG){
    const char* inm[NIN]={"o0_dir","o0_ind","o1_dir","o1_ind","o2_dir","o2_ind","o3_dir","o3_ind",
      "o4_dir","o4_ind","o6_dir","o6_ind","wd_dir","wd_ind","match1","match2","bias"};
    for(int i=0;i<NIN;++i) fprintf(stderr,"  %-8s standalone bpc=%.4f\n",inm[i],icost[i]/bufsz);
    fprintf(stderr,"  %-8s bpc=%.4f\n","MIXER",mixcost/bufsz);
  }
  if(COLLECT){ FILE*o=fopen(outhist,"wb"); fwrite(hist,sizeof(U32)*NSTAT*NPBIN*NBINS*2,1,o); fclose(o); }
  return 0;
}
