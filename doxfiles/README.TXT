Midas Doxygen Documentation:
===========================

This directory contains the "related" files to the Midas documentation
as well as the doxygen configuration files for linux and Windows OS

These files combined with the images directory found on
      http://midas.psi.ch/download/doc
or
     http://midas.triumf.ca/download/doc
can be used to rebuild the full documentation through the doxygen package.

Procedure:
mkdir     my_own_midas_doc            << create your own dir
cd        my_own_midas_doc            
cp        midas/doxfiles/* .          << copy all doxfiles
tar      -zxvf midas-images.tar.gz    << insert the images dir
doxygen   doxlx.cfg                   << generate the documentation
                                         with doxygen


!! Prior running doxygen, make sure the path defined in the dox*.cfg
are correct:
Reference to the midas source code : /midas/...
Reference to the current dir for the .dox
Reference to the ./images for the images
					 
This will generate new subdirectories (html, latex, man, rtf)
The html/index.html and latex/refman.tex are the main documentation.
The latex/ directory contains a Makefile for ps and pdf generation.

