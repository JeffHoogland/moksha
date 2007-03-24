/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <math.h>
#include <fnmatch.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <glob.h>
#include <errno.h>
#include <signal.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Ipc.h>
#include <Ecore_File.h>
#include <Evas.h>
#include <Eet.h>
#include <Edje.h>

/* FIXME: things to add to the slave enlightenment_fm process and ipc to e:
 * 
 * * icon finding
 * * .desktop parsing
 * * file operations (fop's) (cp, mv, rm etc.)
 * * reporting results of fop's 
 * * dbus removable device monitoring
 * * mount/umount of removable devices
 * 
 */

#define DEF_SYNC_NUM 8
#define DEF_ROUND_TRIP 0.05
#define DEF_ROUND_TRIP_TOLERANCE 0.01

typedef enum _E_Fop_Type
{
   E_FOP_NONE,
   E_FOP_DEL,
   E_FOP_TRASH,
   E_FOP_RENAME,
   E_FOP_MV,
   E_FOP_CP,
   E_FOP_MKDIR,
   E_FOP_MOUNT,
   E_FOP_UMOUNT,
   E_FOP_LAST
} E_Fop_Type;

typedef struct _E_Dir E_Dir;
typedef struct _E_Fop E_Fop;

struct _E_Dir
{
   int                 id;
   const char         *dir;
   Ecore_File_Monitor *mon;
   int                 mon_ref;
   Evas_List          *fq;
   Ecore_Idler        *idler;
   int                 dot_order;
   int                 sync;
   double              sync_time;
   int                 sync_num;
};

struct _E_Fop
{
   int                 id;
   E_Fop_Type          type;
   const char         *src;
   const char         *dst;
};

/* local subsystem functions */
static int _e_ipc_init(void);
static int _e_ipc_cb_server_add(void *data, int type, void *event);
static int _e_ipc_cb_server_del(void *data, int type, void *event);
static int _e_ipc_cb_server_data(void *data, int type, void *event);

static void _e_cb_file_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);

static void _e_file_add_mod(int id, const char *path, int op, int listing);
static void _e_file_add(int id, const char *path, int listing);
static void _e_file_del(int id, const char *path);
static void _e_file_mod(int id, const char *path);
static void _e_file_mon_dir_del(int id, const char *path);
static void _e_file_mon_list_sync(E_Dir *ed);

static int _e_cb_file_mon_list_idler(void *data);
static char *_e_str_list_remove(Evas_List **list, char *str);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server = NULL;

static Evas_List *_e_dirs = NULL;
static int _e_sync_num = 0;

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i;

   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-h")) ||
	    (!strcmp(argv[i], "-help")) ||
	    (!strcmp(argv[i], "--help")))
	  {
	     printf(
		    "This is an internal tool for Enlightenment.\n"
		    "do not use it.\n"
		    );
	     exit(0);
	  }
     }

   ecore_init();
   ecore_app_args_set(argc, (const char **)argv);
   eet_init();
   evas_init();
   edje_init();
   ecore_file_init();
   ecore_ipc_init();

   if (_e_ipc_init()) ecore_main_loop_begin();
   
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }

   ecore_ipc_shutdown();
   ecore_file_shutdown();
   ecore_evas_shutdown();
   edje_shutdown();
   evas_shutdown();
   eet_shutdown();
   ecore_shutdown();
   
   return 0;
}

/* local subsystem functions */
static int
_e_ipc_init(void)
{
   char *sdir;
   
   sdir = getenv("E_IPC_SOCKET");
   if (!sdir)
     {
	printf("The E_IPC_SOCKET environment variable is not set. This is\n"
	       "exported by Enlightenment to all processes it launches.\n"
	       "This environment variable must be set and must point to\n"
	       "Enlightenment's IPC socket file (minus port number).\n");
	return 0;
     }
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM, sdir, 0, NULL);
   if (!_e_ipc_server)
     {
	printf("Cannot connect to enlightenment - abort\n");
	return 0;
     }
   
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_ipc_cb_server_data, NULL);
   
   return 1;
}

static int
_e_ipc_cb_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;
   
   e = event;
   ecore_ipc_server_send(e->server, 
			 6/*E_IPC_DOMAIN_FM*/,
			 1/*hello*/, 
			 0, 0, 0, NULL, 0); /* send hello */
   return 1;
}

