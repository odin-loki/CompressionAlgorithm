#!/bin/bash
echo "===== csv ====="
grep -E '^(xor_|and_|or_|prod_|maj_|rm_|all2_|fullbool)' /home/odin/hp_tmp/fx2exp/results.csv
echo "===== log starts ====="
grep -E 'START |OK |FAIL |BOOL_SCREEN' /home/odin/hp_tmp/fx2exp/bool_screen.log
echo "===== live8m ====="
ps -p 105700 -o pid,etime,rss --no-headers || echo DEAD
echo "===== bool pid ====="
ps -p 127381 -o pid,etime --no-headers || echo BOOL_DONE
echo "===== bool screens ====="
pgrep -af run_bool_screen || true
echo "===== current bool run_one ====="
pgrep -af 'run_one.sh' | grep -E 'xor_|and_|or_|prod_|maj_|rm_|all2_|fullbool' || true
echo "===== fails ====="
ls /home/odin/hp_tmp/fx2exp/out/*/fail.txt 2>/dev/null | grep -E 'xor_|and_|or_|prod_|maj_|rm_|all2_|fullbool' || echo no_bool_fails