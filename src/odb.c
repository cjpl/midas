/********************************************************************\

  Name:         ODB.C
  Created by:   Stefan Ritt

  Contents:     MIDAS online database functions

  $Log$
  Revision 1.25  1999/10/28 09:57:53  midas
  Added lock/unload of ODB for db_find_key/link

  Revision 1.24  1999/09/17 11:48:07  midas
  Alarm system half finished

  Revision 1.23  1999/09/13 11:07:58  midas
  Test for NULL strings in equal_ustring

  Revision 1.22  1999/08/27 08:14:47  midas
  Fixed bug with several strings in db_set_value

  Revision 1.21  1999/06/02 07:43:03  midas
  Fixed second bug with "//"

  Revision 1.20  1999/05/31 11:13:59  midas
  Fixed bug which caused ODB ASCII files to be saved with leading "//" instead "/"

  Revision 1.19  1999/05/06 15:28:18  midas
  Fixed bug in db_sprintf where '0' was not returned in data size for strings

  Revision 1.18  1999/05/05 12:02:34  midas
  Added and modified history functions, added db_set_num_values

  Revision 1.17  1999/05/03 10:41:35  midas
  Show open record function now scans all links, not keys

  Revision 1.16  1999/04/30 13:19:55  midas
  Changed inter-process communication (ss_resume, bm_notify_clients, etc)
  to strings so that server process can receive it's own watchdog produced
  messages (pass buffer name insteas buffer handle)

  Revision 1.15  1999/04/29 10:48:02  midas
  Implemented "/System/Client Notify" key

  Revision 1.14  1999/04/22 15:32:18  midas
  db_get_key_info returns the numbe of subkeys for TID_KEYs

  Revision 1.13  1999/04/15 09:57:01  midas
  - Added key name to db_get_key_info
  - Added db_notify_clients to db_set_record

  Revision 1.12  1999/04/13 12:20:45  midas
  Added db_get_data1 (for Java)

  Revision 1.11  1999/04/08 15:25:17  midas
  Added db_get_key_info and CF_ASCII client notification (for Java)

  Revision 1.10  1999/02/18 11:20:08  midas
  Added "level" parameter to db_scan_tree and db_scan_tree_link

  Revision 1.9  1999/02/18 07:10:14  midas
  - db_save now stores full path name in .odb file
  - db_load loads .odb entries to absolute ODB location if they start with "[/...]"

  Revision 1.8  1999/01/22 10:31:49  midas
  Fixes status return from ss_mutex_create in db_open_database

  Revision 1.7  1999/01/20 08:55:44  midas
  - Renames ss_xxx_mutex to ss_mutex_xxx
  - Added timout flag to ss_mutex_wait_for

  Revision 1.6  1999/01/19 12:42:08  midas
  Records can be open several times with different dispatchers

  Revision 1.5  1999/01/13 09:40:49  midas
  Added db_set_data_index2 function

  Revision 1.4  1998/10/29 14:53:17  midas
  BOOL values are displayed and read as 'y' and 'n'

  Revision 1.3  1998/10/12 12:19:03  midas
  Added Log tag in header

  Revision 1.2  1998/10/12 11:59:11  midas
  Added Log tag in header

\********************************************************************/

#include "midas.h"
#include "msystem.h"

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*                 db_xxx  -  Database Functions                      *
*                                                                    *
\********************************************************************/

/* Globals */

DATABASE           *_database;
INT                _database_entries = 0;

static RECORD_LIST *_record_list;
static INT         _record_list_entries = 0;

extern             char* tid_name[];

static INT         _odb_size = DEFAULT_ODB_SIZE;

/*------------------------------------------------------------------*/

/********************************************************************\
*                                                                    *
*            Shared Memory Allocation                                *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/

void *malloc_key(DATABASE_HEADER *pheader, INT size)
{
FREE_DESCRIP   *pfree, *pfound, *pprev;

  if (size == 0)
    return NULL;

  /* quadword alignment for alpha CPU */
  size = ALIGN(size);

  /* search for free block */
  pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

  while (pfree->size < size && pfree->next_free)
    {
    pprev = pfree;
    pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
    }

  /* return if not enough memory */
  if (pfree->size < size)
    return 0;

  pfound = pfree;

  /* if found block is first in list, correct pheader */
  if (pfree == (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key))
    {
    if (size < pfree->size)
      {
      /* free block is only used partially */
      pheader->first_free_key += size;
      pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

      pfree->size = pfound->size - size;
      pfree->next_free = pfound->next_free;
      }
    else
      {
      /* free block is used totally */
      pheader->first_free_key = pfree->next_free;
      }
    } 
  else
    {
    /* check if free block is used totally */
    if (pfound->size - size < sizeof(FREE_DESCRIP))
      {
      /* skip block totally */
      pprev->next_free = pfound->next_free;
      }
    else
      {
      /* decrease free block */
      pfree = (FREE_DESCRIP *) ((char *) pfound + size);

      pfree->size = pfound->size - size;
      pfree->next_free = pfound->next_free;

      pprev->next_free = (PTYPE) pfree - (PTYPE) pheader;
      }
    }


  memset(pfound, 0, size);

  return pfound;
}

/*------------------------------------------------------------------*/

void free_key(DATABASE_HEADER *pheader, void *address, INT size)
{
FREE_DESCRIP   *pfree, *pprev, *pnext;

  if (size == 0)
    return;

  /* quadword alignment for alpha CPU */
  size = ALIGN(size);

  pfree = address;
  pprev = NULL;

  /* clear current block */
  memset(address, 0, size);
  
  /* if key comes before first free block, adjust pheader */
  if ((PTYPE) address - (PTYPE) pheader < pheader->first_free_key)
    {
    pfree->size = size;
    pfree->next_free = pheader->first_free_key;
    pheader->first_free_key = (PTYPE) address - (PTYPE) pheader;
    }
  else
    {
    /* find last free block before current block */
    pprev = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

    while (pprev->next_free < (PTYPE) address - (PTYPE) pheader)
      pprev = (FREE_DESCRIP *) ((char *) pheader + pprev->next_free);

    pfree->size = size;
    pfree->next_free = pprev->next_free;

    pprev->next_free = (PTYPE) pfree - (PTYPE) pheader;
    }

  /* try to melt adjacent free blocks after current block */
  pnext = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
  if ((PTYPE) pnext == (PTYPE) pfree + pfree->size)
    {
    pfree->size +=     pnext->size;
    pfree->next_free = pnext->next_free;

    memset(pnext, 0, pnext->size);
    }

  /* try to melt adjacent free blocks before current block */
  if (pprev && pprev->next_free == 
                 (PTYPE) pprev - (PTYPE) pheader + pprev->size)
    {
    pprev->size += pfree->size;
    pprev->next_free = pfree->next_free;

    memset(pfree, 0, pfree->size);
    }
}

/*------------------------------------------------------------------*/

void *malloc_data(DATABASE_HEADER *pheader, INT size)
{
FREE_DESCRIP   *pfree, *pfound, *pprev;

  if (size == 0)
    return NULL;

  /* quadword alignment for alpha CPU */
  size = ALIGN(size);

  /* search for free block */
  pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

  while (pfree->size < size && pfree->next_free)
    {
    pprev = pfree;
    pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
    }

  /* return if not enough memory */
  if (pfree->size < size)
    return 0;

  pfound = pfree;

  /* if found block is first in list, correct pheader */
  if (pfree == (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data))
    {
    if (size < pfree->size)
      {
      /* free block is only used partially */
      pheader->first_free_data += size;
      pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

      pfree->size = pfound->size - size;
      pfree->next_free = pfound->next_free;
      }
    else
      {
      /* free block is used totally */
      pheader->first_free_data = pfree->next_free;
      }
    } 
  else
    {
    /* check if free block is used totally */
    if (pfound->size - size < sizeof(FREE_DESCRIP))
      {
      /* skip block totally */
      pprev->next_free = pfound->next_free;
      }
    else
      {
      /* decrease free block */
      pfree = (FREE_DESCRIP *) ((char *) pfound + size);

      pfree->size = pfound->size - size;
      pfree->next_free = pfound->next_free;

      pprev->next_free = (PTYPE) pfree - (PTYPE) pheader;
      }
    }

  /* zero memeory */
  memset(pfound, 0, size);

  return pfound;
}

/*------------------------------------------------------------------*/

void free_data(DATABASE_HEADER *pheader, void *address, INT size)
{
FREE_DESCRIP   *pfree, *pprev, *pnext;

  if (size == 0)
    return;

  /* quadword alignment for alpha CPU */
  size = ALIGN(size);

  pfree = (FREE_DESCRIP *) address;
  pprev = NULL;

  /* clear current block */
  memset(address, 0, size);
  
  /* if data comes before first free block, adjust pheader */
  if ((PTYPE) address - (PTYPE) pheader < pheader->first_free_data)
    {
    pfree->size = size;
    pfree->next_free = pheader->first_free_data;
    pheader->first_free_data = (PTYPE) address - (PTYPE) pheader;
    }
  else
    {
    /* find last free block before current block */
    pprev = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

    while (pprev->next_free < (PTYPE) address - (PTYPE) pheader)
      pprev = (FREE_DESCRIP *) ((char *) pheader + pprev->next_free);

    pfree->size = size;
    pfree->next_free = pprev->next_free;

    pprev->next_free = (PTYPE) pfree - (PTYPE) pheader;
    }

  /* try to melt adjacent free blocks after current block */
  pnext = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
  if ((PTYPE) pnext == (PTYPE) pfree + pfree->size)
    {
    pfree->size +=     pnext->size;
    pfree->next_free = pnext->next_free;

    memset(pnext, 0, pnext->size);
    }

  /* try to melt adjacent free blocks before current block */
  if (pprev && pprev->next_free == 
                 (PTYPE) pprev - (PTYPE) pheader + pprev->size)
    {
    pprev->size += pfree->size;
    pprev->next_free = pfree->next_free;

    memset(pfree, 0, pfree->size);
    }
}

/*------------------------------------------------------------------*/

void *realloc_data(DATABASE_HEADER *pheader, void *address, 
                   INT old_size, INT new_size)
{
void *tmp, *new;

  if (old_size)
    {
    tmp = malloc(old_size);
    if (tmp == NULL)
      return NULL;
  
    memcpy(tmp, address, old_size);
    free_data(pheader, address, old_size);
    }
  
  new = malloc_data(pheader, new_size);

  if (new && old_size)
    memcpy(new, tmp, old_size < new_size ? old_size : new_size);

  if (old_size)
    free(tmp);

  return new;
}

/*------------------------------------------------------------------*/

char *strcomb(char **list)
/* convert list of strings into single string to be used by db_paste() */
{
INT i, j;
static char *str = NULL;

  /* counter number of chars */
  for (i=0,j=0 ; list[i] ; i++)
    j += strlen(list[i]) + 1;
  j += 1;

  if (str == NULL)
    str = malloc(j);
  else
    str = realloc(str, j);

  str[0] = 0;
  for (i=0 ; list[i] ; i++)
    {
    strcat(str, list[i]);
    strcat(str, "\n");
    }

  return str;
}

/*------------------------------------------------------------------*/

INT db_show_mem(HNDLE hDB, char *result, INT buf_size)
{
DATABASE_HEADER *pheader;
INT total_size_key, total_size_data;
FREE_DESCRIP *pfree;

  pheader = _database[hDB-1].database_header;

  sprintf(result, "Keylist:\n");
  strcat(result, "--------\n");
  total_size_key = 0;
  pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);

  while ((PTYPE) pfree != (PTYPE) pheader)
    {
    total_size_key += pfree->size;
    sprintf(result+strlen(result), "Free block at %8d, size %8d, next %8d\n",
      (PTYPE) pfree - (PTYPE) pheader - sizeof(DATABASE_HEADER),
      pfree->size,
      pfree->next_free ?
      pfree->next_free - sizeof(DATABASE_HEADER) : 0);
    pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
    }

  strcat(result, "\nData:\n");
  strcat(result, "-----\n");
  total_size_data = 0;
  pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);

  while ((PTYPE) pfree != (PTYPE) pheader)
    {
    total_size_data += pfree->size;
    sprintf(result+strlen(result), "Free block at %8d, size %8d, next %8d\n",
      (PTYPE) pfree - (PTYPE) pheader - sizeof(DATABASE_HEADER),
      pfree->size,
      pfree->next_free ?
      pfree->next_free - sizeof(DATABASE_HEADER) : 0);
    pfree = (FREE_DESCRIP *) ((char *) pheader + pfree->next_free);
    }
  sprintf(result+strlen(result), "\nTotal size: %1d keylist, %1d data\n",
          total_size_key, total_size_data);
  sprintf(result+strlen(result), "\nFree: %1d (%1.1lf%%) keylist, %1d (%1.1lf%%) data\n",
          total_size_key,
          100 * (double) total_size_key / pheader->key_size,
          total_size_data,
          100 * (double) total_size_data / pheader->data_size);

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_open_database(char *database_name, INT database_size, 
                     HNDLE *hDB, char *client_name)
