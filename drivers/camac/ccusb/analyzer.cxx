//
// ROOT analyzer
//
// K.Olchanski
//
// $Id$
//

/// \mainpage
///
/// \section intro_sec Introduction
///
/// This "ROOT analyzer" package is a collection of C++ classes to
/// simplify online and offline analysis of data
/// collected using the MIDAS data acquisition system.
///
/// To permit standalone data analysis in mobile or "home institution"
/// environments, this package does not generally require that MIDAS
/// itself be present or installed.
///
/// It is envisioned that the user will use this package to develop
/// their experiment specific analyzer using the online data
/// connection to a MIDAS experiment. Then they could copy all the code
/// and data (.mid files) to their laptop and continue further analysis
/// without depending on or requiring installation of MIDAS software.
///
/// It is assumed that data will be analyzed using the ROOT
/// toolkit. However, to permit the most wide use of this
/// package, most classes do not use or require ROOT.
///
/// \section features_sec Features
///
/// - C++ classes for reading MIDAS events from .mid files
/// - C++ classes for reading MIDAS events from a running
/// MIDAS experiment via the mserver or directly from the MIDAS
/// shared memory (this requires linking with MIDAS libraries).
/// - C++ classes for accessing ODB data from .mid files
/// - C++ classes for accessing ODB from MIDAS shared memory
/// (this requires linking with MIDAS libraries).
/// - an example C++ analyzer main program
/// - the example analyzer creates a graphical ROOT application permitting full
/// use of ROOT graphics in both online and offline modes.
/// - for viewing "live" histograms using the ROODY graphical histogram viewer,
/// included is the "midasServer" part of the MIDAS analyzer.
///
/// \section links_sec Links to external packages
///
/// - ROOT data analysis toolkit: http://root.cern.ch
/// - MIDAS data acquisition system: http://midas.psi.ch
/// - ROODY graphical histogram viewer: http://daq-plone.triumf.ca/SR/ROODY/
///
/// \section starting_sec Getting started
///
/// - "get" the sources: svn checkout svn://ladd00.triumf.ca/rootana/trunk rootana
/// - cd rootana
/// - make
/// - make dox (generate this documentation); cd html; mozilla index.html
///  

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "TMidasOnline.h"
#include "TMidasEvent.h"
#include "TMidasFile.h"
#include "XmlOdb.h"

#include <TSystem.h>
#include <TApplication.h>
#include <TTimer.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TGClient.h>
#include <TGFrame.h>

#include "Globals.h"

// Global Variables
int  gRunNumber = 0;
bool gIsRunning = false;
bool gIsPedestalsRun = false;
bool gIsOffline = false;
int  gEventCutoff = 0;

TFile* gOutputFile = NULL;
VirtualOdb* gOdb = NULL;

//TCanvas  *gMainWindow = NULL; 	// the online histogram window

double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}

class MyPeriodic : public TTimer
{
public:
  typedef void (*TimerHandler)(void);

  int          fPeriod_msec;
  TimerHandler fHandler;
  double       fLastTime;

  MyPeriodic(int period_msec,TimerHandler handler)
  {
    assert(handler != NULL);
    fPeriod_msec = period_msec;
    fHandler  = handler;
    fLastTime = GetTimeSec();
    Start(period_msec,kTRUE);
  }

  Bool_t Notify()
  {
    double t = GetTimeSec();
    //printf("timer notify, period %f should be %f!\n",t-fLastTime,fPeriod_msec*0.001);

    if (t - fLastTime >= 0.9*fPeriod_msec*0.001)
      {
	//printf("timer: call handler %p\n",fHandler);
	if (fHandler)
	  (*fHandler)();

	fLastTime = t;
      }

    Reset();
    return kTRUE;
  }

  ~MyPeriodic()
  {
    TurnOff();
  }
};

#include "TH1D.h"
#include "TFolder.h"
#include "midasServer.h"

const int gNumHist = 4;
TH1D* gHist[gNumHist];

void CloseOutputFile()
{
  // clear out references to histograms saved to the output file
  for (int i=0; i<gNumHist; i++)
    {
      if (gManaHistosFolder)
	gManaHistosFolder->Remove(gHist[i]);
      gHist[i] = NULL;
    }

  // close output file
  gOutputFile->Write();
  gOutputFile->Close();
  gOutputFile = NULL;
}

