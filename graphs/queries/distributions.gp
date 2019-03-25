load 'settings.gp'
set title "Data Distributions"

plot \
data using 1:2 with lines title "Linear" @Linear, \
data using 1:4 with lines title "Log" @Log, \
data using 1:5 with lines title "Cubic" @Cubic, \
data using 1:6 with lines title "Quadratic" @Quadratic, \
data using 1:7 with lines title "Random" @Random, \
data using 1:8 with lines title "Normal" @Normal
