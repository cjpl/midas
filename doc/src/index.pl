#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
# invoke script with argument 1 to sort only

sub next_field($$$);
sub next_fields($$$);
sub next_fields_levels($$$);
sub write_levels();
sub mine();
sub mainpage();
sub subpage($);
sub fill_page_hash();
sub copy($$);
sub find_page($$);
sub indent();
sub header();
sub footer();
sub idx();
sub fix_idx();
sub write_idx(@);
$|=1; # flush output buffers
# mine.pl *.dox
our $debug =0;
#my $doxpath = "midas/my_doxfiles/";
my $num = $#ARGV + 1 ; # number of arguments
our $outfile = "mined_info.txt"; # all pages, subpages, sections etc
our $sortfile = "sorted_info.txt"; # subpages substituted 
our $tempfile= "tempfile.txt";
our $tempfile2="level_info.txt"; #  input file 1 for doxfile (used by doit.pl)
our $hdrfile="header.tmp";  #   input file 2 for doxfile (used by doit.pl)

#index files
our $outf ="index_info.txt";
our $idxf ="docindex.tmp";
our $idxd ="docindex.dox";

our $linenum;
our @indent;
# concatenate strings with . and use ".." as any two characters
#   $lpattern =  $gifpath."\/"."Labels"."\/"."..".$dev; # specific device
our %hashpages;
our %hashpagelevel;
# input parameters:
my ($param) = @ARGV;
unless($num){$param=0};
my $level;
my $done;

print "$num  $param\n";

if ($num > 0)
{ idx(); 
  fix_idx();
  exit;
}

mine(); # mines dox files to produce mined_info.txt
fill_page_hash(); # finds pages and mainpage in mined_info.txt
mainpage(); # fills sorted_info.txt.0 with mainpage
$level=1;
$done=1;
while ($done)
{
    if($level > 10){ die "stopping at level 10; may be something wrong\n";};
    print "Calling subpage with level=$level\n";
    $done = subpage($level); # replaces subpages with pages
    $level++;
}

for(keys %hashpagelevel) {
    print "Page $_ is at level  $hashpagelevel{$_}\n";}

print "done at level $level\n";
$level--;
copy ($sortfile.".".$level, $sortfile);
print "renamed file $sortfile.$level to  $sortfile\n";
write_levels();
print "Now run doit.pl to produce Organization.dox\n";






