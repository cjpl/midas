/********************************************************************\

  Name:         json_paste.cxx
  Created by:   Konstantin Olchanski

  Contents:     JSON decoder for ODB

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "midas.h"
#include "mjson.h"

INT EXPRT db_load_json(HNDLE hDB, HNDLE hKey, const char *filename)
{
   int status;
   //printf("db_load_json: filename [%s], handle %d\n", filename, hKey);

   FILE* fp = fopen(filename, "r");
   if (!fp) {
      cm_msg(MERROR, "db_load_json", "file \"%s\" not found", filename);
      return DB_FILE_ERROR;
   }

   std::string data;

   while (1) {
      char buf[102400];
      int rd = read(fileno(fp), buf, sizeof(buf)-1);
      if (rd == 0)
         break; // end of file
      if (rd < 0) {
         // error
         cm_msg(MERROR, "db_load_json", "file \"%s\" read error %d (%s)", filename, errno, strerror(errno));
         fclose(fp);
         return DB_FILE_ERROR;
      }
      // make sure string is nul terminated
      buf[sizeof(buf)-1] = 0;
      data += buf;
   }

   fclose(fp);
   fp = NULL;

   //printf("file contents: [%s]\n", data.c_str());

   const char* jptr = strchr(data.c_str(), '{');
   
   if (!jptr) {
      cm_msg(MERROR, "db_load_json", "file \"%s\" does not look like JSON data", filename);
      fclose(fp);
      return DB_FILE_ERROR;
   }

   status = db_paste_json(hDB, hKey, jptr);

   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_load_json", "error loading JSON data from file \"%s\"", filename);
      return status;
   }

   return status;
}

static int tid_from_key(const MJsonNode* key)
{
   // FIXME: read from key
   //const MJsonNodeMap* jmap = objnode->GetObject();
   //iassert(jmap!=NULL);
   return 0;
}

static int guess_tid(const MJsonNode* node)
{
   switch (node->GetType()) {
   case MJSON_ARRAY:  { const MJsonNodeVector* a = node->GetArray(); if (a && a->size()>0) return guess_tid((*a)[0]); else return 0; }
   case MJSON_OBJECT: return TID_KEY;
   case MJSON_STRING: {
      // FIXME: also watch for inf and nan
      std::string v = node->GetString();
      if (v[0]=='0' && v[1]=='x' && isxdigit(v[2]))
         return TID_DWORD;
      return TID_STRING;
   }
   case MJSON_INT:    return TID_INT;
   case MJSON_NUMBER: return TID_DOUBLE;
   case MJSON_BOOL:   return TID_BOOL;
   default: return 0;
   }
}

static int paste_node(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key);

static int paste_array(HNDLE hDB, HNDLE hKey, const char* path, const MJsonNode* node, int tid, const MJsonNode* key)
{
   int status;
   const MJsonNodeVector* a = node->GetArray();

   if (a==NULL) {
      cm_msg(MERROR, "db_paste_json", "invalid array at \"%s\"", path);
      return DB_FILE_ERROR;
   }

   int slength = 32; // FIXME: read from key

   printf("paste array %s, tid %d, size %d, string length %d\n", path, tid, (int)a->size(), slength);

   for (unsigned i=0; i<a->size(); i++) {
      MJsonNode* n = (*a)[i];
      if (!n)
         continue;
      status = paste_node(hDB, hKey, path, i, n, tid, slength, key);
      //if (status != DB_SUCCESS)
      //return status;
   }

   return DB_SUCCESS;
}

static int paste_object(HNDLE hDB, HNDLE hKey, const char* path, const MJsonNode* objnode)
{
   int status;
   const MJsonNodeMap* jmap = objnode->GetObject();
   if (jmap==NULL) {
      cm_msg(MERROR, "db_paste_json", "invalid object at \"%s\"", path);
      return DB_FILE_ERROR;
   }
   for(MJsonNodeMap::const_iterator it = jmap->begin(); it != jmap->end(); ++it) {
      const char* name = it->first.c_str();
      const MJsonNode* node = it->second;
      const MJsonNode* key = NULL;
      if (strchr(name, '/')) // skip special entries
         continue;
      int tid = 0;
      if (node->GetType() == MJSON_OBJECT)
         tid = TID_KEY;
      else {
         key = jmap->at(std::string(name) + "/key");
         tid = tid_from_key(key);
         if (!tid)
            tid = guess_tid(node);
         //printf("entry [%s] type %s, tid %d\n", name, MJsonNode::TypeToString(node->GetType()), tid);
      }

      status = db_create_key(hDB, hKey, name, tid);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot create \"%s\" of type %d in \"%s\", db_create_key() status %d", name, tid, path, status);
         return status;
      }

      HNDLE hSubkey;
      status = db_find_key(hDB, hKey, name, &hSubkey);
      assert(status==DB_SUCCESS);

      status = paste_node(hDB, hSubkey, (std::string(path)+"/"+name).c_str(), 0, node, tid, 0, key);
      //if (status != DB_SUCCESS)
      //return status;
   }
   return DB_SUCCESS;
}

static int paste_bool(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node)
{
   int status;
   BOOL value = node->GetBool();
   int size = sizeof(value);
   status = db_set_data_index(hDB, hKey, &value, size, index, TID_BOOL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_paste_json", "cannot set TID_BOOL value for \"%s\", db_set_data_index() status %d", path, status);
      return status;
   }
   return DB_SUCCESS;
}

static int paste_value(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   // FIXME: this is incomplete
   int status;
   if (tid == TID_STRING) {
      char* buf = NULL;
      const char* ptr = NULL;
      int size = 0;
      const std::string value = node->GetString();
      if (string_length) {
         buf = new char[string_length];
         strlcpy(buf, value.c_str(), string_length);
         ptr = buf;
         size = string_length;
      } else {
         ptr = value.c_str();
         size = strlen(ptr) + 1;
      }

      status = db_set_data_index(hDB, hKey, ptr, size, index, TID_STRING);

      if (buf)
         delete buf;

      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_STRING value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
   } if (tid == TID_DWORD) {
      //DWORD value = ...;
   }

   return DB_SUCCESS;
}

static int paste_node(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   //node->Dump();
   switch (node->GetType()) {
   case MJSON_ARRAY: return paste_array(hDB, hKey, path, node, tid, key);
   case MJSON_OBJECT: return paste_object(hDB, hKey, path, node);
   case MJSON_STRING:
   case MJSON_INT:
   case MJSON_NUMBER:
      return paste_value(hDB, hKey, path, index, node, tid, string_length, key);
   case MJSON_BOOL:
      return paste_bool(hDB, hKey, path, index, node);
   default:
      cm_msg(MERROR, "db_paste_json", "unexpected JSON node type %d (%s)", node->GetType(), MJsonNode::TypeToString(node->GetType()));
      return DB_FILE_ERROR;
   }
   return DB_SUCCESS;
}

INT EXPRT db_paste_json(HNDLE hDB, HNDLE hKeyRoot, const char *buffer)
{
   int status;
   char path[MAX_ODB_PATH];

   status = db_get_path(hDB, hKeyRoot, path, sizeof(path));

   //printf("db_paste_json: handle %d, path [%s]\n", hKeyRoot, path);

   MJsonNode* node = MJsonNode::Parse(buffer);
   status = paste_node(hDB, hKeyRoot, path, 0, node, 0, 0, NULL);
   delete node;

   return status;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
