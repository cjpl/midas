// HVeditDoc.h : interface of the CeditDoc class
//


#pragma once

class CHVeditDoc : public CDocument
{
public: // create from serialization only
	CHVeditDoc();
	DECLARE_DYNCREATE(CHVeditDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	BOOL OnNewDocument();
	void Serialize(CArchive& ar);

// Implementation
public:
	~CHVeditDoc();

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


