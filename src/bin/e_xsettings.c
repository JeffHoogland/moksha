/* TODO
   check http://www.pvv.org/~mariusbu/proposal.html
   for advances in cross toolkit settings */

#include <e.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>            /* For CARD16 */

#define RETRY_TIMEOUT 2.0

#define SETTING_TYPE_INT	0
#define SETTING_TYPE_STRING	1
#define SETTING_TYPE_COLOR	2

#define OFFSET_ADD(n) ((n + 4 - 1) & (~(4 - 1)))

#define DBG printf

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

static void _e_xsettings_apply(Settings_Manager *sm);

static Ecore_X_Atom _atom_manager = 0;
static Ecore_X_Atom _atom_xsettings = 0;
static Eina_List *managers = NULL;
static Eina_List *handlers = NULL;
static Eina_List *settings = NULL;
static Eina_Bool running = EINA_FALSE;
static const char _setting_icon_theme_name[] = "Net/IconThemeName";
static const char _setting_theme_name[]      = "Net/ThemeName";
static const char _setting_font_name[]       = "Gtk/FontName";
static const char _setting_xft_dpi[]         = "Xft/DPI";

static Ecore_X_Atom
_e_xsettings_atom_screen_get(int screen_num)
{
   char buf[32];
   snprintf(buf, sizeof(buf), "_XSETTINGS_S%d", screen_num);
   return ecore_x_atom_get(buf);
}

static Eina_Bool
_e_xsettings_selection_owner_set(Settings_Manager *sm)
{
   Ecore_X_Atom atom;
   Ecore_X_Window cur_selection;
   Eina_Bool ret;

   atom = _e_xsettings_atom_screen_get(sm->man->num);
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
_e_xsettings_deactivate(Settings_Manager *sm)
{
   Ecore_X_Window old;

   if (sm->selection == 0) return;

   old = sm->selection;
   sm->selection = 0;
   _e_xsettings_selection_owner_set(sm);
   ecore_x_sync();
   ecore_x_window_free(old);
}

static Eina_Bool
_e_xsettings_activate(Settings_Manager *sm)
{
   Ecore_X_Atom atom;
   Ecore_X_Window old_win;

   if (sm->selection != 0) return 1;

   atom = _e_xsettings_atom_screen_get(sm->man->num);
   old_win = ecore_x_selection_owner_get(atom);
   if (old_win != 0) return 0;

   sm->selection = ecore_x_window_input_new(0, 0, 0, 1, 1);
   if (sm->selection == 0)
     return 0;

   if (!_e_xsettings_selection_owner_set(sm))
     {
        ecore_x_window_free(sm->selection);
        sm->selection = 0;
        return 0;
     }

   ecore_x_client_message32_send(e_manager_current_get()->root, _atom_manager,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 ecore_x_current_time_get(), atom,
                                 sm->selection, 0, 0);

   _e_xsettings_apply(sm);

   return 1;
}

static Eina_Bool
_e_xsettings_activate_retry(void *data)
{
   Settings_Manager *sm = data;
   Eina_Bool ret;

   fputs("XSETTINGS: reactivate...\n", stderr);
   ret = _e_xsettings_activate(sm);
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
_e_xsettings_retry(Settings_Manager *sm)
{
   if (sm->timer_retry) return;
   sm->timer_retry = ecore_timer_add
     (RETRY_TIMEOUT, _e_xsettings_activate_retry, sm);
}

static void
_e_xsettings_string_set(const char *name, const char *value)
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
        DBG("remove %s\n", name);
        eina_stringshare_del(name);
        eina_stringshare_del(s->name);
        eina_stringshare_del(s->s.value);
        settings = eina_list_remove(settings, s);
        E_FREE(s);
        return;
     }
   if (s)
     {
        DBG("update %s %s\n", name, value);
        eina_stringshare_del(name);
        eina_stringshare_replace(&s->s.value, value);
     }
   else
     {
        DBG("add %s %s\n", name, value);
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
_e_xsettings_int_set(const char *name, int value, Eina_Bool set)
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
   if (!set)
     {
        if (!s) return;
        DBG("remove %s\n", name);
        eina_stringshare_del(name);
        eina_stringshare_del(s->name);
        settings = eina_list_remove(settings, s);
        E_FREE(s);
        return;
     }
   if (s)
     {
        DBG("update %s %d\n", name, value);
        eina_stringshare_del(name);
        s->i.value = value;
     }
   else
     {
        DBG("add %s %d\n", name, value);
        s = E_NEW(Setting, 1);
        s->type = SETTING_TYPE_INT;
        s->name = name;
        s->i.value = value;
        settings = eina_list_append(settings, s);
     }

   // type + pad + name-len + last-change-serial + value
   s->length = 12;
   s->length += OFFSET_ADD(strlen(name));
}

static unsigned char *
_e_xsettings_copy(unsigned char *buffer, Setting *s)
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
_e_xsettings_apply(Settings_Manager *sm)
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
     pos = _e_xsettings_copy(pos, s);

   ecore_x_window_prop_property_set(sm->selection,
                                    _atom_xsettings,
                                    _atom_xsettings,
                                    8, data, len);
   free(data);
}

