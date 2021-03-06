set xrange [-6:6]
set yrange [-1:11]
set size square
set xlabel "km (E-W)"
set ylabel "km (N-S)"
set arrow from -6,0 to 6,0 nohead lt 0
set arrow from 0,-1 to 0,11 nohead lt 0
set label "0" at 0.4,-0.4 left
set label "1" at 0.4,9.6 left
set label "High risk\n(merged)" at 1.25,8.5 left tc lt 1
set label "Back-\nground" at 3.66,5.4 left
# the triangle
set arrow from 0,0 to -2.2913,5 nohead lt -1
set arrow from -2.2913,5 to 0,5 nohead lt -1
set arrow from 0,5 to 0,0 nohead lt -1
set label "5.5" at first -2,2.2 center
set label "q" at first -0.5,2.35 center font "Symbol"
set label "5" at first 0.5,2.5 center
set parametric
plot [0:2*pi] 5.5*sin(t),5.5*cos(t) notitle w l lt 1 lw 3,\
 5.5*sin(t),5.5*cos(t)+10 notitle w l lt 1 lw 3,\
 '-' notitle w p lt 6 pt 7 ps 2, \
 '-' notitle w p lt 6 pt 7 ps 2
0 0
e
0 10
e
