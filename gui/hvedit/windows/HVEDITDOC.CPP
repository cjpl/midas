// HVeditDoc.cpp : implementation of the CHVeditDoc class
//

#include "stdafx.h"

#include "HVeditDoc.h"

// CHVeditDoc

IMPLEMENT_DYNCREATE(CHVeditDoc, CDocument)

    BEGIN_MESSAGE_MAP(CHVeditDoc, CDocument)
    END_MESSAGE_MAP()

// CHVeditDoc construction/destruction
    CHVeditDoc::CHVeditDoc()
{
}

CHVeditDoc::~CHVeditDoc()
{
}

BOOL CHVeditDoc::OnNewDocument()
{
   return CDocument::OnNewDocument();
}

// CHVeditDoc serialization

void CHVeditDoc::Serialize(CArchive & ar)
{
}
