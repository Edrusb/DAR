#!/bin/bash

total=$(grep target < my_makefile | wc -l)
echo "total = $total"
done=$(ls -l | grep -E 'target.*[yn]$' | wc -l)
echo "done = $done"

ratio=$(echo "$done * 100 / $total" | bc)

echo "achieved: ${ratio}%"

