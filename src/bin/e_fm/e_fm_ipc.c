#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#ifdef __linux__
#include <features.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <utime.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <glob.h>
#include <errno.h>
#include <signal.h>
#include <Ecore.h>
#include <Ecore_Ipc.h>
#include <Ecore_File.h>
#include <Efreet.h>
#include <Eet.h>

#include "e_fm_ipc.h"
#define E_TYPEDEFS
#include "e_fm_op.h"
#undef E_TYPEDEFS
#include "e_fm_main.h"
#include "e_fm_shared_codec.h"
#define DEF_MOD_BACKOFF          0.2

typedef struct _E_Dir          E_Dir;
typedef struct _E_Fop          E_Fop;
typedef struct _E_Mod          E_Mod;
typedef struct _e_fm_ipc_slave E_Fm_Slave;
typedef struct _E_Fm_Task      E_Fm_Task;

struct _E_Dir
{
   int                 id;
   const char         *dir;
   Ecore_File_Monitor *mon;
   int                 mon_ref;
   E_Dir              *mon_real;
   Eina_List          *fq;
   Eina_Iterator      *lister_iterator;
   Ecore_Thread       *lister_thread;
   Eina_List          *recent_mods;
   Ecore_Timer        *recent_clean;
   Eina_Bool           cleaning : 1;
   Eina_Bool           delete_me : 1;
};

struct _E_Fop
{
   int           id;
   const char   *src;
   const char   *dst;
   const char   *rel;
   int           rel_to;
   int           x, y;
   unsigned char del_after : 1;
   unsigned char gone_bad : 1;
   Ecore_Idler  *idler;
   void         *data;
};

struct _E_Mod
{
   const char   *path;
   double        timestamp;
   unsigned char add : 1;
   unsigned char del : 1;
   unsigned char mod : 1;
   unsigned char done : 1;
};

struct _e_fm_ipc_slave
{
   Ecore_Exe *exe;
   int        id;
};

struct _E_Fm_Task
{
   int          id;
   E_Fm_Op_Type type;
   E_Fm_Slave  *slave;
   const char  *src;
   const char  *dst;
   const char  *rel;
   int          rel_to;
   int          x, y;
};

/* local subsystem globals */
Ecore_Ipc_Server *_e_fm_ipc_server = NULL;

static Eina_List *_e_dirs = NULL;
static Eina_List *_e_fops = NULL;

static Eina_List *_e_fm_ipc_slaves = NULL;
static Eina_List *_e_fm_tasks = NULL;

static Eina_Bool _init_timer_run = EINA_FALSE;
static Ecore_Timer *_init_timer = NULL;

/* local subsystem functions */
static Eina_Bool   _e_fm_ipc_cb_server_add(void *data, int type, void *event);
static Eina_Bool   _e_fm_ipc_cb_server_del(void *data, int type, void *event);
static Eina_Bool   _e_fm_ipc_cb_server_data(void *data, int type, void *event);

static void        _e_fm_ipc_monitor_start(int id, const char *path);
static void        _e_fm_ipc_monitor_start_try(E_Fm_Task *task);
static void        _e_fm_ipc_monitor_end(int id, const char *path);
static E_Fm_Task  *_e_fm_ipc_task_get(int id);
static Eina_List  *_e_fm_ipc_task_node_get(int id);
static void        _e_fm_ipc_task_remove(E_Fm_Task *task);
static void        _e_fm_ipc_mkdir_try(E_Fm_Task *task);
static void        _e_fm_ipc_mkdir(int id, const char *src, const char *rel, int rel_to, int x, int y);
static void        _e_fm_ipc_handle_error_response(int id, E_Fm_Op_Type type);

static int         _e_fm_ipc_client_send(int id, E_Fm_Op_Type type, void *data, int size);

static int         _e_fm_ipc_slave_run(E_Fm_Op_Type type, const char *args, int id);
static E_Fm_Slave *_e_fm_ipc_slave_get(int id);
static int         _e_fm_ipc_slave_send(E_Fm_Slave *slave, E_Fm_Op_Type type, void *data, int size);

static void        _e_fm_ipc_cb_file_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static Eina_Bool   _e_fm_ipc_cb_recent_clean(void *data);

static void        _e_fm_ipc_file_add_mod(E_Dir *ed, const char *path, E_Fm_Op_Type op, int listing);
static void        _e_fm_ipc_file_add(E_Dir *ed, const char *path, int listing);
static void        _e_fm_ipc_file_del(E_Dir *ed, const char *path);
static void        _e_fm_ipc_file_mod(E_Dir *ed, const char *path);
static void        _e_fm_ipc_file_mon_dir_del(E_Dir *ed, const char *path);

static Eina_Bool   _e_fm_ipc_cb_fop_trash_idler(void *data);
static void        _e_fm_ipc_reorder(const char *file, const char *dst, const char *relative, int after);
static void        _e_fm_ipc_dir_del(E_Dir *ed);

static char *_e_fm_ipc_prepare_command(E_Fm_Op_Type type, const char *args);

/* local subsystem functions */
int
_e_fm_ipc_init(void)
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
   _e_fm_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM, sdir, 0, NULL);
   if (!_e_fm_ipc_server)
     {
        printf("Cannot connect to enlightenment (socket '%s') - abort\n", sdir);
        return 0;
     }

   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_fm_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_fm_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_fm_ipc_cb_server_data, NULL);

   return 1;
}

