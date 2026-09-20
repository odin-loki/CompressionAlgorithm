#!/bin/bash
# Boolean extras screen on fx2 linear mixer. Unique names; do not touch
# fx2_build, champ binaries, or the live 8 MiB cmix.
set -uo pipefail
ROOT=/home/odin/hp_tmp/fx2exp
CSV="$ROOT/results.csv"
LOG="$ROOT/bool_screen.log"
HARNESS=/home/odin/hp_tmp/run_one.sh
export MAKEFLAGS="${MAKEFLAGS:--j16}"
mkdir -p "$ROOT/out" "$ROOT/work"

already() {
  awk -F, -v n="$1" 'NR>1 && $1==n {found=1} END{exit !found}' "$CSV" 2>/dev/null
}

run_one() {
  local name="$1"
  shift
  if already "$name"; then
    echo "$(date -Iseconds) SKIP $name already in results" | tee -a "$LOG"
    return 0
  fi
  # Force rsync from current src_base (Boolean FillDerived). Never reuse old dirs.
  rm -rf "$ROOT/work/$name"
  mkdir -p "$ROOT/out/$name"
  local runlog="$ROOT/out/$name/run.log"
  echo "$(date -Iseconds) START $name $*" | tee -a "$LOG"
  set +e
  bash "$HARNESS" "$name" "$@" >"$runlog" 2>&1
  local rc=$?
  set -e
  if [ "$rc" -ne 0 ]; then
    tail -n 120 "$runlog" > "$ROOT/out/$name/fail.txt"
    echo "$(date -Iseconds) FAIL $name rc=$rc (saved fail.txt)" | tee -a "$LOG"
    tail -n 20 "$runlog" | tee -a "$LOG"
    return 0
  fi
  echo "$(date -Iseconds) OK $name" | tee -a "$LOG"
  tail -n 5 "$runlog" | tee -a "$LOG"
}

echo "$(date -Iseconds) BOOL_SCREEN START" | tee "$LOG"

run_one xor_k4     -DFX2_BOOL_XOR=1 -DFX2_BOOL_K=4
run_one xor_k8     -DFX2_BOOL_XOR=1 -DFX2_BOOL_K=8
run_one xor_k12    -DFX2_BOOL_XOR=1 -DFX2_BOOL_K=12
run_one and_k6     -DFX2_BOOL_AND=1
run_one and_k8     -DFX2_BOOL_AND=1 -DFX2_BOOL_K=8
run_one or_k6      -DFX2_BOOL_OR=1
run_one prod_k6    -DFX2_BOOL_PROD=1
run_one prod_k8    -DFX2_BOOL_PROD=1 -DFX2_BOOL_K=8
run_one maj_k6     -DFX2_BOOL_MAJ=1
run_one maj_k12    -DFX2_BOOL_MAJ=1 -DFX2_BOOL_K=12
run_one rm_k6      -DFX2_BOOL_XOR=1 -DFX2_BOOL_AND=1
run_one all2_k6    -DFX2_BOOL_XOR=1 -DFX2_BOOL_AND=1 -DFX2_BOOL_OR=1
run_one all2_k8    -DFX2_BOOL_XOR=1 -DFX2_BOOL_AND=1 -DFX2_BOOL_OR=1 -DFX2_BOOL_K=8
run_one xor_prod   -DFX2_BOOL_XOR=1 -DFX2_BOOL_PROD=1
run_one fullbool   -DFX2_BOOL_XOR=1 -DFX2_BOOL_AND=1 -DFX2_BOOL_OR=1 -DFX2_BOOL_PROD=1 -DFX2_BOOL_MAJ=1
run_one xor_k6_skip01 -DFX2_BOOL_XOR=1 -DFX2_SKIP_ERR=0.01f
run_one rm_k8_c256 -DFX2_BOOL_XOR=1 -DFX2_BOOL_AND=1 -DFX2_BOOL_K=8 -DFX2_LSTM_CELLS=256
run_one xor_tiny   -DFX2_BOOL_XOR=1 -DFX2_TINY_LSTM=1

echo "$(date -Iseconds) BOOL_SCREEN DONE" | tee -a "$LOG"
echo "===== results =====" | tee -a "$LOG"
cat "$CSV" | tee -a "$LOG"