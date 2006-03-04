#include "e.h"

#define ELOCK_POPUP_LAYER 10000
#define PASSWD_LEN 256

/**************************** private data ******************************/
typedef struct _E_Desklock_Data		E_Desklock_Data;
typedef struct _E_Desklock_Popup_Data	E_Desklock_Popup_Data;

struct _E_Desklock_Popup_Data
{
   E_Popup	*popup_wnd;
   Evas_Object	*bg_object;
   Evas_Object	*login_box;
};

struct _E_Desklock_Data
{
   Evas_List	  *elock_wnd_list;
   Ecore_X_Window  elock_wnd;
   Evas_List	  *handlers;
   Ecore_X_Window  elock_grab_break_wnd;
   char		  passwd[PASSWD_LEN];
};

static	E_Desklock_Data	*edd = NULL;
static	E_Zone		*last_active_zone = NULL;

/***********************************************************************/

static int _e_desklock_cb_key_down(void *data, int type, void *event);
static int _e_desklock_cb_mouse_down(void *data, int type, void *event);
static int _e_desklock_cb_mouse_up(void *data, int type, void *event);
static int _e_desklock_cb_mouse_wheel(void *data, int type, void *event);
static int _e_desklock_cb_mouse_move(void *data, int type, void *event);
//static int _e_desklock_idler(void *data);

static void _e_desklock_passwd_update();
static void _e_desklock_backspace();
static void _e_desklock_delete();
static int  _e_desklock_zone_num_get();

