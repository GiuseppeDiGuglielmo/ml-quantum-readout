#!/bin/bash
# Recreate the data symlinks pointing to the NAS copy.
# Links are created next to this script. Existing paths are never touched.
NAS=/nas/work/research/quantum/readout/data
HERE="$(cd "$(dirname "$0")" && pwd)"
for d in fpga_testing qick_data qubit_readout_purdue; do
    link="$HERE/$d"
    if [ -e "$link" ] || [ -L "$link" ]; then
        echo "WARNING: $link already exists, skipping" >&2
        continue
    fi
    ln -s "$NAS/$d" "$link"
    echo "$link -> $NAS/$d"
done
