//
// SIS event handling routines for ALPHA ROOT analyzer
//
// Name: HandleSIS.cxx
// Author: R.Hydomako
//
// $Id$
//
// $Log: HandleSIS.cxx,v $
// Revision 1.42  2006/09/13 12:07:24  alpha
// RAH - Added the integratio to the fine details display
//
// Revision 1.41  2006/09/11 13:38:24  alpha
// RAH - Scrolling now properly works in reversed direction
//
// Revision 1.40  2006/09/09 21:57:49  alpha
// RAH - Added fine SIS display
//
// Revision 1.39  2006/09/07 05:47:24  alpha
// RAH - Changed the filename written to the root file to include spill number
//
// Revision 1.38  2006/08/24 11:47:48  alpha
// RAH - Cleaned up some of the code, tried to made it more transparent
//
// Revision 1.37  2006/08/07 09:27:08  alpha
// RAH - got rid of all the SISFrame stuff
//
// Revision 1.36  2006/07/28 19:33:53  alpha
// RAH - Moved all the processing into the GUI classes. Very slow, will have to rearrange processes
//
// Revision 1.35  2006/07/28 13:10:31  alpha
// RAH - All the base code added for a global SIS GUI frame
//
// Revision 1.34  2006/07/22 13:35:45  alpha
// RAH- beamSpill and PbarsInAd into gbeamSpill and gPbarsInAd
//
// Revision 1.33  2006/07/21 13:59:12  alpha
// RAH - Switches histograms are only filled when not zero
//
// Revision 1.32  2006/07/20 14:45:26  alpha
// RAH - Removed some unnesessary gOutputFile->cd()s
//
// Revision 1.31  2006/07/19 11:06:35  alpha
// RAH - Histograms now placed in the rihg directories
//
// Revision 1.30  2006/07/17 14:41:29  alpha
// RAH- Now, hopefully, the per second works
//
// Revision 1.29  2006/07/17 13:44:14  alpha
// RAH- Hopefully fixed the per second plotting for the online monitor
//
// Revision 1.28  2006/07/16 17:27:18  alpha
// RAH- Changed how the Online Monitor per second ploting is done
//
// Revision 1.27  2006/07/16 13:53:17  alpha
// RAH- Fixed up the scrolling to work with when running offline
//
// Revision 1.26  2006/07/14 12:41:03  alpha
// RAH- Changed how the online monitor steps through the data
//
// Revision 1.25  2006/07/13 14:15:21  alpha
// RAH- Added offline handling (changed how the routines handle the current time)
//
// Revision 1.24  2006/07/11 19:45:28  alpha
// RAH- Changed how the online monitor fill the histograms
//
// Revision 1.23  2006/07/10 16:27:39  alpha
// RAH- Time between pulses histogram now saved at the end of run
//
// Revision 1.22  2006/07/09 12:50:54  alpha
// RAH- Added code to histogram the time between pulses on a single SIS channel
//
// Revision 1.21  2006/07/07 16:40:43  alpha
// RAH- Stopped the online monitor from starting up on pedestal runs
//
// Revision 1.20  2006/07/03 12:43:03  alpha
// RAH- Made the SIS online monitor apart of alpharoot.ini
//
// Revision 1.19  2006/06/29 11:34:46  alpha
// RAH- Made it so that the file paths are read from alpharoot.ini
//
// Revision 1.18  2006/06/27 12:19:25  alpha
// RAH- Fixed the online histograms so that you can see everything if there is more than one per panel
//
// Revision 1.17  2006/06/26 14:36:59  alpha
// RAH- Got rid of all the list and ADC stuff
//
// Revision 1.16  2006/06/26 12:36:42  alpha
// RAH- Added a more sophisticated online monitor, which initializes from onlinemonitor.txt
//
// Revision 1.15  2006/06/23 08:47:23  alpha
// RAH- Fixed the parsing of the histogram titles
//
// Revision 1.14  2006/06/22 16:14:06  alpha
// RAH- Added startevents to the switching routine, changed the parsing of the switches reading
//
// Revision 1.13  2006/06/22 13:16:16  alpha
// RAH- Added a per bin histogram display type
//
// Revision 1.12  2006/06/21 13:37:17  alpha
// RAH- Added a list structure and redid the switches routine
//
// Revision 1.11  2006/06/18 08:28:26  alpha
// RAH- Put all the switches stuff into structs
//
// Revision 1.10  2006/06/15 19:27:18  alpha
// RAH- Changing the parsing of switches.txt (still working on it)
//
// Revision 1.9  2006/06/15 18:11:11  alpha
// RAH- Changed some formatting, removed some swithces code
//

// Libraries
#include <TSystem.h>
#include <TApplication.h>
#include <TTimer.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TStyle.h>
#include <TFile.h>
#include <TRootEmbeddedCanvas.h>

#include "Globals.h"

// Switches variables

const int kMaxSwitches = 20;
int numswitches = 0;

int tptr = 0;
bool tinit = false;


