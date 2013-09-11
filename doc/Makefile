#####################################################################
#
#  Name:         Makefile
#  Created by:   Suzannah Daviel, Konstantin Olchanski
#
#  Contents:     Makefile for MIDAS documentation
#
#  $Id$
#
#####################################################################
#
# Usage:
#        make              To compile the MIDAS documentation
#        make publish      To generate tarballs and copy them to the main MIDAS web site
#
#####################################################################

# Midas and Project version number
version = 2.3.0

all::	message

ver:	
	echo "Frozen at version $(version)"

html: index rmtmp
	doxygen
#	Javascript : copy files and required images to html
	cp src/*.js html/.
	cp src/*.css html/.
	#cp images/page_*.gif html/.
	#cp images/section_*.gif html/.

#images/done: midasdoc-images.tar.gz
#	tar xzvf midasdoc-images.tar.gz
#	touch $@

#pdf:	latex/refman.tex
#	cd latex; latex refman
#	cd latex; dvips -Ppdf -o midas.ps refman.dvi
#	cd latex; ps2pdf midas.ps midas.pdf ; cp midas.pdf ../.

publish:
	#tar -czvf midasdoc-html-$(version).tar.gz html/
	#tar -czvf midasdoc-man-$(version).tar.gz man/
    #tar -czvf midasdoc-images.tar.gz images/
	#rsync -uv html/* /home/daqweb/public_html/doc/midas/html/.
	#rsync -av *.tar.gz daqweb@ladd00.triumf.ca:/home/daqweb/public_html/ftp/.
	#rsync -av /midas/doc/CHANGELOG.TXT /home1/ftp/pub/midas/.

clean:	
	#cd latex; rm -f *.*~ *.pnm *.tex *.log *.aux *.dvi *.eps
	#-rm -rf html
#	remove docindex.dox and Organization.dox created by perlscripts
	#rm -f src/docindex.dox src/Organization.dox src/*.txt       

index:
#       make the Contents and Index pages (Organization.dox, docindex.dox)
#       output files *_output.txt are for debug.
	cd src ; ./make_contents.pl >& contents_output.txt ; ./doit.pl >& doit_output.txt ; ./make_index.pl >& index_output.txt ;   cd ..
rmtmp:
#	remove temporary files created by make_index.pl
	rm -f src/*.tmp src/sorted_info.txt.*
# end

message:
	@echo "  " \  
	@echo " ======================================================================== " \ 
	@echo " =  This version of the MIDAS Doc is meant for checking the C-code doc  = " \ 
	@echo " =  visible here http://ladd00.triumf.ca/~daqweb/doc/midas/doc/html     = " \   
	@echo " =  You can view the pre-git doc                                        = " \  
	@echo " =    http://ladd00.triumf.ca/~daqweb/doc/midas-old/html/index.html     = " \  
	@echo " =                                                                      = " \   
	@echo " = It is in the process of being replaced by a wiki at                  = " \ 
	@echo " =  https://midas.triumf.ca/MidasWiki/index.php/Midas_documentatio      = " \ 
	@echo " ======================================================================== " \  
	@echo "  " \          
# end
