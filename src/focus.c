#include "debug.h"
#include "focus.h"
#include "e.h"
#include "util.h"

static Evas_List focus_list = NULL;

void
e_focus_set_focus(E_Border *b)
{
   D_ENTER;

   e_icccm_send_focus_to(b->win.client, e_focus_can_focus(b));   

   D_RETURN;
}

int
e_focus_can_focus(E_Border *b)
{
   D_ENTER;

   D_RETURN_(b->client.takes_focus);
}

void
e_focus_list_border_add(E_Border *b)
{
   D_ENTER;

   D_RETURN;
   UN(b);
}

void
e_focus_list_border_del(E_Border *b)
{
   D_ENTER;

   D_RETURN;
   UN(b);
}

void
e_focus_list_clear(void)
{
   D_ENTER;

   if (focus_list) 
     {
	evas_list_free(focus_list);
	focus_list = NULL;
     }

   D_RETURN;
}