/********************************************************************\

  Routine: db_open_database

  Purpose: Open an online database

  Input:
    char *database_name     Database name.
    INT  database_size      Initial size of database if not existing
    char *client_name       Name of this application

  Output:
    INT  *hDB               Handle to database

  Function value:
    DB_SUCCESS              Successful completion
    DB_CREATED              Database was created
    DB_INVALID_NAME         Invalid database name
    DB_NO_MEMORY            Not enough memeory to create new database
                            descriptor
    DB_MEMSIZE_MISMATCH     Database size conflicts with an existing
                            database of different size
    DB_NO_MUTEX             Cannot create mutex
    DB_INVALID_PARAM        Invalid parameter
    RPC_NET_ERROR           Network error

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_OPEN_DATABASE, database_name, database_size, hDB, client_name);

#ifdef LOCAL_ROUTINES
{
INT                  i, status;
HNDLE                handle;
DATABASE_CLIENT      *pclient;
BOOL                 shm_created;
HNDLE                shm_handle;
DATABASE_HEADER      *pheader;
KEY                  *pkey;
KEYLIST              *pkeylist;
FREE_DESCRIP         *pfree;
BOOL                 call_watchdog;
INT                  timeout;

  if (database_size <0 || database_size > 10E7)
    {
    cm_msg(MERROR, "db_open_database", "invalid database size");
    return DB_INVALID_PARAM;
    }

  /* restrict name length */
  if (strlen(database_name) >= NAME_LENGTH)
    database_name[NAME_LENGTH] = 0;

  /* allocate new space for the new database descriptor */
  if (_database_entries == 0)
    {
    _database = malloc(sizeof(DATABASE));
    memset(_database, 0, sizeof(DATABASE));
    if (_database == NULL)
      {
      *hDB = 0;
      return DB_NO_MEMORY;
      }

    _database_entries = 1;
    i = 0;
    }
  else
    {
    /* check if database already open */
    for (i=0 ; i<_database_entries ; i++)
      if (_database[i].attached &&
          equal_ustring(_database[i].name, database_name))
        {
        /* check if database belongs to this thread */
        if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_MTHREAD)
          {
          if (_database[i].index == ss_gettid())
            {
            *hDB = i+1;
            return DB_SUCCESS;
            }
          }
        else
          {
          *hDB = i+1;
          return DB_SUCCESS;
          }
        }

    /* check for a deleted entry */
    for (i=0 ; i<_database_entries ; i++)
      if (!_database[i].attached)
        break;

    /* if not found, create new one */
    if (i == _database_entries)
      {
      _database = realloc(_database, sizeof(DATABASE) * (_database_entries+1));
      memset(&_database[_database_entries], 0, sizeof(DATABASE));

      _database_entries++;
      if (_database == NULL)
        {
        _database_entries--;
        *hDB = 0;
        return DB_NO_MEMORY;
        }
      }
    }

  handle = (HNDLE) i;

  /* open shared memory region */
  status = ss_open_shm(database_name, 
                       sizeof(DATABASE_HEADER) + 2*ALIGN(database_size/2),
                       (void *) &(_database[(INT) handle].database_header),
                       &shm_handle);

  if (status == SS_NO_MEMORY || status == SS_FILE_ERROR)
    {
    *hDB = 0;
    return DB_INVALID_NAME;
    }

  /* shortcut to header */
  pheader = _database[handle].database_header;

  /* save name */
  strcpy(_database[handle].name, database_name);

  shm_created = (status == SS_CREATED);

  /* clear memeory for debugging */
  /* memset(pheader, 0, sizeof(DATABASE_HEADER) + 2*ALIGN(database_size/2)); */

  if (shm_created && pheader->name[0] == 0)
    {
    /* setup header info if database was created */
    memset(pheader, 0, sizeof(DATABASE_HEADER) + 2*ALIGN(database_size/2));

    strcpy(pheader->name, database_name);
    pheader->key_size  = ALIGN(database_size / 2);
    pheader->data_size = ALIGN(database_size / 2);
    pheader->root_key = sizeof(DATABASE_HEADER);
    pheader->first_free_key  = sizeof(DATABASE_HEADER);
    pheader->first_free_data = sizeof(DATABASE_HEADER) + pheader->key_size;

    /* set up free list */
    pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_key);
    pfree->size = pheader->key_size;
    pfree->next_free = 0;

    pfree = (FREE_DESCRIP *) ((char *) pheader + pheader->first_free_data);
    pfree->size = pheader->data_size;
    pfree->next_free = 0;

    /* create root key */
    pkey = malloc_key(pheader, sizeof(KEY));
    
    /* set key properties */
    pkey->type = TID_KEY;
    pkey->num_values = 1;
    pkey->access_mode = MODE_READ | MODE_WRITE | MODE_DELETE;
    strcpy(pkey->name, "root");
    pkey->parent_keylist = 0;

    /* create keylist */
    pkeylist = malloc_key(pheader, sizeof(KEYLIST));

    /* store keylist in data field */
    pkey->data = (PTYPE) pkeylist - (PTYPE) pheader;
    pkey->item_size = sizeof(KEYLIST);

    pkeylist->parent       = (PTYPE) pkey - (PTYPE) pheader;
    pkeylist->num_keys     = 0;
    pkeylist->first_key    = 0;
    }
  else
    {
    /* check if database size is identical */
/*
    if (pheader->data_size != ALIGN(database_size / 2))
      {
      database_size = pheader->data_size + pheader->key_size;

      status = ss_close_shm(database_name, _database[handle].database_header,
                            shm_handle, FALSE);
      if (status != SS_SUCCESS)
        return DB_MEMSIZE_MISMATCH;

      status = ss_open_shm(database_name, 
                           sizeof(DATABASE_HEADER) + database_size,
                           (void *) &(_database[handle].database_header),
                           &shm_handle);

      if (status == SS_NO_MEMORY || status == SS_FILE_ERROR)
        {
        *hDB = 0;
        return DB_INVALID_NAME;
        }

      pheader = _database[handle].database_header;
      }
  */
    }

  /* create mutex for the database */
  status = ss_mutex_create(database_name, &(_database[handle].mutex));
  if (status != SS_SUCCESS && status != SS_CREATED)
    {
    *hDB = 0;
    return DB_NO_MUTEX;
    }
  _database[handle].lock_cnt = 0;

  /* first lock database */
  db_lock_database(handle + 1);

  /*
  Now we have a DATABASE_HEADER, so let's setup a CLIENT
  structure in that database. The information there can also
  be seen by other processes.
  */

  for (i=0 ; i<MAX_CLIENTS ; i++)
    if (pheader->client[i].pid == 0)
      break;

  if (i == MAX_CLIENTS)
    {
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
  if (i+1 > pheader->max_client_index)
    pheader->max_client_index = i+1;

  /* setup database header and client structure */
  pclient = &pheader->client[i];

  memset(pclient, 0, sizeof(DATABASE_CLIENT));
  /* use client name previously set by bm_set_name */
  strcpy(pclient->name, client_name);
  pclient->pid = ss_getpid();
  pclient->tid = ss_gettid();
  pclient->thandle = ss_getthandle();
  pclient->num_open_records = 0;
  
  ss_suspend_get_port(&pclient->port);
  
  pclient->last_activity    = ss_millitime();

  cm_get_watchdog_params(&call_watchdog, &timeout);
  pclient->watchdog_timeout = timeout;

  db_unlock_database(handle + 1);

  /* setup _database entry */
  _database[handle].database_data  = _database[handle].database_header + 1;
  _database[handle].attached       = TRUE;
  _database[handle].shm_handle     = shm_handle;

  /* remember to which connection acutal buffer belongs */
  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE)
    _database[handle].index = rpc_get_server_acception();
  else
    _database[handle].index = ss_gettid();

  *hDB = (handle+1);

  /* setup dispatcher for updated records */
  ss_suspend_set_dispatch(CH_IPC, 0, cm_dispatch_ipc);

  if (shm_created) return DB_CREATED;
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_close_database(HNDLE hDB)
/********************************************************************\

  Routine: db_close_database

  Purpose: Close a database

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
    return rpc_call(RPC_DB_CLOSE_DATABASE, hDB);

#ifdef LOCAL_ROUTINES
  else
{
DATABASE_HEADER  *pheader;
DATABASE_CLIENT  *pclient;
INT              index, destroy_flag, i, j;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_close_database", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /*
  Check if database was opened by current thread. This is necessary
  in the server process where one thread may not close the database
  of other threads.
  */

  index   = _database[hDB-1].client_index;
  pheader = _database[hDB-1].database_header;
  pclient  = &pheader->client[index];

  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
      _database[hDB-1].index != rpc_get_server_acception())
    return DB_INVALID_HANDLE;
  
  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
      _database[hDB-1].index != ss_gettid())
    return DB_INVALID_HANDLE;

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_close_database", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /* close all open records */
  for (i=0 ; i<pclient->max_index ; i++)
    if (pclient->open_record[i].handle)
      db_remove_open_record(hDB, pclient->open_record[i].handle);

  /* first lock database */
  db_lock_database(hDB);

  /* mark entry in _database as empty */
  _database[hDB-1].attached = FALSE;

  /* clear entry from client structure in database header */
  memset(&(pheader->client[index]), 0, sizeof(DATABASE_CLIENT));

  /* calculate new max_client_index entry */
  for (i=MAX_CLIENTS-1 ; i>=0 ; i--)
    if (pheader->client[i].pid != 0)
      break;
  pheader->max_client_index = i+1;

  /* count new number of clients */
  for (i=MAX_CLIENTS-1,j=0 ; i>=0 ; i--)
    if (pheader->client[i].pid != 0)
      j++;
  pheader->num_clients = j;

  destroy_flag = (pheader->num_clients == 0);

  /* flush shared memory to disk */
  ss_flush_shm(pheader->name, pheader, sizeof(DATABASE_HEADER)+2*pheader->data_size);

  /* unmap shared memory, delete it if we are the last */
  ss_close_shm(pheader->name, pheader,
               _database[hDB-1].shm_handle, destroy_flag);

  /* unlock database */
  db_unlock_database(hDB);

  /* delete mutex */
  ss_mutex_delete(_database[hDB-1].mutex, destroy_flag);

  /* update _database_entries */
  if (hDB == _database_entries)
    _database_entries--;

  if (_database_entries > 0)
    _database = realloc(_database, sizeof(DATABASE) * (_database_entries));
  else
    {
    free(_database);
    _database = NULL;
    }
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

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
  else
{
DATABASE_HEADER  *pheader;
DATABASE_CLIENT  *pclient;
INT              index;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_close_database", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /*
  Check if database was opened by current thread. This is necessary
  in the server process where one thread may not close the database
  of other threads.
  */

  index   = _database[hDB-1].client_index;
  pheader = _database[hDB-1].database_header;
  pclient  = &pheader->client[index];

  if (rpc_get_server_option(RPC_OSERVER_TYPE) == ST_SINGLE &&
      _database[hDB-1].index != rpc_get_server_acception())
    return DB_INVALID_HANDLE;
  
  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_SINGLE &&
      _database[hDB-1].index != ss_gettid())
    return DB_INVALID_HANDLE;

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_close_database", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /* flush shared memory to disk */
  ss_flush_shm(pheader->name, pheader, sizeof(DATABASE_HEADER)+2*pheader->data_size);

}
#endif /* LOCAL_ROUTINES */

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

  if (rpc_is_remote())
    {
    status = rpc_call(RPC_DB_CLOSE_ALL_DATABASES);
    if (status != DB_SUCCESS)
      return status;
    }

#ifdef LOCAL_ROUTINES
{
INT i;

  for (i=_database_entries ; i>0 ; i--)
    db_close_database(i);
}
#endif /* LOCAL_ROUTINES */

  return db_close_all_records();
}

/*------------------------------------------------------------------*/

INT db_set_client_name(HNDLE hDB, char *client_name)
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
DATABASE_HEADER  *pheader;
DATABASE_CLIENT  *pclient;
INT              index;

  index   = _database[hDB-1].client_index;
  pheader = _database[hDB-1].database_header;
  pclient  = &pheader->client[index];

  strcpy(pclient->name, client_name);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef LOCAL_ROUTINES

INT db_lock_database(HNDLE hDB)
/********************************************************************\

  Routine: db_lock_database

  Purpose: Lock a database for exclusive access via system mutex calls.

  Input:
    HNDLE hDB   Handle to the database to lock
  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_lock_database", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (_database[hDB-1].lock_cnt == 0)
    ss_mutex_wait_for(_database[hDB-1].mutex, 0);

  _database[hDB-1].lock_cnt++;

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_unlock_database(HNDLE hDB)
/********************************************************************\

  Routine: db_unlock_database

  Purpose: Unlock a database via system mutex calls.

  Input:
    HNDLE hDB               Handle to the database to lock

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_unlock_database", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (_database[hDB-1].lock_cnt == 1)
    ss_mutex_release(_database[hDB-1].mutex);

  if (_database[hDB-1].lock_cnt > 0)
    _database[hDB-1].lock_cnt--;

  return DB_SUCCESS;
}

#endif /* LOCAL_ROUTINES */

/*---- helper routines ---------------------------------------------*/

char *extract_key(char *key_list, char *key_name)
{
  if (*key_list == '/')
    key_list++;

  while (*key_list && *key_list != '/')
    *key_name++ = *key_list++;
  *key_name = 0;

  return key_list;
}

BOOL equal_ustring(char *str1, char *str2)
{
  if (str1 == NULL && str2 != NULL)
    return FALSE;
  if (str1 != NULL && str2 == NULL)
    return FALSE;
  if (str1 == NULL && str2 == NULL)
    return TRUE;

  while (*str1)
    if (toupper(*str1++) != toupper(*str2++))
      return FALSE;

  if (*str2)
    return FALSE;

  return TRUE;
}

/*------------------------------------------------------------------*/

INT db_create_key(HNDLE hDB, HNDLE hKey, 
                  char *key_name, DWORD type)
