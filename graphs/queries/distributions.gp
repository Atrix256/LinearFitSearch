load 'settings.gp'
set title "Data Distributions"

plot \
data using 1:2 with lines @Linear, \
data using 1:4 with lines @Log, \
data using 1:5 with lines @Cubic, \
data using 1:6 with lines @Quadratic, \
data using 1:7 with lines @Random
