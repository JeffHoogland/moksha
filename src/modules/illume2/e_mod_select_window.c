#include "e.h"
#include "e_mod_main.h"
#include "e_mod_select_window.h"
#include "e_mod_config.h"

/* local function prototypes */
static void *_il_config_select_window_create_data(E_Config_Dialog *cfd);
static void _il_config_select_window_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_config_select_window_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_config_select_window_list_changed(void *data);
static int _il_config_select_window_change_timeout(void *data);
static int _il_config_select_window_match(E_Border *bd);

/* local variables */
Il_Select_Window_Type stype;
Ecore_Timer *_change_timer = NULL;

/* public functions */
void 
il_config_select_window(Il_Select_Window_Type type) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_select_window")) return;
   stype = type;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _il_config_select_window_create_data;
   v->free_cfdata = _il_config_select_window_free_data;
   v->basic.create_widgets = _il_config_select_window_create;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()), 
                             _("Select Home Window"), "E", 
                             "_config_illume_select_window", 
                             "enlightenment/windows", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

static void *
_il_config_select_window_create_data(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_config_select_window_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{

}

static Evas_Object *
_il_config_select_window_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *ow;
   Eina_List *bds, *l;
   int i = 0, sel = -1;

   list = e_widget_list_add(evas, 0, 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_selector_set(ow, 1);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(ow);
   e_widget_ilist_clear(ow);
   e_widget_ilist_go(ow);

   bds = e_border_client_list();
   for (i = 0, l = bds; l; l = l->next, i++) 
     {
        E_Border *bd;
        const char *name;

        if (!(bd = l->data)) continue;
        if (e_object_is_del(E_OBJECT(bd))) continue;
        if (_il_config_select_window_match(bd)) sel = i;
        name = e_border_name_get(bd);
        e_widget_ilist_append(ow, NULL, name, 
                              _il_config_select_window_list_changed, 
                              bd, name);
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
_il_config_select_window_list_changed(void *data) 
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
      case IL_SELECT_WINDOW_TYPE_HOME:
        if (il_cfg->policy.home.title)
          eina_stringshare_del(il_cfg->policy.home.title);
        if (title) il_cfg->policy.home.title = eina_stringshare_add(title);
        if (il_cfg->policy.home.class)
          eina_stringshare_del(il_cfg->policy.home.class);
        if (class) il_cfg->policy.home.class = eina_stringshare_add(class);
        if (il_cfg->policy.home.name)
          eina_stringshare_del(il_cfg->policy.home.name);
        if (name) il_cfg->policy.home.name = eina_stringshare_add(name);
        break;
      case IL_SELECT_WINDOW_TYPE_VKBD:
        if (il_cfg->policy.vkbd.title)
          eina_stringshare_del(il_cfg->policy.vkbd.title);
        if (title) il_cfg->policy.vkbd.title = eina_stringshare_add(title);
        if (il_cfg->policy.vkbd.class)
          eina_stringshare_del(il_cfg->policy.vkbd.class);
        if (class) il_cfg->policy.vkbd.class = eina_stringshare_add(class);
        if (il_cfg->policy.vkbd.name)
          eina_stringshare_del(il_cfg->policy.vkbd.name);
        if (name) il_cfg->policy.vkbd.name = eina_stringshare_add(name);
        break;
      case IL_SELECT_WINDOW_TYPE_SOFTKEY:
        if (il_cfg->policy.softkey.title)
          eina_stringshare_del(il_cfg->policy.softkey.title);
        if (title) il_cfg->policy.softkey.title = eina_stringshare_add(title);
        if (il_cfg->policy.softkey.class)
          eina_stringshare_del(il_cfg->policy.softkey.class);
        if (class) il_cfg->policy.softkey.class = eina_stringshare_add(class);
        if (il_cfg->policy.softkey.name)
          eina_stringshare_del(il_cfg->policy.softkey.name);
        if (name) il_cfg->policy.softkey.name = eina_stringshare_add(name);
        break;
      case IL_SELECT_WINDOW_TYPE_INDICATOR:
        if (il_cfg->policy.indicator.title)
          eina_stringshare_del(il_cfg->policy.indicator.title);
        if (title) il_cfg->policy.indicator.title = eina_stringshare_add(title);
        if (il_cfg->policy.indicator.class)
          eina_stringshare_del(il_cfg->policy.indicator.class);
        if (class) il_cfg->policy.indicator.class = eina_stringshare_add(class);
        if (il_cfg->policy.indicator.name)
          eina_stringshare_del(il_cfg->policy.indicator.name);
        if (name) il_cfg->policy.indicator.name = eina_stringshare_add(name);
        break;
     }

   if (title) free(title);
   if (name) free(name);
   if (class) free(class);

   if (_change_timer) ecore_timer_del(_change_timer);
   _change_timer = 
     ecore_timer_add(0.5, _il_config_select_window_change_timeout, data);
}

static int 
_il_config_select_window_change_timeout(void *data) 
{
   e_config_save_queue();
   _change_timer = NULL;
   return 0;
}

static int 
_il_config_select_window_match(E_Border *bd) 
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
      case IL_SELECT_WINDOW_TYPE_HOME:
        if (il_cfg->policy.home.match.title) 
          {
             if ((title) && (!strcmp(title, il_cfg->policy.home.title)))
               match = 1;
             break;
          }
        if (il_cfg->policy.home.match.name) 
          {
             if ((name) && (!strcmp(name, il_cfg->policy.home.name)))
               match = 1;
             break;
          }
        if (il_cfg->policy.home.match.class) 
          {
             if ((class) && (!strcmp(class, il_cfg->policy.home.class)))
               match = 1;
             break;
          }
        break;
      case IL_SELECT_WINDOW_TYPE_VKBD:
        if (il_cfg->policy.vkbd.match.title) 
          {
             if ((title) && (!strcmp(title, il_cfg->policy.vkbd.title)))
               match = 1;
             break;
          }
        if (il_cfg->policy.vkbd.match.name) 
          {
             if ((name) && (!strcmp(name, il_cfg->policy.vkbd.name)))
               match = 1;
             break;
          }
        if (il_cfg->policy.vkbd.match.class) 
          {
             if ((class) && (!strcmp(class, il_cfg->policy.vkbd.class)))
               match = 1;
             break;
          }
        break;
      case IL_SELECT_WINDOW_TYPE_SOFTKEY:
        if (il_cfg->policy.softkey.match.title) 
          {
             if ((title) && (!strcmp(title, il_cfg->policy.softkey.title)))
               match = 1;
             break;
          }
        if (il_cfg->policy.softkey.match.name) 
          {
             if ((name) && (!strcmp(name, il_cfg->policy.softkey.name)))
               match = 1;
             break;
          }
        if (il_cfg->policy.softkey.match.class) 
          {
             if ((class) && (!strcmp(class, il_cfg->policy.softkey.class)))
               match = 1;
             break;
          }
        break;
      case IL_SELECT_WINDOW_TYPE_INDICATOR:
        if (il_cfg->policy.indicator.match.title) 
          {
             if ((title) && (!strcmp(title, il_cfg->policy.indicator.title)))
               match = 1;
             break;
          }
        if (il_cfg->policy.indicator.match.name) 
          {
             if ((name) && (!strcmp(name, il_cfg->policy.indicator.name)))
               match = 1;
             break;
          }
        if (il_cfg->policy.indicator.match.class) 
          {
             if ((class) && (!strcmp(class, il_cfg->policy.indicator.class)))
               match = 1;
             break;
          }
        break;
     }

   if (title) free(title);
   if (name) free(name);
   if (class) free(class);

   return match;
}
