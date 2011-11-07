#include "e_illume_private.h"
#include "e_mod_policy.h"

/* local function prototypes */
static char *_e_mod_policy_find(void);
static int _e_mod_policy_load(char *file);
static void _e_mod_policy_handlers_add(void);
static void _e_mod_policy_hooks_add(void);
static void _e_mod_policy_cb_free(E_Illume_Policy *p);
static Eina_Bool _e_mod_policy_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_show(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_zone_move_resize(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_property(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_policy_change(void *data __UNUSED__, int type, void *event __UNUSED__);
static void _e_mod_policy_cb_hook_post_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_policy_cb_hook_post_assign(void *data __UNUSED__, void *data2);
static void _e_mod_policy_cb_hook_layout(void *data __UNUSED__, void *data2 __UNUSED__);

/* local variables */
static E_Illume_Policy *_policy = NULL;
static Eina_List *_policy_hdls = NULL, *_policy_hooks = NULL;

/* external variables */
int E_ILLUME_POLICY_EVENT_CHANGE = 0;

int 
e_mod_policy_init(void) 
{
   Eina_List *ml;
   E_Manager *man;
   char *file;

   /* try to find the policy specified in config */
   if (!(file = _e_mod_policy_find())) 
     {
        printf("Cannot find policy\n");
        return 0;
     }

   /* attempt to load policy */
   if (!_e_mod_policy_load(file)) 
     {
        /* loading policy failed, bail out */
        printf("Cannot load policy: %s\n", file);
        if (file) free(file);
        return 0;
     }

   /* create new event for policy changes */
   E_ILLUME_POLICY_EVENT_CHANGE = ecore_event_type_new();

   /* add our event handlers */
   _e_mod_policy_handlers_add();

   /* add our border hooks */
   _e_mod_policy_hooks_add();

   /* loop the root windows */
   EINA_LIST_FOREACH(e_manager_list(), ml, man) 
     {
        Eina_List *cl;
        E_Container *con;

        /* loop the containers */
        EINA_LIST_FOREACH(man->containers, cl, con) 
          {
             Eina_List *zl;
             E_Zone *zone;

             /* loop the zones */
             EINA_LIST_FOREACH(con->zones, zl, zone) 
               {
                  E_Illume_Config_Zone *cz;
                  Ecore_X_Illume_Mode mode = ECORE_X_ILLUME_MODE_SINGLE;

                  /* check for zone config */
                  if (!(cz = e_illume_zone_config_get(zone->id))) 
                    continue;

                  /* set mode on this zone */
                  if (cz->mode.dual == 0)
                    mode = ECORE_X_ILLUME_MODE_SINGLE;
                  else 
                    {
                       if ((cz->mode.dual == 1) && (cz->mode.side == 0)) 
                         mode = ECORE_X_ILLUME_MODE_DUAL_TOP;
                       else if ((cz->mode.dual == 1) && (cz->mode.side == 1))
                         mode = ECORE_X_ILLUME_MODE_DUAL_LEFT;
                    }
                  ecore_x_e_illume_mode_set(zone->black_win, mode);
               }
          }
     }

   return 1;
}

int 
e_mod_policy_shutdown(void) 
{
   Ecore_Event_Handler *hdl;
   E_Border_Hook *hook;

   /* remove the ecore event handlers */
   EINA_LIST_FREE(_policy_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* remove the border hooks */
   EINA_LIST_FREE(_policy_hooks, hook)
     e_border_hook_del(hook);

   /* destroy the policy if it exists */
   if (_policy) e_object_del(E_OBJECT(_policy));

   /* reset event type */
   E_ILLUME_POLICY_EVENT_CHANGE = 0;

   return 1;
}

/* local functions */
static char *
_e_mod_policy_find(void) 
{
   Eina_List *files;
   char buff[PATH_MAX], dir[PATH_MAX], *file;

   snprintf(buff, sizeof(buff), "%s.so", _e_illume_cfg->policy.name);
   snprintf(dir, sizeof(dir), "%s/policies", _e_illume_mod_dir);

   /* try to list all files in this directory */
   if (!(files = ecore_file_ls(dir))) return NULL;

   /* loop the returned files */
   EINA_LIST_FREE(files, file) 
     {
        /* compare file with needed .so */
        if (!strcmp(file, buff)) 
          {
             snprintf(dir, sizeof(dir), "%s/policies/%s", 
                      _e_illume_mod_dir, file);
             break;
          }
        free(file);
     }
   if (file) free(file);
   else 
     {
        /* if we did not find the requested policy, use a fallback */
        snprintf(dir, sizeof(dir), "%s/policies/illume.so", _e_illume_mod_dir);
     }

   return strdup(dir);
}

static int 
_e_mod_policy_load(char *file) 
{
   /* safety check */
   if (!file) return 0;

   /* delete existing policy first */
   if (_policy) e_object_del(E_OBJECT(_policy));

   /* try to create our new policy object */
   _policy = 
     E_OBJECT_ALLOC(E_Illume_Policy, E_ILLUME_POLICY_TYPE, 
                    _e_mod_policy_cb_free);
   if (!_policy) 
     {
        printf("Failed to allocate new policy object\n");
        return 0;
     }

   /* attempt to open the .so */
   if (!(_policy->handle = dlopen(file, (RTLD_NOW | RTLD_GLOBAL)))) 
     {
        /* cannot open the .so file, bail out */
        printf("Cannot open policy: %s\n", ecore_file_file_get(file));
        printf("\tError: %s\n", dlerror());
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   /* clear any existing errors in dynamic loader */
   dlerror();

   /* try to link to the needed policy api functions */
   _policy->api = dlsym(_policy->handle, "e_illume_policy_api");
   _policy->funcs.init = dlsym(_policy->handle, "e_illume_policy_init");
   _policy->funcs.shutdown = dlsym(_policy->handle, "e_illume_policy_shutdown");

   /* check that policy supports needed functions */
   if ((!_policy->api) || (!_policy->funcs.init) || (!_policy->funcs.shutdown)) 
     {
        /* policy doesn't support needed functions, bail out */
        printf("Policy does not support needed functions: %s\n", 
               ecore_file_file_get(file));
        printf("\tError: %s\n", dlerror());
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   /* check policy api version */
   if (_policy->api->version < E_ILLUME_POLICY_API_VERSION) 
     {
        /* policy is too old, bail out */
        printf("Policy is too old: %s\n", ecore_file_file_get(file));
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   /* try to initialize the policy */
   if (!_policy->funcs.init(_policy)) 
     {
        /* init failed, bail out */
        printf("Policy failed to initialize: %s\n", ecore_file_file_get(file));
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   return 1;
}

static void 
_e_mod_policy_handlers_add(void) 
{
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                              _e_mod_policy_cb_border_add, NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _e_mod_policy_cb_border_del, NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, 
                                              _e_mod_policy_cb_border_focus_in, 
                                              NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT, 
                                              _e_mod_policy_cb_border_focus_out, 
                                              NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_SHOW, 
                                              _e_mod_policy_cb_border_show, 
                                              NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE, 
                                              _e_mod_policy_cb_zone_move_resize, 
                                              NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_mod_policy_cb_client_message, 
                                              NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _e_mod_policy_cb_window_property, 
                                              NULL));
   _policy_hdls = 
     eina_list_append(_policy_hdls, 
                      ecore_event_handler_add(E_ILLUME_POLICY_EVENT_CHANGE, 
                                              _e_mod_policy_cb_policy_change, 
                                              NULL));
}

static void 
_e_mod_policy_hooks_add(void) 
{
   _policy_hooks = 
     eina_list_append(_policy_hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH, 
                                        _e_mod_policy_cb_hook_post_fetch, NULL));
   _policy_hooks = 
     eina_list_append(_policy_hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN, 
                                        _e_mod_policy_cb_hook_post_assign, NULL));
   _policy_hooks = 
     eina_list_append(_policy_hooks, 
                      e_border_hook_add(E_BORDER_HOOK_CONTAINER_LAYOUT, 
                                        _e_mod_policy_cb_hook_layout, NULL));
}

static void 
_e_mod_policy_cb_free(E_Illume_Policy *p) 
{
   /* tell the policy to shutdown */
   if (p->funcs.shutdown) p->funcs.shutdown(p);
   p->funcs.shutdown = NULL;

   p->funcs.init = NULL;
   p->api = NULL;

   /* close the linked .so */
   if (p->handle) dlclose(p->handle);
   p->handle = NULL;

   E_FREE(p);
}

static Eina_Bool
_e_mod_policy_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Add *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_add))
     _policy->funcs.border_add(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_del(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Remove *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_del))
     _policy->funcs.border_del(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_focus_in))
     _policy->funcs.border_focus_in(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_focus_out))
     _policy->funcs.border_focus_out(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_show(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Show *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_show))
     _policy->funcs.border_show(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_zone_move_resize(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Zone_Move_Resize *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.zone_move_resize))
     _policy->funcs.zone_move_resize(ev->zone);

   return ECORE_CALLBACK_PASS_ON;
}

