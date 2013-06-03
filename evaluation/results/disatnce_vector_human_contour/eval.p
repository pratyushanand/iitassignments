unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto
set ytic auto                          # set ytics automatically
set ylabel "Distance from human centroid(Pixel)"
set xlabel "Human contour points"
set term png
set output "distance.png"
set grid
set key box top right
set yrange [-10:50]
plot 'distance.csv' using 1:2 title 'Human contour distance from centroid' with lines
