#include "E_Illume.h"
#include "e_mod_policy.h"

/* local function prototypes */
static void *_il_config_policy_create(E_Config_Dialog *cfd);
static void _il_config_policy_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_config_policy_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_config_policy_list_changed(void *data);
static int _il_config_policy_change_timeout(void *data);
static Evas_Object *_il_config_policy_settings_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* local variables */
Ecore_Timer *_policy_change_timer = NULL;
const char *_policy_name = NULL;

void 
il_config_policy_show(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_policy")) return;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata = _il_config_policy_create;
   v->free_cfdata = _il_config_policy_free;
   v->basic.create_widgets = _il_config_policy_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(con, _("Policy"), "E", 
                             "_config_illume_policy", 
                             "enlightenment/policy", 0, v, NULL);
   if (!cfd) return;
   e_dialog_resizable_set(cfd->dia, 1);
}

/* local functions */
static void *
_il_config_policy_create(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_config_policy_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (_policy_change_timer) ecore_timer_del(_policy_change_timer);
   _policy_change_timer = NULL;
}

static Evas_Object *
_il_config_policy_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *ow;
   Eina_List *policies;
   E_Illume_Layout_Policy *p;
   int i = 0, sel = 0;

   list = e_widget_list_add(evas, 0, 0);
   ow = e_widget_ilist_add(evas, 24, 24, &(_policy_name));
   e_widget_ilist_selector_set(ow, 1);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(ow);
   e_widget_ilist_clear(ow);
   e_widget_ilist_go(ow);

   policies = e_illume_layout_policies_get();
   if (policies) 
     {
        EINA_LIST_FREE(policies, p) 
          {
             e_widget_ilist_append(ow, NULL, strdup(p->api->label), 
                                   _il_config_policy_list_changed, NULL, 
                                   strdup(p->api->name));

             if ((p) && (il_cfg->policy.name) && 
                 (!strcmp(il_cfg->policy.name, p->api->name))) 
               sel = i;

             if (p) 
               {
                  e_object_del(E_OBJECT(p));
                  p = NULL;
               }
             i++;
          }
     }

   e_widget_size_min_set(ow, 100, 200);
   e_widget_ilist_go(ow);
   e_widget_ilist_selected_set(ow, sel);
   e_widget_ilist_thaw(ow);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_list_object_append(list, ow, 1, 0, 0.0);
   return list;
}

static void 
_il_config_policy_list_changed(void *data) 
{
   if (il_cfg->policy.name) eina_stringshare_del(il_cfg->policy.name);
   if (_policy_name) il_cfg->policy.name = eina_stringshare_add(_policy_name);
   if (_policy_change_timer) ecore_timer_del(_policy_change_timer);
   _policy_change_timer = 
     ecore_timer_add(0.5, _il_config_policy_change_timeout, data);
}

static int 
_il_config_policy_change_timeout(void *data) 
{
   e_config_save_queue();
   _policy_change_timer = NULL;
   ecore_event_add(E_ILLUME_EVENT_POLICY_CHANGE, NULL, NULL, NULL);
   return 0;
}
