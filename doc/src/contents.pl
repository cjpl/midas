#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
##
## This file required by make_contents.pl
##
sub next_fields($$$);
sub next_fields_levels($$$);
sub write_levels();
sub mine();
sub mainpage();
sub subpage($);
sub fill_page_hash();
sub copy($$);
sub find_page($$);
sub header();
sub footer();
sub create_dummy_files();
sub check_sublevel($);
sub set_sublevel($$$);
sub reverse_sublevel($);
our $debug;
our $linenum;
our @indent;
our $idxd;

$|=1; # flush output buffers

our $outfile = "mined_info.txt"; # all pages, subpages, sections etc
our $sortfile = "sorted_info.txt"; # subpages substituted 
our $tempfile= "tempfile.txt";
our $tempfile2="level_info.txt"; #  input file 1 for doxfile (used by doit.pl)
our $hdrfile="header.tmp";  #   input file 2 for doxfile (used by doit.pl)

# concatenate strings with . and use ".." as any two characters
#   $lpattern =  $gifpath."\/"."Labels"."\/"."..".$dev; # specific device
our %hashpages;
our %hashpagelevel;

require "./write_levels.pl"; # subroutines to make the contents page are in this file
require "./common_subs.pl";     # common subroutines between this file and contents.pl

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
    my $tmpstr;
    my $tmpline;
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
        $tmpline=0;
	
#	if ($infile eq "Organization.dox")
#	{ print "Skipping Organization.dox\n";next;}

	print  "\n ********* Working on file $infile *********** \n";
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
                    next;
		}
		#else { print "did not find page in line $_\n";}

		#look for \subpage
		$temp=next_field($_, "subpage",$linenum); # returns subpage name if subpage
		if($temp)
		{   
                   # see if there's any level information for this subpage (e.g. <-- \level= + or - -->
                    $tmpstr = check_sublevel($_); # returns any \level string
                    # print "after check_sublevel, tmpstr=\"$tmpstr\" \n"; 
		    #note which page this is a subpage of
		    print OUT "\\subpage $temp   <!-- subpage under page $pagename in $infile  $tmpstr -->\n";
                    next;
		}

		#look for \section
		$temp=next_field($_, "section",$linenum); # returns section name if section
		if($temp)
		{ 
		    print OUT "\\section $temp   <!-- section under page $pagename in $infile -->\n";
                    next;
		}
		#look for \subsection
		$temp=next_field($_, "subsection",$linenum); # returns section name if section
		if($temp)
		{ 
		    print OUT "\\subsection $temp   <!-- subsection under page $pagename in $infile -->\n";
                    next;
		}
		#look for \subsubsection
		$temp=next_field($_, "subsubsection",$linenum); # returns name 
		if($temp)
		{ 
                    # $tmpstr may contain level information
                    if ($tmpline > 0 ) {print "tmpline: $tmpline  linenum: $linenum\n";}
                    if ($tmpline != $linenum)
                    { 
                        $tmpstr = ""; 
                    }
                    else
                    { print " adding  tmpstr = $tmpstr  at line $linenum\n"; }
		    print OUT "\\subsubsection $temp   <!-- subsubsection under page $pagename in $infile  $tmpstr -->\n";
                    next;
		}

                #look for \level inside a comment (level information for following subsubsection)
                #         (e.g. <-- \level+  level change for following subsubsubsection  -->
                $tmpstr = check_sublevel($_); # returns any \level string
                if ($tmpstr ne " ")
                { 
                    $tmpline = $linenum + 1;
                    print " tmpstr = $tmpstr   at line $linenum   (tmpline = $tmpline)\n";
                } # only good for following line if a subsubsection
                # print "after check_sublevel, tmpstr=\"$tmpstr\" for line $tmpline \n"; 
                
                
            }

	} # end of a file   
	close IN;
	
    } # end of all files
  
   
    print OUT "\n\%\n"; # page separator
    close OUT; # outfile = mined_info.txt
    print "Successfully closed $outfile\n";
    return;    
}

