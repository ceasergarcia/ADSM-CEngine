set xrange [-11:11]
set yrange [-11:11]
set size square
set xlabel "km (E-W)"
set ylabel "km (N-S)"
set arrow from -11,0 to 11,0 nohead lt 0
set arrow from 0,-11 to 0,11 nohead lt 0
set label "0" at 0.733333,-10.7333 left
set label "1" at 0.733333,-0.733333 left
set label "2" at 0.733333,9.26666 left
set parametric
plot [0:2*pi] 2*sin(t), 2*cos(t) notitle w l lt 1 lw 3,\
  '-' notitle w p lt 6 pt 7 ps 2, \
  '-' notitle w p lt 7 pt 71 ps 2, \
  '-' notitle w p lt 7 pt 71 ps 2
0 -10
e
0 0
e
0 10
e
