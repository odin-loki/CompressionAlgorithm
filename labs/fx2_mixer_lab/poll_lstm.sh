#!/bin/bash
for i in 1 2 3 4 5 6 7; do
  sleep 50
  echo "--- poll $i ---"
  grep -E 'DONE lstm_|FAIL lstm_|START lstm_|screen done' /home/odin/hp_tmp/fx2exp/screen_lstm.console | tail -6
  awk -F, '$1 ~ /^lstm_/' /home/odin/hp_tmp/fx2exp/results.csv | awk -F, '{printf "%s %s\n",$1,$2}'
  pgrep -af 'run_one.sh lstm_' | grep -v pgrep || true
  if grep -q 'LSTM screen done' /home/odin/hp_tmp/fx2exp/screen_lstm.console; then
    echo ALL_DONE
    break
  fi
  echo
done