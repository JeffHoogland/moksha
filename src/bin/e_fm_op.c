/*
 * vim:cindent:ts=8:sw=3:sts=8:expandtab:cino=>5n-3f0^-2{2
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>
#include <errno.h>

#include <Evas_Data.h>

#include <Ecore.h>
#include <Ecore_File.h>

#include "e_fm_op.h"

#define READBUFSIZE 65536
#define COPYBUFSIZE 16384
#define REMOVECHUNKSIZE 4096

#define FREE(p) do { if (p) {free((void *)p); p = NULL;} } while (0)

typedef struct _E_Fm_Op_Task E_Fm_Op_Task;
typedef struct _E_Fm_Op_Copy_Data E_Fm_Op_Copy_Data;

static E_Fm_Op_Task *_e_fm_op_task_new();
static void _e_fm_op_task_free(void *t);

static void _e_fm_op_remove_link_task(E_Fm_Op_Task *task);
static int _e_fm_op_stdin_data(void *data, Ecore_Fd_Handler * fd_handler);

static int _e_fm_op_idler_handle_error(int *mark, Evas_List **queue, Evas_List **node, E_Fm_Op_Task *task);

static int _e_fm_op_work_idler(void *data);
static int _e_fm_op_scan_idler(void *data);

static void _e_fm_op_send_error(E_Fm_Op_Task * task, E_Fm_Op_Type type, const char *fmt, ...);
static void _e_fm_op_rollback(E_Fm_Op_Task * task);
static void  _e_fm_op_update_progress(long long _plus_e_fm_op_done, long long _plus_e_fm_op_total);
static void _e_fm_op_copy_stat_info(E_Fm_Op_Task *task);
static int _e_fm_op_handle_overwrite(E_Fm_Op_Task *task);

static int _e_fm_op_copy_dir(E_Fm_Op_Task * task);
static int _e_fm_op_copy_link(E_Fm_Op_Task *task);
static int _e_fm_op_copy_fifo(E_Fm_Op_Task *task);
static int _e_fm_op_open_files(E_Fm_Op_Task *task);
static int _e_fm_op_copy_chunk(E_Fm_Op_Task *task);

static int _e_fm_op_copy_atom(E_Fm_Op_Task * task);
static int _e_fm_op_scan_atom(E_Fm_Op_Task * task);
static int _e_fm_op_copy_stat_info_atom(E_Fm_Op_Task * task);
static int _e_fm_op_remove_atom(E_Fm_Op_Task * task);

Ecore_Fd_Handler *_e_fm_op_stdin_handler = NULL;

Evas_List *_e_fm_op_work_queue = NULL, *_e_fm_op_scan_queue = NULL;
Ecore_Idler *_e_fm_op_work_idler_p = NULL, *_e_fm_op_scan_idler_p = NULL;

long long _e_fm_op_done, _e_fm_op_total; /* Type long long should be 64 bits wide everywhere,
                                            this means that it's max value is 2^63 - 1, which 
                                            is 8 388 608 terabytes, and this should be enough.
                                            Well, we'll be multipling _e_fm_op_done by 100, but 
                                            still, it is big enough. */

int _e_fm_op_abort = 0;	/* Abort mark. */
int _e_fm_op_scan_error = 0;
int _e_fm_op_work_error = 0;
int _e_fm_op_overwrite = 0;

int _e_fm_op_error_response = E_FM_OP_NONE;
int _e_fm_op_overwrite_response = E_FM_OP_NONE;

Evas_List *_e_fm_op_separator = NULL;

void *_e_fm_op_stdin_buffer = NULL;

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
      size_t done;
   } dst;

   int started, finished;

   void *data;

   E_Fm_Op_Type type;
   E_Fm_Op_Type overwrite;

   Evas_List *link;
};

struct _E_Fm_Op_Copy_Data
{
   FILE *from;
   FILE *to;
};

