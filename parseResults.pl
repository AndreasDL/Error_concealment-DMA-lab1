use strict;
use warnings;
use Data::Dumper;

my $solutionDir = "solutions";
my $output = "timings.csv";
my @timings;
my @names;

my $fnr = 0;
opendir(DIR, $solutionDir) or die $!;
while (my $file = readdir(DIR)){
	next if !($file =~ /.*txt/);

	print "Parsing: $solutionDir/$file\n";
	push(@names,$file);
	open my $f, "$solutionDir/$file" or die $!;

	#cleanup
	for (my $i = 0 ; $i < 6 ; $i++){
		<$f>;
	}

	my $nr = -1;
	while(<$f>){
		chomp;
		if ($_ =~ /^Decoding.*$/){
			$nr++;
			$timings[$nr][$fnr] = 0;
		}else{
			$_ =~ /.*time needed : (.*)$/;
			$timings[$nr][$fnr] += $1; # b/c we want total time even if multiple methods are used
		}
	}

	close $f;
	$fnr++;
}

print "Printing output\n";
open OUT, ">$output" or die $!;
#names
print OUT "frame number;";
print OUT "$_;" foreach (@names);
print OUT "\n";
#values
my $nr = 0;
foreach my $arr (@timings){
	print OUT "$nr;";
	foreach my $time (@{$arr}){
		print OUT "$time;";
	}
	print OUT "\n";
	$nr++;
}
close OUT;