sub set_sublevel($$$)
{
# checking for level information of the form \level+2  \level  \level-
# which is found in the \subpage declarations, e.g.
#     \subpage RC_new_page   <!--  at present level -->
#     \subpage RC_next       <!--  \level-2   down two levels for this page -->
# 
# call with  $level = present level; returns new level
#
    my $len = @_ ; # supplied params;
# parameters
  
    $_=shift; #line
    my $linenum=shift;
    my $level = shift; # present level
    
    my $item = "level"; # looking for @level or \level (level change with a subpage)
    
    my ($temp,$sign);
    my $x=0;
    my (@fields);

#    print ("Item:$item\n");
#    print ("Len:$len\n");
#    print "level = $level\n";
    
    
    
    unless (/(\\|@)$item((.\d*)?)/) 
    {   #  print "set_sublevel: page $linenum: did not find $item in $_\n";
        return $level; 
    }
    print "set_sublevel: at level $level; found $item in line:$_ \n";
    #print "dollar1=$1\n";
    #print "dollar2=$2\n";
    $temp = $2;
    if ($temp =~/(\+|\-)/)
    {
        # print "set_sublevel: found + or - in  dollar2; $1\n";
        $sign=$1;
        $x=1; # default
        
        if  ($temp=~ /(\d$)/ )
             {
                 $x=$1;
                 # print "set_sublevel: found a digit $x\n";
             }
             
             
             if ($sign=~/\+/)
             { 
                # print "set_sublevel: found + $x \n";
                 $level += $x;
             }
             else 
             { 
                # print "set_sublevel: found - $x \n";
                 $level -= $x;
             }
             print "set_sublevel: level was 5, now level is $level\n";
         }
        return $level;        
}
    
sub check_sublevel($)
{
# checking for level information of the form \level+2  \level  \level-
#             NOTE:      \level++ not allowed
# which is found in the \subpage declarations, e.g.
#     \subpage RC_new_page   <!--  \level or blank  at nominal pagelevel of subpage -->
#     \subpage RC_next       <!--  \level-2   down two levels from nominal pagelevel for this subpage -->
# 
# called from mine to return string e.g. \level-2 if present
#
    my $len = @_ ; # supplied params;
# parameters
  
    $_=shift; #line
   

    my $item = "level"; # looking for @level or \level (level change with a subpage)
    
    # print ("check_sublevel working on line $_");
    
    unless (/(\\|@)$item((.\d*)?)/) 
    {   
        # print "check_sublevel: did not find $item in $_\n";
        return " "; 
    }
    print "check_sublevel: found \"\\level\" or \"\@level\" (i.e. level change) in line:\n";
    print "    $_";
    #print "dollar1=$1\n";
    #print "dollar2=$2\n";
    
    print "check_sublevel: returning \"\@level$2\" \n";
    return "\@level$2";  # for mine
}