int
main(int argc, char **argv)
{
   E_Fm_Op_Task *task = NULL;
   int i, last;
   char *byte = "/";
   char buf[PATH_MAX];
   const char *name = NULL;
   E_Fm_Op_Type type;

   ecore_init();

   _e_fm_op_stdin_buffer = malloc(READBUFSIZE);

   _e_fm_op_stdin_handler =
      ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ, _e_fm_op_stdin_data, NULL,
				NULL, NULL);

   if (argc <= 2)
     {
	return 0;
     }

   last = argc - 1;
   i = 2;

   if(strcmp(argv[1], "cp") == 0)
     {
        type = E_FM_OP_COPY;
     }
   else if(strcmp(argv[1], "mv") == 0)
     {
        type = E_FM_OP_MOVE;
     }
   else if(strcmp(argv[1], "rm") == 0)
     {
        type = E_FM_OP_REMOVE;
     }

   if (type == E_FM_OP_COPY || type == E_FM_OP_MOVE)
     {
	if (argc < 4)
	  {
	     return 0;
	  }

        if(type == E_FM_OP_MOVE)
          {
             _e_fm_op_work_queue = evas_list_append(_e_fm_op_work_queue, NULL);
             _e_fm_op_separator = _e_fm_op_work_queue;
          }

        if(argc > 4 && ecore_file_is_dir(argv[last]))
          {
             if(argv[last][strlen(argv[last] - 1)] == '/') byte = "";

             while(i < last)
               {
                  name = ecore_file_file_get(argv[i]);
                  task = _e_fm_op_task_new();
                  task->type = type;
                  task->src.name = evas_stringshare_add(argv[i]);

                  snprintf(buf, PATH_MAX, "%s%s%s", argv[last], byte, name);
                  task->dst.name = evas_stringshare_add(buf);
 
                  if(type == E_FM_OP_MOVE && rename(task->src.name, task->dst.name) == 0)
                    _e_fm_op_task_free(task);
                  else
                    _e_fm_op_scan_queue = evas_list_append(_e_fm_op_scan_queue, task);
                 
                  i++;
               }
          }
        else
          {
             if(type == E_FM_OP_MOVE && rename(argv[2], argv[3]) == 0)
               goto quit;

             task = _e_fm_op_task_new();
             task->type = type;
             task->src.name = evas_stringshare_add(argv[2]);
             task->dst.name = evas_stringshare_add(argv[3]);
          
             _e_fm_op_scan_queue = evas_list_append(_e_fm_op_scan_queue, task);
          }
     }
   else if (type == E_FM_OP_REMOVE)
     {
	if (argc < 3)
	  {
	     return 0;
	  }

        while(i <= last)
          {
             task = _e_fm_op_task_new();
             task->type = type;
             task->src.name = evas_stringshare_add(argv[i]);
             
             _e_fm_op_scan_queue = evas_list_append(_e_fm_op_scan_queue, task);
             
             i++;
          }
     }

   _e_fm_op_scan_idler_p = ecore_idler_add(_e_fm_op_scan_idler, NULL);
   _e_fm_op_work_idler_p = ecore_idler_add(_e_fm_op_work_idler, NULL);

   ecore_main_loop_begin();

quit:
   ecore_shutdown();

   free(_e_fm_op_stdin_buffer);

   E_FM_OP_DEBUG("Slave quit.\n");

   return 0;
}

/* Create new task. */

static E_Fm_Op_Task *_e_fm_op_task_new()
{
   E_Fm_Op_Task *t = malloc(sizeof(E_Fm_Op_Task));

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

   return t;
}

/* Free task. */

static void
_e_fm_op_task_free(void *t)
{
   E_Fm_Op_Task *task = t;
   E_Fm_Op_Copy_Data *data;

   if (!task)
      return;

   if (task->src.name)
      evas_stringshare_del(task->src.name);
   if (task->dst.name)
      evas_stringshare_del(task->dst.name);

   if (task->data)
     {
        data = task->data;

        if(task->type == E_FM_OP_COPY)
          {
             if(data->from)
               {
                  fclose(data->from);
               }
 
             if(data->to)
               {
                  fclose(data->to);
               }
         }
	FREE(task->data);
     }

   FREE(task);
}

/* Removes link task from work queue.
 * Link task is not NULL in case of MOVE. Then two tasks are created: copy and remove.
 * Remove task is a link task for the copy task. If copy task is aborted (e.g. error 
 * occured and user chooses to ignore this), then the remove task is removed from 
 * queue with this functions.
 */

