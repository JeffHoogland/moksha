#include "e.h"

static EfsdConnection     *ec = NULL;
static Evas_List           fs_handlers = NULL;

static void _e_fs_fd_handle(int fd);

static void
_e_fs_fd_handle(int fd)
{
   Evas_List events = NULL;
   double start, current;
   
   printf("############## fs event...\n");
   start = e_get_time();
   while ((ec) && efsd_events_pending(ec))
     {
	EfsdEvent *ev;
	
	ev = NEW(EfsdEvent, 1);
	ZERO(ev, EfsdEvent, 1);
	
	if (efsd_next_event(ec, ev) >= 0)
	  {
	     events = evas_list_append(events, ev);
	  }
	else
	  {
	     FREE(ev);
	     efsd_close(ec);
	     e_del_event_fd(fd);
	     ec = NULL;
	     /* FIXME: need to queue a popup dialog here saying */
	     /* efsd went wonky */
	     printf("EEEEEEEEEEK efsd went wonky\n");
	  }
	
	/* spent more thna 1/20th of a second here.. get out */
	current = e_get_time();
	if ((current - start) > 0.05) 
	  {
	     printf("fs... too much time spent..\n");
	     break;
	  }
    }
   if (events)
     {
	Evas_List l;
	
	for (l = events; l; l = l->next)
	  {
	     Evas_List ll;
	     EfsdEvent *ev;
	     
	     ev = l->data;
	     for (ll = fs_handlers; ll; ll = ll->next)
	       {
		  void (*func) (EfsdEvent *ev);
		  
		  func = ll->data;
		  func(ev);
	       }
	     efsd_event_cleanup(ev);
	     FREE(ev);
	  }
	evas_list_free(events);
     }
   printf("############## fs done\n");
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
	for (i = 0; (!ec) && (i < 4); i++)
	  {
	    sleep(1);
	    ec = efsd_open();
	  }
     }
   /* after several atempts to talk to efsd - lets give up */
   if (!ec)
     {
	fprintf(stderr, "efsd is not running - please run efsd.\n");
	exit(-1);
     }
   e_add_event_fd(efsd_get_connection_fd(ec), _e_fs_fd_handle);
}

EfsdConnection *
e_fs_get_connection(void)
{
   return ec;
}
