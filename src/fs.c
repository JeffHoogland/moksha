#include "fs.h"
#include "exec.h"

static EfsdConnection     *ec = NULL;
static Evas_List           fs_handlers = NULL;
static Evas_List           fs_restart_handlers = NULL;
static pid_t               efsd_pid = 0;

static void e_fs_child_handle(Eevent *ev);
static void _e_fs_fd_handle(int fd);
static void _e_fs_restarter(int val, void *data);

static void
e_fs_child_handle(Eevent *ev)
{
   Ev_Child *e;
   
   e = ev->event;
   if (e->pid == efsd_pid)
     {
	efsd_close(ec);
	efsd_pid = 0;
	ec = NULL;
	_e_fs_restarter(0, NULL);
     }
}

static void
_e_fs_fd_handle(int fd)
{
   double start, current;
   
   start = e_get_time();
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
	     e_del_event_fd(fd);
	     ec = NULL;
	     if (efsd_pid == -2)
	       _e_fs_restarter(0, NULL);
	     /* FIXME: need to queue a popup dialog here saying */
	     /* efsd went wonky */
	     printf("EEEEEEEEEEK efsd went wonky. Bye bye efsd.\n");
	  }
	
	/* spent more thna 1/20th of a second here.. get out */
	current = e_get_time();
	if ((current - start) > 0.05) 
	  {
	     printf("fs... too much time spent..\n");
	     break;
	  }
    }
}

static void
_e_fs_restarter(int val, void *data)
{
   if (ec) return;
   if (val > 0)    
     {
	if (efsd_pid <= 0) efsd_pid = e_exec_run("efsd -f");
	if (efsd_pid > 0) ec = efsd_open();
     }
   if (ec)
     {
	Evas_List l;
	
	e_add_event_fd(efsd_get_connection_fd(ec), _e_fs_fd_handle);
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
	e_add_event_timer("e_fs_restarter", gap, _e_fs_restarter, val + 1, NULL);
     }
   UN(data);
}

E_FS_Restarter *
e_fs_add_restart_handler(void (*func) (void *data), void *data)
{
   E_FS_Restarter *rs;
   
   rs = NEW(E_FS_Restarter, 1);
   ZERO(rs, E_FS_Restarter, 1);
   rs->func = func;
   rs->data = data;
   fs_restart_handlers = evas_list_append(fs_restart_handlers, rs);

   return rs;
}

void
e_fs_del_restart_handler(E_FS_Restarter *rs)
{
   if (evas_list_find(fs_restart_handlers, rs))
     {
	fs_restart_handlers = evas_list_remove(fs_restart_handlers, rs);
	FREE(rs);	
     }
}

void
e_fs_add_event_handler(void (*func) (EfsdEvent *ev))
{
   if (!func) return;
   fs_handlers = evas_list_append(fs_handlers, func);
}

void
e_fs_init(void)
{
   e_event_filter_handler_add(EV_CHILD, e_fs_child_handle);   
   _e_fs_restarter(0, NULL);
}

EfsdConnection *
e_fs_get_connection(void)
{
   return ec;
}