struct SWITCH {
  int switchtype; 	// the type of switch: 0-repeat, 1-one time only 2-reset 
  int startchannel; 	// the channel that the start signal comes in from
  int stopchannel; 	// the channel where the stop signal comes in from
  int numstartevents;	// number of start events before turning on the switch
  int numstopevents; 	// number of stop events before turning off the switch
  int startthr;		// the number of counts in the start channel to turn on the switch
  int stopthr;		// the number of counts in the stop channel to turn off the switch
  int numbins; 		// number of bins in the histogram
  double xmin; 		// minimum histogram bin
  double xmax; 		// maximum histogram bin
  int channeltoread; 	// channel to histogram
  int displaytype;	// type of histogram to display 0-per bin 1-per second
  char title[80]; 	// title of the histogram
	
  bool switchon; 		// switch on or off
  bool keepgoing;		// whether to keepgoing (used for type 1)
	
  int startcounts; 	// the number of events from the start channel
  int stopcounts;		// the number of events from the stop channel
  int eventcounts;	// the number of events inputed from the channel being read
  int timescalled;	// the number of times the switch is flipped
	
  double start; 		// time the switch was turned on
  TH1D *histo; 		// the histogram
};

SWITCH switches[kMaxSwitches];  // the array of switches


// Online Monitor Variables

const int kNumPanels = 64;
const int kNumOnlineHistos = 5;

struct histopanel {
  int panelnumber;
  int nhistos;
  char title[80];
  int histochannel[kNumOnlineHistos];
  int nbins[kNumOnlineHistos];
  double xmin[kNumOnlineHistos];
  double xmax[kNumOnlineHistos];
  int colour[kNumOnlineHistos];
  int displaytype[kNumOnlineHistos];
  TH1D *onlinehisto[kNumOnlineHistos];   
};

histopanel onlinemonitor[kNumPanels];
histopanel detailedSISmonitor[kNumPanels];

int numpanels = 0;
int xpanels = 2;
int ypanels = 8;
double textsize = 0.1;
double xlabelsize = .1;
double ylabelsize = .1;
double ylabeloffset = .01;
int ynoexponent = 1;
int xcanvas = 1200;
int ycanvas = 800;

int dSISnumpanels = 0;
int dSISxpanels = 2;
int dSISypanels = 8;
double dSIStextsize = 0.1;
double dSISxlabelsize = .1;
double dSISylabelsize = .1;
double dSISylabeloffset = .01;
int dSISynoexponent = 1;
int dSISxcanvas = 1200;
int dSISycanvas = 800;

TCanvas *timetestcanvas = NULL;  
TH1D *timefromsec = NULL;
double gtfs = 0.;

// Routine to convert the Clock value into a Time (in seconds)

double clock2time(uint64_t clock)
{
  const double freq = 10000000.0;

  if (gSaveTime == 0)
    {
      gSaveClock = clock;
      gSaveTime  = time(NULL);
    }

  return gSaveTime + (clock - gSaveClock)/freq;
}

// Routine to reset the global clock and time values

void resetClock2time()
{
  gSaveClock = 0;
  gSaveTime  = 0;
}

// Routine to read all the switches from switches.txt