static int
_e_ipc_cb_server_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Del *e;
   
   e = event;
   /* quit now */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;
   if (e->major != 6/*E_IPC_DOMAIN_FM*/) return 1;
   switch (e->minor)
     {
      case 1: /* monitor dir (and implicitly list) */
	  {
	     E_Dir *ed, *ped = NULL;
	     DIR *dir;
	     
	     Evas_List *l;
	     
	     for (l = _e_dirs; l; l = l->next)
	       {
		  E_Dir *ed;
	     
		  ed = l->data;
		  if ((ed->mon) && (!strcmp(ed->dir, e->data)))
		    {
		       ped = ed;
		       break;
		    }
	       }
	     dir = opendir(e->data);
	     if (!dir)
	       {
		  _e_file_mon_dir_del(e->ref, e->data);
	       }
	     else
	       {
		  Evas_List *files = NULL;
		  struct dirent *dp;
		  int dot_order = 0;
		  char buf[4096];
		  FILE *f;
		  
		  ed = calloc(1, sizeof(E_Dir));
		  ed->id = e->ref;
		  ed->dir = evas_stringshare_add(e->data);
		  if (!ped)
		    {
		       ed->mon = ecore_file_monitor_add(ed->dir, _e_cb_file_monitor, ed);
		       ed->mon_ref = 1;
		    }
		  else
		    ped->mon_ref++;
		  _e_dirs = evas_list_append(_e_dirs, ed);
		  
		  while ((dp = readdir(dir)))
		    {
		       if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, "..")))
			 continue;
		       if (!strcmp(dp->d_name, ".order")) dot_order = 1;
		       files = evas_list_append(files, strdup(dp->d_name));
		    }
		  closedir(dir);
		  if (dot_order)
		    {
		       snprintf(buf, sizeof(buf), "%s/.order", (char *)e->data);
		       f = fopen(buf, "r");
		       if (f)
			 {
			    Evas_List *f2 = NULL;
			    int len;
			    char *s;
			    
			    while (fgets(buf, sizeof(buf), f))
			      {
				 len = strlen(buf);
				 if (len > 0) buf[len - 1] = 0;
				 s = _e_str_list_remove(&files, buf);
				 if (s) f2 = evas_list_append(f2, s);
			      }
			    fclose(f);
			    while (files)
			      {
				 f2 = evas_list_append(f2, files->data);
				 files = evas_list_remove_list(files, files);
			      }
			    files = f2;
			 }
		    }
		  /* FIXME: if .order file- load it, sort all items int it
		   * that are in files then just append whatever is left in
		   * alphabetical order
		   */
		  /* FIXME: maybe one day we can sort files here and handle
		   * .order file stuff here - but not today
		   */
		  ed->dot_order = dot_order;
		  if (dot_order)
		    {
		       if (!strcmp(e->data, "/"))
			 snprintf(buf, sizeof(buf), "/.order");
		       else
			 snprintf(buf, sizeof(buf), "%s/.order", (char *)e->data);
		       if (evas_list_count(files) == 1)
			 _e_file_add(ed->id, buf, 2);
		       else
			 _e_file_add(ed->id, buf, 1);
		    }
		  ed->fq = files;
		  ed->idler = ecore_idler_add(_e_cb_file_mon_list_idler, ed);
		  ed->sync_num = DEF_SYNC_NUM;
	       }
	  }
	break;
      case 2: /* monitor dir end */
	  {
	     Evas_List *l, *ll;
	     
	     for (l = _e_dirs; l; l = l->next)
	       {
		  E_Dir *ed;
	     
		  ed = l->data;
		  if ((e->ref == ed->id) && (!strcmp(ed->dir, e->data)))
		    {
		       if (ed->mon)
			 {
			    if (ed->mon_ref > 1)
			      {
				 for (ll = _e_dirs; ll; ll = ll->next)
				   {
				      E_Dir *ped;
				      
				      ped = ll->data;
				      if ((!ped->mon) &&
					  (!strcmp(ped->dir, ed->dir)))
					{
					   ped->mon = ed->mon;
					   ped->mon_ref = ed->mon_ref - 1;
					   goto contdel;
					}
				   }
			      }
			    ecore_file_monitor_del(ed->mon);
			 }
		       contdel:
		       evas_stringshare_del(ed->dir);
		       if (ed->idler) ecore_idler_del(ed->idler);
		       while (ed->fq)
			 {
			    free(ed->fq->data);
			    ed->fq = evas_list_remove_list(ed->fq, ed->fq);
			 }
		       free(ed);
		       _e_dirs = evas_list_remove_list(_e_dirs, l);
		       break;
		    }
	       }
	  }
	break;
      case 3: /* fop delete file/dir */
	  {
	     ecore_file_recursive_rm(e->data);
	     /* FIXME: send back file del if src is monitored */
	  }
	break;
      case 4: /* fop trash file/dir */
	  {
	     ecore_file_recursive_rm(e->data);
	     /* FIXME: send back file del if src is monitored */
	  }
	break;
      case 5: /* fop rename file/dir */
	/* FIXME: send back file del + add if dir is monitored */
	break;
      case 6: /* fop mv file/dir */
	/* FIXME: send back file del + add if src and dest are monitored */
	break;
      case 7: /* fop cp file/dir */
	/* FIXME: send back file add if succeeded - and dest monitored */
	break;
      case 8: /* fop mkdir */
	  {
	     ecore_file_mkdir(e->data);
	     /* FIXME: send back file add if succeeded */
	  }
	break;
      case 9: /* fop mount fs */
	/* FIXME: implement later - for now in e */
	break;
      case 10: /* fop umount fs */
	/* FIXME: implement later - for now in e */
	break;
      case 11: /* quit */
	ecore_main_loop_quit();
	break;
      case 12: /* mon list sync */
	  {
	     Evas_List *l;
	     double stime;
	     
             for (l = _e_dirs; l; l = l->next)
	       {
		  E_Dir *ed;
		  
		  ed = l->data;
		  if (ed->fq)
		    {
		       if (ed->sync == e->response)
			 {
			    stime = ecore_time_get() - ed->sync_time;
			    /* try keep round trips to round trip tolerance */
			    if 
			      (stime < (DEF_ROUND_TRIP - DEF_ROUND_TRIP_TOLERANCE))
			      ed->sync_num += 1;
			    else if
			      (stime > (DEF_ROUND_TRIP + DEF_ROUND_TRIP_TOLERANCE))
			      ed->sync_num -= 1;
			    /* always sync at least 1 file */
			    if (ed->sync_num < 1) ed->sync_num = 1;
			    ed->idler = ecore_idler_add(_e_cb_file_mon_list_idler, ed);
			    break;
			 }
		    }
	       }
	  }
	break;
      default:
	break;
     }
   /* always send back an "OK" for each request so e can basically keep a
    * count of outstanding requests - maybe for balancing between fm
    * slaves later. ref_to is set to the the ref id in the request to 
    * allow for async handling later */
   ecore_ipc_server_send(_e_ipc_server,
			 6/*E_IPC_DOMAIN_FM*/,
			 2/*req ok*/,
			 0, e->ref, 0, NULL, 0);
   return 1;
}

