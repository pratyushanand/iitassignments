#!/bin/bash
cat cleaning_execution_time.csv | awk '{sum += $2} END {print sum / 723}'
