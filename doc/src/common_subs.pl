#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
##
## This file required by make_index.pl and make_contents.pl
##
sub next_field($$$);
sub fill_indent();

our $debug;
our @indent;

sub next_field($$$)
{
    my $len = @_ ; # supplied params;
# parameters
    $_=shift; #line
    my $item=shift;
    my $linenum=shift;

    my (@fields);
#    print ("next_field starting...");
#    print ("Line:$_\n");
#    print ("Item:$item\n");
#    print ("Len:$len\n");
    
    unless (/(\\|@)$item /) 
    {  # print "next_field: page $linenum: did not find $item in $_\n";
        return 0; 
   }
    # print "found $item in line:$_ \n";
	#print "$linenum:";
    s/ +/ /g; # compact
 
     
    @fields = split / /,$_;
 #   print "fields: @fields";
    my $gotcha=0;
    my $string=0;
    my $ret=0;
    
    foreach $_ (@fields)
    {
#	print "working on field: $_\n";
	
	if  (/(\\|@)$item/i)
	{ 
	    $gotcha=$_; # found new item
	    if($debug){ print "next_field: found string \"$item\":  $gotcha\n"; }
	    next; # find next field
	} 

	if ($gotcha)
	{    
	    chomp;
	  #  print  " $_ \n";
	    return $_; 
	}
    }
    return (0);
}


sub fill_indent()
{
    my $i;
    my $maxlevel=10;
    
    $indent[0]="";
    for ($i=1; $i<$maxlevel; $i++)
    {
	$indent[$i]=$indent[$i-1]."   ";
#        print "fill_indent: indent[$i]=\"$indent[$i]\"\n";
    }
}

# include this 1 at EOF so require returns a true value
1;
