/********************************************************************\

  Name:         mserver.c
  Created by:   Stefan Ritt

  Contents:     Server program for midas RPC calls

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"

#ifdef OS_UNIX
#include <grp.h>
#include <sys/types.h>
#endif

#ifdef OS_WINNT
/* critical section object for open/close buffer */
CRITICAL_SECTION buffer_critial_section;
#endif

struct callback_addr callback;

BOOL use_callback_addr = TRUE;

INT rpc_server_dispatch(INT index, void *prpc_param[]);

/*---- msg_print ---------------------------------------------------*/

INT msg_print(const char *msg)
{
   /* print message to system log */
   ss_syslog(msg);

   /* print message to stdout */
   return puts(msg);
}

/*---- debug_print -------------------------------------------------*/

void debug_print(char *msg)
{
   FILE *f;
   char file_name[256], str[1000];
   static char client_name[NAME_LENGTH] = "";
   static DWORD start_time = 0;

   if (!start_time)
      start_time = ss_millitime();

   /* print message to file */
#ifdef OS_LINUX
   strlcpy(file_name, "/tmp/mserver.log", sizeof(file_name));
#else
   getcwd(file_name, sizeof(file_name));
   if (file_name[strlen(file_name) - 1] != DIR_SEPARATOR)
      strlcat(file_name, DIR_SEPARATOR_STR, sizeof(file_name));
   strlcat(file_name, "mserver.log", sizeof(file_name));
#endif
   f = fopen(file_name, "a");

   if (!client_name[0])
      cm_get_client_info(client_name);

   sprintf(str, "%10.3lf [%d,%s,%s] ", (ss_millitime() - start_time) / 1000.0,
           ss_getpid(), callback.host_name, client_name);
   strlcat(str, msg, sizeof(str));
   strlcat(str, "\n", sizeof(str));

   if (f != NULL) {
      fputs(str, f);
      fclose(f);
   } else {
      printf("Cannot open \"%s\": %s\n", file_name, strerror(errno));
   }
}

/*---- main --------------------------------------------------------*/

