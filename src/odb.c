/********************************************************************\

  Name:         ODB.C
  Created by:   Stefan Ritt

  Contents:     MIDAS online database functions

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file odb.c
The Online Database file
*/

/** @defgroup odbcode The odb.c
 */
/** @defgroup odbfunctionc Midas ODB Functions (db_xxx)
 */

/**dox***************************************************************/
/** @addtogroup odbcode
*  
 *  @{  */

/**dox***************************************************************/
/** @addtogroup odbfunctionc
*  
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "midas.h"
#include "msystem.h"
#include "mxml.h"
#include "strlcpy.h"
#include <assert.h>
#include <signal.h>

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 db_xxx  -  Database Functions                      *
*                                                                    *
\********************************************************************/

/* Globals */

DATABASE *_database;
INT _database_entries = 0;

static RECORD_LIST *_record_list;
static INT _record_list_entries = 0;

extern char *tid_name[];

INT db_save_xml_key(HNDLE hDB, HNDLE hKey, INT level, MXML_WRITER * writer);

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*            Shared Memory Allocation                                *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
void *malloc_key(DATABASE_HEADER * pheader, INT size)
{
   FREE_DESCRIP *pfree, *pfound, *pprev = NULL;

   if (size == 0)
      return NULL;

   /* quadword alignment for alpha CPU */
   size = ALIGN8(size);

   /* search for free block */
   pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

   while (pfree->size < size && pfree->next_free) {
      pprev = pfree;
      pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
   }

   /* return if not enough memory */
   if (pfree->size < size)
      return 0;

   pfound = pfree;

   /* if found block is first in list, correct pheader */
   if (pfree == (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key)) {
      if (size < pfree->size) {
         /* free block is only used partially */
         pheader->first_free_key += size;
         pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

         pfree->size = pfound->size - size;
         pfree->next_free = pfound->next_free;
      } else {
         /* free block is used totally */
         pheader->first_free_key = pfree->next_free;
      }
   } else {
      /* check if free block is used totally */
      if (pfound->size - size < (int) sizeof(FREE_DESCRIP)) {
         /* skip block totally */
         pprev->next_free = pfound->next_free;
      } else {
         /* decrease free block */
         pfree = (FREE_DESCRIP *) ((char *) pfound + size);

         pfree->size = pfound->size - size;
         pfree->next_free = pfound->next_free;

         pprev->next_free = (POINTER_T) pfree - (POINTER_T) pheader;
      }
   }


   memset(pfound, 0, size);

   return pfound;
}

/*------------------------------------------------------------------*/
void free_key(DATABASE_HEADER * pheader, void *address, INT size)
{
   FREE_DESCRIP *pfree, *pprev, *pnext;

   if (size == 0)
      return;

   /* quadword alignment for alpha CPU */
   size = ALIGN8(size);

   pfree = (FREE_DESCRIP *) address;
   pprev = NULL;

   /* clear current block */
   memset(address, 0, size);

   /* if key comes before first free block, adjust pheader */
   if ((POINTER_T) address - (POINTER_T) pheader < pheader->first_free_key) {
      pfree->size = size;
      pfree->next_free = pheader->first_free_key;
      pheader->first_free_key = (POINTER_T) address - (POINTER_T) pheader;
   } else {
      /* find last free block before current block */
      pprev = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

      while (pprev->next_free < (POINTER_T) address - (POINTER_T) pheader) {
         if (pprev->next_free <= 0) {
            cm_msg(MERROR, "free_key",
                   "database is corrupted: pprev=0x%x, pprev->next_free=%d", pprev, pprev->next_free);
            return;
         }
         pprev = (FREE_DESCRIP *) ((char *) pheader + pprev->next_free);
      }

      pfree->size = size;
      pfree->next_free = pprev->next_free;

      pprev->next_free = (POINTER_T) pfree - (POINTER_T) pheader;
   }

   /* try to melt adjacent free blocks after current block */
   pnext = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
   if ((POINTER_T) pnext == (POINTER_T) pfree + pfree->size) {
      pfree->size += pnext->size;
      pfree->next_free = pnext->next_free;

      memset(pnext, 0, pnext->size);
   }

   /* try to melt adjacent free blocks before current block */
   if (pprev && pprev->next_free == (POINTER_T) pprev - (POINTER_T) pheader + pprev->size) {
      pprev->size += pfree->size;
      pprev->next_free = pfree->next_free;

      memset(pfree, 0, pfree->size);
   }
}

/*------------------------------------------------------------------*/
void *malloc_data(DATABASE_HEADER * pheader, INT size)
{
   FREE_DESCRIP *pfree, *pfound, *pprev = NULL;

   if (size == 0)
      return NULL;

   /* quadword alignment for alpha CPU */
   size = ALIGN8(size);

   /* search for free block */
   pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

   while (pfree->size < size && pfree->next_free) {
      pprev = pfree;
      pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
   }

   /* return if not enough memory */
   if (pfree->size < size)
      return 0;

   pfound = pfree;

   /* if found block is first in list, correct pheader */
   if (pfree == (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data)) {
      if (size < pfree->size) {
         /* free block is only used partially */
         pheader->first_free_data += size;
         pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

         pfree->size = pfound->size - size;
         pfree->next_free = pfound->next_free;
      } else {
         /* free block is used totally */
         pheader->first_free_data = pfree->next_free;
      }
   } else {
      /* check if free block is used totally */
      if (pfound->size - size < (int) sizeof(FREE_DESCRIP)) {
         /* skip block totally */
         pprev->next_free = pfound->next_free;
      } else {
         /* decrease free block */
         pfree = (FREE_DESCRIP *) ((char *) pfound + size);

         pfree->size = pfound->size - size;
         pfree->next_free = pfound->next_free;

         pprev->next_free = (POINTER_T) pfree - (POINTER_T) pheader;
      }
   }

   /* zero memeory */
   memset(pfound, 0, size);

   return pfound;
}

/*------------------------------------------------------------------*/
void free_data(DATABASE_HEADER * pheader, void *address, INT size)
{
   FREE_DESCRIP *pfree, *pprev, *pnext;

   if (size == 0)
      return;

   /* quadword alignment for alpha CPU */
   size = ALIGN8(size);

   pfree = (FREE_DESCRIP *) address;
   pprev = NULL;

   /* clear current block */
   memset(address, 0, size);

   /* if data comes before first free block, adjust pheader */
   if ((POINTER_T) address - (POINTER_T) pheader < pheader->first_free_data) {
      pfree->size = size;
      pfree->next_free = pheader->first_free_data;
      pheader->first_free_data = (POINTER_T) address - (POINTER_T) pheader;
   } else {
      /* find last free block before current block */
      pprev = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

      while (pprev->next_free < (POINTER_T) address - (POINTER_T) pheader) {
         if (pprev->next_free <= 0) {
            cm_msg(MERROR, "free_data",
                   "database is corrupted: pprev=0x%x, pprev->next_free=%d", pprev, pprev->next_free);
            return;
         }

         pprev = (FREE_DESCRIP *) ((char *) pheader + pprev->next_free);
      }

      pfree->size = size;
      pfree->next_free = pprev->next_free;

      pprev->next_free = (POINTER_T) pfree - (POINTER_T) pheader;
   }

   /* try to melt adjacent free blocks after current block */
   pnext = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
   if ((POINTER_T) pnext == (POINTER_T) pfree + pfree->size) {
      pfree->size += pnext->size;
      pfree->next_free = pnext->next_free;

      memset(pnext, 0, pnext->size);
   }

   /* try to melt adjacent free blocks before current block */
   if (pprev && pprev->next_free == (POINTER_T) pprev - (POINTER_T) pheader + pprev->size) {
      pprev->size += pfree->size;
      pprev->next_free = pfree->next_free;

      memset(pfree, 0, pfree->size);
   }
}

/*------------------------------------------------------------------*/
void *realloc_data(DATABASE_HEADER * pheader, void *address, INT old_size, INT new_size)
{
   void *tmp = NULL, *pnew;

   if (old_size) {
      tmp = malloc(old_size);
      if (tmp == NULL)
         return NULL;

      memcpy(tmp, address, old_size);
      free_data(pheader, address, old_size);
   }

   pnew = malloc_data(pheader, new_size);

   if (pnew && old_size)
      memcpy(pnew, tmp, old_size < new_size ? old_size : new_size);

   if (old_size)
      free(tmp);

   return pnew;
}

/*------------------------------------------------------------------*/
char *strcomb(const char **list)
/* convert list of strings into single string to be used by db_paste() */
{
   INT i, j;
   static char *str = NULL;

   /* counter number of chars */
   for (i = 0, j = 0; list[i]; i++)
      j += strlen(list[i]) + 1;
   j += 1;

   if (str == NULL)
      str = (char *) malloc(j);
   else
      str = (char *) realloc(str, j);

   str[0] = 0;
   for (i = 0; list[i]; i++) {
      strcat(str, list[i]);
      strcat(str, "\n");
   }

   return str;
}

/*------------------------------------------------------------------*/
INT print_key_info(HNDLE hDB, HNDLE hKey, KEY * pkey, INT level, void *info)
{
   int i;
   char *p;

   i = hDB;
   p = (char *) info;

   sprintf(p + strlen(p), "%08X  %08X  %04X    ",
           (int) (hKey - sizeof(DATABASE_HEADER)),
           (int) (pkey->data - sizeof(DATABASE_HEADER)), (int) pkey->total_size);

   for (i = 0; i < level; i++)
      sprintf(p + strlen(p), "  ");

   sprintf(p + strlen(p), "%s\n", pkey->name);

   return SUCCESS;
}

INT db_show_mem(HNDLE hDB, char *result, INT buf_size, BOOL verbose)
{
   DATABASE_HEADER *pheader;
   INT total_size_key, total_size_data;
   FREE_DESCRIP *pfree;

   /* avoid compiler warning */
   total_size_data = buf_size;

   db_lock_database(hDB);

   pheader = _database[hDB - 1].database_header;

   result[0] = 0;
   sprintf(result + strlen(result), "Database header size is 0x%04X, all following values are offset by this!\n", (int)sizeof(DATABASE_HEADER));
   sprintf(result + strlen(result), "Key area  0x00000000 - 0x%08X, size %d bytes\n",  pheader->key_size - 1, pheader->key_size);
   sprintf(result + strlen(result), "Data area 0x%08X - 0x%08X, size %d bytes\n\n",    pheader->key_size, pheader->key_size + pheader->data_size, pheader->data_size);

   strcat(result, "Keylist:\n");
   strcat(result, "--------\n");
   total_size_key = 0;
   pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

   while ((POINTER_T) pfree != (POINTER_T) pheader) {
      total_size_key += pfree->size;
      sprintf(result + strlen(result),
              "Free block at 0x%08X, size 0x%08X, next 0x%08X\n",
              (int) ((POINTER_T) pfree - (POINTER_T) pheader - sizeof(DATABASE_HEADER)),
              pfree->size, pfree->next_free ? (int) (pfree->next_free - sizeof(DATABASE_HEADER)) : 0);
      pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
   }

   sprintf(result + strlen(result), "\nFree Key area: %d bytes out of %d bytes\n", total_size_key, pheader->key_size);

   strcat(result, "\nData:\n");
   strcat(result, "-----\n");
   total_size_data = 0;
   pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

   while ((POINTER_T) pfree != (POINTER_T) pheader) {
      total_size_data += pfree->size;
      sprintf(result + strlen(result),
              "Free block at 0x%08X, size 0x%08X, next 0x%08X\n",
              (int) ((POINTER_T) pfree - (POINTER_T) pheader - sizeof(DATABASE_HEADER)),
              pfree->size, pfree->next_free ? (int) (pfree->next_free - sizeof(DATABASE_HEADER)) : 0);
      pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
   }

   sprintf(result + strlen(result), "\nFree Data area: %d bytes out of %d bytes\n", total_size_data, pheader->data_size);

   sprintf(result + strlen(result),
           "\nFree: %1d (%1.1lf%%) keylist, %1d (%1.1lf%%) data\n",
           total_size_key,
           100 * (double) total_size_key / pheader->key_size,
           total_size_data, 100 * (double) total_size_data / pheader->data_size);

   if (verbose) {
      sprintf(result + strlen(result), "\n\n");
      sprintf(result + strlen(result), "Key       Data      Size\n");
      sprintf(result + strlen(result), "------------------------\n");
      db_scan_tree(hDB, pheader->root_key, 0, print_key_info, result);
   }

   db_unlock_database(hDB);

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
static int db_validate_key_offset(DATABASE_HEADER * pheader, int offset)
/* check if key offset lies in valid range */
{
   if (offset != 0 && offset < (int) sizeof(DATABASE_HEADER))
      return 0;

   if (offset > (int) sizeof(DATABASE_HEADER) + pheader->key_size)
      return 0;

   return 1;
}

static int db_validate_data_offset(DATABASE_HEADER * pheader, int offset)
/* check if data offset lies in valid range */
{
   if (offset != 0 && offset < (int) sizeof(DATABASE_HEADER))
      return 0;

   if (offset > (int) sizeof(DATABASE_HEADER) + pheader->key_size + pheader->data_size)
      return 0;

   return 1;
}

static int db_validate_hkey(DATABASE_HEADER * pheader, HNDLE hKey)
{
   return db_validate_key_offset(pheader, hKey);
}

static int db_validate_key(DATABASE_HEADER * pheader, int recurse, const char *path, KEY * pkey)
{
   KEYLIST *pkeylist;
   int i;
   static time_t t_min = 0, t_max;

   if (!db_validate_key_offset(pheader, (POINTER_T) pkey - (POINTER_T) pheader)) {
      cm_msg(MERROR, "db_validate_key",
             "Warning: database corruption, key \"%s\", data 0x%08X", path, pkey->data - sizeof(DATABASE_HEADER));
      return 0;
   }

   if (!db_validate_data_offset(pheader, pkey->data)) {
      cm_msg(MERROR, "db_validate_key",
             "Warning: database corruption, data \"%s\", data 0x%08X", path, pkey->data - sizeof(DATABASE_HEADER));
      return 0;
   }
#if 0
   /* correct invalid key type 0 */
   if (pkey->type == 0) {
      cm_msg(MERROR, "db_validate_key",
             "Warning: invalid key type, key \"%s\", type %d, changed to type %d", path, pkey->type, TID_KEY);
      pkey->type = TID_KEY;
   }
#endif

   /* check key type */
   if (pkey->type <= 0 || pkey->type >= TID_LAST) {
      cm_msg(MERROR, "db_validate_key", "Warning: invalid key type, key \"%s\", type %d", path, pkey->type);
      return 0;
   }

   /* check key sizes */
   if ((pkey->total_size < 0) || (pkey->total_size > pheader->key_size)) {
      cm_msg(MERROR, "db_validate_key", "Warning: invalid key \"%s\" total_size: %d", path, pkey->total_size);
      return 0;
   }

   if ((pkey->item_size < 0) || (pkey->item_size > pheader->key_size)) {
      cm_msg(MERROR, "db_validate_key", "Warning: invalid key \"%s\" item_size: %d", path, pkey->item_size);
      return 0;
   }

   if ((pkey->num_values < 0) || (pkey->num_values > pheader->key_size)) {
      cm_msg(MERROR, "db_validate_key", "Warning: invalid key \"%s\" num_values: %d", path, pkey->num_values);
      return 0;
   }

   /* check and correct key size */
   if (pkey->total_size != pkey->item_size * pkey->num_values) {
      cm_msg(MINFO, "db_validate_key",
             "Warning: corrected key \"%s\" size: total_size=%d, should be %d*%d=%d",
             path, pkey->total_size, pkey->item_size, pkey->num_values, pkey->item_size * pkey->num_values);
      pkey->total_size = pkey->item_size * pkey->num_values;
   }

   /* check access mode */
   if ((pkey->access_mode & ~(MODE_READ | MODE_WRITE | MODE_DELETE | MODE_EXCLUSIVE | MODE_ALLOC))) {
      cm_msg(MERROR, "db_validate_key", "Warning: invalid access mode, key \"%s\", mode %d", path, pkey->access_mode);
      return 0;
   }

   /* check access time, consider valid if within +- 10 years */
   if (t_min == 0) {
      t_min = ss_time() - 3600 * 24 * 365 * 10;
      t_max = ss_time() + 3600 * 24 * 365 * 10;
   }

   if (pkey->last_written > 0 && (pkey->last_written < t_min || pkey->last_written > t_max)) {
      cm_msg(MERROR, "db_validate_key", "Warning: invalid access time, key \"%s\", time %d", path, pkey->last_written);
      return 0;
   }

   if (pkey->type == TID_KEY && recurse) {
      /* if key has subkeys, go through whole list */

      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (pkeylist->num_keys != 0 &&
          (pkeylist->first_key == 0 || !db_validate_key_offset(pheader, pkeylist->first_key))) {
         cm_msg(MERROR, "db_validate_key",
                "Warning: database corruption, key \"%s\", first_key 0x%08X",
                path, pkeylist->first_key - sizeof(DATABASE_HEADER));
         return 0;
      }

      /* check if key is in keylist */
      pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

      for (i = 0; i < pkeylist->num_keys; i++) {
         char buf[1024];
         sprintf(buf, "%s/%s", path, pkey->name);

         if (!db_validate_key_offset(pheader, pkey->next_key)) {
            cm_msg(MERROR, "db_validate_key",
                   "Warning: database corruption, key \"%s\", next_key 0x%08X",
                   buf, pkey->next_key - sizeof(DATABASE_HEADER));
            return 0;
         }

         if (!db_validate_key(pheader, recurse + 1, buf, pkey))
            return 0;

         pkey = (KEY *) ((char *) pheader + pkey->next_key);
      }
   }

   return 1;
}

/*------------------------------------------------------------------*/
static void db_validate_sizes()
{
   /* validate size of data structures (miscompiled, 32/64 bit mismatch, etc */

   if (0) {
#define S(x) printf("assert(sizeof(%-20s) == %6d);\n", #x, (int)sizeof(x))
      // basic data types
      S(char *);
      S(char);
      S(int);
      S(long int);
      S(float);
      S(double);
      S(BOOL);
      S(WORD);
      S(DWORD);
      S(INT);
      S(POINTER_T);
      S(midas_thread_t);
      // data buffers
      S(EVENT_REQUEST);
      S(BUFFER_CLIENT);
      S(BUFFER_HEADER);
      // history files
      S(HIST_RECORD);
      S(DEF_RECORD);
      S(INDEX_RECORD);
      S(TAG);
      // ODB shared memory structures
      S(KEY);
      S(KEYLIST);
      S(OPEN_RECORD);
      S(DATABASE_CLIENT);
      S(DATABASE_HEADER);
      // misc structures
      S(EVENT_HEADER);
      S(EQUIPMENT_INFO);
      S(EQUIPMENT_STATS);
      S(BANK_HEADER);
      S(BANK);
      S(BANK32);
      S(ANA_OUTPUT_INFO);
      S(PROGRAM_INFO);
      S(ALARM_CLASS);
      S(ALARM);
      S(CHN_SETTINGS);
      S(CHN_STATISTICS);
#undef S
   }
#ifdef OS_LINUX
   assert(sizeof(EVENT_REQUEST) == 16); // ODB v3
   assert(sizeof(BUFFER_CLIENT) == 256);
   assert(sizeof(BUFFER_HEADER) == 16444);
   assert(sizeof(HIST_RECORD) == 20);
   assert(sizeof(DEF_RECORD) == 40);
   assert(sizeof(INDEX_RECORD) == 12);
   assert(sizeof(TAG) == 40);
   assert(sizeof(KEY) == 68);
   assert(sizeof(KEYLIST) == 12);
   assert(sizeof(OPEN_RECORD) == 8);
   assert(sizeof(DATABASE_CLIENT) == 2112);
   assert(sizeof(DATABASE_HEADER) == 135232);
   assert(sizeof(EVENT_HEADER) == 16);
   //assert(sizeof(EQUIPMENT_INFO) == 400); // ODB v3, midas.h before rev 4558
   assert(sizeof(EQUIPMENT_INFO) == 688); // ODB v3, midas.h after rev 4558
   assert(sizeof(EQUIPMENT_STATS) == 24);
   assert(sizeof(BANK_HEADER) == 8);
   assert(sizeof(BANK) == 8);
   assert(sizeof(BANK32) == 12);
   assert(sizeof(ANA_OUTPUT_INFO) == 792);
   assert(sizeof(PROGRAM_INFO) == 316);
   assert(sizeof(ALARM_CLASS) == 348);
   assert(sizeof(ALARM) == 452);
   assert(sizeof(CHN_SETTINGS) == 648); // ODB v3
   assert(sizeof(CHN_STATISTICS) == 56);        // ODB v3
#endif
}

/*------------------------------------------------------------------*/
static int db_validate_db(DATABASE_HEADER * pheader)
{
   int total_size_key = 0;
   int total_size_data = 0;
   double ratio;
   FREE_DESCRIP *pfree;

   /* validate size of data structures (miscompiled, 32/64 bit mismatch, etc */

   db_validate_sizes();

   /* validate the key free list */

   if (!db_validate_key_offset(pheader, pheader->first_free_key)) {
      cm_msg(MERROR, "db_validate_db",
             "Warning: database corruption, first_free_key 0x%08X", pheader->first_free_key - sizeof(DATABASE_HEADER));
      return 0;
   }

   pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

   while ((POINTER_T) pfree != (POINTER_T) pheader) {
      FREE_DESCRIP *nextpfree;

      if (pfree->next_free != 0 && !db_validate_key_offset(pheader, pfree->next_free)) {
         cm_msg(MERROR, "db_validate_db",
                "Warning: database corruption, key area next_free 0x%08X", pfree->next_free - sizeof(DATABASE_HEADER));
         return 0;
      }

      total_size_key += pfree->size;
      nextpfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);

      if (pfree->next_free != 0 && nextpfree == pfree) {
         cm_msg(MERROR, "db_validate_db",
                "Warning: database corruption, key area next_free 0x%08X is same as current free",
                pfree - sizeof(DATABASE_HEADER));
         return 0;
      }

      pfree = nextpfree;
   }

   ratio = ((double) (pheader->key_size - total_size_key)) / ((double) pheader->key_size);
   if (ratio > 0.9)
      cm_msg(MERROR, "db_validate_db", "Warning: database key area is %.0f%% full", ratio * 100.0);

   if (total_size_key > pheader->key_size) {
      cm_msg(MERROR, "db_validate_db", "Warning: database corruption, total_key_size 0x%08X", total_size_key);
      return 0;
   }

   /* validate the data free list */

   if (!db_validate_data_offset(pheader, pheader->first_free_data)) {
      cm_msg(MERROR, "db_validate_db",
             "Warning: database corruption, first_free_data 0x%08X",
             pheader->first_free_data - sizeof(DATABASE_HEADER));
      return 0;
   }

   pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

   while ((POINTER_T) pfree != (POINTER_T) pheader) {
      FREE_DESCRIP *nextpfree;

      if (pfree->next_free != 0 && !db_validate_data_offset(pheader, pfree->next_free)) {
         cm_msg(MERROR, "db_validate_db",
                "Warning: database corruption, data area next_free 0x%08X", pfree->next_free - sizeof(DATABASE_HEADER));
         return 0;
      }

      total_size_data += pfree->size;
      nextpfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);

      if (pfree->next_free != 0 && nextpfree == pfree) {
         cm_msg(MERROR, "db_validate_db",
                "Warning: database corruption, data area next_free 0x%08X is same as current free",
                pfree - sizeof(DATABASE_HEADER));
         return 0;
      }

      pfree = nextpfree;
   }

   ratio = ((double) (pheader->data_size - total_size_data)) / ((double) pheader->data_size);
   if (ratio > 0.9)
      cm_msg(MERROR, "db_validate_db", "Warning: database data area is %.0f%% full", ratio * 100.0);

   if (total_size_data > pheader->data_size) {
      cm_msg(MERROR, "db_validate_db", "Warning: database corruption, total_size_data 0x%08X", total_size_key);
      return 0;
   }

   /* validate the tree of keys, starting from the root key */

   if (!db_validate_key_offset(pheader, pheader->root_key)) {
      cm_msg(MERROR, "db_validate_db",
             "Warning: database corruption, root_key 0x%08X", pheader->root_key - sizeof(DATABASE_HEADER));
      return 0;
   }

   return db_validate_key(pheader, 1, "", (KEY *) ((char *) pheader + pheader->root_key));
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Open an online database
@param database_name     Database name.
@param database_size     Initial size of database if not existing
@param client_name       Name of this application
@param hDB          ODB handle obtained via cm_get_experiment_database().
@return DB_SUCCESS, DB_CREATED, DB_INVALID_NAME, DB_NO_MEMORY, 
        DB_MEMSIZE_MISMATCH, DB_NO_SEMAPHORE, DB_INVALID_PARAM,
        RPC_NET_ERROR
*/
INT db_open_database(const char *xdatabase_name, INT database_size, HNDLE * hDB, const char *client_name)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_OPEN_DATABASE, xdatabase_name, database_size, hDB, client_name);

#ifdef LOCAL_ROUTINES
   {
      INT i, status;
      HNDLE handle;
      DATABASE_CLIENT *pclient;
      BOOL shm_created;
      HNDLE shm_handle;
      DATABASE_HEADER *pheader;
      KEY *pkey;
      KEYLIST *pkeylist;
      FREE_DESCRIP *pfree;
      BOOL call_watchdog;
      DWORD timeout;
      char database_name[NAME_LENGTH];
      void *p;

      /* restrict name length */
      strlcpy(database_name, xdatabase_name, NAME_LENGTH);

      if (database_size < 0 || database_size > 10E7) {
         cm_msg(MERROR, "db_open_database", "invalid database size");
         return DB_INVALID_PARAM;
      }

      if (strlen(client_name) >= NAME_LENGTH) {
         cm_msg(MERROR, "db_open_database", "client name \'%s\' is longer than %d characters", client_name, NAME_LENGTH-1);
         return DB_INVALID_PARAM;
      }

      if (strchr(client_name, '/') != NULL) {
         cm_msg(MERROR, "db_open_database", "client name \'%s\' should not contain the slash \'/\' character", client_name);
         return DB_INVALID_PARAM;
      }

      /* allocate new space for the new database descriptor */
      if (_database_entries == 0) {
         _database = (DATABASE *) malloc(sizeof(DATABASE));
         memset(_database, 0, sizeof(DATABASE));
         if (_database == NULL) {
            *hDB = 0;
            return DB_NO_MEMORY;
         }

         _database_entries = 1;
         i = 0;
      } else {
         /* check if database already open */
         for (i = 0; i < _database_entries; i++)
            if (_database[i].attached && equal_ustring(_database[i].name, database_name)) {
               /* check if database belongs to this thread */
               if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_MTHREAD) {
                  if (_database[i].index == ss_gettid()) {
                     *hDB = i + 1;
                     return DB_SUCCESS;
                  }
               } else {
                  *hDB = i + 1;
                  return DB_SUCCESS;
               }
            }

         /* check for a deleted entry */
         for (i = 0; i < _database_entries; i++)
            if (!_database[i].attached)
               break;

         /* if not found, create new one */
         if (i == _database_entries) {
            _database = (DATABASE *) realloc(_database, sizeof(DATABASE) * (_database_entries + 1));
            memset(&_database[_database_entries], 0, sizeof(DATABASE));

            _database_entries++;
            if (_database == NULL) {
               _database_entries--;
               *hDB = 0;
               return DB_NO_MEMORY;
            }
         }
      }

      handle = (HNDLE) i;

      /* open shared memory region */
      status = ss_shm_open(database_name,
                           sizeof(DATABASE_HEADER) + 2 * ALIGN8(database_size / 2), &p, &shm_handle, TRUE);

      _database[(INT) handle].database_header = (DATABASE_HEADER *) p;
      if (status == SS_NO_MEMORY || status == SS_FILE_ERROR) {
         *hDB = 0;
         return DB_INVALID_NAME;
      }

      /* shortcut to header */
      pheader = _database[handle].database_header;

      /* save name */
      strcpy(_database[handle].name, database_name);

      shm_created = (status == SS_CREATED);

      /* clear memeory for debugging */
      /* memset(pheader, 0, sizeof(DATABASE_HEADER) + 2*ALIGN8(database_size/2)); */

      if (shm_created && pheader->name[0] == 0) {
         /* setup header info if database was created */
         memset(pheader, 0, sizeof(DATABASE_HEADER) + 2 * ALIGN8(database_size / 2));

         strcpy(pheader->name, database_name);
         pheader->version = DATABASE_VERSION;
         pheader->key_size = ALIGN8(database_size / 2);
         pheader->data_size = ALIGN8(database_size / 2);
         pheader->root_key = sizeof(DATABASE_HEADER);
         pheader->first_free_key = sizeof(DATABASE_HEADER);
         pheader->first_free_data = sizeof(DATABASE_HEADER) + pheader->key_size;

         /* set up free list */
         pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);
         pfree->size = pheader->key_size;
         pfree->next_free = 0;

         pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);
         pfree->size = pheader->data_size;
         pfree->next_free = 0;

         /* create root key */
         pkey = (KEY *) malloc_key(pheader, sizeof(KEY));

         /* set key properties */
         pkey->type = TID_KEY;
         pkey->num_values = 1;
         pkey->access_mode = MODE_READ | MODE_WRITE | MODE_DELETE;
         strcpy(pkey->name, "root");
         pkey->parent_keylist = 0;

         /* create keylist */
         pkeylist = (KEYLIST *) malloc_key(pheader, sizeof(KEYLIST));

         /* store keylist in data field */
         pkey->data = (POINTER_T) pkeylist - (POINTER_T) pheader;
         pkey->item_size = sizeof(KEYLIST);
         pkey->total_size = sizeof(KEYLIST);

         pkeylist->parent = (POINTER_T) pkey - (POINTER_T) pheader;
         pkeylist->num_keys = 0;
         pkeylist->first_key = 0;
      }

      /* check database version */
      if (pheader->version != DATABASE_VERSION) {
         cm_msg(MERROR, "db_open_database",
                "Different database format: Shared memory is %d, program is %d", pheader->version, DATABASE_VERSION);
         return DB_VERSION_MISMATCH;
      }

      /* create semaphore for the database */
      status = ss_semaphore_create(database_name, &(_database[handle].semaphore));
      if (status != SS_SUCCESS && status != SS_CREATED) {
         *hDB = 0;
         return DB_NO_SEMAPHORE;
      }
      _database[handle].lock_cnt = 0;

      /* first lock database */
      status = db_lock_database(handle + 1);
      if (status != DB_SUCCESS)
         return status;

      /*
         Now we have a DATABASE_HEADER, so let's setup a CLIENT
         structure in that database. The information there can also
         be seen by other processes.
       */

      /*
         update the client count
       */
      pheader->num_clients = 0;
      pheader->max_client_index = 0;
      for (i = 0; i < MAX_CLIENTS; i++) {
         if (pheader->client[i].pid == 0)
            continue;
         pheader->num_clients++;
         pheader->max_client_index = i + 1;
      }

      /*fprintf(stderr,"num_clients: %d, max_client: %d\n",pheader->num_clients,pheader->max_client_index); */

      /* remove dead clients */

