#include "config.h"

#ifndef _FILE_OFFSET_BITS
# define _FILE_OFFSET_BITS 64
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca(size_t);
#endif

#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>

#include <Ecore.h>
#include <Ecore_File.h>

#include <eina_stringshare.h>

#define E_TYPEDEFS
#include "e_fm_op.h"
#undef E_TYPEDEFS
#include "e_fm_op.h"

#ifndef strdupa
# define strdupa(str) strcpy(alloca(strlen(str) + 1), str)
#endif

#define READBUFSIZE     65536
#define COPYBUFSIZE     16384
#define REMOVECHUNKSIZE 4096
#define NB_PASS         3

#define E_FREE(p) do { free(p); p = NULL; } while (0)

#define _E_FM_OP_ERROR_SEND_SCAN(_task, _e_fm_op_error_type, _fmt, ...)                      \
  do                                                                                         \
    {                                                                                        \
       int _errno = errno;                                                                   \
       _e_fm_op_scan_error = 1;                                                              \
       _e_fm_op_send_error(_task, _e_fm_op_error_type, _fmt, __VA_ARGS__, strerror(_errno)); \
       return 1;                                                                             \
    }                                                                                        \
  while (0)

#define _E_FM_OP_ERROR_SEND_WORK(_task, _e_fm_op_error_type, _fmt, ...)                      \
  do                                                                                         \
    {                                                                                        \
       int _errno = errno;                                                                   \
       _e_fm_op_work_error = 1;                                                              \
       _e_fm_op_send_error(_task, _e_fm_op_error_type, _fmt, __VA_ARGS__, strerror(_errno)); \
       return 1;                                                                             \
    }                                                                                        \
  while (0)

typedef struct _E_Fm_Op_Task      E_Fm_Op_Task;
typedef struct _E_Fm_Op_Copy_Data E_Fm_Op_Copy_Data;

static E_Fm_Op_Task *_e_fm_op_task_new(void);
static void          _e_fm_op_task_free(void *t);

static void          _e_fm_op_remove_link_task(E_Fm_Op_Task *task);
static Eina_Bool     _e_fm_op_stdin_data(void *data, Ecore_Fd_Handler *fd_handler);
static void          _e_fm_op_set_up_idlers(void);
static void          _e_fm_op_delete_idler(int *mark);
static int           _e_fm_op_idler_handle_error(int *mark, Eina_List **queue, Eina_List **node, E_Fm_Op_Task *task);

static Eina_Bool     _e_fm_op_work_idler(void *data);
static Eina_Bool     _e_fm_op_scan_idler(void *data);

static void          _e_fm_op_send_error(E_Fm_Op_Task *task, E_Fm_Op_Type type, const char *fmt, ...) EINA_PRINTF(3, 4);
static void          _e_fm_op_rollback(E_Fm_Op_Task *task);
static void          _e_fm_op_update_progress_report_simple(int percent, const char *src, const char *dst);
static void          _e_fm_op_update_progress(E_Fm_Op_Task *task, off_t _plus_e_fm_op_done, off_t _plus_e_fm_op_total);
static void          _e_fm_op_copy_stat_info(E_Fm_Op_Task *task);
static int           _e_fm_op_handle_overwrite(E_Fm_Op_Task *task);

static int           _e_fm_op_copy_dir(E_Fm_Op_Task *task);
static int           _e_fm_op_copy_link(E_Fm_Op_Task *task);
static int           _e_fm_op_copy_fifo(E_Fm_Op_Task *task);
static int           _e_fm_op_open_files(E_Fm_Op_Task *task);
static int           _e_fm_op_copy_chunk(E_Fm_Op_Task *task);

static int           _e_fm_op_copy_atom(E_Fm_Op_Task *task);
static int           _e_fm_op_scan_atom(E_Fm_Op_Task *task);
static int           _e_fm_op_copy_stat_info_atom(E_Fm_Op_Task *task);
static int           _e_fm_op_symlink_atom(E_Fm_Op_Task *task);
static int           _e_fm_op_remove_atom(E_Fm_Op_Task *task);
static int           _e_fm_op_rename_atom(E_Fm_Op_Task *task);
static int           _e_fm_op_destroy_atom(E_Fm_Op_Task *task);
static void          _e_fm_op_random_buf(char *buf, ssize_t len);
static void          _e_fm_op_random_char(char *buf, size_t len);

static int           _e_fm_op_make_copy_name(const char *abs, char *buf, size_t buf_size);

Eina_List *_e_fm_op_work_queue = NULL, *_e_fm_op_scan_queue = NULL;
Ecore_Idler *_e_fm_op_work_idler_p = NULL, *_e_fm_op_scan_idler_p = NULL;

int _e_fm_op_abort = 0; /* Abort mark. */
int _e_fm_op_scan_error = 0;
int _e_fm_op_work_error = 0;
int _e_fm_op_overwrite = 0;

int _e_fm_op_error_response = E_FM_OP_NONE;
int _e_fm_op_overwrite_response = E_FM_OP_NONE;

Eina_List *_e_fm_op_separator = NULL;

char *_e_fm_op_stdin_buffer = NULL;

struct _E_Fm_Op_Task
{
   struct
   {
      const char *name;
      struct stat st;
   } src;

   struct
   {
      const char *name;
      size_t      done;
   } dst;

   int          started, finished;
   unsigned int passes;
   off_t pos;

   void        *data;

   E_Fm_Op_Task *parent;
   E_Fm_Op_Type type;
   E_Fm_Op_Type overwrite;

   Eina_List   *link;
};

struct _E_Fm_Op_Copy_Data
{
   FILE *from;
   FILE *to;
};

