#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
#
# These are the subroutines needed to make the alphabetical index in docindex.dox
#
# this file is required by make_index.pl
#
sub idx();
sub fix_idx();
sub fix_idx2();
sub check_line();
sub write_idx(@);
sub dosort($$);
#
#index files
our $indexfile; # index_add.txt contains  extra items to be added to the index (input)
                # index_add.txt is produced by index_add.pl
our $outf ="index_info.txt";
our $idxf ="docindex.tmp";
our $idxf2 ="docindex2.tmp";
our $idxd ="docindex.dox";
our @alpha=qw( a b c d e f g h i j k l m n o p q r s t u v w x y z ); 
our @skip_alpha; # needed for indexing

our $debug;
our @indent;
our $linenum;

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

    fill_indent();
    my $idxstr="idx_";
    my @array = @alpha; # a-z

#   $indexfile (index_add.info) contains info to add to the index (e.g. "format-see-event->format")
    open IN, $indexfile or print "\n\n ******  Can't open input file $indexfile : $! *****\n\n"; #non-fatal 

    while (<IN>)
       {
           chomp $_;
           unless ($_){ next; } # skip any blank lines
           if (/^#/){ next;} # skip any comments

           $_=$_."!"; # add ! to mark these as having no references
           push @array,$_; # add to the array
       }    
    
    $len=$#array+1;
    $i=0;
    while ($i<$len)
    {
	$array[$i]=$idxstr.$array[$i]; # precede with idx_
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
    my ($i,$j,$k,$l,$m);
    my @last= qw( 0 0 0 0 0 0 0 );
#    my @last1=qw( x x x x x x x );
    my (@fields, $first);
    my $letter;
    my $level;
    my ($lclast, $lcfields);
    my $flag=0; # index_add flag
    my $match;
    my @anchor; # split for anchors
    my $tempstring="";
    my ($len1,$len2);
    my @sorted;

# note: array is the only parameter, so passing directly is OK.
    my @array = @_;
   @sorted = sort { lc($a) cmp lc($b);  } @array; # sort as all lower case
#    @sorted = sort { dosort ($a,$b)} @array; # special sort to deal with hyphens # works better with regular sort
    print "\n sorted array: @sorted\n";
    $len= $#sorted;
   
    print " len=$len\n";
    $i=0;
    
  
    open OUTF, ">$idxf" or die "Can't open input file $idxf : $!\n";
    print OUTF "\n <!-- sorted array:\n @sorted \n  -->\n";
    $level=0; 



    print OUTF "\@anchor IDX_A\n";  # print an anchor for A

    print OUTF "<table  style=\"text-align: left; width: 100%;\" border=\"0\" cellpadding=\"2\"  cellspacing=\"2\">\n";
    print OUTF "<tr>\n";
    print OUTF "<td style=\"vertical-align: top; font-weight: bold; text-align: left; background-color: white;\">\n";
    print OUTF "$indent[$level]<ul class=\"j$level\"> <!-- j$level -->\n";
    $first = 1; # first time
    while ($i<$len+1)
    {
	if ($first) { $first = 0; }
        else { @last = @fields; } # remember last item for comparison (duplicates)


	$_=$sorted[$i];
        print "next sorted value $i:$_\n";
	# s/idx_//;
        @fields=qw( 0 0 0 0 0 0 0 ); # clear out all levels
        @fields=split('_');
        print "\n *****  new item; last=@last;   fields=@fields  ******** \n"; 
        $l=$#fields;
        $m=$#last;
#        print "m=$m  l=$l \n";
        while ($m < $l)  # make sure last is the same length as fields by adding blanks
        {   $last[$m+1] = " "; $m++;  }
        # print "\n *** now last=@last;   fields=@fields  ******** \n"; 
	$j=0;
        print "l=$l fields: @fields\n";
        while ($j < $l)
        {
            $j++;
            print "j=$j; level=$level;  l=$l;  last= $last[$j]\n";
            #   case may be different
            $lcfields = lc $fields[$j];
            $lclast   = lc $last[$j];
            my $duplicate = 0;
	    if ($lcfields eq $lclast)
            {
                $duplicate = 1;
		if ($fields[$j]=~/^[a-z]$/) { die "duplicate single letter $fields[$j]\n";}
                # check previous levels to be sure
                print "found a duplicate at j=$j;$lcfields=\"$lcfields\" and lclast=\"$lclast\" ";
                if ($j > 1)
                {
                    $m=$j-1;
                    print "checking higher level (j=$m)\n";
              
                    $lcfields = lc $fields[$m];
                    $lclast = lc $last[$m];
                    print "now comparing \"$lcfields\" to \"$lclast\" \n";   
                    if($lcfields ne $lclast) 
                    { 
                        print "!!! saved a duplicate \n";
                        $duplicate = 0;
                    } 
                }   
                if ($duplicate > 0)
                {
                    print "\nduplicate:j=$j level=$level last[$level]=$last[$level]; \n";
                    next;
                }
	    }
#           there could be a comment on a matching item (e.g. buffer and buffer-see-also-  )
#           or  mhttpd and mhttpd-dot-h

            my @ftemp = split /-/, $lcfields;
            my @ltemp =  split /-/, $lclast;
        
            print "split by  hyphen:  ftemp = @ftemp;  ltemp = @ltemp\n";
        
# this section gets rid of some duplications
            if ($ftemp[0] eq $ltemp[0]) 
            { 
    
               print "matched $ftemp[0] and $ltemp[0] \n"; 
      #             because of dosort, if there is a hyphen, the previous value should have it
                $len2=$#ftemp; $len1=$#ltemp;

                if ($len1 > 0)
                {   
                   if ($ltemp[1] eq "see")  # if the previous line has "-see" on it, then combine; otherwise don't
                   {                        # event-see-blah and event should combine;
                      print "\"see\" match:  $fields[$j] and $last[$j]; keeping $last[$j]\n";
                      print " at j=$j level=$level last[$level]=$last[$level]; \n";
                      next;
                   }
                  else
                   {
              #         print "NOT a \"see\" match:  $fields[$j] and $last[$j]; keeping both\n";
                   }
                }
             }
          #   else {  print " $ftemp[0] and $ltemp[0] do not match \n"; }            
            if ($fields[$j]=~/!$/)
            {  
                print "found item with \"!\" (i.e. from \"$indexfile\" ): $fields[$j]\n";
                $flag=1;  # item will be parsed and fixed up in check_line
                 
                $fields[$j]=~s/!$//;  # remove the final !
                print "now item is : $fields[$j]   and flag is set instead \n";
             #  originally these may not have had the reference. Now their format has changed to include a reference.

             #  $fields[$j]=~s/!$//;  # remove the final !
             #  $fields[$j]=~s/-(?!>)/ /g; # replace the hyphens (NOT -> )
             #  print "line: $_";
             #  print "j=$j; l=$l; @fields\n";
              
	

            }           
	    elsif ($fields[$j]=~/^[a-z]$/)
	    {  # found single letter
		$letter = uc $fields[$j] ; # convert to Upper Case
                if ($j != 1){ die "single letter $letter found at fields index $j. Expect 1\n";}
                # flag for single letter
		$k=0;  # take level down to zero for single letter
		while ($k < $level)
		{
		    print OUTF "$indent[$level]</ul>  <!-- j$level -->\n";
		    $level--;
		}
		print OUTF "\n\%\n"; #marker
                # can't seem to get this to work properly for "A" so skip it
                unless ($letter eq "A"){

                    if ($letter eq "M")
                    {
                        print OUTF "</ul>  <!-- j0 -->\n";
                        print OUTF "</td>\n";
                        print OUTF "<td style=\"vertical-align: top; font-weight: bold; text-align: left; background-color: white;\"><br>\n";
                    }

                   print OUTF "\@anchor IDX_$letter\n";} # print an anchor for this letter
                if ($letter eq "M") { print OUTF "<ul class=\"j0\"> <!-- j0 -->\n";}
		   print OUTF "$indent[$level]<li><b>$letter</b><br>\n"; # write single bold letter at level 0
		while ($j > $level)
		{ # restore level to previous
		    $level++;
		  #  print OUTF "<br>$indent[$level]<ul class=\"j$level\"> <!-- j$level --> replace by tempstring\n";
                    $tempstring=sprintf("<br>%s<ul class=\"j%s\"> <!-- j%s -->",$indent[$level],$level,$level);
		}
		next;
	    }



            
            while ($j < $level)
	    {
                print OUTF "$indent[$level]</ul>  <!-- j$level -->\n";
                $level--;
	    }

            while ($j > $level)
            { 
                $level++;
                print OUTF "$indent[$level]<ul class=\"j$level\"> <!-- j$level -->\n";
            }


            if ($j == 1)
            {
                @anchor=split('\b', $fields[$level]); # one word only for anchor
               
            

                # anchor
                print OUTF "\@anchor IDX_$anchor[0] \n"; 
                if($tempstring)
                {
                    print OUTF "$tempstring  <!--- wrote  tempstring -->\n";
                    $tempstring="";
                }
            }


            if( $j == $l)
            { # last term
                print "level=$level l=$l  last[$level]=$last[$level]\n";              
                if ($flag)
                {
                    print "flagged line: $indent[$level]<li>$fields[$level]\n";
                    print OUTF "$indent[$level]<li>$fields[$level]<br> <!-- line from  $indexfile -->\n";
                  #  print "$indent[$level]<li>$fields[$level]<br>\n";  # no link for this item
                  #  print OUTF "$indent[$level]<li>$fields[$level]<br>\n";
                    $flag = 0;
                }
                else
                {
                    print "$indent[$level]<li>\@ref $sorted[$i] \"$fields[$level]\"<br>\n";
                    print OUTF "$indent[$level]<li>\@ref $sorted[$i] \"$fields[$level]\"<br>\n";
                }		
            }
            else
            {
                print "level=$level l=$l  last[$level]=$last[$level]  j=$j\n";
                print "$indent[$level]<li>$fields[$level]<br>\n";
                print OUTF "$indent[$level]<li>$fields[$level]<br>\n";
	    }
	}
	$i++;
    }
    print "at end; i=$i level=$level l=$l  last[$level]=$last[$level]  j=$j\n";

    $level=$j;
    $j=0;
    while ($j < $level)
    {
	print  "$indent[$level]</ul>  <!-- j$level -->\n";
	print OUTF "$indent[$level]</ul>  <!-- j$level -->\n";
	$level--;
    }

#    print OUTF "*/\n";  
    print OUTF "</td></tr></table>\n";
    close OUTF; 
    print "closed $idxf\n";

    return;    
} 



sub fix_idx()
{

    my $num;
    my $i=0;
    my $string;

    my $temp=$/;
    $/ = "\n%\n";  # page separator 

    open INF, "$idxf" or die "Can't open input file $idxf : $!\n";
    open OUTF, ">$idxf2" or die "Can't open output file $idxf2 : $!\n";


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

	    if ($i == 0)  # item 0 includes <ul class "j0">
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
                if (/<b>([A-Z])<\/b>/) 
                {  push @skip_alpha,$1; } # remember which letters are skipped
		print "found no index items for letter $1 at : \n $_ \n skipping...\n";
		next;
	    }
	}
    } # while

    close OUTF;
    print "closed output file $idxf2\n";
    print "skip_alpha: @skip_alpha\n";
}

