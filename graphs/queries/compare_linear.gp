load 'settings.gp'

@TerminalDefault size 1920,480

set multiplot layout 1, 3 title "Searches on Linear Data"

plot @Dists using 1:2 with lines title "Linear" @Linear

plot \
data using 1:2 with lines title "Gradient Avg" @FnGradient, \
data using 1:3 with lines title "Line Fit Avg" @FnLineFit

plot \
data using 1:4 with lines title "Gradient Min", \
data using 1:5 with lines title "Gradient Max", \
data using 1:6 with lines title "Line Fit Min", \
data using 1:7 with lines title "Line Fit Max"

unset multiplot