int
main(int argc, char **argv)
{
   int i, last;
   E_Fm_Op_Type type;

   ecore_init();

   _e_fm_op_stdin_buffer = malloc(READBUFSIZE);
   if (!_e_fm_op_stdin_buffer) return 0;

   ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ, _e_fm_op_stdin_data,
                             NULL, NULL, NULL);

   if (argc < 3) return 0;

   last = argc - 1;
   i = 2;

   if (!strcmp(argv[1], "cp")) type = E_FM_OP_COPY;
   else if (!strcmp(argv[1], "mv"))
     type = E_FM_OP_MOVE;
   else if (!strcmp(argv[1], "rm"))
     type = E_FM_OP_REMOVE;
   else if (!strcmp(argv[1], "srm"))
     type = E_FM_OP_SECURE_REMOVE;
   else if (!strcmp(argv[1], "lns"))
     type = E_FM_OP_SYMLINK;
   else if (!strcmp(argv[1], "mvf"))
     type = E_FM_OP_RENAME;
   else return 0;

   if (type == E_FM_OP_SECURE_REMOVE)
     {
        _e_fm_op_work_queue = eina_list_append(_e_fm_op_work_queue, NULL);
        _e_fm_op_separator = _e_fm_op_work_queue;
     }

   if ((type == E_FM_OP_COPY) ||
       (type == E_FM_OP_SYMLINK) ||
       (type == E_FM_OP_MOVE) ||
       (type == E_FM_OP_RENAME))
     {
        if (argc < 4) goto quit;

        if (ecore_file_is_dir(argv[last]))
          {
             char buf[PATH_MAX];
             char *p2, *p3;
             int p2_len, last_len, done, total;

             p2 = ecore_file_realpath(argv[last]);
             if (!p2) goto quit;
             p2_len = strlen(p2);

             last_len = strlen(argv[last]);
             if ((last_len < 1) || (last_len + 2 >= PATH_MAX))
               {
                  E_FREE(p2);
                  goto quit;
               }
             memcpy(buf, argv[last], last_len);
             if (buf[last_len - 1] != '/')
               {
                  buf[last_len] = '/';
                  last_len++;
               }

             p3 = buf + last_len;

             done = 0;
             total = last - 2;

             for (; i < last; i++)
               {
                  const char *name;
                  size_t name_len;
                  char *dir;
                  int r;
                  Eina_Bool make_copy;

                  /* Don't move a dir into itself */
                  if (ecore_file_is_dir(argv[i]) &&
                      (strcmp(argv[i], p2) == 0))
                    goto skip_arg;

                  make_copy = EINA_FALSE;
                  /* Don't move/rename/symlink into same directory
                   * in case of copy, make copy name automatically */
                  dir = ecore_file_dir_get(argv[i]);
                  if (dir)
                    {
                       r = (strcmp(dir, p2) == 0);
                       E_FREE(dir);
                       if (r)
                         {
                            if (type == E_FM_OP_COPY) make_copy = EINA_TRUE;
                            else goto skip_arg;
                         }
                    }

                  name = ecore_file_file_get(argv[i]);
                  if (!name) goto skip_arg;
                  name_len = strlen(name);
                  if (p2_len + name_len >= PATH_MAX) goto skip_arg;
                  memcpy(p3, name, name_len + 1);
                  if (make_copy)
                    {
                      if (_e_fm_op_make_copy_name(buf, p3, PATH_MAX - p2_len - 1))
                         goto skip_arg;
                    }

                  if ((type == E_FM_OP_SYMLINK) &&
                           (symlink(argv[i], buf) == 0))
                    {
                       done++;
                       _e_fm_op_update_progress_report_simple
                         (done * 100 / total, argv[i], buf);
                    }
                  else
                    {
                       if (type == E_FM_OP_MOVE)
                         {
                            if (!strcmp(argv[i], buf))
                              goto skip_arg;

                            if (buf[0] != '/')
                              {
                                 free(p2);
                                 _E_FM_OP_ERROR_SEND_SCAN(0, E_FM_OP_ERROR,
                                                          "Unknown destination '%s': %s.", buf);
                              }
                         }
                       else if (type == E_FM_OP_RENAME)
                         {
                            if (!strcmp(argv[i], buf))
                              goto skip_arg;

                            if (buf[0] != '/')
                              {
                                 free(p2);
                                 _E_FM_OP_ERROR_SEND_SCAN(0, E_FM_OP_ERROR,
                                                          "Unknown destination '%s': %s.", buf);
                              }

                            if (access(buf, F_OK) == -1)
                              {
                                 /* race condition, i know, but it's
                                  * unvoidable. */
                                 if (rename(argv[i], buf) == -1)
                                   {
                                      /* if it's another device */
                                      if (errno == EXDEV)
                                        type = E_FM_OP_MOVE;
                                      else
                                        {
                                           free(p2);
                                           _E_FM_OP_ERROR_SEND_SCAN(0, E_FM_OP_ERROR,
                                                                    "Cannot move '%s' to '%s': %s.",
                                                                    argv[i], buf);
                                        }
                                   }
                                 else
                                   {
                                      done++;
                                      _e_fm_op_update_progress_report_simple
                                        (done * 100 / total, argv[i], buf);
                                      goto skip_arg;
                                   }
                              }
                            else
                              {
                                 /* if the destination file already exists,
                                  * store a task which handles overwrite */
                                 struct stat st1;
                                 struct stat st2;
                                 if ((stat(argv[i], &st1) == 0) &&
                                     (stat(buf, &st2) == 0))
                                   {
                                      /* if files are not on the same device */
                                      if (st1.st_dev != st2.st_dev)
                                        type = E_FM_OP_MOVE;
                                   }
                                 else type = E_FM_OP_MOVE;
                              }
                         }

                       if (type == E_FM_OP_MOVE)
                         {
                            _e_fm_op_work_queue = eina_list_append(_e_fm_op_work_queue, NULL);
                            _e_fm_op_separator = _e_fm_op_work_queue;
                         }

                       E_Fm_Op_Task *task;

                       task = _e_fm_op_task_new();
                       task->type = type;
                       task->src.name = eina_stringshare_add(argv[i]);
                       task->dst.name = eina_stringshare_add(buf);

                       _e_fm_op_scan_queue =
                         eina_list_append(_e_fm_op_scan_queue, task);
                    }

skip_arg:
                  continue;
               }

             E_FREE(p2);
          }
        else if (argc == 4)
          {
             char *p, *p2;

             p = ecore_file_realpath(argv[2]);
             p2 = ecore_file_realpath(argv[3]);

             /* Don't move a file on top of itself. */
             i = (strcmp(p, p2) == 0);
             E_FREE(p);
             E_FREE(p2);
             if (i) goto quit;

             if ((type == E_FM_OP_SYMLINK) &&
                      (symlink(argv[2], argv[3]) == 0))
               {
                  _e_fm_op_update_progress_report_simple(100, argv[2], argv[3]);
                  goto quit;
               }
             else
               {
                  if (type == E_FM_OP_MOVE)
                    type = E_FM_OP_RENAME;

                  E_Fm_Op_Task *task;

                  task = _e_fm_op_task_new();
                  task->type = type;
                  task->src.name = eina_stringshare_add(argv[2]);
                  task->dst.name = eina_stringshare_add(argv[3]);
                  _e_fm_op_scan_queue = eina_list_append(_e_fm_op_scan_queue, task);
               }
          }
        else
          goto quit;
     }
   else if ((type == E_FM_OP_REMOVE) || (type == E_FM_OP_SECURE_REMOVE))
     {
        E_Fm_Op_Task *task;

        while (i <= last)
          {
             task = _e_fm_op_task_new();
             task->type = type;
             task->src.name = eina_stringshare_add(argv[i]);
             _e_fm_op_scan_queue = eina_list_append(_e_fm_op_scan_queue, task);
             i++;
          }
     }

   _e_fm_op_set_up_idlers();

   ecore_main_loop_begin();

quit:
   ecore_shutdown();

   E_FREE(_e_fm_op_stdin_buffer);

   E_FM_OP_DEBUG("Slave quit.\n");

   return 0;
}

/* Create new task. */
static E_Fm_Op_Task *
_e_fm_op_task_new()
{
   E_Fm_Op_Task *t;

   t = malloc(sizeof(E_Fm_Op_Task));
   t->src.name = NULL;
   memset(&(t->src.st), 0, sizeof(struct stat));

   t->dst.name = NULL;
   t->dst.done = 0;
   t->started = 0;
   t->finished = 0;
   t->data = NULL;
   t->type = E_FM_OP_NONE;
   t->overwrite = E_FM_OP_NONE;
   t->link = NULL;
   t->pos = t->passes = 0;

   return t;
}