static void _e_fm_op_remove_link_task(E_Fm_Op_Task *task)
{
   if(task->link)
     {
        _e_fm_op_work_queue = evas_list_remove_list(_e_fm_op_work_queue, task->link);
        _e_fm_op_task_free(task->link);
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

static int
_e_fm_op_stdin_data(void *data, Ecore_Fd_Handler * fd_handler)
{
   int fd = ecore_main_fd_handler_fd_get(fd_handler);
   static void *buf = NULL;
   static int length = 0;
   void *begin = NULL;
   ssize_t num = 0;
   int msize;
   int identity;

   if(!buf) 
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

	while (length >= 3 * sizeof(int))
	  {
             begin = buf;

	     /* Check magic. */
	     if (*(int *)buf != E_FM_OP_MAGIC)
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
     
	     if (length - 3*sizeof(int) < msize)
	       {
		  /* There is not enough data to read the whole message. */
                  break;
	       }

             length -= 3*sizeof(int);

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
                  break;

               case E_FM_OP_OVERWRITE_RESPONSE_NO:
               case E_FM_OP_OVERWRITE_RESPONSE_NO_ALL:
               case E_FM_OP_OVERWRITE_RESPONSE_YES:
               case E_FM_OP_OVERWRITE_RESPONSE_YES_ALL:
                  _e_fm_op_overwrite_response = identity;
                  E_FM_OP_DEBUG("Overwrite response set.\n");
                  break;
	       }
	  }

        if(length > 0)
          {
             memmove(_e_fm_op_stdin_buffer, begin, length);
          }

        buf = _e_fm_op_stdin_buffer + length;
     }

   return 1;
}

#define _E_FM_OP_ERROR_SEND_SCAN(_task, _e_fm_op_error_type, _fmt, ...)\
   do\
     {\
        int _errno = errno;\
        _e_fm_op_scan_error = 1;\
        _e_fm_op_send_error(_task, _e_fm_op_error_type, _fmt, __VA_ARGS__, strerror(_errno));\
        return 1;\
     }\
   while(0)

#define _E_FM_OP_ERROR_SEND_WORK(_task, _e_fm_op_error_type, _fmt, ...)\
    do\
      {\
         int _errno = errno;\
         _e_fm_op_work_error = 1;\
         _e_fm_op_send_error(_task, _e_fm_op_error_type, _fmt, __VA_ARGS__, strerror(_errno));\
         return 1;\
      }\
    while(0)

/* Code to deal with overwrites and errors in idlers.
 * Basically, it checks if we got a response. 
 * Returns 1 if we did; otherwise checks it and does what needs to be done.
 */

static int _e_fm_op_idler_handle_error(int *mark, Evas_List **queue, Evas_List **node, E_Fm_Op_Task *task)
{
   if(_e_fm_op_overwrite)
     {
        if(_e_fm_op_overwrite_response != E_FM_OP_NONE)
          {
             task->overwrite = _e_fm_op_overwrite_response;
             _e_fm_op_work_error = 0;
             _e_fm_op_scan_error = 0;
          }
        else
          {
             return 1;
          }
     }
   else if(*mark)
     {
        if (_e_fm_op_error_response == E_FM_OP_NONE)
          { 
             /* No response yet. */
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
                  *queue = evas_list_remove_list(*queue, *node);
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
                  *queue = evas_list_remove_list(*queue, *node);
                  *node = NULL;
                  *mark = 0;
                  /* Do not clean out _e_fm_op_error_response. This way when another error occures, it would be handled automatically. */ 
                  return 1; 
               } 
          } 
     }
   else if( _e_fm_op_work_error || _e_fm_op_scan_error)
     {
        return 1;
     }
   return 0;
} 
/* This works very simple. Take a task from queue and run appropriate _atom() on it.
 * If after _atom() is done, task->finished is 1 remove the task from queue. Otherwise,
 * run _atom() on the same task on next call.
 *
 * If we have an abort (_e_fm_op_abort = 1), then _atom() should recognize it and do smth. 
 * After this, just finish everything.
 */