void startRun(int transition,int run,int time)
{
  gIsRunning = true;
  gRunNumber = run;
  gIsPedestalsRun = gOdb->odbReadBool("/experiment/edit on start/Pedestals run");
  printf("Begin run: %d, pedestal run: %d\n", gRunNumber, gIsPedestalsRun);
    
  if (gOutputFile!=NULL)
    CloseOutputFile();

  char filename[1024];
  sprintf(filename, "output%05d.root", run);
  //printf("run %d %d, filename %s\n", run, gRunNumber, filename);

  gOutputFile = new TFile(filename,"UPDATE");

  gOutputFile->cd();
  for (int i=0; i<gNumHist; i++)
    {
      char name[256];
      sprintf(name, "ADC%d", i);
      gHist[i] = new TH1D(name, name, 4096, 0, 4096);
      if (gManaHistosFolder)
	gManaHistosFolder->Add(gHist[i]);
    }

  gOutputFile->ls();
}

void endRun(int transition,int run,int time)
{
  gIsRunning = false;
  gRunNumber = run;

  if (gOutputFile)
    CloseOutputFile();

  printf("End of run %d\n",run);
}

void HandleNormalEvent(const void* data, int size)
{
  // unpacking of camac events has to match the camac readout list
  // in this case, we have 12 ADC channels (16-bit words)
  if (size != 12*2)
    {
      printf("Camac event size mismatch: expected %d, got %d bytes\n", 8, size);
      return;
    }

  const uint16_t* data16 = (const uint16_t*)data;

  for (int i=0; i<gNumHist; i++)
    gHist[i]->Fill(data16[i]);
}

void HandleScalerEvent(const void* data, int size)
{
  // unpacking of camac events has to match the camac readout list
  // in this case, we have 6 Scaler channels (24-bit words)

  if (size != 6*4)
    {
      printf("Camac scaler event size mismatch: expected %d, got %d bytes\n", 6*4, size);
      return;
    }

  const uint32_t* data32 = (const uint32_t*)data;

  printf("Scalers: ");
  for (int i=0; i<6; i++)
    printf(" %8d", data32[i] & 0x00FFFFFF);
  printf("\n");
}

void HandleWatchdogEvent(const void* data, int size)
{


}

void HandleCCUSB(int size, const void* data)
{
  bool disasm = false;
  const uint16_t *data16 = (const uint16_t*) data;
  int i = 0;
  int nevents = data16[i];
  if (disasm)
    printf("%04d: 0x%04x: Buffer header: %d events in this buffer\n", i, data16[i], nevents);
  i++;
  for (int ievent=0; ievent<nevents; ievent++)
    {
      if (disasm)
	printf("Event %d:\n", ievent);
      int eventLength = data16[i] & 0x0FFF;
      int isWatchdog = 0; // this has to be verified: data16[i] & 0x8000;
      int isScaler   = data16[i] & 0x2000;
      if (disasm)
	printf("%04d: 0x%04x: Event header: %d words, flags: watchdog=%d, scaler=%d\n", i, data16[i], eventLength, isWatchdog, isScaler);
      i++;
      if (disasm)
	for (int iword=0; iword<eventLength; iword++)
	  printf("%04d: 0x%04x: Event data (%d)\n", i+iword, data16[i+iword], data16[i+iword]);
      if (isWatchdog)
	HandleWatchdogEvent(data16+i, 2*eventLength);
      else if (isScaler)
	HandleWatchdogEvent(data16+i, 2*eventLength);
      else
	HandleNormalEvent(data16+i, 2*eventLength);
      i+=eventLength;
    }

  if (data16[i] != 0xFFFF) // expect the buffer terminator
    printf("CCUSB unpacking error: missing buffer terminator!\n");
  i++;
  if (i != size)
    printf("CCUSB unpacking error: unexpected data after end of event!\n");

  if (disasm)
    for (; i<size; i++)
      printf("%04d: 0x%04x: Leftover data (%d)\n", i, data16[i], data16[i]);
}

void HandleMidasEvent(TMidasEvent& event)
{
  int eventId = event.GetEventId();
 
  if ((eventId == 1)&&(gIsRunning==true)) // CCUSB CAMAC data
    {
      //printf("SIS event\n");
      //event.Print("a");
      void *ptr;
      int size = event.LocateBank(NULL,"CCUS",&ptr);
      HandleCCUSB(size,ptr);
    }
  else
    {
      // unknown event type
      event.Print();
    }
}