#ifdef OS_UNIX
#ifdef ESRCH
      /* Only enable this for systems that define ESRCH and hope that
         they also support kill(pid,0) */
      for (i = 0; i < MAX_CLIENTS; i++) {
         int k;

         errno = 0;
         kill(pheader->client[i].pid, 0);
         if (errno == ESRCH) {
	   char client_name_tmp[NAME_LENGTH];
	   int client_pid;

	   strlcpy(client_name_tmp, pheader->client[i].name, sizeof(client_name_tmp));
	   client_pid = pheader->client[i].pid;

            /* decrement notify_count for open records and clear exclusive mode */
            for (k = 0; k < pheader->client[i].max_index; k++)
               if (pheader->client[i].open_record[k].handle) {
                  pkey = (KEY *) ((char *) pheader + pheader->client[i].open_record[k].handle);
                  if (pkey->notify_count > 0)
                     pkey->notify_count--;

                  if (pheader->client[i].open_record[k].access_mode & MODE_WRITE)
                     db_set_mode(handle + 1, pheader->client[i].open_record[k].handle,
                                 (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), 2);
               }

            /* clear entry from client structure in database header */
            memset(&(pheader->client[i]), 0, sizeof(DATABASE_CLIENT));

            cm_msg(MERROR, "db_open_database", "Removed ODB client \'%s\', index %d because process pid %d does not exists", client_name_tmp, i, client_pid);
         }
      }
#endif
#endif

      /*
         Look for empty client slot
       */
      for (i = 0; i < MAX_CLIENTS; i++)
         if (pheader->client[i].pid == 0)
            break;

      if (i == MAX_CLIENTS) {
         db_unlock_database(handle + 1);
         *hDB = 0;
         cm_msg(MERROR, "db_open_database", "maximum number of clients exceeded");
         return DB_NO_SLOT;
      }

      /* store slot index in _database structure */
      _database[handle].client_index = i;

      /*
         Save the index of the last client of that database so that later only
         the clients 0..max_client_index-1 have to be searched through.
       */
      pheader->num_clients++;
      if (i + 1 > pheader->max_client_index)
         pheader->max_client_index = i + 1;

      /* setup database header and client structure */
      pclient = &pheader->client[i];

      memset(pclient, 0, sizeof(DATABASE_CLIENT));
      /* use client name previously set by bm_set_name */
      strlcpy(pclient->name, client_name, sizeof(pclient->name));
      pclient->pid = ss_getpid();
      pclient->num_open_records = 0;

      ss_suspend_get_port(&pclient->port);

      pclient->last_activity = ss_millitime();

      cm_get_watchdog_params(&call_watchdog, &timeout);
      pclient->watchdog_timeout = timeout;

      /* check ODB for corruption */
      if (!db_validate_db(pheader)) {
         /* do not treat corrupted odb as a fatal error- allow the user
            to preceed at own risk- the database is already corrupted,
            so no further harm can possibly be made. */
         /*
            db_unlock_database(handle + 1);
            *hDB = 0;
            return DB_CORRUPTED;
          */
      }

      /* setup _database entry */
      _database[handle].database_data = _database[handle].database_header + 1;
      _database[handle].attached = TRUE;
      _database[handle].shm_handle = shm_handle;
      _database[handle].protect = FALSE;

      /* remember to which connection acutal buffer belongs */
      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE)
         _database[handle].index = rpc_get_server_acception();
      else
         _database[handle].index = ss_gettid();

      *hDB = (handle + 1);

      /* setup dispatcher for updated records */
      ss_suspend_set_dispatch(CH_IPC, 0, (int (*)(void)) cm_dispatch_ipc);

      db_unlock_database(handle + 1);

      if (shm_created)
         return DB_CREATED;
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Close a database
@param   hDB          ODB handle obtained via cm_get_experiment_database().
@return DB_SUCCESS, DB_INVALID_HANDLE, RPC_NET_ERROR 
*/
INT db_close_database(HNDLE hDB)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_CLOSE_DATABASE, hDB);

#ifdef LOCAL_ROUTINES
   else {
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      INT idx, destroy_flag, i, j;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_close_database", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /*
         Check if database was opened by current thread. This is necessary
         in the server process where one thread may not close the database
         of other threads.
       */

      /* first lock database */
      db_lock_database(hDB);

      idx = _database[hDB - 1].client_index;
      pheader = _database[hDB - 1].database_header;
      pclient = &pheader->client[idx];

      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
          _database[hDB - 1].index != rpc_get_server_acception()) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE && _database[hDB - 1].index != ss_gettid()) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_close_database", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /* close all open records */
      for (i = 0; i < pclient->max_index; i++)
         if (pclient->open_record[i].handle)
            db_remove_open_record(hDB, pclient->open_record[i].handle, FALSE);

      /* mark entry in _database as empty */
      _database[hDB - 1].attached = FALSE;

      /* clear entry from client structure in database header */
      memset(&(pheader->client[idx]), 0, sizeof(DATABASE_CLIENT));

      /* calculate new max_client_index entry */
      for (i = MAX_CLIENTS - 1; i >= 0; i--)
         if (pheader->client[i].pid != 0)
            break;
      pheader->max_client_index = i + 1;

      /* count new number of clients */
      for (i = MAX_CLIENTS - 1, j = 0; i >= 0; i--)
         if (pheader->client[i].pid != 0)
            j++;
      pheader->num_clients = j;

      destroy_flag = (pheader->num_clients == 0);

      /* flush shared memory to disk */
      ss_shm_flush(pheader->name, pheader, sizeof(DATABASE_HEADER) + 2 * pheader->data_size, _database[hDB - 1].shm_handle);

      /* unmap shared memory, delete it if we are the last */
      ss_shm_close(pheader->name, pheader, _database[hDB - 1].shm_handle, destroy_flag);

      /* unlock database */
      db_unlock_database(hDB);

      /* delete semaphore */
      ss_semaphore_delete(_database[hDB - 1].semaphore, destroy_flag);

      /* update _database_entries */
      if (hDB == _database_entries)
         _database_entries--;

      if (_database_entries > 0)
         _database = (DATABASE *) realloc(_database, sizeof(DATABASE) * (_database_entries));
      else {
         free(_database);
         _database = NULL;
      }

      /* if we are the last one, also delete other semaphores */
      if (destroy_flag) {
         extern INT _semaphore_elog, _semaphore_alarm, _semaphore_history, _semaphore_msg;

         if (_semaphore_elog)
            ss_semaphore_delete(_semaphore_elog, TRUE);
         if (_semaphore_alarm)
            ss_semaphore_delete(_semaphore_alarm, TRUE);
         if (_semaphore_history)
            ss_semaphore_delete(_semaphore_history, TRUE);
         if (_semaphore_msg)
            ss_semaphore_delete(_semaphore_msg, TRUE);
      }

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_flush_database(HNDLE hDB)
/********************************************************************\

  Routine: db_flush_database

  Purpose: Flushes the shared memory of a database to its disk file.

  Input:
    HNDLE  hDB              Handle to the database, which is used as
                            an index to the _database array.

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    RPC_NET_ERROR           Network error

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_FLUSH_DATABASE, hDB);

#ifdef LOCAL_ROUTINES
   else {
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      INT idx;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_close_database", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /*
         Check if database was opened by current thread. This is necessary
         in the server process where one thread may not close the database
         of other threads.
       */

      db_lock_database(hDB);
      idx = _database[hDB - 1].client_index;
      pheader = _database[hDB - 1].database_header;
      pclient = &pheader->client[idx];

      if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
          _database[hDB - 1].index != rpc_get_server_acception()) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE && _database[hDB - 1].index != ss_gettid()) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_close_database", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /* flush shared memory to disk */
      ss_shm_flush(pheader->name, pheader, sizeof(DATABASE_HEADER) + 2 * pheader->data_size, _database[hDB - 1].shm_handle);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_close_all_databases(void)
/********************************************************************\

  Routine: db_close_all_databases

  Purpose: Close all open databases and open records

  Input:
    none

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion

\********************************************************************/
{
   INT status;

   if (rpc_is_remote()) {
      status = rpc_call(RPC_DB_CLOSE_ALL_DATABASES);
      if (status != DB_SUCCESS)
         return status;
   }
#ifdef LOCAL_ROUTINES
   {
      INT i;

      for (i = _database_entries; i > 0; i--)
         db_close_database(i);
   }
#endif                          /* LOCAL_ROUTINES */

   return db_close_all_records();
}

/*------------------------------------------------------------------*/
INT db_set_client_name(HNDLE hDB, const char *client_name)
/********************************************************************\

  Routine: db_set_client_name

  Purpose: Set client name for a database. Used by cm_connect_experiment
           if a client name is duplicate and changed.

  Input:
    INT  hDB                Handle to database
    char *client_name       Name of this application

  Output:

  Function value:
    DB_SUCCESS              Successful completion
    RPC_NET_ERROR           Network error

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_CLIENT_NAME, hDB, client_name);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      INT idx;

      idx = _database[hDB - 1].client_index;
      pheader = _database[hDB - 1].database_header;
      pclient = &pheader->client[idx];

      strcpy(pclient->name, client_name);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Lock a database for exclusive access via system semaphore calls.
@param hDB   Handle to the database to lock
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TIMEOUT
*/

/* Define CHECK_THREAD_ID to enable thread checking. This ensures that
   only the main thread does the locking/unlocking of the ODB. In multi-thread
   applications (slow control front-end for example), unexpected behaviour can
   occur if several threads lock/unlock the ODB at the same time, since the midas
   API is not thread safe. */
#ifdef CHECK_THREAD_ID
static int _lock_tid = 0;
#endif

INT db_lock_database(HNDLE hDB)
{
#ifdef LOCAL_ROUTINES
   int status;
   void *p;

#ifdef CHECK_THREAD_ID
   if (_lock_tid == 0)
      _lock_tid = ss_gettid();

   assert(_lock_tid == ss_gettid());
#endif

   if (hDB > _database_entries || hDB <= 0) {
      cm_msg(MERROR, "db_lock_database", "invalid database handle, aborting...");
      abort();
      return DB_INVALID_HANDLE;
   }

   if (_database[hDB - 1].protect && _database[hDB - 1].database_header != NULL) {
      cm_msg(MERROR, "db_lock_database", "internal error: DB already locked, aborting...");
      abort();
      return DB_NO_SEMAPHORE;
   }

   _database[hDB - 1].lock_cnt++;

   if (_database[hDB - 1].lock_cnt == 1) {
      /* wait max. 5 minutes for semaphore (required if locking process is being debugged) */
      status = ss_semaphore_wait_for(_database[hDB - 1].semaphore, 5 * 60 * 1000);
      if (status == SS_TIMEOUT) {
         cm_msg(MERROR, "db_lock_database", "timeout obtaining lock for database, exiting...");
         exit(1);
         return DB_TIMEOUT;
      }
      if (status != SS_SUCCESS) {
         cm_msg(MERROR, "db_lock_database", "cannot lock database, ss_semaphore_wait_for() status %d, aborting...", status);
         abort();
         return DB_NO_SEMAPHORE;
      }
   }

#ifdef CHECK_LOCK_COUNT
   {
      char str[256];

      sprintf(str, "db_lock_database, lock_cnt=%d", _database[hDB - 1].lock_cnt);
      ss_stack_history_entry(str);
   }
#endif

   if (_database[hDB - 1].protect) {
      if (_database[hDB - 1].database_header == NULL) {
         ss_shm_unprotect(_database[hDB - 1].shm_handle, &p);
         _database[hDB - 1].database_header = (DATABASE_HEADER *) p;
      }
   }
#endif                          /* LOCAL_ROUTINES */
   return DB_SUCCESS;
}

/********************************************************************/
/**
Unlock a database via system semaphore calls.
@param hDB   Handle to the database to unlock
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_unlock_database(HNDLE hDB)
{

#ifdef LOCAL_ROUTINES

#ifdef CHECK_THREAD_ID
   if (_lock_tid == 0)
      _lock_tid = ss_gettid();

   assert(_lock_tid == ss_gettid());
#endif

   if (hDB > _database_entries || hDB <= 0) {
      cm_msg(MERROR, "db_unlock_database", "invalid database handle");
      return DB_INVALID_HANDLE;
   }
#ifdef CHECK_LOCK_COUNT
   {
      char str[256];

      sprintf(str, "db_unlock_database, lock_cnt=%d", _database[hDB - 1].lock_cnt);
      ss_stack_history_entry(str);
   }
#endif

   if (_database[hDB - 1].lock_cnt == 1)
      ss_semaphore_release(_database[hDB - 1].semaphore);

   if (_database[hDB - 1].protect) {
      ss_shm_protect(_database[hDB - 1].shm_handle, _database[hDB - 1].database_header);
      _database[hDB - 1].database_header = NULL;
   }

   assert(_database[hDB - 1].lock_cnt > 0);
   _database[hDB - 1].lock_cnt--;

#endif                          /* LOCAL_ROUTINES */
   return DB_SUCCESS;
}

/********************************************************************/

INT db_get_lock_cnt(HNDLE hDB)
{
#ifdef LOCAL_ROUTINES

   /* return zero if no ODB is open or we run remotely */
   if (_database_entries == 0)
      return 0;

   if (hDB > _database_entries || hDB <= 0) {
      cm_msg(MERROR, "db_lock_database", "invalid database handle, aborting...");
      abort();
      return DB_INVALID_HANDLE;
   }

   return _database[hDB - 1].lock_cnt;
#else
   return 0;
#endif
}

/********************************************************************/
/**
Protect a database for read/write access outside of the \b db_xxx functions
@param hDB          ODB handle obtained via cm_get_experiment_database().
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_protect_database(HNDLE hDB)
{
#ifdef LOCAL_ROUTINES
   if (hDB > _database_entries || hDB <= 0) {
      cm_msg(MERROR, "db_unlock_database", "invalid database handle");
      return DB_INVALID_HANDLE;
   }

   _database[hDB - 1].protect = TRUE;
   ss_shm_protect(_database[hDB - 1].shm_handle, _database[hDB - 1].database_header);
   _database[hDB - 1].database_header = NULL;
#endif                          /* LOCAL_ROUTINES */
   return DB_SUCCESS;
}

/*---- helper routines ---------------------------------------------*/

const char *extract_key(const char *key_list, char *key_name, int key_name_length)
{
   int i = 0;

   if (*key_list == '/')
      key_list++;

   while (*key_list && *key_list != '/' && ++i < key_name_length)
      *key_name++ = *key_list++;
   *key_name = 0;

   return key_list;
}

BOOL equal_ustring(const char *str1, const char *str2)
{
   if (str1 == NULL && str2 != NULL)
      return FALSE;
   if (str1 != NULL && str2 == NULL)
      return FALSE;
   if (str1 == NULL && str2 == NULL)
      return TRUE;
   if (strlen(str1) != strlen(str2))
      return FALSE;

   while (*str1)
      if (toupper(*str1++) != toupper(*str2++))
         return FALSE;

   if (*str2)
      return FALSE;

   return TRUE;
}

/********************************************************************/
/**
Create a new key in a database
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey  Key handle to start with, 0 for root
@param key_name    Name of key in the form "/key/key/key"
@param type        Type of key, one of TID_xxx (see @ref Midas_Data_Types)
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_INVALID_PARAM, DB_FULL, DB_KEY_EXIST, DB_NO_ACCESS
*/
INT db_create_key(HNDLE hDB, HNDLE hKey, const char *key_name, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_CREATE_KEY, hDB, hKey, key_name, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey, *pprev_key, *pkeyparent;
      const char *pkey_name;
      char str[MAX_STRING_LENGTH];
      INT i;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_create_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_create_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /* check type */
      if (type <= 0 || type >= TID_LAST) {
         cm_msg(MERROR, "db_create_key", "invalid key type %d for \'%s\'", type, key_name);
         return DB_INVALID_PARAM;
      }

      /* lock database */
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_create_key", "key has no subkeys");
         return DB_NO_KEY;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      pkey_name = key_name;
      do {
         /* extract single key from key_name */
         pkey_name = extract_key(pkey_name, str, sizeof(str));

         /* do not allow empty names, like '/dir/dir//dir/' */
         if (str[0] == 0) {
            db_unlock_database(hDB);
            return DB_INVALID_PARAM;
         }

         /* check if parent or current directory */
         if (strcmp(str, "..") == 0) {
            if (pkey->parent_keylist) {
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
               pkey = (KEY *) ((char *) pheader + pkeylist->parent);
            }
            continue;
         }
         if (strcmp(str, ".") == 0)
            continue;

         /* check if key is in keylist */
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);
         pprev_key = NULL;

         for (i = 0; i < pkeylist->num_keys; i++) {
            if (!db_validate_key_offset(pheader, pkey->next_key)) {
               db_unlock_database(hDB);
               cm_msg(MERROR, "db_create_key",
                      "Warning: database corruption, key %s, next_key 0x%08X",
                      key_name, pkey->next_key - sizeof(DATABASE_HEADER));
               return DB_CORRUPTED;
            }

            if (equal_ustring(str, pkey->name))
               break;

            pprev_key = pkey;
            pkey = (KEY *) ((char *) pheader + pkey->next_key);
         }

         if (i == pkeylist->num_keys) {
            /* not found: create new key */

            /* check parent for write access */
            pkeyparent = (KEY *) ((char *) pheader + pkeylist->parent);
            if (!(pkeyparent->access_mode & MODE_WRITE) || (pkeyparent->access_mode & MODE_EXCLUSIVE)) {
               db_unlock_database(hDB);
               return DB_NO_ACCESS;
            }

            pkeylist->num_keys++;

            if (*pkey_name == '/' || type == TID_KEY) {
               /* create new key with keylist */
               pkey = (KEY *) malloc_key(pheader, sizeof(KEY));

               if (pkey == NULL) {
                  db_unlock_database(hDB);
                  cm_msg(MERROR, "db_create_key", "online database full");
                  return DB_FULL;
               }

               /* append key to key list */
               if (pprev_key)
                  pprev_key->next_key = (POINTER_T) pkey - (POINTER_T) pheader;
               else
                  pkeylist->first_key = (POINTER_T) pkey - (POINTER_T) pheader;

               /* set key properties */
               pkey->type = TID_KEY;
               pkey->num_values = 1;
               pkey->access_mode = MODE_READ | MODE_WRITE | MODE_DELETE;
               strlcpy(pkey->name, str, sizeof(pkey->name));
               pkey->parent_keylist = (POINTER_T) pkeylist - (POINTER_T) pheader;

               /* find space for new keylist */
               pkeylist = (KEYLIST *) malloc_key(pheader, sizeof(KEYLIST));

               if (pkeylist == NULL) {
                  db_unlock_database(hDB);
                  cm_msg(MERROR, "db_create_key", "online database full");
                  return DB_FULL;
               }

               /* store keylist in data field */
               pkey->data = (POINTER_T) pkeylist - (POINTER_T) pheader;
               pkey->item_size = sizeof(KEYLIST);
               pkey->total_size = sizeof(KEYLIST);

               pkeylist->parent = (POINTER_T) pkey - (POINTER_T) pheader;
               pkeylist->num_keys = 0;
               pkeylist->first_key = 0;
            } else {
               /* create new key with data */
               pkey = (KEY *) malloc_key(pheader, sizeof(KEY));

               if (pkey == NULL) {
                  db_unlock_database(hDB);
                  cm_msg(MERROR, "db_create_key", "online database full");
                  return DB_FULL;
               }

               /* append key to key list */
               if (pprev_key)
                  pprev_key->next_key = (POINTER_T) pkey - (POINTER_T) pheader;
               else
                  pkeylist->first_key = (POINTER_T) pkey - (POINTER_T) pheader;

               pkey->type = type;
               pkey->num_values = 1;
               pkey->access_mode = MODE_READ | MODE_WRITE | MODE_DELETE;
               strlcpy(pkey->name, str, sizeof(pkey->name));
               pkey->parent_keylist = (POINTER_T) pkeylist - (POINTER_T) pheader;

               /* zero data */
               if (type != TID_STRING && type != TID_LINK) {
                  pkey->item_size = rpc_tid_size(type);
                  pkey->data = (POINTER_T) malloc_data(pheader, pkey->item_size);
                  pkey->total_size = pkey->item_size;

                  if (pkey->data == 0) {
                     db_unlock_database(hDB);
                     cm_msg(MERROR, "db_create_key", "online database full");
                     return DB_FULL;
                  }

                  pkey->data -= (POINTER_T) pheader;
               } else {
                  /* first data is empty */
                  pkey->item_size = 0;
                  pkey->total_size = 0;
                  pkey->data = 0;
               }
            }
         } else {
            /* key found: descend */

            /* resolve links */
            if (pkey->type == TID_LINK && pkey_name[0]) {
               /* copy destination, strip '/' */
               strcpy(str, (char *) pheader + pkey->data);
               if (str[strlen(str) - 1] == '/')
                  str[strlen(str) - 1] = 0;

               /* append rest of key name */
               strcat(str, pkey_name);

               db_unlock_database(hDB);

               return db_create_key(hDB, 0, str, type);
            }

            if (!(*pkey_name == '/')) {
               db_unlock_database(hDB);

               if ((WORD) pkey->type != type)
                  cm_msg(MERROR, "db_create_key", "redefinition of key type mismatch");

               return DB_KEY_EXIST;
            }

            if (pkey->type != TID_KEY) {
               db_unlock_database(hDB);
               cm_msg(MERROR, "db_create_key", "key used with value and as parent key");
               return DB_KEY_EXIST;
            }

            pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);
         }
      } while (*pkey_name == '/');

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Create a link to a key or set the destination of and existing link.
@param hDB           ODB handle obtained via cm_get_experiment_database().
@param hKey          Key handle to start with, 0 for root
@param link_name     Name of key in the form "/key/key/key"
@param destination   Destination of link in the form "/key/key/key"
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_FULL, DB_KEY_EXIST, DB_NO_ACCESS
*/
INT db_create_link(HNDLE hDB, HNDLE hKey, const char *link_name, const char *destination)
{
   HNDLE hkey;
   int status;

   if (rpc_is_remote())
      return rpc_call(RPC_DB_CREATE_LINK, hDB, hKey, link_name, destination);

   /* check if destination exists */
   status = db_find_key(hDB, hKey, destination, &hkey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_create_link", "Link destination \"%s\" does not exist", destination);
      return DB_NO_KEY;
   }

   return db_set_value(hDB, hKey, link_name, destination, strlen(destination) + 1, 1, TID_LINK);
}

/********************************************************************/
/**
Delete a subtree, using level information (only called internally by db_delete_key())
@internal 
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey  Key handle to start with, 0 for root
@param level            Recursion level, must be zero when
@param follow_links     Follow links when TRUE called from a user routine
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_OPEN_RECORD
*/
INT db_delete_key1(HNDLE hDB, HNDLE hKey, INT level, BOOL follow_links)
{
#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey, *pnext_key, *pkey_tmp;
      HNDLE hKeyLink;
      BOOL deny_delete;
      INT status;
      BOOL locked = FALSE;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_delete_key1", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_delete_key1", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_delete_key1", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      /* lock database at the top level */
      if (level == 0) {
         db_lock_database(hDB);
         locked = TRUE;
      }

      pheader = _database[hDB - 1].database_header;

      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         if (locked)
            db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check if someone has opened key or parent */
      if (level == 0)
         do {
            if (pkey->notify_count) {
               if (locked)
                  db_unlock_database(hDB);
               return DB_OPEN_RECORD;
            }

            if (pkey->parent_keylist == 0)
               break;

            pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
            pkey = (KEY *) ((char *) pheader + pkeylist->parent);
         } while (TRUE);

      pkey = (KEY *) ((char *) pheader + hKey);
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      deny_delete = FALSE;

      /* first recures subtree for keys */
      if (pkey->type == TID_KEY && pkeylist->first_key) {
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

         do {
            pnext_key = (KEY *) (POINTER_T) pkey->next_key;

            status = db_delete_key1(hDB, (POINTER_T) pkey - (POINTER_T) pheader, level + 1, follow_links);

            if (status == DB_NO_ACCESS)
               deny_delete = TRUE;

            if (pnext_key)
               pkey = (KEY *) ((char *) pheader + (POINTER_T) pnext_key);
         } while (pnext_key);
      }

      /* follow links if requested */
      if (pkey->type == TID_LINK && follow_links) {
         status = db_find_key1(hDB, 0, (char *) pheader + pkey->data, &hKeyLink);
         if (status == DB_SUCCESS && follow_links < 100)
            db_delete_key1(hDB, hKeyLink, level + 1, follow_links + 1);

         if (follow_links == 100)
            cm_msg(MERROR, "db_delete_key1", "try to delete cyclic link");
      }

      pkey = (KEY *) ((char *) pheader + hKey);

      /* return if key was already deleted by cyclic link */
      if (pkey->parent_keylist == 0) {
         if (locked)
            db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      /* now delete key */
      if (hKey != pheader->root_key) {
         if (!(pkey->access_mode & MODE_DELETE) || deny_delete) {
            if (locked)
               db_unlock_database(hDB);
            return DB_NO_ACCESS;
         }

         if (pkey->notify_count) {
            if (locked)
               db_unlock_database(hDB);
            return DB_OPEN_RECORD;
         }

         /* delete key data */
         if (pkey->type == TID_KEY)
            free_key(pheader, (char *) pheader + pkey->data, pkey->total_size);
         else
            free_data(pheader, (char *) pheader + pkey->data, pkey->total_size);

         /* unlink key from list */
         pnext_key = (KEY *) (POINTER_T) pkey->next_key;
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);

         if ((KEY *) ((char *) pheader + pkeylist->first_key) == pkey) {
            /* key is first in list */
            pkeylist->first_key = (POINTER_T) pnext_key;
         } else {
            /* find predecessor */
            pkey_tmp = (KEY *) ((char *) pheader + pkeylist->first_key);
            while ((KEY *) ((char *) pheader + pkey_tmp->next_key) != pkey)
               pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);
            pkey_tmp->next_key = (POINTER_T) pnext_key;
         }

         /* delete key */
         free_key(pheader, pkey, sizeof(KEY));
         pkeylist->num_keys--;
      }

      if (locked)
         db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Delete a subtree in a database starting from a key (including this key).