static int
_e_fm_op_work_idler(void *data)
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
   static Evas_List *node = NULL;
   E_Fm_Op_Task *task = NULL;
   
   if(!node) node = _e_fm_op_work_queue;

   task = evas_list_data(node);

   if(!task) 
     {
        node = _e_fm_op_work_queue;
        task = evas_list_data(node);
     }

   if(!task)
     {
        if( _e_fm_op_separator && _e_fm_op_work_queue == _e_fm_op_separator && _e_fm_op_scan_idler_p == NULL)
          {
             /* You may want to look at the comment in _e_fm_op_scan_atom() about this separator thing. */
             _e_fm_op_work_queue = evas_list_remove_list(_e_fm_op_work_queue, _e_fm_op_separator);
             node = NULL;
             return 1;
          }

        if (_e_fm_op_scan_idler_p == NULL)
          {
             ecore_main_loop_quit();
          }

        return 1;
     }

   if(_e_fm_op_idler_handle_error(&_e_fm_op_work_error, &_e_fm_op_work_queue, &node, task)) return 1;

   task->started = 1;
   
   if(task->type == E_FM_OP_COPY)
     {
        _e_fm_op_copy_atom(task);
     }
   else if(task->type == E_FM_OP_REMOVE)
     {
        _e_fm_op_remove_atom(task);
     }
   else if(task->type == E_FM_OP_COPY_STAT_INFO)
     {
        _e_fm_op_copy_stat_info_atom(task);
     }

   if (task->finished)
     {
       _e_fm_op_work_queue = evas_list_remove_list(_e_fm_op_work_queue, node);
       _e_fm_op_task_free(task);
       node = NULL;
     }

   if (_e_fm_op_abort)
     {
	/* So, _atom did what it whats in case of abort. Now to idler. */
	ecore_main_loop_quit();
	return 0;
     }


   return 1;
}

/* This works pretty much the same as _e_fm_op_work_idler(), except that 
 * if this is a dir, then look into its contents and create a task 
 * for those files. And we don't have _e_fm_op_separator here.
 */

int
_e_fm_op_scan_idler(void *data)
{
   static Evas_List *node = NULL;
   E_Fm_Op_Task *task = NULL;
   char buf[PATH_MAX];
   static struct dirent *de = NULL;
   static DIR *dir = NULL;
   E_Fm_Op_Task *ntask = NULL;

   if(!node) node = _e_fm_op_scan_queue;
   
   task = evas_list_data(node);

   if(!task) 
     {
        node = _e_fm_op_scan_queue;
        task = evas_list_data(node);
     }

   if (!task)
     {
	_e_fm_op_scan_idler_p = NULL;
	return 0;
     }

   if (_e_fm_op_abort)
     {
	/* We're marked for abortion. */
	ecore_main_loop_quit();
	return 0;
     }

   if(_e_fm_op_idler_handle_error(&_e_fm_op_scan_error, &_e_fm_op_scan_queue, &node, task)) return 1;

   if (task->type == E_FM_OP_COPY_STAT_INFO)
     {
        _e_fm_op_scan_atom(task);
        if (task->finished)
          {
             _e_fm_op_scan_queue = evas_list_remove_list(_e_fm_op_scan_queue, node);
             _e_fm_op_task_free(task);
             node = NULL;
          }
     }
   else if (!dir && !task->started)
     {
	if (lstat(task->src.name, &(task->src.st)) < 0)
	  {
	     _E_FM_OP_ERROR_SEND_SCAN(task, E_FM_OP_ERROR, "Cannot lstat '%s': %s.", task->src.name);
	  }

	if (S_ISDIR(task->src.st.st_mode))
	  {
	     /* If it's a dir, then look through it and add a task for each. */

             dir = opendir(task->src.name);
             if (!dir)
               {
                  _E_FM_OP_ERROR_SEND_SCAN(task, E_FM_OP_ERROR, "Cannot open directory '%s': %s.", task->dst.name);
	       }
          }
        else
          task->started = 1;
     }
   else if(!task->started)
     {
        de = readdir(dir);

        if(!de)
          {
             ntask = _e_fm_op_task_new();
             
             ntask->type = E_FM_OP_COPY_STAT_INFO;
             
             ntask->src.name = evas_stringshare_add(task->src.name);
             memcpy(&(ntask->src.st), &(task->src.st), sizeof(struct stat));
             
             if (task->dst.name)
               {
                  ntask->dst.name = evas_stringshare_add(task->dst.name);
               }
             else
               {
                  ntask->dst.name = NULL;
               }
             
             if(task->type == E_FM_OP_REMOVE)
               _e_fm_op_scan_queue = evas_list_prepend(_e_fm_op_scan_queue, ntask);
             else
               _e_fm_op_scan_queue = evas_list_append(_e_fm_op_scan_queue, ntask);

             task->started = 1;
             closedir(dir);
             dir = NULL;
             node = NULL;
             return 1;
          }
             
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
          {
             return 1;
          }
        
        ntask = _e_fm_op_task_new();
        
        ntask->type = task->type;
        
        snprintf(buf, sizeof(buf), "%s/%s", task->src.name,
              de->d_name);
        ntask->src.name = evas_stringshare_add(buf);
        
        if (task->dst.name)
          {
             snprintf(buf, sizeof(buf), "%s/%s", task->dst.name,
                   de->d_name);
             ntask->dst.name = evas_stringshare_add(buf);
          }
        else
          {
             ntask->dst.name = NULL;
          }

        if(task->type == E_FM_OP_REMOVE)
          _e_fm_op_scan_queue = evas_list_prepend(_e_fm_op_scan_queue, ntask);
        else
          _e_fm_op_scan_queue = evas_list_append(_e_fm_op_scan_queue, ntask);
     }
   else
     {
	_e_fm_op_scan_atom(task);
	if (task->finished)
          {
             _e_fm_op_scan_queue = evas_list_remove_list(_e_fm_op_scan_queue, node);
             _e_fm_op_task_free(task);
             node = NULL;
          }
     }

   return 1;
}