static Eina_Bool
_e_fm_ipc_cb_server_add(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;

   e = event;
   ecore_ipc_server_send(e->server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_HELLO,
                         0, 0, 0, NULL, 0); /* send hello */
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_fm_ipc_cb_server_del(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   /* quit now */
   ecore_main_loop_quit();
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_fm_ipc_monitor_start(int id, const char *path)
{
   E_Fm_Task *task = malloc(sizeof(E_Fm_Task));

   if (!task) return;

   task->id = id;
   task->type = E_FM_OP_MONITOR_START;
   task->slave = NULL;
   task->src = eina_stringshare_add(path);
   task->dst = NULL;
   task->rel = NULL;
   task->rel_to = 0;
   task->x = 0;
   task->y = 0;

   _e_fm_tasks = eina_list_append(_e_fm_tasks, task);

   _e_fm_ipc_monitor_start_try(task);
}

static void
_e_fm_ipc_cb_list_result(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg_data)
{
   E_Dir *ed = data;
   Eina_List *files = msg_data;
   const char *s;

   if (!files) _e_fm_ipc_file_add(ed, "", 2);
   else
     {
        EINA_LIST_FREE(files, s)
          {
             _e_fm_ipc_file_add(ed, s, 2);
             eina_stringshare_del(s);
          }
     }
}

static void
_e_fm_ipc_cb_list(void *data, Ecore_Thread *thread)
{
   E_Dir *ed = data;
   Eina_List *files = NULL;
   int i, total = 0;
   Eina_File_Direct_Info *info;
   Eina_Iterator *it = ed->lister_iterator;
   char buf[4096];
   const char *s;

   if (!strcmp(ed->dir, "/")) snprintf(buf, sizeof(buf), "/.order");
   else snprintf(buf, sizeof(buf), "%s/.order", ed->dir);
   if (ecore_file_exists(buf))
     {
        s = eina_stringshare_add(buf);
        files = eina_list_append(files, s);
     }
   i = 0;
   EINA_ITERATOR_FOREACH(it, info)
     {
        if (!strcmp(info->path + info->name_start, ".order")) continue;
        if (ecore_thread_check(thread))
          {
             EINA_LIST_FREE(files, s) eina_stringshare_del(s);
             break;
          }
        s = eina_stringshare_add(info->path);
        files = eina_list_append(files, s);
        total++;
        i++;
        if (i >= 16)
          {
             i = 0;
             ecore_thread_feedback(thread, files);
             files = NULL;
          }
     }
   if (files) ecore_thread_feedback(thread, files);
   if (total == 0) ecore_thread_feedback(thread, NULL);
}

static void
_e_fm_ipc_cb_list_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   E_Dir *ed = data;
   ed->lister_thread = NULL;
   eina_iterator_free(ed->lister_iterator);
   ed->lister_iterator = NULL;
   if (ed->delete_me) _e_fm_ipc_dir_del(ed);
}

static void
_e_fm_ipc_cb_list_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   E_Dir *ed = data;
   ed->lister_thread = NULL;
   eina_iterator_free(ed->lister_iterator);
   ed->lister_iterator = NULL;
   if (ed->delete_me) _e_fm_ipc_dir_del(ed);
}

static void
_e_fm_ipc_monitor_start_try(E_Fm_Task *task)
{
   E_Dir *ed, *ped = NULL;
   Eina_Iterator *it;

   Eina_List *l;

   /* look for any previous dir entries monitoring this dir */
   EINA_LIST_FOREACH(_e_dirs, l, ed)
     {
        if ((ed->mon) && (!strcmp(ed->dir, task->src)))
          {
             /* found a previous dir - save it in ped */
             ped = ed;
             break;
          }
     }

   /* open the dir to list */
   it = eina_file_direct_ls(task->src);
   if (!it)
     {
        char buf[PATH_MAX + 4096];

        snprintf(buf, sizeof(buf), "Cannot open directory '%s': %s.", task->src, strerror(errno));
        _e_fm_ipc_client_send(task->id, E_FM_OP_ERROR_RETRY_ABORT, buf, strlen(buf) + 1);
     }
   else
     {
        /* create a new dir entry */
        ed = calloc(1, sizeof(E_Dir));
        ed->id = task->id;
        ed->dir = eina_stringshare_add(task->src);
        ed->lister_iterator = it;
        if (!ped)
          {
             /* if no previous monitoring dir exists - this one
              * becomes the master monitor entry */
             ed->mon = ecore_file_monitor_add(ed->dir, _e_fm_ipc_cb_file_monitor, ed);
             ed->mon_ref = 1;
          }
        else
          {
             /* an existing monitor exists - ref it up */
             ed->mon_real = ped;
             ped->mon_ref++;
          }
        _e_dirs = eina_list_append(_e_dirs, ed);

        ed->lister_thread = ecore_thread_feedback_run(_e_fm_ipc_cb_list,
                                                      _e_fm_ipc_cb_list_result,
                                                      _e_fm_ipc_cb_list_end,
                                                      _e_fm_ipc_cb_list_cancel,
                                                      ed, EINA_TRUE);
     }
}