/********************************************************************\

  Routine: db_create_key

  Purpose: Create a new key in a database

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle to start with, 0 for root
    char   *key_name        Name of key in the form "/key/key/key"
    DWORD  type             Type of key, one of TID_xxx

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_FULL                 Shared memory is full
    DB_KEY_EXIST            Key exists already
    DB_NO_ACCESS            Parent key is write locked

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_CREATE_KEY, hDB, hKey, key_name, type);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey, *pprev_key, *pkeyparent;
char             *pkey_name, str[MAX_STRING_LENGTH];
INT              i;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_create_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_create_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /* lock database */
  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);
  if (pkey->type != TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_create_key", "key has no subkeys");
    return DB_NO_KEY;
    }
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

  pkey_name = key_name;
  do
    {
    /* extract single key from key_name */
    pkey_name = extract_key(pkey_name, str);

    /* check if parent or current directory */
    if (strcmp(str, "..") == 0)
      {
      if (pkey->parent_keylist)
        {
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

    for (i=0 ; i<pkeylist->num_keys ; i++)
      {
      if (equal_ustring(str, pkey->name))
        break;

      pprev_key = pkey;
      pkey = (KEY *) ((char *) pheader + pkey->next_key);
      }

    if (i == pkeylist->num_keys)
      {
      /* not found: create new key */

      /* check parent for write access */
      pkeyparent = (KEY *) ((char *) pheader + pkeylist->parent);
      if (!(pkeyparent->access_mode & MODE_WRITE) ||
           (pkeyparent->access_mode & MODE_EXCLUSIVE))
        {
        db_unlock_database(hDB);
        return DB_NO_ACCESS;
        }

      pkeylist->num_keys++;

      if (*pkey_name == '/' || type == TID_KEY)
        {
        /* create new key with keylist */
        pkey = malloc_key(pheader, sizeof(KEY));
        
        if (pkey == NULL)
          {
          db_unlock_database(hDB);
          cm_msg(MERROR, "db_create_key", "online database full");
          return DB_FULL;
          }

        /* append key to key list */
        if (pprev_key)
          pprev_key->next_key = (PTYPE) pkey - (PTYPE) pheader;
        else
          pkeylist->first_key = (PTYPE) pkey - (PTYPE) pheader;

        /* set key properties */
        pkey->type = TID_KEY;
        pkey->num_values = 1;
        pkey->access_mode = MODE_READ | MODE_WRITE | MODE_DELETE;
        strcpy(pkey->name, str);
        pkey->parent_keylist = (PTYPE) pkeylist - (PTYPE) pheader;

        /* find space for new keylist */
        pkeylist = malloc_key(pheader, sizeof(KEYLIST));

        if (pkeylist == NULL)
          {
          db_unlock_database(hDB);
          cm_msg(MERROR, "db_create_key", "online database full");
          return DB_FULL;
          }

        /* store keylist in data field */
        pkey->data = (PTYPE) pkeylist - (PTYPE) pheader;
        pkey->item_size = sizeof(KEYLIST);
        pkey->total_size = sizeof(KEYLIST);

        pkeylist->parent       = (PTYPE) pkey - (PTYPE) pheader;
        pkeylist->num_keys     = 0;
        pkeylist->first_key    = 0;
        }
      else
        {
        /* create new key with data */
        pkey = malloc_key(pheader, sizeof(KEY));

        if (pkey == NULL)
          {
          db_unlock_database(hDB);
          cm_msg(MERROR, "db_create_key", "online database full");
          return DB_FULL;
          }

        /* append key to key list */
        if (pprev_key)
          pprev_key->next_key = (PTYPE) pkey - (PTYPE) pheader;
        else
          pkeylist->first_key = (PTYPE) pkey - (PTYPE) pheader;

        pkey->type = type;
        pkey->num_values = 1;
        pkey->access_mode = MODE_READ | MODE_WRITE | MODE_DELETE;
        strcpy(pkey->name, str);
        pkey->parent_keylist = (PTYPE) pkeylist - (PTYPE) pheader;

        /* zero data */
        if (type != TID_STRING && type != TID_LINK)
          {
          pkey->item_size = rpc_tid_size(type);
          pkey->data = (PTYPE) malloc_data(pheader, pkey->item_size);
          pkey->total_size = pkey->item_size;
                 
          if (pkey->data == 0)
            {
            db_unlock_database(hDB);
            cm_msg(MERROR, "db_create_key", "online database full");
            return DB_FULL;
            }

          pkey->data -= (PTYPE) pheader;
          }
        else
          {
          /* first data is empty */
          pkey->item_size = 0;
          pkey->total_size = 0;
          pkey->data = 0;
          }
        }
      }
    else
      {
      /* key found: descend */

      /* resolve links */
      if (pkey->type == TID_LINK && pkey_name[0])
        {
        /* copy destination, strip '/' */
        strcpy(str, (char *) pheader + pkey->data);
        if (str[strlen(str)-1] == '/')
          str[strlen(str)-1] = 0;

        /* append rest of key name */
        strcat(str, pkey_name);

        db_unlock_database(hDB);

        return db_create_key(hDB, 0, str, type);
        }

      if (!(*pkey_name == '/'))
        {
        if ((WORD) pkey->type != type)
          cm_msg(MERROR, "db_create_key", "redefinition of key type mismatch");

        db_unlock_database(hDB);
        return DB_KEY_EXIST;
        }

      if (pkey->type != TID_KEY)
        {
        db_unlock_database(hDB);
        cm_msg(MERROR, "db_create_key", "key used with value and as parent key");
        return DB_KEY_EXIST;
        }

      pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);
      }
    } while (*pkey_name == '/');

  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_create_link(HNDLE hDB, HNDLE hKey, 
                   char *link_name, char *destination)
/********************************************************************\

  Routine: db_create_link

  Purpose: Create a link to a key or set the destination of and
           existing link.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle to start with, 0 for root
    char   *key_name        Name of key in the form "/key/key/key"
    char   *destination     Destination of link in the form "/key/key/key"

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_FULL                 Shared memory is full
    DB_KEY_EXIST            Key exists already

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_CREATE_LINK, hDB, hKey, link_name, destination);

  return db_set_value(hDB, hKey, link_name, destination, 
                      strlen(destination)+1, 1, TID_LINK);
}

/*------------------------------------------------------------------*/

INT db_delete_key1(HNDLE hDB, HNDLE hKey, INT level, BOOL follow_links)
/********************************************************************\

  Routine: db_delete_key1

  Purpose: Delete a subtree, using level information
           (only called internally by db_delete_key)

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle to start with, 0 for root
    INT    level            Recursion level, must be zero when
    BOOL   follow_links     Follow links when TRUE
                            called from a user routine

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_ACCESS            Key is locked for delete
    DB_OPEN_RECORD          Key, subkey or parent key is open

\********************************************************************/
{
#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey, *pnext_key, *pkey_tmp;
HNDLE            hKeyLink;
BOOL             deny_delete;
INT              status;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_delete_key1", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_delete_key1", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_delete_key1", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  pheader  = _database[hDB-1].database_header;

  /* lock database at the top level */
  if (level == 0)
    db_lock_database(hDB);

  pkey = (KEY *) ((char *) pheader + hKey);

  /* check if someone has opened key or parent */
  if (level == 0)
    do
      {
      if (pkey->notify_count)
        {
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
  if (pkey->type == TID_KEY && pkeylist->first_key)
    {
    pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

    do
      {
      pnext_key = (KEY *) (PTYPE) pkey->next_key;

      status = db_delete_key1(hDB, (PTYPE) pkey - (PTYPE) pheader, 
                             level+1, follow_links);

      if (status == DB_NO_ACCESS)
        deny_delete = TRUE;
      
      if (pnext_key)
        pkey = (KEY *) ((char *) pheader + (PTYPE) pnext_key);
      } while (pnext_key);
    }

  /* follow links if requested */
  if (pkey->type == TID_LINK && follow_links)
    {
    status = db_find_key(hDB, 0, (char *) pheader + pkey->data, 
                         &hKeyLink);
    if (status == DB_SUCCESS && follow_links<100)
      db_delete_key1(hDB, hKeyLink, level+1, follow_links+1);

    if (follow_links == 100)
      cm_msg(MERROR, "db_delete_key1", "try to delete cyclic link");
    }

  pkey = (KEY *) ((char *) pheader + hKey);

  /* return if key was already deleted by cyclic link */
  if (pkey->parent_keylist == 0)
    {
    if (level == 0)
      db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  /* now delete key */
  if (hKey != pheader->root_key)
    {
    if (!(pkey->access_mode & MODE_DELETE) || deny_delete)
      {
      if (level == 0)
        db_unlock_database(hDB);
      return DB_NO_ACCESS;
      }

    if (pkey->notify_count)
      {
      if (level == 0)
        db_unlock_database(hDB);
      return DB_OPEN_RECORD;
      }

    /* delete key data */
    if (pkey->type == TID_KEY)
      free_key(pheader, (char *) pheader + pkey->data, pkey->total_size);
    else
      free_data(pheader, (char *) pheader + pkey->data, pkey->total_size);
    
    /* unlink key from list */
    pnext_key = (KEY *) (PTYPE) pkey->next_key;
    pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);

    if ((KEY *) ((char *) pheader + pkeylist->first_key) == pkey)
      {
      /* key is first in list */
      pkeylist->first_key = (PTYPE) pnext_key;
      }
    else
      {
      /* find predecessor */
      pkey_tmp = (KEY *) ((char *) pheader + pkeylist->first_key);
      while ((KEY *) ((char *) pheader + pkey_tmp->next_key) != pkey)
        pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);
      pkey_tmp->next_key = (PTYPE) pnext_key;
      }
    
    /* delete key */
    free_key(pheader, pkey, sizeof(KEY));
    pkeylist->num_keys--;
    }

  if (level == 0)
    db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_delete_key(HNDLE hDB, HNDLE hKey, BOOL follow_links)
/********************************************************************\

  Routine: db_delete_key

  Purpose: Delete a subtree in a database starting from a key 
           (including this key)

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle to start with, 0 for root
    BOOL   follow_links     Follow links when TRUE

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_ACCESS            Key is locked for delete
    DB_OPEN_RECORD          Key, subkey or parent key is open

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_DELETE_KEY, hDB, hKey, follow_links);

  return db_delete_key1(hDB, hKey, 0, follow_links);
}

/*------------------------------------------------------------------*/

INT db_find_key(HNDLE hDB, HNDLE hKey, char *key_name, 
                HNDLE *subhKey)
/********************************************************************\

  Routine: db_find_key

  Purpose: Find a key by name and return its handle (internal address)

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
    return rpc_call(RPC_DB_FIND_KEY, hDB, hKey, key_name,
                                        subhKey);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey;
char             *pkey_name, str[MAX_STRING_LENGTH];
INT              i;

  *subhKey = 0;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_find_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_find_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);
  if (pkey->type != TID_KEY)
    {
    cm_msg(MERROR, "db_find_key", "key has no subkeys");
    *subhKey = 0;
    db_unlock_database(hDB);
    return DB_NO_KEY;
    }
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

  if (key_name[0] == 0 ||
      strcmp(key_name, "/") == 0)
    {
    if (!(pkey->access_mode & MODE_READ))
      {
      *subhKey = 0;
      db_unlock_database(hDB);
      return DB_NO_ACCESS;
      }

    *subhKey = (PTYPE) pkey - (PTYPE) pheader;

    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  pkey_name = key_name;
  do
    {
    /* extract single subkey from key_name */
    pkey_name = extract_key(pkey_name, str);

    /* check if parent or current directory */
    if (strcmp(str, "..") == 0)
      {
      if (pkey->parent_keylist)
        {
        pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
        pkey = (KEY *) ((char *) pheader + pkeylist->parent);
        }
      continue;
      }
    if (strcmp(str, ".") == 0)
      continue;

    /* check if key is in keylist */
    pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

    for (i=0 ; i<pkeylist->num_keys ; i++)
      {
      if (equal_ustring(str, pkey->name))
        break;

      pkey = (KEY *) ((char *) pheader + pkey->next_key);
      }

    if (i == pkeylist->num_keys)
      {
      *subhKey = 0;
      db_unlock_database(hDB);
      return DB_NO_KEY;
      }

    /* resolve links */
    if (pkey->type == TID_LINK)
      {
      /* copy destination, strip '/' */
      strcpy(str, (char *) pheader + pkey->data);
      if (str[strlen(str)-1] == '/')
        str[strlen(str)-1] = 0;

      /* append rest of key name if existing */
      if (pkey_name[0])
        {
        strcat(str, pkey_name);
        db_unlock_database(hDB);
        return db_find_key(hDB, 0, str, subhKey);
        }
      else
        {
        /* if last key in chain is a link, return its destination */
        db_unlock_database(hDB);
        return db_find_link(hDB, 0, str, subhKey);
        }
      }

    /* key found: check if last in chain */
    if (*pkey_name == '/')
      {
      if (pkey->type != TID_KEY)
        {
        *subhKey = 0;
        db_unlock_database(hDB);
        return DB_NO_KEY;
        }
      }

    /* descend one level */
    pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

    } while (*pkey_name == '/' && *(pkey_name+1));

  *subhKey = (PTYPE) pkey - (PTYPE) pheader;
}
#endif /* LOCAL_ROUTINES */

  db_unlock_database(hDB);
  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_find_link(HNDLE hDB, HNDLE hKey, char *key_name, 
                 HNDLE *subhKey)
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
    return rpc_call(RPC_DB_FIND_LINK, hDB, hKey, key_name,
                                         subhKey);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey;
char             *pkey_name, str[MAX_STRING_LENGTH];
INT              i;

  *subhKey = 0;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_find_link", "Invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_find_link", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);
  if (pkey->type != TID_KEY)
    {
    cm_msg(MERROR, "db_find_link", "key has no subkeys");
    db_unlock_database(hDB);
    return DB_NO_KEY;
    }
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

  if (key_name[0] == 0 ||
      strcmp(key_name, "/") == 0)
    {
    if (!(pkey->access_mode & MODE_READ))
      {
      *subhKey = 0;
      db_unlock_database(hDB);
      return DB_NO_ACCESS;
      }

    *subhKey = (PTYPE) pkey - (PTYPE) pheader;

    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  pkey_name = key_name;
  do
    {
    /* extract single subkey from key_name */
    pkey_name = extract_key(pkey_name, str);

    /* check if parent or current directory */
    if (strcmp(str, "..") == 0)
      {
      if (pkey->parent_keylist)
        {
        pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
        pkey = (KEY *) ((char *) pheader + pkeylist->parent);
        }
      continue;
      }
    if (strcmp(str, ".") == 0)
      continue;

    /* check if key is in keylist */
    pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

    for (i=0 ; i<pkeylist->num_keys ; i++)
      {
      if (equal_ustring(str, pkey->name))
        break;

      pkey = (KEY *) ((char *) pheader + pkey->next_key);
      }

    if (i == pkeylist->num_keys)
      {
      *subhKey = 0;
      db_unlock_database(hDB);
      return DB_NO_KEY;
      }

    /* resolve links if not last in chain */
    if (pkey->type == TID_LINK && *pkey_name == '/')
      {
      /* copy destination, strip '/' */
      strcpy(str, (char *) pheader + pkey->data);
      if (str[strlen(str)-1] == '/')
        str[strlen(str)-1] = 0;

      /* append rest of key name */
      strcat(str, pkey_name);
      db_unlock_database(hDB);
      return db_find_link(hDB, 0, str, subhKey);
      }

    /* key found: check if last in chain */
    if ((*pkey_name == '/'))
      {
      if (pkey->type != TID_KEY)
        {
        *subhKey = 0;
        db_unlock_database(hDB);
        return DB_NO_KEY;
        }
      }

    /* descend one level */
    pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

    } while (*pkey_name == '/' && *(pkey_name+1));

  *subhKey = (PTYPE) pkey - (PTYPE) pheader;
}
#endif /* LOCAL_ROUTINES */

  db_unlock_database(hDB);
  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_scan_tree(HNDLE hDB, HNDLE hKey, INT level,
                 void (*callback)(HNDLE,HNDLE,KEY*,INT,void *), void *info)
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
KEY   key;
INT   i, status;

  status = db_get_key(hDB, hKey, &key);
  if (status != DB_SUCCESS)
    return status;

  callback(hDB, hKey, &key, level, info);

  if (key.type == TID_KEY)
    {
    for (i=0 ; ; i++)
      {
      db_enum_key(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
        break;

      db_scan_tree(hDB, hSubkey, level+1, callback, info);
      }
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_scan_tree_link(HNDLE hDB, HNDLE hKey, INT level,
                      void (*callback)(HNDLE,HNDLE,KEY*,INT,void *), void *info)
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
KEY   key;
INT   i, status;

  status = db_get_key(hDB, hKey, &key);
  if (status != DB_SUCCESS)
    return status;

  callback(hDB, hKey, &key, level, info);

  if (key.type == TID_KEY)
    {
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
        break;

      db_scan_tree_link(hDB, hSubkey, level+1, callback, info);
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
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey;
char             str[MAX_ODB_PATH];

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_path", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_path", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);

  if (hKey == pheader->root_key)
    {
    strcpy(path, "/");
    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  *path = 0;
  do 
    {
    /* add key name in front of path */
    strcpy(str, path);
    strcpy(path, "/");
    strcat(path, pkey->name);

    if (strlen(path)+strlen(str)+1 > (DWORD) buf_size)
      {
      *path=0;
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
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

void db_find_open_records(HNDLE hDB, HNDLE hKey, KEY *key, INT level, void *result)
{
DATABASE_HEADER *pheader;
DATABASE_CLIENT *pclient;
INT             i, j, size, status;
char            line[256], str[80], host[HOST_NAME_LENGTH];

  pheader  = _database[hDB-1].database_header;

  /* check if this key has notify count set */
  if (key->notify_count)
    {
    db_get_path(hDB, hKey, str, sizeof(str));
    sprintf(line, "%s open %d times by ", str, key->notify_count);
    for (i=0 ; i<pheader->max_client_index ; i++)
      {
      pclient = &pheader->client[i];
      for (j=0 ; j<pclient->max_index ; j++)
        if (pclient->open_record[j].handle == hKey)
          {
          sprintf(str, "/system/clients/%1d/host", pclient->tid);
          size = sizeof(host);
          host[0] = 0;
          status = db_get_value(hDB, 0, str, host, &size, TID_STRING);
          if (status == DB_SUCCESS)
            sprintf(line+strlen(line), "%s on %s", pclient->name, host);
          else
            sprintf(line+strlen(line), "%s",pclient->name);
          }
      }
    strcat(line, "\n");
    strcat((char *) result, line);
    }
}

INT db_get_open_records(HNDLE hDB, HNDLE hKey, char *str, INT buf_size)
/********************************************************************\

  Routine: db_get_open_records

  Purpose: Return a string with all open records 

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key to start search from, 0 for root
    INT    buf_size         Size of string

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

  db_scan_tree_link(hDB, hKey, 0, db_find_open_records, str);
  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_set_value(HNDLE hDB, HNDLE hKeyRoot, char *key_name, void *data, 
                 INT data_size, INT num_values, DWORD type)
/********************************************************************\

  Routine: db_set_value

  Purpose: Set value of a single key

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKeyRoot         Key to start search from, 0 for root
    char   *key_name        Name of key in the form "/key/key/key"
    void   *data            Address of data
    INT    data_size        Size of data
    INT    num_values       Number of data elements
    DWORD  type             Type of key, one of TID_xxx

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TYPE_MISMATCH        Key was created with different type
    DB_NO_ACCESS            Key is locked for writing

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_SET_VALUE, hDB, hKeyRoot, key_name, 
                    data, data_size, num_values, type);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;
HNDLE            hKey;
INT              status;

  status = db_find_key(hDB, hKeyRoot, key_name, &hKey);
  if (status == DB_NO_KEY)
    {
    db_create_key(hDB, hKeyRoot, key_name, type);
    status = db_find_link(hDB, hKeyRoot, key_name, &hKey);
    }

  if (status != DB_SUCCESS)
    return status;

  pheader  = _database[hDB-1].database_header;

  db_lock_database(hDB);

  /* get address from handle */
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for write access */
  if (!(pkey->access_mode & MODE_WRITE) ||
       (pkey->access_mode & MODE_EXCLUSIVE))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_value", "\"%s\" is of type %s, not %s", key_name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_value", "key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  if (data_size == 0)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_value", "zero data size not allowed");
    return DB_TYPE_MISMATCH;
    }

  /* resize data size if necessary */
  if (pkey->total_size < data_size)
    {
    pkey->data = (PTYPE) realloc_data(pheader, (char *) pheader + pkey->data,
                                      pkey->total_size, data_size);
                 
    if (pkey->data == 0)
      {
      db_unlock_database(hDB);
      cm_msg(MERROR, "db_set_value", "online database full");
      return DB_FULL;
      }

    pkey->data -= (PTYPE) pheader;
    pkey->total_size = data_size;
    }

  /* set number of values */
  pkey->num_values = num_values;

  if (type == TID_STRING || 
      type == TID_LINK)
    pkey->item_size = data_size / num_values;
  else
    pkey->item_size = rpc_tid_size(type);

  /* copy data */
  memcpy((char *) pheader + pkey->data, data, data_size);

  /* update time */
  pkey->last_written = ss_time();

  db_unlock_database(hDB);

  db_notify_clients(hDB, hKey, TRUE);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_value(HNDLE hDB, HNDLE hKeyRoot, char *key_name, void *data, 
                 INT *buf_size, DWORD type)
/********************************************************************\

  Routine: db_get_value

  Purpose: Get value of a single key

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKeyRoot         Key to start search from, 0 for root
    char   *key_name        Name of key in the form "/key/key/key"
    void   *data            Address of default data
    INT    *buf_size        Size of data buffer
    DWORD  type             Type of key, one of TID_xxx

  Output:
    void   *data            Key data
    INT    *buf_size        Size of copied data

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_KEY               Key doesn't exist
    DB_TRUNCATED            Return buffer is smaller than key data
    DB_TYPE_MISMATCH        Key was created with different type

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_GET_VALUE, hDB, hKeyRoot, key_name, 
                    data, buf_size, type);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
HNDLE            hkey;
KEY              *pkey;
INT              status, size;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_value", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_value", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  pheader  = _database[hDB-1].database_header;

  db_lock_database(hDB);

  status = db_find_key(hDB, hKeyRoot, key_name, &hkey);
  if (status == DB_NO_KEY)
    {
    db_create_key(hDB, hKeyRoot, key_name, type);
    status = db_find_key(hDB, hKeyRoot, key_name, &hkey);
    if (status != DB_SUCCESS)
      {
      db_unlock_database(hDB);
      return status;
      }

    /* get string size from data size */
    if (type == TID_STRING || 
        type == TID_LINK)
      size = *buf_size;
    else
      size = rpc_tid_size(type);

    if (size == 0)
      {
      db_unlock_database(hDB);
      return DB_TYPE_MISMATCH;
      }

    /* set default value if key was created */
    status = db_set_value(hDB, hKeyRoot, key_name, data, 
                          *buf_size, *buf_size/size, type);
    }

  if (status != DB_SUCCESS)
    {
    db_unlock_database(hDB);
    return status;
    }

  /* get address from handle */
  pkey = (KEY *) ((char *) pheader + hkey);

  /* check for correct type */
  if (pkey->type != (type))
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_value", "\"%s\" is of type %s, not %s", key_name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* check for read access */
  if (!(pkey->access_mode & MODE_READ))
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_value", "%s has no read access", key_name);
    return DB_NO_ACCESS;
    }

  /* check if buffer is too small */
  if (pkey->num_values * pkey->item_size > *buf_size)
    {
    memcpy(data, (char *) pheader + pkey->data, *buf_size);
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_value", "buffer too small, %s data truncated", key_name);
    return DB_TRUNCATED;
    }

  /* copy key data */
  memcpy(data, (char *) pheader + pkey->data, pkey->num_values * pkey->item_size);
  *buf_size = pkey->num_values * pkey->item_size;

  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_enum_key(HNDLE hDB, HNDLE hKey, 
                INT index, HNDLE *subkey_handle)
/********************************************************************\

  Routine: db_enum_key

  Purpose: Enumerate subkeys from a key, follow links

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to enumerate, zero for the
                            root key
    INT   index             Subkey index, sould be initially 0, then
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
    return rpc_call(RPC_DB_ENUM_KEY, hDB, hKey, index, subkey_handle);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey;
INT              i;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_enum_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_enum_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /* first lock database */
  db_lock_database(hDB);

  *subkey_handle = 0;

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);
  if (pkey->type != TID_KEY)
    {
    db_unlock_database(hDB);
    return DB_NO_MORE_SUBKEYS;
    }
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

  if (index >= pkeylist->num_keys)
    {
    db_unlock_database(hDB);
    return DB_NO_MORE_SUBKEYS;
    }

  pkey = (KEY *) ((char *) pheader + pkeylist->first_key);
  for (i=0 ; i<index ; i++)
    pkey = (KEY *) ((char *) pheader + pkey->next_key);
  
  /* resolve links */
  if (pkey->type == TID_LINK)
    {
    db_unlock_database(hDB);

    if (*((char *) pheader + pkey->data) == '/')
      {
      /* absolute path */
      return db_find_key(hDB, 0, (char *) pheader + pkey->data, subkey_handle);
      }
    else
      {
      /* relative path */
      if (pkey->parent_keylist)
        {
        pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
        return db_find_key(hDB, pkeylist->parent, 
          (char *) pheader + pkey->data, subkey_handle);
        }
      else
        return db_find_key(hDB, 0, (char *) pheader + pkey->data, subkey_handle);
      }
    }

  *subkey_handle = (PTYPE) pkey - (PTYPE) pheader;
  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_enum_link(HNDLE hDB, HNDLE hKey, 
                 INT index, HNDLE *subkey_handle)
/********************************************************************\

  Routine: db_enum_link

  Purpose: Enumerate subkeys from a key, don't follow links

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to enumerate, zero for the
                            root key
    INT   index             Subkey index, sould be initially 0, then
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
    return rpc_call(RPC_DB_ENUM_LINK, hDB, hKey, index, subkey_handle);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey;
INT              i;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_enum_link", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_enum_link", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  /* first lock database */
  db_lock_database(hDB);

  *subkey_handle = 0;

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);
  if (pkey->type != TID_KEY)
    {
    db_unlock_database(hDB);
    return DB_NO_MORE_SUBKEYS;
    }
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

  if (index >= pkeylist->num_keys)
    {
    db_unlock_database(hDB);
    return DB_NO_MORE_SUBKEYS;
    }

  pkey = (KEY *) ((char *) pheader + pkeylist->first_key);
  for (i=0 ; i<index ; i++)
    pkey = (KEY *) ((char *) pheader + pkey->next_key);
  
  *subkey_handle = (PTYPE) pkey - (PTYPE) pheader;
  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_key(HNDLE hDB, HNDLE hKey, KEY *key)
/********************************************************************\

  Routine: db_get_key

  Purpose: Get key structure from a handle

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to enumerate

  Output:
    KEY    *key             Pointer to KEY stucture

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database or key handle is invalid

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_GET_KEY, hDB, hKey, key); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_get_key", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  if (!pkey->type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_key", "invalid key");
    return DB_INVALID_HANDLE;
    }

  memcpy(key, pkey, sizeof(KEY));

  db_unlock_database(hDB);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_key_time(HNDLE hDB, HNDLE hKey, DWORD *delta)
/********************************************************************\

  Routine: db_get_key_time

  Purpose: Get time when key was last modified.

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to enumerate

  Output:
    DWORD *delta            Seconds since last update

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database or key handle is invalid

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_GET_KEY_TIME, hDB, hKey, delta); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_get_key", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  *delta = ss_time() - pkey->last_written;

  db_unlock_database(hDB);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_key_info(HNDLE hDB, HNDLE hKey, char *name, INT name_size,
                    INT *type, INT *num_values, INT *item_size)
/********************************************************************\

  Routine: db_get_key_info

  Purpose: Get key info (separate values instead of structure)

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to retrieve info

  Output:
    char  *name             Key name
    INT   *type             TID value
    INT   *num_values       Number of values in key
    INT   *item_size        Size of individual key value (used for 
                            strings)

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database or key handle is invalid

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_GET_KEY_INFO, hDB, hKey, type, 
                    num_values, item_size); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;
KEYLIST          *pkeylist;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_key_info", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_key_info", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_get_key_info", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  if ((INT) strlen(pkey->name)+1 > name_size)
    {
    /* truncate name */
    memcpy(name, pkey->name, name_size-1);
    name[name_size] = 0;
    }
  else
    strcpy(name, pkey->name);

  /* convert "root" to "/" */
  if (strcmp(name, "root") == 0)
    strcpy(name, "/");

  *type       = pkey->type;
  *num_values = pkey->num_values;
  *item_size  = pkey->item_size;

  if (pkey->type == TID_KEY)
    {
    pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);
    *num_values = pkeylist->num_keys;
    }

  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_rename_key(HNDLE hDB, HNDLE hKey, char *name)
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

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_RENAME_KEY, hDB, hKey, name); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_rename_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_rename_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_rename_key", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  if (!pkey->type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_rename_key", "invalid key");
    return DB_INVALID_HANDLE;
    }

  name[NAME_LENGTH] = 0;
  strcpy(pkey->name, name);

  db_unlock_database(hDB);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_reorder_key(HNDLE hDB, HNDLE hKey, INT index)
