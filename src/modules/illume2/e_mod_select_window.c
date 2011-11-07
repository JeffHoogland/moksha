#include "e_illume_private.h"
#include "e_mod_select_window.h"

/* local function prototypes */
static void *_e_mod_illume_config_select_window_create_data(E_Config_Dialog *cfd);
static void _e_mod_illume_config_select_window_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_e_mod_illume_config_select_window_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _e_mod_illume_config_select_window_list_changed(void *data);
static Eina_Bool _e_mod_illume_config_select_window_change_timeout(void *data);
static int _e_mod_illume_config_select_window_match(E_Border *bd);

/* local variables */
E_Illume_Select_Window_Type stype;
Ecore_Timer *_sw_change_timer = NULL;

/* public functions */
void 
e_mod_illume_config_select_window(E_Illume_Select_Window_Type type) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_select_window")) return;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   stype = type;
   v->create_cfdata = _e_mod_illume_config_select_window_create_data;
   v->free_cfdata = _e_mod_illume_config_select_window_free_data;
   v->basic.create_widgets = _e_mod_illume_config_select_window_create;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()), 
                             _("Select Home Window"), "E", 
                             "_config_illume_select_window", 
                             "enlightenment/windows", 0, v, NULL);
   if (!cfd) return;
   e_dialog_resizable_set(cfd->dia, 1);
}

static void *
_e_mod_illume_config_select_window_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   return NULL;
}

static void 
_e_mod_illume_config_select_window_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   if (_sw_change_timer) ecore_timer_del(_sw_change_timer);
   _sw_change_timer = NULL;
}

static Evas_Object *
_e_mod_illume_config_select_window_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   Evas_Object *list, *ow;
   Eina_List *bds, *l;
   E_Zone *zone;
   int i, sel = -1;

   zone = e_util_zone_current_get(e_manager_current_get());
   list = e_widget_list_add(evas, 0, 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_selector_set(ow, 1);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(ow);
   e_widget_ilist_clear(ow);
   e_widget_ilist_go(ow);

   if ((bds = e_border_client_list()))
     {
        for (i = 0, l = bds; l; l = l->next, i++) 
          {
             E_Border *bd;
             const char *name;

             if (!(bd = l->data)) continue;
             if (bd->zone != zone) continue;
             if (e_object_is_del(E_OBJECT(bd))) continue;
             if (!(name = e_border_name_get(bd))) continue;
             if (_e_mod_illume_config_select_window_match(bd)) sel = i;
             e_widget_ilist_append(ow, NULL, name, 
                                   _e_mod_illume_config_select_window_list_changed, 
                                   bd, name);
          }
     }

   e_widget_size_min_set(ow, 100, 200);
   e_widget_ilist_go(ow);
   if (sel >= 0) e_widget_ilist_selected_set(ow, sel);
   e_widget_ilist_thaw(ow);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_list_object_append(list, ow, 1, 0, 0.0);
   return list;
}

static void 
_e_mod_illume_config_select_window_list_changed(void *data) 
{
   E_Border *bd;
   Ecore_X_Window_Type wtype;
   char *title, *name, *class;

   if (!(bd = data)) return;
   title = ecore_x_icccm_title_get(bd->client.win);
   ecore_x_icccm_name_class_get(bd->client.win, &name, &class);
   ecore_x_netwm_window_type_get(bd->client.win, &wtype);

   switch (stype) 
     {
      case E_ILLUME_SELECT_WINDOW_TYPE_HOME:
        eina_stringshare_replace(&_e_illume_cfg->policy.home.title, title);
        eina_stringshare_replace(&_e_illume_cfg->policy.home.class, class);
        eina_stringshare_replace(&_e_illume_cfg->policy.home.name, name);
        break;
      case E_ILLUME_SELECT_WINDOW_TYPE_VKBD:
        eina_stringshare_replace(&_e_illume_cfg->policy.vkbd.title, title);
        eina_stringshare_replace(&_e_illume_cfg->policy.vkbd.class, class);
        eina_stringshare_replace(&_e_illume_cfg->policy.vkbd.name, name);
        break;
      case E_ILLUME_SELECT_WINDOW_TYPE_SOFTKEY:
        eina_stringshare_replace(&_e_illume_cfg->policy.softkey.title, title);
        eina_stringshare_replace(&_e_illume_cfg->policy.softkey.class, class);
        eina_stringshare_replace(&_e_illume_cfg->policy.softkey.name, name);
        break;
      case E_ILLUME_SELECT_WINDOW_TYPE_INDICATOR:
        eina_stringshare_replace(&_e_illume_cfg->policy.indicator.title, title);
        eina_stringshare_replace(&_e_illume_cfg->policy.indicator.class, class);
        eina_stringshare_replace(&_e_illume_cfg->policy.indicator.name, name);
        break;
     }

   if (title) free(title);
   if (name) free(name);
   if (class) free(class);

   if (_sw_change_timer) ecore_timer_del(_sw_change_timer);
   _sw_change_timer = 
     ecore_timer_add(0.5, _e_mod_illume_config_select_window_change_timeout, data);
}

