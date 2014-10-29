use strict;
use warnings;

my $file = "error_pattern_simple_beowulf.txt";
my $i = 0;
open(F, $file);
while (<F>){
	chomp;
	$i++ if length $_ > 4 ;
}
print "$file: $i\n";


$file = "error_pattern_complex_beowulf.txt";
$i = 0;
open(F, $file);
while (<F>){
	chomp;
	$i++ if length $_ > 4 ;
}
print "$file: $i\n";