/********************************************************************\

  Routine: db_reorder_key

  Purpose: Reorder key so that key hKey apprears at position 'index'
           in keylist (or at bottom if index<0)

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key
    INT   index             New positio of key in keylist

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
    return rpc_call(RPC_DB_REORDER_KEY, hDB, hKey, index);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey, *pprev_key, *pnext_key, *pkey_tmp;
KEYLIST          *pkeylist;
INT              i;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_rename_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_rename_key", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_rename_key", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  if (!pkey->type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_reorder_key", "invalid key");
    return DB_INVALID_HANDLE;
    }

  if (!(pkey->access_mode & MODE_WRITE))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  /* check if someone has opened key or parent */
  do
    {
    if (pkey->notify_count)
      {
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
  pnext_key = (KEY *) (PTYPE) pkey->next_key;

  if ((KEY *) ((char *) pheader + pkeylist->first_key) == pkey)
    {
    /* key is first in list */
    pkeylist->first_key = (PTYPE) pnext_key;
    }
  else
    {
    /* find predecessor */
    pkey_tmp = (KEY *) ((char *) pheader + pkeylist->first_key);
    while ((KEY *) ((char *) pheader + pkey_tmp->next_key) != pkey)
      pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);
    pkey_tmp->next_key = (PTYPE) pnext_key;
    }

  /* add key to list at proper index */
  pkey_tmp = (KEY *) ((char *) pheader + pkeylist->first_key);
  if (index < 0 || index >= pkeylist->num_keys -1)
    {
    /* add at bottom */

    /* find last key */
    for (i=0 ; i<pkeylist->num_keys-2 ; i++)
      {
      pprev_key = pkey_tmp;
      pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);
      }

    pkey_tmp->next_key = (PTYPE) pkey - (PTYPE) pheader;
    pkey->next_key = 0;
    }
  else
    {
    if (index == 0)
      {
      /* add at top */
      pkey->next_key = pkeylist->first_key;
      pkeylist->first_key = (PTYPE) pkey - (PTYPE) pheader;
      }
    else
      {
      /* add at position index */
      for (i=0 ; i<index-1 ; i++)
        pkey_tmp = (KEY *) ((char *) pheader + pkey_tmp->next_key);

      pkey->next_key = pkey_tmp->next_key;
      pkey_tmp->next_key = (PTYPE) pkey - (PTYPE) pheader;
      }
    }

  db_unlock_database(hDB);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_data(HNDLE hDB, HNDLE hKey, void *data, INT *buf_size, 
                DWORD type)
