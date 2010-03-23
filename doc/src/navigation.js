function top_mainpage( next_page,bottom_of_page)
{
   if(next_page)
   {
      document.writeln('<a href="'+next_page+'.html">                <!-- Next Page -->')
      document.writeln(' <img  ALIGN="left" alt="->" src="next_page.gif">');
      document.writeln('</a>')
   }
   if(bottom_of_page)
      {  // bottom arrow only
         document.writeln('<a href="index.html#'+bottom_of_page+'">  <!-- BOTTOM OF PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="\/" src="bottom_page.gif">');
         document.writeln('</a>');
      }
 document.write('<div style="color: teal;">Page');
document.write('</div>'); 
}


function pages(previous_page,  page_index, next_page, top_of_page, bottom_of_page  ) // bottom is a reference #
{
   if(previous_page)   
   {
      document.writeln('<a href="'+previous_page+'.html#end">      <!-- Previous Page -->');
      document.writeln(' <img  ALIGN="left" alt="<-" src="previous_page.gif">');
      document.writeln('</a>');
   }

   document.writeln('<a href="index.html#Top">  <!-- TOP OF DOCUMENT -->');
   document.writeln(' <img  ALIGN="left" alt="top" src="top.gif">');
   document.writeln('</a>');
   // top AND bottom --> bottom arrow
   // top only       --> top arrow   (at present either top or bottom)
   if(top_of_page)
   {
      if(bottom_of_page)
      {  // bottom arrow only
         document.writeln('<a href="'+top_of_page+'.html#'+bottom_of_page+'">  <!-- BOTTOM OF PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="\/" src="bottom_page.gif">');
         document.writeln('</a>');
      }
      else
      {   
         document.writeln('<a href="'+top_of_page+'.html">  <!-- TOP OF PAGE -->');
         document.writeln(' <img  ALIGN="left" alt="/\" src="top_page.gif">');
         document.writeln('</a>');
      }
    }
   if(page_index)
   {
      document.writeln('<a href="Organization.html#'+page_index+'"> <!-- PAGES IN THIS SECTION MAP -->');
      document.writeln(' <img  ALIGN="left" alt="map" src="map_page.gif">');
      document.writeln('</a>'); 
   }
   if(next_page)
   {
      document.writeln('<a href="'+next_page+'.html">                <!-- Next Page -->')
      document.writeln(' <img  ALIGN="left" alt="->" src="next_page.gif">');
      document.writeln('</a>')
   }
document.write('<div style="color: teal;">Page');
document.write('</div>');
}


function sections(previous_section, section_top, next_section)
{

   if(next_section)
   {
      document.writeln('<a href="'+next_section+'.html">               <!-- Next Section -->');
      document.writeln('<img  ALIGN="right" alt="->>" src="forward_sec.gif">');
      document.writeln('</a>');
   }
 

   document.writeln('<a href="Organization.html">  <!-- MAP of section names -->');
   document.writeln(' <img  ALIGN="right" alt="map" src="map_sec.gif">');
   document.writeln('</a>');     


   if(section_top)   
   {
      document.writeln('<a href="'+section_top+'.html">    <!-- Top of this Section -->');
      document.writeln('<img ALIGN="right" alt="-" src="top_sec.gif">');
      document.writeln('</a>');
   }

  if(previous_section)
   {
      document.writeln('<a href="'+previous_section+'.html">         <!-- Previous Section -->');
      document.writeln('<img ALIGN="right" alt="-" src="backward_sec.gif">');
      document.writeln('</a>');
   }
document.write('<div style="text-align: right;color: aqua;">Section<br>')
document.write('</div>');
}
 