void eventHandler(const void*pheader,const void*pdata,int size)
{
  TMidasEvent event;
  memcpy(event.GetEventHeader(), pheader, sizeof(EventHeader_t));
  event.SetData(size, (char*)pdata);
  event.SetBankList();
  HandleMidasEvent(event);
}

int ProcessMidasFile(TApplication*app,const char*fname)
{
  TMidasFile f;
  bool tryOpen = f.Open(fname);

  if (!tryOpen)
    {
      printf("Cannot open input file \"%s\"\n",fname);
      return -1;
    }

  int i=0;
  while (1)
    {
      TMidasEvent event;
      if (!f.Read(&event))
	break;

      int eventId = event.GetEventId();
      //printf("Have an event of type %d\n",eventId);

      if ((eventId & 0xFFFF) == 0x8000)
	{
	  // begin run
	  event.Print();

	  //char buf[256];
	  //memset(buf,0,sizeof(buf));
	  //memcpy(buf,event.GetData(),255);
	  //printf("buf is [%s]\n",buf);

	  //
	  // Load ODB contents from the ODB XML file
	  //
	  if (gOdb)
	    delete gOdb;
	  gOdb = new XmlOdb(event.GetData(),event.GetDataSize());

	  startRun(0,event.GetSerialNumber(),0);
	}
      else if ((eventId & 0xFFFF) == 0x8001)
	{
	  // end run
	  event.Print();
	}
      else
	{
	  event.SetBankList();
	  //event.Print();
	  HandleMidasEvent(event);
	}
	
      if((i%500)==0)
	{
	  //resetClock2time();
	  printf("Processing event %d\n",i);
	  //SISperiodic();
	  //StepThroughSISBuffer();
	}
      
      i++;
      if ((gEventCutoff!=0)&&(i>=gEventCutoff))
	{
	  printf("Reached event %d, exiting loop.\n",i);
	  break;
	}
    }
  
  f.Close();

  endRun(0,gRunNumber,0);

  // start the ROOT GUI event loop
  //  app->Run(kTRUE);

  return 0;
}

#ifdef HAVE_MIDAS

void MidasPollHandler()
{
  if (!(TMidasOnline::instance()->poll(0)))
    gSystem->ExitLoop();
}

int ProcessMidasOnline(TApplication*app)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(NULL,NULL,"alpharoot");
   assert(err == 0);

   gOdb = midas;

   midas->setTransitionHandlers(startRun,endRun,NULL,NULL);
   midas->registerTransitions();

   /* reqister event requests */

   midas->setEventHandler(eventHandler);
   midas->eventRequest("SYSTEM",-1,-1,(1<<1));

   /* fill present run parameters */

   gRunNumber = gOdb->odbReadInt("/runinfo/Run number");

   if ((gOdb->odbReadInt("/runinfo/State") == 3))
     startRun(0,gRunNumber,0);

   printf("Startup: run %d, is running: %d, is pedestals run: %d\n",gRunNumber,gIsRunning,gIsPedestalsRun);
   
   MyPeriodic tm(100,MidasPollHandler);
   //MyPeriodic th(1000,SISperiodic);
   //MyPeriodic tn(1000,StepThroughSISBuffer);
   //MyPeriodic to(1000,Scalerperiodic);

   /*---- start main loop ----*/

   //loop_online();
   app->Run(kTRUE);

   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif

#include <TGMenu.h>

class MainWindow: public TGMainFrame {

private:
  TGPopupMenu*		menuFile;
  TGPopupMenu* 		menuControls;
  TGMenuBar*		menuBar;
  TGLayoutHints*	menuBarLayout;
  TGLayoutHints*	menuBarItemLayout;
  
public:
  MainWindow(const TGWindow*w,int s1,int s2);
  virtual ~MainWindow(); // Closing the control window closes the whole program
  virtual void CloseWindow();
  
  Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);
};

#define M_FILE_EXIT 0

Bool_t MainWindow::ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2)
{
   // printf("GUI Message %d %d %d\n",(int)msg,(int)parm1,(int)parm2);
    switch (GET_MSG(msg))
      {
      default:
	break;
      case kC_COMMAND:
	switch (GET_SUBMSG(msg))
	  {
	  default:
	    break;
	  case kCM_MENU:
	    switch (parm1)
	      {
	      default:
		break;
	      case M_FILE_EXIT:
	        if(gIsRunning)
    		   endRun(0,gRunNumber,0);
		gSystem->ExitLoop();
		break;
	      }
	    break;
	  }
	break;
      }

    return kTRUE;
}

