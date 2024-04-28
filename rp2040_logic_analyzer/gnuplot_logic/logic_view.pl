#! /usr/bin/perl
use strict;
use warnings;
use String::Scanf;
use IO::Select;

my $s = IO::Select->new();
$s->add(\*STDIN);

my $numarg = $#ARGV + 1;
if(1 != ($ARGV[0] =~ m/\/dev\/tty/)) { printf "looks like bad port name\n"; exit();}

#open up /dev/tty serial port from rp2040
system("stty 1:0:80001cb2:0:3:1c:7f:15:4:5:1:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0 -F /dev/ttyACM1\n");
sleep(1);
open( COM, $ARGV[0]) || die "Cannot read serial port : $!\n";

my $line;
my $foo;
my $first = 0;
while (1) {
   #process lines from ro2040 serial output
   $line = <COM>;
   if($line =~ m/highspeed/) {
      ++$first; 
      print $line;
      (my $samp, my $rate, my $trigpin, my $off) = sscanf("highspeed samp=%d rate=%fus trigpin=%d trigoff=%f", $line); 
      if($first == 2){ 
         printf(STDERR  "            >>>  Logic Trace Specifics  <<\n");
         printf(STDERR  "   samples     rate         width     Extent     trigpin      trigoff\n");
         printf(STDERR  "    %4d     %6.2fus    %6.1fms  %6.1fms         %2d      %6.2fms\n",
		 $samp, $rate, 0.001*$samp*$rate, 0.001*50000*$rate, $trigpin, 0.001*$samp*$rate*$off); 
	 printf(STDERR  "                                        --type help for command list\n\n--> ");
      }
   } 
   elsif($line =~ m/e/) { print $line; } 
   elsif($line =~ m/#/) { } 
   else { printf  "%s", $line ; $| = 1;} 


   #process commands from monitor stdin
   if ($s->can_read(0) ) {
      chomp($foo = <STDIN>);

      if($foo =~ m/exit/) { exit(); } 
      elsif($foo =~ m/help/) { 
         printf(STDERR "command  format           description\n"); 
	 printf(STDERR "________________________________________\n"); 
	 printf(STDERR "samples  int            - number of samples to display\n"); 
	 printf(STDERR "rate     float          - rate of sample acquire in usec\n"); 
	 printf(STDERR "col      int int ...    - gpio pin number groups, comma betwwen numbers makes bus\n"); 
	 printf(STDERR "labels   str str ...    - labels of the col pin groups\n"); 
	 printf(STDERR "trig     int            - gpio trigger pin\n"); 
	 printf(STDERR "toff     float          - trigger offset - float from 0 - 1.0\n"); 
	 printf(STDERR "exit                    - exit program\n\n--> "); 
	 } 
      else {
	 system ( "printf '$foo\\r' > $ARGV[0]\n");
         $first = 0;
      }
  }
}
