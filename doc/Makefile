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

all::	html

html: index rmtmp images/done
	doxygen
#	Javascript : copy files and required images to html
	cp src/*.js html/.
	cp src/*.css html/.
	cp images/page_*.gif html/.
	cp images/section_*.gif html/.

images/done: 
	midasdoc-images.tar.gz
	tar xzvf midasdoc-images.tar.gz
	touch $@

midasdoc-images.tar.gz:
	#wget https://midas.psi.ch/download/doc/midasdoc-images.tar.gz
	wget http://ladd00.triumf.ca/~daqweb/ftp/midasdoc-images.tar.gz

#pdf:	latex/refman.tex
#	cd latex; latex refman
#	cd latex; dvips -Ppdf -o midas.ps refman.dvi
#	cd latex; ps2pdf midas.ps midas.pdf ; cp midas.pdf ../.

publish:
	#tar -czvf midasdoc-html-$(version).tar.gz html/
	#tar -czvf midasdoc-man-$(version).tar.gz man/
	tar -czvf midasdoc-images.tar.gz images/
	rsync -uv html/* /home/daqweb/public_html/doc/midas3/html/.
	#rsync -av *.tar.gz daqweb@ladd00.triumf.ca:/home/daqweb/public_html/ftp/.
	#rsync -av /midas/doc/CHANGELOG.TXT /home1/ftp/pub/midas/.

publishh: 
	rsync -uv html/* /home/daqweb/public_html/doc/midas3/html/.

clean:	rmtmp
#       remove files docindex.dox and Organization.dox. These are created each time by index.pl
	rm -f src/docindex.dox
	rm -f src/Organization.dox
	cd latex; rm -f *.*~ *.pnm *.tex *.log *.aux *.dvi *.eps
	-rm -rf html
        

ver:	
	echo $(version)

index:
#       make the Contents and Index pages (Organization.dox, docindex.dox)
	cd src ; ./index.pl ; ./doit.pl ; ./index.pl 1;   cd ..
rmtmp:
#	remove temporary files created by index.pl
	rm -f src/*.tmp
	rm -f src/*_info.txt*


# end
