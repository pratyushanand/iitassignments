unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic ('Ours'1, 'HAAR'2, 'Covariance'3)
set ytic auto                          # set ytics automatically
set ylabel "Time (Sec)"
set term png
set output "pipeline_execution_time.png"
set style boxplot
set xrange [0:4]
set yrange [0.001:0.5]
set grid
set key box top center
plot 'performance.csv' using (1):2 title 'Ours' lt 1 lc -1, \
'performance.csv' using (2):3 title 'HAAR' lt 3 lc -1, \
'performance.csv' using (3):4 title 'Covariance' lt 4 lc -1
