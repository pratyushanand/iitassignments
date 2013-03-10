set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set title "Forground Extraction Algorithm Comparision"
set xlabel "Frame Number"
set ylabel "Time in Second"
set term png
set output "fg_compare.png"
set label "Performance with the System having DMIPS = 800" at 40, 0.1
plot "bg.csv" using 1:2 title 'IDIAP=>0.388779', "bg.csv" using 1:3 title 'vibe=>0.004991'