static void
_e_fm_ipc_monitor_end(int id, const char *path)
{
   E_Fm_Task *task;
   Eina_List *l;
   E_Dir *ed;

   EINA_LIST_FOREACH(_e_dirs, l, ed)
     /* look for the dire entry to stop monitoring */
     if ((id == ed->id) && (!strcmp(ed->dir, path)))
       {
          /* if this is not the real monitoring node - unref the
           * real one */
          if (ed->mon_real)
            {
               /* unref original monitor node */
               ed->mon_real->mon_ref--;
               if (ed->mon_real->mon_ref == 0)
                 {
                    /* original is at 0 ref - free it */
                    _e_fm_ipc_dir_del(ed->mon_real);
                    ed->mon_real = NULL;
                 }
               /* free this node */
               _e_fm_ipc_dir_del(ed);
            }
          /* this is a core monitoring node - remove ref */
          else
            {
               ed->mon_ref--;
               /* we are the last ref - free */
               if (ed->mon_ref == 0) _e_fm_ipc_dir_del(ed);
            }
          /* remove from dirs list anyway */
          _e_dirs = eina_list_remove_list(_e_dirs, l);
          break;
       }

   task = _e_fm_ipc_task_get(id);
   if (task) _e_fm_ipc_task_remove(task);
}

static E_Fm_Task *
_e_fm_ipc_task_get(int id)
{
   Eina_List *l = _e_fm_ipc_task_node_get(id);

   return (E_Fm_Task *)eina_list_data_get(l);
}

static Eina_List *
_e_fm_ipc_task_node_get(int id)
{
   E_Fm_Task *task;
   Eina_List *l;

   EINA_LIST_FOREACH(_e_fm_tasks, l, task)
     if (task->id == id)
       return l;

   return NULL;
}

static void
_e_fm_ipc_task_remove(E_Fm_Task *task)
{
   Eina_List *l = _e_fm_ipc_task_node_get(task->id);

   switch (task->type)
     {
      case E_FM_OP_MONITOR_START:
      {
         E_Dir ted;

         /* we can't open the dir - tell E the dir is deleted as
          * * we can't look in it */
         memset(&ted, 0, sizeof(E_Dir));
         ted.id = task->id;
         _e_fm_ipc_file_mon_dir_del(&ted, task->src);
      }
      break;

      default:
        break;
     }

   _e_fm_tasks = eina_list_remove_list(_e_fm_tasks, l);

   if (task->src) eina_stringshare_del(task->src);
   if (task->dst) eina_stringshare_del(task->dst);
   if (task->rel) eina_stringshare_del(task->rel);

   free(task);
}

