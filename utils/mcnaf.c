/********************************************************************\
  
  Name:         mcnaf.c
  Created by:   Pierre-Andre Amaudruz
  
  Contents:     CAMAC utility
  
  $Log$
  Revision 1.4  1998/10/13 16:56:53  pierre
  -PAA- Add log header

\********************************************************************/
#include <stdio.h>
#include "midas.h"
#include "msystem.h"
#include "mcstd.h"

/* internal definitions */
#define  D16   16	 
#define  D24   24
#define  LOOP   1
#define  QUIT   2
#define  HELP   3
#define  SKIP   4
#define  JOB    5
#define  CHECK  6
#define  READ   7

/* structure for CAMAC definition */
typedef struct {
  INT       m;
  INT				b;
  INT				c;
  INT				n;
  INT				a;
  INT				f;
  WORD         d16;
  DWORD        d24;
  INT          r;
  INT          w;
  INT          q;
  INT          x;
  } CAMAC;

/* Default CAMAC definition */
CAMAC Prompt[2] ={ {24,0,0,1,2,0,0,0,1,0,0,0},
                   {0}
                 };

/* Default job file name */
char     job_name[128]="cnaf.cnf";

HNDLE    hDB, hKey, hConn; 
FILE *   pF;
BOOL  jobflag;
char  addr[128];

/* prototype */
void cnafsub();
void help_page();
INT  decode_line (CAMAC *p, char * str);
INT  read_job_file(FILE * pF, INT action, void ** job, char * name);
void make_display_string(CAMAC *p);
void cc_services(CAMAC *p);

/*--------------------------------------------------------------------*/
void cc_services (CAMAC *p)
{
  if (p->n == 30)
    {
      if (p->a==9 && p->f==24)
        cam_inhibit_clear(p->c);
      else if (p->a==9 && p->f==26)
        cam_inhibit_set(p->c);
      else if (p->a>=0 && p->a<8 && p->f==1)
        cam_lam_read(p->c, &p->d24);
      else if (p->a>=13 && p->f==17)
        cam_lam_enable(p->c, p->d24);
      else if (p->a>=12 && p->f==17)
        cam_lam_disable(p->c, p->d24);
/*      else if (p->a>=13 && p->f==1)
         we don't support that function "read lam mask" */
    }
  else if (p->n == 28)
    {
      if (p->a==8 && p->f==26)
        cam_crate_zinit(p->c);
      else if (p->a==9 && p->f==26)
        cam_crate_clear(p->c);
    }
}
/*--------------------------------------------------------------------*/
void make_display_string(CAMAC *p)
{
  if (p->m == D24)
    sprintf(addr,"B%01dC%01dN%02dA%02dF%02d [%d/0x%06x Q%01dX%01d] R%dW%dM%2d"
	    ,p->b,p->c,p->n,p->a,p->f,p->d24,p->d24,p->q,p->x,p->r,p->w,p->m);
  else
    sprintf(addr,"B%01dC%01dN%02dA%02dF%02d [%d/0x%04x Q%01dX%01d] R%dW%dM%2d"
	    ,p->b,p->c,p->n,p->a,p->f,p->d16,p->d16,p->q,p->x,p->r,p->w,p->m);
}
/*--------------------------------------------------------------------*/
INT  read_job_file(FILE * pF, INT action, void ** job, char * name)
  {
    DWORD n;
    char  line[128];
    CAMAC * pjob;

    if (action == CHECK)
      {
        if (*name == 0)
          sprintf(name,"%s",job_name);
        pF = fopen(name, "r");
        if (pF)
          {
            fclose (pF);
            return JOB;
          }
        printf("CNAF-I- File not found :%s\n",name);
        return SKIP;
      }
    else
      {
        pF = fopen(name, "r");
        if (pF)
          {
           n = 0;
		   /* count the total number of line */
            while (fgets(line, 128, pF))
              n++;
			/* allocate memory for full job */
            *job = malloc ((n+1)*sizeof(CAMAC));
            pjob = *job;
			/* preset the full job with 0 */
            memset ((char *)pjob, 0, (n+1)*sizeof(CAMAC));
			/* preset the first entry with the predefined CAMAC access */
            memcpy((char *) pjob, (char *)Prompt, sizeof(CAMAC));
            rewind (pF);
            while (fgets(line, 128, pF))
	      {
		if (pjob->m == 0)
		  /* load previous command before overwriting */
		  memcpy((char *)pjob, (char *) (pjob-1), sizeof(CAMAC));
		decode_line(pjob++, line);
	      }
            fclose (pF);
            return JOB;
          }
      }
      return SKIP;
  }

