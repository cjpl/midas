// Navigation Images
//
// Page:                        Section:
//       page_right.gif                 section_right.gif       Next
//       page_left.gif                  section_left.gif        Previous
//       page_up.gif                    section_up.gif          Top
//       page_up_hat.gif                                        Beginning
//       page_down.gif                                          Bottom
//       page_contents.gif              section_contents.gif    Contents list
//       page_index.gif                                         Alphabetical Index
//

// Used only by first page of document
function top_mainpage( next_page,bottom_of_page)
{
   if(next_page)
   {
      document.writeln('<a href="'+next_page+'.html">                <!-- Next Page -->')
      document.writeln(' <img  ALIGN="left" alt="->" src="page_right.gif" title="Next Page: '+next_page+'" border=0>');
      document.writeln('</a>')
   }

//   page_index = "Organization"  
//   var ref = '#'+page_index+'_section_index';
//   document.writeln('<a href="Organization.html'+ref+'">  <!-- PAGES IN THIS SECTION -->');
//   document.writeln(' <img  ALIGN="left" alt="map" src="page_contents.gif" title="Page List"    border=0 >');
//   document.writeln('</a>'); 
   

   document.writeln('<a href="DocIndex.html">  <!-- docindex -->');
   document.writeln(' <img  ALIGN="left" alt="/\" src="page_index.gif" title="Alphabetical Index"   border=0 >');
   document.writeln('</a>');
   
   if(bottom_of_page)
      {  // bottom arrow only
         document.writeln('<a href="index.html#'+bottom_of_page+'">  <!-- BOTTOM OF PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="\/" src="page_down.gif" title="Bottom of Page"   border=0 >');
         document.writeln('</a>');
      }
//document.write('<div style="color: teal;">Page');
//document.write('</div>'); 
}


// Single functions (needed by Contents page)

// az A-Z Index symbol
function az()
{
   document.writeln('<a href="DocIndex.html">  <!-- docindex -->');
   document.writeln(' <img  ALIGN="left" alt="/\" src="page_index.gif" title="Alphabetical Index"   border=0 >');
   document.writeln('</a>');
}


// Bottom of page symbol; bottom_of_page is usually "end"
function bot(top_of_page,bottom_of_page)
{
      if(top_of_page)
      {  // bottom arrow only
         document.writeln('<a href="'+top_of_page+'.html#'+bottom_of_page+'">  <!-- BOTTOM OF THIS PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="\/" src="page_down.gif" title="Bottom of Page"   border=0 >');
         document.writeln('</a>');
      }

}
// Top of page symbol
function top(top_of_page)
{
      if(top_of_page)
      {
         document.writeln('<a href="'+top_of_page+'.html">  <!-- TOP OF THIS PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="/\" src="page_up.gif" title="Top of Page"   border=0  >');
         document.writeln('</a>');
      }   
}

// Start-of-Document  (page up hat)  symbol
function page_up_hat()
{
   document.writeln('<a href="index.html#Top">  <!-- TOP OF DOCUMENT blue with line -->');
   document.writeln(' <img  ALIGN="left" alt="top" src="page_up_hat.gif" title="Start of Document"    border=0>');
   document.writeln('</a>');
}

// Contents symbol
function contents()
{
   document.writeln('<a href="Organization.html">  <!-- LIST of section names -->');
   document.writeln(' <img  ALIGN="right" alt="map" src="section_contents.gif" title="Section List"    border=0 >');
   document.writeln('</a>');     
}

// end of single functions
//


// compound functions used by almost all the pages