static void
_e_fm_ipc_mkdir_try(E_Fm_Task *task)
{
   char buf[PATH_MAX + 4096], *dir;

   if (mkdir(task->src, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
     {
        snprintf(buf, sizeof(buf), "Cannot make directory '%s': %s.", task->src, strerror(errno));
        _e_fm_ipc_client_send(task->id, E_FM_OP_ERROR_RETRY_ABORT, buf, strlen(buf) + 1);
     }
   else
     {
        dir = ecore_file_dir_get(task->src);
        _e_fm_ipc_reorder(ecore_file_file_get(task->src), dir, task->rel, task->rel_to);
        _e_fm_ipc_task_remove(task);
        free(dir);
     }
}

static void
_e_fm_ipc_mkdir(int id, const char *src, const char *rel, int rel_to EINA_UNUSED, int x, int y)
{
   E_Fm_Task *task;

   task = malloc(sizeof(E_Fm_Task));

   task->id = id;
   task->type = E_FM_OP_MKDIR;
   task->slave = NULL;
   task->src = eina_stringshare_add(src);
   task->dst = NULL;
   task->rel = eina_stringshare_add(rel);
   task->x = x;
   task->y = y;

   _e_fm_tasks = eina_list_append(_e_fm_tasks, task);

   _e_fm_ipc_mkdir_try(task);
}

static void
_e_fm_ipc_handle_error_response(int id, E_Fm_Op_Type type)
{
   E_Fm_Task *task = _e_fm_ipc_task_get(id);
   E_Fm_Slave *slave = NULL;

   if (!task)
     {
        slave = _e_fm_ipc_slave_get(id);
        if (slave) _e_fm_ipc_slave_send(slave, type, NULL, 0);
        return;
     }

   if (type == E_FM_OP_ERROR_RESPONSE_ABORT)
     {
        _e_fm_ipc_task_remove(task);
     }
   else if (type == E_FM_OP_ERROR_RESPONSE_RETRY)
     {
        switch (task->type)
          {
           case E_FM_OP_MKDIR:
             _e_fm_ipc_mkdir_try(task);
             break;

           case E_FM_OP_MONITOR_START:
             _e_fm_ipc_monitor_start_try(task);

           default:
             break;
          }
     }
}

static Eina_Bool
_e_fm_ipc_cb_server_data(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;

   e = event;
   if (e->major != 6 /*E_IPC_DOMAIN_FM*/) return ECORE_CALLBACK_PASS_ON;
   switch (e->minor)
     {
      case E_FM_OP_MONITOR_START: /* monitor dir (and implicitly list) */
      {
         _e_fm_ipc_monitor_start(e->ref, e->data);
      }
      break;

      case E_FM_OP_MONITOR_END: /* monitor dir end */
      {
//	     printf("End listing directory: %s\n", e->data);
         _e_fm_ipc_monitor_end(e->ref, e->data);
      }
      break;

      case E_FM_OP_REMOVE: /* fop delete file/dir */
      {
         _e_fm_ipc_slave_run(E_FM_OP_REMOVE, (const char *)e->data, e->ref);
      }
      break;

      case E_FM_OP_SECURE_REMOVE: /* fop delete file/dir */
      {
         _e_fm_ipc_slave_run(E_FM_OP_SECURE_REMOVE, (const char *)e->data, e->ref);
      }
      break;

      case E_FM_OP_TRASH: /* fop trash file/dir */
      {
         E_Fop *fop;

         fop = calloc(1, sizeof(E_Fop));
         if (fop)
           {
              fop->id = e->ref;
              fop->src = eina_stringshare_add(e->data);
              _e_fops = eina_list_append(_e_fops, fop);
              fop->idler = ecore_idler_add(_e_fm_ipc_cb_fop_trash_idler, fop);
           }
      }
      break;

      case E_FM_OP_MOVE: /* fop cp file/dir && rm file/dir */
      {
         _e_fm_ipc_slave_run(E_FM_OP_MOVE, (const char *)e->data, e->ref);
      }
      break;

      case E_FM_OP_COPY: /* fop cp file/dir */
      {
         _e_fm_ipc_slave_run(E_FM_OP_COPY, (const char *)e->data, e->ref);
      }
      break;

      case E_FM_OP_SYMLINK: /* fop ln -s */
      {
         _e_fm_ipc_slave_run(E_FM_OP_SYMLINK, (const char *)e->data, e->ref);
      }
      break;

      case E_FM_OP_RENAME: /* fop mv */
      {
         _e_fm_ipc_slave_run(E_FM_OP_RENAME, (const char *)e->data, e->ref);
      }
      break;

      case E_FM_OP_MKDIR: /* fop mkdir */
      {
         const char *src, *rel;
         int rel_to, x, y;

         src = e->data;
         rel = src + strlen(src) + 1;
         memcpy(&rel_to, rel + strlen(rel) + 1, sizeof(int));
         memcpy(&x, rel + strlen(rel) + 1 + sizeof(int), sizeof(int));
         memcpy(&y, rel + strlen(rel) + 1 + sizeof(int), sizeof(int));

         _e_fm_ipc_mkdir(e->ref, src, rel, rel_to, x, y);
      }
      break;

      case E_FM_OP_MOUNT: /* mount udi mountpoint */
      {
         E_Volume *v;
         const char *udi, *mountpoint;
         int ulen;

         udi = e->data;
         v = e_volume_find(udi);
         if (!v) break;
         ulen = strlen(udi);
         if (ulen + 1 >= e->size)
           WRN("No mount point available for %s, trying anyway", udi);
         else
           {
              mountpoint = udi + strlen(udi) + 1;
              if (mountpoint[0])
                eina_stringshare_replace(&v->mount_point, mountpoint);
//             printf("REQ M %p (find from %s -> %s)\n", v, udi, mountpoint); fflush(stdout);
           }
         e_volume_mount(v);
      }
      break;

      case E_FM_OP_UNMOUNT: /* unmount udi */
      {
         E_Volume *v;
         const char *udi;

         udi = e->data;
         v = e_volume_find(udi);
         if (v)
           {
//		  printf("REQ UM\n"); fflush(stdout);
              e_volume_unmount(v);
           }
      }
      break;

      case E_FM_OP_EJECT: /* eject udi */
      {
         E_Volume *v;
         const char *udi;

         udi = e->data;
         v = e_volume_find(udi);
         if (v)
           e_volume_eject(v);
      }
      break;

      case E_FM_OP_QUIT: /* quit */
        ecore_main_loop_quit();
        break;

      case E_FM_OP_ABORT: // abort copy/move/delete operation by user
      {
         E_Fm_Slave *slave = _e_fm_ipc_slave_get(e->ref);
         if (slave)
           _e_fm_ipc_slave_send(slave, e->minor, NULL, 0);
      }
      break;

      case E_FM_OP_ERROR_RESPONSE_IGNORE_THIS:
      case E_FM_OP_ERROR_RESPONSE_IGNORE_ALL:
      case E_FM_OP_ERROR_RESPONSE_ABORT:
      case E_FM_OP_ERROR_RESPONSE_RETRY:
      {
         _e_fm_ipc_handle_error_response(e->ref, e->minor);
      }
      break;

      case E_FM_OP_OVERWRITE_RESPONSE_NO:
      case E_FM_OP_OVERWRITE_RESPONSE_NO_ALL:
      case E_FM_OP_OVERWRITE_RESPONSE_YES:
      case E_FM_OP_OVERWRITE_RESPONSE_YES_ALL:
      {
         _e_fm_ipc_slave_send(_e_fm_ipc_slave_get(e->ref), e->minor, NULL, 0);
      }
      break;

      case E_FM_OP_REORDER:
      {
         const char *file, *dst, *relative;
         int after;
         char *p = e->data;

         file = p;
         p += strlen(file) + 1;

         dst = p;
         p += strlen(dst) + 1;

         relative = p;
         p += strlen(relative) + 1;

         memcpy(&after, p, sizeof(int));

         _e_fm_ipc_reorder(file, dst, relative, after);
      }
      break;

      default:
        break;
     }
   /* always send back an "OK" for each request so e can basically keep a
    * count of outstanding requests - maybe for balancing between fm
    * slaves later. ref_to is set to the the ref id in the request to
    * allow for async handling later */
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_OK,
                         0, e->ref, 0, NULL, 0);
   return ECORE_CALLBACK_PASS_ON;
}

static int
_e_fm_ipc_client_send(int id, E_Fm_Op_Type type, void *data, int size)
{
   return ecore_ipc_server_send(_e_fm_ipc_server,
                                6 /*E_IPC_DOMAIN_FM*/,
                                type,
                                id, 0, 0, data, size);
}

static int
_e_fm_ipc_slave_run(E_Fm_Op_Type type, const char *args, int id)
{
   E_Fm_Slave *slave;
   char *command;

   slave = malloc(sizeof(E_Fm_Slave));

   if (!slave) return 0;

   command = _e_fm_ipc_prepare_command(type, args);
   if (!command)
     {
        free(slave);
        return 0;
     }

   slave->id = id;
   slave->exe = ecore_exe_pipe_run(command, ECORE_EXE_PIPE_WRITE | ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR, slave);
//   printf("EFM command: %s\n", command);

   free(command);

   _e_fm_ipc_slaves = eina_list_append(_e_fm_ipc_slaves, slave);

   return !!slave->exe;
}

static E_Fm_Slave *
_e_fm_ipc_slave_get(int id)
{
   Eina_List *l;
   E_Fm_Slave *slave;

   EINA_LIST_FOREACH(_e_fm_ipc_slaves, l, slave)
     {
        if (slave->id == id)
          return slave;
     }

   return NULL;
}

static int
_e_fm_ipc_slave_send(E_Fm_Slave *slave, E_Fm_Op_Type type, void *data, int size)
{
   char *sdata;
   int ssize;
   int magic = E_FM_OP_MAGIC;
   int result;

   ssize = 3 * sizeof(int) + size;
   sdata = malloc(ssize);

   if (!sdata) return 0;

   memcpy(sdata, &magic, sizeof(int));
   memcpy(sdata + sizeof(int), &type, sizeof(E_Fm_Op_Type));
   memcpy(sdata + sizeof(int) + sizeof(E_Fm_Op_Type), &size, sizeof(int));

   memcpy(sdata + 2 * sizeof(int) + sizeof(E_Fm_Op_Type), data, size);

   result = ecore_exe_send(slave->exe, sdata, ssize);

   free(sdata);

   return result;
}

Eina_Bool
_e_fm_ipc_slave_data_cb(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Data *e = event;
   E_Fm_Slave *slave;
   int magic, id, size;
   char *sdata;
   int ssize;

   if (!e) return ECORE_CALLBACK_PASS_ON;

   slave = ecore_exe_data_get(e->exe);
   if (!slave) return ECORE_CALLBACK_RENEW;
   if (data) return ECORE_CALLBACK_PASS_ON; /* ipc handlers have NULL data */
   if (!eina_list_data_find(_e_fm_ipc_slaves, slave)) return ECORE_CALLBACK_PASS_ON;

   sdata = e->data;
   ssize = e->size;

   while ((unsigned int)ssize >= 3 * sizeof(int))
     {
        memcpy(&magic, sdata, sizeof(int));
        memcpy(&id, sdata + sizeof(int), sizeof(int));
        memcpy(&size, sdata + sizeof(int) + sizeof(int), sizeof(int));

        if (magic != E_FM_OP_MAGIC)
          {
             printf("%s:%s(%d) Wrong magic number from slave #%d. ", __FILE__, __FUNCTION__, __LINE__, slave->id);
             break;
          }

        sdata += 3 * sizeof(int);
        ssize -= 3 * sizeof(int);

        if (id == E_FM_OP_OVERWRITE)
          {
             _e_fm_ipc_client_send(slave->id, E_FM_OP_OVERWRITE, sdata, size);
             printf("%s:%s(%d) Overwrite sent to client from slave #%d.\n", __FILE__, __FUNCTION__, __LINE__, slave->id);
          }
        else if (id == E_FM_OP_ERROR)
          {
             _e_fm_ipc_client_send(slave->id, E_FM_OP_ERROR, sdata, size);
          }
        else if (id == E_FM_OP_PROGRESS)
          {
             _e_fm_ipc_client_send(slave->id, E_FM_OP_PROGRESS, sdata, size);
          }

        sdata += size;
        ssize -= size;
     }

   return ECORE_CALLBACK_PASS_ON;
}

Eina_Bool
_e_fm_ipc_slave_error_cb(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Data *e = event;
   E_Fm_Slave *slave;

   if (!e) return 1;

   slave = ecore_exe_data_get(e->exe);
   if (!slave) return ECORE_CALLBACK_RENEW;
   if (data) return ECORE_CALLBACK_PASS_ON; /* ipc handlers have NULL data */
   if (!eina_list_data_find(_e_fm_ipc_slaves, slave)) return ECORE_CALLBACK_PASS_ON;

   printf("EFM: Data from STDERR of slave #%d: %.*s", slave->id, e->size, (char *)e->data);

   return 1;
}

Eina_Bool
_e_fm_ipc_slave_del_cb(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *e = event;
   E_Fm_Slave *slave;
   Eina_List *l;

   if (!e) return ECORE_CALLBACK_RENEW;

   slave = ecore_exe_data_get(e->exe);
   if (!slave) return ECORE_CALLBACK_RENEW;
   if (data) return ECORE_CALLBACK_PASS_ON; /* ipc handlers have NULL data */
   l = eina_list_data_find_list(_e_fm_ipc_slaves, slave);
   if (!l) return ECORE_CALLBACK_PASS_ON;
   _e_fm_ipc_client_send(slave->id, E_FM_OP_QUIT, NULL, 0);

   _e_fm_ipc_slaves = eina_list_remove(_e_fm_ipc_slaves, l);
   free(slave);

   return ECORE_CALLBACK_RENEW;
}

static void
_e_fm_ipc_cb_file_monitor(void *data EINA_UNUSED, Ecore_File_Monitor *em EINA_UNUSED, Ecore_File_Event event, const char *path)
{
   E_Dir *ed;
   char *dir, *rp, *drp;
   Eina_List *l;

   dir = ecore_file_dir_get(path);
   /* FIXME: get no create events if dir is empty */
   if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
       (event == ECORE_FILE_EVENT_CREATED_DIRECTORY))
     {
        rp = ecore_file_realpath(dir);
        EINA_LIST_FOREACH(_e_dirs, l, ed)
          {
             drp = ecore_file_realpath(ed->dir);
             if (drp)
               {
                  if (!strcmp(rp, drp))
                    _e_fm_ipc_file_add(ed, path, 0);
                  free(drp);
               }
          }
        free(rp);
     }
   else if ((event == ECORE_FILE_EVENT_DELETED_FILE) ||
            (event == ECORE_FILE_EVENT_DELETED_DIRECTORY))
     {
        rp = ecore_file_realpath(dir);
        EINA_LIST_FOREACH(_e_dirs, l, ed)
          {
             drp = ecore_file_realpath(ed->dir);
             if (drp)
               {
                  if (!strcmp(rp, drp))
                    _e_fm_ipc_file_del(ed, path);
                  free(drp);
               }
          }
        free(rp);
     }
   else if (event == ECORE_FILE_EVENT_MODIFIED)
     {
        rp = ecore_file_realpath(dir);
        EINA_LIST_FOREACH(_e_dirs, l, ed)
          {
             drp = ecore_file_realpath(ed->dir);
             if (drp)
               {
                  if (!strcmp(rp, drp))
                    _e_fm_ipc_file_mod(ed, path);
                  free(drp);
               }
          }
        free(rp);
     }
   else if (event == ECORE_FILE_EVENT_DELETED_SELF)
     {
        rp = ecore_file_realpath(path);
        EINA_LIST_FOREACH(_e_dirs, l, ed)
          {
             drp = ecore_file_realpath(ed->dir);
             if (drp)
               {
                  if (!strcmp(rp, drp))
                    _e_fm_ipc_file_mon_dir_del(ed, path);
                  free(drp);
               }
          }
        free(rp);
     }
   free(dir);
}

