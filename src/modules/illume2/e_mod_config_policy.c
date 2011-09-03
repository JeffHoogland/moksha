#include "e_illume_private.h"
#include "e_mod_config_policy.h"

/* local function prototypes */
static void *_e_mod_illume_config_policy_create(E_Config_Dialog *cfd);
static void _e_mod_illume_config_policy_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_e_mod_illume_config_policy_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _e_mod_illume_config_policy_list_changed(void *data);
static Eina_Bool _e_mod_illume_config_policy_change_timeout(void *data);
static Eina_List *_e_mod_illume_config_policy_policies_get(void);
static void _e_mod_illume_config_policy_policy_free(E_Illume_Policy *p);

/* local variables */
Ecore_Timer *_policy_change_timer = NULL;
const char *_policy_name = NULL;

void 
e_mod_illume_config_policy_show(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "illume/policy")) return;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata = _e_mod_illume_config_policy_create;
   v->free_cfdata = _e_mod_illume_config_policy_free;
   v->basic.create_widgets = _e_mod_illume_config_policy_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(con, _("Policy"), "E", "illume/policy", 
                             "enlightenment/policy", 0, v, NULL);
   if (!cfd) return;
   e_dialog_resizable_set(cfd->dia, 1);
}

/* local functions */
static void *
_e_mod_illume_config_policy_create(E_Config_Dialog *cfd __UNUSED__) 
{
   return NULL;
}

static void 
_e_mod_illume_config_policy_free(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   if (_policy_change_timer) ecore_timer_del(_policy_change_timer);
   _policy_change_timer = NULL;
}

static Evas_Object *
_e_mod_illume_config_policy_ui(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   Evas_Object *list, *ow;
   Eina_List *policies;
   E_Illume_Policy *p;
   int i = 0, sel = 0;

   list = e_widget_list_add(evas, 0, 0);
   ow = e_widget_ilist_add(evas, 24, 24, &(_policy_name));
   e_widget_ilist_selector_set(ow, 1);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(ow);
   e_widget_ilist_clear(ow);
   e_widget_ilist_go(ow);

   policies = _e_mod_illume_config_policy_policies_get();
   if (policies) 
     {
        EINA_LIST_FREE(policies, p) 
          {
             e_widget_ilist_append(ow, NULL, strdup(p->api->label), 
                                   _e_mod_illume_config_policy_list_changed, NULL, 
                                   strdup(p->api->name));

             if ((p) && (_e_illume_cfg->policy.name) && 
                 (!strcmp(_e_illume_cfg->policy.name, p->api->name))) 
               sel = i;

             if (p) e_object_del(E_OBJECT(p));
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
_e_mod_illume_config_policy_list_changed(void *data) 
{
   if (_e_illume_cfg->policy.name) 
     eina_stringshare_del(_e_illume_cfg->policy.name);
   if (_policy_name) 
     _e_illume_cfg->policy.name = eina_stringshare_add(_policy_name);
   if (_policy_change_timer) ecore_timer_del(_policy_change_timer);
   _policy_change_timer = 
     ecore_timer_add(0.5, _e_mod_illume_config_policy_change_timeout, data);
}

static Eina_Bool
_e_mod_illume_config_policy_change_timeout(void *data __UNUSED__) 
{
   e_config_save_queue();
   _policy_change_timer = NULL;
   ecore_event_add(E_ILLUME_POLICY_EVENT_CHANGE, NULL, NULL, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_List *
_e_mod_illume_config_policy_policies_get(void) 
{
   Eina_List *l = NULL, *files;
   char dir[PATH_MAX], *file;

   snprintf(dir, sizeof(dir), "%s/policies", _e_illume_mod_dir);

   if (!(files = ecore_file_ls(dir))) return NULL;

   EINA_LIST_FREE(files, file)
     {
        E_Illume_Policy *p;

        if (!strstr(file, ".so")) continue;
        snprintf(dir, sizeof(dir),"%s/policies/%s", _e_illume_mod_dir, file);

        p = E_OBJECT_ALLOC(E_Illume_Policy, E_ILLUME_POLICY_TYPE,
                         _e_mod_illume_config_policy_policy_free);
        if (!p) continue;

        p->handle = dlopen(dir, RTLD_NOW | RTLD_GLOBAL);
        if (!p->handle)
          {
             e_object_del(E_OBJECT(p));
             continue;
          }
        p->api = dlsym(p->handle, "e_illume_policy_api");
        if (!p->api)
          {
             e_object_del(E_OBJECT(p));
             continue;
          }
        if (p->api->version < E_ILLUME_POLICY_API_VERSION)
          {
             e_object_del(E_OBJECT(p));
             continue;
          }
        if (file) free(file);
        l = eina_list_append(l, p);
     }

   return l;
}

static void 
_e_mod_illume_config_policy_policy_free(E_Illume_Policy *p) 
{
   p->api = NULL;

   if (p->handle) dlclose(p->handle);
   p->handle = NULL;

   E_FREE(p);
}
