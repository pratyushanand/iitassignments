unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto
set ytic auto                          # set ytics automatically
set ylabel "Angle (Degree)"
set xlabel "Frame"
set term png
set output "leg_movement.png"
set grid
set key box top right
set yrange [-10:120]
plot 'leg_angle.csv' using 1:2 title 'Human' with lines , 'leg_angle.csv' using 1:3 title 'Vehicle' with lines
