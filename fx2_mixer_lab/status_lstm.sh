#!/bin/bash
echo '=== produced rows ==='
awk -F, 'NR==1 || $1 ~ /^lstm_/' /home/odin/hp_tmp/fx2exp/results.csv
echo
echo '=== fails for lstm names ==='
ls /home/odin/hp_tmp/fx2exp/out/lstm_*/fail.txt 2>/dev/null || echo none
echo
echo '=== live cmix ==='
ps -p 105700 -o pid,etime,cmd --no-headers || echo GONE
echo
echo '=== screen done line ==='
grep 'LSTM screen done' /home/odin/hp_tmp/fx2exp/screen_lstm.console
echo
echo '=== FAIL lines ==='
grep 'FAIL lstm_' /home/odin/hp_tmp/fx2exp/screen_lstm.console || echo no_fail_lines