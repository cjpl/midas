#!/bin/echo You must source
export MIDASSYS=$HOME/packages/midas
export MIDAS_OS=linux
#export MIDASSYS=$HOME/daq/midas
#export MIDAS_OS=darwin
export MIDAS_EXPTAB=$PWD/exptab
export PATH=$PATH:$MIDASSYS/$MIDAS_OS/bin
