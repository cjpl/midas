#include <stdio.h>

#include "midas.h"
#include "msystem.h"

#define FOUND 1
#define NOT_FOUND 0
#define MAX_NAME 256

#ifdef OS_ULTRIX
int fnmatch (const char * pat, const char * str, const int flag)
{
  while (*str != '\0')
    {
      if (*pat == '*')
	{
	  pat++;
	  if ((str = strchr(str ,*pat)) == NULL)
	    return -1;
	}
      if (*pat == *str)
	{
	  pat++;
	  str++;
	}
      else
	return -1;
    }
  if (*pat == '\0')
    return 0;
  else
    return -1;
}
#endif

int ss_file_find(char * path, char * pattern, char **plist)
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
  DIR *dir_pointer;
  struct dirent *dp;
  
  if ((dir_pointer = opendir(path)) == NULL)
    return 0;
  *plist = (char *) malloc(MAX_NAME);
  i = 0;
  for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer))
    {
      if (fnmatch (pattern, dp->d_name, 0) == 0)
	{
	  *plist = (char *)realloc(*plist, (i+1)*MAX_NAME);
	  strncpy(*plist+(i*MAX_NAME), dp->d_name, strlen(dp->d_name)); 
	  i++;
	  seekdir(dir_pointer, telldir(dir_pointer));
	}
    }
  closedir(dir_pointer); 
  return i;
}

int ss_file_remove(char * path)
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

double ss_file_size(char * path)
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
  struct stat stat_buf;

  /* allocate buffer with file size */
  stat(path, &stat_buf);
  return (double) stat_buf.st_size;
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
  struct statfs st;
  statfs(path, &st);
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
#else
#error ss_disk_size not defined for this OS 
#endif
#endif /* OS_UNIX */
  
#ifdef OS_WINNT
  DWORD  SectorsPerCluster;
  DWORD  BytesPerSector;
  DWORD  NumberOfFreeClusters;
  DWORD  TotalNumberOfClusters;
  char   str[80];
  
  strcpy(str, path);
  if (strchr(str, ':') != NULL)
    {
      *(strchr(str, ':')+1) = 0;
      strcat(str, DIR_SEPARATOR_STR);
      GetDiskFreeSpace(str, &SectorsPerCluster, &BytesPerSector,
		       &NumberOfFreeClusters, &TotalNumberOfClusters);
    }
  else
    GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
                     &NumberOfFreeClusters, &TotalNumberOfClusters);
  
  return (double) TotalNumberOfClusters * SectorsPerCluster * BytesPerSector;
#endif /* OS_WINNT */
  
  return 1e9;
}                 

main(unsigned int argc,char **argv)
{
  char *list;
  char str[256];
  int i, status;
  double size;

  status = ss_file_find("./", "*.c", &list);
  printf ("status :%i \n",status);
  for (i=0;i<status;i++)
    {
      printf ("list[%i]:%s\n",i, list+i*MAX_NAME);
    }

  sprintf(str, "./%s", list);
  free (list);
  size = ss_file_size(str);
  printf("file :%s of size : %1.0lf\n",str, size);
  size = ss_disk_size("/usr");
  printf("/usr size : %1.0lf\n",size);
  printf ("%i\n",ss_file_remove ("junk.txt"));
}
