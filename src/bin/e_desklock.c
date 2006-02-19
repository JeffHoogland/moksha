
#include "e.h"

#define ELOCK_POPUP_LAYER 10000

#define PASSWD_LEN 256

static Ecore_X_Window elock_wnd = 0;
static Evas_List *handlers = NULL;
static E_Popup *elock_wnd_popup = NULL;

static Evas_Object *bg_object = NULL;
static Evas_Object *passwd_entry = NULL;

static char passwd[PASSWD_LEN]="";

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
  //Evas_List  *managers, *l, *l2, *l3;
  Evas_Object *eo;
  int x, y, w, h;
  //Evas_List *elock_wnd_list = NULL;
  
  E_Zone  *zone = NULL;

  e_error_dialog_show(_("Enlightenment Desktop Lock!"),
		      _("The Desktop Lock mechanism is not complitely working yet.\n"
		        "It is just a simple development version of Desktop Locking.\n"
		        "To return to E, just hit Enter"));

  zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
  {
    elock_wnd = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
    ecore_x_window_show(elock_wnd);
    e_grabinput_get(elock_wnd, 0, elock_wnd);

    x = zone->x + 100;
    y = zone->y + 100;
    w = zone->w - 200;
    h = zone->h - 200;

    elock_wnd_popup = e_popup_new(zone, x, y, w, h);
    evas_event_feed_mouse_move(elock_wnd_popup->evas, -1000000, -1000000,
			       ecore_x_current_time_get(), NULL);

    e_popup_layer_set(elock_wnd_popup, ELOCK_POPUP_LAYER);

    evas_event_freeze(elock_wnd_popup->evas);
    bg_object = edje_object_add(elock_wnd_popup->evas);

    // this option should be made as a user option.
    e_theme_edje_object_set(bg_object, "base/theme/background", "desktop/background");

    passwd_entry = edje_object_add(elock_wnd_popup->evas);
    e_theme_edje_object_set(passwd_entry, "base/theme/exebuf", "widgets/exebuf/main");
    edje_object_part_text_set(passwd_entry, "label", passwd);

    edje_object_part_swallow(bg_object, "passwd_entry", passwd_entry);
    evas_object_move(passwd_entry, x + 200, y + 200);
    evas_object_show(passwd_entry);


    e_popup_move_resize(elock_wnd_popup, x, y, w, h);
    evas_object_move(bg_object, 0, 0);
    evas_object_resize(bg_object, w, h);
    evas_object_show(bg_object);
    e_popup_edje_bg_object_set(elock_wnd_popup, bg_object);

    evas_event_thaw(elock_wnd_popup->evas);

    /* handlers */
    handlers = evas_list_append(handlers,
				ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
							_e_desklock_cb_key_down, NULL));
    handlers = evas_list_append(handlers, 
				ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
							_e_desklock_cb_mouse_down, NULL));
    handlers = evas_list_append(handlers,
				ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
							_e_desklock_cb_mouse_up, NULL));
    handlers = evas_list_append(handlers,
				ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
							_e_desklock_cb_mouse_wheel,
							NULL));

    //elock_wnd_idler = ecore_idler_add(_e_desklock_idler, NULL);

    e_popup_show(elock_wnd_popup);
  }

  /*
  // TODO: I think that creation of the elock_wnd can be moved into the e_main.c.
  // Actually this lock wnd can be created just once. And then used.
  managers = e_manager_list();

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

	      input_window = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
	      ecore_x_window_show(input_window);
	      //e_grabinput_get(input_window, 0, input_window);

	      x = zone->x + 20;
	      y = zone->y + 20 + ((zone->h - 20 - 20 - 20)/ 2);
	      w = zone->w - 20 - 20;
	      h = 20;

	      elock_wnd = e_popup_new(zone, 100, 100, 400, 300);
	      evas_event_feed_mouse_move(elock_wnd->evas, -1000000, -1000000,
					 ecore_x_current_time_get(), NULL);

	      e_popup_layer_set(elock_wnd, ELOCK_POPUP_LAYER);

	      evas_event_freeze(elock_wnd->evas);
	      eo = edje_object_add(elock_wnd->evas);

	      evas_object_move(eo, 0, 0);
	      evas_object_resize(eo, 400, 300);

	      // this option should be made as a user option.
	      e_theme_edje_object_set(eo, "base/theme/background", "desktop/background");
	      evas_object_show(eo);

	      e_popup_edje_bg_object_set(elock_wnd, eo);

	      evas_event_thaw(elock_wnd->evas);

	      handlers = evas_list_append(handlers,
					  ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
								  _e_desklock_cb_key_down, NULL));
	      handlers = evas_list_append(handlers, 
					  ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
								  _e_desklock_cb_mouse_down, NULL));
	      handlers = evas_list_append(handlers,
					  ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
								  _e_desklock_cb_mouse_up, NULL));
	      handlers = evas_list_append(handlers,
					  ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
								  _e_desklock_cb_mouse_wheel,
								  NULL));

	      //elock_wnd_idler = ecore_idler_add(_e_desklock_idler, NULL);

	      e_popup_show(elock_wnd);
	    }
	}
    }*/


   /*  e_error_dialog_show(_("Enlightenment IPC setup error!"),
			 _("Enlightenment cannot set up the IPC socket.\n"
			   "It likely is already in use by an existing copy of Enlightenment.\n"
			   "Double check to see if Enlightenment is not already on this display,\n"
			   "but if that fails try deleting all files in ~/.ecore/enlightenment-*\n"
			   "and try running again."));*/

  return 1;
}

