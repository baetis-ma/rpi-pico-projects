stty 1:0:80001cb2:0:3:1c:7f:15:4:5:1:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0 -F /dev/ttyACM0

cat /dev/ttyACM0 | ./read_logic | gnuplot