EAPI int
e_desklock_show(void)
{
   Evas_List		  *managers, *l, *l2, *l3;
   E_Desklock_Popup_Data  *edp;
   Evas_Coord		  mw, mh;
   E_Zone		  *current_zone;
   int			  zone_counter;
   int			  total_zone_num;

   if (edd) return 0;
   
   if (!e_config->desklock_personal_passwd)
     {
	E_Zone  *zone;
	
	zone = e_util_zone_current_get(e_manager_current_get());
	if (zone)
	  {
	     E_Config_Dialog *cfd;
	     
	     cfd = e_int_config_desklock(zone->container);
	  }
	return 0;
     }
   
   edd = E_NEW(E_Desklock_Data, 1);
   if (!edd) return 0;
   edd->elock_wnd = ecore_x_window_input_new(e_manager_current_get()->root, 
					     0, 0, 1, 1);
   ecore_x_window_show(edd->elock_wnd);
   managers = e_manager_list();
   if (!e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd))
     {
	for (l = managers; l; l = l->next)
	  {
	     E_Manager *man;
	     Ecore_X_Window *windows;
	     int wnum;
	     
	     man = l->data;
	     windows = ecore_x_window_children_get(man->root, &wnum);
	     if (windows)
	       {
		  int i;
		  
		  for (i = 0; i < wnum; i++)
		    {
		       Ecore_X_Window_Attributes att;
		       memset(&att, 0, sizeof(Ecore_X_Window_Attributes));
		       
		       ecore_x_window_attributes_get(windows[i], &att);
		       if (att.visible)
			 {
			    ecore_x_window_hide(windows[i]);
			    if (e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd))
			      {
				 edd->elock_grab_break_wnd = windows[i];
				 free(windows);
				 goto works;
			      }
			    ecore_x_window_show(windows[i]);
			 }
		    }
		  free(windows);
	       }
	  }
	/* everything failed - cant lock */
	e_util_dialog_show(_("Lock Failed"),
			   _("Locking the desktop failed because some application<br>"
			     "has grabbed either they keyboard or the mouse or both<br>"
			     "and their grab is unable to be broken."));
	ecore_x_window_del(edd->elock_wnd);
	free(edd);
	edd = NULL;
	return 0;
     }
   works:
   
   last_active_zone = current_zone = 
     e_zone_current_get(e_container_current_get(e_manager_current_get()));
   
   zone_counter = 0;
   total_zone_num = _e_desklock_zone_num_get();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  E_Zone *zone;
		  
		  zone = l3->data;
		  edp = E_NEW(E_Desklock_Popup_Data, 1);
		  if (edp)
		    {
		       edp->popup_wnd = e_popup_new(zone, 0, 0, zone->w, zone->h);
		       evas_event_feed_mouse_move(edp->popup_wnd->evas, -1000000, -1000000,
						  ecore_x_current_time_get(), NULL);
		       
		       e_popup_layer_set(edp->popup_wnd, ELOCK_POPUP_LAYER);
		       ecore_evas_raise(edp->popup_wnd->ecore_evas);
		       
		       evas_event_freeze(edp->popup_wnd->evas);
		       edp->bg_object = edje_object_add(edp->popup_wnd->evas);

		       if ((!e_config->desklock_background) ||
			   (!strcmp(e_config->desklock_background, "theme_desklock_background")))
			 {
			   e_theme_edje_object_set(edp->bg_object,
						   "base/theme/desklock",
						   "desklock/background");
			 }
		       else if (!strcmp(e_config->desklock_background, "theme_background"))
			 {
			    e_theme_edje_object_set(edp->bg_object,
						    "base/theme/backgrounds",
						    "desktop/background");
			 }
		       else
			 {
			   if (e_util_edje_collection_exists(e_config->desklock_background,
							     "desklock/background"))
			     {
			       edje_object_file_set(edp->bg_object, e_config->desklock_background,
						    "desklock/background");
			     }
			   else
			     {
			       if (!edje_object_file_set(edp->bg_object,
							 e_config->desklock_background,
							 "desktop/background"))
				 {
				   edje_object_file_set(edp->bg_object,
						      e_theme_edje_file_get("base/theme/desklock",
									    "desklock/background"),
									    "desklock/background");
				 }
			     }
			 }

		       evas_object_move(edp->bg_object, 0, 0);
		       evas_object_resize(edp->bg_object, zone->w, zone->h);
		       evas_object_show(edp->bg_object);
		       edp->login_box = edje_object_add(edp->popup_wnd->evas);
		       e_theme_edje_object_set(edp->login_box,
					       "base/theme/desklock",
					       "desklock/login_box");
		       edje_object_part_text_set(edp->login_box, "title", 
						 _("Please enter your unlock password"));
		       edje_object_part_swallow(edp->bg_object, "login_box", edp->login_box);
		       edje_object_size_min_calc(edp->login_box, &mw, &mh);
		       evas_object_move(edp->login_box, (int)((zone->w - mw)/2),
						    (int)((zone->h - mh)/2));

		       if (total_zone_num > 1)
		       {
			 if (e_config->desklock_login_box_zone == -1)
			   evas_object_show(edp->login_box);
			 else if(e_config->desklock_login_box_zone == -2 && zone == current_zone)
			   evas_object_show(edp->login_box);
			 else if(e_config->desklock_login_box_zone == zone_counter )
			   evas_object_show(edp->login_box);
		       }
		       else
			 evas_object_show(edp->login_box);
		       /**/
		       
		       e_popup_edje_bg_object_set(edp->popup_wnd, edp->bg_object);
		       evas_event_thaw(edp->popup_wnd->evas);
		       
		       e_popup_show(edp->popup_wnd);
		       
		       edd->elock_wnd_list = evas_list_append(edd->elock_wnd_list, edp);
		    }

		  zone_counter ++;
	       }
	  }
     }
   
   /* handlers */
   edd->handlers = evas_list_append(edd->handlers,
				    ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
							    _e_desklock_cb_key_down, NULL));
   edd->handlers = evas_list_append(edd->handlers, 
				    ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
							    _e_desklock_cb_mouse_down, NULL));
   edd->handlers = evas_list_append(edd->handlers,
				    ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
							    _e_desklock_cb_mouse_up, NULL));
   edd->handlers = evas_list_append(edd->handlers,
				    ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
							    _e_desklock_cb_mouse_wheel,
							    NULL));
   if (total_zone_num > 1 && e_config->desklock_login_box_zone == -2 )
     edd->handlers = evas_list_append(edd->handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,
							      _e_desklock_cb_mouse_move,
							      NULL));
   //elock_wnd_idler = ecore_idler_add(_e_desklock_idler, NULL);
   
   _e_desklock_passwd_update();
   return 1;
}

EAPI void
e_desklock_hide(void)
{
   E_Desklock_Popup_Data	*edp;
   
   if (!edd) return;

   if (edd->elock_grab_break_wnd)
     ecore_x_window_show(edd->elock_grab_break_wnd);
   while (edd->elock_wnd_list)
     {
	edp = edd->elock_wnd_list->data;
	if (edp)
	  {
	     e_popup_hide(edp->popup_wnd);
	     
	     evas_event_freeze(edp->popup_wnd->evas);
	     evas_object_del(edp->bg_object);
	     evas_object_del(edp->login_box);
	     evas_event_thaw(edp->popup_wnd->evas);
	     
	     e_object_del(E_OBJECT(edp->popup_wnd));
	     E_FREE(edp);
	  }
	edd->elock_wnd_list = evas_list_remove_list(edd->elock_wnd_list, edd->elock_wnd_list);
     }
   
   while (edd->handlers)
     {
	ecore_event_handler_del(edd->handlers->data);
	edd->handlers = evas_list_remove_list(edd->handlers, edd->handlers);
     }
   
   e_grabinput_release(edd->elock_wnd, edd->elock_wnd);
   ecore_x_window_del(edd->elock_wnd);
   
   E_FREE(edd);
   edd = NULL;
}

