unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set ylabel "Execution Time (in ms)"
set term png
set output "pipeline_execution_time.png"
set auto x
set logscale y
set yrange [0.1:10000]
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9
set xtic scale 0
set key box top left

plot 'exec_time.csv' using 3:xtic(2) lc rgb 'white' title 'ARM(DMIPS=44)',  \
       'exec_time.csv' using 0:($3*1.2):3 with labels notitle, \
       'exec_time.csv' using 4:xtic(2) lc rgb 'black' title 'x86(DMIPS=800)', \
       'exec_time.csv' using 0:($4*1.2):4 with labels notitle'
