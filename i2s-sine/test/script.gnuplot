# expected input format:
# - sample float cordic16 cordic32 c16_delta c32_delta

# cordic 16 bits
set term wxt 1
set key outside
set title "sine approximation: cordic16"
plot "sine.dat" using 1:2 title "sin" with lines, "sine.dat" using 1:3 title "cordic16" with lines, "sine.dat" using 1:4 title "delta" with lines

# cordic 32 bits
set term wxt 2
set key outside
set title "sine approximation: cordic32"
plot "sine.dat" using 1:5 title "sin" with lines, "sine.dat" using 1:6 title "cordic32" with lines, "sine.dat" using 1:7 title "delta" with lines
