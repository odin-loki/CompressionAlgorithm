#!/bin/bash
set -uo pipefail
CSV=/home/odin/hp_tmp/fx2exp/results.csv
OUTROOT=/home/odin/hp_tmp/fx2exp/out
HARNESS=/home/odin/hp_tmp/run_one.sh
LOG=/home/odin/hp_tmp/fx2exp/screen_lstm.log

already() {
  awk -F, -v n="$1" 'NR>1 && $1==n {found=1} END{exit !found}' "$CSV" 2>/dev/null
}

run_one() {
  local name="$1"; shift
  if already "$name"; then
    echo "SKIP $name already in CSV"
    return 0
  fi
  echo "==== START $name $* $(date -Iseconds) ===="
  mkdir -p "$OUTROOT/$name"
  local tmp
  tmp=$(mktemp)
  set +e
  bash "$HARNESS" "$name" "$@" >"$tmp" 2>&1
  local rc=$?
  set -e
  cat "$tmp"
  if [ $rc -ne 0 ]; then
    echo "FAIL $name rc=$rc"
    tail -120 "$tmp" > "$OUTROOT/$name/fail.txt"
    echo "logged $OUTROOT/$name/fail.txt"
  fi
  rm -f "$tmp"
  echo "==== END $name rc=$rc $(date -Iseconds) ===="
  return 0
}

{
echo "LSTM screen begin $(date -Iseconds)"
run_one lstm_c100   -DFX2_LSTM_CELLS=100
run_one lstm_c150   -DFX2_LSTM_CELLS=150
run_one lstm_c256   -DFX2_LSTM_CELLS=256
run_one lstm_c300   -DFX2_LSTM_CELLS=300
run_one lstm_h64    -DFX2_LSTM_HORIZON=64
run_one lstm_h256   -DFX2_LSTM_HORIZON=256
run_one lstm_h32    -DFX2_LSTM_HORIZON=32
run_one lstm_lr01   -DFX2_LSTM_LR=0.01f
run_one lstm_lr05   -DFX2_LSTM_LR=0.05f
run_one lstm_lr08   -DFX2_LSTM_LR=0.08f
run_one lstm_clip5  -DFX2_LSTM_CLIP=5
run_one lstm_clip20 -DFX2_LSTM_CLIP=20
run_one lstm_l2c100 -DFX2_LSTM_LAYERS=2 -DFX2_LSTM_CELLS=100
run_one lstm_l2c150 -DFX2_LSTM_LAYERS=2 -DFX2_LSTM_CELLS=150
run_one lstm_c256_h64  -DFX2_LSTM_CELLS=256 -DFX2_LSTM_HORIZON=64
run_one lstm_c256_lr05 -DFX2_LSTM_CELLS=256 -DFX2_LSTM_LR=0.05f
echo "LSTM screen done $(date -Iseconds)"
echo "==== CSV ===="
cat "$CSV"
} 2>&1 | tee -a "$LOG"