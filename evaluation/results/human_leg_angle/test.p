set terminal pngcairo  background "#ffffff" fontscale 1.0 dashed size 640, 480 
set output "angle_plot.png"
set style function linespoints
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto
set ytic auto                          # set ytics automatically
set ylabel "Angle (Degree)"
set xlabel "Frame"
set grid
set key box top right
set yrange [-10:120]
unset colorbox
plot 'leg_angle.csv' using 1:2 lt -1 pi -4 pt 6 title 'Human' with lines, 'leg_angle.csv' using 1:3 lt -1 pi -3 pt 7 ps 0.2 title 'Vehicle' with lines
