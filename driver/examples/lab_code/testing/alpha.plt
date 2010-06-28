# vim:ft=gnuplot:

set terminal png
set output 'gnuplot/alpha_associations.png'

set xlabel "Number of associations"
set ylabel "Average bandwidth in kbit/s"
set title ""

plot \
   'gnuplot/N.dat' title "ALPHA-N" with linespoints, \
   'gnuplot/C.dat' title "ALPHA-C" with linespoints, \
   'gnuplot/M.dat' title "ALPHA-M" with linespoints, \
 'gnuplot/CNa.dat' title "1:x:0" with linespoints, \
 'gnuplot/CNb.dat' title "2:x:0" with linespoints, \
 'gnuplot/MNa.dat' title "1:0:x" with linespoints, \
 'gnuplot/MNb.dat' title "2:0:x" with linespoints

set terminal postscript
set output 'gnuplot/alpha_associations.eps'

set style line 1 lw 2 lc rgbcolor "black" pt 2 lt 1
set style line 2 lw 2 lc rgbcolor "black" pt 4 lt 1
set style line 3 lw 2 lc rgbcolor "black" pt 6 lt 1

plot \
 'gnuplot/N.dat' title "ALPHA-N" with linespoints ls 1, \
 'gnuplot/C.dat' title "ALPHA-C" with linespoints ls 2, \
 'gnuplot/M.dat' title "ALPHA-M" with linespoints ls 3
