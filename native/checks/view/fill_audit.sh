#!/bin/sh
# fill_audit.sh DIR: the audit lines (VIEW.md 5.w) of a map dump's NN-fill.txt, every level the dump has, play field then backdrop, and their totals:
# one-offs the margins copy (copies), props, strip boundaries inside a prop (cuts), rows where the level's own edge cuts a prop (edge).
# Run from the repository root.
d=$1
for f in "$d"/??-fill.txt; do l=$(basename "$f" -fill.txt)
  grep -h "audit (VIEW" "$d/$l-fill.txt" | awk -v l=$l '{ n++; printf "%s %s %s %s %s %s %s %s %s\n", l, (n == 1 ? "field" : "back"), $4, $5, $6, $7, $8, $9, $10 }'
done | sed 's/[a-z_]*=//g' | tee "$d/audit.txt" | awk '{ print }' > /dev/null
awk '{ printf "%s %-5s one-offs %3d copies %3d props %3d cuts %2d+%-2d edge %2d+%-2d\n", $1, $2, $3, $4, $5, $6, $7, $8, $9; o += $3; c += $4; p += $5; k += $6 + $7; e += $8 + $9 } END { printf "TOTAL copies %d, props %d, cuts %d, edge-cut rows %d\n", c, p, k, e }' "$d/audit.txt"