/* Free task. */
static void
_e_fm_op_task_free(void *t)
{
   E_Fm_Op_Task *task = t;
   E_Fm_Op_Copy_Data *data;

   if (!task) return;

   if (task->src.name) eina_stringshare_del(task->src.name);
   if (task->dst.name) eina_stringshare_del(task->dst.name);

   if (task->data)
     {
        data = task->data;
        if (task->type == E_FM_OP_COPY)
          {
             if (data->from) fclose(data->from);
             if (data->to) fclose(data->to);
          }
        E_FREE(task->data);
     }
   E_FREE(task);
}

/* Removes link task from work queue.
 * Link task is not NULL in case of MOVE. Then two tasks are created: copy and remove.
 * Remove task is a link task for the copy task. If copy task is aborted (e.g. error
 * occurred and user chooses to ignore this), then the remove task is removed from
 * queue with this functions.
 */
static void
_e_fm_op_remove_link_task(E_Fm_Op_Task *task)
{
   E_Fm_Op_Task *ltask;

   if (!task) return;
   if (task->link)
     {
        ltask = eina_list_data_get(task->link);
        _e_fm_op_work_queue =
          eina_list_remove_list(_e_fm_op_work_queue, task->link);
        _e_fm_op_task_free(ltask);
        task->link = NULL;
     }
}

/*
 * Handles data from STDIN.
 * Received data must be in this format:
 * 1) (int) magic number,
 * 2) (int) id,
 * 3) (int) message length.
 * Right now message length is always 0. Id is what matters.
 *
 * This function uses a couple of static variables and a global
 * variable _e_fm_op_stdin_buffer to deal with a situation, when read()
 * did not actually read enough data.
 */