EAPI void
e_desklock_hide(void)
{
  if (!elock_wnd_popup) return;

  evas_event_freeze(elock_wnd_popup->evas);

  e_popup_hide(elock_wnd_popup);
  evas_object_del(bg_object);
  bg_object = NULL;

  evas_event_thaw(elock_wnd_popup->evas);
  e_object_del(E_OBJECT(elock_wnd_popup));
  elock_wnd_popup = NULL;

  while (handlers)
    {
      ecore_event_handler_del(handlers->data);
      handlers = evas_list_remove_list(handlers, handlers);
    }
  ecore_x_window_del(elock_wnd);
  e_grabinput_release(elock_wnd, elock_wnd);
  elock_wnd = 0;
}

static int
_e_desklock_cb_key_down(void *data, int type, void *event)
{
  Ecore_X_Event_Key_Down *ev;

  ev = event;
  if (ev->win != elock_wnd) return 1;

  if (!strcmp(ev->keysymbol, "Escape"))
    ;
  else if (!strcmp(ev->keysymbol, "KP_Enter"))
    {
      // here we have to go to auth
      e_desklock_hide(); // Actually, escape MUST be ignored.
      ;
    }
  else if (!strcmp(ev->keysymbol, "Return"))
    {
      // here we have to go to auth
      e_desklock_hide(); // Actually, escape MUST be ignored.
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
	  if ((strlen(passwd) < (PASSWD_LEN - strlen(ev->key_compose))))
	    {
	      strcat(passwd, ev->key_compose);
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

  for (ii = 0; ii < strlen(passwd); ii ++)
    {
      strcat(passwd_hidden, "*");
    }
  edje_object_part_text_set(passwd_entry, "label", passwd_hidden);
  //edje_object_part_text_set(passwd_entry, "label", passwd);
  return;
}

static void
_e_desklock_backspace()
{
  int len, val, pos;

  len = strlen(passwd);
  if (len > 0)
    {
      pos = evas_string_char_prev_get(passwd, len, &val);
      if ((pos < len) && (pos >= 0))
	{
	  passwd[pos] = 0;
	  _e_desklock_passwd_update();
	}
    }
}
static void
_e_desklock_delete()
{
  _e_desklock_backspace();
}