sub mine()
{
    my @files=glob("*.dox");
    my $nfiles = $#files + 1 ;
    my $infile;
    my ($i,$j);
    my $new_page=0;
    my $code;
    my $pagename;
    my $temp;
    my $subpage_flag=0;
    my $main_page=0;
    my $main_page_flag=0;
    my $first=0;
    
    print "mine starting\n";
    open OUT, ">$outfile" or die "Can't open output file $outfile : $!\n";
    print "Successfully opened $outfile\n";
    
    print "@files\n";
    foreach $infile (@files)
    {
	
#	if ($infile eq "Organization.dox")
#	{ print "Skipping Organization.dox\n";next;}
	print "Working on file $infile :\n";
	#print OUT "\n ********* Working on file $infile :\n";
	open IN, $infile or die "Can't open input file $infile : $!\n";
	$code=0; #new file 
	$linenum=0;
	while (<IN>)
	{
	    $linenum++;
	    #print "$linenum:";
	    if(/^\s*(\\|@)endcode/i)
	    {
		print "Found endcode at line $linenum .. no longer skipping \n"; 
		$code=0; 
	    } 
	    if(/^\s*(\\|@)code/i)
	    { 
		print "Found code at line $linenum .. skipping \n"; 
		$code=1;  # skip everything inside \code
		next; # next line
	    }
	    unless ($code == 1)
	    {
		if(/^\s*\*\//)
		{   # looking for end-of-page */
		    unless ($new_page == 1)
		    { die "End of page without a new page at line $linenum of file  $infile\n";}
		    if ($main_page_flag)
		    {
			print OUT "<!--  end of mainpage $pagename  level 0 --> \n";
			$main_page_flag=0;
		    }
                    else { print OUT "\\end-of-page $pagename \n";}
		    $new_page = $pagename=0;
		    next; # next line
		    #  die "success";
		} 
		unless ($main_page == 1)
		{
		    #print "checking for main page...\n";
		    #look for \mainpage (only one)
		    $temp=next_field($_, "mainpage",$linenum); # returns mainpage name
		    if($temp)
		    {         
			print OUT "\n\%\n"; # page separator
			print OUT "\\mainpage $temp   # mainpage found in $infile  (level 0) -->\n";
			$pagename=$temp;
			$main_page=1; # only one mainpage
			$main_page_flag=1;
			$new_page=1; # this is a new page
			
		    }
                    # else { print "did not find mainpage yet\n";}
		}
		
		# look for \page
                #print "checking for page.. \n";
                my $comment;
		($temp,$comment)=next_fields($_, "page",$linenum); # returns pagename if new page
		if($temp)
		{ #         newpage 
		    $pagename=$temp;
                    
                    print OUT "\n\%\n"; # page separator
		    print OUT "\\page $pagename   <!-- \#$comment\# new page in $infile -->\n";
		    $new_page=1;
		  
		}
		#else { print "did not find page in line $_\n";}

		#look for \subpage
		$temp=next_field($_, "subpage",$linenum); # returns subpage name if subpage
		if($temp)
		{  
		    #note which page this is a subpage 
		    print OUT "\\subpage $temp   <!-- subpage under page $pagename in $infile -->\n";
		}
		#look for \section
		$temp=next_field($_, "section",$linenum); # returns section name if section
		if($temp)
		{ 
		    print OUT "\\section $temp   <!-- section under page $pagename in $infile -->\n";
		}
		#look for \subsection
		$temp=next_field($_, "subsection",$linenum); # returns section name if section
		if($temp)
		{ 
		    print OUT "\\subsection $temp   <!-- subsection under page $pagename in $infile -->\n";
		}
		#look for \subsubsection
		$temp=next_field($_, "subsubsection",$linenum); # returns section name if section
		if($temp)
		{ 
		    print OUT "\\subsubsection $temp   <!-- subsubsection under page $pagename in $infile -->\n";
		}
	    }
	} # end of a file   
	close IN;
	
    } # end of all files
  
   
    print OUT "\n\%\n"; # page separator
    close OUT; # outfile = mined_info.txt
    print "Successfully closed $outfile\n";
    return;    
} 
sub idx()
{
    my @files=glob("*.dox");
    my $nfiles = $#files + 1 ;
    my $infile;
    my ($i,$j,$k);
    my $code;
    my $anchor;
    my $comment;
    my $temp;
    my $len; 
    my @array=qw( a b c d e f g h i j k l m n o p q r s t u v w x y z );

    indent();
        my $idx="idx_";
    $len=$#array;
    $i=0;
    while ($i<$len)
    {
	$array[$i]=$idx.$array[$i];
	$i++;
    }
    print "@array\n";
    
    print "index starting\n";
    open OUTF, ">$outf" or die "Can't open output file $outf : $!\n";
    print "Successfully opened $outf\n";
    
    $i=0;
    print "@files\n";
    foreach $infile (@files)
    {
	
	if ($infile eq "Organization.dox")
	{ print "Skipping Organization.dox\n";next;}
	
	print "Working on file $infile :\n";
	print  "\n ********* Working on file $infile :\n";
	open IN, $infile or die "Can't open input file $infile : $!\n";
	$code=0; #new file 
	$linenum=0;
	while (<IN>)
	{
	    $linenum++;
	    #print "$linenum:";
            chomp $_;
            unless ($_){next;}
	    
	    if(/^\s*(\\|@)endcode/i)
	    {
		#print "Found endcode at line $linenum .. no longer skipping \n"; 
		$code=0; 
	    } 
	    if(/^\s*(\\|@)code/i)
	    { 
		#print "Found code at line $linenum .. skipping \n"; 
		$code=1;  # skip everything inside \code
		next; # next line
	    }
	    unless ($code == 1)
	    {
		
		
		# look for \anchor
                #print "checking for anchor.. \n";
                my $comment;
		($anchor)=next_field($_, "anchor",$linenum); # returns anchor_name 
		if($anchor)
		{                    
                    if ($anchor=~ /^end/){ next;}
                    print OUTF "$anchor \n";
		    # print  "found anchor $anchor in line $_\n";
                    
		    if ($anchor=~ /idx/)
                    {
			print "found idx in $anchor\n";
			#$_=$anchor;
			#s/idx_//;
			push @array, $anchor;
			#$hashanchor{$anchor}=$_;
			#$i++;
                    }
		    
		}
	    }
	} # end of a file   
	close IN;
	
    } # end of all files    
    close OUTF;
    print "Successfully closed $outf\n";

    print "array of index items:\n @array\n";
    print "calling write_idx with array as parameter\n";
    write_idx(@array);
    return;
}

sub write_idx(@)
{
    my $len;
    my ($i,$j,$k);
    my @last=qw( 0 0 0 0 0 0 0 );
    my (@fields, $l);
    my $letter;
    my $level;

# note: array is the only parameters, so passing directly is OK.
    my @array = @_;
    my @sorted = sort @array;
    print "\n sorted array: @sorted\n";
    $len= $#sorted;
   
    print " len=$len\n";
    $i=0;
    
  
    open OUTF, ">$idxf" or die "Can't open input file $idxf : $!\n";
    print OUTF "\n <!-- sorted array:\n @sorted \n  -->\n";
    $level=0; 
    print OUTF "$indent[$level]<ul class=\"i$level\"> <!-- i$level -->\n";
    while ($i<$len+1)
    {
	
	$_=$sorted[$i];
        print "next sorted value $i:$_\n";
	# s/idx_//;
        @fields=split('_');
        $l=$#fields;
	$j=0;
	#   print "l=$l fields: @fields\n";
        while ($j < $l)
        {
            $j++;
	    #    print "j=$j; level=$level  l=$l\n";
	    if ($fields[$j] eq $last[$j])
            {
		if ($fields[$j]=~/^[a-z]$/) { die "duplicate single letter $fields[$j]\n";}
		print "duplicate:j=$j level=$level last[$level]=$last[$level]\n";
		next;
	    }
            $last[$j]=$fields[$j];
           
	    if ($fields[$j]=~/^[a-z]$/)
	    {  # found single letter
		$letter = uc $fields[$j] ; # convert to Upper Case
                if ($j != 1){ die "single letter $letter found at fields index $j. Expect 1\n";}
                # flag for single letter
		$k=0;  # take level down to zero for single letter
		while ($k < $level)
		{
		    print OUTF "$indent[$level]</ul>  <!-- i$level -->\n";
		    $level--;
		}
		print OUTF "\n\%\n"; #marker
		print OUTF "$indent[$level]<li><b>$letter</b><br>\n"; # write single bold letter at level 0
		while ($j > $level)
		{ # restore level to previous
		    $level++;
		    print OUTF "<br>$indent[$level]<ul class=\"i$level\"> <!-- i$level -->\n";
		}
		next;
	    }


	   

            
            while ($j < $level)
	    {
                print OUTF "$indent[$level]</ul>  <!-- i$level -->\n";
                $level--;
	    }

            while ($j > $level)
            { 
                $level++;
                print OUTF "$indent[$level]<ul class=\"i$level\"> <!-- i$level -->\n";
            }
           
            
            if( $j == $l)
            { # last term
                #print "level=$level l=$l  last[$level]=$last[$level]\n";
	        print "$indent[$level]<li>\@ref $sorted[$i] \"$fields[$level]\"<br>\n";
	        print OUTF "$indent[$level]<li>\@ref $sorted[$i] \"$fields[$level]\"<br>\n";
		
            }
            else
            {
                #print "level=$level l=$l  last[$level]=$last[$level]\n";
	        print "$indent[$level]<li>$fields[$level]<br>\n";
  	        print OUTF "$indent[$level]<li>$fields[$level]<br>\n";
	    }
	}
	$i++;
    }
    $level=0;
    while ($j < $level)
    {
	print OUTF "$indent[$level]</ul>  <!-- i$level -->\n";
	$level--;
    }
    print OUTF "*/\n";             
    close OUTF; 
    print "closed $idxf\n";
    return;    
} 

sub fix_idx()
{
    my $num;
    my $i=0;
    my $temp=$/;
    $/ = "\n%\n";  # page separator 

    open INF, "$idxf" or die "Can't open input file $idxf : $!\n";
    open OUTF, ">$idxd" or die "Can't open output file $idxd : $!\n";

    print OUTF "/*! \@page  DocIndex Index to Documentation pages\n";
    print OUTF "\\htmlonly\n";
    print OUTF "<script type=\"text/javascript\" src=\"http://ladd00.triumf.ca/~daqweb/doc/my_midas/html/navigation.js\"></script>\n";
    print OUTF "\\endhtmlonly\n";
    print OUTF "\n<span class=\"note\">This file produced automatically ...  DO NOT EDIT </span>\n";
    print OUTF "<span class=\"warn\">\n";
    print OUTF "WARNING - this page is under construction\n";
    print OUTF "</span>\n\n";
  


    my @file = <INF>;
    close INF;
    print "closed input file $idxf\n";

    $/ = $temp; # restore to former value
    $num = $#file + 1 ; # number of pages
    print "num=$num\n";
    $i=-1;
    while ($i < $num-1)
    {
	$i++;
	$_ = $file[$i];
        chomp $_;
	s/\n%//;
	print "\n Item $i of $num: \n$_\n";
	if ($_)
	{ # string is not empty
	    if ($i == 0)  # item 0 includes <ul class "i0">
	    { 
		print OUTF $_;
		next;
	    }
	    elsif(/\@ref/) # no @ref means no index items for that letter
	    {
		print OUTF $_;
	    }
	    else
	    {
		print "found a repeated letter at : \n $_ \n skipping...\n";
		next;
	    }
	}
    } # while
    # add the footer
    print OUTF "</ul>  <!-- i0 -->\n";
    print OUTF "*/\n"; # end of page
    close OUTF;
    print "closed output file $idxd\n";
}


sub copy($$)
{
   my $in=shift;
   my $out=shift;
   open IN, "$in" or die "Can't open file $in : $!\n";
    print "copy: successfully opened $in\n";
    open OUT, ">$out" or die "Can't open output file $out : $!\n";
    print "copy: successfully opened $out\n";
    while (<IN>)
       {print OUT "$_";}
    return 1;
}

sub mainpage()
{
# Write the mainpage into $sortfile

    print "mainpage starting\n";
    my $level=0;
    open OUTS, ">$sortfile.$level" or die "Can't open output file $sortfile.$level : $!\n";
    print "mainpage: successfully opened $sortfile.$level\n";

    if ( $hashpages{"mainpage"} ) { print OUTS "$hashpages{\"mainpage\"}";}
    else { die "mainpage: no mainpage found\n";}

    close OUTS;
    print "mainpage: closed $sortfile.$level\n";
    return;
}


sub subpage($)
{
# level should increase each time this is called
    my $pagename;  
    my $linenum=0;
    my $level=shift;
    my $gotit;
    my $sl;
 
    $gotit=0;
    $sl=$level-1;
# now look for the subpages 
    print "subpage starting\n";
    open INS, "$sortfile.$sl" or die "Can't open input file $sortfile.$sl : $!\n"; # e.g.sorted_info.txt0

    print "subpage: successfully opened input file (sort) $sortfile.$sl\n";
    
    open OUTS,  ">$sortfile.$level" or die "Can't open output file $sortfile.$level : $!\n";
    print "subpage: successfully opened output file $sortfile.$level\n";
    
    while (<INS>)
    {
	
	$linenum++;
        #print "$linenum:\n";

        #look for \subpage
	$pagename=next_field($_, "subpage",$linenum); # returns subpage name
	if($pagename)
	{     
	    $gotit=1; # still finding subpage in file
	    print "found \"\\subpage $pagename\" at line $linenum \n";
            # look for this page
	    $hashpagelevel{$pagename}=$level; #level hash 
            print OUTS "\n<!-- substituting $pagename for subpage $pagename (level $level) -->\n";  
	    if($hashpages{$pagename})
	    { 
		print OUTS "$hashpages{$pagename}"; # copy to OUTS
		delete $hashpages{$pagename};
	    }
	    else {die "subpage: could not find page $pagename in hashpages; may be used already\n";}
	}
	else
	{
            #no string "\subpage" so copy across unchanged
	    print OUTS "$_";
	}
    } # end of while INS
    print "end of while INS\n";
    close INS;
    close OUTS;
    print "closed input file $sortfile.$sl and output file $sortfile.$level\n";

    for(keys %hashpagelevel) 
    { print  "Page $_ Level $hashpagelevel{$_}\n";}

    return $gotit;
}    

sub fill_page_hash()
{
    my $pagename;
    my $temp=$/;
    my $num;
    my $i=0;

    $/ = "\n%\n";  # page separator
    open IN, "$outfile" or die "Can't open file $outfile : $!\n"; #mined_info.txt
    print "Successfully opened $outfile\n";

    my @file = <IN>;
    $num = $#file + 1 ; # number of pages
    print "num=$num\n";
    while ($i < $num)
    {
	$_ = $file[$i];
	s/\n%\n//;
	print "\n\n Item $i of $num \n";
	if ($_)
	{ # string is not empty
	    
	    # look for \page
	    #print "checking for page.. \n";
	    $pagename=next_field($_, "page",$i); # returns pagename if new page
	    if($pagename)
	    { #         newpage 
		print "page name : $pagename\n";
#	s/\n%\n/\n/; 
		if ( $hashpages{$pagename} )
		{ 
		    die "ERROR main page $pagename is already known; duplicate page?? \n"; 
		}
		$hashpages{$pagename}="$_"; # make an entry
	    }
	    else
	    {
		print "did not find page in string $_\n";
		# look for \mainpage
		print "checking for mainpage.. \n";
		$pagename=next_field($_, "mainpage",$i); # returns pagename if new page
		if($pagename)
		{ #         newpage 
		    print "page name : $pagename\n";
		    #    s/\n%\n/\n/; 
		    if ( $hashpages{"mainpage"} )
		    { 
			die "ERROR mainpage is already known; a second mainpage?? \n"; 
		    }
		    $hashpages{"mainpage"}="$_"; # make an entry
		}
		else
		{ die "unexpected item (not page or mainpage) item $i in line $_\n";}
	    }
	    
	    
	} # end of non-empty string
	else { print "skipping empty string at item $i\n";}
	
	$i++; # next item   
    } # while IN
    close IN;
    print "\nClosed file $outfile\n";

    $/=$temp;
    $i=1;
    for(keys %hashpages) 
    {
	print "$i: Page $_ contains:\n  $hashpages{$_}\n\n";
	$i++;
    }
    

    print "SpecialConfig:\n $hashpages{\"SpecialConfig\"}\n";
    print "Mainpage:\n $hashpages{\"mainpage\"}\n";
    return;
}


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
	    print "found string \"$item\":  $gotcha\n";
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



sub find_page($$)
{
    my $pagename = shift;
    my $level = shift;
    my $temp = $/;
    $/ = "\n%\n";  # page separator
    open IN, "$outfile" or die "Can't open file $outfile : $!\n"; #mined_info.txt
    #print "Successfully opened $outfile\n";
    
    my $i=1;
    my @file = <IN>;
    my $num = $#file + 1 ; # number of pages
    print "num=$num\n";
    while ($i <= $num)
    {
	  print "\nPage $i  $pagename  $num \n";
	  print "$file[$i]";
	if ($file[$i]=~/\\page $pagename/)
	{ 
	    $_ = $file[$i];
	    s/\n%\n/\n/; 
            s/end-of-page/level $level  end-of-page/;
	    print "find_page: found page $pagename  \n";
	    print "$_";
            close IN;
            $/=$temp;
	    return $_;
	}
	$i++;
    }
    close IN;
    $/=$temp;
    die "find_page: page $pagename not found in $outfile\n";
    return 0;
}


sub find_string($$$)
{
# If item is "subpage" looks for string @subpage or \subpage at beginning of line
# returns complete line or 0 if not found

    my $len = @_ ; # supplied params;
# parameters
    $_=shift; #line
    my $item=shift;
    my $linenum=shift;

   
    unless (/^(\\|@)$item /) 
    { # print "and ending. Did not find $item in $_\n";
	return 0; 
    }
    # print "found $item in line:$_ \n";
    s/ +/ /g; # compact   
    chomp;
    return $_; 
}

sub next_fields($$$)
{
    my $len = @_ ; # supplied params;
# parameters
    $_=shift; #line
    my $item=shift;
    my $linenum=shift;
    my $value;
    my (@fields);
#    print ("next_fields starting...");
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
    $len= $#fields ;
    my $gotcha=0;
    my $string=0;
    my $ret=0;
    my $i=0;
    foreach $_ (@fields)
    {
        $i++; 
#	print "working on field: $_\n";
	
	if  (/(\\|@)$item/i)
	{ 
	    print "fields:found string \"$item\" in $_\n";
	    $value=$fields[$i];
            my $comment="";
            while ($i < $len)
	    { 
		$i++;
		$comment=$comment." ".$fields[$i];
                print "$i comment:$comment\n";
	    }
            chomp $comment;
            return($value,$comment);
	} 
    }
    return (0,0);
}


sub write_levels()
{
#our $outfile = "Organization.dox";
my $linenum;
my $pagename=0;
my $sectname=0;
my $subsectname=0;
my $subsubsectname=0;
my ($level);
my $pagelevel=0;
my $section_index="_section_index";
my @comment;
my $myline="";
my $pageflag=0;
my  $previous="";
my $line="";
my $gotcha;

open IN, "$sortfile" or die "Can't open input file $sortfile : $!\n";
open OUT, ">$tempfile2" or die "Can't open output file $tempfile2 : $!\n";
open OUTD, ">$hdrfile" or die "Can't open output file $hdrfile : $!\n";
indent(); # fill indent array with blanks for each level
header(); # write fixed header into $hdrfile
#my @name= split /./,$hdrfile ; #Organization

#for(keys %hashpagelevel) 
#{ print  "Page $_ Level $hashpagelevel{$_}\n";}

$pagelevel=$level=1;
print "\n\nwrite_levels starting\n\n";


$linenum=0;
while (<IN>)
{
    $linenum++;
    print "\n ***** working on line $linenum: $_";
    chomp $_;
    unless ($_){ next; } # blank line

    if ($linenum < 7) { next; } # skip lines containing Organization

#    if(/$name/)
#    {   # skip first lines containing "Organization" as we are rewriting this file (Organization.dox)  
#        print "skipping line $_ ($name)\n";
#        next;        
#    }
   
   
    if(/^\<!--/)
    {  # comment
	print OUT "$_\n";
	next;
    }
    
	print "line $linenum \n";
	print "pagelevel:$pagelevel\n";
	print "level:$level\n";
#page 
    @comment=0;
    ($pagename,@comment)=next_fields_levels($_, "page",$linenum); # returns page name
    if($pagename)
    {
	print "after next_fields, $pagename and comment:\"@comment\"\n";
	s/\\page/\@ref/;
	$pagelevel = $hashpagelevel{$pagename};
	unless ($pagelevel) {die "no level found for page $pagename\n";}
	
        print "page $pagename pagelevel= $pagelevel, present level=$level\n";
	my $tmp=$pagelevel-1;
	if($level < 1) { die "$level is 0 at \\page, expect $pagelevel\n";}
	elsif ($level > $pagelevel)
	{
	    print "page $pagename: level $level. Reducing level to $pagelevel\n";
	    while ($level > $pagelevel)
	    {
		$level--;
		print OUT "$indent[$level]</ul> <!-- i$level -->\n"; # down 1
	    }
	}       
	elsif ($level < $tmp )
	{   
	    print "page $pagename: level $level. Increasing level to $tmp\n";
	    while ($level < $tmp)
	    {
		$level++; # up 1
		print OUT "$indent[$level]<ul class=\"i$level\"> <!-- i$level -->\n";	
	    }
	}

	if ($pagelevel ==1)
	{  
	    print "Level 1  comment=\"@comment\"\n";
            print OUT "\n\n<!--                         Section : $comment[0] (level $level)  -->\n";
	    print OUT "\\anchor $pagename$section_index\n"; # match name to ref on next line
	    print OUTD "<li> \@ref $pagename$section_index \"@comment\" \n";
	    print OUT "<br>\n";
	}
	if ($pagelevel ==2){  print OUT "<br>\n";}
	print OUT "$indent[$level]<li> \@ref $pagename <!-- subpage $pagename -->\n";

       # $pageflag=1;
	# $myline = "$indent[$level]<ul class=\"i$level\"> <!-- i$level -->\n";
        #$level++; # up 1
	print OUT "$indent[$level]<ul class=\"i$level\"> <!-- i$level -->\n";
        $level++; # up 1
	$previous = "page";
	next;
    }
    
#end-of-page
    $pagename=next_field($_, "end-of-page",$linenum); # returns page name
    if($pagename)
    {
        if ($pageflag)
        { 
           print "found page/end-of-page pair at line $linenum\n";
           $pageflag=0; 
           $previous="end-of-page";
           next;
        } # page/end-of-page pair detected.
   
	$pagelevel = $hashpagelevel{$pagename};
	unless ($pagelevel) {die "end-of-page; no level found for page $pagename\n";}
	if ($pagelevel != ($level-1) ) 
	{ print "end-of-page $pagename; present level is $level; expect $pagelevel+1\n";}

	unless ($previous eq "page")
	{ # only write </ul> if page contains sections
	   
	   if($level < 1) { die "illegal is $level at end-of-page\n";}
	   elsif ($level > $pagelevel)
	   {
	      print "end-of-page $pagename: level $level. Reducing level to $pagelevel\n";
	      while ($level > $pagelevel)
	      {
		  $level--; # down 1
		  print OUT "$indent[$level]</ul> <!-- i$level  end-of-page $pagename -->\n"; # down 1	
	      }
	   }
	   else  { die "$level is $level; too low at end-of-page $pagename (level $pagelevel)\n";}
        }       
	$previous = "end-of-page";
	next;

    }

 #   if($pageflag)
 #   {
        # not page/end-of-page pair, so write myline
  #      print OUT $myline;
  #      $pageflag=0;
  #      $level++;
  #  }


# section
    $sectname=next_field($_, "section",$linenum); # returns section name
#    print "looking for section; $sectname=$sectname $previous=$previous\n";
    if($sectname)
    {
	s/\\section/\@ref/;
	
        if ($previous eq "section"){ ; } # no change
	
	
        elsif ($previous eq "subsection")
	{ # down one level
	    $level--; 
	    print OUT "$indent[$level]</ul> <!-- i$level -->\n"; # down 1
	    
	}
	
        elsif ($previous eq "subsubsection")
	{ # down two levels
	    print OUT "$indent[$level]</ul> <!-- i$level -->\n"; # down 1
	    $level--;
	    print OUT "$indent[$level]</ul> <!-- i$level -->\n"; # down 2
	    $level--; 
	}
	
        else { print " previous $previous with section $sectname ... no level change \n";}
	
        print OUT "$indent[$level]<li> $_\n"; 
	$previous = "section";
	next;
    }
    
    
# subsection
    $subsectname=next_field($_, "subsection",$linenum); # returns subsection name
#    print "looking for subsection; $subsectname=$subsectname $previous=$previous\n";
    if($subsectname)
    {
	s/\\subsection/\@ref/;
	if ($previous eq "page")
	{ 
	    die "subsection $subsectname preceded by $previous $pagename (no section)";
	}
	elsif  ($previous eq "section")
	{
	    print OUT "$indent[$level]<ul class=\"i$level\">\n" ;# up 1
	    $level++;
	}
	
	elsif ($previous eq "subsection"){ ; } # no level change
	
	
        elsif ($previous eq "subsubsection")
	{ # down one level
	    $level--; 
	    print OUT "$indent[$level]</ul> <!-- i$level -->\n"; # down 1
	}


        else { die "unexpected previous $previous with subsection $subsectname\n";}
        print OUT "$indent[$level]<li> $_\n"; 
	$previous = "subsection";
	next;
    }


# subsubsection
    $subsubsectname=next_field($_, "subsubsection",$linenum); # returns subsubsection name
#   print "looking for subsubsection; $subsubsectname=$subsubsectname $previous=$previous\n";

    if($subsubsectname)
    {
	s/\\subsubsection/\@ref/;
	if ($previous eq "page")
	{ 
	    die "subsubsection $subsubsectname preceded by $previous $pagename (no section)";
	}
	elsif ($previous eq "section")
	{ 
	    die "subsubsection $subsubsectname preceded by $previous $sectname $pagename (no section)";
	}
	
	elsif  ($previous eq "subsection")
	{
	    print OUT "$indent[$level]<ul class=\"i$level\" > \n";# up 1
		$level++;
	}

        elsif ($previous eq "subsubsection"){ ; } # no level change
	
        else { die "unexpected previous $previous with subsubsection $subsubsectname\n";}
	
        print OUT "$indent[$level]<li> $_\n"; 
	$previous = "subsubsection";
	next;
    }

    
    print " line unmodified $_ \n";
    print OUT "$_\n";
} # while in

print OUT "</ol>\n\n";
print OUT "\\anchor end\n";
print OUT "\\htmlonly <img ALIGN=\"left\" alt=\"previous.gif\" src=\"previous.gif\"> \\endhtmlonly \@ref Top \n";
print OUT "\\htmlonly <img alt=\"next.gif\" src=\"next.gif\"> \\endhtmlonly\n";
print OUT "*/\n";
close $sortfile;
close $tempfile2;
print "closed $sortfile and $tempfile2\n";

footer(); # write the last lines of the header to $hdrfile

close IN;
close OUTD;
print "  output files are $tempfile2 and $hdrfile\n";
print "done. Now run \"doit.pl \"\n";

return;
}

sub indent()
{
    my $i;
    my $maxlevel=10;
    
    $indent[0]="";
    for ($i=1; $i<$maxlevel; $i++)
    {
	$indent[$i]=$indent[$i-1]."   ";
        print "indent[$i]=\"$indent[$i]\"\n";
    }
}



sub next_fields_levels($$$)
{
    my $len = @_ ; # supplied params;
# parameters
    my $line=shift; #line
    my $item=shift;
    my $linenum=shift;
    my ($value);
    my (@fields,@hash,@comment);
    $_ = $line;
#    print ("next_fields starting...");
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
    my $i=0;
    foreach $_ (@fields)
    {
        $i++; 
#	print "working on field: $_\n";
	
	if  (/(\\|@)$item/i)
	{ 
	    print "fields:found string \"$item\" in $_\n";
	    $value=$fields[$i];
# now find the comment enclosed by # signs
            $_=$line;
	    @hash = split /\#/,$_;
	    $len= $#hash +1;
            if($len>=2)
	    { 
		$_=$hash[1];
                s/^ //;
                s/ $//;
		@comment= split / /,$_;
	    }
            else {@comment=0;}
           # for ($i=0; $i<$len; $i++)
           # { print "$i, $hash[$i]\n";}
            print "returning \"@comment\"\n";
            return($value,@comment);
	} 
    }
    return (0,0);
}


sub header()
{
    print OUTD "/*! \@page  Organization Manual Contents \n";
    print OUTD "\\htmlonly\n";
    print OUTD "<script type=\"text/javascript\" src=\"http://ladd00.triumf.ca/~daqweb/doc/my_midas/html/navigation.js\"></script>\n";
    print OUTD "\\endhtmlonly\n";
    print OUTD "\n<span class=\"note\">This file produced automatically ...  DO NOT EDIT </span>\n";
    print OUTD "\@section O_what What can be found in this manual?\n\n";
    print OUTD "<span class=\"warn\">\n";
    print OUTD "WARNING - this page is under construction as the sections\n";
    print OUTD "are being changed/added to etc.\n";
    print OUTD "</span>\n\n";
    print OUTD "\\anchor Organization_section_index\n";
    print OUTD "<span  style=\"font-weight: bold; font-size: 125%;\">Section Index</span>\n";
    print OUTD "<ol class=\"i0\">\n";
    print OUTD "<li> \@ref Top \"Main Page\"\n";
    return;
}

sub footer()
{
    print OUTD "</ol>\n\n";
    print OUTD "\\anchor O_main_index\n";
    print OUTD "<span  style=\"font-weight: bold; font-size: 125%;\">Main Index</span>\n";
    print OUTD "<ol class=\"i0\">\n"; 
    print OUTD "   <li>\@ref Top \"Main Page\"\n";
    print OUTD "   <br>\n";
    return;
}
