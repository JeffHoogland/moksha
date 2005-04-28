/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

void
e_hints_init(void)
{
   Ecore_X_Window *roots = NULL;
   int num;

   roots = ecore_x_window_root_list(&num);
   if (roots)
     {
	int i;
	
	for (i = 0; i < num; i++)
	  {
	     Ecore_X_Window win;
	     
	     win = ecore_x_window_new(roots[i], -200, -200, 5, 5);
//	     I don't FUCKING believe it. if we PRETENT we are Kwin - java is
//	     happy. why? it expects a double reparenting wm then. java insists
//	     on finding this out when it shoudl be irrelevant! stupid FUCKS.
	     ecore_x_netwm_wm_identify(roots[i], win, "KWin");
//	     ecore_x_netwm_wm_identify(roots[i], win, "Enlightenment");
	  }
        free(roots);
     }
}

void
e_hints_client_list_set(void)
{
   Evas_List *ml = NULL, *cl = NULL, *bl = NULL;
   unsigned int i = 0, num = 0;
   E_Manager *m;
   E_Container *c;
   E_Border *b;
   Ecore_X_Window *clients = NULL;

   /* Get client count by adding client lists on all containers */
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	m = ml->data;
	for (cl = m->containers; cl; cl = cl->next)
	  {
	     c = cl->data;
	     num += evas_list_count(c->clients);
	  }
     }
   
   clients = calloc(num, sizeof(Ecore_X_Window));
   if (!clients)
      return;

   /* Fetch window IDs and add to array */
   if (num > 0)
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     for (cl = m->containers; cl; cl = cl->next)
	       {
		  c = cl->data;
		  for (bl = c->clients; bl; bl = bl->next)
		    {
		       b = bl->data;
		       clients[i++] = b->win;
		    }
	       }
	  }
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_set(m->root, num, clients);
	     ecore_x_netwm_client_list_stacking_set(m->root, num, clients);
	  }
     }
   else
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_set(m->root, 0, NULL);
	     ecore_x_netwm_client_list_stacking_set(m->root, 0, NULL);
	  }
     }

}

/* Client list is already in stacking order, so this function is nearly
 * identical to the previous one */
void
e_hints_client_stacking_set(void)
{
   Evas_List *ml = NULL, *cl = NULL, *bl = NULL;
   unsigned int i = 0, num = 0;
   E_Manager *m;
   E_Container *c;
   E_Border *b;
   Ecore_X_Window *clients = NULL;

   /* Get client count */
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	m = ml->data;
	for (cl = m->containers; cl; cl = cl->next)
	  {
	     c = cl->data;
	     num += evas_list_count(c->clients);
	  }
     }
   
   clients = calloc(num, sizeof(Ecore_X_Window));
   if (!clients)
      return;

   if (num > 0)
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     for (cl = m->containers; cl; cl = cl->next)
	       {
		  c = cl->data;
		  for (bl = c->clients; bl; bl = bl->next)
		    {
		       b = bl->data;
		       clients[i++] = b->win;
		    }
	       }
	  }
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_stacking_set(m->root, num, clients);
	  }
     }
   else
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_stacking_set(m->root, 0, NULL);
	  }
     }

}

void
e_hints_active_window_set(E_Manager *man, Ecore_X_Window win)
{
   E_OBJECT_CHECK(man);
   ecore_x_netwm_client_active_set(man->root, win);
}

void
e_hints_window_name_set(Ecore_X_Window win, const char *name)
{
   ecore_x_icccm_title_set(win, name);
   ecore_x_netwm_name_set(win, name);
}

char *
e_hints_window_name_get(Ecore_X_Window win)
{
   char		*name;

   name = ecore_x_netwm_name_get(win);
   if (!name)
     name = ecore_x_icccm_title_get(win);
   if (!name)
     name = strdup("No name!!");
   return name;
}

void
e_hints_desktop_config_set(void)
{
   /* Set desktop count, desktop names and workarea */
   
   int			i = 0, num = 0;
   unsigned int		*areas = NULL;
   E_Manager		*m;
   E_Container		*c;
   Evas_List		*ml, *cl;
   Ecore_X_Window	*vroots = NULL;
   /* FIXME: Desktop names not yet implemented */
/*   char			**names; */

   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	m = ml->data;
	num += evas_list_count(m->containers);
     }

   vroots = calloc(num, sizeof(Ecore_X_Window));
   if (!vroots) return;
   
/*   names = calloc(num, sizeof(char *));*/
   
   areas = calloc(4 * num, sizeof(unsigned int));
   if (!areas)
     {
	free(vroots);
	return;
     }
   
   for (ml = e_manager_list(); ml; ml=ml->next)
     {
	m = ml->data;
	for (cl = m->containers; cl; cl=cl->next)
	  {
	     c = cl->data;
	     areas[4 * i] = c->x;
	     areas[4 * i + 1] = c->y;
	     areas[4 * i + 2] = c->w;
	     areas[4 * i + 3] = c->h;
	     vroots[i++] = c->win;
	  }
     }
   
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	m = ml->data;
	ecore_x_netwm_desk_count_set(m->root, num);
	if (e_config->use_virtual_roots)
	  {
	     ecore_x_netwm_desk_roots_set(m->root, num, vroots);
	  }
	ecore_x_netwm_desk_workareas_set(m->root, num, areas);
     }
   free(vroots);
   free(areas);
}

