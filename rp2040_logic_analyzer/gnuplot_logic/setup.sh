  printf "col 0 1 2 3 4\r" >  /dev/ttyACM0 
  printf "rate 1\r" >  /dev/ttyACM0 
  printf "trig 1\r" >  /dev/ttyACM0 
  printf "toff .1\r" >  /dev/ttyACM0 
  printf "samples 2000\r" >  /dev/ttyACM0 
  printf "label ce csn sck mosi miso\r" >  /dev/ttyACM0 
