#!/bin/bash

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
solver="$script_dir/poodlepuzzleDailyCalendarSolver.bin"

if (( $# > 1 )) || { (( $# == 1 )) && [[ ! $1 =~ ^([1-9]|1[0-2])$ ]]; }; then
    echo "Usage: $0 [month 1-12]" >&2
    exit 2
fi
if [[ ! -x "$solver" ]]; then
    echo "Solver executable not found: $solver (run make in cpp/)" >&2
    exit 2
fi

if (( $# == 0 )); then
    pids=()
    for month in {1..12}; do
        bash "$script_dir/solveAllDates.sh" "$month" &
        pids+=("$!")
    done
    status=0
    for pid in "${pids[@]}"; do
        wait "$pid" || status=1
    done
    exit "$status"
fi

filename="Month_$1.txt"
printf '{\n' > "$filename"
first=true
for weekday in {1..7}; do
    for day in {1..31}; do
        entry=$("$solver" "$weekday" "$day" "$1" -i)
        status=$?
        # Exit 1 means this date has no solution; its JSON entry is still valid.
        if (( status > 1 )); then
            echo "Solver failed for weekday=$weekday day=$day month=$1" >&2
            exit "$status"
        fi
        if [[ $first == false ]]; then printf ',\n' >> "$filename"; fi
        first=false
        printf '%s' "$entry" >> "$filename"
    done
done
printf '\n}\n' >> "$filename"