static void
_e_cb_file_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
   E_Dir *ed;
   char *dir, *rp, *drp;
   const char *file;
   Evas_List *l;

   dir = ecore_file_get_dir(path);
   file = ecore_file_get_file(path);
   if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
       (event == ECORE_FILE_EVENT_CREATED_DIRECTORY))
     {
	rp = ecore_file_realpath(dir);
	for (l = _e_dirs; l; l = l->next)
	  {
	     ed = l->data;
	     drp = ecore_file_realpath(ed->dir);
	     if (!strcmp(rp, drp))
	       _e_file_add(ed->id, path, 0);
	     free(drp);
	  }
	free(rp);
     }
   else if ((event == ECORE_FILE_EVENT_DELETED_FILE) ||
	    (event == ECORE_FILE_EVENT_DELETED_DIRECTORY))
     {
	rp = ecore_file_realpath(dir);
	for (l = _e_dirs; l; l = l->next)
	  {
	     ed = l->data;
	     drp = ecore_file_realpath(ed->dir);
	     if (!strcmp(rp, drp))
	       _e_file_del(ed->id, path);
	  }
	free(rp);
     }
   else if (event == ECORE_FILE_EVENT_MODIFIED)
     {
	rp = ecore_file_realpath(dir);
	for (l = _e_dirs; l; l = l->next)
	  {
	     ed = l->data;
	     drp = ecore_file_realpath(ed->dir);
	     if (!strcmp(rp, drp))
	       _e_file_mod(ed->id, path);
	  }
	free(rp);
     }
   else if (event == ECORE_FILE_EVENT_DELETED_SELF)
     {
	rp = ecore_file_realpath(path);
	for (l = _e_dirs; l; l = l->next)
	  {
	     ed = l->data;
	     drp = ecore_file_realpath(ed->dir);
	     if (!strcmp(rp, drp))
	       _e_file_mon_dir_del(ed->id, path);
	  }
	free(rp);
     }
   free(dir);
}

