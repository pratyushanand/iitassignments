unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic ('IDIAP'1, 'Vibe'2)
set ytic auto                          # set ytics automatically
set ylabel "Time (Sec)"
set term png
set output "bg_compare.png"
set style boxplot
set xrange [0:3]
set yrange [0.1:5.0]
set grid
set key box center right
plot 'bg.csv' using (1):2 title 'IDIAP' , 'bg.csv' using (2):3 title 'Vibe'
