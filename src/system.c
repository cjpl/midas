/********************************************************************\

  Name:         system.c
  Created by:   Stefan Ritt

  Contents:     All operating system dependent system services. This
                file containt routines which hide all system specific
                behaviour to higher levels. This is done by con-
                ditional compiling using the OS_xxx variable defined
                in MIDAS.H.

                Details about interprocess communication can be
                found in "UNIX distributed programming" by Chris
                Brown, Prentice Hall

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file system.c
The Midas System file
*/

/** @defgroup msfunctionc  System Functions (ss_xxx)
 */

/**dox***************************************************************/
/** @addtogroup msystemincludecode
 *  
 *  @{  */

/**dox***************************************************************/
/** @addtogroup msfunctionc
 *  
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS


#include "midas.h"
#include "msystem.h"

#ifdef OS_UNIX
#include <sys/mount.h>
#endif

static INT ss_in_async_routine_flag = 0;
#ifdef LOCAL_ROUTINES
#include <signal.h>

/*------------------------------------------------------------------*/
/* globals */

/* if set, don't write to *SHM file (used for Linux cluster) */
BOOL disable_shm_write = FALSE;

/*------------------------------------------------------------------*/
INT ss_set_async_flag(INT flag)
/********************************************************************\

  Routine: ss_set_async_flag

  Purpose: Sets the ss_in_async_routine_flag according to the flag
     value. This is necessary when semaphore operations under
     UNIX are called inside an asynchrounous routine (via alarm)
     because they then behave different.

  Input:
    INT  flag               May be 1 or 0

  Output:
    none

  Function value:
    INT                     Previous value of the flag

\********************************************************************/
{
   INT old_flag;

   old_flag = ss_in_async_routine_flag;
   ss_in_async_routine_flag = flag;
   return old_flag;
}

#if defined(OS_UNIX)

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#if defined(OS_DARWIN)
#include <sys/posix_shm.h>
#endif

#define MAX_MMAP 100
static void *mmap_addr[MAX_MMAP];
static int mmap_size[MAX_MMAP];

static int debug = 0;

static int use_sysv_shm = 0;
static int use_mmap_shm = 0;
static int use_posix_shm = 0;
static int use_posix1_shm = 0;
static int use_posix2_shm = 0;

#endif

