#!/usr/bin/perl
my $counter = 0;
my $ptr = 0;
my @storage;
my $updaterate = 10;
my $arraysize = 500;
my $cnt;
while (1) {
    my $line = <STDIN>; 
    $storage[$ptr] = $line;
    ++$counter;
    if( $ptr < $arraysize ) { ++$ptr; } else { $ptr = 0; }

    if ($counter % $updaterate == 0) {
       open(FILE, '>', "./spectrum.data") or die "cant open file";
       for ( $cnt = 0; $cnt < $arraysize; $cnt++) {
	       #printf ("%s", $storage[($ptr + $cnt)%$arraysize]);
          print FILE  $storage[($ptr + $cnt)%$arraysize];
       }
       close(FILE) or print "couldnt close file";
       0==system("cp spectrum.png temp.png\n") or die exit 0;
       0==system("./spectrum.gnp\n") or die exit 0;
    }
}
