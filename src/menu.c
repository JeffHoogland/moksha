#include "e.h"

static Evas_List open_menus = NULL;
static Evas_list menus = NULL;

static void e_idle(void *data);
static void e_key_down(Eevent * ev);
static void e_key_up(Eevent * ev);
static void e_mouse_down(Eevent * ev);
static void e_mouse_up(Eevent * ev);
static void e_mouse_in(Eevent * ev);
static void e_mouse_out(Eevent * ev);
static void e_window_expose(Eevent * ev);

static void
e_idle(void *data)
{
   Evas_List l;
   
   for (l = menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	e_menu_update(m);
     }
   e_db_runtime_flush();
   return;
   UN(data);
}

static void
e_key_down(Eevent * ev)
{
   Ev_Key_Down          *e;
   
   e = ev->event;
     {
	if (!strcmp(e->key, "Up"))
	  {
	  }
	else if (!strcmp(e->key, "Down"))
	  {
	  }
	else if (!strcmp(e->key, "Left"))
	  {
	  }
	else if (!strcmp(e->key, "Right"))
	  {
	  }
	else if (!strcmp(e->key, "Escape"))
	  {
	  }
	else
	  {
	  }
     }
}

static void
e_key_up(Eevent * ev)
{
   Ev_Key_Up          *e;
   
   e = ev->event;
     {
     }
}

E_Menu *
e_menu_new(void)
{
}

void
e_menu_free(E_Menu *m)
{
}

void
e_menu_hide(E_Menu *m)
{
}

void
e_menu_show(E_Menu *m)
{
}

void
e_menu_move_to(E_Menu *m, int x, int y)
{
}

void
e_menu_show_at_mouse(E_Menu *m, int x, int y)
{
}

void
e_menu_add_item(E_Menu *m, E_Menu_Item *mi)
{
}

void
e_menu_del_item(E_Menu *m, E_Menu_Item *mi)
{
}

void
e_menu_update(E_Menu *m)
{
}
