#include <e.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>            /* For CARD16 */

#define RETRY_TIMEOUT 2.0

#define SETTING_TYPE_INT	0
#define SETTING_TYPE_STRING	1
#define SETTING_TYPE_COLOR	2

#define OFFSET_ADD(n) ((n + 4 - 1) & (~(4 - 1)))

typedef struct _Settings_Manger Settings_Manager;
typedef struct _Setting Setting;

struct _Settings_Manger
{
   E_Manager *man;
   Ecore_X_Window selection;
   Ecore_Timer *timer_retry;
   unsigned long serial;
   Ecore_X_Atom _atom_xsettings_screen;
};

struct _Setting
{
  unsigned short type;

  const char *name;

  struct { const char *value; } s;
  struct { int value; } i;
  struct { unsigned short red, green, blue, alpha; } c;

  unsigned long length;
  unsigned long last_change;
};

static void _xsettings_apply(Settings_Manager *sm);

static Ecore_X_Atom _atom_manager = 0;
static Ecore_X_Atom _atom_xsettings = 0;
static Eina_List *managers = NULL;
static Eina_List *handlers = NULL;
static Eina_List *settings = NULL;

static const char _setting_icon_theme_name[] = "Net/IconThemeName";
static const char _setting_theme_name[]	     = "Net/ThemeName";

static Ecore_X_Atom
_xsettings_atom_screen_get(int screen_num)
{
   char buf[32];
   snprintf(buf, sizeof(buf), "_XSETTINGS_S%d", screen_num);
   return ecore_x_atom_get(buf);
}

static Eina_Bool
_xsettings_selection_owner_set(Settings_Manager *sm)
{
   Ecore_X_Atom atom;
   Ecore_X_Window cur_selection;
   Eina_Bool ret;

   atom = _xsettings_atom_screen_get(sm->man->num);
   ecore_x_selection_owner_set(sm->selection, atom, ecore_x_current_time_get());
   ecore_x_sync();
   cur_selection = ecore_x_selection_owner_get(atom);

   ret = (cur_selection == sm->selection);
   if (!ret)
     fprintf(stderr, "XSETTINGS: tried to set selection to %#x, but got %#x\n",
	     sm->selection, cur_selection);

   return ret;
}

static void
_xsettings_deactivate(Settings_Manager *sm)
{
   Ecore_X_Window old;

   if (sm->selection == 0) return;

   old = sm->selection;
   sm->selection = 0;
   _xsettings_selection_owner_set(sm);
   ecore_x_sync();
   ecore_x_window_free(old);
}

static Eina_Bool
_xsettings_activate(Settings_Manager *sm)
{
   unsigned int visual;
   Ecore_X_Atom atom;
   Ecore_X_Window old_win;
   Ecore_X_Window_Attributes attr;

   if (sm->selection != 0) return 1;

   atom = _xsettings_atom_screen_get(sm->man->num);
   old_win = ecore_x_selection_owner_get(atom);
   if (old_win != 0) return 0;

   sm->selection = ecore_x_window_input_new(0, 0, 0, 1, 1);
   if (sm->selection == 0)
     return 0;

   if (!_xsettings_selection_owner_set(sm))
     {
	ecore_x_window_free(sm->selection);
	sm->selection = 0;
	return 0;
     }

   ecore_x_client_message32_send(e_manager_current_get()->root, _atom_manager,
				 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
				 ecore_x_current_time_get(), atom,
				 sm->selection, 0, 0);

   _xsettings_apply(sm);

   return 1;
}

