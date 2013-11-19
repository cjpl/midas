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
   if (!key)
      return 0;
   const MJsonNode* n = key->FindObjectNode("type");
   if (!n)
      return 0;
   int tid = n->GetInt();
   if (tid > 0)
      return tid;
   return 0;
}

static int item_size_from_key(const MJsonNode* key)
{
   if (!key)
      return 0;
   const MJsonNode* n = key->FindObjectNode("item_size");
   if (!n)
      return 0;
   int item_size = n->GetInt();
   if (item_size > 0)
      return item_size;
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

   int slength = item_size_from_key(key);
   if (slength == 0)
      slength = NAME_LENGTH;

   for (unsigned i=0; i<a->size(); i++) {
      MJsonNode* n = (*a)[i];
      if (!n)
         continue;
      status = paste_node(hDB, hKey, path, i, n, tid, slength, key);
      if (status != DB_SUCCESS)
         return status;
   }

   return DB_SUCCESS;
}

static int paste_object(HNDLE hDB, HNDLE hKey, const char* path, const MJsonNode* objnode)
{
   int status;
   const MJsonStringVector* names = objnode->GetObjectNames();
   const MJsonNodeVector* nodes = objnode->GetObjectNodes();
   if (names==NULL||nodes==NULL||names->size()!=nodes->size()) {
      cm_msg(MERROR, "db_paste_json", "invalid object at \"%s\"", path);
      return DB_FILE_ERROR;
   }
   for(unsigned i=0; i<names->size(); i++) {
      const char* name = (*names)[i].c_str();
      const MJsonNode* node = (*nodes)[i];
      const MJsonNode* key = NULL;
      if (strchr(name, '/')) // skip special entries
         continue;
      int tid = 0;
      if (node->GetType() == MJSON_OBJECT)
         tid = TID_KEY;
      else {
         key = objnode->FindObjectNode((std::string(name) + "/key").c_str());
         if (key)
            tid = tid_from_key(key);
         if (!tid)
            tid = guess_tid(node);
         //printf("entry [%s] type %s, tid %d\n", name, MJsonNode::TypeToString(node->GetType()), tid);
      }

      status = db_create_key(hDB, hKey, name, tid);

      if (status == DB_KEY_EXIST) {
         HNDLE hSubkey;
         KEY key;
         status = db_find_link(hDB, hKey, name, &hSubkey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_json", "key exists, but cannot find it \"%s\" of type %d in \"%s\", db_find_link() status %d", name, tid, path, status);
            return status;
         }

         status = db_get_key(hDB, hSubkey, &key);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_json", "cannot create \"%s\" of type %d in \"%s\", db_create_key() status %d", name, tid, path, status);
            return status;
         }

         if ((int)key.type == tid) {
            // existing item is of the same type, continue with overwriting it
            status = DB_SUCCESS;
         } else {
            // FIXME: delete wrong item, create item with correct tid
            cm_msg(MERROR, "db_paste_json", "cannot overwrite existing item \"%s\" of type %d in \"%s\" with new tid %d", name, key.type, path, tid);
            return status;
         }
      }

      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot create \"%s\" of type %d in \"%s\", db_create_key() status %d", name, tid, path, status);
         return status;
      }

      HNDLE hSubkey;
      status = db_find_link(hDB, hKey, name, &hSubkey);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot find \"%s\" of type %d in \"%s\", db_find_link() status %d", name, tid, path, status);
         return status;
      }

      status = paste_node(hDB, hSubkey, (std::string(path)+"/"+name).c_str(), 0, node, tid, 0, key);
      if (status != DB_SUCCESS) {
         //cm_msg(MERROR, "db_paste_json", "cannot..."); // paste_node() reports it's own failures
         return status;
      }
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

static DWORD GetDWORD(const MJsonNode* node, const char* path)
{
   switch (node->GetType()) {
   default:
      cm_msg(MERROR, "db_paste_json", "GetDWORD: unexpeted node type %d at \"%s\"", node->GetType(), path);
      return 0;
   case MJSON_INT:
      return node->GetInt();
   case MJSON_STRING:
      return strtoul(node->GetString().c_str(), NULL, 0);
   }
}

static int paste_value(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   int status;
   //printf("paste_value: path [%s], index %d, tid %d, slength %d, key %p\n", path, index, tid, string_length, key);

   switch (tid) {
   default:
      cm_msg(MERROR, "db_paste_json", "do not know what to do with tid %d at \"%s\"", tid, path);
      return DB_FILE_ERROR;
   case TID_BITFIELD:
      cm_msg(MERROR, "db_paste_json", "paste of TID_BITFIELD is not implemented at \"%s\"", path);
      return DB_SUCCESS;
   case TID_CHAR: {
      const std::string value = node->GetString();
      int size = 1;
      status = db_set_data_index(hDB, hKey, value.c_str(), size, index, TID_CHAR);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_CHAR value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_BOOL: {
      DWORD dw = GetDWORD(node, path);
      BOOL v;
      if (dw) v = TRUE;
      else v = FALSE;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_BOOL);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_BOOL value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_BYTE:
   case TID_SBYTE: {
      char v = node->GetInt();
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, tid);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_BYTE/TID_SBYTE value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_WORD:
   case TID_SHORT: {
      WORD v = GetDWORD(node, path);
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, tid);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_WORD/TID_SHORT value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_DWORD: {
      DWORD v = GetDWORD(node, path);
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_DWORD);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_DWORD value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_INT: {
      int v = node->GetInt();
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_INT);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_INT value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_FLOAT: {
      float v = node->GetNumber();
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_FLOAT);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_FLOAT value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_DOUBLE: {
      double v = node->GetNumber();
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_DOUBLE);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_DOUBLE value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_STRING: {
      char* buf = NULL;
      const char* ptr = NULL;
      int size = 0;
      const std::string value = node->GetString();
      if (string_length == 0)
         string_length = item_size_from_key(key);
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

      return DB_SUCCESS;
   }
   case TID_LINK: {
      const char* value = node->GetString().c_str();
      int size = strlen(value) + 1;

      status = db_set_data(hDB, hKey, value, size, 1, TID_LINK);

      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_LINK value for \"%s\", db_set_data() status %d", path, status);
         return status;
      }

      return DB_SUCCESS;
   }
   }
   // NOT REACHED
}

static int paste_node(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   //node->Dump();
   switch (node->GetType()) {
   case MJSON_ARRAY:  return paste_array(hDB, hKey, path, node, tid, key);
   case MJSON_OBJECT: return paste_object(hDB, hKey, path, node);
   case MJSON_STRING: return paste_value(hDB, hKey, path, index, node, tid, string_length, key);
   case MJSON_INT:    return paste_value(hDB, hKey, path, index, node, tid, 0, key);
   case MJSON_NUMBER: return paste_value(hDB, hKey, path, index, node, tid, 0, key);
   case MJSON_BOOL:   return paste_bool(hDB, hKey, path, index, node);
   case MJSON_ERROR:
      cm_msg(MERROR, "db_paste_json", "JSON parse error: %s", node->GetError().c_str());
      return DB_FILE_ERROR;
   default:
      cm_msg(MERROR, "db_paste_json", "unexpected JSON node type %d (%s)", node->GetType(), MJsonNode::TypeToString(node->GetType()));
      return DB_FILE_ERROR;
   }
   // NOT REACHED
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