\code
...
    status = db_find_link(hDB, 0, str, &hkey);
    if (status != DB_SUCCESS)
    {
      cm_msg(MINFO,"my_delete"," "Cannot find key %s", str);
      return;
    }

    status = db_delete_key(hDB, hkey, FALSE);
    if (status != DB_SUCCESS)
    {
      cm_msg(MERROR,"my_delete"," "Cannot delete key %s", str);
      return;
    }
  ...
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         for key where search starts, zero for root.
@param follow_links Follow links when TRUE.
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_ACCESS, DB_OPEN_RECORD
*/
INT db_delete_key(HNDLE hDB, HNDLE hKey, BOOL follow_links)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_DELETE_KEY, hDB, hKey, follow_links);

   return db_delete_key1(hDB, hKey, 0, follow_links);
}

/********************************************************************/
/**
Returns key handle for a key with a specific name.

Keys can be accessed by their name including the directory
or by a handle. A key handle is an internal offset to the shared memory
where the ODB lives and allows a much faster access to a key than via its
name.

The function db_find_key() must be used to convert a key name to a handle.
Most other database functions use this key handle in various operations.
\code
HNDLE hkey, hsubkey;
// use full name, start from root
db_find_key(hDB, 0, "/Runinfo/Run number", &hkey);
// start from subdirectory
db_find_key(hDB, 0, "/Runinfo", &hkey);
db_find_key(hdb, hkey, "Run number", &hsubkey);
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param key_name Name of key to search, can contain directories.
@param subhKey Returned handle of key, zero if key cannot be found.
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_ACCESS, DB_NO_KEY
*/
INT db_find_key(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_FIND_KEY, hDB, hKey, key_name, subhKey);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      const char *pkey_name;
      char str[MAX_STRING_LENGTH];
      INT i, status;

      *subhKey = 0;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_find_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_find_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;

      if (!hKey)
         hKey = pheader->root_key;

      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type < 1 || pkey->type >= TID_LAST) {
         *subhKey = 0;
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_find_key", "hkey %d invalid key type %d", hKey, pkey->type);
         return DB_NO_KEY;
      }

      if (pkey->type != TID_KEY) {
         db_unlock_database(hDB);
         db_get_path(hDB, hKey, str, sizeof(str));
         *subhKey = 0;
         cm_msg(MERROR, "db_find_key", "key \"%s\" has no subkeys", str);
         return DB_NO_KEY;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (key_name[0] == 0 || strcmp(key_name, "/") == 0) {
         if (!(pkey->access_mode & MODE_READ)) {
            *subhKey = 0;
            db_unlock_database(hDB);
            return DB_NO_ACCESS;
         }

         *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;

         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      pkey_name = key_name;
      do {
         /* extract single subkey from key_name */
         pkey_name = extract_key(pkey_name, str, sizeof(str));

         /* strip trailing '[n]' */
         if (strchr(str, '[') && str[strlen(str) - 1] == ']')
            *strchr(str, '[') = 0;

         /* check if parent or current directory */
         if (strcmp(str, "..") == 0) {
            if (pkey->parent_keylist) {
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
               pkey = (KEY *) ((char *) pheader + pkeylist->parent);
            }
            continue;
         }
         if (strcmp(str, ".") == 0)
            continue;

         /* check if key is in keylist */
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

         for (i = 0; i < pkeylist->num_keys; i++) {
            if (pkey->name[0] == 0 || !db_validate_key_offset(pheader, pkey->next_key)) {
               db_unlock_database(hDB);
               cm_msg(MERROR, "db_find_key",
                      "Warning: database corruption, key %s, next_key 0x%08X",
                      key_name, pkey->next_key - sizeof(DATABASE_HEADER));
               *subhKey = 0;
               return DB_CORRUPTED;
            }

            if (equal_ustring(str, pkey->name))
               break;

            pkey = (KEY *) ((char *) pheader + pkey->next_key);
         }

         if (i == pkeylist->num_keys) {
            *subhKey = 0;
            db_unlock_database(hDB);
            return DB_NO_KEY;
         }

         /* resolve links */
         if (pkey->type == TID_LINK) {
            /* copy destination, strip '/' */
            strcpy(str, (char *) pheader + pkey->data);
            if (str[strlen(str) - 1] == '/')
               str[strlen(str) - 1] = 0;

            /* if link is pointer to array index, return link instead of destination */
            if (str[strlen(str) - 1] == ']')
               break;

            /* append rest of key name if existing */
            if (pkey_name[0]) {
               strcat(str, pkey_name);
               db_unlock_database(hDB);
               return db_find_key(hDB, 0, str, subhKey);
            } else {
               /* if last key in chain is a link, return its destination */
               db_unlock_database(hDB);
               status = db_find_link(hDB, 0, str, subhKey);
               if (status == DB_NO_KEY)
                  return DB_INVALID_LINK;
               return status;
            }
         }

         /* key found: check if last in chain */
         if (*pkey_name == '/') {
            if (pkey->type != TID_KEY) {
               *subhKey = 0;
               db_unlock_database(hDB);
               return DB_NO_KEY;
            }
         }

         /* descend one level */
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      } while (*pkey_name == '/' && *(pkey_name + 1));

      *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_find_key1(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey)
/********************************************************************\

  Routine: db_find_key1

  Purpose: Same as db_find_key, but without DB locking

  Input:
    HNDLE  bufer_handle     Handle to the database
    HNDLE  hKey             Key handle to start the search
    char   *key_name        Name of key in the form "/key/key/key"

  Output:
    INT    *handle          Key handle

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_KEY               Key doesn't exist
    DB_NO_ACCESS            No access to read key

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_FIND_KEY, hDB, hKey, key_name, subhKey);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      const char *pkey_name;
      char str[MAX_STRING_LENGTH];
      INT i;

      *subhKey = 0;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_find_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_find_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != TID_KEY) {
         cm_msg(MERROR, "db_find_key", "key has no subkeys");
         *subhKey = 0;
         return DB_NO_KEY;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (key_name[0] == 0 || strcmp(key_name, "/") == 0) {
         if (!(pkey->access_mode & MODE_READ)) {
            *subhKey = 0;
            return DB_NO_ACCESS;
         }

         *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;

         return DB_SUCCESS;
      }

      pkey_name = key_name;
      do {
         /* extract single subkey from key_name */
         pkey_name = extract_key(pkey_name, str, sizeof(str));

         /* check if parent or current directory */
         if (strcmp(str, "..") == 0) {
            if (pkey->parent_keylist) {
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
               pkey = (KEY *) ((char *) pheader + pkeylist->parent);
            }
            continue;
         }
         if (strcmp(str, ".") == 0)
            continue;

         /* check if key is in keylist */
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

         for (i = 0; i < pkeylist->num_keys; i++) {
            if (equal_ustring(str, pkey->name))
               break;

            pkey = (KEY *) ((char *) pheader + pkey->next_key);
         }

         if (i == pkeylist->num_keys) {
            *subhKey = 0;
            return DB_NO_KEY;
         }

         /* resolve links */
         if (pkey->type == TID_LINK) {
            /* copy destination, strip '/' */
            strcpy(str, (char *) pheader + pkey->data);
            if (str[strlen(str) - 1] == '/')
               str[strlen(str) - 1] = 0;

            /* append rest of key name if existing */
            if (pkey_name[0]) {
               strcat(str, pkey_name);
               return db_find_key1(hDB, 0, str, subhKey);
            } else {
               /* if last key in chain is a link, return its destination */
               return db_find_link1(hDB, 0, str, subhKey);
            }
         }

         /* key found: check if last in chain */
         if (*pkey_name == '/') {
            if (pkey->type != TID_KEY) {
               *subhKey = 0;
               return DB_NO_KEY;
            }
         }

         /* descend one level */
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      } while (*pkey_name == '/' && *(pkey_name + 1));

      *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_find_link(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey)
/********************************************************************\

  Routine: db_find_link

  Purpose: Find a key or link by name and return its handle
           (internal address). The only difference of this routine
           compared with db_find_key is that if the LAST key in
           the chain is a link, it is NOT evaluated. Links not being
           the last in the chain are evaluated.

  Input:
    HNDLE  bufer_handle     Handle to the database
    HNDLE  hKey       Key handle to start the search
    char   *key_name        Name of key in the form "/key/key/key"

  Output:
    INT    *handle          Key handle

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_KEY               Key doesn't exist
    DB_NO_ACCESS            No access to read key

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_FIND_LINK, hDB, hKey, key_name, subhKey);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      const char *pkey_name;
      char str[MAX_STRING_LENGTH];
      INT i;

      *subhKey = 0;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_find_link", "Invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_find_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_find_link", "key has no subkeys");
         return DB_NO_KEY;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (key_name[0] == 0 || strcmp(key_name, "/") == 0) {
         if (!(pkey->access_mode & MODE_READ)) {
            *subhKey = 0;
            db_unlock_database(hDB);
            return DB_NO_ACCESS;
         }

         *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;

         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      pkey_name = key_name;
      do {
         /* extract single subkey from key_name */
         pkey_name = extract_key(pkey_name, str, sizeof(str));

         /* check if parent or current directory */
         if (strcmp(str, "..") == 0) {
            if (pkey->parent_keylist) {
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
               pkey = (KEY *) ((char *) pheader + pkeylist->parent);
            }
            continue;
         }
         if (strcmp(str, ".") == 0)
            continue;

         /* check if key is in keylist */
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

         for (i = 0; i < pkeylist->num_keys; i++) {
            if (!db_validate_key_offset(pheader, pkey->next_key)) {
               db_unlock_database(hDB);
               cm_msg(MERROR, "db_find_link",
                      "Warning: database corruption, key \"%s\", next_key 0x%08X",
                      key_name, pkey->next_key - sizeof(DATABASE_HEADER));
               *subhKey = 0;
               return DB_CORRUPTED;
            }

            if (equal_ustring(str, pkey->name))
               break;

            pkey = (KEY *) ((char *) pheader + pkey->next_key);
         }

         if (i == pkeylist->num_keys) {
            *subhKey = 0;
            db_unlock_database(hDB);
            return DB_NO_KEY;
         }

         /* resolve links if not last in chain */
         if (pkey->type == TID_LINK && *pkey_name == '/') {
            /* copy destination, strip '/' */
            strcpy(str, (char *) pheader + pkey->data);
            if (str[strlen(str) - 1] == '/')
               str[strlen(str) - 1] = 0;

            /* append rest of key name */
            strcat(str, pkey_name);
            db_unlock_database(hDB);
            return db_find_link(hDB, 0, str, subhKey);
         }

         /* key found: check if last in chain */
         if ((*pkey_name == '/')) {
            if (pkey->type != TID_KEY) {
               *subhKey = 0;
               db_unlock_database(hDB);
               return DB_NO_KEY;
            }
         }

         /* descend one level */
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      } while (*pkey_name == '/' && *(pkey_name + 1));

      *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_find_link1(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey)
/********************************************************************\

  Routine: db_find_link1

  Purpose: Same ad db_find_link, but without DB locking

  Input:
    HNDLE  bufer_handle     Handle to the database
    HNDLE  hKey       Key handle to start the search
    char   *key_name        Name of key in the form "/key/key/key"

  Output:
    INT    *handle          Key handle

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_KEY               Key doesn't exist
    DB_NO_ACCESS            No access to read key

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_FIND_LINK, hDB, hKey, key_name, subhKey);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      const char *pkey_name;
      char str[MAX_STRING_LENGTH];
      INT i;

      *subhKey = 0;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_find_link", "Invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_find_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != TID_KEY) {
         cm_msg(MERROR, "db_find_link", "key has no subkeys");
         return DB_NO_KEY;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (key_name[0] == 0 || strcmp(key_name, "/") == 0) {
         if (!(pkey->access_mode & MODE_READ)) {
            *subhKey = 0;
            return DB_NO_ACCESS;
         }

         *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;

         return DB_SUCCESS;
      }

      pkey_name = key_name;
      do {
         /* extract single subkey from key_name */
         pkey_name = extract_key(pkey_name, str, sizeof(str));

         /* check if parent or current directory */
         if (strcmp(str, "..") == 0) {
            if (pkey->parent_keylist) {
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
               pkey = (KEY *) ((char *) pheader + pkeylist->parent);
            }
            continue;
         }
         if (strcmp(str, ".") == 0)
            continue;

         /* check if key is in keylist */
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

         for (i = 0; i < pkeylist->num_keys; i++) {
            if (!db_validate_key_offset(pheader, pkey->next_key)) {
               cm_msg(MERROR, "db_find_link1",
                      "Warning: database corruption, key \"%s\", next_key 0x%08X",
                      key_name, pkey->next_key - sizeof(DATABASE_HEADER));
               *subhKey = 0;
               return DB_CORRUPTED;
            }

            if (equal_ustring(str, pkey->name))
               break;

            pkey = (KEY *) ((char *) pheader + pkey->next_key);
         }

         if (i == pkeylist->num_keys) {
            *subhKey = 0;
            return DB_NO_KEY;
         }

         /* resolve links if not last in chain */
         if (pkey->type == TID_LINK && *pkey_name == '/') {
            /* copy destination, strip '/' */
            strcpy(str, (char *) pheader + pkey->data);
            if (str[strlen(str) - 1] == '/')
               str[strlen(str) - 1] = 0;

            /* append rest of key name */
            strcat(str, pkey_name);
            return db_find_link1(hDB, 0, str, subhKey);
         }

         /* key found: check if last in chain */
         if ((*pkey_name == '/')) {
            if (pkey->type != TID_KEY) {
               *subhKey = 0;
               return DB_NO_KEY;
            }
         }

         /* descend one level */
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      } while (*pkey_name == '/' && *(pkey_name + 1));

      *subhKey = (POINTER_T) pkey - (POINTER_T) pheader;
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_scan_tree(HNDLE hDB, HNDLE hKey, INT level, INT(*callback) (HNDLE, HNDLE, KEY *, INT, void *), void *info)
/********************************************************************\

  Routine: db_scan_tree

  Purpose: Scan a subtree recursively and call 'callback' for each key

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKeyRoot         Key to start scan from, 0 for root
    INT    level            Recursion level
    void   *callback        Callback routine called with params:
                              hDB   Copy of hDB
                              hKey  Copy of hKey
                              key   Key associated with hKey
                              INT   Recursion level
                              info  Copy of *info
    void   *info            Optional data copied to callback routine

  Output:
    implicit via callback

  Function value:
    DB_SUCCESS              Successful completion
    <all error codes of db_get_key>

\********************************************************************/
{
   HNDLE hSubkey;
   KEY key;
   INT i, status;

   status = db_get_link(hDB, hKey, &key);
   if (status != DB_SUCCESS)
      return status;

   status = callback(hDB, hKey, &key, level, info);
   if (status == 0)
      return status;

   if (key.type == TID_KEY) {
      for (i = 0;; i++) {
         db_enum_link(hDB, hKey, i, &hSubkey);

         if (!hSubkey)
            break;

         db_scan_tree(hDB, hSubkey, level + 1, callback, info);
      }
   }

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_scan_tree_link(HNDLE hDB, HNDLE hKey, INT level, void (*callback) (HNDLE, HNDLE, KEY *, INT, void *), void *info)
/********************************************************************\

  Routine: db_scan_tree_link

  Purpose: Scan a subtree recursively and call 'callback' for each key.
           Similar to db_scan_tree but without follwing links.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKeyRoot         Key to start scan from, 0 for root
    INT    level            Recursion level
    void   *callback        Callback routine called with params:
                              hDB   Copy of hDB
                              hKey  Copy of hKey
                              key   Key associated with hKey
                              INT   Recursion level
                              info  Copy of *info
    void   *info            Optional data copied to callback routine

  Output:
    implicit via callback

  Function value:
    DB_SUCCESS              Successful completion
    <all error codes of db_get_key>

\********************************************************************/
{
   HNDLE hSubkey;
   KEY key;
   INT i, status;

   status = db_get_key(hDB, hKey, &key);
   if (status != DB_SUCCESS)
      return status;

   callback(hDB, hKey, &key, level, info);

   if (key.type == TID_KEY) {
      for (i = 0;; i++) {
         db_enum_link(hDB, hKey, i, &hSubkey);

         if (!hSubkey)
            break;

         db_scan_tree_link(hDB, hSubkey, level + 1, callback, info);
      }
   }

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_get_path(HNDLE hDB, HNDLE hKey, char *path, INT buf_size)
/********************************************************************\

  Routine: db_get_path

  Purpose: Get full path of a key

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle
    INT    buf_size         Maximum size of path buffer (including
                            trailing zero)

  Output:
    char   path[buf_size]   Path string

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_MEMORY            path buffer is to small to contain full
                            string

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_PATH, hDB, hKey, path, buf_size);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      char str[MAX_ODB_PATH];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_path", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_path", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (hKey == pheader->root_key) {
         strcpy(path, "/");
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      *path = 0;
      do {
         /* add key name in front of path */
         strcpy(str, path);
         strcpy(path, "/");
         strcat(path, pkey->name);

         if (strlen(path) + strlen(str) + 1 > (DWORD) buf_size) {
            *path = 0;
            db_unlock_database(hDB);
            return DB_NO_MEMORY;
         }
         strcat(path, str);

         /* find parent key */
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
         pkey = (KEY *) ((char *) pheader + pkeylist->parent);
      } while (pkey->parent_keylist);

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
void db_find_open_records(HNDLE hDB, HNDLE hKey, KEY * key, INT level, void *result)
{
#ifdef LOCAL_ROUTINES
   DATABASE_HEADER *pheader;
   DATABASE_CLIENT *pclient;
   INT i, j;
   char line[256], str[80];

   /* avoid compiler warning */
   i = level;

   /* check if this key has notify count set */
   if (key->notify_count) {
      db_get_path(hDB, hKey, str, sizeof(str));
      sprintf(line, "%s open %d times by ", str, key->notify_count);

      db_lock_database(hDB);
      pheader = _database[hDB - 1].database_header;

      for (i = 0; i < pheader->max_client_index; i++) {
         pclient = &pheader->client[i];
         for (j = 0; j < pclient->max_index; j++)
            if (pclient->open_record[j].handle == hKey)
               sprintf(line + strlen(line), "%s ", pclient->name);
      }
      strcat(line, "\n");
      strcat((char *) result, line);

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */
}

void db_fix_open_records(HNDLE hDB, HNDLE hKey, KEY * key, INT level, void *result)
{
#ifdef LOCAL_ROUTINES
   DATABASE_HEADER *pheader;
   DATABASE_CLIENT *pclient;
   INT i, j;
   char str[256];
   KEY *pkey;

   /* avoid compiler warning */
   i = level;

   /* check if this key has notify count set */
   if (key->notify_count) {
      db_lock_database(hDB);
      pheader = _database[hDB - 1].database_header;

      for (i = 0; i < pheader->max_client_index; i++) {
         pclient = &pheader->client[i];
         for (j = 0; j < pclient->max_index; j++)
            if (pclient->open_record[j].handle == hKey)
               break;
         if (j < pclient->max_index)
            break;
      }
      if (i == pheader->max_client_index) {
         db_get_path(hDB, hKey, str, sizeof(str));
         strcat(str, " fixed\n");
         strcat((char *) result, str);

         /* reset notify count */
         pkey = (KEY *) ((char *) pheader + hKey);
         pkey->notify_count = 0;
      }

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */
}

INT db_get_open_records(HNDLE hDB, HNDLE hKey, char *str, INT buf_size, BOOL fix)
/********************************************************************\

  Routine: db_get_open_records

  Purpose: Return a string with all open records

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key to start search from, 0 for root
    INT    buf_size         Size of string
    INT    fix              If TRUE, fix records which are open
                            but have no client belonging to it.

  Output:
    char   *str             Result string. Individual records are
                            separated with new lines.

  Function value:
    DB_SUCCESS              Successful completion

\********************************************************************/
{
   str[0] = 0;

   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_OPEN_RECORDS, hDB, hKey, str, buf_size);

   if (fix)
      db_scan_tree_link(hDB, hKey, 0, db_fix_open_records, str);
   else
      db_scan_tree_link(hDB, hKey, 0, db_find_open_records, str);

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Set value of a single key.

The function sets a single value or a whole array to a ODB key.
Since the data buffer is of type void, no type checking can be performed by the
compiler. Therefore the type has to be explicitly supplied, which is checked
against the type stored in the ODB. key_name can contain the full path of a key
(like: "/Equipment/Trigger/Settings/Level1") while hkey is zero which refers
to the root, or hkey can refer to a sub-directory (like /Equipment/Trigger)
and key_name is interpreted relative to that directory like "Settings/Level1".
\code
INT level1;
  db_set_value(hDB, 0, "/Equipment/Trigger/Settings/Level1",
                          &level1, sizeof(level1), 1, TID_INT);
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKeyRoot Handle for key where search starts, zero for root.
@param key_name Name of key to search, can contain directories.
@param data Address of data.
@param data_size Size of data (in bytes).
@param num_values Number of data elements.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types)
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_ACCESS, DB_TYPE_MISMATCH
*/
INT db_set_value(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, const void *data,
                 INT data_size, INT num_values, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_VALUE, hDB, hKeyRoot, key_name, data, data_size, num_values, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      HNDLE hKey;
      INT status;

      if (num_values == 0)
         return DB_INVALID_PARAM;

      status = db_find_key(hDB, hKeyRoot, key_name, &hKey);
      if (status == DB_NO_KEY) {
         db_create_key(hDB, hKeyRoot, key_name, type);
         status = db_find_link(hDB, hKeyRoot, key_name, &hKey);
      }

      if (status != DB_SUCCESS)
         return status;

      db_lock_database(hDB);
      pheader = _database[hDB - 1].database_header;

      /* get address from handle */
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_value", "\"%s\" is of type %s, not %s",
                key_name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_value", "key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      if (data_size == 0) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_value", "zero data size not allowed");
         return DB_TYPE_MISMATCH;
      }

      if (type != TID_STRING && type != TID_LINK && data_size != rpc_tid_size(type) * num_values) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_value", "data_size (%d) does not match num_values (%d)", data_size, num_values);
         return DB_TYPE_MISMATCH;
      }

      /* resize data size if necessary */
      if (pkey->total_size != data_size) {
         pkey->data = (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, data_size);

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_value", "online database full");
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         pkey->total_size = data_size;
      }

      /* set number of values */
      pkey->num_values = num_values;

      if (type == TID_STRING || type == TID_LINK)
         pkey->item_size = data_size / num_values;
      else
         pkey->item_size = rpc_tid_size(type);

      /* copy data */
      memcpy((char *) pheader + pkey->data, data, data_size);

      /* update time */
      pkey->last_written = ss_time();

      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Set single value of an array.

The function sets a single value of an ODB key which is an array.
key_name can contain the full path of a key
(like: "/Equipment/Trigger/Settings/Level1") while hkey is zero which refers
to the root, or hkey can refer to a sub-directory (like /Equipment/Trigger)
and key_name is interpreted relative to that directory like "Settings/Level1".
\code
INT level1;
  db_set_value_index(hDB, 0, "/Equipment/Trigger/Settings/Level1",
                          &level1, sizeof(level1), 15, TID_INT, FALSE);
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKeyRoot Handle for key where search starts, zero for root.
@param key_name Name of key to search, can contain directories.
@param data Address of data.
@param data_size Size of data (in bytes).
@param index Array index of value.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types)
@param truncate Truncate array to current index if TRUE
@return \<same as db_set_data_index\>
*/
INT db_set_value_index(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, const void *data,
                       INT data_size, INT idx, DWORD type, BOOL trunc)
{
   HNDLE hkey;

   db_find_key(hDB, hKeyRoot, key_name, &hkey);
   if (!hkey) {
      db_create_key(hDB, hKeyRoot, key_name, type);
      db_find_key(hDB, hKeyRoot, key_name, &hkey);
      assert(hkey);
   } else
      if (trunc)
         db_set_num_values(hDB, hkey, idx + 1);

   return db_set_data_index(hDB, hkey, data, data_size, idx, type);
}

/********************************************************************/
/**
Get value of a single key.

The function returns single values or whole arrays which are contained
in an ODB key. Since the data buffer is of type void, no type checking can be
performed by the compiler. Therefore the type has to be explicitly supplied,
which is checked against the type stored in the ODB. key_name can contain the
full path of a key (like: "/Equipment/Trigger/Settings/Level1") while hkey is
zero which refers to the root, or hkey can refer to a sub-directory
(like: /Equipment/Trigger) and key_name is interpreted relative to that directory
like "Settings/Level1".
\code
INT level1, size;
  size = sizeof(level1);
  db_get_value(hDB, 0, "/Equipment/Trigger/Settings/Level1",
                                   &level1, &size, TID_INT, 0);
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKeyRoot Handle for key where search starts, zero for root.
@param key_name Name of key to search, can contain directories.
@param data Address of data.
@param buf_size Maximum buffer size on input, number of written bytes on return.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types)
@param create If TRUE, create key if not existing
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_ACCESS, DB_TYPE_MISMATCH,
DB_TRUNCATED, DB_NO_KEY
*/
INT db_get_value(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, void *data, INT * buf_size, DWORD type, BOOL create)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_VALUE, hDB, hKeyRoot, key_name, data, buf_size, type, create);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      HNDLE hkey;
      KEY *pkey;
      INT status, size, idx;
      char *p, path[256], keyname[256];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_value", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_value", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /* check if key name contains index */
      strlcpy(keyname, key_name, sizeof(keyname));
      idx = -1;
      if (strchr(keyname, '[') && strchr(keyname, ']')) {
         for (p = strchr(keyname, '[') + 1; *p && *p != ']'; p++)
            if (!isdigit(*p))
               break;

         if (*p && *p == ']') {
            idx = atoi(strchr(keyname, '[') + 1);
            *strchr(keyname, '[') = 0;
         }
      }

      status = db_find_key(hDB, hKeyRoot, keyname, &hkey);
      if (status == DB_NO_KEY) {
         if (create) {
            db_create_key(hDB, hKeyRoot, keyname, type);
            status = db_find_key(hDB, hKeyRoot, keyname, &hkey);
            if (status != DB_SUCCESS)
               return status;

            /* get string size from data size */
            if (type == TID_STRING || type == TID_LINK)
               size = *buf_size;
            else
               size = rpc_tid_size(type);

            if (size == 0)
               return DB_TYPE_MISMATCH;

            /* set default value if key was created */
            status = db_set_value(hDB, hKeyRoot, keyname, data, *buf_size, *buf_size / size, type);
         } else
            return DB_NO_KEY;
      }

      if (status != DB_SUCCESS)
         return status;

      /* now lock database */
      db_lock_database(hDB);
      pheader = _database[hDB - 1].database_header;

      /* get address from handle */
      pkey = (KEY *) ((char *) pheader + hkey);

      /* check for correct type */
      if (pkey->type != (type)) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_value", "hkey %d entry \"%s\" is of type %s, not %s",
                hKeyRoot, keyname, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* check for read access */
      if (!(pkey->access_mode & MODE_READ)) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_value", "%s has no read access", keyname);
         return DB_NO_ACCESS;
      }

      /* check if buffer is too small */
      if ((idx == -1 && pkey->num_values * pkey->item_size > *buf_size) || (idx != -1 && pkey->item_size > *buf_size)) {
         memcpy(data, (char *) pheader + pkey->data, *buf_size);
         db_unlock_database(hDB);
         db_get_path(hDB, hkey, path, sizeof(path));
         cm_msg(MERROR, "db_get_value", "buffer too small, data truncated for key \"%s\"", path);
         return DB_TRUNCATED;
      }

      /* check if index in boundaries */
      if (idx != -1 && idx >= pkey->num_values) {
         db_unlock_database(hDB);
         db_get_path(hDB, hkey, path, sizeof(path));
         cm_msg(MERROR, "db_get_value", "invalid index \"%d\" for key \"%s\"", idx, path);
         return DB_INVALID_PARAM;
      }

      /* copy key data */
      if (idx == -1) {
         memcpy(data, (char *) pheader + pkey->data, pkey->num_values * pkey->item_size);
         *buf_size = pkey->num_values * pkey->item_size;
      } else {
         memcpy(data, (char *) pheader + pkey->data + idx * pkey->item_size, pkey->item_size);
         *buf_size = pkey->item_size;
      }

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Enumerate subkeys from a key, follow links.