static void
_e_xsettings_update(void)
{
   Settings_Manager *sm;
   Eina_List *l;

   EINA_LIST_FOREACH(managers, l, sm)
     if (sm->selection) _e_xsettings_apply(sm);
}

static Eina_Bool
_cb_icon_theme_change(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Config_Icon_Theme *ev = event;

   if (e_config->xsettings.match_e17_icon_theme)
     {
        _e_xsettings_string_set(_setting_icon_theme_name,
                              ev->icon_theme);
        _e_xsettings_update();
     }

   return ECORE_CALLBACK_PASS_ON;
}


static void
_e_xsettings_icon_theme_set(void)
{
   if (e_config->xsettings.match_e17_icon_theme)
     {
        _e_xsettings_string_set(_setting_icon_theme_name,
                                e_config->icon_theme);
        return;
     }

   if (e_config->xsettings.net_icon_theme_name)
     {
        _e_xsettings_string_set(_setting_icon_theme_name,
                              e_config->xsettings.net_icon_theme_name);
        return;
     }

   _e_xsettings_string_set(_setting_icon_theme_name, NULL);
}

static void
_e_xsettings_theme_set(void)
{
   if (e_config->xsettings.match_e17_theme)
     {
        E_Config_Theme *ct;
        if ((ct = e_theme_config_get("theme")))
          {
             char *theme;

             if ((theme = edje_file_data_get(ct->file, "gtk-theme")))
               {
                  char buf[4096], *dir;
                  Eina_List *xdg_dirs, *l;

                  e_user_homedir_snprintf(buf, sizeof(buf), ".themes/%s", theme);
                  if (ecore_file_exists(buf))
                    {
                       _e_xsettings_string_set(_setting_theme_name, theme);
                       return;
                    }

                  xdg_dirs = efreet_data_dirs_get();
                  EINA_LIST_FOREACH(xdg_dirs, l, dir)
                    {
                       snprintf(buf, sizeof(buf), "%s/themes/%s", dir, theme);
                       if (ecore_file_exists(buf))
                         {
                            _e_xsettings_string_set(_setting_theme_name, theme);
                            return;
                         }
                    }
               }
          }
     }

   if (e_config->xsettings.net_theme_name)
     {
        _e_xsettings_string_set(_setting_theme_name,
                              e_config->xsettings.net_theme_name);
        return;
     }

   _e_xsettings_string_set(_setting_theme_name, NULL);
}

static void
_e_xsettings_font_set(void)
{
   E_Font_Default *efd;
   E_Font_Properties *efp;

   efd = e_font_default_get("application");

   if (efd && efd->font)
     {
        efp = e_font_fontconfig_name_parse(efd->font);
        if (efp->name)
          {
             int size = efd->size;
             char buf[128];
             /* TODO better way to convert evas font sizes? */
             if (size < 0) size /= -10;
             if (size < 5) size = 5;
             if (size > 25) size = 25;

             snprintf(buf, sizeof(buf), "%s %d", efp->name, size);
             _e_xsettings_string_set(_setting_font_name, buf);
             e_font_properties_free(efp);
             return;
          }

        e_font_properties_free(efp);
     }

   _e_xsettings_string_set(_setting_font_name, NULL);
}

static void
_e_xsettings_xft_set(void)
{

   if (e_config->scale.use_dpi)
     _e_xsettings_int_set(_setting_xft_dpi, e_config->scale.base_dpi, EINA_TRUE);
   else
     _e_xsettings_int_set(_setting_xft_dpi, 0, EINA_FALSE);

}

static void
_e_xsettings_start(void)
{
   Eina_List *l;
   E_Manager *man;

   if (running) return;

   _e_xsettings_theme_set();
   _e_xsettings_icon_theme_set();
   _e_xsettings_font_set();

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        Settings_Manager *sm = E_NEW(Settings_Manager, 1);
        sm->man = man;

        if (!_e_xsettings_activate(sm))
          _e_xsettings_retry(sm);

        managers = eina_list_append(managers, sm);
     }

   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_CONFIG_ICON_THEME,
                                                                 _cb_icon_theme_change, NULL));

   running = EINA_TRUE;
}

static void
_e_xsettings_stop(void)
{
   Settings_Manager *sm;
   Ecore_Event_Handler *h;
   Setting *s;

   if (!running) return;

   EINA_LIST_FREE(managers, sm)
     {
        if (sm->timer_retry)
          ecore_timer_del(sm->timer_retry);

        _e_xsettings_deactivate(sm);

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

   running = EINA_FALSE;
}

EINTERN int
e_xsettings_init(void)
{
   _atom_manager = ecore_x_atom_get("MANAGER");
   _atom_xsettings = ecore_x_atom_get("_XSETTINGS_SETTINGS");

   if (e_config->xsettings.enabled)
     _e_xsettings_start();

   return 1;
}

EINTERN int
e_xsettings_shutdown(void)
{
   _e_xsettings_stop();

   return 1;
}

EAPI void
e_xsettings_config_update(void)
{
   if (!e_config->xsettings.enabled)
     {
        _e_xsettings_stop();
        return;
     }

   if (!running)
     {
        _e_xsettings_start();
     }
   else
     {
        _e_xsettings_theme_set();
        _e_xsettings_icon_theme_set();
        _e_xsettings_font_set();
        _e_xsettings_update();
     }
}
