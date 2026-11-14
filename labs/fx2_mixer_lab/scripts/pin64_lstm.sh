#!/bin/bash
# 64 KiB pin of LSTM-sweep winners. Reuse existing binaries when present.
set -euo pipefail
ROOT=/home/odin/hp_tmp/fx2exp
SLICE=/home/odin/hp_tmp/slice.64k
CSV=$ROOT/results64.csv
WIN="/mnt/c/Users/odinl/OneDrive/Desktop/Compression Algorithm/fx2_mixer_lab/results/screen64.csv"
dd if=/tmp/hp_hutter/enwik8.8mb of=$SLICE bs=1 count=65536 status=none
echo 'name,bytes,bpc,wall,rss_kb,ec,defines' > "$CSV"
mkdir -p "$(dirname "$WIN")"
cp -f "$CSV" "$WIN"

enc() {
  local name="$1" bin="$2" defs="$3"
  local out=$ROOT/out/${name}_64
  mkdir -p "$out"
  rm -f "$out/arc" "$out/arc.cmix.temp"
  exec 9>/tmp/hp_tmp_enc.lock
  flock 9
  echo "ENC64 $name $(date -Iseconds)"
  local cwd; cwd=$(dirname "$bin")
  set +e
  ( cd "$cwd" && /usr/bin/time -f "ec=%x rss_kb=%M wall=%e" -o "$out/time.txt" \
      ./cmix -n "$SLICE" "$out/arc" >"$out/log.txt" 2>&1 )
  local ec=$?
  set -e
  flock -u 9
  local bytes=0
  [ -f "$out/arc" ] && bytes=$(wc -c < "$out/arc")
  local rss wall bpc
  rss=$(grep -o 'rss_kb=[0-9]*' "$out/time.txt" | head -1 | cut -d= -f2 || echo 0)
  wall=$(grep -o 'wall=[0-9.]*' "$out/time.txt" | head -1 | cut -d= -f2 || echo 0)
  bpc=$(python3 -c "print('%.4f' % ($bytes*8.0/65536.0))")
  local line="$name,$bytes,$bpc,$wall,$rss,$ec,$defs"
  echo "$line"
  echo "$line" >> "$CSV"
  echo "$line" >> "$WIN"
}

# reuse binaries from 32k sweep
enc baseline64 "$ROOT/work/baseline/cmix" "lab-default"
enc lr08_64 "$ROOT/work/lstm_lr08/cmix" "-DFX2_LSTM_LR=0.08f"
enc h32_64 "$ROOT/work/lstm_h32/cmix" "-DFX2_LSTM_HORIZON=32"
enc lr08_skip01_64 "$ROOT/work/c200_lr08_skip01/cmix" "-DFX2_LSTM_LR=0.08f -DFX2_SKIP_ERR=0.01f"

# new combo
if [ ! -x "$ROOT/work/lr08_h32/cmix" ]; then
  bash /home/odin/hp_tmp/run_one.sh lr08_h32 -DFX2_LSTM_LR=0.08f -DFX2_LSTM_HORIZON=32 || true
fi
enc lr08_h32_64 "$ROOT/work/lr08_h32/cmix" "-DFX2_LSTM_LR=0.08f -DFX2_LSTM_HORIZON=32"
echo PIN64_DONE
cat "$CSV"
