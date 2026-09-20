#!/bin/bash
# Compile one fx2 variant and encode the 32 KiB screen slice.
# Usage: run_one.sh NAME [ -DFOO=bar ... ]
set -euo pipefail
NAME="${1:?name}"
shift
DEFS=("$@")
ROOT="/home/odin/hp_tmp/fx2exp"
SLICE="/home/odin/hp_tmp/slice.32k"
CSV_L="$ROOT/results.csv"
CSV_W="/mnt/c/Users/odinl/OneDrive/Desktop/Compression Algorithm/fx2_mixer_lab/results/screen32.csv"
SRC="$ROOT/work/$NAME"
mkdir -p "$ROOT/work" "$ROOT/out"

if [ ! -f "$SRC/makefile" ]; then
  rsync -a --delete "$ROOT/src_base/" "$SRC/"
fi

cd "$SRC"
export CFLAGS_DEFINES="-DSEED=923 -DUPDATE_LIMIT=3000 ${DEFS[*]}"
echo "BUILD $NAME $CFLAGS_DEFINES"
make clean >/dev/null
make cmix
mkdir -p "$ROOT/out/$NAME"
rm -f "$SRC/ppm.temp" "$ROOT/out/$NAME/arc" "$ROOT/out/$NAME/arc.cmix.temp"

# At most 2 experiment encodes plus the live 8 MB job.
exec 9>/tmp/hp_tmp_enc.lock
flock 9
echo "ENC $NAME $(date -Iseconds)"
set +e
/usr/bin/time -f "ec=%x rss_kb=%M wall=%e" -o "$ROOT/out/$NAME/time.txt" \
  ./cmix -n "$SLICE" "$ROOT/out/$NAME/arc" >"$ROOT/out/$NAME/log.txt" 2>&1
ec=$?
set -e
flock -u 9

bytes=0
if [ -f "$ROOT/out/$NAME/arc" ]; then
  bytes=$(wc -c < "$ROOT/out/$NAME/arc")
fi
rss=$(grep -o 'rss_kb=[0-9]*' "$ROOT/out/$NAME/time.txt" | head -1 | cut -d= -f2 || echo 0)
wall=$(grep -o 'wall=[0-9.]*' "$ROOT/out/$NAME/time.txt" | head -1 | cut -d= -f2 || echo 0)
bpc=$(python3 -c "print('%.4f' % ($bytes*8.0/32768.0))" 2>/dev/null || echo na)
defstr="${DEFS[*]}"
defstr="${defstr//,/;}"
line="$NAME,$bytes,$bpc,$wall,$rss,$ec,$defstr"
echo "$line"
mkdir -p "$(dirname "$CSV_W")"
{
  flock 8
  echo "$line" >>"$CSV_L"
  echo "$line" >>"$CSV_W"
} 8>>"$CSV_L"
echo "DONE $NAME bytes=$bytes bpc=$bpc wall=$wall rss=$rss ec=$ec"