void ReadSwitches()
{
  FILE *fswitches = NULL;
  fswitches = fopen(gSwitchesPath,"r"); // open the switches file
	
  char inputstring[200];  // buffer for the input data
	
  if(fswitches != NULL)	// try to open the file
    {
      printf("Switches being read from the file: %s\n",gSwitchesPath);
      while(!feof(fswitches)) //loop over all lines in the file
	{
	  char dummychar[5];
	  if(fgets(dummychar,2,fswitches) == NULL) break; // safety check for end of file
  	  else fseek(fswitches,-1,SEEK_CUR); // rewind if not end of file
			
  	  //Parse switches.txt line by line
			
	  fgets(inputstring,199,fswitches); // put the line into a buffer string
	  if(strchr(inputstring,'#')==NULL) // if there is a #, then it is a comment
	    {
		//printf("string: %s\n",inputstring);
				
		char *parsestring;
		parsestring = strtok(inputstring, " ");	// Parse the type of switch
		switches[numswitches].switchtype= atoi(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the channel to switch on
		switches[numswitches].startchannel= atoi(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the channel to switch off
		switches[numswitches].stopchannel= atoi(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the number of stop channel events before 
		switches[numswitches].numstartevents= atoi(parsestring);// switching off
		
		parsestring = strtok(NULL, " ");	// Parse the number of stop channel events before 
		switches[numswitches].numstopevents= atoi(parsestring);// switching off
				
		parsestring = strtok(NULL, " ");	// Parse the number of stop channel events in the channel  
		switches[numswitches].startthr= atoi(parsestring);// before switching on
		
		parsestring = strtok(NULL, " ");	// Parse the number of stop channel events in the channel  
		switches[numswitches].stopthr= atoi(parsestring);// before switching off
				
		parsestring = strtok(NULL, " ");	// Parse the number of histogram bins	
		switches[numswitches].numbins= atoi(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the minimum histogram bin
		switches[numswitches].xmin= atof(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the max histogram bin
		switches[numswitches].xmax= atof(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the channel to histogram
		switches[numswitches].channeltoread= atoi(parsestring);
		
		parsestring = strtok(NULL, " ");	// Parse the displaytype
		switches[numswitches].displaytype= atoi(parsestring);
		
		parsestring++;
						
		// then to parse the title
		int i = 0;
		char title=false; 				
		while(i<100) // safety check
		  {
		    parsestring++;
		    if((!isspace(*parsestring))||(title==true))
			{
			  title=true;
			  if (*parsestring == 10 || *parsestring == 13 || *parsestring == 0)
		    		break;
			  switches[numswitches].title[i++] = *parsestring;
			}
		  }
		  //printf("%s\n",switches[numswitches].title);
		  numswitches++;
	  }
	}
	//printf("Number of switches: %d\n",numswitches);
	fclose(fswitches);
   }
  else
     printf("Cannot open %s!\n",gSwitchesPath);	
  
  if(numswitches>kMaxSwitches) 
      printf("Exceeded maximum number of switches!\n");
}

const int kNumberofDetailChannels = 5;

bool DetailsRecord[kNumberofDetailChannels] = {false,false,false,false,false};
int DetailsChannels[kNumberofDetailChannels] = {2,4,5,6,7};
char* DetailsTitles[kNumberofDetailChannels] = {"PMT OR","Pbar TimeStamps","Mixing TimeStamps","Pbar Sequence Start","Mixing Sequence Start"};
TH1D* DetailsHisto[kNumberofDetailChannels];

const double DetailsLength = 3.0; // JWS CHANGED!!!! from 10.0

const int ADchannel = 1;
double detailsStartTime = 0.0;

double detailsmin = 0.2;
double detailsmax = 0.4;

int Integral = 0;

void CheckDetails(int Channel, double counts, uint64_t clock)
{
	if(counts!=0) {
		if(Channel == ADchannel) {
			if(DetailsRecord[0] == false) {
				detailsStartTime = clock2time(clock);
				
				char title[80];
				
				for(int i = 0;i<kNumberofDetailChannels;i++) {
					sprintf(title,"%s, Run %d, Spill %d",DetailsTitles[i],gRunNumber,gbeamSpill);
					DetailsHisto[i] = new TH1D(title,title,200,0,DetailsLength);  
					gStyle->SetOptStat(kTRUE);
					//DetailsHisto[i] = new TH1D(title,title,20,detailsmin,detailsmax);  
					
					DetailsRecord[i] = true;
				}
				Integral = 0;
			}
			else {
//				printf("AD trigger while details still recording\n");
			}
		}
		for(int i = 0;i < kNumberofDetailChannels;i++) {
			if (Channel == DetailsChannels[i])
			{
				if(DetailsRecord[i] == true){
					if((clock2time(clock)-detailsStartTime)>=DetailsLength) {
						for(int j=0;j<kNumberofDetailChannels;j++) {
							DetailsHisto[j]->Write();
							if(j==0)	
								printf("Spill: %d, Integral: %d\n",gbeamSpill,Integral);
							
							DetailsRecord[j]=false;
						
							if(!gSISDetails) {
								gSISDetails = new TCanvas("SIS Fine Details","SIS Fine Details",600,900);
								gSISDetails->Divide(1,kNumberofDetailChannels,0.001,0.001);
								
								//gStyle->SetOptStat(kFALSE); // Turn off the statistics for the online histograms
  								gStyle->SetTitleFontSize(0.09);
							}
							
							gSISDetails->cd(j+1);
							
							if(j==0)
							{
								char text[80];
								sprintf(text,"Integral (%lf-%lf): %d",detailsmin,detailsmax,Integral);
								TText *txt = new TText(.2,2,text);
								txt->SetTextSize(0.1);txt->SetTextColor(1);
								DetailsHisto[j]->GetListOfFunctions()->Add(txt);
							}
							DetailsHisto[j]->Draw();
						}
						gSISDetails->Modified();
						gSISDetails->Update();
					}
					else {
						DetailsHisto[i]->Fill(clock2time(clock)-detailsStartTime,counts);
						if(((clock2time(clock)-detailsStartTime)>=detailsmin)&&((clock2time(clock)-detailsStartTime)<=detailsmax))
						    Integral++;
					}
				}
				else{
					// Don't record
				}
			}
		}
	}
}



// Check the inputted value against the defined switches

void CheckTriggers(int Channel, double counts, uint64_t clock)
{
	for(int i=0;i<numswitches;i++) //loop over all switches
	{
		if(switches[i].keepgoing==true) // used for switch type 1 (when turned off, stays off)
		{
			if((switches[i].startchannel==Channel)&&(switches[i].switchon==false)&&(counts==switches[i].startthr)) // turn on the switch
			{
				if(switches[i].startcounts==switches[i].numstartevents)
				{
					switches[i].switchon=true;
					switches[i].timescalled++;
//					printf("Switch %d - On!\n",i);
									
					char title[80];
					sprintf(title,"%s, Run %d, Spill Number %d",switches[i].title,gRunNumber,gbeamSpill);			
					
					switches[i].histo = new TH1D(title,switches[i].title,switches[i].numbins,switches[i].xmin,switches[i].xmax);
					switches[i].start = clock2time(clock);  
					
					//printf("i: %d, clock: %ld, start: %lf, gSaveClock: %d, gSaveTime: %d\n", i,(long int)clock,switches[i].start,(int)gSaveClock,(int)gSaveTime);
					
					switches[i].stopcounts = 0;
					switches[i].eventcounts = 0;
				}
				else switches[i].startcounts++;
			}
			else if ((switches[i].stopchannel==Channel)&&(switches[i].switchon==true)&&(counts==switches[i].stopthr)) // a event in stop channel
			{
				if(switches[i].stopcounts==switches[i].numstopevents) // and if there have been enough endsignals
				{
//					printf("Switch %d - Off!\n", i);
					switches[i].switchon= false;
					
					switches[i].histo->Write();
																
					switches[i].stopcounts=0;
					switches[i].startcounts=0;
					switches[i].eventcounts = 0;
					if(switches[i].switchtype==1) // no repeating
						switches[i].keepgoing = false;
					else if (switches[i].switchtype==2) // reseting
					{	
						switches[i].switchon=true;
						switches[i].timescalled++;
//						printf("Resetting switch %d!\n",i);
						
						delete switches[i].histo;
						switches[i].histo=NULL;	
						
						char title[80];
						sprintf(title,"%s, Run %d, Spill Number %d",switches[i].title,gRunNumber,gbeamSpill);	
					
						switches[i].histo = new TH1D(title,switches[i].title,switches[i].numbins,switches[i].xmin,switches[i].xmax);
						switches[i].start = clock2time(clock); 
					}
				}
				else switches[i].stopcounts++;
			}
			if((switches[i].switchon == true)&&(switches[i].stopcounts<=switches[i].numstopevents))
			{
				if(switches[i].displaytype==0) // display the results per bin
				{
					if(Channel==switches[i].channeltoread) {
						switches[i].histo->Fill(switches[i].eventcounts,counts);
						switches[i].eventcounts++;
					}
				}
				else if(Channel==switches[i].channeltoread)
				{
					if(counts!=0) {
						double plottime = clock2time(clock)-switches[i].start;
						switches[i].histo->Fill(plottime,counts);
					}
				}
			}	
		}
	}
}

bool mixingsequence = false;
bool pbarsequence = false;

bool mixingrecording = false;
bool pbarrecord = false;

TH1D	*mixinghisto = NULL;
TH1D	*pbarhisto = NULL;

int mixingindex = 0;
int pbarindex = 0;

int dumpnumber = 0;

int recordingstarttime = 0;

void CheckMixingTriggers(int Channel, double counts, uint64_t clock)
{
	int sequencestartchannel = 7;
	int mixingtimestampchannel = 5;
	int recordingchannel = 2;
	
	if(counts!=0){
	
		if(Channel == sequencestartchannel){
		
			if(mixingsequence == false) { // Startup
				printf("Starting of mixingsequence\n");
			
				mixingsequence = true;
				mixingindex = 0;
			}
			else if (mixingsequence == true) { // Reset
			
				printf("Mixing dump start signal called when already in a sequence!\n");
			
			}
		}
		else if(Channel == mixingtimestampchannel) { // Mixing timestamp to check
			if(mixingsequence == true) { 
				
	//			printf("mtimestamp!\n");
				
				mixingindex++;
				
				if((mixingrecording==false)&&(gmdump.singledump[dumpnumber].steps[mixingindex]==1)){
					mixingrecording = true;
					
					printf("1!  Step: %d\n",mixingindex);
					//start histogram here
					dumpnumber++;
			
					char title[80];
					sprintf(title,"Dump (mixing) number %d, spill %d, run%d",dumpnumber,gbeamSpill,gRunNumber);
			
					if(mixinghisto){
						mixinghisto->Write();
						delete mixinghisto;
						mixinghisto = NULL;
					}
					mixinghisto = new TH1D(title,title,15000,0,15000);
					
					recordingstarttime = clock;
				}
				else if ((mixingrecording==true)&&(gmdump.singledump[dumpnumber].steps[mixingindex]==0)){ // Dump finished, write histogram
					mixingrecording = false;
					
					//int mixingintegral = mixinghisto->Integral();
					mixinghisto->Write();
				}
			}
			else {
	//			printf("Mixing timestamp when no mixing sequence!\n");
			}
			
			if(mixingindex == gmdump.singledump[dumpnumber].totalsteps){
	//			printf("Max size (%d)!\n",gmdump.singledump[dumpnumber].totalsteps);
				mixingrecording = false;
				mixingsequence = false;
			}	
		
		}
		else if ((Channel == recordingchannel)&&(mixingrecording)) {
			printf("Record!\n");
			if(mixinghisto){
				mixinghisto->Fill(clock-recordingstarttime,counts);
			} 
			else
				printf("No histogram to record to!\n");
		}
		
	}
}

void CheckPbarTriggers(int Channel, double counts, uint64_t clock)
{
//	int sequencestartchannel = 6;
//	int pbartimestampchannel = 4;

}


// Moves through the SIS data, sends the next event to be check against the switches
// (Called once a second)

void StepThroughSISBuffer()
{	
	if(gIsRunning==true) // proceed only when running
	{
	gOutputFile->cd("SIS");
		if((gSisData->cptr<gSisData->wptr)&&(gSisData->cptr<gSisData->wmax)) // when the read pointer is behind both the write pointer and max
		{
			while(gSisData->cptr<gSisData->wptr) // send all the unchecked events to check against the switches
			{
				for(int i = 0;i<kMaxSisChannels;i++)
				{
					int channel = i;
					double counts = gSisData->sisChannels[i].sisData[gSisData->cptr];
					uint64_t clock = gSisData->sisClock[gSisData->cptr];
					
					CheckMixingTriggers(channel,counts,clock);
					CheckPbarTriggers(channel,counts,clock);
					
					CheckTriggers(channel,counts,clock);
					
					if(gDisplayDetailedSIS)
						CheckDetails(channel,counts,clock);
				}
				gSisData->cptr++;
			}
		}
		else if ((gSisData->cptr>gSisData->wptr)&&(gSisData->cptr<gSisData->wmax)) // when the write pointer has gone back to zero, but the read pointer hasn't yet
		{
			while(gSisData->cptr<gSisData->wmax)
			{
				for(int i = 0;i<kMaxSisChannels;i++)
				{
					int channel = i;
					double counts = gSisData->sisChannels[i].sisData[gSisData->cptr];
					uint64_t clock = gSisData->sisClock[gSisData->cptr];
					
					CheckMixingTriggers(channel,counts,clock);
					CheckPbarTriggers(channel,counts,clock);
					
					CheckTriggers(channel,counts,clock);
					
					if(gDisplayDetailedSIS)
						CheckDetails(channel,counts,clock);
				}
				gSisData->cptr++;
			}
		}
		else if((gSisData->cptr>gSisData->wptr)&&(gSisData->cptr==gSisData->wmax)) // if the read pointer has readed the maximum (go back to zero)
			gSisData->cptr = 0;
			
		//printf("cptr: %d, wptr: %d, wmax: %d\n",gSisData->cptr,gSisData->wptr, gSisData->wmax);
	gOutputFile->cd("../");	
	}	
}


// Routine to handle the incoming SIS data -- write the data into an ordered buffer

void HandleSIS(int numChan,int size,const void*ptr)
{
  const uint32_t *sis = (uint32_t*)ptr;
//  printf("SIS data size %d, ",size);

  assert(gSisData);

  int i;
  for (i=0; i<size; i+=numChan, sis+=numChan)
    {
      	//printf("bucket %d, data: %d %d %d %d\n",i,sis[16],sis[17],sis[18],sis[19]);
      	uint64_t clock = sis[0];
	
	for (int j=0; j<numChan; j++)
	{	    
	    gSisData->sisChannels[j].sisData[gSisData->wptr] = sis[j];
	}
	
	gSisData->clock += clock;
	gSisData->sisClock[gSisData->wptr] = gSisData->clock;
	
	gSisData->wptr++;
	if (gSisData->wptr >= kMaxSisBufferSize)
	  gSisData->wptr = 0;

	if (gSisData->wptr > gSisData->wmax)
	  gSisData->wmax = gSisData->wptr;
    }
  assert(i==size);

  //printf("wptr: %d\n",gSisData->wptr);

  time_t now = time(NULL);

  static time_t gNow = 0;
  if (now > gNow + 10)
    {
      gNow = now;
      double sistime = clock2time(gSisData->clock);
      printf("time drift: %d %f %f\n",(int)now,sistime,sistime-now);
    }
}



void onlineMonitorStart()
{
	FILE *fmonitor = NULL;
	fmonitor = fopen(gOLMPath,"r"); // open the switches file
	
	char inputstring[120];  // buffer for the input data
	bool divideinit = false;
	
	if(fmonitor != NULL)	// try to open the file
	{
		printf("Online Monitor values being read from file: %s\n",gOLMPath);
		while(!feof(fmonitor)) //loop over all lines in the file
		{
			char dummychar[5];
			if(fgets(dummychar,2,fmonitor) == NULL) break; // safety check for end of file
			else fseek(fmonitor,-1,SEEK_CUR); // rewind if not end of file
			
			//Parse onlinemonitor.txt line by line
			
			fgets(inputstring,119,fmonitor); // put the line into a buffer string
			if(strchr(inputstring,'#')==NULL) // if there is a #, then it is a comment
			{
				//printf("string: %s\n",inputstring);
				char *parsestring; // buffer to parse
				
				if(divideinit==false) // grab the first line (window options)
				{
					parsestring = strtok(inputstring, " ");	// Parse the number of columns
					xpanels= atoi(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse the number of rows
					ypanels= atoi(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse the size of the text
					textsize= atof(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse the size of the x labels
					xlabelsize= atof(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse the size of the y labels
					ylabelsize= atof(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse the size of the y label offset
					ylabeloffset= atof(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse if to show exponents on the y axis
					ynoexponent= atoi(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse if to show exponents on the y axis
					xcanvas= atoi(parsestring);
					
					parsestring = strtok(NULL, " ");	// Parse if to show exponents on the y axis
					ycanvas= atoi(parsestring);
					
					divideinit=true;
				}
				else	// then grab the rest of the file
				{
					parsestring = strtok(inputstring, " ");	// Parse the panel number
					onlinemonitor[numpanels].panelnumber= atoi(parsestring);
				
					parsestring = strtok(NULL, " ");	// Parse the number of histograms in the panel
					onlinemonitor[numpanels].nhistos= atoi(parsestring);				
				
					if(onlinemonitor[numpanels].nhistos>kNumOnlineHistos)
					{
						printf("Too many histograms in panel %d!\n",numpanels);
						break;
					}
					parsestring++;
						
					// then to parse the title
					int i = 0;
					char title=false; 				
					while(i<100) // safety check
					{
						parsestring++;
						if((!isspace(*parsestring))||(title==true))
						{
							title=true;
							if (*parsestring == 10 || *parsestring == 13 || *parsestring == 0)
		    						break;
							onlinemonitor[numpanels].title[i++] = *parsestring;
						}		
					}
				
					//printf("Number of histograms: %d, title: %s\n",onlinemonitor[numpanels].nhistos,onlinemonitor[numpanels].title);
					for(int i=0;i<onlinemonitor[numpanels].nhistos;i++)
					{
						fgets(inputstring,119,fmonitor);
						if(strchr(inputstring,'#')==NULL) // if there is a #, then it is a comment
						{
							parsestring = strtok(inputstring, " ");	// Parse the channel to read
							onlinemonitor[numpanels].histochannel[i]= atoi(parsestring);
							parsestring = strtok(NULL, " ");	// Parse the number of bins
							onlinemonitor[numpanels].nbins[i]= atoi(parsestring);
							parsestring = strtok(NULL, " ");	// Parse the minimum bin
							onlinemonitor[numpanels].xmin[i]= atof(parsestring);
							parsestring = strtok(NULL, " ");	// Parse the max bin
							onlinemonitor[numpanels].xmax[i]= atof(parsestring);
							parsestring = strtok(NULL, " ");	// Parse the colour
							onlinemonitor[numpanels].colour[i]= atoi(parsestring);
							parsestring = strtok(NULL, " ");	// Parse the display type  
							onlinemonitor[numpanels].displaytype[i]= atoi(parsestring);
						}
					}
					numpanels++;
					if(numswitches>kNumPanels)
					{
						printf("Exceeded maximum number of panels!\n");
						break;
					}
				}
			}
		}
		fclose(fmonitor);
	}
	else
	{
		printf("Cannot open %s!\n",gOLMPath);
	}

  if(gMainWindow==NULL)
  {
       	gMainWindow = new TCanvas("SIS Display","SIS Display",xcanvas,ycanvas);
	gMainWindow->Divide(xpanels,ypanels,0.001,0.001);
  }
      
  gStyle->SetOptStat(kFALSE); // Turn off the statistics for the online histograms
  gStyle->SetTitleFontSize(textsize);
  
  if(onlinemonitor[0].onlinehisto[0]!=NULL)
  {
  	for(int i=0;i<numpanels;i++)
	{
		for(int j=0;j<onlinemonitor[i].nhistos;j++)
		{
			delete onlinemonitor[i].onlinehisto[j];
		}
	}
  }
      
  gROOT->cd();	// Move outside gOutputFile so that these histograms
  char title[80];	// aren't affected between runs
  for(int i=0;i<numpanels;i++)
  {
	gMainWindow->cd(onlinemonitor[i].panelnumber);
	for(int j=0;j<onlinemonitor[i].nhistos;j++)
	{	
		if(j==(onlinemonitor[i].nhistos-1))
		{
			onlinemonitor[i].onlinehisto[j] = new TH1D(onlinemonitor[i].title,onlinemonitor[i].title,onlinemonitor[i].nbins[j],onlinemonitor[i].xmin[j],onlinemonitor[i].xmax[j]);
		}
		else
		{
			char dummytitle[80];
			sprintf(dummytitle,"%s, Channel %d",onlinemonitor[i].title,onlinemonitor[i].histochannel[j]);
			onlinemonitor[i].onlinehisto[j] = new TH1D(dummytitle,onlinemonitor[i].title,onlinemonitor[i].nbins[j],onlinemonitor[i].xmin[j],onlinemonitor[i].xmax[j]);
		}
		switch(onlinemonitor[i].colour[j])
		{
			case 0:
				onlinemonitor[i].onlinehisto[j]->SetLineColor(kBlack);
				break;
			case 1:
				onlinemonitor[i].onlinehisto[j]->SetLineColor(kRed);
				break;
			case 2:
				onlinemonitor[i].onlinehisto[j]->SetLineColor(kBlue);
				break;
			default:
				onlinemonitor[i].onlinehisto[j]->SetLineColor(kBlack);
		}
		onlinemonitor[i].onlinehisto[j]->GetYaxis()->SetNoExponent(ynoexponent);
  		onlinemonitor[i].onlinehisto[j]->SetLabelSize(xlabelsize,"X");
  		onlinemonitor[i].onlinehisto[j]->SetLabelSize(ylabelsize,"Y");
  		onlinemonitor[i].onlinehisto[j]->SetLabelOffset(ylabeloffset,"Y");
	}
  }
  if(gOutputFile!=NULL)
  {
  	sprintf(title,"data/run%d.root:/",gRunNumber);
  	gROOT->Cd(title);	// Move back into gOutputFile
  }
  else
  {
	printf("Data file improperly initialized\n");
	return;
  }
  gStyle->SetOptStat(kTRUE);  // Turn the statistics back on for the other histograms
  //gStyle->SetTitleFontSize();
}


// Routine to update the online histograms (called once a second)  

void SISperiodic()
{   
  double now = 0;
  double datatime = 0;
  double diff = 0;
  uint32_t data = 0;
  
  
  if(gMainWindow!=NULL)
  {
	  // Reset and fill all the online histograms
	  for(int i=0;i<numpanels;i++)
      	  {
		gMainWindow->cd(onlinemonitor[i].panelnumber);
		for(int j=0;j<onlinemonitor[i].nhistos;j++)
		{
			onlinemonitor[i].onlinehisto[j]->Reset();
		
			// Display the data per bin
		
			if(onlinemonitor[i].displaytype[j]==0)
			{	
				if(gSisData->wptr==gSisData->wmax)  // for the first loop through the buffer
				{
					if((gSisData->wptr-(int)onlinemonitor[i].xmax[j])<=0) // when the data doesn't fill the whole histogram
					{
						for(int k = 0;k<gSisData->wptr;k++)
							onlinemonitor[i].onlinehisto[j]->Fill(gSisData->wptr-k,gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k]);					
					}
					else // fill the whole histogram
					{
						for(int k = (gSisData->wptr-(int)onlinemonitor[i].xmax[j]);k<gSisData->wptr;k++)
							onlinemonitor[i].onlinehisto[j]->Fill(gSisData->wptr-k,gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k]);					
					}
				}
				else // after the buffer has been looped through
				{
					if((gSisData->wptr-(int)onlinemonitor[i].xmax[j])<=0) 
					{
						for(int k = 0;k<gSisData->wptr;k++) // the data from the front of the buffer
							onlinemonitor[i].onlinehisto[j]->Fill(gSisData->wptr-k,gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k]);					
							// the data from the end of the buffer
						for(int k = (gSisData->wmax-(int)onlinemonitor[i].xmax[j]+gSisData->wptr);k<gSisData->wmax;k++)
							onlinemonitor[i].onlinehisto[j]->Fill(gSisData->wmax+gSisData->wptr-k,gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k]);					
					}
					else // data fills the whole histogram
					{
						for(int k = (gSisData->wptr-(int)onlinemonitor[i].xmax[j]);k<gSisData->wptr;k++)
							onlinemonitor[i].onlinehisto[j]->Fill(gSisData->wptr-k,gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k]);					
					}	
				}
			}
			
			// Display per second
			
			else if(onlinemonitor[i].displaytype[j]==1) {
				
				// If the data buffer hasn't looped and gone back the the beginning
				
				if(gSisData->wptr==gSisData->wmax)
				{
					// While all the data can be fitted onto one histogram
					
					if((clock2time(gSisData->sisClock[gSisData->wptr-1])-clock2time(gSisData->sisClock[0])-onlinemonitor[i].xmax[j])>=0.)
					{
						for(int k=0;k<(gSisData->wptr-1);k++)
						{
							now = clock2time(gSisData->sisClock[gSisData->wptr-1]);
							datatime = clock2time(gSisData->sisClock[k]);
							diff = datatime - now;
							data = gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k];
							
							//printf("i: %d, diff: %lf, data: %lf\n",i,diff,(double)data);
							
							onlinemonitor[i].onlinehisto[j]->Fill(diff,data);
						}
					}
					
					// Not all the data can fit in one histogram, so we don't have to loop over all the data
					
					else
					{
						for(int k=(gSisData->wptr-(int)onlinemonitor[i].xmax[j]*1000);k<gSisData->wptr;k++)
						{
							if(k>0)
							{
								now = clock2time(gSisData->sisClock[gSisData->wptr-1]);
								datatime = clock2time(gSisData->sisClock[k]);
								diff = datatime - now;
								data = gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k];
							
								onlinemonitor[i].onlinehisto[j]->Fill(diff,data);
							}
						}
					}
				}
				
				// We've made a full loop around the data buffer so now there is data we want to look at both at the beginning and end of the buffer
				
				else
				{
					if((clock2time(gSisData->sisClock[gSisData->wptr-1])-clock2time(gSisData->sisClock[0])-onlinemonitor[i].xmax[j])>=0.)
					{
						for(int k=0;k<gSisData->wptr;k++)
							{
								now = clock2time(gSisData->sisClock[gSisData->wptr-1]);
								datatime = clock2time(gSisData->sisClock[k]);
								diff = datatime - now;
								data = gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k];
							
								onlinemonitor[i].onlinehisto[j]->Fill(diff,data);
							}
						for(int k=(gSisData->wmax-(int)onlinemonitor[i].xmax[j]*1000+gSisData->wptr);k<(gSisData->wmax+1);k++)
							{
								now = clock2time(gSisData->sisClock[gSisData->wptr-1]);
								datatime = clock2time(gSisData->sisClock[k]);
								diff = datatime - now;
								uint32_t data = gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k];
							
								onlinemonitor[i].onlinehisto[j]->Fill(diff,data);
							}
					}
					else
					{
						for(int k=(gSisData->wptr-(int)onlinemonitor[i].xmax[j]*1000);k<gSisData->wptr;k++)
						{
							if(k>0)
							{
								now = clock2time(gSisData->sisClock[gSisData->wptr-1]);
								datatime = clock2time(gSisData->sisClock[k]);
								diff = now - datatime;
								uint32_t data = gSisData->sisChannels[onlinemonitor[i].histochannel[j]].sisData[k];
							
								onlinemonitor[i].onlinehisto[j]->Fill(diff,data);
							}
						}
					}
				}
			}
			
			// Draw the histograms
			
			if(j==0)
			{
				onlinemonitor[i].onlinehisto[j]->Draw();
			}
			else
			{	// if the next histogram has larger values, expand the y range of the first
				if(onlinemonitor[i].onlinehisto[j]->GetMaximum()>onlinemonitor[i].onlinehisto[0]->GetMaximum())
					onlinemonitor[i].onlinehisto[0]->SetAxisRange(0,onlinemonitor[i].onlinehisto[j]->GetMaximum(),"Y");
				onlinemonitor[i].onlinehisto[j]->Draw("same");
			}
		}
      	  }
	  printf("cptr: %d, wptr: %d, wmax: %d\n",gSisData->cptr,gSisData->wptr, gSisData->wmax);
	  
	  gMainWindow->GetCanvas()->Modified();
	  gMainWindow->GetCanvas()->Update();
  }
  
  // Histogram the time between pulses
  
  if(gDisplayTimeTest&&gIsRunning&&!gIsPedestalsRun)
  {
  	double tfsdiff = 0;
  
  	if(!timefromsec)
  	{
		timetestcanvas= new TCanvas("Time Between Signals","Time Between Signals");
		char name[80];
		sprintf(name,"Number of Seconds Between Signals, Channel %d, Run %d",gtestchannel,gRunNumber);
		timefromsec = new TH1D(name,name,20000,0,200);
  	}
  
  	if((tptr<gSisData->wptr)&&(tptr<gSisData->wmax)) // when the read pointer is behind both the write pointer and max
	{
		while(tptr<gSisData->wptr) // send all the unchecked events to check against the switches
		{
			if(gSisData->sisChannels[gtestchannel].sisData[tptr]!=0)
			{
				if(tinit==false)
				{
					gtfs = clock2time(gSisData->sisClock[tptr]);
					tinit = true;
				}
				else
				{
					tfsdiff = clock2time(gSisData->sisClock[tptr])-gtfs;
					timefromsec->Fill(tfsdiff);
					gtfs = clock2time(gSisData->sisClock[tptr]);
				}
  			}
			tptr++;
		}
	}
	else if ((tptr>gSisData->wptr)&&(tptr<gSisData->wmax)) // when the write pointer has gone back to zero, but the read pointer hasn't yet
	{
		while(tptr<gSisData->wmax)
		{
			if(gSisData->sisChannels[gtestchannel].sisData[tptr]!=0)
			{
					tfsdiff = clock2time(gSisData->sisClock[tptr])-gtfs;
					timefromsec->Fill(tfsdiff);
					gtfs = clock2time(gSisData->sisClock[tptr]);
  			}
			tptr++;
		}
	}
	else if((tptr>gSisData->wptr)&&(tptr==gSisData->wmax)) // if the read pointer has readed the maximum (go back to zero)
	{
		tptr = 0;
	}
    	timefromsec->Draw();
  
  	timetestcanvas->Modified();
  	timetestcanvas->Update();
  }
}

// To be run at the beginning of each run

void SISBeginRun()
{
	//reset the SIS buffer
	gSisData->wptr=0;
	gSisData->wmax=0;
	gSisData->clock=0;
	gSisData->cptr=0;
	
	tptr = 0;
	tinit = false;
	
	//reset the switches
	for(int i=0;i<kMaxSwitches;i++)
	{
		switches[i].switchon=false;
		switches[i].keepgoing=true;
		switches[i].timescalled=0;
	}

	//reset the number of switches
	numswitches=0;
	ReadSwitches(); //read in the new switches
}


void SISEndRun()
{
	gOutputFile->cd("SIS");
	// Write the switches histograms if stopped in the middle
	for(int i=0;i<kMaxSwitches;i++)
	{
		if(switches[i].histo!=NULL)
			switches[i].histo->Write();
	}
	
	// Write the pulse timing histograms
	if(timetestcanvas&&gDisplayTimeTest)
	{
		timetestcanvas->Write();
		timetestcanvas->Close();
		timetestcanvas= NULL;
	}
	gOutputFile->cd("../");
}