sub fix_idx2()
{
    my $num;
    my $i=0;
    my $string;
    my $linenum;
    my $letter;
    my $element;
    my $copy=1;
    my $skip=0;
    my @temp = @skip_alpha;
    my $len = $#temp +1;
    my $az = ""; # store the A-Z line for end of file

    print "fix_idx2: starting\n";
    print "fix_idx2: @temp\n";
    open INF, "$idxf2" or die "Can't open input file $idxf : $!\n";
    open OUTF, ">$idxd" or die "Can't open output file $idxd : $!\n";


# add information and navigation to top of docindex (A-Z)
    print OUTF<<EOT;
/*! \@page  DocIndex SECTION 14: Alphabetical Index to Documentation

<!--  This file (docindex.dox) produced automatically (by make_index.pl)  
       
           DO NOT EDIT 
-->
 
\\htmlonly
<script type="text/javascript">
page_up_hat();
// bot {top bottom}
bot("DocIndex","end");
contents();
</script>
\\endhtmlonly
<br><br>
EOT


	$linenum=0;
	while (<INF>)
	{
	    $linenum++;
            if ($copy)
            {
                # unless (/class="j0"/)   # find the top lines of the file (if any)
                 unless (/^<table/)   # find the top lines of the file (if any)
                {
                    if ($skip)
                    {
                        if (/-->/)
                        {
                            print "last line to skip is line $linenum : $_\n";
                            $skip=0;
                        }
                        print "skipping line $linenum : $_\n";
                        next;
                    }

                    elsif (/<!-- sorted array/)
                    {
                        # no need to copy sorted array to docindex.dox
                        print "first line to skip is line $linenum : $_\n";
                        $skip=1;
                        next;
                    }
                         
                    print "copying line $linenum: $_ to file $idxd\n";   
                }
                else 
                { 
                    $copy = 0;
                    #print "Found \"class=\"j0\"\" at Line $linenum\n";
                    print "Found \"<table\" at Line $linenum\n";
       #
       #    Add     A - Z  Alphabetical Index links at top  (one line) 
       #
                      print OUTF "<div class=\"az\">\n";
                    # add Alphabet Index links A B C D etc.  BEFORE line with "table"
                     print  "\n";
                    $letter="done";
                    print "skipping letters  temp: @temp   len=$len\n";
                    if ($len > 0)
                    {
                        $letter = shift @temp; # the first letter to be skipped
                        $len--;
                    }
                     print ("skip letter = $letter  len= $len\n");

                  
                     for $element (@alpha)  # a-z
                     { 
                         $element = uc $element;
                         #  print " element=$element, letter=$letter\n";
                         if ($element eq $letter)
                         {   # skip this letter
                               print "...skipping letter $letter \n";
                             # next letter to skip
                             $letter="done";
                             if ($len > 0)
                             {
                                 $letter = shift @temp; # the first letter to be skipped
                                 $len--;
                             }
                 
                             print ("skip letter = $letter  len= $len\n");


                         }
                         else
                         {
                           #  print  "\@ref IDX_$element \"$element\" \n";
                           # can't seem to get anchor to work properly for "A" so skip it here
                           # A's anchor IDX_A is added to top of file
                           #  unless ($element eq "A")
                                  {  print OUTF  "\@ref IDX_$element \"$element\"&nbsp;|&nbsp;"; 
                                     $az = $az."\@ref IDX_$element \"$element\"&nbsp;|&nbsp;"; # store for index at EOF
                                 }
                              }
         
                     } # end of for loop
                    print OUTF "\n</div>\n\n";
                } 
                 print OUTF "$_ \n";
                 

                 next;
             } 
            
            print "working on line $linenum\n";
  # try a few substitutions
            chomp $_;
            my $string = $_;
            check_line(); # passes string with $_
                  
            if($string eq $_)
            {   
              #  print OUTF "$_\n"; #     $string has not changed
            }
            else 
            { 
                print "line $linenum:  $string: $string \n   has changed to $_\n";
            #    print OUTF "<!-- $string -->\n";  # debug...  keep a copy of original 
            }
                print OUTF "$_\n";

        } # end of file 
	close INF;


    print OUTF "</ul>  <!-- j0 -->\n"; # close list level j0 

    #
    # Add   A - Z  Alphabetical Index links at EOF  (one line) 
    #

    print OUTF "\n<div class=\"az\">\n";
    print OUTF "$az";  # print A-Z index
    print OUTF "\n</div>\n\n";
    
    # add the docindex page footer (at end of the page)
    print OUTF<<EOT;

\\anchor end
\\htmlonly
<script type="text/javascript">
top("DocIndex");    
contents();
</script>
\\endhtmlonly
*/
EOT

    close OUTF;

    print "closed output file $idxd\n";
}



