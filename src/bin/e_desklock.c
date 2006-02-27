#include "e.h"

#define ELOCK_POPUP_LAYER 10000
#define PASSWD_LEN 256

/**************************** private data ******************************/
typedef struct _E_Desklock_Data		E_Desklock_Data;
typedef struct _E_Desklock_Popup_Data	E_Desklock_Popup_Data;

struct _E_Desklock_Popup_Data
{
   E_Popup     *popup_wnd;
   Evas_Object *bg_object;
};

struct _E_Desklock_Data
{
   Evas_List	  *elock_wnd_list;
   Ecore_X_Window  elock_wnd;
   Evas_List	  *handlers;
   char		  passwd[PASSWD_LEN];
};

static	E_Desklock_Data	*edd = NULL;

/***********************************************************************/

static int _e_desklock_cb_key_down(void *data, int type, void *event);
static int _e_desklock_cb_mouse_down(void *data, int type, void *event);
static int _e_desklock_cb_mouse_up(void *data, int type, void *event);
static int _e_desklock_cb_mouse_wheel(void *data, int type, void *event);
//static int _e_desklock_idler(void *data);

static void _e_desklock_passwd_update();
static void _e_desklock_backspace();
static void _e_desklock_delete();

EAPI int
e_desklock_show(void)
{
   Evas_List  *managers, *l, *l2, *l3;
   int m = 0, c = 0, z = 0;   
   E_Desklock_Popup_Data	*edp;

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
   
   if (!edd)
     {
	edd = E_NEW(E_Desklock_Data, 1);
	if (!edd) return 0;
	edd->elock_wnd_list = NULL;
	edd->elock_wnd = 0;
	edd->handlers = NULL;
	edd->passwd[0] = 0;
     }
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	m++;
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     c++;
	     
	     con = l2->data;
	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  E_Zone *zone;
		  
		  zone = l3->data;
		  if (!edd->elock_wnd)
		    {
		       edd->elock_wnd = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
		       ecore_x_window_show(edd->elock_wnd);
		       e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd);
		    }
		  
		  edp = E_NEW(E_Desklock_Popup_Data, 1);
		  if (edp)
		    {
		       edp->popup_wnd = e_popup_new(zone, 0, 0, zone->w, zone->h);
		       evas_event_feed_mouse_move(edp->popup_wnd->evas, -1000000, -1000000,
						  ecore_x_current_time_get(), NULL);
		       
		       e_popup_layer_set(edp->popup_wnd, ELOCK_POPUP_LAYER);
		       
		       evas_event_freeze(edp->popup_wnd->evas);
		       edp->bg_object = edje_object_add(edp->popup_wnd->evas);
		       //FIXME: This should come from config file
		       e_theme_edje_object_set(edp->bg_object,
					       "base/theme/desklock",
					       "widgets/desklock/main");
		       
		       evas_object_move(edp->bg_object, 0, 0);
		       evas_object_resize(edp->bg_object, zone->w, zone->h);
		       evas_object_show(edp->bg_object);
		       edje_object_part_text_set(edp->bg_object, "title", 
						 _("Please enter your unlock password"));
		       
		       e_popup_edje_bg_object_set(edp->popup_wnd, edp->bg_object);
		       evas_event_thaw(edp->popup_wnd->evas);
		       
		       e_popup_show(edp->popup_wnd);
		       
		       edd->elock_wnd_list = evas_list_append(edd->elock_wnd_list, edp);
		    }
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
   //elock_wnd_idler = ecore_idler_add(_e_desklock_idler, NULL);
   
   _e_desklock_passwd_update();
   return 1;
}

EAPI void
e_desklock_hide(void)
{
   E_Desklock_Popup_Data	*edp;
   
   if (!edd) return;
   
   while (edd->elock_wnd_list)
     {
	edp = edd->elock_wnd_list->data;
	if (edp)
	  {
	     e_popup_hide(edp->popup_wnd);
	     
	     evas_event_freeze(edp->popup_wnd->evas);
	     evas_object_del(edp->bg_object);
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
_e_desklock_idler(void *data)
{
   return 1;
}

EAPI int
e_desklock_init(void)
{
//  desklock_edd = E_CONFIG_DD_NEW("E_Desklock", E_Decklock);
//#undef T
//#undef D
//#define T E_Desklock
//#define D desklock_edd
  //E_CONFIG_VAL(D, T, path, STR);

//#undef T
//#undef D

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
	edje_object_part_text_set(edp->bg_object, "passwd", passwd_hidden);
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


