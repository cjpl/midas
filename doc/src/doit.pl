#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
my $tempfile1 = "header.tmp";
my $tempfile2 = "level_info.txt";
my $doxfile = "Organization.dox"; # output file
my $num = $#ARGV + 1 ; # number of arguments
#my ($doxfile) = @ARGV;
my $linenum=0;

#unless ($num)
#{
#    die "Supply doxfilename as parameter e.g. Organization.dox\n";
#}
#unless ($doxfile){ die "Supply doxfilename as parameter e.g. Organization.dox\n";};

print "Output file: $doxfile\n";



open OUTD, ">$doxfile" or die "Can't open output file $doxfile : $!\n";
open IN, "$tempfile1" or die "Can't open input file $tempfile1 : $!\n";

while (<IN>)
{  # write header to $doxfile
    $linenum++;
    print OUTD $_;
}
print "Wrote header ($linenum lines) to $doxfile\n";
close IN;
my ($line,$gotcha);
$gotcha=0;
open IN, "$tempfile2" or die "Can't open input file $tempfile2 : $!\n";

while (<IN>)
{
    $linenum++;
    chomp $_;
    unless ($_) {next;}

    if(/^\<!--/)
    {  # lose comments or they create a blank line in the index
	print "found comment at $linenum: $_\n";
	next;
    }
    
    if ($gotcha)
    {
	if (/<li/) # remove empty <ul> </ul> pairs which dox doesn't like
	{
	    print OUTD "$line\n";
	    print OUTD "$_\n";
            $gotcha=0;
	    next;
	}
        elsif  (/<\/ul/)
	{  
	    print "removing empty ul pair at line $linenum\n";
	    $gotcha=0; 
	    next;	    
	}
    }
 
    if (/<ul/)
    {
#	print "found \"<ul\" at line $linenum \n";
	$gotcha=$linenum;
        $line=$_;
        next;
    }

    print "copying $_\n";
    print OUTD "$_\n"
}
print "copied $linenum lines to $doxfile\n";
close IN;
close OUTD;
print "$doxfile is done. Now  make the index (docindex.dox) with \"make_index.pl \", then make or make publish (dox-> html)\n";
exit;