static Eina_Bool
_xsettings_activate_retry(void *data)
{
   Settings_Manager *sm = data;
   Eina_Bool ret;

   fputs("XSETTINGS: reactivate...\n", stderr);
   ret = _xsettings_activate(sm);
   if (ret)
     fputs("XSETTINGS: activate success!\n", stderr);
   else
     fprintf(stderr, "XSETTINGS: activate failure! retrying in %0.1f seconds\n",
	     RETRY_TIMEOUT);

   if (!ret)
     return ECORE_CALLBACK_RENEW;

   sm->timer_retry = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_xsettings_retry(Settings_Manager *sm)
{
   if (sm->timer_retry) return;
   sm->timer_retry = ecore_timer_add
     (RETRY_TIMEOUT, _xsettings_activate_retry, sm);
}

static void
_xsettings_string_set(const char *name, const char *value)
{
   Setting *s;
   Eina_List *l;

   if (!name) return;
   name = eina_stringshare_add(name);

   EINA_LIST_FOREACH(settings, l, s)
     {
	if (s->type != SETTING_TYPE_STRING) continue;
	if (s->name == name) break;
     }
   if (!value)
     {
	if (!s) return;
	printf("remove %s\n", name);
	eina_stringshare_del(name);
	eina_stringshare_del(s->name);
	eina_stringshare_del(s->s.value);
	settings = eina_list_remove(settings, s);
	E_FREE(s);
	return;
     }
   if (s)
     {
	printf("update %s %s\n", name, value);
	eina_stringshare_del(name);
	eina_stringshare_replace(&s->s.value, value);
     }
   else
     {
	printf("add %s %s\n", name, value);
	s = E_NEW(Setting, 1);
	s->type = SETTING_TYPE_STRING;
	s->name = name;
	s->s.value = eina_stringshare_add(value);
	settings = eina_list_append(settings, s);
     }
   
   /* type + pad + name-len + last-change-serial + str_len */
   s->length = 12;
   s->length += OFFSET_ADD(strlen(name));
   s->length += OFFSET_ADD(strlen(value));
   s->last_change = ecore_x_current_time_get();
}

static void
_xsettings_int_set(const char *name, int value)
{
   Setting *s;
   Eina_List *l;

   if (!name) return;
   name = eina_stringshare_add(name);

   EINA_LIST_FOREACH(settings, l, s)
     {
	if (s->type != SETTING_TYPE_INT) continue;
	if (s->name == name) break;
     }
   if (!value)
     {
	if (!s) return;
	printf("remove %s\n", name);
	eina_stringshare_del(name);
	eina_stringshare_del(s->name);
	settings = eina_list_remove(settings, s);
	E_FREE(s);
	return;
     }
   if (s)
     {
	printf("update %s %d\n", name, value);
	eina_stringshare_del(name);
	s->i.value = value;
     }
   else
     {
	printf("add %s %d\n", name, value);
	s = E_NEW(Setting, 1);
	s->type = SETTING_TYPE_INT;
	s->name = name;
	s->i.value = value;
	settings = eina_list_append(settings, s);
     }
   
   /* type + pad + name-len + last-change-serial + value */
   s->length = 12;
   s->length += OFFSET_ADD(strlen(name));
}

static unsigned char *
_xsettings_copy(unsigned char *buffer, Setting *s)
{
   size_t str_len;
   size_t len;

   buffer[0] = s->type;
   buffer[1] = 0;
   buffer += 2;

   str_len = strlen(s->name);
   *(CARD16 *)(buffer) = str_len;
   buffer += 2;

   memcpy(buffer, s->name, str_len);
   buffer += str_len;

   len = OFFSET_ADD(str_len) - str_len;
   memset(buffer, 0, len);
   buffer += len;

   *(CARD32 *)(buffer) = s->last_change;
   buffer += 4;

   switch (s->type)
     {
      case SETTING_TYPE_INT:
	 *(CARD32 *)(buffer) = s->i.value;
	 buffer += 4;
	 break;

      case SETTING_TYPE_STRING:
	 str_len = strlen (s->s.value);
	 *(CARD32 *)(buffer) = str_len;
	 buffer += 4;

	 memcpy(buffer, s->s.value, str_len);
	 buffer += str_len;

	 len = OFFSET_ADD(str_len) - str_len;
	 memset(buffer, 0, len);
	 buffer += len;
	 break;

      case SETTING_TYPE_COLOR:
	 *(CARD16 *)(buffer) = s->c.red;
	 *(CARD16 *)(buffer + 2) = s->c.green;
	 *(CARD16 *)(buffer + 4) = s->c.blue;
	 *(CARD16 *)(buffer + 6) = s->c.alpha;
	 buffer += 8;
	 break;
     }

   return buffer;
}

static void
_xsettings_apply(Settings_Manager *sm)
{
   unsigned char *data;
   unsigned char *pos;
   size_t len = 12;
   Setting *s;
   Eina_List *l;

   EINA_LIST_FOREACH(settings, l, s)
     len += s->length;

   pos = data = malloc(len);
   if (!data) return;
   
#if __BYTE_ORDER == __LITTLE_ENDIAN
   *pos = LSBFirst;
#else
   *pos = MSBFirst;
#endif

   pos += 4;
   *(CARD32*)pos = sm->serial++;
   pos += 4;
   *(CARD32*)pos = eina_list_count(settings);
   pos += 4;

   EINA_LIST_FOREACH(settings, l, s)
     pos = _xsettings_copy(pos, s);

   ecore_x_window_prop_property_set(sm->selection,
				    _atom_xsettings,
				    _atom_xsettings,
				    8, data, len);
   free(data);
}

static void
_xsettings_update(void)
{
   Settings_Manager *sm;
   Eina_List *l;

   EINA_LIST_FOREACH(managers, l, sm)
     if (sm->selection) _xsettings_apply(sm);
}

static Eina_Bool
_cb_icon_theme_change(void *data, int type __UNUSED__, void *event)
{
   E_Event_Config_Icon_Theme *ev = event;

   if (e_config->xsettings.match_e17_icon_theme)
     {
	_xsettings_string_set(_setting_icon_theme_name,
			      ev->icon_theme);
	_xsettings_update();
     }

   return ECORE_CALLBACK_PASS_ON;
}


static void
_xsettings_icon_theme_set(void)
{
   if (e_config->xsettings.match_e17_icon_theme)
     {
	_xsettings_string_set(_setting_icon_theme_name,
			      e_config->icon_theme);
	return;
     }
   else if (e_config->xsettings.icon_theme)
     {
	_xsettings_string_set(_setting_icon_theme_name,
			      e_config->xsettings.icon_theme);
	return;
     }

   _xsettings_string_set(_setting_icon_theme_name, NULL);
}

static void
_xsettings_theme_set(void)
{
   if (e_config->xsettings.match_e17_theme)
     {
	E_Config_Theme *ct;
	if ((ct = e_theme_config_get("theme")))
	  {
	     char *theme;
	     if ((theme = edje_file_data_get(ct->file, "gtk-theme")))
	       {
		  _xsettings_string_set(_setting_theme_name, theme);
		  return;
	       }
	  }
     }
   else if (e_config->xsettings.gtk_theme)
     {
	_xsettings_string_set(_setting_theme_name,
			      e_config->xsettings.gtk_theme);
	return;
     }

   _xsettings_string_set(_setting_theme_name, NULL);
}

EINTERN int
e_xsettings_init(void)
{
   Eina_List *l;
   E_Manager *man;

   _atom_manager = ecore_x_atom_get("MANAGER");
   _atom_xsettings = ecore_x_atom_get("_XSETTINGS_SETTINGS");

   _xsettings_theme_set();
   _xsettings_icon_theme_set();

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
	Settings_Manager *sm = E_NEW(Settings_Manager, 1);
	sm->man = man;

	if (!_xsettings_activate(sm))
	  _xsettings_retry(sm);

	managers = eina_list_append(managers, sm);
     }

   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_CONFIG_ICON_THEME,
								 _cb_icon_theme_change, NULL));
   return 1;
}

EINTERN int
e_xsettings_shutdown(void)
{
   Settings_Manager *sm;
   Ecore_Event_Handler *h;
   Setting *s;
   
   EINA_LIST_FREE(managers, sm)
     {
	if (sm->timer_retry)
	  ecore_timer_del(sm->timer_retry);

	_xsettings_deactivate(sm);

	E_FREE(sm);
     }

   EINA_LIST_FREE(settings, s)
     {
	if (s->name) eina_stringshare_del(s->name);
	if (s->s.value) eina_stringshare_del(s->s.value);
	E_FREE(s);
     }

   EINA_LIST_FREE(handlers, h)
     ecore_event_handler_del(h);

   return 1;
}

EAPI void
e_xsettings_config_update(void)
{
   _xsettings_theme_set();
   _xsettings_icon_theme_set();
   _xsettings_update();
}
