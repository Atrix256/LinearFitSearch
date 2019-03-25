load 'settings.gp'

@TerminalDefault size 1920,480

set multiplot layout 1, 3 title "Searches on Normal Data"

plot @Dists using 1:8 with lines title "Normal" @Normal

plot \
data using 1:2 with lines title "Line Fit Avg" @FnLineFit, \
data using 1:3 with lines title "Binary Search Avg" @FnBinary

plot \
data using 1:4 with lines title "Line Fit Min", \
data using 1:5 with lines title "Line Fit Max" @FnLineFit, \
data using 1:6 with lines title "Binary Search Min", \
data using 1:7 with lines title "Binary Search Max" @FnBinary

unset multiplot
