Midas Doxygen Documentation:
===========================

This directory contains the "related" files to the Midas documentation
as well as the doxygen configuration files for linux and Windows OS
(no build has been tested under cygwin yet!)

The current Doxygen version used is 1.3.5

These files combined with the images directory found on
      http://midas.psi.ch/download/doc
or
     http://midas.triumf.ca/download/doc
can be used to rebuild the full documentation through the doxygen package.

Linux Documentation Build Procedure:
===================================
mkdir     my_doc               << create your own dir
cd        my_doc            
cp        midas/doxfiles/* .             << copy all doxfiles
tar      -zxvf midasdoc-images.tar.gz    << insert the images dir
doxygen   doxlx.cfg                      << generate the documentation
                                            with doxygen


!! Prior running doxygen, make sure the path defined in the dox*.cfg
are correct:
Reference to the midas source code : /midas/...
Reference to the current dir for the .dox
Reference to the ./images for the images
					 
This will generate new subdirectories (html, latex, man, rtf)
The html/index.html and latex/refman.tex are the main documentation.
The latex/ directory contains a Makefile for ps and pdf generation.

WindowNT/XP Documentation Build Procedure:
=========================================
Extract the midas package from CVS (password cvs) or from the tar file
This will create a new directory midas under your current path:
cd \             << Whatever is your drive label
cvs -e ssh -d :ext:cvs@midas.psi.ch:/usr/local/cvsroot checkout midas
mkdir       my_doc
cd          my_doc
copy        \midas\doxfiles .
gunzip/rar  midasdoc-images.tar.gz
doxygen     doxnt.cfg