function pages(previous_page,  page_index, next_page, top_of_page, bottom_of_page  ) // bottom is a reference #
{
   var string;
   var result;
   var pattern=/#/g;
   
   if(previous_page)   
   {
     result = previous_page.match(pattern); // is there a # in this link?
     if (result)
       doprev(previous_page);
     else
     {       
       string = clean(previous_page);
       document.writeln('<a href="'+previous_page+'.html#end">      <!-- Previous Page -->');
       document.writeln(' <img  ALIGN="left" alt="<-" src="page_left.gif" title="Previous Page: '+string+'"   border=0 >');
       document.writeln('</a>');
     }
   }



   document.writeln('<a href="index.html#Top">  <!-- TOP OF DOCUMENT blue with line -->');
   document.writeln(' <img  ALIGN="left" alt="top" src="page_up_hat.gif" title="Start of Document"    border=0>');
   document.writeln('</a>');

   
   // top AND bottom --> bottom arrow
   // top only       --> top arrow   (at present either top or bottom)
   if(top_of_page)
   {
      if(bottom_of_page)
      {  // bottom arrow only
         document.writeln('<a href="'+top_of_page+'.html#'+bottom_of_page+'">  <!-- BOTTOM OF THIS PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="\/" src="page_down.gif" title="Bottom of Page"   border=0 >');
         document.writeln('</a>');
      }
      else
      {   
         document.writeln('<a href="'+top_of_page+'.html">  <!-- TOP OF THIS PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="/\" src="page_up.gif" title="Top of Page"   border=0  >');
         document.writeln('</a>');
      }
    }
   if(page_index)
   {
      var ref = '#'+page_index+'_section_index';
      document.writeln('<a href="O_Contents_Page.html'+ref+'">  <!-- PAGES IN THIS SECTION -->');
      document.writeln(' <img  ALIGN="left" alt="map" src="page_contents.gif" title="Page List"    border=0 >');
      document.writeln('</a>'); 
   }

   document.writeln('<a href="DocIndex.html">  <!-- docindex -->');
   document.writeln(' <img  ALIGN="left" alt="/\" src="page_index.gif" title="Alphabetical Index"   border=0 >');
   document.writeln('</a>');
   
   if(next_page)
   {
     result = next_page.match(pattern); // is there a # in this link?
     if (result)
       donext(next_page);
     else
     {       
       string = clean(next_page);
       document.writeln('<a href="'+next_page+'.html">             <!-- Next Page -->')
       document.writeln(' <img  ALIGN="left" alt="->" src="page_right.gif" title="Next Page: '+string+'" border=0>');
       document.writeln('</a>')
     }
   }
//document.write('<div style="color: teal;">Page');
//document.write('</div>');
}


function sections(previous_section, section_top, next_section)
{
      
   var string;

   if(next_section)
   {
      string = clean(next_section);
      document.writeln('<a href="'+next_section+'.html">               <!-- Next Section -->');
      document.writeln('<img  ALIGN="right" alt="->>" src="section_right.gif" title="Next Section: '+string+'" border=0 >');
      document.writeln('</a>');
   }
 
   
 
   document.writeln('<a href="Organization.html">  <!-- LIST of section names -->');
   document.writeln(' <img  ALIGN="right" alt="map" src="section_contents.gif" title="Section List"    border=0 >');
   document.writeln('</a>');     


   if(section_top)   
   {
      string = clean(section_top);
      document.writeln('<a href="'+section_top+'.html">    <!-- Top of this Section -->');
      document.writeln('<img ALIGN="right" alt="-" src="section_up.gif" title="Top of Section '+string+'" border=0 >');
      document.writeln('</a>');
   }

  if(previous_section)
   {
      document.writeln('<a href="'+previous_section+'.html">         <!-- Previous Section -->');
      document.writeln('<img ALIGN="right" alt="-" src="section_left.gif" title="Previous Section: '+previous_section+'" border=0 >');
      document.writeln('</a>');
   }

   document.writeln('</a>');
//document.write('<div style="text-align: right;color: aqua;">Section<br>')
//document.write('</div>');

  document.write('<br><br>');
}

function doprev(string)
{
// there is a # in the string, e.g. RC_blah_blah#RC_rabbit
      var pattern=/#/g;
      var string1,string2;
      
      string1 =string.replace(pattern,'.html#');
      document.writeln('<a href="'+string1+'">      <!-- Previous Page -->');
      string2 = clean(string);
      document.writeln(' <img  ALIGN="left" alt="<-" src="page_left.gif" title="Previous Page: '+string2+'"   border=0 >');
      document.writeln('</a>');
        
    return;    
}        

function donext(string)
{
// there is a # in the string, e.g. RC_blah_blah#RC_rabbit
      var pattern=/#/g;
      var string1,string2;
      
      string1 =string.replace(pattern,'.html#');
      document.writeln('<a href="'+string1+'">                <!-- Next Page -->')
      string2 = clean(string);
      document.writeln(' <img  ALIGN="left" alt="->" src="page_right.gif" title="Next Page: '+string2+'" border=0>');
      document.writeln('</a>')
}




function clean(string)
{
// try to make the next page string (e.g. RC_blah_blah) look more like English (blah blah)
// for the mouse-over information
//
// also look for a # in the string, e.g. RC_blah_blah#RC_rabbit

   var debug=0;
   if (debug) document.write( '<br>clean: string: '+string);

    var pattern=/^[A-Z]+_/;
    var pattern2=/_/g;
    
    var result = string.match(pattern);
    if(debug) document.write('<br>pattern: '+result);
    if (result)
    {
       string =string.replace(pattern,''); 
       if (debug) document.write('<br>now string: '+string);
    }
    var result = string.match(pattern2);
    if (result)
        string =string.replace(pattern2,' ');
    if (debug) document.write('<br>now string: '+string);  
    return string; 
}

