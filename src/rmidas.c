/********************************************************************\

  Name:         fmidas.c
  Created by:   Stefan Ritt

  Contents:     ROOT based Midas GUI for histo display and experiment
                control

  $Log$
  Revision 1.1  2003/04/17 14:18:03  midas
  Initial revision

\*******************************************************************/

#include "TGButton.h"
#include "TRootEmbeddedCanvas.h"
#include "TGLayout.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TSocket.h"
#include "TMessage.h"
#include "TGMsgBox.h"
#include "TApplication.h"
#include "TROOT.h"
#include "TGListBox.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TGMenu.h"

/*------------------------------------------------------------------*/

enum RMidasCommandIdentifiers {
  M_FILE_EXIT,
  M_FILE_CONNECT,
  B_CONNECT,
  B_UPDATE,
};

/*------------------------------------------------------------------*/

class RMidasFrame : public TGMainFrame {

private:
   TGMenuBar           *fMenuBar;
   TGPopupMenu         *fMenuFile;

   TGHorizontalFrame   *fHorz1;
   TGHorizontalFrame   *fHorz2;

   TGListBox           *fListBox;
   TRootEmbeddedCanvas *fCanvas;

   TGTextButton        *fBUpdate;

   char                 fHost[256];   // remote host name
   TSocket             *fSock;

   TObjArray           *fHistoNames;
   TH1                 *fHisto;

   int  ConnectServer();
   void GetHistoList();
   void GetHisto(const char *name);

public:
   RMidasFrame(const TGWindow *p, UInt_t w, UInt_t h, char *host = "localhost");
   ~RMidasFrame();

   void CloseWindow();
   Bool_t ProcessMessage(Long_t msg, Long_t param1, Long_t param2);
};

/*------------------------------------------------------------------*/

int RMidasFrame::ConnectServer()
{
char str[80];

  if (fSock)
    {
    /* disconnect */
    fSock->Close();
    delete fSock;
    fSock = NULL;

    SetWindowName("RMidas");

    fMenuFile->DeleteEntry(fMenuFile->GetEntry(M_FILE_CONNECT));
    sprintf(str, "&Connect to %s", fHost);
    fMenuFile->AddEntry(str, M_FILE_CONNECT, NULL, NULL, fMenuFile->GetEntry(M_FILE_EXIT));

    fListBox->RemoveEntries(0, 999);
    return 1;
    }

  /* Connect to RMidasServ */
  fSock = new TSocket(fHost, 9090);

  if (!fSock->IsValid())
    {
    sprintf(str, "Cannot connect to RMidas server on host %s, port 9090\n", fHost);
    new TGMsgBox(gClient->GetRoot(), this, "Error", str, kMBIconExclamation, 0, NULL);

    SetWindowName("RMidas");
    return 0;
    }
  else
    {
    fSock->Recv(str, sizeof(str));
    if (strncmp(str, "RMSERV", 6))
      {
      sprintf(str, "RMidas server not found on host %s, port 9090\n", fHost);
      new TGMsgBox(gClient->GetRoot(), this, "Error", str, kMBIconExclamation, 0, NULL);

      fSock->Close();
      delete fSock;
      fSock = NULL;

      SetWindowName("RMidas");
      return 0;
      }
    else
      {
      sprintf(str, "RMidas connected to %s", fHost);
      SetWindowName(str);
      GetHistoList();
 
      fMenuFile->DeleteEntry(fMenuFile->GetEntry(M_FILE_CONNECT));
      fMenuFile->AddEntry("&Disconnect", M_FILE_CONNECT, NULL, NULL, fMenuFile->GetEntry(M_FILE_EXIT));
      return 1;
      }
    }
}

/*------------------------------------------------------------------*/

void RMidasFrame::GetHistoList()
{
int i;

  fSock->Send("LIST");

  TMessage *m;
  fSock->Recv(m);

  if (fHistoNames)
    delete fHistoNames;

  fHistoNames = (TObjArray *)m->ReadObject(m->GetClass());

  for (i=0 ; i<fHistoNames->GetLast()+1 ; i++)
    fListBox->AddEntry((const char *)((TObjString *)(fHistoNames->At(i)))->String(), i);

  fListBox->MapSubwindows();
  fListBox->Layout();
  delete m;
}

/*------------------------------------------------------------------*/

void RMidasFrame::GetHisto(const char *name)
{
TMessage *m;
char str[32];

  sprintf(str, "GET %s", name);
  fSock->Send(str);
  fSock->Recv(m);

  if (fHisto)
    delete fHisto;

  fHisto = (TH1*)m->ReadObject(m->GetClass());

  if (!fHisto)
    printf("Histo \"%s\" not available\n", name);
  else
    fHisto->Draw();

  fCanvas->GetCanvas()->Modified();
  fCanvas->GetCanvas()->Update();

  delete m;
}

