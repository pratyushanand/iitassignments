unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set ylabel "Execution Time (in ms)"
set term png
set output "bg_compare.png"
set auto x
set logscale y
set yrange [0.1:1000]
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.5
set xtic scale 0
set key box top left

plot 'exec_time.csv' using 3:xtic(2) lc rgb 'black' notitle,  \
       'exec_time.csv' using 0:($3*1.2):3 with labels notitle