/* Packs and sends an error to STDOUT.
 * type is either E_FM_OP_ERROR or E_FM_OP_OVERWRITE.
 * fmt is a printf format string, the other arguments 
 * are for this format string,
 */

static void
_e_fm_op_send_error(E_Fm_Op_Task * task, E_Fm_Op_Type type, const char *fmt, ...)
{
   va_list ap;
   char buffer[READBUFSIZE];
   void *buf = &buffer[0];
   char *str = buf + 3 * sizeof(int);
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

        *(int *)buf = E_FM_OP_MAGIC;
        *(int *)(buf + sizeof(int)) = type;
        *(int *)(buf + 2 * sizeof(int)) = len + 1;

        write(STDOUT_FILENO, buf, 3*sizeof(int) + len + 1);

        E_FM_OP_DEBUG(str);
	E_FM_OP_DEBUG(" Error sent.\n");
     }

   va_end(ap);
}

/* Unrolls task: makes a clean up and updates progress info.
 */

static void
_e_fm_op_rollback(E_Fm_Op_Task * task)
{
   E_Fm_Op_Copy_Data *data;

   if (!task)
      return;

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

	FREE(task->data);
     }

   if(task->type == E_FM_OP_COPY)
     _e_fm_op_update_progress(-task->dst.done, -task->src.st.st_size);
   else
     _e_fm_op_update_progress(-REMOVECHUNKSIZE, -REMOVECHUNKSIZE);
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
_e_fm_op_update_progress(long long _plus_e_fm_op_done, long long _plus_e_fm_op_total)
{
   static int ppercent = -1;
   int percent;

   static double ctime = 0;
   static double stime = 0;
   double eta = 0;
   static int peta = -1;

   int data[5];

   _e_fm_op_done += _plus_e_fm_op_done;
   _e_fm_op_total += _plus_e_fm_op_total;

   if(_e_fm_op_scan_idler_p) return; /* Do not send progress until scan is done.*/

   if(_e_fm_op_total != 0)
     {
	percent = _e_fm_op_done * 100 / _e_fm_op_total % 101;	/* % 101 is for the case when somehow work queue works faster than scan queue. _e_fm_op_done * 100 should not cause arithmetic overflow, since long long can hold really big values. */

        eta = peta;

        if(!stime) stime = ecore_time_get();

        if(_e_fm_op_done && ecore_time_get() - ctime > 1.0 ) /* Update ETA once a second */
          {
             ctime = ecore_time_get();
             eta = (ctime - stime) * (_e_fm_op_total - _e_fm_op_done) / _e_fm_op_done;
             eta = (int) (eta + 0.5);
        }

	if(percent != ppercent || eta != peta)
	  {
	     ppercent = percent;
             peta = eta;

             data[0] = E_FM_OP_MAGIC;
             data[1] = E_FM_OP_PROGRESS;
             data[2] = 2*sizeof(int);
             data[3] = percent;
             data[4] = peta;

             write(STDOUT_FILENO, &data[0], 5*sizeof(int));
	
             E_FM_OP_DEBUG("Time left: %d at %e\n", peta, ctime - stime);
	     E_FM_OP_DEBUG("Progress %d. \n", percent);
	  }
     }
}