/*------------------------------------------------------------------*/

RMidasFrame::RMidasFrame(const TGWindow *p, UInt_t w, UInt_t h, char *host) :
  TGMainFrame(p, w, h)
{
  /* save host name */
  strcpy(fHost, host);
  fSock = NULL;

  /* Create Menus */
  fMenuFile = new TGPopupMenu(fClient->GetRoot());
  fMenuFile->AddEntry("&Disconnect", M_FILE_CONNECT);
  fMenuFile->AddEntry("E&xit", M_FILE_EXIT);
  fMenuFile->Associate(this);

  fMenuBar = new TGMenuBar(this, 1, 1, kHorizontalFrame);
  fMenuBar->AddPopup("&File", fMenuFile, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0));

  AddFrame(fMenuBar, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX, 0, 0, 1, 1));

  /* Create a horizonal frame containing histo selection tree and canvas */
  fHorz1 = new TGHorizontalFrame(this, 600, 400);
  AddFrame(fHorz1, new TGLayoutHints(kLHintsExpandX|kLHintsExpandY, 3, 0, 3, 3));

  /* Create ListBox widget */
  fListBox = new TGListBox(fHorz1, 1);
  fListBox->Resize(100,400);
  fListBox->Associate(this);
  fHorz1->AddFrame(fListBox,  new TGLayoutHints(kLHintsExpandY, 0, 0, 0, 0));

  /* Create an embedded canvas and add to the main frame, centered in x and y */
  fCanvas = new TRootEmbeddedCanvas("Canvas", fHorz1, 400, 400);
  fHorz1->AddFrame(fCanvas, new TGLayoutHints(kLHintsExpandX|kLHintsExpandY, 0, 0, 0, 0));

  /* Create a horizonal frame containing text buttons */
  fHorz2 = new TGHorizontalFrame(this, 600, 30);
  AddFrame(fHorz2, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));

  /* Create "Update" button */
  fBUpdate = new TGTextButton(fHorz2, "Update", B_UPDATE);
  fBUpdate->Associate(this);
  fHorz2->AddFrame(fBUpdate, new TGLayoutHints(kLHintsCenterX, 10, 10, 0, 0));

  SetWindowName("RMidas");

  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();

  fHisto = 0;
  fHistoNames = 0;

  /* connect to MRootServer */
  ConnectServer();
}

void RMidasFrame::CloseWindow()
{
   // Got close message for this MainFrame. Terminate the application
   // or returns from the TApplication event loop (depending on the
   // argument specified in TApplication::Run()).

   gApplication->Terminate(0);
}

Bool_t RMidasFrame::ProcessMessage(Long_t msg, Long_t param1, Long_t param2)
{
  // Process messages coming from widgets associated with the dialog.

  switch (GET_MSG(msg)) 
    {
    case kC_COMMAND:

       switch (GET_SUBMSG(msg)) 
         {
         case kCM_MENU:
           switch(param1)
             {
             case M_FILE_EXIT:
               CloseWindow();
               break;

             case M_FILE_CONNECT:
               ConnectServer();
               break;
             }
           break;

         case kCM_BUTTON:
           switch(param1) 
             {
             case B_UPDATE:
               if (fHisto)
                 GetHisto((char *)fHisto->GetName());
               break;
             }
           break;

         case kCM_LISTBOX:
           GetHisto((const char *)((TObjString *)(fHistoNames->At(param2)))->String());
           break;


         case kCM_TAB:
           printf("Tab item %ld activated\n", param1);
           break;
         }
       break;

    }

  return kTRUE;
}

RMidasFrame::~RMidasFrame()
{
   // Clean up

   delete fMenuBar;
   delete fMenuFile;
   delete fHisto;
   delete fHistoNames;
   delete fSock;
   delete fBUpdate;
   delete fHorz1;
   delete fHorz2;
   delete fCanvas;
}

void rmidas()
{
   new RMidasFrame(gClient->GetRoot(), 200, 200, "pc2948");
}

int main(int argc, char **argv)
{
  if (argc < 2)
    {
    printf("Usage: %s <hostname>\n", argv[0]);
    return 1;
    }

  TApplication theApp("RMidas", &argc, argv);

  if (gROOT->IsBatch()) 
    {
    printf("%s: cannot run in batch mode\n", argv[0]);
    return 1;
    }

  new RMidasFrame(gClient->GetRoot(), 200, 200, argv[1]);

  theApp.Run();

  return 0;
}
