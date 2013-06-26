unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic ('LBP and color based'1, 'Vibe'2)
set ytic auto                          # set ytics automatically
set ylabel "Time (Sec)"
set term png
set output "bg_compare.png"
set style boxplot
set xrange [0:3]
set yrange [0.001:0.5]
set grid
set key box center right
plot 'bg.csv' using (1):2 title 'LBP and color based' lt 1 lc -1, 'bg.csv' using (2):3 title 'Vibe' lt 3 lc -1