int main(int argc, char **argv)
/********************************************************************\

  Routine: main (server.exe)

  Purpose: Main routine for MIDAS server process. If called one
           parameter, it listens for a connection under
           MIDAS_TCP_PORT. If a connection is requested, the action
           depends on the parameter:

           0: Single process server: This process executes RPC calls
              even if there are several connections.

           1: Multi thread server (only Windows NT): For each conn-
              ection, a seperate thread is started which servers this
              connection.

           2: Multi process server (Not MS-DOS):  For each conn-
              ection, a seperate subprocess is started which servers 
              this connection. The subprocess is again server.exe, now
              with four parameters: the IP address of the calling 
              host, the two port numbers used by the calling
              host and optionally the name of the experiment to connect
              to. With this information it calles back the client.

           This technique is necessary since the usual fork() doesn't
           work on some of the supported systems.

  Input:
    int  argc               Number of parameters in command line
    char **argv             Command line parameters:

                            ip-addr     callback address as longword
                            port-no     port number
                            program     program name as string
                            experiment  experiment name as string


  Output:
    none

  Function value:
    BM_SUCCESS              Successful completion

\********************************************************************/
{
   int i, flag, size, server_type;
   char name[256], str[1000];
   BOOL inetd, daemon, debug;
   int port = MIDAS_TCP_PORT;

#ifdef OS_WINNT
   /* init critical section object for open/close buffer */
   InitializeCriticalSection(&buffer_critial_section);
#endif

#if defined(SIGPIPE) && defined(SIG_IGN)
   signal(SIGPIPE, SIG_IGN);
#endif

   setbuf(stdout, NULL);
   setbuf(stderr, NULL);

   /* save executable file name */
   if (argv[0] == NULL || argv[0][0] == 0)
     strlcpy(name, "mserver", sizeof(name));
   else
     strlcpy(name, argv[0], sizeof(name));

#ifdef OS_UNIX
   /* if no full path given, assume /usr/local/bin */
   if (strchr(name, '/') == 0) {
      strlcpy(str, "/usr/local/bin/", sizeof(str));
      strlcat(str, name, sizeof(str));
      strlcpy(name, str, sizeof(name));
   }
#endif

#if 0
   printf("mserver main, name [%s]\n", name);
   for (i=0; i<=argc; i++)
      printf("argv[%d] is [%s]\n", i, argv[i]);
   system("/bin/ls -la /proc/self/fd");
#endif

   if (getenv("MIDAS_MSERVER_DO_NOT_USE_CALLBACK_ADDR"))
      use_callback_addr = FALSE;

   rpc_set_server_option(RPC_OSERVER_NAME, (POINTER_T) name);

   /* redirect message print */
   cm_set_msg_print(MT_ALL, MT_ALL, msg_print);

   /* find out if we were started by inetd */
   size = sizeof(int);
   inetd = (getsockopt(0, SOL_SOCKET, SO_TYPE, (void *) &flag, (void *) &size) == 0);

   /* check for debug flag */
   debug = FALSE;
   for (i = 1; i < argc; i++)
      if (argv[i][0] == '-' && argv[i][1] == 'd')
         debug = TRUE;

   if (debug) {
      debug_print("mserver startup");
      for (i = 0; i < argc; i++)
         debug_print(argv[i]);
      rpc_set_debug(debug_print, 1);
   }

   if (argc < 7 && inetd) {
      /* accept connection from stdin */
      rpc_set_server_option(RPC_OSERVER_TYPE, ST_MPROCESS);
      rpc_server_accept(0);

      return 0;
   }

   if (!inetd && argc < 7)
      printf("%s started interactively\n", argv[0]);

   debug = daemon = FALSE;
   server_type = ST_MPROCESS;

   if (argc < 7) {
      /* parse command line parameters */
      for (i = 1; i < argc; i++) {
         if (argv[i][0] == '-' && argv[i][1] == 'd')
            debug = TRUE;
         else if (argv[i][0] == '-' && argv[i][1] == 'D')
            daemon = TRUE;
         else if (argv[i][0] == '-' && argv[i][1] == 's')
            server_type = ST_SINGLE;
         else if (argv[i][0] == '-' && argv[i][1] == 't')
            server_type = ST_MTHREAD;
         else if (argv[i][0] == '-' && argv[i][1] == 'm')
            server_type = ST_MPROCESS;
         else if (argv[i][0] == '-' && argv[i][1] == 'p')
            port = strtoul(argv[++i], NULL, 0);
         else if (argv[i][1] == 'a')
            rpc_add_allowed_host(argv[++i]);
         else if (argv[i][0] == '-') {
            if (i + 1 >= argc || argv[i + 1][0] == '-')
               goto usage;
            else {
             usage:
               printf("usage: mserver [-s][-t][-m][-d][-p port][-a hostname]\n");
               printf("               -s    Single process server (DO NOT USE!)\n");
               printf("               -t    Multi threaded server (DO NOT USE!)\n");
               printf("               -m    Multi process server (default)\n");
               printf("               -p port Listen for connections on non-default tcp port\n");
#ifdef OS_LINUX
               printf("               -D    Become a daemon\n");
               printf("               -d    Write debug info to stdout or to \"/tmp/mserver.log\"\n\n");
#else
               printf("               -d    Write debug info\"\n\n");
#endif
               printf("               -a hostname Only allow access for specified hosts\n");
               return 0;
            }
         }
      }

      /* turn on debugging */
      if (debug) {
         if (daemon || inetd)
            rpc_set_debug(debug_print, 1);
         else
            rpc_set_debug((void (*)(char *)) puts, 1);

         sprintf(str, "Arguments: ");
         for (i = 0; i < argc; i++)
            sprintf(str + strlen(str), " %s", argv[i]);
         rpc_debug_printf(str);

         rpc_debug_printf("Debugging mode is on");
      }

      /* if command line parameter given, start according server type */
      if (server_type == ST_MTHREAD) {
         if (ss_thread_create(NULL, NULL) == 0) {
            printf("MIDAS doesn't support threads on this OS.\n");
            return 0;
         }

         printf("NOTE: THE MULTI THREADED SERVER IS BUGGY, ONLY USE IT FOR TEST PURPOSES\n");
         printf("Multi thread server started\n");
      }

      /* become a daemon */
      if (daemon) {
         printf("Becoming a daemon...\n");
         ss_daemon_init(FALSE);
      }

      /* register server */
      if (rpc_register_server(server_type, argv[0], &port, rpc_server_dispatch) != RPC_SUCCESS) {
         printf("Cannot start server\n");
         return 0;
      }

      /* register MIDAS library functions */
      rpc_register_functions(rpc_get_internal_list(1), rpc_server_dispatch);

      /* run forever */
      while (ss_suspend(5000, 0) != RPC_SHUTDOWN);
   } else {

      /* here we come if this program is started as a subprocess */

      memset(&callback, 0, sizeof(callback));

      /* extract callback arguments and start receiver */
#ifdef OS_VMS
      strlcpy(callback.host_name, argv[2], sizeof(callback.host_name));
      callback.host_port1 = atoi(argv[3]);
      callback.host_port2 = atoi(argv[4]);
      callback.host_port3 = atoi(argv[5]);
      callback.debug = atoi(argv[6]);
      if (argc > 7)
         strlcpy(callback.experiment, argv[7], sizeof(callback.experiment));
      if (argc > 8)
         strlcpy(callback.directory, argv[8], sizeof(callback.directory));
      if (argc > 9)
         strlcpy(callback.user, argv[9], sizeof(callback.user));
#else
      strlcpy(callback.host_name, argv[1], sizeof(callback.host_name));
      callback.host_port1 = atoi(argv[2]);
      callback.host_port2 = atoi(argv[3]);
      callback.host_port3 = atoi(argv[4]);
      callback.debug = atoi(argv[5]);
      if (argc > 6)
         strlcpy(callback.experiment, argv[6], sizeof(callback.experiment));
      if (argc > 7)
         strlcpy(callback.directory, argv[7], sizeof(callback.directory));
      if (argc > 8)
         strlcpy(callback.user, argv[8], sizeof(callback.user));
#endif
      callback.index = 0;

      if (callback.debug) {
         rpc_set_debug(debug_print, 1);
         if (callback.directory[0]) {
            if (callback.user[0])
               rpc_debug_printf("Start subprocess in %s under user %s", callback.directory, callback.user);
            else
               rpc_debug_printf("Start subprocess in %s", callback.directory);

         } else
            rpc_debug_printf("Start subprocess in current directory");
      }

      /* change the directory and uid */
      if (callback.directory[0])
         if (chdir(callback.directory) != 0)
            rpc_debug_printf("Cannot change to directory \"%s\"", callback.directory);

#ifdef OS_UNIX

      /* under UNIX, change user and group ID if started under root */
      if (callback.user[0] && geteuid() == 0) {
         struct passwd *pw;

         pw = getpwnam(callback.user);
         if (pw == NULL) {
            rpc_debug_printf("Cannot change UID, unknown user \"%s\"", callback.user);
            cm_msg(MERROR, "main", "Cannot change UID, unknown user \"%s\"", callback.user);
         } else {
            if (setgid(pw->pw_gid) < 0 || initgroups(pw->pw_name, pw->pw_gid) < 0) {
               rpc_debug_printf("Unable to set group premission for user %s", callback.user);
               cm_msg(MERROR, "main", "Unable to set group premission for user %s", callback.user);
            } else {
               if (setuid(pw->pw_uid) < 0) {
                  rpc_debug_printf("Unable to set user ID for %s", callback.user);
                  cm_msg(MERROR, "main", "Unable to set user ID for %s", callback.user);
               } else {
                  if (debug) {
                     rpc_debug_printf("Changed UID to user %s (GID %d, UID %d)",
                                      callback.user, pw->pw_gid, pw->pw_uid);
                     cm_msg(MLOG, "main", "Changed UID to user %s (GID %d, UID %d)",
                            callback.user, pw->pw_gid, pw->pw_uid);
                  }
               }
            }
         }
      }
#endif

      rpc_register_server(ST_SUBPROCESS, NULL, NULL, rpc_server_dispatch);

      /* register MIDAS library functions */
      rpc_register_functions(rpc_get_internal_list(1), rpc_server_dispatch);

      rpc_server_thread(&callback);
   }

   return BM_SUCCESS;
}

