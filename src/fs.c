#include "debug.h"
#include "fs.h"
#include "exec.h"

static EfsdConnection     *ec = NULL;
static Evas_List           fs_handlers = NULL;
static Evas_List           fs_restart_handlers = NULL;
static pid_t               efsd_pid = 0;

static void e_fs_child_handle(Ecore_Event *ev);
static void _e_fs_fd_handle(int fd);
static void _e_fs_restarter(int val, void *data);
static void e_fs_idle(void *data);
static void e_fs_flush_timeout(int val, void *data);

static void
e_fs_flush_timeout(int val, void *data)
{
   D_ENTER;

   if (!ec) D_RETURN;
   if (efsd_commands_pending(ec) > 0)
     {
	if (efsd_flush(ec) > 0)
	  ecore_add_event_timer("e_fs_flush_timeout()", 
			    0.00, e_fs_flush_timeout, 0, NULL);	
     }

   D_RETURN;
   UN(data);
   UN(val);
}

static void
e_fs_idle(void *data)
{
   D_ENTER;

   e_fs_flush_timeout(0, NULL);

   D_RETURN;
   UN(data);
}

static void
e_fs_child_handle(Ecore_Event *ev)
{
   Ecore_Event_Child *e;
   
   D_ENTER;

   e = ev->event;
   printf("pid went splat! (%i)\n", e->pid);
   if (e->pid == efsd_pid)
     {
	printf("it was efsd!\n");
	if (ec) efsd_close(ec);
	ec = NULL;
	efsd_pid = 0;
	_e_fs_restarter(1, NULL);
     }

   D_RETURN;
}

static void
_e_fs_fd_handle(int fd)
{
   double start, current;
   
   D_ENTER;

   start = ecore_get_time();
   while ((ec) && efsd_events_pending(ec))
     {
	EfsdEvent ev;
	
	if (efsd_next_event(ec, &ev) >= 0)
	  {
	     Evas_List l;
	     
	     for (l = fs_handlers; l; l = l->next)
	       {
		  void (*func) (EfsdEvent *ev);
		  
		  func = l->data;
		  func(&ev);
	       }
	     efsd_event_cleanup(&ev);
	  }
	else
	  {
	     efsd_close(ec);
	     ecore_del_event_fd(fd);
	     ec = NULL;
	     _e_fs_restarter(0, NULL);
	     /* FIXME: need to queue a popup dialog here saying */
	     /* efsd went wonky */
	     printf("EEEEEEEEEEK efsd went wonky. Bye bye efsd.\n");
	  }
	
	/* spent more thna 1/20th of a second here.. get out */
	current = ecore_get_time();
	if ((current - start) > 0.05) 
	  {
	     printf("fs... too much time spent..\n");
	     break;
	  }
     }

   D_RETURN;
}

static void
_e_fs_restarter(int val, void *data)
{
   D_ENTER;

   if (ec) D_RETURN;

   ec = efsd_open();

   if ((!ec) && (val > 0))
     {
	if (efsd_pid <= 0) 
	  {
	     efsd_pid = e_exec_run("efsd -f");
	     printf("launch efsd... %i\n", efsd_pid);
	  }
	if (efsd_pid > 0) ec = efsd_open();
     }
   if (ec)
     {
	Evas_List l;
	
	ecore_add_event_fd(efsd_get_connection_fd(ec), _e_fs_fd_handle);
	for (l = fs_restart_handlers; l; l = l->next)
	  {
	     E_FS_Restarter *rs;
	     
	     rs = l->data;
	     rs->func(rs->data);
	  }
     }
   else
     {
	double gap;
	
	gap = (double)val / 10;
	if (gap > 10.0) gap = 10.0;
	ecore_add_event_timer("e_fs_restarter", gap, _e_fs_restarter, val + 1, NULL);
     }

   D_RETURN;
   UN(data);
}

E_FS_Restarter *
e_fs_add_restart_handler(void (*func) (void *data), void *data)
{
   E_FS_Restarter *rs;
   
   D_ENTER;

   rs = NEW(E_FS_Restarter, 1);
   ZERO(rs, E_FS_Restarter, 1);
   rs->func = func;
   rs->data = data;
   fs_restart_handlers = evas_list_append(fs_restart_handlers, rs);

   D_RETURN_(rs);
}

void
e_fs_del_restart_handler(E_FS_Restarter *rs)
{
   D_ENTER;

   if (evas_list_find(fs_restart_handlers, rs))
     {
	fs_restart_handlers = evas_list_remove(fs_restart_handlers, rs);
	FREE(rs);	
     }

   D_RETURN;
}

void
e_fs_add_event_handler(void (*func) (EfsdEvent *ev))
{
   D_ENTER;

   if (!func) D_RETURN;
   fs_handlers = evas_list_append(fs_handlers, func);

   D_RETURN;
}

void
e_fs_init(void)
{
   D_ENTER;

   /* Hook in an fs handler that gets called whenever
      a child of this process exits.
   */
   ecore_event_filter_handler_add(ECORE_EVENT_CHILD, e_fs_child_handle);   

   /* Also hook in an idle handler to flush efsd's
      write queue.
      
      FIXME: This should be handled by letting ecore
      report when Efsd's file descriptor becomes
      writeable, and then calling efsd_flush().
   */
   ecore_event_filter_idle_handler_add(e_fs_idle, NULL);   
   _e_fs_restarter(0, NULL);

   D_RETURN;
}

EfsdConnection *
e_fs_get_connection(void)
{
   D_ENTER;

   D_RETURN_(ec);
}