# dosort not used at present
sub dosort($$)
{
    my $a = shift;
    my $b = shift;
    my $code;
    my ($a1,$b1,$l1,$l2);
    my (@array1,@array2);
    
    
    $l1=$l2=0;
    if ($a=~/-/)
    {
        $_ = $a;
        @array1 = split ('-');
        $l1 =$#array1; 
        # print "array1: @array1; l1=$l1\n";
    }
    if ($b=~/-/)
    {
        $_ = $b;
        @array2 = split ('-');
        $l2 =$#array2; 
        #  print "dosort: array2: @array2; l2=$l2\n";
    }
    
    if ($l1 == $l2) # neither or both have hyphens
    {
        $code = (lc($a) cmp lc($b)); # compare lower case
        #   print "dosort: a=$a; b=$b; code = $code\n";
        return ($code);
    }
    
    $a1 = $a; $b1 = $b;
    if ($l1 > 0){ $a1 = $array1[0];}
    if ($l2 > 0){ $b1 = $array2[0];}
    
    #  print "dosort: found hyphen; comparing a1=$a1; b1=$b1;\n";
    $code = (lc($a1) cmp lc($b1)); 
    if ($code == 0)
    {
        print "dosort: a=$a; b=$b; code = $code\n";  # the same
        if ($l1 > 0 ) { $code = -1 };
        if ($l2 > 0 ) { $code = 1 };
        print "dosort: original code is 0; now code is $code\n";
        
    }
    
    
    return ($code);
}