void
e_hints_window_state_set(Ecore_X_Window win)
{
   E_Border	*bd;

   bd = e_border_find_by_client_window(win);
   
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_MODAL, 
				  bd->client.netwm.state.modal);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_STICKY,
				  bd->sticky);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_MAXIMIZED_VERT,
				  bd->client.netwm.state.maximized_v);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ,
				  bd->client.netwm.state.maximized_h);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_SHADED,
				  bd->shaded);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_SKIP_TASKBAR,
				  bd->client.netwm.state.skip_taskbar);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_SKIP_PAGER,
				  bd->client.netwm.state.skip_pager);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_HIDDEN,
				  !(bd->visible));
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_FULLSCREEN,
				  bd->client.netwm.state.fullscreen);
   
   switch (bd->client.netwm.state.stacking)
     {
      case 1:
	 ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_ABOVE, 1);
	 ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_BELOW, 0);
      case 2:
	 ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_ABOVE, 0);
	 ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_BELOW, 1);
      default:
	 ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_ABOVE, 0);
	 ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_BELOW, 0);
	 break;
     }
}

void
e_hints_window_state_get(Ecore_X_Window win)
{
   E_Border	*bd;
   int		 above, below;

   bd = e_border_find_by_client_window(win);
   
   bd->client.netwm.state.modal = 
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_MODAL);
   bd->sticky = 
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_STICKY);
   bd->client.netwm.state.maximized_v = 
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_MAXIMIZED_VERT);
   bd->client.netwm.state.maximized_h = 
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ);
   bd->shaded =
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_SHADED);
   bd->client.netwm.state.skip_taskbar =
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_SKIP_TASKBAR);
   bd->client.netwm.state.skip_pager =
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_SKIP_PAGER);
   bd->visible =
      !ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_HIDDEN);
   bd->client.netwm.state.fullscreen =
      ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_FULLSCREEN);
   
   above = ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_ABOVE);
   below = ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_BELOW);
   bd->client.netwm.state.stacking = (above << 0) + (below << 1);
}

void
e_hints_window_visible_set(Ecore_X_Window win)
{
   ecore_x_icccm_state_set(win, ECORE_X_WINDOW_STATE_HINT_NORMAL);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_HIDDEN, 0);
}

int
e_hints_window_visible_isset(Ecore_X_Window win)
{
   return !ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_HIDDEN)
	  || (ecore_x_icccm_state_get(win) == ECORE_X_WINDOW_STATE_HINT_NORMAL);
}

void
e_hints_window_iconic_set(Ecore_X_Window win)
{
   ecore_x_icccm_state_set(win, ECORE_X_WINDOW_STATE_HINT_ICONIC);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_HIDDEN, 1);
}

int
e_hints_window_iconic_isset(Ecore_X_Window win)
{
   return (ecore_x_icccm_state_get(win) == ECORE_X_WINDOW_STATE_HINT_ICONIC);
}

void
e_hints_window_hidden_set(Ecore_X_Window win)
{
   ecore_x_icccm_state_set(win, ECORE_X_WINDOW_STATE_HINT_WITHDRAWN);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_HIDDEN, 1);
}

int
e_hints_window_hidden_isset(Ecore_X_Window win)
{
   return ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_HIDDEN)
	  || (ecore_x_icccm_state_get(win) != ECORE_X_WINDOW_STATE_HINT_NORMAL);
}

void
e_hints_window_shaded_set(Ecore_X_Window win, int on)
{
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_SHADED, on);
}

int
e_hints_window_shaded_isset(Ecore_X_Window win)
{
   return ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_SHADED);
}

void
e_hints_window_shade_direction_set(Ecore_X_Window win, E_Direction dir)
{
   ecore_x_window_prop_card32_set(win, E_ATOM_SHADE_DIRECTION, &dir, 1);
}

E_Direction
e_hints_window_shade_direction_get(Ecore_X_Window win)
{
   int ret;
   E_Direction dir;

   ret = ecore_x_window_prop_card32_get(win,
					E_ATOM_SHADE_DIRECTION,
				       	&dir, 1);
   if (ret == 1)
     return dir;

   return E_DIRECTION_UP;
}

void
e_hints_window_maximized_set(Ecore_X_Window win, int on)
{
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_MAXIMIZED_VERT, on);
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ, on);
}

int
e_hints_window_maximized_isset(Ecore_X_Window win)
{
   return ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_MAXIMIZED_VERT)
	  && ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ);
}

void
e_hints_window_fullscreen_set(Ecore_X_Window win, int on)
{
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_FULLSCREEN, on);
}

int
e_hints_window_fullscreen_isset(Ecore_X_Window win)
{
   return ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_FULLSCREEN);
}

void
e_hints_window_sticky_set(Ecore_X_Window win, int on)
{
   ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_STICKY, on);
}

int
e_hints_window_sticky_isset(Ecore_X_Window win)
{
   return ecore_x_netwm_window_state_isset(win, ECORE_X_WINDOW_STATE_STICKY);
}

/*
ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_MODAL, on);
ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_SKIP_TASKBAR, on);
ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_SKIP_PAGER, on);
ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_FULLSCREEN, on);
ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_ABOVE, on);
ecore_x_netwm_window_state_set(win, ECORE_X_WINDOW_STATE_BELOW, on);
*/

void
e_hints_window_icon_name_get(Ecore_X_Window win)
{
   E_Border	*bd;
   char		*name;

   name = ecore_x_netwm_icon_name_get(win);
   bd = e_border_find_by_client_window(win);
   if (bd->client.icccm.icon_name)
     free(bd->client.icccm.icon_name);
   bd->client.icccm.icon_name = name;
   bd->changed = 1;
}
