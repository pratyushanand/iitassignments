unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic ('Ours'1, 'Haar'2, 'IDIAP'3)
set ytic auto                          # set ytics automatically
set ylabel "Time (Sec)"
set term png
set output "pipeline_execution_time.png"
set style boxplot
set xrange [0:4]
set yrange [0.1:5]
set grid
set key box center right
plot 'performance.csv' using (1):2 title 'Ours' , 'performance.csv' using (2):3 title 'Haar','performance.csv' using (3):4 title 'IDIAP'
