set key outside
set title "sine approximations"
plot for [col=2:4] 'sine.dat' using 1:col with lines title columnheader
