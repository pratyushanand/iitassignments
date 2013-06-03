set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set yrange [0.0200:0.0500]
set title "Cleaing execution time"
set xlabel "Frame Number"
set ylabel "Time in Second"
set term png
set output "cleaning_execution_time.png"
set label "Performance with the System having DMIPS = 800" at 40,0.000100
plot "cleaning_execution_time.csv" using 1:2 title 'Cleaing execution time = 37.2 ms'