/********************************************************************\

  Routine: db_get_data

  Purpose: Get key data from a handle

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key
    INT    *buf_size        Size of data buffer
    DWORD  type             Type of data

  Output:
    void   *data            Key data
    INT    *buf_size        Size of key data

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TRUNCATED            Return buffer is smaller than key data
    DB_TYPE_MISMATCH        Type mismatch

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_GET_DATA, hDB, hKey, data, buf_size, type); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_data", "Invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_get_data", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for read access */
  if (!(pkey->access_mode & MODE_READ))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (!pkey->type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data", "invalid key");
    return DB_INVALID_HANDLE;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data", "\"%s\" is of type %s, not %s", pkey->name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data", "Key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* check if key has data */
  if (pkey->data == 0)
    {
    memset(data, 0, *buf_size);
    *buf_size = 0;
    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  /* check if buffer is too small */
  if (pkey->num_values * pkey->item_size > *buf_size)
    {
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
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_data1(HNDLE hDB, HNDLE hKey, void *data, INT *buf_size, 
                 DWORD type, INT *num_values)
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
    return rpc_call(RPC_DB_GET_DATA, hDB, hKey, data, buf_size, type); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_data", "Invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_get_data", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for read access */
  if (!(pkey->access_mode & MODE_READ))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (!pkey->type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data", "invalid key");
    return DB_INVALID_HANDLE;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data", "\"%s\" is of type %s, not %s", pkey->name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data", "Key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* check if key has data */
  if (pkey->data == 0)
    {
    memset(data, 0, *buf_size);
    *buf_size = 0;
    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  /* check if buffer is too small */
  if (pkey->num_values * pkey->item_size > *buf_size)
    {
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
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_data_index(HNDLE hDB, HNDLE hKey, 
                      void *data, INT *buf_size, INT index, DWORD type)
/********************************************************************\

  Routine: db_get_data_index

  Purpose: Get key data from a handle

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key
    INT    *buf_size        Size of data buffer
    INT    index            Index of array [0..n-1]
    DWORD  type             Type of data

  Output:
    void   *data            Key data
    INT    *buf_size        Size of key data

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TRUNCATED            Return buffer is smaller than key data
    DB_OUT_OF_RANGE         Index out of range

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_GET_DATA_INDEX, hDB, hKey, data, buf_size, index, type); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_get_data", "Invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_get_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_get_data", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for read access */
  if (!(pkey->access_mode & MODE_READ))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (!pkey->type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data_index", "invalid key");
    return DB_INVALID_HANDLE;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data_index", "\"%s\" is of type %s, not %s", pkey->name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data_index", "Key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* check if key has data */
  if (pkey->data == 0)
    {
    memset(data, 0, *buf_size);
    *buf_size = 0;
    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  /* check if index in range */
  if (index < 0 || index >= pkey->num_values)
    {
    memset(data, 0, *buf_size);
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data_index", "index (%d) exceeds array length (%d) for key %s", 
           index, pkey->num_values, pkey->name);
    return DB_OUT_OF_RANGE;
    }

  /* check if buffer is too small */
  if (pkey->item_size > *buf_size)
    {
    /* copy data */
    memcpy(data, (char *) pheader + pkey->data + index * pkey->item_size, *buf_size);
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_get_data_index", "data for key \"%s\" truncated", pkey->name);
    return DB_TRUNCATED;
    }

  /* copy key data */
  memcpy(data, (char *) pheader + pkey->data + index * pkey->item_size, pkey->item_size);
  *buf_size = pkey->item_size;

  db_unlock_database(hDB);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_set_data(HNDLE hDB, HNDLE hKey, 
                void *data, INT buf_size, INT num_values, DWORD type)
/********************************************************************\

  Routine: db_set_data

  Purpose: Set key data from a handle. Adjust number of values if
           previous data has different size.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key to enumerate
    INT    buf_size         Size of data buffer
    INT    num_values       Number of data values (for arrays)
    DWORD  type             Type of data

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TRUNCATED            Return buffer is smaller than key data

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_SET_DATA, hDB, hKey, 
                    data, buf_size, num_values, type); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_set_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_set_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_set_data", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for write access */
  if (!(pkey->access_mode & MODE_WRITE) ||
       (pkey->access_mode & MODE_EXCLUSIVE))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data", "\"%s\" is of type %s, not %s", pkey->name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data", "Key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* if no buf_size given (Java!), calculate it */
  if (buf_size == 0)
    buf_size = pkey->item_size * num_values;

  /* resize data size if necessary */
  if (pkey->total_size != buf_size)
    {
    pkey->data = (PTYPE) realloc_data(pheader, (char *) pheader + pkey->data,
                                      pkey->total_size, buf_size);
                 
    if (pkey->data == 0)
      {
      db_unlock_database(hDB);
      cm_msg(MERROR, "db_set_data", "online database full");
      return DB_FULL;
      }

    pkey->data -= (PTYPE) pheader;
    pkey->total_size = buf_size;
    }

  /* set number of values */
  pkey->num_values = num_values;
  if (num_values)
    pkey->item_size = buf_size/num_values;

  /* copy data */
  memcpy((char *) pheader + pkey->data, data, buf_size);

  /* update time */
  pkey->last_written = ss_time();

  db_unlock_database(hDB);

  db_notify_clients(hDB, hKey, TRUE);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

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
DATABASE_HEADER  *pheader;
KEY              *pkey;
INT              new_size;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_set_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_set_data", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_set_data", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for write access */
  if (!(pkey->access_mode & MODE_WRITE) ||
       (pkey->access_mode & MODE_EXCLUSIVE))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data", "Key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* resize data size if necessary */
  if (pkey->num_values != num_values)
    {
    new_size = pkey->item_size*num_values;
    pkey->data = (PTYPE) realloc_data(pheader, (char *) pheader + pkey->data,
                                      pkey->total_size, new_size);
                 
    if (pkey->data == 0)
      {
      db_unlock_database(hDB);
      cm_msg(MERROR, "db_set_data", "online database full");
      return DB_FULL;
      }

    pkey->data -= (PTYPE) pheader;
    pkey->total_size = new_size;
    pkey->num_values = num_values;
    }

  /* update time */
  pkey->last_written = ss_time();

  db_unlock_database(hDB);

  db_notify_clients(hDB, hKey, TRUE);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_set_data_index(HNDLE hDB, HNDLE hKey, 
                      void *data, INT data_size, INT index, DWORD type)
/********************************************************************\

  Routine: db_set_data_index

  Purpose: Set key data for a key which contains an array of values

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key to enumerate
    void   *data            Pointer to single value of data
    INT    data_size        Size of single data element
    INT    index            Index of array to change [0..n-1]
    DWORD  type             Type of data

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
    return rpc_call(RPC_DB_SET_DATA_INDEX, hDB, hKey, 
                    data, data_size, index, type); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_set_data_index", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_set_data_index", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_set_data_index", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for write access */
  if (!(pkey->access_mode & MODE_WRITE) ||
       (pkey->access_mode & MODE_EXCLUSIVE))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data_index", "\"%s\" is of type %s, not %s", pkey->name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data_index", "key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* check for valid index */
  if (index < 0)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data_index", "invalid index");
    return DB_FULL;
    }

  /* increase data size if necessary */
  if (index >= pkey->num_values || pkey->item_size == 0)
    {
    pkey->data = (PTYPE) realloc_data(pheader, (char *) pheader + pkey->data,
                                      pkey->total_size, data_size*(index+1));
                 
    if (pkey->data == 0)
      {
      db_unlock_database(hDB);
      cm_msg(MERROR, "db_set_data_index", "online database full");
      return DB_FULL;
      }

    pkey->data -= (PTYPE) pheader;
    if (!pkey->item_size)
      pkey->item_size = data_size;
    pkey->total_size = data_size*(index+1);
    pkey->num_values = index + 1;
    }

  /* cut strings which are too long */
  if ((type == TID_STRING || type == TID_LINK) && 
       (int) strlen(data)+1 > pkey->item_size)
    *((char *) data + pkey->item_size-1) = 0;

  /* copy data */
  memcpy((char *) pheader + pkey->data + index * pkey->item_size, 
         data, pkey->item_size);

  /* update time */
  pkey->last_written = ss_time();

  db_unlock_database(hDB);

  db_notify_clients(hDB, hKey, TRUE);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_set_data_index2(HNDLE hDB, HNDLE hKey, void *data, 
                       INT data_size, INT index, DWORD type, BOOL bNotify)
/********************************************************************\

  Routine: db_set_data_index2

  Purpose: Set key data for a key which contains an array of values.
           Optionally notify clients which have key open.

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Handle of key to enumerate
    void   *data            Pointer to single value of data
    INT    data_size        Size of single data element
    INT    index            Index of array to change [0..n-1]
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
    return rpc_call(RPC_DB_SET_DATA_INDEX2, hDB, hKey, 
                    data, data_size, index, type, bNotify); 

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER  *pheader;
KEY              *pkey;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_set_data_index2", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_set_data_index2", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (hKey < sizeof(DATABASE_HEADER))
    {
    cm_msg(MERROR, "db_set_data_index2", "invalid key handle");
    return DB_INVALID_HANDLE;
    }

  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);

  /* check for write access */
  if (!(pkey->access_mode & MODE_WRITE) ||
       (pkey->access_mode & MODE_EXCLUSIVE))
    {
    db_unlock_database(hDB);
    return DB_NO_ACCESS;
    }

  if (pkey->type != type)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data_index2", "\"%s\" is of type %s, not %s", pkey->name, 
           rpc_tid_name(pkey->type), rpc_tid_name(type));
    return DB_TYPE_MISMATCH;
    }

  /* keys cannot contain data */
  if (pkey->type == TID_KEY)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data_index2", "key cannot contain data");
    return DB_TYPE_MISMATCH;
    }

  /* check for valid index */
  if (index < 0)
    {
    db_unlock_database(hDB);
    cm_msg(MERROR, "db_set_data_index2", "invalid index");
    return DB_FULL;
    }

  /* increase key size if necessary */
  if (index >= pkey->num_values)
    {
    pkey->data = (PTYPE) realloc_data(pheader, (char *) pheader + pkey->data,
                                      pkey->total_size, data_size*(index+1));
                 
    if (pkey->data == 0)
      {
      db_unlock_database(hDB);
      cm_msg(MERROR, "db_set_data_index2", "online database full");
      return DB_FULL;
      }

    pkey->data -= (PTYPE) pheader;
    if (!pkey->item_size)
      pkey->item_size = data_size;
    pkey->total_size = data_size*(index+1);
    pkey->num_values = index + 1;
    }

  /* cut strings which are too long */
  if ((type == TID_STRING || type == TID_LINK) && 
       (int) strlen(data)+1 > pkey->item_size)
    *((char *) data + pkey->item_size-1) = 0;

  /* copy data */
  memcpy((char *) pheader + pkey->data + index * pkey->item_size, 
         data, pkey->item_size);

  /* update time */
  pkey->last_written = ss_time();

  db_unlock_database(hDB);

  if (bNotify)
    db_notify_clients(hDB, hKey, TRUE);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT db_merge_data(HNDLE hDB, HNDLE hKeyRoot, char *name, void *data, INT data_size, 
                  INT num_values, INT type)
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
INT   status, old_size;

  status = db_find_key(hDB, hKeyRoot, name, &hKey);
  if (status != DB_SUCCESS)
    {
    db_create_key(hDB, hKeyRoot, name, type);
    db_find_key(hDB, hKeyRoot, name, &hKey);
    status = db_set_data(hDB, hKey, data, data_size, num_values, type);
    }
  else
    {
    old_size = data_size;
    db_get_data(hDB, hKey, data, &old_size, type);
    status = db_set_data(hDB, hKey, data, data_size, num_values, type);
    }

  return status;
}

/*------------------------------------------------------------------*/

INT db_set_mode(HNDLE hDB, HNDLE hKey, 
                WORD mode, BOOL recurse)
/********************************************************************\

  Routine: db_set_mode

  Purpose: Set access mode of key

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle
    DWORD  mode             Access mode, any or'ed combination of
                            MODE_READ, MODE_WRITE, MODE_EXCLUSIVE
                            and MODE_DELETE
    BOOL   recurse          Recurse subtree if TRUE, also used
                            as recurse level

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
DATABASE_HEADER  *pheader;
KEYLIST          *pkeylist;
KEY              *pkey, *pnext_key;
HNDLE            hKeyLink;

  if (hDB > _database_entries || hDB <= 0)
    {
    cm_msg(MERROR, "db_set_mode", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (!_database[hDB-1].attached)
    {
    cm_msg(MERROR, "db_set_mode", "invalid database handle");
    return DB_INVALID_HANDLE;
    }

  if (recurse < 2)
    db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  if (!hKey)
    hKey = pheader->root_key;
  pkey = (KEY *) ((char *) pheader + hKey);
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);

  if (pkey->type == TID_KEY && pkeylist->first_key &&
      recurse)
    {
    /* first recurse subtree */
    pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

    do
      {
      pnext_key = (KEY *) (PTYPE) pkey->next_key;

      db_set_mode(hDB, (PTYPE) pkey - (PTYPE) pheader, 
                  mode, recurse+1);
      
      if (pnext_key)
        pkey = (KEY *) ((char *) pheader + (PTYPE) pnext_key);
      } while (pnext_key);
    }

  pkey = (KEY *) ((char *) pheader + hKey);

  /* resolve links */
  if (pkey->type == TID_LINK)
    {
    db_unlock_database(hDB);
    if (*((char *) pheader + pkey->data) == '/')
      db_find_key(hDB, 0, (char *) pheader + pkey->data, &hKeyLink);
    else
      db_find_key(hDB, hKey, (char *) pheader + pkey->data, &hKeyLink);
    if (hKeyLink)
      db_set_mode(hDB, hKeyLink, mode, recurse > 0);
    db_lock_database(hDB);
    }

  /* now set mode */
  pkey->access_mode = mode;

  if (recurse < 2)
    db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_load(HNDLE hDB, HNDLE hKeyRoot, char *filename, BOOL bRemote)
/********************************************************************\

  Routine: db_load

  Purpose: Load a branch of a database from an .ODB file

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKeyRoot          Handle of key to start
    char  *filename         Filename of .ODB file
    BOOL  bRemote           Save file on remote server

  Output:
    implicit                Append file to database

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_FILE_ERROR           File not found

\********************************************************************/
{
struct stat stat_buf;
INT    hfile, size, n, i, status;
char   *buffer;

  if (rpc_is_remote() && bRemote)
    return rpc_call(RPC_DB_LOAD, hDB, hKeyRoot, filename);

  /* open file */
  hfile = open(filename, O_RDONLY | O_TEXT, 0644);
  if (hfile == -1)
    {
    cm_msg(MERROR, "db_load", "file \"%s\" not found", filename);
    return DB_FILE_ERROR;
    }

  /* allocate buffer with file size */
  fstat(hfile, &stat_buf);
  size = stat_buf.st_size;
  buffer = malloc(size+1);

  if (buffer == NULL)
    {
    cm_msg(MERROR, "db_load", "cannot allocate ODB load buffer");
    close(hfile);
    return DB_NO_MEMORY;
    }

  n = 0;

  do
    {
    i = read(hfile, buffer+n, size);
    if (i<=0)
      break;
    n += i;
    } while (TRUE);

  buffer[n] = 0;

  status = db_paste(hDB, hKeyRoot, buffer);

  close(hfile);
  free(buffer);

  return status;
}

/*------------------------------------------------------------------*/