/*--------------------------------------------------------------------*/

void cnafsub()
{
  char str[128], line[128];
  INT  status, j;
  CAMAC *P, *p, *job;
  
  /* Loop return label */
  if (jobflag)
    {
      jobflag = FALSE;
    }
  
  /* Load default CAMAC */
  P = Prompt;
  while(1)
    {
      make_display_string(P);
      /* prompt */
      printf("CNAF> [%s] :",addr);
      ss_gets(str,128);
      
      /* decode line */    
      status = decode_line(P, str);
      if (status == QUIT)
	return;
      else if (status == HELP)
	help_page();
      else if (status == JOB)
	{
	  printf("\nCNAF> Job file name [%s]:",job_name);
	  ss_gets (line,128);
	  status = read_job_file(pF, CHECK, (void **) &job, line);
	  if (status == JOB)
	    {
	      sprintf(job_name,"%s",line);
	      status=read_job_file(pF, READ, (void **) &job, job_name);
	    }
	}
      if (status == LOOP || status == JOB)
	{
	  if (status == LOOP)
	    p = P;
	  if (status == JOB)
	    p = job;
	  while (p->m)
	    {
	      for (j=0;j<p->r;j++)
		{
		  if (p->n == 28 || p->n == 30)
		    cc_services(p);
		  else
		    if (p->m == 24) /* Actual 24 bits CAMAC operation */
		      if (p->f<16)
			cam24i_q(p->c, p->n, p->a, p->f, &p->d24, &p->x, &p->q);
		      else
			cam24o_q(p->c, p->n, p->a, p->f, p->d24, &p->x, &p->q);
		    else /* Actual 16 bits CAMAC operation */
		      if (p->f<16)
			cam16i_q(p->c, p->n, p->a, p->f, &p->d16, &p->x, &p->q);
		      else
			cam16o_q(p->c, p->n, p->a, p->f, p->d16, &p->x, &p->q);
		  make_display_string(p);
		  /* Result display */
		  if (p->r > 1)
		    {
		      /* repeat mode */
		      if (status == JOB)
			{
			  printf("\nCNAF> [%s]",addr);
			  if (p->w != 0)
			    ss_sleep(p->w);
			}  
		      else
			{
			  printf("CNAF> [%s] <-%03i\n",addr,j+1);
			  if (p->w != 0)
			    ss_sleep(p->w);
			  if (j > p->r-1)
			    break;
			}
		    }
		  else
		    {
		      /* single command */
		      if (status == JOB)
			{
			  printf("CNAF> [%s]\n",addr);
			  if (p->w != 0)
			    ss_sleep(p->w);
			}
		    }
		}
	      p++; 
	    };
	  if (status == JOB)
	    {
	      free (job);
	      printf("\n");
	    }
	}
    }
}