static Eina_Bool
_e_fm_ipc_cb_recent_clean(void *data)
{
   E_Dir *ed;
   Eina_List *l, *pl;
   E_Mod *m;
   double t_now;

   ed = data;
   ed->cleaning = 1;
   t_now = ecore_time_unix_get();
   EINA_LIST_FOREACH_SAFE (ed->recent_mods, pl, l, m)
     if ((m->mod) && ((t_now - m->timestamp) >= DEF_MOD_BACKOFF))
       {
          ed->recent_mods = eina_list_remove_list(ed->recent_mods, pl);
          if (!m->done) _e_fm_ipc_file_add_mod(ed, m->path, 5, 0);
          eina_stringshare_del(m->path);
          free(m);
       }
   ed->cleaning = 0;
   if (ed->recent_mods) return ECORE_CALLBACK_RENEW;
   ed->recent_clean = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_ipc_file_add_mod(E_Dir *ed, const char *path, E_Fm_Op_Type op, int listing)
{
   struct stat st;
   char *lnk = NULL, *rlnk = NULL;
   int broken_lnk = 0;
   Eina_Binbuf *buf;
   /* file add/change format is as follows:
    *
    * stat_info[stat size] + broken_link[1] + path[n]\0 + lnk[n]\0 + rlnk[n]\0 */

   /* FIXME: handle BACKOFF */
   if ((!listing) && (op == E_FM_OP_FILE_CHANGE) && (!ed->cleaning)) /* 5 == mod */
     {
        Eina_List *l;
        E_Mod *m;
        double t_now;
        int skip = 0;

        t_now = ecore_time_unix_get();
        EINA_LIST_FOREACH(ed->recent_mods, l, m)
          {
             if ((m->mod) && (!strcmp(m->path, path)))
               {
                  if ((t_now - m->timestamp) < DEF_MOD_BACKOFF)
                    {
                       m->done = 0;
                       skip = 1;
                    }
               }
          }
        if (!skip)
          {
             m = calloc(1, sizeof(E_Mod));
             m->path = eina_stringshare_add(path);
             m->mod = 1;
             m->done = 1;
             m->timestamp = t_now;
             ed->recent_mods = eina_list_append(ed->recent_mods, m);
          }
        if ((!ed->recent_clean) && (ed->recent_mods))
          ed->recent_clean = ecore_timer_loop_add(DEF_MOD_BACKOFF, _e_fm_ipc_cb_recent_clean, ed);
        if (skip)
          {
//	     printf("SKIP MOD %s %3.3f\n", path, t_now);
             return;
          }
     }
//   printf("MOD %s %3.3f\n", path, ecore_time_unix_get());
   lnk = ecore_file_readlink(path);
   memset(&st, 0, sizeof(struct stat));
   if (stat((lnk && lnk[0]) ? lnk : path, &st) == -1)
     {
        if ((path[0] == 0) || (lnk)) broken_lnk = 1;
        else return;
     }
   if ((lnk) && (lnk[0] != '/'))
     {
        rlnk = ecore_file_realpath(path);
        if ((rlnk == NULL) || (rlnk[0] == 0) ||
            (stat(rlnk, &st) == -1))
          broken_lnk = 1;
        else
          broken_lnk = 0;
     }
   else if (lnk)
     rlnk = strdup(lnk);
   if (!lnk) lnk = strdup("");
   if (!rlnk) rlnk = strdup("");

   buf = eina_binbuf_new();
   /* NOTE: i am NOT converting this data to portable arch/os independent
    * format. i am ASSUMING e_fm_main and e are local and built together
    * and thus this will work. if this ever changes this here needs to
    * change */
   eina_binbuf_append_length(buf, (void*)&st, sizeof(struct stat));

   eina_binbuf_append_char(buf, !!broken_lnk);
   eina_binbuf_append_length(buf, (void*)path, strlen(path) + 1);
   eina_binbuf_append_length(buf, (void*)lnk, strlen(lnk) + 1);
   eina_binbuf_append_length(buf, (void*)rlnk, strlen(rlnk) + 1);

   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, op, 0, ed->id,
                         listing, eina_binbuf_string_get(buf), eina_binbuf_length_get(buf));
   eina_binbuf_free(buf);
   free(lnk);
   free(rlnk);
}