static int
_e_desklock_cb_key_down(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev;
   
   ev = event;
   if (ev->win != edd->elock_wnd) return 1;
   
   if (!strcmp(ev->keysymbol, "Escape"))
    ;
   else if (!strcmp(ev->keysymbol, "KP_Enter"))
     {
	// here we have to go to auth
	if (!strcmp(edd->passwd, e_config->desklock_personal_passwd))
	  {
	     e_desklock_hide();
	     return 1;
	  }
	else
	  ; // report about invalid password
	
	memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
	_e_desklock_passwd_update();
     }
   else if (!strcmp(ev->keysymbol, "Return"))
     {
	// here we have to go to auth
	if ((e_config->desklock_personal_passwd) &&
	    (!strcmp(edd->passwd, e_config->desklock_personal_passwd)))
	  {
	     e_desklock_hide();
	     return 1;
	  }
	else
	  ; // report about invalid password
	
	memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
	_e_desklock_passwd_update();
     }
   else if (!strcmp(ev->keysymbol, "BackSpace"))
     _e_desklock_backspace();
   else if (!strcmp(ev->keysymbol, "Delete"))
     _e_desklock_delete();
   else
     {
	// here we have to grab a password
	if (ev->key_compose)
	  {
	     if ((strlen(edd->passwd) < (PASSWD_LEN - strlen(ev->key_compose))))
	       {
		  strcat(edd->passwd, ev->key_compose);
		  _e_desklock_passwd_update();
	       }
	  }
     }
   
   return 1;
}

static int
_e_desklock_cb_mouse_down(void *data, int type, void *event)
{
   return 1;
}
static int
_e_desklock_cb_mouse_up(void *data, int type, void *event)
{
   return 1;
}
static int
_e_desklock_cb_mouse_wheel(void *data, int type, void *event)
{
   return 1;
}
static int
_e_desklock_cb_mouse_move(void *data, int type, void *event)
{
  E_Desklock_Popup_Data	*edp;
  E_Zone *current_zone;
  Evas_List *l;

  current_zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));

  if (current_zone == last_active_zone)
    return 1;

  for (l = edd->elock_wnd_list; l; l = l->next)
    {
      edp = l->data;

      if (!edp) continue;

      if (edp->popup_wnd->zone == last_active_zone)
	evas_object_hide(edp->login_box);
      else if (edp->popup_wnd->zone == current_zone)
	evas_object_show(edp->login_box);
    }
  last_active_zone = current_zone;
  return 1;
}
static int
_e_desklock_idler(void *data)
{
   return 1;
}

EAPI int
e_desklock_init(void)
{
   if (e_config->desklock_disable_screensaver)
     ecore_x_screensaver_timeout_set(0);
   else
     {
	if (e_config->desklock_use_timeout)
	  ecore_x_screensaver_timeout_set(e_config->desklock_timeout);
     }
   return 1;
}

EAPI int
e_desklock_shutdown(void)
{
   e_desklock_hide();
   return 1;
}

static void
_e_desklock_passwd_update()
{
   int ii;
   char passwd_hidden[PASSWD_LEN * 3]="";
   E_Desklock_Popup_Data	*edp;
   Evas_List *l;
   
   if (!edd) return;
   
   for (ii = 0; ii < strlen(edd->passwd); ii ++)
     {
	passwd_hidden[ii] = '*';
	passwd_hidden[ii+1] = 0;
     }
   
   for (l = edd->elock_wnd_list; l; l = l->next)
     {
	edp = l->data;
	edje_object_part_text_set(edp->login_box, "passwd", passwd_hidden);
     }
}

static void
_e_desklock_backspace()
{
  int len, val, pos;

  if (!edd) return;
   
   len = strlen(edd->passwd);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(edd->passwd, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     edd->passwd[pos] = 0;
	     _e_desklock_passwd_update();
	  }
     }
}

static void
_e_desklock_delete()
{
  _e_desklock_backspace();
}

static int
_e_desklock_zone_num_get()
{
  int num;
  Evas_List *l, *l2;

  num = 0;
  for (l = e_manager_list(); l; l = l->next)
    {
      E_Manager *man = l->data;

      for (l2 = man->containers; l2; l2 = l2->next)
	{
	  E_Container *con = l2->data;

	  num += evas_list_count(con->zones);
	}
    }

  return num;
}