/* We just use this code in several places.
 */

static void 
_e_fm_op_copy_stat_info(E_Fm_Op_Task *task)
{
   struct utimbuf ut;

   if(!task->dst.name) return;

   chmod(task->dst.name, task->src.st.st_mode);
   chown(task->dst.name, task->src.st.st_uid,
         task->src.st.st_gid);
   ut.actime = task->src.st.st_atime;
   ut.modtime = task->src.st.st_mtime;
   utime(task->dst.name, &ut);
}

static int
_e_fm_op_handle_overwrite(E_Fm_Op_Task *task)
{
   struct stat st;

   if(task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_YES_ALL 
         || _e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_YES_ALL)
     {
        _e_fm_op_overwrite = 0;
        return 0;
     }
   else if(task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_YES 
         || _e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_YES)
     {
        _e_fm_op_overwrite_response = E_FM_OP_NONE;
        _e_fm_op_overwrite = 0;
        return 0;
     }
   else if(task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_NO 
         || _e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_NO)
     {
        task->finished = 1;
        _e_fm_op_rollback(task);
        _e_fm_op_remove_link_task(task);
        _e_fm_op_overwrite_response = E_FM_OP_NONE;
        _e_fm_op_overwrite = 0;
        return 1;
     }
   else if(task->overwrite == E_FM_OP_OVERWRITE_RESPONSE_NO_ALL 
         || _e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_NO_ALL)
     {
        task->finished = 1;
        _e_fm_op_rollback(task);
        _e_fm_op_remove_link_task(task);
        _e_fm_op_overwrite = 0;
        return 1;
     }
   
   if( stat(task->dst.name, &st) == 0)
     {
        /* File exists. */
        if( _e_fm_op_overwrite_response == E_FM_OP_OVERWRITE_RESPONSE_NO_ALL)
          {
             task->finished = 1;
             _e_fm_op_rollback(task);
             return 1;
          }
        else
          {
             _e_fm_op_overwrite = 1;
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_OVERWRITE, "File '%s' already exists. Overwrite?", task->dst.name);
          }
     }
 
   return 0;
}

