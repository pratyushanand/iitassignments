unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto
set ytic auto                          # set ytics automatically
set ylabel "Angle (Degree)"
set xlabel "Frame"
set term png
set output "angle_plot.png"
set grid
set key box top right
set yrange [-10:120]
set style func linespoints
plot 'leg_angle.csv' using 1:2 title 'Human' with lines lc -1 lw 4, \
'leg_angle.csv' using 1:2 notitle lt -1 lw 1 pi -1 pt 6, \
'leg_angle.csv' using 1:3 title 'Vehicle' with lines lc -1, \
'leg_angle.csv' using 1:3 notitle lt -1 lw 1 pi -1 pt 6
