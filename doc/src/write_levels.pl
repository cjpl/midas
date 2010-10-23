
########################################################################################################
#  
# Note: for the purposes of the contents, subpages cannot be declared in sections unless \level is used
#       this restrictions is imposed for ease of scripting
#
#       declare at top or bottom of a page unless using \level
#
########################################################################################################
sub write_vertsect($);

sub write_levels()
{
    my $linenum;
    my $pagename=0;
    my $sectname=0;
    my $subsectname=0;
    my $subsubsectname=0;
    my ($level, $il);
    my $pagelevel=0;
    my $section_index="_section_index";
    my @comment;
    my $myline="";
    my $pageflag=0;
    my $line="";
    my $gotcha;
    my $tmplevel;
    my $wantlevel;
    my $gotsec;
    my $sectioncntr=1;
    my $first=1;
    my $nextsection;
    my $len;
    my %hashpagecopy;
    my @contents;

    open IN, "$sortfile" or die "Can't open input file $sortfile : $!\n";
    open OUT, ">$tempfile2" or die "Can't open output file $tempfile2 : $!\n";
    open OUTD, ">$hdrfile" or die "Can't open output file $hdrfile : $!\n";
    fill_indent(); # fill indent array with blanks for each level
    header(); # write fixed header into $hdrfile
#my @name= split /./,$hdrfile ; #Organization

#print "\n hashpagelevel....\n";    
#for(keys %hashpagelevel) 
#{ print  "Page $_ Level $hashpagelevel{$_}\n";}

    $pagelevel=$level=1;
    $il=0;
    print "\n\n==============================================================\n";
    print "write_levels starting...  working on $sortfile\n";
    print " output files are $tempfile2 (input to doit.pl)  and $hdrfile\n";
    print "\n================================================================\n\n";    

   
    $linenum=0;
    while (<IN>)
    {
        $linenum++;
        chomp $_;
        unless ($_){ next; } # blank line
        print "\n write_levels:  working on line $linenum of $sortfile:\n";
        print "     $_ \n";       
        
        ### Organization has been moved to the end (section 15) 
        ###if ($linenum < 7) { next; } # skip lines containing Organization
        
#       Expect first line of sorted_info.txt to contain "mainpage"
        if ($linenum < 2) 
        {  # skip line containing Mainpage
            if (/mainpage/i) 
            {
                print "found mainpage... skipping line $_ \n";
                next; 
            }
            else
            {
                print "write_levels: error... expect first line of sorted_info.txt to contain \"mainpage\" \n";
                exit; 
            }
        }
# remove dummy Organization from end of file
        if (/Organization/) 
        {
            print "found \"Organization in line %linenum of $sortfile \n";
            print " ... skipping this line:  $_ \n";
            next; 
        }


        
        if(/^\<!--/)
        {  # comment
        # SPECIAL CASE...   look for any \level info  e.g. <!-- end of substitution $pagename (at level $level  @level-2)  -->
            if(/end of substitution/)
            {  
                print "write_levels: end-of-substitution... checking for any @level information\n";
                my $origlevel=$pagelevel;
                $pagelevel = set_sublevel($_, $linenum, $origlevel);
                if ($pagelevel != $origlevel)
                {
                    print "change of pagelevel detected after substitution ($origlevel -> $pagelevel) in line $_\n";
                    $pagelevel--;
                    print "decreasing one more level for end-of-page; pagelevel is now $pagelevel \n";  # fudge factor
                }
            }
            
            print OUT "$_\n";
            next;
        }
        
	print "write_levels:  line $linenum;  pagelevel:$pagelevel;   level:$level\n";

        
# look for begin of page 
        @comment=0;
        
        ($pagename,@comment)=next_fields_levels($_, "page",$linenum); # returns page name
        if($pagename)
        {
            print "after next_fields, $pagename and comment:\"@comment\"\n";
            print "and previous=$previous\n";
            s/\\page/\@ref/;
            $pagelevel = $hashpagelevel{$pagename};
            unless ($pagelevel) {die "write_levels: no pagelevel found for page $pagename\n";}
            delete $hashpagecopy{$pagename};
            print "page $pagename pagelevel= $pagelevel, present level=$level\n";
            
            if($level < 1) { die "write_levels: $level is 0 at \\page, expect level >= $pagelevel\n";}
            
            if ($level > $pagelevel)
            {
                print "New page $pagename: level $level. Reducing level to pagelevel of new page $pagelevel \n";
                while ($level > $pagelevel)
                {
                    $il=$level-1; # indent level
                    print OUT "$indent[$il]</ul> <!-- end of i$level (new page: reducing level to $pagelevel) -->\n"; # down 1
                    $level--;
                }
            }       
            elsif ($level < $pagelevel )
            {   
                print "New page $pagename: level $level. Increasing level to $pagelevel\n";
                while ($level < $pagelevel)
                {
                    $il=$level;
                    $level++; # up 1 level
                    print OUT "$indent[$il]<ul class=\"i$level\"> <!-- new page: increasing level to i$level  -->\n";	
                }
            }
            
#       special case - pagelevel 1
 
            if ($pagelevel ==1)  # New Section 
            {  
                unless ($first) { print OUT "</td></tr>  <!-- end of section $sectioncntr -->\n"; }
                else { $first=0; };

                $nextsection = $sectioncntr + 1; # next section number
                if ($nextsection == $section_next_column)  # start a new column with this section
                {   # new column
                    
                    print OUT "</table> <!-- end of inner table 1  --> \n";
                    print OUT "<td style=\"vertical-align: top; font-weight: bold; text-align: left; background-color: white;\">   <!-- outer table -->\n";
                    print OUT "<table style=\"text-align: left; width: 100%;\" border=\"0\" cellpadding=\"2\" cellspacing=\"2\"> <!-- open inner Table 2 -->\n";

                }
                print OUT "<tr>  <!--  Section $nextsection --> \n";
                print OUT "<td style=\"vertical-align: top; font-weight: bold; text-align: left; background-color: white;\">\n";
                write_vertsect($sectioncntr); # write section name vertically
                print OUT "<td style=\"vertical-align: top; font-weight: bold; text-align: left; background-color: white;\">\n";
                print "Page Level 1  comment=\"@comment\"\n";
                print OUT "\n\n<!--                         Section : $comment[0] (level $level)  -->\n";
                print OUT "\\anchor $pagename$section_index\n"; # match name to ref on next line
#                print OUTD "<li>\@ref $pagename$section_index \"@comment\"  <!-- level $level -->  \n";
#               add section to "List of Sections" table 
#                $contents [$sectioncntr]= "\@ref $pagename$section_index  \"@comment\"  <!-- level $level --> <br>";
                $contents [$sectioncntr]= "\@ref $pagename$section_index";
#                print OUTD "<li> \@ref $pagename \"@comment\"  --> \@ref $pagename$section_index \"contents\"  <!-- level $level -->  \n";
                print OUTD "<li> \@ref $pagename   <!-- level $level -->  \n";
                print OUT "<br>\n";
                $sectioncntr++;
                print OUT "<span class=\"secthdr\">\@ref $pagename</span> "; # write section header i.e. SECTION N BLAH in contents page

            }
            else
            {
                if ($pagelevel ==2){  print OUT "<br>\n";} # add a linefeed
                print OUT "$indent[$level]<li> \@ref $pagename <!-- subpage $pagename at level $level -->\n";
            }
            next;
        } # end of new page
    
#end-of-page
        $pagename=next_field($_, "end-of-page",$linenum); # returns page name
        if($pagename)
        {       
            $pagelevel = $hashpagelevel{$pagename};
            unless ($pagelevel) {die "write_levels: end-of-page; no level found for page $pagename\n";}
            print "end-of-page $pagename ; present level=$level, pagelevel=$pagelevel\n";
                        
            # return to the initial pagelevel for this page

	    
            if($level < 1) { die "write_levels: illegal level $level at end-of-page (previous=\"$previous\")\n";}
            print "end-of-page $pagename pagelevel $pagelevel present level $level\n";
            # at the end-of-page,  level should not be less than $pagelevel
            if($level < $pagelevel) 
            { 
                die "write_levels: error with $sortfile line $linenum - at end-of-page $pagename (pagelevel $pagelevel) expect level=$level >= $pagelevel\n";
            }
            
            while ($level > $pagelevel)
            {
                $il=$level-1; # indent level
                print OUT "$indent[$il]</ul> <!-- end of i$level  at end-of-page $pagename -->\n"; # down 1	
                $level--; # down 1
            }
            print "end-of-page $pagename pagelevel ... now $pagelevel level $level\n";
            next;
            
        } # end of end-of-page
    
# section 
        $gotsec = 0; # flag
        $sectname=next_field($_, "section",$linenum); # returns section name
#    print "looking for section; $sectname=$sectname \n";
        if($sectname)
        {
            s/\\section/\@ref/;
            # section is one level up from pagelevel
            $wantlevel = $pagelevel + 1;
            $gotsec=1;
        }
        else
        {
# subsection
            $subsectname=next_field($_, "subsection",$linenum); # returns subsection name
#    print "looking for subsection; $subsectname=$subsectname \n";
            if($subsectname)
            {
                s/\\subsection/\@ref/;
                # subsection is two levels up from pagelevel
                $wantlevel = $pagelevel + 2;
                $gotsec=1;
            }     
            else
            {
# subsubsection
                $subsubsectname=next_field($_, "subsubsection",$linenum); # returns subsubsection name
#   print "looking for subsubsection; $subsubsectname=$subsubsectname \n";
        
                if($subsubsectname)
                {
                    s/\\subsubsection/\@ref/;
                    # subsubsection is three levels up from pagelevel ... unless there's level information... 
                    $wantlevel = $pagelevel + 3;
                    my $tmplevel = $wantlevel;
                    $wantlevel = set_sublevel($_, $linenum, $tmplevel);
                    if ($wantlevel != $tmplevel){ print "subsubsubsection  change in level from $tmplevel to $wantlevel\n";} 
                    $gotsec=1;
                }
            }
        }
        if ($gotsec)
        { # got a section/subsection/subsubsection  
            while ($level > $wantlevel)
            {
                $il=$level-1; # indent level
                print OUT "$indent[$il]</ul> <!-- end of level i$level   -->\n"; # down 1	
                $level--; # down 1
            }
            while ($level < $wantlevel)
            {
                $il=$level;
                $level++; # up 1 level
                print OUT "$indent[$il]<ul class=\"i$level\"> <!-- start of level i$level  -->\n";	
            }             
            print OUT "$indent[$level]<li> $_   <!-- level $level -->\n"; 
            next;
     
        }
        
        print " line unmodified $_ \n";
        print OUT "$_\n";
        
    } # while in

# add last item - link to list of Sections    
# add navigation to end of the Contents file

    print OUT<<EOT;
<br><br>
<!-- this last Section added by write_levels.pl -->
<li><span class=\"secthdr\"> \@ref Organization </span>
   <ul class="i2"> <!-- start of level i2  -->
      <li> \@ref O_what  <!-- level 2 -->
      <li> \@ref O_Contents_Page  <!-- level 2 -->
   </ul> <!-- end of i2  at end-of-page O_Section_Index -->
</ul>
</td></tr></table>  <!-- inner table 2 -->
</td></tr></table>  <!-- outer table  -->
<br>
\\anchor end
\\htmlonly
<script type="text/javascript">
// top {top }
// top("Organization");
// pages { previous, page_index, next, top, bot }
pages("Organization","","","O_Contents_Page","");
// az(); 
</script>
\\endhtmlonly
<br>
*/
EOT

   close $sortfile;
   close $tempfile2;
   print "closed $sortfile and $tempfile2\n";
    
  #  print OUTD  "<li>\@ref Organization  \n";
    print OUTD  "</ol></td>\n";
    #
    # Write Manual Sections Table  column 2 (Links to Section Indices)
    #
    print OUTD  "<td  style=\"vertical-align: top; font-weight: bold; background-color: mintcream;\">\n";

    my @spiel =  qw ( Main Intro Quick-Start Features Run-Control Frontend-Operation Data-Analysis Performance Special-Configurations
                      Build-Options New-Features Frequently-Asked-Questions Convention Alphabetical Manual-Contents);

    $contents[0]="\@ref O_Contents_Page"; # Main index (top of Section contents page)
    my $elem;
    my $len = $#contents + 1;
    $contents[$len]="\@ref O_Contents_Page"; # Section contents page
    print OUTD "<ul class=\"c1\">\n";
    my $i=0;
    for $elem (@spiel)
    {
        $elem=~s/-/ /g;
    }
    for $elem (@contents)
    {
        print OUTD "<li> $elem \"$spiel[$i] index\" \n";
        $i++;
    }

    footer(); # write the last lines of the header to OUTD ( $hdrfile )
    
    close IN;
    close OUTD;


    print "\n  output files are $tempfile2 and $hdrfile\n";
    print "done. Now run \"doit.pl \"\n";

    return;
}