static int
_e_fm_op_copy_dir(E_Fm_Op_Task * task)
{
   struct stat st;
   /* Directory. Just create one in destatation. */
   if (mkdir
         (task->dst.name,
          S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
          S_IXOTH) == -1)
     {
        if (errno == EEXIST)
          {
             if (lstat(task->dst.name, &st) < 0)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot lstat '%s': %s.", task->dst.name);
             if (!S_ISDIR(st.st_mode))
               {
                  /* Let's try to delete the file and create a dir */
                  if(unlink(task->dst.name) == -1)
                    _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot unlink '%s': %s.", task->dst.name);
                  if(mkdir(task->dst.name, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
                    _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot make directory '%s': %s.", task->dst.name);
               }
          }
        else
          {
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot make directory '%s': %s.", task->dst.name);
          }
     }
   
   task->dst.done += task->src.st.st_size;
   _e_fm_op_update_progress(task->src.st.st_size, 0);

   /* Finish with this task. */
   task->finished = 1;

   return 0;
}

static int
_e_fm_op_copy_link(E_Fm_Op_Task *task)
{
   size_t len;
   char path[PATH_MAX];

   len = readlink(task->src.name, &path[0], PATH_MAX);
   path[len] = 0;
   
   if(symlink(path, task->dst.name) != 0)
     {
        if(errno == EEXIST)
          {
             if(unlink(task->dst.name) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot unlink '%s': %s.", task->dst.name);
             if(symlink(path, task->dst.name) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot create link from '%s' to '%s': %s.", path, task->dst.name);
          }
        else
          {
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot create link from '%s' to '%s': %s.", path, task->dst.name);
          }
     }
   
   task->dst.done += task->src.st.st_size;
   _e_fm_op_update_progress(task->src.st.st_size, 0);
   
   _e_fm_op_copy_stat_info(task);
   
   task->finished = 1;

   return 0;
}

static int
_e_fm_op_copy_fifo(E_Fm_Op_Task *task)
{
   if(mkfifo(task->dst.name, task->src.st.st_mode) == -1)
     {
        if(errno == EEXIST)
          {
             if(unlink(task->dst.name) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot unlink '%s': %s.", task->dst.name);
             if(mkfifo(task->dst.name, task->src.st.st_mode) == -1)
               _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot make FIFO at '%s': %s.", task->dst.name);
          }
        else
          {
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot make FIFO at '%s': %s.", task->dst.name);
          }
     }
   
   _e_fm_op_copy_stat_info(task);

   task->dst.done += task->src.st.st_size;
   _e_fm_op_update_progress(task->src.st.st_size, 0);
   
   task->finished = 1;

   return 0;
}

static int
_e_fm_op_open_files(E_Fm_Op_Task *task)
{
   E_Fm_Op_Copy_Data *data = task->data;

   /* Ordinary file. */
   if(!data)
     {
        data = malloc(sizeof(E_Fm_Op_Copy_Data));
        task->data = data;
        data->to = NULL;
        data->from = NULL;
     }
   
   if(!data->from) 
     {
        data->from = fopen(task->src.name, "rb");
        if (data->from == NULL)
          {
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot open file '%s' for reading: %s.", task->src.name);
          }
     }
   
   if(!data->to)
     {
        data->to = fopen(task->dst.name, "wb");
        if (data->to == NULL)
          {
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot open file '%s' for writing: %s.", task->dst.name);
          }
        
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
   if (dread <= 0)
     {
        if (!feof(data->from))
          {	
             _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot read data from '%s': %s.", task->dst.name);
          }
        
        fclose(data->from);
        fclose(data->to);
        data->from = NULL;
        data->from = NULL;
        
        _e_fm_op_copy_stat_info(task);
        
        FREE(task->data);
        
        task->finished = 1;
        
        _e_fm_op_update_progress(0, 0);
        
        return 1;
     }
   
   dwrite = fwrite(buf, 1, dread, data->to);
   
   if (dwrite < dread)
     {
        _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot write data to '%s': %s.", task->dst.name);
     }
   
   task->dst.done += dread;
   _e_fm_op_update_progress(dwrite, 0);
   
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
_e_fm_op_copy_atom(E_Fm_Op_Task * task)
{
   E_Fm_Op_Copy_Data *data;

   if (!task)
     {
        return 1;
     }

   data = task->data;

   if (!data || !data->to || !data->from)		/* Did not touch the files yet. */
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

        if(_e_fm_op_handle_overwrite(task)) return 1;
       
	if (S_ISDIR(task->src.st.st_mode))
	  {
             if(_e_fm_op_copy_dir(task)) return 1;
	  }
	else if (S_ISLNK(task->src.st.st_mode))
	  {
             if(_e_fm_op_copy_link(task)) return 1;
	  }
	else if (S_ISFIFO(task->src.st.st_mode))
	  {
             if(_e_fm_op_copy_fifo(task)) return 1;
	  }
	else if (S_ISREG(task->src.st.st_mode))
	  {
             if(_e_fm_op_open_files(task)) return 1;
          }
     }
   else
     {
        if(_e_fm_op_copy_chunk(task)) return 1;
     }

   return 1;
}

static int
_e_fm_op_scan_atom(E_Fm_Op_Task * task)
{
   E_Fm_Op_Task *ctask, *rtask;

   if (!task)
     {				/* Error. */
	return 1;
     }

   task->finished = 1;

   /* Now push a new task to the work idler. */

   if(task->type == E_FM_OP_COPY)
     {
        _e_fm_op_update_progress(0, task->src.st.st_size);

        ctask = _e_fm_op_task_new();
        ctask->src.name = evas_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = evas_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_COPY;
        
        _e_fm_op_work_queue = evas_list_append(_e_fm_op_work_queue, ctask);
     }
   else if(task->type == E_FM_OP_COPY_STAT_INFO)
     {
        _e_fm_op_update_progress(0, REMOVECHUNKSIZE);
        
        ctask = _e_fm_op_task_new();
        ctask->src.name = evas_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = evas_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_COPY_STAT_INFO;
        
        _e_fm_op_work_queue = evas_list_append(_e_fm_op_work_queue, ctask);
     }
   else if(task->type == E_FM_OP_REMOVE)
     {
        _e_fm_op_update_progress(0, REMOVECHUNKSIZE);

        rtask = _e_fm_op_task_new();
        rtask->src.name = evas_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = evas_stringshare_add(task->dst.name);
        rtask->type = E_FM_OP_REMOVE;
        
        _e_fm_op_work_queue = evas_list_prepend(_e_fm_op_work_queue, rtask);

     }
   else if(task->type == E_FM_OP_MOVE)
     {
        /* Copy task. */
        _e_fm_op_update_progress(0, task->src.st.st_size);
        ctask = _e_fm_op_task_new();

        ctask->src.name = evas_stringshare_add(task->src.name);
        memcpy(&(ctask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          ctask->dst.name = evas_stringshare_add(task->dst.name);
        ctask->type = E_FM_OP_COPY;

        _e_fm_op_work_queue = evas_list_prepend(_e_fm_op_work_queue, ctask);

        /* Remove task. */
        _e_fm_op_update_progress(0, REMOVECHUNKSIZE);
        rtask = _e_fm_op_task_new();

        rtask->src.name = evas_stringshare_add(task->src.name);
        memcpy(&(rtask->src.st), &(task->src.st), sizeof(struct stat));
        if (task->dst.name)
          rtask->dst.name = evas_stringshare_add(task->dst.name);
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

        _e_fm_op_work_queue = evas_list_append_relative_list(_e_fm_op_work_queue, rtask, _e_fm_op_separator);

        ctask->link = _e_fm_op_separator->next;
     }

   return 1;
}

static int 
_e_fm_op_copy_stat_info_atom(E_Fm_Op_Task * task)
{
   E_FM_OP_DEBUG("Stat: %s --> %s\n", task->src.name, task->dst.name);

   _e_fm_op_copy_stat_info(task);
   task->finished = 1;
   task->dst.done += REMOVECHUNKSIZE;

   _e_fm_op_update_progress(REMOVECHUNKSIZE, 0);

   return 0;
}

static int
_e_fm_op_remove_atom(E_Fm_Op_Task * task)
{
   if (_e_fm_op_abort)
     {
	return 1;
     }

   E_FM_OP_DEBUG("Remove: %s\n", task->src.name);

   if (S_ISDIR(task->src.st.st_mode))
     {
	if (rmdir(task->src.name) == -1)
	  {
	     if (errno == ENOTEMPTY)
	       {
                  E_FM_OP_DEBUG("Attempt to remove non-empty directory.\n");
		  /* This should never happen due to way tasks are added to the work queue. If this happens (for example new files were created after the scan was complete), implicitly delete everything. */
                  ecore_file_recursive_rm(task->src.name);
                  task->finished = 1; /* Make sure that task is removed. */
		  return 1;
	       }
	     else
	       {
		   _E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot remove directory '%s': %s.", task->src.name);
	       }
	  }
     }
   else if (unlink(task->src.name) == -1)
     {
	_E_FM_OP_ERROR_SEND_WORK(task, E_FM_OP_ERROR, "Cannot remove file '%s': %s.", task->src.name);
     }

   task->dst.done += REMOVECHUNKSIZE;
   _e_fm_op_update_progress(REMOVECHUNKSIZE, 0);

   task->finished = 1;

   return 1;
}
