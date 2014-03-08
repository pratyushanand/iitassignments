#!/bin/bash
cat bg.csv | awk 'NR > 1 {sum += $2} END {print sum *1000 / 723}'
cat bg.csv | awk 'NR > 1 {sum += $3} END {print sum *1000 / 723}'