static E_Zone *
_e_mod_zone_win_get(Ecore_X_Window win)
{
   E_Zone *zone = NULL;
   E_Border *bd;

   if (!(bd = e_border_find_by_client_window(win)))
     {
        if (!(zone = e_util_zone_window_find(win))) return NULL;
     }
   else if (bd->zone) zone = bd->zone;
   return zone;
}

static Eina_Bool
_e_mod_policy_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW) 
     {
        E_Border *bd;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.border_activate))
          _policy->funcs.border_activate(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_MODE) 
     {
        E_Zone *zone;
        
        if (!(zone = _e_mod_zone_win_get(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.zone_mode_change))
          _policy->funcs.zone_mode_change(zone, ev->data.l[0]);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_CLOSE) 
     {
        E_Zone *zone;

        if (!(zone = _e_mod_zone_win_get(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.zone_close))
          _policy->funcs.zone_close(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_FOCUS_BACK) 
     {
        E_Zone *zone;

        if (!(zone = _e_mod_zone_win_get(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.focus_back))
          _policy->funcs.focus_back(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_FOCUS_FORWARD) 
     {
        E_Zone *zone;

        if (!(zone = _e_mod_zone_win_get(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.focus_forward))
          _policy->funcs.focus_forward(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_FOCUS_HOME) 
     {
        E_Zone *zone;

        if (!(zone = _e_mod_zone_win_get(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.focus_home))
          _policy->funcs.focus_home(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_START) 
     {
        E_Border *bd;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.drag_start))
          _policy->funcs.drag_start(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_END) 
     {
        E_Border *bd;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.drag_end))
          _policy->funcs.drag_end(bd);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_property(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Window_Property *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.property_change))
     _policy->funcs.property_change(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_policy_change(void *data __UNUSED__, int type, void *event __UNUSED__) 
{
   char *file;

   if (type != E_ILLUME_POLICY_EVENT_CHANGE) return ECORE_CALLBACK_PASS_ON;

   /* find policy specified in config */
   if (!(file = _e_mod_policy_find())) return ECORE_CALLBACK_PASS_ON;

   /* try to load the policy */
   _e_mod_policy_load(file);

   if (file) free(file);

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_mod_policy_cb_hook_post_fetch(void *data __UNUSED__, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_post_fetch))
     _policy->funcs.border_post_fetch(bd);
}

static void 
_e_mod_policy_cb_hook_post_assign(void *data __UNUSED__, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_post_assign))
     _policy->funcs.border_post_assign(bd);
}

static void 
_e_mod_policy_cb_hook_layout(void *data __UNUSED__, void *data2 __UNUSED__) 
{
   E_Zone *zone;
   E_Border *bd;
   Eina_List *zl = NULL, *l;

   /* loop through border list and find what changed */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        if ((bd->new_client) || (bd->pending_move_resize) || 
            (bd->changes.pos) || (bd->changes.size) || (bd->changes.visible) || 
            (bd->need_shape_export) || (bd->need_shape_merge)) 
          {
             /* NB: this border changed. add it's zone to list of what needs 
              * updating. This is done so we do not waste cpu cycles 
              * updating zones where nothing changed */
             if (!eina_list_data_find(zl, bd->zone))
               zl = eina_list_append(zl, bd->zone);
          }
     }
   l = eina_list_free(l);

   /* loop the zones that need updating and call the policy update function */
   EINA_LIST_FREE(zl, zone) 
     {
        if ((_policy) && (_policy->funcs.zone_layout))
          _policy->funcs.zone_layout(zone);
     }
}