/*--------------------------------------------------------------------*/
INT decode_line (CAMAC *P, char * ss)
{
	char * c, * cmd;
	char pp[128], *p, *s, *ps;
  DWORD  tmp;
  BOOL   ok=FALSE;

	p = pp;
	s = ss;
	memset (pp,0,128);

  /* convert to uppercase */
  c = ss;
  while (*c)
    {
    *c = toupper(*c);
    c++;
    }
	
	while (*s)
  {
		if ((*s == 'B') || (*s == 'C') ||
			  (*s == 'N') || (*s == 'A') ||
			  (*s == 'F') || (*s == 'C') ||
			  (*s == 'R') || (*s == 'W') || 
			  (*s == 'G') || (*s == 'H') ||
			  (*s == 'E') || (*s == 'Q'))
          *p++ = ' ';
		else if ((*s == 'X') || (*s == 'D') || (*s == 'O'))
      {
        *p++ = ' ';
     	  *p++ = *s++;
        while (*s)
          *p++ = *s++;
        break;
      };
      *p++ = *s++;
  }
	*p++ = ' ';

	p = pp;

	if (cmd = strpbrk(p,"G"))
		return LOOP;
	if (cmd = strpbrk(p,"H"))
    return HELP;
	if (cmd = strpbrk(p,"QE"))
		return QUIT;
	if (cmd = strpbrk(p,"J"))
    return JOB;
	if (cmd = strpbrk(p,"D"))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
		if (P->m == D24)
      {
        tmp = strtoul((cmd+1),NULL,10);
        if (tmp>=0x0 && tmp<=0xffffff)
          {
    			  P->d24 = tmp;
            ok = TRUE;
          }
        else
          printf("mcnaf-E- Data out of range 0:16777215\n");
      }
		else
      {
        tmp = strtoul((cmd+1),NULL,10);
        if (tmp>=0x0 && tmp<=0xffff)
          {
            P->d16 = (WORD) tmp;
            ok = TRUE;
          }
        else
          printf("mcnaf-E- Data out of range 0:65535\n");
      }
		*ps = ' '; 
	}
	if (cmd = strpbrk(p,"X"))
	{
		ps = strchr(cmd,' ');
		*ps = 0;       
		if (P->m == D24)
      {
        tmp = strtoul((cmd+1),NULL,16);
        if (tmp>=0x0 && tmp<=0xffffff)
          {
            P->d24 = tmp;
            ok = TRUE;
          }
        else
          printf("mcnaf-E- Data out of range 0x0:0xffffff\n");
      }
		else
      {
        tmp = strtoul((cmd+1),NULL,16);
        if (tmp>=0x0 && tmp<=0xffff)
          {
            P->d16 = (WORD) tmp;
            ok = TRUE;
          }
        else
          printf("mcnaf-E- Data out of range 0x0:0xffff\n");
      }
		*ps = ' '; 
	}
	if (cmd = strpbrk(p,"O"))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
		if (P->m == D24)
      {
        tmp = strtoul((cmd+1),NULL,8);
        if (tmp>=0 && tmp<=077777777)
          {
            P->d24 = tmp;
            ok = TRUE;
          }
        else
          printf("mcnaf-E- Data out of range O0:O77777777\n");
      }
		else
      {
        tmp = strtoul((cmd+1),NULL,8);
        if (tmp>=00 && tmp<=0177777)
          {
            P->d16 = (WORD) tmp;
            ok = TRUE;
          }
        else
          printf("mcnaf-E- Data out of range O0:O177777\n");
      }
    *ps = ' '; 
	}
  if (cmd = strchr(p,'B'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<8 && tmp>=0)
      {
        P->b = tmp;
        ok = TRUE;
      }
    else
      printf("mcnaf-E- B out of range 0:7\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'C'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<8 && tmp>=0)
      {
        P->c = tmp;
        ok = TRUE;
      }
    else
      printf("mcnaf-E- C out of range 0:7\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'N'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<32 && tmp>=0)
      {
        P->n = tmp;
        ok = TRUE;
      }
    else
      printf("mcnaf-E- N out of range 0:31\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'A'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<16 && tmp>=0)
      {
        P->a = tmp;
        ok = TRUE;
      }
    else
      printf("mcnaf-E- A out of range 0:15\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'F'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<32 && tmp>=0)
      {
        P->f = tmp;
        ok = TRUE;
      }
    else
      printf("mcnaf-E- F out of range 0:31\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'R'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<1000 && tmp>=0)
      {
        if (tmp==0)
          tmp = 1;
        P->r = tmp;
      }
    else
      printf("mcnaf-E- R out of range 1:1000\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'W'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp<10001 || tmp>=0)
      P->w = tmp;
    else
      printf("mcnaf-E- W out of range 0:10000\n");
		*ps = ' '; 
	}
	if (cmd = strchr(p,'M'))
	{
		ps = strchr(cmd,' ');
		*ps = 0;
    tmp = atoi((cmd+1));
    if (tmp==D16 || tmp==D24)
      P->m = tmp;
    else
      printf("mcnaf-E- M out of range 16,24\n");
		*ps = ' '; 
	}
  if (ok)
    return LOOP;
	return SKIP;
}

/*--------------------------------------------------------------------*/
void help_page()
{
  printf("\n*-v%1.2lf----------- H E L P   C N A F -------------------*\n"
	 ,cm_get_version()/100.0);
  printf("          Interactive Midas CAMAC command\n");
  printf("          ===============================\n");
  printf(" Output : Data [dec/hex X=0/1 - Q=0/1 ]\n");
  printf(" Inputs : Bx :  Branch   [0 -  7]    Cx :  Crate    [0 -  7]\n");
  printf("          Nx :  Slot     [1 - 31]    Ax :  SubAdd.  [0 - 15]\n");
  printf("          Fx :  Function [0 - 31]    Mx :  Access mode [16,24]\n");
  printf("           H :  HELP on   CNAF      Q/E :  Quit/Exit from CNAF\n");
  printf("          Rx :  Repetition counter               [1 -   999]\n");
  printf("          Wx :  Delay between command (ms)       [0 - 10000]\n");
  printf("           J :  Perform JOB (command list from file )\n");
  printf("                Same syntax as in interactive session\n");
  printf("           G :  PERFORM ACTION of predefine set-up\n");
  printf("           D :  Decimal     Data     [0 - max data_size]\n");
  printf("           O :  Octal       Data     [0 - max data_size]\n");
  printf("           X :  Hexadecimal Data     [0 - max data_size]\n");
  printf("\n");
  printf(">>>>>>>>>> Data has to be given LAST if needed in the command string <<<<<\n");
  printf("\n");
  printf(" N30A9F24 : Crate clear inhibit    N30A9F26 : Crate set inhibit\n");
  printf(" N28A8F26 : Z crate                N28A9F26 : C crate\n");
  printf(" N30A13F17: CC Lam enable          N30A12F17: CC Lam disable\n");
  printf(" N30A0-7F1: CC read Lam\n");
  printf("\n");
  printf(" examples: ");
  printf(">mcnaf -cl\"triggerFE\" -h local -e myexpt\n");
  printf("      CNAF> [B0C1N30A00F00 [0/0x000000 Q0X0] R1W0M24] :n30a9f24\n");
  printf("      CNAF> [B0C1N06A00F24 [0/0x000000 Q1X1] R1W0M24] :n6f9a0\n");
}

/*--------------------------------------------------------------------*/

int main(int argc, char **argv)
{
char   host_name[30], exp_name[30], client_name[256], rpc_server[256];
INT    i, status;
BOOL   debug;
  
  /* set default */
  host_name[0] = '\0';
  exp_name[0] = '\0';
  client_name[0] = '\0';
  rpc_server[0] = '\0';
  debug = FALSE;

  /* get parameters */
  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'd')
      debug = TRUE;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (strncmp(argv[i],"-e",3) == 0)
      	strcpy(exp_name, argv[++i]);
      else if (strncmp(argv[i],"-h",3)==0)
        strcpy(host_name, argv[++i]);
      else if (strncmp(argv[i],"-c",3)==0)
        strcpy(client_name, argv[++i]);
      else if (strncmp(argv[i],"-s",3)==0)
        strcpy(rpc_server, argv[++i]);
      else
        {
usage:
        printf("usage: mcnaf [-c Client] [-h Hostname] [-e Experiment] [-s RPC server]\n\n");
        return 0;
        }
      }
    }

  if (rpc_server[0])
    status = cam_init_rpc("", "", "", rpc_server);
  else
    status = cam_init_rpc(host_name, exp_name, client_name, "");
  if (status == SUCCESS)
    {
      status = cam_init();
      if (status == SUCCESS)
        cnafsub();
    }
  cam_exit();
  return 0;
}