sub  write_vertsect($)
{
#   vertical section name
    my @section_array = qw ( Intro Quick_Start Features Run_Control Frontend_Operation Data_Analysis Performance Special_Config Building_Options New_Features );
    my @repeat        = qw ( 1 1 5 13 4 0 0 0 1 1 ); # 0 = don't write name vertically;  1 = write it once
    my ($string,$rep,$i);
    my $index = shift;
    $index--; 
    my $len = $#section_array;
    if ($index < 0 || $index > $len) 
    { 
        $string="<br>"; 
    }
    else 
    { 
        $string =  $section_array[$index]; 
        $rep = $repeat[$index];
        $i=0;
        if ($rep)
        {
            $i=1; # also indicates we need </span>       
            print "string = $string\n\n";
            $string =~ s/([a-z]|[A-Z])/$1<br>/g ;
#    print "string is now $string\n";
            $string =~ s/_/ <br>/g ;
            $string = "<br><br><br>".$string;
            print "string is now $string\n\n";
            print OUT "<span class=\"vertsect\">\n";
            while ($i < $rep)
            {
                print OUT "$string\n";
                print OUT "<br>\n";
                $i++;
            }
        }
        else
        { # not enough contents for vertical section name
            $string="<br>"; 
        } 
    }
    print OUT "$string\n";
    if ($i) { print OUT "</span>\n"; }
    print OUT "</td>\n";
    return;
}

# include this 1 at EOF so require returns a true value
1;