hkey must correspond to a valid ODB directory. The index is
usually incremented in a loop until the last key is reached. Information about the
sub-keys can be obtained with db_get_key(). If a returned key is of type
TID_KEY, it contains itself sub-keys. To scan a whole ODB sub-tree, the
function db_scan_tree() can be used.
\code
INT   i;
HNDLE hkey, hsubkey;
KEY   key;
  db_find_key(hdb, 0, "/Runinfo", &hkey);
  for (i=0 ; ; i++)
  {
   db_enum_key(hdb, hkey, i, &hsubkey);
   if (!hSubkey)
    break; // end of list reached
   // print key name
   db_get_key(hdb, hkey, &key);
   printf("%s\n", key.name);
  }
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey          Handle for key where search starts, zero for root.
@param idx          Subkey index, sould be initially 0, then
                    incremented in each call until
                    *subhKey becomes zero and the function
                    returns DB_NO_MORE_SUBKEYS
@param subkey_handle Handle of subkey which can be used in
                    db_get_key() and db_get_data()
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_MORE_SUBKEYS
*/
INT db_enum_key(HNDLE hDB, HNDLE hKey, INT idx, HNDLE * subkey_handle)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_ENUM_KEY, hDB, hKey, idx, subkey_handle);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      INT i;
      char str[256];
      HNDLE parent;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_enum_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_enum_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      *subkey_handle = 0;

      /* first lock database */
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != TID_KEY) {
         db_unlock_database(hDB);
         return DB_NO_MORE_SUBKEYS;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (idx >= pkeylist->num_keys) {
         db_unlock_database(hDB);
         return DB_NO_MORE_SUBKEYS;
      }

      pkey = (KEY *) ((char *) pheader + pkeylist->first_key);
      for (i = 0; i < idx; i++)
         pkey = (KEY *) ((char *) pheader + pkey->next_key);

      /* resolve links */
      if (pkey->type == TID_LINK) {
         strcpy(str, (char *) pheader + pkey->data);

         /* no not reolve if link to array index */
         if (strlen(str) > 0 && str[strlen(str) - 1] == ']') {
            *subkey_handle = (POINTER_T) pkey - (POINTER_T) pheader;
            db_unlock_database(hDB);
            return DB_SUCCESS;
         }

         if (*str == '/') {
            /* absolute path */
            db_unlock_database(hDB);
            return db_find_key(hDB, 0, str, subkey_handle);
         } else {
            /* relative path */
            if (pkey->parent_keylist) {
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
               parent = pkeylist->parent;
               db_unlock_database(hDB);
               return db_find_key(hDB, parent, str, subkey_handle);
            } else {
               db_unlock_database(hDB);
               return db_find_key(hDB, 0, str, subkey_handle);
            }
         }
      }

      *subkey_handle = (POINTER_T) pkey - (POINTER_T) pheader;
      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS


/*------------------------------------------------------------------*/
INT db_enum_link(HNDLE hDB, HNDLE hKey, INT idx, HNDLE * subkey_handle)
/********************************************************************\

  Routine: db_enum_link

  Purpose: Enumerate subkeys from a key, don't follow links

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to enumerate, zero for the
                            root key
    INT   idx               Subkey index, sould be initially 0, then
                            incremented in each call until
                            *subhKey becomes zero and the function
                            returns DB_NO_MORE_SUBKEYS

  Output:
    HNDLE *subkey_handle    Handle of subkey which can be used in
                            db_get_key and db_get_data

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_MORE_SUBKEYS      Last subkey reached

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_ENUM_LINK, hDB, hKey, idx, subkey_handle);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      INT i;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_enum_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_enum_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      *subkey_handle = 0;

      /* first lock database */
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != TID_KEY) {
         db_unlock_database(hDB);
         return DB_NO_MORE_SUBKEYS;
      }
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (idx >= pkeylist->num_keys) {
         db_unlock_database(hDB);
         return DB_NO_MORE_SUBKEYS;
      }

      pkey = (KEY *) ((char *) pheader + pkeylist->first_key);
      for (i = 0; i < idx; i++)
         pkey = (KEY *) ((char *) pheader + pkey->next_key);

      *subkey_handle = (POINTER_T) pkey - (POINTER_T) pheader;
      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_get_next_link(HNDLE hDB, HNDLE hKey, HNDLE * subkey_handle)
/********************************************************************\

  Routine: db_get_next_link

  Purpose: Get next key in ODB after hKey

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to enumerate, zero for the
                            root key

  Output:
    HNDLE *subkey_handle    Handle of subkey which can be used in
                            db_get_key and db_get_data

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_MORE_SUBKEYS      Last subkey reached

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_NEXT_LINK, hDB, hKey, subkey_handle);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey;
      INT descent;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_enum_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_enum_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      *subkey_handle = 0;

      /* first lock database */
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      descent = TRUE;
      do {
         if (pkey->type != TID_KEY || !descent) {
            if (pkey->next_key) {
               /* key has next key, return it */
               pkey = (KEY *) ((char *) pheader + pkey->next_key);

               if (pkey->type != TID_KEY) {
                  *subkey_handle = (POINTER_T) pkey - (POINTER_T) pheader;
                  db_unlock_database(hDB);
                  return DB_SUCCESS;
               }

               /* key has subkeys, so descent on the next iteration */
               descent = TRUE;
            } else {
               if (pkey->parent_keylist == 0) {
                  /* return if we are back to the root key */
                  db_unlock_database(hDB);
                  return DB_NO_MORE_SUBKEYS;
               }

               /* key is last in list, traverse up */
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);

               pkey = (KEY *) ((char *) pheader + pkeylist->parent);
               descent = FALSE;
            }
         } else {
            if (descent) {
               /* find first subkey */
               pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

               if (pkeylist->num_keys == 0) {
                  /* if key has no subkeys, look for next key on this level */
                  descent = FALSE;
               } else {
                  /* get first subkey */
                  pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

                  if (pkey->type != TID_KEY) {
                     *subkey_handle = (POINTER_T) pkey - (POINTER_T) pheader;
                     db_unlock_database(hDB);
                     return DB_SUCCESS;
                  }
               }
            }
         }

      } while (TRUE);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Get key structure from a handle.

KEY structure has following format:
\code
typedef struct {
  DWORD         type;                 // TID_xxx type
  INT           num_values;           // number of values
  char          name[NAME_LENGTH];    // name of variable
  INT           data;                 // Address of variable (offset)
  INT           total_size;           // Total size of data block
  INT           item_size;            // Size of single data item
  WORD          access_mode;          // Access mode
  WORD          notify_count;         // Notify counter
  INT           next_key;             // Address of next key
  INT           parent_keylist;       // keylist to which this key belongs
  INT           last_written;         // Time of last write action
} KEY;
\endcode
Most of these values are used for internal purposes, the values which are of
public interest are type, num_values, and name. For keys which contain a
single value, num_values equals to one and total_size equals to
item_size. For keys which contain an array of strings (TID_STRING),
item_size equals to the length of one string.
\code
KEY   key;
HNDLE hkey;
db_find_key(hDB, 0, "/Runinfo/Run number", &hkey);
db_get_key(hDB, hkey, &key);
printf("The run number is of type %s\n", rpc_tid_name(key.type));
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param key Pointer to KEY stucture.
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_get_key(HNDLE hDB, HNDLE hKey, KEY * key)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_KEY, hDB, hKey, key);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      HNDLE hkeylink;
      char link_name[256];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER) && hKey != 0) {
         cm_msg(MERROR, "db_get_key", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;

      if (!hKey)
         hKey = pheader->root_key;

      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type < 1 || pkey->type >= TID_LAST) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_key", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      /* check for link to array index */
      if (pkey->type == TID_LINK) {
         strlcpy(link_name, (char *) pheader + pkey->data, sizeof(link_name));
         if (strlen(link_name) > 0 && link_name[strlen(link_name) - 1] == ']') {
            db_unlock_database(hDB);
            if (strchr(link_name, '[') == NULL)
               return DB_INVALID_LINK;
            if (db_find_key(hDB, 0, link_name, &hkeylink) != DB_SUCCESS)
               return DB_INVALID_LINK;
            db_get_key(hDB, hkeylink, key);
            key->num_values = 1;        // fake number of values
            return DB_SUCCESS;
         }
      }

      memcpy(key, pkey, sizeof(KEY));

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Same as db_get_key, but it does not follow a link to an array index
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param key Pointer to KEY stucture.
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_get_link(HNDLE hDB, HNDLE hKey, KEY * key)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_LINK, hDB, hKey, key);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_link", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER) && hKey != 0) {
         cm_msg(MERROR, "db_get_link", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;

      if (!hKey)
         hKey = pheader->root_key;

      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type < 1 || pkey->type >= TID_LAST) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_link", "hkey %d invalid key type %d", hKey, pkey->type);
         return DB_INVALID_HANDLE;
      }

      memcpy(key, pkey, sizeof(KEY));

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Get time when key was last modified
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey              Handle of key to operate on
@param delta             Seconds since last update
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_get_key_time(HNDLE hDB, HNDLE hKey, DWORD * delta)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_KEY_TIME, hDB, hKey, delta);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_get_key", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      *delta = ss_time() - pkey->last_written;

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Get key info (separate values instead of structure)
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey              Handle of key to operate on
@param name             Key name
@param name_size        Size of the give name (done with sizeof())
@param type             Key type (see @ref Midas_Data_Types).
@param num_values       Number of values in key.
@param item_size        Size of individual key value (used for strings)
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_get_key_info(HNDLE hDB, HNDLE hKey, char *name, INT name_size, INT * type, INT * num_values, INT * item_size)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_KEY_INFO, hDB, hKey, name, name_size, type, num_values, item_size);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      KEYLIST *pkeylist;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_key_info", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_key_info", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_get_key_info", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if ((INT) strlen(pkey->name) + 1 > name_size) {
         /* truncate name */
         memcpy(name, pkey->name, name_size - 1);
         name[name_size] = 0;
      } else
         strcpy(name, pkey->name);

      /* convert "root" to "/" */
      if (strcmp(name, "root") == 0)
         strcpy(name, "/");

      *type = pkey->type;
      *num_values = pkey->num_values;
      *item_size = pkey->item_size;

      if (pkey->type == TID_KEY) {
         pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);
         *num_values = pkeylist->num_keys;
      }

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS


/*------------------------------------------------------------------*/
INT db_rename_key(HNDLE hDB, HNDLE hKey, const char *name)
/********************************************************************\

  Routine: db_get_key

  Purpose: Rename a key

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key
    char  *name             New key name

  Output:
    <none>

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_INVALID_NAME         Key name contains '/'

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_RENAME_KEY, hDB, hKey, name);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_rename_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_rename_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_rename_key", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      if (strchr(name, '/')) {
         cm_msg(MERROR, "db_rename_key", "key name may not contain \"/\"");
         return DB_INVALID_NAME;
      }
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (!pkey->type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_rename_key", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      strlcpy(pkey->name, name, NAME_LENGTH);

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_reorder_key(HNDLE hDB, HNDLE hKey, INT idx)
/********************************************************************\

  Routine: db_reorder_key

  Purpose: Reorder key so that key hKey apprears at position 'index'
           in keylist (or at bottom if index<0)

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key
    INT   idx               New positio of key in keylist

  Output:
    <none>

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_ACCESS            Key is locked for write
    DB_OPEN_RECORD          Key, subkey or parent key is open

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_REORDER_KEY, hDB, hKey, idx);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey, *pprev_key, *pnext_key, *pkey_tmp;
      KEYLIST *pkeylist;
      INT i;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_rename_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_rename_key", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_rename_key", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      if (!pkey->type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_reorder_key", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      if (!(pkey->access_mode & MODE_WRITE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      /* check if someone has opened key or parent */
      do {
         if (pkey->notify_count) {
            db_unlock_database(hDB);
            return DB_OPEN_RECORD;
         }

         if (pkey->parent_keylist == 0)
            break;

         pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
         pkey = (KEY *) ((char *) pheader + pkeylist->parent);
      } while (TRUE);

      pkey = (KEY *) ((char *) pheader + hKey);
      pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);

      /* first remove key from list */
      pnext_key = (KEY *) (POINTER_T) pkey->next_key;

      if ((KEY *) ((char *) pheader + pkeylist->first_key) == pkey) {
         /* key is first in list */
         pkeylist->first_key = (POINTER_T) pnext_key;
      } else {
         /* find predecessor */
         pkey_tmp = (KEY *) ((char *) pheader + pkeylist->first_key);
         while ((KEY *) ((char *) pheader + pkey_tmp->next_key) != pkey)
            pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);
         pkey_tmp->next_key = (POINTER_T) pnext_key;
      }

      /* add key to list at proper index */
      pkey_tmp = (KEY *) ((char *) pheader + pkeylist->first_key);
      if (idx < 0 || idx >= pkeylist->num_keys - 1) {
         /* add at bottom */

         /* find last key */
         for (i = 0; i < pkeylist->num_keys - 2; i++) {
            pprev_key = pkey_tmp;
            pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);
         }

         pkey_tmp->next_key = (POINTER_T) pkey - (POINTER_T) pheader;
         pkey->next_key = 0;
      } else {
         if (idx == 0) {
            /* add at top */
            pkey->next_key = pkeylist->first_key;
            pkeylist->first_key = (POINTER_T) pkey - (POINTER_T) pheader;
         } else {
            /* add at position index */
            for (i = 0; i < idx - 1; i++)
               pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);

            pkey->next_key = pkey_tmp->next_key;
            pkey_tmp->next_key = (POINTER_T) pkey - (POINTER_T) pheader;
         }
      }

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */


