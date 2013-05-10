/********************************************************************\

  Name:         mhttpd.cxx
  Created by:   Stefan Ritt

  Contents:     Web server program for midas RPC calls

  $Id$

\********************************************************************/

const char *mhttpd_svn_revision = "$Rev$";

#include <math.h>
#include <assert.h>
#include <float.h>
#include <algorithm>
#include "midas.h"
#include "msystem.h"
#include "mxml.h"
extern "C" {
#include "mgd.h"
}
#include "history.h"

#ifdef HAVE_MSCB
#include "mscb.h"
#endif

/* refresh times in seconds */
#define DEFAULT_REFRESH 60

/* time until mhttpd disconnects from MIDAS */
#define CONNECT_TIME  3600*24

/* size of buffer for incoming data, must fit sum of all attachments */
#define WEB_BUFFER_SIZE 100000

size_t return_size = WEB_BUFFER_SIZE;
char *return_buffer = (char*)malloc(return_size);

int strlen_retbuf;
int return_length;
char host_name[256];
char referer[256];
int tcp_port = 80;

#define MAX_GROUPS    32
#define MAX_VARS     100
#define MAX_PARAM    500
#define PARAM_LENGTH 256
#define TEXT_SIZE  50000

char _param[MAX_PARAM][PARAM_LENGTH];
char *_value[MAX_PARAM];
char _text[TEXT_SIZE];
char *_attachment_buffer[3];
INT _attachment_size[3];
struct in_addr remote_addr;
INT _sock;
BOOL elog_mode = FALSE;
BOOL history_mode = FALSE;
BOOL verbose = FALSE;
char midas_hostname[256];
char midas_expt[256];
#define MAX_N_ALLOWED_HOSTS 10
char allowed_host[MAX_N_ALLOWED_HOSTS][PARAM_LENGTH];
int  n_allowed_hosts;

const char *mname[] = {
   "January",
   "February",
   "March",
   "April",
   "May",
   "June",
   "July",
   "August",
   "September",
   "October",
   "November",
   "December"
};

char type_list[20][NAME_LENGTH] = {
   "Routine",
   "Shift summary",
   "Minor error",
   "Severe error",
   "Fix",
   "Question",
   "Info",
   "Modification",
   "Reply",
   "Alarm",
   "Test",
   "Other"
};

char system_list[20][NAME_LENGTH] = {
   "General",
   "DAQ",
   "Detector",
   "Electronics",
   "Target",
   "Beamline"
};


struct Filetype {
   char ext[32];
   char type[32];
};

const Filetype filetype[] = {

   {
   ".JPG", "image/jpeg",}, {
   ".GIF", "image/gif",}, {
   ".PNG", "image/png",}, {
   ".PS", "application/postscript",}, {
   ".EPS", "application/postscript",}, {
   ".HTML", "text/html",}, {
   ".HTM", "text/html",}, {
   ".XLS", "application/x-msexcel",}, {
   ".DOC", "application/msword",}, {
   ".PDF", "application/pdf",}, {
   ".TXT", "text/plain",}, {
   ".ASC", "text/plain",}, {
   ".ZIP", "application/x-zip-compressed",}, {
""},};

/*------------------------------------------------------------------*/

const unsigned char favicon_png[] = {
   0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48,
   0x44, 0x52,
   0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90,
   0x91, 0x68,
   0x36, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4D, 0x45, 0x07, 0xD4, 0x0B, 0x1A, 0x08,
   0x37, 0x07,
   0x0D, 0x7F, 0x16, 0x5C, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00,
   0x2E, 0x23,
   0x00, 0x00, 0x2E, 0x23, 0x01, 0x78, 0xA5, 0x3F, 0x76, 0x00, 0x00, 0x00, 0x04, 0x67,
   0x41, 0x4D,
   0x41, 0x00, 0x00, 0xB1, 0x8F, 0x0B, 0xFC, 0x61, 0x05, 0x00, 0x00, 0x01, 0x7D, 0x49,
   0x44, 0x41,
   0x54, 0x78, 0xDA, 0x63, 0xFC, 0xFF, 0xFF, 0x3F, 0x03, 0x29, 0x80, 0x09, 0xAB, 0xE8,
   0xD2, 0x65,
   0x77, 0x36, 0x6F, 0x7E, 0x8A, 0x5D, 0xC7, 0x7F, 0x0C, 0x30, 0x67, 0xEE, 0x0D, 0x56,
   0xCE, 0xCD,
   0x5C, 0xBC, 0x3B, 0xB6, 0x6D, 0x7F, 0x81, 0x29, 0xCB, 0x88, 0xE6, 0x24, 0x20, 0x57,
   0x50, 0x7C,
   0xDD, 0xCF, 0x1F, 0x6C, 0x40, 0xCB, 0xB5, 0xB5, 0x05, 0xCF, 0x1C, 0xB7, 0x42, 0xB3,
   0x80, 0x05,
   0x8D, 0xCF, 0xC8, 0xC8, 0x58, 0x5A, 0x2A, 0xFB, 0xF6, 0x4D, 0x37, 0x1B, 0xAB, 0xA0,
   0xB4, 0x4C,
   0x0A, 0x51, 0x4E, 0x02, 0x82, 0x85, 0xCB, 0x12, 0x0E, 0x1D, 0xAB, 0xC7, 0x2A, 0xC5,
   0x82, 0x69,
   0xC4, 0xAF, 0x5F, 0x7F, 0x1E, 0x3F, 0xF8, 0xCD, 0xCB, 0xF1, 0xF5, 0xEF, 0xDF, 0x7F,
   0xCC, 0xCC,
   0x4C, 0x84, 0x6D, 0x98, 0x59, 0xD5, 0xEB, 0xCF, 0xA5, 0x16, 0xC4, 0xAB, 0x71, 0x72,
   0xCB, 0x21,
   0x4C, 0x59, 0x74, 0x03, 0x5E, 0x3F, 0x7F, 0xB3, 0x6B, 0xD6, 0x22, 0x46, 0xA6, 0x7F,
   0x0C, 0x0C,
   0x7F, 0xD7, 0x75, 0x4D, 0xFB, 0xF1, 0xFD, 0x27, 0x81, 0x78, 0xB8, 0x7D, 0xE9, 0x0A,
   0xCB, 0xFF,
   0xDF, 0x4C, 0x8C, 0x8C, 0x40, 0xF6, 0xAD, 0x4B, 0x67, 0x1F, 0xDE, 0xBD, 0x8B, 0x45,
   0x03, 0x3C,
   0x60, 0x8F, 0x9D, 0xD8, 0xB3, 0xEB, 0x74, 0xB5, 0x90, 0x26, 0x07, 0x03, 0x48, 0xE4,
   0x3F, 0x8F,
   0xF6, 0xFF, 0x1B, 0x0F, 0x9A, 0x1E, 0x3E, 0x3A, 0xFB, 0xF3, 0xDB, 0x8F, 0xB7, 0x0F,
   0x9E, 0x43,
   0x83, 0xF1, 0xCF, 0xDF, 0x3F, 0x8A, 0x29, 0xCE, 0x3F, 0x7F, 0xFD, 0xFC, 0xCF, 0xF0,
   0xDF, 0x98,
   0xE9, 0xB5, 0x8F, 0xBD, 0x8A, 0x3C, 0x6F, 0xEC, 0xB9, 0x2D, 0x47, 0xFE, 0xFC, 0xFF,
   0x6F, 0x16,
   0x6C, 0xF3, 0xEC, 0xD3, 0x1C, 0x2E, 0x96, 0xEF, 0xBF, 0xAB, 0x7E, 0x32, 0x7D, 0xE2,
   0x10, 0xCE,
   0x88, 0xF4, 0x69, 0x2B, 0x60, 0xFC, 0xF4, 0xF5, 0x97, 0x78, 0x8A, 0x36, 0xD8, 0x44,
   0x86, 0x18,
   0x0D, 0xD7, 0x29, 0x95, 0x13, 0xD8, 0xD9, 0x58, 0xE1, 0x0E, 0xF8, 0xF1, 0xF3, 0xDB,
   0xC6, 0xD6,
   0xEC, 0x5F, 0x53, 0x8E, 0xBF, 0xFE, 0xC3, 0x70, 0x93, 0x8D, 0x6D, 0xDA, 0xCB, 0x0B,
   0x4C, 0x3F,
   0xFF, 0xFC, 0xFA, 0xCF, 0x0C, 0xB4, 0x09, 0x84, 0x54, 0xD5, 0x74, 0x91, 0x55, 0x03,
   0x01, 0x07,
   0x3B, 0x97, 0x96, 0x6E, 0xC8, 0x17, 0xFE, 0x7F, 0x4F, 0xF8, 0xFE, 0xBC, 0x95, 0x16,
   0x60, 0x62,
   0x62, 0x64, 0xE1, 0xE6, 0x60, 0x73, 0xD1, 0xB2, 0x7A, 0xFA, 0xE2, 0xF1, 0xDF, 0x3F,
   0xFF, 0xC4,
   0x78, 0x44, 0x31, 0xA3, 0x45, 0x2B, 0xD0, 0xE3, 0xF6, 0xD9, 0xE3, 0x2F, 0x2E, 0x9D,
   0x29, 0xA9,
   0xAC, 0x07, 0xA6, 0x03, 0xF4, 0xB4, 0x44, 0x10, 0x00, 0x00, 0x75, 0x65, 0x12, 0xB0,
   0x49, 0xFF,
   0x3F, 0x68, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

const unsigned char favicon_ico[] = {
   0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x10, 0x00, 0x01, 0x00, 0x04, 0x00,
   0x28, 0x01,
   0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
   0x20, 0x00,
   0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00,
   0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xB4, 0x0F,
   0x0A, 0x00, 0x5C, 0x86, 0x4C, 0x00, 0x2F, 0x5E, 0x1A, 0x00, 0xBF, 0xD3, 0xD7, 0x00,
   0x29, 0x17,
   0x8D, 0x00, 0x50, 0xA7, 0xA4, 0x00, 0x59, 0x57, 0x7F, 0x00, 0xC6, 0xA3, 0xAC, 0x00,
   0xFC, 0xFE,
   0xFC, 0x00, 0x28, 0x12, 0x53, 0x00, 0x58, 0x7D, 0x72, 0x00, 0xC4, 0x3A, 0x34, 0x00,
   0x3C, 0x3D,
   0x69, 0x00, 0xC5, 0xB6, 0xB9, 0x00, 0x94, 0x92, 0x87, 0x00, 0x7E, 0x7A, 0xAA, 0x00,
   0x88, 0x88,
   0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x81, 0x22, 0xD8, 0x88, 0x88, 0x88, 0xF6, 0xD8,
   0x82, 0x22,
   0xE8, 0x88, 0x88, 0x8D, 0x44, 0x98, 0x82, 0x22, 0xA8, 0x88, 0x88, 0x8F, 0x44, 0x48,
   0x82, 0x22,
   0x25, 0x76, 0x67, 0x55, 0x44, 0xF8, 0x88, 0x88, 0x3A, 0xC9, 0x9C, 0x53, 0x83, 0x88,
   0x88, 0x88,
   0x8D, 0x99, 0x99, 0x38, 0x88, 0x88, 0x88, 0x88, 0x88, 0x99, 0x9C, 0x88, 0x88, 0x88,
   0x88, 0x88,
   0x88, 0xF9, 0x9D, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x8A, 0x58, 0x88, 0x88, 0x88,
   0x88, 0x88,
   0x88, 0x85, 0xD8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xEA, 0xAE, 0x88, 0x88, 0x88,
   0x88, 0x88,
   0x88, 0x00, 0x0B, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0D, 0x88, 0x88, 0x88,
   0x88, 0x88,
   0x88, 0x87, 0xD8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
   0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char reload_png[] = {
0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x20,0x08,0x06,0x00,0x00,0x00,0x73,0x7A,0x7A,
0xF4,0x00,0x00,0x00,0x07,0x74,0x49,0x4D,0x45,0x07,0xD9,0x0C,0x0B,0x0B,0x00,0x2D,
0xA2,0x9E,0x3E,0xD3,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0B,0x13,
0x00,0x00,0x0B,0x13,0x01,0x00,0x9A,0x9C,0x18,0x00,0x00,0x00,0x04,0x67,0x41,0x4D,
0x41,0x00,0x00,0xB1,0x8F,0x0B,0xFC,0x61,0x05,0x00,0x00,0x08,0x37,0x49,0x44,0x41,
0x54,0x78,0xDA,0xC5,0x57,0x7B,0x8C,0x54,0xD5,0x19,0xFF,0xDD,0xE7,0xDC,0x99,0xD9,
0x9D,0x9D,0x19,0x16,0x76,0x61,0x59,0xD8,0x5D,0x97,0x5D,0xCA,0xAE,0xF2,0xD8,0x15,
0x2D,0xD0,0x08,0x0D,0x0A,0x26,0xAD,0x8D,0x0F,0xA0,0x2A,0x49,0x63,0x83,0x86,0xBA,
0xA9,0xD2,0x6A,0x52,0x4D,0xF8,0xD7,0x98,0xDA,0xC6,0x34,0x4D,0xAB,0x49,0xD3,0x98,
0xC6,0xB2,0x92,0xC6,0x0A,0x12,0x35,0x51,0x8B,0xA5,0x8D,0x46,0x14,0x44,0xAA,0x3C,
0x0A,0xD8,0x65,0x81,0x65,0x76,0xD9,0xE7,0xBC,0xEF,0xCC,0x7D,0xF7,0x3B,0xE7,0xCE,
0x63,0x77,0x0B,0x5A,0x9A,0x26,0x9E,0xE4,0x9B,0x3B,0xF7,0x9C,0xEF,0xF1,0x3B,0xDF,
0xF7,0x9D,0xEF,0x7C,0x17,0xF8,0x9A,0x87,0x70,0x1D,0xBC,0x12,0xD1,0x1A,0xA2,0xCD,
0x44,0xAB,0x05,0x11,0x9D,0x5A,0x08,0xF3,0x54,0x0D,0xAA,0x59,0x84,0x59,0xD4,0x31,
0xE6,0xB9,0x38,0x47,0x6B,0x47,0x88,0xDE,0x26,0xFA,0x90,0xC8,0xFE,0x7F,0x00,0x08,
0x13,0xED,0x24,0xCE,0xBE,0xE6,0xE5,0x4A,0x6B,0xD7,0x7A,0x19,0x5D,0xB7,0x0A,0x58,
0xDA,0xED,0x62,0xC1,0x7C,0x1B,0xD1,0x5A,0x1B,0xD9,0xBC,0x84,0xE1,0x2B,0x32,0xCE,
0x9E,0x14,0x71,0xE2,0xB0,0x87,0x93,0x7F,0xB3,0x71,0xE9,0xB8,0x7D,0x01,0x1E,0x5E,
0x20,0xD9,0x17,0x89,0x72,0xFF,0x2B,0x80,0x7B,0x05,0x09,0xBF,0x6A,0xD9,0x10,0x5E,
0xD8,0xB3,0x25,0x8C,0xE6,0x76,0x20,0x56,0x63,0x61,0x5E,0xCC,0xA4,0xA7,0x83,0x90,
0xE6,0x40,0x51,0x3C,0x14,0x4D,0x11,0xB9,0x82,0x84,0x0C,0x01,0x49,0x66,0x65,0xA4,
0x72,0x0A,0x86,0x06,0x80,0x4F,0xFE,0xA4,0x63,0xE0,0xBD,0x7C,0xC2,0x73,0xF0,0x13,
0xD2,0xF5,0xEA,0xB5,0xDC,0x7A,0xB5,0xA1,0x10,0xFD,0x36,0xDC,0xAC,0xFE,0x7C,0xE5,
0xAE,0xC6,0xBA,0x25,0x1B,0x43,0xE8,0x6C,0xB7,0xD0,0xDD,0x9A,0x47,0x7B,0x53,0x11,
0x91,0xB0,0x03,0x41,0x14,0x60,0xB8,0x64,0xB8,0x28,0x21,0x6F,0xCA,0x30,0x1C,0x11,
0x0E,0xC5,0x45,0x94,0x7D,0xAD,0x9E,0x2A,0x23,0xB2,0x34,0x8C,0xE8,0x4D,0x35,0x91,
0xE4,0x80,0x71,0x9F,0x99,0x76,0x16,0xD0,0xCA,0xBB,0x44,0xCE,0x57,0x01,0x08,0x10,
0xBD,0x56,0xB7,0x2A,0xF2,0x40,0xC7,0x23,0x0B,0x84,0xEE,0x2E,0x13,0x77,0xAF,0x9B,
0xC0,0x8D,0x6D,0x79,0xDA,0xB1,0x0B,0xC7,0x15,0x60,0x39,0x44,0x64,0xDC,0xB2,0x05,
0x98,0x64,0x98,0xBF,0xD3,0xD3,0xE4,0xF3,0x02,0x3C,0x02,0xA7,0x85,0x3C,0x4E,0x96,
0xAC,0x20,0xD8,0x1D,0x17,0xF2,0x93,0x4E,0x4F,0x71,0xD8,0xE8,0x21,0xDD,0xFB,0xA6,
0xE7,0xC6,0x6C,0x00,0x2C,0x24,0x7B,0x6A,0x7A,0x62,0xF7,0x34,0x6F,0x6D,0xC4,0xB6,
0x0D,0xE3,0xF8,0xDE,0xDA,0x49,0x68,0xAA,0xCB,0x0D,0x94,0x0D,0x31,0x32,0xC8,0xB8,
0x61,0xF9,0x20,0xEC,0xF2,0x3C,0x07,0x25,0x72,0x10,0xEC,0xDD,0x23,0x75,0x35,0x11,
0xFA,0x25,0xAF,0x98,0xF3,0x63,0x28,0xA4,0x9C,0x25,0xE6,0x70,0xB1,0x83,0x6D,0xF0,
0x5A,0x00,0x1E,0x53,0x5A,0xC2,0x4F,0x34,0xDD,0xDF,0x24,0x3C,0xB9,0x75,0x08,0xBD,
0x9D,0xB9,0xAA,0x72,0x7A,0xEA,0x86,0x84,0x34,0xC5,0x38,0x93,0x95,0x50,0xA0,0x98,
0x5B,0x14,0x7B,0x87,0x00,0x78,0xB4,0x26,0xBA,0xA4,0xCC,0xF3,0xB8,0x12,0x93,0x03,
0x14,0x2B,0xDE,0x51,0x35,0x0F,0xA1,0xB0,0x87,0x4C,0x34,0x86,0xFC,0x85,0xC2,0x32,
0x27,0x65,0xA5,0x88,0xED,0xE3,0xD9,0x00,0x9A,0xA1,0x8A,0xFB,0xA2,0x0F,0xB4,0x05,
0x76,0x3F,0x74,0x19,0x5D,0xAD,0xBA,0xBF,0x1B,0x52,0x52,0x20,0x43,0x57,0xA6,0x02,
0x48,0x66,0x64,0x98,0xB4,0x6B,0xD7,0x13,0xF8,0xEE,0x98,0xB9,0xE9,0xE4,0x2B,0x24,
0xD7,0x13,0x1A,0x49,0xF0,0x90,0xB3,0x28,0x37,0x6C,0x89,0xEB,0x10,0x64,0x81,0x83,
0x98,0x0C,0xC5,0x05,0xFD,0x1F,0x53,0xEB,0xE0,0x78,0x7B,0x88,0x3D,0x33,0x1D,0xC0,
0x73,0xCA,0x37,0x1B,0xD7,0x3C,0xBC,0x53,0xC7,0xFA,0xE5,0xA9,0x8A,0xBB,0xD3,0xBA,
0x8C,0xA1,0xB1,0x00,0x77,0xB7,0x6F,0x94,0x91,0x84,0xB9,0xC1,0x15,0x58,0x1C,0xDE,
0x88,0x05,0xE1,0x6F,0x21,0xA6,0x76,0xC2,0xA3,0xE3,0x92,0xB3,0xC6,0x4B,0x60,0x04,
0xC8,0xA2,0x87,0x5A,0xC5,0x46,0xDE,0xA2,0x79,0xD3,0x07,0x21,0x52,0x6A,0x0B,0xAA,
0x84,0xC9,0xA4,0xA2,0x3A,0x17,0x73,0x21,0x62,0x7D,0x53,0x2E,0x19,0x8F,0x41,0x11,
0xB7,0x77,0x6C,0x0E,0x53,0xC2,0x9D,0xAF,0x18,0x4F,0xE6,0xC8,0xF8,0x78,0x80,0x2B,
0xF4,0x13,0x44,0xC2,0xF2,0xF8,0x0F,0xB0,0xBA,0xFE,0x31,0x44,0xD4,0x45,0x10,0x84,
0xEA,0x29,0xF6,0xC8,0xFD,0x19,0xF3,0x12,0x8E,0x4C,0xFC,0x1A,0x9F,0x4D,0xFD,0x81,
0x64,0x5C,0x3E,0xDF,0x54,0x53,0x44,0x91,0xC0,0x4F,0x18,0x3E,0x88,0x39,0x0D,0x54,
0x3B,0xD6,0xC6,0x30,0x76,0x78,0x74,0x3B,0x25,0xCB,0x53,0x65,0x0F,0xDC,0x87,0x25,
0xD1,0x6D,0x3F,0xDB,0x9D,0x47,0x63,0xDC,0xE2,0x8C,0x79,0x12,0x18,0x18,0x0D,0x91,
0xA7,0xC8,0xE5,0x7C,0x47,0xB5,0xB8,0xB7,0xA5,0x1F,0x3D,0xF5,0x3B,0xA1,0xC9,0xD1,
0x19,0xC6,0x39,0x38,0x7A,0x67,0xF3,0x37,0x44,0x36,0x61,0x7E,0xE8,0x66,0x9C,0x49,
0xBF,0x0D,0xCB,0xB3,0x38,0xF8,0x48,0xC0,0xC6,0x68,0x2E,0x80,0x82,0xE5,0x87,0x54,
0xD6,0x04,0x8C,0x9D,0xB1,0x55,0x4C,0x16,0x4F,0x94,0x01,0xFC,0xB8,0xF9,0xF6,0xE8,
0xAA,0x9D,0x0F,0xA5,0x60,0xBB,0x02,0x3F,0x4E,0xE7,0xC7,0x82,0xD0,0xC9,0x75,0x3C,
0xDE,0x9E,0x84,0x2D,0x64,0xBC,0x2D,0xF2,0xED,0xD2,0x6E,0x5D,0x5C,0xCC,0x1E,0xC2,
0xB1,0xF1,0x17,0x71,0x62,0xEA,0x8F,0x48,0xE4,0x0E,0x43,0x24,0xEF,0xD4,0xA9,0x8B,
0x39,0x90,0x58,0xA0,0x15,0x0D,0xDA,0x0A,0xF2,0xC4,0x6B,0xB4,0x01,0x3F,0x24,0xAA,
0xEC,0x62,0x38,0xA3,0x71,0xEF,0x0A,0x14,0x8A,0xE4,0x08,0x79,0xF9,0x5C,0x26,0x59,
0x0E,0xC1,0x8D,0x6B,0x6E,0xB3,0xB8,0x71,0xC6,0x90,0xD6,0x15,0x4C,0xE6,0x95,0x4A,
0xA1,0xEC,0x99,0xF3,0x20,0xDA,0x6A,0xD7,0x97,0x32,0x3C,0x87,0xFD,0x83,0x3B,0xF0,
0xCF,0xCC,0x41,0xAA,0x28,0x7E,0x4E,0xB0,0xDF,0xF7,0xC7,0x5E,0xC2,0x37,0x22,0x1B,
0x71,0x77,0xEB,0xEF,0xA1,0x4A,0x35,0x9C,0xBF,0x3B,0xF6,0x20,0x8E,0x4D,0xF6,0x73,
0xB9,0x10,0x1D,0x65,0x4D,0x76,0xA8,0x68,0xA9,0xDC,0x46,0xCD,0x0D,0x01,0xE8,0xC0,
0x2A,0xB1,0x7C,0x02,0x96,0x2D,0x35,0x2B,0xB1,0xBF,0x92,0x51,0x61,0xBA,0x12,0x91,
0xC8,0xE9,0xD6,0xB9,0x3B,0xF8,0xCE,0x58,0x9C,0x0F,0x5C,0x78,0x02,0xC7,0x52,0x87,
0xA0,0xBB,0x94,0xE1,0x25,0x1E,0xF6,0x64,0xEF,0x6C,0x9E,0xAD,0x33,0x3E,0xC6,0xCF,
0xE4,0xCA,0x3A,0x2C,0x0A,0x65,0x34,0x64,0xFB,0x36,0xE8,0xE8,0xAA,0xF5,0x7C,0xEF,
0x8B,0xCB,0x00,0xEA,0x1B,0xEA,0xED,0xCA,0x79,0x9F,0x2C,0x28,0x15,0x41,0x45,0xAC,
0x47,0x43,0x70,0x29,0x67,0x4A,0x9B,0x09,0x1C,0x9E,0x78,0x73,0x06,0xB8,0x99,0x24,
0xF1,0x75,0xC6,0xC7,0x06,0x93,0x63,0xF2,0xE5,0xF5,0x60,0xC0,0x85,0x59,0xAA,0x0F,
0xAA,0xC6,0xBD,0x1B,0x95,0x4B,0xE9,0xAD,0x28,0x8A,0xEB,0x97,0x52,0x62,0xC8,0x18,
0x32,0x4F,0x3E,0x36,0xEA,0xC4,0x38,0x44,0xC1,0x4F,0x95,0x11,0xFD,0x5F,0x28,0x50,
0x4E,0xC0,0xFB,0xF2,0x3B,0x6C,0xB4,0x70,0x01,0xD1,0xC0,0x42,0x2E,0xC7,0x00,0x18,
0x6E,0x8A,0xCF,0x8B,0xA4,0xC6,0xF7,0x32,0xE5,0x19,0x65,0x0D,0xD9,0x15,0x7D,0x00,
0x1E,0x4C,0xC3,0x40,0x40,0xB5,0xFD,0x7A,0x5E,0x70,0xA8,0x9C,0x94,0x8C,0x24,0x72,
0x97,0xF0,0xE9,0x95,0x37,0xD0,0x16,0xED,0xC5,0xA1,0xCB,0x2F,0xF3,0x4B,0xA7,0x5A,
0xB7,0x45,0x3A,0xEB,0x71,0x64,0xAC,0x89,0x19,0x00,0xA2,0x81,0xA6,0x4A,0xB2,0xA6,
0x8D,0x4C,0x45,0xC6,0x71,0x51,0xBD,0x37,0x4C,0x9E,0x9D,0xD9,0x72,0x12,0x8E,0x4C,
0x8E,0x0A,0x2D,0x81,0xB8,0xBF,0xC8,0x76,0xCF,0x5C,0x56,0x1E,0xCF,0x9F,0xEE,0x83,
0x48,0xE9,0xEC,0x50,0x11,0xF1,0x44,0xDF,0x1B,0xAA,0xA8,0xE1,0xE9,0xEE,0x97,0xD1,
0x11,0xE9,0xC5,0x5B,0x97,0x7F,0x87,0x57,0x06,0x9F,0x65,0x67,0x11,0x2B,0x62,0xB7,
0x61,0x9E,0xD6,0xE2,0x7B,0x22,0x7F,0x01,0xA3,0x46,0x92,0xE6,0x7D,0x19,0xD3,0xAA,
0x96,0x75,0x33,0xC5,0xEF,0xA3,0xCB,0x65,0x00,0x47,0x07,0x4F,0x7A,0x2D,0x73,0x3B,
0xFC,0x45,0x91,0xAA,0x18,0x2B,0x1E,0x95,0x21,0xD2,0x7F,0xB1,0x54,0x6F,0x1D,0xF0,
0xBC,0xEF,0xEB,0xF8,0x05,0x3A,0xEB,0x6E,0xE6,0xCB,0x1D,0xB5,0xAB,0xC9,0x6B,0x22,
0xDA,0x6B,0x6F,0xC2,0x23,0x1D,0xBF,0xAC,0xD4,0x88,0x77,0x2E,0xF5,0xA3,0x38,0x6D,
0x23,0x69,0x5D,0xF2,0xC3,0xCC,0xEE,0x8A,0xA1,0x22,0x9B,0xFA,0xA4,0xBC,0x7A,0xE0,
0xC4,0xDF,0x9D,0x8A,0x7B,0xC2,0x8A,0x83,0x22,0x85,0xE1,0x6A,0xC4,0x0C,0xDD,0xB3,
0xE8,0x71,0xAC,0x6D,0xF8,0x4E,0xA5,0x02,0x9E,0x19,0x3F,0x8E,0x47,0x3B,0x9F,0xC3,
0xB3,0xAB,0x5E,0x45,0x54,0xAD,0xE7,0xF3,0xE7,0x26,0x3E,0xC3,0xBE,0xA1,0xBD,0x33,
0x64,0xC7,0xD2,0x6A,0xF5,0x56,0x3D,0x9B,0x61,0x6C,0xEF,0x96,0x3D,0xF0,0xD6,0xE7,
0x07,0xED,0x7C,0x26,0x25,0x84,0xA5,0xA0,0x80,0xB8,0x6A,0xE2,0xB4,0x53,0xCB,0x2B,
0xE0,0xEC,0xB1,0xA1,0x61,0x33,0xEE,0x6F,0xED,0x9B,0x51,0x01,0xEF,0x5A,0xF2,0xC3,
0x19,0x3C,0x5F,0x4C,0x9C,0xC2,0xD3,0x1F,0xFD,0x08,0x19,0x95,0x5C,0xE6,0xF8,0xEE,
0x77,0x29,0xFE,0xA3,0x49,0xBF,0x06,0x98,0x79,0x0A,0xE7,0xA9,0x54,0x9E,0xD9,0x2D,
0x7B,0x20,0x65,0xE4,0xB1,0xE7,0xFD,0x7E,0xBF,0x16,0x30,0x57,0xCF,0xD3,0x0C,0xE8,
0xB6,0xC4,0x13,0xB2,0x4C,0x3A,0x25,0xE9,0xD6,0x96,0x1D,0x94,0xDD,0x22,0xAE,0x36,
0xB2,0x46,0x16,0x2F,0x1D,0x7D,0x01,0x3B,0x0E,0x6D,0x47,0x42,0x4C,0xCF,0x90,0x4D,
0x8C,0xAB,0xFC,0x56,0x65,0xC7,0xD0,0xFA,0x70,0x0C,0x74,0x34,0xD8,0x6D,0x98,0x9A,
0x7E,0x1B,0x7E,0x3E,0x74,0xC2,0x7E,0xB8,0xFB,0xBB,0x41,0x55,0x0C,0x48,0x08,0x49,
0x36,0x46,0x0A,0x1A,0x72,0xB6,0xEC,0x37,0x1A,0x44,0x36,0x25,0xA7,0x48,0xBD,0xC0,
0x9A,0x85,0x6B,0x66,0x80,0x18,0x4E,0x27,0xB0,0xFB,0x2F,0xBB,0xF1,0xCC,0xD1,0x67,
0xF0,0x41,0xE1,0x23,0x14,0xA9,0xA7,0x62,0x85,0xA7,0x2C,0x97,0xD3,0xA9,0xB8,0x8D,
0x28,0xBE,0xF1,0xA4,0x0D,0x6F,0xEF,0x40,0x96,0x32,0xFD,0xFB,0x98,0x75,0x1D,0xA7,
0x1D,0x13,0xFA,0x95,0xB3,0xF6,0xA6,0x8E,0xDB,0x83,0x02,0x33,0x56,0x27,0xDB,0x18,
0x2E,0x6A,0xD0,0x1D,0xD9,0x57,0x48,0x74,0x3C,0x79,0x1A,0x1F,0x9C,0x3A,0x8C,0xDE,
0xC6,0x55,0x88,0x87,0xA2,0x5C,0xB0,0x56,0x8B,0x60,0x10,0x09,0xFC,0x35,0x7B,0x04,
0x16,0x9D,0x92,0x32,0x2F,0xA3,0x42,0x51,0xC4,0xF8,0xB0,0xDF,0x47,0x58,0x16,0x39,
0xF7,0x95,0x2F,0x3C,0x8C,0x17,0x9F,0x24,0xB1,0x83,0x4C,0x76,0x76,0x47,0x74,0x24,
0x95,0x70,0x97,0xE5,0xA6,0xDC,0xAE,0xA6,0x5B,0x82,0xFC,0x22,0x8A,0x4B,0x26,0x92,
0x96,0x8A,0xB4,0xAD,0xF8,0x5E,0xA0,0x13,0x31,0x24,0x8C,0xA3,0xFF,0xD8,0xEB,0x08,
0x9A,0x21,0xAC,0x6C,0xEA,0x22,0x6F,0x08,0xF8,0x74,0xF4,0x0C,0x0E,0x8E,0x7D,0x5C,
0xD9,0x35,0x4B,0xB6,0xA2,0x2E,0x20,0x33,0x2A,0xF8,0xC6,0x59,0xE7,0x74,0xE0,0x22,
0x70,0x72,0x6A,0x2F,0xD9,0x79,0xAA,0x5A,0x4B,0xFE,0x73,0x68,0x44,0xFB,0x96,0xDC,
0x19,0xBA,0xF3,0x96,0x5D,0x71,0xDE,0xE9,0xB2,0xBC,0x18,0x36,0x82,0x18,0x34,0xC2,
0xC8,0xBB,0x4A,0x85,0xD1,0xCD,0xBB,0x58,0x69,0x77,0x51,0x2D,0x58,0x8C,0x03,0xD9,
0xF7,0x60,0x46,0x2C,0x5F,0xA9,0x4D,0xE9,0x9B,0xA5,0x3E,0x52,0x2F,0x15,0x1E,0x83,
0x76,0xBE,0x7F,0x10,0x38,0x3E,0xF1,0x06,0x2D,0x6F,0x21,0x32,0xBE,0x0C,0x00,0xAF,
0x33,0x44,0xBF,0x89,0x77,0xAA,0x3B,0x7A,0x7F,0x5A,0x2F,0x68,0x4D,0x81,0x6A,0x93,
0x42,0xD7,0xF8,0x04,0x51,0x96,0x80,0x14,0x4B,0xBD,0x82,0x50,0xEA,0x07,0x25,0x32,
0x2C,0x99,0x0E,0x5C,0xA3,0x5A,0xF1,0xAC,0xE1,0x22,0xBC,0x3F,0x9F,0xF7,0x90,0xC8,
0xB3,0x0F,0x94,0x5D,0x44,0xD6,0x74,0x43,0x5F,0xF5,0x61,0xB2,0x8D,0x3A,0xDA,0xE7,
0x9B,0xEF,0x88,0x2C,0x68,0xBE,0x2B,0x06,0x79,0x8E,0x5A,0x6D,0xBF,0x1D,0xDF,0xAD,
0x33,0x5A,0xF2,0xE9,0x2D,0xFA,0x38,0x9D,0xA8,0x43,0x23,0x54,0x6A,0x26,0x86,0x28,
0x96,0x8F,0x93,0xAE,0xFD,0x57,0x33,0xF0,0xDF,0x7C,0x9A,0xD5,0x10,0xF5,0x11,0xE7,
0xA3,0x75,0x5D,0xC1,0x45,0x91,0x15,0xB5,0xD0,0xDA,0xC3,0x10,0xE7,0x06,0xE0,0x28,
0x72,0xF5,0xFB,0x40,0xF7,0x60,0x8C,0x18,0x30,0x06,0xF2,0xB0,0x4E,0xA5,0xE1,0x9D,
0xCF,0x9E,0x9F,0xF6,0x69,0xA6,0x5F,0x4B,0xF9,0xF5,0x7C,0x9C,0xB2,0xA2,0xB5,0x8E,
0x68,0x13,0xD1,0x6A,0x92,0xEC,0x10,0x02,0xC2,0x3C,0x41,0x16,0x15,0xD7,0x72,0x4D,
0x98,0xDE,0x18,0x19,0x3C,0x0B,0xBF,0xDD,0x7E,0x07,0xFE,0xC7,0xA9,0x73,0x1D,0xFA,
0xBF,0x9E,0xF1,0x6F,0x1F,0xE9,0x4A,0x5D,0x98,0x0E,0x02,0x3D,0x00,0x00,0x00,0x00,
0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82
};

/*------------------------------------------------------------------*/

void show_hist_page(const char *path, int path_size, char *buffer, int *buffer_size,
                    int refresh);
int vaxis(gdImagePtr im, gdFont * font, int col, int gcol, int x1, int y1, int width,
          int minor, int major, int text, int label, int grid, double ymin, double ymax,
          BOOL logaxis);
void haxis(gdImagePtr im, gdFont * font, int col, int gcol, int x1, int y1, int width,
           int minor, int major, int text, int label, int grid, double xmin, double xmax);
void get_elog_url(char *url, int len);

/* functions from sequencer.cxx */
extern void show_seq_page();
extern void sequencer();
extern void init_sequencer();

/*------------------------------------------------------------------*/

char *stristr(const char *str, const char *pattern)
{
   char c1, c2, *ps, *pp;

   if (str == NULL || pattern == NULL)
      return NULL;

   while (*str) {
      ps = (char *) str;
      pp = (char *) pattern;
      c1 = *ps;
      c2 = *pp;
      if (toupper(c1) == toupper(c2)) {
         while (*pp) {
            c1 = *ps;
            c2 = *pp;

            if (toupper(c1) != toupper(c2))
               break;

            ps++;
            pp++;
         }

         if (!*pp)
            return (char *) str;
      }
      str++;
   }

   return NULL;
}

/*------------------------------------------------------------------*/

int return_grow(size_t len)
{
   //printf("size %d, grow %d, room %d\n", return_size, len, return_size - strlen_retbuf);

   for (int i=0; i<1000; i++) { // infinite loop with protection against infinite looping
      if (strlen_retbuf + len < return_size-40)
         return SUCCESS;
   
      return_size *= 2;
      return_buffer = (char*)realloc(return_buffer, return_size);

      assert(return_buffer);

      //printf("new size %d\n", return_size);
   }

   assert(!"Cannot happen!"); // does not return
   return 0;
}

/*------------------------------------------------------------------*/

void rsputs(const char *str)
{
   size_t len = strlen(str);

   return_grow(len);

   if (strlen_retbuf + len > return_size-40) {
      strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
      strlen_retbuf = strlen(return_buffer);
   } else {
      strcpy(return_buffer + strlen_retbuf, str);
      strlen_retbuf += len;
   }
}

/*------------------------------------------------------------------*/

void rsputs2(const char *str)
{
   size_t len = strlen(str);

   return_grow(len);

   if (strlen_retbuf + len > return_size) {
      strlcpy(return_buffer, "<H1>Error: return buffer too small</H1>", return_size);
      strlen_retbuf = strlen(return_buffer);
   } else {
      int j = strlen_retbuf;
      for (size_t i = 0; i < len; i++) {
         if (strncmp(str + i, "http://", 7) == 0) {
            int k;
            char link[256];
            char* p = (char *) (str + i + 7);

            i += 7;
            for (k = 0; *p && *p != ' ' && *p != '\n'; k++, i++)
               link[k] = *p++;
            link[k] = 0;

            sprintf(return_buffer + j, "<a href=\"http://%s\">http://%s</a>", link, link);
            j += strlen(return_buffer + j);
         } else
            switch (str[i]) {
            case '<':
               strlcat(return_buffer, "&lt;", return_size);
               j += 4;
               break;
            case '>':
               strlcat(return_buffer, "&gt;", return_size);
               j += 4;
               break;
            default:
               return_buffer[j++] = str[i];
            }
      }

      return_buffer[j] = 0;
      strlen_retbuf = j;
   }
}

/*------------------------------------------------------------------*/

void rsprintf(const char *format, ...)
{
   va_list argptr;
   char str[10000];

   va_start(argptr, format);
   vsprintf(str, (char *) format, argptr);
   va_end(argptr);

   // catch array overrun. better too late than never...
   assert(strlen(str) < sizeof(str));

   return_grow(strlen(str));

   if (strlen_retbuf + strlen(str) > return_size)
      strcpy(return_buffer, "<H1>Error: return buffer too small</H1>");
   else
      strcpy(return_buffer + strlen_retbuf, str);

   strlen_retbuf += strlen(str);
}

/*------------------------------------------------------------------*/

/* Parameter handling functions similar to setenv/getenv */

void initparam()
{
   memset(_param, 0, sizeof(_param));
   memset(_value, 0, sizeof(_value));
   _text[0] = 0;
}

void setparam(const char *param, const char *value)
{
   int i;

   if (equal_ustring(param, "text")) {
      if (strlen(value) >= TEXT_SIZE)
         printf("Error: parameter value too big\n");

      strlcpy(_text, value, TEXT_SIZE);
      _text[TEXT_SIZE - 1] = 0;
      return;
   }

   for (i = 0; i < MAX_PARAM; i++)
      if (_param[i][0] == 0)
         break;

   if (i < MAX_PARAM) {
      strlcpy(_param[i], param, PARAM_LENGTH);

      int size = strlen(value)+1;
      _value[i] = (char*)malloc(size);
      strlcpy(_value[i], value, size);
      _value[i][strlen(value)] = 0;

   } else {
      printf("Error: parameter array too small\n");
   }
}

void freeparam()
{
   int i;

   for (i=0 ; i<MAX_PARAM ; i++)
      if (_value[i] != NULL) {
         free(_value[i]);
         _value[i] = NULL;
      }
}

const char *getparam(const char *param)
{
   int i;

   if (equal_ustring(param, "text"))
      return _text;

   for (i = 0; i < MAX_PARAM && _param[i][0]; i++)
      if (equal_ustring(param, _param[i]))
         break;

   if (i == MAX_PARAM)
      return NULL;

   if (_value[i] == NULL)
      return "";
      
   return _value[i];
}

BOOL isparam(const char *param)
{
   int i;

   for (i = 0; i < MAX_PARAM && _param[i][0]; i++)
      if (equal_ustring(param, _param[i]))
         break;

   if (i < MAX_PARAM && _param[i][0])
      return TRUE;

   return FALSE;
}

void unsetparam(const char *param)
{
   int i;

   for (i = 0; i < MAX_PARAM; i++)
      if (equal_ustring(param, _param[i]))
         break;

   if (i < MAX_PARAM) {
      _param[i][0] = 0;
      _value[i][0] = 0;
   }
}

/*------------------------------------------------------------------*/

int mhttpd_revision()
{
   return atoi(mhttpd_svn_revision+6);
}

/*------------------------------------------------------------------*/

void urlDecode(char *p)
/********************************************************************\
   Decode the given string in-place by expanding %XX escapes
\********************************************************************/
{
   char *pD, str[3];
   int i;

   pD = p;
   while (*p) {
      if (*p == '%') {
         /* Escape: next 2 chars are hex representation of the actual character */
         p++;
         if (isxdigit(p[0]) && isxdigit(p[1])) {
            str[0] = p[0];
            str[1] = p[1];
            str[2] = 0;
            sscanf(p, "%02X", &i);

            *pD++ = (char) i;
            p += 2;
         } else
            *pD++ = '%';
      } else if (*p == '+') {
         /* convert '+' to ' ' */
         *pD++ = ' ';
         p++;
      } else {
         *pD++ = *p++;
      }
   }
   *pD = '\0';
}

void urlEncode(char *ps, int ps_size)
/********************************************************************\
   Encode the given string in-place by adding %XX escapes
\********************************************************************/
{
   char *pd, *p, str[256];

   pd = str;
   p = ps;
   while (*p) {
      if (strchr(" %&=+#\"'", *p)) {
         sprintf(pd, "%%%02X", *p);
         pd += 3;
         p++;
      } else {
         *pd++ = *p++;
      }
   }
   *pd = '\0';
   assert(pd - str < (int)sizeof(str));
   strlcpy(ps, str, ps_size);
}

/*------------------------------------------------------------------*/

char message_buffer[256] = "";

INT print_message(const char *message)
{
   strlcpy(message_buffer, message, sizeof(message_buffer));
   return SUCCESS;
}

void receive_message(HNDLE hBuf, HNDLE id, EVENT_HEADER * pheader, void *message)
{
   time_t tm;
   char str[80], line[256];

   /* prepare time */
   time(&tm);
   strcpy(str, ctime(&tm));
   str[19] = 0;

   /* print message text which comes after event header */
   strlcpy(line, str + 11, sizeof(line));
   strlcat(line, (char *) message, sizeof(line));
   print_message(line);
}

/*-------------------------------------------------------------------*/

INT sendmail(const char *smtp_host, const char *from, const char *to, const char *subject, const char *text)
{
   struct sockaddr_in bind_addr;
   struct hostent *phe;
   int i, s, strsize, offset;
   char *str, buf[256];
   time_t now;
   struct tm *ts;

   if (verbose)
      printf("\n\nEmail from %s to %s, SMTP host %s:\n", from, to, smtp_host);

   /* create a new socket for connecting to remote server */
   s = socket(AF_INET, SOCK_STREAM, 0);
   if (s == -1)
      return -1;

   /* connect to remote node port 25 */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_port = htons((short) 25);

   phe = gethostbyname(smtp_host);
   if (phe == NULL)
      return -1;
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);

   if (connect(s, (const sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
      closesocket(s);
      return -1;
   }

   strsize = TEXT_SIZE + 1000;
   str = (char*)malloc(strsize);

   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   /* drain server messages */
   do {
      str[0] = 0;
      recv_string(s, str, strsize, 300);
      if (verbose)
         puts(str);
   } while (str[0]);

   sprintf(str, "HELO %s\r\n", host_name);
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);
   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   if (strchr(from, '<')) {
      strlcpy(buf, strchr(from, '<') + 1, sizeof(buf));
      if (strchr(buf, '>'))
         *strchr(buf, '>') = 0;
   } else
      strlcpy(buf, from, sizeof(buf));

   sprintf(str, "MAIL FROM: %s\n", buf);
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);
   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   sprintf(str, "RCPT TO: <%s>\r\n", to);
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);
   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   sprintf(str, "DATA\r\n");
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);
   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   sprintf(str, "To: %s\r\nFrom: %s\r\nSubject: %s\r\n", to, from, subject);
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);

   sprintf(str, "X-Mailer: mhttpd revision %d\r\n", mhttpd_revision());
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);

   time(&now);
   ts = localtime(&now);
   strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S", ts);
   offset = (-(int) timezone);
   if (ts->tm_isdst)
      offset += 3600;
   sprintf(str, "Date: %s %+03d%02d\r\n", buf, (int) (offset / 3600),
           (int) ((abs((int) offset) / 60) % 60));
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);

   sprintf(str, "Content-Type: TEXT/PLAIN; charset=US-ASCII\r\n\r\n");
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);

   /* analyze text for "." at beginning of line */
   const char* p = text;
   str[0] = 0;
   while (strstr(p, "\r\n.\r\n")) {
      i = (POINTER_T) strstr(p, "\r\n.\r\n") - (POINTER_T) p + 1;
      strlcat(str, p, i);
      p += i + 4;
      strlcat(str, "\r\n..\r\n", strsize);
   }
   strlcat(str, p, strsize);
   strlcat(str, "\r\n", strsize);
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);

   /* send ".<CR>" to signal end of message */
   sprintf(str, ".\r\n");
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);
   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   sprintf(str, "QUIT\n");
   send(s, str, strlen(str), 0);
   if (verbose)
      puts(str);
   recv_string(s, str, strsize, 3000);
   if (verbose)
      puts(str);

   closesocket(s);
   free(str);

   return 1;
}

/*------------------------------------------------------------------*/

void redirect(const char *path)
{
   char str[256];

   //printf("redirect to [%s]\n", path);

   strlcpy(str, path, sizeof(str));
   if (str[0] == 0)
      strcpy(str, "./");

   /* redirect */
   rsprintf("HTTP/1.0 302 Found\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n");

   if (strncmp(path, "http:", 5) == 0)
      rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
   else if (strncmp(path, "https:", 6) == 0)
      rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
   else {
      rsprintf("Location: %s\r\n\r\n<html>redir</html>\r\n", str);
   }
}

void redirect2(const char *path)
{
   redirect(path);
   send_tcp(_sock, return_buffer, strlen(return_buffer) + 1, 0x10000);
   closesocket(_sock);
   return_length = -1;
}

/*------------------------------------------------------------------*/

INT search_callback(HNDLE hDB, HNDLE hKey, KEY * key, INT level, void *info)
{
   INT i, size, status;
   char *search_name, *p;
   static char data_str[MAX_ODB_PATH];
   static char str1[MAX_ODB_PATH], str2[MAX_ODB_PATH], ref[MAX_ODB_PATH];
   char path[MAX_ODB_PATH], data[10000];

   search_name = (char *) info;

   /* convert strings to uppercase */
   for (i = 0; key->name[i]; i++)
      str1[i] = toupper(key->name[i]);
   str1[i] = 0;
   for (i = 0; key->name[i]; i++)
      str2[i] = toupper(search_name[i]);
   str2[i] = 0;

   if (strstr(str1, str2) != NULL) {
      db_get_path(hDB, hKey, str1, MAX_ODB_PATH);
      strlcpy(path, str1 + 1, sizeof(path));    /* strip leading '/' */
      strlcpy(str1, path, sizeof(str1));
      urlEncode(str1, sizeof(str1));

      if (key->type == TID_KEY || key->type == TID_LINK) {
         /* for keys, don't display data value */
         rsprintf("<tr><td bgcolor=#FFD000><a href=\"%s\">%s</a></tr>\n", str1, path);
      } else {
         /* strip variable name from path */
         p = path + strlen(path) - 1;
         while (*p && *p != '/')
            *p-- = 0;
         if (*p == '/')
            *p = 0;

         /* display single value */
         if (key->num_values == 1) {
            size = sizeof(data);
            status = db_get_data(hDB, hKey, data, &size, key->type);
            if (status == DB_NO_ACCESS)
               strcpy(data_str, "<no read access>");
            else
               db_sprintf(data_str, data, key->item_size, 0, key->type);

            sprintf(ref, "%s?cmd=Set", str1);

            rsprintf("<tr><td bgcolor=#FFFF00>");

            rsprintf("<a href=\"%s\">%s</a>/%s", path, path, key->name);

            rsprintf("<td><a href=\"%s\">%s</a></tr>\n", ref, data_str);
         } else {
            /* display first value */
            rsprintf("<tr><td rowspan=%d bgcolor=#FFFF00>%s\n", key->num_values, path);

            for (i = 0; i < key->num_values; i++) {
               size = sizeof(data);
               db_get_data(hDB, hKey, data, &size, key->type);
               db_sprintf(data_str, data, key->item_size, i, key->type);

               sprintf(ref, "%s?cmd=Set&index=%d", str1, i);

               if (i > 0)
                  rsprintf("<tr>");

               rsprintf("<td><a href=\"%s\">[%d] %s</a></tr>\n", ref, i, data_str);
            }
         }
      }
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void show_help_page()
{
   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<title>MIDAS WWW Gateway Help</title>\n");
   rsprintf("</head>\n\n");

   rsprintf("<body>\n");
   rsprintf("<h1>Using the MIDAS WWW Gateway</h1>\n");
   rsprintf("With the MIDAS WWW Gateway basic experiment control can be achieved.\n");
   rsprintf("The status page displays the current run status including front-end\n");
   rsprintf("and logger statistics. The Start and Stop buttons can start and stop\n");
   rsprintf("a run. The ODB button switches into the Online Database mode, where\n");
   rsprintf
       ("the contents of the experiment database can be displayed and modified.<P>\n\n");

   rsprintf("For more information, refer to the\n");
   rsprintf("<A HREF=\"http://midas.psi.ch/htmldoc/index.html\">PSI MIDAS manual</A>,\n");
   rsprintf
       ("<A HREF=\"http://ladd00.triumf.ca/~daqweb/doc/midas/html/\">Triumf MIDAS manual</A>.<P>\n\n");

   rsprintf("<hr>\n");
   rsprintf("<address>\n");
   rsprintf("<a href=\"http://pibeta.psi.ch/~stefan\">S. Ritt</a>, 26 Sep 2000");
   rsprintf("</address>");

   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_header(HNDLE hDB, const char *title, const char *method, const char *path, int colspan,
                 int refresh)
{
   time_t now;
   char str[256];
   int size;

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Pragma: no-cache\r\n");
   rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n");
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
   rsprintf("<html><head>\n");

   /* style sheet */
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");

   /* auto refresh */
   if (refresh > 0)
      rsprintf("<meta http-equiv=\"Refresh\" content=\"%02d\">\n", refresh);

   rsprintf("<title>%s</title></head>\n", title);

   strlcpy(str, path, sizeof(str));
   if (str[0] == 0)
      strcpy(str, "./");

   urlEncode(str, sizeof(str));

   if (equal_ustring(method, "POST"))
      rsprintf
          ("<body><form name=\"form1\" method=\"POST\" action=\"%s\" enctype=\"multipart/form-data\">\n\n",
           str);
   else
      rsprintf("<body><form name=\"form1\" method=\"%s\" action=\"%s\">\n\n", method,
               str);

   /* title row */

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);

   rsprintf("<table border=3 cellpadding=2>\n");
   rsprintf("<tr><th colspan=%d bgcolor=\"#A0A0FF\">MIDAS experiment \"%s\"", colspan,
            str);

   if (refresh > 0)
      rsprintf("<th colspan=%d bgcolor=\"#A0A0FF\">%s &nbsp;&nbsp;Refr:%d</tr>\n",
               colspan, ctime(&now), refresh);
   else
      rsprintf("<th colspan=%d bgcolor=\"#A0A0FF\">%s</tr>\n", colspan, ctime(&now));
}

/*------------------------------------------------------------------*/

void show_text_header()
{
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Pragma: no-cache\r\n");
   rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n");
   rsprintf("Content-Type: text/plain; charset=iso-8859-1\r\n\r\n");
}

/*------------------------------------------------------------------*/

void show_error(const char *error)
{
   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS error</title></head>\n");
   rsprintf("<body><H1>%s</H1></body></html>\n", error);
}

/*------------------------------------------------------------------*/

void exec_script(HNDLE hkey)
/********************************************************************\

  Routine: exec_script

  Purpose: Execute script from /Script tree

  exec_script is enabled by the tree /Script
  The /Script struct is composed of list of keys
  from which the name of the key is the button name
  and the sub-structure is a record as follow:

  /Script/<button_name> = <script command> (TID_STRING)

  The "Script command", containing possible arguements,
  is directly executed.

  /Script/<button_name>/<script command>
                        <soft link1>|<arg1>
                        <soft link2>|<arg2>
                           ...

  The arguments for the script are derived from the
  subtree below <button_name>, where <button_name> must be
  TID_KEY. The subtree may then contain arguments or links
  to other values in the ODB, like run number etc.

\********************************************************************/
{
   INT i, size;
   KEY key;
   HNDLE hDB, hsubkey;
   char command[256];
   char data[1000], str[256];

   cm_get_experiment_database(&hDB, NULL);
   db_get_key(hDB, hkey, &key);
   command[0] = 0;

   if (key.type == TID_STRING) {
      size = sizeof(command);
      db_get_data(hDB, hkey, command, &size, TID_STRING);
   } else
      for (i = 0;; i++) {
         db_enum_key(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;
         db_get_key(hDB, hsubkey, &key);

         if (key.type != TID_KEY) {
            size = sizeof(data);
            db_get_data(hDB, hsubkey, data, &size, key.type);
            db_sprintf(str, data, key.item_size, 0, key.type);

            if (i > 0)
               strlcat(command, " ", sizeof(command));

            strlcat(command, str, sizeof(command));
         }
      }

   /* printf("exec_script: %s\n", command); */

   if (command[0])
      ss_system(command);

   return;
}

/*------------------------------------------------------------------*/

int requested_transition = 0;
int requested_old_state = 0;

void show_status_page(int refresh, const char *cookie_wpwd)
{
   int i, j, k, h, m, s, status, size, type, n_alarm, n_items;
   BOOL flag, first;
   char str[1000], msg[256], name[32], ref[256], bgcol[32], fgcol[32], alarm_class[32],
      value_str[256], status_data[256], *p;
   const char *trans_name[] = { "Start", "Stop", "Pause", "Resume" };
   time_t now;
   DWORD difftime;
   double d, value, compression_ratio;
   HNDLE hDB, hkey, hLKey, hsubkey, hkeytmp;
   KEY key;
   int  ftp_mode, previous_mode;
   char client_name[NAME_LENGTH];
   struct tm *gmt;
   BOOL new_window;

   RUNINFO_STR(runinfo_str);
   RUNINFO runinfo;
   EQUIPMENT_INFO equipment;
   EQUIPMENT_STATS equipment_stats;
   CHN_SETTINGS chn_settings;
   CHN_STATISTICS chn_stats;

   cm_get_experiment_database(&hDB, NULL);

   status = db_check_record(hDB, 0, "/Runinfo", strcomb(runinfo_str), FALSE);
   if (status == DB_STRUCT_MISMATCH) {
      cm_msg(MERROR, "show_status_page", "Aborting on mismatching /Runinfo structure");
      cm_disconnect_experiment();
      abort();
   }

   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "show_status_page",
             "Aborting on invalid access to ODB /Runinfo, status=%d", status);
      cm_disconnect_experiment();
      abort();
   }

   db_find_key(hDB, 0, "/Runinfo", &hkey);
   assert(hkey);

   size = sizeof(runinfo);
   status = db_get_record(hDB, hkey, &runinfo, &size, 0);
   assert(status == DB_SUCCESS);

   /* count alarms */
   db_find_key(hDB, 0, "/Alarms/Alarms", &hkey);
   n_alarm = 0;
   if (hkey) {
      /* check global alarm flag */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Alarms/Alarm System active", &flag, &size, TID_BOOL, TRUE);
      if (flag) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkey, i, &hsubkey);

            if (!hsubkey)
               break;

            size = sizeof(flag);
            db_get_value(hDB, hsubkey, "Triggered", &flag, &size, TID_INT, TRUE);
            if (flag)
               n_alarm = 1;
         }
      }
   }

   /* header */
   rsprintf("HTTP/1.1 200 OK\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n");
   rsprintf("Pragma: no-cache\r\n");
   rsprintf("Expires: Fri, 01-Jan-1983 00:00:00 GMT\r\n");
   if (cookie_wpwd[0]) {
      time(&now);
      now += 3600 * 24;
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%Y %H:%M:%S GMT", gmt);

      rsprintf("Set-Cookie: midas_wpwd=%s; path=/; expires=%s\r\n", cookie_wpwd, str);
   }

   rsprintf("\r\n<html>\n");

   /* auto refresh */
   if (refresh > 0)
      rsprintf("<head><meta http-equiv=\"Refresh\" content=\"%02d\">\n", refresh);

   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);

   rsprintf("<title>%s status</title>\n", str);
   
   if (n_alarm) {
      rsprintf("<bgsound src=\"alarm.mid\" loop=\"false\">\n");
   }
   
   rsprintf("</head>\n");

   rsprintf("<body><form method=\"GET\" action=\".\">\n");

   if (n_alarm) {
      rsprintf("<embed src=\"alarm.mid\" autostart=\"true\" loop=\"false\" hidden=\"true\" height=\"0\" width=\"0\">\n");
   }

   rsprintf("<table border=3 cellpadding=2>\n");

   /*---- title row ----*/

   rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
   rsprintf("<th colspan=3 bgcolor=#A0A0FF>%s &nbsp;&nbsp;Refr:%d</tr>\n", ctime(&now),
            refresh);

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

#ifdef HAVE_MSCB
   strlcpy(str, "Start, Pause, ODB, Messages, ELog, Alarms, Programs, History, MSCB, Sequencer, Config, Help", sizeof(str));
#else
   strlcpy(str, "Start, Pause, ODB, Messages, ELog, Alarms, Programs, History, Sequencer, Config, Help", sizeof(str));
#endif
   size = sizeof(str);
   db_get_value(hDB, 0, "/Experiment/Menu Buttons", str, &size, TID_STRING, TRUE);

   p = strtok(str, ",");

   while (p) {

      while (*p == ' ')
         p++;
      strlcpy(str, p, sizeof(str));
      while (str[strlen(str)-1] == ' ')
         str[strlen(str)-1] = 0;

      if (stricmp(str, "Start") == 0) {
         if (runinfo.state == STATE_STOPPED)
            rsprintf("<input type=submit name=cmd %s value=Start>\n", runinfo.transition_in_progress?"disabled":"");
         else {
            rsprintf("<noscript>\n");
            if (runinfo.state == STATE_PAUSED || runinfo.state == STATE_RUNNING)
               rsprintf("<input type=submit name=cmd %s value=Stop>\n", runinfo.transition_in_progress?"disabled":"");
            rsprintf("</noscript>\n");
            rsprintf("<script type=\"text/javascript\">\n");
            rsprintf("<!--\n");
            rsprintf("function stop()\n");
            rsprintf("{\n");
            rsprintf("   flag = confirm('Are you sure to stop the run?');\n");
            rsprintf("   if (flag == true)\n");
            rsprintf("      window.location.href = '?cmd=Stop';\n");
            rsprintf("}\n");
            if (runinfo.state == STATE_PAUSED || runinfo.state == STATE_RUNNING)
               rsprintf("document.write('<input type=button %s value=Stop onClick=\"stop();\">\\n');\n", runinfo.transition_in_progress?"disabled":"");
            rsprintf("//-->\n");
            rsprintf("</script>\n");
         }
      } else if (stricmp(str, "Pause") == 0) {
         if (runinfo.state != STATE_STOPPED) {
            rsprintf("<noscript>\n");
            if (runinfo.state == STATE_RUNNING)
               rsprintf("<input type=submit name=cmd %s value=Pause>\n", runinfo.transition_in_progress?"disabled":"");
            rsprintf("</noscript>\n");
            rsprintf("<script type=\"text/javascript\">\n");
            rsprintf("<!--\n");
            rsprintf("function pause()\n");
            rsprintf("{\n");
            rsprintf("   flag = confirm('Are you sure to pause the run?');\n");
            rsprintf("   if (flag == true)\n");
            rsprintf("      window.location.href = '?cmd=Pause';\n");
            rsprintf("}\n");
            if (runinfo.state == STATE_RUNNING)
               rsprintf("document.write('<input type=button %s value=Pause onClick=\"pause();\"\\n>');\n", runinfo.transition_in_progress?"disabled":"");
            rsprintf("//-->\n");
            rsprintf("</script>\n");
            if (runinfo.state == STATE_PAUSED)
               rsprintf("<input type=submit name=cmd %s value=Resume>\n", runinfo.transition_in_progress?"disabled":"");
         }
      } else
         rsprintf("<input type=submit name=cmd value=\"%s\">\n", str);

      p = strtok(NULL, ",");
   }

   /*---- script buttons ----*/

   status = db_find_key(hDB, 0, "Script", &hkey);
   if (status == DB_SUCCESS) {
      rsprintf("<tr><td colspan=6 bgcolor=#E0FFFF>\n");

      for (i = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;
         db_get_key(hDB, hsubkey, &key);
         rsprintf("<input type=submit name=script value=\"%s\">\n", key.name);
      }
   }

   rsprintf("</tr>\n\n");

   /*---- manual triggered equipment ----*/

   if (db_find_key(hDB, 0, "/equipment", &hkey) == DB_SUCCESS) {
      first = TRUE;
      for (i = 0;; i++) {
         db_enum_key(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         db_get_key(hDB, hsubkey, &key);

         db_find_key(hDB, hsubkey, "Common", &hkeytmp);

         if (hkeytmp) {
            size = sizeof(type);
            db_get_value(hDB, hkeytmp, "Type", &type, &size, TID_INT, TRUE);
            if (type & EQ_MANUAL_TRIG) {
               if (first)
                  rsprintf("<tr><td colspan=6 bgcolor=#E0FFE0>\n");

               first = FALSE;

               sprintf(str, "Trigger %s event", key.name);
               rsprintf("<input type=submit name=cmd value=\"%s\">\n", str);
            }
         }
      }
      if (!first)
         rsprintf("</tr>\n\n");
   }

   /*---- aliases ----*/

   first = TRUE;

   db_find_key(hDB, 0, "/Alias", &hkey);
   if (hkey) {
      if (first) {
         rsprintf("<tr><td colspan=6 bgcolor=#E0E0FF>\n");
         first = FALSE;
      }
      for (i = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         db_get_key(hDB, hsubkey, &key);

         strlcpy(name, key.name, sizeof(name));
         new_window = (name[strlen(name) - 1] != '&');
         if (!new_window)
            name[strlen(name) - 1] = 0;

         if (key.type == TID_STRING) {
            /* html link */
            size = sizeof(ref);
            db_get_data(hDB, hsubkey, ref, &size, TID_STRING);
            if (new_window)
               rsprintf("<button type=\"button\" onclick=\"window.open('%s');\">%s</button>\n", ref, name);
            else
               rsprintf("<button type=\"button\" onclick=\"document.location.href='%s';\">%s</button>\n", ref, name);
         } else if (key.type == TID_LINK) {
            /* odb link */
            sprintf(ref, "./Alias/%s", key.name);

            if (new_window)
               rsprintf("<button type=\"button\" onclick=\"window.open('%s');\">%s</button>\n", ref, name);
            else
               rsprintf("<button type=\"button\" onclick=\"document.location.href='%s';\">%s</button>\n", ref, name);
         }
      }
   }

   /*---- custom pages ----*/

   db_find_key(hDB, 0, "/Custom", &hkey);
   if (hkey) {
      for (i = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         db_get_key(hDB, hsubkey, &key);

         /* skip "Images" */
         if (key.type != TID_STRING)
            continue;

         /* skip "Path" */
         if (equal_ustring(key.name, "Path"))
            continue;

         strlcpy(name, key.name, sizeof(name));

         /* check if hidden page */
         if (name[strlen(name) - 1] == '!')
            continue;

         if (first) {
            rsprintf("<tr><td colspan=6 bgcolor=#E0E0FF>\n");
            first = FALSE;
         }

         new_window = (name[strlen(name) - 1] != '&');
         if (!new_window)
            name[strlen(name) - 1] = 0;

         sprintf(ref, "./CS/%s", name);

         if (new_window)
            rsprintf("<button type=\"button\" onclick=\"window.open('%s');\">%s</button>\n", ref, name);
         else
            rsprintf("<button type=\"button\" onclick=\"document.location.href='%s';\">%s</button>\n", ref, name);
       }
   }

   /*---- alarms ----*/

   /* go through all triggered alarms */
   db_find_key(hDB, 0, "/Alarms/Alarms", &hkey);
   if (hkey) {
      /* check global alarm flag */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Alarms/Alarm System active", &flag, &size, TID_BOOL, TRUE);
      if (flag) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkey, i, &hsubkey);

            if (!hsubkey)
               break;

            size = sizeof(flag);
            db_get_value(hDB, hsubkey, "Triggered", &flag, &size, TID_INT, TRUE);
            if (flag) {
               size = sizeof(alarm_class);
               db_get_value(hDB, hsubkey, "Alarm Class", alarm_class, &size, TID_STRING,
                            TRUE);

               strcpy(bgcol, "red");
               sprintf(str, "/Alarms/Classes/%s/Display BGColor", alarm_class);
               size = sizeof(bgcol);
               db_get_value(hDB, 0, str, bgcol, &size, TID_STRING, TRUE);

               strcpy(fgcol, "black");
               sprintf(str, "/Alarms/Classes/%s/Display FGColor", alarm_class);
               size = sizeof(fgcol);
               db_get_value(hDB, 0, str, fgcol, &size, TID_STRING, TRUE);

               size = sizeof(msg);
               db_get_value(hDB, hsubkey, "Alarm Message", msg, &size, TID_STRING, TRUE);

               size = sizeof(j);
               db_get_value(hDB, hsubkey, "Type", &j, &size, TID_INT, TRUE);

               if (j == AT_EVALUATED) {
                  size = sizeof(str);
                  db_get_value(hDB, hsubkey, "Condition", str, &size, TID_STRING, TRUE);

                  /* retrieve value */
                  al_evaluate_condition(str, value_str);
                  sprintf(str, msg, value_str);
               } else
                  strlcpy(str, msg, sizeof(str));


               db_get_key(hDB, hsubkey, &key);
               rsprintf("<tr><td colspan=6 bgcolor=\"%s\" align=center>", bgcol);
               rsprintf("<table width=\"100%%\"><tr><td align=center width=\"99%%\"><font color=\"%s\" size=+3>%s: %s</font></td>\n", fgcol,
                        alarm_class, str);
               rsprintf("<td width=\"1%%\"><button type=\"button\" onclick=\"document.location.href='?cmd=alrst&name=%s'\"n\">Reset</button></td></tr></table></td></tr>\n", key.name);
            }
         }
      }
   }

   /*---- run info ----*/

   rsprintf("<tr align=center><td>Run #%d", runinfo.run_number);

   if (runinfo.state == STATE_STOPPED)
      rsprintf("<td colspan=1 bgcolor=#FF0000>Stopped");
   else if (runinfo.state == STATE_PAUSED)
      rsprintf("<td colspan=1 bgcolor=#FFFF00>Paused");
   else if (runinfo.state == STATE_RUNNING)
      rsprintf("<td colspan=1 bgcolor=#00FF00>Running");
   else
      rsprintf("<td colspan=1 bgcolor=#FFFFFF>Unknown");

   if (runinfo.transition_in_progress)
      requested_transition = 0;

   if (runinfo.state != requested_old_state)
      requested_transition = 0;

   if (requested_transition == TR_STOP)
      rsprintf("<br><b>Run stop requested</b>");

   if (requested_transition == TR_START)
      rsprintf("<br><b>Run start requested</b>");

   if (requested_transition == TR_PAUSE)
      rsprintf("<br><b>Run pause requested</b>");

   if (requested_transition == TR_RESUME)
      rsprintf("<br><b>Run resume requested</b>");

   if (runinfo.transition_in_progress == TR_STOP)
      rsprintf("<br><b>Stopping run</b>");

   if (runinfo.transition_in_progress == TR_START)
      rsprintf("<br><b>Starting run</b>");

   if (runinfo.transition_in_progress == TR_PAUSE)
      rsprintf("<br><b>Pausing run</b>");

   if (runinfo.transition_in_progress == TR_RESUME)
      rsprintf("<br><b>Resuming run</b>");

   if (runinfo.requested_transition)
      for (i = 0; i < 4; i++)
         if (runinfo.requested_transition & (1 << i))
            rsprintf("<br><b>%s requested</b>", trans_name[i]);

   sprintf(ref, "Alarms/Alarm system active?cmd=set");

   size = sizeof(flag);
   db_get_value(hDB, 0, "Alarms/Alarm system active", &flag, &size, TID_BOOL, TRUE);
   strlcpy(str, flag ? "00FF00" : "FFC0C0", sizeof(str));
   rsprintf("<td bgcolor=#%s><a href=\"%s\">Alarms: %s</a>", str, ref,
            flag ? "On" : "Off");

   sprintf(ref, "Logger/Auto restart?cmd=set");

   size = sizeof(flag);
   db_get_value(hDB, 0, "/Sequencer/State/Running", &flag, &size, TID_BOOL, FALSE);
   if (flag)
      rsprintf("<td bgcolor=#00FF00>Restart: Sequencer");
   else if (cm_exist("RunSubmit", FALSE) == CM_SUCCESS)
      rsprintf("<td bgcolor=#00FF00>Restart: RunSubmit");
   else {
     size = sizeof(flag);
     db_get_value(hDB, 0, "Logger/Auto restart", &flag, &size, TID_BOOL, TRUE);
     strlcpy(str, flag ? "00FF00" : "FFFF00", sizeof(str));
     rsprintf("<td bgcolor=#%s><a href=\"%s\">Restart: %s</a>", str, ref,
              flag ? "Yes" : "No");
   }

   if (cm_exist("Logger", FALSE) != CM_SUCCESS && cm_exist("FAL", FALSE) != CM_SUCCESS)
      rsprintf("<td colspan=2 bgcolor=#FF0000>Logger not running</tr>\n");
   else {
      /* write data flag */
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Logger/Write data", &flag, &size, TID_BOOL, TRUE);

      if (!flag)
         rsprintf("<td colspan=2 bgcolor=#FFFF00>Logging disabled</tr>\n");
      else {
         size = sizeof(str);
         db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING, TRUE);

         rsprintf("<td colspan=3>Data dir: %s</tr>\n", str);
      }
   }

   /*---- time ----*/

   rsprintf("<tr align=center><td colspan=3>Start: %s", runinfo.start_time);

   difftime = (DWORD) (now - runinfo.start_time_binary);
   h = difftime / 3600;
   m = difftime % 3600 / 60;
   s = difftime % 60;

   if (runinfo.state == STATE_STOPPED)
      rsprintf("<td colspan=3>Stop: %s</tr>\n", runinfo.stop_time);
   else
      rsprintf("<td colspan=3>Running time: %dh%02dm%02ds</tr>\n", h, m, s);

   /*---- run comment ----*/ 
 
   size = sizeof(str); 
   if (db_get_value(hDB, 0, "/Experiment/Run parameters/Comment", str, 
                    &size, TID_STRING, FALSE) == DB_SUCCESS) 
      rsprintf("<tr align=center><td colspan=6 bgcolor=#E0E0FF><b>%s</b></td></tr>\n", 
               str); 
   size = sizeof(str); 
   if (db_get_value(hDB, 0, "/Experiment/Run parameters/Run Description", str, 
                    &size, TID_STRING, FALSE) == DB_SUCCESS) 
      rsprintf("<tr align=center><td colspan=6 bgcolor=#E0E0FF><b>%s</b></td></tr>\n", 
               str); 
 
   /*---- Status items ----*/

   n_items = 0;
   if (db_find_key(hDB, 0, "/Experiment/Status items", &hkey) == DB_SUCCESS) {
      for (i = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         if (n_items++ == 0)
            rsprintf("<tr><td colspan=6><table width=100%%>\n");

         db_get_key(hDB, hsubkey, &key);
         rsprintf("<tr><td align=right width=30%% bgcolor=\"#E0E0FF\">%s:</td>", key.name);
         
         db_enum_key(hDB, hkey, i, &hsubkey);
         db_get_key(hDB, hsubkey, &key);
         size = sizeof(status_data);
         if (db_get_data(hDB, hsubkey, status_data, &size, key.type) == DB_SUCCESS) {
            db_sprintf(str, status_data, key.item_size, 0, key.type);
            rsprintf("<td >%s</td></tr>\n", str);
         }
      }
      if (n_items)
         rsprintf("</table></td></tr>\n");
   }

   /*---- Equipment list ----*/

   rsprintf("<tr><td colspan=6><table border=1 width=100%%>\n");

   rsprintf("<tr><th>Equipment<th>Status<th>Events");
   rsprintf("<th>Events[/s]<th>Data[MB/s]\n");

   if (db_find_key(hDB, 0, "/equipment", &hkey) == DB_SUCCESS) {
      for (i = 0;; i++) {
         db_enum_key(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         db_get_key(hDB, hsubkey, &key);

         memset(&equipment, 0, sizeof(equipment));
         memset(&equipment_stats, 0, sizeof(equipment_stats));

         db_find_key(hDB, hsubkey, "Common", &hkeytmp);

         if (hkeytmp) {
            db_get_record_size(hDB, hkeytmp, 0, &size);
            /* discard wrong equipments (caused by analyzer) */
            if (size <= (int)sizeof(equipment))
               db_get_record(hDB, hkeytmp, &equipment, &size, 0);
         }

         db_find_key(hDB, hsubkey, "Statistics", &hkeytmp);

         if (hkeytmp) {
            db_get_record_size(hDB, hkeytmp, 0, &size);
            if (size == sizeof(equipment_stats))
               db_get_record(hDB, hkeytmp, &equipment_stats, &size, 0);
         }

         sprintf(ref, "SC/%s", key.name);

         /* check if client running this equipment is present */
         if (cm_exist(equipment.frontend_name, TRUE) != CM_SUCCESS
             && cm_exist("FAL", TRUE) != CM_SUCCESS)
            rsprintf
                ("<tr><td><a href=\"%s\">%s</a><td align=center bgcolor=#FF0000>(frontend stopped)",
                 ref, key.name);
         else {
            if (equipment.enabled) {
               if (equipment.status[0] == 0)
                  rsprintf("<tr><td><a href=\"%s\">%s</a><td align=center bgcolor=\"%s\">%s@%s", ref, key.name, "#00FF00", equipment.frontend_name, equipment.frontend_host);
               else
                  rsprintf("<tr><td><a href=\"%s\">%s</a><td align=center bgcolor=\"%s\">%s", ref, key.name, equipment.status_color, equipment.status);
            } else
               rsprintf("<tr><td><a href=\"%s\">%s</a><td align=center bgcolor=#FFFF00>(disabled)", ref, key.name);
         }

         /* event statistics */
         d = equipment_stats.events_sent;
         if (d > 1E9)
            sprintf(str, "%1.3lfG", d / 1E9);
         else if (d > 1E6)
            sprintf(str, "%1.3lfM", d / 1E6);
         else
            sprintf(str, "%1.0lf", d);

         rsprintf("<td align=center>%s<td align=center>%1.1lf<td align=center>%1.3lf\n",
                  str, equipment_stats.events_per_sec, equipment_stats.kbytes_per_sec/1024.0);
      }
   }

   rsprintf("</table></td></tr>\n");

   /*---- Logging channels ----*/

   rsprintf
       ("<tr><th colspan=2>Channel<th>Events<th>MB written<th>Compression<th width=\"150px\">Disk level</tr>\n");

   if (db_find_key(hDB, 0, "/Logger/Channels", &hkey) == DB_SUCCESS) {
      for (i = 0;; i++) {
         db_enum_key(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         db_get_key(hDB, hsubkey, &key);

         db_find_key(hDB, hsubkey, "Settings", &hkeytmp);
	 assert(hkeytmp);
         size = sizeof(chn_settings);
         if (db_get_record(hDB, hkeytmp, &chn_settings, &size, 0) != DB_SUCCESS)
            continue;

         db_find_key(hDB, hsubkey, "Statistics", &hkeytmp);
	 assert(hkeytmp);
         size = sizeof(chn_stats);
         if (db_get_record(hDB, hkeytmp, &chn_stats, &size, 0) != DB_SUCCESS)
            continue;

         /* filename */

         strlcpy(str, chn_settings.current_filename, sizeof(str));

         if (equal_ustring(chn_settings.type, "FTP")) {
            char *token, orig[256];

            strlcpy(orig, str, sizeof(orig));

            strlcpy(str, "ftp://", sizeof(str));
            token = strtok(orig, ", ");
            if (token) {
               strlcat(str, token, sizeof(str));
               token = strtok(NULL, ", ");
               token = strtok(NULL, ", ");
               token = strtok(NULL, ", ");
               token = strtok(NULL, ", ");
               if (token) {
                  strlcat(str, "/", sizeof(str));
                  strlcat(str, token, sizeof(str));
                  strlcat(str, "/", sizeof(str));
                  token = strtok(NULL, ", ");
                  strlcat(str, token, sizeof(str));
               }
            }
         }

         sprintf(ref, "Logger/Channels/%s/Settings", key.name);

         if (cm_exist("Logger", FALSE) != CM_SUCCESS
             && cm_exist("FAL", FALSE) != CM_SUCCESS)
            rsprintf("<tr><td colspan=2 bgcolor=\"FF0000\">");
         else if (!flag)
            rsprintf("<tr><td colspan=2 bgcolor=\"FFFF00\">");
         else if (chn_settings.active)
            rsprintf("<tr><td colspan=2 bgcolor=\"00FF00\">");
         else
            rsprintf("<tr><td colspan=2 bgcolor=\"FFFF00\">");

         rsprintf("<B><a href=\"%s\">#%s:</a></B>&nbsp;&nbsp;%s", ref, key.name, str);

         /* statistics */

         if (chn_settings.compression > 0) {
            rsprintf("<td align=center>%1.0lf<td align=center>%1.3lf\n",
                 chn_stats.events_written, chn_stats.bytes_written / 1024 / 1024);

            if (chn_stats.bytes_written_uncompressed > 0)
               compression_ratio = 1 - chn_stats.bytes_written / chn_stats.bytes_written_uncompressed;
            else
               compression_ratio = 0;

            rsprintf("<td align=center>%4.1lf%%", compression_ratio * 100);
         } else {
            rsprintf("<td align=center>%1.0lf<td align=center>%1.3lf\n",
                 chn_stats.events_written, chn_stats.bytes_written_uncompressed / 1024 / 1024);

            rsprintf("<td align=center>N/A</td>");
         }

         char col[80];
         if (chn_stats.disk_level >= 0.9)
            strcpy(col, "red");
         else if (chn_stats.disk_level >= 0.7)
            strcpy(col, "yellow");
         else
            strcpy(col, "#00FF00");
         
         rsprintf("<td style=\"padding:0;\">\n");
         rsprintf("<div style=\"background-color:%s;width:%dpx;height:23px;\">\n", col, (int)(chn_stats.disk_level*150));
         rsprintf("<div style=\"position:relative;top:2px;left:15px\">%1.1lf&nbsp;%%</div>\n", chn_stats.disk_level*100);
         rsprintf("</td></tr>\n");
      }
   }

   /*---- Lazy Logger ----*/

   if (db_find_key(hDB, 0, "/Lazy", &hkey) == DB_SUCCESS) {
      status = db_find_key(hDB, 0, "System/Clients", &hkey);
      if (status != DB_SUCCESS)
         return;

      k = 0;
      previous_mode = -1;
      /* loop over all clients */
      for (j = 0;; j++) {
         status = db_enum_key(hDB, hkey, j, &hsubkey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         if (status == DB_SUCCESS) {
            /* get client name */
            size = sizeof(client_name);
            db_get_value(hDB, hsubkey, "Name", client_name, &size, TID_STRING, TRUE);
            client_name[4] = 0; /* search only for the 4 first char */
            if (equal_ustring(client_name, "Lazy")) {
               sprintf(str, "/Lazy/%s", &client_name[5]);
               status = db_find_key(hDB, 0, str, &hLKey);
               if (status == DB_SUCCESS) {
                  size = sizeof(str);
                  db_get_value(hDB, hLKey, "Settings/Backup Type", str, &size, TID_STRING,
                               TRUE);
                  ftp_mode = equal_ustring(str, "FTP");

                  if (previous_mode != ftp_mode)
                     k = 0;
                  if (k == 0) {
                     if (ftp_mode)
                        rsprintf
                            ("<tr><th colspan=2>Lazy Destination<th>Progress<th>File Name<th>Speed [MB/s]<th>Total</tr>\n");
                     else
                        rsprintf
                            ("<tr><th colspan=2>Lazy Label<th>Progress<th>File Name<th># Files<th>Total</tr>\n");
                  }
                  previous_mode = ftp_mode;
                  if (ftp_mode) {
                     size = sizeof(str);
                     db_get_value(hDB, hLKey, "Settings/Path", str, &size, TID_STRING,
                                  TRUE);
                     if (strchr(str, ','))
                        *strchr(str, ',') = 0;
                  } else {
                     size = sizeof(str);
                     db_get_value(hDB, hLKey, "Settings/List Label", str, &size,
                                  TID_STRING, TRUE);
                     if (str[0] == 0)
                        strcpy(str, "(empty)");
                  }

                  sprintf(ref, "Lazy/%s/Settings", &client_name[5]);

                  rsprintf("<tr><td colspan=2><B><a href=\"%s\">%s</a></B>", ref, str);

                  size = sizeof(value);
                  db_get_value(hDB, hLKey, "Statistics/Copy progress (%)", &value, &size,
                               TID_DOUBLE, TRUE);
                  rsprintf("<td align=center>%1.0f %%", value);

                  size = sizeof(str);
                  db_get_value(hDB, hLKey, "Statistics/Backup File", str, &size,
                               TID_STRING, TRUE);
                  rsprintf("<td align=center>%s", str);

                  if (ftp_mode) {
                     size = sizeof(value);
                     db_get_value(hDB, hLKey, "Statistics/Copy Rate (Bytes per s)",
                                  &value, &size, TID_DOUBLE, TRUE);
                     rsprintf("<td align=center>%1.1f", value / 1024.0 / 1024.0);
                  } else {
                     size = sizeof(i);
                     db_get_value(hDB, hLKey, "/Statistics/Number of files", &i, &size,
                                  TID_INT, TRUE);
                     rsprintf("<td align=center>%d", i);
                  }

                  size = sizeof(value);
                  db_get_value(hDB, hLKey, "Statistics/Backup status (%)", &value, &size,
                               TID_DOUBLE, TRUE);
                  rsprintf("<td align=center>%1.1f %%", value);
                  k++;
               }
            }
         }
      }

      rsprintf("</tr>\n");
   }

   /*---- Messages ----*/

   rsprintf("<tr><td colspan=6>");

   if (message_buffer[0]) {
      if (strstr(message_buffer, ",ERROR]"))
         rsprintf("<span style=\"color:white;background-color:red\"><b>%s</b></span>",
                  message_buffer);
      else
         rsprintf("<b>%s</b>", message_buffer);
   }

   rsprintf("</tr>");

   /*---- Clients ----*/

   if (db_find_key(hDB, 0, "/System/Clients", &hkey) == DB_SUCCESS) {
      for (i = 0;; i++) {
         db_enum_key(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         if (i % 3 == 0)
            rsprintf("<tr bgcolor=#E0E0FF>");

         size = sizeof(name);
         db_get_value(hDB, hsubkey, "Name", name, &size, TID_STRING, TRUE);
         size = sizeof(str);
         db_get_value(hDB, hsubkey, "Host", str, &size, TID_STRING, TRUE);

         rsprintf("<td colspan=2 align=center>%s [%s]", name, str);

         if (i % 3 == 2)
            rsprintf("</tr>\n");
      }

      if (i % 3 != 0)
         rsprintf("</tr>\n");
   }

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_messages_page(int refresh, int n_message)
{
   int size, more;
   char str[256], *buffer, line[256], *pline;
   time_t now;
   HNDLE hDB;
   BOOL eob;

   cm_get_experiment_database(&hDB, NULL);

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");

   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");

   /* auto refresh */
   if (refresh > 0)
      rsprintf("<meta http-equiv=\"Refresh\" content=\"%d\">\n\n", refresh);

   rsprintf("<title>MIDAS messages</title></head>\n");
   rsprintf("<body><form method=\"GET\" action=\".\">\n");

   rsprintf("<table columns=2 border=3 cellpadding=2>\n");

   /*---- title row ----*/

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);

   rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
   rsprintf("<th bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=ODB>\n");
   rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("<input type=submit name=cmd value=Config>\n");
   rsprintf("<input type=submit name=cmd value=Help>\n");
   rsprintf("</tr>\n\n");

   /*---- messages ----*/

   rsprintf("<tr><td colspan=2>\n");

   /* more button */
   if (n_message == 20)
      more = 100;
   else
      more = n_message + 100;

   rsprintf("<input type=submit name=cmd value=More%d><p>\n", more);

   buffer = (char *)malloc(1000000);
   cm_msg_retrieve(n_message, buffer, 1000000);

   pline = buffer;
   eob = FALSE;

   do {
      strlcpy(line, pline, sizeof(line));

      /* extract single line */
      if (strchr(line, '\n'))
         *strchr(line, '\n') = 0;
      if (strchr(line, '\r'))
         *strchr(line, '\r') = 0;

      pline += strlen(line);

      while (*pline == '\r' || *pline == '\n')
         pline++;

      /* check for error */
      if (strstr(line, ",ERROR]"))
         rsprintf("<span style=\"color:white;background-color:red\">%s</span>", line);
      else
         rsprintf("%s", line);

      rsprintf("<br>\n");
   } while (!eob && *pline);

   rsprintf("</tr></table>\n");
   rsprintf("</body></html>\r\n");
   free(buffer);
}

/*------------------------------------------------------------------*/

void strencode(char *text)
{
   int i;

   for (i = 0; i < (int) strlen(text); i++) {
      switch (text[i]) {
      case '\n':
         rsprintf("<br>\n");
         break;
      case '<':
         rsprintf("&lt;");
         break;
      case '>':
         rsprintf("&gt;");
         break;
      case '&':
         rsprintf("&amp;");
         break;
      case '\"':
         rsprintf("&quot;");
         break;
      default:
         rsprintf("%c", text[i]);
      }
   }
}

/*------------------------------------------------------------------*/

void strencode2(char *b, char *text)
{
   int i;

   for (i = 0; i < (int) strlen(text); b++, i++) {
      switch (text[i]) {
      case '\n':
         sprintf(b, "<br>\n");
         break;
      case '<':
         sprintf(b, "&lt;");
         break;
      case '>':
         sprintf(b, "&gt;");
         break;
      case '&':
         sprintf(b, "&amp;");
         break;
      case '\"':
         sprintf(b, "&quot;");
         break;
      default:
         sprintf(b, "%c", text[i]);
      }
   }
   *b = 0;
}

/*------------------------------------------------------------------*/

void strencode3(char *text)
{
   int i;

   for (i = 0; i < (int) strlen(text); i++) {
      switch (text[i]) {
      case '<':
         rsprintf("&lt;");
         break;
      case '>':
         rsprintf("&gt;");
         break;
      case '&':
         rsprintf("&amp;");
         break;
      case '\"':
         rsprintf("&quot;");
         break;
      default:
         rsprintf("%c", text[i]);
      }
   }
}

/*------------------------------------------------------------------*/

void strencode4(char *text)
{
   int i;
   
   for (i = 0; i < (int) strlen(text); i++) {
      switch (text[i]) {
         case '\n':
            rsprintf("<br>\n");
            break;
         case '<':
            rsprintf("&lt;");
            break;
         case '>':
            rsprintf("&gt;");
            break;
         case '&':
            rsprintf("&amp;");
            break;
         case '\"':
            rsprintf("&quot;");
            break;
         case ' ':
            rsprintf("&nbsp;");
            break;
         default:
            rsprintf("%c", text[i]);
      }
   }
}

/*------------------------------------------------------------------*/

void show_elog_new(const char *path, BOOL bedit, const char *odb_att, const char *action_path)
{
   int i, j, size, run_number, wrap, status;
   char str[256], ref[256], *p;
   char date[80], author[80], type[80], system[80], subject[256], text[10000],
       orig_tag[80], reply_tag[80], att1[256], att2[256], att3[256], encoding[80];
   time_t now;
   HNDLE hDB, hkey, hsubkey;
   BOOL display_run_number;
   KEY key;

   //printf("show_elog_new, path [%s], action_path [%s], att [%s]\n", path, action_path, odb_att);

   if (!action_path)
     action_path = "./";

   cm_get_experiment_database(&hDB, NULL);
   display_run_number = TRUE;
   size = sizeof(BOOL);
   db_get_value(hDB, 0, "/Elog/Display run number", &display_run_number, &size, TID_BOOL,
                TRUE);

   /* get message for reply */
   type[0] = system[0] = 0;
   att1[0] = att2[0] = att3[0] = 0;
   subject[0] = 0;
   run_number = 0;

   if (path) {
      strlcpy(str, path, sizeof(str));
      size = sizeof(text);
      el_retrieve(str, date, &run_number, author, type, system, subject,
                  text, &size, orig_tag, reply_tag, att1, att2, att3, encoding);
   }

   if (run_number < 0) {
      cm_msg(MERROR, "show_elog_new", "aborting on attempt to use invalid run number %d",
             run_number);
      abort();
   }

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS ELog</title></head>\n");
   rsprintf
       ("<body><form method=\"POST\" action=\"%s\" enctype=\"multipart/form-data\">\n", action_path);

   rsprintf("<table border=3 cellpadding=5>\n");

  /*---- title row ----*/

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

   rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS Electronic Logbook");
   if (elog_mode)
      rsprintf("<th bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", str);
   else
      rsprintf("<th bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=Submit>\n");
   rsprintf("</tr>\n\n");

  /*---- entry form ----*/

   if (display_run_number) {
      if (bedit) {
         rsprintf("<tr><td bgcolor=#FFFF00>Entry date: %s<br>", date);
         time(&now);
         rsprintf("Revision date: %s", ctime(&now));
      } else {
         time(&now);
         rsprintf("<tr><td bgcolor=#FFFF00>Entry date: %s", ctime(&now));
      }

      if (!bedit) {
         run_number = 0;
         size = sizeof(run_number);
         status =
             db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT,
                          TRUE);
         assert(status == SUCCESS);
      }

      if (run_number < 0) {
         cm_msg(MERROR, "show_elog_new",
                "aborting on attempt to use invalid run number %d", run_number);
         abort();
      }

      rsprintf("<td bgcolor=#FFFF00>Run number: ");
      rsprintf("<input type=\"text\" size=10 maxlength=10 name=\"run\" value=\"%d\"</tr>",
               run_number);
   } else {
      if (bedit) {
         rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: %s<br>", date);
         time(&now);
         rsprintf("Revision date: %s", ctime(&now));
      } else {
         time(&now);
         rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: %s", ctime(&now));
      }
   }

   if (bedit) {
      strlcpy(str, author, sizeof(str));
      if (strchr(str, '@'))
         *strchr(str, '@') = 0;
   } else
      str[0] = 0;

   rsprintf
       ("<tr><td bgcolor=#FFA0A0>Author: <input type=\"text\" size=\"15\" maxlength=\"80\" name=\"Author\" value=\"%s\">\n",
        str);

   /* get type list from ODB */
   size = 20 * NAME_LENGTH;
   if (db_find_key(hDB, 0, "/Elog/Types", &hkey) != DB_SUCCESS)
      db_set_value(hDB, 0, "/Elog/Types", type_list, NAME_LENGTH * 20, 20, TID_STRING);
   db_find_key(hDB, 0, "/Elog/Types", &hkey);
   if (hkey)
      db_get_data(hDB, hkey, type_list, &size, TID_STRING);

   /* add types from forms */
   for (j = 0; j < 20 && type_list[j][0]; j++);
   db_find_key(hDB, 0, "/Elog/Forms", &hkey);
   if (hkey)
      for (i = 0; j < 20; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;

         db_get_key(hDB, hsubkey, &key);
         strlcpy(type_list[j++], key.name, NAME_LENGTH);
      }

   /* get system list from ODB */
   size = 20 * NAME_LENGTH;
   if (db_find_key(hDB, 0, "/Elog/Systems", &hkey) != DB_SUCCESS)
      db_set_value(hDB, 0, "/Elog/Systems", system_list, NAME_LENGTH * 20, 20,
                   TID_STRING);
   db_find_key(hDB, 0, "/Elog/Systems", &hkey);
   if (hkey)
      db_get_data(hDB, hkey, system_list, &size, TID_STRING);

   sprintf(ref, "/ELog/");

   rsprintf
       ("<td bgcolor=#FFA0A0><a href=\"%s\" target=\"_blank\">Type:</a> <select name=\"type\">\n",
        ref);
   for (i = 0; i < 20 && type_list[i][0]; i++)
      if ((path && !bedit && equal_ustring(type_list[i], "reply")) ||
          (bedit && equal_ustring(type_list[i], type)))
         rsprintf("<option selected value=\"%s\">%s\n", type_list[i], type_list[i]);
      else
         rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
   rsprintf("</select></tr>\n");

   rsprintf
       ("<tr><td bgcolor=#A0FFA0><a href=\"%s\" target=\"_blank\">  System:</a> <select name=\"system\">\n",
        ref);
   for (i = 0; i < 20 && system_list[i][0]; i++)
      if (path && equal_ustring(system_list[i], system))
         rsprintf("<option selected value=\"%s\">%s\n", system_list[i], system_list[i]);
      else
         rsprintf("<option value=\"%s\">%s\n", system_list[i], system_list[i]);
   rsprintf("</select>\n");

   str[0] = 0;
   if (path && !bedit)
      sprintf(str, "Re: %s", subject);
   else
      sprintf(str, "%s", subject);
   rsprintf
       ("<td bgcolor=#A0FFA0>Subject: <input type=text size=20 maxlength=\"80\" name=Subject value=\"%s\"></tr>\n",
        str);

   if (path) {
      /* hidden text for original message */
      rsprintf("<input type=hidden name=orig value=\"%s\">\n", path);

      if (bedit)
         rsprintf("<input type=hidden name=edit value=1>\n");
   }

   /* increased wrapping for replys (leave space for '> ' */
   wrap = (path && !bedit) ? 78 : 76;

   rsprintf("<tr><td colspan=2>Text:<br>\n");
   rsprintf("<textarea rows=10 cols=%d wrap=hard name=Text>", wrap);

   if (path) {
      if (bedit) {
         rsputs(text);
      } else {
         p = text;
         do {
            if (strchr(p, '\r')) {
               *strchr(p, '\r') = 0;
               rsprintf("> %s\n", p);
               p += strlen(p) + 1;
               if (*p == '\n')
                  p++;
            } else {
               rsprintf("> %s\n\n", p);
               break;
            }

         } while (TRUE);
      }
   }

   rsprintf("</textarea><br>\n");

   /* HTML check box */
   if (bedit && encoding[0] == 'H')
      rsprintf
          ("<input type=checkbox checked name=html value=1>Submit as HTML text</tr>\n");
   else
      rsprintf("<input type=checkbox name=html value=1>Submit as HTML text</tr>\n");

   if (bedit && att1[0])
      rsprintf
          ("<tr><td colspan=2 align=center bgcolor=#8080FF>If no attachment are resubmitted, the original ones are kept</tr>\n");

   /* attachment */
   rsprintf
       ("<tr><td colspan=2 align=center>Enter attachment filename(s) or ODB tree(s), use \"\\\" as an ODB directory separator:</tr>");

   if (odb_att) {
      str[0] = 0;
      if (odb_att[0] != '\\' && odb_att[0] != '/')
         strlcpy(str, "\\", sizeof(str));
      strlcat(str, odb_att, sizeof(str));
      rsprintf
          ("<tr><td colspan=2>Attachment1: <input type=hidden name=attachment0 value=\"%s\"><b>%s</b></tr>\n",
           str, str);
   } else
      rsprintf
          ("<tr><td colspan=2>Attachment1: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile1\" value=\"%s\" accept=\"filetype/*\"></tr>\n",
           att1);

   rsprintf
       ("<tr><td colspan=2>Attachment2: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile2\" value=\"%s\" accept=\"filetype/*\"></tr>\n",
        att2);
   rsprintf
       ("<tr><td colspan=2>Attachment3: <input type=\"file\" size=\"60\" maxlength=\"256\" name=\"attfile3\" value=\"%s\" accept=\"filetype/*\"></tr>\n",
        att3);

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_query()
{
   int i, size;
   char str[256];
   time_t now;
   struct tm *tms;
   HNDLE hDB, hkey, hkeyroot;
   KEY key;
   BOOL display_run_number;

   /* get flag for displaying run number */
   cm_get_experiment_database(&hDB, NULL);
   display_run_number = TRUE;
   size = sizeof(BOOL);
   db_get_value(hDB, 0, "/Elog/Display run number", &display_run_number, &size, TID_BOOL,
                TRUE);

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS ELog</title></head>\n");
   rsprintf("<body><form method=\"GET\" action=\"./\">\n");

   rsprintf("<table border=3 cellpadding=5>\n");

  /*---- title row ----*/

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

   rsprintf("<tr><th colspan=2 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
   if (elog_mode)
      rsprintf("<th colspan=2 bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", str);
   else
      rsprintf("<th colspan=2 bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

  /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=4 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=\"Submit Query\">\n");
   rsprintf("<input type=reset value=\"Reset Form\">\n");
   rsprintf("</tr>\n\n");

  /*---- entry form ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#C0C000>");
   rsprintf("<input type=checkbox name=mode value=\"summary\">Summary only\n");
   rsprintf("<td colspan=2 bgcolor=#C0C000>");
   rsprintf("<input type=checkbox name=attach value=1>Show attachments</tr>\n");

   time(&now);
   now -= 3600 * 24;
   tms = localtime(&now);
   tms->tm_year += 1900;

   rsprintf("<tr><td bgcolor=#FFFF00>Start date: ");
   rsprintf("<td colspan=3 bgcolor=#FFFF00><select name=\"m1\">\n");

   for (i = 0; i < 12; i++)
      if (i == tms->tm_mon)
         rsprintf("<option selected value=\"%s\">%s\n", mname[i], mname[i]);
      else
         rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
   rsprintf("</select>\n");

   rsprintf("<select name=\"d1\">");
   for (i = 0; i < 31; i++)
      if (i + 1 == tms->tm_mday)
         rsprintf("<option selected value=%d>%d\n", i + 1, i + 1);
      else
         rsprintf("<option value=%d>%d\n", i + 1, i + 1);
   rsprintf("</select>\n");

   rsprintf(" <input type=\"text\" size=5 maxlength=5 name=\"y1\" value=\"%d\">",
            tms->tm_year);
   rsprintf("</tr>\n");

   rsprintf("<tr><td bgcolor=#FFFF00>End date: ");
   rsprintf("<td colspan=3 bgcolor=#FFFF00><select name=\"m2\" value=\"%s\">\n",
            mname[tms->tm_mon]);

   rsprintf("<option value=\"\">\n");
   for (i = 0; i < 12; i++)
      rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
   rsprintf("</select>\n");

   rsprintf("<select name=\"d2\">");
   rsprintf("<option selected value=\"\">\n");
   for (i = 0; i < 31; i++)
      rsprintf("<option value=%d>%d\n", i + 1, i + 1);
   rsprintf("</select>\n");

   rsprintf(" <input type=\"text\" size=5 maxlength=5 name=\"y2\">");
   rsprintf("</tr>\n");

   if (display_run_number) {
      rsprintf("<tr><td bgcolor=#A0FFFF>Start run: ");
      rsprintf
          ("<td bgcolor=#A0FFFF><input type=\"text\" size=\"10\" maxlength=\"10\" name=\"r1\">\n");
      rsprintf("<td bgcolor=#A0FFFF>End run: ");
      rsprintf
          ("<td bgcolor=#A0FFFF><input type=\"text\" size=\"10\" maxlength=\"10\" name=\"r2\">\n");
      rsprintf("</tr>\n");
   }

   /* get type list from ODB */
   size = 20 * NAME_LENGTH;
   if (db_find_key(hDB, 0, "/Elog/Types", &hkey) != DB_SUCCESS)
      db_set_value(hDB, 0, "/Elog/Types", type_list, NAME_LENGTH * 20, 20, TID_STRING);
   db_find_key(hDB, 0, "/Elog/Types", &hkey);
   if (hkey)
      db_get_data(hDB, hkey, type_list, &size, TID_STRING);

   /* get system list from ODB */
   size = 20 * NAME_LENGTH;
   if (db_find_key(hDB, 0, "/Elog/Systems", &hkey) != DB_SUCCESS)
      db_set_value(hDB, 0, "/Elog/Systems", system_list, NAME_LENGTH * 20, 20,
                   TID_STRING);
   db_find_key(hDB, 0, "/Elog/Systems", &hkey);
   if (hkey)
      db_get_data(hDB, hkey, system_list, &size, TID_STRING);

   rsprintf("<tr><td colspan=2 bgcolor=#FFA0A0>Author: ");
   rsprintf("<input type=\"test\" size=\"15\" maxlength=\"80\" name=\"author\">\n");

   rsprintf("<td colspan=2 bgcolor=#FFA0A0>Type: ");
   rsprintf("<select name=\"type\">\n");
   rsprintf("<option value=\"\">\n");
   for (i = 0; i < 20 && type_list[i][0]; i++)
      rsprintf("<option value=\"%s\">%s\n", type_list[i], type_list[i]);
   /* add forms as types */
   db_find_key(hDB, 0, "/Elog/Forms", &hkeyroot);
   if (hkeyroot)
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyroot, i, &hkey);
         if (!hkey)
            break;
         db_get_key(hDB, hkey, &key);
         rsprintf("<option value=\"%s\">%s\n", key.name, key.name);
      }
   rsprintf("</select></tr>\n");

   rsprintf("<tr><td colspan=2 bgcolor=#A0FFA0>System: ");
   rsprintf("<select name=\"system\">\n");
   rsprintf("<option value=\"\">\n");
   for (i = 0; i < 20 && system_list[i][0]; i++)
      rsprintf("<option value=\"%s\">%s\n", system_list[i], system_list[i]);
   rsprintf("</select>\n");

   rsprintf("<td colspan=2 bgcolor=#A0FFA0>Subject: ");
   rsprintf("<input type=\"text\" size=\"15\" maxlength=\"80\" name=\"subject\"></tr>\n");

   rsprintf("<tr><td colspan=4 bgcolor=#FFA0FF>Text: ");
   rsprintf("<input type=\"text\" size=\"15\" maxlength=\"80\" name=\"subtext\">\n");
   rsprintf("<i>(case insensitive substring)</i><tr>\n");

   rsprintf("</tr></table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_delete(char *path)
{
   HNDLE hDB;
   int size, status;
   char str[256];
   BOOL allow_delete;

   /* get flag for allowing delete */
   cm_get_experiment_database(&hDB, NULL);
   allow_delete = FALSE;
   size = sizeof(BOOL);
   db_get_value(hDB, 0, "/Elog/Allow delete", &allow_delete, &size, TID_BOOL, TRUE);

   /* redirect if confirm = NO */
   if (getparam("confirm") && *getparam("confirm")
       && strcmp(getparam("confirm"), "No") == 0) {
      sprintf(str, "../EL/%s", path);
      redirect(str);
      return;
   }

   /* header */
   sprintf(str, "../EL/%s", path);
   show_header(hDB, "Delete ELog entry", "GET", str, 1, 0);

   if (!allow_delete) {
      rsprintf
          ("<tr><td colspan=2 bgcolor=#FF8080 align=center><h1>Message deletion disabled in ODB</h1>\n");
   } else {
      if (getparam("confirm") && *getparam("confirm")) {
         if (strcmp(getparam("confirm"), "Yes") == 0) {
            /* delete message */
            status = el_delete_message(path);
            rsprintf("<tr><td colspan=2 bgcolor=#80FF80 align=center>");
            if (status == EL_SUCCESS)
               rsprintf("<b>Message successfully deleted</b></tr>\n");
            else
               rsprintf("<b>Error deleting message: status = %d</b></tr>\n", status);

            rsprintf("<input type=hidden name=cmd value=last>\n");
            rsprintf
                ("<tr><td colspan=2 align=center><input type=submit value=\"Goto last message\"></tr>\n");
         }
      } else {
         /* define hidden field for command */
         rsprintf("<input type=hidden name=cmd value=delete>\n");

         rsprintf("<tr><td colspan=2 bgcolor=#FF8080 align=center>");
         rsprintf("<b>Are you sure to delete this message?</b></tr>\n");

         rsprintf("<tr><td align=center><input type=submit name=confirm value=Yes>\n");
         rsprintf("<td align=center><input type=submit name=confirm value=No>\n");
         rsprintf("</tr>\n\n");
      }
   }

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_elog_submit_query(INT last_n)
{
   int i, size, run, status, m1, d2, m2, y2, index, colspan;
   char date[80], author[80], type[80], system[80], subject[256], text[10000],
       orig_tag[80], reply_tag[80], attachment[3][256], encoding[80];
   char str[256], str2[10000], tag[256], ref[256], file_name[256], *pc;
   HNDLE hDB;
   BOOL full, show_attachments, display_run_number;
   time_t ltime_start, ltime_end, ltime_current, now;
   struct tm tms, *ptms;
   FILE *f;

   /* get flag for displaying run number */
   cm_get_experiment_database(&hDB, NULL);
   display_run_number = TRUE;
   size = sizeof(BOOL);
   db_get_value(hDB, 0, "/Elog/Display run number", &display_run_number, &size, TID_BOOL,
                TRUE);

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS ELog</title></head>\n");
   rsprintf("<body><form method=\"GET\" action=\"./\">\n");

   rsprintf("<table border=3 cellpadding=2 width=\"100%%\">\n");

   /* get mode */
   if (last_n) {
      full = TRUE;
      show_attachments = FALSE;
   } else {
      full = !(*getparam("mode"));
      show_attachments = (*getparam("attach") > 0);
   }

   /*---- title row ----*/

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

   colspan = full ? 3 : 4;
   if (!display_run_number)
      colspan--;

   rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
   if (elog_mode)
      rsprintf("<th colspan=%d bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", colspan, str);
   else
      rsprintf("<th colspan=%d bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", colspan, str);

   /*---- menu buttons ----*/

   if (!full) {
      colspan = display_run_number ? 7 : 6;
      rsprintf("<tr><td colspan=%d bgcolor=#C0C0C0>\n", colspan);

      rsprintf("<input type=submit name=cmd value=\"Query\">\n");
      rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
      rsprintf("<input type=submit name=cmd value=\"Status\">\n");
      rsprintf("</tr>\n\n");
   }

   /*---- convert end date to ltime ----*/

   ltime_end = ltime_start = 0;
   m1 = m2 = d2 = y2 = 0;

   if (!last_n) {
      strlcpy(str, getparam("m1"), sizeof(str));
      for (m1 = 0; m1 < 12; m1++)
         if (equal_ustring(str, mname[m1]))
            break;
      if (m1 == 12)
         m1 = 0;

      if (*getparam("m2") || *getparam("y2") || *getparam("d2")) {
         if (*getparam("m2")) {
            strlcpy(str, getparam("m2"), sizeof(str));
            for (m2 = 0; m2 < 12; m2++)
               if (equal_ustring(str, mname[m2]))
                  break;
            if (m2 == 12)
               m2 = 0;
         } else
            m2 = m1;

         if (*getparam("y2"))
            y2 = atoi(getparam("y2"));
         else
            y2 = atoi(getparam("y1"));

         if (*getparam("d2"))
            d2 = atoi(getparam("d2"));
         else
            d2 = atoi(getparam("d1"));

         memset(&tms, 0, sizeof(struct tm));
         tms.tm_year = y2 % 100;
         tms.tm_mon = m2;
         tms.tm_mday = d2;
         tms.tm_hour = 24;

         if (tms.tm_year < 90)
            tms.tm_year += 100;
         ltime_end = mktime(&tms);
      }
   }

  /*---- title row ----*/

   colspan = full ? 6 : 7;
   if (!display_run_number)
      colspan--;

   /* menu buttons */
   rsprintf("<tr><td colspan=%d bgcolor=#C0C0C0>\n", colspan);
   rsprintf("<input type=submit name=cmd value=Query>\n");
   rsprintf("<input type=submit name=cmd value=Last>\n");
   if (!elog_mode)
      rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("</tr>\n");

   if (*getparam("r1")) {
      if (*getparam("r2"))
         rsprintf
             ("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between runs %s and %s</b></tr>\n",
              colspan, getparam("r1"), getparam("r2"));
      else
         rsprintf
             ("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between run %s and today</b></tr>\n",
              colspan, getparam("r1"));
   } else {
      if (last_n) {
         if (last_n < 24) {
            rsprintf("<tr><td colspan=6><a href=\"last%d\">Last %d hours</a></tr>\n",
                        last_n * 2, last_n * 2);

            rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><b>Last %d hours</b></tr>\n",
                     last_n);
         } else {
            rsprintf("<tr><td colspan=6><a href=\"last%d\">Last %d days</a></tr>\n",
                        last_n * 2, last_n / 24 * 2);

            rsprintf("<tr><td colspan=6 bgcolor=#FFFF00><b>Last %d days</b></tr>\n",
                     last_n / 24);
         }
      }

      else if (*getparam("m2") || *getparam("y2") || *getparam("d2"))
         rsprintf
             ("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
              colspan, getparam("m1"), getparam("d1"), getparam("y1"), mname[m2], d2, y2);
      else {
         time(&now);
         ptms = localtime(&now);
         ptms->tm_year += 1900;

         rsprintf
             ("<tr><td colspan=%d bgcolor=#FFFF00><b>Query result between %s %s %s and %s %d %d</b></tr>\n",
              colspan, getparam("m1"), getparam("d1"), getparam("y1"),
              mname[ptms->tm_mon], ptms->tm_mday, ptms->tm_year);
      }
   }

   rsprintf("</tr>\n<tr>");

   rsprintf("<td colspan=%d bgcolor=#FFA0A0>\n", colspan);

   if (*getparam("author"))
      rsprintf("Author: <b>%s</b>   ", getparam("author"));

   if (*getparam("type"))
      rsprintf("Type: <b>%s</b>   ", getparam("type"));

   if (*getparam("system"))
      rsprintf("System: <b>%s</b>   ", getparam("system"));

   if (*getparam("subject"))
      rsprintf("Subject: <b>%s</b>   ", getparam("subject"));

   if (*getparam("subtext"))
      rsprintf("Text: <b>%s</b>   ", getparam("subtext"));

   rsprintf("</tr>\n");

  /*---- table titles ----*/

   if (display_run_number) {
      if (full)
         rsprintf("<tr><th>Date<th>Run<th>Author<th>Type<th>System<th>Subject</tr>\n");
      else
         rsprintf
             ("<tr><th>Date<th>Run<th>Author<th>Type<th>System<th>Subject<th>Text</tr>\n");
   } else {
      if (full)
         rsprintf("<tr><th>Date<th>Author<th>Type<th>System<th>Subject</tr>\n");
      else
         rsprintf("<tr><th>Date<th>Author<th>Type<th>System<th>Subject<th>Text</tr>\n");
   }

  /*---- do query ----*/

   if (last_n) {
      time(&now);
      ltime_start = now - 3600 * last_n;
      ptms = localtime(&ltime_start);
      sprintf(tag, "%02d%02d%02d.0", ptms->tm_year % 100, ptms->tm_mon + 1,
              ptms->tm_mday);
   } else if (*getparam("r1")) {
      /* do run query */
      el_search_run(atoi(getparam("r1")), tag);
   } else {
      /* do date-date query */
      sprintf(tag, "%02d%02d%02d.0", atoi(getparam("y1")) % 100, m1 + 1,
              atoi(getparam("d1")));
   }

   do {
      size = sizeof(text);
      status = el_retrieve(tag, date, &run, author, type, system, subject,
                           text, &size, orig_tag, reply_tag,
                           attachment[0], attachment[1], attachment[2], encoding);
      strlcat(tag, "+1", sizeof(tag));

      /* check for end run */
      if (*getparam("r2") && atoi(getparam("r2")) < run)
         break;

      /* convert date to unix format */
      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = (tag[0] - '0') * 10 + (tag[1] - '0');
      tms.tm_mon = (tag[2] - '0') * 10 + (tag[3] - '0') - 1;
      tms.tm_mday = (tag[4] - '0') * 10 + (tag[5] - '0');
      tms.tm_hour = (date[11] - '0') * 10 + (date[12] - '0');
      tms.tm_min = (date[14] - '0') * 10 + (date[15] - '0');
      tms.tm_sec = (date[17] - '0') * 10 + (date[18] - '0');

      if (tms.tm_year < 90)
         tms.tm_year += 100;
      ltime_current = mktime(&tms);

      /* check for start date */
      if (ltime_start > 0)
         if (ltime_current < ltime_start)
            continue;

      /* check for end date */
      if (ltime_end > 0) {
         if (ltime_current > ltime_end)
            break;
      }

      if (status == EL_SUCCESS) {
         /* do filtering */
         if (*getparam("type") && !equal_ustring(getparam("type"), type))
            continue;
         if (*getparam("system") && !equal_ustring(getparam("system"), system))
            continue;

         if (*getparam("author")) {
            strlcpy(str, getparam("author"), sizeof(str));
            for (i = 0; i < (int) strlen(str); i++)
               str[i] = toupper(str[i]);
            str[i] = 0;
            for (i = 0; i < (int) strlen(author) && author[i] != '@'; i++)
               str2[i] = toupper(author[i]);
            str2[i] = 0;

            if (strstr(str2, str) == NULL)
               continue;
         }

         if (*getparam("subject")) {
            strlcpy(str, getparam("subject"), sizeof(str));
            for (i = 0; i < (int) strlen(str); i++)
               str[i] = toupper(str[i]);
            str[i] = 0;
            for (i = 0; i < (int) strlen(subject); i++)
               str2[i] = toupper(subject[i]);
            str2[i] = 0;

            if (strstr(str2, str) == NULL)
               continue;
         }

         if (*getparam("subtext")) {
            strlcpy(str, getparam("subtext"), sizeof(str));
            for (i = 0; i < (int) strlen(str); i++)
               str[i] = toupper(str[i]);
            str[i] = 0;
            for (i = 0; i < (int) strlen(text); i++)
               str2[i] = toupper(text[i]);
            str2[i] = 0;

            if (strstr(str2, str) == NULL)
               continue;
         }

         /* filter passed: display line */

         strlcpy(str, tag, sizeof(str));
         if (strchr(str, '+'))
            *strchr(str, '+') = 0;
         sprintf(ref, "/EL/%s", str);

         strlcpy(str, text, sizeof(str));

         if (full) {
            if (display_run_number) {
               rsprintf("<tr><td><a href=%s>%s</a><td>%d<td>%s<td>%s<td>%s<td>%s</tr>\n",
                        ref, date, run, author, type, system, subject);
               rsprintf("<tr><td colspan=6>");
            } else {
               rsprintf("<tr><td><a href=%s>%s</a><td>%s<td>%s<td>%s<td>%s</tr>\n", ref,
                        date, author, type, system, subject);
               rsprintf("<tr><td colspan=5>");
            }

            if (equal_ustring(encoding, "plain")) {
               rsputs("<pre>");
               rsputs2(text);
               rsputs("</pre>");
            } else
               rsputs(text);

            if (!show_attachments && attachment[0][0]) {
               if (attachment[1][0])
                  rsprintf("Attachments: ");
               else
                  rsprintf("Attachment: ");
            } else
               rsprintf("</tr>\n");

            for (index = 0; index < 3; index++) {
               if (attachment[index][0]) {
                  char ref1[256];

                  for (i = 0; i < (int) strlen(attachment[index]); i++)
                     str[i] = toupper(attachment[index][i]);
                  str[i] = 0;

                  strlcpy(ref1, attachment[index], sizeof(ref1));
                  urlEncode(ref1, sizeof(ref1));
		  
                  sprintf(ref, "/EL/%s", ref1);

                  if (!show_attachments) {
                     rsprintf("<a href=\"%s\"><b>%s</b></a> ", ref,
                              attachment[index] + 14);
                  } else {
                     colspan = display_run_number ? 6 : 5;
                     if (strstr(str, ".GIF") || strstr(str, ".PNG")
                         || strstr(str, ".JPG")) {
                        rsprintf
                            ("<tr><td colspan=%d>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n",
                             colspan, ref, attachment[index] + 14);
                        if (show_attachments)
                           rsprintf("<img src=\"%s\"></tr>", ref);
                     } else {
                        rsprintf
                            ("<tr><td colspan=%d bgcolor=#C0C0FF>Attachment: <a href=\"%s\"><b>%s</b></a>\n",
                             colspan, ref, attachment[index] + 14);

                        if ((strstr(str, ".TXT") ||
                             strstr(str, ".ASC") || strchr(str, '.') == NULL)
                            && show_attachments) {
                           /* display attachment */
                           rsprintf("<br><pre>");

                           file_name[0] = 0;
                           size = sizeof(file_name);
                           memset(file_name, 0, size);
                           db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size,
                                        TID_STRING, TRUE);
                           if (file_name[0] != 0)
                              if (file_name[strlen(file_name) - 1] != DIR_SEPARATOR)
                                 strlcat(file_name, DIR_SEPARATOR_STR, sizeof(file_name));
                           strlcat(file_name, attachment[index], sizeof(file_name));

                           f = fopen(file_name, "rt");
                           if (f != NULL) {
                              while (!feof(f)) {
                                 str[0] = 0;
                                 pc = fgets(str, sizeof(str), f);
                                 if (pc == NULL)
                                    break;
                                 rsputs2(str);
                              }
                              fclose(f);
                           }

                           rsprintf("</pre>\n");
                        }
                        rsprintf("</tr>\n");
                     }
                  }
               }
            }

            if (!show_attachments && attachment[0][0])
               rsprintf("</tr>\n");

         } else {
            if (display_run_number)
               rsprintf("<tr><td><a href=%s>%s</a><td>%d<td>%s<td>%s<td>%s<td>%s\n", ref,
                        date, run, author, type, system, subject);
            else
               rsprintf("<tr><td><a href=%s>%s</a><td>%s<td>%s<td>%s<td>%s\n", ref, date,
                        author, type, system, subject);

            if (equal_ustring(encoding, "HTML"))
               rsputs(text);
            else
               strencode(text);

            rsprintf("</tr>\n");
         }
      }

   } while (status == EL_SUCCESS);

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_rawfile(char *path)
{
   int size, lines, i, buf_size, offset;
   char *p;
   FILE *f;
   char file_name[256], str[100], buffer[100000];
   HNDLE hDB;

   cm_get_experiment_database(&hDB, NULL);

   lines = 10;
   if (*getparam("lines"))
      lines = atoi(getparam("lines"));

   if (*getparam("cmd") && equal_ustring(getparam("cmd"), "More lines"))
      lines *= 2;

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS File Display %s</title></head>\n", path);
   rsprintf("<body><form method=\"GET\" action=\"./%s\">\n", path);

   rsprintf("<input type=hidden name=lines value=%d>\n", lines);

   rsprintf("<table border=3 cellpadding=1 width=\"100%%\">\n");

   /*---- title row ----*/

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

   rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS File Display <code>\"%s\"</code>", path);
   if (elog_mode)
      rsprintf("<th bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", str);
   else
      rsprintf("<th bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=\"ELog\">\n");
   if (!elog_mode)
      rsprintf("<input type=submit name=cmd value=\"Status\">\n");

   rsprintf("<input type=submit name=cmd value=\"More lines\">\n");

   rsprintf("</tr>\n\n");

   /*---- open file ----*/

   cm_get_experiment_database(&hDB, NULL);
   file_name[0] = 0;
   if (hDB > 0) {
      size = sizeof(file_name);
      memset(file_name, 0, size);
      db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING, TRUE);
      if (file_name[0] != 0)
         if (file_name[strlen(file_name) - 1] != DIR_SEPARATOR)
            strlcat(file_name, DIR_SEPARATOR_STR, sizeof(file_name));
   }
   strlcat(file_name, path, sizeof(file_name));

   f = fopen(file_name, "r");
   if (f == NULL) {
      rsprintf("<h3>Cannot find file \"%s\"</h3>\n", file_name);
      rsprintf("</body></html>\n");
      return;
   }

   /*---- file contents ----*/

   rsprintf("<tr><td colspan=2>\n");

   rsprintf("<pre>\n");

   buf_size = sizeof(buffer);

   /* position buf_size bytes before the EOF */
   fseek(f, -(buf_size - 1), SEEK_END);
   offset = ftell(f);
   if (offset != 0) {
      /* go to end of line */
      fgets(buffer, buf_size - 1, f);
      offset = ftell(f) - offset;
      buf_size -= offset;
   }

   memset(buffer, 0, buf_size);
   fread(buffer, 1, buf_size - 1, f);
   buffer[buf_size - 1] = 0;
   fclose(f);

   p = buffer + (buf_size - 2);

   /* goto end of buffer */
   while (p != buffer && *p == 0)
      p--;

   /* strip line break */
   while (p != buffer && (*p == '\n' || *p == '\r'))
      *(p--) = 0;

   /* trim buffer so that last lines remain */
   for (i = 0; i < lines; i++) {
      while (p != buffer && *p != '\n')
         p--;

      while (p != buffer && (*p == '\n' || *p == '\r'))
         p--;
   }
   if (p != buffer) {
      p++;
      while (*p == '\n' || *p == '\r')
         p++;
   }

   buf_size = (buf_size - 1) - ((POINTER_T) p - (POINTER_T) buffer);

   memmove(buffer, p, buf_size);
   buffer[buf_size] = 0;

   rsputs(buffer);

   rsprintf("</pre>\n");

   rsprintf("</td></tr></table></body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_form_query()
{
   int i = 0, size, run_number, status;
   char str[256];
   time_t now;
   HNDLE hDB, hkey, hkeyroot;
   KEY key;

   cm_get_experiment_database(&hDB, NULL);

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS ELog</title></head>\n");
   rsprintf("<body><form method=\"GET\" action=\"./\">\n");

   if (*getparam("form") == 0)
      return;

   /* hidden field for form */
   rsprintf("<input type=hidden name=form value=\"%s\">\n", getparam("form"));

   rsprintf("<table border=3 cellpadding=1>\n");

   /*---- title row ----*/

   rsprintf("<tr><th colspan=2 bgcolor=#A0A0FF>MIDAS Electronic Logbook");
   rsprintf("<th colspan=2 bgcolor=#A0A0FF>Form \"%s\"</tr>\n", getparam("form"));

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=4 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=\"Submit\">\n");
   rsprintf("<input type=reset value=\"Reset Form\">\n");
   rsprintf("</tr>\n\n");

   /*---- entry form ----*/

   time(&now);
   rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: %s", ctime(&now));

   run_number = 0;
   size = sizeof(run_number);
   status =
       db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT, TRUE);
   assert(status == SUCCESS);

   if (run_number < 0) {
      cm_msg(MERROR, "show_form_query",
             "aborting on attempt to use invalid run number %d", run_number);
      abort();
   }

   rsprintf("<td bgcolor=#FFFF00>Run number: ");
   rsprintf("<input type=\"text\" size=10 maxlength=10 name=\"run\" value=\"%d\"</tr>",
            run_number);

   rsprintf
       ("<tr><td colspan=2 bgcolor=#FFA0A0>Author: <input type=\"text\" size=\"15\" maxlength=\"80\" name=\"Author\">\n");

   rsprintf
       ("<tr><th bgcolor=#A0FFA0>Item<th bgcolor=#FFFF00>Checked<th bgcolor=#A0A0FF colspan=2>Comment</tr>\n");

   sprintf(str, "/Elog/Forms/%s", getparam("form"));
   db_find_key(hDB, 0, str, &hkeyroot);
   i = 0;
   if (hkeyroot)
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyroot, i, &hkey);
         if (!hkey)
            break;

         db_get_key(hDB, hkey, &key);

         strlcpy(str, key.name, sizeof(str));
         if (str[0])
            str[strlen(str) - 1] = 0;
         if (equal_ustring(str, "attachment")) {
            size = sizeof(str);
            db_get_data(hDB, hkey, str, &size, TID_STRING);
            rsprintf("<tr><td colspan=2 align=center bgcolor=#FFFFFF><b>%s:</b>",
                     key.name);
            rsprintf
                ("<td bgcolor=#A0A0FF colspan=2><input type=text size=30 maxlength=255 name=c%d value=\"%s\"></tr>\n",
                 i, str);
         } else {
            rsprintf("<tr><td bgcolor=#A0FFA0>%d <b>%s</b>", i + 1, key.name);
            rsprintf
                ("<td bgcolor=#FFFF00 align=center><input type=checkbox name=x%d value=1>",
                 i);
            rsprintf
                ("<td bgcolor=#A0A0FF colspan=2><input type=text size=30 maxlength=255 name=c%d></tr>\n",
                 i);
         }
      }


   /*---- menu buttons at bottom ----*/

   if (i > 10) {
      rsprintf("<tr><td colspan=4 bgcolor=#C0C0C0>\n");

      rsprintf("<input type=submit name=cmd value=\"Submit\">\n");
      rsprintf("</tr>\n\n");
   }

   rsprintf("</tr></table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void gen_odb_attachment(char *path, char *b)
{
   HNDLE hDB, hkeyroot, hkey;
   KEY key;
   INT i, j, size;
   char str[256], data_str[256], hex_str[256];
   char data[10000];
   time_t now;

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, path, &hkeyroot);
   assert(hkeyroot);

   /* title row */
   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);

   sprintf(b, "<table border=3 cellpadding=1>\n");
   sprintf(b + strlen(b), "<tr><th colspan=2 bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));
   sprintf(b + strlen(b), "<tr><th colspan=2 bgcolor=#FFA0A0>%s</tr>\n", path);

   /* enumerate subkeys */
   for (i = 0;; i++) {
      db_enum_link(hDB, hkeyroot, i, &hkey);
      if (!hkey)
         break;
      db_get_key(hDB, hkey, &key);

      /* resolve links */
      if (key.type == TID_LINK) {
         db_enum_key(hDB, hkeyroot, i, &hkey);
         db_get_key(hDB, hkey, &key);
      }

      if (key.type == TID_KEY) {
         /* for keys, don't display data value */
         sprintf(b + strlen(b), "<tr><td colspan=2 bgcolor=#FFD000>%s</td></tr>\n",
                 key.name);
      } else {
         /* display single value */
         if (key.num_values == 1) {
            size = sizeof(data);
            db_get_data(hDB, hkey, data, &size, key.type);
            db_sprintf(data_str, data, key.item_size, 0, key.type);
            db_sprintfh(hex_str, data, key.item_size, 0, key.type);

            if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>")) {
               strcpy(data_str, "(empty)");
               hex_str[0] = 0;
            }

            if (strcmp(data_str, hex_str) != 0 && hex_str[0])
               sprintf(b + strlen(b),
                       "<tr><td bgcolor=#FFFF00>%s</td><td bgcolor=#FFFFFF>%s (%s)</td></tr>\n",
                       key.name, data_str, hex_str);
            else {
               sprintf(b + strlen(b),
                       "<tr><td bgcolor=#FFFF00>%s</td><td bgcolor=#FFFFFF>", key.name);
               strencode2(b + strlen(b), data_str);
               sprintf(b + strlen(b), "</td></tr>\n");
            }
         } else {
            /* display first value */
            sprintf(b + strlen(b), "<tr><td  bgcolor=#FFFF00 rowspan=%d>%s</td>\n",
                    key.num_values, key.name);

            for (j = 0; j < key.num_values; j++) {
               size = sizeof(data);
               db_get_data_index(hDB, hkey, data, &size, j, key.type);
               db_sprintf(data_str, data, key.item_size, 0, key.type);
               db_sprintfh(hex_str, data, key.item_size, 0, key.type);

               if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>")) {
                  strcpy(data_str, "(empty)");
                  hex_str[0] = 0;
               }

               if (j > 0)
                  sprintf(b + strlen(b), "<tr>");

               if (strcmp(data_str, hex_str) != 0 && hex_str[0])
                  sprintf(b + strlen(b),
                          "<td bgcolor=#FFFFFF>[%d] %s (%s)<br></td></tr>\n", j, data_str,
                          hex_str);
               else
                  sprintf(b + strlen(b), "<td bgcolor=#FFFFFF>[%d] %s<br></td></tr>\n", j,
                          data_str);
            }
         }
      }
   }

   sprintf(b + strlen(b), "</table>\n");
}

/*------------------------------------------------------------------*/

void submit_elog()
{
   char str[80], author[256], path[256], path1[256];
   char mail_to[256], mail_from[256], mail_text[10000], mail_list[256],
       smtp_host[256], tag[80], mail_param[1000];
   char *buffer[3], *p, *pitem;
   HNDLE hDB, hkey;
   char att_file[3][256];
   int i, fh, size, n_mail, index;
   struct hostent *phe;
   char mhttpd_full_url[256];

   cm_get_experiment_database(&hDB, NULL);
   strlcpy(att_file[0], getparam("attachment0"), sizeof(att_file[0]));
   strlcpy(att_file[1], getparam("attachment1"), sizeof(att_file[1]));
   strlcpy(att_file[2], getparam("attachment2"), sizeof(att_file[2]));

   /* check for author */
   if (*getparam("author") == 0) {
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
      rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

      rsprintf("<html><head>\n");
      rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
      rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
      rsprintf("<title>ELog Error</title></head>\n");
      rsprintf("<i>Error: No author supplied.</i><p>\n");
      rsprintf("Please go back and enter your name in the <i>author</i> field.\n");
      rsprintf("<body></body></html>\n");
      return;
   }

   /* check for valid attachment files */
   for (i = 0; i < 3; i++) {
      buffer[i] = NULL;
      sprintf(str, "attachment%d", i);
      if (getparam(str) && *getparam(str) && _attachment_size[i] == 0) {
         /* replace '\' by '/' */
         strlcpy(path, getparam(str), sizeof(path));
         strlcpy(path1, path, sizeof(path1));
         while (strchr(path, '\\'))
            *strchr(path, '\\') = '/';

         /* check if valid ODB tree */
         if (db_find_key(hDB, 0, path, &hkey) == DB_SUCCESS) {
           buffer[i] = (char*)M_MALLOC(100000);
            gen_odb_attachment(path, buffer[i]);
            strlcpy(att_file[i], path, sizeof(att_file[0]));
            strlcat(att_file[i], ".html", sizeof(att_file[0]));
            _attachment_buffer[i] = buffer[i];
            _attachment_size[i] = strlen(buffer[i]) + 1;
         }
         /* check if local file */
         else if ((fh = open(path1, O_RDONLY | O_BINARY)) >= 0) {
            size = lseek(fh, 0, SEEK_END);
            buffer[i] = (char*)M_MALLOC(size);
            lseek(fh, 0, SEEK_SET);
            read(fh, buffer[i], size);
            close(fh);
            strlcpy(att_file[i], path, sizeof(att_file[0]));
            _attachment_buffer[i] = buffer[i];
            _attachment_size[i] = size;
         } else if (strncmp(path, "/HS/", 4) == 0) {
           buffer[i] = (char*)M_MALLOC(100000);
            size = 100000;
            strlcpy(str, path + 4, sizeof(str));
            if (strchr(str, '?')) {
               p = strchr(str, '?') + 1;
               p = strtok(p, "&");
               while (p != NULL) {
                  pitem = p;
                  p = strchr(p, '=');
                  if (p != NULL) {
                     *p++ = 0;
                     urlDecode(pitem);
                     urlDecode(p);

                     setparam(pitem, p);

                     p = strtok(NULL, "&");
                  }
               }
               *strchr(str, '?') = 0;
            }
            show_hist_page(str, sizeof(str), buffer[i], &size, 0);
            strlcpy(att_file[i], str, sizeof(att_file[0]));
            _attachment_buffer[i] = buffer[i];
            _attachment_size[i] = size;
            unsetparam("scale");
            unsetparam("offset");
            unsetparam("width");
            unsetparam("index");
         } else {
            rsprintf("HTTP/1.0 200 Document follows\r\n");
            rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
            rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

            rsprintf("<html><head>\n");
            rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
            rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
            rsprintf("<title>ELog Error</title></head>\n");
            rsprintf("<i>Error: Attachment file <i>%s</i> not valid.</i><p>\n",
                     getparam(str));
            rsprintf
                ("Please go back and enter a proper filename (use the <b>Browse</b> button).\n");
            rsprintf("<body></body></html>\n");
            return;
         }
      }
   }

   /* add remote host name to author */
   phe = gethostbyaddr((char *) &remote_addr, 4, PF_INET);
   if (phe == NULL) {
      /* use IP number instead */
      strlcpy(str, (char *) inet_ntoa(remote_addr), sizeof(str));
   } else
      strlcpy(str, phe->h_name, sizeof(str));

   strlcpy(author, getparam("author"), sizeof(author));
   strlcat(author, "@", sizeof(author));
   strlcat(author, str, sizeof(author));

   tag[0] = 0;
   if (*getparam("edit"))
      strlcpy(tag, getparam("orig"), sizeof(tag));

   el_submit(atoi(getparam("run")), author, getparam("type"),
             getparam("system"), getparam("subject"), getparam("text"),
             getparam("orig"), *getparam("html") ? "HTML" : "plain",
             att_file[0], _attachment_buffer[0], _attachment_size[0],
             att_file[1], _attachment_buffer[1], _attachment_size[1],
             att_file[2], _attachment_buffer[2], _attachment_size[2], tag, sizeof(tag));

   /* supersede host name with "/Elog/Host name" */
   size = sizeof(host_name);
   db_get_value(hDB, 0, "/Elog/Host name", host_name, &size, TID_STRING, TRUE);

   if (tcp_port == 80)
      sprintf(mhttpd_full_url, "http://%s/", host_name);
   else
      sprintf(mhttpd_full_url, "http://%s:%d/", host_name, tcp_port);

   /* check for mail submissions */
   mail_param[0] = 0;
   n_mail = 0;

   for (index = 0; index <= 1; index++) {
      if (index == 0)
         sprintf(str, "/Elog/Email %s", getparam("type"));
      else
         sprintf(str, "/Elog/Email %s", getparam("system"));

      if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS) {
         size = sizeof(mail_list);
         db_get_data(hDB, hkey, mail_list, &size, TID_STRING);

         if (db_find_key(hDB, 0, "/Elog/SMTP host", &hkey) != DB_SUCCESS) {
            show_error("No SMTP host defined under /Elog/SMTP host");
            return;
         }
         size = sizeof(smtp_host);
         db_get_data(hDB, hkey, smtp_host, &size, TID_STRING);

         p = strtok(mail_list, ",");
         for (i = 0; p; i++) {
            strlcpy(mail_to, p, sizeof(mail_to));

            size = sizeof(str);
            str[0] = 0;
            db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

            sprintf(mail_from, "MIDAS %s <MIDAS@%s>", str, host_name);

            sprintf(mail_text, "A new entry has been submitted by %s:\n\n", author);
            sprintf(mail_text + strlen(mail_text), "Experiment : %s\n", str);
            sprintf(mail_text + strlen(mail_text), "Type       : %s\n", getparam("type"));
            sprintf(mail_text + strlen(mail_text), "System     : %s\n",
                    getparam("system"));
            sprintf(mail_text + strlen(mail_text), "Subject    : %s\n",
                    getparam("subject"));

            sprintf(mail_text + strlen(mail_text), "Link       : %sEL/%s\n",
                       mhttpd_full_url, tag);

            assert(strlen(mail_text) + 100 < sizeof(mail_text));        // bomb out on array overrun.

            strlcat(mail_text + strlen(mail_text), "\n", sizeof(mail_text));
            strlcat(mail_text + strlen(mail_text), getparam("text"),
                    sizeof(mail_text) - strlen(mail_text) - 50);
            strlcat(mail_text + strlen(mail_text), "\n", sizeof(mail_text));

            assert(strlen(mail_text) < sizeof(mail_text));      // bomb out on array overrun.

            sendmail(smtp_host, mail_from, mail_to, getparam("type"), mail_text);

            if (mail_param[0] == 0)
               strlcpy(mail_param, "?", sizeof(mail_param));
            else
               strlcat(mail_param, "&", sizeof(mail_param));
            sprintf(mail_param + strlen(mail_param), "mail%d=%s", n_mail++, mail_to);

            p = strtok(NULL, ",");
            if (!p)
               break;
            while (*p == ' ')
               p++;
         }
      }
   }

   for (i = 0; i < 3; i++)
      if (buffer[i]) {
         M_FREE(buffer[i]);
         buffer[i] = NULL;
      }

   rsprintf("HTTP/1.0 302 Found\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());

   if (mail_param[0])
      rsprintf("Location: ../EL/%s?%s\n\n<html>redir</html>\r\n", tag, mail_param + 1);
   else
      rsprintf("Location: ../EL/%s\n\n<html>redir</html>\r\n", tag);
}

/*------------------------------------------------------------------*/

void submit_form()
{
   char str[256], att_name[256];
   char text[10000];
   int i, n_att, size;
   HNDLE hDB, hkey, hkeyroot;
   KEY key;

   /* check for author */
   if (*getparam("author") == 0) {
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
      rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

      rsprintf("<html><head>\n");
      rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
      rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
      rsprintf("<title>ELog Error</title></head>\n");
      rsprintf("<i>Error: No author supplied.</i><p>\n");
      rsprintf("Please go back and enter your name in the <i>author</i> field.\n");
      rsprintf("<body></body></html>\n");
      return;
   }

   /* assemble text from form */
   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Elog/Forms/%s", getparam("form"));
   db_find_key(hDB, 0, str, &hkeyroot);
   text[0] = 0;
   n_att = 0;
   if (hkeyroot)
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyroot, i, &hkey);
         if (!hkey)
            break;

         db_get_key(hDB, hkey, &key);

         strlcpy(str, key.name, sizeof(str));
         if (str[0])
            str[strlen(str) - 1] = 0;
         if (equal_ustring(str, "attachment")) {
            /* generate attachments */
            size = sizeof(str);
            db_get_data(hDB, hkey, str, &size, TID_STRING);
            _attachment_size[n_att] = 0;
            sprintf(att_name, "attachment%d", n_att++);

            sprintf(str, "c%d", i);
            setparam(att_name, getparam(str));
         } else {
            sprintf(str, "x%d", i);
            sprintf(text + strlen(text), "%d %s : [%c]  ", i + 1, key.name,
                    *getparam(str) == '1' ? 'X' : ' ');
            sprintf(str, "c%d", i);
            sprintf(text + strlen(text), "%s\n", getparam(str));
         }
      }

   /* set parameters for submit_elog() */
   setparam("type", getparam("form"));
   setparam("system", "General");
   setparam("subject", getparam("form"));
   setparam("text", text);
   setparam("orig", "");
   setparam("html", "");

   submit_elog();
}

/*------------------------------------------------------------------*/

void show_elog_page(char *path, int path_size)
{
   int size, i, run, msg_status, status, fh, length, first_message, last_message, index,
      fsize;
   char str[256], orig_path[256], command[80], ref[256], file_name[256], dir[256], *fbuffer;
   char date[80], author[80], type[80], system[80], subject[256], text[10000],
       orig_tag[80], reply_tag[80], attachment[3][256], encoding[80], att[256], url[256];
   HNDLE hDB, hkey, hkeyroot, hkeybutton;
   KEY key;
   FILE *f;
   BOOL display_run_number, allow_delete;
   time_t now;
   struct tm *tms;
   char def_button[][NAME_LENGTH] = { "8h", "24h", "7d" };

   /* get flag for displaying run number and allow delete */
   cm_get_experiment_database(&hDB, NULL);
   display_run_number = TRUE;
   allow_delete = FALSE;
   size = sizeof(BOOL);
   db_get_value(hDB, 0, "/Elog/Display run number", &display_run_number, &size, TID_BOOL,
                TRUE);
   db_get_value(hDB, 0, "/Elog/Allow delete", &allow_delete, &size, TID_BOOL, TRUE);

   /*---- interprete commands ---------------------------------------*/

   strlcpy(command, getparam("cmd"), sizeof(command));

   if (*getparam("form")) {
      if (*getparam("type")) {
         sprintf(str, "EL/?form=%s", getparam("form"));
         redirect(str);
         return;
      }
      if (equal_ustring(command, "submit"))
         submit_form();
      else
         show_form_query();
      return;
   }

   if (equal_ustring(command, "new")) {
      if (*getparam("file"))
         show_elog_new(NULL, FALSE, getparam("file"), NULL);
      else
         show_elog_new(NULL, FALSE, NULL, NULL);
      return;
   }

   if (equal_ustring(command, "Create ELog from this page")) {

      size = sizeof(url);
      if (db_get_value(hDB, 0, "/Elog/URL", url, &size, TID_STRING, FALSE) == DB_SUCCESS) {

         get_elog_url(url, sizeof(url));

         /*---- use external ELOG ----*/
         fsize = 100000;
         fbuffer = (char*)M_MALLOC(fsize);
         assert(fbuffer != NULL);

         /* write ODB contents to buffer */
         gen_odb_attachment(path, fbuffer);
         fsize = strlen(fbuffer);

         /* save temporary file */
         size = sizeof(dir);
         dir[0] = 0;
         db_get_value(hDB, 0, "/Elog/Logbook Dir", dir, &size, TID_STRING, TRUE);
         if (strlen(dir) > 0 && dir[strlen(dir)-1] != DIR_SEPARATOR)
            strlcat(dir, DIR_SEPARATOR_STR, sizeof(dir));

         time(&now);
         tms = localtime(&now);

         if (strchr(path, '/'))
            strlcpy(str, strrchr(path, '/') + 1, sizeof(str));
         else
            strlcpy(str, path, sizeof(str));
         sprintf(file_name, "%02d%02d%02d_%02d%02d%02d_%s.html",
                  tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday,
                  tms->tm_hour, tms->tm_min, tms->tm_sec, str);
         sprintf(str, "%s%s", dir, file_name);

         /* save attachment */
         fh = open(str, O_CREAT | O_RDWR | O_BINARY, 0644);
         if (fh < 0) {
            cm_msg(MERROR, "show_hist_page", "Cannot write attachment file \"%s\"",
                     str);
         } else {
            write(fh, fbuffer, fsize);
            close(fh);
         }

         /* redirect to ELOG */
         if (strlen(url) > 1 && url[strlen(url)-1] != '/')
            strlcat(url, "/", sizeof(url));
         strlcat(url, "?cmd=New&fa=", sizeof(url));
         strlcat(url, file_name, sizeof(url));
         redirect(url);

         M_FREE(fbuffer);
         return;
      
      } else {

         char action_path[256];

         action_path[0] = 0;

         strlcpy(str, path, sizeof(str));
         while (strchr(path, '/')) {
            *strchr(path, '/') = '\\';
            strlcat(action_path, "../", sizeof(action_path));
         }

         strlcat(action_path, "EL/", sizeof(action_path));

         show_elog_new(NULL, FALSE, path, action_path);
         return;
      }
   }

   if (equal_ustring(command, "edit")) {
      show_elog_new(path, TRUE, NULL, NULL);
      return;
   }

   if (equal_ustring(command, "reply")) {
      show_elog_new(path, FALSE, NULL, NULL);
      return;
   }

   if (equal_ustring(command, "submit")) {
      submit_elog();
      return;
   }

   if (equal_ustring(command, "query")) {
      show_elog_query();
      return;
   }

   if (equal_ustring(command, "submit query")) {
      show_elog_submit_query(0);
      return;
   }

   if (strncmp(command, "Last ", 5) == 0) {
      if (command[strlen(command) - 1] == 'h')
         sprintf(str, "last%d", atoi(command + 5));
      else if (command[strlen(command) - 1] == 'd')
         sprintf(str, "last%d", atoi(command + 5) * 24);

      redirect(str);
      return;
   }

   if (equal_ustring(command, "delete")) {
      show_elog_delete(path);
      return;
   }

   if (strncmp(path, "last", 4) == 0) {
      show_elog_submit_query(atoi(path + 4));
      return;
   }

   if (equal_ustring(command, "runlog")) {
      sprintf(str, "runlog.txt");
      redirect(str);
      return;
   }

  /*---- check if file requested -----------------------------------*/

   if (strlen(path) > 13 && path[6] == '_' && path[13] == '_') {
      cm_get_experiment_database(&hDB, NULL);
      file_name[0] = 0;
      if (hDB > 0) {
         size = sizeof(file_name);
         memset(file_name, 0, size);

         status =
             db_get_value(hDB, 0, "/Logger/Elog dir", file_name, &size, TID_STRING,
                          FALSE);
         if (status != DB_SUCCESS)
            db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING, TRUE);

         if (file_name[0] != 0)
            if (file_name[strlen(file_name) - 1] != DIR_SEPARATOR)
               strlcat(file_name, DIR_SEPARATOR_STR, sizeof(file_name));
      }
      strlcat(file_name, path, sizeof(file_name));

      fh = open(file_name, O_RDONLY | O_BINARY);
      if (fh > 0) {
         lseek(fh, 0, SEEK_END);
         length = TELL(fh);
         lseek(fh, 0, SEEK_SET);

         rsprintf("HTTP/1.0 200 Document follows\r\n");
         rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
         rsprintf("Accept-Ranges: bytes\r\n");

         /* return proper header for file type */
         for (i = 0; i < (int) strlen(path); i++)
            str[i] = toupper(path[i]);
         str[i] = 0;

         for (i = 0; filetype[i].ext[0]; i++)
            if (strstr(str, filetype[i].ext))
               break;

         if (filetype[i].ext[0])
            rsprintf("Content-Type: %s\r\n", filetype[i].type);
         else if (strchr(str, '.') == NULL)
            rsprintf("Content-Type: text/plain\r\n");
         else
            rsprintf("Content-Type: application/octet-stream\r\n");

         rsprintf("Content-Length: %d\r\n\r\n", length);

         /* return if file too big */
         if (length > (int) (return_size - strlen(return_buffer))) {
            printf("return buffer too small\n");
            close(fh);
            return;
         }

         return_length = strlen(return_buffer) + length;
         read(fh, return_buffer + strlen(return_buffer), length);

         close(fh);
      }

      return;
   }

  /*---- check if runlog is requested ------------------------------*/

   if (path[0] > '9') {
      show_rawfile(path);
      return;
   }

  /*---- check next/previous message -------------------------------*/

   last_message = first_message = FALSE;
   if (equal_ustring(command, "next") || equal_ustring(command, "previous")
       || equal_ustring(command, "last")) {
      strlcpy(orig_path, path, sizeof(orig_path));

      if (equal_ustring(command, "last"))
         path[0] = 0;

      do {
         strlcat(path, equal_ustring(command, "next") ? "+1" : "-1", path_size);
         status = el_search_message(path, &fh, TRUE);
         close(fh);
         if (status != EL_SUCCESS) {
            if (equal_ustring(command, "next"))
               last_message = TRUE;
            else
               first_message = TRUE;
            strlcpy(path, orig_path, path_size);
            break;
         }

         size = sizeof(text);
         el_retrieve(path, date, &run, author, type, system, subject,
                     text, &size, orig_tag, reply_tag, attachment[0], attachment[1],
                     attachment[2], encoding);

         if (strchr(author, '@'))
            *strchr(author, '@') = 0;
         if (*getparam("lauthor") == '1' && !equal_ustring(getparam("author"), author))
            continue;
         if (*getparam("ltype") == '1' && !equal_ustring(getparam("type"), type))
            continue;
         if (*getparam("lsystem") == '1' && !equal_ustring(getparam("system"), system))
            continue;
         if (*getparam("lsubject") == '1') {
            strlcpy(str, getparam("subject"), sizeof(str));
            for (i = 0; i < (int) strlen(str); i++)
               str[i] = toupper(str[i]);
            for (i = 0; i < (int) strlen(subject); i++)
               subject[i] = toupper(subject[i]);

            if (strstr(subject, str) == NULL)
               continue;
         }

         sprintf(str, "%s", path);

         if (*getparam("lauthor") == '1') {
            if (strchr(str, '?') == NULL)
               strlcat(str, "?lauthor=1", sizeof(str));
            else
               strlcat(str, "&lauthor=1", sizeof(str));
         }

         if (*getparam("ltype") == '1') {
            if (strchr(str, '?') == NULL)
               strlcat(str, "?ltype=1", sizeof(str));
            else
               strlcat(str, "&ltype=1", sizeof(str));
         }

         if (*getparam("lsystem") == '1') {
            if (strchr(str, '?') == NULL)
               strlcat(str, "?lsystem=1", sizeof(str));
            else
               strlcat(str, "&lsystem=1", sizeof(str));
         }

         if (*getparam("lsubject") == '1') {
            if (strchr(str, '?') == NULL)
               strlcat(str, "?lsubject=1", sizeof(str));
            else
               strlcat(str, "&lsubject=1", sizeof(str));
         }

         redirect(str);
         return;

      } while (TRUE);
   }

   /*---- get current message ---------------------------------------*/

   size = sizeof(text);
   strlcpy(str, path, sizeof(str));
   subject[0] = 0;
   msg_status = el_retrieve(str, date, &run, author, type, system, subject,
                            text, &size, orig_tag, reply_tag,
                            attachment[0], attachment[1], attachment[2], encoding);

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS ELog - %s</title></head>\n", subject);
   rsprintf("<body><form method=\"GET\" action=\"../EL/%s\">\n", str);

   rsprintf("<table cols=2 border=2 cellpadding=2>\n");

   /*---- title row ----*/

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

   rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS Electronic Logbook");
   if (elog_mode)
      rsprintf("<th bgcolor=#A0A0FF>Logbook \"%s\"</tr>\n", str);
   else
      rsprintf("<th bgcolor=#A0A0FF>Experiment \"%s\"</tr>\n", str);

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
   rsprintf("<input type=submit name=cmd value=New>\n");
   rsprintf("<input type=submit name=cmd value=Edit>\n");
   if (allow_delete)
      rsprintf("<input type=submit name=cmd value=Delete>\n");
   rsprintf("<input type=submit name=cmd value=Reply>\n");
   rsprintf("<input type=submit name=cmd value=Query>\n");

   /* check forms from ODB */
   db_find_key(hDB, 0, "/Elog/Forms", &hkeyroot);
   if (hkeyroot)
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyroot, i, &hkey);
         if (!hkey)
            break;

         db_get_key(hDB, hkey, &key);

         rsprintf("<input type=submit name=form value=\"%s\">\n", key.name);
      }

   rsprintf("<input type=submit name=cmd value=Runlog>\n");

   if (!elog_mode)
      rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("</tr>\n");

   /* "last x" button row */
   rsprintf("<tr><td colspan=2 bgcolor=#D0D0D0>\n");

   db_find_key(hDB, 0, "/Elog/Buttons", &hkeybutton);
   if (hkeybutton == 0) {
      /* create default buttons */
      db_create_key(hDB, 0, "/Elog/Buttons", TID_STRING);
      db_find_key(hDB, 0, "/Elog/Buttons", &hkeybutton);
      assert(hkeybutton);
      db_set_data(hDB, hkeybutton, def_button, sizeof(def_button), 3, TID_STRING);
   }

   db_get_key(hDB, hkeybutton, &key);

   for (i = 0; i < key.num_values; i++) {
      size = sizeof(str);
      db_get_data_index(hDB, hkeybutton, str, &size, i, TID_STRING);
      rsprintf("<input type=submit name=cmd value=\"Last %s\">\n", str);
   }

   rsprintf("</tr>\n");

   rsprintf("<tr><td colspan=2 bgcolor=#E0E0E0>");
   rsprintf("<input type=submit name=cmd value=Next>\n");
   rsprintf("<input type=submit name=cmd value=Previous>\n");
   rsprintf("<input type=submit name=cmd value=Last>\n");
   rsprintf("<i>Check a category to browse only entries from that category</i>\n");
   rsprintf("</tr>\n\n");

   if (msg_status != EL_FILE_ERROR && (reply_tag[0] || orig_tag[0])) {
      rsprintf("<tr><td colspan=2 bgcolor=#F0F0F0>");
      if (orig_tag[0]) {
         sprintf(ref, "/EL/%s", orig_tag);
         rsprintf("  <a href=\"%s\">Original message</a>  ", ref);
      }
      if (reply_tag[0]) {
         sprintf(ref, "/EL/%s", reply_tag);
         rsprintf("  <a href=\"%s\">Reply to this message</a>  ", ref);
      }
      rsprintf("</tr>\n");
   }

  /*---- message ----*/

   if (msg_status == EL_FILE_ERROR)
      rsprintf
          ("<tr><td bgcolor=#FF0000 colspan=2 align=center><h1>No message available</h1></tr>\n");
   else {
      if (last_message)
         rsprintf
             ("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>This is the last message in the ELog</b></tr>\n");

      if (first_message)
         rsprintf
             ("<tr><td bgcolor=#FF0000 colspan=2 align=center><b>This is the first message in the ELog</b></tr>\n");

      /* check for mail submissions */
      for (i = 0;; i++) {
         sprintf(str, "mail%d", i);
         if (*getparam(str)) {
            if (i == 0)
               rsprintf("<tr><td colspan=2 bgcolor=#FFC020>");
            rsprintf("Mail sent to <b>%s</b><br>\n", getparam(str));
         } else
            break;
      }
      if (i > 0)
         rsprintf("</tr>\n");


      if (display_run_number) {
         rsprintf("<tr><td bgcolor=#FFFF00>Entry date: <b>%s</b>", date);

         rsprintf("<td bgcolor=#FFFF00>Run number: <b>%d</b></tr>\n\n", run);
      } else
         rsprintf("<tr><td colspan=2 bgcolor=#FFFF00>Entry date: <b>%s</b></tr>\n\n",
                  date);


      /* define hidded fields */
      strlcpy(str, author, sizeof(str));
      if (strchr(str, '@'))
         *strchr(str, '@') = 0;
      rsprintf("<input type=hidden name=author  value=\"%s\">\n", str);
      rsprintf("<input type=hidden name=type    value=\"%s\">\n", type);
      rsprintf("<input type=hidden name=system  value=\"%s\">\n", system);
      rsprintf("<input type=hidden name=subject value=\"%s\">\n\n", subject);

      if (*getparam("lauthor") == '1')
         rsprintf
             ("<tr><td bgcolor=#FFA0A0><input type=\"checkbox\" checked name=\"lauthor\" value=\"1\">");
      else
         rsprintf
             ("<tr><td bgcolor=#FFA0A0><input type=\"checkbox\" name=\"lauthor\" value=\"1\">");
      rsprintf("  Author: <b>%s</b>\n", author);

      if (*getparam("ltype") == '1')
         rsprintf
             ("<td bgcolor=#FFA0A0><input type=\"checkbox\" checked name=\"ltype\" value=\"1\">");
      else
         rsprintf
             ("<td bgcolor=#FFA0A0><input type=\"checkbox\" name=\"ltype\" value=\"1\">");
      rsprintf("  Type: <b>%s</b></tr>\n", type);

      if (*getparam("lsystem") == '1')
         rsprintf
             ("<tr><td bgcolor=#A0FFA0><input type=\"checkbox\" checked name=\"lsystem\" value=\"1\">");
      else
         rsprintf
             ("<tr><td bgcolor=#A0FFA0><input type=\"checkbox\" name=\"lsystem\" value=\"1\">");

      rsprintf("  System: <b>%s</b>\n", system);

      if (*getparam("lsubject") == '1')
         rsprintf
             ("<td bgcolor=#A0FFA0><input type=\"checkbox\" checked name=\"lsubject\" value=\"1\">");
      else
         rsprintf
             ("<td bgcolor=#A0FFA0><input type=\"checkbox\" name=\"lsubject\" value=\"1\">");
      rsprintf("  Subject: <b>%s</b></tr>\n", subject);


      /* message text */
      rsprintf("<tr><td colspan=2>\n");
      if (equal_ustring(encoding, "plain")) {
         rsputs("<pre>");
         rsputs2(text);
         rsputs("</pre>");
      } else
         rsputs(text);
      rsputs("</tr>\n");

      for (index = 0; index < 3; index++) {
         if (attachment[index][0]) {
            char ref1[256];

            for (i = 0; i < (int) strlen(attachment[index]); i++)
               att[i] = toupper(attachment[index][i]);
            att[i] = 0;

            strlcpy(ref1, attachment[index], sizeof(ref1));
            urlEncode(ref1, sizeof(ref1));
		  
            sprintf(ref, "/EL/%s", ref1);

            if (strstr(att, ".GIF") || strstr(att, ".PNG") || strstr(att, ".JPG")) {
               rsprintf
                   ("<tr><td colspan=2>Attachment: <a href=\"%s\"><b>%s</b></a><br>\n",
                    ref, attachment[index] + 14);
               rsprintf("<img src=\"%s\"></tr>", ref);
            } else {
               rsprintf
                   ("<tr><td colspan=2 bgcolor=#C0C0FF>Attachment: <a href=\"%s\"><b>%s</b></a>\n",
                    ref, attachment[index] + 14);
               if (strstr(att, ".TXT") || strstr(att, ".ASC") || strchr(att, '.') == NULL) {
                  /* display attachment */
                  rsprintf("<br>");
                  if (!strstr(att, ".HTML"))
                     rsprintf("<pre>");

                  file_name[0] = 0;
                  size = sizeof(file_name);
                  memset(file_name, 0, size);
                  db_get_value(hDB, 0, "/Logger/Data dir", file_name, &size, TID_STRING,
                               TRUE);
                  if (file_name[0] != 0)
                     if (file_name[strlen(file_name) - 1] != DIR_SEPARATOR)
                        strlcat(file_name, DIR_SEPARATOR_STR, sizeof(file_name));
                  strlcat(file_name, attachment[index], sizeof(file_name));

                  f = fopen(file_name, "rt");
                  if (f != NULL) {
                     while (!feof(f)) {
                        str[0] = 0;
                        fgets(str, sizeof(str), f);
                        if (!strstr(att, ".HTML"))
                           rsputs2(str);
                        else
                           rsputs(str);
                     }
                     fclose(f);
                  }

                  if (!strstr(att, ".HTML"))
                     rsprintf("</pre>");
                  rsprintf("\n");
               }
               rsprintf("</tr>\n");
            }
         }
      }
   }

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void get_elog_url(char *url, int len)
{
   HNDLE hDB;
   char str[256], str2[256], *p;
   int size;

   /* redirect to external ELOG if URL present */
   cm_get_experiment_database(&hDB, NULL);
   size = sizeof(str);
   if (db_get_value(hDB, 0, "/Elog/URL", str, &size, TID_STRING, FALSE) == DB_SUCCESS) {
      if (str[0] == ':') {
         strcpy(str2, referer);
         while ((p = strrchr(str2, '/')) != NULL && p > str2 && *(p-1) != '/')
            *p = 0;
         if (strrchr(str2+5, ':'))
            *strrchr(str2+5, ':') = 0;
         if (str2[strlen(str2)-1] == '/')
            str2[strlen(str2)-1] = 0;
         sprintf(url, "%s%s", str2, str);
      } else
         strlcpy(url, str, len);
   } else
      strlcpy(url, "EL/", len);
}

/*------------------------------------------------------------------*/

BOOL is_editable(char *eq_name, char *var_name)
{
   HNDLE hDB, hkey;
   KEY key;
   char str[256];
   int i, size;

   cm_get_experiment_database(&hDB, NULL);
   sprintf(str, "/Equipment/%s/Settings/Editable", eq_name);
   db_find_key(hDB, 0, str, &hkey);

   /* if no editable entry found, use default */
   if (!hkey) {
      return (equal_ustring(var_name, "Demand") ||
              equal_ustring(var_name, "Output") || strncmp(var_name, "D_", 2) == 0);
   }

   db_get_key(hDB, hkey, &key);
   for (i = 0; i < key.num_values; i++) {
      size = sizeof(str);
      db_get_data_index(hDB, hkey, str, &size, i, TID_STRING);
      if (equal_ustring(var_name, str))
         return TRUE;
   }
   return FALSE;
}

void show_sc_page(char *path, int refresh)
{
   int i, j, k, colspan, size, n_var, i_edit, i_set;
   char str[256], eq_name[32], group[32], name[32], ref[256];
   char group_name[MAX_GROUPS][32], data[256], back_path[256], *p;
   HNDLE hDB, hkey, hkeyeq, hkeyset, hkeynames, hkeyvar, hkeyroot;
   KEY eqkey, key, varkey;
   char data_str[256], hex_str[256];

   cm_get_experiment_database(&hDB, NULL);

   /* check if variable to edit */
   i_edit = -1;
   if (equal_ustring(getparam("cmd"), "Edit"))
      i_edit = atoi(getparam("index"));

   /* check if variable to set */
   i_set = -1;
   if (equal_ustring(getparam("cmd"), "Set"))
      i_set = atoi(getparam("index"));

   /* split path into equipment and group */
   strlcpy(eq_name, path, sizeof(eq_name));
   strlcpy(group, "All", sizeof(group));
   if (strchr(eq_name, '/')) {
      strlcpy(group, strchr(eq_name, '/') + 1, sizeof(group));
      *strchr(eq_name, '/') = 0;
   }

   back_path[0] = 0;
   for (p=path ; *p ; p++)
      if (*p == '/')
         strlcat(back_path, "../", sizeof(back_path));

   /* check for "names" in settings */
   if (eq_name[0]) {
      sprintf(str, "/Equipment/%s/Settings", eq_name);
      db_find_key(hDB, 0, str, &hkeyset);
      hkeynames = 0;
      if (hkeyset) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkeyset, i, &hkeynames);

            if (!hkeynames)
               break;

            db_get_key(hDB, hkeynames, &key);

            if (strncmp(key.name, "Names", 5) == 0)
               break;
         }
      }

      /* redirect if no names found */
      if (!hkeyset || !hkeynames) {
         /* redirect */
         sprintf(str, "../Equipment/%s/Variables", eq_name);
         redirect(str);
         return;
      }
   }

   sprintf(str, "%s", group);
   show_header(hDB, "MIDAS slow control", "GET", str, 8, i_edit == -1 ? refresh : 0);

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=15 bgcolor=#C0C0C0>\n");

   if (equal_ustring(getparam("cmd"), "Edit"))
      rsprintf("<input type=submit name=cmd value=Set>\n");
   else {
      rsprintf("<input type=submit name=cmd value=ODB>\n");
      rsprintf("<input type=submit name=cmd value=Status>\n");
      rsprintf("<input type=submit name=cmd value=Help>\n");
   }
   rsprintf("</tr>\n\n");

   /*---- enumerate SC equipment ----*/

   rsprintf("<tr><td colspan=15 bgcolor=#FFFF00><i>Equipment:</i> &nbsp;&nbsp;\n");

   db_find_key(hDB, 0, "/Equipment", &hkey);
   if (hkey)
      for (i = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hkeyeq);

         if (!hkeyeq)
            break;

         db_get_key(hDB, hkeyeq, &eqkey);

         db_find_key(hDB, hkeyeq, "Settings", &hkeyset);
         if (hkeyset) {
            for (j = 0;; j++) {
               db_enum_link(hDB, hkeyset, j, &hkeynames);

               if (!hkeynames)
                  break;

               db_get_key(hDB, hkeynames, &key);

               if (strncmp(key.name, "Names", 5) == 0) {
                  if (equal_ustring(eq_name, eqkey.name))
                     rsprintf("<b>%s</b> &nbsp;&nbsp;", eqkey.name);
                  else {
                     rsprintf("<a href=\"%s%s\">%s</a> &nbsp;&nbsp;", 
                                 back_path, eqkey.name, eqkey.name);
                  }
                  break;
               }
            }
         }
      }
   rsprintf("</tr>\n");

   if (!eq_name[0]) {
      rsprintf("</table>");
      return;
   }

   /*---- display SC ----*/

   n_var = 0;
   sprintf(str, "/Equipment/%s/Settings/Names", eq_name);
   db_find_key(hDB, 0, str, &hkey);

   if (hkey) {

      /*---- single name array ----*/
      rsprintf("<tr><td colspan=15 bgcolor=#FFFFA0><i>Groups:</i> &nbsp;&nbsp;");

      /* "all" group */
      if (equal_ustring(group, "All"))
         rsprintf("<b>All</b> &nbsp;&nbsp;");
      else
         rsprintf("<a href=\"%s%s/All\">All</a> &nbsp;&nbsp;", back_path, eq_name);

      /* collect groups */

      memset(group_name, 0, sizeof(group_name));
      db_get_key(hDB, hkey, &key);

      for (int level = 0; ; level++) {
         bool next_level = false;
         for (i = 0; i < key.num_values; i++) {
            size = sizeof(str);
            db_get_data_index(hDB, hkey, str, &size, i, TID_STRING);
            
            char *s = strchr(str, '%');
            for (int k=0; s && k<level; k++)
               s = strchr(s+1, '%');

            if (s) {
               *s = 0;
               if (strchr(s+1, '%'))
                   next_level = true;

               //printf("try group [%s] name [%s], level %d, %d\n", str, s+1, level, next_level);

               for (j = 0; j < MAX_GROUPS; j++) {
                  if (equal_ustring(group_name[j], str) || group_name[j][0] == 0)
                     break;
               }
               if (group_name[j][0] == 0)
                  strlcpy(group_name[j], str, sizeof(group_name[0]));
            }
         }

         if (!next_level)
            break;
      }

      for (i = 0; i < MAX_GROUPS && group_name[i][0]; i++) {
         if (equal_ustring(group_name[i], group))
            rsprintf("<b>%s</b> &nbsp;&nbsp;", group_name[i]);
         else {
            char s[256];
            strlcpy(s, group_name[i], sizeof(s));
            urlEncode(s, sizeof(s));
            rsprintf("<a href=\"%s%s/%s\">%s</a> &nbsp;&nbsp;", back_path, eq_name,
                        s, group_name[i]);
         }
      }
      rsprintf("</tr>\n");

      /* count variables */
      sprintf(str, "/Equipment/%s/Variables", eq_name);
      db_find_key(hDB, 0, str, &hkeyvar);
      if (!hkeyvar) {
         rsprintf("</table>");
         return;
      }
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyvar, i, &hkey);
         if (!hkey)
            break;
      }

      if (i == 0 || i > 15) {
         rsprintf("</table>");
         return;
      }

      /* title row */
      colspan = 15 - i;
      rsprintf("<tr><th colspan=%d>Names", colspan);

      /* display entries for this group */
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyvar, i, &hkey);

         if (!hkey)
            break;

         db_get_key(hDB, hkey, &key);
         rsprintf("<th>%s", key.name);
      }

      rsprintf("</tr>\n");

      /* data for current group */
      sprintf(str, "/Equipment/%s/Settings/Names", eq_name);
      db_find_key(hDB, 0, str, &hkeyset);
      assert(hkeyset);
      db_get_key(hDB, hkeyset, &key);
      for (i = 0; i < key.num_values; i++) {
         size = sizeof(str);
         db_get_data_index(hDB, hkeyset, str, &size, i, TID_STRING);

         strlcpy(name, str, sizeof(name));

         //printf("group [%s], name [%s], str [%s]\n", group, name, str);

         if (!equal_ustring(group, "All")) {
            // check if name starts with the name of the group we want to display
            char *s = strstr(name, group);
            if (s != name)
               continue;
            if (name[strlen(group)] != '%')
               continue;
         }

         if (strlen(name) < 1)
            sprintf(name, "[%d]", i);

         rsprintf("<tr><td colspan=%d>%s", colspan, name);

         for (j = 0;; j++) {
            db_enum_link(hDB, hkeyvar, j, &hkey);
            if (!hkey)
               break;
            db_get_key(hDB, hkey, &varkey);

            /* check if "variables" array is shorter than the "names" array */
            if (i >= varkey.num_values)
               continue;

            size = sizeof(data);
            db_get_data_index(hDB, hkey, data, &size, i, varkey.type);
            db_sprintf(str, data, varkey.item_size, 0, varkey.type);

            if (is_editable(eq_name, varkey.name)) {
               if (n_var == i_set) {
                  /* set value */
                  strlcpy(str, getparam("value"), sizeof(str));
                  db_sscanf(str, data, &size, 0, varkey.type);
                  db_set_data_index(hDB, hkey, data, size, i, varkey.type);

                  /* redirect (so that 'reload' does not reset value) */
                  strlen_retbuf = 0;
                  sprintf(str, "%s", group);
                  redirect(str);
                  return;
               }
               if (n_var == i_edit) {
                  rsprintf
                      ("<td align=center><input type=text size=10 maxlenth=80 name=value value=\"%s\">\n",
                       str);
                  rsprintf("<input type=submit size=20 name=cmd value=Set>\n");
                  rsprintf("<input type=hidden name=index value=%d>\n", i_edit);
                  n_var++;
               } else {
                  sprintf(ref, "%s/%s?cmd=Edit&index=%d", eq_name, group, n_var);

                  rsprintf("<td align=center><a href=\"%s%s\">%s</a>", back_path, ref, str);
                  n_var++;
               }
            } else
               rsprintf("<td align=center>%s", str);
         }

         rsprintf("</tr>\n");
      }
   } else {
      /*---- multiple name arrays ----*/
      rsprintf("<tr><td colspan=15 bgcolor=#FFFFA0><i>Groups:</i> ");

      /* "all" group */
      if (equal_ustring(group, "All"))
         rsprintf("<b>All</b> &nbsp;&nbsp;");
      else
         rsprintf("<a href=\"%s%s\">All</a> &nbsp;&nbsp;\n", back_path, eq_name);

      /* groups from Variables tree */

      sprintf(str, "/Equipment/%s/Variables", eq_name);
      db_find_key(hDB, 0, str, &hkeyvar);
      assert(hkeyvar);

      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyvar, i, &hkey);

         if (!hkey)
            break;

         db_get_key(hDB, hkey, &key);

         if (equal_ustring(key.name, group))
            rsprintf("<b>%s</b> &nbsp;&nbsp;", key.name);
         else
            rsprintf("<a href=\"%s%s/%s\">%s</a> &nbsp;&nbsp;\n", back_path, 
                        eq_name, key.name, key.name);
      }

      rsprintf("</tr>\n");

      /* enumerate variable arrays */

      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyvar, i, &hkey);

         if (!hkey)
            break;

         db_get_key(hDB, hkey, &varkey);

         if (!equal_ustring(group, "All") && !equal_ustring(varkey.name, group))
            continue;

         /* title row */
         rsprintf("<tr><th colspan=9>Names<th>%s</tr>\n", varkey.name);

         if (varkey.type == TID_KEY) {
            hkeyroot = hkey;

            /* enumerate subkeys */
            for (j = 0;; j++) {
               db_enum_key(hDB, hkeyroot, j, &hkey);
               if (!hkey)
                  break;
               db_get_key(hDB, hkey, &key);

               if (key.type == TID_KEY) {
                  /* for keys, don't display data value */
                  rsprintf("<tr><td colspan=9>%s<br></tr>\n", key.name);
               } else {
                  /* display single value */
                  if (key.num_values == 1) {
                     size = sizeof(data);
                     db_get_data(hDB, hkey, data, &size, key.type);
                     db_sprintf(data_str, data, key.item_size, 0, key.type);
                     db_sprintfh(hex_str, data, key.item_size, 0, key.type);

                     if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>")) {
                        strcpy(data_str, "(empty)");
                        hex_str[0] = 0;
                     }

                     if (strcmp(data_str, hex_str) != 0 && hex_str[0])
                        rsprintf
                            ("<tr><td colspan=9>%s<td align=center>%s (%s)<br></tr>\n",
                             key.name, data_str, hex_str);
                     else
                        rsprintf("<tr><td colspan=9>%s<td align=center>%s<br></tr>\n",
                                 key.name, data_str);
                  } else {
                     /* display first value */
                     rsprintf("<tr><td colspan=9 rowspan=%d>%s\n", key.num_values,
                              key.name);

                     for (k = 0; k < key.num_values; k++) {
                        size = sizeof(data);
                        db_get_data_index(hDB, hkey, data, &size, k, key.type);
                        db_sprintf(data_str, data, key.item_size, 0, key.type);
                        db_sprintfh(hex_str, data, key.item_size, 0, key.type);

                        if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>")) {
                           strcpy(data_str, "(empty)");
                           hex_str[0] = 0;
                        }

                        if (k > 0)
                           rsprintf("<tr>");

                        if (strcmp(data_str, hex_str) != 0 && hex_str[0])
                           rsprintf("<td>[%d] %s (%s)<br></tr>\n", k, data_str, hex_str);
                        else
                           rsprintf("<td>[%d] %s<br></tr>\n", k, data_str);
                     }
                  }
               }
            }
         } else {
            /* data for current group */
            sprintf(str, "/Equipment/%s/Settings/Names %s", eq_name, varkey.name);
            db_find_key(hDB, 0, str, &hkeyset);
            if (hkeyset)
               db_get_key(hDB, hkeyset, &key);

            if (varkey.num_values > 1000)
               rsprintf("<tr><td colspan=9>%s<td align=center><i>... %d values ...</i>",
                        varkey.name, varkey.num_values);
            else {
               for (j = 0; j < varkey.num_values; j++) {
                  if (hkeyset && j<key.num_values) {
                     size = sizeof(name);
                     db_get_data_index(hDB, hkeyset, name, &size, j, TID_STRING);
                  } else
                     sprintf(name, "%s[%d]", varkey.name, j);

                  if (strlen(name) < 1)
                     sprintf(name, "%s[%d]", varkey.name, j);

                  rsprintf("<tr><td colspan=9>%s", name);

                  size = sizeof(data);
                  db_get_data_index(hDB, hkey, data, &size, j, varkey.type);
                  db_sprintf(str, data, varkey.item_size, 0, varkey.type);

                  if (is_editable(eq_name, varkey.name)) {
                     if (n_var == i_set) {
                        /* set value */
                        strlcpy(str, getparam("value"), sizeof(str));
                        db_sscanf(str, data, &size, 0, varkey.type);
                        db_set_data_index(hDB, hkey, data, size, j, varkey.type);

                        /* redirect (so that 'reload' does not reset value) */
                        strlen_retbuf = 0;
                        sprintf(str, "%s", group);
                        redirect(str);
                        return;
                     }
                     if (n_var == i_edit) {
                        rsprintf
                            ("<td align=center><input type=text size=10 maxlenth=80 name=value value=\"%s\">\n",
                             str);
                        rsprintf("<input type=submit size=20 name=cmd value=Set></tr>\n");
                        rsprintf("<input type=hidden name=index value=%d>\n", i_edit);
                        rsprintf("<input type=hidden name=cmd value=Set>\n");
                        n_var++;
                     } else {
                        sprintf(ref, "%s%s/%s?cmd=Edit&index=%d", back_path, 
                                eq_name, group, n_var);

                        rsprintf("<td align=center><a href=\"%s\">%s</a>\n", ref, str);
                        n_var++;
                     }

                  } else
                     rsprintf("<td align=center>%s\n", str);
               }
            }

            rsprintf("</tr>\n");
         }
      }
   }

   rsprintf("</table></form>\r\n");
}

/*------------------------------------------------------------------*/

char *find_odb_tag(char *p, char *path, char *format, int *edit, char *type, char *pwd, char *tail)
{
   char str[256], *ps, *pt;
   BOOL in_script;

   *edit = 0;
   *tail = 0;
   *format = 0;
   pwd[0] = 0;
   in_script = FALSE;
   strcpy(type, "text");
   do {
      while (*p && *p != '<')
         p++;

      /* return if end of string reached */
      if (!*p)
         return NULL;

      p++;
      while (*p && ((*p == ' ') || iscntrl(*p)))
         p++;

      strncpy(str, p, 6);
      str[6] = 0;
      if (equal_ustring(str, "script"))
         in_script = TRUE;

      strncpy(str, p, 7);
      str[7] = 0;
      if (equal_ustring(str, "/script"))
         in_script = FALSE;

      strncpy(str, p, 4);
      str[4] = 0;
      if (equal_ustring(str, "odb ")) {
         ps = p - 1;
         p += 4;
         while (*p && ((*p == ' ') || iscntrl(*p)))
            p++;

         do {
            strncpy(str, p, 7);
            str[7] = 0;
            if (equal_ustring(str, "format=")) {
               p += 7;
               if (*p == '\"') {
                  p++;
                  while (*p && *p != '\"')
                     *format++ = *p++;
                  *format = 0;
                  if (*p == '\"')
                    p++;
               } else {
                  while (*p && *p != ' ' && *p != '>')
                     *format++ = *p++;
                  *format = 0;
               }

            } else {

            strncpy(str, p, 4);
            str[4] = 0;
            if (equal_ustring(str, "src=")) {
               p += 4;
               if (*p == '\"') {
                  p++;
                  while (*p && *p != '\"')
                     *path++ = *p++;
                  *path = 0;
                  if (*p == '\"')
                    p++;
               } else {
                  while (*p && *p != ' ' && *p != '>')
                     *path++ = *p++;
                  *path = 0;
               }
            } else {

               if (in_script)
                  break;

               strncpy(str, p, 5);
               str[5] = 0;
               if (equal_ustring(str, "edit=")) {
                  p += 5;

                  if (*p == '\"') {
                     p++;
                     *edit = atoi(p);
                     if (*p == '\"')
                       p++;
                  } else {
                     *edit = atoi(p);
                     while (*p && *p != ' ' && *p != '>')
                        p++;
                  }

               } else {

                  strncpy(str, p, 5);
                  str[5] = 0;
                  if (equal_ustring(str, "type=")) {
                     p += 5;
                     if (*p == '\"') {
                        p++;
                        while (*p && *p != '\"')
                           *type++ = *p++;
                        *type = 0;
                        if (*p == '\"')
                          p++;
                     } else {
                        while (*p && *p != ' ' && *p != '>')
                           *type++ = *p++;
                        *type = 0;
                     }
                  } else {
                     strncpy(str, p, 4);
                     str[4] = 0;
                     if (equal_ustring(str, "pwd=")) {
                        p += 4;
                        if (*p == '\"') {
                           p++;
                           while (*p && *p != '\"')
                              *pwd++ = *p++;
                           *pwd = 0;
                           if (*p == '\"')
                             p++;
                        } else {
                           while (*p && *p != ' ' && *p != '>')
                              *pwd++ = *p++;
                           *pwd = 0;
                        }
                     } else {
                        if (strchr(p, '=')) {
                           strlcpy(str, p, sizeof(str));
                           pt = strchr(str, '=')+1;
                           if (*pt == '\"') {
                              pt++;
                              while (*pt && *pt != '\"')
                                 pt++;
                              if (*pt == '\"')
                                 pt++;
                              *pt = 0;
                           } else {
                              while (*pt && *pt != ' ' && *pt != '>')
                                 pt++;
                              *pt = 0;
                           }
                           if (tail[0]) {
                              strlcat(tail, " ", 256);
                              strlcat(tail, str, 256);
                           } else {
                              strlcat(tail, str, 256);
                           }
                           p += strlen(str);
                        }
                     }
                  }
               }
            }
            }

            while (*p && ((*p == ' ') || iscntrl(*p)))
               p++;

            if (*p == '<') {
               cm_msg(MERROR, "find_odb_tag", "Invalid odb tag '%s'", ps);
               return NULL;
            }
         } while (*p != '>');

         return ps;
      }

      while (*p && *p != '>')
         p++;

   } while (1);

}

/*------------------------------------------------------------------*/

void show_odb_tag(const char *path, const char *keypath1, const char *format, int n_var, int edit, char *type, char *pwd, char *tail)
{
   int size, index, i_edit, i_set;
   char str[TEXT_SIZE], data[TEXT_SIZE], options[1000], full_keypath[256], keypath[256], *p;
   HNDLE hDB, hkey;
   KEY key;

   /* check if variable to edit */
   i_edit = -1;
   if (equal_ustring(getparam("cmd"), "Edit"))
      i_edit = atoi(getparam("index"));

   /* check if variable to set */
   i_set = -1;
   if (equal_ustring(getparam("cmd"), "Set"))
      i_set = atoi(getparam("index"));

   /* check if path contains index */
   strlcpy(full_keypath, keypath1, sizeof(full_keypath));
   strlcpy(keypath, keypath1, sizeof(keypath));
   index = 0;

   if (strchr(keypath, '[') && strchr(keypath, ']')) {
      for (p = strchr(keypath, '[') + 1; *p && *p != ']'; p++)
         if (!isdigit(*p))
            break;

      if (*p && *p == ']') {
         index = atoi(strchr(keypath, '[') + 1);
         *strchr(keypath, '[') = 0;
      }
   }

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, keypath, &hkey);
   if (!hkey)
      rsprintf("<b>Key \"%s\" not found in ODB</b>\n", keypath);
   else {
      db_get_key(hDB, hkey, &key);
      size = sizeof(data);
      db_get_data_index(hDB, hkey, data, &size, index, key.type);

      if (format && strlen(format)>0)
         db_sprintff(str, format, data, key.item_size, 0, key.type);
      else
         db_sprintf(str, data, key.item_size, 0, key.type);

      if (equal_ustring(type, "checkbox")) {

         if (isparam("cbi"))
            i_set = atoi(getparam("cbi"));
         if (n_var == i_set) {
            /* toggle state */
            if (key.type == TID_BOOL) {
               if (str[0] == 'y')
                  strcpy(str, "n");
               else
                  strcpy(str, "y");
            } else {
               if (atoi(str) > 0)
                  strcpy(str, "0");
               else
                  strcpy(str, "1");
            }

            db_sscanf(str, data, &size, 0, key.type);
            db_set_data_index(hDB, hkey, data, size, index, key.type);
         }

         options[0] = 0;
         if (str[0] == 'y' || atoi(str) > 0)
            strcat(options, "checked ");
         if (!edit)
            strcat(options, "disabled ");
         else {
            if (edit == 1) {
               strlcat(options, "onClick=\"o=document.createElement('input');o.type='hidden';o.name='cbi';o.value='", sizeof(options));
               sprintf(options+strlen(options), "%d", n_var);
               strlcat(options, "';document.form1.appendChild(o);", sizeof(options));
               strlcat(options, "document.form1.submit();\" ", sizeof(options));
            }
         }

         if (tail[0])
            strlcat(options, tail, sizeof(options));

         rsprintf("<input type=\"checkbox\" %s>\n", options);
      
      } else { // checkbox
      
         if (edit == 1) {
            if (n_var == i_set) {
               /* set value */
               strlcpy(str, getparam("value"), sizeof(str));
               db_sscanf(str, data, &size, 0, key.type);
               db_set_data_index(hDB, hkey, data, size, index, key.type);

               /* read back value */
               size = sizeof(data);
               db_get_data_index(hDB, hkey, data, &size, index, key.type);
               db_sprintf(str, data, key.item_size, 0, key.type);
            }

            if (n_var == i_edit) {
               rsprintf("<input type=text size=10 maxlength=80 name=value value=\"%s\">\n",
                        str);
               rsprintf("<input type=submit size=20 name=cmd value=Set>\n");
               rsprintf("<input type=hidden name=index value=%d>\n", n_var);
               rsprintf("<input type=hidden name=cmd value=Set>\n");
            } else {
               if (edit == 2) {
                  /* edit handling through user supplied JavaScript */
                  rsprintf("<a href=\"#\" %s>", tail);
               } else {
                  /* edit handling through form submission */
                  if (pwd[0]) {
                     rsprintf("<a onClick=\"promptpwd('%s?cmd=Edit&index=%d&pnam=%s')\" href=\"#\">", path, n_var, pwd);
                  } else {
                     rsprintf("<a href=\"%s?cmd=Edit&index=%d\" %s>", path, n_var, tail);
                  }
               }

               rsputs(str);
               rsprintf("</a>");
            }
         } else if (edit == 2) {
            rsprintf("<a href=\"#\" onclick=\"ODBEdit('%s')\">\n", full_keypath);
            rsputs(str);
            rsprintf("</a>");
         }
           else
            rsputs(str);
      }
   }
}

/*------------------------------------------------------------------*/

/* add labels using following syntax under /Custom/Images/<name.gif>/Labels/<name>:
   
   [Name]    [Description]                       [Example]

   Src       ODB path for vairable to display    /Equipment/Environment/Variables/Input[0]
   Format    Formt for float/double              %1.2f Deg. C
   Font      Font to use                         small | medium | giant
   X         X-position in pixel                 90
   Y         Y-position from top                 67
   Align     horizontal align left/center/right  left
   FGColor   Foreground color RRGGBB             000000
   BGColor   Background color RRGGBB             FFFFFF
*/

const char *cgif_label_str[] = {
   "Src = STRING : [256] ",
   "Format = STRING : [32] %1.1f",
   "Font = STRING : [32] Medium",
   "X = INT : 0",
   "Y = INT : 0",
   "Align = INT : 0",
   "FGColor = STRING : [8] 000000",
   "BGColor = STRING : [8] FFFFFF",
   NULL
};

typedef struct {
   char src[256];
   char format[32];
   char font[32];
   int x, y, align;
   char fgcolor[8];
   char bgcolor[8];
} CGIF_LABEL;

/* add labels using following syntax under /Custom/Images/<name.gif>/Bars/<name>:
   
   [Name]    [Description]                       [Example]

   Src       ODB path for vairable to display    /Equipment/Environment/Variables/Input[0]
   X         X-position in pixel                 90
   Y         Y-position from top                 67
   Width     Width in pixel                      20
   Height    Height in pixel                     100
   Direction 0(vertical)/1(horiz.)               0
   Axis      Draw axis 0(none)/1(left)/2(right)  1
   Logscale  Draw logarithmic axis               n
   Min       Min value for axis                  0
   Max       Max value for axis                  10
   FGColor   Foreground color RRGGBB             000000
   BGColor   Background color RRGGBB             FFFFFF
   BDColor   Border color RRGGBB                 808080
*/

const char *cgif_bar_str[] = {
   "Src = STRING : [256] ",
   "X = INT : 0",
   "Y = INT : 0",
   "Width = INT : 10",
   "Height = INT : 100",
   "Direction = INT : 0",
   "Axis = INT : 1",
   "Logscale = BOOL : n",
   "Min = DOUBLE : 0",
   "Max = DOUBLE : 10",
   "FGColor = STRING : [8] 000000",
   "BGColor = STRING : [8] FFFFFF",
   "BDColor = STRING : [8] 808080",
   NULL
};

typedef struct {
   char src[256];
   int x, y, width, height, direction, axis;
   BOOL logscale;
   double min, max;
   char fgcolor[8];
   char bgcolor[8];
   char bdcolor[8];
} CGIF_BAR;

/*------------------------------------------------------------------*/

int evaluate_src(char *key, char *src, double *fvalue)
{
   HNDLE hDB, hkeyval;
   KEY vkey;
   int i, n, size, status, ivalue;
   char str[256], data[256], value[256];

   cm_get_experiment_database(&hDB, NULL);

   /* separate source from operators */
   for (i=0 ; i<(int)strlen(src) ; i++)
      if (src[i] == '>' || src[i] == '&')
         break;
   strncpy(str, src, i);
   str[i] = 0;

   /* strip trailing blanks */
   while (strlen(str) > 0 && str[strlen(str)-1] == ' ')
      str[strlen(str)-1] = 0;

   db_find_key(hDB, 0, str, &hkeyval);
   if (!hkeyval) {
      cm_msg(MERROR, "evaluate_src", "Invalid Src key \"%s\" for Fill \"%s\"",
             src, key);
      return 0;
   }

   db_get_key(hDB, hkeyval, &vkey);
   size = sizeof(data);
   status = db_get_value(hDB, 0, src, data, &size, vkey.type, FALSE);
   db_sprintf(value, data, size, 0, vkey.type);
   if (equal_ustring(value, "NAN"))
      return 0;
   
   if (vkey.type == TID_BOOL) {
      *fvalue = (value[0] == 'y');
   } else
      *fvalue = atof(value);

   /* evaluate possible operators */
   do {
      if (src[i] == '>' && src[i+1] == '>') {
         i+=2;
         n = atoi(src+i);
         while (src[i] == ' ' || isdigit(src[i]))
            i++;
         ivalue = (int)*fvalue;
         ivalue >>= n;
         *fvalue = ivalue;
      }

      if (src[i] == '&') {
         i+=1;
         while (src[i] == ' ')
            i++;
         if (src[i] == '0' && src[i+1] == 'x')
            sscanf(src+2+i, "%x", &n);
         else
            n = atoi(src+i);
         while (src[i] == ' ' || isxdigit(src[i]) || src[i] == 'x')
            i++;
         ivalue = (int)*fvalue;
         ivalue &= n;
         *fvalue = ivalue;
      }

   } while (src[i]);

   return 1;
}

/*------------------------------------------------------------------*/

void show_custom_gif(const char *name)
{
   char str[256], filename[256], data[256], value[256], src[256], custom_path[256],
      full_filename[256];
   int i, fh, index, length, status, size, width, height, bgcol, fgcol, bdcol, r, g, b, x, y;
   HNDLE hDB, hkeygif, hkeyroot, hkey, hkeyval;
   double fvalue, ratio;
   KEY key, vkey;
   gdImagePtr im;
   gdGifBuffer gb;
   gdFontPtr pfont;
   FILE *f;
   CGIF_LABEL label;
   CGIF_BAR bar;

   cm_get_experiment_database(&hDB, NULL);

   custom_path[0] = 0;
   size = sizeof(custom_path);
   db_get_value(hDB, 0, "/Custom/Path", custom_path, &size, TID_STRING, FALSE);

   /* find image description in ODB */
   sprintf(str, "/Custom/Images/%s", name);
   db_find_key(hDB, 0, str, &hkeygif);
   if (!hkeygif) {

      /* check for file */
      strlcpy(filename, custom_path, sizeof(str));
      if (filename[strlen(filename)-1] != DIR_SEPARATOR)
         strlcat(filename, DIR_SEPARATOR_STR, sizeof(filename));
      strlcat(filename, name, sizeof(filename));
      fh = open(filename, O_RDONLY | O_BINARY);
      if (fh < 0) {
         sprintf(str, "Cannot open file \"%s\" and cannot find ODB key \"/Custom/Images/%s\"", filename, name);
         show_error(str);
         return;
      }

      size = lseek(fh, 0, SEEK_END);
      lseek(fh, 0, SEEK_SET);

      /* return GIF image */
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());

      rsprintf("Content-Type: image/gif\r\n");
      rsprintf("Content-Length: %d\r\n", size);
      rsprintf("Pragma: no-cache\r\n");
      rsprintf("Expires: Fri, 01-Jan-1983 00:00:00 GMT\r\n\r\n");

      if (size > (int) (return_size - strlen(return_buffer))) {
         printf("return buffer too small\n");
         return;
      }

      return_length = strlen(return_buffer) + size;
      read(fh, return_buffer + strlen(return_buffer), size);
      close(fh);
      return;
   }

   /* load background image */
   size = sizeof(filename);
   db_get_value(hDB, hkeygif, "Background", filename, &size, TID_STRING, FALSE);

   strlcpy(full_filename, custom_path, sizeof(str));
   if (full_filename[strlen(full_filename)-1] != DIR_SEPARATOR)
      strlcat(full_filename, DIR_SEPARATOR_STR, sizeof(full_filename));
   strlcat(full_filename, filename, sizeof(full_filename));

   f = fopen(full_filename, "rb");
   if (f == NULL) {
      sprintf(str, "Cannot open file \"%s\"", full_filename);
      show_error(str);
      return;
   }

   im = gdImageCreateFromGif(f);
   fclose(f);

   if (im == NULL) {
      sprintf(str, "File \"%s\" is not a GIF image", filename);
      show_error(str);
      return;
   }

   /*---- draw labels ----------------------------------------------*/

   db_find_key(hDB, hkeygif, "Labels", &hkeyroot);
   if (hkeyroot) {
      for (index = 0;; index++) {
         db_enum_key(hDB, hkeyroot, index, &hkey);
         if (!hkey)
            break;
         db_get_key(hDB, hkey, &key);

         db_check_record(hDB, hkey, "", strcomb(cgif_label_str), TRUE);
         size = sizeof(label);
         status = db_get_record(hDB, hkey, &label, &size, 0);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "show_custom_gif", "Cannot open data record for label \"%s\"",
                   key.name);
            continue;
         }

         if (label.src[0] == 0) {
            cm_msg(MERROR, "show_custom_gif", "Empty Src key for label \"%s\"", key.name);
            continue;
         }

         db_find_key(hDB, 0, label.src, &hkeyval);
         if (!hkeyval) {
            cm_msg(MERROR, "show_custom_gif", "Invalid Src key \"%s\" for label \"%s\"",
                   label.src, key.name);
            continue;
         }

         db_get_key(hDB, hkeyval, &vkey);
         size = sizeof(data);
         status = db_get_value(hDB, 0, label.src, data, &size, vkey.type, FALSE);

         if (label.format[0]) {
            if (vkey.type == TID_FLOAT)
               sprintf(value, label.format, *(((float *) data)));
            else if (vkey.type == TID_DOUBLE)
               sprintf(value, label.format, *(((double *) data)));
            else if (vkey.type == TID_INT)
               sprintf(value, label.format, *(((INT *) data)));
            else if (vkey.type == TID_BOOL) {
               if (strstr(label.format, "%c"))
                  sprintf(value, label.format, *(((INT *) data)) ? 'y' : 'n');
               else
                  sprintf(value, label.format, *(((INT *) data)));
            } else
               db_sprintf(value, data, size, 0, vkey.type);
         } else
            db_sprintf(value, data, size, 0, vkey.type);

         sscanf(label.fgcolor, "%02x%02x%02x", &r, &g, &b);
         fgcol = gdImageColorAllocate(im, r, g, b);
         if (fgcol == -1)
            fgcol = gdImageColorClosest(im, r, g, b);

         sscanf(label.bgcolor, "%02x%02x%02x", &r, &g, &b);
         bgcol = gdImageColorAllocate(im, r, g, b);
         if (bgcol == -1)
            bgcol = gdImageColorClosest(im, r, g, b);

         /* select font */
         if (equal_ustring(label.font, "Small"))
            pfont = gdFontSmall;
         else if (equal_ustring(label.font, "Medium"))
            pfont = gdFontMediumBold;
         else if (equal_ustring(label.font, "Giant"))
            pfont = gdFontGiant;
         else
            pfont = gdFontMediumBold;

         width = strlen(value) * pfont->w + 5 + 5;
         height = pfont->h + 2 + 2;

         if (label.align == 0) {
            /* left */
            gdImageFilledRectangle(im, label.x, label.y, label.x + width,
                                   label.y + height, bgcol);
            gdImageRectangle(im, label.x, label.y, label.x + width, label.y + height,
                             fgcol);
            gdImageString(im, pfont, label.x + 5, label.y + 2, value, fgcol);
         } else if (label.align == 1) {
            /* center */
            gdImageFilledRectangle(im, label.x - width / 2, label.y, label.x + width / 2,
                                   label.y + height, bgcol);
            gdImageRectangle(im, label.x - width / 2, label.y, label.x + width / 2,
                             label.y + height, fgcol);
            gdImageString(im, pfont, label.x + 5 - width / 2, label.y + 2, value, fgcol);
         } else {
            /* right */
            gdImageFilledRectangle(im, label.x - width, label.y, label.x,
                                   label.y + height, bgcol);
            gdImageRectangle(im, label.x - width, label.y, label.x, label.y + height,
                             fgcol);
            gdImageString(im, pfont, label.x - width + 5, label.y + 2, value, fgcol);
         }
      }
   }

   /*---- draw bars ------------------------------------------------*/

   db_find_key(hDB, hkeygif, "Bars", &hkeyroot);
   if (hkeyroot) {
      for (index = 0;; index++) {
         db_enum_key(hDB, hkeyroot, index, &hkey);
         if (!hkey)
            break;
         db_get_key(hDB, hkey, &key);

         db_check_record(hDB, hkey, "", strcomb(cgif_bar_str), TRUE);
         size = sizeof(bar);
         status = db_get_record(hDB, hkey, &bar, &size, 0);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "show_custom_gif", "Cannot open data record for bar \"%s\"",
                   key.name);
            continue;
         }

         if (bar.src[0] == 0) {
            cm_msg(MERROR, "show_custom_gif", "Empty Src key for bar \"%s\"", key.name);
            continue;
         }

         db_find_key(hDB, 0, bar.src, &hkeyval);
         if (!hkeyval) {
            cm_msg(MERROR, "show_custom_gif", "Invalid Src key \"%s\" for bar \"%s\"",
                   bar.src, key.name);
            continue;
         }

         db_get_key(hDB, hkeyval, &vkey);
         size = sizeof(data);
         status = db_get_value(hDB, 0, bar.src, data, &size, vkey.type, FALSE);
         db_sprintf(value, data, size, 0, vkey.type);
         if (equal_ustring(value, "NAN"))
            continue;

         fvalue = atof(value);

         sscanf(bar.fgcolor, "%02x%02x%02x", &r, &g, &b);
         fgcol = gdImageColorAllocate(im, r, g, b);
         if (fgcol == -1)
            fgcol = gdImageColorClosest(im, r, g, b);

         sscanf(bar.bgcolor, "%02x%02x%02x", &r, &g, &b);
         bgcol = gdImageColorAllocate(im, r, g, b);
         if (bgcol == -1)
            bgcol = gdImageColorClosest(im, r, g, b);

         sscanf(bar.bdcolor, "%02x%02x%02x", &r, &g, &b);
         bdcol = gdImageColorAllocate(im, r, g, b);
         if (bdcol == -1)
            bdcol = gdImageColorClosest(im, r, g, b);

         if (bar.min == bar.max)
            bar.max += 1;

         if (bar.logscale) {
            if (fvalue < 1E-20)
               fvalue = 1E-20;
            ratio = (log(fvalue) - log(bar.min)) / (log(bar.max) - log(bar.min));
         } else
            ratio = (fvalue - bar.min) / (bar.max - bar.min);
         if (ratio < 0)
            ratio = 0;
         if (ratio > 1)
            ratio = 1;

         if (bar.direction == 0) {
            /* vertical */
            ratio = (bar.height - 2) - ratio * (bar.height - 2);
            r = (int) (ratio + 0.5);

            gdImageFilledRectangle(im, bar.x, bar.y, bar.x + bar.width,
                                   bar.y + bar.height, bgcol);
            gdImageRectangle(im, bar.x, bar.y, bar.x + bar.width, bar.y + bar.height,
                             bdcol);
            gdImageFilledRectangle(im, bar.x + 1, bar.y + r + 1, bar.x + bar.width - 1,
                                   bar.y + bar.height - 1, fgcol);

            if (bar.axis == 1)
               vaxis(im, gdFontSmall, bdcol, 0, bar.x, bar.y + bar.height, bar.height, -3,
                     -5, -7, -8, 0, bar.min, bar.max, bar.logscale);
            else if (bar.axis == 2)
               vaxis(im, gdFontSmall, bdcol, 0, bar.x + bar.width, bar.y + bar.height,
                     bar.height, 3, 5, 7, 10, 0, bar.min, bar.max, bar.logscale);

         } else {
            /* horizontal */
            ratio = ratio * (bar.height - 2);
            r = (int) (ratio + 0.5);

            gdImageFilledRectangle(im, bar.x, bar.y, bar.x + bar.height,
                                   bar.y + bar.width, bgcol);
            gdImageRectangle(im, bar.x, bar.y, bar.x + bar.height, bar.y + bar.width,
                             bdcol);
            gdImageFilledRectangle(im, bar.x + 1, bar.y + 1, bar.x + r,
                                   bar.y + bar.width - 1, fgcol);

            if (bar.axis == 1)
               haxis(im, gdFontSmall, bdcol, 0, bar.x, bar.y, bar.height, -3, -5, -7, -18,
                     0, bar.min, bar.max);
            else if (bar.axis == 2)
               haxis(im, gdFontSmall, bdcol, 0, bar.x, bar.y + bar.width, bar.height, 3,
                     5, 7, 8, 0, bar.min, bar.max);
         }
      }
   }

   /*---- draw fills -----------------------------------------------*/

   db_find_key(hDB, hkeygif, "Fills", &hkeyroot);
   if (hkeyroot) {
      for (index = 0;; index++) {
         db_enum_key(hDB, hkeyroot, index, &hkey);
         if (!hkey)
            break;
         db_get_key(hDB, hkey, &key);

         size = sizeof(src);
         src[0] = 0;
         db_get_value(hDB, hkey, "Src", src, &size, TID_STRING, TRUE);

         if (src[0] == 0) {
            cm_msg(MERROR, "show_custom_gif", "Empty Src key for Fill \"%s\"", key.name);
            continue;
         }

         if (!evaluate_src(key.name, src, &fvalue))
            continue;

         x = y = 0;
         size = sizeof(x);
         db_get_value(hDB, hkey, "X", &x, &size, TID_INT, TRUE);
         db_get_value(hDB, hkey, "Y", &y, &size, TID_INT, TRUE);

         size = sizeof(data);
         status = db_get_value(hDB, hkey, "Limits", data, &size, TID_DOUBLE, FALSE);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "show_custom_gif", "No \"Limits\" entry for Fill \"%s\"",
                   key.name);
            continue;
         }
         for (i = 0; i < size / (int) sizeof(double); i++)
            if (*((double *) data + i) > fvalue)
               break;
         if (i > 0)
            i--;

         db_find_key(hDB, hkey, "Fillcolors", &hkeyval);
         if (!hkeyval) {
            cm_msg(MERROR, "show_custom_gif", "No \"Fillcolors\" entry for Fill \"%s\"",
                   key.name);
            continue;
         }

         size = sizeof(data);
         strcpy(data, "FFFFFF");
         status = db_get_data_index(hDB, hkeyval, data, &size, i, TID_STRING);
         if (status == DB_SUCCESS) {
            sscanf(data, "%02x%02x%02x", &r, &g, &b);
            fgcol = gdImageColorAllocate(im, r, g, b);
            if (fgcol == -1)
               fgcol = gdImageColorClosest(im, r, g, b);
            gdImageFill(im, x, y, fgcol);
         }
      }
   }

   /* generate GIF */
   gdImageInterlace(im, 1);
   gdImageGif(im, &gb);
   gdImageDestroy(im);
   length = gb.size;

   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());

   rsprintf("Content-Type: image/gif\r\n");
   rsprintf("Content-Length: %d\r\n", length);
   rsprintf("Pragma: no-cache\r\n");
   rsprintf("Expires: Fri, 01-Jan-1983 00:00:00 GMT\r\n\r\n");

   if (length > (int) (return_size - strlen(return_buffer))) {
      printf("return buffer too small\n");
      return;
   }

   return_length = strlen(return_buffer) + length;
   memcpy(return_buffer + strlen(return_buffer), gb.data, length);
}

/*------------------------------------------------------------------*/

void show_custom_audio(const char *name)
{
   char str[256], filename[256], custom_path[256];
   int fh, size;
   HNDLE hDB;

   cm_get_experiment_database(&hDB, NULL);
   
   custom_path[0] = 0;
   size = sizeof(custom_path);
   db_get_value(hDB, 0, "/Custom/Path", custom_path, &size, TID_STRING, FALSE);
   
   /* check for file */
   strlcpy(filename, custom_path, sizeof(str));
   if (filename[strlen(filename)-1] != DIR_SEPARATOR)
      strlcat(filename, DIR_SEPARATOR_STR, sizeof(filename));
   strlcat(filename, name, sizeof(filename));
   fh = open(filename, O_RDONLY | O_BINARY);
   if (fh < 0) {
      sprintf(str, "Cannot open file \"%s\" and cannot find ODB key \"/Custom/Images/%s\"", filename, name);
      show_error(str);
      return;
   }
   
   size = lseek(fh, 0, SEEK_END);
   lseek(fh, 0, SEEK_SET);
   
   /* return audio file */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   
   if (strstr(name, ".mp3"))
      rsprintf("Content-Type: audio/mp3\r\n");
   else
      rsprintf("Content-Type: audio/ogg\r\n");
   rsprintf("Content-Length: %d\r\n\r\n", size);
   
   if (size > (int) (return_size - strlen(return_buffer))) {
      printf("return buffer too small\n");
      return;
   }
   
   return_length = strlen(return_buffer) + size;
   read(fh, return_buffer + strlen(return_buffer), size);
   close(fh);
   return;
}

/*------------------------------------------------------------------*/

void do_jrpc_rev0()
{
   static RPC_LIST rpc_list[] = {
      { 9999, "mhttpd_jrpc_rev0", {
            {TID_STRING, RPC_IN}, // arg0
            {TID_STRING, RPC_IN}, // arg1
            {TID_STRING, RPC_IN}, // arg2
            {TID_STRING, RPC_IN}, // arg3
            {TID_STRING, RPC_IN}, // arg4
            {TID_STRING, RPC_IN}, // arg5
            {TID_STRING, RPC_IN}, // arg6
            {TID_STRING, RPC_IN}, // arg7
            {TID_STRING, RPC_IN}, // arg8
            {TID_STRING, RPC_IN}, // arg9
            {0}} },
      { 0 }
   };

   int status, count = 0, substring = 0, rpc;

   const char *xname   = getparam("name");
   const char *srpc    = getparam("rpc");

   if (!srpc || !xname) {
      show_text_header();
      rsprintf("<INVALID_ARGUMENTS>");
      return;
   }

   char sname[256];
   strlcpy(sname, xname, sizeof(sname));

   if (sname[strlen(sname)-1]=='*') {
      sname[strlen(sname)-1] = 0;
      substring = 1;
   }

   rpc = atoi(srpc);

   if (rpc<RPC_MIN_ID || rpc>RPC_MAX_ID) {
      show_text_header();
      rsprintf("<INVALID_RPC_ID>");
      return;
   }

   rpc_list[0].id = rpc;
   status = rpc_register_functions(rpc_list, NULL);

   //printf("cm_register_functions() for format \'%s\' status %d\n", sformat, status);

   show_text_header();
   rsprintf("calling rpc %d | ", rpc);

   if (1) {
      int status, i;
      char str[256];
      HNDLE hDB, hrootkey, hsubkey, hkey;

      cm_get_experiment_database(&hDB, NULL);
      
      /* find client which exports FCNA function */
      status = db_find_key(hDB, 0, "System/Clients", &hrootkey);
      if (status == DB_SUCCESS) {
         for (i=0; ; i++) {
            status = db_enum_key(hDB, hrootkey, i, &hsubkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;
            
            sprintf(str, "RPC/%d", rpc);
            status = db_find_key(hDB, hsubkey, str, &hkey);
            if (status == DB_SUCCESS) {
               char client_name[NAME_LENGTH];
               HNDLE hconn;
               int size;

               size = sizeof(client_name);
               status = db_get_value(hDB, hsubkey, "Name", client_name, &size, TID_STRING, FALSE);
               if (status != DB_SUCCESS)
                  continue;

               if (strlen(sname) > 0) {
                  if (substring) {
                     if (strstr(client_name, sname) != client_name)
                        continue;
                  } else {
                     if (strcmp(sname, client_name) != 0)
                        continue;
                  }
               }

               count++;

               rsprintf("client %s", client_name);

               status = cm_connect_client(client_name, &hconn);
               rsprintf(" %d", status);
      
               if (status == RPC_SUCCESS) {
                  status = rpc_client_call(hconn, rpc,
                                           getparam("arg0"),
                                           getparam("arg1"),
                                           getparam("arg2"),
                                           getparam("arg3"),
                                           getparam("arg4"),
                                           getparam("arg5"),
                                           getparam("arg6"),
                                           getparam("arg7"),
                                           getparam("arg8"),
                                           getparam("arg9")
                                           );
                  rsprintf(" %d", status);

                  status = cm_disconnect_client(hconn, FALSE);
                  rsprintf(" %d", status);
               }

               rsprintf(" | ");
            }
         }
      }
   }

   rsprintf("rpc %d, called %d clients\n", rpc, count);
}

/*------------------------------------------------------------------*/

void do_jrpc_rev1()
{
   static RPC_LIST rpc_list[] = {
      { 9998, "mhttpd_jrpc_rev1", {
            {TID_STRING, RPC_OUT}, // return string
            {TID_INT,    RPC_IN},  // return string max length
            {TID_STRING, RPC_IN}, // arg0
            {TID_STRING, RPC_IN}, // arg1
            {TID_STRING, RPC_IN}, // arg2
            {TID_STRING, RPC_IN}, // arg3
            {TID_STRING, RPC_IN}, // arg4
            {TID_STRING, RPC_IN}, // arg5
            {TID_STRING, RPC_IN}, // arg6
            {TID_STRING, RPC_IN}, // arg7
            {TID_STRING, RPC_IN}, // arg8
            {TID_STRING, RPC_IN}, // arg9
            {0}} },
      { 0 }
   };

   int status, count = 0, substring = 0, rpc;

   const char *xname   = getparam("name");
   const char *srpc    = getparam("rpc");

   if (!srpc || !xname) {
      show_text_header();
      rsprintf("<INVALID_ARGUMENTS>");
      return;
   }

   char sname[256];
   strlcpy(sname, xname, sizeof(sname));

   if (sname[strlen(sname)-1]=='*') {
      sname[strlen(sname)-1] = 0;
      substring = 1;
   }

   rpc = atoi(srpc);

   if (rpc<RPC_MIN_ID || rpc>RPC_MAX_ID) {
      show_text_header();
      rsprintf("<INVALID_RPC_ID>");
      return;
   }

   rpc_list[0].id = rpc;
   status = rpc_register_functions(rpc_list, NULL);

   //printf("cm_register_functions() for format \'%s\' status %d\n", sformat, status);

   show_text_header();

   std::string reply_header;
   std::string reply_body;

   //rsprintf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
   //rsprintf("<!-- created by MHTTPD on (timestamp) -->\n");
   //rsprintf("<jrpc_rev1>\n");
   //rsprintf("  <rpc>%d</rpc>\n", rpc);

   if (1) {
      HNDLE hDB, hrootkey, hsubkey, hkey;

      cm_get_experiment_database(&hDB, NULL);
      
      int buf_length = 1024;

      int max_reply_length = atoi(getparam("max_reply_length"));
      if (max_reply_length > buf_length)
         buf_length = max_reply_length;

      char* buf = (char*)malloc(buf_length);

      /* find client which exports our RPC function */
      status = db_find_key(hDB, 0, "System/Clients", &hrootkey);
      if (status == DB_SUCCESS) {
         for (int i=0; ; i++) {
            status = db_enum_key(hDB, hrootkey, i, &hsubkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;
            
            char str[256];
            sprintf(str, "RPC/%d", rpc);
            status = db_find_key(hDB, hsubkey, str, &hkey);
            if (status == DB_SUCCESS) {
               char client_name[NAME_LENGTH];
               HNDLE hconn;
               int size;

               size = sizeof(client_name);
               status = db_get_value(hDB, hsubkey, "Name", client_name, &size, TID_STRING, FALSE);
               if (status != DB_SUCCESS)
                  continue;

               if (strlen(sname) > 0) {
                  if (substring) {
                     if (strstr(client_name, sname) != client_name)
                        continue;
                  } else {
                     if (strcmp(sname, client_name) != 0)
                        continue;
                  }
               }

               count++;

               //rsprintf("  <client>\n");
               //rsprintf("    <name>%s</name>\n", client_name);

               int connect_status = -1;
               int call_status = -1;
               int call_length = 0;
               int disconnect_status = -1;

               connect_status = cm_connect_client(client_name, &hconn);

               //rsprintf("    <connect_status>%d</connect_status>\n", status);

               if (connect_status == RPC_SUCCESS) {
                  buf[0] = 0;

                  call_status = rpc_client_call(hconn, rpc,
                                                buf,
                                                buf_length,
                                                getparam("arg0"),
                                                getparam("arg1"),
                                                getparam("arg2"),
                                                getparam("arg3"),
                                                getparam("arg4"),
                                                getparam("arg5"),
                                                getparam("arg6"),
                                                getparam("arg7"),
                                                getparam("arg8"),
                                                getparam("arg9")
                                                );

                  //rsprintf("    <rpc_status>%d</rpc_status>\n", status);
                  ////rsprintf("    <data>%s</data>\n", buf);
                  //rsputs("<data>");
                  //rsputs(buf);
                  //rsputs("</data>\n");

                  if (call_status == RPC_SUCCESS) {
                     call_length = strlen(buf);
                     reply_body += buf;
                  }

                  disconnect_status = cm_disconnect_client(hconn, FALSE);
                  //rsprintf("    <disconnect_status>%d</disconnect_status>\n", status);
               }

               //rsprintf("  </client>\n");

               if (reply_header.length() > 0)
                  reply_header += " | ";

               char tmp[256];
               sprintf(tmp, "%s %d %d %d %d", client_name, connect_status, call_status, disconnect_status, call_length);
               reply_header += tmp;
            }
         }
      }

      free(buf);
   }

   //rsprintf("  <called_clients>%d</called_clients>\n", count);
   //rsprintf("</jrpc_rev1>\n");

   if (reply_header.length() > 0) {
      rsputs(reply_header.c_str());
      rsputs(" || ");
      rsputs(reply_body.c_str());
      rsputs("\n");
   }
}

/*------------------------------------------------------------------*/

void output_key(HNDLE hkey, int index, const char *format)
{
   int size, i;
   char str[TEXT_SIZE];
   HNDLE hDB, hsubkey;
   KEY key;
   char data[TEXT_SIZE];
   
   cm_get_experiment_database(&hDB, NULL);
   
   db_get_key(hDB, hkey, &key);
   if (key.type == TID_KEY) {
      for (i=0 ; ; i++) {
         db_enum_key(hDB, hkey, i, &hsubkey);
         if (!hsubkey)
            break;
         output_key(hsubkey, -1, format);
      }
   } else {
      if (key.item_size <= (int)sizeof(data)) {
         size = sizeof(data);
         db_get_data(hDB, hkey, data, &size, key.type);
         if (index == -1) {
            for (i=0 ; i<key.num_values ; i++) {
               if (isparam("name") && atoi(getparam("name")) == 1) {
                  if (key.num_values == 1)
                     rsprintf("%s:", key.name);
                  else
                     rsprintf("%s[%d]:", key.name, i);
               }
               if (format && format[0])
                  db_sprintff(str, format, data, key.item_size, i, key.type);
               else
                  db_sprintf(str, data, key.item_size, i, key.type);
               rsputs(str);
               if (i<key.num_values-1)
                  rsputs("\n");
            }
         } else {
            if (isparam("name") && atoi(getparam("name")) == 1)
               rsprintf("%s[%d]:", key.name, index);
            if (index >= key.num_values)
               rsputs("<DB_OUT_OF_RANGE>");
            else {
               if (isparam("format"))
                  db_sprintff(str, getparam("format"), data, key.item_size, index, key.type);
               else
                  db_sprintf(str, data, key.item_size, index, key.type);
               rsputs(str);
            }
         }
         rsputs("\n");
      }
   }
}
   
/*------------------------------------------------------------------*/

void java_script_commands(const char *path, const char *cookie_cpwd)
{
   int size, i, index;
   char str[TEXT_SIZE], ppath[256], format[256];
   HNDLE hDB, hkey;
   KEY key;
   char data[TEXT_SIZE];

   cm_get_experiment_database(&hDB, NULL);
   
   /* process "jset" command */
   if (equal_ustring(getparam("cmd"), "jset")) {
      
      if (*getparam("pnam")) {
         sprintf(ppath, "/Custom/Pwd/%s", getparam("pnam"));
         str[0] = 0;
         db_get_value(hDB, 0, ppath, str, &size, TID_STRING, TRUE);
         if (!equal_ustring(cookie_cpwd, str)) {
            show_text_header();
            rsprintf("Invalid password!");
            return;
         }
      }
      strlcpy(str, getparam("odb"), sizeof(str));
      if (strchr(str, '[')) {
         if (*(strchr(str, '[')+1) == '*')
            index = -1;
         else
            index = atoi(strchr(str, '[')+1);
         *strchr(str, '[') = 0;
      } else
         index = 0;
      
      if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS && isparam("value")) {
         db_get_key(hDB, hkey, &key);
         memset(data, 0, sizeof(data));
         if (key.item_size <= (int)sizeof(data)) {
            if (index == -1) {
               const char* p = getparam("value");
               for (i=0 ; p != NULL ; i++) {
                  size = sizeof(data);
                  db_sscanf(p, data, &size, 0, key.type);
                  db_set_data_index(hDB, hkey, data, key.item_size, i, key.type);
                  p = strchr(p, ',');
                  if (p != NULL)
                     p++;
               } 
            } else {
               size = sizeof(data);
               db_sscanf(getparam("value"), data, &size, 0, key.type);
               db_set_data_index(hDB, hkey, data, key.item_size, index, key.type);
            }
         }
      } else {
         if (isparam("value") && isparam("type") && isparam("len")) {
            int type = atoi(getparam("type"));
            if (type == 0) {
               show_text_header();
               rsprintf("Invalid type %d!", type);
               return;
            }
            db_create_key(hDB, 0, str, type);
            db_find_key(hDB, 0, str, &hkey);
            if (!hkey) {
               show_text_header();
               rsprintf("Cannot create \'%s\' type %d", str, type);
               return;
            }
            db_get_key(hDB, hkey, &key);
            memset(data, 0, sizeof(data));
            size = sizeof(data);
            db_sscanf(getparam("value"), data, &size, 0, key.type);
            if (key.type == TID_STRING)
               db_set_data(hDB, hkey, data, atoi(getparam("len")), 1, TID_STRING);
            else {
               for (i=0 ; i<atoi(getparam("len")) ; i++)
                  db_set_data_index(hDB, hkey, data, rpc_tid_size(key.type), i, key.type);
            }
         }
      }
      
      show_text_header();
      rsprintf("OK");
      return;
   }
   
   /* process "jget" command */
   if (equal_ustring(getparam("cmd"), "jget")) {
      
      if (isparam("odb")) {
         strlcpy(str, getparam("odb"), sizeof(str));
         if (strchr(str, '[')) {
            if (*(strchr(str, '[')+1) == '*')
               index = -1;
            else
               index = atoi(strchr(str, '[')+1);
            *strchr(str, '[') = 0;
         } else
            index = 0;
         
         show_text_header();
         
         if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS)
            output_key(hkey, index, getparam("format"));
         else
            rsputs("<DB_NO_KEY>");
      }
      
      if (isparam("odb0")) {
         show_text_header();
         for (i=0 ; ; i++) {
            sprintf(ppath, "odb%d", i);
            sprintf(format, "format%d", i);
            if (isparam(ppath)) {
               strlcpy(str, getparam(ppath), sizeof(str));
               if (strchr(str, '[')) {
                  if (*(strchr(str, '[')+1) == '*')
                     index = -1;
                  else
                     index = atoi(strchr(str, '[')+1);
                  *strchr(str, '[') = 0;
               } else
                  index = 0;
               if (i > 0)
                  rsputs("$#----#$\n");
               if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS)
                  output_key(hkey, index, getparam(format));
               else
                  rsputs("<DB_NO_KEY>");
               
            } else
               break;
         }
      }
      
      return;
   }
   
   /* process "jcopy" command */
   if (equal_ustring(getparam("cmd"), "jcopy")) {
      
      if (isparam("odb")) {
         strlcpy(str, getparam("odb"), sizeof(str));

         show_text_header();
         
         if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS) {
            int end = 0;
            int bufsize = WEB_BUFFER_SIZE;
            char* buf = (char *)malloc(size);
            if (isparam("format") && equal_ustring(getparam("format"), "xml"))
               db_copy_xml(hDB, hkey, buf, &bufsize);
            else if (isparam("format") && equal_ustring(getparam("format"), "json"))
               db_copy_json(hDB, hkey, &buf, &bufsize, &end, 1, 1);
            else
               db_copy(hDB, hkey, buf, &bufsize, (char *)"");
            rsputs(buf);
            free(buf);
         } else
            rsputs("<DB_NO_KEY>");
      }
      return;
   }

   /* process "jkey" command */
   if (equal_ustring(getparam("cmd"), "jkey")) {
      
      show_text_header();
      
      if (isparam("odb") && db_find_key(hDB, 0, getparam("odb"), &hkey) == DB_SUCCESS) {
         db_get_key(hDB, hkey, &key);
         rsprintf("%s\n", key.name);
         rsprintf("TID_%s\n", rpc_tid_name(key.type));
         rsprintf("%d\n", key.num_values);
         rsprintf("%d\n", key.item_size);
         rsprintf("%d", key.last_written);
      } else
         rsputs("<DB_NO_KEY>");
      
      return;
   }
   
   /* process "jmsg" command */
   if (equal_ustring(getparam("cmd"), "jmsg")) {
      
      i = 1;
      if (*getparam("n"))
         i = atoi(getparam("n"));
      
      show_text_header();
      cm_msg_retrieve(i, str, sizeof(str));
      rsputs(str);
      return;
   }
   
   /* process "jgenmsg" command */
   if (equal_ustring(getparam("cmd"), "jgenmsg")) {
      
      if (*getparam("msg")) {
         cm_msg(MINFO, "show_custom_page", getparam("msg"));
      }
      
      show_text_header();
      rsputs("Message successfully created\n");
      return;
   }
   
   /* process "jalm" command */
   if (equal_ustring(getparam("cmd"), "jalm")) {
      
      show_text_header();
      al_get_alarms(str, sizeof(str));
      rsputs(str);
      return;
   }
   
   /* process "jrpc" command */
   if (equal_ustring(getparam("cmd"), "jrpc_rev0")) {
      do_jrpc_rev0();
      return;
   }
   
   /* process "jrpc" command */
   if (equal_ustring(getparam("cmd"), "jrpc_rev1")) {
      do_jrpc_rev1();
      return;
   }
}

/*------------------------------------------------------------------*/

void show_custom_page(const char *path, const char *cookie_cpwd)
{
   int size, n_var, fh, index, edit;
   char str[TEXT_SIZE], keypath[256], type[32], *p, *ps, custom_path[256],
      filename[256], pwd[256], ppath[256], tail[256];
   HNDLE hDB, hkey;
   KEY key;
   char data[TEXT_SIZE];

   if (strstr(path, ".gif")) {
      show_custom_gif(path);
      return;
   }

   if (strstr(path, ".mp3") || strstr(path, ".ogg")) {
      show_custom_audio(path);
      return;
   }
   cm_get_experiment_database(&hDB, NULL);

   if (path[0] == 0) {
      show_error("Invalid custom page: NULL path");
      return;
   }
   sprintf(str, "/Custom/%s", path);

   custom_path[0] = 0;
   size = sizeof(custom_path);
   db_get_value(hDB, 0, "/Custom/Path", custom_path, &size, TID_STRING, FALSE);
   db_find_key(hDB, 0, str, &hkey);
   if (!hkey) {
      sprintf(str, "/Custom/%s&", path);
      db_find_key(hDB, 0, str, &hkey);
      if (!hkey) {
         sprintf(str, "/Custom/%s!", path);
         db_find_key(hDB, 0, str, &hkey);
      }
   }

   if (hkey) {
      char* ctext;
      int status;

      status = db_get_key(hDB, hkey, &key);
      assert(status == DB_SUCCESS);
      size = key.total_size;
      ctext = (char*)malloc(size);
      status = db_get_data(hDB, hkey, ctext, &size, TID_STRING);
      if (status != DB_SUCCESS) {
         sprintf(str, "Error: db_get_data() status %d", status);
         show_error(str);
         free(ctext);
         return;
      }

      /* check if filename */
      if (strchr(ctext, '\n') == 0) {
         strlcpy(filename, custom_path, sizeof(str));
         if (filename[strlen(filename)-1] != DIR_SEPARATOR)
            strlcat(filename, DIR_SEPARATOR_STR, sizeof(filename));
         strlcat(filename, ctext, sizeof(filename));
         fh = open(filename, O_RDONLY | O_BINARY);
         if (fh < 0) {
            sprintf(str, "Cannot open file \"%s\"", filename);
            show_error(str);
            free(ctext);
            return;
         }
         free(ctext);
         ctext = NULL;
         size = lseek(fh, 0, SEEK_END) + 1;
         lseek(fh, 0, SEEK_SET);
         ctext = (char*)malloc(size);
         memset(ctext, 0, size);
         read(fh, ctext, size);
         close(fh);
      }

      /* check for valid password */
      if (equal_ustring(getparam("cmd"), "Edit")) {
         p = ps = ctext;
         n_var = 0;
         do {
            char format[256];

            p = find_odb_tag(ps, keypath, format, &edit, type, pwd, tail);
            if (p == NULL)
               break;
            ps = strchr(p, '>') + 1;

            if (pwd[0] && n_var == atoi(getparam("index"))) {
               size = NAME_LENGTH;
               strlcpy(str, path, sizeof(str));
               if (strlen(str)>0 && str[strlen(str)-1] == '&')
                  str[strlen(str)-1] = 0;
               if (*getparam("pnam"))
                  sprintf(ppath, "/Custom/Pwd/%s", getparam("pnam"));
               else
                  sprintf(ppath, "/Custom/Pwd/%s", str);
               str[0] = 0;
               db_get_value(hDB, 0, ppath, str, &size, TID_STRING, TRUE);
               if (!equal_ustring(cookie_cpwd, str)) {
                  show_error("Invalid password!");
                  free(ctext);
                  return;
               } else
                  break;
            }

            n_var++;
         } while (p != NULL);
      }

      /* process toggle command */
      if (equal_ustring(getparam("cmd"), "Toggle")) {

         if (*getparam("pnam")) {
            sprintf(ppath, "/Custom/Pwd/%s", getparam("pnam"));
            str[0] = 0;
            db_get_value(hDB, 0, ppath, str, &size, TID_STRING, TRUE);
            if (!equal_ustring(cookie_cpwd, str)) {
               show_error("Invalid password!");
               free(ctext);
               return;
            }
         }
         strlcpy(str, getparam("odb"), sizeof(str));
         if (strchr(str, '[')) {
            index = atoi(strchr(str, '[')+1);
            *strchr(str, '[') = 0;
         } else
            index = 0;

         if (db_find_key(hDB, 0, str, &hkey)) {
            db_get_key(hDB, hkey, &key);
            memset(data, 0, sizeof(data));
            if (key.item_size <= (int)sizeof(data)) {
               size = sizeof(data);
               db_get_data_index(hDB, hkey, data, &size, index, key.type);
               db_sprintf(str, data, size, 0, key.type);
               if (atoi(str) == 0)
                  db_sscanf("1", data, &size, 0, key.type);
               else
                  db_sscanf("0", data, &size, 0, key.type);
               db_set_data_index(hDB, hkey, data, key.item_size, index, key.type);
            }
         }

         /* redirect (so that 'reload' does not toggle again) */
         redirect(path);
         free(ctext);
         return;
      }

      /* HTTP header */
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
      rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

      /* interprete text, replace <odb> tags with ODB values */
      p = ps = ctext;
      n_var = 0;
      do {
         char format[256];
         p = find_odb_tag(ps, keypath, format, &edit, type, pwd, tail);
         if (p != NULL)
            *p = 0;
         rsputs(ps);

         if (p == NULL)
            break;
         ps = strchr(p + 1, '>') + 1;

         show_odb_tag(path, keypath, format, n_var, edit, type, pwd, tail);
         n_var++;

      } while (p != NULL);

      if (equal_ustring(getparam("cmd"), "Set") || isparam("cbi")) {
         /* redirect (so that 'reload' does not change value) */
         strlen_retbuf = 0;
         sprintf(str, "%s", path);
         redirect(str);
      }

      free(ctext);
      ctext = NULL;
   } else {
      show_error("Invalid custom page: Page not found in ODB");
      return;
   }
}

/*------------------------------------------------------------------*/

void show_cnaf_page()
{
   char str[256];
   int c, n, a, f, d, q, x, r, ia, id, w;
   int i, size, status;
   HNDLE hDB, hrootkey, hsubkey, hkey;

   static char client_name[NAME_LENGTH];
   static HNDLE hconn = 0;

   cm_get_experiment_database(&hDB, NULL);

   /* find FCNA server if not specified */
   if (hconn == 0) {
      /* find client which exports FCNA function */
      status = db_find_key(hDB, 0, "System/Clients", &hrootkey);
      if (status == DB_SUCCESS) {
         for (i = 0;; i++) {
            status = db_enum_key(hDB, hrootkey, i, &hsubkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            sprintf(str, "RPC/%d", RPC_CNAF16);
            status = db_find_key(hDB, hsubkey, str, &hkey);
            if (status == DB_SUCCESS) {
               size = sizeof(client_name);
               db_get_value(hDB, hsubkey, "Name", client_name, &size, TID_STRING, TRUE);
               break;
            }
         }
      }

      if (client_name[0]) {
         status = cm_connect_client(client_name, &hconn);
         if (status != RPC_SUCCESS)
            hconn = 0;
      }
   }

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>MIDAS CAMAC interface</title></head>\n");
   rsprintf("<body><form method=\"GET\" action=\"CNAF\">\n\n");

   /* title row */

   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);

   rsprintf("<table border=3 cellpadding=1>\n");
   rsprintf("<tr><th colspan=3 bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);

   if (client_name[0] == 0)
      rsprintf("<th colspan=3 bgcolor=#FF0000>No CAMAC server running</tr>\n");
   else if (hconn == 0)
      rsprintf("<th colspan=3 bgcolor=#FF0000>Cannot connect to %s</tr>\n", client_name);
   else
      rsprintf("<th colspan=3 bgcolor=#A0A0FF>CAMAC server: %s</tr>\n", client_name);

   /* default values */
   c = n = 1;
   a = f = d = q = x = 0;
   r = 1;
   ia = id = w = 0;

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=3 bgcolor=#C0C0C0>\n");
   rsprintf("<input type=submit name=cmd value=Execute>\n");

   rsprintf("<td colspan=3 bgcolor=#C0C0C0>\n");
   rsprintf("<input type=submit name=cmd value=ODB>\n");
   rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("<input type=submit name=cmd value=Help>\n");
   rsprintf("</tr>\n\n");

   /* header */
   rsprintf("<tr><th bgcolor=#FFFF00>N");
   rsprintf("<th bgcolor=#FF8000>A");
   rsprintf("<th bgcolor=#00FF00>F");
   rsprintf("<th colspan=3 bgcolor=#8080FF>Data");

   /* execute commands */
   size = sizeof(d);

   const char* cmd = getparam("cmd");
   if (equal_ustring(cmd, "C cycle")) {
      rpc_client_call(hconn, RPC_CNAF16, CNAF_CRATE_CLEAR, 0, 0, 0, 0, 0, &d, &size, &x,
                      &q);

      rsprintf("<tr><td colspan=6 bgcolor=#00FF00>C cycle executed sucessfully</tr>\n");
   } else if (equal_ustring(cmd, "Z cycle")) {
      rpc_client_call(hconn, RPC_CNAF16, CNAF_CRATE_ZINIT, 0, 0, 0, 0, 0, &d, &size, &x,
                      &q);

      rsprintf("<tr><td colspan=6 bgcolor=#00FF00>Z cycle executed sucessfully</tr>\n");
   } else if (equal_ustring(cmd, "Clear inhibit")) {
      rpc_client_call(hconn, RPC_CNAF16, CNAF_INHIBIT_CLEAR, 0, 0, 0, 0, 0, &d, &size, &x,
                      &q);

      rsprintf
          ("<tr><td colspan=6 bgcolor=#00FF00>Clear inhibit executed sucessfully</tr>\n");
   } else if (equal_ustring(cmd, "Set inhibit")) {
      rpc_client_call(hconn, RPC_CNAF16, CNAF_INHIBIT_SET, 0, 0, 0, 0, 0, &d, &size, &x,
                      &q);

      rsprintf
          ("<tr><td colspan=6 bgcolor=#00FF00>Set inhibit executed sucessfully</tr>\n");
   } else if (equal_ustring(cmd, "Execute")) {
      c = atoi(getparam("C"));
      n = atoi(getparam("N"));
      a = atoi(getparam("A"));
      f = atoi(getparam("F"));
      r = atoi(getparam("R"));
      w = atoi(getparam("W"));
      id = atoi(getparam("ID"));
      ia = atoi(getparam("IA"));

      const char* pd = getparam("D");
      if (strncmp(pd, "0x", 2) == 0)
         sscanf(pd + 2, "%x", &d);
      else
         d = atoi(getparam("D"));

      /* limit repeat range */
      if (r == 0)
         r = 1;
      if (r > 100)
         r = 100;
      if (w > 1000)
         w = 1000;

      for (i = 0; i < r; i++) {
         status = SUCCESS;

         if (hconn) {
            size = sizeof(d);
            status =
                rpc_client_call(hconn, RPC_CNAF24, CNAF, 0, c, n, a, f, &d, &size, &x,
                                &q);

            if (status == RPC_NET_ERROR) {
               /* try to reconnect */
               cm_disconnect_client(hconn, FALSE);
               status = cm_connect_client(client_name, &hconn);
               if (status != RPC_SUCCESS) {
                  hconn = 0;
                  client_name[0] = 0;
               }

               if (hconn)
                  status =
                      rpc_client_call(hconn, RPC_CNAF24, CNAF, 0, c, n, a, f, &d, &size,
                                      &x, &q);
            }
         }

         if (status != SUCCESS) {
            rsprintf
                ("<tr><td colspan=6 bgcolor=#FF0000>Error executing function, code = %d</tr>",
                 status);
         } else {
            rsprintf("<tr align=center><td bgcolor=#FFFF00>%d", n);
            rsprintf("<td bgcolor=#FF8000>%d", a);
            rsprintf("<td bgcolor=#00FF00>%d", f);
            rsprintf("<td colspan=3 bgcolor=#8080FF>%d / 0x%04X  Q%d X%d", d, d, q, x);
         }

         d += id;
         a += ia;

         if (w > 0)
            ss_sleep(w);
      }
   }

   /* input fields */
   rsprintf
       ("<tr align=center><td bgcolor=#FFFF00><input type=text size=3 name=N value=%d>\n",
        n);
   rsprintf("<td bgcolor=#FF8000><input type=text size=3 name=A value=%d>\n", a);
   rsprintf("<td bgcolor=#00FF00><input type=text size=3 name=F value=%d>\n", f);
   rsprintf
       ("<td bgcolor=#8080FF colspan=3><input type=text size=8 name=D value=%d></tr>\n",
        d);

   /* control fields */
   rsprintf("<tr><td colspan=2 bgcolor=#FF8080>Repeat");
   rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=R value=%d>\n", r);

   rsprintf
       ("<td align=center colspan=3 bgcolor=#FF0000><input type=submit name=cmd value=\"C cycle\">\n");
   rsprintf("<input type=submit name=cmd value=\"Z cycle\">\n");

   rsprintf("<tr><td colspan=2 bgcolor=#FF8080>Repeat delay [ms]");
   rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=W value=%d>\n", w);

   rsprintf
       ("<td align=center colspan=3 bgcolor=#FF0000><input type=submit name=cmd value=\"Set inhibit\">\n");
   rsprintf("<input type=submit name=cmd value=\"Clear inhibit\">\n");

   rsprintf("<tr><td colspan=2 bgcolor=#FF8080>Data increment");
   rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=ID value=%d>\n", id);

   rsprintf
       ("<td colspan=3 align=center bgcolor=#FFFF80>Branch <input type=text size=3 name=B value=0>\n");

   rsprintf("<tr><td colspan=2 bgcolor=#FF8080>A increment");
   rsprintf("<td bgcolor=#FF8080><input type=text size=3 name=IA value=%d>\n", ia);

   rsprintf
       ("<td colspan=3 align=center bgcolor=#FFFF80>Crate <input type=text size=3 name=C value=%d>\n",
        c);

   rsprintf("</table></body>\r\n");
}

/*------------------------------------------------------------------*/

#ifdef HAVE_MSCB

typedef struct {
   char id;
   char name[32];
} NAME_TABLE;

NAME_TABLE prefix_table[] = {
   {PRFX_PICO, "pico",},
   {PRFX_NANO, "nano",},
   {PRFX_MICRO, "micro",},
   {PRFX_MILLI, "milli",},
   {PRFX_NONE, "",},
   {PRFX_KILO, "kilo",},
   {PRFX_MEGA, "mega",},
   {PRFX_GIGA, "giga",},
   {PRFX_TERA, "tera",},
   {99}
};

NAME_TABLE unit_table[] = {

   {UNIT_METER, "meter",},
   {UNIT_GRAM, "gram",},
   {UNIT_SECOND, "second",},
   {UNIT_MINUTE, "minute",},
   {UNIT_HOUR, "hour",},
   {UNIT_AMPERE, "ampere",},
   {UNIT_KELVIN, "kelvin",},
   {UNIT_CELSIUS, "deg. celsius",},
   {UNIT_FARENHEIT, "deg. farenheit",},

   {UNIT_HERTZ, "hertz",},
   {UNIT_PASCAL, "pascal",},
   {UNIT_BAR, "bar",},
   {UNIT_WATT, "watt",},
   {UNIT_VOLT, "volt",},
   {UNIT_OHM, "ohm",},
   {UNIT_TESLA, "tesls",},
   {UNIT_LITERPERSEC, "liter/sec",},
   {UNIT_RPM, "RPM",},
   {UNIT_FARAD, "farad",},

   {UNIT_BOOLEAN, "boolean",},
   {UNIT_BYTE, "byte",},
   {UNIT_WORD, "word",},
   {UNIT_DWORD, "dword",},
   {UNIT_ASCII, "ascii",},
   {UNIT_STRING, "string",},
   {UNIT_BAUD, "baud",},

   {UNIT_PERCENT, "percent",},
   {UNIT_PPM, "RPM",},
   {UNIT_COUNT, "counts",},
   {UNIT_FACTOR, "factor",},
   {0}
};

/*------------------------------------------------------------------*/

void print_mscb_var(char *value, char *evalue, char *unit, MSCB_INFO_VAR *info_chn, void *pdata)
{
   char str[80];
   signed short sdata;
   unsigned short usdata;
   signed int idata;
   unsigned int uidata;
   float fdata;
   int i;

   value[0] = 0;
   evalue[0] = 0;

   if (info_chn->unit == UNIT_STRING) {
      memset(str, 0, sizeof(str));
      strncpy(str, (char *)pdata, info_chn->width);
      for (i = 0; i < (int) strlen(str); i++)
         switch (str[i]) {
         case 1:
            strcat(value, "\\001");
            break;
         case 2:
            strcat(value, "\\002");
            break;
         case 9:
            strcat(value, "\\t");
            break;
         case 10:
            strcat(value, "\\n");
            break;
         case 13:
            strcat(value, "\\r");
            break;
         default:
            value[strlen(value) + 1] = 0;
            value[strlen(value)] = str[i];
            break;
         }
      strlcpy(evalue, value, 256);
   } else {
      switch (info_chn->width) {
      case 0:
         strcpy(value, "0");
         strcpy(evalue, "0");
         break;

      case 1:
         if (info_chn->flags & MSCBF_SIGNED) {
            sprintf(value, "%d (0x%02X/", *((signed char *)pdata), *((signed char *)pdata));
            sprintf(evalue, "%d", *((signed char *)pdata));
         } else {
            sprintf(value, "%u (0x%02X/", *((unsigned char *)pdata), *((unsigned char *)pdata));
            sprintf(evalue, "%u", *((unsigned char *)pdata));
         }

         for (i = 0; i < 8; i++)
            if (*((unsigned char *)pdata) & (0x80 >> i))
               sprintf(value + strlen(value), "1");
            else
               sprintf(value + strlen(value), "0");
         sprintf(value + strlen(value), ")");
         break;

      case 2:
         if (info_chn->flags & MSCBF_SIGNED) {
            sdata = *((signed short *)pdata);
            WORD_SWAP(&sdata);
            sprintf(value, "%d (0x%04X)", sdata, sdata);
            sprintf(evalue, "%d", sdata);
         } else {
            usdata = *((unsigned short *)pdata);
            WORD_SWAP(&usdata);
            sprintf(value, "%u (0x%04X)", usdata, usdata);
            sprintf(evalue, "%u", usdata);
         }
         break;

      case 4:
         if (info_chn->flags & MSCBF_FLOAT) {
            fdata = *((float *)pdata);
            DWORD_SWAP(&fdata);
            sprintf(value, "%1.6lg", fdata);
            sprintf(evalue, "%1.6lg", fdata);
         } else {
            if (info_chn->flags & MSCBF_SIGNED) {
               idata = *((signed int *)pdata);
               DWORD_SWAP(&idata);
               sprintf(value, "%d (0x%08X)", idata, idata);
               sprintf(evalue, "%d", idata);
            } else {
               uidata = *((unsigned int *)pdata);
               DWORD_SWAP(&uidata);
               sprintf(value, "%u (0x%08X)", uidata, uidata);
               sprintf(evalue, "%u", uidata);
            }
         }
         break;
      }
   }

   /* evaluate prefix */
   unit[0] = 0;
   if (info_chn->prefix) {
      for (i = 0; prefix_table[i].id != 99; i++)
	if ((unsigned char)prefix_table[i].id == info_chn->prefix)
            break;
      if (prefix_table[i].id)
         strcpy(unit, prefix_table[i].name);
   }

   /* evaluate unit */
   if (info_chn->unit && info_chn->unit != UNIT_STRING) {
      for (i = 0; unit_table[i].id; i++)
	if ((unsigned char)unit_table[i].id == info_chn->unit)
            break;
      if (unit_table[i].id)
         strcat(unit, unit_table[i].name);
   }
}

static int cmp_int(const void *a, const void *b)
{
  return *((int *)a) > *((int *)b);
}

/*------------------------------------------------------------------*/

void create_mscb_tree()
{
   HNDLE hDB, hKeySubm, hKeyEq, hKeyAdr, hKey, hKeyDev;
   KEY key;
   int i, j, k, l, size, address[1000], dev_badr[1000], dev_adr[1000], dev_chn[1000], 
      n_address, n_dev_adr;
   char mscb_dev[256], mscb_pwd[32], eq_name[32];

   cm_get_experiment_database(&hDB, NULL);

   db_create_key(hDB, 0, "MSCB/Submaster", TID_KEY);
   db_find_key(hDB, 0, "MSCB/Submaster", &hKeySubm);
   assert(hKeySubm);

   /*---- go through equipment list ----*/
   db_find_key(hDB, 0, "Equipment", &hKeyEq);
   assert(hKeyEq);
   for (i=0 ; ; i++) {
      db_enum_key(hDB, hKeyEq, i, &hKey);
      if (!hKey)
         break;
      db_get_key(hDB, hKey, &key);
      strcpy(eq_name, key.name);
      db_find_key(hDB, hKey, "Settings/Devices", &hKeyDev);
      if (hKeyDev) {
         for (j=0 ;; j++) {
            db_enum_key(hDB, hKeyDev, j, &hKey);
            if (!hKey)
               break;

            if (db_find_key(hDB, hKey, "MSCB Address", &hKeyAdr) == DB_SUCCESS) {
               /* mscbdev type of device */
               size = sizeof(mscb_dev);
               if (db_get_value(hDB, hKey, "Device", mscb_dev, &size, TID_STRING, FALSE) != DB_SUCCESS)
                   continue;
               size = sizeof(mscb_pwd);
               if (db_get_value(hDB, hKey, "Pwd", mscb_pwd, &size, TID_STRING, FALSE) != DB_SUCCESS)
                   continue;

               size = sizeof(dev_adr);
               db_get_data(hDB, hKeyAdr, dev_adr, &size, TID_INT);
               n_dev_adr = size / sizeof(int);
            } else if (db_find_key(hDB, hKey, "Block Address", &hKeyAdr) == DB_SUCCESS) {
               /* mscbhvr type of device */
               size = sizeof(mscb_dev);
               if (db_get_value(hDB, hKey, "MSCB Device", mscb_dev, &size, TID_STRING, FALSE) != DB_SUCCESS)
                  continue;
               size = sizeof(mscb_pwd);
               if (db_get_value(hDB, hKey, "MSCB Pwd", mscb_pwd, &size, TID_STRING, FALSE) != DB_SUCCESS)
                  continue;

               n_dev_adr = 0;
               size = sizeof(dev_badr);
               db_get_data(hDB, hKeyAdr, dev_badr, &size, TID_INT);
               size = sizeof(dev_chn);
               if (db_get_value(hDB, hKey, "Block Channels", dev_chn, &size, TID_INT, FALSE) == DB_SUCCESS) {
                  for (k=0 ; k<size/(int)sizeof(int) && n_dev_adr < (int)(sizeof(dev_adr)/sizeof(int)) ; k++) {
                     for (l=0 ; l<dev_chn[k] ; l++)
                        dev_adr[n_dev_adr++] = dev_badr[k]+l;
                  }
               }
            } else
	       continue;

            /* create or open submaster entry */
            db_find_key(hDB, hKeySubm, mscb_dev, &hKey);
            if (!hKey) {
               db_create_key(hDB, hKeySubm, mscb_dev, TID_KEY);
               db_find_key(hDB, hKeySubm, mscb_dev, &hKey);
               assert(hKey);
            }

            /* get old address list */
            size = sizeof(address);
            if (db_get_value(hDB, hKey, "Address", address, &size, TID_INT, FALSE) == DB_SUCCESS)
               n_address = size / sizeof(int);
            else
               n_address = 0;

            /* merge with new address list */
            for (k=0 ; k<n_dev_adr ; k++) {
               for (l=0 ; l<n_address ; l++)
                  if (address[l] == dev_adr[k])
                     break;

               if (l == n_address)
                  address[n_address++] = dev_adr[k];
            }

            /* sort address list */
            qsort(address, n_address, sizeof(int), cmp_int);

            /* store new address list */
            db_set_value(hDB, hKey, "Pwd", mscb_pwd, 32, 1, TID_STRING);
            db_set_value(hDB, hKey, "Comment", eq_name, 32, 1, TID_STRING);
            db_set_value(hDB, hKey, "Address", address, n_address*sizeof(int), n_address, TID_INT);
         }
      }
   }
}

/*------------------------------------------------------------------*/

void show_mscb_page(char *path, int refresh)
{
   int i, j, fi, fd, status, size, cur_subm_index, cur_node, adr, show_hidden;
   unsigned int uptime;
   float fvalue;
   char str[256], comment[256], *pd, dbuf[256], value[256], evalue[256], unit[256], cur_subm_name[256];
   time_t now;
   HNDLE hDB, hKeySubm, hKeyCurSubm, hKey, hKeyAddr;
   KEY key;
   MSCB_INFO info;
   MSCB_INFO_VAR info_var;

   cm_get_experiment_database(&hDB, NULL);

   status = db_find_key(hDB, 0, "MSCB/Submaster", &hKeySubm);
   if (!hKeySubm)
      create_mscb_tree();

   if (strstr(path, "favicon") != NULL)
      return;

   if (isparam("subm") && isparam("node")) {
      strlcpy(cur_subm_name, getparam("subm"), sizeof(cur_subm_name));
      cur_node = atoi(getparam("node"));

      if (isparam("idx") && isparam("value")) {
         i = atoi(getparam("idx"));
         strlcpy(value, getparam("value"), sizeof(value));

         fd = mscb_init(cur_subm_name, 0, "", FALSE);
         if (fd) {
            status = mscb_info_variable(fd, 
                       (unsigned short) cur_node, (unsigned char) i, &info_var);
            if (status == MSCB_SUCCESS) {
               if (info_var.unit == UNIT_STRING) {
                  memset(str, 0, sizeof(str));
                  strncpy(str, value, info_var.width);
                  if (strlen(str) > 0 && str[strlen(str) - 1] == '\n')
                     str[strlen(str) - 1] = 0;

                  status = mscb_write(fd, (unsigned short) cur_node,
                                    (unsigned char) i, str, strlen(str) + 1);
               } else {
                  if (info_var.flags & MSCBF_FLOAT) {
                     fvalue = (float) atof(value);
                     memcpy(&dbuf, &fvalue, sizeof(float));
                  } else {
                     if (value[1] == 'x')
                        sscanf(value + 2, "%x", (int *)&dbuf);
                     else
                        *((int *)dbuf) = atoi(value);
                  }

                  status = mscb_write(fd, (unsigned short) cur_node,
                                    (unsigned char) i, dbuf, info_var.width);
               }
            }
         }
      }

      if (path[0])
         sprintf(str, "../%s/%d", cur_subm_name, cur_node);
      else
         sprintf(str, "%s/%d", cur_subm_name, cur_node);
      if (isparam("hidden"))
         strlcat(str, "h", sizeof(str));
      redirect(str);
      return;
   }

   if (path[0]) {
      strlcpy(cur_subm_name, path, sizeof(cur_subm_name));
      if (strchr(cur_subm_name, '/'))
         *strchr(cur_subm_name, '/') = 0;
      if (strchr(cur_subm_name, '?'))
         *strchr(cur_subm_name, '?') = 0;
      if (strchr(path, '/')) {
         cur_node = atoi(strchr(path, '/')+1);
      }
   } else {
      cur_subm_name[0] = 0;
      cur_node = 0;
   }

   if (path[0] && path[strlen(path)-1] == 'h')
      show_hidden = TRUE;
   else
      show_hidden = FALSE;

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Pragma: no-cache\r\n");
   rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n");
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
   rsprintf("<html><head>\n");

   /* style sheet */
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<style type=\"text/css\">\r\n");
   rsprintf("select { width:150px; background-color:#FFFFE0; font-size:12px; }\r\n");
   rsprintf(".subm {\r\n");
   rsprintf("  background-color:#E0E0E0; text-align:center; font-weight:bold;\r\n");
   rsprintf("  padding:5px;\r\n");
   rsprintf("  vertical-align:top;\r\n");
   rsprintf("  font-size:16px;\r\n");
   rsprintf("  border-right:1px solid #808080;\r\n");
   rsprintf("}\r\n");
   rsprintf(".node {\r\n");
   rsprintf("  background-color:#E0E0E0; text-align:center; font-weight:bold;\r\n");
   rsprintf("  padding:5px;\r\n");
   rsprintf("  vertical-align:top;\r\n");
   rsprintf("  font-size:16px;\r\n");
   rsprintf("  border-right:1px solid #808080;\r\n");
   rsprintf("}\r\n");
   rsprintf(".vars {\r\n");
   rsprintf("  background-color:#E0E0E0; text-align:center; font-weight:bold;\r\n");
   rsprintf("  padding:5px;\r\n");
   rsprintf("  vertical-align:top;\r\n");
   rsprintf("  font-size:10px;\r\n");
   rsprintf("}\r\n");
   rsprintf(".v1 {\r\n");
   rsprintf("  padding:3px;\r\n");
   rsprintf("  font-weight:bold;\r\n");
   rsprintf("  font-size:12px;\r\n");
   rsprintf("}\r\n");
   rsprintf(".v2 {\r\n");
   rsprintf("  background-color:#F0F0F0;\r\n");
   rsprintf("  padding:3px;\r\n");
   rsprintf("  font-size:12px;\r\n");
   rsprintf("  border:1px solid #808080;\r\n");
   rsprintf("  border-right:1px solid #FFFFFF;\r\n");
   rsprintf("  border-bottom:1px solid #FFFFFF;\r\n");
   rsprintf("}\r\n");
   rsprintf(".v3 {\r\n");
   rsprintf("  padding:3px;\r\n");
   rsprintf("  font-size:12px;\r\n");
   rsprintf("}\r\n");
   rsprintf("</style>\r\n\r\n");

   /* javascript */
   rsprintf("<script type=\"text/javascript\">\r\n");
   rsprintf("function mscb_edit(index, value)\r\n");
   rsprintf("{\r\n");
   rsprintf("   var new_value = prompt('Please enter new value', value);\r\n");
   rsprintf("   if (new_value != undefined) {\r\n");
   rsprintf("     o = document.createElement('input');\r\n");
   rsprintf("     o.type = 'hidden';\r\n");
   rsprintf("     o.name = 'idx';\r\n");
   rsprintf("     o.value = index;\r\n");
   rsprintf("     document.form1.appendChild(o);\r\n");
   rsprintf("     o = document.createElement('input');\r\n");
   rsprintf("     o.type = 'hidden';\r\n");
   rsprintf("     o.name = 'value';\r\n");
   rsprintf("     o.value = new_value;\r\n");
   rsprintf("     document.form1.appendChild(o);\r\n");
   rsprintf("     document.form1.submit()\r\n");
   rsprintf("   }\n");
   rsprintf("}\r\n");
   rsprintf("</script>\r\n\r\n");


   /* auto refresh */
   if (refresh > 0)
      rsprintf("<meta http-equiv=\"Refresh\" content=\"%02d\">\n", refresh);

   rsprintf("<title>MSCB Interface Page</title></head>\n");

   rsprintf("<body><form name=\"form1\" method=\"GET\" action=\".\">\n\n");

   /* title row */
   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);

   rsprintf("<table border=3 cellpadding=2>\n");
   rsprintf("<tr><th bgcolor=\"#A0A0FF\">MIDAS experiment \"%s\"", str);

   rsprintf("<th bgcolor=\"#A0A0FF\">%s  &nbsp;&nbsp;Refr:%d</tr>\n", ctime(&now), refresh);

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
   rsprintf("<table width=100%%><tr><td>\n");
   rsprintf("<input type=submit name=cmd value=ODB>\n");
   rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("</td>\n");
   rsprintf("<td align=right width=10>");
   rsprintf("<button type=\"button\" onClick=\"document.form1.submit()\">\n");
   rsprintf("<img alt=\"Reload page\" title=\"Reload page\" ");
   rsprintf("src=\"reload.png\"></button></td></tr></table></td></tr>\n\n");

   rsprintf("<tr><td bgcolor=#E0E0E0 colspan=\"2\" cellpadding=\"0\" cellspacing=\"0\">\r\n");

   status = db_find_key(hDB, 0, "MSCB/Submaster", &hKeySubm);
   if (status != DB_SUCCESS) {
      rsprintf("<h1>No MSCB Submasters defined in ODB</h1>\r\n");
      rsprintf("</td></tr>\r\n");
      rsprintf("</table></body></html>\r\n");
      return;
   }

   rsprintf("<table width=\"100%%\" cellpadding=\"0\" cellspacing=\"0\">");

   /*---- submaster list ----*/
   rsprintf("<tr><td class=\"subm\">\r\n");
   rsprintf("Submaster<hr>\r\n");

   rsprintf("<select name=\"subm\" size=20 onChange=\"document.form1.submit();\">\r\n");
   hKeyCurSubm = 0;
   for (i = 0;;i++) {
      db_enum_key(hDB, hKeySubm, i, &hKey);
      if (!hKey)
         break;
      db_get_key(hDB, hKey, &key);
      strcpy(str, key.name);
      size = sizeof(comment);
      if (db_get_value(hDB, hKey, "Comment", comment, &size, TID_STRING, FALSE) == DB_SUCCESS) {
         strcat(str, ": ");
         strlcat(str, comment, sizeof(str));
      }

      if ((cur_subm_name[0] && equal_ustring(cur_subm_name, key.name)) || 
          (cur_subm_name[0] == 0 && i == 0)) {
         cur_subm_index = i;
         rsprintf("<option value=\"%s\" selected>%s</option>\r\n", key.name, str);
         hKeyCurSubm = hKey;
      } else
         rsprintf("<option value=\"%s\">%s</option>\r\n", key.name, str);
   }
   rsprintf("</select>\r\n");

   /*---- node list ----*/
   rsprintf("<td class=\"node\">\r\n");
   rsprintf("Node<hr>\r\n");

   if (!hKeyCurSubm) {
      rsprintf("No submaster found in ODB\r\n");
      return;
   }

   rsprintf("<select name=\"node\" size=20 onChange=\"document.form1.submit();\">\r\n");
   db_find_key(hDB, hKeyCurSubm, "Address", &hKeyAddr);
   assert(hKeyAddr);
   db_get_key(hDB, hKeyAddr, &key);
   size = sizeof(adr);

   /* check if current node is in list */
   for (i = 0; i<key.num_values ;i++) {
      size = sizeof(adr);
      db_get_data_index(hDB, hKeyAddr, &adr, &size, i, TID_INT);
      if (adr == cur_node)
         break;
   }
   if (i == key.num_values) // if not found, use first one in list
      db_get_data_index(hDB, hKeyAddr, &cur_node, &size, 0, TID_INT);

   for (i = 0; i<key.num_values ;i++) {
      size = sizeof(adr);
      db_get_data_index(hDB, hKeyAddr, &adr, &size, i, TID_INT);
      sprintf(str, "%d", adr);
      if (cur_node == 0 && i == 0)
         cur_node = adr;
      if (adr == cur_node)
         rsprintf("<option selected>%s</option>\r\n", str);
      else
         rsprintf("<option>%s</option>\r\n", str);
   }
   rsprintf("</select>\r\n");

   /*---- node contents ----*/
   rsprintf("<td class=\"vars\">\r\n");
   rsprintf("<table>\r\n");
   db_get_key(hDB, hKeyCurSubm, &key);
   rsprintf("<tr><td colspan=3 align=center><b>%s:%d</b>", key.name, cur_node);
   rsprintf("<hr></td></tr>\r\n");
   str[0] = 0;
   size = sizeof(str);
   db_get_value(hDB, hKeyCurSubm, "Pwd", str, &size, TID_STRING, TRUE);

   fd = mscb_init(key.name, 0, str, FALSE);
   if (fd < 0) {
      if (fd == EMSCB_WRONG_PASSWORD)
         rsprintf("<tr><td colspan=3><b>Invalid password</b></td>");
      else
         rsprintf("<tr><td colspan=3><b>Communication problem</b></td>");
      goto mscb_error;
   }
   mscb_set_eth_max_retry(fd, 3);
   mscb_set_max_retry(1);

   status = mscb_ping(fd, cur_node, TRUE);
   if (status != MSCB_SUCCESS) {
      rsprintf("<tr><td colspan=3><b>No response from node</b></td>");
      goto mscb_error;
   }
   status = mscb_info(fd, (unsigned short) cur_node, &info);
   if (status != MSCB_SUCCESS) {
      rsprintf("<tr><td colspan=3><b>No response from node</b></td>");
      goto mscb_error;
   }
   strncpy(str, info.node_name, sizeof(info.node_name));
   str[16] = 0;
   rsprintf("<tr><td class=\"v1\">Node name<td colspan=2 class=\"v2\">%s</tr>\n", str);
   rsprintf("<tr><td class=\"v1\">SVN revision<td colspan=2 class=\"v2\">%d</tr>\n", info.svn_revision);

   if (info.rtc[0] && info.rtc[0] != 0xFF) {
      for (i=0 ; i<6 ; i++)
         info.rtc[i] = (info.rtc[i] / 0x10) * 10 + info.rtc[i] % 0x10;
      rsprintf("<tr><td class=\"v1\">Real Time Clock<td colspan=2 class=\"v2\">%02d-%02d-%02d %02d:%02d:%02d</td>\n",
         info.rtc[0], info.rtc[1], info.rtc[2], 
         info.rtc[3], info.rtc[4], info.rtc[5]);
   }

   status = mscb_uptime(fd, (unsigned short) cur_node, &uptime);
    if (status == MSCB_SUCCESS)
      rsprintf("<tr><td class=\"v1\">Uptime<td colspan=2 class=\"v2\">%dd %02dh %02dm %02ds</tr>\n",
             uptime / (3600 * 24),
             (uptime % (3600 * 24)) / 3600, (uptime % 3600) / 60,
             (uptime % 60));

   rsprintf("<tr><td colspan=3><hr></td></tr>\r\n");

   /* check for hidden variables */
   for (i=0 ; i < info.n_variables ; i++) {
      mscb_info_variable(fd, cur_node, i, &info_var);
      if (info_var.flags & MSCBF_HIDDEN)
         break;
   }
   if (i < info.n_variables) {
      strcpy(str, show_hidden ? " checked" : "");
      rsprintf("<tr><td colspan=3><input type=checkbox%s name=\"hidden\" value=\"1\"", str);
      rsprintf("onChange=\"document.form1.submit();\">Display hidden variables<hr></td></tr>\r\n");
   }

   /* read variables in blocks of 100 bytes */
   for (fi=0 ; fi < info.n_variables ; ) {
      for (i=fi,size=0 ; i < info.n_variables && size < 100; i++) {
         mscb_info_variable(fd, cur_node, i, &info_var);
         size += info_var.width;
      }

      size = sizeof(dbuf);
      status = mscb_read_range(fd, cur_node, fi, i-1, dbuf, &size);
      if (status != MSCB_SUCCESS) {
         rsprintf("<tr><td colspan=3><b>Error reading data from node</b></td>");
         goto mscb_error;
      }
      pd = dbuf;

      for (j=fi ; j<i ; j++) {
         status = mscb_info_variable(fd, cur_node, j, &info_var);
         if ((info_var.flags & MSCBF_HIDDEN) == 0 || show_hidden) {
            memcpy(str, info_var.name, 8);
            str[8] = 0;
            rsprintf("<tr><td class=\"v1\">%s</td>\r\n", str);
            rsprintf("<td class=\"v2\">\r\n");
            print_mscb_var(value, evalue, unit, &info_var, pd);
            rsprintf("<a href=\"#\" onClick=\"mscb_edit(%d,'%s')\">%s</a>", 
               j, evalue, value);
            rsprintf("</td><td class=\"v3\">%s</td>", unit);
            rsprintf("</tr>\r\n");
         }
         pd += info_var.width;
      }

      fi = i;
   }

mscb_error:
   rsprintf("</tr></table>\r\n");
   rsprintf("</td></tr>\r\n");
   rsprintf("</table></body></html>\r\n");
}

#endif // HAVE_MSCB

/*------------------------------------------------------------------*/

void show_password_page(const char *password, const char *experiment)
{
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>Enter password</title></head><body>\n\n");

   rsprintf("<form method=\"GET\" action=\".\">\n\n");

   /* define hidden fields for current experiment */
   if (experiment[0])
      rsprintf("<input type=hidden name=exp value=\"%s\">\n", experiment);

   rsprintf("<table border=1 cellpadding=5>");

   if (password[0])
      rsprintf("<tr><th bgcolor=#FF0000>Wrong password!</tr>\n");

   rsprintf("<tr><th bgcolor=#A0A0FF>Please enter password</tr>\n");
   rsprintf("<tr><td align=center><input type=password name=pwd></tr>\n");
   rsprintf("<tr><td align=center><input type=submit value=Submit></tr>");

   rsprintf("</table>\n");

   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

BOOL check_web_password(const char *password, const char *redir, const char *experiment)
{
   HNDLE hDB, hkey;
   INT size;
   char str[256];

   cm_get_experiment_database(&hDB, NULL);

   /* check for password */
   db_find_key(hDB, 0, "/Experiment/Security/Web Password", &hkey);
   if (hkey) {
      size = sizeof(str);
      db_get_data(hDB, hkey, str, &size, TID_STRING);
      if (strcmp(password, str) == 0)
         return TRUE;

      /* show web password page */
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
      rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

      rsprintf("<html><head>\n");
      rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
      rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
      rsprintf("<title>Enter password</title></head><body>\n\n");

      rsprintf("<form method=\"GET\" action=\".\">\n\n");

      /* define hidden fields for current experiment and destination */
      if (experiment[0])
         rsprintf("<input type=hidden name=exp value=\"%s\">\n", experiment);
      if (redir[0])
         rsprintf("<input type=hidden name=redir value=\"%s\">\n", redir);

      rsprintf("<table border=1 cellpadding=5>");

      if (password[0])
         rsprintf("<tr><th bgcolor=#FF0000>Wrong password!</tr>\n");

      rsprintf
          ("<tr><th bgcolor=#A0A0FF>Please enter password to obtain write access</tr>\n");
      rsprintf("<tr><td align=center><input type=password name=wpwd></tr>\n");
      rsprintf("<tr><td align=center><input type=submit value=Submit></tr>");

      rsprintf("</table>\n");

      rsprintf("</body></html>\r\n");

      return FALSE;
   } else
      return TRUE;
}

/*------------------------------------------------------------------*/

void show_start_page(int script)
{
   int rn, i, j, n, size, status, maxlength;
   HNDLE hDB, hkey, hsubkey, hkeycomm, hkeyc;
   KEY key;
   char data[1000], str[32];
   char data_str[256], comment[1000];

   cm_get_experiment_database(&hDB, NULL);

   if (script) {
      show_header(hDB, "Start sequence", "GET", "", 1, 0);
      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Start script</th>\n");
   } else {
      show_header(hDB, "Start run", "GET", "", 1, 0);

      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Start new run</tr>\n");
      rsprintf("<tr><td>Run number");
      
      /* run number */
      size = sizeof(rn);
      status = db_get_value(hDB, 0, "/Runinfo/Run number", &rn, &size, TID_INT, TRUE);
      assert(status == SUCCESS);
      
      if (rn < 0) { // value "zero" is ok
         cm_msg(MERROR, "show_start_page",
                "aborting on attempt to use invalid run number %d", rn);
         abort();
      }
      
      size = sizeof(i);
      if (db_find_key(hDB, 0, "/Experiment/Edit on start/Edit Run number", &hkey) ==
          DB_SUCCESS && db_get_data(hDB, hkey, &i, &size, TID_BOOL) && i == 0)
         rsprintf("<td><input type=hidden name=value value=%d>%d</tr>\n", rn + 1, rn + 1);
      else
         rsprintf("<td><input type=text size=20 maxlength=80 name=value value=%d></tr>\n",
                  rn + 1);
   }
   /* run parameters */
   if (script)
      db_find_key(hDB, 0, "/Experiment/Edit on sequence", &hkey);
   else
      db_find_key(hDB, 0, "/Experiment/Edit on start", &hkey);
   db_find_key(hDB, 0, "/Experiment/Parameter Comments", &hkeycomm);
   if (hkey) {
      for (i = 0, n = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);

         if (!hsubkey)
            break;

         db_get_link(hDB, hsubkey, &key);
         strlcpy(str, key.name, sizeof(str));

         if (equal_ustring(str, "Edit run number"))
            continue;

         db_enum_key(hDB, hkey, i, &hsubkey);
         db_get_key(hDB, hsubkey, &key);

         size = sizeof(data);
         status = db_get_data(hDB, hsubkey, data, &size, key.type);
         if (status != DB_SUCCESS)
            continue;

         for (j = 0; j < key.num_values; j++) {
            if (key.num_values > 1)
               rsprintf("<tr><td>%s [%d]", str, j);
            else
               rsprintf("<tr><td>%s", str);

            if (j == 0 && hkeycomm) {
               /* look for comment */
               if (db_find_key(hDB, hkeycomm, key.name, &hkeyc) == DB_SUCCESS) {
                  size = sizeof(comment);
                  if (db_get_data(hDB, hkeyc, comment, &size, TID_STRING) == DB_SUCCESS)
                     rsprintf("<br>%s\n", comment);
               }
            }

            db_sprintf(data_str, data, key.item_size, j, key.type);

            maxlength = 80;
            if (key.type == TID_STRING)
               maxlength = key.item_size;

            if (key.type == TID_BOOL) {
               if (((DWORD*)data)[j])
                  rsprintf("<td><input type=checkbox checked name=x%d value=1></td></tr>\n", n++);
               else
                  rsprintf("<td><input type=checkbox name=x%d value=1></td></tr>\n", n++);
            } else
               rsprintf("<td><input type=text size=%d maxlength=%d name=x%d value=\"%s\"></tr>\n",
                        (maxlength<80)?maxlength:80, maxlength-1, n++, data_str);
         }
      }
   }

   rsprintf("<tr><td align=center colspan=2>\n");
   if (script) {
      rsprintf("<input type=submit name=cmd value=\"Start Script\">\n");
      rsprintf("<input type=hidden name=params value=1>\n");
   } else
      rsprintf("<input type=submit name=cmd value=Start>\n");
   rsprintf("<input type=submit name=cmd value=Cancel>\n");
   rsprintf("</tr>\n");
   rsprintf("</table>\n");

   if (isparam("redir"))
      rsprintf("<input type=hidden name=\"redir\" value=\"%s\">\n", getparam("redir"));

   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_odb_page(char *enc_path, int enc_path_size, char *dec_path)
{
   int i, j, size, status;
   char str[256], tmp_path[256], url_path[256], data_str[TEXT_SIZE], 
      hex_str[256], ref[256], keyname[32], link_name[256], link_ref[256],
      full_path[256], root_path[256];
   char *p, *pd;
   char data[TEXT_SIZE];
   HNDLE hDB, hkey, hkeyroot;
   KEY key;

   cm_get_experiment_database(&hDB, NULL);

   if (strcmp(enc_path, "root") == 0) {
      strcpy(enc_path, "");
      strcpy(dec_path, "");
   }

   strlcpy(str, dec_path, sizeof(str));
   if (strrchr(str, '/'))
      strlcpy(str, strrchr(str, '/')+1, sizeof(str));
   show_header(hDB, "MIDAS online database", "GET", str, 1, 0);

   /* find key via path */
   status = db_find_key(hDB, 0, dec_path, &hkeyroot);
   if (status != DB_SUCCESS) {
      rsprintf("Error: cannot find key %s<P>\n", dec_path);
      rsprintf("</body></html>\r\n");
      return;
   }

   /* if key is not of type TID_KEY, cut off key name */
   db_get_key(hDB, hkeyroot, &key);
   if (key.type != TID_KEY) {
      /* strip variable name from path */
      p = dec_path + strlen(dec_path) - 1;
      while (*p && *p != '/')
         *p-- = 0;
      if (*p == '/')
         *p = 0;

      strlcpy(enc_path, dec_path, enc_path_size);
      urlEncode(enc_path, enc_path_size);

      status = db_find_key(hDB, 0, dec_path, &hkeyroot);
      if (status != DB_SUCCESS) {
         rsprintf("Error: cannot find key %s<P>\n", dec_path);
         rsprintf("</body></html>\r\n");
         return;
      }
   }

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=2 bgcolor=#A0A0A0>\n");
   if (elog_mode) {
      rsprintf("<input type=submit name=cmd value=ELog>\n");
      rsprintf("</tr>\n");
   } else {
      rsprintf("<input type=submit name=cmd value=Find>\n");
      rsprintf("<input type=submit name=cmd value=Create>\n");
      rsprintf("<input type=submit name=cmd value=Delete>\n");
      rsprintf("<input type=submit name=cmd value=Alarms>\n");
      rsprintf("<input type=submit name=cmd value=Programs>\n");
      rsprintf("<input type=submit name=cmd value=Status>\n");
      rsprintf("<input type=submit name=cmd value=Help>\n");
      rsprintf("</tr>\n");

      rsprintf("<tr><td colspan=2 bgcolor=#A0A0A0>\n");
      rsprintf("<input type=submit name=cmd value=\"Create Elog from this page\">\n");
      rsprintf("</tr>\n");
   }

   /*---- ODB display -----------------------------------------------*/

   /* add one "../" for each level */
   tmp_path[0] = 0;
   for (p = dec_path ; *p ; p++)
      if (*p == '/')
         strlcat(tmp_path, "../", sizeof(tmp_path));

   p = dec_path;
   if (*p == '/')
      p++;

   /* display root key */
   rsprintf("<tr><td colspan=2 align=center><b>");
   rsprintf("<a href=\"%sroot\">/</a> \n", tmp_path);
   strlcpy(root_path, tmp_path, sizeof(root_path));

   /*---- display path ----*/
   while (*p) {
      pd = str;
      while (*p && *p != '/')
         *pd++ = *p++;
      *pd = 0;

      strlcat(tmp_path, str, sizeof(tmp_path));
      strlcpy(url_path, tmp_path, sizeof(url_path));
      urlEncode(url_path, sizeof(url_path));

      rsprintf("<a href=\"%s\">%s</a>\n / ", url_path, str);

      strlcat(tmp_path, "/", sizeof(tmp_path));
      if (*p == '/')
         p++;
   }
   rsprintf("</b></tr>\n");

   rsprintf("<tr><th>Key<th>Value</tr>\n");

   /* enumerate subkeys */
   for (i = 0;; i++) {
      db_enum_link(hDB, hkeyroot, i, &hkey);
      if (!hkey)
         break;
      db_get_link(hDB, hkey, &key);

      if (strrchr(dec_path, '/'))
         strlcpy(str, strrchr(dec_path, '/')+1, sizeof(str));
      else
         strlcpy(str, dec_path, sizeof(str));
      if (str[0] && str[strlen(str) - 1] != '/')
         strlcat(str, "/", sizeof(str));
      strlcat(str, key.name, sizeof(str));
      strlcpy(full_path, str, sizeof(full_path));
      urlEncode(full_path, sizeof(full_path));
      strlcpy(keyname, key.name, sizeof(keyname));

      /* resolve links */
      link_name[0] = 0;
      status = DB_SUCCESS;
      if (key.type == TID_LINK) {
         size = sizeof(link_name);
         db_get_link_data(hDB, hkey, link_name, &size, TID_LINK);
         status = db_enum_key(hDB, hkeyroot, i, &hkey);
         db_get_key(hDB, hkey, &key);
      }

      if (link_name[0]) {
         if (root_path[strlen(root_path)-1] == '/' && link_name[0] == '/')
            sprintf(ref, "%s%s?cmd=Set", root_path, link_name+1);
         else
            sprintf(ref, "%s%s?cmd=Set", root_path, link_name);
         sprintf(link_ref, "%s?cmd=Set", full_path);
      } else
         sprintf(ref, "%s?cmd=Set", full_path);

      if (status != DB_SUCCESS) {
         rsprintf("<tr><td bgcolor=#FFFF00>");
         rsprintf("%s <i>-> <a href=\"%s\">%s</a></i><td><b><font color=\"red\">&lt;cannot resolve link&gt;</font><b></tr>\n",
              keyname, link_ref, link_name);
      } else {
         if (key.type == TID_KEY) {
            /* for keys, don't display data value */
            rsprintf("<tr><td colspan=2 bgcolor=#FFD000><a href=\"%s\">%s</a><br></tr>\n",
                    full_path, keyname);
         } else {
            /* display single value */
            if (key.num_values == 1) {
               size = sizeof(data);
               db_get_data(hDB, hkey, data, &size, key.type);
               db_sprintf(data_str, data, key.item_size, 0, key.type);

               if (key.type != TID_STRING)
                  db_sprintfh(hex_str, data, key.item_size, 0, key.type);
               else
                  hex_str[0] = 0;

               if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>")) {
                  strcpy(data_str, "(empty)");
                  hex_str[0] = 0;
               }

               if (strcmp(data_str, hex_str) != 0 && hex_str[0]) {
                  if (link_name[0]) {
                     rsprintf("<tr><td bgcolor=#FFFF00>");
                     rsprintf("%s <i>-> <a href=\"%s\">%s</a></i><td><a href=\"%s\">%s (%s)</a><br></tr>\n",
                          keyname, link_ref, link_name, ref, data_str, hex_str);
                  } else {
                     rsprintf("<tr><td bgcolor=#FFFF00>");
                     rsprintf("%s<td><a href=\"%s\">%s (%s)</a><br></tr>\n",
                              keyname, ref, data_str, hex_str);
                  }
               } else {
                  if (strchr(data_str, '\n')) {
                     if (link_name[0]) {
                        rsprintf("<tr><td bgcolor=#FFFF00>");
                        rsprintf("%s <i>-> <a href=\"%s\">%s</a></i><td>", keyname, link_ref, link_name);
                     } else
                        rsprintf("<tr><td bgcolor=#FFFF00>%s<td>", keyname);
                     rsprintf("\n<pre>");
                     strencode3(data_str);
                     rsprintf("</pre>");
                     if (strlen(data) > strlen(data_str))
                        rsprintf("<i>... (%d bytes total)<p>\n", strlen(data));

                     rsprintf("<a href=\"%s\">Edit</a></tr>\n", ref);
                  } else {
                     if (link_name[0]) {
                        rsprintf("<tr><td bgcolor=#FFFF00>");
                        rsprintf("%s <i>-> <a href=\"%s\">%s</a></i><td><a href=\"%s\">",
                             keyname, link_ref, link_name, ref);
                     } else
                        rsprintf("<tr><td bgcolor=#FFFF00>%s<td><a href=\"%s\">", keyname,
                                 ref);
                     strencode(data_str);
                     rsprintf("</a><br></tr>\n");
                  }
               }
            } else {
               /* check for exceeding length */
               if (key.num_values > 1000)
                  rsprintf("<tr><td bgcolor=#FFFF00>%s<td><i>... %d values ...</i>\n",
                           keyname, key.num_values);
               else {
                  /* display first value */
                  if (link_name[0])
                     rsprintf("<tr><td  bgcolor=#FFFF00 rowspan=%d>%s<br><i>-> %s</i>\n",
                              key.num_values, keyname, link_name);
                  else
                     rsprintf("<tr><td  bgcolor=#FFFF00 rowspan=%d>%s\n", key.num_values,
                              keyname);

                  for (j = 0; j < key.num_values; j++) {
                     size = sizeof(data);
                     db_get_data_index(hDB, hkey, data, &size, j, key.type);
                     db_sprintf(data_str, data, key.item_size, 0, key.type);
                     db_sprintfh(hex_str, data, key.item_size, 0, key.type);

                     if (data_str[0] == 0 || equal_ustring(data_str, "<NULL>")) {
                        strcpy(data_str, "(empty)");
                        hex_str[0] = 0;
                     }

                     sprintf(ref, "%s?cmd=Set&index=%d", full_path, j);

                     if (j > 0)
                        rsprintf("<tr>");

                     if (strcmp(data_str, hex_str) != 0 && hex_str[0])
                        rsprintf("<td><a href=\"%s\">[%d] %s (%s)</a><br></tr>\n", ref, j,
                                 data_str, hex_str);
                     else
                        rsprintf("<td><a href=\"%s\">[%d] %s</a><br></tr>\n", ref, j,
                                 data_str);
                  }
               }
            }
         }
      }
   }

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_set_page(char *enc_path, int enc_path_size, char *dec_path, const char *group,
                   int index, const char *value)
{
   int status, size;
   HNDLE hDB, hkey;
   KEY key;
   char data_str[TEXT_SIZE], str[256], *p, eq_name[NAME_LENGTH];
   char data[TEXT_SIZE];

   cm_get_experiment_database(&hDB, NULL);

   /* show set page if no value is given */
   if (!isparam("value") && !*getparam("text")) {
      status = db_find_link(hDB, 0, dec_path, &hkey);
      if (status != DB_SUCCESS) {
         rsprintf("Error: cannot find key %s<P>\n", dec_path);
         return;
      }
      db_get_key(hDB, hkey, &key);

      strlcpy(str, dec_path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      show_header(hDB, "Set value", "POST", str, 1, 0);

      if (index > 0)
         rsprintf("<input type=hidden name=index value=\"%d\">\n", index);
      else
         index = 0;

      if (group[0])
         rsprintf("<input type=hidden name=group value=\"%s\">\n", group);

      strlcpy(data_str, rpc_tid_name(key.type), sizeof(data_str));
      if (key.num_values > 1) {
         sprintf(str, "[%d]", key.num_values);
         strlcat(data_str, str, sizeof(data_str));

         sprintf(str, "%s[%d]", dec_path, index);
      } else
         strlcpy(str, dec_path, sizeof(str));

      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Set new value - type = %s</tr>\n",
               data_str);
      rsprintf("<tr><td bgcolor=#FFFF00>%s<td>\n", str);

      /* set current value as default */
      size = sizeof(data);
      db_get_data(hDB, hkey, data, &size, key.type);
      db_sprintf(data_str, data, key.item_size, index, key.type);

      if (equal_ustring(data_str, "<NULL>"))
         data_str[0] = 0;

      if (strchr(data_str, '\n') != NULL) {
         rsprintf("<textarea rows=20 cols=80 name=\"text\">\n");
         strencode3(data);
         rsprintf("</textarea>\n");
      } else {
         size = 20;
         if ((int) strlen(data_str) > size)
            size = strlen(data_str) + 3;
         if (size > 80)
            size = 80;

         rsprintf("<input type=\"text\" size=%d maxlength=256 name=\"value\" value=\"", size);
         strencode(data_str);
         rsprintf("\">\n");
      }

      rsprintf("</tr>\n");

      rsprintf("<tr><td align=center colspan=2>");
      rsprintf("<input type=submit name=cmd value=Set>");
      rsprintf("<input type=submit name=cmd value=Cancel>");
      rsprintf("</tr>");
      rsprintf("</table>");

      rsprintf("<input type=hidden name=cmd value=Set>\n");

      rsprintf("</body></html>\r\n");
      return;
   } else {
      /* set value */

      status = db_find_link(hDB, 0, dec_path, &hkey);
      if (status != DB_SUCCESS) {
         rsprintf("Error: cannot find key %s<P>\n", dec_path);
         return;
      }
      db_get_key(hDB, hkey, &key);

      memset(data, 0, sizeof(data));

      if (*getparam("text"))
         strlcpy(data, getparam("text"), sizeof(data));
      else
         db_sscanf(value, data, &size, 0, key.type);

      if (index < 0)
         index = 0;

      /* extend data size for single string if necessary */
      if ((key.type == TID_STRING || key.type == TID_LINK)
          && (int) strlen(data) + 1 > key.item_size && key.num_values == 1)
         key.item_size = strlen(data) + 1;

      if (key.item_size == 0)
         key.item_size = rpc_tid_size(key.type);

      if (key.num_values > 1)
         status = db_set_link_data_index(hDB, hkey, data, key.item_size, index, key.type);
      else
         status = db_set_link_data(hDB, hkey, data, key.item_size, 1, key.type);

      if (status == DB_NO_ACCESS)
         rsprintf("<h2>Write access not allowed</h2>\n");

      /* strip variable name from path */
      p = dec_path + strlen(dec_path) - 1;
      while (*p && *p != '/')
         *p-- = 0;
      if (*p == '/')
         *p = 0;

      //strlcpy(enc_path, dec_path, enc_path_size);
      //urlEncode(enc_path, enc_path_size);
      enc_path[0] = 0;

      /* redirect */

      if (group[0]) {
         /* extract equipment name */
         eq_name[0] = 0;
         if (strncmp(enc_path, "Equipment/", 10) == 0) {
            strlcpy(eq_name, enc_path + 10, sizeof(eq_name));
            if (strchr(eq_name, '/'))
               *strchr(eq_name, '/') = 0;
         }

         /* back to SC display */
         sprintf(str, "SC/%s/%s", eq_name, group);
         redirect(str);
      } else
         redirect(enc_path);

      return;
   }

}

/*------------------------------------------------------------------*/

void show_find_page(const char *enc_path, const char *value)
{
   HNDLE hDB, hkey;
   char str[256];

   cm_get_experiment_database(&hDB, NULL);

   if (value[0] == 0) {
      /* without value, show find dialog */
      str[0] = 0;
      for (const char* p=enc_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      show_header(hDB, "Find value", "GET", str, 1, 0);

      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Find string in Online Database</tr>\n");
      rsprintf("<tr><td>Enter substring (case insensitive)\n");

      rsprintf("<td><input type=\"text\" size=\"20\" maxlength=\"80\" name=\"value\">\n");
      rsprintf("</tr>");

      rsprintf("<tr><td align=center colspan=2>");
      rsprintf("<input type=submit name=cmd value=Find>");
      rsprintf("<input type=submit name=cmd value=Cancel>");
      rsprintf("</tr>");
      rsprintf("</table>");

      rsprintf("<input type=hidden name=cmd value=Find>");

      rsprintf("</body></html>\r\n");
   } else {
      strlcpy(str, enc_path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      show_header(hDB, "Search results", "GET", str, 1, 0);

      rsprintf("<tr><td colspan=2 bgcolor=#A0A0A0>\n");
      rsprintf("<input type=submit name=cmd value=Find>\n");
      rsprintf("<input type=submit name=cmd value=ODB>\n");
      rsprintf("<input type=submit name=cmd value=Help>\n");
      rsprintf("</tr>\n\n");

      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>");
      rsprintf("Results of search for substring \"%s\"</tr>\n", value);
      rsprintf("<tr><th>Key<th>Value</tr>\n");

      /* start from root */
      db_find_key(hDB, 0, "", &hkey);
      assert(hkey);

      /* scan tree, call "search_callback" for each key */
      db_scan_tree(hDB, hkey, 0, search_callback, (void *) value);

      rsprintf("</table>");
      rsprintf("</body></html>\r\n");
   }
}

/*------------------------------------------------------------------*/

void show_create_page(const char *enc_path, const char *dec_path, const char *value, int index, int type)
{
   char str[256], link[256], error[256], *p;
   char data[10000];
   int status;
   HNDLE hDB, hkey;
   KEY key;

   cm_get_experiment_database(&hDB, NULL);

   if (value[0] == 0) {
      /* without value, show create dialog */

      strlcpy(str, enc_path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      show_header(hDB, "Create ODB entry", "GET", str, 1, 0);

      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Create ODB entry</tr>\n");

      rsprintf("<tr><td>Type");
      rsprintf("<td><select type=text size=1 name=type>\n");

      rsprintf("<option value=7> Integer (32-bit)\n");
      rsprintf("<option value=9> Float (4 Bytes)\n");
      rsprintf("<option value=12> String\n");
      rsprintf("<option value=13> Multi-line String\n");
      rsprintf("<option value=15> Subdirectory\n");

      rsprintf("<option value=1> Byte\n");
      rsprintf("<option value=2> Signed byte\n");
      rsprintf("<option value=3> Character (8-bit)\n");
      rsprintf("<option value=4> Word (16-bit)\n");
      rsprintf("<option value=5> Short integer(16-bit)\n");
      rsprintf("<option value=6> Double Word (32-bit)\n");
      rsprintf("<option value=8> Boolean\n");
      rsprintf("<option value=10> Double float(8 Bytes)\n");
      rsprintf("<option value=16> Symbolic link\n");

      rsprintf("</select></tr>\n");

      rsprintf("<tr><td>Name");
      rsprintf("<td><input type=text size=20 maxlength=80 name=value>\n");
      rsprintf("</tr>");

      rsprintf("<tr><td>Array size");
      rsprintf("<td><input type=text size=20 maxlength=80 name=index value=1>\n");
      rsprintf("</tr>");

      rsprintf("<tr><td align=center colspan=2>");
      rsprintf("<input type=submit name=cmd value=Create>");
      rsprintf("<input type=submit name=cmd value=Cancel>");
      rsprintf("</tr>");
      rsprintf("</table>");

      rsprintf("</body></html>\r\n");
   } else {
      if (type == TID_LINK) {
         /* check if destination exists */
         status = db_find_key(hDB, 0, value, &hkey);
         if (status != DB_SUCCESS) {
            rsprintf("<h1>Error: Link destination \"%s\" does not exist!</h1>", value);
            return;
         }

         /* extract key name from destination */
         strlcpy(str, value, sizeof(str));
         p = str + strlen(str) - 1;
         while (*p && *p != '/')
            p--;
         p++;

         /* use it as link name */
         strlcpy(link, p, sizeof(link));

         strlcpy(str, dec_path, sizeof(str));
         if (str[strlen(str) - 1] != '/')
            strlcat(str, "/", sizeof(str));
         strlcat(str, link, sizeof(str));

         status = db_create_link(hDB, 0, str, value);
         if (status != DB_SUCCESS) {
            sprintf(error, "Cannot create key %s</h1>\n", str);
            show_error(error);
            return;
         }

      } else {
         if (dec_path[0] != '/')
            strcpy(str, "/");
         else
            str[0] = 0;
         strlcat(str, dec_path, sizeof(str));
         if (str[strlen(str) - 1] != '/')
            strlcat(str, "/", sizeof(str));
         strlcat(str, value, sizeof(str));

         if (type == TID_ARRAY)
            /* multi-line string */
            status = db_create_key(hDB, 0, str, TID_STRING);
         else
            status = db_create_key(hDB, 0, str, type);
         if (status != DB_SUCCESS) {
            sprintf(error, "Cannot create key %s</h1>\n", str);
            show_error(error);
            return;
         }

         db_find_key(hDB, 0, str, &hkey);
	 assert(hkey);
         db_get_key(hDB, hkey, &key);
         memset(data, 0, sizeof(data));
         if (key.type == TID_STRING || key.type == TID_LINK)
            key.item_size = NAME_LENGTH;
         if (type == TID_ARRAY)
            strcpy(data, "\n");

         if (index > 1)
            db_set_data_index(hDB, hkey, data, key.item_size, index - 1, key.type);
         else if (key.type == TID_STRING || key.type == TID_LINK)
            db_set_data(hDB, hkey, data, key.item_size, 1, key.type);
      }

      /* redirect */
      strlcpy(str, enc_path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      redirect(str);
      return;
   }
}

/*------------------------------------------------------------------*/

void show_delete_page(const char *enc_path, const char *dec_path, const char *value, int index)
{
   char str[256];
   int i, status;
   HNDLE hDB, hkeyroot, hkey;
   KEY key;

   cm_get_experiment_database(&hDB, NULL);

   if (value[0] == 0) {
      /* without value, show delete dialog */

      strlcpy(str, enc_path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      show_header(hDB, "Delete ODB entry", "GET", str, 1, 0);

      rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Delete ODB entry</tr>\n");

      /* find key via from */
      status = db_find_key(hDB, 0, dec_path, &hkeyroot);
      if (status != DB_SUCCESS) {
         rsprintf("Error: cannot find key %s<P>\n", dec_path);
         rsprintf("</body></html>\r\n");
         return;
      }

      /* count keys */
      for (i = 0;; i++) {
         db_enum_key(hDB, hkeyroot, i, &hkey);
         if (!hkey)
            break;
      }

      rsprintf("<tr><td align=center colspan=2><select type=text size=%d name=value>\n",
               i);

      /* enumerate subkeys */
      for (i = 0;; i++) {
         db_enum_link(hDB, hkeyroot, i, &hkey);
         if (!hkey)
            break;
         db_get_link(hDB, hkey, &key);
         rsprintf("<option> %s\n", key.name);
      }

      rsprintf("</select></tr>\n");

      rsprintf("<tr><td align=center colspan=2>");
      rsprintf("<input type=submit name=cmd value=Delete>");
      rsprintf("<input type=submit name=cmd value=Cancel>");
      rsprintf("</tr>");
      rsprintf("</table>");

      rsprintf("</body></html>\r\n");
   } else {
      strlcpy(str, dec_path, sizeof(str));
      if (str[strlen(str) - 1] != '/')
         strlcat(str, "/", sizeof(str));
      strlcat(str, value, sizeof(str));

      status = db_find_link(hDB, 0, str, &hkey);
      if (status != DB_SUCCESS) {
         rsprintf("<h1>Cannot find key %s</h1>\n", str);
         return;
      }

      status = db_delete_key(hDB, hkey, FALSE);
      if (status != DB_SUCCESS) {
         rsprintf("<h1>Cannot delete key %s</h1>\n", str);
         return;
      }

      /* redirect */
      strlcpy(str, enc_path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      redirect(str);
      return;
   }
}

/*------------------------------------------------------------------*/

void show_alarm_page()
{
   INT i, size, triggered, type, index, ai;
   BOOL active;
   HNDLE hDB, hkeyroot, hkey;
   KEY key;
   char str[256], ref[256], condition[256], value[256];
   time_t now, last, interval;
   INT al_list[] = { AT_EVALUATED, AT_PROGRAM, AT_INTERNAL, AT_PERIODIC };

   cm_get_experiment_database(&hDB, NULL);

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");

   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   rsprintf("<title>Alarms</title></head>\n");
   rsprintf("<body>\n");

   /* title row */
   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);

   rsprintf("<form method=\"GET\" action=\".\">\n");

   rsprintf("<table border=3 cellpadding=2>\n");
   rsprintf("<tr><th colspan=4 bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
   rsprintf("<th colspan=3 bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));

   /*---- menu buttons ----*/

   rsprintf("<tr>\n");
   rsprintf("<td colspan=7 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=\"Reset all alarms\">\n");
   rsprintf("<input type=submit name=cmd value=\"Alarms on/off\">\n");
   rsprintf("<input type=submit name=cmd value=Status>\n");

   rsprintf("</tr></form>\n\n");

   /*---- global flag ----*/

   active = TRUE;
   size = sizeof(active);
   db_get_value(hDB, 0, "/Alarms/Alarm System active", &active, &size, TID_BOOL, TRUE);
   if (!active) {
      sprintf(ref, "Alarms/Alarm System active?cmd=set");
      rsprintf("<tr><td align=center colspan=7 bgcolor=#FFC0C0><a href=\"%s\"><h1>Alarm system disabled</h1></a></tr>",
           ref);
   }

   /*---- alarms ----*/

   for (ai = 0; ai < AT_LAST; ai++) {
      index = al_list[ai];

      if (index == AT_EVALUATED) {
         rsprintf
             ("<tr><th align=center colspan=7 bgcolor=#C0C0C0>Evaluated alarms</tr>\n");
         rsprintf
             ("<tr><th>Alarm<th>State<th>First triggered<th>Class<th>Condition<th>Current value<th></tr>\n");
      } else if (index == AT_PROGRAM) {
         rsprintf("<tr><th align=center colspan=7 bgcolor=#C0C0C0>Program alarms</tr>\n");
         rsprintf
             ("<tr><th>Alarm<th>State<th>First triggered<th>Class<th colspan=2>Condition<th></tr>\n");
      } else if (index == AT_INTERNAL) {
         rsprintf
             ("<tr><th align=center colspan=7 bgcolor=#C0C0C0>Internal alarms</tr>\n");
         rsprintf
             ("<tr><th>Alarm<th>State<th>First triggered<th>Class<th colspan=2>Condition/Message<th></tr>\n");
      } else if (index == AT_PERIODIC) {
         rsprintf
             ("<tr><th align=center colspan=7 bgcolor=#C0C0C0>Periodic alarms</tr>\n");
         rsprintf
             ("<tr><th>Alarm<th>State<th>First triggered<th>Class<th colspan=2>Time/Message<th></tr>\n");
      }

      /* go through all alarms */
      db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
      if (hkeyroot) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkeyroot, i, &hkey);

            if (!hkey)
               break;

            db_get_key(hDB, hkey, &key);

            /* type */
            size = sizeof(INT);
            db_get_value(hDB, hkey, "Type", &type, &size, TID_INT, TRUE);
            if (type != index)
               continue;

            /* start form for each alarm to make "reset" button work */
            sprintf(ref, "%s", key.name);

            rsprintf("<form method=\"GET\" action=\"%s\">\n", ref);

            /* alarm name */
            sprintf(ref, "Alarms/Alarms/%s", key.name);
            rsprintf("<tr><td bgcolor=#C0C0FF><a href=\"%s\"><b>%s</b></a>", ref,
                     key.name);

            /* state */
            size = sizeof(BOOL);
            db_get_value(hDB, hkey, "Active", &active, &size, TID_BOOL, TRUE);
            size = sizeof(INT);
            db_get_value(hDB, hkey, "Triggered", &triggered, &size, TID_INT, TRUE);
            if (!active)
               rsprintf("<td bgcolor=#FFFF00 align=center>Disabled");
            else {
               if (!triggered)
                  rsprintf("<td bgcolor=#00FF00 align=center>OK");
               else
                  rsprintf("<td bgcolor=#FF0000 align=center>Triggered");
            }

            /* time */
            size = sizeof(str);
            db_get_value(hDB, hkey, "Time triggered first", str, &size, TID_STRING, TRUE);
            if (!triggered)
               strcpy(str, "-");
            rsprintf("<td align=center>%s", str);

            /* class */
            size = sizeof(str);
            db_get_value(hDB, hkey, "Alarm Class", str, &size, TID_STRING, TRUE);

            sprintf(ref, "Alarms/Classes/%s", str);
            rsprintf("<td align=center><a href=\"%s\">%s</a>", ref, str);

            /* condition */
            size = sizeof(condition);
            db_get_value(hDB, hkey, "Condition", condition, &size, TID_STRING, TRUE);

            if (index == AT_EVALUATED) {
               /* print condition */
               rsprintf("<td>");
               strencode(condition);

               /* retrieve value */
               al_evaluate_condition(condition, value);
               rsprintf("<td align=center bgcolor=#C0C0FF>%s", value);
            } else if (index == AT_PROGRAM) {
               /* print condition */
               rsprintf("<td colspan=2>");
               strencode(condition);
            } else if (index == AT_INTERNAL) {
               size = sizeof(str);
               if (triggered)
                  db_get_value(hDB, hkey, "Alarm message", str, &size, TID_STRING, TRUE);
               else
                  db_get_value(hDB, hkey, "Condition", str, &size, TID_STRING, TRUE);

               rsprintf("<td colspan=2>%s", str);
            } else if (index == AT_PERIODIC) {
               size = sizeof(str);
               if (triggered)
                  db_get_value(hDB, hkey, "Alarm message", str, &size, TID_STRING, TRUE);
               else {
                  size = sizeof(last);
                  db_get_value(hDB, hkey, "Checked last", &last, &size, TID_DWORD, TRUE);
                  if (last == 0) {
                     last = ss_time();
                     db_set_value(hDB, hkey, "Checked last", &last, size, 1, TID_DWORD);
                  }

                  size = sizeof(interval);
                  db_get_value(hDB, hkey, "Check interval", &interval, &size, TID_INT,
                               TRUE);
                  last += interval;
                  
                  if (ctime(&last) == NULL)
                     strcpy(value, "<invalid>");
                  else
                     strcpy(value, ctime(&last));
                  value[16] = 0;

                  sprintf(str, "Alarm triggers at %s", value);
               }

               rsprintf("<td colspan=2>%s", str);
            }

            rsprintf("<td>\n");
            if (triggered)
               rsprintf("<input type=submit name=cmd value=\"Reset\">\n");
            else
               rsprintf(" &nbsp;&nbsp;&nbsp;&nbsp;");

            rsprintf("</tr>\n");
            rsprintf("</form>\n");
         }
      }
   }

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_programs_page()
{
   INT i, j, k, size, count, status;
   BOOL restart, first, required, client;
   HNDLE hDB, hkeyroot, hkey, hkey_rc, hkeycl;
   KEY key, keycl;
   char str[256], ref[256], command[256], name[80], name_addon[80];

   cm_get_experiment_database(&hDB, NULL);

   /* stop command */
   if (*getparam("Stop")) {
      status = cm_shutdown(getparam("Stop") + 5, FALSE);

      if (status == CM_SUCCESS)
         redirect("./?cmd=programs");
      else {
         sprintf(str,
                 "Cannot shut down client \"%s\", please kill manually and do an ODB cleanup",
                 getparam("Stop") + 5);
         show_error(str);
      }

      return;
   }

   /* start command */
   if (*getparam("Start")) {
      /* for NT: close reply socket before starting subprocess */
      redirect2("./?cmd=programs");

      strlcpy(name, getparam("Start") + 6, sizeof(name));
      if (strchr(name, '?'))
         *strchr(name, '?') = 0;
      strlcpy(str, "/Programs/", sizeof(str));
      strlcat(str, name, sizeof(str));
      strlcat(str, "/Start command", sizeof(str));
      command[0] = 0;
      size = sizeof(command);
      db_get_value(hDB, 0, str, command, &size, TID_STRING, FALSE);
      if (command[0]) {
         ss_system(command);
         for (i = 0; i < 50; i++) {
            if (cm_exist(name, FALSE) == CM_SUCCESS)
               break;
            ss_sleep(100);
         }
      }

      return;
   }

   show_header(hDB, "Programs", "GET", "", 3, 0);

   /*---- menu buttons ----*/

   rsprintf("<tr><td colspan=6 bgcolor=#C0C0C0>\n");

   rsprintf("<input type=submit name=cmd value=Alarms>\n");
   rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("</tr>\n\n");

   rsprintf("<input type=hidden name=cmd value=Programs>\n");

   /*---- programs ----*/

   rsprintf("<tr><th>Program<th>Running on host<th>Alarm class<th>Autorestart</tr>\n");

   /* go through all programs */
   db_find_key(hDB, 0, "/Programs", &hkeyroot);
   if (hkeyroot) {
      for (i = 0;; i++) {
         db_enum_key(hDB, hkeyroot, i, &hkey);

         if (!hkey)
            break;

         db_get_key(hDB, hkey, &key);

         /* skip "execute on xxxx" */
         if (key.type != TID_KEY)
            continue;

         sprintf(ref, "Programs/%s", key.name);

         /* required? */
         size = sizeof(required);
         db_get_value(hDB, hkey, "Required", &required, &size, TID_BOOL, TRUE);

         /* running */
         count = 0;
         if (db_find_key(hDB, 0, "/System/Clients", &hkey_rc) == DB_SUCCESS) {
            first = TRUE;
            for (j = 0;; j++) {
               db_enum_key(hDB, hkey_rc, j, &hkeycl);
               if (!hkeycl)
                  break;

               size = sizeof(name);
               memset(name, 0, size);
               db_get_value(hDB, hkeycl, "Name", name, &size, TID_STRING, TRUE);

               db_get_key(hDB, hkeycl, &keycl);
               
               // check if client name is longer than the program name
               // necessary for multiple started processes which will have names
               // as client_name, client_name1, ...
               client = TRUE;
               if (strlen(name) > strlen(key.name)) {
                  size = strlen(name)-strlen(key.name);
                  memcpy(name_addon, name+strlen(key.name), size);
                  name_addon[size] = 0;
                  for (k=0; k<size; k++) {
                     if (!isdigit(name_addon[k])) {
                        client = FALSE;
                        break;
                     }
                  }  
               }
               name[strlen(key.name)] = 0;

               if (equal_ustring(name, key.name) && client) {
                  size = sizeof(str);
                  db_get_value(hDB, hkeycl, "Host", str, &size, TID_STRING, TRUE);

                  if (first) {
                     rsprintf("<tr><td bgcolor=#C0C0FF><a href=\"%s\"><b>%s</b></a>", ref,
                              key.name);
                     rsprintf("<td align=center bgcolor=#00FF00>");
                  }
                  if (!first)
                     rsprintf("<br>");
                  rsprintf(str);

                  first = FALSE;
                  count++;
               }
            }
         }

         if (count == 0 && required) {
            rsprintf("<tr><td bgcolor=#C0C0FF><a href=\"%s\"><b>%s</b></a>", ref,
                     key.name);
            rsprintf("<td align=center bgcolor=#FF0000>Not running");
         }

         /* dont display non-running programs which are not required */
         if (count == 0 && !required)
            continue;

         /* Alarm */
         size = sizeof(str);
         db_get_value(hDB, hkey, "Alarm Class", str, &size, TID_STRING, TRUE);
         if (str[0]) {
            sprintf(ref, "Alarms/Classes/%s", str);
            rsprintf("<td bgcolor=#FFFF00 align=center><a href=\"%s\">%s</a>", ref, str);
         } else
            rsprintf("<td align=center>-");

         /* auto restart */
         size = sizeof(restart);
         db_get_value(hDB, hkey, "Auto restart", &restart, &size, TID_BOOL, TRUE);

         if (restart)
            rsprintf("<td align=center>Yes\n");
         else
            rsprintf("<td align=center>No\n");

         /* start/stop button */
         size = sizeof(str);
         db_get_value(hDB, hkey, "Start Command", str, &size, TID_STRING, TRUE);
         if (str[0] && count == 0) {
            sprintf(str, "Start %s", key.name);
            rsprintf("<td align=center><input type=submit name=\"Start\" value=\"%s\">\n",
                     str);
         }

         if (count > 0 && strncmp(key.name, "mhttpd", 6) != 0) {
            sprintf(str, "Stop %s", key.name);
            rsprintf("<td align=center><input type=submit name=\"Stop\" value=\"%s\">\n",
                     str);
         }

         rsprintf("</tr>\n");
      }
   }


   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void show_config_page(int refresh)
{
   char str[80];
   HNDLE hDB;

   cm_get_experiment_database(&hDB, NULL);

   show_header(hDB, "Configure", "GET", "", 1, 0);

   rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Configure</tr>\n");

   rsprintf("<tr><td bgcolor=#FFFF00>Update period\n");

   sprintf(str, "5");
   rsprintf("<td><input type=text size=5 maxlength=5 name=refr value=%d>\n", refresh);
   rsprintf("</tr>\n");

   rsprintf("<tr><td align=center colspan=2>\n");
   rsprintf("<input type=submit name=cmd value=Accept>\n");
   rsprintf("<input type=submit name=cmd value=Cancel>\n");
   rsprintf("<input type=hidden name=cmd value=Accept>\n");
   rsprintf("</tr>\n");
   rsprintf("</table>\n");

   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

#define LN10 2.302585094
#define LOG2 0.301029996
#define LOG5 0.698970005

void haxis(gdImagePtr im, gdFont * font, int col, int gcol,
           int x1, int y1, int width,
           int minor, int major, int text, int label, int grid, double xmin, double xmax)
{
   double dx, int_dx, frac_dx, x_act, label_dx, major_dx, x_screen, maxwidth;
   int tick_base, major_base, label_base, n_sig1, n_sig2, xs;
   char str[80];
   double base[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };

   if (xmax <= xmin || width <= 0)
      return;

   /* use 5 as min tick distance */
   dx = (xmax - xmin) / (double) (width / 5);

   frac_dx = modf(log(dx) / LN10, &int_dx);
   if (frac_dx < 0) {
      frac_dx += 1;
      int_dx -= 1;
   }

   tick_base = frac_dx < LOG2 ? 1 : frac_dx < LOG5 ? 2 : 3;
   major_base = label_base = tick_base + 1;

   /* rounding up of dx, label_dx */
   dx = pow(10, int_dx) * base[tick_base];
   major_dx = pow(10, int_dx) * base[major_base];
   label_dx = major_dx;

   /* number of significant digits */
   if (xmin == 0)
      n_sig1 = 0;
   else
      n_sig1 = (int) floor(log(fabs(xmin)) / LN10) - (int) floor(log(fabs(label_dx)) / LN10) + 1;

   if (xmax == 0)
      n_sig2 = 0;
   else
      n_sig2 =
         (int) floor(log(fabs(xmax)) / LN10) - (int) floor(log(fabs(label_dx)) / LN10) + 1;

   n_sig1 = MAX(n_sig1, n_sig2);
   n_sig1 = MAX(n_sig1, 4);

   /* determination of maximal width of labels */
   sprintf(str, "%1.*lG", n_sig1, floor(xmin / dx) * dx);
   maxwidth = font->h / 2 * strlen(str);
   sprintf(str, "%1.*lG", n_sig1, floor(xmax / dx) * dx);
   maxwidth = MAX(maxwidth, font->h / 2 * strlen(str));
   sprintf(str, "%1.*lG", n_sig1, floor(xmax / dx) * dx + label_dx);
   maxwidth = MAX(maxwidth, font->h / 2 * strlen(str));

   /* increasing label_dx, if labels would overlap */
   while (maxwidth > 0.7 * label_dx / (xmax - xmin) * width) {
      label_base++;
      label_dx = pow(10, int_dx) * base[label_base];
      if (label_base % 3 == 2 && major_base % 3 == 1) {
         major_base++;
         major_dx = pow(10, int_dx) * base[major_base];
      }
   }

   x_act = floor(xmin / dx) * dx;

   gdImageLine(im, x1, y1, x1 + width, y1, col);

   do {
      x_screen = (x_act - xmin) / (xmax - xmin) * width + x1;
      xs = (int) (x_screen + 0.5);

      if (x_screen > x1 + width + 0.001)
         break;

      if (x_screen >= x1) {
         if (fabs(floor(x_act / major_dx + 0.5) - x_act / major_dx) <
             dx / major_dx / 10.0) {

            if (fabs(floor(x_act / label_dx + 0.5) - x_act / label_dx) <
                dx / label_dx / 10.0) {
               /* label tick mark */
               gdImageLine(im, xs, y1, xs, y1 + text, col);

               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  gdImageLine(im, xs, y1, xs, y1 + grid, col);

               /* label */
               if (label != 0) {
                  sprintf(str, "%1.*lG", n_sig1, x_act);
                  gdImageString(im, font, (int) xs - font->w * strlen(str) / 2,
                                y1 + label, str, col);
               }
            } else {
               /* major tick mark */
               gdImageLine(im, xs, y1, xs, y1 + major, col);

               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  gdImageLine(im, xs, y1 - 1, xs, y1 + grid, gcol);
            }

         } else
            /* minor tick mark */
            gdImageLine(im, xs, y1, xs, y1 + minor, col);

      }

      x_act += dx;

      /* supress 1.23E-17 ... */
      if (fabs(x_act) < dx / 100)
         x_act = 0;

   } while (1);
}

/*------------------------------------------------------------------*/

void sec_to_label(char *result, int sec, int base, int force_date)
{
   char mon[80];
   struct tm *tms;
   time_t t_sec;

   t_sec = (time_t) sec;
   tms = localtime(&t_sec);
   strcpy(mon, mname[tms->tm_mon]);
   mon[3] = 0;

   if (force_date) {
      if (base < 600)
         sprintf(result, "%02d %s %02d %02d:%02d:%02d",
                 tms->tm_mday, mon, tms->tm_year % 100, tms->tm_hour, tms->tm_min,
                 tms->tm_sec);
      else if (base < 3600 * 24)
         sprintf(result, "%02d %s %02d %02d:%02d",
                 tms->tm_mday, mon, tms->tm_year % 100, tms->tm_hour, tms->tm_min);
      else
         sprintf(result, "%02d %s %02d", tms->tm_mday, mon, tms->tm_year % 100);
   } else {
      if (base < 600)
         sprintf(result, "%02d:%02d:%02d", tms->tm_hour, tms->tm_min, tms->tm_sec);
      else if (base < 3600 * 3)
         sprintf(result, "%02d:%02d", tms->tm_hour, tms->tm_min);
      else if (base < 3600 * 24)
         sprintf(result, "%02d %s %02d %02d:%02d",
                 tms->tm_mday, mon, tms->tm_year % 100, tms->tm_hour, tms->tm_min);
      else
         sprintf(result, "%02d %s %02d", tms->tm_mday, mon, tms->tm_year % 100);
   }
}

void taxis(gdImagePtr im, gdFont * font, int col, int gcol,
           int x1, int y1, int width, int xr,
           int minor, int major, int text, int label, int grid, double xmin, double xmax)
{
   int dx, x_act, label_dx, major_dx, x_screen, maxwidth;
   int tick_base, major_base, label_base, xs, xl;
   char str[80];
   int base[] = { 1, 5, 10, 60, 300, 600, 1800, 3600, 3600 * 6, 3600 * 12, 3600 * 24, 0 };
   time_t ltime;
   int force_date, d1, d2;
   struct tm *ptms;

   if (xmax <= xmin || width <= 0)
      return;

   /* force date display if xmax not today */
   ltime = ss_time();
   ptms = localtime(&ltime);
   d1 = ptms->tm_mday;
   ltime = (time_t) xmax;
   ptms = localtime(&ltime);
   d2 = ptms->tm_mday;
   force_date = (d1 != d2);

   /* use 5 pixel as min tick distance */
   dx = (int) ((xmax - xmin) / (double) (width / 5) + 0.5);

   for (tick_base = 0; base[tick_base]; tick_base++) {
      if (base[tick_base] > dx)
         break;
   }
   if (!base[tick_base])
      tick_base--;
   dx = base[tick_base];

   if (base[tick_base + 1])
      major_base = tick_base + 1;
   else
      major_base = tick_base;
   major_dx = base[major_base];

   if (base[major_base + 1])
      label_base = major_base + 1;
   else
      label_base = major_base;
   label_dx = base[label_base];

   do {
      sec_to_label(str, (int) (xmin + 0.5), label_dx, force_date);
      maxwidth = font->h / 2 * strlen(str);

      /* increasing label_dx, if labels would overlap */
      if (maxwidth > 0.7 * label_dx / (xmax - xmin) * width) {
         if (base[label_base + 1])
            label_dx = base[++label_base];
         else
            label_dx += 3600 * 24;
      } else
         break;
   } while (1);

   x_act =
       (int) floor((double) (xmin - ss_timezone()) / label_dx) * label_dx + ss_timezone();

   gdImageLine(im, x1, y1, x1 + width, y1, col);

   do {
      x_screen = (int) ((x_act - xmin) / (xmax - xmin) * width + x1 + 0.5);
      xs = (int) (x_screen + 0.5);

      if (x_screen > x1 + width + 0.001)
         break;

      if (x_screen >= x1) {
         if ((x_act - ss_timezone()) % major_dx == 0) {
            if ((x_act - ss_timezone()) % label_dx == 0) {
               /* label tick mark */
               gdImageLine(im, xs, y1, xs, y1 + text, col);

               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  gdImageLine(im, xs, y1, xs, y1 + grid, col);

               /* label */
               if (label != 0) {
                  sec_to_label(str, x_act, label_dx, force_date);

                  /* if labels at edge, shift them in */
                  xl = (int) xs - font->w * strlen(str) / 2;
                  if (xl < 0)
                     xl = 0;
                  if (xl + font->w * (int) strlen(str) > xr)
                     xl = xr - font->w * strlen(str);
                  gdImageString(im, font, xl, y1 + label, str, col);
               }
            } else {
               /* major tick mark */
               gdImageLine(im, xs, y1, xs, y1 + major, col);

               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  gdImageLine(im, xs, y1 - 1, xs, y1 + grid, gcol);
            }

         } else
            /* minor tick mark */
            gdImageLine(im, xs, y1, xs, y1 + minor, col);

      }

      x_act += dx;

      /* supress 1.23E-17 ... */
      if (fabs((double)x_act) < dx / 100)
         x_act = 0;

   } while (1);
}

/*------------------------------------------------------------------*/

int vaxis(gdImagePtr im, gdFont * font, int col, int gcol,
          int x1, int y1, int width,
          int minor, int major, int text, int label, int grid, double ymin, double ymax,
          BOOL logaxis)
{
   double dy, int_dy, frac_dy, y_act, label_dy, major_dy, y_screen, y_next;
   int tick_base, major_base, label_base, n_sig1, n_sig2, ys, max_width;
   int last_label_y;
   char str[80];
   double base[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };

   if (ymax <= ymin || width <= 0)
      return 0;

   // data type "double" only has about 15 significant digits, if difference between ymax and ymin is less than 10 significant digits, bailout! Otherwise code below goes into infinite loop
   if (fabs(ymax - ymin) <= 1e-10)
      return 0;

   if (logaxis) {
      dy = pow(10, floor(log(ymin) / LN10));
      label_dy = dy;
      major_dy = dy * 10;
      n_sig1 = 4;
   } else {
      dy = (ymax - ymin) / (double) (width / 5);

      frac_dy = modf(log(dy) / LN10, &int_dy);
      if (frac_dy < 0) {
         frac_dy += 1;
         int_dy -= 1;
      }

      tick_base = frac_dy < LOG2 ? 1 : frac_dy < LOG5 ? 2 : 3;
      major_base = label_base = tick_base + 1;

      /* rounding up of dy, label_dy */
      dy = pow(10, int_dy) * base[tick_base];
      major_dy = pow(10, int_dy) * base[major_base];
      label_dy = major_dy;

      /* number of significant digits */
      if (ymin == 0)
         n_sig1 = 0;
      else
         n_sig1 =
             (int) floor(log(fabs(ymin)) / LN10) -
             (int) floor(log(fabs(label_dy)) / LN10) + 1;

      if (ymax == 0)
         n_sig2 = 0;
      else
         n_sig2 =
             (int) floor(log(fabs(ymax)) / LN10) -
             (int) floor(log(fabs(label_dy)) / LN10) + 1;

      n_sig1 = MAX(n_sig1, n_sig2);
      n_sig1 = MAX(n_sig1, 4);

      /* increasing label_dy, if labels would overlap */
      while (label_dy / (ymax - ymin) * width < 1.5 * font->h) {
         label_base++;
         label_dy = pow(10, int_dy) * base[label_base];
         if (label_base % 3 == 2 && major_base % 3 == 1) {
            major_base++;
            major_dy = pow(10, int_dy) * base[major_base];
         }
      }
   }

   max_width = 0;
   y_act = floor(ymin / dy) * dy;

   if (x1 != 0 || y1 != 0)
      gdImageLine(im, x1, y1, x1, y1 - width, col);

   last_label_y = y1 + 2 * font->h;

   do {
      if (logaxis)
         y_screen = y1 - (log(y_act) - log(ymin)) / (log(ymax) - log(ymin)) * width;
      else
         y_screen = y1 - (y_act - ymin) / (ymax - ymin) * width;
      ys = (int) (y_screen + 0.5);

      if (y_screen < y1 - width - 0.001)
         break;

      if (y_screen <= y1 + 0.001) {
         if (fabs(floor(y_act / major_dy + 0.5) - y_act / major_dy) <
             dy / major_dy / 10.0) {
            if (fabs(floor(y_act / label_dy + 0.5) - y_act / label_dy) <
                dy / label_dy / 10.0) {
               if (x1 != 0 || y1 != 0) {
                  /* label tick mark */
                  gdImageLine(im, x1, ys, x1 + text, ys, col);

                  /* grid line */
                  if (grid != 0 && y_screen < y1 && y_screen > y1 - width) {
                     if (grid > 0)
                        gdImageLine(im, x1 + 1, ys, x1 + grid, ys, gcol);
                     else
                        gdImageLine(im, x1 - 1, ys, x1 + grid, ys, gcol);
                  }

                  /* label */
                  if (label != 0) {
                     sprintf(str, "%1.*lG", n_sig1, y_act);
                     if (label < 0)
                        gdImageString(im, font, x1 + label - font->w * strlen(str),
                                      ys - font->h / 2, str, col);
                     else
                        gdImageString(im, font, x1 + label, ys - font->h / 2, str, col);

                     last_label_y = ys - font->h / 2;
                  }
               } else {
                  sprintf(str, "%1.*lG", n_sig1, y_act);
                  max_width = MAX(max_width, (int) (font->w * strlen(str)));
               }
            } else {
               if (x1 != 0 || y1 != 0) {
                  /* major tick mark */
                  gdImageLine(im, x1, ys, x1 + major, ys, col);

                  /* grid line */
                  if (grid != 0 && y_screen < y1 && y_screen > y1 - width)
                     gdImageLine(im, x1, ys, x1 + grid, ys, col);
               }
            }
            if (logaxis) {
               dy *= 10;
               major_dy *= 10;
               label_dy *= 10;
            }

         } else {
            if (x1 != 0 || y1 != 0) {
               /* minor tick mark */
               gdImageLine(im, x1, ys, x1 + minor, ys, col);
            }

            /* for logaxis, also put labes on minor tick marks */
            if (logaxis) {
               if (label != 0) {
                  if (x1 != 0 || y1 != 0) {
                     /* calculate position of next major label */
                     y_next = pow(10, floor(log(y_act) / LN10) + 1);
                     y_screen =
                         (int) (y1 -
                                (log(y_next) - log(ymin)) / (log(ymax) -
                                                             log(ymin)) * width + 0.5);

                     if (ys + font->h / 2 < last_label_y
                         && ys - font->h / 2 > y_screen + font->h / 2) {
                        sprintf(str, "%1.*lG", n_sig1, y_act);
                        if (label < 0)
                           gdImageString(im, font, x1 + label - font->w * strlen(str),
                                         ys - font->h / 2, str, col);
                        else
                           gdImageString(im, font, x1 + label, ys - font->h / 2, str,
                                         col);
                     }

                     last_label_y = ys - font->h / 2;
                  } else {
                     sprintf(str, "%1.*lG", n_sig1, y_act);
                     max_width = MAX(max_width, (int) (font->w * strlen(str)));
                  }
               }
            }
         }
      }

      y_act += dy;

      /* supress 1.23E-17 ... */
      if (fabs(y_act) < dy / 100)
         y_act = 0;

   } while (1);

   return max_width + abs(label);
}

/*------------------------------------------------------------------*/

int time_to_sec(const char *str)
{
   double s;

   s = atof(str);
   switch (str[strlen(str) - 1]) {
   case 'm':
   case 'M':
      s *= 60;
      break;
   case 'h':
   case 'H':
      s *= 3600;
      break;
   case 'd':
   case 'D':
      s *= 3600 * 24;
      break;
   }

   return (int) s;
}

/*------------------------------------------------------------------*/

static MidasHistoryInterface* mh = NULL;
static bool using_odbc = 0;

static void set_history_path()
{
   int status;
   HNDLE hDB;

   cm_get_experiment_database(&hDB, NULL);

#ifdef HAVE_ODBC
   /* check ODBC connection */
   char str[256];
   int size = sizeof(str);
   str[0] = 0;
   status = db_get_value(hDB, 0, "/History/ODBC_DSN", str, &size, TID_STRING, TRUE);
   if ((status==DB_SUCCESS || status==DB_TRUNCATED) && str[0]!=0 && str[0]!='#') {

      if (!using_odbc)
         if (mh) {
            delete mh;
            mh = NULL;
         }

      if (!mh)
         mh = MakeMidasHistoryODBC();

      using_odbc = true;

      int debug = 0;
      size = sizeof(debug);
      status = db_get_value(hDB, 0, "/History/ODBC_Debug", &debug, &size, TID_INT, TRUE);
      assert(status==DB_SUCCESS);

      mh->hs_set_debug(debug);

      status = mh->hs_connect(str);

      if (status != HS_SUCCESS) {
         cm_msg(MERROR, "set_history_path", "Cannot connect to history database, hs_connect() status %d", status);
      }

   } else {
      if (using_odbc)
         if (mh) {
            delete mh;
            mh = NULL;

         }
      using_odbc = false;
   }
#endif

   if (!using_odbc) {
      if (!mh)
         mh = MakeMidasHistory();

      status = mh->hs_connect(NULL);

      if (status != HS_SUCCESS) {
         cm_msg(MERROR, "set_history_path", "Cannot connect to midas history, hs_connect() status %d", status);
      }
   }
}

/*------------------------------------------------------------------*/

#ifdef OS_WINNT
#undef DELETE
#endif

#define ALLOC(t,n) (t*)calloc(sizeof(t),(n))
#define DELETE(x) if (x) { free(x); (x)=NULL; }
#define DELETEA(x, n) if (x) { for (int i=0; i<(n); i++) { free((x)[i]); (x)[i]=NULL; }; DELETE(x); }
#define STRDUP(x) strdup(x)

struct HistoryData
{
   int nvars;
   int alloc_nvars;
   char** event_names;
   char** var_names;
   int* var_index;
   int* odb_index;
   int* status;
   int* num_entries;
   time_t** t;
   double** v;

   time_t tstart;
   time_t tend;
   time_t scale;

   void Allocate(int xnvars) {
      nvars = 0;
      alloc_nvars = xnvars;
      event_names = ALLOC(char*, alloc_nvars);
      var_names = ALLOC(char*, alloc_nvars);
      var_index = ALLOC(int, alloc_nvars);
      odb_index = ALLOC(int, alloc_nvars);
      status = ALLOC(int, alloc_nvars);
      num_entries = ALLOC(int, alloc_nvars);
      t = ALLOC(time_t*, alloc_nvars);
      v = ALLOC(double*, alloc_nvars);
   }

   void Free() {
      nvars = 0;
      alloc_nvars = 0;
      DELETEA(event_names, alloc_nvars);
      DELETEA(var_names, alloc_nvars);
      DELETE(var_index);
      DELETE(odb_index);
      DELETE(status);
      DELETE(num_entries);
      DELETEA(t, alloc_nvars);
      DELETEA(v, alloc_nvars);
   }

   void Print() const {
      printf("this %p, nvars %d. tstart %d, tend %d, scale %d\n", this, nvars, (int)tstart, (int)tend, (int)scale);
      for (int i=0; i<nvars; i++) {
         printf("var[%d]: [%s/%s][%d] %d entries, status %d", i, event_names[i], var_names[i], var_index[i], num_entries[i], status[i]);
         if (status[i]==HS_SUCCESS && num_entries[i]>0 && t[i] && v[i])
            printf(", t %d:%d, v %g:%g\n", (int)t[i][0], (int)t[i][num_entries[i]-1], v[i][0], v[i][num_entries[i]-1]);
         else
            printf("\n");
      }
   }

   HistoryData() // ctor
   {
      nvars = 0;
   }

   ~HistoryData() // dtor
   {
      if (nvars > 0)
         Free();
   }
};

int read_history(HNDLE hDB, const char *path, int index, int runmarker, time_t tstart, time_t tend, time_t scale, HistoryData *data)
{
   HNDLE hkeypanel, hkeydvar, hkey;
   KEY key;
   int n_vars, status;

   // printf("read_history, path %s, index %d, runmarker %d, start %d, end %d, scale %d, data %p\n", path, index, runmarker, (int)tstart, (int)tend, (int)scale, data);

   /* connect to history */
   set_history_path();

   /* check panel name in ODB */
   status = db_find_key(hDB, 0, "/History/Display", &hkey);
   if (!hkey) {
      cm_msg(MERROR, "read_history", "Cannot find \'/History/Display\' in ODB, status %d", status);
      return HS_FILE_ERROR;
   }

   /* check panel name in ODB */
   status = db_find_key(hDB, hkey, path, &hkeypanel);
   if (!hkeypanel) {
      cm_msg(MERROR, "read_history", "Cannot find \'%s\' in ODB, status %d", path, status);
      return HS_FILE_ERROR;
   }

   status = db_find_key(hDB, hkeypanel, "Variables", &hkeydvar);
   if (!hkeydvar) {
      cm_msg(MERROR, "read_history", "Cannot find \'%s/Variables\' in ODB, status %d", path, status);
      return HS_FILE_ERROR;
   }

   db_get_key(hDB, hkeydvar, &key);
   n_vars = key.num_values;

   data->Allocate(n_vars+2);

   data->tstart = tstart;
   data->tend = tend;
   data->scale = scale;

   for (int i=0; i<n_vars; i++) {
      if (index != -1 && index != i)
         continue;

      char str[256];
      int size = sizeof(str);
      status = db_get_data_index(hDB, hkeydvar, str, &size, i, TID_STRING);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "read_history", "Cannot read tag %d in panel %s, status %d", i, path, status);
         continue;
      }

      /* split varname in event, variable and index: "event/tag[index]" */

      char *p = strchr(str, ':');
      if (!p)
         p = strchr(str, '/');

      if (!p) {
         cm_msg(MERROR, "read_history", "Tag \"%s\" has wrong format in panel \"%s\"", str, path);
         continue;
      }

      *p = 0;
      
      data->odb_index[data->nvars] = i;
      data->event_names[data->nvars] = STRDUP(str);
      data->var_index[data->nvars] = 0;

      char *q = strchr(p+1, '[');
      if (q) {
         data->var_index[data->nvars] = atoi(q+1);
         *q = 0;
      }

      data->var_names[data->nvars] = STRDUP(p+1);

      data->nvars++;
   } // loop over variables

   /* write run markes if selected */
   if (runmarker) {

      data->event_names[data->nvars+0] = STRDUP("Run transitions");
      data->event_names[data->nvars+1] = STRDUP("Run transitions");

      data->var_names[data->nvars+0] = STRDUP("State");
      data->var_names[data->nvars+1] = STRDUP("Run number");

      data->var_index[data->nvars+0] = 0;
      data->var_index[data->nvars+1] = 0;

      data->odb_index[data->nvars+0] = -1;
      data->odb_index[data->nvars+1] = -2;

      data->nvars += 2;
   }

   status = mh->hs_read(tstart, tend, scale,
                        data->nvars,
                        data->event_names,
                        data->var_names,
                        data->var_index,
                        data->num_entries,
                        data->t,
                        data->v,
                        data->status);
   
   if (status != HS_SUCCESS) {
      cm_msg(MERROR, "read_history", "Complete history failure, hs_read() status %d, see messages", status);
      return HS_FILE_ERROR;
   }
   
   return SUCCESS;
}

void generate_hist_graph(const char *path, char *buffer, int *buffer_size,
                         int width, int height, int scale, int toffset, int index,
                         int labels, char *bgcolor, char *fgcolor, char *gridcolor)
{
   HNDLE hDB, hkey, hkeypanel, hkeyeq, hkeydvar, hkeyvars, hkeyroot, hkeynames;
   KEY key;
   gdImagePtr im;
   gdGifBuffer gb;
   int i, j, k, l, n_vars, size, status, row, x_marker, n_vp, r, g, b;
   int length, aoffset;
   int flag, x1, y1, x2, y2, xs, xs_old, ys, xold, yold, xmaxm;
   int white, black, grey, ltgrey, red, green, blue, fgcol, bgcol, gridcol,
       curve_col[MAX_VARS], state_col[3];
   char str[256], panel[256], *p, odbpath[256];
   INT var_index[MAX_VARS];
   char event_name[MAX_VARS][NAME_LENGTH];
   char tag_name[MAX_VARS][64], var_name[MAX_VARS][NAME_LENGTH], varname[64],
       key_name[256];
#define MAX_POINTS 1000
   DWORD n_point[MAX_VARS];
   int x[MAX_VARS][MAX_POINTS];
   double y[MAX_VARS][MAX_POINTS];
   float factor[MAX_VARS], offset[MAX_VARS];
   BOOL logaxis, runmarker;
   double xmin, xmax, ymin, ymax;
   gdPoint poly[3];
   double upper_limit[MAX_VARS], lower_limit[MAX_VARS];
   double yb1, yb2, yf1, yf2, ybase;
   float minvalue = (float) -HUGE_VAL;
   float maxvalue = (float) +HUGE_VAL;
   int show_values = 0;
   int sort_vars = 0;
   char var_status[MAX_VARS][256];
   double tstart, tend;

   static char *ybuffer;
   static DWORD *tbuffer;
   static int hbuffer_size = 0;

   if (hbuffer_size == 0) {
      hbuffer_size = 1000 * sizeof(DWORD);
      tbuffer = (DWORD*)malloc(hbuffer_size);
      ybuffer = (char*)malloc(hbuffer_size);
   }

   cm_get_experiment_database(&hDB, NULL);

   /* generate image */
   im = gdImageCreate(width, height);

   /* allocate standard colors */
   sscanf(bgcolor, "%02x%02x%02x", &r, &g, &b);
   bgcol = gdImageColorAllocate(im, r, g, b);
   sscanf(fgcolor, "%02x%02x%02x", &r, &g, &b);
   fgcol = gdImageColorAllocate(im, r, g, b);
   sscanf(gridcolor, "%02x%02x%02x", &r, &g, &b);
   gridcol = gdImageColorAllocate(im, r, g, b);

   grey = gdImageColorAllocate(im, 192, 192, 192);
   ltgrey = gdImageColorAllocate(im, 208, 208, 208);
   white = gdImageColorAllocate(im, 255, 255, 255);
   black = gdImageColorAllocate(im, 0, 0, 0);
   red = gdImageColorAllocate(im, 255, 0, 0);
   green = gdImageColorAllocate(im, 0, 255, 0);
   blue = gdImageColorAllocate(im, 0, 0, 255);

   curve_col[0] = gdImageColorAllocate(im, 0, 0, 255);
   curve_col[1] = gdImageColorAllocate(im, 0, 192, 0);
   curve_col[2] = gdImageColorAllocate(im, 255, 0, 0);
   curve_col[3] = gdImageColorAllocate(im, 0, 192, 192);
   curve_col[4] = gdImageColorAllocate(im, 255, 0, 255);
   curve_col[5] = gdImageColorAllocate(im, 192, 192, 0);
   curve_col[6] = gdImageColorAllocate(im, 128, 128, 128);
   curve_col[7] = gdImageColorAllocate(im, 128, 255, 128);
   curve_col[8] = gdImageColorAllocate(im, 255, 128, 128);
   curve_col[9] = gdImageColorAllocate(im, 128, 128, 255);
   for (i=10; i<MAX_VARS; i++)
      curve_col[i] = gdImageColorAllocate(im, 128, 128, 128);

   state_col[0] = gdImageColorAllocate(im, 255, 0, 0);
   state_col[1] = gdImageColorAllocate(im, 255, 255, 0);
   state_col[2] = gdImageColorAllocate(im, 0, 255, 0);

   /* Set transparent color. */
   gdImageColorTransparent(im, grey);

   /* Title */
   strlcpy(panel, path, sizeof(panel));
   if (strstr(panel, ".gif"))
      *strstr(panel, ".gif") = 0;
   gdImageString(im, gdFontGiant, width / 2 - (strlen(panel) * gdFontGiant->w) / 2, 2,
                 panel, fgcol);

   /* connect to history */
   set_history_path();

   /* check panel name in ODB */
   sprintf(str, "/History/Display/%s", panel);
   db_find_key(hDB, 0, str, &hkeypanel);
   if (!hkeypanel) {
      sprintf(str, "Cannot find /History/Display/%s in ODB", panel);
      gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                    height / 2, str, red);
      goto error;
   }

   db_find_key(hDB, hkeypanel, "Variables", &hkeydvar);
   if (!hkeydvar) {
      sprintf(str, "Cannot find /History/Display/%s/Variables in ODB", panel);
      gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                    height / 2, str, red);
      goto error;
   }

   db_get_key(hDB, hkeydvar, &key);
   n_vars = key.num_values;

   if (n_vars > MAX_VARS) {
      sprintf(str, "Too many variables in panel %s", panel);
      gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                    height / 2, str, red);
      goto error;
   }

   ymin = ymax = 0;
   logaxis = runmarker = 0;

   tstart = ss_millitime();

   for (i = 0; i < n_vars; i++) {
      if (index != -1 && index != i)
         continue;

      size = sizeof(str);
      status = db_get_data_index(hDB, hkeydvar, str, &size, i, TID_STRING);
      if (status != DB_SUCCESS) {
         sprintf(str, "Cannot read tag %d in panel %s, status %d", i, panel, status);
         gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                       height / 2, str, red);
         goto error;
      }

      strlcpy(tag_name[i], str, sizeof(tag_name[0]));

      /* split varname in event, variable and index */
      if (strchr(tag_name[i], ':')) {
         strlcpy(event_name[i], tag_name[i], sizeof(event_name[0]));
         *strchr(event_name[i], ':') = 0;
         strlcpy(var_name[i], strchr(tag_name[i], ':') + 1, sizeof(var_name[0]));
         var_index[i] = 0;
         if (strchr(var_name[i], '[')) {
            var_index[i] = atoi(strchr(var_name[i], '[') + 1);
            *strchr(var_name[i], '[') = 0;
         }
      } else {
         sprintf(str, "Tag \"%s\" has wrong format in panel \"%s\"", tag_name[i], panel);
         gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                       height / 2, str, red);
         goto error;
      }

      db_find_key(hDB, hkeypanel, "Colour", &hkey);
      if (hkey) {
         size = sizeof(str);
         status = db_get_data_index(hDB, hkey, str, &size, i, TID_STRING);
         if (status == DB_SUCCESS) {
            if (str[0] == '#') {
               char sss[3];
               int r, g, b;

               sss[0] = str[1];
               sss[1] = str[2];
               sss[2] = 0;
               r = strtoul(sss, NULL, 16);
               sss[0] = str[3];
               sss[1] = str[4];
               sss[2] = 0;
               g = strtoul(sss, NULL, 16);
               sss[0] = str[5];
               sss[1] = str[6];
               sss[2] = 0;
               b = strtoul(sss, NULL, 16);

               curve_col[i] = gdImageColorAllocate(im, r, g, b);
	    }
	 }
      }

      /* get timescale */
      if (scale == 0) {
         strlcpy(str, "1h", sizeof(str));
         size = NAME_LENGTH;
         status = db_get_value(hDB, hkeypanel, "Timescale", str, &size, TID_STRING, TRUE);
         if (status != DB_SUCCESS) {
            /* delete old integer key */
            db_find_key(hDB, hkeypanel, "Timescale", &hkey);
            if (hkey)
               db_delete_key(hDB, hkey, FALSE);

            strcpy(str, "1h");
            size = NAME_LENGTH;
            status =
                db_get_value(hDB, hkeypanel, "Timescale", str, &size, TID_STRING, TRUE);
         }

         scale = time_to_sec(str);
      }

      for (j = 0; j < MAX_VARS; j++) {
         factor[j] = 1;
         offset[j] = 0;
      }

      /* get factors */
      size = sizeof(float) * n_vars;
      db_get_value(hDB, hkeypanel, "Factor", factor, &size, TID_FLOAT, TRUE);

      /* get offsets */
      size = sizeof(float) * n_vars;
      db_get_value(hDB, hkeypanel, "Offset", offset, &size, TID_FLOAT, TRUE);

      /* get axis type */
      size = sizeof(logaxis);
      logaxis = 0;
      db_get_value(hDB, hkeypanel, "Log axis", &logaxis, &size, TID_BOOL, TRUE);

      /* get show_values type */
      size = sizeof(show_values);
      show_values = 0;
      db_get_value(hDB, hkeypanel, "Show values", &show_values, &size, TID_BOOL, TRUE);

      /* get sort_vars type */
      size = sizeof(sort_vars);
      sort_vars = 0;
      db_get_value(hDB, hkeypanel, "Show values", &sort_vars, &size, TID_BOOL, TRUE);

      /* get min value */
      size = sizeof(minvalue);
      minvalue = (float) -HUGE_VAL;
      db_get_value(hDB, hkeypanel, "Minimum", &minvalue, &size, TID_FLOAT, TRUE);

      /* get max value */
      size = sizeof(maxvalue);
      maxvalue = (float) +HUGE_VAL;
      db_get_value(hDB, hkeypanel, "Maximum", &maxvalue, &size, TID_FLOAT, TRUE);

      if ((minvalue == 0) && (maxvalue == 0)) {
         minvalue = (float) -HUGE_VAL;
         maxvalue = (float) +HUGE_VAL;
      }

      /* get runmarker flag */
      size = sizeof(runmarker);
      runmarker = 1;
      db_get_value(hDB, hkeypanel, "Show run markers", &runmarker, &size, TID_BOOL, TRUE);

      /* make ODB path from tag name */
      odbpath[0] = 0;
      db_find_key(hDB, 0, "/Equipment", &hkeyroot);
      if (hkeyroot) {
         for (j = 0;; j++) {
            db_enum_key(hDB, hkeyroot, j, &hkeyeq);

            if (!hkeyeq)
               break;

            db_get_key(hDB, hkeyeq, &key);
            if (equal_ustring(key.name, event_name[i])) {
               /* check if variable is individual key under variables/ */
               sprintf(str, "Variables/%s", var_name[i]);
               db_find_key(hDB, hkeyeq, str, &hkey);
               if (hkey) {
                  sprintf(odbpath, "/Equipment/%s/Variables/%s", event_name[i],
                          var_name[i]);
                  break;
               }

               /* check if variable is in setttins/names array */
               db_find_key(hDB, hkeyeq, "Settings/Names", &hkeynames);
               if (hkeynames) {
                  /* extract variable name and Variables/<key> */
                  strlcpy(str, var_name[i], sizeof(str));
                  p = str + strlen(str) - 1;
                  while (p > str && *p != ' ')
                     p--;
                  strlcpy(key_name, p + 1, sizeof(key_name));
                  *p = 0;
                  strlcpy(varname, str, sizeof(varname));

                  /* find key in single name array */
                  db_get_key(hDB, hkeynames, &key);
                  for (k = 0; k < key.num_values; k++) {
                     size = sizeof(str);
                     db_get_data_index(hDB, hkeynames, str, &size, k, TID_STRING);
                     if (equal_ustring(str, varname)) {
                        sprintf(odbpath, "/Equipment/%s/Variables/%s[%d]", event_name[i],
                                key_name, k);
                        break;
                     }
                  }
               } else {
                  /* go through /variables/<name> entries */
                  db_find_key(hDB, hkeyeq, "Variables", &hkeyvars);
                  if (hkeyvars) {
                     for (k = 0;; k++) {
                        db_enum_key(hDB, hkeyvars, k, &hkey);

                        if (!hkey)
                           break;

                        /* find "settins/names <key>" for this key */
                        db_get_key(hDB, hkey, &key);

                        /* find key in key_name array */
                        strlcpy(key_name, key.name, sizeof(key_name));
                        sprintf(str, "Settings/Names %s", key_name);

                        db_find_key(hDB, hkeyeq, str, &hkeynames);
                        if (hkeynames) {
                           db_get_key(hDB, hkeynames, &key);
                           for (l = 0; l < key.num_values; l++) {
                              size = sizeof(str);
                              db_get_data_index(hDB, hkeynames, str, &size, l,
                                                TID_STRING);
                              if (equal_ustring(str, var_name[i])) {
                                 sprintf(odbpath, "/Equipment/%s/Variables/%s[%d]",
                                         event_name[i], key_name, l);
                                 break;
                              }
                           }
                        }
                     }
                  }
               }

               break;
            }
         }

         if (!hkeyeq) {
            db_find_key(hDB, 0, "/History/Links", &hkeyroot);
            if (hkeyroot) {
               for (j = 0;; j++) {
                  db_enum_link(hDB, hkeyroot, j, &hkey);

                  if (!hkey)
                     break;

                  db_get_key(hDB, hkey, &key);
                  if (equal_ustring(key.name, event_name[i])) {
                     db_enum_key(hDB, hkeyroot, j, &hkey);
                     db_find_key(hDB, hkey, var_name[i], &hkey);
                     if (hkey) {
                        db_get_key(hDB, hkey, &key);
                        db_get_path(hDB, hkey, odbpath, sizeof(odbpath));
                        if (key.num_values > 1)
                           sprintf(odbpath + strlen(odbpath), "[%d]", var_index[i]);
                        break;
                     }
                  }
               }
            }
         }
      }

      /* search alarm limits */
      upper_limit[i] = lower_limit[i] = -12345;
      db_find_key(hDB, 0, "Alarms/Alarms", &hkeyroot);
      if (odbpath[0] && hkeyroot) {
         for (j = 0;; j++) {
            db_enum_key(hDB, hkeyroot, j, &hkey);

            if (!hkey)
               break;

            size = sizeof(str);
            db_get_value(hDB, hkey, "Condition", str, &size, TID_STRING, TRUE);

            if (strstr(str, odbpath)) {
               if (strchr(str, '<')) {
                  p = strchr(str, '<') + 1;
                  if (*p == '=')
                     p++;
                  lower_limit[i] = (factor[i] * atof(p) + offset[i]);
               }
               if (strchr(str, '>')) {
                  p = strchr(str, '>') + 1;
                  if (*p == '=')
                     p++;
                  upper_limit[i] = (factor[i] * atof(p) + offset[i]);
               }
            }
         }
      }
   } // loop over variables

   if (1) {
      HistoryData  hsxxx;
      HistoryData* hsdata = &hsxxx;
      time_t now = ss_time();
   
      status = read_history(hDB, panel, index, runmarker, now-scale+toffset, now+toffset, scale/1000+1, hsdata);
      
      if (status != HS_SUCCESS) {
         sprintf(str, "Complete history failure, read_history() status %d, see messages", status);
         gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                       height / 2, str, red);
         goto error;
      }
      
      for (int k=0; k<hsdata->nvars; k++) {
         int i = hsdata->odb_index[k];

         if (i<0)
            continue;

         if (index != -1 && index != i)
            continue;

         n_point[i] = 0;

         var_status[i][0] = 0;
         if (hsdata->status[k] == HS_UNDEFINED_VAR) {
            sprintf(var_status[i], "not found in history");
            continue;
         } else if (hsdata->status[k] != HS_SUCCESS) {
            sprintf(var_status[i], "hs_read() error %d, see messages", hsdata->status[k]);
            continue;
         }

         for (int j = n_vp = 0; j < hsdata->num_entries[k]; j++) {
            x[i][n_vp] = (int)(hsdata->t[k][j] - now);
            y[i][n_vp] = hsdata->v[k][j];
         
            /* skip NaNs */
            if (ss_isnan(y[i][n_vp]))
               continue;
         
            /* skip INFs */
            if (!ss_isfin(y[i][n_vp]))
               continue;
         
            /* avoid overflow */
            if (y[i][n_vp] > 1E30)
               y[i][n_vp] = 1E30f;
         
            /* apply factor and offset */
            y[i][n_vp] = y[i][n_vp] * factor[i] + offset[i];
         
            /* calculate ymin and ymax */
            if ((i == 0 || index != -1) && n_vp == 0)
               ymin = ymax = y[i][0];
            else {
               if (y[i][n_vp] > ymax)
                  ymax = y[i][n_vp];
               if (y[i][n_vp] < ymin)
                  ymin = y[i][n_vp];
            }
         
            /* increment number of valid points */
            n_vp++;

         } // loop over data

         n_point[i] = n_vp;

         assert(n_point[i]<=MAX_POINTS);
      }
      
      // free arrays in history data
      for (i=0 ; i<hsdata->nvars ; i++) {
         free(hsdata->event_names[i]);
         hsdata->event_names[i] = NULL;

         free(hsdata->var_names[i]);
         hsdata->var_names[i] = NULL;

         free(hsdata->t[i]);
         hsdata->t[i] = NULL;

         free(hsdata->v[i]);
         hsdata->v[i] = NULL;
      }
   }

   if (0) {
      time_t now = ss_time();

      int num_entries[MAX_VARS];
      time_t *times[MAX_VARS];
      double *datas[MAX_VARS];
      int st[MAX_VARS];

      const char* xevent_name[MAX_VARS];
      const char* xvar_name[MAX_VARS];

      for (i = 0; i < n_vars; i++) {
         if (index != -1 && index != i) {
            var_status[i][0] = 0;
            xevent_name[i] = NULL;
            xvar_name[i] = NULL;
            continue;
         }

         var_status[i][0] = 0;
         xevent_name[i] = event_name[i];
         xvar_name[i] = var_name[i];
         
         //printf("event [%s][%s] tag [%s][%s] index [%d]\n", event_name[i], xevent_name[i], var_name[i], xvar_name[i], var_index[i]);
      }

      for (i=0 ; i<MAX_VARS ; i++) {
         times[i] = NULL;
         datas[i] = NULL;
      }

      status = mh->hs_read(now - scale + toffset, now + toffset, scale / 1000 + 1,
                           n_vars,
                           xevent_name, xvar_name, var_index,
                           num_entries,
                           times, datas,
                           st);
     
      if (status != HS_SUCCESS) {
         sprintf(str, "Complete history failure, hs_read() status %d, see messages", status);
         gdImageString(im, gdFontSmall, width / 2 - (strlen(str) * gdFontSmall->w) / 2,
                       height / 2, str, red);
         goto error;
      }

      for (i = 0; i < n_vars; i++) {
         var_status[i][0] = 0;

         if (st[i] == HS_UNDEFINED_VAR) {
            sprintf(var_status[i], "not found in history");
            num_entries[i] = 0;
         } else if (st[i] != HS_SUCCESS) {
            sprintf(var_status[i], "hs_read() error %d, see messages", st[i]);
            num_entries[i] = 0;
         }
      }
     
      for (i = 0; i < n_vars; i++) {
         if (index != -1 && index != i)
            continue;
       
         for (j = n_vp = 0; j < num_entries[i]; j++) {
            x[i][n_vp] = (int)(times[i][j] - now);
            y[i][n_vp] = datas[i][j];
         
            /* skip NaNs */
            if (ss_isnan(y[i][n_vp]))
               continue;
         
            /* skip INFs */
            if (!ss_isfin(y[i][n_vp]))
               continue;
         
            /* avoid overflow */
            if (y[i][n_vp] > 1E30)
               y[i][n_vp] = 1E30f;
         
            /* apply factor and offset */
            y[i][n_vp] = y[i][n_vp] * factor[i] + offset[i];
         
            /* calculate ymin and ymax */
            if ((i == 0 || index != -1) && n_vp == 0)
               ymin = ymax = y[i][0];
            else {
               if (y[i][n_vp] > ymax)
                  ymax = y[i][n_vp];
               if (y[i][n_vp] < ymin)
                  ymin = y[i][n_vp];
            }
         
            /* increment number of valid points */
            n_vp++;

         } // loop over data

         n_point[i] = n_vp;

         assert(n_point[i]<=MAX_POINTS);

         if (times[i]) {
            free(times[i]);
            times[i] = NULL;
         }
         if (datas[i]) {
            free(datas[i]);
            datas[i] = NULL;
         }
      } // loop over variables
   }

   tend = ss_millitime();

   if (ymin < minvalue)
      ymin = minvalue;

   if (ymax > maxvalue)
      ymax = maxvalue;

   /* check if ylow = 0 */
   if (index == -1) {
      flag = 0;
      size = sizeof(flag);
      db_get_value(hDB, hkeypanel, "Zero ylow", &flag, &size, TID_BOOL, TRUE);
      if (flag && ymin > 0)
         ymin = 0;
   }

   /* if min and max too close together, switch to linear axis */
   if (logaxis && ymin > 0 && ymax > 0) {
      yb1 = pow(10, floor(log(ymin) / LN10));
      yf1 = floor(ymin / yb1);
      yb2 = pow(10, floor(log(ymax) / LN10));
      yf2 = floor(ymax / yb2);

      if (yb1 == yb2 && yf1 == yf2)
         logaxis = 0;
      else {
         /* round down and up ymin and ymax */
         ybase = pow(10, floor(log(ymin) / LN10));
         ymin = (float) (floor(ymin / ybase) * ybase);
         ybase = pow(10, floor(log(ymax) / LN10));
         ymax = (float) ((floor(ymax / ybase) + 1) * ybase);
      }
   }

   /* avoid negative limits for log axis */
   if (logaxis) {
      if (ymax <= 0)
         ymax = 1;
      if (ymin <= 0)
         ymin = 1E-12f;
   }

   /* increase limits by 5% */
   if (ymin == 0 && ymax == 0) {
      ymin = -1;
      ymax = 1;
   } else {
      if (!logaxis) {
         ymax += (ymax - ymin) / 20.f;

         if (ymin != 0)
            ymin -= (ymax - ymin) / 20.f;
      }
   }

   /* avoid ymin == ymax */
   if (ymax == ymin) {
      if (logaxis) {
         ymax *= 2;
         ymin /= 2;
      } else {
         ymax += 10;
         ymin -= 10;
      }
   }

   /* calculate X limits */
   xmin = (float) (-scale / 3600.0 + toffset / 3600.0);
   xmax = (float) (toffset / 3600.0);

   /* caluclate required space for Y-axis */
   aoffset =
       vaxis(im, gdFontSmall, fgcol, gridcol, 0, 0, height, -3, -5, -7, -8, 0, ymin, ymax,
             logaxis);
   aoffset += 2;

   x1 = aoffset;
   y1 = height - 20;
   x2 = width - 20;
   y2 = 20;

   gdImageFilledRectangle(im, x1, y2, x2, y1, bgcol);

   /* draw axis frame */
   taxis(im, gdFontSmall, fgcol, gridcol, x1, y1, x2 - x1, width, 3, 5, 9, 10, 0,
         ss_time() - scale + toffset, ss_time() + toffset);

   vaxis(im, gdFontSmall, fgcol, gridcol, x1, y1, y1 - y2, -3, -5, -7, -8, x2 - x1, ymin,
         ymax, logaxis);
   gdImageLine(im, x1, y2, x2, y2, fgcol);
   gdImageLine(im, x2, y2, x2, y1, fgcol);

   xs = ys = xold = yold = 0;

   /* write run markes if selected */
   if (runmarker) {

      const char* event_names[] = {
         "Run transitions", 
         "Run transitions", 
         0 };

      const char* tag_names[] = {
         "State", 
         "Run number", 
         0 };

      const int tag_indexes[] = {
         0,
         0,
         0 };

      int num_entries[3];
      time_t *tbuf[3];
      double *dbuf[3];
      int     st[3];

      num_entries[0] = 0;
      num_entries[1] = 0;

      status = mh->hs_read(ss_time() - scale + toffset - scale, ss_time() + toffset, 0,
                           2, event_names, tag_names, tag_indexes,
                           num_entries, tbuf, dbuf, st);

      //printf("read run info: status %d, entries %d %d\n", status, num_entries[0], num_entries[1]);

      int n_marker = num_entries[0];

      if (status == HS_SUCCESS && n_marker > 0 && n_marker < 100) {
         xs_old = -1;
         xmaxm = x1;
         for (j = 0; j < (int) n_marker; j++) {
            int col;

            x_marker = (int)(tbuf[1][j] - ss_time());
            xs = (int) ((x_marker / 3600.0 - xmin) / (xmax - xmin) * (x2 - x1) + x1 +
                        0.5);

            if (xs < x1)
               continue;
            if (xs >= x2)
               continue;

            double run_number = dbuf[1][j];

            if (xs <= xs_old)
               xs = xs_old + 1;
            xs_old = xs;

            if (dbuf[0][j] == 1)
               col = state_col[0];
            else if (dbuf[0][j] == 2)
               col = state_col[1];
            else if (dbuf[0][j] == 3)
               col = state_col[2];
            else
               col = state_col[0];

            gdImageDashedLine(im, xs, y1, xs, y2, col);

            sprintf(str, "%.0f", run_number);

            if (dbuf[0][j] == STATE_RUNNING) {
               if (xs > xmaxm) {
                  gdImageStringUp(im, gdFontSmall, xs + 0,
                                  y2 + 2 + gdFontSmall->w * strlen(str), str, fgcol);
                  xmaxm = xs - 2 + gdFontSmall->h;
               }
            } else if (dbuf[0][j] == STATE_STOPPED) {
               if (xs + 2 - gdFontSmall->h > xmaxm) {
                  gdImageStringUp(im, gdFontSmall, xs + 2 - gdFontSmall->h,
                                  y2 + 2 + gdFontSmall->w * strlen(str), str, fgcol);
                  xmaxm = xs - 1;
               }
            }
         }
      }

      if (num_entries[0]) {
         free(tbuf[0]);
         free(dbuf[0]);
         tbuf[0] = NULL;
         dbuf[0] = NULL;
      }

      if (num_entries[1]) {
         free(tbuf[1]);
         free(dbuf[1]);
         tbuf[1] = NULL;
         dbuf[1] = NULL;
      }
   }

   for (i = 0; i < n_vars; i++) {
      if (index != -1 && index != i)
         continue;

      /* draw alarm limits */
      if (lower_limit[i] != -12345) {
         if (logaxis) {
            if (lower_limit[i] <= 0)
               ys = y1;
            else
               ys = (int) (y1 -
                           (log(lower_limit[i]) - log(ymin)) / (log(ymax) -
                                                                log(ymin)) * (y1 - y2) +
                           0.5);
         } else
            ys = (int) (y1 - (lower_limit[i] - ymin) / (ymax - ymin) * (y1 - y2) + 0.5);

         if (xs < 0)
            xs = 0;
         if (xs >= width)
            xs = width-1;
         if (ys < 0)
            ys = 0;
         if (ys >= height)
            ys = height-1;

         if (ys > y2 && ys < y1) {
            gdImageDashedLine(im, x1, ys, x2, ys, curve_col[i]);

            poly[0].x = x1;
            poly[0].y = ys;
            poly[1].x = x1 + 5;
            poly[1].y = ys;
            poly[2].x = x1;
            poly[2].y = ys - 5;

            gdImageFilledPolygon(im, poly, 3, curve_col[i]);
         }
      }
      if (upper_limit[i] != -12345) {
         if (logaxis) {
            if (upper_limit[i] <= 0)
               ys = y1;
            else
               ys = (int) (y1 -
                           (log(upper_limit[i]) - log(ymin)) / (log(ymax) -
                                                                log(ymin)) * (y1 - y2) +
                           0.5);
         } else
            ys = (int) (y1 - (upper_limit[i] - ymin) / (ymax - ymin) * (y1 - y2) + 0.5);

         if (xs < 0)
            xs = 0;
         if (xs >= width)
            xs = width-1;
         if (ys < 0)
            ys = 0;
         if (ys >= height)
            ys = height-1;

         if (ys > y2 && ys < y1) {
            gdImageDashedLine(im, x1, ys, x2, ys, curve_col[i]);

            poly[0].x = x1;
            poly[0].y = ys;
            poly[1].x = x1 + 5;
            poly[1].y = ys;
            poly[2].x = x1;
            poly[2].y = ys + 5;

            gdImageFilledPolygon(im, poly, 3, curve_col[i]);
         }
      }

      for (j = 0; j < (int) n_point[i]; j++) {
         xs = (int) ((x[i][j] / 3600.0 - xmin) / (xmax - xmin) * (x2 - x1) + x1 + 0.5);

         if (logaxis) {
            if (y[i][j] <= 0)
               ys = y1;
            else
               ys = (int) (y1 -
                           (log(y[i][j]) - log(ymin)) / (log(ymax) - log(ymin)) * (y1 -
                                                                                   y2) +
                           0.5);
         } else
            ys = (int) (y1 - (y[i][j] - ymin) / (ymax - ymin) * (y1 - y2) + 0.5);

         if (xs < 0)
            xs = 0;
         if (xs >= width)
            xs = width-1;
         if (ys < 0)
            ys = 0;
         if (ys >= height)
            ys = height-1;

         if (j > 0)
            gdImageLine(im, xold, yold, xs, ys, curve_col[i]);
         xold = xs;
         yold = ys;
      }

      if (n_point[i] > 0) {
         poly[0].x = xs;
         poly[0].y = ys;
         poly[1].x = xs + 12;
         poly[1].y = ys - 6;
         poly[2].x = xs + 12;
         poly[2].y = ys + 6;

         gdImageFilledPolygon(im, poly, 3, curve_col[i]);
      }
   }

   if (labels) {
      for (i = 0; i < n_vars; i++) {
         if (index != -1 && index != i)
            continue;

         str[0] = 0;
         status = db_find_key(hDB, hkeypanel, "Label", &hkeydvar);
         if (status == DB_SUCCESS) {
            size = sizeof(str);
            status = db_get_data_index(hDB, hkeydvar, str, &size, i, TID_STRING);
         }

         if (status != DB_SUCCESS || strlen(str) < 1) {
            if (factor[i] != 1) {
               if (offset[i] == 0)
                  sprintf(str, "%s * %1.2lG", strchr(tag_name[i], ':') + 1, factor[i]);
               else
                  sprintf(str, "%s * %1.2lG %c %1.5lG", strchr(tag_name[i], ':') + 1,
                          factor[i], offset[i] < 0 ? '-' : '+', fabs(offset[i]));
            } else {
               if (offset[i] == 0)
                  sprintf(str, "%s", strchr(tag_name[i], ':') + 1);
               else
                  sprintf(str, "%s %c %1.5lG", strchr(tag_name[i], ':') + 1,
                          offset[i] < 0 ? '-' : '+', fabs(offset[i]));
            }
         }

         if (show_values) {
            char xstr[256];
            if (n_point[i] > 0)
               sprintf(xstr," = %g", y[i][n_point[i]-1]);
            else
               sprintf(xstr," = no data");
            strlcat(str, xstr, sizeof(str));
         }

         if (strlen(var_status[i]) > 1) {
            char xstr[256];
            sprintf(xstr," (%s)", var_status[i]);
            strlcat(str, xstr, sizeof(str));
         }

         row = index == -1 ? i : 0;

         gdImageFilledRectangle(im,
                                x1 + 10,
                                y2 + 10 + row * (gdFontMediumBold->h + 10),
                                x1 + 10 + strlen(str) * gdFontMediumBold->w + 10,
                                y2 + 10 + row * (gdFontMediumBold->h + 10) +
                                gdFontMediumBold->h + 2 + 2, white);
         gdImageRectangle(im, x1 + 10, y2 + 10 + row * (gdFontMediumBold->h + 10),
                          x1 + 10 + strlen(str) * gdFontMediumBold->w + 10,
                          y2 + 10 + row * (gdFontMediumBold->h + 10) +
                          gdFontMediumBold->h + 2 + 2, curve_col[i]);

         gdImageString(im, gdFontMediumBold,
                       x1 + 10 + 5, y2 + 10 + 2 + row * (gdFontMediumBold->h + 10), str,
                       curve_col[i]);
      }
   }

   gdImageRectangle(im, x1, y2, x2, y1, fgcol);

 error:

   /* generate GIF */
   gdImageInterlace(im, 1);
   gdImageGif(im, &gb);
   gdImageDestroy(im);
   length = gb.size;

   if (buffer == NULL) {
      rsprintf("HTTP/1.0 200 Document follows\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());

      rsprintf("Content-Type: image/gif\r\n");
      rsprintf("Content-Length: %d\r\n", length);
      rsprintf("Pragma: no-cache\r\n");
      rsprintf("Expires: Fri, 01-Jan-1983 00:00:00 GMT\r\n\r\n");

      if (length > (int) (return_size - strlen(return_buffer))) {
         printf("return buffer too small\n");
         return;
      }

      return_length = strlen(return_buffer) + length;
      memcpy(return_buffer + strlen(return_buffer), gb.data, length);
   } else {
      if (length > *buffer_size) {
         printf("return buffer too small\n");
         return;
      }

      memcpy(buffer, gb.data, length);
      *buffer_size = length;
   }
}

/*------------------------------------------------------------------*/

void show_query_page(const char *path)
{
   int i;
   time_t ltime_start, ltime_end;
   HNDLE hDB;
   char str[256], redir[256];
   time_t now;
   struct tm *ptms, tms;

   if (*getparam("m1")) {
      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = atoi(getparam("y1")) % 100;

      strlcpy(str, getparam("m1"), sizeof(str));
      for (i = 0; i < 12; i++)
         if (equal_ustring(str, mname[i]))
            break;
      if (i == 12)
         i = 0;

      tms.tm_mon = i;
      tms.tm_mday = atoi(getparam("d1"));
      tms.tm_hour = 0;

      if (tms.tm_year < 90)
         tms.tm_year += 100;

      ltime_start = mktime(&tms);
      memset(&tms, 0, sizeof(struct tm));
      tms.tm_year = atoi(getparam("y2")) % 100;

      strlcpy(str, getparam("m2"), sizeof(str));
      for (i = 0; i < 12; i++)
         if (equal_ustring(str, mname[i]))
            break;
      if (i == 12)
         i = 0;

      tms.tm_mon = i;
      tms.tm_mday = atoi(getparam("d2"));
      tms.tm_hour = 0;

      if (tms.tm_year < 90)
         tms.tm_year += 100;
      ltime_end = mktime(&tms);
      ltime_end += 3600 * 24;

      strcpy(str, path);
      if (strrchr(str, '/'))
         strcpy(str, strrchr(str, '/')+1);
      sprintf(redir, "%s?scale=%d&offset=%d", str, (int) (ltime_end - ltime_start),
              MIN((int) (ltime_end - ss_time()), 0));
      redirect(redir);
      return;
   }

   cm_get_experiment_database(&hDB, NULL);

   strcpy(str, path);
   if (strrchr(str, '/'))
      strcpy(str, strrchr(str, '/')+1);
   show_header(hDB, "History", "GET", str, 1, 0);

   /* menu buttons */
   rsprintf("<tr><td colspan=2 bgcolor=#C0C0C0>\n");
   rsprintf("<input type=submit name=cmd value=Query>\n");
   rsprintf("<input type=submit name=cmd value=History>\n");
   rsprintf("<input type=submit name=cmd value=Status></tr>\n");
   rsprintf("</tr>\n\n");

   time(&now);
   now -= 3600 * 24;
   ptms = localtime(&now);
   ptms->tm_year += 1900;

   rsprintf("<tr><td nowrap bgcolor=#CCCCFF>Start date:</td>", "Start date");

   rsprintf("<td bgcolor=#DDEEBB>Month: <select name=\"m1\">\n");
   rsprintf("<option value=\"\">\n");
   for (i = 0; i < 12; i++)
      if (i == ptms->tm_mon)
         rsprintf("<option selected value=\"%s\">%s\n", mname[i], mname[i]);
      else
         rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
   rsprintf("</select>\n");

   rsprintf("&nbsp;Day: <select name=\"d1\">");
   rsprintf("<option selected value=\"\">\n");
   for (i = 0; i < 31; i++)
      if (i + 1 == ptms->tm_mday)
         rsprintf("<option selected value=%d>%d\n", i + 1, i + 1);
      else
         rsprintf("<option value=%d>%d\n", i + 1, i + 1);
   rsprintf("</select>\n");

   rsprintf
       ("&nbsp;Year: <input type=\"text\" size=5 maxlength=5 name=\"y1\" value=\"%d\">",
        ptms->tm_year);
   rsprintf("</td></tr>\n");

   rsprintf("<tr><td nowrap bgcolor=#CCCCFF>End date:</td>");
   time(&now);
   ptms = localtime(&now);
   ptms->tm_year += 1900;

   rsprintf("<td bgcolor=#DDEEBB>Month: <select name=\"m2\">\n");
   rsprintf("<option value=\"\">\n");
   for (i = 0; i < 12; i++)
      if (i == ptms->tm_mon)
         rsprintf("<option selected value=\"%s\">%s\n", mname[i], mname[i]);
      else
         rsprintf("<option value=\"%s\">%s\n", mname[i], mname[i]);
   rsprintf("</select>\n");

   rsprintf("&nbsp;Day: <select name=\"d2\">");
   rsprintf("<option selected value=\"\">\n");
   for (i = 0; i < 31; i++)
      if (i + 1 == ptms->tm_mday)
         rsprintf("<option selected value=%d>%d\n", i + 1, i + 1);
      else
         rsprintf("<option value=%d>%d\n", i + 1, i + 1);
   rsprintf("</select>\n");

   rsprintf
       ("&nbsp;Year: <input type=\"text\" size=5 maxlength=5 name=\"y2\" value=\"%d\">",
        ptms->tm_year);
   rsprintf("</td></tr>\n");

   rsprintf("</table>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

static int cmp_names(const void *a, const void *b)
{
  int i;
  const char*sa = (const char*)a;
  const char*sb = (const char*)b;

  int debug = 0;

  // Cannot use strcmp() because it does not know how to compare numerical values, e.g.
  // it thinks "111" is smaller than "9"
  //return strcmp(sa, sb);

  if (debug)
    printf("compare [%s] and [%s]\n", sa, sb);

  for (i=0; ; i++) {
    if (sa[i]==0 && sb[i]==0)
      return 0; // both strings have the same length and the same characters

    //printf("index %d, char [%c] [%c], isdigit %d %d\n", i, sa[i], sb[i], isdigit(sa[i]), isdigit(sb[i]));

    if (isdigit(sa[i]) && isdigit(sb[i])) {
      int va = atoi(sa+i);
      int vb = atoi(sb+i);

      if (debug)
        printf("index %d, values %d %d\n", i, va, vb);

      if (va < vb)
        return -1;
      else if (va > vb)
        return 1;

      // values are equal, skip the the end of the digits, compare any trailing text
      continue;
    }

    if (sa[i]==sb[i]) {
      continue;
    }

    if (debug)
      printf("index %d, char [%c] [%c]\n", i, sa[i], sb[i]);
    
    if (sa[i] == 0) // string sa is shorter
      return -1;
    else if (sb[i] == 0) // string sb is shorter
      return 1;

    if (sa[i]<sb[i])
      return -1;
    else
      return 1;
  }

  // NOT REACHED
}

const bool cmp_events(const std::string& a, const std::string& b)
{
  return cmp_names(a.c_str(), b.c_str()) < 0;
}

const bool cmp_events1(const std::string& a, const std::string& b)
{
  return a < b;
}

const bool cmp_tags(const TAG& a, const TAG& b)
{
  return cmp_names(a.name, b.name) < 0;
}

#if 0
static int cmp_tags(const void *a, const void *b)
{
  const TAG*sa = (const TAG*)a;
  const TAG*sb = (const TAG*)b;
  return cmp_names(sa->name, sb->name);
}

static void sort_tags(int ntags, TAG* tags)
{
   qsort(tags, ntags, sizeof(TAG), cmp_tags);
}
#endif

#define STRLCPY(dst, src) strlcpy((dst), (src), sizeof(dst))
#define STRLCAT(dst, src) strlcat((dst), (src), sizeof(dst))

struct hist_var_t
{
   char event_name[NAME_LENGTH];
   char var_name[NAME_LENGTH];
   float hist_factor;
   float hist_offset;
   char hist_col[NAME_LENGTH];
   char hist_label[NAME_LENGTH];
   int  hist_order;
};

static int cmp_vars(const void *a, const void *b)
{
   return ((const hist_var_t*)a)->hist_order >= ((const hist_var_t*)b)->hist_order;
}

static void print_vars(const hist_var_t vars[])
{
   for (int i=0; i<MAX_VARS; i++) {
      if (vars[i].event_name[0]==0)
         break;
      printf("%d event [%s][%s] factor %f, offset %f, color [%s] label [%s] order %d\n", i, vars[i].event_name, vars[i].var_name, vars[i].hist_factor, vars[i].hist_offset, vars[i].hist_col, vars[i].hist_label, vars[i].hist_order);
   }
}

int xdb_get_data_index(HNDLE hDB, const char* str, void *value, int size, int index, int tid)
{
   HNDLE hKey;
   int status = db_find_key(hDB, 0, str, &hKey);
   if (status != DB_SUCCESS)
      return status;

   KEY key;
   db_get_key(hDB, hKey, &key);
   if (index >= key.num_values)
      return DB_OUT_OF_RANGE;

   status = db_get_data_index(hDB, hKey, value, &size, index, tid);
   return status;
}

static void load_vars_odb(HNDLE hDB, const char* path, hist_var_t vars[])
{
   for (int index=0; index<MAX_VARS; index++) {
      char str[256];
      char var_name_odb[256];

      var_name_odb[0] = 0;

      sprintf(str, "/History/Display/%s/Variables", path);
      xdb_get_data_index(hDB, str, var_name_odb, sizeof(var_name_odb), index, TID_STRING);

      if (var_name_odb[0] == 0)
         break;

      //printf("index %d, var_name_odb %s\n", index, var_name_odb);

      char* s = strchr(var_name_odb, ':');

      //printf("index %d, var_name_odb [%s] %p [%s]\n", index, var_name_odb, s, s?s:"");

      if (s)
         *s = 0;
      STRLCPY(vars[index].event_name, var_name_odb);
      if (s)
         STRLCPY(vars[index].var_name, s+1);

      //printf("index %d, var_name_odb [%s] %p [%s]\n", index, var_name_odb, s, s?(s+1):"");

      vars[index].hist_factor = 1;

      sprintf(str, "/History/Display/%s/Factor", path);
      xdb_get_data_index(hDB, str, &vars[index].hist_factor, sizeof(float), index, TID_FLOAT);

      vars[index].hist_offset = 0;

      sprintf(str, "/History/Display/%s/Offset", path);
      xdb_get_data_index(hDB, str, &vars[index].hist_offset, sizeof(float), index, TID_FLOAT);

      sprintf(str, "/History/Display/%s/Colour", path);
      xdb_get_data_index(hDB, str, &vars[index].hist_col, sizeof(vars[index].hist_col), index, TID_STRING);

      sprintf(str, "/History/Display/%s/Label", path);
      xdb_get_data_index(hDB, str, &vars[index].hist_label, sizeof(vars[index].hist_label), index, TID_STRING);

      vars[index].hist_order = (index+1)*10;
   }

   //print_vars(vars);
}

static void load_vars_param(hist_var_t vars[])
{
   for (int index=0; index<MAX_VARS; index++) {
      char str[256];

      sprintf(str, "event%d", index);
      STRLCPY(vars[index].event_name, getparam(str));

      if (strlen(vars[index].event_name) < 1) {
         vars[index].event_name[0] = 0;
         break;
      }

      sprintf(str, "var%d", index);
      STRLCPY(vars[index].var_name, getparam(str));

      sprintf(str, "fac%d", index);
      vars[index].hist_factor = (float) atof(getparam(str));
    
      sprintf(str, "ofs%d", index);
      vars[index].hist_offset = (float) atof(getparam(str));
    
      sprintf(str, "col%d", index);
      STRLCPY(vars[index].hist_col, getparam(str));

      sprintf(str, "lab%d", index);
      STRLCPY(vars[index].hist_label, getparam(str));

      sprintf(str, "ord%d", index);
      vars[index].hist_order = atoi(getparam(str));
   }
}

static int xdb_find_key(HNDLE hDB, HNDLE dir, const char* str, HNDLE* hKey, int tid, int size)
{
   int status = db_find_key(hDB, dir, str, hKey);
   if (status == DB_SUCCESS)
      return status;

   db_create_key(hDB, dir, str, tid);
   status = db_find_key(hDB, dir, str, hKey);
   assert(*hKey);

   if (tid == TID_STRING) {
      db_set_data_index(hDB, *hKey, "", size, 0, TID_STRING);
   }

   return status;
}

static void resize_vars_odb(HNDLE hDB, const char* path, int index)
{
   int status;
   HNDLE hKey;
   char str[256];

   if (index == 0) {
      index = 1;
   }

   sprintf(str, "/History/Display/%s/Variables", path);
   xdb_find_key(hDB, 0, str, &hKey, TID_STRING, 2*NAME_LENGTH);

   status = db_set_num_values(hDB, hKey, index);
   assert(status == DB_SUCCESS);

   sprintf(str, "/History/Display/%s/Label", path);
   xdb_find_key(hDB, 0, str, &hKey, TID_STRING, NAME_LENGTH);
   status = db_set_num_values(hDB, hKey, index);
   assert(status == DB_SUCCESS);

   sprintf(str, "/History/Display/%s/Colour", path);
   xdb_find_key(hDB, 0, str, &hKey, TID_STRING, NAME_LENGTH);
   status = db_set_num_values(hDB, hKey, index);
   assert(status == DB_SUCCESS);

   sprintf(str, "/History/Display/%s/Factor", path);
   xdb_find_key(hDB, 0, str, &hKey, TID_FLOAT, 0);
   status = db_set_num_values(hDB, hKey, index);
   assert(status == DB_SUCCESS);

   sprintf(str, "/History/Display/%s/Offset", path);
   xdb_find_key(hDB, 0, str, &hKey, TID_FLOAT, 0);
   status = db_set_num_values(hDB, hKey, index);
   assert(status == DB_SUCCESS);
}

void save_vars_odb(HNDLE hDB, const char* path, const hist_var_t vars[])
{
   int numvars = 0;
   for (int i=0; i<MAX_VARS; i++)
      if (vars[i].event_name[0] == 0) {
         numvars = i;
         break;
      }

   resize_vars_odb(hDB, path, numvars);

   for (int index=0; index<numvars; index++) {
      HNDLE hKey;
      char str[256];
      char var_name[256];
      sprintf(var_name, "%s:%s", vars[index].event_name, vars[index].var_name);

      sprintf(str, "/History/Display/%s/Variables", path);
      xdb_find_key(hDB, 0, str, &hKey, TID_STRING, 2*NAME_LENGTH);
      db_set_data_index(hDB, hKey, var_name, 2 * NAME_LENGTH, index, TID_STRING);

      sprintf(str, "/History/Display/%s/Factor", path);
      xdb_find_key(hDB, 0, str, &hKey, TID_FLOAT, 0);
      db_set_data_index(hDB, hKey, &vars[index].hist_factor, sizeof(float), index, TID_FLOAT);
     
      sprintf(str, "/History/Display/%s/Offset", path);
      xdb_find_key(hDB, 0, str, &hKey, TID_FLOAT, 0);
      db_set_data_index(hDB, hKey, &vars[index].hist_offset, sizeof(float), index, TID_FLOAT);
     
      sprintf(str, "/History/Display/%s/Colour", path);
      xdb_find_key(hDB, 0, str, &hKey, TID_STRING, NAME_LENGTH);
      db_set_data_index(hDB, hKey, vars[index].hist_col, NAME_LENGTH, index, TID_STRING);
     
      sprintf(str, "/History/Display/%s/Label", path);
      xdb_find_key(hDB, 0, str, &hKey, TID_STRING, NAME_LENGTH);
      db_set_data_index(hDB, hKey, vars[index].hist_label, NAME_LENGTH, index, TID_STRING);
   }
}

static void add_vars(struct hist_var_t vars[], const char* event_name, const char* tag_name)
{
   for (int i=0; i<MAX_VARS; i++) {
     if (vars[i].event_name[0] == 0) {
       STRLCPY(vars[i].event_name, event_name);
       STRLCPY(vars[i].var_name, tag_name);
       vars[i].hist_factor = 1;
       vars[i].hist_order = (i+1)*10;
       return;
     }
   }
}

/*------------------------------------------------------------------*/

void show_hist_config_page(const char *path, const char *hgroup, const char *panel)
{
   int status, size, index, sort_vars;
   BOOL flag;
   HNDLE hDB, hKeyVar;
   int max_display_events = 20;
   int max_display_tags = 200;
   char str[256], cmd[256], ref[256];
   struct hist_var_t vars[MAX_VARS];
   char def_hist_col[MAX_VARS][NAME_LENGTH] = { "#0000FF", "#00C000", "#FF0000", "#00C0C0", "#FF00FF",
      "#C0C000", "#808080", "#80FF80", "#FF8080", "#8080FF"
   };

   cm_get_experiment_database(&hDB, NULL);

   size = sizeof(max_display_events);
   db_get_value(hDB, 0, "/History/MaxDisplayEvents", &max_display_events, &size, TID_INT, TRUE);

   size = sizeof(max_display_tags);
   db_get_value(hDB, 0, "/History/MaxDisplayTags", &max_display_tags, &size, TID_INT, TRUE);

   strlcpy(cmd, getparam("cmd"), sizeof(cmd));
   hKeyVar = 0;

   if (equal_ustring(cmd, "Clear history cache")) {
      strcpy(cmd, "refresh");
      if (mh)
         mh->hs_clear_cache();
   }

   memset(vars, 0, sizeof(vars));

   for (int i=0; i<MAX_VARS; i++) {
      if (def_hist_col[i])
         STRLCPY(vars[i].hist_col, def_hist_col[i]);
      vars[i].hist_factor = 1;
   }

   //printf("cmd [%s]\n", cmd);
   //printf("cmdx [%s]\n", getparam("cmdx"));

   /* load variables from web request parameters */
   /* if list is empty, we are here for the first time,
    * load variables from ODB */

   load_vars_param(vars);
   if ((vars[0].event_name[0] == 0) && !equal_ustring(cmd, "refresh") && !equal_ustring(cmd, "save"))
      load_vars_odb(hDB, path, vars);

   /* delete variables according to "hist_order" */

   for (int i=0; i<MAX_VARS; i++) {
      if (vars[i].event_name[0]==0) {
         break;
      }

      while ((vars[i].hist_order <= 0) && (vars[i].event_name[0] != 0)) {
         for (int j=i+1; j<MAX_VARS; j++)
            vars[j-1] = vars[j];
      }
   }

   /* sort variables according to "hist_order" */

   bool need_sort = false;
   int num_vars = 0;
   for (int i=1; i<MAX_VARS; i++) {
      if (vars[i].event_name[0]==0) {
         num_vars = i;
         break;
      }
      if (vars[i-1].hist_order >= vars[i].hist_order) {
         need_sort = true;
      }
   }

   if (need_sort) {
      /* sort variables by order */
      qsort(vars, num_vars, sizeof(vars[0]), cmp_vars);

      for (int index=0; index<MAX_VARS; index++)
         vars[index].hist_order = (index+1)*10;
   }

   if (strlen(getparam("seln")) > 0) {
      int seln = atoi(getparam("seln"));
      for (int i=0; i<seln; i++) {
         char str[256];
         sprintf(str, "sel%d", i);

	 char event_name[256];
	 STRLCPY(event_name, getparam(str));
	 if (strlen(event_name) < 1)
            continue;

	 char tag_name[256];
	 char* s = strchr(event_name, ':');
	 if (!s)
            continue;
	 *s = 0;
	 STRLCPY(tag_name, s+1);

         add_vars(vars, event_name, tag_name);
      } 
   }

   //print_vars(vars);

   if (cmd[0] && equal_ustring(cmd, "save")) {
      save_vars_odb(hDB, path, vars);

      if (*getparam("timescale")) {
         sprintf(ref, "/History/Display/%s/Timescale", path);
         strlcpy(str, getparam("timescale"), sizeof(str));
         db_set_value(hDB, 0, ref, str, NAME_LENGTH, 1, TID_STRING);
      }

      if (*getparam("minimum")) {
         float val = (float) strtod(getparam("minimum"),NULL);
         sprintf(ref, "/History/Display/%s/Minimum", path);
         db_set_value(hDB, 0, ref, &val, sizeof(val), 1, TID_FLOAT);
      }

      if (*getparam("maximum")) {
         float val = (float) strtod(getparam("maximum"),NULL);
         sprintf(ref, "/History/Display/%s/Maximum", path);
         db_set_value(hDB, 0, ref, &val, sizeof(val), 1, TID_FLOAT);
      }

      sprintf(ref, "/History/Display/%s/Zero ylow", path);
      flag = *getparam("zero_ylow");
      db_set_value(hDB, 0, ref, &flag, sizeof(flag), 1, TID_BOOL);

      sprintf(ref, "/History/Display/%s/Log Axis", path);
      flag = *getparam("log_axis");
      db_set_value(hDB, 0, ref, &flag, sizeof(flag), 1, TID_BOOL);

      sprintf(ref, "/History/Display/%s/Show run markers", path);
      flag = *getparam("run_markers");
      db_set_value(hDB, 0, ref, &flag, sizeof(flag), 1, TID_BOOL);

      sprintf(ref, "/History/Display/%s/Show values", path);
      flag = *getparam("show_values");
      db_set_value(hDB, 0, ref, &flag, sizeof(flag), 1, TID_BOOL);

      sprintf(ref, "/History/Display/%s/Sort Vars", path);
      flag = *getparam("sort_vars");
      db_set_value(hDB, 0, ref, &flag, sizeof(flag), 1, TID_BOOL);

      strlcpy(str, path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      redirect(str);
      return;
   }

   if (panel[0]) {
      str[0] = 0;
      for (const char* p=path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      strlcat(str, hgroup, sizeof(str));
      strlcat(str, "/", sizeof(str));
      strlcat(str, panel, sizeof(str));
   } else {
      strlcpy(str, path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
   }
   show_header(hDB, "History Config", "GET", str, 4, 0);

   /* menu buttons */
   rsprintf("<tr><td colspan=8 bgcolor=\"#C0C0C0\">\n");
   rsprintf("<input type=submit name=cmd value=Save>\n");
   rsprintf("<input type=submit name=cmd value=Cancel>\n");
   rsprintf("<input type=submit name=cmd value=Refresh>\n");
   rsprintf("<input type=submit name=cmd value=\"Clear history cache\">\n");
   rsprintf("<input type=submit name=cmd value=\"Delete Panel\">\n");
   rsprintf("</td></tr>\n");

   rsprintf("<tr><td colspan=8 bgcolor=\"#FFFF00\" align=center><b>Panel \"%s / %s\"</b>\n",
            hgroup, panel);

   /* hidden command for refresh */
   rsprintf("<input type=hidden name=cmd value=Refresh>\n");
   rsprintf("<input type=hidden name=panel value=\"%s\">\n", panel);
   rsprintf("<input type=hidden name=group value=\"%s\">\n", hgroup);
   rsprintf("</td></tr>\n");

   /* time scale */
   if (equal_ustring(cmd, "refresh"))
      strlcpy(str, getparam("timescale"), sizeof(str));
   else {
      sprintf(ref, "/History/Display/%s/%s/Timescale", hgroup, panel);
      size = NAME_LENGTH;
      db_get_value(hDB, 0, ref, str, &size, TID_STRING, TRUE);
   }
   rsprintf("<tr><td bgcolor=\"#E0E0E0\" colspan=8>Time scale: &nbsp;&nbsp;");
   rsprintf("<input type=text name=timescale value=%s></td></tr>\n", str);

   /* ylow_zero */
   if (equal_ustring(cmd, "refresh"))
      flag = *getparam("zero_ylow");
   else {
      sprintf(ref, "/History/Display/%s/%s/Zero ylow", hgroup, panel);
      size = sizeof(flag);
      db_get_value(hDB, 0, ref, &flag, &size, TID_BOOL, TRUE);
   }
   if (flag)
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox checked name=zero_ylow value=1>",
           str);
   else
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox name=zero_ylow value=1>",
           str);
   rsprintf("&nbsp;&nbsp;Zero Ylow</td></tr>\n");

   /* minimum */
   if (equal_ustring(cmd, "refresh"))
      strlcpy(str, getparam("minimum"), sizeof(str));
   else {
      float xxminimum = 0;
      sprintf(ref, "/History/Display/%s/%s/Minimum", hgroup, panel);
      size = sizeof(float);
      db_get_value(hDB, 0, ref, &xxminimum, &size, TID_FLOAT, TRUE);
      sprintf(str, "%f", xxminimum);
   }
   rsprintf("<tr><td bgcolor=\"#E0E0E0\" colspan=8>Minimum: &nbsp;&nbsp;");
   rsprintf("<input type=text name=minimum value=%s></td></tr>\n", str);

   /* maximum */
   if (equal_ustring(cmd, "refresh"))
      strlcpy(str, getparam("maximum"), sizeof(str));
   else {
      float xxmaximum = 0;
      sprintf(ref, "/History/Display/%s/%s/Maximum", hgroup, panel);
      size = sizeof(float);
      db_get_value(hDB, 0, ref, &xxmaximum, &size, TID_FLOAT, TRUE);
      sprintf(str, "%f", xxmaximum);
   }
   rsprintf("<tr><td bgcolor=\"#E0E0E0\" colspan=8>Maximum: &nbsp;&nbsp;");
   rsprintf("<input type=text name=maximum value=%s></td></tr>\n", str);

   /* log_axis */
   if (equal_ustring(cmd, "refresh"))
      flag = *getparam("log_axis");
   else {
      sprintf(ref, "/History/Display/%s/%s/Log axis", hgroup, panel);
      size = sizeof(flag);
      db_get_value(hDB, 0, ref, &flag, &size, TID_BOOL, TRUE);
   }
   if (flag)
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox checked name=log_axis value=1>",
           str);
   else
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox name=log_axis value=1>",
           str);
   rsprintf("&nbsp;&nbsp;Logarithmic Y axis</td></tr>\n");

   /* run_markers */
   if (equal_ustring(cmd, "refresh"))
      flag = *getparam("run_markers");
   else {
      sprintf(ref, "/History/Display/%s/%s/Show run markers", hgroup, panel);
      size = sizeof(flag);
      db_get_value(hDB, 0, ref, &flag, &size, TID_BOOL, TRUE);
   }
   if (flag)
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox checked name=run_markers value=1>",
           str);
   else
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox name=run_markers value=1>",
           str);
   rsprintf("&nbsp;&nbsp;Show run markers</td></tr>\n");

   /* show_values */
   if (equal_ustring(cmd, "refresh"))
      flag = *getparam("show_values");
   else {
      sprintf(ref, "/History/Display/%s/Show values", path);
      size = sizeof(flag);
      flag = 0;
      db_get_value(hDB, 0, ref, &flag, &size, TID_BOOL, FALSE);
   }
   if (flag)
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox checked name=show_values value=1>",
           str);
   else
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox name=show_values value=1>",
           str);
   rsprintf("&nbsp;&nbsp;Show values of variables</td></tr>\n");

   /* sort_vars */
   if (equal_ustring(cmd, "refresh"))
      sort_vars = *getparam("sort_vars");
   else {
      sprintf(ref, "/History/Display/%s/Sort Vars", path);
      size = sizeof(sort_vars);
      sort_vars = 0;
      db_get_value(hDB, 0, ref, &sort_vars, &size, TID_BOOL, FALSE);
   }
   if (sort_vars)
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox checked name=sort_vars value=1>",
           str);
   else
      rsprintf
          ("<tr><td bgcolor=\"#E0E0E0\" colspan=8><input type=checkbox name=sort_vars value=1>",
           str);
   rsprintf("&nbsp;&nbsp;Sort variable names (\"Save\" or \"Refresh\" to update the list)</td></tr>\n");

   /*---- events and variables ----*/

   /* get display event name */

   set_history_path();

   std::vector<std::string> events;
   mh->hs_get_events(&events);

   // has to be sorted or equipment name code below would not work
   //std::sort(events.begin(), events.end(), cmp_events);
   std::sort(events.begin(), events.end(), cmp_events1);

   if (strlen(getparam("cmdx")) > 0) {
      rsprintf("<tr><th colspan=1>Sel<th colspan=1>Equipment<th colspan=1>Event<th colspan=1>Variable</tr>\n");

      std::string cmdx = getparam("cmdx");
      std::string xeqname;

      int i=0;
      for (unsigned e=0; e<events.size(); e++) {
         std::string eqname;
         eqname = events[e].substr(0, events[e].find("/"));

         if (eqname.length() < 1)
            eqname = events[e];

         bool once = false;
         if (eqname != xeqname)
            once = true;

         std::string qcmd = "Expand " + eqname;

         //printf("param [%s] is [%s]\n", qcmd.c_str(), getparam(qcmd.c_str()));

         bool collapsed = true;

         if (cmdx == qcmd)
            collapsed = false;

         if (strlen(getparam(qcmd.c_str())) > 0)
            collapsed = false;

         if (collapsed) {
            if (eqname == xeqname)
               continue;

            rsprintf("<tr align=left>\n");
            rsprintf("<td></td>\n");
            rsprintf("<td>%s</td>\n", eqname.c_str());
            rsprintf("<td><input type=submit name=cmdx value=\"%s\"></td>\n", qcmd.c_str());
            rsprintf("<td>%s</td>\n", "");
            rsprintf("</tr>\n");
            xeqname = eqname;
            continue;
         }

         if (once)
            rsprintf("<tr><input type=hidden name=\"%s\" value=%d></tr>\n", qcmd.c_str(), 1);

         std::string rcmd = "Expand " + events[e];

         //printf("param [%s] is [%s]\n", rcmd.c_str(), getparam(rcmd.c_str()));

         collapsed = true;

         if (cmdx == rcmd)
            collapsed = false;

         if (strlen(getparam(rcmd.c_str())) > 0)
            collapsed = false;

         if (collapsed) {
            rsprintf("<tr align=left>\n");
            rsprintf("<td></td>\n");
            rsprintf("<td>%s</td>\n", eqname.c_str());
            rsprintf("<td>%s</td>\n", events[e].c_str());
            rsprintf("<td><input type=submit name=cmdx value=\"%s\"></td>\n", rcmd.c_str());
            rsprintf("</tr>\n");
            continue;
         }

         rsprintf("<tr><input type=hidden name=\"%s\" value=%d></tr>\n", rcmd.c_str(), 1);

         xeqname = eqname;

         std::vector<TAG> tags;

         status = mh->hs_get_tags(events[e].c_str(), &tags);

         if (tags.size() > 0) {

            if (sort_vars)
               std::sort(tags.begin(), tags.end(), cmp_tags);

            for (unsigned v=0; v<tags.size(); v++) {

               for (unsigned j=0; j<tags[v].n_data; j++) {
                  char tagname[256];

                  if (tags[v].n_data == 1)
                     sprintf(tagname, "%s", tags[v].name);
                  else
                     sprintf(tagname, "%s[%d]", tags[v].name, j);

		  bool checked = false;
#if 0
		  for (int index=0; index<MAX_VARS; index++) {
		    if (equal_ustring(vars[index].event_name, events[e].c_str()) && equal_ustring(vars[index].var_name, tagname)) {
		      checked = true;
		      break;
		    }
		  }
#endif
                  


		  rsprintf("<tr align=left>\n");
		  rsprintf("<td><input type=checkbox %s name=\"sel%d\" value=\"%s:%s\"></td>\n", checked?"checked":"", i++, events[e].c_str(), tagname);
		  rsprintf("<td>%s</td>\n", eqname.c_str());
		  rsprintf("<td>%s</td>\n", events[e].c_str());
		  rsprintf("<td>%s</td>\n", tagname);
		  rsprintf("</tr>\n");
	       }
	    }
	 }
      }

      rsprintf("<tr>\n");
      rsprintf("<td></td>\n");
      rsprintf("<td>\n");
      rsprintf("<input type=hidden name=seln value=%d>\n", i);
      rsprintf("<input type=submit value=\"Add Selected\">\n");
      rsprintf("</td>\n");
      rsprintf("</tr>\n");
   }

   rsprintf("<tr><th>Col<th>Event<th>Variable<th>Factor<th>Offset<th>Colour<th>Label<th>Order</tr>\n");

   //print_vars(vars);

   /* extract colours */
   for (index = 0; index < MAX_VARS; index++) {

      if (!vars[index].hist_col[0])
         strlcpy(vars[index].hist_col, "#808080", NAME_LENGTH);

      rsprintf("<tr><td bgcolor=\"%s\">&nbsp;<td>\n", vars[index].hist_col);

      /* event and variable selection */

      rsprintf("<select name=\"event%d\" size=1 onChange=\"document.form1.submit()\">\n", index);

      /* enumerate events */

      /* empty option */
      rsprintf("<option value=\"\">&lt;empty&gt;\n");

      bool found_event = false;

      if ((int)events.size() < max_display_events || (strlen(vars[index].event_name) <= 0)) {
         for (unsigned e=0; e<events.size(); e++) {
            const char *s = "";
            const char *p = events[e].c_str();
            if (equal_ustring(vars[index].event_name, p)) {
               s = "selected";
               found_event = true;
            }
            //printf("s [%s] p [%s]\n", s, p);
            rsprintf("<option %s value=\"%s\">%s\n", s, p, p);
         }
      }

      if (!found_event)
         if (strlen(vars[index].event_name) > 0) {
            rsprintf("<option selected value=\"%s\">%s\n", vars[index].event_name, vars[index].event_name);
         }

      rsprintf("</select></td>\n");

      //if (vars[index].hist_order <= 0)
      //vars[index].hist_order = (index+1)*10;

      if (vars[index].event_name[0]) {
         char selected_var[NAME_LENGTH];

         bool found_var = false;

	 STRLCPY(selected_var, vars[index].var_name);

         rsprintf("<td><select name=\"var%d\">\n", index);

         std::vector<TAG> tags;

         status = mh->hs_get_tags(vars[index].event_name, &tags);

         if (tags.size() > 0) {

            if (0) {
               printf("Compare %d\n", cmp_names("AAA", "BBB"));
               printf("Compare %d\n", cmp_names("BBB", "AAA"));
               printf("Compare %d\n", cmp_names("AAA", "AAA"));
               printf("Compare %d\n", cmp_names("A", "AAA"));
               printf("Compare %d\n", cmp_names("A111", "A1"));
               printf("Compare %d\n", cmp_names("A111", "A2"));
               printf("Compare %d\n", cmp_names("A111", "A222"));
               printf("Compare %d\n", cmp_names("A111a", "A111b"));
            }

            if (sort_vars)
               std::sort(tags.begin(), tags.end(), cmp_tags);

            if (0) {
               printf("Event [%s] %d tags\n", vars[index].event_name, (int)tags.size());

               for (unsigned v=0; v<tags.size(); v++) {
                 printf("tag[%d] [%s]\n", v, tags[v].name);
               }
            }

            int count_tags = 0;
            for (unsigned v=0; v<tags.size(); v++)
               count_tags += tags[v].n_data;

            //printf("output %d option tags\n", count_tags);

            if (count_tags < max_display_tags) {
               for (unsigned v=0; v<tags.size(); v++) {

                 for (unsigned j=0; j<tags[v].n_data; j++) {
                    char tagname[256];

                    if (tags[v].n_data == 1)
                       sprintf(tagname, "%s", tags[v].name);
                    else
                       sprintf(tagname, "%s[%d]", tags[v].name, j);

                    if (equal_ustring(selected_var, tagname)) {
                       rsprintf("<option selected value=\"%s\">%s\n", tagname, tagname);
                       found_var = true;
                    }
                    else
                       rsprintf("<option value=\"%s\">%s\n", tagname, tagname);

                    //printf("%d [%s] [%s] [%s][%s] %d\n", index, vars[index].event_name, tagname, vars[index].var_name, selected_var, found_var);
                 }
               }
            }
         }

         if (!found_var)
            if (strlen(vars[index].var_name) > 0)
               rsprintf("<option selected value=\"%s\">%s\n", vars[index].var_name, vars[index].var_name);

         rsprintf("</select></td>\n");

      rsprintf("<td><input type=text size=10 maxlength=10 name=\"fac%d\" value=%g></td>\n", index, vars[index].hist_factor);

      rsprintf("<td><input type=text size=10 maxlength=10 name=\"ofs%d\" value=%g></td>\n", index, vars[index].hist_offset);

      rsprintf("<td><input type=text size=10 maxlength=10 name=\"col%d\" value=%s></td>\n", index, vars[index].hist_col);

      rsprintf("<td><input type=text size=10 maxlength=%d name=\"lab%d\" value=\"%s\"></td>\n", NAME_LENGTH, index, vars[index].hist_label);

      rsprintf("<td><input type=text size=5 maxlength=10 name=\"ord%d\" value=\"%d\"></td>\n", index, vars[index].hist_order);

      } else {
         rsprintf("<td><input type=submit name=cmdx value=\"List all variables\"></td>\n");
         rsprintf("<input type=hidden name=\"fac%d\" value=%g>\n", index, vars[index].hist_factor);
         rsprintf("<input type=hidden name=\"col%d\" value=%s>\n", index, vars[index].hist_col);
         rsprintf("<input type=hidden name=\"ord%d\" value=\"%d\">\n", index, (index+1)*10);
      }

      rsprintf("</tr>\n");

      /* loop over all already defined entries,
       * show one "empty" entry and exit */
      if (!vars[index].event_name[0])
         break;
   }

   rsprintf("</table></form>\n");
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

void export_hist(const char *path, int scale, int toffset, int index, int labels)
{
   HNDLE hDB, hkey, hkeypanel;
   int size, status;
   char str[256];

   /* header */
   rsprintf("HTTP/1.1 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Accept-Ranges: bytes\r\n");
   rsprintf("Pragma: no-cache\r\n");
   rsprintf("Expires: Fri, 01 Jan 1983 00:00:00 GMT\r\n");
   rsprintf("Content-Type: text/plain\r\n");
   rsprintf("Content-disposition: attachment; filename=\"export.csv\"\r\n");
   rsprintf("\r\n");

   cm_get_experiment_database(&hDB, NULL);

   /* check panel name in ODB */
   sprintf(str, "/History/Display/%s", path);
   db_find_key(hDB, 0, str, &hkeypanel);
   if (!hkeypanel) {
      rsprintf("Cannot find /History/Display/%s in ODB\n", path);
      return;
   }

   /* get runmarker flag */
   BOOL runmarker = 1;
   size = sizeof(runmarker);
   db_get_value(hDB, hkeypanel, "Show run markers", &runmarker, &size, TID_BOOL, TRUE);

   if (scale == 0) {
      /* get timescale */
      strlcpy(str, "1h", sizeof(str));
      size = NAME_LENGTH;
      status = db_get_value(hDB, hkeypanel, "Timescale", str, &size, TID_STRING, TRUE);
      if (status != DB_SUCCESS) {
         /* delete old integer key */
         db_find_key(hDB, hkeypanel, "Timescale", &hkey);
         if (hkey)
            db_delete_key(hDB, hkey, FALSE);
         
         strcpy(str, "1h");
         size = NAME_LENGTH;
         status = db_get_value(hDB, hkeypanel, "Timescale", str, &size, TID_STRING, TRUE);
      }
      
      scale = time_to_sec(str);
   }

   HistoryData hsxxx;
   HistoryData* hsdata = &hsxxx;

   time_t now = ss_time();

   status = read_history(hDB, path, index, runmarker, now - scale + toffset, now + toffset, 0, hsdata);
   if (status != HS_SUCCESS) {
      rsprintf(str, "History error, status %d\n", status);
      return;
   }

   //hsdata->Print();

   int *i_var = (int *)malloc(sizeof(int)*hsdata->nvars);

   for (int i = 0; i < hsdata->nvars; i++) 
      i_var[i] = -1;

   time_t t = 0;

   /* find first time where all variables are available */
   for (int i = 0; i < hsdata->nvars; i++)
      if (hsdata->odb_index[i] >= 0)
         if ((t == 0) || (hsdata->num_entries[i] > 0 && hsdata->t[i][0] > t))
            t = hsdata->t[i][0];

   if (t == 0 && hsdata->nvars > 1) {
      rsprintf("=== No history available for choosen period ===\n");
      free(i_var);
      return;
   }

   int run_index = -1;
   int state_index = -1;
   int n_run_number = 0;
   time_t* t_run_number = NULL;
   if (runmarker)
      for (int i = 0; i < hsdata->nvars; i++) {
         if (hsdata->odb_index[i] == -2) {
            n_run_number = hsdata->num_entries[i];
            t_run_number = hsdata->t[i];
            run_index = i;
         } else if (hsdata->odb_index[i] == -1) {
            state_index = i;
         }
      }

   //printf("runmarker %d, state %d, run %d\n", runmarker, state_index, run_index);

   /* output header line with variable names */
   if (runmarker && t_run_number)
      rsprintf("Time, Timestamp, Run, Run State, ");
   else
      rsprintf("Time, Timestamp, ");

   for (int i = 0, first = 1; i < hsdata->nvars; i++) {
      if (hsdata->odb_index[i] < 0)
         continue;
      if (hsdata->num_entries[i] <= 0)
         continue;
      if (!first)
         rsprintf(", ");
      first = 0;
      rsprintf("%s", hsdata->var_names[i]);
   }
   rsprintf("\n");

   int i_run = 0;

   do {

      //printf("hsdata %p, t %d, irun %d\n", hsdata, t, i_run);

      /* find run number/state which is valid for t */
      if (runmarker && t_run_number)
         while (i_run < n_run_number-1 && t_run_number[i_run+1] <= t)
            i_run++;

      //printf("irun %d\n", i_run);

      /* find index for all variables which is valid for t */
      for (int i = 0; i < hsdata->nvars; i++)
         while (hsdata->num_entries[i] > 0 && i_var[i] < hsdata->num_entries[i] - 1 && hsdata->t[i][i_var[i]+1] <= t)
            i_var[i]++;

      /* finish if last point for all variables reached */
      bool done = true;
      for (int i = 0 ; i < hsdata->nvars ; i++)
         if (hsdata->num_entries[i] > 0 && i_var[i] < hsdata->num_entries[i] - 1) {
            done = false;
            break;
         }

      if (done)
         break;

      struct tm* tms = localtime(&t);

      char fmt[256];
      //strcpy(fmt, "%c");
      strcpy(fmt, "%Y.%m.%d %H:%M:%S");
      strftime(str, sizeof(str), fmt, tms);

      if (t_run_number && run_index>=0 && state_index>=0) {
         if (t_run_number[i_run] <= t)
            rsprintf("%s, %d, %.0f, %.0f, ", str, (int)t, hsdata->v[run_index][i_run], hsdata->v[state_index][i_run]);
         else
            rsprintf("%s, %d, N/A, N/A, ", str, (int)t);
      } else
         rsprintf("%s, %d, ", str, (int)t);
      
      //for (int i= 0 ; i < hsdata->nvars ; i++)
      //printf(" %d", i_var[i]);
      //printf("\n");

      for (int i=0, first=1 ; i<hsdata->nvars ; i++) {
         if (i_var[i] < 0)
            continue;
         if (hsdata->odb_index[i] < 0)
            continue;
         if (!first)
            rsprintf(", ");
         first = 0;
         //rsprintf("(%d %g)", i_var[i], hsdata->v[i][i_var[i]]);
         rsprintf("%g", hsdata->v[i][i_var[i]]);
      }
      rsprintf("\n");

      /* find next t as smallest delta t */
      int dt = -1;
      for (int i = 0 ; i < hsdata->nvars ; i++)
         if (i_var[i]>=0 && hsdata->odb_index[i]>=0)
            if (dt <= 0 || hsdata->t[i][i_var[i]+1] - t < dt)
               dt = (int)(hsdata->t[i][i_var[i]+1] - t);

      if (dt <= 0)
         break;

      t += dt;

   } while (1);

   free(i_var);
}

/*------------------------------------------------------------------*/

void show_hist_page(const char *path, int path_size, char *buffer, int *buffer_size,
                    int refresh)
{
   char str[256], ref[256], ref2[256], paramstr[256], scalestr[256], hgroup[256],
       bgcolor[32], fgcolor[32], gridcolor[32], url[256], dir[256], file_name[256],
       back_path[256], panel[256];
   char hurl[256], strwidth[256];
   const char *p;
   HNDLE hDB, hkey, hikeyp, hkeyp, hkeybutton;
   KEY key, ikey;
   int i, j, k, scale, offset, index, width, size, status, labels, fh, fsize;
   float factor[2];
   char def_button[][NAME_LENGTH] = { "10m", "1h", "3h", "12h", "24h", "3d", "7d" };
   time_t now;
   struct tm *tms;

   cm_get_experiment_database(&hDB, NULL);

   if (equal_ustring(getparam("cmd"), "Query")) {
      show_query_page(path);
      return;
   }

   if (equal_ustring(getparam("cmd"), "cancel")) {
      strlcpy(str, path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      redirect(str);
      return;
   }

   if (equal_ustring(getparam("cmd"), "Config") ||
       equal_ustring(getparam("cmd"), "Save")
       || equal_ustring(getparam("cmd"), "Clear history cache")
       || equal_ustring(getparam("cmd"), "Refresh")) {

      /* get group and panel from path if not given */
      if (!isparam("group")) {
         strlcpy(hgroup, path, sizeof(hgroup));
         if (strchr(hgroup, '/'))
            *strchr(hgroup, '/') = 0;
         panel[0] = 0;
         if (strrchr(path, '/'))
            strlcpy(panel, strrchr(path, '/')+1, sizeof(panel));
      } else {
         strlcpy(hgroup, getparam("group"), sizeof(hgroup));
         strlcpy(panel, getparam("panel"), sizeof(panel));
      }

      show_hist_config_page(path, hgroup, panel);
      return;
   }

   /* evaluate path pointing back to /HS */
   back_path[0] = 0;
   for (p=path ; *p ; p++)
      if (*p == '/')
         strlcat(back_path, "../", sizeof(back_path));

   if (isparam("fpanel") && isparam("fgroup") && 
      !isparam("scale")  && !isparam("shift") && !isparam("width") && !isparam("cmd")) {

      if (strchr(path, '/')) {
         strlcpy(panel, strchr(path, '/') + 1, sizeof(panel));
         strlcpy(hgroup, path, sizeof(hgroup));
         *strchr(hgroup, '/') = 0;
      } else {
         strlcpy(hgroup, path, sizeof(str));
         panel[0] = 0;
      }

      /* rewrite path if parameters come from a form submission */

      char path[256];
      /* check if group changed */
      if (!equal_ustring(getparam("fgroup"), hgroup))
         sprintf(path, "%s%s", back_path, getparam("fgroup"));
      else if (*getparam("fpanel"))
         sprintf(path, "%s%s/%s", back_path, getparam("fgroup"), getparam("fpanel"));
      else
         sprintf(path, "%s%s", back_path, getparam("fgroup"));

      redirect(path);
      return;
   }

   if (equal_ustring(getparam("cmd"), "New")) {
      strlcpy(str, path, sizeof(str));
      if (strrchr(str, '/'))
         strlcpy(str, strrchr(str, '/')+1, sizeof(str));
      show_header(hDB, "History", "GET", str, 1, 0);

      rsprintf("<tr><td align=center bgcolor=\"#FFFF80\" colspan=2>\n");
      rsprintf("Select group: &nbsp;&nbsp;");
      rsprintf("<select name=\"group\">\n");

      /* list existing groups */
      db_find_key(hDB, 0, "/History/Display", &hkey);
      if (hkey) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkey, i, &hkeyp);

            if (!hkeyp)
               break;

            db_get_key(hDB, hkeyp, &key);
            if (equal_ustring(path, key.name))
               rsprintf("<option selected>%s</option>\n", key.name);
            else
               rsprintf("<option>%s</option>\n", key.name);
         }
      }
      if (!hkey || i == 0)
         rsprintf("<option>Default</option>\n");
      rsprintf("</select><p>\n");

      rsprintf("Or enter new group name: &nbsp;&nbsp;");
      rsprintf("<input type=text size=15 maxlength=31 name=new_group>\n");

      rsprintf("<tr><td align=center bgcolor=\"#FFFF00\" colspan=2>\n");
      rsprintf("<br>Panel name: &nbsp;&nbsp;");
      rsprintf("<input type=text size=15 maxlength=31 name=panel><br><br>\n");
      rsprintf("</td></tr>\n");

      rsprintf("<tr><td align=center colspan=2>");
      rsprintf("<input type=submit value=Submit>\n");
      rsprintf("</td></tr>\n");

      rsprintf("</table>\r\n");
      rsprintf("</body></html>\r\n");
      return;
   }

   if (equal_ustring(getparam("cmd"), "Delete Panel")) {
      sprintf(str, "/History/Display/%s", path);
      if (db_find_key(hDB, 0, str, &hkey)==DB_SUCCESS)
         db_delete_key(hDB, hkey, FALSE);

      redirect("../");
      return;
   }

   if (*getparam("panel")) {
      strlcpy(panel, getparam("panel"), sizeof(panel));
      
      /* strip leading/trailing spaces */
      while (*panel == ' ') {
         strlcpy(str, panel+1, sizeof(str));
         strlcpy(panel, str, sizeof(panel));
      }
      while (strlen(panel)> 1 && panel[strlen(panel)-1] == ' ')
         panel[strlen(panel)-1] = 0;

      strlcpy(hgroup, getparam("group"), sizeof(hgroup));
      /* use new group if present */
      if (isparam("new_group") && *getparam("new_group"))
         strlcpy(hgroup, getparam("new_group"), sizeof(hgroup));

      /* create new panel */
      sprintf(str, "/History/Display/%s/%s", hgroup, panel);
      db_create_key(hDB, 0, str, TID_KEY);
      db_find_key(hDB, 0, str, &hkey);
      assert(hkey);
      db_set_value(hDB, hkey, "Timescale", "1h", NAME_LENGTH, 1, TID_STRING);
      i = 1;
      db_set_value(hDB, hkey, "Zero ylow", &i, sizeof(BOOL), 1, TID_BOOL);
      db_set_value(hDB, hkey, "Show run markers", &i, sizeof(BOOL), 1, TID_BOOL);
      i = 0;
      db_set_value(hDB, hkey, "Show values", &i, sizeof(BOOL), 1, TID_BOOL);
      i = 0;
      db_set_value(hDB, hkey, "Sort Vars", &i, sizeof(BOOL), 1, TID_BOOL);
      i = 0;
      db_set_value(hDB, hkey, "Log axis", &i, sizeof(BOOL), 1, TID_BOOL);

      /* configure that panel */
      show_hist_config_page(path, hgroup, panel);
      return;
   }

   const char* pscale = getparam("scale");
   if (pscale == NULL || *pscale == 0)
      pscale = getparam("hscale");
   const char* poffset = getparam("offset");
   if (poffset == NULL || *poffset == 0)
      poffset = getparam("hoffset");
   const char* pmag = getparam("width");
   if (pmag == NULL || *pmag == 0)
      pmag = getparam("hwidth");
   const char* pindex = getparam("index");
   if (pindex == NULL || *pindex == 0)
      pindex = getparam("hindex");

   labels = 1;
   if (*getparam("labels") && atoi(getparam("labels")) == 0)
      labels = 0;

   if (*getparam("bgcolor"))
      strlcpy(bgcolor, getparam("bgcolor"), sizeof(bgcolor));
   else
      strcpy(bgcolor, "FFFFFF");

   if (*getparam("fgcolor"))
      strlcpy(fgcolor, getparam("fgcolor"), sizeof(fgcolor));
   else
      strcpy(fgcolor, "000000");

   if (*getparam("gcolor"))
      strlcpy(gridcolor, getparam("gcolor"), sizeof(gridcolor));
   else
      strcpy(gridcolor, "A0A0A0");

   /* evaluate scale and offset */

   if (poffset && *poffset)
      offset = time_to_sec(poffset);
   else
      offset = 0;

   if (pscale && *pscale)
      scale = time_to_sec(pscale);
   else
      scale = 0;

   index = -1;
   if (pindex && *pindex)
      index = atoi(pindex);

   /* use contents of "/History/URL" to access history images (*images only*)
    * otherwise, use relative addresses from "back_path" */

   size = sizeof(hurl);
   status = db_get_value(hDB, 0, "/History/URL", hurl, &size, TID_STRING, FALSE);
   if (status != DB_SUCCESS)
      strlcpy(hurl, back_path, sizeof(hurl));

   if (equal_ustring(getparam("cmd"), "Create ELog")) {
      size = sizeof(url);
      if (db_get_value(hDB, 0, "/Elog/URL", url, &size, TID_STRING, FALSE) == DB_SUCCESS) {

         get_elog_url(url, sizeof(url));

         /*---- use external ELOG ----*/
         fsize = 100000;
         char* fbuffer = (char*)M_MALLOC(fsize);
         assert(fbuffer != NULL);

         if (equal_ustring(pmag, "Large"))
            generate_hist_graph(path, fbuffer, &fsize, 1024, 768, scale, offset, index,
                              labels, bgcolor, fgcolor, gridcolor);
         else if (equal_ustring(pmag, "Small"))
            generate_hist_graph(path, fbuffer, &fsize, 320, 200, scale, offset, index,
                              labels, bgcolor, fgcolor, gridcolor);
         else if (atoi(pmag) > 0)
            generate_hist_graph(path, fbuffer, &fsize, atoi(pmag), 200, scale, offset, index,
                              labels, bgcolor, fgcolor, gridcolor);
         else
            generate_hist_graph(path, fbuffer, &fsize, 640, 400, scale, offset, index,
                              labels, bgcolor, fgcolor, gridcolor);

         /* save temporary file */
         size = sizeof(dir);
         dir[0] = 0;
         db_get_value(hDB, 0, "/Elog/Logbook Dir", dir, &size, TID_STRING, TRUE);
         if (strlen(dir) > 0 && dir[strlen(dir)-1] != DIR_SEPARATOR)
            strlcat(dir, DIR_SEPARATOR_STR, sizeof(dir));

         time(&now);
         tms = localtime(&now);

         if (strchr(path, '/'))
            strlcpy(str, strchr(path, '/') + 1, sizeof(str));
         else
            strlcpy(str, path, sizeof(str));
         sprintf(file_name, "%02d%02d%02d_%02d%02d%02d_%s.gif",
                  tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday,
                  tms->tm_hour, tms->tm_min, tms->tm_sec, str);
         sprintf(str, "%s%s", dir, file_name);

         /* save attachment */
         fh = open(str, O_CREAT | O_RDWR | O_BINARY, 0644);
         if (fh < 0) {
            cm_msg(MERROR, "show_hist_page", "Cannot write attachment file \"%s\"",
                     str);
         } else {
            write(fh, fbuffer, fsize);
            close(fh);
         }

         /* redirect to ELOG */
         if (strlen(url) > 1 && url[strlen(url)-1] != '/')
            strlcat(url, "/", sizeof(url));
         strlcat(url, "?cmd=New&fa=", sizeof(url));
         strlcat(url, file_name, sizeof(url));
         redirect(url);

         M_FREE(fbuffer);
         return;

      } else {
         /*---- use internal ELOG ----*/
         sprintf(str, "\\HS\\%s.gif", path);
         if (getparam("hscale") && *getparam("hscale"))
            sprintf(str + strlen(str), "?scale=%s", getparam("hscale"));
         if (getparam("hoffset") && *getparam("hoffset")) {
            if (strchr(str, '?'))
               strlcat(str, "&", sizeof(str));
            else
               strlcat(str, "?", sizeof(str));
            sprintf(str + strlen(str), "offset=%s", getparam("hoffset"));
         }
         if (getparam("hwidth") && *getparam("hwidth")) {
            if (strchr(str, '?'))
               strlcat(str, "&", sizeof(str));
            else
               strlcat(str, "?", sizeof(str));
            sprintf(str + strlen(str), "width=%s", getparam("hwidth"));
         }
         if (getparam("hindex") && *getparam("hindex")) {
            if (strchr(str, '?'))
               strlcat(str, "&", sizeof(str));
            else
               strlcat(str, "?", sizeof(str));
            sprintf(str + strlen(str), "index=%s", getparam("hindex"));
         }

         show_elog_new(NULL, FALSE, str, "../../EL/");
         return;
      }
   }

   if (equal_ustring(getparam("cmd"), "Export")) {
      export_hist(path, scale, offset, index, labels);
      return;
   }

   if (strstr(path, ".gif")) {
      if (equal_ustring(pmag, "Large"))
         generate_hist_graph(path, buffer, buffer_size, 1024, 768, scale, offset, index,
                             labels, bgcolor, fgcolor, gridcolor);
      else if (equal_ustring(pmag, "Small"))
         generate_hist_graph(path, buffer, buffer_size, 320, 200, scale, offset, index,
                             labels, bgcolor, fgcolor, gridcolor);
      else if (atoi(pmag) > 0)
         generate_hist_graph(path, buffer, buffer_size, atoi(pmag),
                             (int) (atoi(pmag) * 0.625), scale, offset, index, labels,
                             bgcolor, fgcolor, gridcolor);
      else
         generate_hist_graph(path, buffer, buffer_size, 640, 400, scale, offset, index,
                             labels, bgcolor, fgcolor, gridcolor);

      return;
   }

   if (history_mode && index < 0)
      return;

   /* evaluate offset shift */
   if (equal_ustring(getparam("shift"), "<"))
      offset -= scale / 2;

   if (equal_ustring(getparam("shift"), ">")) {
      offset += scale / 2;
      if (offset > 0)
         offset = 0;
   }
   if (equal_ustring(getparam("shift"), ">>"))
      offset = 0;

   if (equal_ustring(getparam("shift"), " + ")) {
      offset -= scale / 4;
      scale /= 2;
   }

   if (equal_ustring(getparam("shift"), " - ")) {
      offset += scale / 2;
      if (offset > 0)
         offset = 0;
      scale *= 2;
   }

   strlcpy(str, path, sizeof(str));
   if (strrchr(str, '/'))
      strlcpy(str, strrchr(str, '/')+1, sizeof(str));
   show_header(hDB, str, "GET", str, 1, offset == 0 ? refresh : 0);

   /* menu buttons */
   rsprintf("<tr><td colspan=2 bgcolor=\"#C0C0C0\">\n");
   rsprintf("<input type=submit name=cmd value=ODB>\n");
   rsprintf("<input type=submit name=cmd value=Alarms>\n");
   rsprintf("<input type=submit name=cmd value=Status>\n");
   rsprintf("<input type=submit name=cmd value=History>\n");

   /* check if panel exists */
   sprintf(str, "/History/Display/%s", path);
   status = db_find_key(hDB, 0, str, &hkey);
   if (status != DB_SUCCESS && !equal_ustring(path, "All") && !equal_ustring(path,"")) {
      rsprintf("<h1>Error: History panel \"%s\" does not exist</h1>\n", path);
      rsprintf("</table></form></body></html>\r\n");
      return;
   }

   /* define hidden field for parameters */
   if (pscale && *pscale)
      rsprintf("<input type=hidden name=hscale value=%d>\n", scale);
   else {
      /* if no scale and offset given, get it from default */
      if (path[0] && !equal_ustring(path, "All") && strchr(path, '/') != NULL) {
         sprintf(str, "/History/Display/%s/Timescale", path);

         strcpy(scalestr, "1h");
         size = NAME_LENGTH;
         status = db_get_value(hDB, 0, str, scalestr, &size, TID_STRING, TRUE);
         if (status != DB_SUCCESS) {
            /* delete old integer key */
            db_find_key(hDB, 0, str, &hkey);
            if (hkey)
               db_delete_key(hDB, hkey, FALSE);

            strcpy(scalestr, "1h");
            size = NAME_LENGTH;
            db_get_value(hDB, 0, str, scalestr, &size, TID_STRING, TRUE);
         }

         rsprintf("<input type=hidden name=hscale value=%s>\n", scalestr);
         scale = time_to_sec(scalestr);
      }
   }

   if (offset != 0)
      rsprintf("<input type=hidden name=hoffset value=%d>\n", offset);
   if (pmag && *pmag)
      rsprintf("<input type=hidden name=hwidth value=%s>\n", pmag);
   if (pindex && *pindex)
      rsprintf("<input type=hidden name=hindex value=%s>\n", pindex);

   rsprintf("</td></tr>\n");

   if (path[0] == 0) {
      /* show big selection page */

      /* links for history panels */
      rsprintf("<tr><td colspan=2 bgcolor=\"#FFFFA0\">\n");
      if (!path[0])
         rsprintf("<b>Please select panel:</b><br>\n");

      /* table for panel selection */
      rsprintf("<table border=1 cellpadding=3 style='text-align: left;'>");

      /* "All" link */
      rsprintf("<tr><td colspan=2>\n");
      if (equal_ustring(path, "All"))
         rsprintf("<b>All</b> &nbsp;&nbsp;");
      else
         rsprintf("<a href=\"%sAll\">ALL</a>\n", back_path);
      rsprintf("</td></tr>\n");

      /* Setup History table links */
      db_find_key(hDB, 0, "/History/Display", &hkey);
      if (!hkey) {
         /* create default panel */
         strcpy(str, "System:Trigger per sec.");
         strcpy(str + 2 * NAME_LENGTH, "System:Trigger kB per sec.");
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Variables",
                      str, NAME_LENGTH * 4, 2, TID_STRING);
         strcpy(str, "1h");
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Time Scale",
                      str, NAME_LENGTH, 1, TID_STRING);

         factor[0] = 1;
         factor[1] = 1;
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Factor",
                      factor, 2 * sizeof(float), 2, TID_FLOAT);
         factor[0] = 0;
         factor[1] = 0;
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Offset",
                      factor, 2 * sizeof(float), 2, TID_FLOAT);
         strcpy(str, "1h");
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Timescale",
                      str, NAME_LENGTH, 1, TID_STRING);
         i = 1;
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Zero ylow", &i,
                      sizeof(BOOL), 1, TID_BOOL);
         i = 1;
         db_set_value(hDB, 0, "/History/Display/Default/Trigger rate/Show run markers",
                      &i, sizeof(BOOL), 1, TID_BOOL);
      }

      db_find_key(hDB, 0, "/History/Display", &hkey);
      if (hkey) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkey, i, &hkeyp);

            if (!hkeyp)
               break;

            // Group key
            db_get_key(hDB, hkeyp, &key);

            if (strchr(path, '/'))
               strlcpy(str, strchr(path, '/') + 1, sizeof(str));
            else
               strlcpy(str, path, sizeof(str));

            if (equal_ustring(str, key.name))
               rsprintf("<tr><td><b>%s</b></td>\n<td>", key.name);
            else
               rsprintf("<tr><td><b><a href=\"%s%s\">%s</a></b></td>\n<td>",
                         back_path, key.name, key.name);

            for (j = 0;; j++) {
               // scan items 
               db_enum_link(hDB, hkeyp, j, &hikeyp);

               if (!hikeyp) {
                  rsprintf("</tr>");
                  break;
               }
               // Item key
               db_get_key(hDB, hikeyp, &ikey);

               if (strchr(path, '/'))
                  strlcpy(str, strchr(path, '/') + 1, sizeof(str));
               else
                  strlcpy(str, path, sizeof(str));

               if (equal_ustring(str, ikey.name))
                  rsprintf("<small><b>%s</b></small> &nbsp;", ikey.name);
               else
                  rsprintf("<small><a href=\"%s%s/%s\">%s</a></small> &nbsp;\n",
                              back_path, key.name, ikey.name, ikey.name);
            }
         }
      }

      /* "New" button */
      rsprintf("<tr><td colspan=2><input type=submit name=cmd value=New></td></tr>\n");
      rsprintf("</table></tr>\n");

   } else {
      int found = 0;

      /* show drop-down selectors */
      rsprintf("<tr><td colspan=2 bgcolor=\"#FFFFA0\">\n");

      rsprintf("Group:\n");

      rsprintf("<select title=\"Select group\" name=\"fgroup\" onChange=\"document.form1.submit()\">\n");

      db_find_key(hDB, 0, "/History/Display", &hkey);
      if (hkey) {
         hkeyp = 0;
         for (i = 0;; i++) {
            db_enum_link(hDB, hkey, i, &hikeyp);

            if (!hikeyp)
               break;

            if (i == 0)
               hkeyp = hikeyp;

            // Group key
            db_get_key(hDB, hikeyp, &key);

            if (strchr(path, '/')) {
               strlcpy(panel, strchr(path, '/') + 1, sizeof(panel));
               strlcpy(hgroup, path, sizeof(hgroup));
               *strchr(hgroup, '/') = 0;
            } else {
               strlcpy(hgroup, path, sizeof(str));
               panel[0] = 0;
            }

            if (equal_ustring(key.name, hgroup)) {
               rsprintf("<option selected value=\"%s\">%s\n", key.name, key.name);
               hkeyp = hikeyp;
            } else
               rsprintf("<option value=\"%s\">%s\n", key.name, key.name);
         }

         rsprintf("</select>\n");
         rsprintf("&nbsp;&nbsp;Panel:\n");
         rsprintf("<select title=\"Select panel\" name=\"fpanel\" onChange=\"document.form1.submit()\">\n");

         found = 0;
         if (hkeyp) {
            for (i = 0;; i++) {
               // scan panels
               db_enum_link(hDB, hkeyp, i, &hikeyp);

               if (!hikeyp)
                  break;

               // Item key
               db_get_key(hDB, hikeyp, &key);

               if (strchr(path, '/'))
                  strlcpy(str, strchr(path, '/') + 1, sizeof(str));
               else
                  strlcpy(str, path, sizeof(str));

               if (equal_ustring(str, key.name)) {
                  rsprintf("<option selected value=\"%s\">%s\n", key.name, key.name);
                  found = 1;
               } else
                  rsprintf("<option value=\"%s\">%s\n", key.name, key.name);
            }
         }

         if (found) 
            rsprintf("<option value=\"\">- all -\n");
         else
            rsprintf("<option selected value=\"\">- all -\n");

         rsprintf("</select>\n");
      }

      rsprintf("<noscript>\n");
      rsprintf("<input type=submit value=\"Go\">\n");
      rsprintf("</noscript>\n");

      rsprintf("&nbsp;&nbsp;<input type=\"button\" name=\"New\" value=\"New\" ");

      if (found)
         rsprintf("onClick=\"window.location.href='../?cmd=New'\">\n");
      else
         rsprintf("onClick=\"window.location.href='?cmd=New'\">\n");

      rsprintf("</td></tr>\n");
   }


   /* check if whole group should be displayed */
   if (path[0] && !equal_ustring(path, "ALL") && strchr(path, '/') == NULL) {
      strcpy(strwidth, "Small");
      size = sizeof(strwidth);
      db_get_value(hDB, 0, "/History/Display Settings/Width Gropup", strwidth, &size, TID_STRING, TRUE);
      
      sprintf(str, "/History/Display/%s", path);
      db_find_key(hDB, 0, str, &hkey);
      if (hkey) {
         for (i = 0 ;; i++) {     // scan group
            db_enum_link(hDB, hkey, i, &hikeyp);

            if (!hikeyp)
               break;

            db_get_key(hDB, hikeyp, &key);
            sprintf(ref, "%s%s/%s.gif?width=%s", hurl, path, key.name, strwidth);
            sprintf(ref2, "%s/%s", path, key.name);

            if (i % 2 == 0)
               rsprintf("<tr><td><a href=\"%s%s\"><img src=\"%s\" alt=\"%s.gif\"></a>\n",
                        back_path, ref2, ref, key.name);
            else
               rsprintf("<td><a href=\"%s%s\"><img src=\"%s\" alt=\"%s.gif\"></a></tr>\n",
                        back_path, ref2, ref, key.name);
         }

      } else {
         rsprintf("Group \"%s\" not found", path);
      }
   }

   /* image panel */
   else if (path[0] && !equal_ustring(path, "All")) {
      /* navigation links */
      rsprintf("<tr><td bgcolor=\"#A0FFA0\">\n");

      sprintf(str, "/History/Display/%s/Buttons", path);
      db_find_key(hDB, 0, str, &hkeybutton);
      if (hkeybutton == 0) {
         /* create default buttons */
         db_create_key(hDB, 0, str, TID_STRING);
         db_find_key(hDB, 0, str, &hkeybutton);
	 assert(hkeybutton);
         db_set_data(hDB, hkeybutton, def_button, sizeof(def_button), 7, TID_STRING);
      }

      db_get_key(hDB, hkeybutton, &key);

      for (i = 0; i < key.num_values; i++) {
         size = sizeof(str);
         db_get_data_index(hDB, hkeybutton, str, &size, i, TID_STRING);
         rsprintf("<input type=submit name=scale value=%s>\n", str);
      }

      rsprintf("<input type=submit name=shift value=\"<\">\n");
      rsprintf("<input type=submit name=shift value=\" + \">\n");
      rsprintf("<input type=submit name=shift value=\" - \">\n");
      if (offset != 0) {
         rsprintf("<input type=submit name=shift value=\">\">\n");
         rsprintf("<input type=submit name=shift value=\">>\">\n");
      }

      rsprintf("<td bgcolor=\"#A0FFA0\">\n");
      rsprintf("<input type=submit name=width value=Large>\n");
      rsprintf("<input type=submit name=width value=Small>\n");
      rsprintf("<input type=submit name=cmd value=\"Create ELog\">\n");
      rsprintf("<input type=submit name=cmd value=Config>\n");
      rsprintf("<input type=submit name=cmd value=Export>\n");
      rsprintf("<input type=submit name=cmd value=Query>\n");

      rsprintf("</tr>\n");

      paramstr[0] = 0;
      sprintf(paramstr + strlen(paramstr), "&scale=%d", scale);
      if (offset != 0)
         sprintf(paramstr + strlen(paramstr), "&offset=%d", offset);
      if (pmag && *pmag)
         sprintf(paramstr + strlen(paramstr), "&width=%s", pmag);
      else {
         strlcpy(str, "640", sizeof(str));
         size = sizeof(str);
         db_get_value(hDB, 0, "/History/Display Settings/Width Individual", str, &size, TID_STRING, TRUE);
         sprintf(paramstr + strlen(paramstr), "&width=%s", str);
      }

      /* define image map */
      rsprintf("<map name=\"%s\">\r\n", path);

      if (!(pindex && *pindex)) {
         sprintf(str, "/History/Display/%s/Variables", path);
         db_find_key(hDB, 0, str, &hkey);
         if (hkey) {
            db_get_key(hDB, hkey, &key);

            for (i = 0; i < key.num_values; i++) {
               if (paramstr[0])
                  sprintf(ref, "%s?%s&index=%d", path, paramstr, i);
               else
                  sprintf(ref, "%s?index=%d", path, i);

               rsprintf("  <area shape=rect coords=\"%d,%d,%d,%d\" href=\"%s%s\">\r\n",
                        30, 31 + 23 * i, 150, 30 + 23 * i + 17, back_path, ref);
            }
         }
      } else {
         if (paramstr[0])
            sprintf(ref, "%s?%s", path, paramstr);
         else
            sprintf(ref, "%s", path);

         if (equal_ustring(pmag, "Large"))
            width = 1024;
         else if (equal_ustring(pmag, "Small"))
            width = 320;
         else if (atoi(pmag) > 0)
            width = atoi(pmag);
         else
            width = 640;

         rsprintf("  <area shape=rect coords=\"%d,%d,%d,%d\" href=\"%s%s\">\r\n", 0, 0,
                  width, 20, back_path, ref);
      }

      rsprintf("</map>\r\n");

      /* Display individual panels */
      if (pindex && *pindex)
         sprintf(paramstr + strlen(paramstr), "&index=%s", pindex);
      if (paramstr[0])
         sprintf(ref, "%s%s.gif?%s", hurl, path, paramstr);
      else
         sprintf(ref, "%s%s.gif", hurl, path);

      /* put reference to graph */
      rsprintf("<tr><td colspan=2><img src=\"%s\" alt=\"%s.gif\" usemap=\"#%s\"></tr>\n",
               ref, path, path);
   }

   else if (equal_ustring(path, "All")) {
      /* Display all panels */
      db_find_key(hDB, 0, "/History/Display", &hkey);
      if (hkey)
         for (i = 0, k = 0;; i++) {     // scan Groups
            db_enum_link(hDB, hkey, i, &hkeyp);

            if (!hkeyp)
               break;

            db_get_key(hDB, hkeyp, &key);

            for (j = 0;; j++, k++) {
               // scan items 
               db_enum_link(hDB, hkeyp, j, &hikeyp);

               if (!hikeyp)
                  break;

               db_get_key(hDB, hikeyp, &ikey);
               sprintf(ref, "%s%s/%s.gif?width=Small", hurl, key.name, ikey.name);
               sprintf(ref2, "%s/%s", key.name, ikey.name);

               if (k % 2 == 0)
                  rsprintf("<tr><td><a href=\"%s%s\"><img src=\"%s\" alt=\"%s.gif\"></a>\n",
                           back_path, ref2, ref, key.name);
               else
                  rsprintf("<td><a href=\"%s%s\"><img src=\"%s\" alt=\"%s.gif\"></a></tr>\n",
                       back_path, ref2, ref, key.name);
            }                   // items loop
         }                      // Groups loop
   }                            // All
   rsprintf("</table>\r\n");
   rsprintf("</form></body></html>\r\n");
}


/*------------------------------------------------------------------*/

void get_password(char *password)
{
   static char last_password[32];

   if (strncmp(password, "set=", 4) == 0)
      strlcpy(last_password, password + 4, sizeof(last_password));
   else
      strcpy(password, last_password);  // unsafe: do not know size of password string, has to be this way because of cm_connect_experiment() KO 27-Jul-2006
}

/*------------------------------------------------------------------*/

void send_icon(const char *icon)
{
   int length;
   const unsigned char *picon;
   char str[256], format[256];
   time_t now;
   struct tm *gmt;

   if (strstr(icon, "favicon.ico") != 0) {
      length = sizeof(favicon_ico);
      picon = favicon_ico;
   } else if (strstr(icon, "favicon.png") != 0) {
      length = sizeof(favicon_png);
      picon = favicon_png;
   } else if (strstr(icon, "reload.png") != 0){
      length = sizeof(reload_png);
      picon = reload_png;
   } else
      return;

   rsprintf("HTTP/1.1 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Accept-Ranges: bytes\r\n");

   /* set expiration time to one day */
   time(&now);
   now += (int) (3600 * 24);
   gmt = gmtime(&now);
   strcpy(format, "%A, %d-%b-%y %H:%M:%S GMT");
   strftime(str, sizeof(str), format, gmt);
   rsprintf("Expires: %s\r\n", str);

   if (equal_ustring(icon, "favicon.ico"))
      rsprintf("Content-Type: image/x-icon\r\n");
   else
      rsprintf("Content-Type: image/png\r\n");

   rsprintf("Content-Length: %d\r\n\r\n", length);

   return_length = strlen(return_buffer) + length;
   memcpy(return_buffer + strlen(return_buffer), picon, length);
}

/*------------------------------------------------------------------*/

char mhttpd_css[] = "body {\
  margin:3px;\
  color:black;\
  background-color:white;\
  font-family:verdana,tahoma,sans-serif;\
}\
\
/* standard link colors and decorations */\
a:link { color:#0000FF; text-decoration:none }\
a:visited { color:#800080; text-decoration:none }\
a:hover { color:#0000FF; text-decoration:underline }\
a:active { color:#0000FF; text-decoration:underline }\
a:focus { color:#0000FF; text-decoration:underline }\
";


void send_css()
{
   int length;
   char str[256], format[256];
   time_t now;
   struct tm *gmt;

   rsprintf("HTTP/1.1 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Accept-Ranges: bytes\r\n");

   /* set expiration time to one day */
   time(&now);
   now += (int) (3600 * 24);
   gmt = gmtime(&now);
   strcpy(format, "%A, %d-%b-%y %H:%M:%S GMT");
   strftime(str, sizeof(str), format, gmt);
   rsprintf("Expires: %s\r\n", str);
   rsprintf("Content-Type: text/css\r\n");

   length = strlen(mhttpd_css);
   rsprintf("Content-Length: %d\r\n\r\n", length);

   return_length = strlen(return_buffer) + length;
   memcpy(return_buffer + strlen(return_buffer), mhttpd_css, length);
}

/*------------------------------------------------------------------*/

const char *mhttpd_js = 
"\n"
"document.onmousemove = getMouseXY;\n"
"\n"
"function getMouseXY(e)\n"
"{\n"
"   try {\n"
"      var x = e.pageX;\n"
"      var y = e.pageY;\n"
"      var p = 'abs: ' + x + '/' + y;\n"
"      i = document.getElementById('refimg');\n"
"      if (i == null)\n"
"         return false;\n"
"      document.body.style.cursor = 'crosshair';\n"
"      x -= i.offsetLeft;\n"
"      y -= i.offsetTop;\n"
"      while (i = i.offsetParent) {\n"
"         x -= i.offsetLeft;\n"
"         y -= i.offsetTop;\n"
"      }\n"
"      p += '   rel: ' + x + '/' + y;\n"
"      window.status = p;\n"
"      return true;\n"
"      }\n"
"   catch (e) {\n"
"      return false;\n"
"   }\n"
"}\n"
"\n"
"function XMLHttpRequestGeneric()\n"
"{\n"
"   var request;\n"
"   try {\n"
"      request = new XMLHttpRequest(); // Firefox, Opera 8.0+, Safari\n"
"   }\n"
"   catch (e) {\n"
"      try {\n"
"         request = new ActiveXObject('Msxml2.XMLHTTP'); // Internet Explorer\n"
"      }\n"
"      catch (e) {\n"
"         try {\n"
"            request = new ActiveXObject('Microsoft.XMLHTTP');\n"
"         }\n"
"         catch (e) {\n"
"           alert('Your browser does not support AJAX!');\n"
"           return undefined;\n"
"         }\n"
"      }\n"
"   }\n"
"\n"
"   return request;\n"
"}\n"
"\n"
"function ODBSet(path, value, pwdname)\n"
"{\n"
"   var value, request, url;\n"
"\n"
"   if (pwdname != undefined)\n"
"      pwd = prompt('Please enter password', '');\n"
"   else\n"
"      pwd = '';\n"
"\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   url = '?cmd=jset&odb=' + path + '&value=' + value;\n"
"\n"
"   if (pwdname != undefined)\n"
"      url += '&pnam=' + pwdname;\n"
"\n"
"   request.open('GET', url, false);\n"
"\n"
"   if (pwdname != undefined)\n"
"      request.setRequestHeader('Cookie', 'cpwd='+pwd);\n"
"\n"
"   request.send(null);\n"
"\n"
"   if (request.status != 200 || request.responseText != 'OK') \n"
"      alert('ODBSet error:\\nPath: '+path+'\\nHTTP Status: '+request.status+'\\nMessage: '+request.responseText+'\\n'+document.location) ;\n"
"}\n"
"\n"
"function ODBGet(path, format, defval, len, type)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jget&odb=' + path;\n"
"   if (format != undefined && format != '')\n"
"      url += '&format=' + format;\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"\n"
"   if (path.match(/[*]/)) {\n"
"      if (request.responseText == null)\n"
"         return null;\n"
"      if (request.responseText == '<DB_NO_KEY>') {\n"
"         url = '?cmd=jset&odb=' + path + '&value=' + defval + '&len=' + len + '&type=' + type;\n"
"\n"
"         request.open('GET', url, false);\n"
"         request.send(null);\n"
"         return defval;\n"
"      } else {\n"
"         var array = request.responseText.split('\\n');\n"
"         return array;\n"
"      }\n"
"   } else {\n"
"      if ((request.responseText == '<DB_NO_KEY>' ||\n"
"           request.responseText == '<DB_OUT_OF_RANGE>') && defval != undefined) {\n"
"         url = '?cmd=jset&odb=' + path + '&value=' + defval + '&len=' + len + '&type=' + type;\n"
"\n"
"         request.open('GET', url, false);\n"
"         request.send(null);\n"
"         return defval;\n"
"      }\n"
"      return request.responseText.split('\\n')[0];\n"
"   }\n"
"}\n"
"\n"
"function ODBMGet(paths, callback, formats)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"   
"   var url = '?cmd=jget';\n"
"   for (var i=0 ; i<paths.length ; i++) {\n"
"      url += '&odb'+i+'='+paths[i];\n"
"      if (formats != undefined && formats != '')\n"
"         url += '&format'+i+'=' + formats[i];\n"
"   }\n"
"\n"   
"   if (callback != undefined) {\n"
"      request.onreadystatechange = function() \n"
"         {\n"
"         if (request.readyState == 4) {\n"
"            if (request.status == 200) {\n"
"               var array = request.responseText.split('$#----#$\\n');\n"
"               for (var i=0 ; i<array.length ; i++)\n"
"                  if (paths[i].match(/[*]/)) {\n"
"                     array[i] = array[i].split('\\n');\n"
"                     array[i].length--;\n"
"                  } else\n"
"                     array[i] = array[i].split('\\n')[0];\n"
"               callback(array);\n"
"            }\n"
"         }\n"
"      }\n"
"      request.open('GET', url, true);\n"
"   } else\n"
"      request.open('GET', url, false);\n"
"   request.send(null);\n"
"\n"   
"   if (callback == undefined) {\n"
"      var array = request.responseText.split('$#----#$\\n');\n"
"      for (var i=0 ; i<array.length ; i++) {\n"
"         if (paths[i].match(/[*]/)) {\n"
"            array[i] = array[i].split('\\n');\n"
"            array[i].length--;\n"
"         } else\n"
"            array[i] = array[i].split('\\n')[0];\n"
"      }\n"
"      return array;\n"
"   }\n"
"}\n"
"\n"
"function ODBGetRecord(path)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jget&odb=' + path + '&name=1';\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"   return request.responseText;\n"
"}\n"
"\n"
"function ODBExtractRecord(record, key)\n"
"{\n"
"   var array = record.split('\\n');\n"
"   for (var i=0 ; i<array.length ; i++) {\n"
"      var ind = array[i].indexOf(':');\n"
"      if (ind > 0) {\n"
"         var k = array[i].substr(0, ind);\n"
"         if (k == key)\n"
"            return array[i].substr(ind+1, array[i].length);\n"
"      }\n"
"      var ind = array[i].indexOf('[');\n"
"      if (ind > 0) {\n"
"         var k = array[i].substr(0, ind);\n"
"         if (k == key) {\n"
"            var a = new Array();\n"
"            for (var j=0 ; ; j++,i++) {\n"
"               if (array[i].substr(0, ind) != key)\n"
"                  break;\n"
"               var k = array[i].indexOf(':');\n"
"               a[j] = array[i].substr(k+1, array[i].length);\n"
"            }\n"
"            return a;\n"
"         }\n"
"      }\n"
"   }\n"
"   return null;\n"
"}\n"
"\n"
"function ODBKey(path)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jkey&odb=' + path;\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"   if (request.responseText == null)\n"
"      return null;\n"
"   var res = request.responseText.split('\\n');\n"
"   this.name = res[0];\n"
"   this.type = res[1];\n"
"   this.num_values = res[2];\n"
"   this.item_size = res[3];\n"
"   this.last_written = res[4];\n"
"}\n"
"\n"
"function ODBCopy(path, format)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jcopy&odb=' + path;\n"
"   if (format != undefined && format != '')\n"
"      url += '&format=' + format;\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"   return request.responseText;\n"
"}\n"
"\n"
"function ODBRpc_rev0(name, rpc, args)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jrpc_rev0&name=' + name + '&rpc=' + rpc;\n"
"   for (var i = 2; i < arguments.length; i++) {\n"
"     url += '&arg'+(i-2)+'='+arguments[i];\n"
"   };\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"   if (request.responseText == null)\n"
"      return null;\n"
"   this.reply = request.responseText.split('\\n');\n"
"}\n"
"\n"
"function ODBRpc_rev1(name, rpc, max_reply_length, args)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jrpc_rev1&name=' + name + '&rpc=' + rpc + '&max_reply_length=' + max_reply_length;\n"
"   for (var i = 3; i < arguments.length; i++) {\n"
"     url += '&arg'+(i-3)+'='+arguments[i];\n"
"   };\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"   if (request.responseText == null)\n"
"      return null;\n"
"   return request.responseText;\n"
"}\n"
"\n"
"function ODBGetMsg(n)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jmsg&n=' + n;\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"\n"
"   if (n > 1) {\n"
"      var array = request.responseText.split('\\n');\n"
"      return array;\n"
"   } else\n"
"      return request.responseText;\n"
"}\n"
"\n"
"function ODBGenerateMsg(m)\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"\n"
"   var url = '?cmd=jgenmsg&msg=' + m;\n"
"   request.open('GET', url, false);\n"
"   request.send(null);\n"
"   return request.responseText;\n"
"}\n"
"\n"
"function ODBGetAlarms()\n"
"{\n"
"   var request = XMLHttpRequestGeneric();\n"
"   request.open('GET', '?cmd=jalm', false);\n"
"   request.send(null);\n"
"   var a = request.responseText.split('\\n');\n"
"   a.length = a.length-1;\n"
"   return a;\n"
"}\n"
"\n"
"function ODBEdit(path)\n"
"{\n"
"   var value = ODBGet(path);\n"
"   var new_value = prompt('Please enter new value', value);\n"
"   if (new_value != undefined) {\n"
"      ODBSet(path, new_value);\n"
"      window.location.reload();\n"
"   }\n"
"}\n"
"";

void send_js()
{
   int i, length;
   char str[256], format[256], *buf;
   time_t now;
   struct tm *gmt;

   rsprintf("HTTP/1.1 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Accept-Ranges: bytes\r\n");

   /* set expiration time to one day */
   time(&now);
   now += (int) (3600 * 24);
   gmt = gmtime(&now);
   strcpy(format, "%A, %d-%b-%y %H:%M:%S GMT");
   strftime(str, sizeof(str), format, gmt);
   rsprintf("Expires: %s\r\n", str);
   rsprintf("Content-Type: text/javascript\r\n");

   buf = (char*)malloc(strlen(mhttpd_js)+10000);
   strcpy(buf, mhttpd_js);
   strcat(buf, "\n/* MIDAS type definitions */\n");
   for (i=1 ; i<TID_LAST ; i++) {
      sprintf(str, "var TID_%s = %d;\n", rpc_tid_name(i), i);
      strcat(buf, str);
   }

   length = strlen(buf);
   rsprintf("Content-Length: %d\r\n\r\n", length);

   return_length = strlen(return_buffer) + length;
   memcpy(return_buffer + strlen(return_buffer), buf, length);

   free(buf);
}

/*------------------------------------------------------------------*/

// 0x27: handclap
// 0x39: crash
// 0x51: open triangle

unsigned char alarm_sound[] = {
0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06,0x00,0x01,
0x00,0x01,0x01,0xE0,0x4D,0x54,0x72,0x6B,0x00,0x00,
0x00,0x2B,0x00,0xFF,0x03,0x07,0x44,0x72,0x75,0x6D,
0x6B,0x69,0x74,0x00,0xC9,0x00,0x00,0xFF,0x51,0x03,
0x07,0xA1,0x20,0x00,0xFF,0x58,0x04,0x04,0x02,0x18,
0x08,0x87,0x40,0x99,0x51,0x48,0x96,0x40,0x89,0x51,  // <-- 4 & 9
0x40,0x00,0xFF,0x2F,0x00,
};

void send_alarm_sound()
{
   rsprintf("HTTP/1.1 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Accept-Ranges: bytes\r\n");
   rsprintf("Content-Type: audio/midi\r\n");
   rsprintf("Content-Length: %d\r\n\r\n", sizeof(alarm_sound));

   memcpy(return_buffer + strlen_retbuf, alarm_sound, sizeof(alarm_sound));
   return_length = strlen_retbuf + sizeof(alarm_sound);
}

/*------------------------------------------------------------------*/

void interprete(const char *cookie_pwd, const char *cookie_wpwd, const char *cookie_cpwd, const char *path, int refresh)
/********************************************************************\

  Routine: interprete

  Purpose: Interprete parametersand generate HTML output from odb.

  Input:
    char *cookie_pwd        Cookie containing encrypted password
    char *path              ODB path "/dir/subdir/key"

  <implicit>
    _param/_value array accessible via getparam()

\********************************************************************/
{
   int i, j, n, status, size, run_state, index;
   WORD event_id;
   HNDLE hkey, hsubkey, hDB, hconn;
   KEY key;
   char *p, str[256];
   char enc_path[256], dec_path[256], eq_name[NAME_LENGTH], fe_name[NAME_LENGTH];
   char data[TEXT_SIZE];
   time_t now;
   struct tm *gmt;

   if (strstr(path, "favicon.ico") != 0 || 
       strstr(path, "favicon.png") != 0 ||
       strstr(path, "reload.png")) {
      send_icon(path);
      return;
   }

   if (strstr(path, "mhttpd.css")) {
      send_css();
      return;
   }

   if (strstr(path, "mhttpd.js")) {
      send_js();
      return;
   }

   /* encode path for further usage */
   strlcpy(dec_path, path, sizeof(dec_path));
   urlDecode(dec_path);
   urlDecode(dec_path); /* necessary for %2520 -> %20 -> ' ', used e.g. in deleting ODB entries with blanks in path */
   strlcpy(enc_path, dec_path, sizeof(enc_path));
   urlEncode(enc_path, sizeof(enc_path));

   const char* experiment = getparam("exp");
   const char* password = getparam("pwd");
   const char* wpassword = getparam("wpwd");
   const char* command = getparam("cmd");
   const char* value = getparam("value");
   const char* group = getparam("group");
   index = atoi(getparam("index"));

   cm_get_experiment_database(&hDB, NULL);

   if (history_mode) {
      if (strncmp(path, "HS/", 3) == 0) {
         if (equal_ustring(command, "config")) {
            return;
         }

         show_hist_page(dec_path + 3, sizeof(dec_path) - 3, NULL, NULL, refresh);
         return;
      }
      
      return;
   }

   /* check for password */
   db_find_key(hDB, 0, "/Experiment/Security/Password", &hkey);
   if (!password[0] && hkey) {
      size = sizeof(str);
      db_get_data(hDB, hkey, str, &size, TID_STRING);

      /* check for excemption */
      db_find_key(hDB, 0, "/Experiment/Security/Allowed programs/mhttpd", &hkey);
      if (hkey == 0 && strcmp(cookie_pwd, str) != 0) {
         show_password_page("", experiment);
         return;
      }
   }

   /* get run state */
   run_state = STATE_STOPPED;
   size = sizeof(run_state);
   db_get_value(hDB, 0, "/Runinfo/State", &run_state, &size, TID_INT, TRUE);

   /*---- redirect with cookie if password given --------------------*/

   if (password[0]) {
      rsprintf("HTTP/1.0 302 Found\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());

      time(&now);
      now += 3600 * 24;
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%Y %H:00:00 GMT", gmt);

      rsprintf("Set-Cookie: midas_pwd=%s; path=/; expires=%s\r\n",
               ss_crypt(password, "mi"), str);

      rsprintf("Location: ./\n\n<html>redir</html>\r\n");
      return;
   }

   if (wpassword[0]) {
      /* check if password correct */
      if (!check_web_password(ss_crypt(wpassword, "mi"), getparam("redir"), experiment))
         return;

      rsprintf("HTTP/1.0 302 Found\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());

      time(&now);
      now += 3600 * 24;
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%Y %H:%M:%S GMT", gmt);

      rsprintf("Set-Cookie: midas_wpwd=%s; path=/; expires=%s\r\n",
               ss_crypt(wpassword, "mi"), str);

      sprintf(str, "./%s", getparam("redir"));
      rsprintf("Location: %s\n\n<html>redir</html>\r\n", str);
      return;
   }

   /*---- redirect if ODB command -----------------------------------*/

   if (equal_ustring(command, "ODB")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      strlcat(str, "root", sizeof(str));
      redirect(str);
      return;
   }

   /*---- send sound file -------------------------------------------*/

   if (equal_ustring(dec_path, "alarm.mid")) {
      send_alarm_sound();
      return;
   }

   /*---- java script commands --------------------------------------*/
   
   if (equal_ustring(command, "jset") ||
       equal_ustring(command, "jget") ||
       equal_ustring(command, "jcopy") ||
       equal_ustring(command, "jpaste") ||
       equal_ustring(command, "jkey") ||
       equal_ustring(command, "jmsg") ||
       equal_ustring(command, "jalm") ||
       equal_ustring(command, "jgenmsg") ||
       equal_ustring(command, "jrpc_rev0") ||
       equal_ustring(command, "jrpc_rev1")) {
      java_script_commands(path, cookie_cpwd);
      return;
   }
   
   /*---- redirect if SC command ------------------------------------*/

   if (equal_ustring(command, "SC")) {
      redirect("SC/");
      return;
   }

   /*---- redirect if status command --------------------------------*/

   if (equal_ustring(command, "status")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      redirect(str);
      return;
   }

   /*---- script command --------------------------------------------*/

   if (getparam("script") && *getparam("script")) {
      sprintf(str, "%s?script=%s", path, getparam("script"));
      if (!check_web_password(cookie_wpwd, str, experiment))
         return;

      sprintf(str, "/Script/%s", getparam("script"));

      db_find_key(hDB, 0, str, &hkey);

      if (hkey) {
         /* for NT: close reply socket before starting subprocess */
         if (isparam("redir"))
           redirect2(getparam("redir"));
         else
           redirect2("");         
         exec_script(hkey);
      } else {
         if (isparam("redir"))
           redirect2(getparam("redir"));
         else
           redirect2("");         
      }

      return;
   }

   /*---- customscript command --------------------------------------*/

   if (getparam("customscript") && *getparam("customscript")) {
      sprintf(str, "%s?customscript=%s", path, getparam("customscript"));
      if (!check_web_password(cookie_wpwd, str, experiment))
         return;

      sprintf(str, "/CustomScript/%s", getparam("customscript"));

      db_find_key(hDB, 0, str, &hkey);

      if (hkey) {
         /* for NT: close reply socket before starting subprocess */
         if (isparam("redir"))
           redirect2(getparam("redir"));
         else
           redirect2("");         
         exec_script(hkey);
      } else {
         if (isparam("redir"))
           redirect(getparam("redir"));
         else
           redirect("");         
      }

      return;
   }

   /*---- alarms command --------------------------------------------*/

   if (equal_ustring(command, "alarms")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      if (str[0]) {
         strlcat(str, "./?cmd=alarms", sizeof(str));
         redirect(str);
         return;
      }
      show_alarm_page();
      return;
   }

   /*---- alarms reset command --------------------------------------*/

   if (equal_ustring(command, "alrst")) {
      if (*getparam("name"))
         al_reset_alarm(getparam("name"));
      redirect("");
      return;
   }

   /*---- history command -------------------------------------------*/

   if (equal_ustring(command, "history")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      strlcat(str, "HS/", sizeof(str));
      redirect(str);
      return;
   }

   if (strncmp(path, "HS/", 3) == 0) {
      if (equal_ustring(command, "config")) {
         sprintf(str, "%s?cmd=%s", path, command);
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;
      }

      show_hist_page(dec_path + 3, sizeof(dec_path) - 3, NULL, NULL, refresh);
      return;
   }

   /*---- MSCB command ----------------------------------------------*/

   if (equal_ustring(command, "MSCB")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      strlcat(str, "MS/", sizeof(str));
      redirect(str);
      return;
   }

   if (strncmp(path, "MS/", 3) == 0) {
      if (equal_ustring(command, "set")) {
         sprintf(str, "%s?cmd=%s", path, command);
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;
      }

#ifdef HAVE_MSCB
      show_mscb_page(dec_path + 3, refresh);
#else
      show_error("MSCB support not compiled into this version of mhttpd");
#endif
      return;
   }

   /*---- help command ----------------------------------------------*/

   if (equal_ustring(command, "help")) {
      show_help_page();
      return;
   }

   /*---- pause run -------------------------------------------*/

   if (equal_ustring(command, "pause")) {
      if (run_state != STATE_RUNNING) {
         show_error("Run is not running");
         return;
      }

      if (!check_web_password(cookie_wpwd, "?cmd=pause", experiment))
         return;

      status = cm_transition(TR_PAUSE, 0, str, sizeof(str), DETACH, FALSE);
      if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
         show_error(str);
      else if (isparam("redir"))
         redirect(getparam("redir"));
      else
         redirect("");

      requested_old_state = run_state;
      if (status == SUCCESS)
         requested_transition = TR_PAUSE;

      return;
   }

   /*---- resume run ------------------------------------------*/

   if (equal_ustring(command, "resume")) {
      if (run_state != STATE_PAUSED) {
         show_error("Run is not paused");
         return;
      }

      if (!check_web_password(cookie_wpwd, "?cmd=resume", experiment))
         return;

      status = cm_transition(TR_RESUME, 0, str, sizeof(str), DETACH, FALSE);
      if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
         show_error(str);
      else if (isparam("redir"))
         redirect(getparam("redir"));
      else
         redirect("");

      requested_old_state = run_state;
      if (status == SUCCESS)
         requested_transition = TR_RESUME;

      return;
   }

   /*---- start dialog --------------------------------------------*/

   if (equal_ustring(command, "start")) {
      if (run_state == STATE_RUNNING) {
         show_error("Run is already started");
         return;
      }

      if (value[0] == 0) {
         if (!check_web_password(cookie_wpwd, "?cmd=start", experiment))
            return;
         show_start_page(FALSE);
      } else {
         /* set run parameters */
         db_find_key(hDB, 0, "/Experiment/Edit on start", &hkey);
         if (hkey) {
            for (i = 0, n = 0;; i++) {
               db_enum_key(hDB, hkey, i, &hsubkey);

               if (!hsubkey)
                  break;

               db_get_key(hDB, hsubkey, &key);

               for (j = 0; j < key.num_values; j++) {
                  size = key.item_size;
                  sprintf(str, "x%d", n++);
                  db_sscanf(getparam(str), data, &size, 0, key.type);
                  db_set_data_index(hDB, hsubkey, data, key.item_size, j, key.type);
               }
            }
         }

         i = atoi(value);
         if (i <= 0) {
            cm_msg(MERROR, "interprete", "Start run: invalid run number %d", i);
            sprintf(str, "Invalid run number %d", i);
            show_error(str);
            return;
         }

         status = cm_transition(TR_START, i, str, sizeof(str), DETACH, FALSE);
         if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION) {
            show_error(str);
         } else {

            requested_old_state = run_state;
            requested_transition = TR_START;

            if (isparam("redir"))
               redirect(getparam("redir"));
            else
               redirect("");
         }
      }
      return;
   }

   /*---- stop run --------------------------------------------*/

   if (equal_ustring(command, "stop")) {
      if (run_state != STATE_RUNNING && run_state != STATE_PAUSED) {
         show_error("Run is not running");
         return;
      }

      if (!check_web_password(cookie_wpwd, "?cmd=stop", experiment))
         return;

      status = cm_transition(TR_STOP, 0, str, sizeof(str), DETACH, FALSE);
      if (status != CM_SUCCESS && status != CM_DEFERRED_TRANSITION)
         show_error(str);
      else if (isparam("redir"))
         redirect(getparam("redir"));
      else
         redirect("");

      requested_old_state = run_state;
      if (status == CM_SUCCESS)
         requested_transition = TR_STOP;

      return;
   }

   /*---- trigger equipment readout ---------------------------*/

   if (strncmp(command, "Trigger", 7) == 0) {
      sprintf(str, "?cmd=%s", command);
      if (!check_web_password(cookie_wpwd, str, experiment))
         return;

      /* extract equipment name */
      strlcpy(eq_name, command + 8, sizeof(eq_name));
      if (strchr(eq_name, ' '))
         *strchr(eq_name, ' ') = 0;

      /* get frontend name */
      sprintf(str, "/Equipment/%s/Common/Frontend name", eq_name);
      size = NAME_LENGTH;
      db_get_value(hDB, 0, str, fe_name, &size, TID_STRING, TRUE);

      /* and ID */
      sprintf(str, "/Equipment/%s/Common/Event ID", eq_name);
      size = sizeof(event_id);
      db_get_value(hDB, 0, str, &event_id, &size, TID_WORD, TRUE);

      if (cm_exist(fe_name, FALSE) != CM_SUCCESS) {
         sprintf(str, "Frontend \"%s\" not running!", fe_name);
         show_error(str);
      } else {
         status = cm_connect_client(fe_name, &hconn);
         if (status != RPC_SUCCESS) {
            sprintf(str, "Cannot connect to frontend \"%s\" !", fe_name);
            show_error(str);
         } else {
            status = rpc_client_call(hconn, RPC_MANUAL_TRIG, event_id);
            if (status != CM_SUCCESS)
               show_error("Error triggering event");
            else
               redirect("");

            cm_disconnect_client(hconn, FALSE);
         }
      }

      return;
   }

   /*---- switch to next subrun -------------------------------------*/

   if (strncmp(command, "Next Subrun", 11) == 0) {
      i = TRUE;
      db_set_value(hDB, 0, "/Logger/Next subrun", &i, sizeof(i), 1, TID_BOOL);
      redirect("");
      return;
   }

   /*---- cancel command --------------------------------------------*/

   if (equal_ustring(command, "cancel")) {

      if (group[0]) {
         /* extract equipment name */
         eq_name[0] = 0;
         if (strncmp(enc_path, "Equipment/", 10) == 0) {
            strlcpy(eq_name, enc_path + 10, sizeof(eq_name));
            if (strchr(eq_name, '/'))
               *strchr(eq_name, '/') = 0;
         }

         /* back to SC display */
         sprintf(str, "SC/%s/%s", eq_name, group);
         redirect(str);
      } else {
         if (isparam("redir"))
            redirect(getparam("redir"));
         else
            redirect("./");
      }

      return;
   }

   /*---- set command -----------------------------------------------*/

   if (equal_ustring(command, "set") && strncmp(path, "SC/", 3) != 0
       && strncmp(path, "CS/", 3) != 0) {

      if (strchr(enc_path, '/'))
         strlcpy(str, strrchr(enc_path, '/') + 1, sizeof(str));
      else
         strlcpy(str, enc_path, sizeof(str));
      strlcat(str, "?cmd=set", sizeof(str));
      if (!check_web_password(cookie_wpwd, str, experiment))
         return;

      show_set_page(enc_path, sizeof(enc_path), dec_path, group, index, value);
      return;
   }

   /*---- find command ----------------------------------------------*/

   if (equal_ustring(command, "find")) {
      show_find_page(enc_path, value);
      return;
   }

   /*---- create command --------------------------------------------*/

   if (equal_ustring(command, "create")) {
      sprintf(str, "%s?cmd=create", enc_path);
      if (!check_web_password(cookie_wpwd, str, experiment))
         return;

      show_create_page(enc_path, dec_path, value, index, atoi(getparam("type")));
      return;
   }

   /*---- CAMAC CNAF command ----------------------------------------*/

   if (equal_ustring(command, "CNAF") || strncmp(path, "CNAF", 4) == 0) {
      if (!check_web_password(cookie_wpwd, "?cmd=CNAF", experiment))
         return;

      show_cnaf_page();
      return;
   }

   /*---- alarms command --------------------------------------------*/

   if (equal_ustring(command, "reset all alarms")) {
      if (!check_web_password(cookie_wpwd, "?cmd=reset%20all%20alarms", experiment))
         return;

      al_reset_alarm(NULL);
      redirect("./?cmd=alarms");
      return;
   }

   if (equal_ustring(command, "reset")) {
      if (!check_web_password(cookie_wpwd, "?cmd=reset%20all%20alarms", experiment))
         return;

      al_reset_alarm(dec_path);
      redirect("./?cmd=alarms");
      return;
   }

   if (equal_ustring(command, "Alarms on/off")) {
      redirect("Alarms/Alarm system active?cmd=set");
      return;
   }

   /*---- programs command ------------------------------------------*/

   if (equal_ustring(command, "programs")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      if (str[0]) {
         strlcat(str, "./?cmd=programs", sizeof(str));
         redirect(str);
         return;
      }

      str[0] = 0;
      if (*getparam("Start"))
         sprintf(str, "?cmd=programs&Start=%s", getparam("Start"));
      if (*getparam("Stop"))
         sprintf(str, "?cmd=programs&Stop=%s", getparam("Stop"));

      if (str[0])
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;

      show_programs_page();
      return;
   }

   /*---- config command --------------------------------------------*/

   if (equal_ustring(command, "config")) {
      show_config_page(refresh);
      return;
   }

   /*---- Messages command ------------------------------------------*/

   if (equal_ustring(command, "messages")) {
      show_messages_page(refresh, 20);
      return;
   }

   if (strncmp(command, "More", 4) == 0 && strncmp(path, "EL/", 3) != 0) {
      i = atoi(command + 4);
      if (i == 0)
         i = 100;
      show_messages_page(0, i);
      return;
   }

  /*---- ELog command ----------------------------------------------*/

   if (equal_ustring(command, "elog")) {
      get_elog_url(str, sizeof(str));
      redirect(str);
      return;
   }

   if (strncmp(path, "EL/", 3) == 0) {
      if (equal_ustring(command, "new") || equal_ustring(command, "edit")
          || equal_ustring(command, "reply")) {
         sprintf(str, "%s?cmd=%s", path, command);
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;
      }

      show_elog_page(dec_path + 3, sizeof(dec_path) - 3);
      return;
   }

   if (equal_ustring(command, "Create ELog from this page")) {
      show_elog_page(dec_path, sizeof(dec_path));
      return;
   }

   /*---- accept command --------------------------------------------*/

   if (equal_ustring(command, "accept")) {
      refresh = atoi(getparam("refr"));

      /* redirect with cookie */
      rsprintf("HTTP/1.0 302 Found\r\n");
      rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
      rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n");

      time(&now);
      now += 3600 * 24 * 365;
      gmt = gmtime(&now);
      strftime(str, sizeof(str), "%A, %d-%b-%Y %H:00:00 GMT", gmt);

      rsprintf("Set-Cookie: midas_refr=%d; path=/; expires=%s\r\n", refresh, str);

      rsprintf("Location: ./\r\n\r\n<html>redir</html>\r\n");

      return;
   }

   /*---- delete command --------------------------------------------*/

   if (equal_ustring(command, "delete")) {
      sprintf(str, "%s?cmd=delete", enc_path);
      if (!check_web_password(cookie_wpwd, str, experiment))
         return;

      show_delete_page(enc_path, dec_path, value, index);
      return;
   }

   /*---- slow control display --------------------------------------*/

   if (strncmp(path, "SC/", 3) == 0) {
      if (equal_ustring(command, "edit")) {
         sprintf(str, "%s?cmd=Edit&index=%d", path, index);
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;
      }

      show_sc_page(dec_path + 3, refresh);
      return;
   }

   /*---- sequencer page --------------------------------------------*/

   if (equal_ustring(command, "sequencer")) {
      str[0] = 0;
      for (p=dec_path ; *p ; p++)
         if (*p == '/')
            strlcat(str, "../", sizeof(str));
      strlcat(str, "SEQ/", sizeof(str));
      redirect(str);
      return;
   }

   if (strncmp(path, "SEQ/", 4) == 0) {
      show_seq_page();
      return;
   }
   
   /*---- custom page -----------------------------------------------*/
   
   if (strncmp(path, "CS/", 3) == 0) {
      if (equal_ustring(command, "edit")) {
         sprintf(str, "%s?cmd=Edit&index=%d", path+3, index);
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;
      }

      show_custom_page(dec_path + 3, cookie_cpwd);
      return;
   }

   if (db_find_key(hDB, 0, "/Custom/Status", &hkey) == DB_SUCCESS && path[0] == 0) {
      if (equal_ustring(command, "edit")) {
         sprintf(str, "%s?cmd=Edit&index=%d", path, index);
         if (!check_web_password(cookie_wpwd, str, experiment))
            return;
      }

      show_custom_page("Status", cookie_cpwd);
      return;
   }

   /*---- show status -----------------------------------------------*/

   if (path[0] == 0) {
      if (elog_mode) {
         redirect("EL/");
         return;
      }

      show_status_page(refresh, cookie_wpwd);
      return;
   }

   /*---- show ODB --------------------------------------------------*/

   if (path[0]) {
      show_odb_page(enc_path, sizeof(enc_path), dec_path);
      return;
   }
}

/*------------------------------------------------------------------*/

void decode_get(char *string, char *cookie_pwd, char *cookie_wpwd, char *cookie_cpwd, int refresh)
{
   char path[256];
   char *p, *pitem;

   initparam();   
   strlcpy(path, string + 1, sizeof(path));     /* strip leading '/' */
   path[255] = 0;
   if (strchr(path, '?'))
      *strchr(path, '?') = 0;
   setparam("path", path);

   if (strchr(string, '?')) {
      p = strchr(string, '?') + 1;

      /* cut trailing "/" from netscape */
      if (p[strlen(p) - 1] == '/')
         p[strlen(p) - 1] = 0;

      p = strtok(p, "&");
      while (p != NULL) {
         pitem = p;
         p = strchr(p, '=');
         if (p != NULL) {
            *p++ = 0;
            urlDecode(pitem);
            if (!equal_ustring(pitem, "format"))
               urlDecode(p);

            setparam(pitem, p);

            p = strtok(NULL, "&");
         }
      }
   }

   interprete(cookie_pwd, cookie_wpwd, cookie_cpwd, path, refresh);

   freeparam();
}

/*------------------------------------------------------------------*/

void decode_post(char *header, char *string, char *boundary, int length,
                 char *cookie_pwd, char *cookie_wpwd, int refresh)
{
   char *pinit, *p, *pitem, *ptmp, file_name[256], str[256], path[256];
   int n;

   initparam();

   strlcpy(path, header + 1, sizeof(path));     /* strip leading '/' */
   path[255] = 0;
   if (strchr(path, '?'))
      *strchr(path, '?') = 0;
   if (strchr(path, ' '))
      *strchr(path, ' ') = 0;
   setparam("path", path);

   _attachment_size[0] = _attachment_size[1] = _attachment_size[2] = 0;
   pinit = string;

   /* return if no boundary defined */
   if (!boundary[0])
      return;

   if (strstr(string, boundary))
      string = strstr(string, boundary) + strlen(boundary);

   do {
      if (strstr(string, "name=")) {
         pitem = strstr(string, "name=") + 5;
         if (*pitem == '\"')
            pitem++;

         if (strncmp(pitem, "attfile", 7) == 0) {
            n = pitem[7] - '1';

            /* evaluate file attachment */
            if (strstr(pitem, "filename=")) {
               p = strstr(pitem, "filename=") + 9;
               if (*p == '\"')
                  p++;
               if (strstr(p, "\r\n\r\n"))
                  string = strstr(p, "\r\n\r\n") + 4;
               else if (strstr(p, "\r\r\n\r\r\n"))
                  string = strstr(p, "\r\r\n\r\r\n") + 6;
               if (strchr(p, '\"'))
                  *strchr(p, '\"') = 0;

               /* set attachment filename */
               strlcpy(file_name, p, sizeof(file_name));
               sprintf(str, "attachment%d", n);
               setparam(str, file_name);
            } else
               file_name[0] = 0;

            /* find next boundary */
            ptmp = string;
            do {
               while (*ptmp != '-')
                  ptmp++;

               if ((p = strstr(ptmp, boundary)) != NULL) {
                  while (*p == '-')
                     p--;
                  if (*p == 10)
                     p--;
                  if (*p == 13)
                     p--;
                  p++;
                  break;
               } else
                  ptmp += strlen(ptmp);

            } while (TRUE);

            /* save pointer to file */
            if (file_name[0]) {
               _attachment_buffer[n] = string;
               _attachment_size[n] = (POINTER_T) p - (POINTER_T) string;
            }

            string = strstr(p, boundary) + strlen(boundary);
         } else {
            p = pitem;
            if (strstr(p, "\r\n\r\n"))
               p = strstr(p, "\r\n\r\n") + 4;
            else if (strstr(p, "\r\r\n\r\r\n"))
               p = strstr(p, "\r\r\n\r\r\n") + 6;

            if (strchr(pitem, '\"'))
               *strchr(pitem, '\"') = 0;

            if (strstr(p, boundary)) {
               string = strstr(p, boundary) + strlen(boundary);
               *strstr(p, boundary) = 0;
               ptmp = p + (strlen(p) - 1);
               while (*ptmp == '-' || *ptmp == '\n' || *ptmp == '\r')
                  *ptmp-- = 0;
            }
            setparam(pitem, p);
         }

         while (*string == '-' || *string == '\n' || *string == '\r')
            string++;
      }

   } while ((POINTER_T) string - (POINTER_T) pinit < length);

   interprete(cookie_pwd, cookie_wpwd, "", path, refresh);
}

/*------------------------------------------------------------------*/

BOOL _abort = FALSE;

void ctrlc_handler(int sig)
{
   _abort = TRUE;
}

/*------------------------------------------------------------------*/

char net_buffer[WEB_BUFFER_SIZE];

void server_loop()
{
   int status, i, refresh, n_error;
   struct sockaddr_in bind_addr, acc_addr;
   char cookie_pwd[256], cookie_wpwd[256], cookie_cpwd[256], boundary[256], *p;
   int lsock, flag, content_length, header_length;
   unsigned int len;
   struct hostent *local_phe = NULL;
   fd_set readfds;
   struct timeval timeout;
   INT last_time = 0;
   struct hostent *remote_phe;
   char hname[256];
   BOOL allowed;

   /* establish Ctrl-C handler */
   ss_ctrlc_handler(ctrlc_handler);

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return;
   }
#endif

   /* create a new socket */
   lsock = socket(AF_INET, SOCK_STREAM, 0);

   if (lsock == -1) {
      printf("Cannot create socket\n");
      return;
   }

   /* bind local node name and port to socket */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   bind_addr.sin_port = htons((short) tcp_port);

   /* try reusing address */
   flag = 1;
   setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(INT));
   status = bind(lsock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));

   if (status < 0) {
      printf
          ("Cannot bind to port %d.\nPlease try later or use the \"-p\" flag to specify a different port\n",
           tcp_port);
      return;
   }

   /* get host name for mail notification */
   gethostname(host_name, sizeof(host_name));

   local_phe = gethostbyname(host_name);
   if (local_phe != NULL)
      local_phe = gethostbyaddr(local_phe->h_addr, sizeof(int), AF_INET);

   /* if domain name is not in host name, hope to get it from phe */
   if (local_phe != NULL && strchr(host_name, '.') == NULL)
      strlcpy(host_name, local_phe->h_name, sizeof(host_name));

#ifdef OS_UNIX
   /* give up root privilege */
   setuid(getuid());
   setgid(getgid());
#endif

   /* listen for connection */
   status = listen(lsock, SOMAXCONN);
   if (status < 0) {
      printf("Cannot listen\n");
      return;
   }

   printf("Server listening on port %d...\n", tcp_port);

   do {
      FD_ZERO(&readfds);
      FD_SET(lsock, &readfds);

      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;

#ifdef OS_UNIX
      do {
         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
         /* if an alarm signal was cought, restart with reduced timeout */
      } while (status == -1 && errno == EINTR);
#else
      status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
#endif

      if (FD_ISSET(lsock, &readfds)) {

         len = sizeof(acc_addr);
#ifdef OS_WINNT
         _sock = accept(lsock, (struct sockaddr *) &acc_addr, (int *)&len);
#else
         _sock = accept(lsock, (struct sockaddr *) &acc_addr, (socklen_t *)&len);
#endif

         last_time = (INT) ss_time();

         /* save remote host address */
         memcpy(&remote_addr, &(acc_addr.sin_addr), sizeof(remote_addr));

         /* check access control list */
         if (n_allowed_hosts > 0) {
            allowed = FALSE;

            remote_phe = gethostbyaddr((char *) &remote_addr, 4, PF_INET);

            if (remote_phe == NULL) {
               /* use IP number instead */
               strlcpy(hname, (char *)inet_ntoa(remote_addr), sizeof(hname));
            } else
               strlcpy(hname, remote_phe->h_name, sizeof(hname));

            /* always permit localhost */
            if (strcmp(hname, "localhost.localdomain") == 0)
               allowed = TRUE;
            if (strcmp(hname, "localhost") == 0)
               allowed = TRUE;

            if (!allowed) {
               for (i=0 ; i<n_allowed_hosts ; i++)
                  if (strcmp(hname, allowed_host[i]) == 0) {
                     allowed = TRUE;
                     break;
                  }
            }

            if (!allowed) {
               printf("Rejecting http connection from \'%s\'\n", hname);
               closesocket(_sock);
               continue;
            }
         }

         /* save remote host address */
         memcpy(&remote_addr, &(acc_addr.sin_addr), sizeof(remote_addr));
         if (verbose) {
            struct hostent *remote_phe;
            char str[256];

            printf("%s", ss_asctime());
            printf("=== Received request from ");

            remote_phe = gethostbyaddr((char *) &remote_addr, 4, PF_INET);
            if (remote_phe == NULL) {
               /* use IP number instead */
               strlcpy(str, (char *) inet_ntoa(remote_addr), sizeof(str));
            } else
               strlcpy(str, remote_phe->h_name, sizeof(str));

            puts(str);
            printf("===========\n");
            fflush(stdout);
         }

         memset(net_buffer, 0, sizeof(net_buffer));
         len = 0;
         header_length = 0;
         content_length = 0;
         n_error = 0;
         do {
            FD_ZERO(&readfds);
            FD_SET(_sock, &readfds);

            timeout.tv_sec = 6;
            timeout.tv_usec = 0;

#ifdef OS_UNIX
            do {
               status =
                   select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
               /* if an alarm signal was cought, restart with reduced timeout */
            } while (status == -1 && errno == EINTR);
#else
            status = select(FD_SETSIZE, (fd_set *) &readfds, NULL, NULL, (const timeval *) &timeout);
#endif
            if (FD_ISSET(_sock, &readfds))
               i = recv(_sock, net_buffer + len, sizeof(net_buffer) - len, 0);
            else
               goto error;

            /* abort if connection got broken */
            if (i < 0)
               goto error;

            if (i > 0)
               len += i;

            /* check if net_buffer too small */
            if (len >= sizeof(net_buffer)) {
               /* drain incoming remaining data */
               do {
                  FD_ZERO(&readfds);
                  FD_SET(_sock, &readfds);

                  timeout.tv_sec = 2;
                  timeout.tv_usec = 0;

                  status =
                      select(FD_SETSIZE, &readfds, NULL, NULL,
                             &timeout);

                  if (FD_ISSET(_sock, &readfds))
                     i = recv(_sock, net_buffer, sizeof(net_buffer), 0);
                  else
                     break;
               } while (i);

               memset(return_buffer, 0, return_size);
               strlen_retbuf = 0;
               return_length = 0;

               show_error("Submitted attachment too large, please increase WEB_BUFFER_SIZE in mhttpd.c and recompile");
               send(_sock, return_buffer, strlen_retbuf + 1, 0);
               if (verbose) {
                  printf("==== Return error info %i bytes ==============\n",
                         strlen_retbuf + 1);
                  puts(return_buffer);
                  printf("\n\n");
               }
               goto error;
            }

            if (i == 0) {
               n_error++;
               if (n_error == 100)
                  goto error;
            }

            /* finish when empty line received (fragmented TCP packets) */
            if (strstr(net_buffer, "\r\n\r\n")) {
               if (strstr(net_buffer, "GET") != NULL && strstr(net_buffer, "POST") == NULL) {
                  if (len > 4 && strcmp(&net_buffer[len - 4], "\r\n\r\n") == 0)
                     break;
                  if (len > 6 && strcmp(&net_buffer[len - 6], "\r\r\n\r\r\n") == 0)
                     break;
               } else if (strstr(net_buffer, "POST") != NULL) {
                  if (header_length == 0) {
                     /* extract header and content length */
                     if (strstr(net_buffer, "Content-Length:"))
                        content_length = atoi(strstr(net_buffer, "Content-Length:") + 15);
                     else if (strstr(net_buffer, "Content-length:"))
                        content_length = atoi(strstr(net_buffer, "Content-length:") + 15);
                     
                     boundary[0] = 0;
                     if (strstr(net_buffer, "boundary=")) {
                        strlcpy(boundary, strstr(net_buffer, "boundary=") + 9,
                                sizeof(boundary));
                        if (strchr(boundary, '\r'))
                           *strchr(boundary, '\r') = 0;
                     }
                     
                     if (strstr(net_buffer, "\r\n\r\n"))
                        header_length =
                        (POINTER_T) strstr(net_buffer,
                                           "\r\n\r\n") - (POINTER_T) net_buffer + 4;
                     
                     if (strstr(net_buffer, "\r\r\n\r\r\n"))
                        header_length =
                        (POINTER_T) strstr(net_buffer,
                                           "\r\r\n\r\r\n") - (POINTER_T) net_buffer + 6;
                     
                     if (header_length)
                        net_buffer[header_length - 1] = 0;
                  }
                  
                  if (header_length > 0 && (int) len >= header_length + content_length)
                     break;
               } else if (strstr(net_buffer, "OPTIONS") != NULL)
                  goto error;
               else {
                  printf("%s", net_buffer);
                  goto error;
               }
            }

         } while (1);

         if (!strchr(net_buffer, '\r'))
            goto error;

         /* extract cookies */
         cookie_pwd[0] = 0;
         cookie_wpwd[0] = 0;
         if (strstr(net_buffer, "midas_pwd=") != NULL) {
            strlcpy(cookie_pwd, strstr(net_buffer, "midas_pwd=") + 10,
                    sizeof(cookie_pwd));
            cookie_pwd[strcspn(cookie_pwd, " ;\r\n")] = 0;
         }
         if (strstr(net_buffer, "midas_wpwd=") != NULL) {
            strlcpy(cookie_wpwd, strstr(net_buffer, "midas_wpwd=") + 11,
                    sizeof(cookie_wpwd));
            cookie_wpwd[strcspn(cookie_wpwd, " ;\r\n")] = 0;
         }
         if (strstr(net_buffer, "cpwd=") != NULL) {
            strlcpy(cookie_cpwd, strstr(net_buffer, "cpwd=") + 5,
                    sizeof(cookie_cpwd));
            cookie_cpwd[strcspn(cookie_cpwd, " ;\r\n")] = 0;
         }

         refresh = 0;
         if (strstr(net_buffer, "midas_refr=") != NULL)
            refresh = atoi(strstr(net_buffer, "midas_refr=") + 11);
         else
            refresh = DEFAULT_REFRESH;

         /* extract referer */
         referer[0] = 0;
         if ((p = strstr(net_buffer, "Referer:")) != NULL) {
            p += 9;
            while (*p && *p == ' ')
               p++;
            strlcpy(referer, p, sizeof(referer));
            if (strchr(referer, '\r'))
               *strchr(referer, '\r') = 0;
            if (strchr(referer, '?'))
               *strchr(referer, '?') = 0;
            for (p = referer + strlen(referer) - 1; p > referer && *p != '/'; p--)
               *p = 0;
         }

         memset(return_buffer, 0, return_size);
         strlen_retbuf = 0;

         if (strncmp(net_buffer, "GET", 3) != 0 && strncmp(net_buffer, "POST", 4) != 0)
            goto error;

         return_length = 0;

         if (verbose) {
            INT temp;
            printf("Received buffer of %i bytes :\n%s\n", len, net_buffer);
            fflush(stdout);
            if (strncmp(net_buffer, "POST", 4) == 0) {
               printf("Contents of POST has %i bytes:\n", content_length);
               if (content_length > 2000) {
                  printf("--- Dumping first 2000 bytes only\n");
                  temp = net_buffer[header_length + 2000];
                  net_buffer[header_length + 2000] = 0;
                  puts(net_buffer + header_length);
                  net_buffer[header_length + 2000] = temp;
               } else
                  puts(net_buffer + header_length);
               printf("\n\n");
               fflush(stdout);
            }
         }

         if (strncmp(net_buffer, "GET", 3) == 0) {
            /* extract path and commands */
            *strchr(net_buffer, '\r') = 0;

            if (!strstr(net_buffer, "HTTP"))
               goto error;
            *(strstr(net_buffer, "HTTP") - 1) = 0;

            /* decode command and return answer */
            decode_get(net_buffer + 4, cookie_pwd, cookie_wpwd, cookie_cpwd, refresh);
         } else {
            decode_post(net_buffer + 5, net_buffer + header_length, boundary,
                        content_length, cookie_pwd, cookie_wpwd, refresh);
         }

         if (return_length != -1) {
            if (return_length == 0)
               return_length = strlen(return_buffer);

            if (verbose) {
               printf("%s", ss_asctime());
               printf("==== Return buffer %i bytes ===\n", return_length);
               printf("\n\n");
            }

            i = send_tcp(_sock, return_buffer, return_length, 0x10000);

            if (verbose) {
               if (return_length > 200) {
                  printf("--- Dumping first 200 bytes only\n");
                  return_buffer[200] = 0;
               }
               puts(return_buffer);
               printf("\n\n");
            }

          error:

            closesocket(_sock);
         }
      }

      /* re-establish ctrl-c handler */
      ss_ctrlc_handler(ctrlc_handler);

      /* check for shutdown message */
      status = cm_yield(0);
      if (status == RPC_SHUTDOWN)
         break;
      
      /* call sequencer periodically */
      sequencer();

   } while (!_abort);

   cm_disconnect_experiment();
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   int i, status;
   int daemon = FALSE;
   char str[256];
   const char *myname = "mhttpd";

   setbuf(stdout, NULL);
   setbuf(stderr, NULL);
#ifdef SIGPIPE
   /* avoid getting killed by "Broken pipe" signals */
   signal(SIGPIPE, SIG_IGN);
#endif

   /* get default from environment */
   cm_get_environment(midas_hostname, sizeof(midas_hostname), midas_expt, sizeof(midas_expt));

   /* parse command line parameters */
   n_allowed_hosts = 0;
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'D')
         daemon = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'v')
         verbose = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'E')
         elog_mode = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'H')
         history_mode = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'p')
            tcp_port = atoi(argv[++i]);
         else if (argv[i][1] == 'h')
            strlcpy(midas_hostname, argv[++i], sizeof(midas_hostname));
         else if (argv[i][1] == 'e')
            strlcpy(midas_expt, argv[++i], sizeof(midas_hostname));
         else if (argv[i][1] == 'a') {
            if (n_allowed_hosts < MAX_N_ALLOWED_HOSTS)
               strlcpy(allowed_host[n_allowed_hosts++], argv[++i], sizeof(allowed_host[0]));
         } else {
          usage:
            printf("usage: %s [-h Hostname] [-e Experiment] [-p port] [-v] [-D] [-c] [-a Hostname]\n\n", argv[0]);
            printf("       -h connect to midas server (mserver) on given host\n");
            printf("       -e experiment to connect to\n");
            printf("       -v display verbose HTTP communication\n");
            printf("       -D become a daemon\n");
            printf("       -E only display ELog system\n");
            printf("       -H only display history plots\n");
            printf("       -a only allow access for specific host(s), several\n");
            printf("          [-a Hostname] statements might be given\n");
            return 0;
         }
      }
   }

   if (daemon) {
      printf("Becoming a daemon...\n");
      ss_daemon_init(FALSE);
   }

#ifdef OS_LINUX
   /* write PID file */
   FILE *f = fopen("/var/run/mhttpd.pid", "w");
   if (f != NULL) {
      fprintf(f, "%d", ss_getpid());
      fclose(f);
   }
#endif

   if (history_mode)
      myname = "mhttpd_history";

   /*---- connect to experiment ----*/
   status = cm_connect_experiment1(midas_hostname, midas_expt, myname, NULL,
                                   DEFAULT_ODB_SIZE, DEFAULT_WATCHDOG_TIMEOUT);
   if (status == CM_WRONG_PASSWORD)
      return 1;
   else if (status == DB_INVALID_HANDLE) {
      cm_get_error(status, str);
      puts(str);
   } else if (status != CM_SUCCESS) {
      cm_get_error(status, str);
      puts(str);
      return 1;
   }

   /* place a request for system messages */
   cm_msg_register(receive_message);

   /* redirect message display, turn on message logging */
   cm_set_msg_print(MT_ALL, MT_ALL, print_message);

   /* initialize sequencer */
   init_sequencer();
   
   server_loop();

   return 0;
}