sub reverse_sublevel($)
{   

  my $tmpstr = shift; #line
  print "reverse_sublevel: starting with \"$tmpstr\"\n";
  $tmpstr =~  tr/-/%/  ;
  $tmpstr =~  tr/+/-/  ;
  $tmpstr =~  tr/%/+/  ;
  print "reverse_sublevel: returning  \"$tmpstr\"\n";
  return $tmpstr;
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
    my $origlevel;
    my $tmpstr;
    $gotit=0;
    $sl=$level-1; # sortedlevel for the filename extension
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
        $tmpstr=" ";
	$pagename=next_field($_, "subpage",$linenum); # returns subpage name
	if($pagename)
	{     
	    $gotit=1; # still finding subpage in file
	    print "found \"\\subpage $pagename\" at line $linenum \n";
            
            # see if there's any level information for this subpage (e.g. <-- \level= + or - -->
            $origlevel=$level;
            $level = set_sublevel($_, $linenum, $origlevel);
            if($origlevel != $level)
            {
                print "subpage: change of level ($origlevel -> $level) detected for subpage $pagename\n";
                if ($level <= 0)
                {
                    die "subpage: illegal new level for subpage $pagename  at line $linenum i.e. $_";    
                }  

                $tmpstr = check_sublevel($_); # returns any \level string
                print "after check_sublevel, tmpstr=\"$tmpstr\" \n"; 
                $tmpstr = reverse_sublevel($tmpstr);
                print "after reverse_sublevel, tmpstr=\"$tmpstr\" \n"; 
 
            }  
            $hashpagelevel{$pagename}=$level; #level hash 
            print OUTS "\n<!-- substituting $pagename for subpage $pagename (at level $level) -->\n";  
	    if($hashpages{$pagename})
	    { 
		print OUTS "$hashpages{$pagename}"; # copy to OUTS 
		delete $hashpages{$pagename};
                print OUTS "<!-- end of substitution $pagename (at level $level  $tmpstr)  -->\n";    # pick this up in write_levels to restore the level
            }
            else 
            {  # now dummy Organization.dox and docindex.dox are created so should not get this after make clean 
                die "subpage: could not find page $pagename in hashpages; page may be missing, or used already (declared as subpage twice??)\n"; 
            }
            $level = $origlevel; # back to original level
        }
        else
        {
            #no string "\subpage" so copy line across unchanged
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
	print "\n\n Item $i of $num :";  # no carriage return
	if ($_)
	{ # string is not empty
	    
	    # look for \page
	    #print "checking for page.. \n";
	    $pagename=next_field($_, "page",$i); # returns pagename if new page
	    if($pagename)
	    { #         newpage 
		print "page name : $pagename";  # has a <CR>
#	s/\n%\n/\n/; 
		if ( $hashpages{$pagename} )
		{ 
		    die "ERROR main page $pagename is already known; duplicate page in the .dox files ?? \n"; 
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
	print "next_fields: working on field: $_  (linenum $linenum)\n";
	
	if  (/(\\|@)$item/i)
	{ 
	    print "next_fields: found string \"$item\" in $_\n";
	    $value=$fields[$i];
            my $comment="";
            while ($i < $len)
	    { 
		$i++;
		$comment=$comment." ".$fields[$i];
              #  print "$i comment:$comment\n";
	    }
            chomp $comment;
            print "next_fields: returning value=$value and comment: $comment\n";
            return($value,$comment);
	} 
    }
    return (0,0);
}


        




sub next_fields_levels($$$)
{
    my $len = @_ ; # supplied params;
# parameters
    my $line=shift; #line read from sorted_info.txt
    my $item=shift; # item to be found e.g. "page"
    my $linenum=shift; # present linenumber
    my ($value);
    my (@fields,@hash,@comment);
    $_ = $line;
#    print ("next_fields_levels starting...");
#    print ("Line:$_\n");
#    print ("Item:$item\n");
#    print ("Len:$len\n");
    
    unless (/(\\|@)$item /) 
    {  # print "next_fields_levels: page $linenum: did not find $item in $_\n";
        return 0; 
    }
    # print "next_fields_levels: found $item in line:$_ \n";
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
	    print " next_fields_levels: found string \"$item\" in $_\n";
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
            print "next_fields_levels: returning value=$value and comment: \"@comment\"\n";
            return($value,@comment);
	} 
    }
    return (0,0);
}


sub header()
{
#   add title and navigation to the top of the Contents file
print OUTD<<EOT;

/*! \@page  Organization SECTION 15: MIDAS Manual Contents
\\htmlonly
<script type="text/javascript">
// bot {top bottom}
// bot("Organization","end");
// pages { previous, index, next, top, bot }
pages("","","O_Contents_Page","","");
//az();
</script>
\\endhtmlonly
<br><br>
 
<!--   This file ( Organization.dox ) produced automatically  by make_contents.pl  

                   DO NOT EDIT     
-->

\@section O_what List of Sections in the Midas Manual
<span  style=" font-size: 120%;">This manual is divided into the \\b sections listed in the following table:</span>

\\anchor Organization_section_index

<table style="text-align: left; width: 100%;" border="0" cellpadding="1"  cellspacing="1">
<tr>
<td  colspan="2" style="vertical-align: top; font-weight: bold; background-color:  rgb(204, 204, 255); color: black; text-align: center;  font-size: 150%;">
List of Manual Sections</td>
</tr>
<tr>
<td style="vertical-align: top; font-weight: bold; background-color:  rgb(204, 204, 255); color: black; text-align: center;  font-size: 125%;">
Links to <span style="color: white;">Start</span> of Section</td>
<td style="vertical-align: top; font-weight: bold; background-color:  rgb(204, 204, 255);  color: black; text-align: center;  font-size: 125%;">
Links to Section  <span style="color: white;">Index</span></td>
</tr>
<tr>
<td  style="vertical-align: top; font-weight: bold; background-color: mistyrose;">
<ul class="c0">
<li> \@ref Top "SECTION 1: Main" 
EOT
    return;
}

sub footer()
{  # write last lines of the header to the Contents file
    print OUTD<<EOT;
</ul>
</td>
</tr>
</table>
</center>
<br>
<ul style=" font-size: 120%;">
  <li> Each <b> section </b> contains at least one <b> page</b>; most contain multiple pages.
  <li> \@ref C_Navigation "Navigation icons" are located at the top and bottom of each page. 
  <li>\@ref O_Contents_Page "Page List" 
</ul>
\@anchor end
\\htmlonly
<script type="text/javascript">
// pages { previous, index, next, top, bot }
pages("","","O_Contents_Page","","");
</script>
\\endhtmlonly
<br>
*/

/*! \@page  O_Contents_Page  List of contents of each Section

\\htmlonly
<script type="text/javascript">
// bot {top bottom}
// bot("Organization","end");
// pages { previous, page_index, next, top, bot }
pages("Organization","","","O_Contents_Page","end");
    //az();
</script>
\\endhtmlonly
<br><br>


\@anchor Main_section_index

<!---  Outer table --->

<br>
<table style="text-align: left; width: 100%;" border="8" cellpadding="5"  cellspacing="3">

<tr>
<td  colspan="2" style="vertical-align: top; font-weight: bold; background-color:  rgb(204, 204, 255); text-align: center;  font-size: 150%;">
Section Contents
</td>
</tr>
<tr>  <!-- outertable -->
<td  style="vertical-align: top; font-weight: bold; background-color: white;"> <!-- outertable -->
 

<!---  Inner table 1 --->

<table  style="text-align: left; width: 100%;\" border=\"0\" cellpadding=\"2\"  cellspacing=\"2\">
<tr>  <!---  Section 1 -->
<td style="vertical-align: top; font-weight: bold; text-align: left; background-color: white;">
<br></td>
<td style="vertical-align: top; font-weight: bold; text-align: left; background-color: white;">
<span class="secthdr"> \@ref Top "SECTION 1:  Main Page"</span>
<br>
</td></tr>  <!-- end of Section 1 -->
EOT
    return;
}



sub create_dummy_files()
{
#   1. removes previous  Organization.dox and docindex.dox 
#   2. creates dummy Organization.dox and docindex.dox so mine.pl can find them
#
    my $file1="Organization.dox";
    my $file2="docindex.dox";

    if(-e $file1) 
    {  
        print "create_dummy_files: removing old version of $file1 \n";
        system("rm -f $file1"); 
        if(-e $file1) { die "create_dummy_files: could not remove $file1 "; }
    }
    if(-e $file2) 
    {  
        print "create_dummy_files: removing old version of $file2 \n";
        system("rm -f $file2"); 
        if(-e $file2) { die "create_dummy_files: could not remove $file2 "; }
    }

    open OUT, ">$file1" or die "Can't open dummy output file $file1 : $!\n";
    print OUT<<EOT;
/*! \@page  Organization SECTION 15: Manual Contents  (dummy)
\@section O_what What can be found in this manual?
        
        This is a dummy
        
        Content to be supplied by Makefile running script "make_contents.pl"

        If successful, you will not see this message.


*/
EOT
close OUT;
    print "create_dummy_files: created dummy file $file1\n";


 
    open OUT, ">$file2" or die "Can't open dummy output file $file2 : $!\n";
    print OUT<<EOT;
/*! \@page  DocIndex SECTION 15: Alphabetical Index to Documentation 

     This is a dummy

     Content to be supplied by Makefile running script "make_index.pl"

     If successful, you will not see this message.


*/
EOT
    close OUT;
    print "create_dummy_files: created dummy file $file2\n";

    return;
}

# include this 1 at EOF so require returns a true value
1;
