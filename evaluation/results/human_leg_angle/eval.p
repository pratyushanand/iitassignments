unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto
set ytic auto                          # set ytics automatically
set ylabel "Angle (Degree)"
set xlabel "Frame"
set term png
set output "leg_movement_2.png"
set grid
set key box top right
set yrange [-10:90]
plot 'leg_angle_2.csv' using 1:2 title 'Human Leg Movement' with lines