INT db_copy(HNDLE hDB, HNDLE hKey, char *buffer, INT *buffer_size, char *path)
/********************************************************************\

  Routine: db_copy

  Purpose: Copy an ODB subtree in ASCII format to a buffer

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to start, 0 for root
    char  *buffer           Buffer
    INT   *buffer_size      Size of buffer
    char  *path             Starting path

  Output:
    INT   *buffer_size      Remaining space in buffer

  Function value:
    DB_SUCCESS              Successful completion
    DB_TRUNCATED            Buffer size too small, data truncated
    DB_NO_MEMORY            Not enough memory to allocate data buffer

\********************************************************************/
{
INT    i, j, size, status;
KEY    key;
HNDLE  hSubkey;
char   full_path[MAX_ODB_PATH], str[MAX_STRING_LENGTH];
char   *data, line[MAX_STRING_LENGTH];
BOOL   bWritten;

  strcpy(full_path, path);

  bWritten = FALSE;

  /* first enumerate this level */
  for (i=0 ; ; i++)
    {
    db_enum_link(hDB, hKey, i, &hSubkey);

    if (i==0 && !hSubkey)
      {
      /* If key has no subkeys, just write this key */
      db_get_key(hDB, hKey, &key);
      size = key.total_size;
      data = malloc(size);
      if (data == NULL)
        {
        cm_msg(MERROR, "db_copy", "cannot allocate data buffer");
        return DB_NO_MEMORY;
        }
      
      if (key.type != TID_KEY)
        {
        db_get_data(hDB, hKey, data, &size, key.type);
        if (key.num_values == 1)
          {
          sprintf(line, "%s = %s : ", key.name, tid_name[key.type]);

          db_sprintf(str, data, key.item_size, 0, key.type);
        
          if (key.type == TID_STRING || key.type == TID_LINK)
            sprintf(line+strlen(line), "[%d] ", key.item_size);

          sprintf(line+strlen(line), "%s\n", str);
          }
        else
          {
          sprintf(line, "%s = %s[%d] :\n", key.name, tid_name[key.type], 
                                               key.num_values);

          for (j=0 ; j<key.num_values ; j++)
            {
            if (key.type == TID_STRING || key.type == TID_LINK)
              sprintf(line+strlen(line), "[%d] ", key.item_size);
            else
              sprintf(line+strlen(line), "[%d] ", j);

            db_sprintf(str, data, key.item_size, j, key.type);
            sprintf(line+strlen(line), "%s\n", str);

            /* copy line to buffer */
            if ((INT) (strlen(line)+1) > *buffer_size)
              {
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
      if ((INT) (strlen(line)+1) > *buffer_size)
        {
        free(data);
        return DB_TRUNCATED;
        }

      strcpy(buffer, line);
      buffer += strlen(line);
      *buffer_size -= strlen(line);

      free(data);
      }

    if (!hSubkey)
      break;

    db_get_key(hDB, hSubkey, &key);
    size = key.total_size;
    data = malloc(size);
    if (data == NULL)
      {
      cm_msg(MERROR, "db_copy", "cannot allocate data buffer");
      return DB_NO_MEMORY;
      }

    line[0] = 0;

    if (key.type == TID_KEY)
      {
      /* new line */
      if (bWritten)
        {
        if (*buffer_size < 2)
          {
          free(data);
          return DB_TRUNCATED;
          }

        strcpy(buffer, "\n");
        buffer += 1;
        *buffer_size -= 1;
        }

      strcpy(str, full_path);
      if (str[0] && str[strlen(str)-1] != '/')
        strcat(str, "/");
      strcat(str, key.name);

      /* recurse */
      status = db_copy(hDB, hSubkey, buffer, buffer_size, str);
      if (status != DB_SUCCESS)
        {
        free(data);
        return status;
        }

      buffer += strlen(buffer);
      bWritten = FALSE;
      }
    else
      {
      db_get_data(hDB, hSubkey, data, &size, key.type);
      if (!bWritten)
        {
        if (path[0] == 0)
          sprintf(line, "[.]\n");
        else
          sprintf(line, "[%s]\n", path);
        bWritten = TRUE;
        }

      if (key.num_values == 1)
        {
        sprintf(line+strlen(line), "%s = %s : ", key.name, tid_name[key.type]);

        db_sprintf(str, data, key.item_size, 0, key.type);
        
        if (key.type == TID_STRING || key.type == TID_LINK)
          sprintf(line+strlen(line), "[%d] ", key.item_size);

        sprintf(line+strlen(line), "%s\n", str);
        }
      else
        {
        sprintf(line+strlen(line), "%s = %s[%d] :\n", key.name, tid_name[key.type], 
                                   key.num_values);

        for (j=0 ; j<key.num_values ; j++)
          {
          if (key.type == TID_STRING || key.type == TID_LINK)
            sprintf(line+strlen(line), "[%d] ", key.item_size);
          else
            sprintf(line+strlen(line), "[%d] ", j);

          db_sprintf(str, data, key.item_size, j, key.type);
          sprintf(line+strlen(line), "%s\n", str);

          /* copy line to buffer */
          if ((INT)(strlen(line)+1) > *buffer_size)
            {
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
      if ((INT) (strlen(line)+1) > *buffer_size)
        {
        free(data);
        return DB_TRUNCATED;
        }

      strcpy(buffer, line);
      buffer += strlen(line);
      *buffer_size -= strlen(line);
      }

    free(data);
    }

  if (bWritten)
    {
    if (*buffer_size < 2)
      return DB_TRUNCATED;

    strcpy(buffer, "\n");
    buffer += 1;
    *buffer_size -= 1;
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_paste(HNDLE hDB, HNDLE hKeyRoot, char *buffer)
/********************************************************************\

  Routine: db_paste

  Purpose: Copy an ODB subtree in ASCII format from a buffer

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKeyRoot          Handle of key to start, 0 for root
    char  *buffer           NULL-terminated buffer

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_TRUNCATED            Line inside buffer too long
    DB_NO_MEMORY            Not enough memory to allocate data buffer

\********************************************************************/
{
char             line[MAX_STRING_LENGTH];
char             title[MAX_STRING_LENGTH];
char             key_name[MAX_STRING_LENGTH];
char             data_str[MAX_STRING_LENGTH + 50];
char             test_str[MAX_STRING_LENGTH];
char             *pc, *pold, *data;
INT              data_size;
INT              tid, i, j, n_data, string_length, status, size;
HNDLE            hKey;
KEY              root_key;

  title[0] = 0;

  if (hKeyRoot == 0)
    db_find_key(hDB, hKeyRoot, "", &hKeyRoot);

  db_get_key(hDB, hKeyRoot, &root_key);

  /* initial data size */
  data_size = 1000;
  data = malloc(data_size);
  if (data == NULL)
    {
    cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
    return DB_NO_MEMORY;
    }

  do
    {
    if (*buffer == 0)
      break;

    for(i=0 ; *buffer != '\n' && *buffer && i<MAX_STRING_LENGTH ; i++)
      line[i] = *buffer++;

    if (i == MAX_STRING_LENGTH)
      {
      cm_msg(MERROR, "db_paste", "line too long");
      free(data);
      return DB_TRUNCATED;
      }

    line[i] = 0;
    if (*buffer == '\n')
      buffer++;

    /* check if it is a section title */
    if (line[0] == '[')
      {
      /* extract title and append '/' */
      strcpy(title, line+1);
      if (strchr(title, ']'))
        *strchr(title, ']') = 0;
      if (title[0] && title[strlen(title) - 1] != '/')
        strcat(title, "/");
      }
    else
      {
      /* valid data line if it includes '=' and no ';' */
      if (strchr(line, '=') && line[0] != ';')
        {
        /* copy type info and data */
        pc = strchr(line, '=')+1;
        while (*pc == ' ')
          pc++;
        strcpy(data_str, pc);

        /* extract key name */
        *strchr(line, '=') = 0;

        pc = &line[ strlen(line)-1 ];
        while (*pc == ' ')
          *pc-- = 0;

        key_name[0] = 0;
        if (title[0] != '.')
          strcpy(key_name, title);

        strcat(key_name, line);

        /* evaluate type info */
        strcpy(line, data_str);
        if (strchr(line, ' '))
          *strchr(line, ' ') = 0;

        n_data = 1;
        if (strchr(line, '['))
          {
          n_data = atol(strchr(line, '[')+1);
          *strchr(line, '[') = 0;
          }

        for (tid=0 ; tid<TID_LAST ; tid++)
          if (strcmp(tid_name[tid], line) == 0)
            break;

        string_length = 0;

        if (tid == TID_LAST)
          cm_msg(MERROR, "db_paste", "found unknown data type in ODB file");
        else
          {
          /* skip type info */
          pc = data_str;
          while (*pc != ' ' && *pc)
            pc++;
          while ((*pc == ' ' || *pc == ':')&& *pc)
            pc++;
          strcpy(data_str, pc);

          if (n_data > 1)
            {
            data_str[0] = 0;
            if (!*buffer)
              break;

            for(j=0 ; *buffer != '\n' && *buffer ; j++)
              data_str[j] = *buffer++;
            data_str[j] = 0;
            if (*buffer == '\n')
              buffer++;
            }

          for (i=0 ; i<n_data ; i++)
            {
            /* strip trailing \n */
            pc = &data_str[strlen(data_str)-1];
            while (*pc == '\n' || *pc == '\r')
              *pc-- = 0;

            if (tid == TID_STRING || tid == TID_LINK)
              {
              if (!string_length)
                string_length = atoi(data_str+1);
              if (string_length > MAX_STRING_LENGTH)
                {
                string_length = MAX_STRING_LENGTH;
                cm_msg(MERROR, "db_paste", "found string exceeding MAX_STRING_LENGTH");
                }

              pc = data_str + 2;
              while (*pc && *pc != ' ')
                pc++;
              while (*pc && *pc == ' ')
                pc++;

              /* limit string size */
              *(pc + string_length -1 ) = 0;

              /* increase data buffer if necessary */
              if (string_length*(i+1) >= data_size)
                {
                data_size += 1000;
                data = realloc(data, data_size);
                if (data == NULL)
                  {
                  cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
                  return DB_NO_MEMORY;
                  }
                }

              strcpy(data + string_length*i, pc);
              }
            else
              {
              pc = data_str;

              if (n_data > 1 && data_str[0] == '[')
                {
                pc = strchr(data_str, ']')+1;
                while (*pc && *pc == ' ')
                  pc++;
                }

              db_sscanf(pc, data, &size, i, tid);

              /* increase data buffer if necessary */
              if (size*(i+1) >= data_size)
                {
                data_size += 1000;
                data = realloc(data, data_size);
                if (data == NULL)
                  {
                  cm_msg(MERROR, "db_paste", "cannot allocate data buffer");
                  return DB_NO_MEMORY;
                  }
                }

              }

            if (i < n_data-1)
              {
              data_str[0] = 0;
              if (!*buffer)
                break;

              pold = buffer;

              for(j=0 ; *buffer != '\n' && *buffer ; j++)
                data_str[j] = *buffer++;
              data_str[j] = 0;
              if (*buffer == '\n')
                buffer++;

              /* test if valid data */
              if (tid != TID_STRING && tid != TID_LINK)
                {
                if (data_str[0] == 0 ||
                    (strchr(data_str, '=') && strchr(data_str, ':')))
                  buffer = pold;
                }
              }
            }

          /* skip system client entries */
          strcpy(test_str, key_name);
          test_str[15] = 0;

          if (!equal_ustring(test_str, "/System/Clients"))
            {
            if (root_key.type != TID_KEY)
              {
              /* root key is destination key */
              hKey = hKeyRoot;
              }
            else
              {
              /* create key and set value */
              if (key_name[0] == '/')
                {
                status = db_find_link(hDB, 0, key_name, &hKey);
                if (status == DB_NO_KEY)
                  {
                  db_create_key(hDB, 0, key_name, tid);
                  status = db_find_link(hDB, 0, key_name, &hKey);
                  }
                }
              else
                {
                status = db_find_link(hDB, hKeyRoot, key_name, &hKey);
                if (status == DB_NO_KEY)
                  {
                  db_create_key(hDB, hKeyRoot, key_name, tid);
                  status = db_find_link(hDB, hKeyRoot, key_name, &hKey);
                  }
                }
              }

            /* set key data if created sucessfully */
            if (hKey)
              {
              if (tid == TID_STRING || tid == TID_LINK)
                db_set_data(hDB, hKey, data, string_length*n_data, n_data, tid);
              else
                db_set_data(hDB, hKey, data, rpc_tid_size(tid)*n_data, n_data, tid);
              }
            }
          }
        }
      }
    } while (TRUE);

  free(data);
  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

void name2c(char *str)
/********************************************************************\

  Routine: name2c

  Purpose: Convert key name to C name. Internal use only.

\********************************************************************/
{
  if (*str >= '0' && *str <= '9')
    *str = '_'; 

  while (*str)
    {
    if (!(*str >= 'a' && *str <= 'z') &&
        !(*str >= 'A' && *str <= 'Z') &&
        !(*str >= '0' && *str <= '9'))
      *str = '_';
    *str = (char) tolower(*str);
    str++;
    }
}

/*------------------------------------------------------------------*/

static void db_save_tree_struct(HNDLE hDB, HNDLE hKey, int hfile, 
                                INT level)
/********************************************************************\

  Routine: db_save_tree_struct

  Purpose: Save database tree as a C structure. Gets called by
           db_save_struct(). Internal use only.

\********************************************************************/
{
INT    i, index;
KEY    key;
HNDLE  hSubkey;
char   line[MAX_ODB_PATH], str[MAX_STRING_LENGTH];

  /* first enumerate this level */
  for (index=0 ; ; index++)
    {
    db_enum_key(hDB, hKey, index, &hSubkey);

    if (!hSubkey)
      break;

    db_get_key(hDB, hSubkey, &key);

    if (key.type != TID_KEY)
      {
      for (i=0 ; i<=level ; i++)
        write(hfile, "  ", 2);

      switch (key.type)
        {
        case TID_SBYTE:
        case TID_CHAR: strcpy(line, "char"); break;
        case TID_SHORT: strcpy(line, "short"); break;
        case TID_FLOAT: strcpy(line, "float"); break;
        case TID_DOUBLE: strcpy(line, "double"); break;
        case TID_BITFIELD: strcpy(line, "unsigned char"); break;
        case TID_STRING: strcpy(line, "char"); break;
        case TID_LINK: strcpy(line, "char"); break;
        default: strcpy(line, tid_name[key.type]); break;
        }
      
      strcat(line, "                    ");
      strcpy(str, key.name);
      name2c(str); 

      if (key.num_values > 1)
        sprintf(str+strlen(str), "[%d]", key.num_values);
      if (key.type == TID_STRING || key.type == TID_LINK)
        sprintf(str+strlen(str), "[%d]", key.item_size);

      strcpy(line+10, str);
      strcat(line, ";\n");

      write(hfile, line, strlen(line));
      }
    else
      {
      /* recurse subtree */
      for (i=0 ; i<=level ; i++)
        write(hfile, "  ", 2);

      sprintf(line, "struct {\n");
      write(hfile, line, strlen(line));
      db_save_tree_struct(hDB, hSubkey, hfile, level+1);

      for (i=0 ; i<=level ; i++)
        write(hfile, "  ", 2);

      strcpy(str, key.name);
      name2c(str);

      sprintf(line, "} %s;\n", str);
      write(hfile, line, strlen(line));
      }
    }
}

/*------------------------------------------------------------------*/

INT db_save(HNDLE hDB, HNDLE hKey, char *filename, BOOL bRemote)
/********************************************************************\

  Routine: db_save

  Purpose: Save a branch of a database to an .ODB file

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to start, 0 for root
    char  *filename         Filename of .ODB file
    BOOL  bRemote           Save database at remote server

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_FILE_ERROR           Cannot write ODB file

\********************************************************************/
{
  if (rpc_is_remote() && bRemote)
    return rpc_call(RPC_DB_SAVE, hDB, hKey, filename, bRemote);

#ifdef LOCAL_ROUTINES
{
INT   hfile, size, buffer_size, n, status;
char  *buffer, path[256];

  /* open file */
  hfile = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT, 0644);
  if (hfile == -1)
    {
    cm_msg(MERROR, "db_save", "Cannot open file \"%s\"", filename);
    return DB_FILE_ERROR;
    }

  db_get_path(hDB, hKey, path, sizeof(path));

  buffer_size = 10000;
  do
    {
    buffer = malloc(buffer_size);
    if (buffer == NULL)
      {
      cm_msg(MERROR, "db_save", "cannot allocate ODB dump buffer");
      break;
      }

    size = buffer_size;
    status = db_copy(hDB, hKey, buffer, &size, path);
    if (status != DB_TRUNCATED)
      {
      n = write(hfile, buffer, buffer_size-size);
      free(buffer);
      
      if (n != buffer_size-size)
        {
        cm_msg(MERROR, "db_save", "cannot save .ODB file");
        close(hfile);
        return DB_FILE_ERROR;
        }
      break;
      }

    /* increase buffer size if truncated */
    free(buffer);
    buffer_size *= 2;
    } while (1);

  close(hfile);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_save_struct(HNDLE hDB, HNDLE hKey, char *file_name, char *struct_name, BOOL append)
/********************************************************************\

  Routine: db_save_struct

  Purpose: Save a branch of a database to a C structure .H file

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Handle of key to start, 0 for root
    char  file_name         File name to write to
    char  struct_name       Name of structure. If struct_name == NULL,
                            the name of the key is used.
    BOOL  append            If TRUE, append to end of existing file

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_FILE_ERROR           Cannot open output file

\********************************************************************/
{
KEY              key;
char             str[100], line[100];
INT              status, i, fh;

  /* open file */
  fh = open(file_name, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);

  if (fh == -1)
    {
    cm_msg(MERROR, "db_save_struct", "Cannot open file\"%s\"", file_name);
    return DB_FILE_ERROR;
    }

  status = db_get_key(hDB, hKey, &key);
  if (status != DB_SUCCESS)
    {
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
  for (i=0 ; i<(int) strlen(str) ; i++)
    str[i] = (char) toupper(str[i]);

  sprintf(line, "} %s;\n\n", str);
  write(fh, line, strlen(line));

  close(fh);

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_save_string(HNDLE hDB, HNDLE hKey, char *file_name, char *string_name, BOOL append)
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
KEY              key;
char             str[256], line[256];
INT              status, i, size, fh, buffer_size;
char             *buffer, *pc;


  /* open file */
  fh = open(file_name, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);

  if (fh == -1)
    {
    cm_msg(MERROR, "db_save_string", "Cannot open file\"%s\"", file_name);
    return DB_FILE_ERROR;
    }

  status = db_get_key(hDB, hKey, &key);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "db_save_string", "cannot find key");
    return DB_INVALID_HANDLE;
    }

  if (string_name && string_name[0])
    strcpy(str, string_name);
  else
    strcpy(str, key.name);

  name2c(str);
  for (i=0 ; i<(int) strlen(str) ; i++)
    str[i] = (char) toupper(str[i]);

  sprintf(line, "#define %s(_name) char *_name[] = {\\\n", str);
  write(fh, line, strlen(line));

  buffer_size = 10000;
  do
    {
    buffer = malloc(buffer_size);
    if (buffer == NULL)
      {
      cm_msg(MERROR, "db_save", "cannot allocate ODB dump buffer");
      break;
      }

    size = buffer_size;
    status = db_copy(hDB, hKey, buffer, &size, "");
    if (status != DB_TRUNCATED)
      break;

    /* increase buffer size if truncated */
    free(buffer);
    buffer_size *= 2;
    } while (1);


  pc = buffer;
  
  do
    {
    i = 0;
    line[i++] = '"';
    while (*pc != '\n' && *pc != 0)
      line[i++] = *pc++;
    strcpy(&line[i], "\",\\\n");
    if (i>0)
      write(fh, line, strlen(line));

    if (*pc == '\n')
      pc++;

    } while(*pc);

  sprintf(line, "NULL }\n\n");
  write(fh, line, strlen(line));

  close(fh);
  free(buffer);

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_sprintf(char* string, void *data, INT data_size, INT index, DWORD type)
/********************************************************************\

  Routine: db_sprintf

  Purpose: Convert a database value to a string according to its type

  Input:
    void  *data             Value data
    INT   index             Index for array data
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
  else switch (type)
    {
    case TID_BYTE:
      sprintf(string, "%d",  *(((BYTE *) data)+index)); break;
    case TID_SBYTE:
      sprintf(string, "%d",  *(((char *) data)+index)); break;
    case TID_CHAR:
      sprintf(string, "%c",  *(((char *) data)+index)); break;
    case TID_WORD:
      sprintf(string, "%u",  *(((WORD *) data)+index)); break;
    case TID_SHORT:
      sprintf(string, "%d",  *(((short *) data)+index)); break;
    case TID_DWORD:
      sprintf(string, "%u",  *(((DWORD *) data)+index)); break;
    case TID_INT:
      sprintf(string, "%d",  *(((INT *) data)+index)); break;
    case TID_BOOL:
      sprintf(string, "%c",  *(((BOOL *) data)+index) ? 'y' : 'n'); break;
    case TID_FLOAT:
      sprintf(string, "%g",  *(((float *) data)+index)); break;
    case TID_DOUBLE:
      sprintf(string, "%lg", *(((double *) data)+index)); break;
    case TID_BITFIELD:
      /* TBD */
      break;
    case TID_STRING:
    case TID_LINK:
      sprintf(string, "%s", ((char *) data) + data_size*index); break;
    default:
      sprintf(string, "<unknown>");
      break;
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_sprintfh(char* string, void *data, INT data_size, INT index, DWORD type)
/********************************************************************\

  Routine: db_sprintfh

  Purpose: Convert a database value to a string according to its type
           in hex format

  Input:
    void  *data             Value data
    INT   index             Index for array data
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
  else switch (type)
    {
    case TID_BYTE:
      sprintf(string, "0x%X",  *(((BYTE *) data)+index)); break;
    case TID_SBYTE:
      sprintf(string, "0x%X",  *(((char *) data)+index)); break;
    case TID_CHAR:
      sprintf(string, "%c",  *(((char *) data)+index)); break;
    case TID_WORD:
      sprintf(string, "0x%X",  *(((WORD *) data)+index)); break;
    case TID_SHORT:
      sprintf(string, "0x%hX",  *(((short *) data)+index)); break;
    case TID_DWORD:
      sprintf(string, "0x%lX",  *(((DWORD *) data)+index)); break;
    case TID_INT:
      sprintf(string, "0x%X",  *(((INT *) data)+index)); break;
    case TID_BOOL:
      sprintf(string, "%c",  *(((BOOL *) data)+index) ? 'y' : 'n'); break;
    case TID_FLOAT:
      sprintf(string, "%g",  *(((float *) data)+index)); break;
    case TID_DOUBLE:
      sprintf(string, "%lg", *(((double *) data)+index)); break;
    case TID_BITFIELD:
      /* TBD */
      break;
    case TID_STRING:
    case TID_LINK:
      sprintf(string, "%s", ((char *) data) + data_size*index); break;
    default:
      sprintf(string, "<unknown>");
      break;
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_sscanf(char *data_str, void *data, INT *data_size, INT i, DWORD tid)
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
BOOL  hex = FALSE;

  if (data_str == NULL)
    return 0;

  *data_size = rpc_tid_size(tid);
  if (strncmp(data_str, "0x", 2) == 0)
    {
    hex = TRUE;
    sscanf(data_str+2, "%lx", &value);
    }
  
  switch (tid)
    {
    case TID_BYTE:
    case TID_SBYTE:
      if (hex)
        *((char *) data+i) = (char) value;
      else
        *((char *) data+i) = (char) atoi(data_str); 
      break;
    case TID_CHAR:
      *((char *) data+i) = data_str[0]; break;
    case TID_WORD:
      if (hex)
        *((WORD *) data+i) = (WORD) value;
      else
        *((WORD *) data+i) = (WORD) atoi(data_str); 
      break;
    case TID_SHORT:
      if (hex)
        *((short int *) data+i) = (short int) value;
      else
        *((short int *) data+i) = (short int) atoi(data_str); 
      break;
    case TID_DWORD:
      if (hex)
        *((DWORD *) data+i) = value;
      else
        *((DWORD *) data+i) = atol(data_str); 
      break;
    case TID_INT:
      if (hex)
        *((INT *) data+i) = value;
      else
        *((INT *) data+i) = atol(data_str); 
      break;
    case TID_BOOL:
      if (data_str[0] == 'y' || data_str[0] == 'Y' ||
          atoi(data_str) > 0)
        *((BOOL *) data+i) = 1;
      else
        *((BOOL *) data+i) = 0;
      break;
    case TID_FLOAT:
      *((float *) data+i) = (float) atof(data_str); break;
    case TID_DOUBLE:
      *((double *) data+i) = atof(data_str); break;
    case TID_BITFIELD:
      /* TBD */
      break;
    case TID_STRING:
    case TID_LINK:
      strcpy((char *) data, data_str);
      *data_size = strlen(data_str)+1;
      break;
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef LOCAL_ROUTINES

static void db_recurse_record_tree(HNDLE hDB, HNDLE hKey, void **data, 
                                   INT *total_size, INT base_align, 
                                   INT *max_align, BOOL bSet, 
                                   INT convert_flags)
/********************************************************************\

  Routine: db_recurse_record_tree

  Purpose: Recurse a database tree and calculate its size or copy
           data. Internal use only.

\********************************************************************/
{
DATABASE_HEADER *pheader;
KEYLIST         *pkeylist;
KEY             *pkey;
INT             size, align, corr, total_size_tmp;        

  /* get first subkey of hKey */
  pheader  = _database[hDB-1].database_header;
  pkey = (KEY *) ((char *) pheader + hKey);
  pkeylist = (KEYLIST *) ((char *) pheader + pkey->data);
  if (!pkeylist->first_key)
    return;
  pkey = (KEY *) ((char *) pheader + pkeylist->first_key);

  /* first browse through this level */
  do
    {
    if (pkey->type != TID_KEY)
      {
      /* correct for alignment */
      align = 1;

      if (rpc_tid_size(pkey->type))
        align = rpc_tid_size(pkey->type) < base_align ? 
                rpc_tid_size(pkey->type) : base_align;

      if (max_align && align > *max_align)
        *max_align = align;

      corr = VALIGN(*total_size, align) - *total_size;
      *total_size += corr;
      if (data)
        *data = (void *) ((char *) (*data) + corr);

      /* calculate data size */
      size = pkey->item_size * pkey->num_values;

      if (data)
        {
        if (bSet)
          {
          /* copy data if there is write access */
          if (pkey->access_mode & MODE_WRITE)
            {
            memcpy((char *) pheader + pkey->data, *data, 
                   pkey->item_size*pkey->num_values);

            /* convert data */
            if (convert_flags)
              {
              if (pkey->num_values > 1)
                rpc_convert_data((char *) pheader + pkey->data, 
                                 pkey->type, RPC_FIXARRAY, 
                                 pkey->item_size*pkey->num_values, 
                                 convert_flags);
              else
                rpc_convert_single((char *) pheader + pkey->data, 
                                   pkey->type, 0, 
                                   convert_flags);
              }

            /* update time */
            pkey->last_written = ss_time();

            /* notify clients which have key open */
            if (pkey->notify_count)
              db_notify_clients(hDB, (PTYPE) pkey - (PTYPE) pheader, FALSE);
            }
          }
        else
          {
          /* copy key data if there is read access */
          if (pkey->access_mode & MODE_READ)
            {
            memcpy(*data, (char *) pheader + pkey->data, pkey->total_size);

            /* convert data */
            if (convert_flags)
              {
              if (pkey->num_values > 1)
                rpc_convert_data(*data, pkey->type, RPC_FIXARRAY | RPC_OUTGOING, 
                                 pkey->item_size*pkey->num_values, convert_flags);
              else
                rpc_convert_single(*data, pkey->type, RPC_OUTGOING, convert_flags);
              }
            }
          }

        *data = (char *) (*data) + size;
        }

      *total_size += size;
      }
    else
      {
      /* align new substructure according to the maximum 
         align value in this structure */
      align = 1;

      total_size_tmp = *total_size;
      db_recurse_record_tree(hDB, (PTYPE) pkey - (PTYPE) pheader, 
                             NULL, &total_size_tmp, 
                             base_align, &align, bSet, convert_flags);

      if (max_align && align > *max_align)
        *max_align = align;

      corr = VALIGN(*total_size, align) - *total_size;
      *total_size += corr;
      if (data)
        *data = (void *) ((char *) (*data) + corr);

      /* now copy subtree */
      db_recurse_record_tree(hDB, (PTYPE) pkey - (PTYPE) pheader, 
                             data, total_size, 
                             base_align, NULL, bSet, convert_flags);

      corr = VALIGN(*total_size, align) - *total_size;
      *total_size += corr;
      if (data)
        *data = (void *) ((char *) (*data) + corr);

      if (bSet && pkey->notify_count)
        db_notify_clients(hDB, (PTYPE) pkey - (PTYPE) pheader, FALSE);
      }

    if (!pkey->next_key)
      break;

    pkey = (KEY *) ((char *) pheader + pkey->next_key);
    } while (TRUE);
}

#endif /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/

INT db_get_record_size(HNDLE hDB, HNDLE hKey, INT align, INT *buf_size)
/********************************************************************\

  Routine: db_get_record_size

  Purpose: Calculates the size of a record

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle, must have subkeys
    INT    align            Byte alignment calculated by the stub and
                            passed to the rpc side to align data 
                            according to local machine. Must be zero
                            when called from user level

  Output:
    INT    *buf_size        Size of record structure

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TYPE_MISMATCH        Key has no subkeys
    DB_STRUCT_SIZE_MISMATCH *buf_size is different from calculated
                            structure size
    DB_NO_KEY               Key doesn't exist

\********************************************************************/
{
  if (rpc_is_remote())
    {
    align = ss_get_struct_align();
    return rpc_call(RPC_DB_GET_RECORD_SIZE, hDB, hKey, align, buf_size);
    }

#ifdef LOCAL_ROUTINES
{
KEY  key;
INT  status, max_align;

  if (!align)
    align = ss_get_struct_align();

  /* check if key has subkeys */
  status = db_get_key(hDB, hKey, &key);
  if (status != DB_SUCCESS)
    return status;

  if (key.type != TID_KEY)
    {
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
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_get_record(HNDLE hDB, HNDLE hKey, void *data, INT *buf_size, 
                  INT align)
/********************************************************************\

  Routine: db_get_record

  Purpose: Copy a set of keys to local memory

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle, must have subkeys
    INT    *buf_size        Size of data structure, must be obtained
                            via sizeof(RECORD-NAME)
    INT    align            Byte alignment calculated by the stub and
                            passed to the rpc side to align data 
                            according to local machine. Must be zero
                            when called from user level

  Output:
    void   *data            Pointer where data is stored

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_STRUCT_SIZE_MISMATCH *buf_size is different from calculated
                            structure size

\********************************************************************/
{
  if (rpc_is_remote())
    {
    align = ss_get_struct_align();
    return rpc_call(RPC_DB_GET_RECORD, hDB, hKey, data, buf_size, align);
    }

#ifdef LOCAL_ROUTINES
{
KEY     key;
INT     convert_flags, status;
INT     total_size;
void    *pdata;
char    str[256];

  if (!align)
    align = ss_get_struct_align();

  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);
  else
    convert_flags = 0;

  /* check if key has subkeys */
  status = db_get_key(hDB, hKey, &key);
  if (status != DB_SUCCESS)
    return status;

  if (key.type != TID_KEY)
    {
    /* copy single key */
    if (key.item_size * key.num_values != *buf_size)
      {
      cm_msg(MERROR, "db_get_record", "struct size mismatch for \"%s\"", key.name);
      return DB_STRUCT_SIZE_MISMATCH;
      }

    db_get_data(hDB, hKey, data, buf_size, key.type);

    if (convert_flags)
      {
      if (key.num_values > 1)
        rpc_convert_data(data, key.type, RPC_OUTGOING | RPC_FIXARRAY, 
                         key.item_size*key.num_values, 
                         convert_flags);
      else
        rpc_convert_single(data, key.type, RPC_OUTGOING, convert_flags);
      }
    
    return DB_SUCCESS;
    }

  db_lock_database(hDB);

  /* check record size */
  db_get_record_size(hDB, hKey, 0, &total_size);
  if (total_size != *buf_size)
    {
    db_unlock_database(hDB);

    db_get_path(hDB, hKey, str, sizeof(str));
    cm_msg(MERROR, "db_get_record", "struct size mismatch for \"%s\" (%d instead of %d)", 
           str, *buf_size, total_size);
    return DB_STRUCT_SIZE_MISMATCH;
    }

  /* get subkey data */
  pdata = data;
  total_size = 0;
  db_recurse_record_tree(hDB, hKey, &pdata, &total_size, align, 
                         NULL, FALSE, convert_flags);

  db_unlock_database(hDB);

}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_set_record(HNDLE hDB, HNDLE hKey, void *data, INT buf_size, 
                  INT align)
/********************************************************************\

  Routine: db_set_record

  Purpose: Copy a set of keys from local memory to the database

  Input:
    HNDLE  hDB              Handle to the database
    HNDLE  hKey             Key handle, must have subkeys
    void   *data            Pointer where data is stored
    INT    buf_size         Size of data structure, must be obtained
                            via sizeof(RECORD-NAME)
    INT    align            Byte alignment calculated by the stub and
                            passed to the rpc side to align data 
                            according to local machine. Must be zero
                            when called from user level

  Output:
    <none>

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_TYPE_MISMATCH        Key has no subkeys
    DB_STRUCT_SIZE_MISMATCH *buf_size is different from calculated
                            structure size

\********************************************************************/
{
  if (rpc_is_remote())
    {
    align = ss_get_struct_align();
    return rpc_call(RPC_DB_SET_RECORD, hDB, hKey, data, buf_size, align);
    }

#ifdef LOCAL_ROUTINES
{
KEY              key;
INT              convert_flags;
INT              total_size;
void             *pdata;

  if (!align)
    align = ss_get_struct_align();

  if (rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
    convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);
  else
    convert_flags = 0;

  /* check if key has subkeys */
  db_get_key(hDB, hKey, &key);
  if (key.type != TID_KEY)
    {
    /* copy single key */
    if (key.item_size * key.num_values != buf_size)
      {
      cm_msg(MERROR, "db_set_record", "struct size mismatch for \"%s\"", key.name);
      return DB_STRUCT_SIZE_MISMATCH;
      }

    if (convert_flags)
      {
      if (key.num_values > 1)
        rpc_convert_data(data, key.type, RPC_FIXARRAY, 
                         key.item_size*key.num_values, 
                         convert_flags);
      else
        rpc_convert_single(data, key.type, 0, convert_flags);
      }

    db_set_data(hDB, hKey, data, key.total_size, key.num_values, key.type);
    return DB_SUCCESS;
    }

  /* check record size */
  db_get_record_size(hDB, hKey, 0, &total_size);
  if (total_size != buf_size)
    {
    cm_msg(MERROR, "db_set_record", "struct size mismatch for \"%s\"", key.name);
    return DB_STRUCT_SIZE_MISMATCH;
    }

  /* set subkey data */
  pdata = data;
  total_size = 0;
  db_recurse_record_tree(hDB, hKey, &pdata, &total_size, align, 
                         NULL, TRUE, convert_flags);

  db_notify_clients(hDB, hKey, TRUE);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

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
KEY             *pkey;
INT             i;

  /* lock database */
  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pclient  = &pheader->client[_database[hDB-1].client_index];

  /* check if key is already open */
  for (i=0 ; i<pclient->max_index ; i++)
    if (pclient->open_record[i].handle == hKey)
      break;

  if (i<pclient->max_index)
    {
    db_unlock_database(hDB);
    return DB_SUCCESS;
    }

  /* not found, search free entry */
  for (i=0 ; i<pclient->max_index ; i++)
    if (pclient->open_record[i].handle == 0)
      break;

  /* check if maximum number reached */
  if (i == MAX_OPEN_RECORDS)
    {
    db_unlock_database(hDB);
    return DB_NO_MEMORY;
    }

  if (i == pclient->max_index)
    pclient->max_index++;

  pclient->open_record[i].handle      = hKey;
  pclient->open_record[i].access_mode = access_mode;

  /* increment notify_count */
  pkey = (KEY *) ((char *) pheader + hKey);
  pkey->notify_count++;

  pclient->num_open_records++;

  /* set exclusive bit if requested */
  if (access_mode & MODE_WRITE)
    db_set_mode(hDB, hKey, (WORD) (pkey->access_mode | MODE_EXCLUSIVE), TRUE);

  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_remove_open_record(HNDLE hDB, HNDLE hKey)
/********************************************************************\

  Routine: db_remove_open_record

  Purpose: Gets called by db_close_record. Internal use only.

\********************************************************************/
{
  if (rpc_is_remote())
    return rpc_call(RPC_DB_REMOVE_OPEN_RECORD, hDB, hKey);

#ifdef LOCAL_ROUTINES
{
DATABASE_HEADER *pheader;
DATABASE_CLIENT *pclient;
KEY             *pkey;
INT             i, index;

  /* lock database */
  db_lock_database(hDB);

  pheader  = _database[hDB-1].database_header;
  pclient  = &pheader->client[_database[hDB-1].client_index];

  /* search key */
  for (index=0 ; index<pclient->max_index ; index++)
    if (pclient->open_record[index].handle == hKey)
      break;

  if (index == pclient->max_index)
    {
    db_unlock_database(hDB);
    return DB_INVALID_HANDLE;
    }

  /* decrement notify_count */
  pkey = (KEY *) ((char *) pheader + hKey);
  if (pkey->notify_count > 0)
    pkey->notify_count--;

  pclient->num_open_records--;

  /* remove exclusive flag */
  if (pclient->open_record[index].access_mode & MODE_WRITE)
    db_set_mode(hDB, hKey, (WORD) (pkey->access_mode & ~MODE_EXCLUSIVE), TRUE);

  memset(&pclient->open_record[index], 0, sizeof(OPEN_RECORD));

  /* calculate new max_index entry */
  for (i=pclient->max_index-1 ; i>=0 ; i--)
    if (pclient->open_record[i].handle != 0)
      break;
  pclient->max_index = i+1;

  db_unlock_database(hDB);
}
#endif /* LOCAL_ROUTINES */

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
KEY             *pkey;
KEYLIST         *pkeylist;
INT             i, j;
char            str[80];

  pheader  = _database[hDB-1].database_header;

  /* check if key or parent has notify_flag set */
  pkey = (KEY *) ((char *) pheader + hKey);
  
  do
    {

    /* check which client has record open */
    if (pkey->notify_count)
      for (i=0 ; i<pheader->max_client_index ; i++)
        {
        pclient = &pheader->client[i];
        for (j=0 ; j<pclient->max_index ; j++)
          if (pclient->open_record[j].handle == hKey)
            {
            /* send notification to remote process */
            sprintf(str, "O %d %d", hDB, hKey);
            ss_resume(pclient->port, str);
            }
        }

    if (pkey->parent_keylist == 0 || !bWalk)
      return DB_SUCCESS;

    pkeylist = (KEYLIST *) ((char *) pheader + pkey->parent_keylist);
    pkey = (KEY *) ((char *) pheader + pkeylist->parent);
    hKey = (PTYPE) pkey - (PTYPE) pheader;
    } while (TRUE);

}

#endif /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/

void merge_records(HNDLE hDB, HNDLE hKey, KEY* pkey, INT level, void *info)
{
char  full_name[256], buffer[10000];
INT   status, size;
HNDLE hKeyInit;
KEY   initkey, key;

  /* compose name of init key */
  db_get_path(hDB, hKey, full_name, 256);
  *strchr(full_name, 'O') = 'I';

  /* if key in init record found, copy original data to init data */
  status = db_find_key(hDB, 0, full_name, &hKeyInit);
  if (status == DB_SUCCESS)
    {
    db_get_key(hDB, hKeyInit, &initkey);
    db_get_key(hDB, hKey, &key);

    if (initkey.type != TID_KEY && initkey.type == key.type)
      {
      /* copy data from original key to new key */
      size = sizeof(buffer);
      if (db_get_data(hDB, hKey, buffer, &size, initkey.type) == DB_SUCCESS)
        db_set_data(hDB, hKeyInit, buffer, initkey.total_size, 
				            initkey.num_values, initkey.type);
      }
    }
}

static int open_count;

void check_open_keys(HNDLE hDB, HNDLE hKey, KEY* pkey, INT level, void *info)
{
  if (pkey->notify_count)
    open_count++;
}

INT db_create_record(HNDLE hDB, HNDLE hKey, char *key_name, char *init_str)
/********************************************************************\

  Routine: db_create_record

  Purpose: Create a record. If a part of the record exists alreay,
           merge it with the init_str (use values from the init_str
           only when they are not in the existing record).

  Input:                
    HNDLE hDB               Handle to the database
    HNDLE hKey              Key handle to start with, 0 for root
    char  *key_name         Name of record
    char  *init_str         Initialization string in the format of
                            the db_copy/db_save functions.
  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_FULL                 ODB is full
    DB_NO_ACCESS            No read/write access to key
    DB_OPEN_RECORD          Key or subkey is open via hot-link

\********************************************************************/
{
char  str[256], *buffer;
INT   status, size, i, buffer_size;
HNDLE hKeyTmp, hKeyTmpO, hKeyOrig, hSubkey;

  if (rpc_is_remote())
    return rpc_call(RPC_DB_CREATE_RECORD, hDB, hKey, key_name, init_str);

  /* merge temporay record and original record */
  db_find_key(hDB, hKey, key_name, &hKeyOrig);
  if (hKeyOrig)
    {
    /* check if key or subkey is opened */
    open_count = 0;
    db_scan_tree_link(hDB, hKeyOrig, 0, check_open_keys, NULL);
    if (open_count)
      return DB_OPEN_RECORD;
    
    /* create temporary records */
    sprintf(str, "/System/Tmp/%1ldI", ss_gettid());
    db_find_key(hDB, 0, str, &hKeyTmp);
    if (hKeyTmp)
      db_delete_key(hDB, hKeyTmp, FALSE);
    db_create_key(hDB, 0, str, TID_KEY);
    status = db_find_key(hDB, 0, str, &hKeyTmp);
    if (status != DB_SUCCESS)
      return status;

    sprintf(str, "/System/Tmp/%1ldO", ss_gettid());
    db_find_key(hDB, 0, str, &hKeyTmpO);
    if (hKeyTmpO)
      db_delete_key(hDB, hKeyTmpO, FALSE);
    db_create_key(hDB, 0, str, TID_KEY);
    status = db_find_key(hDB, 0, str, &hKeyTmpO);
    if (status != DB_SUCCESS)
      return status;

    status = db_paste(hDB, hKeyTmp, init_str);
    if (status != DB_SUCCESS)
      return status;

    buffer_size = 10000;
    buffer = malloc(buffer_size);
    do
      {
      size = buffer_size;
      status = db_copy(hDB, hKeyOrig, buffer, &size, "");
      if (status == DB_TRUNCATED)
        {
        buffer_size += 10000;
        buffer = realloc(buffer, buffer_size);
        continue;
        }
      if (status != DB_SUCCESS)
        return status;
      } while (status != DB_SUCCESS);

    status = db_paste(hDB, hKeyTmpO, buffer);
    if (status != DB_SUCCESS)
      {
      free(buffer);
      return status;
      }

    /* merge temporay record and original record */
    db_scan_tree_link(hDB, hKeyTmpO, 0, merge_records, NULL);

    /* delete original record */
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hKeyOrig, 0, &hSubkey);
      if (!hSubkey)
        break;
    
      status = db_delete_key(hDB, hSubkey, FALSE);
      if (status != DB_SUCCESS)
        {
        free(buffer);
        return status;
        }
      }

    /* copy merged record to original record */
    do
      {
      size = buffer_size;
      status = db_copy(hDB, hKeyTmp, buffer, &size, "");
      if (status == DB_TRUNCATED)
        {
        buffer_size += 10000;
        buffer = realloc(buffer, buffer_size);
        continue;
        }
      if (status != DB_SUCCESS)
        {
        free(buffer);
        return status;
        }
      } while (status != DB_SUCCESS);

    status = db_paste(hDB, hKeyOrig, buffer);
    if (status != DB_SUCCESS)
      {
      free(buffer);
      return status;
      }

    /* delete temporary records */
    db_delete_key(hDB, hKeyTmp, FALSE);
    db_delete_key(hDB, hKeyTmpO, FALSE);

    free(buffer);
    }
  else
    {
    /* create fresh record */
    db_create_key(hDB, hKey, key_name, TID_KEY);
    status = db_find_key(hDB, hKey, key_name, &hKeyTmp);
    if (status != DB_SUCCESS)
      return status;

    status = db_paste(hDB, hKeyTmp, init_str);
    if (status != DB_SUCCESS)
      return status;
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_open_record(HNDLE hDB, HNDLE hKey, void *ptr, INT rec_size, 
                   WORD access_mode, void (*dispatcher)(INT,INT,void*), void *info)
/********************************************************************\

  Routine: db_open_record

  Purpose: Open a record. Create a local copy and maintain an 
           automatic update.

  Input:                
    HNDLE hDB               Handle to the database
    HNDLE hKey              Key handle
    void  *ptr              If access_mode includes MODE_ALLOC:
                              Address of pointer which points to the
                              record data after the call
                            if access_mode includes not MODE_ALLOC:
                              Address of record
                            if ptr==NULL, only the dispatcher is called
    INT   rec_size          Size of record if allocated by application
    WORD  access_mode       Mode for opening record, either MODE_READ or
                            MODE_WRITE. May be or'ed with MODE_ALLOC to
                            let db_open_record allocate the memory for
                            the record.
    void  *dispatcher()     Function which gets called when record
                            is updated.
    void  *info             Additional info passed to the dispatcher

  Output:
    void  *ptr              **ptr points to new record data

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid
    DB_NO_MEMORY            Not enough memeory
    DB_NO_ACCESS            No read/write access to key
    DB_STRUCT_SIZE_MISMATCH Structure size mismatch

\********************************************************************/
{
INT  index, status, size;
KEY  key;
void *data;
char str[256];

  /* allocate new space for the local record list */
  if (_record_list_entries == 0)
    {
    _record_list = malloc(sizeof(RECORD_LIST));
    memset(_record_list, 0, sizeof(RECORD_LIST));
    if (_record_list == NULL)
      {
      cm_msg(MERROR, "db_open_record", "not enough memory");
      return DB_NO_MEMORY;
      }

    _record_list_entries = 1;
    index = 0;
    }
  else
    {
    /* check for a deleted entry */
    for (index=0 ; index<_record_list_entries ; index++)
      if (!_record_list[index].handle)
        break;

    /* if not found, create new one */
    if (index == _record_list_entries)
      {
      _record_list = realloc(_record_list, 
                             sizeof(RECORD_LIST)*(_record_list_entries+1));
      if (_record_list == NULL)
        {
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
  if (status != DB_SUCCESS)
    {
    _record_list_entries--;
    cm_msg(MERROR, "db_open_record", "cannot get record size");
    return DB_NO_MEMORY;
    }
  if (size != rec_size && ptr != NULL)
    {
    _record_list_entries--;
    db_get_path(hDB, hKey, str, sizeof(str));
    cm_msg(MERROR, "db_open_record", "struct size mismatch for \"%s\" (%d instead of %d)", 
           str, rec_size, size);
    return DB_STRUCT_SIZE_MISMATCH;
    }

  /* check for read access */
  if (((key.access_mode & MODE_EXCLUSIVE) && (access_mode & MODE_WRITE)) ||
      (!(key.access_mode & MODE_WRITE) && (access_mode & MODE_WRITE)) ||
      (!(key.access_mode & MODE_READ) && (access_mode & MODE_READ)))
    {
    _record_list_entries--;
    return DB_NO_ACCESS;
    }

  if (access_mode & MODE_ALLOC)
    {
    data = malloc(size);

    if (data == NULL)
      {
      _record_list_entries--;
      cm_msg(MERROR, "db_open_record", "not enough memory");
      return DB_NO_MEMORY;
      }

    *((void **) ptr) = data;
    }
  else
    data = ptr;

  /* copy record to local memory */
  if (access_mode & MODE_READ && data != NULL)
    {
    status = db_get_record(hDB, hKey, data, &size, 0);
    if (status != DB_SUCCESS)
      {
      _record_list_entries--;
      cm_msg(MERROR, "db_open_record", "cannot get record");
      return DB_NO_MEMORY;
      }
    }

  /* copy local record to ODB */
  if (access_mode & MODE_WRITE)
    {
    status = db_set_record(hDB, hKey, data, size, 0);
    if (status != DB_SUCCESS)
      {
      _record_list_entries--;
      cm_msg(MERROR, "db_open_record", "cannot set record");
      return DB_NO_MEMORY;
      }

    /* init a local copy of the record */
    _record_list[index].copy = malloc(size);
    if (_record_list[index].copy == NULL)
      {
      cm_msg(MERROR, "db_open_record", "not enough memory");
      return DB_NO_MEMORY;
      }

    memcpy(_record_list[index].copy, data, size);
    }

  /* initialize record list */
  _record_list[index].handle      = hKey;
  _record_list[index].hDB         = hDB;
  _record_list[index].access_mode = access_mode;
  _record_list[index].data        = data;
  _record_list[index].buf_size    = size;
  _record_list[index].dispatcher  = dispatcher;
  _record_list[index].info        = info;

  /* add record entry in database structure */
  db_add_open_record(hDB, hKey, (WORD) (access_mode & ~MODE_ALLOC));

  return DB_SUCCESS;
}  

/*------------------------------------------------------------------*/

INT db_close_record(HNDLE hDB, HNDLE hKey)
/********************************************************************\

  Routine: db_close_record

  Purpose: Close a record previously opend with db_open_record

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Key handle

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
INT i;

  for (i=0 ; i<_record_list_entries ; i++)
    if (_record_list[i].handle == hKey &&
        _record_list[i].hDB == hDB)
      break;

  if (i == _record_list_entries)
    return DB_INVALID_HANDLE;

  /* remove record entry from database structure */
  db_remove_open_record(hDB, hKey);

  /* free local memory */
  if (_record_list[i].access_mode & MODE_ALLOC)
    free(_record_list[i].data);
 
  if (_record_list[i].access_mode & MODE_WRITE)
    free(_record_list[i].copy);

  memset(&_record_list[i], 0, sizeof(RECORD_LIST));

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_close_all_records()
/********************************************************************\

  Routine: db_close_all_records

  Purpose: Release local memory for open records. This routines is
           called by db_close_all_databases and cm_disconnect_experiment

  Input:
    none

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
INT i;

  for (i=0 ; i<_record_list_entries ; i++)
    {
    if (_record_list[i].handle)
      {
      if (_record_list[i].access_mode & MODE_WRITE)
        free(_record_list[i].copy);

      if (_record_list[i].access_mode & MODE_ALLOC)
        free(_record_list[i].data);

      memset(&_record_list[i], 0, sizeof(RECORD_LIST));
      }
    }

  if (_record_list_entries > 0)
    {
    _record_list_entries = 0;
    free(_record_list);
    }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

INT db_update_record(INT hDB, INT hKey, int socket)
/********************************************************************\

  Routine: db_update_record

  Purpose: If called locally, update a record (hDB/hKey) and copy
           its new contents to the local copy of it. If called from
           a server, send a network notification to the client.

  Input:
    HNDLE hDB               Handle to the database
    HNDLE hKey              Key handle
    int   socket            optional server socket

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion
    DB_INVALID_HANDLE       Database handle is invalid

\********************************************************************/
{
INT         i, size, convert_flags, status;
char        buffer[32];
NET_COMMAND *nc;

  /* check if we are the server */
  if (socket)
    {
    convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

    if (convert_flags & CF_ASCII)
      {
      sprintf(buffer, "MSG_ODB&%d&%d", hDB, hKey);
      send_tcp(socket, buffer, strlen(buffer)+1, 0);
      }
    else
      {
      nc = (NET_COMMAND *) buffer;

      nc->header.routine_id  = MSG_ODB;
      nc->header.param_size  = 2*sizeof(INT);
      *((INT *) nc->param)   = hDB;
      *((INT *) nc->param+1) = hKey;

      if (convert_flags)
        {
        rpc_convert_single(&nc->header.routine_id, TID_DWORD, 
                           RPC_OUTGOING, convert_flags);
        rpc_convert_single(&nc->header.param_size, TID_DWORD, 
                           RPC_OUTGOING, convert_flags);
        rpc_convert_single(&nc->param[0], TID_DWORD, 
                           RPC_OUTGOING, convert_flags);
        rpc_convert_single(&nc->param[4], TID_DWORD, 
                           RPC_OUTGOING, convert_flags);
        }

      /* send the update notification to the client */
      send_tcp(socket, buffer, sizeof(NET_COMMAND_HEADER) + 2*sizeof(INT), 0);
      }

    return DB_SUCCESS;
    }

  status = DB_INVALID_HANDLE;

  /* check all entries for matching key */
  for (i=0 ; i<_record_list_entries ; i++)
    if (_record_list[i].handle == hKey)
      {
      status = DB_SUCCESS;

      /* get updated data if record not opened in write mode */
      if ((_record_list[i].access_mode & MODE_WRITE) == 0)
        {
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

/*------------------------------------------------------------------*/

INT db_send_changed_records()
/********************************************************************\

  Routine: db_send_changed_record

  Purpose: Send all records to the ODB which were changed locally
           since the last call to this function.

  Input:
    none

  Output:
    none

  Function value:
    DB_SUCCESS              Successful completion

\********************************************************************/
{
INT         i;

  for (i=0 ; i<_record_list_entries ; i++)
    if (_record_list[i].access_mode & MODE_WRITE)
      {
      if (memcmp(_record_list[i].copy, _record_list[i].data, _record_list[i].buf_size) != 0)
        {
        /* switch to fast TCP mode temporarily */
        if (rpc_is_remote())
          rpc_set_option(-1, RPC_OTRANSPORT, RPC_FTCP);
        db_set_record(_record_list[i].hDB, 
                      _record_list[i].handle, 
                      _record_list[i].data,  
                      _record_list[i].buf_size,
                      0);
        if (rpc_is_remote())
          rpc_set_option(-1, RPC_OTRANSPORT, RPC_TCP);
        memcpy(_record_list[i].copy, _record_list[i].data, _record_list[i].buf_size);
        }
      }

  return DB_SUCCESS;
}

/*------------------------------------------------------------------*/