static void
_e_file_add_mod(int id, const char *path, int op, int listing)
{
   struct stat st;
   char *lnk = NULL, *rlnk = NULL;
   int broken_lnk = 0;
   int bsz = 0;
   unsigned char *p, buf
     /* file add/change format is as follows:
      * 
      8 stat_info[stat size] + broken_link[1] + path[n]\0 + lnk[n]\0 + rlnk[n]\0 */
     [sizeof(struct stat) + 1 + 4096 + 4096 + 4096];
   
   lnk = ecore_file_readlink(buf);
   if (stat(path, &st) == -1)
     {
	if (lnk) broken_lnk = 1;
	else return;
     }
   if ((lnk) && (lnk[0] != '/')) rlnk = ecore_file_realpath(path);
   if (!lnk) lnk = strdup("");
   if (!rlnk) rlnk = strdup("");

   p = buf;
   /* NOTE: i am NOT converting this data to portable arch/os independant
    * format. i am ASSUMING e_fm_main and e are local and built together
    * and thus this will work. if this ever changes this here needs to
    * change */
   memcpy(buf, &st, sizeof(struct stat));
   p += sizeof(struct stat);
   
   p[0] = broken_lnk;
   p += 1;
   
   strcpy(p, path);
   p += strlen(path) + 1;
   
   strcpy(p, lnk);
   p += strlen(lnk) + 1;
   
   strcpy(p, rlnk);
   p += strlen(rlnk) + 1;
   
   bsz = p - buf;
   ecore_ipc_server_send(_e_ipc_server, 6/*E_IPC_DOMAIN_FM*/, op, 0, id,
			 listing, buf, bsz);
   if (lnk) free(lnk);
   if (rlnk) free(rlnk);
}

static void
_e_file_add(int id, const char *path, int listing)
{
//   printf("+++FADD %s\n", path);
   _e_file_add_mod(id, path, 3, listing);/*file add*/
}

static void
_e_file_del(int id, const char *path)
{
//   printf("+++FDEL %s\n", path);
   ecore_ipc_server_send(_e_ipc_server,
			 6/*E_IPC_DOMAIN_FM*/,
			 4/*file del*/,
			 0, id, 0, (void *)path, strlen(path) + 1);
}

static void
_e_file_mod(int id, const char *path)
{
//   printf("+++FMOD %s\n", path);
   _e_file_add_mod(id, path, 5, 0);/*file change*/
}

static void
_e_file_mon_dir_del(int id, const char *path)
{
   ecore_ipc_server_send(_e_ipc_server,
			 6/*E_IPC_DOMAIN_FM*/,
			 6/*mon dir del*/,
			 0, id, 0, (void *)path, strlen(path) + 1);
}

static void
_e_file_mon_list_sync(E_Dir *ed)
{
   _e_sync_num++;
   if (_e_sync_num == 0) _e_sync_num = 1;
   ed->sync = _e_sync_num;
   ed->sync_time = ecore_time_get();
   ecore_ipc_server_send(_e_ipc_server,
			 6/*E_IPC_DOMAIN_FM*/,
			 7/*mon list sync*/,
			 0, ed->id, ed->sync, NULL, 0);
}

static int
_e_cb_file_mon_list_idler(void *data)
{
   E_Dir *ed;
   int n = 0;
   char *file, buf[4096];
   
   ed = data;
   /* FIXME: spool off files in idlers and handle sync req's */
   while (ed->fq)
     {
	file = ed->fq->data;
	if (!((ed->dot_order) && (!strcmp(file, ".order"))))
	  {
	     if (!strcmp(ed->dir, "/"))
	       snprintf(buf, sizeof(buf), "/%s", file);
	     else
	       snprintf(buf, sizeof(buf), "%s/%s", ed->dir, file);
	     if ((!ed->fq->next) ||
		 ((!strcmp(ed->fq->next->data, ".order")) &&
		  (!ed->fq->next->next)))
	       _e_file_add(ed->id, buf, 2);
	     else
	       _e_file_add(ed->id, buf, 1);
//	     printf("+++ FLIST %s\n", buf);
	  }
	free(file);
	ed->fq = evas_list_remove_list(ed->fq, ed->fq);
	n++;
	if (n == ed->sync_num)
	  {
	     _e_file_mon_list_sync(ed);
	     ed->idler = NULL;
	     return 0;
	  }
     }
   ed->sync_num = DEF_SYNC_NUM;
   ed->sync = 0;
   ed->sync_time = 0.0;
   ed->idler = NULL;
   return 0;
}

static char *
_e_str_list_remove(Evas_List **list, char *str)
{
   Evas_List *l;
   
   for (l = *list; l; l = l->next)
     {
	char *s;
	
	s = l->data;
	if (!strcmp(s, str))
	  {
	     *list = evas_list_remove_list(*list, l);
	     return s;
	  }
     }
   return NULL;
}