/*------------------------------------------------------------------*/

/* just a small test routine which doubles numbers */
INT rpc_test(BYTE b, WORD w, INT i, float f, double d, BYTE * b1, WORD * w1, INT * i1, float *f1, double *d1)
{
   printf("rpc_test: %d %d %d %1.1f %1.1lf\n", b, w, i, f, d);

   *b1 = b * 2;
   *w1 = w * 2;
   *i1 = i * 2;
   *f1 = f * 2;
   *d1 = d * 2;

   return 1;
}

/*----- rpc_server_dispatch ----------------------------------------*/

INT rpc_server_dispatch(INT index, void *prpc_param[])
/********************************************************************\

  Routine: rpc_server_dispatch

  Purpose: This routine gets registered as the callback function
           for all RPC calls via rpc_server_register. It acts as a
           stub to call the local midas subroutines. It uses the
           Cxxx(i) macros to cast parameters in prpc_param to
           their appropriate types.

  Input:
    int    index            RPC index defined in RPC.H
    void   *prpc_param      pointer to rpc parameter array

  Output:
    none

  Function value:
    status                  status returned from local midas function

\********************************************************************/
{
   INT status = 0;
   INT convert_flags;

   convert_flags = rpc_get_server_option(RPC_CONVERT_FLAGS);

   switch (index) {
      /* common functions */

   case RPC_CM_SET_CLIENT_INFO:
      status = cm_set_client_info(CHNDLE(0), CPHNDLE(1), (use_callback_addr?callback.host_name:CSTRING(2)),
                                  CSTRING(3), CINT(4), CSTRING(5), CINT(6));
      break;

   case RPC_CM_CHECK_CLIENT:
      status = cm_check_client(CHNDLE(0), CHNDLE(1));
      break;

   case RPC_CM_SET_WATCHDOG_PARAMS:
      status = cm_set_watchdog_params(CBOOL(0), CINT(1));
      break;

   case RPC_CM_CLEANUP:
      status = cm_cleanup(CSTRING(0), CBOOL(1));
      break;

   case RPC_CM_GET_WATCHDOG_INFO:
      status = cm_get_watchdog_info(CHNDLE(0), CSTRING(1), CPDWORD(2), CPDWORD(3));
      break;

   case RPC_CM_MSG:
      status = cm_msg(CINT(0), CSTRING(1), CINT(2), CSTRING(3), CSTRING(4));
      break;

   case RPC_CM_MSG_LOG:
      status = cm_msg_log(CINT(0), CSTRING(1));
      break;

   case RPC_CM_MSG_LOG1:
      status = cm_msg_log1(CINT(0), CSTRING(1), CSTRING(2));
      break;

   case RPC_CM_EXECUTE:
      status = cm_execute(CSTRING(0), CSTRING(1), CINT(2));
      break;

   case RPC_CM_EXIST:
      status = cm_exist(CSTRING(0), CBOOL(1));
      break;

   case RPC_CM_SYNCHRONIZE:
      status = cm_synchronize(CPDWORD(0));
      break;

   case RPC_CM_ASCTIME:
      status = cm_asctime(CSTRING(0), CINT(1));
      break;

   case RPC_CM_TIME:
      status = cm_time(CPDWORD(0));
      break;

   case RPC_CM_MSG_RETRIEVE:
      status = cm_msg_retrieve(CINT(0), CSTRING(1), CINT(2));
      break;

      /* buffer manager functions */

   case RPC_BM_OPEN_BUFFER:

#ifdef OS_WINNT
      /*
         bm_open_buffer may only be called from one thread at a time,
         so use critical section object for synchronization.
       */
      EnterCriticalSection(&buffer_critial_section);
#endif

      status = bm_open_buffer(CSTRING(0), CINT(1), CPINT(2));

#ifdef OS_WINNT
      LeaveCriticalSection(&buffer_critial_section);
#endif

      break;

   case RPC_BM_CLOSE_BUFFER:

#ifdef OS_WINNT
      /*
         bm_close_buffer may only be called from one thread at a time,
         so use critical section object for synchronization.
       */
      EnterCriticalSection(&buffer_critial_section);
#endif

      status = bm_close_buffer(CINT(0));

#ifdef OS_WINNT
      LeaveCriticalSection(&buffer_critial_section);
#endif

      break;

   case RPC_BM_CLOSE_ALL_BUFFERS:

#ifdef OS_WINNT
      /*
         bm_close_all_buffers may only be called from one thread at a time,
         so use critical section object for synchronization.
       */
      EnterCriticalSection(&buffer_critial_section);
#endif

      status = bm_close_all_buffers();

#ifdef OS_WINNT
      LeaveCriticalSection(&buffer_critial_section);
#endif

      break;

   case RPC_BM_GET_BUFFER_INFO:
      status = bm_get_buffer_info(CINT(0), CARRAY(1));
      if (convert_flags) {
         BUFFER_HEADER *pb;

         /* convert event header */
         pb = (BUFFER_HEADER *) CARRAY(1);
         rpc_convert_single(&pb->num_clients, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pb->max_client_index, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pb->size, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pb->read_pointer, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pb->write_pointer, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pb->num_in_events, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pb->num_out_events, TID_INT, RPC_OUTGOING, convert_flags);
      }
      break;

   case RPC_BM_GET_BUFFER_LEVEL:
      status = bm_get_buffer_level(CINT(0), CPINT(1));
      break;

   case RPC_BM_INIT_BUFFER_COUNTERS:
      status = bm_init_buffer_counters(CINT(0));
      break;

   case RPC_BM_SET_CACHE_SIZE:
      status = bm_set_cache_size(CINT(0), CINT(1), CINT(2));
      break;

   case RPC_BM_ADD_EVENT_REQUEST:
      status = bm_add_event_request(CINT(0), CSHORT(1),
                                    CSHORT(2), CINT(3), (void (*)(HNDLE, HNDLE, EVENT_HEADER *, void *))
                                    (POINTER_T)
                                    CINT(4), CINT(5));
      break;

   case RPC_BM_REMOVE_EVENT_REQUEST:
      status = bm_remove_event_request(CINT(0), CINT(1));
      break;

   case RPC_BM_SEND_EVENT:
      if (convert_flags) {
         EVENT_HEADER *pevent;

         /* convert event header */
         pevent = (EVENT_HEADER *) CARRAY(1);
         rpc_convert_single(&pevent->event_id, TID_SHORT, 0, convert_flags);
         rpc_convert_single(&pevent->trigger_mask, TID_SHORT, 0, convert_flags);
         rpc_convert_single(&pevent->serial_number, TID_DWORD, 0, convert_flags);
         rpc_convert_single(&pevent->time_stamp, TID_DWORD, 0, convert_flags);
         rpc_convert_single(&pevent->data_size, TID_DWORD, 0, convert_flags);
      }

      status = bm_send_event(CINT(0), CARRAY(1), CINT(2), CINT(3));
      break;

   case RPC_BM_RECEIVE_EVENT:
      status = bm_receive_event(CINT(0), CARRAY(1), CPINT(2), CINT(3));
      break;

   case RPC_BM_SKIP_EVENT:
      status = bm_skip_event(CINT(0));
      break;

   case RPC_BM_FLUSH_CACHE:
      status = bm_flush_cache(CINT(0), CINT(1));
      break;

   case RPC_BM_MARK_READ_WAITING:
      status = bm_mark_read_waiting(CBOOL(0));
      break;

   case RPC_BM_EMPTY_BUFFERS:
      status = bm_empty_buffers();
      break;

      /* database functions */

   case RPC_DB_OPEN_DATABASE:

#ifdef OS_WINNT
      /*
         db_open_database may only be called from one thread at a time,
         so use critical section object for synchronization.
       */
      EnterCriticalSection(&buffer_critial_section);
#endif

      status = db_open_database(CSTRING(0), CINT(1), CPHNDLE(2), CSTRING(3));

#ifdef OS_WINNT
      LeaveCriticalSection(&buffer_critial_section);
#endif

      break;

   case RPC_DB_CLOSE_DATABASE:

#ifdef OS_WINNT
      /*
         db_close_database may only be called from one thread at a time,
         so use critical section object for synchronization.
       */
      EnterCriticalSection(&buffer_critial_section);
#endif

      status = db_close_database(CINT(0));

#ifdef OS_WINNT
      LeaveCriticalSection(&buffer_critial_section);
#endif

      break;

   case RPC_DB_FLUSH_DATABASE:
      status = db_flush_database(CINT(0));
      break;

   case RPC_DB_CLOSE_ALL_DATABASES:

#ifdef OS_WINNT
      /*
         db_close_allo_databases may only be called from one thread at a time,
         so use critical section object for synchronization.
       */
      EnterCriticalSection(&buffer_critial_section);
#endif

      status = db_close_all_databases();

#ifdef OS_WINNT
      LeaveCriticalSection(&buffer_critial_section);
#endif

      break;

   case RPC_DB_CREATE_KEY:
      status = db_create_key(CHNDLE(0), CHNDLE(1), CSTRING(2), CDWORD(3));
      break;

   case RPC_DB_CREATE_LINK:
      status = db_create_link(CHNDLE(0), CHNDLE(1), CSTRING(2), CSTRING(3));
      break;

   case RPC_DB_SET_VALUE:
      rpc_convert_data(CARRAY(3), CDWORD(6), RPC_FIXARRAY, CINT(4), convert_flags);
      status = db_set_value(CHNDLE(0), CHNDLE(1), CSTRING(2), CARRAY(3), CINT(4), CINT(5), CDWORD(6));
      break;

   case RPC_DB_GET_VALUE:
      rpc_convert_data(CARRAY(3), CDWORD(5), RPC_FIXARRAY, CINT(4), convert_flags);
      status = db_get_value(CHNDLE(0), CHNDLE(1), CSTRING(2), CARRAY(3), CPINT(4), CDWORD(5), CBOOL(6));
      rpc_convert_data(CARRAY(3), CDWORD(5), RPC_FIXARRAY | RPC_OUTGOING, CINT(4), convert_flags);
      break;

   case RPC_DB_FIND_KEY:
      status = db_find_key(CHNDLE(0), CHNDLE(1), CSTRING(2), CPHNDLE(3));
      break;

   case RPC_DB_FIND_LINK:
      status = db_find_link(CHNDLE(0), CHNDLE(1), CSTRING(2), CPHNDLE(3));
      break;

   case RPC_DB_GET_PATH:
      status = db_get_path(CHNDLE(0), CHNDLE(1), CSTRING(2), CINT(3));
      break;

   case RPC_DB_DELETE_KEY:
      status = db_delete_key(CHNDLE(0), CHNDLE(1), CBOOL(2));
      break;

   case RPC_DB_ENUM_KEY:
      status = db_enum_key(CHNDLE(0), CHNDLE(1), CINT(2), CPHNDLE(3));
      break;

   case RPC_DB_ENUM_LINK:
      status = db_enum_link(CHNDLE(0), CHNDLE(1), CINT(2), CPHNDLE(3));
      break;

   case RPC_DB_GET_NEXT_LINK:
      status = db_get_next_link(CHNDLE(0), CHNDLE(1), CPHNDLE(2));
      break;

   case RPC_DB_GET_KEY:
      status = db_get_key(CHNDLE(0), CHNDLE(1), CARRAY(2));
      if (convert_flags) {
         KEY *pkey;

         pkey = (KEY *) CARRAY(2);
         rpc_convert_single(&pkey->type, TID_DWORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->num_values, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->data, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->total_size, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->item_size, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->access_mode, TID_WORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->notify_count, TID_WORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->next_key, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->parent_keylist, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->last_written, TID_INT, RPC_OUTGOING, convert_flags);
      }
      break;

   case RPC_DB_GET_LINK:
      status = db_get_link(CHNDLE(0), CHNDLE(1), CARRAY(2));
      if (convert_flags) {
         KEY *pkey;

         pkey = (KEY *) CARRAY(2);
         rpc_convert_single(&pkey->type, TID_DWORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->num_values, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->data, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->total_size, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->item_size, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->access_mode, TID_WORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->notify_count, TID_WORD, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->next_key, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->parent_keylist, TID_INT, RPC_OUTGOING, convert_flags);
         rpc_convert_single(&pkey->last_written, TID_INT, RPC_OUTGOING, convert_flags);
      }
      break;

   case RPC_DB_GET_KEY_INFO:
      status = db_get_key_info(CHNDLE(0), CHNDLE(1), CSTRING(2), CINT(3), CPINT(4), CPINT(5), CPINT(6));
      break;

   case RPC_DB_GET_KEY_TIME:
      status = db_get_key_time(CHNDLE(0), CHNDLE(1), CPDWORD(2));
      break;

   case RPC_DB_RENAME_KEY:
      status = db_rename_key(CHNDLE(0), CHNDLE(1), CSTRING(2));
      break;

   case RPC_DB_REORDER_KEY:
      status = db_reorder_key(CHNDLE(0), CHNDLE(1), CINT(2));
      break;

   case RPC_DB_GET_DATA:
      status = db_get_data(CHNDLE(0), CHNDLE(1), CARRAY(2), CPINT(3), CDWORD(4));
      rpc_convert_data(CARRAY(2), CDWORD(4), RPC_FIXARRAY | RPC_OUTGOING, CINT(3), convert_flags);
      break;

   case RPC_DB_GET_LINK_DATA:
      status = db_get_link_data(CHNDLE(0), CHNDLE(1), CARRAY(2), CPINT(3), CDWORD(4));
      rpc_convert_data(CARRAY(2), CDWORD(4), RPC_FIXARRAY | RPC_OUTGOING, CINT(3), convert_flags);
      break;

   case RPC_DB_GET_DATA1:
      status = db_get_data1(CHNDLE(0), CHNDLE(1), CARRAY(2), CPINT(3), CDWORD(4), CPINT(5));
      rpc_convert_data(CARRAY(2), CDWORD(4), RPC_FIXARRAY | RPC_OUTGOING, CINT(3), convert_flags);
      break;

   case RPC_DB_GET_DATA_INDEX:
      status = db_get_data_index(CHNDLE(0), CHNDLE(1), CARRAY(2), CPINT(3), CINT(4), CDWORD(5));
      rpc_convert_single(CARRAY(2), CDWORD(5), RPC_OUTGOING, convert_flags);
      break;

   case RPC_DB_SET_DATA:
      rpc_convert_data(CARRAY(2), CDWORD(5), RPC_FIXARRAY, CINT(3), convert_flags);
      status = db_set_data(CHNDLE(0), CHNDLE(1), CARRAY(2), CINT(3), CINT(4), CDWORD(5));
      break;

   case RPC_DB_SET_LINK_DATA:
      rpc_convert_data(CARRAY(2), CDWORD(5), RPC_FIXARRAY, CINT(3), convert_flags);
      status = db_set_link_data(CHNDLE(0), CHNDLE(1), CARRAY(2), CINT(3), CINT(4), CDWORD(5));
      break;

   case RPC_DB_SET_DATA_INDEX:
      rpc_convert_single(CARRAY(2), CDWORD(5), 0, convert_flags);
      status = db_set_data_index(CHNDLE(0), CHNDLE(1), CARRAY(2), CINT(3), CINT(4), CDWORD(5));
      break;

   case RPC_DB_SET_LINK_DATA_INDEX:
      rpc_convert_single(CARRAY(2), CDWORD(5), 0, convert_flags);
      status = db_set_link_data_index(CHNDLE(0), CHNDLE(1), CARRAY(2), CINT(3), CINT(4), CDWORD(5));
      break;

   case RPC_DB_SET_DATA_INDEX2:
      rpc_convert_single(CARRAY(2), CDWORD(5), 0, convert_flags);
      status = db_set_data_index2(CHNDLE(0), CHNDLE(1), CARRAY(2), CINT(3), CINT(4), CDWORD(5), CBOOL(6));
      break;

   case RPC_DB_SET_NUM_VALUES:
      status = db_set_num_values(CHNDLE(0), CHNDLE(1), CINT(2));
      break;

   case RPC_DB_SET_MODE:
      status = db_set_mode(CHNDLE(0), CHNDLE(1), CWORD(2), CBOOL(3));
      break;

   case RPC_DB_GET_RECORD:
      status = db_get_record(CHNDLE(0), CHNDLE(1), CARRAY(2), CPINT(3), CINT(4));
      break;

   case RPC_DB_SET_RECORD:
      status = db_set_record(CHNDLE(0), CHNDLE(1), CARRAY(2), CINT(3), CINT(4));
      break;

   case RPC_DB_GET_RECORD_SIZE:
      status = db_get_record_size(CHNDLE(0), CHNDLE(1), CINT(2), CPINT(3));
      break;

   case RPC_DB_CREATE_RECORD:
      status = db_create_record(CHNDLE(0), CHNDLE(1), CSTRING(2), CSTRING(3));
      break;

   case RPC_DB_CHECK_RECORD:
      status = db_check_record(CHNDLE(0), CHNDLE(1), CSTRING(2), CSTRING(3), CBOOL(4));
      break;

   case RPC_DB_ADD_OPEN_RECORD:
      status = db_add_open_record(CHNDLE(0), CHNDLE(1), CWORD(2));
      break;

   case RPC_DB_REMOVE_OPEN_RECORD:
      status = db_remove_open_record(CHNDLE(0), CHNDLE(1), CBOOL(2));
      break;

   case RPC_DB_LOAD:
      status = db_load(CHNDLE(0), CHNDLE(1), CSTRING(2), CBOOL(3));
      break;

   case RPC_DB_SAVE:
      status = db_save(CHNDLE(0), CHNDLE(1), CSTRING(2), CBOOL(3));
      break;

   case RPC_DB_SET_CLIENT_NAME:
      status = db_set_client_name(CHNDLE(0), CSTRING(1));
      break;

   case RPC_DB_GET_OPEN_RECORDS:
      status = db_get_open_records(CHNDLE(0), CHNDLE(1), CSTRING(2), CINT(3), CBOOL(4));
      break;

      /* history functions */

   case RPC_HS_SET_PATH:
      status = hs_set_path(CSTRING(0));
      break;

   case RPC_HS_DEFINE_EVENT:
      if (convert_flags) {
         TAG *tag;
         INT i;

         /* convert tags */
         tag = (TAG *) CARRAY(2);
         for (i = 0; i < CINT(3); i++) {
            rpc_convert_single(&tag[i].type, TID_DWORD, 0, convert_flags);
            rpc_convert_single(&tag[i].n_data, TID_DWORD, 0, convert_flags);
         }
      }

      status = hs_define_event(CDWORD(0), CSTRING(1), CARRAY(2), CDWORD(3));
      break;

   case RPC_HS_WRITE_EVENT:
      status = hs_write_event(CDWORD(0), CARRAY(1), CDWORD(2));
      break;

   case RPC_HS_COUNT_EVENTS:
      status = hs_count_events(CDWORD(0), CPDWORD(1));
      break;

   case RPC_HS_ENUM_EVENTS:
      status = hs_enum_events(CDWORD(0), CSTRING(1), CPDWORD(2), CPINT(3), CPDWORD(4));
      break;

   case RPC_HS_COUNT_VARS:
      status = hs_count_vars(CDWORD(0), CDWORD(1), CPDWORD(2));
      break;

   case RPC_HS_ENUM_VARS:
      status = hs_enum_vars(CDWORD(0), CDWORD(1), CSTRING(2), CPDWORD(3), CPDWORD(4), CPDWORD(5));
      break;

   case RPC_HS_GET_VAR:
      status = hs_get_var(CDWORD(0), CDWORD(1), CSTRING(2), CPDWORD(3), CPINT(4));
      break;

   case RPC_HS_GET_EVENT_ID:
      status = hs_get_event_id(CDWORD(0), CSTRING(1), CPDWORD(2));
      break;

   case RPC_HS_READ:
      status = hs_read(CDWORD(0), CDWORD(1), CDWORD(2), CDWORD(3), CSTRING(4),
                       CDWORD(5), CARRAY(6), CPDWORD(7), CARRAY(8), CPDWORD(9), CPDWORD(10), CPDWORD(11));
      if (convert_flags && rpc_tid_size(CDWORD(10)) > 0) {
         rpc_convert_data(CARRAY(6), TID_DWORD, RPC_FIXARRAY | RPC_OUTGOING,
                          CDWORD(7) / sizeof(DWORD), convert_flags);
         rpc_convert_data(CARRAY(8), CDWORD(10), RPC_FIXARRAY | RPC_OUTGOING,
                          CDWORD(11) / rpc_tid_size(CDWORD(10)), convert_flags);
      }
      break;

   case RPC_EL_SUBMIT:
      status = el_submit(CINT(0), CSTRING(1), CSTRING(2), CSTRING(3), CSTRING(4),
                         CSTRING(5), CSTRING(6), CSTRING(7),
                         CSTRING(8), CARRAY(9), CINT(10),
                         CSTRING(11), CARRAY(12), CINT(13),
                         CSTRING(14), CARRAY(15), CINT(16), CSTRING(17), CINT(18));
      break;

   case RPC_AL_CHECK:
      status = al_check();
      break;

   case RPC_AL_TRIGGER_ALARM:
      status = al_trigger_alarm(CSTRING(0), CSTRING(1), CSTRING(2), CSTRING(3), CINT(4));
      break;

      /* exit functions */
   case RPC_ID_EXIT:
   case RPC_ID_SHUTDOWN:
      status = RPC_SUCCESS;
      break;

      /* various functions */

   case RPC_TEST:
      status = rpc_test(CBYTE(0), CWORD(1), CINT(2), CFLOAT(3), CDOUBLE(4),
                        CPBYTE(5), CPWORD(6), CPINT(7), CPFLOAT(8), CPDOUBLE(9));
      break;

   default:
      cm_msg(MERROR, "rpc_server_dispatch", "received unrecognized command %d", index);
   }

   return status;
}