MainWindow::MainWindow(const TGWindow*w,int s1,int s2) // ctor
    : TGMainFrame(w,s1,s2)
{
   //SetCleanup(kDeepCleanup);
   
   SetWindowName("ROOT Analyzer Control");

   // layout the gui
   menuFile = new TGPopupMenu(gClient->GetRoot());
   menuFile->AddEntry("Exit", M_FILE_EXIT);

   menuBarItemLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft, 0, 4, 0, 0);

   menuFile->Associate(this);
   menuControls->Associate(this);

   menuBar = new TGMenuBar(this, 1, 1, kRaisedFrame);
   menuBar->AddPopup("&File",     menuFile,     menuBarItemLayout);
   //menuBar->AddPopup("&Controls", menuControls, menuBarItemLayout);
   menuBar->Layout();

   menuBarLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft|kLHintsExpandX);
   AddFrame(menuBar,menuBarLayout);
   
   MapSubwindows(); 
   Layout();
   MapWindow();
}

MainWindow::~MainWindow()
{
    delete menuFile;
    delete menuControls;
    delete menuBar;
    delete menuBarLayout;
    delete menuBarItemLayout;
}

void MainWindow::CloseWindow()
{
    if(gIsRunning)
    	endRun(0,gRunNumber,0);
    gSystem->ExitLoop();
}

static bool gEnableShowMem = false;

int ShowMem(const char* label)
{
  if (!gEnableShowMem)
    return 0;

  FILE* fp = fopen("/proc/self/statm","r");
  if (!fp)
    return 0;

  int mem = 0;
  fscanf(fp,"%d",&mem);
  fclose(fp);

  if (label)
    printf("memory at %s is %d\n", label, mem);

  return mem;
}

void help()
{
  printf("\nALPHA ROOT usage:\n");
  printf("\n./alpharoot.exe [-eMaxEvents] [-iInitFile] [-m] [-g] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-i: Specifies a initialization file other than alpharoot.ini\n");
  printf("\t-e: Number of events to be read (only works in offline mode\n");
  printf("\t-m: Enable memory leak debugging\n");
  printf("\t-g: Enable graphics display when processing data files\n");
  printf("\n");
  printf("Example1: analyze online data: alpharoot.exe\n");
  printf("Example2: analyze existing data: alpharoot.exe /data/alpha/current/run00500.mid\n");
  exit(1);
}

// Main function call

#include "midasServer.h"

int main(int argc, char *argv[])
{
   setbuf(stdout,NULL);
   setbuf(stderr,NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
 
   std::vector<std::string> args;
   for (int i=0; i<argc; i++)
     {
       args.push_back(argv[i]);
       if (strcmp(argv[i],"-h")==0)
	 help();
     }

   TApplication *app = new TApplication("alpharoot", &argc, argv);

   if(gROOT->IsBatch()) {
   	printf("Cannot run in batch mode\n");
	return 1;
   }

   bool forceEnableGraphics = false;
   int  tcpPort = 9090;

   for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
     {
       const char* arg = args[i].c_str();
       //printf("argv[%d] is %s\n",i,arg);
	   
       if (strncmp(arg,"-e",2)==0)  // Event cutoff flag (only applicable in offline mode)
	 gEventCutoff = atoi(arg+2);
       else if (strncmp(arg,"-m",2)==0) // Enable memory debugging
	 gEnableShowMem = true;
       else if (strncmp(arg,"-p",2)==0) // Set the histogram server port
	 tcpPort = atoi(arg+2);
       else if (strncmp(arg,"-commands",9)==0)
	 help(); // does not return
       else if (strcmp(arg,"-g")==0)  
	 forceEnableGraphics = true;
       else if (arg[0] == '-')
	 help(); // does not return
    }
    
   //MainWindow mainWindow(gClient->GetRoot(), 200, 300);

   StartMidasServer(tcpPort);
	 
   gIsOffline = false;

   for (unsigned int i=1; i<args.size(); i++)
     {
       const char* arg = args[i].c_str();

       if (arg[0] != '-')  
	 {  
	   gIsOffline = true;
	   //gEnableGraphics = false;
	   //gEnableGraphics |= forceEnableGraphics;
	   ProcessMidasFile(app,arg);
	 }
     }

   // if we processed some data files,
   // do not go into online mode.
   if (gIsOffline)
     return 0;
	   
   gIsOffline = false;
   //gEnableGraphics = true;
#ifdef HAVE_MIDAS
   ProcessMidasOnline(app);
#endif
   
   return 0;
}

//end
