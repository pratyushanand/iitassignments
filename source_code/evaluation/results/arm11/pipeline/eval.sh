#!/bin/bash
cat performance.csv | awk 'NR > 0 {sum += $2} END {print sum * 1000 / 178}'
cat performance.csv | awk 'NR > 0 {sum += $3} END {print sum * 1000 / 178}'
cat performance.csv | awk 'NR > 0 {sum += $4} END {print sum * 1000 / 178}'