/********************************************************************/
/**
Get key data from a handle

The function returns single values or whole arrays which are contained
in an ODB key. Since the data buffer is of type void, no type checking can be
performed by the compiler. Therefore the type has to be explicitly supplied,
which is checked against the type stored in the ODB.
\code
  HNLDE hkey;
  INT   run_number, size;
  // get key handle for run number
  db_find_key(hDB, 0, "/Runinfo/Run number", &hkey);
  // return run number
  size = sizeof(run_number);
  db_get_data(hDB, hkey, &run_number, &size,TID_INT);
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param data         Pointer to the return data. 
@param buf_size     Size of data buffer.
@param type         Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TRUNCATED, DB_TYPE_MISMATCH
*/
INT db_get_data(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_DATA, hDB, hKey, data, buf_size, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      char str[256];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_data", "Invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_get_data", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for read access */
      if (!(pkey->access_mode & MODE_READ)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (!pkey->type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      /* follow links to array index */
      if (pkey->type == TID_LINK) {
         char link_name[256];
         int i;
         HNDLE hkey;
         KEY key;

         strlcpy(link_name, (char *) pheader + pkey->data, sizeof(link_name));
         if (strlen(link_name) > 0 && link_name[strlen(link_name) - 1] == ']') {
            db_unlock_database(hDB);
            if (strchr(link_name, '[') == NULL)
               return DB_INVALID_LINK;
            i = atoi(strchr(link_name, '[') + 1);
            *strchr(link_name, '[') = 0;
            if (db_find_key(hDB, 0, link_name, &hkey) != DB_SUCCESS)
               return DB_INVALID_LINK;
            db_get_key(hDB, hkey, &key);
            return db_get_data_index(hDB, hkey, data, buf_size, i, key.type);
         }
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_get_data", "\"%s\" is of type %s, not %s",
                str, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_get_data", "Key \"%s\" cannot contain data", str);
         return DB_TYPE_MISMATCH;
      }

      /* check if key has data */
      if (pkey->data == 0) {
         memset(data, 0, *buf_size);
         *buf_size = 0;
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      /* check if buffer is too small */
      if (pkey->num_values * pkey->item_size > *buf_size) {
         memcpy(data, (char *) pheader + pkey->data, *buf_size);
         db_unlock_database(hDB);
         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_get_data", "data for key \"%s\" truncated", str);
         return DB_TRUNCATED;
      }

      /* copy key data */
      memcpy(data, (char *) pheader + pkey->data, pkey->num_values * pkey->item_size);
      *buf_size = pkey->num_values * pkey->item_size;

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Same as db_get_data, but do not follow a link to an array index

@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param data         Pointer to the return data. 
@param buf_size     Size of data buffer.
@param type         Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TRUNCATED, DB_TYPE_MISMATCH
*/
INT db_get_link_data(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_LINK_DATA, hDB, hKey, data, buf_size, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_data", "Invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_get_data", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for read access */
      if (!(pkey->access_mode & MODE_READ)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (!pkey->type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "\"%s\" is of type %s, not %s",
                pkey->name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "Key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* check if key has data */
      if (pkey->data == 0) {
         memset(data, 0, *buf_size);
         *buf_size = 0;
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      /* check if buffer is too small */
      if (pkey->num_values * pkey->item_size > *buf_size) {
         memcpy(data, (char *) pheader + pkey->data, *buf_size);
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "data for key \"%s\" truncated", pkey->name);
         return DB_TRUNCATED;
      }

      /* copy key data */
      memcpy(data, (char *) pheader + pkey->data, pkey->num_values * pkey->item_size);
      *buf_size = pkey->num_values * pkey->item_size;

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_get_data1(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, DWORD type, INT * num_values)
/********************************************************************\

  Routine: db_get_data1

  Purpose: Get key data from a handle, return number of values

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key
    INT    *buf_size        Size of data buffer
    DWORD  type             Type of data

  Output:
    void   *data            Key data
    INT    *buf_size        Size of key data
    INT    *num_values      Number of values

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TRUNCATED            Return buffer is smaller than key data
    DB_TYPE_MISMATCH        Type mismatch

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_DATA1, hDB, hKey, data, buf_size, type, num_values);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_data", "Invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_get_data", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for read access */
      if (!(pkey->access_mode & MODE_READ)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (!pkey->type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "\"%s\" is of type %s, not %s",
                pkey->name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "Key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* check if key has data */
      if (pkey->data == 0) {
         memset(data, 0, *buf_size);
         *buf_size = 0;
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      /* check if buffer is too small */
      if (pkey->num_values * pkey->item_size > *buf_size) {
         memcpy(data, (char *) pheader + pkey->data, *buf_size);
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data", "data for key \"%s\" truncated", pkey->name);
         return DB_TRUNCATED;
      }

      /* copy key data */
      memcpy(data, (char *) pheader + pkey->data, pkey->num_values * pkey->item_size);
      *buf_size = pkey->num_values * pkey->item_size;
      *num_values = pkey->num_values;

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
returns a single value of keys containing arrays of values.

The function returns a single value of keys containing arrays of values.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param data         Size of data buffer.
@param buf_size     Return size of the record.
@param idx          Index of array [0..n-1].
@param type         Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TRUNCATED, DB_OUT_OF_RANGE
*/
INT db_get_data_index(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, INT idx, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_GET_DATA_INDEX, hDB, hKey, data, buf_size, idx, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      char str[256];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_get_data", "Invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_get_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_get_data", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for read access */
      if (!(pkey->access_mode & MODE_READ)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (!pkey->type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data_index", "invalid key type %d", pkey->type);
         return DB_INVALID_HANDLE;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data_index",
                "\"%s\" is of type %s, not %s", pkey->name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data_index", "Key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* check if key has data */
      if (pkey->data == 0) {
         memset(data, 0, *buf_size);
         *buf_size = 0;
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      /* check if index in range */
      if (idx < 0 || idx >= pkey->num_values) {
         memset(data, 0, *buf_size);
         db_unlock_database(hDB);

         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_get_data_index",
                "index (%d) exceeds array length (%d) for key \"%s\"", idx, pkey->num_values, str);
         return DB_OUT_OF_RANGE;
      }

      /* check if buffer is too small */
      if (pkey->item_size > *buf_size) {
         /* copy data */
         memcpy(data, (char *) pheader + pkey->data + idx * pkey->item_size, *buf_size);
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_get_data_index", "data for key \"%s\" truncated", pkey->name);
         return DB_TRUNCATED;
      }

      /* copy key data */
      memcpy(data, (char *) pheader + pkey->data + idx * pkey->item_size, pkey->item_size);
      *buf_size = pkey->item_size;

      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Set key data from a handle. Adjust number of values if
previous data has different size.
\code
HNLDE hkey;
 INT   run_number;
 // get key handle for run number
 db_find_key(hDB, 0, "/Runinfo/Run number", &hkey);
 // set run number
 db_set_data(hDB, hkey, &run_number, sizeof(run_number),TID_INT);
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param data Buffer from which data gets copied to.
@param buf_size Size of data buffer.
@param num_values Number of data values (for arrays).
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TRUNCATED
*/
INT db_set_data(HNDLE hDB, HNDLE hKey, const void *data, INT buf_size, INT num_values, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_DATA, hDB, hKey, data, buf_size, num_values, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      HNDLE hkeylink;
      int link_idx;
      char link_name[256];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_set_data", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      if (num_values == 0)
         return DB_INVALID_PARAM;

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      /* check for link to array index */
      if (pkey->type == TID_LINK) {
         strlcpy(link_name, (char *) pheader + pkey->data, sizeof(link_name));
         if (strlen(link_name) > 0 && link_name[strlen(link_name) - 1] == ']') {
            db_unlock_database(hDB);
            if (strchr(link_name, '[') == NULL)
               return DB_INVALID_LINK;
            link_idx = atoi(strchr(link_name, '[') + 1);
            *strchr(link_name, '[') = 0;
            if (db_find_key(hDB, 0, link_name, &hkeylink) != DB_SUCCESS)
               return DB_INVALID_LINK;
            return db_set_data_index(hDB, hkeylink, data, buf_size, link_idx, type);
         }
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data", "\"%s\" is of type %s, not %s",
                pkey->name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data", "Key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* if no buf_size given (Java!), calculate it */
      if (buf_size == 0)
         buf_size = pkey->item_size * num_values;

      /* resize data size if necessary */
      if (pkey->total_size != buf_size) {
         pkey->data = (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, buf_size);

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_data", "online database full");
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         pkey->total_size = buf_size;
      }

      /* set number of values */
      pkey->num_values = num_values;
      if (num_values)
         pkey->item_size = buf_size / num_values;

      /* copy data */
      memcpy((char *) pheader + pkey->data, data, buf_size);

      /* update time */
      pkey->last_written = ss_time();

      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Same as db_set_data, but it does not follow a link to an array index
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param data Buffer from which data gets copied to.
@param buf_size Size of data buffer.
@param num_values Number of data values (for arrays).
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TRUNCATED
*/
INT db_set_link_data(HNDLE hDB, HNDLE hKey, const void *data, INT buf_size, INT num_values, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_LINK_DATA, hDB, hKey, data, buf_size, num_values, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_data", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_set_data", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      if (num_values == 0)
         return DB_INVALID_PARAM;

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_link_data", "\"%s\" is of type %s, not %s",
                pkey->name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_link_data", "Key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* if no buf_size given (Java!), calculate it */
      if (buf_size == 0)
         buf_size = pkey->item_size * num_values;

      /* resize data size if necessary */
      if (pkey->total_size != buf_size) {
         pkey->data = (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, buf_size);

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_link_data", "online database full");
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         pkey->total_size = buf_size;
      }

      /* set number of values */
      pkey->num_values = num_values;
      if (num_values)
         pkey->item_size = buf_size / num_values;

      /* copy data */
      memcpy((char *) pheader + pkey->data, data, buf_size);

      /* update time */
      pkey->last_written = ss_time();

      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_set_num_values(HNDLE hDB, HNDLE hKey, INT num_values)
/********************************************************************\

  Routine: db_set_num_values

  Purpose: Set numbe of values in a key. Extend with zeros or truncate.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key
    INT    num_values       Number of data values

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_NUM_VALUES, hDB, hKey, num_values);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      INT new_size;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_num_values", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_num_values", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_set_num_values", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      if (num_values <= 0) {
         cm_msg(MERROR, "db_set_num_values", "invalid num_values %d", num_values);
         return DB_INVALID_PARAM;
      }

      if (num_values == 0)
         return DB_INVALID_PARAM;

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_num_values", "Key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      if (pkey->total_size != pkey->item_size * pkey->num_values) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_num_values", "Corrupted key");
         return DB_CORRUPTED;
      }

      if (pkey->item_size == 0) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_num_values", "Cannot resize array with item_size equal to zero");
         return DB_INVALID_PARAM;
      }

      /* resize data size if necessary */
      if (pkey->num_values != num_values) {
         new_size = pkey->item_size * num_values;

         pkey->data = (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, new_size);

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_num_values", "hkey %d, num_values %d, new_size %d, online database full", hKey, num_values, new_size);
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         pkey->total_size = new_size;
         pkey->num_values = num_values;
      }

      /* update time */
      pkey->last_written = ss_time();

      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Set key data for a key which contains an array of values.

This function sets individual values of a key containing an array.
If the index is larger than the array size, the array is extended and the intermediate
values are set to zero.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param data Pointer to single value of data.
@param data_size
@param idx Size of single data element.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_ACCESS, DB_TYPE_MISMATCH
*/
INT db_set_data_index(HNDLE hDB, HNDLE hKey, const void *data, INT data_size, INT idx, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_DATA_INDEX, hDB, hKey, data, data_size, idx, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      char link_name[256], str[256];
      int link_idx;
      HNDLE hkeylink;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_data_index", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_data_index", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_set_data_index", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      /* check for link to array index */
      if (pkey->type == TID_LINK) {
         strlcpy(link_name, (char *) pheader + pkey->data, sizeof(link_name));
         if (strlen(link_name) > 0 && link_name[strlen(link_name) - 1] == ']') {
            db_unlock_database(hDB);
            if (strchr(link_name, '[') == NULL)
               return DB_INVALID_LINK;
            link_idx = atoi(strchr(link_name, '[') + 1);
            *strchr(link_name, '[') = 0;
            if (db_find_key(hDB, 0, link_name, &hkeylink) != DB_SUCCESS)
               return DB_INVALID_LINK;
            return db_set_data_index(hDB, hkeylink, data, data_size, link_idx, type);
         }
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_set_data_index",
                "\"%s\" is of type %s, not %s", str, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index", "key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* check for valid idx */
      if (idx < 0) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index", "invalid index %d", idx);
         return DB_FULL;
      }

      /* check for valid array element size: if new element size
         is different from existing size, ODB becomes corrupted */
      if (pkey->item_size != 0 && data_size != pkey->item_size) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index", "invalid element data size %d, expected %d", data_size, pkey->item_size);
         return DB_TYPE_MISMATCH;
      }

      /* increase data size if necessary */
      if (idx >= pkey->num_values || pkey->item_size == 0) {
         pkey->data =
             (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, data_size * (idx + 1));

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_data_index", "online database full");
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         if (!pkey->item_size)
            pkey->item_size = data_size;
         pkey->total_size = data_size * (idx + 1);
         pkey->num_values = idx + 1;
      }

      /* cut strings which are too long */
      if ((type == TID_STRING || type == TID_LINK) && (int) strlen((char *) data) + 1 > pkey->item_size)
         *((char *) data + pkey->item_size - 1) = 0;

      /* copy data */
      memcpy((char *) pheader + pkey->data + idx * pkey->item_size, data, pkey->item_size);

      /* update time */
      pkey->last_written = ss_time();

      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Same as db_set_data_index, but does not follow links.

@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param data Pointer to single value of data.
@param data_size
@param idx Size of single data element.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_ACCESS, DB_TYPE_MISMATCH
*/
INT db_set_link_data_index(HNDLE hDB, HNDLE hKey, const void *data, INT data_size, INT idx, DWORD type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_LINK_DATA_INDEX, hDB, hKey, data, data_size, idx, type);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;
      char str[256];

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_data_index", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_data_index", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_set_data_index", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_set_data_index",
                "\"%s\" is of type %s, not %s", str, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index", "key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* check for valid array element size: if new element size
         is different from existing size, ODB becomes corrupted */
      if (pkey->item_size != 0 && data_size != pkey->item_size) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index", "invalid element data size %d, expected %d", data_size, pkey->item_size);
         return DB_TYPE_MISMATCH;
      }

      /* increase data size if necessary */
      if (idx >= pkey->num_values || pkey->item_size == 0) {
         pkey->data =
             (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, data_size * (idx + 1));

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_data_index", "online database full");
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         if (!pkey->item_size)
            pkey->item_size = data_size;
         pkey->total_size = data_size * (idx + 1);
         pkey->num_values = idx + 1;
      }

      /* cut strings which are too long */
      if ((type == TID_STRING || type == TID_LINK) && (int) strlen((char *) data) + 1 > pkey->item_size)
         *((char *) data + pkey->item_size - 1) = 0;

      /* copy data */
      memcpy((char *) pheader + pkey->data + idx * pkey->item_size, data, pkey->item_size);

      /* update time */
      pkey->last_written = ss_time();

      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_set_data_index2(HNDLE hDB, HNDLE hKey, const void *data, INT data_size, INT idx, DWORD type, BOOL bNotify)
/********************************************************************\

  Routine: db_set_data_index2

  Purpose: Set key data for a key which contains an array of values.
           Optionally notify clients which have key open.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key to enumerate
    void   *data            Pointer to single value of data
    INT    data_size        Size of single data element
    INT    idx              Index of array to change [0..n-1]
    DWORD  type             Type of data
    BOOL   bNotify          If TRUE, notify clients

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TYPE_MISMATCH        Key was created with different type
    DB_NO_ACCESS            No write access

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_DATA_INDEX2, hDB, hKey, data, data_size, idx, type, bNotify);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEY *pkey;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_data_index2", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_data_index2", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (hKey < (int) sizeof(DATABASE_HEADER)) {
         cm_msg(MERROR, "db_set_data_index2", "invalid key handle");
         return DB_INVALID_HANDLE;
      }

      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      /* check for write access */
      if (!(pkey->access_mode & MODE_WRITE) || (pkey->access_mode & MODE_EXCLUSIVE)) {
         db_unlock_database(hDB);
         return DB_NO_ACCESS;
      }

      if (pkey->type != type) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index2",
                "\"%s\" is of type %s, not %s", pkey->name, rpc_tid_name(pkey->type), rpc_tid_name(type));
         return DB_TYPE_MISMATCH;
      }

      /* keys cannot contain data */
      if (pkey->type == TID_KEY) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index2", "key cannot contain data");
         return DB_TYPE_MISMATCH;
      }

      /* check for valid index */
      if (idx < 0) {
         db_unlock_database(hDB);
         cm_msg(MERROR, "db_set_data_index2", "invalid index");
         return DB_FULL;
      }

      /* increase key size if necessary */
      if (idx >= pkey->num_values) {
         pkey->data =
             (POINTER_T) realloc_data(pheader, (char *) pheader + pkey->data, pkey->total_size, data_size * (idx + 1));

         if (pkey->data == 0) {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_set_data_index2", "online database full");
            return DB_FULL;
         }

         pkey->data -= (POINTER_T) pheader;
         if (!pkey->item_size)
            pkey->item_size = data_size;
         pkey->total_size = data_size * (idx + 1);
         pkey->num_values = idx + 1;
      }

      /* cut strings which are too long */
      if ((type == TID_STRING || type == TID_LINK) && (int) strlen((char *) data) + 1 > pkey->item_size)
         *((char *) data + pkey->item_size - 1) = 0;

      /* copy data */
      memcpy((char *) pheader + pkey->data + idx * pkey->item_size, data, pkey->item_size);

      /* update time */
      pkey->last_written = ss_time();

      if (bNotify)
         db_notify_clients(hDB, hKey, TRUE);

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT db_merge_data(HNDLE hDB, HNDLE hKeyRoot, const char *name, void *data, INT data_size, INT num_values, INT type)
/********************************************************************\

  Routine: db_merge_data

  Purpose: Merge an array with an ODB array. If the ODB array doesn't
           exist, create it and fill it with the array. If it exists,
           load it in the array. Adjust ODB array size if necessary.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKeyRoot         Key handle to start with, 0 for root
    cha    *name            Key name relative to hKeyRoot
    void   *data            Pointer to data array
    INT    data_size        Size of data array
    INT    num_values       Number of values in array
    DWORD  type             Type of data

  Output:
    none

  Function value:
    <same as db_set_data>

\********************************************************************/
{
   HNDLE hKey;
   INT status, old_size;

   if (num_values == 0)
      return DB_INVALID_PARAM;

   status = db_find_key(hDB, hKeyRoot, name, &hKey);
   if (status != DB_SUCCESS) {
      db_create_key(hDB, hKeyRoot, name, type);
      db_find_key(hDB, hKeyRoot, name, &hKey);
      status = db_set_data(hDB, hKey, data, data_size, num_values, type);
   } else {
      old_size = data_size;
      db_get_data(hDB, hKey, data, &old_size, type);
      status = db_set_data(hDB, hKey, data, data_size, num_values, type);
   }

   return status;
}

/*------------------------------------------------------------------*/
INT db_set_mode(HNDLE hDB, HNDLE hKey, WORD mode, BOOL recurse)
/********************************************************************\

  Routine: db_set_mode

  Purpose: Set access mode of key

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle
    DWORD  mode             Access mode, any or'ed combination of
                            MODE_READ, MODE_WRITE, MODE_EXCLUSIVE
                            and MODE_DELETE
    BOOL   recurse          Value of 0 (FALSE): do not recurse subtree,
                            value of 1 (TRUE): recurse subtree,
                            value of 2: recurse subtree, assume database is locked by caller.

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_SET_MODE, hDB, hKey, mode, recurse);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      KEYLIST *pkeylist;
      KEY *pkey, *pnext_key;
      HNDLE hKeyLink;
      BOOL locked = FALSE;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_set_mode", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (!_database[hDB - 1].attached) {
         cm_msg(MERROR, "db_set_mode", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (recurse < 2) {
         db_lock_database(hDB);
         locked = TRUE;
      }

      pheader = _database[hDB - 1].database_header;
      if (!hKey)
         hKey = pheader->root_key;
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         if (locked)
            db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

      if (pkey->type == TID_KEY && pkeylist->first_key && recurse) {
         /* first recurse subtree */
         pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

         do {
            pnext_key = (KEY *) (POINTER_T) pkey->next_key;

            db_set_mode(hDB, (POINTER_T) pkey - (POINTER_T) pheader, mode, recurse + 1);

            if (pnext_key)
               pkey = (KEY *) ((char *) pheader + (POINTER_T) pnext_key);
         } while (pnext_key);
      }

      pkey = (KEY *) ((char *) pheader + hKey);

      /* resolve links */
      if (pkey->type == TID_LINK) {
         if (*((char *) pheader + pkey->data) == '/')
            db_find_key1(hDB, 0, (char *) pheader + pkey->data, &hKeyLink);
         else
            db_find_key1(hDB, hKey, (char *) pheader + pkey->data, &hKeyLink);
         if (hKeyLink)
            db_set_mode(hDB, hKeyLink, mode, recurse > 0);
         pheader = _database[hDB - 1].database_header;
         pkey = (KEY *) ((char *) pheader + hKey);
      }

      /* now set mode */
      pkey->access_mode = mode;

      if (locked)
         db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Load a branch of a database from an .ODB file.

This function is used by the ODBEdit command load. For a
description of the ASCII format, see db_copy(). Data can be loaded relative to
the root of the ODB (hkey equal zero) or relative to a certain key.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKeyRoot Handle for key where search starts, zero for root.
@param filename Filename of .ODB file.
@param bRemote If TRUE, the file is loaded by the server process on the
back-end, if FALSE, it is loaded from the current process
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_FILE_ERROR
*/
INT db_load(HNDLE hDB, HNDLE hKeyRoot, const char *filename, BOOL bRemote)
{
   struct stat stat_buf;
   INT hfile, size, n, i, status;
   char *buffer;

   if (rpc_is_remote() && bRemote)
      return rpc_call(RPC_DB_LOAD, hDB, hKeyRoot, filename);

   /* open file */
   hfile = open(filename, O_RDONLY | O_TEXT, 0644);
   if (hfile == -1) {
      cm_msg(MERROR, "db_load", "file \"%s\" not found", filename);
      return DB_FILE_ERROR;
   }

   /* allocate buffer with file size */
   fstat(hfile, &stat_buf);
   size = stat_buf.st_size;
   buffer = (char *) malloc(size + 1);

   if (buffer == NULL) {
      cm_msg(MERROR, "db_load", "cannot allocate ODB load buffer");
      close(hfile);
      return DB_NO_MEMORY;
   }

   n = 0;

   do {
      i = read(hfile, buffer + n, size - n);
      if (i <= 0)
         break;
      n += i;
   } while (TRUE);

   buffer[n] = 0;

   if (strncmp(buffer, "<?xml version=\"1.0\"", 19) == 0) {
      status = db_paste_xml(hDB, hKeyRoot, buffer);
      if (status != DB_SUCCESS)
         printf("Error in file \"%s\"\n", filename);
   } else
      status = db_paste(hDB, hKeyRoot, buffer);

   close(hfile);
   free(buffer);

   return status;
}

/********************************************************************/
/**
Copy an ODB subtree in ASCII format to a buffer

This function converts the binary ODB contents to an ASCII.
The function db_paste() can be used to convert the ASCII representation back
to binary ODB contents. The functions db_load() and db_save() internally
use db_copy() and db_paste(). This function converts the binary ODB
contents to an ASCII representation of the form:
- For single value:
\code
[ODB path]
 key name = type : value
\endcode
- For strings:
\code
key name = STRING : [size] string contents
\endcode
- For arrayes (type can be BYTE, SBYTE, CHAR, WORD, SHORT, DWORD,
INT, BOOL, FLOAT, DOUBLE, STRING or LINK):
\code
key name = type[size] :
 [0] value0
 [1] value1
 [2] value2
 ...
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param buffer ASCII buffer which receives ODB contents.
@param buffer_size Size of buffer, returns remaining space in buffer.
@param path Internal use only, must be empty ("").
@return DB_SUCCESS, DB_TRUNCATED, DB_NO_MEMORY
*/
INT db_copy(HNDLE hDB, HNDLE hKey, char *buffer, INT * buffer_size, char *path)
{
   INT i, j, size, status;
   KEY key;
   HNDLE hSubkey;
   char full_path[MAX_ODB_PATH], str[MAX_STRING_LENGTH * 2];
   char *data, line[MAX_STRING_LENGTH * 2];
   BOOL bWritten;

   strcpy(full_path, path);

   bWritten = FALSE;

   /* first enumerate this level */
   for (i = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (i == 0 && !hSubkey) {
         /* If key has no subkeys, just write this key */
         status = db_get_link(hDB, hKey, &key);
         if (status != DB_SUCCESS)
            continue;
         size = key.total_size;
         data = (char *) malloc(size);
         if (data == NULL) {
            cm_msg(MERROR, "db_copy", "cannot allocate data buffer");
            return DB_NO_MEMORY;
         }
         line[0] = 0;

         if (key.type != TID_KEY) {
            status = db_get_link_data(hDB, hKey, data, &size, key.type);
            if (status != DB_SUCCESS)
               continue;
            if (key.num_values == 1) {
               sprintf(line, "%s = %s : ", key.name, tid_name[key.type]);

               if (key.type == TID_STRING && strchr(data, '\n') != NULL) {
                  /* multiline string */
                  sprintf(line + strlen(line), "[====#$@$#====]\n");

                  /* copy line to buffer */
                  if ((INT) (strlen(line) + 1) > *buffer_size) {
                     free(data);
                     return DB_TRUNCATED;
                  }

                  strcpy(buffer, line);
                  buffer += strlen(line);
                  *buffer_size -= strlen(line);

                  /* copy multiple lines to buffer */
                  if (key.item_size > *buffer_size) {
                     free(data);
                     return DB_TRUNCATED;
                  }

                  strcpy(buffer, data);
                  buffer += strlen(data);
                  *buffer_size -= strlen(data);

                  strcpy(line, "\n====#$@$#====\n");
               } else {
                  db_sprintf(str, data, key.item_size, 0, key.type);

                  if (key.type == TID_STRING || key.type == TID_LINK)
                     sprintf(line + strlen(line), "[%d] ", key.item_size);

                  sprintf(line + strlen(line), "%s\n", str);
               }
            } else {
               sprintf(line, "%s = %s[%d] :\n", key.name, tid_name[key.type], key.num_values);

               for (j = 0; j < key.num_values; j++) {
                  if (key.type == TID_STRING || key.type == TID_LINK)
                     sprintf(line + strlen(line), "[%d] ", key.item_size);
                  else
                     sprintf(line + strlen(line), "[%d] ", j);

                  db_sprintf(str, data, key.item_size, j, key.type);
                  sprintf(line + strlen(line), "%s\n", str);

                  /* copy line to buffer */
                  if ((INT) (strlen(line) + 1) > *buffer_size) {
                     free(data);
                     return DB_TRUNCATED;
                  }

                  strcpy(buffer, line);
                  buffer += strlen(line);
                  *buffer_size -= strlen(line);
                  line[0] = 0;
               }
            }
         }

         /* copy line to buffer */
         if ((INT) (strlen(line) + 1) > *buffer_size) {
            free(data);
            return DB_TRUNCATED;
         }

         strcpy(buffer, line);
         buffer += strlen(line);
         *buffer_size -= strlen(line);

         free(data);
         data = NULL;
      }

      if (!hSubkey)
         break;

      status = db_get_link(hDB, hSubkey, &key);
      if (status != DB_SUCCESS)
         continue;

      if (strcmp(key.name, "arr2") == 0)
         printf("\narr2\n");
      size = key.total_size;
      data = (char *) malloc(size);
      if (data == NULL) {
         cm_msg(MERROR, "db_copy", "cannot allocate data buffer");
         return DB_NO_MEMORY;
      }

      line[0] = 0;

      if (key.type == TID_KEY) {
         /* new line */
         if (bWritten) {
            if (*buffer_size < 2) {
               free(data);
               return DB_TRUNCATED;
            }

            strcpy(buffer, "\n");
            buffer += 1;
            *buffer_size -= 1;
         }

         strcpy(str, full_path);
         if (str[0] && str[strlen(str) - 1] != '/')
            strcat(str, "/");
         strcat(str, key.name);

         /* recurse */
         status = db_copy(hDB, hSubkey, buffer, buffer_size, str);
         if (status != DB_SUCCESS) {
            free(data);
            return status;
         }

         buffer += strlen(buffer);
         bWritten = FALSE;
      } else {
         status = db_get_link_data(hDB, hSubkey, data, &size, key.type);
         if (status != DB_SUCCESS)
            continue;

         if (!bWritten) {
            if (path[0] == 0)
               sprintf(line, "[.]\n");
            else
               sprintf(line, "[%s]\n", path);
            bWritten = TRUE;
         }

         if (key.num_values == 1) {
            sprintf(line + strlen(line), "%s = %s : ", key.name, tid_name[key.type]);

            if (key.type == TID_STRING && strchr(data, '\n') != NULL) {
               /* multiline string */
               sprintf(line + strlen(line), "[====#$@$#====]\n");

               /* ensure string limiter */
               data[size - 1] = 0;

               /* copy line to buffer */
               if ((INT) (strlen(line) + 1) > *buffer_size) {
                  free(data);
                  return DB_TRUNCATED;
               }

               strcpy(buffer, line);
               buffer += strlen(line);
               *buffer_size -= strlen(line);

               /* copy multiple lines to buffer */
               if (key.item_size > *buffer_size) {
                  free(data);
                  return DB_TRUNCATED;
               }

               strcpy(buffer, data);
               buffer += strlen(data);
               *buffer_size -= strlen(data);

               strcpy(line, "\n====#$@$#====\n");
            } else {
               db_sprintf(str, data, key.item_size, 0, key.type);

               if (key.type == TID_STRING || key.type == TID_LINK)
                  sprintf(line + strlen(line), "[%d] ", key.item_size);

               sprintf(line + strlen(line), "%s\n", str);
            }
         } else {
            sprintf(line + strlen(line), "%s = %s[%d] :\n", key.name, tid_name[key.type], key.num_values);

            for (j = 0; j < key.num_values; j++) {
               if (key.type == TID_STRING || key.type == TID_LINK)
                  sprintf(line + strlen(line), "[%d] ", key.item_size);
               else
                  sprintf(line + strlen(line), "[%d] ", j);

               db_sprintf(str, data, key.item_size, j, key.type);
               sprintf(line + strlen(line), "%s\n", str);

               /* copy line to buffer */
               if ((INT) (strlen(line) + 1) > *buffer_size) {
                  free(data);
                  return DB_TRUNCATED;
               }

               strcpy(buffer, line);
               buffer += strlen(line);
               *buffer_size -= strlen(line);
               line[0] = 0;
            }
         }

         /* copy line to buffer */
         if ((INT) (strlen(line) + 1) > *buffer_size) {
            free(data);
            return DB_TRUNCATED;
         }

         strcpy(buffer, line);
         buffer += strlen(line);
         *buffer_size -= strlen(line);
      }

      free(data);
      data = NULL;
   }

   if (bWritten) {
      if (*buffer_size < 2)
         return DB_TRUNCATED;

      strcpy(buffer, "\n");
      buffer += 1;
      *buffer_size -= 1;
   }

   return DB_SUCCESS;
}

/********************************************************************/
/**
Copy an ODB subtree in ASCII format from a buffer
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKeyRoot Handle for key where search starts, zero for root.
@param buffer NULL-terminated buffer
@return DB_SUCCESS, DB_TRUNCATED, DB_NO_MEMORY
*/
INT db_paste(HNDLE hDB, HNDLE hKeyRoot, const char *buffer)
{
   char line[MAX_STRING_LENGTH];
   char title[MAX_STRING_LENGTH];
   char key_name[MAX_STRING_LENGTH];
   char data_str[MAX_STRING_LENGTH + 50];
   char test_str[MAX_STRING_LENGTH];
   char *pc, *data;
   const char *pold;
   INT data_size;
   INT tid, i, j, n_data, string_length, status, size;
   HNDLE hKey;
   KEY root_key;
   BOOL multi_line;

   title[0] = 0;
   multi_line = FALSE;

   if (hKeyRoot == 0)
      db_find_key(hDB, hKeyRoot, "", &hKeyRoot);

   db_get_key(hDB, hKeyRoot, &root_key);

   /* initial data size */
   data_size = 1000;
   data = (char *) malloc(data_size);
   if (data == NULL) {
      cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
      return DB_NO_MEMORY;
   }

   do {
      if (*buffer == 0)
         break;

      for (i = 0; *buffer != '\n' && *buffer && i < MAX_STRING_LENGTH; i++)
         line[i] = *buffer++;

      if (i == MAX_STRING_LENGTH) {
         cm_msg(MERROR, "db_paste", "line too long");
         free(data);
         return DB_TRUNCATED;
      }

      line[i] = 0;
      if (*buffer == '\n')
         buffer++;

      /* check if it is a section title */
      if (line[0] == '[') {
         /* extract title and append '/' */
         strlcpy(title, line + 1, sizeof(title));
         if (strchr(title, ']'))
            *strchr(title, ']') = 0;
         if (title[0] && title[strlen(title) - 1] != '/')
            strlcat(title, "/", sizeof(title));
      } else {
         /* valid data line if it includes '=' and no ';' */
         if (strchr(line, '=') && line[0] != ';') {
            /* copy type info and data */
            pc = strrchr(line, '=') + 1;
            while (strstr(line, ": [") != NULL && strstr(line, ": [") < pc) {
               pc -= 2;
               while (*pc != '=' && pc > line)
                  pc--;
               pc++;
            }
            while (*pc == ' ')
               pc++;
            strlcpy(data_str, pc, sizeof(data_str));

            /* extract key name */
            *strrchr(line, '=') = 0;
            while (strstr(line, ": [") && strchr(line, '='))
               *strrchr(line, '=') = 0;

            pc = &line[strlen(line) - 1];
            while (*pc == ' ')
               *pc-- = 0;

            key_name[0] = 0;
            if (title[0] != '.')
               strlcpy(key_name, title, sizeof(key_name));

            strlcat(key_name, line, sizeof(key_name));

            /* evaluate type info */
            strlcpy(line, data_str, sizeof(line));
            if (strchr(line, ' '))
               *strchr(line, ' ') = 0;

            n_data = 1;
            if (strchr(line, '[')) {
               n_data = atol(strchr(line, '[') + 1);
               *strchr(line, '[') = 0;
            }

            for (tid = 0; tid < TID_LAST; tid++)
               if (strcmp(tid_name[tid], line) == 0)
                  break;

            string_length = 0;

            if (tid == TID_LAST)
               cm_msg(MERROR, "db_paste", "found unknown data type \"%s\" in ODB file", line);
            else {
               /* skip type info */
               pc = data_str;
               while (*pc != ' ' && *pc)
                  pc++;
               while ((*pc == ' ' || *pc == ':') && *pc)
                  pc++;
               strlcpy(data_str, pc, sizeof(data_str));

               if (n_data > 1) {
                  data_str[0] = 0;
                  if (!*buffer)
                     break;

                  for (j = 0; *buffer != '\n' && *buffer; j++)
                     data_str[j] = *buffer++;
                  data_str[j] = 0;
                  if (*buffer == '\n')
                     buffer++;
               }

               for (i = 0; i < n_data; i++) {
                  /* strip trailing \n */
                  pc = &data_str[strlen(data_str) - 1];
                  while (*pc == '\n' || *pc == '\r')
                     *pc-- = 0;

                  if (tid == TID_STRING || tid == TID_LINK) {
                     if (!string_length) {
                        if (data_str[1] == '=')
                           string_length = -1;
                        else
                           string_length = atoi(data_str + 1);
                        if (string_length > MAX_STRING_LENGTH) {
                           string_length = MAX_STRING_LENGTH;
                           cm_msg(MERROR, "db_paste", "found string exceeding MAX_STRING_LENGTH");
                        }
                     }

                     if (string_length == -1) {
                        /* multi-line string */
                        if (strstr(buffer, "\n====#$@$#====\n") != NULL) {
                           string_length = (POINTER_T) strstr(buffer, "\n====#$@$#====\n") - (POINTER_T) buffer + 1;

                           if (string_length >= data_size) {
                              data_size += string_length + 100;
                              data = (char *) realloc(data, data_size);
                              if (data == NULL) {
                                 cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
                                 return DB_NO_MEMORY;
                              }
                           }

                           memset(data, 0, data_size);
                           strncpy(data, buffer, string_length);
                           data[string_length - 1] = 0;
                           buffer = strstr(buffer, "\n====#$@$#====\n") + strlen("\n====#$@$#====\n");
                        } else
                           cm_msg(MERROR, "db_paste", "found multi-line string without termination sequence");
                     } else {
                        pc = data_str + 2;
                        while (*pc && *pc != ' ')
                           pc++;
                        while (*pc && *pc == ' ')
                           pc++;

                        /* limit string size */
                        *(pc + string_length - 1) = 0;

                        /* increase data buffer if necessary */
                        if (string_length * (i + 1) >= data_size) {
                           data_size += 1000;
                           data = (char *) realloc(data, data_size);
                           if (data == NULL) {
                              cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
                              return DB_NO_MEMORY;
                           }
                        }

                        strlcpy(data + string_length * i, pc, string_length);
                     }
                  } else {
                     pc = data_str;

                     if (n_data > 1 && data_str[0] == '[') {
                        pc = strchr(data_str, ']') + 1;
                        while (*pc && *pc == ' ')
                           pc++;
                     }

                     db_sscanf(pc, data, &size, i, tid);

                     /* increase data buffer if necessary */
                     if (size * (i + 1) >= data_size) {
                        data_size += 1000;
                        data = (char *) realloc(data, data_size);
                        if (data == NULL) {
                           cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
                           return DB_NO_MEMORY;
                        }
                     }

                  }

                  if (i < n_data - 1) {
                     data_str[0] = 0;
                     if (!*buffer)
                        break;

                     pold = buffer;

                     for (j = 0; *buffer != '\n' && *buffer; j++)
                        data_str[j] = *buffer++;
                     data_str[j] = 0;
                     if (*buffer == '\n')
                        buffer++;

                     /* test if valid data */
                     if (tid != TID_STRING && tid != TID_LINK) {
                        if (data_str[0] == 0 || (strchr(data_str, '=')
                                                 && strchr(data_str, ':')))
                           buffer = pold;
                     }
                  }
               }

               /* skip system client entries */
               strcpy(test_str, key_name);
               test_str[15] = 0;

               if (!equal_ustring(test_str, "/System/Clients")) {
                  if (root_key.type != TID_KEY) {
                     /* root key is destination key */
                     hKey = hKeyRoot;
                  } else {
                     /* create key and set value */
                     if (key_name[0] == '/') {
                        status = db_find_link(hDB, 0, key_name, &hKey);
                        if (status == DB_NO_KEY) {
                           db_create_key(hDB, 0, key_name, tid);
                           status = db_find_link(hDB, 0, key_name, &hKey);
                        }
                     } else {
                        status = db_find_link(hDB, hKeyRoot, key_name, &hKey);
                        if (status == DB_NO_KEY) {
                           db_create_key(hDB, hKeyRoot, key_name, tid);
                           status = db_find_link(hDB, hKeyRoot, key_name, &hKey);
                        }
                     }
                  }

                  /* set key data if created sucessfully */
                  if (hKey) {
                     if (tid == TID_STRING || tid == TID_LINK)
                        db_set_link_data(hDB, hKey, data, string_length * n_data, n_data, tid);
                     else
                        db_set_link_data(hDB, hKey, data, rpc_tid_size(tid) * n_data, n_data, tid);
                  }
               }
            }
         }
      }
   } while (TRUE);

   free(data);
   return DB_SUCCESS;
}

/********************************************************************/
/* 
  Only internally used by db_paste_xml                             
*/
int db_paste_node(HNDLE hDB, HNDLE hKeyRoot, PMXML_NODE node)
{
   char type[256], data[256], test_str[256], buf[10000];
   int i, index, status, size, tid, num_values;
   HNDLE hKey;
   PMXML_NODE child;

   if (strcmp(mxml_get_name(node), "odb") == 0) {
      for (i = 0; i < mxml_get_number_of_children(node); i++) {
         status = db_paste_node(hDB, hKeyRoot, mxml_subnode(node, i));
         if (status != DB_SUCCESS)
            return status;
      }
   } else if (strcmp(mxml_get_name(node), "dir") == 0) {
      status = db_find_link(hDB, hKeyRoot, mxml_get_attribute(node, "name"), &hKey);

      /* skip system client entries */
      strlcpy(test_str, mxml_get_attribute(node, "name"), sizeof(test_str));
      test_str[15] = 0;
      if (equal_ustring(test_str, "/System/Clients"))
         return DB_SUCCESS;

      if (status == DB_NO_KEY) {
         status = db_create_key(hDB, hKeyRoot, mxml_get_attribute(node, "name"), TID_KEY);
         if (status == DB_NO_ACCESS)
            return DB_SUCCESS;  /* key or tree is locked, just skip it */

         if (status != DB_SUCCESS && status != DB_KEY_EXIST) {
            cm_msg(MERROR, "db_paste_node",
                   "cannot create key \"%s\" in ODB, status = %d", mxml_get_attribute(node, "name"), status);
            return status;
         }
         status = db_find_link(hDB, hKeyRoot, mxml_get_attribute(node, "name"), &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_node", "cannot find key \"%s\" in ODB", mxml_get_attribute(node, "name"));
            return status;
         }
      }

      db_get_path(hDB, hKey, data, sizeof(data));
      if (strncmp(data, "/System/Clients", 15) != 0) {
         for (i = 0; i < mxml_get_number_of_children(node); i++) {
            status = db_paste_node(hDB, hKey, mxml_subnode(node, i));
            if (status != DB_SUCCESS)
               return status;
         }
      }
   } else if (strcmp(mxml_get_name(node), "key") == 0 || strcmp(mxml_get_name(node), "keyarray") == 0) {

      if (strcmp(mxml_get_name(node), "keyarray") == 0)
         num_values = atoi(mxml_get_attribute(node, "num_values"));
      else
         num_values = 0;

      if (mxml_get_attribute(node, "type") == NULL) {
         cm_msg(MERROR, "db_paste_node", "found key \"%s\" with no type in XML data", mxml_get_name(node));
         return DB_TYPE_MISMATCH;
      }

      strlcpy(type, mxml_get_attribute(node, "type"), sizeof(type));
      for (tid = 0; tid < TID_LAST; tid++)
         if (strcmp(tid_name[tid], type) == 0)
            break;
      if (tid == TID_LAST) {
         cm_msg(MERROR, "db_paste_node", "found unknown data type \"%s\" in XML data", type);
         return DB_TYPE_MISMATCH;
      }

      status = db_find_link(hDB, hKeyRoot, mxml_get_attribute(node, "name"), &hKey);
      if (status == DB_NO_KEY) {
         status = db_create_key(hDB, hKeyRoot, mxml_get_attribute(node, "name"), tid);
         if (status == DB_NO_ACCESS)
            return DB_SUCCESS;  /* key or tree is locked, just skip it */

         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_node",
                   "cannot create key \"%s\" in ODB, status = %d", mxml_get_attribute(node, "name"), status);
            return status;
         }
         status = db_find_link(hDB, hKeyRoot, mxml_get_attribute(node, "name"), &hKey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_node",
                   "cannot find key \"%s\" in ODB, status = %d", mxml_get_attribute(node, "name"));
            return status;
         }
      }

      if (num_values) {
         /* evaluate array */
         for (i = 0; i < mxml_get_number_of_children(node); i++) {
            child = mxml_subnode(node, i);
            if (mxml_get_attribute(child, "index"))
               index = atoi(mxml_get_attribute(child, "index"));
            else
               index = i;
            if (tid == TID_STRING || tid == TID_LINK) {
               size = atoi(mxml_get_attribute(node, "size"));
               if (mxml_get_value(child) == NULL)
                  db_set_data_index(hDB, hKey, "", size, i, tid);
               else {
                  strlcpy(buf, mxml_get_value(child), sizeof(buf));
                  db_set_data_index(hDB, hKey, buf, size, index, tid);
               }
            } else {
               db_sscanf(mxml_get_value(child), data, &size, 0, tid);
               db_set_data_index(hDB, hKey, data, rpc_tid_size(tid), index, tid);
            }
         }

      } else {                  /* single value */
         if (tid == TID_STRING || tid == TID_LINK) {
            size = atoi(mxml_get_attribute(node, "size"));
            if (mxml_get_value(node) == NULL)
               db_set_data(hDB, hKey, "", size, 1, tid);
            else
               db_set_data(hDB, hKey, mxml_get_value(node), size, 1, tid);
         } else {
            db_sscanf(mxml_get_value(node), data, &size, 0, tid);
            db_set_data(hDB, hKey, data, rpc_tid_size(tid), 1, tid);
         }
      }
   }

   return DB_SUCCESS;
}

/********************************************************************/
/**
Paste an ODB subtree in XML format from a buffer
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKeyRoot Handle for key where search starts, zero for root.
@param buffer NULL-terminated buffer
@return DB_SUCCESS, DB_INVALID_PARAM, DB_NO_MEMORY, DB_TYPE_MISMATCH
*/
INT db_paste_xml(HNDLE hDB, HNDLE hKeyRoot, const char *buffer)
{
   char error[256];
   INT status;
   PMXML_NODE tree, node;

   if (hKeyRoot == 0)
      db_find_key(hDB, hKeyRoot, "", &hKeyRoot);

   /* parse XML buffer */
   tree = mxml_parse_buffer(buffer, error, sizeof(error), NULL);
   if (tree == NULL) {
      puts(error);
      return DB_TYPE_MISMATCH;
   }

   node = mxml_find_node(tree, "odb");
   if (node == NULL) {
      puts("Cannot find element \"odb\" in XML data");
      return DB_TYPE_MISMATCH;
   }

   status = db_paste_node(hDB, hKeyRoot, node);

   mxml_free_tree(tree);

   return status;
}

/********************************************************************/
/**
Copy an ODB subtree in XML format to a buffer

@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param buffer ASCII buffer which receives ODB contents.
@param buffer_size Size of buffer, returns remaining space in buffer.
@return DB_SUCCESS, DB_TRUNCATED, DB_NO_MEMORY
*/
INT db_copy_xml(HNDLE hDB, HNDLE hKey, char *buffer, INT * buffer_size)
{
#ifdef LOCAL_ROUTINES
   {
      INT len;
      char *p, str[256];
      MXML_WRITER *writer;

      /* open file */
      writer = mxml_open_buffer();
      if (writer == NULL) {
         cm_msg(MERROR, "db_copy_xml", "Cannot allocate buffer");
         return DB_NO_MEMORY;
      }

      db_get_path(hDB, hKey, str, sizeof(str));

      /* write XML header */
      mxml_start_element(writer, "odb");
      mxml_write_attribute(writer, "root", str);
      mxml_write_attribute(writer, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
      mxml_write_attribute(writer, "xsi:noNamespaceSchemaLocation", "http://midas.psi.ch/odb.xsd");

      db_save_xml_key(hDB, hKey, 0, writer);

      mxml_end_element(writer); // "odb"
      p = mxml_close_buffer(writer);

      strlcpy(buffer, p, *buffer_size);
      len = strlen(p);
      free(p);
      p = NULL;
      if (len > *buffer_size) {
         *buffer_size = 0;
         return DB_TRUNCATED;
      }

      *buffer_size -= len;
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
void name2c(char *str)
/********************************************************************\

  Routine: name2c

  Purpose: Convert key name to C name. Internal use only.

\********************************************************************/
{
   if (*str >= '0' && *str <= '9')
      *str = '_';

   while (*str) {
      if (!(*str >= 'a' && *str <= 'z') && !(*str >= 'A' && *str <= 'Z') && !(*str >= '0' && *str <= '9'))
         *str = '_';
      *str = (char) tolower(*str);
      str++;
   }
}

/*------------------------------------------------------------------*/
static void db_save_tree_struct(HNDLE hDB, HNDLE hKey, int hfile, INT level)
/********************************************************************\

  Routine: db_save_tree_struct

  Purpose: Save database tree as a C structure. Gets called by
           db_save_struct(). Internal use only.

\********************************************************************/
{
   INT i, idx;
   KEY key;
   HNDLE hSubkey;
   char line[MAX_ODB_PATH], str[MAX_STRING_LENGTH];

   /* first enumerate this level */
   for (idx = 0;; idx++) {
      db_enum_key(hDB, hKey, idx, &hSubkey);

      if (!hSubkey)
         break;

      db_get_key(hDB, hSubkey, &key);

      if (key.type != TID_KEY) {
         for (i = 0; i <= level; i++)
            write(hfile, "  ", 2);

         switch (key.type) {
         case TID_SBYTE:
         case TID_CHAR:
            strcpy(line, "char");
            break;
         case TID_SHORT:
            strcpy(line, "short");
            break;
         case TID_FLOAT:
            strcpy(line, "float");
            break;
         case TID_DOUBLE:
            strcpy(line, "double");
            break;
         case TID_BITFIELD:
            strcpy(line, "unsigned char");
            break;
         case TID_STRING:
            strcpy(line, "char");
            break;
         case TID_LINK:
            strcpy(line, "char");
            break;
         default:
            strcpy(line, tid_name[key.type]);
            break;
         }

         strcat(line, "                    ");
         strcpy(str, key.name);
         name2c(str);

         if (key.num_values > 1)
            sprintf(str + strlen(str), "[%d]", key.num_values);
         if (key.type == TID_STRING || key.type == TID_LINK)
            sprintf(str + strlen(str), "[%d]", key.item_size);

         strcpy(line + 10, str);
         strcat(line, ";\n");

         write(hfile, line, strlen(line));
      } else {
         /* recurse subtree */
         for (i = 0; i <= level; i++)
            write(hfile, "  ", 2);

         sprintf(line, "struct {\n");
         write(hfile, line, strlen(line));
         db_save_tree_struct(hDB, hSubkey, hfile, level + 1);

         for (i = 0; i <= level; i++)
            write(hfile, "  ", 2);

         strcpy(str, key.name);
         name2c(str);

         sprintf(line, "} %s;\n", str);
         write(hfile, line, strlen(line));
      }
   }
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Save a branch of a database to an .ODB file

This function is used by the ODBEdit command save. For a
description of the ASCII format, see db_copy(). Data of the whole ODB can
be saved (hkey equal zero) or only a sub-tree.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param filename Filename of .ODB file.
@param bRemote Flag for saving database on remote server.
@return DB_SUCCESS, DB_FILE_ERROR
*/
INT db_save(HNDLE hDB, HNDLE hKey, const char *filename, BOOL bRemote)
{
   if (rpc_is_remote() && bRemote)
      return rpc_call(RPC_DB_SAVE, hDB, hKey, filename, bRemote);

#ifdef LOCAL_ROUTINES
   {
      INT hfile, size, buffer_size, n, status;
      char *buffer, path[256];

      /* open file */
      hfile = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT, 0644);
      if (hfile == -1) {
         cm_msg(MERROR, "db_save", "Cannot open file \"%s\"", filename);
         return DB_FILE_ERROR;
      }

      db_get_path(hDB, hKey, path, sizeof(path));

      buffer_size = 10000;
      do {
         buffer = (char *) malloc(buffer_size);
         if (buffer == NULL) {
            cm_msg(MERROR, "db_save", "cannot allocate ODB dump buffer");
            break;
         }

         size = buffer_size;
         status = db_copy(hDB, hKey, buffer, &size, path);
         if (status != DB_TRUNCATED) {
            n = write(hfile, buffer, buffer_size - size);
            free(buffer);
            buffer = NULL;

            if (n != buffer_size - size) {
               cm_msg(MERROR, "db_save", "cannot save .ODB file");
               close(hfile);
               return DB_FILE_ERROR;
            }
            break;
         }

         /* increase buffer size if truncated */
         free(buffer);
         buffer = NULL;
         buffer_size *= 2;
      } while (1);

      close(hfile);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

void xml_encode(char *src, int size)
{
   int i;
   char *dst, *p;

   dst = (char *) malloc(size);
   if (dst == NULL)
      return;

   *dst = 0;
   for (i = 0; i < (int) strlen(src); i++) {
      switch (src[i]) {
      case '<':
         strlcat(dst, "&lt;", size);
         break;
      case '>':
         strlcat(dst, "&gt;", size);
         break;
      case '&':
         strlcat(dst, "&amp;", size);
         break;
      case '\"':
         strlcat(dst, "&quot;", size);
         break;
      case '\'':
         strlcat(dst, "&apos;", size);
         break;
      default:
         if ((int) strlen(dst) >= size) {
            free(dst);
            return;
         }
         p = dst + strlen(dst);
         *p = src[i];
         *(p + 1) = 0;
      }
   }

   strlcpy(src, dst, size);
}

/*------------------------------------------------------------------*/

INT db_save_xml_key(HNDLE hDB, HNDLE hKey, INT level, MXML_WRITER * writer)
{
   INT i, idx, size, status;
   char str[MAX_STRING_LENGTH * 2], *data;
   HNDLE hSubkey;
   KEY key;

   status = db_get_link(hDB, hKey, &key);
   if (status != DB_SUCCESS)
      return status;

   if (key.type == TID_KEY) {

      /* save opening tag for subtree */

      if (level > 0) {
         mxml_start_element(writer, "dir");
         mxml_write_attribute(writer, "name", key.name);
      }

      for (idx = 0;; idx++) {
         db_enum_link(hDB, hKey, idx, &hSubkey);

         if (!hSubkey)
            break;

         /* save subtree */
         status = db_save_xml_key(hDB, hSubkey, level + 1, writer);
         if (status != DB_SUCCESS)
            return status;
      }

      /* save closing tag for subtree */
      if (level > 0)
         mxml_end_element(writer);

   } else {

      /* save key value */

      if (key.num_values > 1)
         mxml_start_element(writer, "keyarray");
      else
         mxml_start_element(writer, "key");
      mxml_write_attribute(writer, "name", key.name);
      mxml_write_attribute(writer, "type", rpc_tid_name(key.type));

      if (key.type == TID_STRING || key.type == TID_LINK) {
         sprintf(str, "%d", key.item_size);
         mxml_write_attribute(writer, "size", str);
      }

      if (key.num_values > 1) {
         sprintf(str, "%d", key.num_values);
         mxml_write_attribute(writer, "num_values", str);
      }

      size = key.total_size;
      data = (char *) malloc(size);
      if (data == NULL) {
         cm_msg(MERROR, "db_save_xml_key", "cannot allocate data buffer");
         return DB_NO_MEMORY;
      }

      db_get_link_data(hDB, hKey, data, &size, key.type);

      if (key.num_values == 1) {

         db_sprintf(str, data, key.item_size, 0, key.type);
         mxml_write_value(writer, str);
         mxml_end_element(writer);

      } else {                  /* array of values */

         for (i = 0; i < key.num_values; i++) {

            mxml_start_element(writer, "value");
            sprintf(str, "%d", i);
            mxml_write_attribute(writer, "index", str);
            db_sprintf(str, data, key.item_size, i, key.type);
            mxml_write_value(writer, str);
            mxml_end_element(writer);
         }

         mxml_end_element(writer);      /* keyarray */
      }

      free(data);
      data = NULL;
   }

   return DB_SUCCESS;
}

/********************************************************************/
/**
Save a branch of a database to an .xml file

This function is used by the ODBEdit command save to write the contents
of the ODB into a XML file. Data of the whole ODB can
be saved (hkey equal zero) or only a sub-tree.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param filename Filename of .XML file.
@return DB_SUCCESS, DB_FILE_ERROR
*/
INT db_save_xml(HNDLE hDB, HNDLE hKey, const char *filename)
{
#ifdef LOCAL_ROUTINES
   {
      INT status;
      char str[256];
      MXML_WRITER *writer;

      /* open file */
      writer = mxml_open_file(filename);
      if (writer == NULL) {
         cm_msg(MERROR, "db_save_xml", "Cannot open file \"%s\"", filename);
         return DB_FILE_ERROR;
      }

      db_get_path(hDB, hKey, str, sizeof(str));

      /* write XML header */
      mxml_start_element(writer, "odb");
      mxml_write_attribute(writer, "root", str);
      mxml_write_attribute(writer, "filename", filename);
      mxml_write_attribute(writer, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");

      if (getenv("MIDASSYS"))
         strcpy(str, getenv("MIDASSYS"));
      else
         strcpy(str, "");
      strcat(str, DIR_SEPARATOR_STR);
      strcat(str, "odb.xsd");
      mxml_write_attribute(writer, "xsi:noNamespaceSchemaLocation", str);

      status = db_save_xml_key(hDB, hKey, 0, writer);

      mxml_end_element(writer); // "odb"
      mxml_close_file(writer);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

static void json_write(char **buffer, int* buffer_size, int* buffer_end, int level, const char* s, int quoted)
{
   int len = strlen(s);
   int remain = *buffer_size - *buffer_end;
   assert(remain >= 0);

   int xlevel = 2*level;

   while (10 + xlevel + 3*len > remain) {
      // reallocate the buffer
      int new_buffer_size = 2*(*buffer_size);
      if (new_buffer_size < 4*1024)
         new_buffer_size = 4*1024;
      //printf("reallocate: len %d, size %d, remain %d, allocate %d\n", len, *buffer_size, remain, new_buffer_size);
      *buffer = realloc(*buffer, new_buffer_size);
      assert(*buffer);
      *buffer_size = new_buffer_size;
      remain = *buffer_size - *buffer_end;
      assert(remain >= 0);
   }

   if (xlevel) {
      int i;
      for (i=0; i<xlevel; i++)
         (*buffer)[(*buffer_end)++] = ' ';
   }

   if (!quoted) {
      memcpy(*buffer + *buffer_end, s, len);
      *buffer_end += len;
      (*buffer)[*buffer_end] = 0; // NUL-terminate the buffer
      return;
   }

   (*buffer)[(*buffer_end)++] = '"';

   while (*s) {
      switch (*s) {
      case '\"':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = '\"';
         s++;
         break;
      case '\\':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = '\\';
         s++;
         break;
#if 0
      case '/':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = '/';
         s++;
         break;
#endif
      case '\b':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = 'b';
         s++;
         break;
      case '\f':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = 'f';
         s++;
         break;
      case '\n':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = 'n';
         s++;
         break;
      case '\r':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = 'r';
         s++;
         break;
      case '\t':
         (*buffer)[(*buffer_end)++] = '\\';
         (*buffer)[(*buffer_end)++] = 't';
         s++;
         break;
      default:
         (*buffer)[(*buffer_end)++] = *s++;
      }
   }

   (*buffer)[(*buffer_end)++] = '"';
   (*buffer)[*buffer_end] = 0; // NUL-terminate the buffer

   remain = *buffer_size - *buffer_end;
   assert(remain > 0);
}

INT db_save_json_key(HNDLE hDB, HNDLE hKey, INT level, char **buffer, int* buffer_size, int* buffer_end, int save_keys, int follow_links)
{
   INT i, idx, size, status;
   char *data;
   HNDLE hSubkey;
   KEY key;

   if (follow_links)
      status = db_get_key(hDB, hKey, &key);
   else
      status = db_get_link(hDB, hKey, &key);

   if (status != DB_SUCCESS)
      return status;

   if (key.type == TID_KEY) {

      json_write(buffer, buffer_size, buffer_end, level, key.name, 1);

      json_write(buffer, buffer_size, buffer_end, 0, " : {", 0);

      for (idx = 0;; idx++) {
         if (follow_links)
            db_enum_key(hDB, hKey, idx, &hSubkey);
         else
            db_enum_link(hDB, hKey, idx, &hSubkey);

         if (!hSubkey)
            break;

         if (idx != 0) {
            json_write(buffer, buffer_size, buffer_end, 0, ",", 0);
         }

         json_write(buffer, buffer_size, buffer_end, 0, "\n", 0);

         /* save subtree */
         status = db_save_json_key(hDB, hSubkey, level + 1, buffer, buffer_size, buffer_end, save_keys, follow_links);
         if (status != DB_SUCCESS)
            return status;
      }

      json_write(buffer, buffer_size, buffer_end, 0, "\n", 0);
      json_write(buffer, buffer_size, buffer_end, level, "}", 0);

   } else {

      json_write(buffer, buffer_size, buffer_end, level, key.name, 1);

      if (key.num_values > 1) {
         json_write(buffer, buffer_size, buffer_end, 0, " : [ ", 0);
      } else {
         json_write(buffer, buffer_size, buffer_end, 0, " : ", 0);
      }

      size = key.total_size;
      data = (char *) malloc(size);
      if (data == NULL) {
         cm_msg(MERROR, "db_save_json_key", "cannot allocate data buffer for %d bytes", size);
         return DB_NO_MEMORY;
      }

      if (follow_links)
         status = db_get_data(hDB, hKey, data, &size, key.type);
      else
         status = db_get_link_data(hDB, hKey, data, &size, key.type);
      
      if (status != DB_SUCCESS)
         return status;

      for (i = 0; i < key.num_values; i++) {
         char str[256];
         char *p = data + key.item_size*i;

         if (i != 0)
            json_write(buffer, buffer_size, buffer_end, 0, ", ", 0);

         switch (key.type) {
         case TID_BYTE:
            sprintf(str, "%u", *(unsigned char*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            break;
         case TID_SBYTE:
            sprintf(str, "%d", *(char*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            break;
         case TID_CHAR:
            sprintf(str, "%c", *(char*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 1);
            break;
         case TID_WORD:
            sprintf(str, "\"0x%04x\"", *(WORD*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            break;
         case TID_SHORT:
            sprintf(str, "%d", *(short*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            break;
         case TID_DWORD:
            sprintf(str, "\"0x%08x\"", *(DWORD*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            break;
         case TID_INT:
            sprintf(str, "%d", *(int*)p);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            break;
         case TID_BOOL:
            if (*(int*)p)
               json_write(buffer, buffer_size, buffer_end, 0, "true", 0);
            else
               json_write(buffer, buffer_size, buffer_end, 0, "false", 0);
            break;
         case TID_FLOAT: {
            float flt = (*(float*)p);
            if (flt == 0)
               json_write(buffer, buffer_size, buffer_end, 0, "0", 0);
            else if (flt == (int)flt) {
               sprintf(str, "%.0f", flt);
               json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            } else {
               sprintf(str, "%.7e", flt);
               json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            }
            break;
         }
         case TID_DOUBLE: {
            double dbl = (*(double*)p);
            if (dbl == 0)
               json_write(buffer, buffer_size, buffer_end, 0, "0", 0);
            else if (dbl == (int)dbl) {
               sprintf(str, "%.0f", dbl);
               json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            } else {
               sprintf(str, "%.16e", dbl);
               json_write(buffer, buffer_size, buffer_end, 0, str, 0);
            }
            break;
         }
         case TID_BITFIELD:
            json_write(buffer, buffer_size, buffer_end, 0, "(TID_BITFIELD value)", 1);
            break;
         case TID_STRING:
            p[key.item_size-1] = 0;  // make sure string is NUL terminated!
            json_write(buffer, buffer_size, buffer_end, 0, p, 1);
            break;
         case TID_ARRAY:
            json_write(buffer, buffer_size, buffer_end, 0, "(TID_ARRAY value)", 1);
            break;
         case TID_STRUCT:
            json_write(buffer, buffer_size, buffer_end, 0, "(TID_STRUCT value)", 1);
            break;
         case TID_KEY:
            json_write(buffer, buffer_size, buffer_end, 0, "(TID_KEY value)", 1);
            break;
         case TID_LINK:
            p[key.item_size-1] = 0;  // make sure string is NUL terminated!
            json_write(buffer, buffer_size, buffer_end, 0, p, 1);
            break;
         default:
            json_write(buffer, buffer_size, buffer_end, 0, "(TID_UNKNOWN value)", 1);
         }

      }

      if (key.num_values > 1) {
         json_write(buffer, buffer_size, buffer_end, 0, " ]", 0);
      } else {
         json_write(buffer, buffer_size, buffer_end, 0, "", 0);
      }

      free(data);
      data = NULL;

      /* save key value */
      
      if (save_keys == 1) {
         char str[NAME_LENGTH+15];
         sprintf(str, "%s/key", key.name);

         json_write(buffer, buffer_size, buffer_end, 0, ",\n", 0);

         json_write(buffer, buffer_size, buffer_end, level, str, 1);
         json_write(buffer, buffer_size, buffer_end, 0, " : { ", 0);

         sprintf(str, "\"type\" : %d", key.type);
         json_write(buffer, buffer_size, buffer_end, 0, str, 0);

         if (key.num_values > 1) {
            json_write(buffer, buffer_size, buffer_end, 0, ", ", 0);

            sprintf(str, "\"num_values\" : %d", key.num_values);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
         }

         if (key.type == TID_STRING || key.type == TID_LINK) {
            json_write(buffer, buffer_size, buffer_end, 0, ", ", 0);

            sprintf(str, "\"item_size\" : %d", key.item_size);
            json_write(buffer, buffer_size, buffer_end, 0, str, 0);
         }

         json_write(buffer, buffer_size, buffer_end, 0, ", ", 0);

         sprintf(str, "\"last_written\" : %d", key.last_written);
         json_write(buffer, buffer_size, buffer_end, 0, str, 0);

         json_write(buffer, buffer_size, buffer_end, 0, " ", 0);

         json_write(buffer, buffer_size, buffer_end, 0, "}", 0);
      }

      if (save_keys == 2) {
         char str[NAME_LENGTH+15];
         sprintf(str, "%s/last_written", key.name);

         json_write(buffer, buffer_size, buffer_end, 0, ",\n", 0);

         json_write(buffer, buffer_size, buffer_end, level, str, 1);

         sprintf(str, " : %d", key.last_written);
         json_write(buffer, buffer_size, buffer_end, 0, str, 0);
      }
   }

   return DB_SUCCESS;
}

/********************************************************************/
/**
Copy an ODB subtree in JSON format to a buffer

@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param buffer returns pointer to ASCII buffer with ODB contents
@param buffer_size returns size of ASCII buffer
@param buffer_end returns number of bytes contained in buffer
@return DB_SUCCESS, DB_NO_MEMORY
*/
INT db_copy_json(HNDLE hDB, HNDLE hKey, char **buffer, int* buffer_size, int* buffer_end, int save_keys, int follow_links)
{
   json_write(buffer, buffer_size, buffer_end, 0, "{\n", 0);

   db_save_json_key(hDB, hKey, 1, buffer, buffer_size, buffer_end, save_keys, follow_links);

   json_write(buffer, buffer_size, buffer_end, 0, "\n}\n", 0);
   
   return DB_SUCCESS;
}

/********************************************************************/
/**
Save a branch of a database to an .json file

This function is used by the ODBEdit command save to write the contents
of the ODB into a JSON file. Data of the whole ODB can
be saved (hkey equal zero) or only a sub-tree.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param filename Filename of .json file.
@return DB_SUCCESS, DB_FILE_ERROR
*/
INT db_save_json(HNDLE hDB, HNDLE hKey, const char *filename)
{
#ifdef LOCAL_ROUTINES
   {
      INT status;
      char str[256];
      FILE *fp;

      /* open file */
      fp = fopen(filename, "w");
      if (fp == NULL) {
         cm_msg(MERROR, "db_save_json", "Cannot open file \"%s\"", filename);
         return DB_FILE_ERROR;
      }

      db_get_path(hDB, hKey, str, sizeof(str));

      fprintf(fp, "# MIDAS ODB JSON\n");
      fprintf(fp, "# FILE %s\n", filename);
      fprintf(fp, "# PATH %s\n", str);

      char* buffer = NULL;
      int buffer_size = 0;
      int buffer_end = 0;

      status = db_copy_json(hDB, hKey, &buffer, &buffer_size, &buffer_end, 1, 0);

      if (status == DB_SUCCESS) {
         if (buffer)
            fwrite(buffer, 1, buffer_end, fp);
      }

      if (buffer)
         free(buffer);

      fclose(fp);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Save a branch of a database to a C structure .H file
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param file_name Filename of .ODB file.
@param struct_name Name of structure. If struct_name == NULL,
                   the name of the key is used.
@param append      If TRUE, append to end of existing file
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_FILE_ERROR
*/
INT db_save_struct(HNDLE hDB, HNDLE hKey, const char *file_name, const char *struct_name, BOOL append)
{
   KEY key;
   char str[100], line[100];
   INT status, i, fh;

   /* open file */
   fh = open(file_name, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);

   if (fh == -1) {
      cm_msg(MERROR, "db_save_struct", "Cannot open file\"%s\"", file_name);
      return DB_FILE_ERROR;
   }

   status = db_get_key(hDB, hKey, &key);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_save_struct", "cannot find key");
      return DB_INVALID_HANDLE;
   }

   sprintf(line, "typedef struct {\n");
   write(fh, line, strlen(line));
   db_save_tree_struct(hDB, hKey, fh, 0);

   if (struct_name && struct_name[0])
      strcpy(str, struct_name);
   else
      strcpy(str, key.name);

   name2c(str);
   for (i = 0; i < (int) strlen(str); i++)
      str[i] = (char) toupper(str[i]);

   sprintf(line, "} %s;\n\n", str);
   write(fh, line, strlen(line));

   close(fh);

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_save_string(HNDLE hDB, HNDLE hKey, const char *file_name, const char *string_name, BOOL append)
/********************************************************************\

  Routine: db_save_string

  Purpose: Save a branch of a database as a string which can be used
           by db_create_record.

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to start, 0 for root
    int   fh                File handle to write to
    char  string_name       Name of string. If struct_name == NULL,
                            the name of the key is used.

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
   KEY key;
   char str[256], line[256];
   INT status, i, size, fh, buffer_size;
   char *buffer, *pc;


   /* open file */
   fh = open(file_name, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);

   if (fh == -1) {
      cm_msg(MERROR, "db_save_string", "Cannot open file\"%s\"", file_name);
      return DB_FILE_ERROR;
   }

   status = db_get_key(hDB, hKey, &key);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_save_string", "cannot find key");
      return DB_INVALID_HANDLE;
   }

   if (string_name && string_name[0])
      strcpy(str, string_name);
   else
      strcpy(str, key.name);

   name2c(str);
   for (i = 0; i < (int) strlen(str); i++)
      str[i] = (char) toupper(str[i]);

   sprintf(line, "#define %s(_name) const char *_name[] = {\\\n", str);
   write(fh, line, strlen(line));

   buffer_size = 10000;
   do {
      buffer = (char *) malloc(buffer_size);
      if (buffer == NULL) {
         cm_msg(MERROR, "db_save", "cannot allocate ODB dump buffer");
         break;
      }

      size = buffer_size;
      status = db_copy(hDB, hKey, buffer, &size, "");
      if (status != DB_TRUNCATED)
         break;

      /* increase buffer size if truncated */
      free(buffer);
      buffer = NULL;
      buffer_size *= 2;
   } while (1);


   pc = buffer;

   do {
      i = 0;
      line[i++] = '"';
      while (*pc != '\n' && *pc != 0) {
         if (*pc == '\"' || *pc == '\'')
            line[i++] = '\\';
         line[i++] = *pc++;
      }
      strcpy(&line[i], "\",\\\n");
      if (i > 0)
         write(fh, line, strlen(line));

      if (*pc == '\n')
         pc++;

   } while (*pc);

   sprintf(line, "NULL }\n\n");
   write(fh, line, strlen(line));

   close(fh);
   free(buffer);

   return DB_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Convert a database value to a string according to its type.

This function is a convenient way to convert a binary ODB value into a
string depending on its type if is not known at compile time. If it is known, the
normal sprintf() function can be used.
\code
...
  for (j=0 ; j<key.num_values ; j++)
  {
    db_sprintf(pbuf, pdata, key.item_size, j, key.type);
    strcat(pbuf, "\n");
  }
  ...
\endcode
@param string output ASCII string of data.
@param data Value data.
@param data_size Size of single data element.
@param idx Index for array data.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS
*/
INT db_sprintf(char *string, const void *data, INT data_size, INT idx, DWORD type)
{
   if (data_size == 0)
      sprintf(string, "<NULL>");
   else
      switch (type) {
      case TID_BYTE:
         sprintf(string, "%d", *(((BYTE *) data) + idx));
         break;
      case TID_SBYTE:
         sprintf(string, "%d", *(((char *) data) + idx));
         break;
      case TID_CHAR:
         sprintf(string, "%c", *(((char *) data) + idx));
         break;
      case TID_WORD:
         sprintf(string, "%u", *(((WORD *) data) + idx));
         break;
      case TID_SHORT:
         sprintf(string, "%d", *(((short *) data) + idx));
         break;
      case TID_DWORD:
         sprintf(string, "%u", *(((DWORD *) data) + idx));
         break;
      case TID_INT:
         sprintf(string, "%d", *(((INT *) data) + idx));
         break;
      case TID_BOOL:
         sprintf(string, "%c", *(((BOOL *) data) + idx) ? 'y' : 'n');
         break;
      case TID_FLOAT:
         if (ss_isnan(*(((float *) data) + idx)))
            sprintf(string, "NAN");
         else
            sprintf(string, "%.7g", *(((float *) data) + idx));
         break;
      case TID_DOUBLE:
         if (ss_isnan(*(((double *) data) + idx)))
            sprintf(string, "NAN");
         else
            sprintf(string, "%.16lg", *(((double *) data) + idx));
         break;
      case TID_BITFIELD:
         /* TBD */
         break;
      case TID_STRING:
      case TID_LINK:
         strlcpy(string, ((char *) data) + data_size * idx, MAX_STRING_LENGTH);
         break;
      default:
         sprintf(string, "<unknown>");
         break;
      }

   return DB_SUCCESS;
}

/********************************************************************/
/**
Same as db_sprintf, but with additional format parameter

@param string output ASCII string of data.
@param format Format specifier passed to sprintf()
@param data Value data.
@param data_size Size of single data element.
@param idx Index for array data.
@param type Type of key, one of TID_xxx (see @ref Midas_Data_Types).
@return DB_SUCCESS
*/

INT db_sprintff(char *string, const char *format, const void *data, INT data_size, INT idx, DWORD type)
{
   if (data_size == 0)
      sprintf(string, "<NULL>");
   else
      switch (type) {
      case TID_BYTE:
         sprintf(string, format, *(((BYTE *) data) + idx));
         break;
      case TID_SBYTE:
         sprintf(string, format, *(((char *) data) + idx));
         break;
      case TID_CHAR:
         sprintf(string, format, *(((char *) data) + idx));
         break;
      case TID_WORD:
         sprintf(string, format, *(((WORD *) data) + idx));
         break;
      case TID_SHORT:
         sprintf(string, format, *(((short *) data) + idx));
         break;
      case TID_DWORD:
         sprintf(string, format, *(((DWORD *) data) + idx));
         break;
      case TID_INT:
         sprintf(string, format, *(((INT *) data) + idx));
         break;
      case TID_BOOL:
         sprintf(string, format, *(((BOOL *) data) + idx) ? 'y' : 'n');
         break;
      case TID_FLOAT:
         if (ss_isnan(*(((float *) data) + idx)))
            sprintf(string, "NAN");
         else
            sprintf(string, format, *(((float *) data) + idx));
         break;
      case TID_DOUBLE:
         if (ss_isnan(*(((double *) data) + idx)))
            sprintf(string, "NAN");
         else
            sprintf(string, format, *(((double *) data) + idx));
         break;
      case TID_BITFIELD:
         /* TBD */
         break;
      case TID_STRING:
      case TID_LINK:
         strlcpy(string, ((char *) data) + data_size * idx, MAX_STRING_LENGTH);
         break;
      default:
         sprintf(string, "<unknown>");
         break;
      }

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_sprintfh(char *string, const void *data, INT data_size, INT idx, DWORD type)
/********************************************************************\

  Routine: db_sprintfh

  Purpose: Convert a database value to a string according to its type
           in hex format

  Input:
    void  *data             Value data
    INT   idx               Index for array data
    INT   data_size         Size of single data element
    DWORD type              Valye type, one of TID_xxx

  Output:
    char  *string           ASCII string of data

  Function value:
    DB_SUCCESS              Successful completion

\********************************************************************/
{
   if (data_size == 0)
      sprintf(string, "<NULL>");
   else
      switch (type) {
      case TID_BYTE:
         sprintf(string, "0x%X", *(((BYTE *) data) + idx));
         break;
      case TID_SBYTE:
         sprintf(string, "0x%X", *(((char *) data) + idx));
         break;
      case TID_CHAR:
         sprintf(string, "%c", *(((char *) data) + idx));
         break;
      case TID_WORD:
         sprintf(string, "0x%X", *(((WORD *) data) + idx));
         break;
      case TID_SHORT:
         sprintf(string, "0x%hX", *(((short *) data) + idx));
         break;
      case TID_DWORD:
         sprintf(string, "0x%X", *(((DWORD *) data) + idx));
         break;
      case TID_INT:
         sprintf(string, "0x%X", *(((INT *) data) + idx));
         break;
      case TID_BOOL:
         sprintf(string, "%c", *(((BOOL *) data) + idx) ? 'y' : 'n');
         break;
      case TID_FLOAT:
         if (ss_isnan(*(((float *) data) + idx)))
            sprintf(string, "NAN");
         else
            sprintf(string, "%.7g", *(((float *) data) + idx));
         break;
      case TID_DOUBLE:
         if (ss_isnan(*(((double *) data) + idx)))
            sprintf(string, "NAN");
         else
            sprintf(string, "%.16lg", *(((double *) data) + idx));
         break;
      case TID_BITFIELD:
         /* TBD */
         break;
      case TID_STRING:
      case TID_LINK:
         sprintf(string, "%s", ((char *) data) + data_size * idx);
         break;
      default:
         sprintf(string, "<unknown>");
         break;
      }

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_sscanf(const char *data_str, void *data, INT * data_size, INT i, DWORD tid)
/********************************************************************\

  Routine: db_sscanf

  Purpose: Convert a string to a database value according to its type

  Input:
    char  *data_str         ASCII string of data
    INT   i                 Index for array data
    DWORD tid               Value type, one of TID_xxx

  Output:
    void  *data             Value data
    INT   *data_size        Size of single data element

  Function value:
    DB_SUCCESS              Successful completion

\********************************************************************/
{
   DWORD value;
   BOOL hex = FALSE;

   if (data_str == NULL)
      return 0;

   *data_size = rpc_tid_size(tid);
   if (strncmp(data_str, "0x", 2) == 0) {
      hex = TRUE;
      sscanf(data_str + 2, "%x", &value);
   }

   switch (tid) {
   case TID_BYTE:
   case TID_SBYTE:
      if (hex)
         *((char *) data + i) = (char) value;
      else
         *((char *) data + i) = (char) atoi(data_str);
      break;
   case TID_CHAR:
      *((char *) data + i) = data_str[0];
      break;
   case TID_WORD:
      if (hex)
         *((WORD *) data + i) = (WORD) value;
      else
         *((WORD *) data + i) = (WORD) atoi(data_str);
      break;
   case TID_SHORT:
      if (hex)
         *((short int *) data + i) = (short int) value;
      else
         *((short int *) data + i) = (short int) atoi(data_str);
      break;
   case TID_DWORD:
      if (!hex)
         sscanf(data_str, "%u", &value);

      *((DWORD *) data + i) = value;
      break;
   case TID_INT:
      if (hex)
         *((INT *) data + i) = value;
      else
         *((INT *) data + i) = atol(data_str);
      break;
   case TID_BOOL:
      if (data_str[0] == 'y' || data_str[0] == 'Y' || atoi(data_str) > 0)
         *((BOOL *) data + i) = 1;
      else
         *((BOOL *) data + i) = 0;
      break;
   case TID_FLOAT:
      if (data_str[0] == 'n' || data_str[0] == 'N')
         *((float *) data + i) = (float) ss_nan();
      else
         *((float *) data + i) = (float) atof(data_str);
      break;
   case TID_DOUBLE:
      if (data_str[0] == 'n' || data_str[0] == 'N')
         *((double *) data + i) = ss_nan();
      else
         *((double *) data + i) = atof(data_str);
      break;
   case TID_BITFIELD:
      /* TBD */
      break;
   case TID_STRING:
   case TID_LINK:
      strcpy((char *) data, data_str);
      *data_size = strlen(data_str) + 1;
      break;
   }

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef LOCAL_ROUTINES

static void db_recurse_record_tree(HNDLE hDB, HNDLE hKey, void **data,
                                   INT * total_size, INT base_align, INT * max_align, BOOL bSet, INT convert_flags)
/********************************************************************\

  Routine: db_recurse_record_tree

  Purpose: Recurse a database tree and calculate its size or copy
           data. Internal use only.

\********************************************************************/
{
   DATABASE_HEADER *pheader;
   KEYLIST *pkeylist;
   KEY *pkey;
   INT size, align, corr, total_size_tmp;

   /* get first subkey of hKey */
   pheader = _database[hDB - 1].database_header;
   pkey = (KEY *) ((char *) pheader + hKey);

   pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);
   if (!pkeylist->first_key)
      return;
   pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

   /* first browse through this level */
   do {
      if (pkey->type != TID_KEY) {
         /* correct for alignment */
         align = 1;

         if (rpc_tid_size(pkey->type))
            align = rpc_tid_size(pkey->type) < base_align ? rpc_tid_size(pkey->type) : base_align;

         if (max_align && align > *max_align)
            *max_align = align;

         corr = VALIGN(*total_size, align) - *total_size;
         *total_size += corr;
         if (data)
            *data = (void *) ((char *) (*data) + corr);

         /* calculate data size */
         size = pkey->item_size * pkey->num_values;

         if (data) {
            if (bSet) {
               /* copy data if there is write access */
               if (pkey->access_mode & MODE_WRITE) {
                  memcpy((char *) pheader + pkey->data, *data, pkey->item_size * pkey->num_values);

                  /* convert data */
                  if (convert_flags) {
                     if (pkey->num_values > 1)
                        rpc_convert_data((char *) pheader + pkey->data,
                                         pkey->type, RPC_FIXARRAY, pkey->item_size * pkey->num_values, convert_flags);
                     else
                        rpc_convert_single((char *) pheader + pkey->data, pkey->type, 0, convert_flags);
                  }

                  /* update time */
                  pkey->last_written = ss_time();

                  /* notify clients which have key open */
                  if (pkey->notify_count)
                     db_notify_clients(hDB, (POINTER_T) pkey - (POINTER_T) pheader, FALSE);
               }
            } else {
               /* copy key data if there is read access */
               if (pkey->access_mode & MODE_READ) {
                  memcpy(*data, (char *) pheader + pkey->data, pkey->item_size * pkey->num_values);

                  /* convert data */
                  if (convert_flags) {
                     if (pkey->num_values > 1)
                        rpc_convert_data(*data, pkey->type,
                                         RPC_FIXARRAY | RPC_OUTGOING,
                                         pkey->item_size * pkey->num_values, convert_flags);
                     else
                        rpc_convert_single(*data, pkey->type, RPC_OUTGOING, convert_flags);
                  }
               }
            }

            *data = (char *) (*data) + size;
         }

         *total_size += size;
      } else {
         /* align new substructure according to the maximum
            align value in this structure */
         align = 1;

         total_size_tmp = *total_size;
         db_recurse_record_tree(hDB, (POINTER_T) pkey - (POINTER_T) pheader,
                                NULL, &total_size_tmp, base_align, &align, bSet, convert_flags);

         if (max_align && align > *max_align)
            *max_align = align;

         corr = VALIGN(*total_size, align) - *total_size;
         *total_size += corr;
         if (data)
            *data = (void *) ((char *) (*data) + corr);

         /* now copy subtree */
         db_recurse_record_tree(hDB, (POINTER_T) pkey - (POINTER_T) pheader,
                                data, total_size, base_align, NULL, bSet, convert_flags);

         corr = VALIGN(*total_size, align) - *total_size;
         *total_size += corr;
         if (data)
            *data = (void *) ((char *) (*data) + corr);

         if (bSet && pkey->notify_count)
            db_notify_clients(hDB, (POINTER_T) pkey - (POINTER_T) pheader, FALSE);
      }

      if (!pkey->next_key)
         break;

      pkey = (KEY *) ((char *) pheader + pkey->next_key);
   } while (TRUE);
}

#endif                          /* LOCAL_ROUTINES */

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Calculates the size of a record.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param align Byte alignment calculated by the stub and
              passed to the rpc side to align data
              according to local machine. Must be zero
              when called from user level
@param buf_size Size of record structure
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TYPE_MISMATCH,
DB_STRUCT_SIZE_MISMATCH, DB_NO_KEY
*/
INT db_get_record_size(HNDLE hDB, HNDLE hKey, INT align, INT * buf_size)
{
   if (rpc_is_remote()) {
      align = ss_get_struct_align();
      return rpc_call(RPC_DB_GET_RECORD_SIZE, hDB, hKey, align, buf_size);
   }
#ifdef LOCAL_ROUTINES
   {
      KEY key;
      INT status, max_align;

      if (!align)
         align = ss_get_struct_align();

      /* check if key has subkeys */
      status = db_get_key(hDB, hKey, &key);
      if (status != DB_SUCCESS)
         return status;

      if (key.type != TID_KEY) {
         /* just a single key */
         *buf_size = key.item_size * key.num_values;
         return DB_SUCCESS;
      }

      db_lock_database(hDB);

      /* determine record size */
      *buf_size = max_align = 0;
      db_recurse_record_tree(hDB, hKey, NULL, buf_size, align, &max_align, 0, 0);

      /* correct for byte padding */
      *buf_size = VALIGN(*buf_size, max_align);

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Copy a set of keys to local memory.

An ODB sub-tree can be mapped to a C structure automatically via a
hot-link using the function db_open_record() or manually with this function.
Problems might occur if the ODB sub-tree contains values which don't match the
C structure. Although the structure size is checked against the sub-tree size, no
checking can be done if the type and order of the values in the structure are the
same than those in the ODB sub-tree. Therefore it is recommended to use the
function db_create_record() before db_get_record() is used which
ensures that both are equivalent.
\code
struct {
  INT level1;
  INT level2;
} trigger_settings;
char *trigger_settings_str =
"[Settings]\n\
level1 = INT : 0\n\
level2 = INT : 0";

main()
{
  HNDLE hDB, hkey;
  INT   size;
  ...
  cm_get_experiment_database(&hDB, NULL);
  db_create_record(hDB, 0, "/Equipment/Trigger", trigger_settings_str);
  db_find_key(hDB, 0, "/Equipment/Trigger/Settings", &hkey);
  size = sizeof(trigger_settings);
  db_get_record(hDB, hkey, &trigger_settings, &size, 0);
  ...
}
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param data         Pointer to the retrieved data.
@param buf_size     Size of data structure, must be obtained via sizeof(RECORD-NAME).
@param align        Byte alignment calculated by the stub and
                    passed to the rpc side to align data
                    according to local machine. Must be zero
                    when called from user level.
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_STRUCT_SIZE_MISMATCH
*/
INT db_get_record(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, INT align)
{
   if (rpc_is_remote()) {
      align = ss_get_struct_align();
      return rpc_call(RPC_DB_GET_RECORD, hDB, hKey, data, buf_size, align);
   }
#ifdef LOCAL_ROUTINES
   {
      KEY key;
      INT convert_flags, status;
      INT total_size;
      void *pdata;
      char str[256];

      convert_flags = 0;

      if (!align)
         align = ss_get_struct_align();
      else
         /* only convert data if called remotely, as indicated by align != 0 */
      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
         convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

      /* check if key has subkeys */
      status = db_get_key(hDB, hKey, &key);
      if (status != DB_SUCCESS)
         return status;

      if (key.type != TID_KEY) {
         /* copy single key */
         if (key.item_size * key.num_values != *buf_size) {
            db_get_path(hDB, hKey, str, sizeof(str));
            cm_msg(MERROR, "db_get_record", 
                   "struct size mismatch for \"%s\" (expected size: %d, size in ODB: %d", str, *buf_size, key.item_size * key.num_values);
            return DB_STRUCT_SIZE_MISMATCH;
         }

         db_get_data(hDB, hKey, data, buf_size, key.type);

         if (convert_flags) {
            if (key.num_values > 1)
               rpc_convert_data(data, key.type,
                                RPC_OUTGOING | RPC_FIXARRAY, key.item_size * key.num_values, convert_flags);
            else
               rpc_convert_single(data, key.type, RPC_OUTGOING, convert_flags);
         }

         return DB_SUCCESS;
      }

      /* check record size */
      db_get_record_size(hDB, hKey, 0, &total_size);
      if (total_size != *buf_size) {
         db_get_path(hDB, hKey, str, sizeof(str));
         cm_msg(MERROR, "db_get_record",
                "struct size mismatch for \"%s\" (expected size: %d, size in ODB: %d)", str, *buf_size, total_size);
         return DB_STRUCT_SIZE_MISMATCH;
      }

      /* get subkey data */
      pdata = data;
      total_size = 0;

      db_lock_database(hDB);
      db_recurse_record_tree(hDB, hKey, &pdata, &total_size, align, NULL, FALSE, convert_flags);
      db_unlock_database(hDB);

   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Copy a set of keys from local memory to the database.

An ODB sub-tree can be mapped to a C structure automatically via a
hot-link using the function db_open_record() or manually with this function.
Problems might occur if the ODB sub-tree contains values which don't match the
C structure. Although the structure size is checked against the sub-tree size, no
checking can be done if the type and order of the values in the structure are the
same than those in the ODB sub-tree. Therefore it is recommended to use the
function db_create_record() before using this function.
\code
...
  memset(&lazyst,0,size);
  if (db_find_key(hDB, pLch->hKey, "Statistics",&hKeyst) == DB_SUCCESS)
    status = db_set_record(hDB, hKeyst, &lazyst, size, 0);
  else
    cm_msg(MERROR,"task","record %s/statistics not found", pLch->name)
...
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@param data Pointer where data is stored.
@param buf_size Size of data structure, must be obtained via sizeof(RECORD-NAME).
@param align  Byte alignment calculated by the stub and
              passed to the rpc side to align data
              according to local machine. Must be zero
              when called from user level.
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_TYPE_MISMATCH, DB_STRUCT_SIZE_MISMATCH
*/
INT db_set_record(HNDLE hDB, HNDLE hKey, void *data, INT buf_size, INT align)
{
   if (rpc_is_remote()) {
      align = ss_get_struct_align();
      return rpc_call(RPC_DB_SET_RECORD, hDB, hKey, data, buf_size, align);
   }
#ifdef LOCAL_ROUTINES
   {
      KEY key;
      INT convert_flags;
      INT total_size;
      void *pdata;

      convert_flags = 0;

      if (!align)
         align = ss_get_struct_align();
      else
         /* only convert data if called remotely, as indicated by align != 0 */
      if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
         convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

      /* check if key has subkeys */
      db_get_key(hDB, hKey, &key);
      if (key.type != TID_KEY) {
         /* copy single key */
         if (key.item_size * key.num_values != buf_size) {
            cm_msg(MERROR, "db_set_record", "struct size mismatch for \"%s\"", key.name);
            return DB_STRUCT_SIZE_MISMATCH;
         }

         if (convert_flags) {
            if (key.num_values > 1)
               rpc_convert_data(data, key.type, RPC_FIXARRAY, key.item_size * key.num_values, convert_flags);
            else
               rpc_convert_single(data, key.type, 0, convert_flags);
         }

         db_set_data(hDB, hKey, data, key.total_size, key.num_values, key.type);
         return DB_SUCCESS;
      }

      /* check record size */
      db_get_record_size(hDB, hKey, 0, &total_size);
      if (total_size != buf_size) {
         cm_msg(MERROR, "db_set_record", "struct size mismatch for \"%s\"", key.name);
         return DB_STRUCT_SIZE_MISMATCH;
      }

      /* set subkey data */
      pdata = data;
      total_size = 0;

      db_lock_database(hDB);
      db_recurse_record_tree(hDB, hKey, &pdata, &total_size, align, NULL, TRUE, convert_flags);
      db_notify_clients(hDB, hKey, TRUE);
      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT db_add_open_record(HNDLE hDB, HNDLE hKey, WORD access_mode)
/********************************************************************\

  Routine: db_add_open_record

  Purpose: Server part of db_open_record. Internal use only.

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_ADD_OPEN_RECORD, hDB, hKey, access_mode);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      KEY *pkey;
      INT i;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_add_open_record", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      /* lock database */
      db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pclient = &pheader->client[_database[hDB - 1].client_index];

      /* check if key is already open */
      for (i = 0; i < pclient->max_index; i++)
         if (pclient->open_record[i].handle == hKey)
            break;

      if (i < pclient->max_index) {
         db_unlock_database(hDB);
         return DB_SUCCESS;
      }

      /* not found, search free entry */
      for (i = 0; i < pclient->max_index; i++)
         if (pclient->open_record[i].handle == 0)
            break;

      /* check if maximum number reached */
      if (i == MAX_OPEN_RECORDS) {
         db_unlock_database(hDB);
         return DB_NO_MEMORY;
      }

      if (i == pclient->max_index)
         pclient->max_index++;

      pclient->open_record[i].handle = hKey;
      pclient->open_record[i].access_mode = access_mode;

      /* increment notify_count */
      pkey = (KEY *) ((char *) pheader + hKey);

      /* check if hKey argument is correct */
      if (!db_validate_hkey(pheader, hKey)) {
         db_unlock_database(hDB);
         return DB_INVALID_HANDLE;
      }

      pkey->notify_count++;

      pclient->num_open_records++;

      /* set exclusive bit if requested */
      if (access_mode & MODE_WRITE)
         db_set_mode(hDB, hKey, (WORD) (pkey->access_mode | MODE_EXCLUSIVE), 2);

      db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
INT db_remove_open_record(HNDLE hDB, HNDLE hKey, BOOL lock)
/********************************************************************\

  Routine: db_remove_open_record

  Purpose: Gets called by db_close_record. Internal use only.

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_DB_REMOVE_OPEN_RECORD, hDB, hKey, lock);

#ifdef LOCAL_ROUTINES
   {
      DATABASE_HEADER *pheader;
      DATABASE_CLIENT *pclient;
      KEY *pkey;
      INT i, idx;

      if (hDB > _database_entries || hDB <= 0) {
         cm_msg(MERROR, "db_remove_open_record", "invalid database handle");
         return DB_INVALID_HANDLE;
      }

      if (lock)
         db_lock_database(hDB);

      pheader = _database[hDB - 1].database_header;
      pclient = &pheader->client[_database[hDB - 1].client_index];

      /* search key */
      for (idx = 0; idx < pclient->max_index; idx++)
         if (pclient->open_record[idx].handle == hKey)
            break;

      if (idx == pclient->max_index) {
         if (lock)
            db_unlock_database(hDB);

         return DB_INVALID_HANDLE;
      }

      /* decrement notify_count */
      pkey = (KEY *) ((char *) pheader + hKey);

      if (pkey->notify_count > 0)
         pkey->notify_count--;

      pclient->num_open_records--;

      /* remove exclusive flag */
      if (pclient->open_record[idx].access_mode & MODE_WRITE)
         db_set_mode(hDB, hKey, (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), 2);

      memset(&pclient->open_record[idx], 0, sizeof(OPEN_RECORD));

      /* calculate new max_index entry */
      for (i = pclient->max_index - 1; i >= 0; i--)
         if (pclient->open_record[i].handle != 0)
            break;
      pclient->max_index = i + 1;

      if (lock)
         db_unlock_database(hDB);
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef LOCAL_ROUTINES

INT db_notify_clients(HNDLE hDB, HNDLE hKey, BOOL bWalk)
/********************************************************************\

  Routine: db_notify_clients

  Purpose: Gets called by db_set_xxx functions. Internal use only.

\********************************************************************/
{
   DATABASE_HEADER *pheader;
   DATABASE_CLIENT *pclient;
   KEY *pkey;
   KEYLIST *pkeylist;
   INT i, j;
   char str[80];

   if (hDB > _database_entries || hDB <= 0) {
      cm_msg(MERROR, "db_notify_clients", "invalid database handle");
      return DB_INVALID_HANDLE;
   }

   pheader = _database[hDB - 1].database_header;

   /* check if key or parent has notify_flag set */
   pkey = (KEY *) ((char *) pheader + hKey);

   do {

      /* check which client has record open */
      if (pkey->notify_count)
         for (i = 0; i < pheader->max_client_index; i++) {
            pclient = &pheader->client[i];
            for (j = 0; j < pclient->max_index; j++)
               if (pclient->open_record[j].handle == hKey) {
                  /* send notification to remote process */
                  sprintf(str, "O %d %d", hDB, hKey);
                  ss_resume(pclient->port, str);
               }
         }

      if (pkey->parent_keylist == 0 || !bWalk)
         return DB_SUCCESS;

      pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
      pkey = (KEY *) ((char *) pheader + pkeylist->parent);
      hKey = (POINTER_T) pkey - (POINTER_T) pheader;
   } while (TRUE);

}

#endif                          /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/
void merge_records(HNDLE hDB, HNDLE hKey, KEY * pkey, INT level, void *info)
{
   char full_name[256], buffer[10000];
   INT status, size;
   void *p;
   HNDLE hKeyInit;
   KEY initkey, key;

   /* avoid compiler warnings */
   status = level;
   p = info;
   p = pkey;

   /* compose name of init key */
   db_get_path(hDB, hKey, full_name, sizeof(full_name));
   *strchr(full_name, 'O') = 'I';

   /* if key in init record found, copy original data to init data */
   status = db_find_key(hDB, 0, full_name, &hKeyInit);
   if (status == DB_SUCCESS) {
      status = db_get_key(hDB, hKeyInit, &initkey);
      assert(status == DB_SUCCESS);
      status = db_get_key(hDB, hKey, &key);
      assert(status == DB_SUCCESS);

      if (initkey.type != TID_KEY && initkey.type == key.type) {
         /* copy data from original key to new key */
         size = sizeof(buffer);
         status = db_get_data(hDB, hKey, buffer, &size, initkey.type);
         assert(status == DB_SUCCESS);
         status = db_set_data(hDB, hKeyInit, buffer, initkey.total_size, initkey.num_values, initkey.type);
         assert(status == DB_SUCCESS);
      }
   } else if (status == DB_NO_KEY) {
      /* do nothing */
   } else if (status == DB_INVALID_LINK) {
      status = db_find_link(hDB, 0, full_name, &hKeyInit);
      if (status == DB_SUCCESS) {
         size = sizeof(full_name);
         db_get_data(hDB, hKeyInit, full_name, &size, TID_LINK);
      }
      cm_msg(MERROR, "merge_records", "Invalid link \"%s\"", full_name);
   } else {
      cm_msg(MERROR, "merge_records",
             "aborting on unexpected failure of db_find_key(%s), status %d", full_name, status);
      abort();
   }
}

static int open_count;

void check_open_keys(HNDLE hDB, HNDLE hKey, KEY * pkey, INT level, void *info)
{
   int i;
   void *p;

   /* avoid compiler warnings */
   i = hDB;
   i = hKey;
   i = level;
   p = info;

   if (pkey->notify_count)
      open_count++;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Create a record. If a part of the record exists alreay,
merge it with the init_str (use values from the init_str only when they are
not in the existing record).

This functions creates a ODB sub-tree according to an ASCII
representation of that tree. See db_copy() for a description. It can be used to
create a sub-tree which exactly matches a C structure. The sub-tree can then
later mapped to the C structure ("hot-link") via the function db_open_record().

If a sub-tree exists already which exactly matches the ASCII representation, it is
not modified. If part of the tree exists, it is merged with the ASCII representation
where the ODB values have priority, only values not present in the ODB are
created with the default values of the ASCII representation. It is therefore
recommended that before creating an ODB hot-link the function
db_create_record() is called to insure that the ODB tree and the C structure
contain exactly the same values in the same order.

Following example creates a record under /Equipment/Trigger/Settings,
opens a hot-link between that record and a local C structure
trigger_settings and registers a callback function trigger_update()
which gets called each time the record is changed.
\code
struct {
  INT level1;
  INT level2;
} trigger_settings;
char *trigger_settings_str =
"[Settings]\n\
level1 = INT : 0\n\
level2 = INT : 0";
void trigger_update(INT hDB, INT hkey, void *info)
{
  printf("New levels: %d %d\n",
    trigger_settings.level1,
    trigger_settings.level2);
}
main()
{
  HNDLE hDB, hkey;
  char[128] info;
  ...
  cm_get_experiment_database(&hDB, NULL);
  db_create_record(hDB, 0, "/Equipment/Trigger", trigger_settings_str);
  db_find_key(hDB, 0,"/Equipment/Trigger/Settings", &hkey);
  db_open_record(hDB, hkey, &trigger_settings,
    sizeof(trigger_settings), MODE_READ, trigger_update, info);
  ...
}
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param orig_key_name     Name of key to search, can contain directories.
@param init_str     Initialization string in the format of the db_copy/db_save functions.
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_FULL, DB_NO_ACCESS, DB_OPEN_RECORD
*/
INT db_create_record(HNDLE hDB, HNDLE hKey, const char *orig_key_name, const char *init_str)
{
   char str[256], key_name[256], *buffer;
   INT status, size, i, buffer_size;
   HNDLE hKeyTmp, hKeyTmpO, hKeyOrig, hSubkey;

   if (rpc_is_remote())
      return rpc_call(RPC_DB_CREATE_RECORD, hDB, hKey, orig_key_name, init_str);

   /* make this function atomic */
   db_lock_database(hDB);

   /* strip trailing '/' */
   strlcpy(key_name, orig_key_name, sizeof(key_name));
   if (strlen(key_name) > 1 && key_name[strlen(key_name) - 1] == '/')
      key_name[strlen(key_name) - 1] = 0;

   /* merge temporay record and original record */
   status = db_find_key(hDB, hKey, key_name, &hKeyOrig);
   if (status == DB_SUCCESS) {
      assert(hKeyOrig != 0);
      /* check if key or subkey is opened */
      open_count = 0;
      db_scan_tree_link(hDB, hKeyOrig, 0, check_open_keys, NULL);
      if (open_count) {
         db_unlock_database(hDB);
         return DB_OPEN_RECORD;
      }

      /* create temporary records */
      sprintf(str, "/System/Tmp/%1dI", ss_gettid());
      db_find_key(hDB, 0, str, &hKeyTmp);
      if (hKeyTmp)
         db_delete_key(hDB, hKeyTmp, FALSE);
      db_create_key(hDB, 0, str, TID_KEY);
      status = db_find_key(hDB, 0, str, &hKeyTmp);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      sprintf(str, "/System/Tmp/%1dO", ss_gettid());
      db_find_key(hDB, 0, str, &hKeyTmpO);
      if (hKeyTmpO)
         db_delete_key(hDB, hKeyTmpO, FALSE);
      db_create_key(hDB, 0, str, TID_KEY);
      status = db_find_key(hDB, 0, str, &hKeyTmpO);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      status = db_paste(hDB, hKeyTmp, init_str);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      buffer_size = 10000;
      buffer = (char *) malloc(buffer_size);
      do {
         size = buffer_size;
         status = db_copy(hDB, hKeyOrig, buffer, &size, "");
         if (status == DB_TRUNCATED) {
            buffer_size += 10000;
            buffer = (char *) realloc(buffer, buffer_size);
            continue;
         }
         if (status != DB_SUCCESS) {
            db_unlock_database(hDB);
            return status;
         }

      } while (status != DB_SUCCESS);

      status = db_paste(hDB, hKeyTmpO, buffer);
      if (status != DB_SUCCESS) {
         free(buffer);
         db_unlock_database(hDB);
         return status;
      }

      /* merge temporay record and original record */
      db_scan_tree_link(hDB, hKeyTmpO, 0, merge_records, NULL);

      /* delete original record */
      for (i = 0;; i++) {
         db_enum_link(hDB, hKeyOrig, 0, &hSubkey);
         if (!hSubkey)
            break;

         status = db_delete_key(hDB, hSubkey, FALSE);
         if (status != DB_SUCCESS) {
            free(buffer);
            db_unlock_database(hDB);
            return status;
         }
      }

      /* copy merged record to original record */
      do {
         size = buffer_size;
         status = db_copy(hDB, hKeyTmp, buffer, &size, "");
         if (status == DB_TRUNCATED) {
            buffer_size += 10000;
            buffer = (char *) realloc(buffer, buffer_size);
            continue;
         }
         if (status != DB_SUCCESS) {
            free(buffer);
            db_unlock_database(hDB);
            return status;
         }

      } while (status != DB_SUCCESS);

      status = db_paste(hDB, hKeyOrig, buffer);
      if (status != DB_SUCCESS) {
         free(buffer);
         db_unlock_database(hDB);
         return status;
      }

      /* delete temporary records */
      db_delete_key(hDB, hKeyTmp, FALSE);
      db_delete_key(hDB, hKeyTmpO, FALSE);

      free(buffer);
      buffer = NULL;
   } else if (status == DB_NO_KEY) {
      /* create fresh record */
      db_create_key(hDB, hKey, key_name, TID_KEY);
      status = db_find_key(hDB, hKey, key_name, &hKeyTmp);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }

      status = db_paste(hDB, hKeyTmp, init_str);
      if (status != DB_SUCCESS) {
         db_unlock_database(hDB);
         return status;
      }
   } else {
      cm_msg(MERROR, "db_create_record",
             "aborting on unexpected failure of db_find_key(%s), status %d", key_name, status);
      abort();
   }

   db_unlock_database(hDB);

   return DB_SUCCESS;
}

/********************************************************************/
/**
This function ensures that a certain ODB subtree matches
a given C structure, by comparing the init_str with the
current ODB structure. If the record does not exist at all,
it is created with the default values in init_str. If it
does exist but does not match the variables in init_str,
the function returns an error if correct=FALSE or calls
db_create_record() if correct=TRUE.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey     Handle for key where search starts, zero for root.
@param keyname  Name of key to search, can contain directories.
@param rec_str  ASCII representation of ODB record in the format
@param correct  If TRUE, correct ODB record if necessary
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_KEY, DB_STRUCT_MISMATCH
*/
INT db_check_record(HNDLE hDB, HNDLE hKey, const char *keyname, const char *rec_str, BOOL correct)
{
   char line[MAX_STRING_LENGTH];
   char title[MAX_STRING_LENGTH];
   char key_name[MAX_STRING_LENGTH];
   char info_str[MAX_STRING_LENGTH + 50];
   char *pc;
   const char *pold, *rec_str_orig;
   DWORD tid;
   INT i, j, n_data, string_length, status;
   HNDLE hKeyRoot, hKeyTest;
   KEY key;
   BOOL multi_line;

   if (rpc_is_remote())
      return rpc_call(RPC_DB_CHECK_RECORD, hDB, hKey, keyname, rec_str, correct);

   /* check if record exists */
   status = db_find_key(hDB, hKey, keyname, &hKeyRoot);

   /* create record if not */
   if (status == DB_NO_KEY) {
      if (correct)
         return db_create_record(hDB, hKey, keyname, rec_str);
      return DB_NO_KEY;
   }

   assert(hKeyRoot);

   title[0] = 0;
   multi_line = FALSE;
   rec_str_orig = rec_str;

   db_get_key(hDB, hKeyRoot, &key);
   if (key.type == TID_KEY)
      /* get next subkey which is not of type TID_KEY */
      db_get_next_link(hDB, hKeyRoot, &hKeyTest);
   else
      /* test root key itself */
      hKeyTest = hKeyRoot;

   if (hKeyTest == 0 && *rec_str != 0) {
      if (correct)
         return db_create_record(hDB, hKey, keyname, rec_str_orig);

      return DB_STRUCT_MISMATCH;
   }

   do {
      if (*rec_str == 0)
         break;

      for (i = 0; *rec_str != '\n' && *rec_str && i < MAX_STRING_LENGTH; i++)
         line[i] = *rec_str++;

      if (i == MAX_STRING_LENGTH) {
         cm_msg(MERROR, "db_check_record", "line too long");
         return DB_TRUNCATED;
      }

      line[i] = 0;
      if (*rec_str == '\n')
         rec_str++;

      /* check if it is a section title */
      if (line[0] == '[') {
         /* extract title and append '/' */
         strcpy(title, line + 1);
         if (strchr(title, ']'))
            *strchr(title, ']') = 0;
         if (title[0] && title[strlen(title) - 1] != '/')
            strcat(title, "/");
      } else {
         /* valid data line if it includes '=' and no ';' */
         if (strchr(line, '=') && line[0] != ';') {
            /* copy type info and data */
            pc = strchr(line, '=') + 1;
            while (*pc == ' ')
               pc++;
            strcpy(info_str, pc);

            /* extract key name */
            *strchr(line, '=') = 0;

            pc = &line[strlen(line) - 1];
            while (*pc == ' ')
               *pc-- = 0;

            strlcpy(key_name, line, sizeof(key_name));

            /* evaluate type info */
            strcpy(line, info_str);
            if (strchr(line, ' '))
               *strchr(line, ' ') = 0;

            n_data = 1;
            if (strchr(line, '[')) {
               n_data = atol(strchr(line, '[') + 1);
               *strchr(line, '[') = 0;
            }

            for (tid = 0; tid < TID_LAST; tid++)
               if (strcmp(tid_name[tid], line) == 0)
                  break;

            string_length = 0;

            if (tid == TID_LAST)
               cm_msg(MERROR, "db_check_record", "found unknown data type \"%s\" in ODB file", line);
            else {
               /* skip type info */
               pc = info_str;
               while (*pc != ' ' && *pc)
                  pc++;
               while ((*pc == ' ' || *pc == ':') && *pc)
                  pc++;

               if (n_data > 1) {
                  info_str[0] = 0;
                  if (!*rec_str)
                     break;

                  for (j = 0; *rec_str != '\n' && *rec_str; j++)
                     info_str[j] = *rec_str++;
                  info_str[j] = 0;
                  if (*rec_str == '\n')
                     rec_str++;
               }

               for (i = 0; i < n_data; i++) {
                  /* strip trailing \n */
                  pc = &info_str[strlen(info_str) - 1];
                  while (*pc == '\n' || *pc == '\r')
                     *pc-- = 0;

                  if (tid == TID_STRING || tid == TID_LINK) {
                     if (!string_length) {
                        if (info_str[1] == '=')
                           string_length = -1;
                        else {
                           pc = strchr(info_str, '[');
                           if (pc != NULL)
                              string_length = atoi(pc + 1);
                           else
                              string_length = -1;
                        }
                        if (string_length > MAX_STRING_LENGTH) {
                           string_length = MAX_STRING_LENGTH;
                           cm_msg(MERROR, "db_check_record", "found string exceeding MAX_STRING_LENGTH");
                        }
                     }

                     if (string_length == -1) {
                        /* multi-line string */
                        if (strstr(rec_str, "\n====#$@$#====\n") != NULL) {
                           string_length = (POINTER_T) strstr(rec_str, "\n====#$@$#====\n") - (POINTER_T) rec_str + 1;

                           rec_str = strstr(rec_str, "\n====#$@$#====\n") + strlen("\n====#$@$#====\n");
                        } else
                           cm_msg(MERROR, "db_check_record", "found multi-line string without termination sequence");
                     } else {
                        if (strchr(info_str, ']'))
                           pc = strchr(info_str, ']') + 1;
                        else
                           pc = info_str + 2;
                        while (*pc && *pc != ' ')
                           pc++;
                        while (*pc && *pc == ' ')
                           pc++;

                        /* limit string size */
                        *(pc + string_length - 1) = 0;
                     }
                  } else {
                     pc = info_str;

                     if (n_data > 1 && info_str[0] == '[') {
                        pc = strchr(info_str, ']') + 1;
                        while (*pc && *pc == ' ')
                           pc++;
                     }
                  }

                  if (i < n_data - 1) {
                     info_str[0] = 0;
                     if (!*rec_str)
                        break;

                     pold = rec_str;

                     for (j = 0; *rec_str != '\n' && *rec_str; j++)
                        info_str[j] = *rec_str++;
                     info_str[j] = 0;
                     if (*rec_str == '\n')
                        rec_str++;

                     /* test if valid data */
                     if (tid != TID_STRING && tid != TID_LINK) {
                        if (info_str[0] == 0 || (strchr(info_str, '=')
                                                 && strchr(info_str, ':')))
                           rec_str = pold;
                     }
                  }
               }

               /* if rec_str contains key, but not ODB, return mismatch */
               if (!hKeyTest) {
                  if (correct)
                     return db_create_record(hDB, hKey, keyname, rec_str_orig);

                  return DB_STRUCT_MISMATCH;
               }

               status = db_get_key(hDB, hKeyTest, &key);
               assert(status == DB_SUCCESS);

               /* check rec_str vs. ODB key */
               if (!equal_ustring(key.name, key_name) || key.type != tid || key.num_values != n_data) {
                  if (correct)
                     return db_create_record(hDB, hKey, keyname, rec_str_orig);

                  return DB_STRUCT_MISMATCH;
               }

               /* get next key in ODB */
               db_get_next_link(hDB, hKeyTest, &hKeyTest);
            }
         }
      }
   } while (TRUE);

   return DB_SUCCESS;
}

/********************************************************************/
/**
Open a record. Create a local copy and maintain an automatic update.

This function opens a hot-link between an ODB sub-tree and a local
structure. The sub-tree is copied to the structure automatically every time it is
modified by someone else. Additionally, a callback function can be declared
which is called after the structure has been updated. The callback function
receives the database handle and the key handle as parameters.

Problems might occur if the ODB sub-tree contains values which don't match the
C structure. Although the structure size is checked against the sub-tree size, no
checking can be done if the type and order of the values in the structure are the
same than those in the ODB sub-tree. Therefore it is recommended to use the
function db_create_record() before db_open_record() is used which
ensures that both are equivalent.

The access mode might either be MODE_READ or MODE_WRITE. In read mode,
the ODB sub-tree is automatically copied to the local structure when modified by
other clients. In write mode, the local structure is copied to the ODB sub-tree if it
has been modified locally. This update has to be manually scheduled by calling
db_send_changed_records() periodically in the main loop. The system
keeps a copy of the local structure to determine if its contents has been changed.

If MODE_ALLOC is or'ed with the access mode, the memory for the structure is
allocated internally. The structure pointer must contain a pointer to a pointer to
the structure. The internal memory is released when db_close_record() is
called.
- To open a record in write mode.
\code
struct {
  INT level1;
  INT level2;
} trigger_settings;
char *trigger_settings_str =
"[Settings]\n\
level1 = INT : 0\n\
level2 = INT : 0";
main()
{
  HNDLE hDB, hkey, i=0;
  ...
  cm_get_experiment_database(&hDB, NULL);
  db_create_record(hDB, 0, "/Equipment/Trigger", trigger_settings_str);
  db_find_key(hDB, 0,"/Equipment/Trigger/Settings", &hkey);
  db_open_record(hDB, hkey, &trigger_settings, sizeof(trigger_settings)
                  , MODE_WRITE, NULL);
  do
  {
    trigger_settings.level1 = i++;
    db_send_changed_records()
    status = cm_yield(1000);
  } while (status != RPC_SHUTDOWN && status != SS_ABORT);
  ...
}
\endcode
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param ptr          If access_mode includes MODE_ALLOC:
                    Address of pointer which points to the record data after 
                    the call if access_mode includes not MODE_ALLOC:
                    Address of record if ptr==NULL, only the dispatcher is called.
@param rec_size     Record size in bytes                    
@param access_mode Mode for opening record, either MODE_READ or
                    MODE_WRITE. May be or'ed with MODE_ALLOC to
                    let db_open_record allocate the memory for the record.
@param (*dispatcher)   Function which gets called when record is updated.The
                    argument list composed of: HNDLE hDB, HNDLE hKey, void *info
@param info Additional info passed to the dispatcher.
@return DB_SUCCESS, DB_INVALID_HANDLE, DB_NO_MEMORY, DB_NO_ACCESS, DB_STRUCT_SIZE_MISMATCH
*/
INT db_open_record(HNDLE hDB, HNDLE hKey, void *ptr, INT rec_size,
                   WORD access_mode, void (*dispatcher) (INT, INT, void *), void *info)
{
   INT idx, status, size;
   KEY key;
   void *data;
   char str[256];

   /* allocate new space for the local record list */
   if (_record_list_entries == 0) {
      _record_list = (RECORD_LIST *) malloc(sizeof(RECORD_LIST));
      memset(_record_list, 0, sizeof(RECORD_LIST));
      if (_record_list == NULL) {
         cm_msg(MERROR, "db_open_record", "not enough memory");
         return DB_NO_MEMORY;
      }

      _record_list_entries = 1;
      idx = 0;
   } else {
      /* check for a deleted entry */
      for (idx = 0; idx < _record_list_entries; idx++)
         if (!_record_list[idx].handle)
            break;

      /* if not found, create new one */
      if (idx == _record_list_entries) {
         _record_list = (RECORD_LIST *) realloc(_record_list, sizeof(RECORD_LIST) * (_record_list_entries + 1));
         if (_record_list == NULL) {
            cm_msg(MERROR, "db_open_record", "not enough memory");
            return DB_NO_MEMORY;
         }

         memset(&_record_list[_record_list_entries], 0, sizeof(RECORD_LIST));

         _record_list_entries++;
      }
   }

   db_get_key(hDB, hKey, &key);

   /* check record size */
   status = db_get_record_size(hDB, hKey, 0, &size);
   if (status != DB_SUCCESS) {
      _record_list_entries--;
      cm_msg(MERROR, "db_open_record", "cannot get record size");
      return DB_NO_MEMORY;
   }
   if (size != rec_size && ptr != NULL) {
      _record_list_entries--;
      db_get_path(hDB, hKey, str, sizeof(str));
      cm_msg(MERROR, "db_open_record", "struct size mismatch for \"%s\" (%d instead of %d)", str, rec_size, size);
      return DB_STRUCT_SIZE_MISMATCH;
   }

   /* check for read access */
   if (((key.access_mode & MODE_EXCLUSIVE) && (access_mode & MODE_WRITE))
       || (!(key.access_mode & MODE_WRITE) && (access_mode & MODE_WRITE))
       || (!(key.access_mode & MODE_READ) && (access_mode & MODE_READ))) {
      _record_list_entries--;
      return DB_NO_ACCESS;
   }

   if (access_mode & MODE_ALLOC) {
      data = malloc(size);
      memset(data, 0, size);

      if (data == NULL) {
         _record_list_entries--;
         cm_msg(MERROR, "db_open_record", "not enough memory");
         return DB_NO_MEMORY;
      }

      *((void **) ptr) = data;
   } else
      data = ptr;

   /* copy record to local memory */
   if (access_mode & MODE_READ && data != NULL) {
      status = db_get_record(hDB, hKey, data, &size, 0);
      if (status != DB_SUCCESS) {
         _record_list_entries--;
         cm_msg(MERROR, "db_open_record", "cannot get record");
         return DB_NO_MEMORY;
      }
   }

   /* copy local record to ODB */
   if (access_mode & MODE_WRITE) {
      /* only write to ODB if not in MODE_ALLOC */
      if ((access_mode & MODE_ALLOC) == 0) {
         status = db_set_record(hDB, hKey, data, size, 0);
         if (status != DB_SUCCESS) {
            _record_list_entries--;
            cm_msg(MERROR, "db_open_record", "cannot set record");
            return DB_NO_MEMORY;
         }
      }

      /* init a local copy of the record */
      _record_list[idx].copy = malloc(size);
      if (_record_list[idx].copy == NULL) {
         cm_msg(MERROR, "db_open_record", "not enough memory");
         return DB_NO_MEMORY;
      }

      memcpy(_record_list[idx].copy, data, size);
   }

   /* initialize record list */
   _record_list[idx].handle = hKey;
   _record_list[idx].hDB = hDB;
   _record_list[idx].access_mode = access_mode;
   _record_list[idx].data = data;
   _record_list[idx].buf_size = size;
   _record_list[idx].dispatcher = dispatcher;
   _record_list[idx].info = info;

   /* add record entry in database structure */
   return db_add_open_record(hDB, hKey, (WORD) (access_mode & ~MODE_ALLOC));
}

/********************************************************************/
/**
Close a record previously opend with db_open_record.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey Handle for key where search starts, zero for root.
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_close_record(HNDLE hDB, HNDLE hKey)
{
#ifdef LOCAL_ROUTINES
   {
      INT i;

      for (i = 0; i < _record_list_entries; i++)
         if (_record_list[i].handle == hKey && _record_list[i].hDB == hDB)
            break;

      if (i == _record_list_entries)
         return DB_INVALID_HANDLE;

      /* remove record entry from database structure */
      db_remove_open_record(hDB, hKey, TRUE);

      /* free local memory */
      if (_record_list[i].access_mode & MODE_ALLOC) {
         free(_record_list[i].data);
         _record_list[i].data = NULL;
      }

      if (_record_list[i].access_mode & MODE_WRITE) {
         free(_record_list[i].copy);
         _record_list[i].copy = NULL;
      }

      memset(&_record_list[i], 0, sizeof(RECORD_LIST));
   }
#endif                          /* LOCAL_ROUTINES */

   return DB_SUCCESS;
}

/********************************************************************/
/**
Release local memory for open records.
This routines is called by db_close_all_databases() and 
cm_disconnect_experiment()
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_close_all_records()
{
   INT i;

   for (i = 0; i < _record_list_entries; i++) {
      if (_record_list[i].handle) {
         if (_record_list[i].access_mode & MODE_WRITE) {
            free(_record_list[i].copy);
            _record_list[i].copy = NULL;
         }

         if (_record_list[i].access_mode & MODE_ALLOC) {
            free(_record_list[i].data);
            _record_list[i].data = NULL;
         }

         memset(&_record_list[i], 0, sizeof(RECORD_LIST));
      }
   }

   if (_record_list_entries > 0) {
      _record_list_entries = 0;
      free(_record_list);
      _record_list = NULL;
   }

   return DB_SUCCESS;
}

/********************************************************************/
/**
If called locally, update a record (hDB/hKey) and copy
its new contents to the local copy of it.

If called from a server, send a network notification to the client.
@param hDB          ODB handle obtained via cm_get_experiment_database().
@param hKey         Handle for key where search starts, zero for root.
@param s            optional server socket
@return DB_SUCCESS, DB_INVALID_HANDLE
*/
INT db_update_record(INT hDB, INT hKey, int s)
{
   INT i, size, convert_flags, status;
   char buffer[32];
   NET_COMMAND *nc;

   /* check if we are the server */
   if (s) {
      convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

      if (convert_flags & CF_ASCII) {
         sprintf(buffer, "MSG_ODB&%d&%d", hDB, hKey);
         send_tcp(s, buffer, strlen(buffer) + 1, 0);
      } else {
         nc = (NET_COMMAND *) buffer;

         nc->header.routine_id = MSG_ODB;
         nc->header.param_size = 2 * sizeof(INT);
         *((INT *) nc->param) = hDB;
         *((INT *) nc->param + 1) = hKey;

         if (convert_flags) {
            rpc_convert_single(&nc->header.routine_id, TID_DWORD, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&nc->header.param_size, TID_DWORD, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&nc->param[0], TID_DWORD, RPC_OUTGOING, convert_flags);
            rpc_convert_single(&nc->param[4], TID_DWORD, RPC_OUTGOING, convert_flags);
         }

         /* send the update notification to the client */
         send_tcp(s, buffer, sizeof(NET_COMMAND_HEADER) + 2 * sizeof(INT), 0);
      }

      return DB_SUCCESS;
   }

   status = DB_INVALID_HANDLE;

   /* check all entries for matching key */
   for (i = 0; i < _record_list_entries; i++)
      if (_record_list[i].handle == hKey) {
         status = DB_SUCCESS;

         /* get updated data if record not opened in write mode */
         if ((_record_list[i].access_mode & MODE_WRITE) == 0) {
            size = _record_list[i].buf_size;
            if (_record_list[i].data != NULL)
               db_get_record(hDB, hKey, _record_list[i].data, &size, 0);

            /* call dispatcher if requested */
            if (_record_list[i].dispatcher)
               _record_list[i].dispatcher(hDB, hKey, _record_list[i].info);
         }
      }

   return DB_SUCCESS;
}

/********************************************************************/
/**
Send all records to the ODB which were changed locally since the last
call to this function.

This function is valid if used in conjunction with db_open_record()
under the condition the record is open as MODE_WRITE access code.

- Full example dbchange.c which can be compiled as follow
\code
gcc -DOS_LINUX -I/midas/include -o dbchange dbchange.c
/midas/linux/lib/libmidas.a -lutil}

\begin{verbatim}
//------- dbchange.c
#include "midas.h"
#include "msystem.h"
\endcode

\code
//-------- BOF dbchange.c
typedef struct {
    INT    my_number;
    float   my_rate;
} MY_STATISTICS;

MY_STATISTICS myrec;

#define MY_STATISTICS(_name) char *_name[] = {\
"My Number = INT : 0",\
"My Rate = FLOAT : 0",\
"",\
NULL }

HNDLE hDB, hKey;

// Main
int main(unsigned int argc,char **argv)
{
  char      host_name[HOST_NAME_LENGTH];
  char      expt_name[HOST_NAME_LENGTH];
  INT       lastnumber, status, msg;
  BOOL      debug=FALSE;
  char      i, ch;
  DWORD     update_time, mainlast_time;
  MY_STATISTICS (my_stat);

  // set default
  host_name[0] = 0;
  expt_name[0] = 0;

  // get default
  cm_get_environment(host_name, sizeof(host_name), expt_name, sizeof(expt_name));

  // get parameters
  for (i=1 ; i<argc ; i++)
  {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-')
    {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (strncmp(argv[i],"-e",2) == 0)
        strcpy(expt_name, argv[++i]);
      else if (strncmp(argv[i],"-h",2)==0)
        strcpy(host_name, argv[++i]);
    }
    else
    {
   usage:
      printf("usage: dbchange [-h <Hostname>] [-e <Experiment>]\n");
      return 0;
    }
  }

  // connect to experiment
  status = cm_connect_experiment(host_name, expt_name, "dbchange", 0);
  if (status != CM_SUCCESS)
    return 1;

  // Connect to DB
  cm_get_experiment_database(&hDB, &hKey);

  // Create a default structure in ODB
  db_create_record(hDB, 0, "My statistics", strcomb(my_stat));

  // Retrieve key for that strucutre in ODB
  if (db_find_key(hDB, 0, "My statistics", &hKey) != DB_SUCCESS)
  {
    cm_msg(MERROR, "mychange", "cannot find My statistics");
    goto error;
  }

  // Hot link this structure in Write mode
  status = db_open_record(hDB, hKey, &myrec
                          , sizeof(MY_STATISTICS), MODE_WRITE, NULL, NULL);
  if (status != DB_SUCCESS)
  {
    cm_msg(MERROR, "mychange", "cannot open My statistics record");
    goto error;
  }

  // initialize ss_getchar()
  ss_getchar(0);

  // Main loop
  do
  {
    // Update local structure
    if ((ss_millitime() - update_time) > 100)
    {
      myrec.my_number += 1;
      if (myrec.my_number - lastnumber) {
        myrec.my_rate = 1000.f * (float) (myrec.my_number - lastnumber)
          / (float) (ss_millitime() - update_time);
      }
      update_time = ss_millitime();
      lastnumber = myrec.my_number;
    }

    // Publish local structure to ODB (db_send_changed_record)
    if ((ss_millitime() - mainlast_time) > 5000)
    {
      db_send_changed_records();                   // <------- Call
      mainlast_time = ss_millitime();
    }

    // Check for keyboard interaction
    ch = 0;
    while (ss_kbhit())
    {
      ch = ss_getchar(0);
      if (ch == -1)
        ch = getchar();
      if ((char) ch == '!')
        break;
    }
    msg = cm_yield(20);
  } while (msg != RPC_SHUTDOWN && msg != SS_ABORT && ch != '!');

 error:
  cm_disconnect_experiment();
  return 1;
}
//-------- EOF dbchange.c
\endcode
@return DB_SUCCESS
*/
INT db_send_changed_records()
{
   INT i;

   for (i = 0; i < _record_list_entries; i++)
      if (_record_list[i].access_mode & MODE_WRITE) {
         if (memcmp(_record_list[i].copy, _record_list[i].data, _record_list[i].buf_size) != 0) {
            /* switch to fast TCP mode temporarily */
            if (rpc_is_remote())
               rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);
            db_set_record(_record_list[i].hDB,
                          _record_list[i].handle, _record_list[i].data, _record_list[i].buf_size, 0);
            if (rpc_is_remote())
               rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);
            memcpy(_record_list[i].copy, _record_list[i].data, _record_list[i].buf_size);
         }
      }

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

/**dox***************************************************************/
                                                       /** @} *//* end of odbfunctionc */

/**dox***************************************************************/
                                                       /** @} *//* end of odbcode */
