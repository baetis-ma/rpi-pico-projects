#!/usr/bin/perl
use Time::HiRes qw(usleep nanosleep);
my $ptr = 0;
my @storage;
for(my $i = 0; $i < $arraysize; $i++) { $storage[$i] = ' '; }
my $updaterate = 10;
my $cnt = 0;
my $arraysize = 400;
while (1) {
    ++$cnt;
    my $line = <STDIN>; 
    my @sline = split(',', $line);
    my $start = $sline[1];
    my $end   = $sline[2];
    my $pulse = $sline[3];
    my $spO2  = $sline[4];
    for(my $n = $start; $n < $end; ++$n)  { $storage[($n % $arraysize)] = $sline[5 + $n - $start]; }

    if(($cnt % 7) == 1) {
        open(FH, '>', 'heart.data');
        printf FH ("#%.1fbpm        spO2 %.1f\%\n", $pulse, $spO2);
        printf  ("%.1fbpm    spO2 %.1f\%\n", $pulse, $spO2);
        for(my $nn = 0; $nn < $arraysize; $nn++) { 
            if ($storage[$nn] > 4000 && $storage[$nn] < 6000) {
                printf (FH "%7.3f %d\n", 0.01 * $nn, 5000 - $storage[$nn]); 
            } 
    }
    close(FH);
    }
    `./heart.gnp`;
}