static Eina_Bool
_e_fm_op_stdin_data(void *data __UNUSED__, Ecore_Fd_Handler *fd_handler)
{
   int fd;
   static char *buf = NULL;
   static int length = 0;
   char *begin = NULL;
   ssize_t num = 0;
   int msize, identity;

   fd = ecore_main_fd_handler_fd_get(fd_handler);
   if (!buf)
     {
        buf = _e_fm_op_stdin_buffer;
        length = 0;
     }

   num = read(fd, buf, READBUFSIZE - length);

   if (num == 0)
     {
        E_FM_OP_DEBUG("STDIN was closed. Abort. \n");
        _e_fm_op_abort = 1;
     }
   else if (num < 0)
     {
        E_FM_OP_DEBUG("Error while reading from STDIN: read returned -1. (%s) Abort. \n", strerror(errno));
        _e_fm_op_abort = 1;
     }
   else
     {
        length += num;

        buf = _e_fm_op_stdin_buffer;
        begin = _e_fm_op_stdin_buffer;

        while (length >= ((int)(3 * sizeof(int))))
          {
             begin = buf;

             /* Check magic. */
             if (*((int *)(void *)buf) != E_FM_OP_MAGIC)
               {
                  E_FM_OP_DEBUG("Error while reading from STDIN: magic is not correct!\n");
                  break;
               }
             buf += sizeof(int);

             /* Read indentifying data. */
             memcpy(&identity, buf, sizeof(int));
             buf += sizeof(int);

             /* Read message length. */
             memcpy(&msize, buf, sizeof(int));
             buf += sizeof(int);

             if ((length - 3 * (int)sizeof(int)) < msize)
               {
                  /* There is not enough data to read the whole message. */
                  break;
               }

             length -= (3 * sizeof(int));

             /* You may want to read msize bytes of data too,
              * but currently commands here do not have any data.
              * msize is always 0.
              */
             switch (identity)
               {
                case E_FM_OP_ABORT:
                  _e_fm_op_abort = 1;
                  E_FM_OP_DEBUG("Aborted.\n");
                  break;

                case E_FM_OP_ERROR_RESPONSE_ABORT:
                case E_FM_OP_ERROR_RESPONSE_IGNORE_THIS:
                case E_FM_OP_ERROR_RESPONSE_IGNORE_ALL:
                case E_FM_OP_ERROR_RESPONSE_RETRY:
                  _e_fm_op_error_response = identity;
                  _e_fm_op_set_up_idlers();
                  break;

                case E_FM_OP_OVERWRITE_RESPONSE_NO:
                case E_FM_OP_OVERWRITE_RESPONSE_NO_ALL:
                case E_FM_OP_OVERWRITE_RESPONSE_YES:
                case E_FM_OP_OVERWRITE_RESPONSE_YES_ALL:
                  _e_fm_op_overwrite_response = identity;
                  _e_fm_op_set_up_idlers();
                  E_FM_OP_DEBUG("Overwrite response set.\n");
                  break;
               }
          }
        if (length > 0) memmove(_e_fm_op_stdin_buffer, begin, length);
        buf = _e_fm_op_stdin_buffer + length;
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_e_fm_op_set_up_idlers()
{
   if (!_e_fm_op_scan_idler_p)
     _e_fm_op_scan_idler_p = ecore_idler_add(_e_fm_op_scan_idler, NULL);
   if (!_e_fm_op_work_idler_p)
     _e_fm_op_work_idler_p = ecore_idler_add(_e_fm_op_work_idler, NULL);
}

static void
_e_fm_op_delete_idler(int *mark)
{
   if (mark == &_e_fm_op_work_error)
     {
        ecore_idler_del(_e_fm_op_work_idler_p);
        _e_fm_op_work_idler_p = NULL;
     }
   else
     {
        ecore_idler_del(_e_fm_op_scan_idler_p);
        _e_fm_op_scan_idler_p = NULL;
     }
}

/* Code to deal with overwrites and errors in idlers.
 * Basically, it checks if we got a response.
 * Returns 1 if we did; otherwise checks it and does what needs to be done.
 */
static int
_e_fm_op_idler_handle_error(int *mark, Eina_List **queue, Eina_List **node, E_Fm_Op_Task *task)
{
   if (_e_fm_op_overwrite)
     {
        if (_e_fm_op_overwrite_response != E_FM_OP_NONE)
          {
             task->overwrite = _e_fm_op_overwrite_response;
             _e_fm_op_work_error = 0;
             _e_fm_op_scan_error = 0;
          }
        else
          {
             /* No response yet. */
             /* So, delete this idler. It'll be added back when response is there. */
             _e_fm_op_delete_idler(mark);
             return 1;
          }
     }
   else if (*mark)
     {
        if (_e_fm_op_error_response == E_FM_OP_NONE)
          {
             /* No response yet. */
             /* So, delete this idler. It'll be added back when response is there. */
             _e_fm_op_delete_idler(mark);
             return 1;
          }
        else
          {
             E_FM_OP_DEBUG("Got response.\n");
             /* Got response. */
             if (_e_fm_op_error_response == E_FM_OP_ERROR_RESPONSE_ABORT)
               {
                  /* Mark as abort. */
                  _e_fm_op_abort = 1;
                  _e_fm_op_error_response = E_FM_OP_NONE;
                  _e_fm_op_rollback(task);
               }
             else if (_e_fm_op_error_response == E_FM_OP_ERROR_RESPONSE_RETRY)
               {
                  *mark = 0;
                  _e_fm_op_error_response = E_FM_OP_NONE;
               }
             else if (_e_fm_op_error_response == E_FM_OP_ERROR_RESPONSE_IGNORE_THIS)
               {
                  _e_fm_op_rollback(task);
                  _e_fm_op_remove_link_task(task);
                  *queue = eina_list_remove_list(*queue, *node);
                  _e_fm_op_error_response = E_FM_OP_NONE;
                  *mark = 0;
                  *node = NULL;
                  return 1;
               }
             else if (_e_fm_op_error_response == E_FM_OP_ERROR_RESPONSE_IGNORE_ALL)
               {
                  E_FM_OP_DEBUG("E_Fm_Op_Task '%s' --> '%s' was automatically aborted.\n",
                                task->src.name, task->dst.name);
                  _e_fm_op_rollback(task);
                  _e_fm_op_remove_link_task(task);
                  *queue = eina_list_remove_list(*queue, *node);
                  *node = NULL;
                  *mark = 0;
                  /* Do not clean out _e_fm_op_error_response. This way when another error
                   * occures, it would be handled automatically. */
                  return 1;
               }
          }
     }
   else if ((_e_fm_op_work_error) || (_e_fm_op_scan_error))
     return 1;
   return 0;
}

/* This works very simple. Take a task from queue and run appropriate _atom() on it.
 * If after _atom() is done, task->finished is 1 remove the task from queue. Otherwise,
 * run _atom() on the same task on next call.
 *
 * If we have an abort (_e_fm_op_abort = 1), then _atom() should recognize it and do smth.
 * After this, just finish everything.
 */
static Eina_Bool
_e_fm_op_work_idler(void *data __UNUSED__)
{
   /* E_Fm_Op_Task is marked static here because _e_fm_op_work_queue can be populated with another
    * tasks between calls. So it is possible when a part of file is copied and then
    * another task is pushed into _e_fm_op_work_queue and the new one if performed while
    * the first one is not finished yet. This is not cool, so we make sure one task
    * is performed until it is finished. Well, this can be an issue with removing
    * directories. For example, if we are trying to remove a non-empty directory,
    * then this will go into infinite loop. But this should never happen.
    *
    * BTW, the same is propably right for the _e_fm_op_scan_idler().
    */
   static Eina_List *node = NULL;
   E_Fm_Op_Task *task = NULL;

   if (!node) node = _e_fm_op_work_queue;
   task = eina_list_data_get(node);
   if (!task)
     {
        node = _e_fm_op_work_queue;
        task = eina_list_data_get(node);
     }

   if (!task)
     {
        if ((_e_fm_op_separator) &&
            (_e_fm_op_work_queue == _e_fm_op_separator) &&
            (!_e_fm_op_scan_idler_p))
          {
             /* You may want to look at the comment in _e_fm_op_scan_atom() about this separator thing. */
             _e_fm_op_work_queue = eina_list_remove_list(_e_fm_op_work_queue, _e_fm_op_separator);
             node = NULL;
             return ECORE_CALLBACK_RENEW;
          }

        if ((!_e_fm_op_scan_idler_p) && (!_e_fm_op_work_error) &&
            (!_e_fm_op_scan_error))
          ecore_main_loop_quit();
        // if
        // _e_fm_op_scan_idler_p == NULL &&
        // _e_fm_op_work_error == NULL &&
        // _e_fm_op_scan_error == 1
        // we can spin forever. why are we spinning at all? there are no
        // tasks to be done. we have an error. wait for it to be handled
        if ((!_e_fm_op_work_queue) && (!_e_fm_op_scan_queue))
          ecore_main_loop_quit();

        return ECORE_CALLBACK_RENEW;
     }

   if (_e_fm_op_idler_handle_error(&_e_fm_op_work_error, &_e_fm_op_work_queue, &node, task))
     return ECORE_CALLBACK_RENEW;

   task->started = 1;

   if (task->type == E_FM_OP_COPY)
     _e_fm_op_copy_atom(task);
   else if (task->type == E_FM_OP_REMOVE)
     _e_fm_op_remove_atom(task);
   else if (task->type == E_FM_OP_DESTROY)
     _e_fm_op_destroy_atom(task);
   else if (task->type == E_FM_OP_COPY_STAT_INFO)
     _e_fm_op_copy_stat_info_atom(task);
   else if (task->type == E_FM_OP_SYMLINK)
     _e_fm_op_symlink_atom(task);
   else if (task->type == E_FM_OP_RENAME)
     _e_fm_op_rename_atom(task);

   if (task->finished)
     {
        _e_fm_op_work_queue = eina_list_remove_list(_e_fm_op_work_queue, node);
        _e_fm_op_task_free(task);
        node = NULL;
     }

   if (_e_fm_op_abort)
     {
        /* So, _atom did what it whats in case of abort. Now to idler. */
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

/* This works pretty much the same as _e_fm_op_work_idler(), except that
 * if this is a dir, then look into its contents and create a task
 * for those files. And we don't have _e_fm_op_separator here.
 */
Eina_Bool
_e_fm_op_scan_idler(void *data __UNUSED__)
{
   static Eina_List *node = NULL;
   E_Fm_Op_Task *task = NULL;
   char buf[PATH_MAX];
   static Eina_Iterator *dir = NULL;
   E_Fm_Op_Task *ntask = NULL;

   if (!node) node = _e_fm_op_scan_queue;
   task = eina_list_data_get(node);
   if (!task)
     {
        node = _e_fm_op_scan_queue;
        task = eina_list_data_get(node);
     }

   if (!task)
     {
        _e_fm_op_scan_idler_p = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   if (_e_fm_op_idler_handle_error(&_e_fm_op_scan_error, &_e_fm_op_scan_queue, &node, task))
     return ECORE_CALLBACK_RENEW;

   if (_e_fm_op_abort)
     {
        /* We're marked for abortion. */
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
     }

   if (task->type == E_FM_OP_COPY_STAT_INFO)
     {
        _e_fm_op_scan_atom(task);
        if (task->finished)
          {
             _e_fm_op_scan_queue =
               eina_list_remove_list(_e_fm_op_scan_queue, node);
             _e_fm_op_task_free(task);
             node = NULL;
          }
     }
   else if (!dir && !task->started)
     {
        if (lstat(task->src.name, &(task->src.st)) < 0)
          _E_FM_OP_ERROR_SEND_SCAN(task, E_FM_OP_ERROR,
                                   "Cannot lstat '%s': %s.", task->src.name);

        if ((task->type != E_FM_OP_SYMLINK) &&
            (task->type != E_FM_OP_RENAME) &&
            S_ISDIR(task->src.st.st_mode))
          {
             /* If it's a dir, then look through it and add a task for each. */

             dir = eina_file_direct_ls(task->src.name);
             if (!dir)
               _E_FM_OP_ERROR_SEND_SCAN(task, E_FM_OP_ERROR,
                                        "Cannot open directory '%s': %s.",
                                        task->dst.name);
          }
        else
          {
             task->started = 1;
             _e_fm_op_scan_queue =
               eina_list_remove_list(_e_fm_op_scan_queue, node);
             node = NULL;
             _e_fm_op_scan_atom(task);
          }
     }
   else if (dir && !task->started)
     {
        Eina_File_Direct_Info *info;

        if (!eina_iterator_next(dir, (void **)&info))
          {
             ntask = _e_fm_op_task_new();
             ntask->type = E_FM_OP_COPY_STAT_INFO;
             ntask->src.name = eina_stringshare_add(task->src.name);
             memcpy(&(ntask->src.st), &(task->src.st), sizeof(struct stat));

             if (task->dst.name)
               ntask->dst.name = eina_stringshare_add(task->dst.name);
             else
               ntask->dst.name = NULL;

             if (task->type == E_FM_OP_REMOVE)
               _e_fm_op_scan_queue =
                 eina_list_prepend(_e_fm_op_scan_queue, ntask);
             else
               _e_fm_op_scan_queue =
                 eina_list_append(_e_fm_op_scan_queue, ntask);

             task->started = 1;
             eina_iterator_free(dir);
             dir = NULL;
             node = NULL;
             return ECORE_CALLBACK_RENEW;
          }

        ntask = _e_fm_op_task_new();
        ntask->type = task->type;
        ntask->src.name = eina_stringshare_add(info->path);

        if (task->dst.name)
          {
             snprintf(buf, sizeof(buf), "%s/%s", task->dst.name, info->path + info->name_start);
             ntask->dst.name = eina_stringshare_add(buf);
          }
        else
          ntask->dst.name = NULL;

        if (task->type == E_FM_OP_REMOVE)
          _e_fm_op_scan_queue = eina_list_prepend(_e_fm_op_scan_queue, ntask);
        else
          _e_fm_op_scan_queue = eina_list_append(_e_fm_op_scan_queue, ntask);
     }
   else
     {
        _e_fm_op_scan_atom(task);
        if (task->finished)
          {
             _e_fm_op_scan_queue =
               eina_list_remove_list(_e_fm_op_scan_queue, node);
             _e_fm_op_task_free(task);
             node = NULL;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

/* Packs and sends an error to STDOUT.
 * type is either E_FM_OP_ERROR or E_FM_OP_OVERWRITE.
 * fmt is a printf format string, the other arguments
 * are for this format string,
 */
static void
_e_fm_op_send_error(E_Fm_Op_Task *task, E_Fm_Op_Type type, const char *fmt, ...)
{
   va_list ap;
   char buffer[READBUFSIZE];
   char *buf = &buffer[0];
   char *str = buf + (3 * sizeof(int));
   int len = 0;

   va_start(ap, fmt);

   if (_e_fm_op_error_response == E_FM_OP_ERROR_RESPONSE_IGNORE_ALL)
     {
        /* Do nothing. */
     }
   else
     {
        vsnprintf(str, READBUFSIZE - 3 * sizeof(int), fmt, ap);
        len = strlen(str);

        *((int *)(void *)buf) = E_FM_OP_MAGIC;
        *((int *)(void *)(buf + sizeof(int))) = type;
        *((int *)(void *)(buf + (2 * sizeof(int)))) = len + 1;

        if (write(STDOUT_FILENO, buf, (3 * sizeof(int)) + len + 1) < 0)
          perror("write");

        E_FM_OP_DEBUG("%s", str);
        E_FM_OP_DEBUG(" Error sent.\n");
     }

   va_end(ap);
   if (!_e_fm_op_overwrite)
     _e_fm_op_remove_link_task(task);
}

/* Unrolls task: makes a clean up and updates progress info. */
static void
_e_fm_op_rollback(E_Fm_Op_Task *task)
{
   E_Fm_Op_Copy_Data *data;

   if (!task) return;

   if (task->type == E_FM_OP_COPY)
     {
        data = task->data;
        if (data)
          {
             if (data->from)
               {
                  fclose(data->from);
                  data->from = NULL;
               }
             if (data->to)
               {
                  fclose(data->to);
                  data->to = NULL;
               }
          }
        E_FREE(task->data);
        _e_fm_op_update_progress(task, -task->dst.done,
                                 -task->src.st.st_size - (task->link ? REMOVECHUNKSIZE : 0));
     }
   else
     _e_fm_op_update_progress(task, -REMOVECHUNKSIZE, -REMOVECHUNKSIZE);
}

static void
_e_fm_op_update_progress_report(int percent, int eta, double elapsed, off_t done, off_t total, const char *src, const char *dst)
{
   const int magic = E_FM_OP_MAGIC;
   const int id = E_FM_OP_PROGRESS;
   char *p, *data;
   size_t size, src_len, dst_len;

   if (!dst) return;

   src_len = strlen(src);
   dst_len = strlen(dst);

   size = (2 * sizeof(int)) + (2 * sizeof(off_t)) + src_len + 1 + dst_len + 1;
   data = alloca(3 * sizeof(int) + size);
   if (!data) return;
   p = data;

#define P(value) memcpy(p, &(value), sizeof(int)); p += sizeof(int)
   P(magic);
   P(id);
   P(size);
   P(percent);
   P(eta);
#undef P

#define P(value) memcpy(p, &(value), sizeof(off_t)); p += sizeof(off_t)
   P(done);
   P(total);
#undef P

#define P(value) memcpy(p, value, value ## _len + 1); p += value ## _len + 1
   P(src);
   P(dst);
#undef P

   if (write(STDOUT_FILENO, data, (3 * sizeof(int)) + size) < 0)
     perror("write");

   E_FM_OP_DEBUG("Time left: %d at %e\n", eta, elapsed);
   E_FM_OP_DEBUG("Progress %d. \n", percent);
}

static void
_e_fm_op_update_progress_report_simple(int percent, const char *src, const char *dst)
{
   size_t done = (percent * REMOVECHUNKSIZE) / 100;
   _e_fm_op_update_progress_report
     (percent, 0, 0, done, REMOVECHUNKSIZE, src, dst);
}

/* Updates progress.
 * _plus_data is how much more works is done and _plus_e_fm_op_total
 * is how much more work we found out needs to be done
 * (it is not zero primarily in _e_fm_op_scan_idler())
 *
 * It calculates progress in percent. And once a second calculates eta.
 * If either of them changes from their previuos values, then the are
 * packed and written to STDOUT.
 */
static void
_e_fm_op_update_progress(E_Fm_Op_Task *task, off_t _plus_e_fm_op_done, off_t _plus_e_fm_op_total)
{
   /* Type long long should be 64 bits wide everywhere, this means
    * that its max value is 2^63 - 1, which is 8 388 608 terabytes,
    * and this should be enough. Well, we'll be multipling _e_fm_op_done
    * by 100, but still, it is big enough. */
   static off_t _e_fm_op_done = 0, _e_fm_op_total = 0;

   static int ppercent = -1;
   int percent;
   static double c_time = 0;
   static double s_time = 0;
   double eta = 0;
   static int peta = -1;
   static E_Fm_Op_Task *ptask = NULL;

   _e_fm_op_done += _plus_e_fm_op_done;
   _e_fm_op_total += _plus_e_fm_op_total;

   /* Do not send progress until scan is done.*/
   if (_e_fm_op_scan_idler_p) return;

   if (_e_fm_op_total != 0)
     {
        /* % 101 is for the case when somehow work queue works faster
           than scan queue. _e_fm_op_done * 100 should not cause arithmetic
           overflow, since long long can hold really big values. */
        percent = _e_fm_op_done * 100 / _e_fm_op_total % 101;

        eta = peta;

        if (eina_dbl_exact(s_time, 0)) s_time = ecore_time_get();

        /* Update ETA once a second */
        if ((_e_fm_op_done) && (ecore_time_get() - c_time > 1.0))
          {
             c_time = ecore_time_get();
             eta = (c_time - s_time) * (_e_fm_op_total - _e_fm_op_done) / _e_fm_op_done;
             eta = (int)(eta + 0.5);
          }

        if (!task) return;
        if ((percent != ppercent) || (!EINA_DBL_EQ(eta, peta)) || (task != ptask))
          {
             ppercent = percent;
             peta = eta;
             ptask = task;
             _e_fm_op_update_progress_report(percent, eta, c_time - s_time,
                                             _e_fm_op_done, _e_fm_op_total,
                                             task->src.name, task->dst.name);
          }
     }
}

/* We just use this code in several places. */
static void
_e_fm_op_copy_stat_info(E_Fm_Op_Task *task)
{
   struct utimbuf ut;

   if (!task->dst.name) return;

   if (chmod(task->dst.name, task->src.st.st_mode) < 0)
     perror("chmod");
   if (chown(task->dst.name, task->src.st.st_uid, task->src.st.st_gid) < 0)
     perror("chown");
   ut.actime = task->src.st.st_atime;
   ut.modtime = task->src.st.st_mtime;
   utime(task->dst.name, &ut);
}

static int
_e_fm_op_handle_overwrite(E_Fm_Op_Task *task)
{
   struct stat st;

   if ((task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_YES_ALL)
       || (_e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_YES_ALL))
     {
        _e_fm_op_overwrite = 0;
        return 0;
     }
   else if ((task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_YES)
            || (_e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_YES))
     {
        _e_fm_op_overwrite_response = E_FM_OP_NONE;
        _e_fm_op_overwrite = 0;
        return 0;
     }
   else if ((task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_NO)
            || (_e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_NO))
     {
        task->finished = 1;
        _e_fm_op_rollback(task);
        _e_fm_op_remove_link_task(task);
        _e_fm_op_overwrite_response = E_FM_OP_NONE;
        _e_fm_op_overwrite = 0;
        return 1;
     }
   else if ((task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_NO_ALL)
            || (_e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_NO_ALL))
     {
        task->finished = 1;
        _e_fm_op_rollback(task);
        _e_fm_op_remove_link_task(task);
        _e_fm_op_overwrite = 0;
        return 1;
     }

   if ( stat(task->dst.name, &st) == 0)
     {
        /* File exists. */
        if ( _e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_NO_ALL)
          {
             task->finished = 1;
             _e_fm_op_rollback(task);
             return 1;
          }
        else
          {
             _e_fm_op_overwrite = 1;
             _e_fm_op_update_progress_report_simple(0, task->src.name, task->dst.name);
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_OVERWRITE, "Cannot overwrite '%s': %s.", task->dst.name);
          }
     }

   return 0;
}

static int
_e_fm_op_copy_dir(E_Fm_Op_Task *task)
{
   struct stat st;

   /* Directory. Just create one in destatation. */
   if (mkdir(task->dst.name,
             S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
             S_IXOTH) == -1)
     {
        if (errno == EEXIST)
          {
             if (lstat(task->dst.name, &st) < 0)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR,
                                        "Cannot lstat '%s': %s.",
                                        task->dst.name);
             if (!S_ISDIR(st.st_mode))
               {
                  /* Let's try to delete the file and create a dir */
                  if (unlink(task->dst.name) == -1)
                    _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR,
                                             "Cannot unlink '%s': %s.",
                                             task->dst.name);
                  if (mkdir(task->dst.name, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
                    _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR,
                                             "Cannot make directory '%s': %s.",
                                             task->dst.name);
               }
          }
        else
          _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR,
                                   "Cannot make directory '%s': %s.",
                                   task->dst.name);
     }

   task->dst.done += task->src.st.st_size;
   _e_fm_op_update_progress(task, task->src.st.st_size, 0);

   /* Finish with this task. */
   task->finished = 1;

   return 0;
}

static int
_e_fm_op_copy_link(E_Fm_Op_Task *task)
{
   char *lnk_path;

   lnk_path = ecore_file_readlink(task->src.name);
   if (!lnk_path)
     {
        _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot read link '%s': %s.", task->src.name);
     }

   E_FM_OP_DEBUG("Creating link from '%s' to '%s'\n", lnk_path, task->dst.name);
   _e_fm_op_update_progress_report_simple(0, lnk_path, task->dst.name);

   if (symlink(lnk_path, task->dst.name) == -1)
     {
        char *buf;

        if (errno == EEXIST)
          {
             if (unlink(task->dst.name) == -1)
               {
                  free(lnk_path);
                  _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot unlink '%s': %s.", task->dst.name);
               }
             if (symlink(lnk_path, task->dst.name) == -1)
               {
                  buf = strdupa(lnk_path);
                  free(lnk_path);
                  _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot create link from '%s' to '%s': %s.", buf, task->dst.name);
               }
          }
        else
          {
             buf = strdupa(lnk_path);
             free(lnk_path);
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot create link from '%s' to '%s': %s.", buf, task->dst.name);
          }
     }
   free(lnk_path);

   task->dst.done += task->src.st.st_size;

   _e_fm_op_update_progress(task, task->src.st.st_size, 0);

   task->finished = 1;

   return 0;
}

static int
_e_fm_op_copy_fifo(E_Fm_Op_Task *task)
{
   if (mkfifo(task->dst.name, task->src.st.st_mode) == -1)
     {
        if (errno == EEXIST)
          {
             if (unlink(task->dst.name) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot unlink '%s': %s.", task->dst.name);
             if (mkfifo(task->dst.name, task->src.st.st_mode) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot make FIFO at '%s': %s.", task->dst.name);
          }
        else
          _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot make FIFO at '%s': %s.", task->dst.name);
     }

   _e_fm_op_copy_stat_info(task);

   task->dst.done += task->src.st.st_size;
   _e_fm_op_update_progress(task, task->src.st.st_size, 0);

   task->finished = 1;

   return 0;
}

static int
_e_fm_op_open_files(E_Fm_Op_Task *task)
{
   E_Fm_Op_Copy_Data *data = task->data;

   /* Ordinary file. */
   if (!data)
     {
        data = malloc(sizeof(E_Fm_Op_Copy_Data));
        task->data = data;
        data->to = NULL;
        data->from = NULL;
     }

   if (!data->from)
     {
        data->from = fopen(task->src.name, "rb");
        if (!data->from)
          _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot open file '%s' for reading: %s.", task->src.name);
     }

   if (!data->to)
     {
        data->to = fopen(task->dst.name, "wb");
        if (!data->to)
          _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot open file '%s' for writing: %s.", task->dst.name);
        _e_fm_op_copy_stat_info(task);
     }

   return 0;
}

static int
_e_fm_op_copy_chunk(E_Fm_Op_Task *task)
{
   E_Fm_Op_Copy_Data *data;
   size_t dread, dwrite;
   char buf[COPYBUFSIZE];

   data = task->data;

   if (_e_fm_op_abort)
     {
        _e_fm_op_rollback(task);

        task->finished = 1;
        return 1;
     }

   dread = fread(buf, 1, sizeof(buf), data->from);
   if (dread == 0)
     {
        if (!feof(data->from))
          _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot read data from '%s': %s.", task->dst.name);

        fclose(data->from);
        fclose(data->to);
        data->to = NULL;
        data->from = NULL;

        _e_fm_op_copy_stat_info(task);

        E_FREE(task->data);

        task->finished = 1;

        _e_fm_op_update_progress(task, 0, 0);

        return 1;
     }

   dwrite = fwrite(buf, 1, dread, data->to);

   if (dwrite < dread)
     _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot write data to '%s': %s.", task->dst.name);

   task->dst.done += dread;
   _e_fm_op_update_progress(task, dwrite, 0);

   return 0;
}

/*
 * _e_fm_op_copy_atom(), _e_fm_op_remove_atom() and _e_fm_op_scan_atom() are functions that
 * perform very small operations.
 *
 * _e_fm_op_copy_atom(), for example, makes one of three things:
 * 1) opens files for writing and reading. This may take several calls -- until overwrite issues are not resolved.
 * 2) reads some bytes from one file and writes them to the other.
 * 3) closes both files if there is nothing more to read.
 *
 * _e_fm_op_remove_atom() removes smth.
 *
 * _e_fm_op_scan_atom() pushes new tasks for the _e_fm_op_work_idler(). One task for cp&rm and two tasks for mv.
 *
 * _e_fm_op_copy_atom() and _e_fm_op_remove_atom() are called from _e_fm_op_work_idler().
 * _e_fm_op_scan_atom() is called from _e_fm_op_scan_idler().
 *
 * These functions are called repeatedly until they put task->finished = 1. After that the task is removed from queue.
 *
 * Return value does not matter. It's there only to _E_FM_OP_ERROR_SEND macro to work correctly. (Well, it works fine, just don't want GCC to produce a warning.)
 */
static int
_e_fm_op_copy_atom(E_Fm_Op_Task *task)
{
   E_Fm_Op_Copy_Data *data;

   if (!task) return 1;

   data = task->data;

   if ((!data) || (!data->to) || (!data->from)) /* Did not touch the files yet. */
     {
        E_FM_OP_DEBUG("Copy: %s --> %s\n", task->src.name, task->dst.name);

        if (_e_fm_op_abort)
          {
             /* We're marked for abortion. Don't do anything.
              * Just return -- abort gets handled in _idler.
              */
             task->finished = 1;
             return 1;
          }
        if (_e_fm_op_handle_overwrite(task)) return 1;

        if (S_ISDIR(task->src.st.st_mode))
          _e_fm_op_copy_dir(task);
        else if (S_ISLNK(task->src.st.st_mode))
          _e_fm_op_copy_link(task);
        else if (S_ISFIFO(task->src.st.st_mode))
          _e_fm_op_copy_fifo(task);
        else if (S_ISREG(task->src.st.st_mode))
          _e_fm_op_open_files(task);
     }
   else
     {
        _e_fm_op_copy_chunk(task);
     }

   return 1;
}

static int
_e_fm_op_scan_atom(E_Fm_Op_Task *task)
{
   E_Fm_Op_Task *ctask, *rtask;

   if (!task) return 1;

   task->finished = 1;

   /* Now push a new task to the work idler. */

   if (task->type == E_FM_OP_COPY)
     {
        _e_fm_op_update_progress(NULL, 0, task->src.st.st_size);

        ctask = _e_fm_op_task_new();
        ctask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = eina_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_COPY;

        _e_fm_op_work_queue = eina_list_append(_e_fm_op_work_queue, ctask);
     }
   else if (task->type == E_FM_OP_COPY_STAT_INFO)
     {
        _e_fm_op_update_progress(NULL, 0, REMOVECHUNKSIZE);

        ctask = _e_fm_op_task_new();
        ctask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = eina_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_COPY_STAT_INFO;

        _e_fm_op_work_queue = eina_list_append(_e_fm_op_work_queue, ctask);
     }
   else if (task->type == E_FM_OP_REMOVE)
     {
        _e_fm_op_update_progress(NULL, 0, REMOVECHUNKSIZE);

        rtask = _e_fm_op_task_new();
        rtask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = eina_stringshare_add(task->dst.name);
        rtask->type = E_FM_OP_REMOVE;

        _e_fm_op_work_queue = eina_list_prepend(_e_fm_op_work_queue, rtask);
     }
   else if (task->type == E_FM_OP_SECURE_REMOVE)
     {
        /* Overwrite task. */
        _e_fm_op_update_progress(NULL, 0, task->src.st.st_size);
        ctask = _e_fm_op_task_new();

        ctask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = eina_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_DESTROY;

        _e_fm_op_work_queue = eina_list_prepend(_e_fm_op_work_queue, ctask);

        /* Remove task. */
        _e_fm_op_update_progress(NULL, 0, REMOVECHUNKSIZE);
        rtask = _e_fm_op_task_new();

        rtask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = eina_stringshare_add(task->dst.name);
        rtask->type = E_FM_OP_REMOVE;

        _e_fm_op_work_queue = eina_list_append_relative_list(_e_fm_op_work_queue, rtask, _e_fm_op_separator);

        ctask->link = eina_list_next(_e_fm_op_separator);
     }
   else if (task->type == E_FM_OP_MOVE)
     {
        /* Copy task. */
        _e_fm_op_update_progress(NULL, 0, task->src.st.st_size);
        ctask = _e_fm_op_task_new();

        ctask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = eina_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_COPY;

        _e_fm_op_work_queue = eina_list_prepend(_e_fm_op_work_queue, ctask);

        /* Remove task. */
        _e_fm_op_update_progress(NULL, 0, REMOVECHUNKSIZE);
        rtask = _e_fm_op_task_new();

        rtask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = eina_stringshare_add(task->dst.name);
        rtask->type = E_FM_OP_REMOVE;

        /* We put remove task after the separator. Work idler won't go
         * there unless scan is done and this means that all tasks for
         * copy are already in queue. And they will be performed before
         * the delete tasks.
         *
         * If we don't use this separator trick, then there easily can be
         * a situation when remove task is performed before all files are
         * copied.
         */

        _e_fm_op_work_queue = eina_list_append_relative_list(_e_fm_op_work_queue, rtask, _e_fm_op_separator);

        ctask->link = eina_list_next(_e_fm_op_separator);
     }
   else if (task->type == E_FM_OP_SYMLINK)
     {
        _e_fm_op_update_progress(NULL, 0, REMOVECHUNKSIZE);

        rtask = _e_fm_op_task_new();
        rtask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = eina_stringshare_add(task->dst.name);
        rtask->type = E_FM_OP_SYMLINK;

        _e_fm_op_work_queue = eina_list_prepend(_e_fm_op_work_queue, rtask);
     }
   else if (task->type == E_FM_OP_RENAME)
     {
        _e_fm_op_update_progress(NULL, 0, REMOVECHUNKSIZE);

        rtask = _e_fm_op_task_new();
        rtask->src.name = eina_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = eina_stringshare_add(task->dst.name);
        rtask->type = E_FM_OP_RENAME;

        _e_fm_op_work_queue = eina_list_prepend(_e_fm_op_work_queue, rtask);
     }

   return 1;
}

static int
_e_fm_op_copy_stat_info_atom(E_Fm_Op_Task *task)
{
   E_FM_OP_DEBUG("Stat: %s --> %s\n", task->src.name, task->dst.name);

   _e_fm_op_copy_stat_info(task);
   task->finished = 1;
   task->dst.done += REMOVECHUNKSIZE;

   _e_fm_op_update_progress(task, REMOVECHUNKSIZE, 0);

   return 0;
}

static int
_e_fm_op_symlink_atom(E_Fm_Op_Task *task)
{
   if (_e_fm_op_abort) return 1;
   if (_e_fm_op_handle_overwrite(task)) return 1;

   E_FM_OP_DEBUG("Symlink: %s -> %s\n", task->src.name, task->dst.name);
   _e_fm_op_update_progress_report_simple(0, task->src.name, task->dst.name);

   if (symlink(task->src.name, task->dst.name) == -1)
     {
        if (errno == EEXIST)
          {
             if (unlink(task->dst.name) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot unlink '%s': %s.", task->dst.name);
             if (symlink(task->src.name, task->dst.name) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot create link from '%s' to '%s': %s.", task->src.name, task->dst.name);
          }
        else
          _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot create link from '%s' to '%s': %s.", task->src.name, task->dst.name);
     }

   task->dst.done += REMOVECHUNKSIZE;
   _e_fm_op_update_progress(task, REMOVECHUNKSIZE, 0);
   task->finished = 1;

   return 0;
}

static int
_e_fm_op_remove_atom(E_Fm_Op_Task *task)
{
   if (_e_fm_op_abort) return 1;

   E_FM_OP_DEBUG("Remove: %s\n", task->src.name);

   if (S_ISDIR(task->src.st.st_mode))
     {
        if (rmdir(task->src.name) == -1)
          {
             if (errno == ENOTEMPTY)
               {
                  E_FM_OP_DEBUG("Attempt to remove non-empty directory.\n");
                  /* This should never happen due to way tasks are added to the work queue.
                     If this happens (for example new files were created after the scan was
                     complete), implicitly delete everything. */
                  ecore_file_recursive_rm(task->src.name);
                  task->finished = 1; /* Make sure that task is removed. */
                  return 1;
               }
             else
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot remove directory '%s': %s.", task->src.name);
          }
     }
   else if (unlink(task->src.name) == -1)
     _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot remove file '%s': %s.", task->src.name);

   task->dst.done += REMOVECHUNKSIZE;
   _e_fm_op_update_progress(task, REMOVECHUNKSIZE, 0);

   task->finished = 1;

   return 1;
}

static int
_e_fm_op_rename_atom(E_Fm_Op_Task *task)
{
   if (_e_fm_op_abort) return 1;
   if (_e_fm_op_handle_overwrite(task)) return 1;

   E_FM_OP_DEBUG("Move: %s -> %s\n", task->src.name, task->dst.name);
   _e_fm_op_update_progress_report_simple(0, task->src.name, task->dst.name);

   if (rename(task->src.name, task->dst.name) == -1)
     {
        _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot move '%s' to '%s': %s.", task->src.name, task->dst.name);
     }

   task->dst.done += REMOVECHUNKSIZE;
   _e_fm_op_update_progress(task, REMOVECHUNKSIZE, 0);
   task->finished = 1;

   return 0;
}

static int
_e_fm_op_destroy_atom(E_Fm_Op_Task *task)
{
   if (_e_fm_op_abort) goto finish;
   static int fd = -1;
   static char *buf = NULL;
   off_t sz;

   if (fd == -1)
     {
       E_FM_OP_DEBUG("Secure remove: %s\n", task->src.name);
       struct stat st2;

       if (!S_ISREG(task->src.st.st_mode))
         goto finish;

       if (task->src.st.st_nlink > 1)
         goto finish;

       if ((fd = open(task->src.name, O_WRONLY|O_NOFOLLOW, 0)) == -1)
         goto finish;

       if (fstat(fd, &st2) == -1)
         goto finish;

       if (st2.st_dev != task->src.st.st_dev ||
           st2.st_ino != task->src.st.st_ino ||
           st2.st_mode != task->src.st.st_mode)
         goto finish;

       if ((buf = malloc(READBUFSIZE)) == NULL)
         goto finish;

       task->src.st.st_size = st2.st_size;
     }

   if (task->pos + READBUFSIZE > task->src.st.st_size) sz = task->src.st.st_size - task->pos;
   else sz = READBUFSIZE;

   _e_fm_op_random_buf(buf, sz);
   if (write(fd, buf, sz) != sz)
     goto finish;
   if (fsync(fd) == -1)
     goto finish;

   task->pos += sz;

   _e_fm_op_update_progress_report_simple(lround((double) ((task->pos + (task->passes * task->src.st.st_size)) /
                                          (double)(task->src.st.st_size * NB_PASS)) * 100.),
                                          "/dev/urandom", task->src.name);

   if (task->pos >= task->src.st.st_size)
     {
       task->passes++;

       if (task->passes == NB_PASS)
         goto finish;
       if (lseek(fd, 0, SEEK_SET) == -1)
         goto finish;

       task->pos = 0;
     }

   return 1;

finish:
   if (fd != -1) close(fd);
   fd = -1;
   E_FREE(buf);
   task->finished = 1;
   return 1;
}

static void
_e_fm_op_random_buf(char *buf, ssize_t len)
{
   int f = -1;

   if ((f = open("/dev/urandom", O_RDONLY)) == -1)
     {
        _e_fm_op_random_char(buf, len);
        return;
     }

   if (read(f, buf, len) != len)
     _e_fm_op_random_char(buf, len);

   close(f);
   return;
}

static void
_e_fm_op_random_char(char *buf, size_t len)
{
   size_t i;
   static int sranded = 0;
   
   if (!sranded)
     {
        srand((unsigned int)time(NULL));
        sranded = 1;
     }

   for (i = 0; i < len; i++)
     {
        buf[i] = (rand() % 256) + 'a';
     }
}


static int
_e_fm_op_make_copy_name(const char *abs, char *buf, size_t buf_size)
{
   size_t name_len;
   size_t file_len;
   char *name;
   char *ext;
   char *copy_str;

   if (!ecore_file_exists(abs))
      return 0;

   file_len = strlen(buf);

   /* TODO: need to make a policy regarding copy postfix:
    * currently attach " (copy)" continuously
    *
    * TODO: i18n */
   copy_str = "(copy)";
   if (strlen(copy_str) + file_len + 1 >= buf_size) return 1;

   name = ecore_file_strip_ext(buf);
   name_len = strlen(name);
   E_FREE(name);

   if (file_len == name_len) ext = NULL;
   else ext = strdup(buf + name_len);

   if (ext)
     {
        sprintf(buf + name_len, " %s%s", copy_str, ext);
        E_FREE(ext);
     }
   else sprintf(buf + name_len, " %s", copy_str);

   return _e_fm_op_make_copy_name(abs, buf, buf_size);
}