static void
_e_fm_ipc_file_add(E_Dir *ed, const char *path, int listing)
{
   if (!listing)
     {
        /* FIXME: handle BACKOFF */
     }
   _e_fm_ipc_file_add_mod(ed, path, E_FM_OP_FILE_ADD, listing); /*file add*/
}

static void
_e_fm_ipc_file_del(E_Dir *ed, const char *path)
{
   {
      /* FIXME: handle BACKOFF */
   }
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_FILE_DEL,
                         0, ed->id, 0, (void *)path, strlen(path) + 1);
}

static void
_e_fm_ipc_file_mod(E_Dir *ed, const char *path)
{
   {
      /* FIXME: handle BACKOFF */
   }
   _e_fm_ipc_file_add_mod(ed, path, E_FM_OP_FILE_CHANGE, 0); /*file change*/
}

static void
_e_fm_ipc_file_mon_dir_del(E_Dir *ed, const char *path)
{
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_MONITOR_END,
                         0, ed->id, 0, (void *)path, strlen(path) + 1);
}

static Eina_Bool
_e_fm_ipc_cb_fop_trash_idler(void *data)
{
   E_Fop *fop = NULL;
   FILE *info = NULL;
   const char *filename;
   const char *escname = NULL;
   char *dest;
   char buf[PATH_MAX + PATH_MAX + 20], trash_dir[PATH_MAX];
   unsigned int i = 0;
   struct tm *lt;
   time_t t;

   /* FIXME: For now, this will only implement 'home trash'
    * Later, implement mount/remote/removable device trash, if wanted. */

   fop = (E_Fop *)data;
   if (!fop) return 0;

   /* Check that 'home trash' and subsequesnt dirs exists, create if not */
   snprintf(trash_dir, sizeof(trash_dir), "%s/Trash", efreet_data_home_get());
   snprintf(buf, sizeof(buf), "%s/files", trash_dir);
   if (!ecore_file_mkpath(buf)) return 0;
   snprintf(buf, sizeof(buf), "%s/info", trash_dir);
   if (!ecore_file_mkpath(buf)) return 0;

   filename = strrchr(fop->src, '/');
   if (!filename) return 0;
   escname = ecore_file_escape_name(filename);

   /* Find path for info file. Pointer address is part of the filename to
    * alleviate some of the looping in case of multiple filenames with the
    * same name. Also use the name of the file to help */
   do
     {
        snprintf(buf, sizeof(buf), "%s/file%s.%p.%u", trash_dir, escname,
                 fop, i++);
     }
   while (ecore_file_exists(buf));
   dest = strdup(buf);

   /* Try to move the file */
   if (rename(fop->src, dest))
     {
        if (errno == EXDEV)
          {
             /* Move failed. Spec says delete files that can't be trashed */
             free(dest);
             ecore_file_unlink(fop->src);
             return ECORE_CALLBACK_CANCEL;
          }
     }

   /* Move worked. Create info file */
   snprintf(buf, sizeof(buf), "%s/info%s.%p.%u.trashinfo", trash_dir,
            escname, fop, --i);
   info = fopen(buf, "w");
   if (info)
     {
        t = time(NULL);
        lt = localtime(&t);

        /* Insert info for trashed file */
        fprintf(info,
                "[Trash Info]\nPath=%s\nDeletionDate=%04u%02u%02uT%02u:%02u:%02u",
                fop->src, lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                lt->tm_hour, lt->tm_min, lt->tm_sec);
        fclose(info);
     }
   else
     /* Could not create info file. Spec says to put orig file back */
     if (rename(dest, fop->src) < 0)
       ERR("File could not be put back!");

   free(dest);
   eina_stringshare_del(fop->src);
   eina_stringshare_del(fop->dst);
   _e_fops = eina_list_remove(_e_fops, fop);
   free(fop);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_ipc_reorder(const char *file, const char *dst, const char *relative, int after)
{
   char buffer[PATH_MAX];
   char order[PATH_MAX];

   if (!file || !dst || !relative) return;
   if (after != 0 && after != 1 && after != 2) return;
//   printf("%s:%s(%d) Reorder:\n\tfile = %s\n\tdst = %s\n\trelative = %s\n\tafter = %d\n", __FILE__, __FUNCTION__, __LINE__, file, dst, relative, after);

   snprintf(order, sizeof(order), "%s/.order", dst);
   if (ecore_file_exists(order))
     {
        FILE *forder;
        Eina_List *files = NULL, *l;
        char *str;

        forder = fopen(order, "r");
        if (forder)
          {
             int len;

             /* inset files in order if the existed in file
              * list before */
             while (fgets(buffer, sizeof(buffer), forder))
               {
                  len = strlen(buffer);
                  if (len > 0) buffer[len - 1] = 0;
                  files = eina_list_append(files, strdup(buffer));
               }
             fclose(forder);
          }
        /* remove dest file from .order - if there */
        EINA_LIST_FOREACH(files, l, str)
          if (!strcmp(str, file))
            {
               free(str);
               files = eina_list_remove_list(files, l);
               break;
            }
        /* now insert dest into list or replace entry */
        EINA_LIST_FOREACH(files, l, str)
          {
             if (!strcmp(str, relative))
               {
                  if (after == 2) /* replace */
                    {
                       free(str);
                       l->data = strdup(file);
                    }
                  else if (after == 0) /* before */
                    {
                       files = eina_list_prepend_relative_list(files, strdup(file), l);
                    }
                  else if (after == 1) /* after */
                    {
                       files = eina_list_append_relative_list(files, strdup(file), l);
                    }
                  break;
               }
          }

        forder = fopen(order, "w");
        if (forder)
          {
             EINA_LIST_FREE(files, str)
               {
                  fprintf(forder, "%s\n", str);
                  free(str);
               }
             fclose(forder);
          }
     }
}

static void
_e_fm_ipc_dir_del(E_Dir *ed)
{
   void *data;
   E_Mod *m;

   if (ed->lister_thread)
     {
        ed->delete_me = EINA_TRUE;
        ecore_thread_cancel(ed->lister_thread);
        return;
     }
   eina_stringshare_del(ed->dir);
   if (ed->recent_clean) ecore_timer_del(ed->recent_clean);
   EINA_LIST_FREE(ed->recent_mods, m)
     {
        eina_stringshare_del(m->path);
        free(m);
     }
   EINA_LIST_FREE(ed->fq, data) eina_stringshare_del(data);
   free(ed);
}

static char *
_e_fm_ipc_prepare_command(E_Fm_Op_Type type, const char *args)
{
   char *buffer;
   const char *libdir;
   unsigned int length = 0;
   char command[4];

   if (type == E_FM_OP_MOVE)
     strcpy(command, "mv");
   else if (type == E_FM_OP_REMOVE)
     strcpy(command, "rm");
   else if (type == E_FM_OP_SECURE_REMOVE)
     strcpy(command, "srm");
   else if (type == E_FM_OP_COPY)
     strcpy(command, "cp");
   else if (type == E_FM_OP_SYMLINK)
     strcpy(command, "lns");
   else if (type == E_FM_OP_RENAME)
     strcpy(command, "mvf");
   else
     return NULL;

   libdir = getenv("E_LIB_DIR") ?: PACKAGE_LIB_DIR;
   length = 256 + strlen(libdir) + strlen(args);
   buffer = malloc(length);
   snprintf(buffer, length,
            "%s/enlightenment/utils/enlightenment_fm_op %s %s",
            getenv("E_LIB_DIR"), command, args);

   return buffer;
}

static Eina_Bool
_e_fm_ipc_timer_cb(void *d EINA_UNUSED)
{
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_VOLUME_LIST_DONE,
                         0, 0, 0, "", 1);
   _init_timer_run = EINA_TRUE;
   return EINA_FALSE;
}

void
e_fm_ipc_volume_add(E_Volume *v)
{
   void *msg_data;
   int msg_size;

   if (_init_timer && (!_init_timer_run))
     ecore_timer_loop_reset(_init_timer);
   else if ((!_init_timer) && (!_init_timer_run))
     _init_timer = ecore_timer_loop_add(2.0, _e_fm_ipc_timer_cb, NULL);

   msg_data = _e_fm_shared_codec_volume_encode(v, &msg_size);
   if (!msg_data) return;
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_VOLUME_ADD,
                         0, 0, 0, msg_data, msg_size);
   free(msg_data);
}
