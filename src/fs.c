#include "e.h"

static EfsdConnection     *ec = NULL;
static Evas_List           fs_handlers = NULL;

static void _e_fs_fd_handle(int fd);

static void
_e_fs_fd_handle(int fd)
{
   EfsdEvent ev;

   #if 0 /* WE REALLY need NON BLOCKING comms with efsd! cK!!!!! */
   while (efsd_read_event(ec->fd, &ev) >= 0)
     {
	Evas_List l;
	
	for (l = fs_handlers; l; l = l->next)
	  {
	     void (*func) (EfsdEvent *ev);
	     
	     func = l->data;
	     func(&ev);
	  }
	efsd_cleanup_event(&ev);
     }
   #endif
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
   int i;

   /* already have an efsd around? */
   ec = efsd_open();
   /* no - efsd around */
   if (!ec)
     {
	/* start efsd */
	e_exec_run("efsd");   
	for (i = 0; (!ec); i++)
	  {
	     ec = efsd_open();
	     sleep(1);
	     if (i > 8) break;
	  }
     }
   if (!ec)
     {
	fprintf(stderr, "efsd is not running - please run efsd.\n");
	exit(-1);
     }
   e_add_event_fd(efsd_get_connection_fd(ec), _e_fs_fd_handle);
   
   /* HACK FIXME: testing.... */
   efsd_start_monitor(ec, getenv("HOME"));   
}
