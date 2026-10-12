#!/usr/bin/env bash
set -euo pipefail

# ---- config ----
TIME_BIN="time"
TIME_FMT="%e"                      # real seconds only
THREADS_FOR_Q7=1
SAMPLE_FOR_Q7=10

# binaries expected (adjust names if yours differ)
SERIAL_BIN="./randtrack"
PARALLEL_BINS=(
  "./randtrack_global_lock"
  "./randtrack_list_lock"
  "./randtrack_element_lock"
  "./randtrack_reduction"
)

# ---- helpers ----
need() { command -v "$1" >/dev/null 2>&1 || { echo "Missing: $1"; exit 1; }; }

run_once() {
  local bin="$1" t="$2" s="$3" out_time="$4"
  echo "Timing: $bin  t=$t  s=$s"
  ${TIME_BIN} -f "${TIME_FMT}" -o "${out_time}" "${bin}" "${t}" "${s}" > /dev/null
}

read_time() {
  local f="$1"
  if [[ -f "$f" ]]; then cat "$f"; else echo "0"; fi
}

open_file() {
  local f="$1"
  if [[ ! -f "$f" ]]; then return 0; fi
  if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$f" >/dev/null 2>&1 || true
  elif command -v open >/dev/null 2>&1; then
    open "$f" >/dev/null 2>&1 || true
  else
    echo "Plot generated: $f"
  fi
}

line() { printf '%*s\n' "$(tput cols 2>/dev/null || echo 60)" | tr ' ' '-'; }

# ---- preflight ----
need "${TIME_BIN#*/}" || true   # allow absolute path
if [[ ! -x "$TIME_BIN" ]]; then
  echo "Error: $TIME_BIN not found or not executable."
  exit 1
fi

# ---- ensure Q7 time files exist (1 thread, sampling=10) ----
serial_time_file="serial.${THREADS_FOR_Q7}.${SAMPLE_FOR_Q7}.time"
if [[ ! -x "$SERIAL_BIN" ]]; then
  echo "Error: ${SERIAL_BIN} not found or not executable. Run: make"
  exit 1
fi
[[ -f "$serial_time_file" ]] || run_once "$SERIAL_BIN" "$THREADS_FOR_Q7" "$SAMPLE_FOR_Q7" "$serial_time_file"

declare -A label_map
label_map["./randtrack_global_lock"]="global"
label_map["./randtrack_list_lock"]="list"
label_map["./randtrack_element_lock"]="element"
label_map["./randtrack_reduction"]="reduction"

declare -A time_files
for bin in "${PARALLEL_BINS[@]}"; do
  label="${label_map[$bin]}"
  tf="${label}.${THREADS_FOR_Q7}.${SAMPLE_FOR_Q7}.time"
  time_files["$label"]="$tf"
  if [[ -x "$bin" ]]; then
    [[ -f "$tf" ]] || run_once "$bin" "$THREADS_FOR_Q7" "$SAMPLE_FOR_Q7" "$tf"
  else
    echo "Warning: $bin not found or not executable; skipping Q7 timing for it."
  fi
done

# ---- print raw times ----
line
echo "Raw times (seconds) for 1 thread, sampling=${SAMPLE_FOR_Q7}"
S=$(read_time "$serial_time_file")
printf "%-10s %s\n" "serial"   "${S}s"
for label in global list element reduction; do
  tf="${time_files[$label]:-}"
  if [[ -n "$tf" && -f "$tf" ]]; then
    T=$(read_time "$tf")
    printf "%-10s %ss\n" "$label" "$T"
  fi
done

# ---- compute Q7 overhead ratios ----
line
echo "Q7 overhead = T(parallel @1 thread) / T(serial)"
if [[ "$S" == "0" ]]; then
  echo "Cannot compute ratios (serial time = 0)."
else
  for label in global list element reduction; do
    tf="${time_files[$label]:-}"
    if [[ -n "$tf" && -f "$tf" ]]; then
      T=$(read_time "$tf")
      awk -v L="$label" -v T="$T" -v S="$S" 'BEGIN { printf "%-10s %.3f\n", L, T/S }'
    fi
  done
fi

# ---- run provided scripts ----
line
if [[ -f "./run-experiment" ]]; then
  chmod +x ./run-experiment || true
  echo "Running ./run-experiment (this may take several minutes)…"
  ./run-experiment
else
  echo "Note: ./run-experiment not found; skipping."
fi

if [[ -f "./plot-experiment" ]]; then
  chmod +x ./plot-experiment || true
  echo "Running ./plot-experiment…"
  ./plot-experiment
else
  echo "Note: ./plot-experiment not found; skipping plotting."
fi

# ---- open plots if present ----
line
open_file "plot-10.pdf"
open_file "plot-100.pdf"

echo "Done."