static void check_shm_type(const char* shm_type)
{
#ifdef OS_UNIX
   char file_name[256];
   const int file_name_size = sizeof(file_name);
   char path[256];
   char buf[256];
   char* s;
   FILE *fp;

   cm_get_path(path);
   if (path[0] == 0) {
      getcwd(path, 256);
      strcat(path, "/");
   }

   strlcpy(file_name, path, file_name_size);
   strlcat(file_name, ".", file_name_size); /* dot file under UNIX */
   strlcat(file_name, "SHM_TYPE", file_name_size);
   strlcat(file_name, ".TXT", file_name_size);

   fp = fopen(file_name, "r");
   if (!fp) {
      fp = fopen(file_name, "w");
      if (!fp)
         cm_msg(MERROR, "ss_shm_open", "Cannot write to \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
      assert(fp != NULL);
      fprintf(fp, "%s\n", shm_type);
      fclose(fp);

      fp = fopen(file_name, "r");
      if (!fp) {
         cm_msg(MERROR, "ss_shm_open", "Cannot open \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
         return;
      }
   }

   fgets(buf, sizeof(buf), fp);
   fclose(fp);

   s = strchr(buf, '\n');
   if (s)
      *s = 0;

   //printf("shm_type [%s]\n", buf);

   if (strcmp(buf, "SYSV_SHM") == 0) {
      use_sysv_shm = 1;
      return;
   }

   if (strcmp(buf, "MMAP_SHM") == 0) {
      use_mmap_shm = 1;
      return;
   }

   if (strcmp(buf, "POSIX_SHM") == 0) {
      use_posix1_shm = 1;
      use_posix_shm = 1;
      return;
   }

   if (strcmp(buf, "POSIXv2_SHM") == 0) {
      use_posix2_shm = 1;
      use_posix_shm = 1;
      return;
   }

#if 0
   if (strcmp(buf, shm_type) == 0)
      return; // success!
#endif

   cm_msg(MERROR, "ss_shm_open", "Error: This MIDAS is built for %s while this experiment uses %s (see \'%s\')", shm_type, buf, file_name);
   exit(1);
#endif
}

static void check_shm_host()
{
   char file_name[256];
   const int file_name_size = sizeof(file_name);
   char path[256];
   char buf[256];
   char hostname[256];
   char* s;
   FILE *fp;

   gethostname(hostname, sizeof(hostname));

   //printf("hostname [%s]\n", hostname);

   cm_get_path1(path, sizeof(path));
   if (path[0] == 0) {
      getcwd(path, 256);
#if defined(OS_VMS)
#elif defined(OS_UNIX)
      strcat(path, "/");
#elif defined(OS_WINNT)
      strcat(path, "\\");
#endif
   }

   strlcpy(file_name, path, file_name_size);
#if defined (OS_UNIX)
   strlcat(file_name, ".", file_name_size); /* dot file under UNIX */
#endif
   strlcat(file_name, "SHM_HOST", file_name_size);
   strlcat(file_name, ".TXT", file_name_size);

   fp = fopen(file_name, "r");
   if (!fp) {
      fp = fopen(file_name, "w");
      if (!fp)
         cm_msg(MERROR, "ss_shm_open", "Cannot write to \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
      assert(fp != NULL);
      fprintf(fp, "%s\n", hostname);
      fclose(fp);
      return;
   }

   buf[0] = 0;

   fgets(buf, sizeof(buf), fp);
   fclose(fp);

   s = strchr(buf, '\n');
   if (s)
      *s = 0;

   if (strlen(buf) < 1)
      return; // success - provide user with a way to defeat this check

   if (strcmp(buf, hostname) == 0)
      return; // success!

   cm_msg(MERROR, "ss_shm_open", "Error: Cannot connect to MIDAS shared memory - this computer hostname is \'%s\' while \'%s\' says that MIDAS shared memory for this experiment is located on computer \'%s\'. To connect to this experiment from this computer, use the mserver. Please see the MIDAS documentation for details.", hostname, file_name, buf);
   exit(1);
}

static int ss_shm_name(const char* name, char* mem_name, int mem_name_size, char* file_name, int file_name_size, char* shm_name, int shm_name_size)
{
   char exptname[256];
   char path[256];

   check_shm_host();
#ifdef OS_UNIX
   check_shm_type("POSIXv2_SHM"); // find type of shared memory in use
#endif

   if (mem_name) {
      /*
         append a leading SM_ to the memory name to resolve name conflicts
         with mutex or semaphore names
       */
      strlcpy(mem_name, "SM_", mem_name_size);
      strlcat(mem_name, name, mem_name_size);
   }

   /* append .SHM and preceed the path for the shared memory file name */

   cm_get_experiment_name(exptname, sizeof(exptname));
   cm_get_path1(path, sizeof(path));

   //printf("shm name [%s], expt name [%s], path [%s]\n", name, exptname, path);

   assert(strlen(path) > 0);
   assert(strlen(exptname) > 0);

   if (path[0] == 0) {
      getcwd(path, 256);
#if defined(OS_VMS)
#elif defined(OS_UNIX)
      strcat(path, "/");
#elif defined(OS_WINNT)
      strcat(path, "\\");
#endif
   }

   strlcpy(file_name, path, file_name_size);
#if defined (OS_UNIX)
   strlcat(file_name, ".", file_name_size); /* dot file under UNIX */
#endif
   strlcat(file_name, name, file_name_size);
   strlcat(file_name, ".SHM", file_name_size);

   if (shm_name) {

#if defined(OS_UNIX)
      strlcpy(shm_name, "/", shm_name_size);
      if (use_posix1_shm)
         strlcat(shm_name, file_name, shm_name_size);
      else {
         strlcat(shm_name, exptname, shm_name_size);
         strlcat(shm_name, "_", shm_name_size);
         strlcat(shm_name, name, shm_name_size);
         strlcat(shm_name, "_SHM", shm_name_size);
      }

      char* s;
      for (s=shm_name+1; *s; s++)
         if (*s == '/')
            *s = '_';

#ifdef PSHMNAMLEN
      if (strlen(shm_name) >= PSHMNAMLEN)
         strlcpy(shm_name, name, shm_name_size);
#endif

      //printf("shm_name [%s]\n", shm_name);

#endif
   }

   return SS_SUCCESS;
}

#if defined OS_UNIX
static int ss_shm_file_name_to_shmid(const char* file_name, int* shmid)
{
   int key, status;

   /* create a unique key from the file name */
   key = ftok(file_name, 'M');

   /* if file doesn't exist ... */
   if (key == -1)
      return SS_NO_MEMORY;

   status = shmget(key, 0, 0);
   if (status == -1)
      return SS_NO_MEMORY;

   (*shmid) = status;
   return SS_SUCCESS;
}
#endif

/*------------------------------------------------------------------*/
INT ss_shm_open(const char *name, INT size, void **adr, HNDLE * handle, BOOL get_size)
/********************************************************************\

  Routine: ss_shm_open

  Purpose: Create a shared memory region which can be seen by several
     processes which know the name.

  Input:
    char *name              Name of the shared memory
    INT  size               Initial size of the shared memory in bytes
                            if .SHM file doesn't exist
    BOOL get_size           If TRUE and shared memory already exists, overwrite
                            "size" parameter with existing memory size

  Output:
    void  *adr              Address of opened shared memory
    HNDLE handle            Handle or key to the shared memory

  Function value:
    SS_SUCCESS              Successful completion
    SS_CREATED              Shared memory was created
    SS_FILE_ERROR           Paging file cannot be created
    SS_NO_MEMORY            Not enough memory
    SS_SIZE_MISMATCH        "size" differs from existing size and
                            get_size is FALSE
\********************************************************************/
{
   INT status;
   char mem_name[256];
   char file_name[256];
   char shm_name[256];

   ss_shm_name(name, mem_name, sizeof(mem_name), file_name, sizeof(file_name), shm_name, sizeof(shm_name));

#ifdef OS_WINNT

   status = SS_SUCCESS;

   {
      HANDLE hFile, hMap;
      char str[256], path[256], *p;
      DWORD file_size;

      /* make the memory name unique using the pathname. This is necessary
         because NT doesn't use ftok. So if different experiments are
         running in different directories, they should not see the same
         shared memory */
      cm_get_path(path);
      strcpy(str, path);

      /* replace special chars by '*' */
      while (strpbrk(str, "\\: "))
         *strpbrk(str, "\\: ") = '*';
      strcat(str, mem_name);

      /* convert to uppercase */
      p = str;
      while (*p)
         *p++ = (char) toupper(*p);

      hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, str);
      if (hMap == 0) {
         hFile = CreateFile(file_name, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
         if (!hFile) {
            cm_msg(MERROR, "ss_shm_open", "CreateFile() failed");
            return SS_FILE_ERROR;
         }

         file_size = GetFileSize(hFile, NULL);
         if (get_size) {
            if (file_size != 0xFFFFFFFF && file_size > 0)
               size = file_size;
         } else {
            if (file_size != 0xFFFFFFFF && file_size > 0 && file_size != size) {
               cm_msg(MERROR, "ss_shm_open", "Requested size (%d) differs from existing size (%d)", size, file_size);
               return SS_SIZE_MISMATCH;
            }
         }

         hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, size, str);

         if (!hMap) {
            status = GetLastError();
            cm_msg(MERROR, "ss_shm_open", "CreateFileMapping() failed, error %d", status);
            return SS_FILE_ERROR;
         }

         CloseHandle(hFile);
         status = SS_CREATED;
      }

      *adr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      *handle = (HNDLE) hMap;

      if (adr == NULL) {
         cm_msg(MERROR, "ss_shm_open", "MapViewOfFile() failed");
         return SS_NO_MEMORY;
      }

      return status;
   }

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   status = SS_SUCCESS;

   {
      int addr[2];
      $DESCRIPTOR(memname_dsc, "dummy");
      $DESCRIPTOR(filename_dsc, "dummy");
      memname_dsc.dsc$w_length = strlen(mem_name);
      memname_dsc.dsc$a_pointer = mem_name;
      filename_dsc.dsc$w_length = strlen(file_name);
      filename_dsc.dsc$a_pointer = file_name;

      addr[0] = size;
      addr[1] = 0;

      status = ppl$create_shared_memory(&memname_dsc, addr, &PPL$M_NOUNI, &filename_dsc);

      if (status == PPL$_CREATED)
         status = SS_CREATED;
      else if (status != PPL$_NORMAL)
         status = SS_FILE_ERROR;

      *adr = (void *) addr[1];
      *handle = 0;              /* not used under VMS */

      if (adr == NULL)
         return SS_NO_MEMORY;

      return status;
   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   if (use_sysv_shm) {
      
      int key, shmid, fh, file_size;
      struct shmid_ds buf;
      
      status = SS_SUCCESS;
      
      /* create a unique key from the file name */
      key = ftok(file_name, 'M');
      
      /* if file doesn't exist, create it */
      if (key == -1) {
         fh = open(file_name, O_CREAT | O_TRUNC | O_BINARY | O_RDWR, 0644);
         if (fh > 0) {
            ftruncate(fh, size);
            close(fh);
         }
         key = ftok(file_name, 'M');
         
         if (key == -1) {
            cm_msg(MERROR, "ss_shm_open", "ftok() failed");
            return SS_FILE_ERROR;
         }
         
         status = SS_CREATED;
         
         /* delete any previously created memory */
         
         shmid = shmget(key, 0, 0);
         shmctl(shmid, IPC_RMID, &buf);
      } else {
         /* if file exists, retrieve its size */
         file_size = (INT) ss_file_size(file_name);
         if (get_size) {
            size = file_size;
         } else if (size != file_size) {
            cm_msg(MERROR, "ss_shm_open", "Existing file \'%s\' has size %d, different from requested size %d",
                   file_name, file_size, size);
            return SS_SIZE_MISMATCH;
         }
      }
      
      /* get the shared memory, create if not existing */
      shmid = shmget(key, size, 0);
      if (shmid == -1) {
         //cm_msg(MINFO, "ss_shm_open", "Creating shared memory segment, key: 0x%x, size: %d",key,size);
         shmid = shmget(key, size, IPC_CREAT | IPC_EXCL);
         if (shmid == -1 && errno == EEXIST) {
            cm_msg(MERROR, "ss_shm_open",
                   "Shared memory segment with key 0x%x already exists, please remove it manually: ipcrm -M 0x%x",
                   key, key);
            return SS_NO_MEMORY;
         }
         status = SS_CREATED;
      }
      
      if (shmid == -1) {
         cm_msg(MERROR, "ss_shm_open", "shmget(key=0x%x,size=%d) failed, errno %d (%s)",
                key, size, errno, strerror(errno));
         return SS_NO_MEMORY;
      }
      
      memset(&buf, 0, sizeof(buf));
      buf.shm_perm.uid = getuid();
      buf.shm_perm.gid = getgid();
      buf.shm_perm.mode = 0666;
      shmctl(shmid, IPC_SET, &buf);
      
      *adr = shmat(shmid, 0, 0);
      *handle = (HNDLE) shmid;
      
      if ((*adr) == (void *) (-1)) {
         cm_msg(MERROR, "ss_shm_open", "shmat(shmid=%d) failed, errno %d (%s)", shmid, errno, strerror(errno));
         return SS_NO_MEMORY;
      }
      
      /* if shared memory was created, try to load it from file */
      if (status == SS_CREATED) {
         fh = open(file_name, O_RDONLY, 0644);
         if (fh == -1)
            fh = open(file_name, O_CREAT | O_RDWR, 0644);
         else
            read(fh, *adr, size);
         close(fh);
      }
      
      return status;
   }
   
   if (use_mmap_shm) {
      
      int ret;
      int fh, file_size;
      int i;
      
      if (1) {
         static int once = 1;
         if (once && strstr(file_name, "ODB")) {
            once = 0;
            cm_msg(MINFO, "ss_shm_open",
                   "WARNING: This version of MIDAS system.c uses the experimental mmap() based implementation of MIDAS shared memory.");
         }
      }
      
      status = SS_SUCCESS;
      
      fh = open(file_name, O_RDWR | O_BINARY | O_LARGEFILE, 0644);
      
      if (fh < 0) {
         if (errno == ENOENT) { // file does not exist
            fh = open(file_name, O_CREAT | O_RDWR | O_BINARY | O_LARGEFILE, 0644);
         }
         
         if (fh < 0) {
            cm_msg(MERROR, "ss_shm_open",
                   "Cannot create shared memory file \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
            return SS_FILE_ERROR;
         }
         
         ret = lseek(fh, size - 1, SEEK_SET);
         
         if (ret == (off_t) - 1) {
            cm_msg(MERROR, "ss_shm_open",
                   "Cannot create shared memory file \'%s\', size %d, lseek() errno %d (%s)",
                   file_name, size, errno, strerror(errno));
            return SS_FILE_ERROR;
         }
         
         ret = 0;
         ret = write(fh, &ret, 1);
         assert(ret == 1);
         
         ret = lseek(fh, 0, SEEK_SET);
         assert(ret == 0);
         
         //cm_msg(MINFO, "ss_shm_open", "Created shared memory file \'%s\', size %d", file_name, size);
         
         status = SS_CREATED;
      }
      
      /* if file exists, retrieve its size */
      file_size = (INT) ss_file_size(file_name);
      if (file_size < size) {
         cm_msg(MERROR, "ss_shm_open",
                "Shared memory file \'%s\' size %d is smaller than requested size %d. Please remove it and try again",
                file_name, file_size, size);
         return SS_NO_MEMORY;
      }
      
      size = file_size;
      
      *adr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fh, 0);
      
      if ((*adr) == MAP_FAILED) {
         cm_msg(MERROR, "ss_shm_open", "mmap() failed, errno %d (%s)", errno, strerror(errno));
         return SS_NO_MEMORY;
      }
      
      *handle = -1;
      for (i = 0; i < MAX_MMAP; i++)
         if (mmap_addr[i] == NULL) {
            mmap_addr[i] = *adr;
            mmap_size[i] = size;
            *handle = i;
            break;
         }
      assert((*handle) >= 0);
      
      return status;
   }
   
   if (use_posix_shm) {
      
      int fh, sh, file_size;
      int created = 0;
      int i;
      
      status = SS_SUCCESS;
      
      fh = open(file_name, O_RDWR | O_BINARY | O_LARGEFILE, 0644);
      
      if (fh < 0) {
         // if cannot open file, try to create it
         fh = open(file_name, O_CREAT | O_RDWR | O_BINARY | O_LARGEFILE, 0644);
         
         if (fh < 0) {
            cm_msg(MERROR, "ss_shm_open", "Cannot create shared memory file \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
            return SS_FILE_ERROR;
         }
         
         status = ftruncate(fh, size);
         if (status < 0) {
            cm_msg(MERROR, "ss_shm_open", "Cannot resize shared memory file \'%s\', size %d, errno %d (%s)", file_name, size, errno, strerror(errno));
            close(fh);
            return SS_FILE_ERROR;
         }
         
         //cm_msg(MINFO, "ss_shm_open", "Created shared memory file \'%s\', size %d", file_name, size);
         
         /* delete shared memory segment containing now stale data */
         ss_shm_delete(name);
         
         status = SS_CREATED;
      }
      
      /* if file exists, retrieve its size */
      file_size = (INT) ss_file_size(file_name);
      if (file_size < size) {
         cm_msg(MERROR, "ss_shm_open", "Shared memory file \'%s\' size %d is smaller than requested size %d. Please backup and remove this file and try again", file_name, file_size, size);
         return SS_NO_MEMORY;
      }
      
      size = file_size;

      sh = shm_open(shm_name, O_RDWR, 0777);
      
      if (sh < 0) {
         // cannot open, try to create new one
         
         sh = shm_open(shm_name, O_RDWR | O_CREAT, 0777);
         
         if (sh < 0) {
            cm_msg(MERROR, "ss_shm_open", "Cannot create shared memory segment \'%s\', shm_open() errno %d (%s)", shm_name, errno, strerror(errno));
            return SS_NO_MEMORY;
         }
         
         status = ftruncate(sh, size);
         if (status < 0) {
            cm_msg(MERROR, "ss_shm_open", "Cannot resize shared memory segment \'%s\', ftruncate(%d) errno %d (%s)", shm_name, size, errno, strerror(errno));
            return SS_NO_MEMORY;
         }
         
         //cm_msg(MINFO, "ss_shm_open", "Created shared memory segment \'%s\', size %d", shm_name, size);
         
         created = 1;
      }
      
      *adr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, sh, 0);
      
      if ((*adr) == MAP_FAILED) {
         cm_msg(MERROR, "ss_shm_open", "Cannot mmap() shared memory \'%s\', errno %d (%s)", shm_name, errno, strerror(errno));
         close(fh);
         close(sh);
         return SS_NO_MEMORY;
      }
      
      close(sh);
      
      /* if shared memory was created, try to load it from file */
      if (created) {
         if (debug)
            printf("ss_shm_open(%s), loading contents of %s, size %d\n", name, file_name, size);
         
         status = read(fh, *adr, size);
         if (status != size) {
            cm_msg(MERROR, "ss_shm_open", "Cannot read \'%s\', read() returned %d instead of %d, errno %d (%s)", file_name, status, size, errno, strerror(errno));
            close(fh);
            return SS_NO_MEMORY;
         }
      }
      
      close(fh);
      
      *handle = -1;
      for (i = 0; i < MAX_MMAP; i++)
         if (mmap_addr[i] == NULL) {
            mmap_addr[i] = *adr;
            mmap_size[i] = size;
            *handle = i;
            break;
         }
      assert((*handle) >= 0);
      
      if (created)
         return SS_CREATED;
      else
         return SS_SUCCESS;
   }

#endif /* OS_UNIX */

   return SS_FILE_ERROR;
}

/*------------------------------------------------------------------*/
INT ss_shm_close(const char *name, void *adr, HNDLE handle, INT destroy_flag)
/********************************************************************\

  Routine: ss_shm_close

  Purpose: Close a shared memory region.

  Input:
    char *name              Name of the shared memory
    void *adr               Base address of shared memory
    HNDLE handle            Handle of shared memeory
    BOOL destroy            Shared memory has to be destroyd and
          flushed to the mapping file.

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_INVALID_ADDRESS      Invalid base address
    SS_FILE_ERROR           Cannot write shared memory file
    SS_INVALID_HANDLE       Invalid shared memory handle

\********************************************************************/
{
   char mem_name[256], file_name[256], path[256];

   /*
      append a leading SM_ to the memory name to resolve name conflicts
      with mutex or semaphore names
    */
   sprintf(mem_name, "SM_%s", name);

   /* append .SHM and preceed the path for the shared memory file name */
   cm_get_path(path);
   if (path[0] == 0) {
      getcwd(path, 256);
#if defined(OS_VMS)
#elif defined(OS_UNIX)
      strcat(path, "/");
#elif defined(OS_WINNT)
      strcat(path, "\\");
#endif
   }

   strcpy(file_name, path);
#if defined (OS_UNIX)
   strcat(file_name, ".");      /* dot file under UNIX */
#endif
   strcat(file_name, name);
   strcat(file_name, ".SHM");

#ifdef OS_WINNT

   if (!UnmapViewOfFile(adr))
      return SS_INVALID_ADDRESS;

   CloseHandle((HANDLE) handle);

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS
/* outcommented because ppl$delete... makes privilege violation
  {
  int addr[2], flags, status;
  char mem_name[100];
  $DESCRIPTOR(memname_dsc, mem_name);

  strcpy(mem_name, "SM_");
  strcat(mem_name, name);
  memname_dsc.dsc$w_length = strlen(mem_name);

  flags = PPL$M_FLUSH | PPL$M_NOUNI;

  addr[0] = 0;
  addr[1] = adr;

  status = ppl$delete_shared_memory( &memname_dsc, addr, &flags);

  if (status == PPL$_NORMAL)
    return SS_SUCCESS;

  return SS_INVALID_ADDRESS;
  }
*/
   return SS_INVALID_ADDRESS;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      struct shmid_ds buf;
      FILE *fh;
      int i;

      i = destroy_flag;         /* avoid compiler warning */

      /* get info about shared memory */
      memset(&buf, 0, sizeof(buf));
      if (shmctl(handle, IPC_STAT, &buf) < 0) {
         cm_msg(MERROR, "ss_shm_close", "shmctl(shmid=%d,IPC_STAT) failed, errno %d (%s)",
                handle, errno, strerror(errno));
         return SS_INVALID_HANDLE;
      }

      /* copy to file and destroy if we are the last one */
      if (buf.shm_nattch == 1) {
         if (!disable_shm_write) {
            fh = fopen(file_name, "w");

            if (fh == NULL) {
               cm_msg(MERROR, "ss_shm_close", "Cannot write to file %s, please check protection", file_name);
            } else {
               /* write shared memory to file */
               fwrite(adr, 1, buf.shm_segsz, fh);
               fclose(fh);
            }
         }

         if (shmdt(adr) < 0) {
            cm_msg(MERROR, "ss_shm_close", "shmdt(shmid=%d) failed, errno %d (%s)", handle, errno, strerror(errno));
            return SS_INVALID_ADDRESS;
         }

         if (shmctl(handle, IPC_RMID, &buf) < 0) {
            cm_msg(MERROR, "ss_shm_close",
                   "shmctl(shmid=%d,IPC_RMID) failed, errno %d (%s)", handle, errno, strerror(errno));
            return SS_INVALID_ADDRESS;
         }
      } else
         /* only detach if we are not the last */
      if (shmdt(adr) < 0) {
         cm_msg(MERROR, "ss_shm_close", "shmdt(shmid=%d) failed, errno %d (%s)", handle, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }

      return SS_SUCCESS;
   }

   if (use_mmap_shm || use_posix_shm) {
      int status;

      if (debug)
         printf("ss_shm_close(%s)\n", name);

      assert(handle>=0 && handle<MAX_MMAP);
      assert(adr == mmap_addr[handle]);

      if (destroy_flag) {
         status = ss_shm_flush(name, mmap_addr[handle], mmap_size[handle], handle);
         if (status != SS_SUCCESS)
            return status;
      }

      status = munmap(mmap_addr[handle], mmap_size[handle]);
      if (status != 0) {
         cm_msg(MERROR, "ss_shm_close", "Cannot unmap shared memory \'%s\', munmap() errno %d (%s)", name, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }

      mmap_addr[handle] = NULL;
      mmap_size[handle] = 0;

      return SS_SUCCESS;
   }
#endif /* OS_UNIX */

   return SS_FILE_ERROR;
}

/*------------------------------------------------------------------*/
INT ss_shm_delete(const char *name)
/********************************************************************\

  Routine: ss_shm_delete

  Purpose: Delete shared memory segment from memory.

  Input:
    char *name              Name of the shared memory

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MEMORY            Shared memory segment does not exist

\********************************************************************/
{
   int status;
   char file_name[256];
   char shm_name[256];

   status = ss_shm_name(name, NULL, 0, file_name, sizeof(file_name), shm_name, sizeof(shm_name));

#ifdef OS_WINNT
   /* no shared memory segments to delete */
   return SS_SUCCESS;
#endif                          /* OS_WINNT */

#ifdef OS_VMS
   assert(!"not implemented!");
   return SS_NO_MEMORY;
#endif                          /* OS_VMS */

#ifdef OS_UNIX

   if (use_sysv_shm) {
      int shmid;
      struct shmid_ds buf;
      
      status = ss_shm_file_name_to_shmid(file_name, &shmid);
      
      if (status != SS_SUCCESS)
         return status;
      
      status = shmctl(shmid, IPC_RMID, &buf);
      
      if (status == -1) {
         cm_msg(MERROR, "ss_shm_delete", "Cannot delete shared memory \'%s\', shmctl(IPC_RMID) failed, errno %d (%s)", name, errno, strerror(errno));
         return SS_FILE_ERROR;
      }
      
      return SS_SUCCESS;
   }
   
   if (use_mmap_shm) {
      /* no shared memory segments to delete */
      return SS_SUCCESS;
   }

   if (use_posix_shm) {

      //printf("ss_shm_delete(%s) shm_name %s\n", name, shm_name);
      status = shm_unlink(shm_name);
      if (status < 0) {
         cm_msg(MERROR, "ss_shm_delete", "shm_unlink(%s) errno %d (%s)", shm_name, errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      return SS_SUCCESS;
   }

#endif /* OS_UNIX */

   return SS_FILE_ERROR;
}

/*------------------------------------------------------------------*/
INT ss_shm_protect(HNDLE handle, void *adr)
/********************************************************************\

  Routine: ss_shm_protect

  Purpose: Protect a shared memory region, disallow read and write
           access to it by this process

  Input:
    HNDLE handle            Handle of shared memeory
    void  *adr              Address of shared memory

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_INVALID_ADDRESS      Invalid base address

\********************************************************************/
{
#ifdef OS_WINNT

   if (!UnmapViewOfFile(adr))
      return SS_INVALID_ADDRESS;

#endif                          /* OS_WINNT */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      int i;
      i = handle;                  /* avoid compiler warning */
      
      if (shmdt(adr) < 0) {
         cm_msg(MERROR, "ss_shm_protect", "shmdt() failed");
         return SS_INVALID_ADDRESS;
      }
   }

   if (use_mmap_shm || use_posix_shm) {

      int ret;

      assert(handle>=0 && handle<MAX_MMAP);
      assert(adr == mmap_addr[handle]);
      
      ret = mprotect(mmap_addr[handle], mmap_size[handle], PROT_NONE);
      if (ret != 0) {
         cm_msg(MERROR, "ss_shm_protect",
                "Cannot mprotect(): return value %d, errno %d (%s)", ret, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }
   }

#endif // OS_UNIX

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_shm_unprotect(HNDLE handle, void **adr)
/********************************************************************\

  Routine: ss_shm_unprotect

  Purpose: Unprotect a shared memory region so that it can be accessed
           by this process

  Input:
    HNDLE handle            Handle or key to the shared memory, must
                            be obtained with ss_shm_open

  Output:
    void  *adr              Address of opened shared memory

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MEMORY            Memory mapping failed

\********************************************************************/
{
#ifdef OS_WINNT

   *adr = MapViewOfFile((HANDLE) handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

   if (*adr == NULL) {
      cm_msg(MERROR, "ss_shm_unprotect", "MapViewOfFile() failed");
      return SS_NO_MEMORY;
   }
#endif                          /* OS_WINNT */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      *adr = shmat(handle, 0, 0);
      
      if ((*adr) == (void *) (-1)) {
         cm_msg(MERROR, "ss_shm_unprotect", "shmat() failed, errno = %d", errno);
         return SS_NO_MEMORY;
      }
   }

   if (use_mmap_shm || use_posix_shm) {
      int ret;

      assert(adr == mmap_addr[handle]);

      ret = mprotect(mmap_addr[handle], mmap_size[handle], PROT_READ | PROT_WRITE);
      if (ret != 0) {
         cm_msg(MERROR, "ss_shm_unprotect",
                "Cannot mprotect(): return value %d, errno %d (%s)", ret, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }
   }

#endif // OS_UNIX

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_shm_flush(const char *name, const void *adr, INT size, HNDLE handle)
/********************************************************************\

  Routine: ss_shm_flush

  Purpose: Flush a shared memory region to its disk file.

  Input:
    char *name              Name of the shared memory
    void *adr               Base address of shared memory
    INT  size               Size of shared memeory
    HNDLE handle            Handle of shared memory

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_INVALID_ADDRESS      Invalid base address

\********************************************************************/
{
   char file_name[256];

   ss_shm_name(name, NULL, 0, file_name, sizeof(file_name), NULL, 0);

#ifdef OS_WINNT

   if (!FlushViewOfFile(adr, size))
      return SS_INVALID_ADDRESS;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return SS_SUCCESS;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   if ((use_sysv_shm || use_posix_shm) && !disable_shm_write) {

      int fd;
      int ret;

      if (use_posix_shm) {
         if (debug)
            printf("ss_shm_flush(%s) size %d\n", file_name, size);
         
         assert(handle>=0 && handle<MAX_MMAP);
         assert(adr == mmap_addr[handle]);
         assert(size == mmap_size[handle]);
      }

      fd = open(file_name, O_RDWR | O_CREAT, 0777);
      if (fd < 0) {
         cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', fopen() errno %d (%s)", file_name, errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      /* write shared memory to file */
      ret = write(fd, adr, size);
      if (ret != size) {
         cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', write() returned %d instead of %d, errno %d (%s)", file_name, ret, size, errno, strerror(errno));
         close(fd);
         return SS_NO_MEMORY;
      }

      ret = close(fd);
      if (ret < 0) {
         cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', close() errno %d (%s)", file_name, errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      return SS_SUCCESS;
   }

   if (use_mmap_shm) {
      int ret = msync((void *)adr, size, MS_ASYNC);
      if (ret != 0) {
         cm_msg(MERROR, "ss_shm_flush", "Cannot msync(): return value %d, errno %d (%s)", ret, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }
      return SS_SUCCESS;
   }


#endif // OS_UNIX

   return SS_SUCCESS;
}

#endif                          /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/
struct {
   char c;
   double d;
} test_align;

INT ss_get_struct_align()
/********************************************************************\

  Routine: ss_get_struct_align

  Purpose: Returns compiler alignment of structures. In C, structures
     can be byte aligned, word or even quadword aligned. This
     can usually be set with compiler switches. This routine
     tests this alignment during runtime and returns 1 for
     byte alignment, 2 for word alignment, 4 for dword alignment
     and 8 for quadword alignment.

  Input:
    <none>

  Output:
    <none>

  Function value:
    INT    Structure alignment

\********************************************************************/
{
   return (POINTER_T) (&test_align.d) - (POINTER_T) & test_align.c;
}

/*------------------------------------------------------------------*/
INT ss_getpid(void)
/********************************************************************\

  Routine: ss_getpid

  Purpose: Return process ID of current process

  Input:
    none

  Output:
    none

  Function value:
    INT              Process ID

\********************************************************************/
{
#ifdef OS_WINNT

   return (int) GetCurrentProcessId();

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return getpid();

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   return getpid();

#endif                          /* OS_UNIX */
#ifdef OS_VXWORKS

   return 0;

#endif                          /* OS_VXWORKS */
#ifdef OS_MSDOS

   return 0;

#endif                          /* OS_MSDOS */
}

/*------------------------------------------------------------------*/

static BOOL _single_thread = FALSE;

void ss_force_single_thread()
{
   _single_thread = TRUE;
}

INT ss_gettid(void)
/********************************************************************\

  Routine: ss_ggettid

  Purpose: Return thread ID of current thread

  Input:
    none

  Output:
    none

  Function value:
    INT              thread ID

\********************************************************************/
{
   /* if forced to single thread mode, simply return fake TID */
   if (_single_thread)
      return 1;

#if defined OS_MSDOS

   return 0;

#elif defined OS_WINNT

   return (int) GetCurrentThreadId();

#elif defined OS_VMS

   return ss_getpid();

#elif defined OS_DARWIN

   return (int)(long int)pthread_self();

#elif defined OS_CYGWIN

   return pthread_self();

#elif defined OS_UNIX

   return syscall(SYS_gettid);

#elif defined OS_VXWORKS

   return ss_getpid();

#else
#error Do not know how to do ss_gettid()
#endif
}

/*------------------------------------------------------------------*/

#ifdef OS_UNIX
void catch_sigchld(int signo)
{
   int status;

   status = signo;              /* avoid compiler warning */
   wait(&status);
   return;
}
#endif

INT ss_spawnv(INT mode, char *cmdname, char *argv[])
/********************************************************************\

  Routine: ss_spawnv

  Purpose: Spawn a subprocess or detached process

  Input:
    INT mode         One of the following modes:
           P_WAIT     Wait for the subprocess to compl.
           P_NOWAIT   Don't wait for subprocess to compl.
           P_DETACH   Create detached process.
    char cmdname     Program name to execute
    char *argv[]     Optional program arguments

  Output:
    none

  Function value:
    SS_SUCCESS       Successful completeion
    SS_INVALID_NAME  Command could not be executed;

\********************************************************************/
{
#ifdef OS_WINNT

   if (spawnvp(mode, cmdname, argv) < 0)
      return SS_INVALID_NAME;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */

#ifdef OS_MSDOS

   spawnvp((int) mode, cmdname, argv);

   return SS_SUCCESS;

#endif                          /* OS_MSDOS */

#ifdef OS_VMS

   {
      char cmdstring[500], *pc;
      INT i, flags, status;
      va_list argptr;

      $DESCRIPTOR(cmdstring_dsc, "dummy");

      if (mode & P_DETACH) {
         cmdstring_dsc.dsc$w_length = strlen(cmdstring);
         cmdstring_dsc.dsc$a_pointer = cmdstring;

         status = sys$creprc(0, &cmdstring_dsc, 0, 0, 0, 0, 0, NULL, 4, 0, 0, PRC$M_DETACH);
      } else {
         flags = (mode & P_NOWAIT) ? 1 : 0;

         for (pc = argv[0] + strlen(argv[0]); *pc != ']' && pc != argv[0]; pc--);
         if (*pc == ']')
            pc++;

         strcpy(cmdstring, pc);

         if (strchr(cmdstring, ';'))
            *strchr(cmdstring, ';') = 0;

         strcat(cmdstring, " ");

         for (i = 1; argv[i] != NULL; i++) {
            strcat(cmdstring, argv[i]);
            strcat(cmdstring, " ");
         }

         cmdstring_dsc.dsc$w_length = strlen(cmdstring);
         cmdstring_dsc.dsc$a_pointer = cmdstring;

         status = lib$spawn(&cmdstring_dsc, 0, 0, &flags, NULL, 0, 0, 0, 0, 0, 0, 0, 0);
      }

      return BM_SUCCESS;
   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX
   pid_t child_pid;

#ifdef OS_ULTRIX
   union wait *status;
#else
   int status;
#endif

   if ((child_pid = fork()) < 0)
      return (-1);

   if (child_pid == 0) {
      /* now we are in the child process ... */
      child_pid = execvp(cmdname, argv);
      return SS_SUCCESS;
   } else {
      /* still in parent process */
      if (mode == P_WAIT)
#ifdef OS_ULTRIX
         waitpid(child_pid, status, WNOHANG);
#else
         waitpid(child_pid, &status, WNOHANG);
#endif

      else
         /* catch SIGCHLD signal to avoid <defunc> processes */
         signal(SIGCHLD, catch_sigchld);
   }

   return SS_SUCCESS;

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
INT ss_shell(int sock)
/********************************************************************\

  Routine: ss_shell

  Purpose: Execute shell via socket (like telnetd)

  Input:
    int  sock        Socket

  Output:
    none

  Function value:
    SS_SUCCESS       Successful completeion

\********************************************************************/
{
#ifdef OS_WINNT

   HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup,
       hChildStdoutRd, hChildStdoutWr, hChildStderrRd, hChildStderrWr, hSaveStdin, hSaveStdout, hSaveStderr;

   SECURITY_ATTRIBUTES saAttr;
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   char buffer[256], cmd[256];
   DWORD dwRead, dwWritten, dwAvail, i, i_cmd;
   fd_set readfds;
   struct timeval timeout;

   /* Set the bInheritHandle flag so pipe handles are inherited. */
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   /* Save the handle to the current STDOUT. */
   hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE);

   /* Create a pipe for the child's STDOUT. */
   if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
      return 0;

   /* Set a write handle to the pipe to be STDOUT. */
   if (!SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr))
      return 0;


   /* Save the handle to the current STDERR. */
   hSaveStderr = GetStdHandle(STD_ERROR_HANDLE);

   /* Create a pipe for the child's STDERR. */
   if (!CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0))
      return 0;

   /* Set a read handle to the pipe to be STDERR. */
   if (!SetStdHandle(STD_ERROR_HANDLE, hChildStderrWr))
      return 0;


   /* Save the handle to the current STDIN. */
   hSaveStdin = GetStdHandle(STD_INPUT_HANDLE);

   /* Create a pipe for the child's STDIN. */
   if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
      return 0;

   /* Set a read handle to the pipe to be STDIN. */
   if (!SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd))
      return 0;

   /* Duplicate the write handle to the pipe so it is not inherited. */
   if (!DuplicateHandle(GetCurrentProcess(), hChildStdinWr, GetCurrentProcess(), &hChildStdinWrDup, 0, FALSE,   /* not inherited */
                        DUPLICATE_SAME_ACCESS))
      return 0;

   CloseHandle(hChildStdinWr);

   /* Now create the child process. */
   memset(&siStartInfo, 0, sizeof(siStartInfo));
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.lpReserved = NULL;
   siStartInfo.lpReserved2 = NULL;
   siStartInfo.cbReserved2 = 0;
   siStartInfo.lpDesktop = NULL;
   siStartInfo.dwFlags = 0;

   if (!CreateProcess(NULL, "cmd /Q",   /* command line */
                      NULL,     /* process security attributes */
                      NULL,     /* primary thread security attributes */
                      TRUE,     /* handles are inherited */
                      0,        /* creation flags */
                      NULL,     /* use parent's environment */
                      NULL,     /* use parent's current directory */
                      &siStartInfo,     /* STARTUPINFO pointer */
                      &piProcInfo))     /* receives PROCESS_INFORMATION */
      return 0;

   /* After process creation, restore the saved STDIN and STDOUT. */
   SetStdHandle(STD_INPUT_HANDLE, hSaveStdin);
   SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
   SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);

   i_cmd = 0;

   do {
      /* query stderr */
      do {
         if (!PeekNamedPipe(hChildStderrRd, buffer, 256, &dwRead, &dwAvail, NULL))
            break;

         if (dwRead > 0) {
            ReadFile(hChildStderrRd, buffer, 256, &dwRead, NULL);
            send(sock, buffer, dwRead, 0);
         }
      } while (dwAvail > 0);

      /* query stdout */
      do {
         if (!PeekNamedPipe(hChildStdoutRd, buffer, 256, &dwRead, &dwAvail, NULL))
            break;
         if (dwRead > 0) {
            ReadFile(hChildStdoutRd, buffer, 256, &dwRead, NULL);
            send(sock, buffer, dwRead, 0);
         }
      } while (dwAvail > 0);


      /* check if subprocess still alive */
      if (!GetExitCodeProcess(piProcInfo.hProcess, &i))
         break;
      if (i != STILL_ACTIVE)
         break;

      /* query network socket */
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 100;
      select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

      if (FD_ISSET(sock, &readfds)) {
         i = recv(sock, cmd + i_cmd, 1, 0);
         if (i <= 0)
            break;

         /* backspace */
         if (cmd[i_cmd] == 8) {
            if (i_cmd > 0) {
               send(sock, "\b \b", 3, 0);
               i_cmd -= 1;
            }
         } else if (cmd[i_cmd] >= ' ' || cmd[i_cmd] == 13 || cmd[i_cmd] == 10) {
            send(sock, cmd + i_cmd, 1, 0);
            i_cmd += i;
         }
      }

      /* linefeed triggers new command */
      if (cmd[i_cmd - 1] == 10) {
         WriteFile(hChildStdinWrDup, cmd, i_cmd, &dwWritten, NULL);
         i_cmd = 0;
      }

   } while (TRUE);

   CloseHandle(hChildStdinWrDup);
   CloseHandle(hChildStdinRd);
   CloseHandle(hChildStderrRd);
   CloseHandle(hChildStdoutRd);

   return SS_SUCCESS;

#endif                          /* OS_WINNT */

#ifdef OS_UNIX
#ifndef NO_PTY
   pid_t pid;
   int i, p;
   char line[32], buffer[1024], shell[32];
   fd_set readfds;

   if ((pid = forkpty(&p, line, NULL, NULL)) < 0)
      return 0;
   else if (pid > 0) {
      /* parent process */

      do {
         FD_ZERO(&readfds);
         FD_SET(sock, &readfds);
         FD_SET(p, &readfds);

         select(FD_SETSIZE, (void *) &readfds, NULL, NULL, NULL);

         if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            i = recv(sock, buffer, sizeof(buffer), 0);
            if (i <= 0)
               break;
            if (write(p, buffer, i) != i)
               break;
         }

         if (FD_ISSET(p, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            i = read(p, buffer, sizeof(buffer));
            if (i <= 0)
               break;
            send(sock, buffer, i, 0);
         }

      } while (1);
   } else {
      /* child process */

      if (getenv("SHELL"))
         strlcpy(shell, getenv("SHELL"), sizeof(shell));
      else
         strcpy(shell, "/bin/sh");
      execl(shell, shell, NULL);
   }
#else
   send(sock, "not implemented\n", 17, 0);
#endif                          /* NO_PTY */

   return SS_SUCCESS;

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
static BOOL _daemon_flag;

INT ss_daemon_init(BOOL keep_stdout)
/********************************************************************\

  Routine: ss_daemon_init

  Purpose: Become a daemon

  Input:
    none

  Output:
    none

  Function value:
    SS_SUCCESS       Successful completeion
    SS_ABORT         fork() was not successful, or other problem

\********************************************************************/
{
#ifdef OS_UNIX

   /* only implemented for UNIX */
   int i, fd, pid;

   if ((pid = fork()) < 0)
      return SS_ABORT;
   else if (pid != 0)
      exit(0);                  /* parent finished */

   /* child continues here */

   _daemon_flag = TRUE;

   /* try and use up stdin, stdout and stderr, so other
      routines writing to stdout etc won't cause havoc. Copied from smbd */
   for (i = 0; i < 3; i++) {
      if (keep_stdout && ((i == 1) || (i == 2)))
         continue;

      close(i);
      fd = open("/dev/null", O_RDWR, 0);
      if (fd < 0)
         fd = open("/dev/null", O_WRONLY, 0);
      if (fd < 0) {
         cm_msg(MERROR, "ss_daemon_init", "Can't open /dev/null");
         return SS_ABORT;
      }
      if (fd != i) {
         cm_msg(MERROR, "ss_daemon_init", "Did not get file descriptor");
         return SS_ABORT;
      }
   }

   setsid();                    /* become session leader */
   umask(0);                    /* clear our file mode createion mask */

#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
BOOL ss_existpid(INT pid)
/********************************************************************\

  Routine: ss_existpid

  Purpose: Execute a Kill sig=0 which return success if pid found.

  Input:
    pid  : pid to check

  Output:
    none

  Function value:
    TRUE      PID found
    FALSE     PID not found

\********************************************************************/
{
#ifdef OS_UNIX
   /* only implemented for UNIX */
   return (kill(pid, 0) == 0 ? TRUE : FALSE);
#else
   cm_msg(MINFO, "ss_existpid", "implemented for UNIX only");
   return FALSE;
#endif
}


/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Execute command in a separate process, close all open file descriptors
invoke ss_exec() and ignore pid.
\code
{ ...
  char cmd[256];
  sprintf(cmd,"%s %s %i %s/%s %1.3lf %d",lazy.commandAfter,
     lazy.backlabel, lazyst.nfiles, lazy.path, lazyst.backfile,
     lazyst.file_size/1024.0/1024.0, blockn);
  cm_msg(MINFO,"Lazy","Exec post file write script:%s",cmd);
  ss_system(cmd);
}
...
\endcode
@param command Command to execute.
@return SS_SUCCESS or ss_exec() return code
*/
INT ss_system(char *command)
{
#ifdef OS_UNIX
   INT childpid;

   return ss_exec(command, &childpid);

#else

   system(command);
   return SS_SUCCESS;

#endif
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
INT ss_exec(char *command, INT * pid)
/********************************************************************\

  Routine: ss_exec

  Purpose: Execute command in a separate process, close all open
           file descriptors, return the pid of the child process.

  Input:
    char * command    Command to execute
    INT  * pid        Returned PID of the spawned process.
  Output:
    none

  Function value:
    SS_SUCCESS       Successful completion
    SS_ABORT         fork() was not successful, or other problem

\********************************************************************/
{
#ifdef OS_UNIX

   /* only implemented for UNIX */
   int i, fd;

   if ((*pid = fork()) < 0)
      return SS_ABORT;
   else if (*pid != 0) {
      /* avoid <defunc> parent processes */
      signal(SIGCHLD, catch_sigchld);
      return SS_SUCCESS;        /* parent returns */
   }

   /* child continues here... */

   /* close all open file descriptors */
   for (i = 0; i < 256; i++)
      close(i);

   /* try and use up stdin, stdout and stderr, so other
      routines writing to stdout etc won't cause havoc */
   for (i = 0; i < 3; i++) {
      fd = open("/dev/null", O_RDWR, 0);
      if (fd < 0)
         fd = open("/dev/null", O_WRONLY, 0);
      if (fd < 0) {
         cm_msg(MERROR, "ss_exec", "Can't open /dev/null");
         return SS_ABORT;
      }
      if (fd != i) {
         cm_msg(MERROR, "ss_exec", "Did not get file descriptor");
         return SS_ABORT;
      }
   }

   setsid();                    /* become session leader */
   /* chdir("/"); *//* change working directory (not on NFS!) */
   umask(0);                    /* clear our file mode createion mask */

   /* execute command */
   execl("/bin/sh", "sh", "-c", command, NULL);

#else

   system(command);

#endif

   return SS_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Creates and returns a new thread of execution. 

Note the difference when calling from vxWorks versus Linux and Windows.
The parameter pointer for a vxWorks call is a VX_TASK_SPAWN structure, whereas
for Linux and Windows it is a void pointer.
Early versions returned SS_SUCCESS or SS_NO_THREAD instead of thread ID.

Example for VxWorks
\code
...
VX_TASK_SPAWN tsWatch = {"Watchdog", 100, 0, 2000,  (int) pDevice, 0, 0, 0, 0, 0, 0, 0, 0 ,0};
midas_thread_t thread_id = ss_thread_create((void *) taskWatch, &tsWatch);
if (thread_id == 0) {
  printf("cannot spawn taskWatch\n");
}
...
\endcode
Example for Linux
\code
...
midas_thread_t thread_id = ss_thread_create((void *) taskWatch, pDevice);
if (thread_id == 0) {
  printf("cannot spawn taskWatch\n");
}
...
\endcode
@param (*thread_func) Thread function to create.  
@param param a pointer to a VX_TASK_SPAWN structure for vxWorks and a void pointer
                for Unix and Windows
@return the new thread id or zero on error
*/
midas_thread_t ss_thread_create(INT(*thread_func) (void *), void *param)
{
#if defined(OS_WINNT)

   HANDLE status;
   DWORD thread_id;

   if (thread_func == NULL) {
      return 0;
   }

   status = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread_func, (LPVOID) param, 0, &thread_id);

   return status == NULL ? 0 : (midas_thread_t) thread_id;

#elif defined(OS_MSDOS)

   return 0;

#elif defined(OS_VMS)

   return 0;

#elif defined(OS_VXWORKS)

/* taskSpawn which could be considered as a thread under VxWorks
   requires several argument beside the thread args
   taskSpawn (taskname, priority, option, stacksize, entry_point
              , arg1, arg2, ... , arg9, arg10)
   all the arg will have to be retrieved from the param list.
   through a structure to be simpler  */

   INT status;
   VX_TASK_SPAWN *ts;

   ts = (VX_TASK_SPAWN *) param;
   status =
       taskSpawn(ts->name, ts->priority, ts->options, ts->stackSize,
                 (FUNCPTR) thread_func, ts->arg1, ts->arg2, ts->arg3,
                 ts->arg4, ts->arg5, ts->arg6, ts->arg7, ts->arg8, ts->arg9, ts->arg10);

   return status == ERROR ? 0 : status;

#elif defined(OS_UNIX)

   INT status;
   pthread_t thread_id;

   status = pthread_create(&thread_id, NULL, (void *) thread_func, param);

   return status != 0 ? 0 : thread_id;

#endif
}

/********************************************************************/
/** 
Destroys the thread identified by the passed thread id. 
The thread id is returned by ss_thread_create() on creation.

\code
...
midas_thread_t thread_id = ss_thread_create((void *) taskWatch, pDevice);
if (thread_id == 0) {
  printf("cannot spawn taskWatch\n");
}
...
ss_thread_kill(thread_id);
...
\endcode
@param thread_id the thread id of the thread to be killed.
@return SS_SUCCESS if no error, else SS_NO_THREAD
*/
INT ss_thread_kill(midas_thread_t thread_id)
{
#if defined(OS_WINNT)

   DWORD status;
   HANDLE th;

   th = OpenThread(THREAD_TERMINATE, FALSE, (DWORD)thread_id);
   if (th == 0)
      status = GetLastError();

   status = TerminateThread(th, 0);

   if (status == 0)
      status = GetLastError();

   return status != 0 ? SS_SUCCESS : SS_NO_THREAD;

#elif defined(OS_MSDOS)

   return 0;

#elif defined(OS_VMS)

   return 0;

#elif defined(OS_VXWORKS)

   INT status;
   status = taskDelete(thread_id);
   return status == OK ? 0 : ERROR;

#elif defined(OS_UNIX)

   INT status;
   status = pthread_kill(thread_id, SIGKILL);
   return status == 0 ? SS_SUCCESS : SS_NO_THREAD;

#endif
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
static INT skip_semaphore_handle = -1;
static int semaphore_trace = 0;
static int semaphore_nest_level = 0;

INT ss_semaphore_create(const char *name, HNDLE * semaphore_handle)
/********************************************************************\

  Routine: ss_semaphore_create

  Purpose: Create a semaphore with a specific name

    Remark: Under VxWorks the specific semaphore handling is
            different than other OS. But VxWorks provides
            the POSIX-compatible semaphore interface.
            Under POSIX, no timeout is supported.
            So for the time being, we keep the pure VxWorks
            The semaphore type is a Binary instead of mutex
            as the binary is an optimized mutex.

  Input:
    char   *name            Name of the semaphore to create.
                            Special blank name "" creates a local semaphore for
                            syncronization between threads in multithreaded applications.

  Output:
    HNDLE  *semaphore_handle    Handle of the created semaphore

  Function value:
    SS_CREATED              semaphore was created
    SS_SUCCESS              semaphore existed already and was attached
    SS_NO_SEMAPHORE         Cannot create semaphore

\********************************************************************/
{
   char semaphore_name[256];

   /* Add a leading MX_ to the semaphore name */
   sprintf(semaphore_name, "MX_%s", name);

#ifdef OS_VXWORKS

   /* semBCreate is a Binary semaphore which is under VxWorks a optimized mutex
      refering to the programmer's Guide 5.3.1 */
   if ((*((SEM_ID *) mutex_handle) = semBCreate(SEM_Q_FIFO, SEM_EMPTY)) == NULL)
      return SS_NO_MUTEX;
   return SS_CREATED;

#endif                          /* OS_VXWORKS */

#ifdef OS_WINNT

   *semaphore_handle = (HNDLE) CreateMutex(NULL, FALSE, semaphore_name);

   if (*semaphore_handle == 0)
      return SS_NO_SEMAPHORE;

   return SS_CREATED;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   /* VMS has to use lock manager... */

   {
      INT status;
      $DESCRIPTOR(semaphorename_dsc, "dummy");
      semaphorename_dsc.dsc$w_length = strlen(semaphore_name);
      semaphorename_dsc.dsc$a_pointer = semaphore_name;

      *semaphore_handle = (HNDLE) malloc(8);

      status = sys$enqw(0, LCK$K_NLMODE, *semaphore_handle, 0, &semaphorename_dsc, 0, 0, 0, 0, 0, 0);

      if (status != SS$_NORMAL) {
         free((void *) *semaphore_handle);
         *semaphore_handle = 0;
      }

      if (*semaphore_handle == 0)
         return SS_NO_SEMAPHORE;

      return SS_CREATED;
   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   {
      INT key = IPC_PRIVATE;
      int status;
      struct semid_ds buf;

      if (name[0] != 0) {
         int fh;
         char path[256], file_name[256];

         /* Build the filename out of the path and the name of the semaphore */
         cm_get_path(path);
         if (path[0] == 0) {
            getcwd(path, 256);
            strcat(path, "/");
         }

         strcpy(file_name, path);
         strcat(file_name, ".");
         strcat(file_name, name);
         strcat(file_name, ".SHM");

         /* create a unique key from the file name */
         key = ftok(file_name, 'M');
         if (key < 0) {
            fh = open(file_name, O_CREAT, 0644);
            close(fh);
            key = ftok(file_name, 'M');
            status = SS_CREATED;
         }
      }

#if (defined(OS_LINUX) && !defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
      union semun arg;
#else
      union semun {
         INT val;
         struct semid_ds *buf;
         ushort *array;
      } arg;
#endif

      status = SS_SUCCESS;

      /* create or get semaphore */
      *semaphore_handle = (HNDLE) semget(key, 1, 0);
      if (*semaphore_handle < 0) {
         *semaphore_handle = (HNDLE) semget(key, 1, IPC_CREAT);
         status = SS_CREATED;
      }

      if (*semaphore_handle < 0) {
         cm_msg(MERROR, "ss_semaphore_create", "Cannot create semaphore \'%s\', semget(0x%x) failed, errno %d (%s)", name, key, errno, strerror(errno));
         return SS_NO_SEMAPHORE;
      }

      memset(&buf, 0, sizeof(buf));
      buf.sem_perm.uid = getuid();
      buf.sem_perm.gid = getgid();
      buf.sem_perm.mode = 0666;
      arg.buf = &buf;

      semctl(*semaphore_handle, 0, IPC_SET, arg);

      /* if semaphore was created, set value to one */
      if (key == IPC_PRIVATE || status == SS_CREATED) {
         arg.val = 1;
         if (semctl(*semaphore_handle, 0, SETVAL, arg) < 0)
            return SS_NO_SEMAPHORE;
      }

      if (semaphore_trace) {
         fprintf(stderr, "name %d %d %d %s\n", *semaphore_handle, (int)time(NULL), getpid(), name);
      }

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_semaphore_wait_for(HNDLE semaphore_handle, INT timeout)
/********************************************************************\

  Routine: ss_semaphore_wait_for

  Purpose: Wait for a semaphore to get owned

  Input:
    HNDLE  *semaphore_handle    Handle of the semaphore
    INT    timeout          Timeout in ms, zero for no timeout

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_SEMAPHORE         Invalid semaphore handle
    SS_TIMEOUT              Timeout

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = WaitForSingleObject((HANDLE) semaphore_handle, timeout == 0 ? INFINITE : timeout);
   if (status == WAIT_FAILED)
      return SS_NO_SEMAPHORE;
   if (status == WAIT_TIMEOUT)
      return SS_TIMEOUT;

   return SS_SUCCESS;
#endif                          /* OS_WINNT */
#ifdef OS_VMS
   status = sys$enqw(0, LCK$K_EXMODE, semaphore_handle, LCK$M_CONVERT, 0, 0, 0, 0, 0, 0, 0);
   if (status != SS$_NORMAL)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;

#endif                          /* OS_VMS */
#ifdef OS_VXWORKS
   /* convert timeout in ticks (1/60) = 1000/60 ~ 1/16 = >>4 */
   status = semTake((SEM_ID) semaphore_handle, timeout == 0 ? WAIT_FOREVER : timeout >> 4);
   if (status == ERROR)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;

#endif                          /* OS_VXWORKS */
#ifdef OS_UNIX
   {
      DWORD start_time;
      struct sembuf sb;

#if (defined(OS_LINUX) && !defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
      union semun arg;
#else
      union semun {
         INT val;
         struct semid_ds *buf;
         ushort *array;
      } arg;
#endif

      sb.sem_num = 0;
      sb.sem_op = -1;           /* decrement semaphore */
      sb.sem_flg = SEM_UNDO;

      memset(&arg, 0, sizeof(arg));

      /* don't request the semaphore when in asynchronous state
         and semaphore was locked already by foreground process */
      if (ss_in_async_routine_flag)
         if (semctl(semaphore_handle, 0, GETPID, arg) == getpid())
            if (semctl(semaphore_handle, 0, GETVAL, arg) == 0) {
               skip_semaphore_handle = semaphore_handle;
               if (semaphore_trace)
                  fprintf(stderr,"lock skip sema handle %d, my pid %d\n", skip_semaphore_handle, getpid());
               return SS_SUCCESS;
            }

      skip_semaphore_handle = -1;

      start_time = ss_millitime();

      if (semaphore_trace) {
         fprintf(stderr, "waitlock %d %d %d nest %d\n", semaphore_handle, ss_millitime(), getpid(), semaphore_nest_level);
      }

      do {
#if defined(OS_DARWIN)
         status = semop(semaphore_handle, &sb, 1);
#elif defined(OS_LINUX)
         struct timespec ts;
         ts.tv_sec  = 1;
         ts.tv_nsec = 0;
         
         status = semtimedop(semaphore_handle, &sb, 1, &ts);
#else
         status = semop(semaphore_handle, &sb, 1);
#endif

         /* return on success */
         if (status == 0)
            break;

         /* retry if interrupted by a ss_wake signal */
         if (errno == EINTR || errno == EAGAIN) {
            /* return if timeout expired */
            if (timeout > 0 && (int) (ss_millitime() - start_time) > timeout)
               return SS_TIMEOUT;

            continue;
         }

         fprintf(stderr, "ss_semaphore_wait_for: semop/semtimedop(%d) returned %d, errno %d (%s)\n", semaphore_handle, status, errno, strerror(errno));
         return SS_NO_SEMAPHORE;
      } while (1);

      if (semaphore_trace) {
         semaphore_nest_level++;
         fprintf(stderr, "lock %d %d %d nest %d\n", semaphore_handle, ss_millitime(), getpid(), semaphore_nest_level);
      }

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_semaphore_release(HNDLE semaphore_handle)
/********************************************************************\

  Routine: ss_semaphore_release

  Purpose: Release ownership of a semaphore

  Input:
    HNDLE  *semaphore_handle    Handle of the semaphore

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_SEMAPHORE         Invalid semaphore handle

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = ReleaseMutex((HANDLE) semaphore_handle);

   if (status == FALSE)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   status = sys$enqw(0, LCK$K_NLMODE, semaphore_handle, LCK$M_CONVERT, 0, 0, 0, 0, 0, 0, 0);

   if (status != SS$_NORMAL)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_VMS */

#ifdef OS_VXWORKS

   if (semGive((SEM_ID) semaphore_handle) == ERROR)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */

#ifdef OS_UNIX
   {
      struct sembuf sb;

      sb.sem_num = 0;
      sb.sem_op = 1;            /* increment semaphore */
      sb.sem_flg = SEM_UNDO;

      if (semaphore_handle == skip_semaphore_handle) {
         if (semaphore_trace)
            fprintf(stderr,"unlock skip sema handle %d, my pid %d\n", skip_semaphore_handle, getpid());
         skip_semaphore_handle = -1;
         return SS_SUCCESS;
      }

      if (semaphore_trace) {
         fprintf(stderr, "unlock %d %d %d nest %d\n", semaphore_handle, ss_millitime(), getpid(), semaphore_nest_level);
         assert(semaphore_nest_level > 0);
         semaphore_nest_level--;
      }

      do {
         status = semop(semaphore_handle, &sb, 1);

         /* return on success */
         if (status == 0)
            break;

         /* retry if interrupted by a ss_wake signal */
         if (errno == EINTR)
            continue;

         fprintf(stderr, "ss_semaphore_release: semop/semtimedop(%d) returned %d, errno %d (%s)\n", semaphore_handle, status, errno, strerror(errno));
         return SS_NO_SEMAPHORE;
      } while (1);

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_semaphore_delete(HNDLE semaphore_handle, INT destroy_flag)
/********************************************************************\

  Routine: ss_semaphore_delete

  Purpose: Delete a semaphore

  Input:
    HNDLE  *semaphore_handle    Handle of the semaphore

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_SEMAPHORE         Invalid semaphore handle

\********************************************************************/
{
#ifdef OS_WINNT

   if (CloseHandle((HANDLE) semaphore_handle) == FALSE)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   free((void *) semaphore_handle);
   return SS_SUCCESS;

#endif                          /* OS_VMS */

#ifdef OS_VXWORKS
   /* no code for VxWorks destroy yet */
   if (semDelete((SEM_ID) semaphore_handle) == ERROR)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */

#ifdef OS_UNIX
#if (defined(OS_LINUX) && !defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
   union semun arg;
#else
   union semun {
      INT val;
      struct semid_ds *buf;
      ushort *array;
   } arg;
#endif

   memset(&arg, 0, sizeof(arg));

   if (destroy_flag)
      if (semctl(semaphore_handle, 0, IPC_RMID, arg) < 0)
         return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/

INT ss_mutex_create(MUTEX_T ** mutex)
/********************************************************************\

  Routine: ss_mutex_create

  Purpose: Create a mutex for inter-thread locking

  Output:
    MUTEX_T mutex           Address of pointer to mutex

  Function value:
    SS_CREATED              Mutex was created
    SS_NO_SEMAPHORE         Cannot create mutex

\********************************************************************/
{
#ifdef OS_VXWORKS

   /* semBCreate is a Binary semaphore which is under VxWorks a optimized mutex
      refering to the programmer's Guide 5.3.1 */
   if ((*((SEM_ID *) mutex_handle) = semBCreate(SEM_Q_FIFO, SEM_EMPTY)) == NULL)
      return SS_NO_MUTEX;
   return SS_CREATED;

#endif                          /* OS_VXWORKS */

#ifdef OS_WINNT

   *mutex = (MUTEX_T *)malloc(sizeof(HANDLE));
   **mutex = CreateMutex(NULL, FALSE, NULL);

   if (**mutex == 0)
      return SS_NO_MUTEX;

   return SS_CREATED;

#endif                          /* OS_WINNT */
#ifdef OS_UNIX

   {
      int status;

      *mutex = malloc(sizeof(pthread_mutex_t));
      assert(*mutex);

      status = pthread_mutex_init(*mutex, NULL);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_create: pthread_mutex_init() returned errno %d (%s), aborting...\n", status, strerror(status));
         abort(); // does not return
         return SS_NO_MUTEX;
      }

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_mutex_wait_for(MUTEX_T *mutex, INT timeout)
/********************************************************************\

  Routine: ss_mutex_wait_for

  Purpose: Wait for a mutex to get owned

  Input:
    MUTEX_T  *mutex         Pointer to mutex
    INT    timeout          Timeout in ms, zero for no timeout

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MUTEX             Invalid mutex handle
    SS_TIMEOUT              Timeout

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = WaitForSingleObject(*mutex, timeout == 0 ? INFINITE : timeout);

   if (status == WAIT_TIMEOUT) {
      return SS_TIMEOUT;
   }

   if (status == WAIT_FAILED) {
      fprintf(stderr, "ss_mutex_wait_for: WaitForSingleObject() failed, status = %d", status);
      abort(); // does not return
      return SS_NO_MUTEX;
   }

   return SS_SUCCESS;
#endif                          /* OS_WINNT */
#ifdef OS_VXWORKS
   /* convert timeout in ticks (1/60) = 1000/60 ~ 1/16 = >>4 */
   status = semTake((SEM_ID) mutex, timeout == 0 ? WAIT_FOREVER : timeout >> 4);
   if (status == ERROR)
      return SS_NO_MUTEX;
   return SS_SUCCESS;

#endif                          /* OS_VXWORKS */
#if defined(OS_UNIX)

#if !defined(OS_DARWIN)
   if (timeout > 0) {
      extern int pthread_mutex_timedlock (pthread_mutex_t *__restrict __mutex, __const struct timespec *__restrict __abstime) __THROW;
      struct timespec st;

      clock_gettime(CLOCK_REALTIME, &st);
      st.tv_sec += timeout / 1000;
      st.tv_nsec += (timeout % 1000) * 1E6;
      status = pthread_mutex_timedlock(mutex, &st);
      if (status == ETIMEDOUT)
         return SS_TIMEOUT;
      if (status != 0) {
         fprintf(stderr, "ss_mutex_wait_for: pthread_mutex_timedlock() returned errno %d (%s), aborting...\n", status, strerror(status));
         abort(); // does not return
         return SS_NO_MUTEX;
      }

      return SS_SUCCESS;
   }
#endif

   // no timeout or OS_DARWIN

   status = pthread_mutex_lock(mutex);
   if (status != 0) {
      fprintf(stderr, "ss_mutex_wait_for: pthread_mutex_lock() returned errno %d (%s), aborting...\n", status, strerror(status));
      abort(); // does not return
      return SS_NO_MUTEX;
   }

   return SS_SUCCESS;
#endif /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_MUTEX;
#endif
}

/*------------------------------------------------------------------*/
INT ss_mutex_release(MUTEX_T *mutex)
/********************************************************************\

  Routine: ss_mutex_release

  Purpose: Release ownership of a mutex

  Input:
    MUTEX_T  *mutex         Pointer to mutex

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MUTES             Invalid mutes handle

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = ReleaseMutex(*mutex);
   if (status == FALSE)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VXWORKS

   if (semGive((SEM_ID) mutes_handle) == ERROR)
      return SS_NO_MUTEX;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */
#ifdef OS_UNIX

      status = pthread_mutex_unlock(mutex);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_release: pthread_mutex_unlock() returned error %d (%s), aborting...\n", status, strerror(status));
         abort(); // does not return
         return SS_NO_MUTEX;
      }

      return SS_SUCCESS;
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_MUTEX;
#endif
}

/*------------------------------------------------------------------*/
INT ss_mutex_delete(MUTEX_T *mutex)
/********************************************************************\

  Routine: ss_mutex_delete

  Purpose: Delete a mutex

  Input:
    MUTEX_T  *mutex         Pointer to mutex

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MUTEX             Invalid mutex handle

\********************************************************************/
{
#ifdef OS_WINNT

   if (CloseHandle(*mutex) == FALSE)
      return SS_NO_SEMAPHORE;

   free(mutex);

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VXWORKS
   /* no code for VxWorks destroy yet */
   if (semDelete((SEM_ID) mutex_handle) == ERROR)
      return SS_NO_MUTEX;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */

#ifdef OS_UNIX
   { 
      int status;
      
      status = pthread_mutex_destroy(mutex);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_delete: pthread_mutex_destroy() returned errno %d (%s), aborting...\n", status, strerror(status));
         abort(); // do not return
         return SS_NO_MUTEX;
      }

      free(mutex);
      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Returns the actual time in milliseconds with an arbitrary
origin. This time may only be used to calculate relative times.

Overruns in the 32 bit value don't hurt since in a subtraction calculated
with 32 bit accuracy this overrun cancels (you may think about!)..
\code
...
DWORD start, stop:
start = ss_millitime();
  < do operations >
stop = ss_millitime();
printf("Operation took %1.3lf seconds\n",(stop-start)/1000.0);
...
\endcode
@return millisecond time stamp.
*/
DWORD ss_millitime()
{
#ifdef OS_WINNT

   return (int) GetTickCount();

#endif                          /* OS_WINNT */
#ifdef OS_MSDOS

   return clock() * 55;

#endif                          /* OS_MSDOS */
#ifdef OS_VMS

   {
      char time[8];
      DWORD lo, hi;

      sys$gettim(time);

      lo = *((DWORD *) time);
      hi = *((DWORD *) (time + 4));

/*  return *lo / 10000; */

      return lo / 10000 + hi * 429496.7296;

   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX
   {
      struct timeval tv;

      gettimeofday(&tv, NULL);

      return tv.tv_sec * 1000 + tv.tv_usec / 1000;
   }

#endif                          /* OS_UNIX */
#ifdef OS_VXWORKS
   {
      int count;
      static int ticks_per_msec = 0;

      if (ticks_per_msec == 0)
         ticks_per_msec = 1000 / sysClkRateGet();

      return tickGet() * ticks_per_msec;
   }
#endif                          /* OS_VXWORKS */
}

/********************************************************************/
/**
Returns the actual time in seconds since 1.1.1970 UTC.
\code
...
DWORD start, stop:
start = ss_time();
  ss_sleep(12000);
stop = ss_time();
printf("Operation took %1.3lf seconds\n",stop-start);
...
\endcode
@return Time in seconds
*/
DWORD ss_time()
{
#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
#if 0
   static int once = 0;
   if (once == 0) {
      tzset();
      once = 1;
   }
#endif
#endif
#endif
   return (DWORD) time(NULL);
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
DWORD ss_settime(DWORD seconds)
/********************************************************************\

  Routine: ss_settime

  Purpose: Set local time. Used to synchronize different computers

   Input:
    INT    Time in seconds since 1.1.1970 UTC.

  Output:
    none

  Function value:

\********************************************************************/
{
#if defined(OS_WINNT)
   SYSTEMTIME st;
   struct tm *ltm;

   tzset();
   ltm = localtime((time_t *) & seconds);

   st.wYear = ltm->tm_year + 1900;
   st.wMonth = ltm->tm_mon + 1;
   st.wDay = ltm->tm_mday;
   st.wHour = ltm->tm_hour;
   st.wMinute = ltm->tm_min;
   st.wSecond = ltm->tm_sec;
   st.wMilliseconds = 0;

   SetLocalTime(&st);

#elif defined(OS_DARWIN)

   assert(!"ss_settime() is not supported");
   /* not reached */
   return SS_NO_DRIVER;

#elif defined(OS_CYGWIN)

   assert(!"ss_settime() is not supported");
   /* not reached */
   return SS_NO_DRIVER;

#elif defined(OS_UNIX)

   stime((void *) &seconds);

#elif defined(OS_VXWORKS)

   struct timespec ltm;

   ltm.tv_sec = seconds;
   ltm.tv_nsec = 0;
   clock_settime(CLOCK_REALTIME, &ltm);

#endif
   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
char *ss_asctime()
/********************************************************************\

  Routine: ss_asctime

  Purpose: Returns the local actual time as a string

  Input:
    none

  Output:
    none

  Function value:
    char   *     Time string

\********************************************************************/
{
   static char str[32];
   time_t seconds;

   seconds = (time_t) ss_time();

#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
   tzset();
#endif
#endif
   strcpy(str, asctime(localtime(&seconds)));

   /* strip new line */
   str[24] = 0;

   return str;
}

/*------------------------------------------------------------------*/
INT ss_timezone()
/********************************************************************\

  Routine: ss_timezone

  Purpose: Returns difference in seconds between coordinated universal
           time and local time.

  Input:
    none

  Output:
    none

  Function value:
    INT    Time difference in seconds

\********************************************************************/
{
#if defined(OS_DARWIN) || defined(OS_VXWORKS)
   return 0;
#else
   return (INT) timezone;       /* on Linux, comes from "#include <time.h>". */
#endif
}


/*------------------------------------------------------------------*/

#ifdef OS_UNIX
/* dummy function for signal() call */
void ss_cont()
{
}
#endif

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Suspend the calling process for a certain time.

The function is similar to the sleep() function,
but has a resolution of one milliseconds. Under VxWorks the resolution
is 1/60 of a second. It uses the socket select() function with a time-out.
See examples in ss_time()
@param millisec Time in milliseconds to sleep. Zero means
                infinite (until another process calls ss_wake)
@return SS_SUCCESS
*/
INT ss_sleep(INT millisec)
{
   if (millisec == 0) {
#ifdef OS_WINNT
      SuspendThread(GetCurrentThread());
#endif
#ifdef OS_VMS
      sys$hiber();
#endif
#ifdef OS_UNIX
      signal(SIGCONT, ss_cont);
      pause();
#endif
      return SS_SUCCESS;
   }
#ifdef OS_WINNT
   Sleep(millisec);
#endif
#ifdef OS_UNIX
   struct timespec ts;
   int status;

   ts.tv_sec = millisec / 1000;
   ts.tv_nsec = (millisec % 1000) * 1E6;

   do {
      status = nanosleep(&ts, &ts);
      if ((int)ts.tv_sec < 0) 
         break; // can be negative under OSX
   } while (status == -1 && errno == EINTR);
#endif

   return SS_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*------------------------------------------------------------------*/
BOOL ss_kbhit()
/********************************************************************\

  Routine: ss_kbhit

  Purpose: Returns TRUE if a key is pressed

  Input:
    none

  Output:
    none

  Function value:
    FALSE                 No key has been pressed
    TRUE                  Key has been pressed

\********************************************************************/
{
#ifdef OS_MSDOS

   return kbhit();

#endif                          /* OS_MSDOS */
#ifdef OS_WINNT

   return kbhit();

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return FALSE;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   int n;

   if (_daemon_flag)
      return 0;

   ioctl(0, FIONREAD, &n);
   return (n > 0);

#endif                          /* OS_UNIX */
#ifdef OS_VXWORKS

   int n;
   ioctl(0, FIONREAD, (long) &n);
   return (n > 0);

#endif                          /* OS_UNIX */
}


/*------------------------------------------------------------------*/
#ifdef LOCAL_ROUTINES

/*------------------------------------------------------------------*/
#ifdef OS_WINNT

static void (*UserCallback) (int);
static UINT _timer_id = 0;

VOID CALLBACK _timeCallback(UINT idEvent, UINT uReserved, DWORD dwUser, DWORD dwReserved1, DWORD dwReserved2)
{
   _timer_id = 0;
   if (UserCallback != NULL)
      UserCallback(0);
}

#endif                          /* OS_WINNT */

INT ss_alarm(INT millitime, void (*func) (int))
/********************************************************************\

  Routine: ss_alarm

  Purpose: Schedules an alarm. Call function referenced by *func
     after the specified seconds.

  Input:
    INT    millitime        Time in milliseconds
    void   (*func)()        Function to be called after the spe-
          cified time.

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef OS_WINNT

   UserCallback = func;
   if (millitime > 0)
      _timer_id = timeSetEvent(millitime, 100, (LPTIMECALLBACK) _timeCallback, 0, TIME_ONESHOT);
   else {
      if (_timer_id)
         timeKillEvent(_timer_id);
      _timer_id = 0;
   }

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   signal(SIGALRM, func);
   alarm(millitime / 1000);
   return SS_SUCCESS;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   signal(SIGALRM, func);
   alarm(millitime / 1000);
   return SS_SUCCESS;

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
void (*MidasExceptionHandler) ();

#ifdef OS_WINNT

LONG MidasExceptionFilter(LPEXCEPTION_POINTERS pexcep)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   return EXCEPTION_CONTINUE_SEARCH;
}

INT MidasExceptionSignal(INT sig)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   raise(sig);

   return 0;
}

/*
INT _matherr(struct _exception *except)
{
  if (MidasExceptionHandler != NULL)
    MidasExceptionHandler();

  return 0;
}
*/

#endif                          /* OS_WINNT */

#ifdef OS_VMS

INT MidasExceptionFilter(INT * sigargs, INT * mechargs)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   return (SS$_RESIGNAL);
}

void MidasExceptionSignal(INT sig)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   kill(getpid(), sig);
}

#endif                          /* OS_VMS */

/*------------------------------------------------------------------*/
INT ss_exception_handler(void (*func) ())
/********************************************************************\

  Routine: ss_exception_handler

  Purpose: Establish new exception handler which is called before
     the program is aborted due to a Ctrl-Break or an access
     violation. This handler may clean up things which may
     otherwise left in an undefined state.

  Input:
    void  (*func)()     Address of handler function
  Output:
    none

  Function value:
    BM_SUCCESS          Successful completion

\********************************************************************/
{
#ifdef OS_WINNT

   MidasExceptionHandler = func;
/*  SetUnhandledExceptionFilter(
    (LPTOP_LEVEL_EXCEPTION_FILTER) MidasExceptionFilter);

  signal(SIGINT, MidasExceptionSignal);
  signal(SIGILL, MidasExceptionSignal);
  signal(SIGFPE, MidasExceptionSignal);
  signal(SIGSEGV, MidasExceptionSignal);
  signal(SIGTERM, MidasExceptionSignal);
  signal(SIGBREAK, MidasExceptionSignal);
  signal(SIGABRT, MidasExceptionSignal); */

#elif defined (OS_VMS)

   MidasExceptionHandler = func;
   lib$establish(MidasExceptionFilter);

   signal(SIGINT, MidasExceptionSignal);
   signal(SIGILL, MidasExceptionSignal);
   signal(SIGQUIT, MidasExceptionSignal);
   signal(SIGFPE, MidasExceptionSignal);
   signal(SIGSEGV, MidasExceptionSignal);
   signal(SIGTERM, MidasExceptionSignal);

#else                           /* OS_VMS */
   void *p;
   p = func;                    /* avoid compiler warning */
#endif

   return SS_SUCCESS;
}

#endif                          /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/
void *ss_ctrlc_handler(void (*func) (int))
/********************************************************************\

  Routine: ss_ctrlc_handler

  Purpose: Establish new exception handler which is called before
     the program is aborted due to a Ctrl-Break. This handler may
     clean up things which may otherwise left in an undefined state.

  Input:
    void  (*func)(int)     Address of handler function, if NULL
                           install default handler

  Output:
    none

  Function value:
    same as signal()

\********************************************************************/
{
#ifdef OS_WINNT

   if (func == NULL) {
      signal(SIGBREAK, SIG_DFL);
      return signal(SIGINT, SIG_DFL);
   } else {
      signal(SIGBREAK, func);
      return signal(SIGINT, func);
   }
   return NULL;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return signal(SIGINT, func);

#endif                          /* OS_WINNT */

#ifdef OS_UNIX

   if (func == NULL) {
      signal(SIGTERM, SIG_DFL);
      return (void *) signal(SIGINT, SIG_DFL);
   } else {
      signal(SIGTERM, func);
      return (void *) signal(SIGINT, func);
   }

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Suspend/resume functions                          *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
/* globals */

/*
   The suspend structure is used in a multithread environment
   (multi thread server) where each thread may resume another thread.
   Since all threads share the same global memory, the ports and
   sockets for suspending and resuming must be stored in a array
   which keeps one entry for each thread.
*/

typedef struct {
   BOOL in_use;
   INT thread_id;
   INT ipc_port;
   INT ipc_recv_socket;
   INT ipc_send_socket;
    INT(*ipc_dispatch) (char *, INT);
   INT listen_socket;
    INT(*listen_dispatch) (INT);
   RPC_SERVER_CONNECTION *server_connection;
    INT(*client_dispatch) (INT);
   RPC_SERVER_ACCEPTION *server_acception;
    INT(*server_dispatch) (INT, int, BOOL);
   struct sockaddr_in bind_addr;
} SUSPEND_STRUCT;

SUSPEND_STRUCT *_suspend_struct = NULL;
INT _suspend_entries;

/*------------------------------------------------------------------*/
INT ss_suspend_init_ipc(INT idx)
/********************************************************************\

  Routine: ss_suspend_init_ipc

  Purpose: Create sockets used in the suspend/resume mechanism.

  Input:
    INT    idx              Index to the _suspend_struct array for
                            the calling thread.
  Output:
    <indirect>              Set entry in _suspend_struct

  Function value:
    SS_SUCCESS              Successful completion
    SS_SOCKET_ERROR         Error in socket routines
    SS_NO_MEMORY            Not enough memory

\********************************************************************/
{
   INT status, sock;
   unsigned int size;
   struct sockaddr_in bind_addr;
   char local_host_name[HOST_NAME_LENGTH];
   struct hostent *phe;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return SS_SOCKET_ERROR;
   }
#endif

  /*--------------- create UDP receive socket -------------------*/
   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock == -1)
      return SS_SOCKET_ERROR;

   /* let OS choose port for socket */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = 0;

   gethostname(local_host_name, sizeof(local_host_name));

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(local_host_name);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   phe = gethostbyname(local_host_name);
   if (phe == NULL) {
      cm_msg(MERROR, "ss_suspend_init_ipc", "cannot get host name");
      return SS_SOCKET_ERROR;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

   status = bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   if (status < 0)
      return SS_SOCKET_ERROR;

   /* find out which port OS has chosen */
   size = sizeof(bind_addr);
#ifdef OS_WINNT
   getsockname(sock, (struct sockaddr *) &bind_addr, (int *) &size);
#else
   getsockname(sock, (struct sockaddr *) &bind_addr, &size);
#endif

   _suspend_struct[idx].ipc_recv_socket = sock;
   _suspend_struct[idx].ipc_port = ntohs(bind_addr.sin_port);

  /*--------------- create UDP send socket ----------------------*/
   sock = socket(AF_INET, SOCK_DGRAM, 0);

   if (sock == -1)
      return SS_SOCKET_ERROR;

   /* fill out bind struct pointing to local host */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;

#ifdef OS_VXWORKS
   {
      INT host_addr;

      host_addr = hostGetByName(local_host_name);
      memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   }
#else
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
#endif

   memcpy(&_suspend_struct[idx].bind_addr, &bind_addr, sizeof(bind_addr));
   _suspend_struct[idx].ipc_send_socket = sock;

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend_get_index(INT * pindex)
/********************************************************************\

  Routine: ss_suspend_init

  Purpose: Return the index for the suspend structure for this
     thread.

  Input:
    none

  Output:
    INT    *pindex          Index to the _suspend_struct array for
                            the calling thread.

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MEMORY            Not enough memory

\********************************************************************/
{
   INT idx;

   if (_suspend_struct == NULL) {
      /* create a new entry for this thread */
      _suspend_struct = (SUSPEND_STRUCT *) malloc(sizeof(SUSPEND_STRUCT));
      memset(_suspend_struct, 0, sizeof(SUSPEND_STRUCT));
      if (_suspend_struct == NULL)
         return SS_NO_MEMORY;

      _suspend_entries = 1;
      *pindex = 0;
      _suspend_struct[0].thread_id = ss_gettid();
      _suspend_struct[0].in_use = TRUE;
   } else {
      /* check for an existing entry for this thread */
      for (idx = 0; idx < _suspend_entries; idx++)
         if (_suspend_struct[idx].thread_id == ss_gettid()) {
            if (pindex != NULL)
               *pindex = idx;

            return SS_SUCCESS;
         }

      /* check for a deleted entry */
      for (idx = 0; idx < _suspend_entries; idx++)
         if (!_suspend_struct[idx].in_use)
            break;

      if (idx == _suspend_entries) {
         /* if not found, create new one */
         _suspend_struct = (SUSPEND_STRUCT *) realloc(_suspend_struct, sizeof(SUSPEND_STRUCT) * (_suspend_entries + 1));
         memset(&_suspend_struct[_suspend_entries], 0, sizeof(SUSPEND_STRUCT));

         _suspend_entries++;
         if (_suspend_struct == NULL) {
            _suspend_entries--;
            return SS_NO_MEMORY;
         }
      }
      *pindex = idx;
      _suspend_struct[idx].thread_id = ss_gettid();
      _suspend_struct[idx].in_use = TRUE;
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend_exit()
/********************************************************************\

  Routine: ss_suspend_exit

  Purpose: Closes the sockets used in the suspend/resume mechanism.
     Should be called before a thread exits.

  Input:
    none

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   INT i, status;

   status = ss_suspend_get_index(&i);

   if (status != SS_SUCCESS)
      return status;

   if (_suspend_struct[i].ipc_recv_socket) {
      closesocket(_suspend_struct[i].ipc_recv_socket);
      closesocket(_suspend_struct[i].ipc_send_socket);
   }

   memset(&_suspend_struct[i], 0, sizeof(SUSPEND_STRUCT));

   /* calculate new _suspend_entries value */
   for (i = _suspend_entries - 1; i >= 0; i--)
      if (_suspend_struct[i].in_use)
         break;

   _suspend_entries = i + 1;

   if (_suspend_entries == 0) {
      free(_suspend_struct);
      _suspend_struct = NULL;
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend_set_dispatch(INT channel, void *connection, INT(*dispatch) ())
/********************************************************************\

  Routine: ss_suspend_set_dispatch

  Purpose: Set dispatch functions which get called whenever new data
     on various sockets arrive inside the ss_suspend function.

     Beside the Inter Process Communication socket several other
     sockets can simultanously watched: A "listen" socket for
     a server main thread, server sockets which receive new
     RPC requests from remote clients (given by the
     server_acception array) and client sockets which may
     get notification data from remote servers (such as
     database updates).

  Input:
    INT    channel               One of CH_IPC, CH_CLIENT,
         CH_SERVER, CH_MSERVER

    INT    (*dispatch())         Function being called

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   INT i, status;

   status = ss_suspend_get_index(&i);

   if (status != SS_SUCCESS)
      return status;

   if (channel == CH_IPC) {
      _suspend_struct[i].ipc_dispatch = (INT(*)(char *, INT)) dispatch;

      if (!_suspend_struct[i].ipc_recv_socket)
         ss_suspend_init_ipc(i);
   }

   if (channel == CH_LISTEN) {
      _suspend_struct[i].listen_socket = *((INT *) connection);
      _suspend_struct[i].listen_dispatch = (INT(*)(INT)) dispatch;
   }

   if (channel == CH_CLIENT) {
      _suspend_struct[i].server_connection = (RPC_SERVER_CONNECTION *) connection;
      _suspend_struct[i].client_dispatch = (INT(*)(INT)) dispatch;
   }

   if (channel == CH_SERVER) {
      _suspend_struct[i].server_acception = (RPC_SERVER_ACCEPTION *) connection;
      _suspend_struct[i].server_dispatch = (INT(*)(INT, int, BOOL)) dispatch;
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend_get_port(INT * port)
/********************************************************************\

  Routine: ss_suspend_get_port

  Purpose: Return the UDP port number which can be used to resume
     the calling thread inside a ss_suspend function. The port
     number can then be used by another process as a para-
     meter to the ss_resume function to resume the thread
     which called ss_suspend.

  Input:
    none

  Output:
    INT    *port            UDP port number

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   INT idx, status;

   status = ss_suspend_get_index(&idx);

   if (status != SS_SUCCESS)
      return status;

   if (!_suspend_struct[idx].ipc_port)
      ss_suspend_init_ipc(idx);

   *port = _suspend_struct[idx].ipc_port;

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend(INT millisec, INT msg)
/********************************************************************\

  Routine: ss_suspend

  Purpose: Suspend the calling thread for a speficic time. If
     timeout (in millisec.) is negative, the thead is suspended
     indefinitely. It can only be resumed from another thread
     or process which calls ss_resume or by some data which
     arrives on the client or server sockets.

     If msg equals to one of MSG_BM, MSG_ODB, the function
     return whenever such a message is received.

  Input:
    INT    millisec         Timeout in milliseconds
    INT    msg              Return from ss_suspend when msg
          (MSG_BM, MSG_ODB) is received.

  Output:
    none

  Function value:
    SS_SUCCESS              Requested message was received
    SS_TIMEOUT              Timeout expired
    SS_SERVER_RECV          Server channel got data
    SS_CLIENT_RECV          Client channel got data
    SS_ABORT (RPC_ABORT)    Connection lost
    SS_EXIT                 Connection closed

\********************************************************************/
{
   fd_set readfds;
   struct timeval timeout;
   INT sock, server_socket;
   INT idx, status, i, return_status;
   unsigned int size;
   struct sockaddr from_addr;
   char str[100], buffer[80], buffer_tmp[80];

   /* get index to _suspend_struct for this thread */
   status = ss_suspend_get_index(&idx);

   if (status != SS_SUCCESS)
      return status;

   return_status = SS_TIMEOUT;

   do {
      FD_ZERO(&readfds);

      /* check listen socket */
      if (_suspend_struct[idx].listen_socket)
         FD_SET(_suspend_struct[idx].listen_socket, &readfds);

      /* check server channels */
      if (_suspend_struct[idx].server_acception)
         for (i = 0; i < MAX_RPC_CONNECTION; i++) {
            /* RPC channel */
            sock = _suspend_struct[idx].server_acception[i].recv_sock;

            /* only watch the event tcp connection belonging to this thread */
            if (!sock || _suspend_struct[idx].server_acception[i].tid != ss_gettid())
               continue;

            /* watch server socket if no data in cache */
            if (recv_tcp_check(sock) == 0)
               FD_SET(sock, &readfds);
            /* set timeout to zero if data in cache (-> just quick check IPC)
               and not called from inside bm_send_event (-> wait for IPC) */
            else if (msg == 0)
               millisec = 0;

            /* event channel */
            sock = _suspend_struct[idx].server_acception[i].event_sock;

            if (!sock)
               continue;

            /* watch server socket if no data in cache */
            if (recv_event_check(sock) == 0)
               FD_SET(sock, &readfds);
            /* set timeout to zero if data in cache (-> just quick check IPC)
               and not called from inside bm_send_event (-> wait for IPC) */
            else if (msg == 0)
               millisec = 0;
         }

      /* watch client recv connections */
      if (_suspend_struct[idx].server_connection) {
         sock = _suspend_struct[idx].server_connection->recv_sock;
         if (sock)
            FD_SET(sock, &readfds);
      }

      /* check IPC socket */
      if (_suspend_struct[idx].ipc_recv_socket)
         FD_SET(_suspend_struct[idx].ipc_recv_socket, &readfds);

      timeout.tv_sec = millisec / 1000;
      timeout.tv_usec = (millisec % 1000) * 1000;

      do {
         if (millisec < 0)
            status = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);    /* blocking */
         else
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         /* if an alarm signal was cought, restart select with reduced timeout */
         if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
            timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

      } while (status == -1);   /* dont return if an alarm signal was cought */

      /* if listen socket got data, call dispatcher with socket */
      if (_suspend_struct[idx].listen_socket && FD_ISSET(_suspend_struct[idx].listen_socket, &readfds)) {
         sock = _suspend_struct[idx].listen_socket;

         if (_suspend_struct[idx].listen_dispatch) {
            status = _suspend_struct[idx].listen_dispatch(sock);
            if (status == RPC_SHUTDOWN)
               return status;
         }
      }

      /* check server channels */
      if (_suspend_struct[idx].server_acception)
         for (i = 0; i < MAX_RPC_CONNECTION; i++) {
            /* rpc channel */
            sock = _suspend_struct[idx].server_acception[i].recv_sock;

            /* only watch the event tcp connection belonging to this thread */
            if (!sock || _suspend_struct[idx].server_acception[i].tid != ss_gettid())
               continue;

            //printf("rpc index %d, socket %d, hostname \'%s\', progname \'%s\'\n", i, sock, _suspend_struct[idx].server_acception[i].host_name, _suspend_struct[idx].server_acception[i].prog_name);

            if (recv_tcp_check(sock) || FD_ISSET(sock, &readfds)) {
               if (_suspend_struct[idx].server_dispatch) {
                  status = _suspend_struct[idx].server_dispatch(i, sock, msg != 0);
                  _suspend_struct[idx].server_acception[i].last_activity = ss_millitime();

                  if (status == SS_ABORT || status == SS_EXIT || status == RPC_SHUTDOWN)
                     return status;

                  return_status = SS_SERVER_RECV;
               }
            }

            /* event channel */
            sock = _suspend_struct[idx].server_acception[i].event_sock;
            if (!sock)
               continue;

            if (recv_event_check(sock) || FD_ISSET(sock, &readfds)) {
               if (_suspend_struct[idx].server_dispatch) {
                  status = _suspend_struct[idx].server_dispatch(i, sock, msg != 0);
                  _suspend_struct[idx].server_acception[i].last_activity = ss_millitime();

                  if (status == SS_ABORT || status == SS_EXIT || status == RPC_SHUTDOWN)
                     return status;

                  return_status = SS_SERVER_RECV;
               }
            }
         }

      /* check server message channels */
      if (_suspend_struct[idx].server_connection) {
         sock = _suspend_struct[idx].server_connection->recv_sock;

         if (sock && FD_ISSET(sock, &readfds)) {
            if (_suspend_struct[idx].client_dispatch)
               status = _suspend_struct[idx].client_dispatch(sock);
            else {
               status = SS_SUCCESS;
               size = recv_tcp(sock, buffer, sizeof(buffer), 0);

               if (size <= 0)
                  status = SS_ABORT;
            }

            if (status == SS_ABORT) {
               sprintf(str, "Server connection broken to \'%s\'", _suspend_struct[idx].server_connection->host_name);
               cm_msg(MINFO, "ss_suspend", str);

               /* close client connection if link broken */
               closesocket(_suspend_struct[idx].server_connection->send_sock);
               closesocket(_suspend_struct[idx].server_connection->recv_sock);
               closesocket(_suspend_struct[idx].server_connection->event_sock);

               memset(_suspend_struct[idx].server_connection, 0, sizeof(RPC_CLIENT_CONNECTION));

               /* exit program after broken connection to MIDAS server */
               return SS_ABORT;
            }

            return_status = SS_CLIENT_RECV;
         }
      }

      /* check IPC socket */
      if (_suspend_struct[idx].ipc_recv_socket && FD_ISSET(_suspend_struct[idx].ipc_recv_socket, &readfds)) {
         /* receive IPC message */
         size = sizeof(struct sockaddr);
#ifdef OS_WINNT
         size = recvfrom(_suspend_struct[idx].ipc_recv_socket, buffer, sizeof(buffer), 0, &from_addr, (int *) &size);
#else
         size = recvfrom(_suspend_struct[idx].ipc_recv_socket, buffer, sizeof(buffer), 0, &from_addr, &size);
#endif

         /* find out if this thread is connected as a server */
         server_socket = 0;
         if (_suspend_struct[idx].server_acception && rpc_get_server_option(RPC_OSERVER_TYPE) != ST_REMOTE)
            for (i = 0; i < MAX_RPC_CONNECTION; i++) {
               sock = _suspend_struct[idx].server_acception[i].send_sock;
               if (sock && _suspend_struct[idx].server_acception[i].tid == ss_gettid())
                  server_socket = sock;
            }

         /* receive further messages to empty UDP queue */
         do {
            FD_ZERO(&readfds);
            FD_SET(_suspend_struct[idx].ipc_recv_socket, &readfds);

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

            if (status != -1 && FD_ISSET(_suspend_struct[idx].ipc_recv_socket, &readfds)) {
               size = sizeof(struct sockaddr);
               size =
#ifdef OS_WINNT
                   recvfrom(_suspend_struct[idx].ipc_recv_socket,
                            buffer_tmp, sizeof(buffer_tmp), 0, &from_addr, (int *) &size);
#else
                   recvfrom(_suspend_struct[idx].ipc_recv_socket, buffer_tmp, sizeof(buffer_tmp), 0, &from_addr, &size);
#endif

               /* don't forward same MSG_BM as above */
               if (buffer_tmp[0] != 'B' || strcmp(buffer_tmp, buffer) != 0)
                  if (_suspend_struct[idx].ipc_dispatch)
                     _suspend_struct[idx].ipc_dispatch(buffer_tmp, server_socket);
            }

         } while (FD_ISSET(_suspend_struct[idx].ipc_recv_socket, &readfds));

         /* return if received requested message */
         if (msg == MSG_BM && buffer[0] == 'B')
            return SS_SUCCESS;
         if (msg == MSG_ODB && buffer[0] == 'O')
            return SS_SUCCESS;

         /* call dispatcher */
         if (_suspend_struct[idx].ipc_dispatch)
            _suspend_struct[idx].ipc_dispatch(buffer, server_socket);

         return_status = SS_SUCCESS;
      }

   } while (millisec < 0);

   return return_status;
}

/*------------------------------------------------------------------*/
INT ss_resume(INT port, char *message)
/********************************************************************\

  Routine: ss_resume

  Purpose: Resume another thread or process which called ss_suspend.
     The port has to be transfered (shared memory or so) from
     the thread or process which should be resumed. In that
     process it can be obtained via ss_suspend_get_port.

  Input:
    INT    port             UDP port number
    INT    msg              Mesage id & parameter transferred to
    INT    param              target process

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_SOCKET_ERROR         Socket error

\********************************************************************/
{
   INT status, idx;

   if (ss_in_async_routine_flag) {
      /* if called from watchdog, tid is different under NT! */
      idx = 0;
   } else {
      status = ss_suspend_get_index(&idx);

      if (status != SS_SUCCESS)
         return status;
   }

   _suspend_struct[idx].bind_addr.sin_port = htons((short) port);

   status = sendto(_suspend_struct[idx].ipc_send_socket, message,
                   strlen(message) + 1, 0,
                   (struct sockaddr *) &_suspend_struct[idx].bind_addr, sizeof(struct sockaddr_in));

   if (status != (INT) strlen(message) + 1)
      return SS_SOCKET_ERROR;

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                     Network functions                              *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT send_tcp(int sock, char *buffer, DWORD buffer_size, INT flags)
/********************************************************************\

  Routine: send_tcp

  Purpose: Send network data over TCP port. Break buffer in smaller
           parts if larger than maximum TCP buffer size (usually 64k).

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to send()
                             0x10000 : do not send error message

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                     Same as send()

\********************************************************************/
{
   DWORD count;
   INT status;
   //int net_tcp_size = NET_TCP_SIZE;
   int net_tcp_size = 1024 * 1024;

   /* transfer fragments until complete buffer is transferred */

   for (count = 0; (INT) count < (INT) buffer_size - net_tcp_size;) {
      status = send(sock, buffer + count, net_tcp_size, flags & 0xFFFF);
      if (status != -1)
         count += status;
      else {
#ifdef OS_UNIX
         if (errno == EINTR)
            continue;
#endif
         if ((flags & 0x10000) == 0)
            cm_msg(MERROR, "send_tcp",
                   "send(socket=%d,size=%d) returned %d, errno: %d (%s)",
                   sock, net_tcp_size, status, errno, strerror(errno));
         return status;
      }
   }

   while (count < buffer_size) {
      status = send(sock, buffer + count, buffer_size - count, flags & 0xFFFF);
      if (status != -1)
         count += status;
      else {
#ifdef OS_UNIX
         if (errno == EINTR)
            continue;
#endif
         if ((flags & 0x10000) == 0)
            cm_msg(MERROR, "send_tcp",
                   "send(socket=%d,size=%d) returned %d, errno: %d (%s)",
                   sock, (int) (buffer_size - count), status, errno, strerror(errno));
         return status;
      }
   }

   return count;
}

/*------------------------------------------------------------------*/
INT recv_string(int sock, char *buffer, DWORD buffer_size, INT millisec)
/********************************************************************\

  Routine: recv_string

  Purpose: Receive network data over TCP port. Since sockets are
     operated in stream mode, a single transmission via send
     may not transfer the full data. Therefore, one has to check
     at the receiver side if the full data is received. If not,
     one has to issue several recv() commands.

     The length of the data is determined by a trailing zero.

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   millisec           Timeout in ms

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                      String length

\********************************************************************/
{
   INT i, status;
   DWORD n;
   fd_set readfds;
   struct timeval timeout;

   n = 0;
   memset(buffer, 0, buffer_size);

   do {
      if (millisec > 0) {
         FD_ZERO(&readfds);
         FD_SET(sock, &readfds);

         timeout.tv_sec = millisec / 1000;
         timeout.tv_usec = (millisec % 1000) * 1000;

         do {
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
         } while (status == -1);        /* dont return if an alarm signal was cought */

         if (!FD_ISSET(sock, &readfds))
            break;
      }

      i = recv(sock, buffer + n, 1, 0);

      if (i <= 0)
         break;

      n++;

      if (n >= buffer_size)
         break;

   } while (buffer[n - 1] && buffer[n - 1] != 10);

   return n - 1;
}

/*------------------------------------------------------------------*/
INT recv_tcp(int sock, char *net_buffer, DWORD buffer_size, INT flags)
/********************************************************************\

  Routine: recv_tcp

  Purpose: Receive network data over TCP port. Since sockets are
     operated in stream mode, a single transmission via send
     may not transfer the full data. Therefore, one has to check
     at the receiver side if the full data is received. If not,
     one has to issue several recv() commands.

     The length of the data is determined by the data header,
     which consists of two DWORDs. The first is the command code
     (or function id), the second is the size of the following
     parameters in bytes. From that size recv_tcp() determines
     how much data to receive.

  Input:
    INT   sock               Socket which was previosly opened.
    char  *net_buffer        Buffer to store data to
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to recv()

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                      Same as recv()

\********************************************************************/
{
   INT param_size, n_received, n;
   NET_COMMAND *nc;

   if (buffer_size < sizeof(NET_COMMAND_HEADER)) {
      cm_msg(MERROR, "recv_tcp", "parameters too large for network buffer");
      return -1;
   }

   /* first receive header */
   n_received = 0;
   do {
#ifdef OS_UNIX
      do {
         n = recv(sock, net_buffer + n_received, sizeof(NET_COMMAND_HEADER), flags);

         /* don't return if an alarm signal was cought */
      } while (n == -1 && errno == EINTR);
#else
      n = recv(sock, net_buffer + n_received, sizeof(NET_COMMAND_HEADER), flags);
#endif

      if (n == 0) {
         cm_msg(MERROR, "recv_tcp",
                "header: recv returned %d, n_received = %d, unexpected connection closure", n, n_received);
         return n;
      }

      if (n < 0) {
         cm_msg(MERROR, "recv_tcp",
                "header: recv returned %d, n_received = %d, errno: %d (%s)", n, n_received, errno, strerror(errno));
         return n;
      }

      n_received += n;

   } while (n_received < (int) sizeof(NET_COMMAND_HEADER));

   /* now receive parameters */

   nc = (NET_COMMAND *) net_buffer;
   param_size = nc->header.param_size;
   n_received = 0;

   if (param_size == 0)
      return sizeof(NET_COMMAND_HEADER);

   if (param_size > (INT)buffer_size) {
      cm_msg(MERROR, "recv_tcp", "param: receive buffer size %d is too small for received data size %d", buffer_size, param_size);
      return -1;
   }

   do {
#ifdef OS_UNIX
      do {
         n = recv(sock, net_buffer + sizeof(NET_COMMAND_HEADER) + n_received, param_size - n_received, flags);

         /* don't return if an alarm signal was cought */
      } while (n == -1 && errno == EINTR);
#else
      n = recv(sock, net_buffer + sizeof(NET_COMMAND_HEADER) + n_received, param_size - n_received, flags);
#endif

      if (n == 0) {
         cm_msg(MERROR, "recv_tcp",
                "param: recv returned %d, n_received = %d, unexpected connection closure", n, n_received);
         return n;
      }

      if (n < 0) {
         cm_msg(MERROR, "recv_tcp",
                "param: recv returned %d, n_received = %d, errno: %d (%s)", n, n_received, errno, strerror(errno));
         return n;
      }

      n_received += n;
   } while (n_received < param_size);

   return sizeof(NET_COMMAND_HEADER) + param_size;
}

/*------------------------------------------------------------------*/
INT send_udp(int sock, char *buffer, DWORD buffer_size, INT flags)
/********************************************************************\

  Routine: send_udp

  Purpose: Send network data over UDP port. If buffer_size is small,
     collect several events and send them together. If
     buffer_size is larger than largest datagram size
     NET_UDP_SIZE, split event in several udp buffers and
     send them separately with serial number protection.

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to send()

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                     Same as send()

\********************************************************************/
{
   INT status;
   UDP_HEADER *udp_header;
   static char udp_buffer[NET_UDP_SIZE];
   static INT serial_number = 0, n_received = 0;
   DWORD i, data_size;

   udp_header = (UDP_HEADER *) udp_buffer;
   data_size = NET_UDP_SIZE - sizeof(UDP_HEADER);

   /*
      If buffer size is between half the UPD size and full UDP size,
      send immediately a single packet.
    */
   if (buffer_size >= NET_UDP_SIZE / 2 && buffer_size <= data_size) {
      /*
         If there is any data already in the buffer, send it first.
       */
      if (n_received) {
         udp_header->serial_number = UDP_FIRST | n_received;
         udp_header->sequence_number = ++serial_number;

         send(sock, udp_buffer, n_received + sizeof(UDP_HEADER), flags);
         n_received = 0;
      }

      udp_header->serial_number = UDP_FIRST | buffer_size;
      udp_header->sequence_number = ++serial_number;

      memcpy(udp_header + 1, buffer, buffer_size);
      status = send(sock, udp_buffer, buffer_size + sizeof(UDP_HEADER), flags);
      if (status == (INT) buffer_size + (int) sizeof(UDP_HEADER))
         status -= sizeof(UDP_HEADER);

      return status;
   }

   /*
      If buffer size is smaller than half the UDP size, collect events
      until UDP buffer is optimal filled.
    */
   if (buffer_size <= data_size) {
      /* If udp_buffer has space, just copy it there */
      if (buffer_size + n_received < data_size) {
         memcpy(udp_buffer + sizeof(UDP_HEADER) + n_received, buffer, buffer_size);

         n_received += buffer_size;
         return buffer_size;
      }

      /* If udp_buffer has not enough space, send it */
      udp_header->serial_number = UDP_FIRST | n_received;
      udp_header->sequence_number = ++serial_number;

      status = send(sock, udp_buffer, n_received + sizeof(UDP_HEADER), flags);

      n_received = 0;

      memcpy(udp_header + 1, buffer, buffer_size);
      n_received = buffer_size;

      return buffer_size;
   }

   /*
      If buffer size is larger than UDP size, split event in several
      buffers.
    */

   /* If there is any data already in the buffer, send it first */
   if (n_received) {
      udp_header->serial_number = UDP_FIRST | n_received;
      udp_header->sequence_number = ++serial_number;

      send(sock, udp_buffer, n_received + sizeof(UDP_HEADER), flags);
      n_received = 0;
   }

   for (i = 0; i < ((buffer_size - 1) / data_size); i++) {
      if (i == 0) {
         udp_header->serial_number = UDP_FIRST | buffer_size;
         udp_header->sequence_number = ++serial_number;
      } else {
         udp_header->serial_number = serial_number;
         udp_header->sequence_number = i;
      }

      memcpy(udp_header + 1, buffer + i * data_size, data_size);
      send(sock, udp_buffer, NET_UDP_SIZE, flags);
   }

   /* Send remaining bytes */
   udp_header->serial_number = serial_number;
   udp_header->sequence_number = i;
   memcpy(udp_header + 1, buffer + i * data_size, buffer_size - i * data_size);
   status = send(sock, udp_buffer, sizeof(UDP_HEADER) + buffer_size - i * data_size, flags);
   if ((DWORD) status == sizeof(UDP_HEADER) + buffer_size - i * data_size)
      return buffer_size;

   return status;
}

/*------------------------------------------------------------------*/
INT recv_udp(int sock, char *buffer, DWORD buffer_size, INT flags)
/********************************************************************\

  Routine: recv_udp

  Purpose: Receive network data over UDP port. If received event
     is splitted into several buffers, recombine them checking
     the serial number. If one buffer is missing in a splitted
     event, throw away the whole event.

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to recv()

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                     Same as recv()

\********************************************************************/
{
   INT i, status;
   UDP_HEADER *udp_header;
   char udp_buffer[NET_UDP_SIZE];
   DWORD serial_number, sequence_number, total_buffer_size;
   DWORD data_size, n_received;
   fd_set readfds;
   struct timeval timeout;

   udp_header = (UDP_HEADER *) udp_buffer;
   data_size = NET_UDP_SIZE - sizeof(UDP_HEADER);

   /* Receive the first buffer */
#ifdef OS_UNIX
   do {
      i = recv(sock, udp_buffer, NET_UDP_SIZE, flags);

      /* dont return if an alarm signal was cought */
   } while (i == -1 && errno == EINTR);
#else
   i = recv(sock, udp_buffer, NET_UDP_SIZE, flags);
#endif

 start:

   /* Receive buffers until we get a sequence start */
   while (!(udp_header->serial_number & UDP_FIRST)) {
      /* wait for data with timeout */
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec = 0;
      timeout.tv_usec = 100000; /* 0.1 s */

      do {
         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      } while (status == -1);

      /*
         If we got nothing, return zero so that calling program can do
         other things like checking TCP port for example.
       */
      if (!FD_ISSET(sock, &readfds))
         return 0;

#ifdef OS_UNIX
      do {
         i = recv(sock, udp_buffer, NET_UDP_SIZE, flags);

         /* dont return if an alarm signal was caught */
      } while (i == -1 && errno == EINTR);
#else
      i = recv(sock, udp_buffer, NET_UDP_SIZE, flags);
#endif
   }

   /* if no others are following, return */
   total_buffer_size = udp_header->serial_number & ~UDP_FIRST;
   serial_number = udp_header->sequence_number;
   sequence_number = 0;

   if (total_buffer_size <= data_size) {
      if (buffer_size < total_buffer_size) {
         memcpy(buffer, udp_header + 1, buffer_size);
         return buffer_size;
      } else {
         memcpy(buffer, udp_header + 1, total_buffer_size);
         return total_buffer_size;
      }
   }

   /* if others are following, collect them */
   n_received = data_size;

   if (buffer_size < data_size) {
      memcpy(buffer, udp_header + 1, buffer_size);
      return buffer_size;
   }

   memcpy(buffer, udp_header + 1, data_size);


   do {
      /* wait for new data with timeout */
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      timeout.tv_sec = 0;
      timeout.tv_usec = 100000; /* 0.1 s */

      do {
         status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      } while (status == -1);

      /*
         If we got nothing, return zero so that calling program can do
         other things like checking TCP port for example.
       */
      if (!FD_ISSET(sock, &readfds))
         return 0;

#ifdef OS_UNIX
      do {
         i = recv(sock, udp_buffer, NET_UDP_SIZE, flags);

         /* dont return if an alarm signal was caught */
      } while (i == -1 && errno == EINTR);
#else
      i = recv(sock, udp_buffer, NET_UDP_SIZE, flags);
#endif

      sequence_number++;

      /* check sequence and serial numbers */
      if (udp_header->serial_number != serial_number || udp_header->sequence_number != sequence_number)
         /* lost one, so start again */
         goto start;

      /* copy what we got */
      memcpy(buffer + n_received, udp_header + 1, i - sizeof(UDP_HEADER));

      n_received += (i - sizeof(UDP_HEADER));

   } while (n_received < total_buffer_size);

   return n_received;
}

/*------------------------------------------------------------------*/

#ifdef OS_MSDOS
#ifdef sopen
/********************************************************************\
   under Turbo-C, sopen is defined as a macro instead a function.
   Since the PCTCP library uses sopen as a function call, we supply
   it here.
\********************************************************************/

#undef sopen

int sopen(const char *path, int access, int shflag, int mode)
{
   return open(path, (access) | (shflag), mode);
}

#endif
#endif

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                     Tape functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT ss_tape_open(char *path, INT oflag, INT * channel)
/********************************************************************\

  Routine: ss_tape_open

  Purpose: Open tape channel

  Input:
    char  *path             Name of tape
                            Under Windows NT, usually \\.\tape0
                            Under UNIX, usually /dev/tape
    INT   oflag             Open flags, same as open()

  Output:
    INT   *channel          Channel identifier

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_TAPE              No tape in device
    SS_DEV_BUSY             Device is used by someone else

\********************************************************************/
{
#ifdef OS_UNIX
   cm_enable_watchdog(FALSE);

   *channel = open(path, oflag, 0644);

   cm_enable_watchdog(TRUE);

   if (*channel < 0)
      cm_msg(MERROR, "ss_tape_open", strerror(errno));

   if (*channel < 0) {
      if (errno == EIO)
         return SS_NO_TAPE;
      if (errno == EBUSY)
         return SS_DEV_BUSY;
      return errno;
   }
#ifdef MTSETBLK
   {
   /* set variable block size */
   struct mtop arg;
   arg.mt_op = MTSETBLK;
   arg.mt_count = 0;

   ioctl(*channel, MTIOCTOP, &arg);
   }
#endif                          /* MTSETBLK */

#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;
   TAPE_GET_MEDIA_PARAMETERS m;

   *channel = (INT) CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);

   if (*channel == (INT) INVALID_HANDLE_VALUE) {
      status = GetLastError();
      if (status == ERROR_SHARING_VIOLATION) {
         cm_msg(MERROR, "ss_tape_open", "tape is used by other process");
         return SS_DEV_BUSY;
      }
      if (status == ERROR_FILE_NOT_FOUND) {
         cm_msg(MERROR, "ss_tape_open", "tape device \"%s\" doesn't exist", path);
         return SS_NO_TAPE;
      }

      cm_msg(MERROR, "ss_tape_open", "unknown error %d", status);
      return status;
   }

   status = GetTapeStatus((HANDLE) (*channel));
   if (status == ERROR_NO_MEDIA_IN_DRIVE || status == ERROR_BUS_RESET) {
      cm_msg(MERROR, "ss_tape_open", "no media in drive");
      return SS_NO_TAPE;
   }

   /* set block size */
   memset(&m, 0, sizeof(m));
   m.BlockSize = TAPE_BUFFER_SIZE;
   SetTapeParameters((HANDLE) (*channel), SET_TAPE_MEDIA_INFORMATION, &m);

#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_close(INT channel)
/********************************************************************\

  Routine: ss_tape_close

  Purpose: Close tape channel

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Low level error number

\********************************************************************/
{
   INT status;

#ifdef OS_UNIX

   status = close(channel);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_close", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT

   if (!CloseHandle((HANDLE) channel)) {
      status = GetLastError();
      cm_msg(MERROR, "ss_tape_close", "unknown error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_status(char *path)
/********************************************************************\

  Routine: ss_tape_status

  Purpose: Print status information about tape

  Input:
    char  *path             Name of tape

  Output:
    <print>                 Tape information

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef OS_UNIX
   char str[256];
   /* let 'mt' do the job */
   sprintf(str, "mt -f %s status", path);
   system(str);
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status, channel;
   DWORD size;
   TAPE_GET_MEDIA_PARAMETERS m;
   TAPE_GET_DRIVE_PARAMETERS d;
   double x;

   channel = (INT) CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);

   if (channel == (INT) INVALID_HANDLE_VALUE) {
      status = GetLastError();
      if (status == ERROR_SHARING_VIOLATION) {
         cm_msg(MINFO, "ss_tape_status", "tape is used by other process");
         return SS_SUCCESS;
      }
      if (status == ERROR_FILE_NOT_FOUND) {
         cm_msg(MINFO, "ss_tape_status", "tape device \"%s\" doesn't exist", path);
         return SS_SUCCESS;
      }

      cm_msg(MINFO, "ss_tape_status", "unknown error %d", status);
      return status;
   }

   /* poll media changed messages */
   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);
   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);

   status = GetTapeStatus((HANDLE) channel);
   if (status == ERROR_NO_MEDIA_IN_DRIVE || status == ERROR_BUS_RESET) {
      cm_msg(MINFO, "ss_tape_status", "no media in drive");
      CloseHandle((HANDLE) channel);
      return SS_SUCCESS;
   }

   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);
   GetTapeParameters((HANDLE) channel, GET_TAPE_MEDIA_INFORMATION, &size, &m);

   printf("Hardware error correction is %s\n", d.ECC ? "on" : "off");
   printf("Hardware compression is %s\n", d.Compression ? "on" : "off");
   printf("Tape %s write protected\n", m.WriteProtected ? "is" : "is not");

   if (d.FeaturesLow & TAPE_DRIVE_TAPE_REMAINING) {
      x = ((double) m.Remaining.LowPart + (double) m.Remaining.HighPart * 4.294967295E9)
          / 1024.0 / 1000.0;
      printf("Tape capacity remaining is %d MB\n", (int) x);
   } else
      printf("Tape capacity is not reported by tape\n");

   CloseHandle((HANDLE) channel);

#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_write(INT channel, void *pdata, INT count)
/********************************************************************\

  Routine: ss_tape_write

  Purpose: Write count bytes to tape channel

  Input:
    INT   channel           Channel identifier
    void  *pdata            Address of data to write
    INT   count             number of bytes

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    SS_IO_ERROR             Physical IO error
    SS_TAPE_ERROR           Unknown tape error

\********************************************************************/
{
#ifdef OS_UNIX
   INT status;

   do {
      status = write(channel, pdata, count);
/*
    if (status != count)
      printf("count: %d - %d\n", count, status);
*/
   } while (status == -1 && errno == EINTR);

   if (status != count) {
      cm_msg(MERROR, "ss_tape_write", strerror(errno));

      if (errno == EIO)
         return SS_IO_ERROR;
      else
         return SS_TAPE_ERROR;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;
   DWORD written;

   WriteFile((HANDLE) channel, pdata, count, &written, NULL);
   if (written != (DWORD) count) {
      status = GetLastError();
      cm_msg(MERROR, "ss_tape_write", "error %d", status);

      return SS_IO_ERROR;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_read(INT channel, void *pdata, INT * count)
/********************************************************************\

  Routine: ss_tape_write

  Purpose: Read count bytes to tape channel

  Input:
    INT   channel           Channel identifier
    void  *pdata            Address of data
    INT   *count            Number of bytes to read

  Output:
    INT   *count            Number of read

  Function value:
    SS_SUCCESS              Successful operation
    <errno>                 Error code

\********************************************************************/
{
#ifdef OS_UNIX
   INT n, status;

   do {
      n = read(channel, pdata, *count);
   } while (n == -1 && errno == EINTR);

   if (n == -1) {
      if (errno == ENOSPC || errno == EIO)
         status = SS_END_OF_TAPE;
      else {
         if (n == 0 && errno == 0)
            status = SS_END_OF_FILE;
         else {
            cm_msg(MERROR, "ss_tape_read", "unexpected tape error: n=%d, errno=%d\n", n, errno);
            status = errno;
         }
      }
   } else
      status = SS_SUCCESS;
   *count = n;

   return status;

#elif defined(OS_WINNT)         /* OS_UNIX */

   INT status;
   DWORD read;

   if (!ReadFile((HANDLE) channel, pdata, *count, &read, NULL)) {
      status = GetLastError();
      if (status == ERROR_NO_DATA_DETECTED)
         status = SS_END_OF_TAPE;
      else if (status == ERROR_FILEMARK_DETECTED)
         status = SS_END_OF_FILE;
      else if (status == ERROR_MORE_DATA)
         status = SS_SUCCESS;
      else
         cm_msg(MERROR, "ss_tape_read", "unexpected tape error: n=%d, errno=%d\n", read, status);
   } else
      status = SS_SUCCESS;

   *count = read;
   return status;

#else                           /* OS_WINNT */

   return SS_SUCCESS;

#endif
}

/*------------------------------------------------------------------*/
INT ss_tape_write_eof(INT channel)
/********************************************************************\

  Routine: ss_tape_write_eof

  Purpose: Write end-of-file to tape channel

  Input:
    INT   *channel          Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   arg.mt_op = MTWEOF;
   arg.mt_count = 1;

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_write_eof", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT

   TAPE_GET_DRIVE_PARAMETERS d;
   DWORD size;
   INT status;

   size = sizeof(TAPE_GET_DRIVE_PARAMETERS);
   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);

   if (d.FeaturesHigh & TAPE_DRIVE_WRITE_FILEMARKS)
      status = WriteTapemark((HANDLE) channel, TAPE_FILEMARKS, 1, FALSE);
   else if (d.FeaturesHigh & TAPE_DRIVE_WRITE_LONG_FMKS)
      status = WriteTapemark((HANDLE) channel, TAPE_LONG_FILEMARKS, 1, FALSE);
   else if (d.FeaturesHigh & TAPE_DRIVE_WRITE_SHORT_FMKS)
      status = WriteTapemark((HANDLE) channel, TAPE_SHORT_FILEMARKS, 1, FALSE);
   else
      cm_msg(MERROR, "ss_tape_write_eof", "tape doesn't support writing of filemarks");

   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_write_eof", "unknown error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_fskip(INT channel, INT count)
/********************************************************************\

  Routine: ss_tape_fskip

  Purpose: Skip count number of files on a tape

  Input:
    INT   *channel          Channel identifier
    INT   count             Number of files to skip

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   if (count > 0)
      arg.mt_op = MTFSF;
   else
      arg.mt_op = MTBSF;
   arg.mt_count = abs(count);

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_fskip", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_SPACE_FILEMARKS, 0, (DWORD) count, 0, FALSE);

   if (status == ERROR_END_OF_MEDIA)
      return SS_END_OF_TAPE;

   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_fskip", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_rskip(INT channel, INT count)
/********************************************************************\

  Routine: ss_tape_rskip

  Purpose: Skip count number of records on a tape

  Input:
    INT   *channel          Channel identifier
    INT   count             Number of records to skip

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   if (count > 0)
      arg.mt_op = MTFSR;
   else
      arg.mt_op = MTBSR;
   arg.mt_count = abs(count);

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_rskip", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_SPACE_RELATIVE_BLOCKS, 0, (DWORD) count, 0, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_rskip", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_rewind(INT channel)
/********************************************************************\

  Routine: ss_tape_rewind

  Purpose: Rewind tape

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   arg.mt_op = MTREW;
   arg.mt_count = 0;

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_rewind", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_REWIND, 0, 0, 0, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_rewind", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_spool(INT channel)
/********************************************************************\

  Routine: ss_tape_spool

  Purpose: Spool tape forward to end of recorded data

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

#ifdef MTEOM
   arg.mt_op = MTEOM;
#else
   arg.mt_op = MTSEOD;
#endif
   arg.mt_count = 0;

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_rewind", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_SPACE_END_OF_DATA, 0, 0, 0, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_spool", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_mount(INT channel)
/********************************************************************\

  Routine: ss_tape_mount

  Purpose: Mount tape

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

#ifdef MTLOAD
   arg.mt_op = MTLOAD;
#else
   arg.mt_op = MTNOP;
#endif
   arg.mt_count = 0;

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_mount", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = PrepareTape((HANDLE) channel, TAPE_LOAD, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_mount", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_unmount(INT channel)
/********************************************************************\

  Routine: ss_tape_unmount

  Purpose: Unmount tape

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

#ifdef MTOFFL
   arg.mt_op = MTOFFL;
#else
   arg.mt_op = MTUNLOAD;
#endif
   arg.mt_count = 0;

   cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_unmount", strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = PrepareTape((HANDLE) channel, TAPE_UNLOAD, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_unmount", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_get_blockn(INT channel)
/********************************************************************\
Routine: ss_tape_get_blockn
Purpose: Ask the tape channel for the present block number
Input:
INT   *channel          Channel identifier
Function value:
blockn:  >0 = block number, =0 option not available, <0 errno
\********************************************************************/
{
#if defined(OS_DARWIN)

   return 0;

#elif defined(OS_UNIX)

   INT status;
   struct mtpos arg;

   cm_enable_watchdog(FALSE);
   status = ioctl(channel, MTIOCPOS, &arg);
   cm_enable_watchdog(TRUE);
   if (status < 0) {
      if (errno == EIO)
         return 0;
      else {
         cm_msg(MERROR, "ss_tape_get_blockn", strerror(errno));
         return -errno;
      }
   }
   return (arg.mt_blkno);

#elif defined(OS_WINNT)

   INT status;
   TAPE_GET_MEDIA_PARAMETERS media;
   unsigned long size;
   /* I'm not sure the partition count corresponds to the block count */
   status = GetTapeParameters((HANDLE) channel, GET_TAPE_MEDIA_INFORMATION, &size, &media);
   return (media.PartitionCount);

#endif
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                     Disk functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
double ss_disk_free(char *path)
/********************************************************************\

  Routine: ss_disk_free

  Purpose: Return free disk space

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    doube                   Number of bytes free on disk

\********************************************************************/
{
#ifdef OS_UNIX
#if defined(OS_OSF1)
   struct statfs st;
   statfs(path, &st, sizeof(st));
   return (double) st.f_bavail * st.f_bsize;
#elif defined(OS_LINUX)
   struct statfs st;
   int status;
   status = statfs(path, &st);
   if (status != 0)
      return -1;
   return (double) st.f_bavail * st.f_bsize;
#elif defined(OS_SOLARIS)
   struct statvfs st;
   statvfs(path, &st);
   return (double) st.f_bavail * st.f_bsize;
#elif defined(OS_IRIX)
   struct statfs st;
   statfs(path, &st, sizeof(struct statfs), 0);
   return (double) st.f_bfree * st.f_bsize;
#else
   struct fs_data st;
   statfs(path, &st);
   return (double) st.fd_otsize * st.fd_bfree;
#endif

#elif defined(OS_WINNT)         /* OS_UNIX */
   DWORD SectorsPerCluster;
   DWORD BytesPerSector;
   DWORD NumberOfFreeClusters;
   DWORD TotalNumberOfClusters;
   char str[80];

   strcpy(str, path);
   if (strchr(str, ':') != NULL) {
      *(strchr(str, ':') + 1) = 0;
      strcat(str, DIR_SEPARATOR_STR);
      GetDiskFreeSpace(str, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
   } else
      GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

   return (double) NumberOfFreeClusters *SectorsPerCluster * BytesPerSector;
#else                           /* OS_WINNT */

   return 1e9;

#endif
}

#if defined(OS_ULTRIX) || defined(OS_WINNT)
int fnmatch(const char *pat, const char *str, const int flag)
{
   while (*str != '\0') {
      if (*pat == '*') {
         pat++;
         if ((str = strchr(str, *pat)) == NULL)
            return -1;
      }
      if (*pat == *str) {
         pat++;
         str++;
      } else
         return -1;
   }
   if (*pat == '\0')
      return 0;
   else
      return -1;
}
#endif

#ifdef OS_WINNT
HANDLE pffile;
LPWIN32_FIND_DATA lpfdata;
#endif

INT ss_file_find(char *path, char *pattern, char **plist)
/********************************************************************\

  Routine: ss_file_find

  Purpose: Return list of files matching 'pattern' from the 'path' location

  Input:
    char  *path             Name of a file in file system to check
    char  *pattern          pattern string (wildcard allowed)

  Output:
    char  **plist           pointer to the lfile list

  Function value:
    int                     Number of files matching request

\********************************************************************/
{
   int i;
#ifdef OS_UNIX
   DIR *dir_pointer;
   struct dirent *dp;

   if ((dir_pointer = opendir(path)) == NULL)
      return 0;
   *plist = (char *) malloc(MAX_STRING_LENGTH);
   i = 0;
   for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer)) {
      if (fnmatch(pattern, dp->d_name, 0) == 0 && (dp->d_type == DT_REG || dp->d_type == DT_UNKNOWN)) {
         *plist = (char *) realloc(*plist, (i + 1) * MAX_STRING_LENGTH);
         strncpy(*plist + (i * MAX_STRING_LENGTH), dp->d_name, strlen(dp->d_name));
         *(*plist + (i * MAX_STRING_LENGTH) + strlen(dp->d_name)) = '\0';
         i++;
         seekdir(dir_pointer, telldir(dir_pointer));
      }
   }
   closedir(dir_pointer);
#endif
#ifdef OS_WINNT
   char str[255];
   int first;

   strcpy(str, path);
   strcat(str, "\\");
   strcat(str, pattern);
   first = 1;
   i = 0;
   lpfdata = (WIN32_FIND_DATA *) malloc(sizeof(WIN32_FIND_DATA));
   *plist = (char *) malloc(MAX_STRING_LENGTH);
   pffile = FindFirstFile(str, lpfdata);
   if (pffile == INVALID_HANDLE_VALUE)
      return 0;
   first = 0;
   *plist = (char *) realloc(*plist, (i + 1) * MAX_STRING_LENGTH);
   strncpy(*plist + (i * MAX_STRING_LENGTH), lpfdata->cFileName, strlen(lpfdata->cFileName));
   *(*plist + (i * MAX_STRING_LENGTH) + strlen(lpfdata->cFileName)) = '\0';
   i++;
   while (FindNextFile(pffile, lpfdata)) {
      *plist = (char *) realloc(*plist, (i + 1) * MAX_STRING_LENGTH);
      strncpy(*plist + (i * MAX_STRING_LENGTH), lpfdata->cFileName, strlen(lpfdata->cFileName));
      *(*plist + (i * MAX_STRING_LENGTH) + strlen(lpfdata->cFileName)) = '\0';
      i++;
   }
   free(lpfdata);
#endif
   return i;
}

INT ss_dir_find(char *path, char *pattern, char **plist)
/********************************************************************\
 
 Routine: ss_dir_find
 
 Purpose: Return list of direcories matching 'pattern' from the 'path' location
 
 Input:
 char  *path             Name of a file in file system to check
 char  *pattern          pattern string (wildcard allowed)
 
 Output:
 char  **plist           pointer to the lfile list
 
 Function value:
 int                     Number of files matching request
 
 \********************************************************************/
{
   int i;
#ifdef OS_UNIX
   DIR *dir_pointer;
   struct dirent *dp;
   
   if ((dir_pointer = opendir(path)) == NULL)
      return 0;
   *plist = (char *) malloc(MAX_STRING_LENGTH);
   i = 0;
   for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer)) {
      if (fnmatch(pattern, dp->d_name, 0) == 0 && dp->d_type == DT_DIR) {
         *plist = (char *) realloc(*plist, (i + 1) * MAX_STRING_LENGTH);
         strncpy(*plist + (i * MAX_STRING_LENGTH), dp->d_name, strlen(dp->d_name));
         *(*plist + (i * MAX_STRING_LENGTH) + strlen(dp->d_name)) = '\0';
         i++;
         seekdir(dir_pointer, telldir(dir_pointer));
      }
   }
   closedir(dir_pointer);
#endif
#ifdef OS_WINNT
   char str[255];
   int first;
   
   strcpy(str, path);
   strcat(str, "\\");
   strcat(str, pattern);
   first = 1;
   i = 0;
   lpfdata = (WIN32_FIND_DATA *) malloc(sizeof(WIN32_FIND_DATA));
   *plist = (char *) malloc(MAX_STRING_LENGTH);
   pffile = FindFirstFile(str, lpfdata);
   if (pffile == INVALID_HANDLE_VALUE)
      return 0;
   first = 0;
   *plist = (char *) realloc(*plist, (i + 1) * MAX_STRING_LENGTH);
   strncpy(*plist + (i * MAX_STRING_LENGTH), lpfdata->cFileName, strlen(lpfdata->cFileName));
   *(*plist + (i * MAX_STRING_LENGTH) + strlen(lpfdata->cFileName)) = '\0';
   i++;
   while (FindNextFile(pffile, lpfdata)) {
      *plist = (char *) realloc(*plist, (i + 1) * MAX_STRING_LENGTH);
      strncpy(*plist + (i * MAX_STRING_LENGTH), lpfdata->cFileName, strlen(lpfdata->cFileName));
      *(*plist + (i * MAX_STRING_LENGTH) + strlen(lpfdata->cFileName)) = '\0';
      i++;
   }
   free(lpfdata);
#endif
   return i;
}

INT ss_file_remove(char *path)
/********************************************************************\

  Routine: ss_file_remove

  Purpose: remove (delete) file given through the path

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    int                     function error 0= ok, -1 check errno

\********************************************************************/
{
   return remove(path);
}

double ss_file_size(char *path)
/********************************************************************\

  Routine: ss_file_size

  Purpose: Return file size in bytes for the given path

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    double                     File size

\********************************************************************/
{
#ifdef _LARGEFILE64_SOURCE
   struct stat64 stat_buf;
   int status;

   /* allocate buffer with file size */
   status = stat64(path, &stat_buf);
   if (status != 0)
      return -1;
   return (double) stat_buf.st_size;
#else
   struct stat stat_buf;
   int status;

   /* allocate buffer with file size */
   status = stat(path, &stat_buf);
   if (status != 0)
      return -1;
   return (double) stat_buf.st_size;
#endif
}

double ss_disk_size(char *path)
/********************************************************************\

  Routine: ss_disk_size

  Purpose: Return full disk space

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    doube                   Number of bytes free on disk

\********************************************************************/
{
#ifdef OS_UNIX
#if defined(OS_OSF1)
   struct statfs st;
   statfs(path, &st, sizeof(st));
   return (double) st.f_blocks * st.f_fsize;
#elif defined(OS_LINUX)
   int status;
   struct statfs st;
   status = statfs(path, &st);
   if (status != 0)
      return -1;
   return (double) st.f_blocks * st.f_bsize;
#elif defined(OS_SOLARIS)
   struct statvfs st;
   statvfs(path, &st);
   if (st.f_frsize > 0)
      return (double) st.f_blocks * st.f_frsize;
   else
      return (double) st.f_blocks * st.f_bsize;
#elif defined(OS_ULTRIX)
   struct fs_data st;
   statfs(path, &st);
   return (double) st.fd_btot * 1024;
#elif defined(OS_IRIX)
   struct statfs st;
   statfs(path, &st, sizeof(struct statfs), 0);
   return (double) st.f_blocks * st.f_bsize;
#else
#error ss_disk_size not defined for this OS
#endif
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   DWORD SectorsPerCluster;
   DWORD BytesPerSector;
   DWORD NumberOfFreeClusters;
   DWORD TotalNumberOfClusters;
   char str[80];

   strcpy(str, path);
   if (strchr(str, ':') != NULL) {
      *(strchr(str, ':') + 1) = 0;
      strcat(str, DIR_SEPARATOR_STR);
      GetDiskFreeSpace(str, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
   } else
      GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

   return (double) TotalNumberOfClusters *SectorsPerCluster * BytesPerSector;
#endif                          /* OS_WINNT */

   return 1e9;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Screen  functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
void ss_clear_screen()
/********************************************************************\

  Routine: ss_clear_screen

  Purpose: Clear the screen

  Input:
    <none>

  Output:
    <none>

  Function value:
    <none>

\********************************************************************/
{
#ifdef OS_WINNT

   HANDLE hConsole;
   COORD coordScreen = { 0, 0 };        /* here's where we'll home the cursor */
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi;     /* to get buffer info */
   DWORD dwConSize;             /* number of character cells in the current buffer */

   hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

   /* get the number of character cells in the current buffer */
   bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

   /* fill the entire screen with blanks */
   bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);

   /* put the cursor at (0, 0) */
   bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
   return;

#endif                          /* OS_WINNT */
#if defined(OS_UNIX) || defined(OS_VXWORKS) || defined(OS_VMS)
   printf("\033[2J");
#endif
#ifdef OS_MSDOS
   clrscr();
#endif
}

/*------------------------------------------------------------------*/
void ss_set_screen_size(int x, int y)
/********************************************************************\

  Routine: ss_set_screen_size

  Purpose: Set the screen size in character cells

  Input:
    <none>

  Output:
    <none>

  Function value:
    <none>

\********************************************************************/
{
#ifdef OS_WINNT

   HANDLE hConsole;
   COORD coordSize;

   coordSize.X = (short) x;
   coordSize.Y = (short) y;
   hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleScreenBufferSize(hConsole, coordSize);

#else                           /* OS_WINNT */
   int i;
   i = x;                       /* avoid compiler warning */
   i = y;
#endif
}

/*------------------------------------------------------------------*/
void ss_printf(INT x, INT y, const char *format, ...)
/********************************************************************\

  Routine: ss_printf

  Purpose: Print string at given cursor position

  Input:
    INT   x,y               Cursor position, starting from zero,
          x=0 and y=0 left upper corner

    char  *format           Format string for printf
    ...                     Arguments for printf

  Output:
    <none>

  Function value:
    <none>

\********************************************************************/
{
   char str[256];
   va_list argptr;

   va_start(argptr, format);
   vsprintf(str, (char *) format, argptr);
   va_end(argptr);

#ifdef OS_WINNT
   {
      HANDLE hConsole;
      COORD dwWriteCoord;
      DWORD cCharsWritten;

      hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

      dwWriteCoord.X = (short) x;
      dwWriteCoord.Y = (short) y;

      WriteConsoleOutputCharacter(hConsole, str, strlen(str), dwWriteCoord, &cCharsWritten);
   }

#endif                          /* OS_WINNT */

#if defined(OS_UNIX) || defined(OS_VXWORKS) || defined(OS_VMS)
   printf("\033[%1d;%1dH", y + 1, x + 1);
   printf("%s", str);
   fflush(stdout);
#endif

#ifdef OS_MSDOS
   gotoxy(x + 1, y + 1);
   cputs(str);
#endif
}

/*------------------------------------------------------------------*/
char *ss_getpass(char *prompt)
/********************************************************************\

  Routine: ss_getpass

  Purpose: Read password without echoing it at the screen

  Input:
    char   *prompt    Prompt string

  Output:
    <none>

  Function value:
    char*             Pointer to password

\********************************************************************/
{
   static char password[32];

   printf("%s", prompt);
   memset(password, 0, sizeof(password));

#ifdef OS_UNIX
   return (char *) getpass("");
#elif defined(OS_WINNT)
   {
      HANDLE hConsole;
      DWORD nCharsRead;

      hConsole = GetStdHandle(STD_INPUT_HANDLE);
      SetConsoleMode(hConsole, ENABLE_LINE_INPUT);
      ReadConsole(hConsole, password, sizeof(password), &nCharsRead, NULL);
      SetConsoleMode(hConsole, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
      printf("\n");

      if (password[strlen(password) - 1] == '\r')
         password[strlen(password) - 1] = 0;

      return password;
   }
#elif defined(OS_MSDOS)
   {
      char c, *ptr;

      ptr = password;
      while ((c = getchar()) != EOF && c != '\n')
         *ptr++ = c;
      *ptr = 0;

      printf("\n");
      return password;
   }
#else
   {
      ss_gets(password, 32);
      return password;
   }
#endif
}

/*------------------------------------------------------------------*/
INT ss_getchar(BOOL reset)
/********************************************************************\

  Routine: ss_getchar

  Purpose: Read a single character

  Input:
    BOOL   reset            Reset terminal to standard mode

  Output:
    <none>

  Function value:
    int             0       for no character available
                    CH_xxs  for special character
                    n       ASCII code for normal character
                    -1      function not available on this OS

\********************************************************************/
{
#ifdef OS_UNIX

   static BOOL init = FALSE;
   static struct termios save_termios;
   struct termios buf;
   int i, fd;
   char c[3];

   if (_daemon_flag)
      return 0;

   fd = fileno(stdin);

   if (reset) {
      if (init)
         tcsetattr(fd, TCSAFLUSH, &save_termios);
      init = FALSE;
      return 0;
   }

   if (!init) {
      tcgetattr(fd, &save_termios);
      memcpy(&buf, &save_termios, sizeof(buf));

      buf.c_lflag &= ~(ECHO | ICANON | IEXTEN);

      buf.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON);

      buf.c_cflag &= ~(CSIZE | PARENB);
      buf.c_cflag |= CS8;
      /* buf.c_oflag &= ~(OPOST); */
      buf.c_cc[VMIN] = 0;
      buf.c_cc[VTIME] = 0;

      tcsetattr(fd, TCSAFLUSH, &buf);
      init = TRUE;
   }

   memset(c, 0, 3);
   i = read(fd, c, 1);

   if (i == 0)
      return 0;

   /* check if ESC */
   if (c[0] == 27) {
      i = read(fd, c, 2);
      if (i == 0)               /* return if only ESC */
         return 27;

      /* cursor keys return 2 chars, others 3 chars */
      if (c[1] < 65)
         read(fd, c, 1);

      /* convert ESC sequence to CH_xxx */
      switch (c[1]) {
      case 49:
         return CH_HOME;
      case 50:
         return CH_INSERT;
      case 51:
         return CH_DELETE;
      case 52:
         return CH_END;
      case 53:
         return CH_PUP;
      case 54:
         return CH_PDOWN;
      case 65:
         return CH_UP;
      case 66:
         return CH_DOWN;
      case 67:
         return CH_RIGHT;
      case 68:
         return CH_LEFT;
      }
   }

   /* BS/DEL -> BS */
   if (c[0] == 127)
      return CH_BS;

   return c[0];

#elif defined(OS_WINNT)

   static BOOL init = FALSE;
   static INT repeat_count = 0;
   static INT repeat_char;
   HANDLE hConsole;
   DWORD nCharsRead;
   INPUT_RECORD ir;
   OSVERSIONINFO vi;

   /* find out if we are under W95 */
   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT) {
      /* under W95, console doesn't work properly */
      int c;

      if (!kbhit())
         return 0;

      c = getch();
      if (c == 224) {
         c = getch();
         switch (c) {
         case 71:
            return CH_HOME;
         case 72:
            return CH_UP;
         case 73:
            return CH_PUP;
         case 75:
            return CH_LEFT;
         case 77:
            return CH_RIGHT;
         case 79:
            return CH_END;
         case 80:
            return CH_DOWN;
         case 81:
            return CH_PDOWN;
         case 82:
            return CH_INSERT;
         case 83:
            return CH_DELETE;
         }
      }
      return c;
   }

   hConsole = GetStdHandle(STD_INPUT_HANDLE);

   if (reset) {
      SetConsoleMode(hConsole, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
      init = FALSE;
      return 0;
   }

   if (!init) {
      SetConsoleMode(hConsole, ENABLE_PROCESSED_INPUT);
      init = TRUE;
   }

   if (repeat_count) {
      repeat_count--;
      return repeat_char;
   }

   PeekConsoleInput(hConsole, &ir, 1, &nCharsRead);

   if (nCharsRead == 0)
      return 0;

   ReadConsoleInput(hConsole, &ir, 1, &nCharsRead);

   if (ir.EventType != KEY_EVENT)
      return ss_getchar(0);

   if (!ir.Event.KeyEvent.bKeyDown)
      return ss_getchar(0);

   if (ir.Event.KeyEvent.wRepeatCount > 1) {
      repeat_count = ir.Event.KeyEvent.wRepeatCount - 1;
      repeat_char = ir.Event.KeyEvent.uChar.AsciiChar;
      return repeat_char;
   }

   if (ir.Event.KeyEvent.uChar.AsciiChar)
      return ir.Event.KeyEvent.uChar.AsciiChar;

   if (ir.Event.KeyEvent.dwControlKeyState & (ENHANCED_KEY)) {
      switch (ir.Event.KeyEvent.wVirtualKeyCode) {
      case 33:
         return CH_PUP;
      case 34:
         return CH_PDOWN;
      case 35:
         return CH_END;
      case 36:
         return CH_HOME;
      case 37:
         return CH_LEFT;
      case 38:
         return CH_UP;
      case 39:
         return CH_RIGHT;
      case 40:
         return CH_DOWN;
      case 45:
         return CH_INSERT;
      case 46:
         return CH_DELETE;
      }

      return ir.Event.KeyEvent.wVirtualKeyCode;
   }

   return ss_getchar(0);

#elif defined(OS_MSDOS)

   int c;

   if (!kbhit())
      return 0;

   c = getch();
   if (!c) {
      c = getch();
      switch (c) {
      case 71:
         return CH_HOME;
      case 72:
         return CH_UP;
      case 73:
         return CH_PUP;
      case 75:
         return CH_LEFT;
      case 77:
         return CH_RIGHT;
      case 79:
         return CH_END;
      case 80:
         return CH_DOWN;
      case 81:
         return CH_PDOWN;
      case 82:
         return CH_INSERT;
      case 83:
         return CH_DELETE;
      }
   }
   return c;

#else
   return -1;
#endif
}

/*------------------------------------------------------------------*/
char *ss_gets(char *string, int size)
/********************************************************************\

  Routine: ss_gets

  Purpose: Read a line from standard input. Strip trailing new line
           character. Return in a loop so that it cannot be interrupted
           by an alarm() signal (like under Sun Solaris)

  Input:
    INT    size             Size of string

  Output:
    BOOL   string           Return string

  Function value:
    char                    Return string

\********************************************************************/
{
   char *p;

   do {
      p = fgets(string, size, stdin);
   } while (p == NULL);


   if (strlen(p) > 0 && p[strlen(p) - 1] == '\n')
      p[strlen(p) - 1] = 0;

   return p;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Direct IO functions                               *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT ss_directio_give_port(INT start, INT end)
{
#ifdef OS_WINNT

   /* under Windows NT, use DirectIO driver to open ports */

   OSVERSIONINFO vi;
   HANDLE hdio = 0;
   DWORD buffer[] = { 6, 0, 0, 0 };
   DWORD size;

   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* use DirectIO driver under NT to gain port access */
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hdio == INVALID_HANDLE_VALUE) {
         printf("hyt1331.c: Cannot access IO ports (No DirectIO driver installed)\n");
         return -1;
      }

      /* open ports */
      buffer[1] = start;
      buffer[2] = end;
      if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), NULL, 0, &size, NULL))
         return -1;
   }

   return SS_SUCCESS;
#else
   int i;
   i = start;                   /* avoid compiler warning */
   i = end;
   return SS_SUCCESS;
#endif
}

/*------------------------------------------------------------------*/
INT ss_directio_lock_port(INT start, INT end)
{
#ifdef OS_WINNT

   /* under Windows NT, use DirectIO driver to lock ports */

   OSVERSIONINFO vi;
   HANDLE hdio;
   DWORD buffer[] = { 7, 0, 0, 0 };
   DWORD size;

   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* use DirectIO driver under NT to gain port access */
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hdio == INVALID_HANDLE_VALUE) {
         printf("hyt1331.c: Cannot access IO ports (No DirectIO driver installed)\n");
         return -1;
      }

      /* lock ports */
      buffer[1] = start;
      buffer[2] = end;
      if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), NULL, 0, &size, NULL))
         return -1;
   }

   return SS_SUCCESS;
#else
   int i;
   i = start;                   /* avoid compiler warning */
   i = end;
   return SS_SUCCESS;
#endif
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  System logging                                    *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT ss_syslog(const char *message)
/********************************************************************\

  Routine: ss_syslog

  Purpose: Write a message to the system logging facility

  Input:
    char   format  Same as for printf

  Output:
    <none>

  Function value:
    SS_SUCCESS     Successful completion

\********************************************************************/
{
#ifdef OS_UNIX
   static BOOL init = FALSE;

   if (!init) {
#ifdef OS_ULTRIX
      openlog("MIDAS", LOG_PID);
#else
      openlog("MIDAS", LOG_PID, LOG_USER);
#endif
      init = TRUE;
   }

   syslog(LOG_DEBUG, message, 0);
   return SS_SUCCESS;
#elif defined(OS_WINNT)         /* OS_UNIX */
/*
HANDLE hlog = 0;
const char *pstr[2];

  if (!hlog)
    {
    HKEY  hk;
    DWORD d;
    char  str[80];

    RegCreateKey(HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Midas", &hk);

    strcpy(str, (char *) rpc_get_server_option(RPC_OSERVER_NAME));
    RegSetValueEx(hk, "EventMessageFile", 0, REG_EXPAND_SZ, (LPBYTE) str, strlen(str) + 1);

    d = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
        EVENTLOG_INFORMATION_TYPE;
    RegSetValueEx(hk, "TypesSupported", 0, REG_DWORD, (LPBYTE) &d, sizeof(DWORD));
    RegCloseKey(hk);

    hlog = RegisterEventSource(NULL, "Midas");
    }

  pstr[0] = message;
  pstr[1] = NULL;

  if (hlog)
    ReportEvent(hlog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, pstr, NULL);
*/
   return SS_SUCCESS;

#else                           /* OS_WINNT */

   return SS_SUCCESS;

#endif
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Encryption                                        *
*                                                                    *
\********************************************************************/

#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

char *ss_crypt(const char *buf, const char *salt)
/********************************************************************\

  Routine: ss_crypt

  Purpose: Simple fake of UNIX crypt(3) function, until we get
           a better one

  Input:
    char   *buf             Plain password
    char   *slalt           Two random characters
                            events. Can be used to skip events

  Output:
    <none>

  Function value:
    char*                   Encrypted password

\********************************************************************/
{
   int i, seed;
   static char enc_pw[13];

   memset(enc_pw, 0, sizeof(enc_pw));
   enc_pw[0] = salt[0];
   enc_pw[1] = salt[1];

   for (i = 0; i < 8 && buf[i]; i++)
      enc_pw[i + 2] = buf[i];
   for (; i < 8; i++)
      enc_pw[i + 2] = 0;

   seed = 123;
   for (i = 2; i < 13; i++) {
      seed = 5 * seed + 27 + enc_pw[i];
      enc_pw[i] = (char) bin_to_ascii(seed & 0x3F);
   }

   return enc_pw;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  NaN's                                             *
*                                                                    *
\********************************************************************/

double ss_nan()
{
   double nan;

   nan = 0;
   nan = 0 / nan;
   return nan;
}

#ifdef OS_WINNT
#include <float.h>
#ifndef isnan
#define isnan(x) _isnan(x)
#endif
#ifndef finite
#define finite(x) _finite(x)
#endif
#elif defined(OS_LINUX)
#include <math.h>
#endif

int ss_isnan(double x)
{
   return isnan(x);
}

int ss_isfin(double x)
{
   return finite(x);
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Stack Trace                                       *
*                                                                    *
\********************************************************************/

#ifdef OS_LINUX
#include <execinfo.h>
#endif

#define N_STACK_HISTORY 500
char stack_history[N_STACK_HISTORY][80];
int stack_history_pointer = -1;

INT ss_stack_get(char ***string)
{
#ifdef OS_LINUX
#define MAX_STACK_DEPTH 16

   void *trace[MAX_STACK_DEPTH];
   int size;

   size = backtrace(trace, MAX_STACK_DEPTH);
   *string = backtrace_symbols(trace, size);
   return size;
#else
   return 0;
#endif
}

void ss_stack_print()
{
   char **string;
   int i, n;

   n = ss_stack_get(&string);
   for (i = 0; i < n; i++)
      printf("%s\n", string[i]);
   if (n > 0)
      free(string);
}

void ss_stack_history_entry(char *tag)
{
   char **string;
   int i, n;

   if (stack_history_pointer == -1) {
      stack_history_pointer++;
      memset(stack_history, 0, sizeof(stack_history));
   }
   strlcpy(stack_history[stack_history_pointer], tag, 80);
   stack_history_pointer = (stack_history_pointer + 1) % N_STACK_HISTORY;
   n = ss_stack_get(&string);
   for (i = 2; i < n; i++) {
      strlcpy(stack_history[stack_history_pointer], string[i], 80);
      stack_history_pointer = (stack_history_pointer + 1) % N_STACK_HISTORY;
   }
   free(string);

   strlcpy(stack_history[stack_history_pointer], "=========================", 80);
   stack_history_pointer = (stack_history_pointer + 1) % N_STACK_HISTORY;
}

void ss_stack_history_dump(char *filename)
{
   FILE *f;
   int i, j;

   f = fopen(filename, "wt");
   if (f != NULL) {
      j = stack_history_pointer;
      for (i = 0; i < N_STACK_HISTORY; i++) {
         if (strlen(stack_history[j]) > 0)
            fprintf(f, "%s\n", stack_history[j]);
         j = (j + 1) % N_STACK_HISTORY;
      }
      fclose(f);
      printf("Stack dump written to %s\n", filename);
   } else
      printf("Cannot open %s: errno=%d\n", filename, errno);
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

         /** @} *//* end of msfunctionc */
         /** @} *//* end of msystemincludecode */