static Eina_Bool
_e_mod_illume_config_select_window_change_timeout(__UNUSED__ void *data)
{
   e_config_save_queue();
   _sw_change_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static int 
_e_mod_illume_config_select_window_match(E_Border *bd) 
{
   Ecore_X_Window_Type wtype;
   char *title, *name, *class;
   int match = 0;

   if (!bd) return 0;
   title = ecore_x_icccm_title_get(bd->client.win);
   ecore_x_icccm_name_class_get(bd->client.win, &name, &class);
   ecore_x_netwm_window_type_get(bd->client.win, &wtype);

   switch (stype) 
     {
      case E_ILLUME_SELECT_WINDOW_TYPE_HOME:
        if (_e_illume_cfg->policy.home.match.title) 
          {
             if ((title) && (!strcmp(title, _e_illume_cfg->policy.home.title)))
               match = 1;
          }
        if (_e_illume_cfg->policy.home.match.name) 
          {
             if ((name) && (!strcmp(name, _e_illume_cfg->policy.home.name)))
               match = 1;
          }
        if (_e_illume_cfg->policy.home.match.class) 
          {
             if ((class) && (!strcmp(class, _e_illume_cfg->policy.home.class)))
               match = 1;
          }
        break;
      case E_ILLUME_SELECT_WINDOW_TYPE_VKBD:
        if (_e_illume_cfg->policy.vkbd.match.title) 
          {
             if ((title) && (!strcmp(title, _e_illume_cfg->policy.vkbd.title)))
               match = 1;
          }
        if (_e_illume_cfg->policy.vkbd.match.name) 
          {
             if ((name) && (!strcmp(name, _e_illume_cfg->policy.vkbd.name)))
               match = 1;
          }
        if (_e_illume_cfg->policy.vkbd.match.class) 
          {
             if ((class) && (!strcmp(class, _e_illume_cfg->policy.vkbd.class)))
               match = 1;
          }
        break;
      case E_ILLUME_SELECT_WINDOW_TYPE_SOFTKEY:
        if (_e_illume_cfg->policy.softkey.match.title) 
          {
             if ((title) && (!strcmp(title, _e_illume_cfg->policy.softkey.title)))
               match = 1;
          }
        if (_e_illume_cfg->policy.softkey.match.name) 
          {
             if ((name) && (!strcmp(name, _e_illume_cfg->policy.softkey.name)))
               match = 1;
          }
        if (_e_illume_cfg->policy.softkey.match.class) 
          {
             if ((class) && (!strcmp(class, _e_illume_cfg->policy.softkey.class)))
               match = 1;
          }
        break;
      case E_ILLUME_SELECT_WINDOW_TYPE_INDICATOR:
        if (_e_illume_cfg->policy.indicator.match.title) 
          {
             if ((title) && (!strcmp(title, _e_illume_cfg->policy.indicator.title)))
               match = 1;
          }
        if (_e_illume_cfg->policy.indicator.match.name) 
          {
             if ((name) && (!strcmp(name, _e_illume_cfg->policy.indicator.name)))
               match = 1;
          }
        if (_e_illume_cfg->policy.indicator.match.class) 
          {
             if ((class) && (!strcmp(class, _e_illume_cfg->policy.indicator.class)))
               match = 1;
          }
        break;
     }

   if (title) free(title);
   if (name) free(name);
   if (class) free(class);

   return match;
}
