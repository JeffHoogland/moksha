#include "focus.h"

static Evas_List focus_list = NULL;

void
e_focus_set_focus(E_Border *b)
{
   if (e_focus_can_focus(b)) ecore_focus_to_window(b->win.client);   
}

int
e_focus_can_focus(E_Border *b)
{
   return (b->client.takes_focus);
}

void
e_focus_list_border_add(E_Border *b)
{
}

void
e_focus_list_border_del(E_Border *b)
{
}

void
e_focus_list_clear(void)
{
   if (focus_list) 
     {
	evas_list_free(focus_list);
	focus_list = NULL;
     }
}