sub check_line()
{
    my $temp;
    my @fields;
    my $len;
    my $flag=0;
    
    print "\n\n check_line starting with string: $_\n";
    if (/$indexfile/)
    {
        print "Flagged line (i.e. line from  $indexfile  !! \n";
        $flag=1;
    }


#    s/"experim-dot-h"/"experim\.h"/;
    if (/"(.*-dot-)/)
    { 
        print "found dot; $1\n"; 
        $temp = $1;
        $temp =~ s/-dot-/./;
        print "temp: $temp\n";
        s/"(.*-dot-)/"$temp/;    
     }



        
    if (/"(.*-see-)/) 
    {
        print "\"-see-  found $1 \n";
        $temp = $1;
        $temp =~ s/-see-//;
        print "temp: $temp\n";
        
        s/"(.*-see-)/"\(see /; 
        print "$_ \n";
        s/"(?!\()/\)"/;
        print "$_ \n";
        s/\@ref/$temp \@ref/;              
    }




#  Reminder
#  ?< look behind              ? lookahead
#  ?<! look behind negatively  ?! lookahead negatively
#
# make sure we only get a single dash
#    elsif (/"(.*(?<!-)-(?!-))/)) 




  if (  /"(.*(?<!-)-(?!-))/ ) 
  { 
     print "single dash within quotes ... found $1 \n";
    
     print "$_\n";
     
     @fields= split /\"/, $_;
     $len = $#fields;
     print "fields @fields   len:$len\n";

     $_ = $fields[1];
     print "Working on $_\n";
     s/(?<!-)-(?!(-|>))/ /g; # substitute single dashes only (leave comments and ->)
     print "Substituted single dashes only: $_\n";       
  
     $fields[1] = $_;


#	print "working on field: $_\n";
     print "fields: @fields\n";
     
     $_ = join "\"", @fields;
  
     print "Complete Line is now $_\n";
     

     }

    if ($flag)
    {
        print "Flag is set... looking for any remaining single dashes...\n";

        if (/(.*(?<!-)-(?!-))/ ) 
        {


           print "single dash NOT within quotes ... found $1 \n";
        
           s/(?<!-)-(?!(-|>))/ /g; # substitute single dashes only (leave comments and ->)
           # print "Now line is $_\n";
           if (/on start param/){ s/on start/on-start/;} # restore hyphen for edit-on-start parameters


           print "Substituted single dashes only: $_\n";
           print "Now line is $_\n";
        }

        # line from  $indexfile  may have @ref IDX=  (underscores marked by =)
        print "Line $_ is from  $indexfile \n";
        s/=/_/g;
        s/ see also/ <span class="see">see also<\/span> /;
        s/ see / <span class="see">see<\/span> /;
        #s/!//;  # remove the first !
        print "Now line is $_\n";
       
     

    }

    
     print "check_line: returning line  : $_ \n";  
     
     return ($_);
}   



# include this 1 at EOF so require returns a true value
1;
