Sample mini MIDAS experiment
============================

This directory contains an example of a MIDAS experiment. It
contains an even definition for a trigger event (ID 1) with
eight ADC and TDC channels and a scaler event (ID 2) also with
eight channels.

The frontend program generates both types of events and fills
them with data. The data is stored in the events in MIDAS bank 
format which is very similar to BOS/YBOS banks.

The analyzer uses a single user routine to recover these banks
from the events and analyzes them, then fills them into 
histograms and N-tuples.

To adapt the sample experiment to a real experiment, following
tasks have to be performed:

1) Run the frontend on a computer which has direct access to
the experiment hardware. Most likely this is a VxWorks machine
or a PC running Windows NT or Linux. Alternatively, the frontend,
the analyzer and the logger can run in a single process on the
same computer.

2) Modify your event definition. If you want to send new banks
of variable size from the frontend, create them in the frontend 
readout code and define them in the bank list in analyzer.c. 

3) Modify the analyzer to suit your needs. 

4) Compile the components. To generate seperate frontend
and analyzer programs, just enter "make". To compile the
integrated frontend-analyzer-logger program, enter 
"make fal".

To run the experiment, first start the processes:

1) Run the frontend on your frontend computer, and run 
the analyzer and the mlogger on the backend computer.
Alternatively, run the "fal" program.

2) Check the data directory in the ODB under /logger/data dir.

> odbedit
[local]/> cd /logger
[lodal]/> ls
[local]/Logger> set "data dir" <... new dir ...>

3) Check the logging channels under /logger/channels/0/Settings. 
If tape writing is desired, set the logging channel type to 
"tape" and the filename to something like /dev/nst0 under Linux or
\\.\tape0 under Windows NT. When writing to disk, set the filename
to run%05d.mid, which gererates names of the type runxxxxx.mid with
xxxxx the current run number automatically.

4) Start/stop runs with the ODBEdit "start"/"stop" commands.

5) You can inspect online histograms and N-tuples with PAW
running on the backend computer:

  PAW> global_s onln
  PAW> ldir
  ...

To turn N-tuples on, set /Analyzer/Bank switches to one for those
banks which should be conainted in N-tuple #1. You can then see
the results with

  PAW> nt/print 1
  PAW> nt/pl 1.adc00 adc00>10.and.tdc00<100

6) Check if run00001.mid etc. is created when logging to disk
is turned on.

7) You can offline-analyze the .mid files with the same
analyzer executable:

  analyzer -i run00001.mid -o run00001.rz

to produce directly a N-tuple file which can be loaded inside
PAW with

  PAW> hi/file 1 run00001.rz 8190




