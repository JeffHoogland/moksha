#include "E_Illume.h"
#include "e_mod_layout.h"
#include "e_mod_main.h" // For logging functions
#include "e_quickpanel.h"

/* local function prototypes */
static const char *_e_mod_layout_policy_find(void);
static int _e_mod_layout_policy_load(const char *dir);
static void _e_mod_layout_policy_free(E_Illume_Layout_Policy *p);
static void _e_mod_layout_hooks_add(void);
static void _e_mod_layout_handlers_add(void);
static void _e_mod_layout_cb_hook_post_fetch(void *data, void *data2);
static void _e_mod_layout_cb_hook_post_assign(void *data, void *data2);
static void _e_mod_layout_cb_hook_layout(void *data, void *data2);
static int _e_mod_layout_cb_policy_change(void *data, int type, void *event);
static int _e_mod_layout_cb_border_add(void *data, int type, void *event);
static int _e_mod_layout_cb_border_del(void *data, int type, void *event);
static int _e_mod_layout_cb_border_focus_in(void *data, int type, void *event);
static int _e_mod_layout_cb_border_focus_out(void *data, int type, void *event);
static int _e_mod_layout_cb_border_property_change(void *data, int type, void *event);
static int _e_mod_layout_cb_zone_move_resize(void *data, int type, void *event);
static int _e_mod_layout_cb_client_message(void *data, int type, void *event);

/* local variables */
static E_Illume_Layout_Policy *policy = NULL;
static Eina_List *hooks = NULL, *handlers = NULL;

/* public variables */
EAPI int E_ILLUME_EVENT_POLICY_CHANGE = 0;

int 
e_mod_layout_init(void) 
{
   const char *file;

   /* find the policy specified in config */
   file = _e_mod_layout_policy_find();
   if (!file) return 0;

   /* try to load the policy */
   if (!_e_mod_layout_policy_load(file)) return 0;

   /* add our border hooks */
   _e_mod_layout_hooks_add();

   /* add event type FIRST */
   E_ILLUME_EVENT_POLICY_CHANGE = ecore_event_type_new();

   /* add our event handlers */
   _e_mod_layout_handlers_add();

   return 1;
}

int 
e_mod_layout_shutdown(void) 
{
   Ecore_Event_Handler *handler;
   E_Border_Hook *hook;

   E_ILLUME_EVENT_POLICY_CHANGE = 0;

   /* remove the ecore event handlers */
   EINA_LIST_FREE(handlers, handler)
     ecore_event_handler_del(handler);

   /* remove the border hooks */
   EINA_LIST_FREE(hooks, hook)
     e_border_hook_del(hook);

   /* destroy the policy if it exists */
   if (policy) e_object_del(E_OBJECT(policy));
   policy = NULL;

   return 1;
}

EAPI Eina_List *
e_illume_layout_policies_get(void) 
{
   Eina_List *l = NULL, *files;
   char dir[PATH_MAX], *file;

   snprintf(dir, sizeof(dir), "%s/enlightenment/modules/illume2/policies", 
            e_prefix_lib_get());

   files = ecore_file_ls(dir);
   if (!files) return NULL;

   EINA_LIST_FREE(files, file) 
     {
        E_Illume_Layout_Policy *p;

        if (!strstr(file, ".so")) continue;
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/%s", 
                 e_prefix_lib_get(), file);

        p = 
          E_OBJECT_ALLOC(E_Illume_Layout_Policy, E_ILLUME_LAYOUT_POLICY_TYPE, 
                         _e_mod_layout_policy_free);
        if (!p) continue;

        p->handle = dlopen(dir, RTLD_NOW | RTLD_GLOBAL);
        if (!p->handle) 
          {
             E_ILLUME_ERR("Error opening policy: %s", file);
             e_object_del(E_OBJECT(p));
             p = NULL;
             continue;
          }
        p->api = dlsym(p->handle, "e_layapi");
        if (!p->api) 
          {
             E_ILLUME_ERR("Policy does not support needed functions: %s", file);
             e_object_del(E_OBJECT(p));
             p = NULL;
             continue;
          }
        if (p->api->version < E_ILLUME_LAYOUT_API_VERSION) 
          {
             E_ILLUME_ERR("Policy is too old: %s", file);
             e_object_del(E_OBJECT(p));
             p = NULL;
             continue;
          }
        if (file) free(file);
        l = eina_list_append(l, p);
     }

   return l;
}

/* local functions */
static const char *
_e_mod_layout_policy_find(void) 
{
   Eina_List *files;
   char buff[PATH_MAX], dir[PATH_MAX], *file;

   snprintf(buff, sizeof(buff), "%s.so", il_cfg->policy.name);
   snprintf(dir, sizeof(dir), "%s/enlightenment/modules/illume2/policies", 
            e_prefix_lib_get());

   /* get all files in this directory */
   files = ecore_file_ls(dir);
   if (!files) return NULL;

   /* loop through the files, searching for this policy */
   EINA_LIST_FREE(files, file) 
     {
        /* compare file with needed .so */
        if (!strcmp(file, buff)) 
          {
             snprintf(dir, sizeof(dir), 
                      "%s/enlightenment/modules/illume2/policies/%s", 
                      e_prefix_lib_get(), file);
             break;
          }
        free(file);
     }
   if (file) free(file);
   else 
     {
        /* if we did not find the policy, use a fallback */
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/illume.so", 
                 e_prefix_lib_get());
     }
   return strdup(dir);
}

static int 
_e_mod_layout_policy_load(const char *dir) 
{
   if (!dir) return 0;

   /* delete existing policy first */
   if (policy) e_object_del(E_OBJECT(policy));
   policy = NULL;

   /* create new policy object */
   policy = 
     E_OBJECT_ALLOC(E_Illume_Layout_Policy, E_ILLUME_LAYOUT_POLICY_TYPE, 
                    _e_mod_layout_policy_free);
   if (!policy) return 0;

   /* attempt to open .so */
   policy->handle = dlopen(dir, RTLD_NOW | RTLD_GLOBAL);
   if (!policy->handle) 
     {
        /* open failed, bail out */
        E_ILLUME_ERR("Error Opening Policy: %s", dir);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }

   /* try to link to the needed policy api functions */
   policy->api = dlsym(policy->handle, "e_layapi");
   policy->funcs.init = dlsym(policy->handle, "e_layapi_init");
   policy->funcs.shutdown = dlsym(policy->handle, "e_layapi_shutdown");

   /* check the policy supports needed api functions */
   if ((!policy->api) || (!policy->funcs.init) || (!policy->funcs.shutdown)) 
     {
        /* policy doesn't support what we need, bail out */
        E_ILLUME_ERR("Policy does not support needed functions: %s", dir);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }

   /* check policy api version */
   if (policy->api->version < E_ILLUME_LAYOUT_API_VERSION) 
     {
        E_ILLUME_ERR("Policy is too old: %s", dir);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }

   /* initialize the policy */
   if (!policy->funcs.init(policy)) 
     {
        E_ILLUME_ERR("Policy failed to initialize: %s", dir);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }

   return 1;
}

static void 
_e_mod_layout_policy_free(E_Illume_Layout_Policy *p) 
{
   /* call the policy shutdown function if we can */
   if (p->funcs.shutdown) p->funcs.shutdown(p);
   p->funcs.shutdown = NULL;

   p->funcs.init = NULL;
   p->api = NULL;

   /* close the linked .so */
   if (p->handle) dlclose(p->handle);
   p->handle = NULL;

   E_FREE(p);
}

static void 
_e_mod_layout_hooks_add(void) 
{
   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH, 
                                        _e_mod_layout_cb_hook_post_fetch, NULL));
   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN, 
                                        _e_mod_layout_cb_hook_post_assign, NULL));
   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_CONTAINER_LAYOUT, 
                                        _e_mod_layout_cb_hook_layout, NULL));
}

static void 
_e_mod_layout_handlers_add(void) 
{
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_ILLUME_EVENT_POLICY_CHANGE, 
                                              _e_mod_layout_cb_policy_change, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                              _e_mod_layout_cb_border_add, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _e_mod_layout_cb_border_del, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, 
                                              _e_mod_layout_cb_border_focus_in, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT, 
                                              _e_mod_layout_cb_border_focus_out, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE, 
                                              _e_mod_layout_cb_zone_move_resize, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_mod_layout_cb_client_message, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _e_mod_layout_cb_border_property_change, 
                                              NULL));
}

static void 
_e_mod_layout_cb_hook_post_fetch(void *data, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((bd->stolen) || (!bd->new_client)) return;
   if (bd->remember) 
     {
        if (bd->bordername) 
          {
             eina_stringshare_del(bd->bordername);
             bd->bordername = NULL;
          }
        e_remember_unuse(bd->remember);
        bd->remember = NULL;
     }
   eina_stringshare_replace(&bd->bordername, "borderless");
   bd->client.border.changed = 1;
}

static void 
_e_mod_layout_cb_hook_post_assign(void *data, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (bd->stolen) return;
   bd->lock_client_shade = 1;
   bd->lock_client_maximize = 1;
}

static void 
_e_mod_layout_cb_hook_layout(void *data, void *data2) 
{
   E_Container *con;
   E_Zone *zone;
   Eina_List *l;

   if (!(con = data2)) return;
   EINA_LIST_FOREACH(con->zones, l, zone) 
     {
        if ((policy) && (policy->funcs.zone_layout)) 
          policy->funcs.zone_layout(zone);
     }
}

static int 
_e_mod_layout_cb_policy_change(void *data, int type, void *event) 
{
   const char *file;

   if (type != E_ILLUME_EVENT_POLICY_CHANGE) return 1;

   if (policy) e_object_del(E_OBJECT(policy));
   policy = NULL;

   /* find the policy specified in config */
   file = _e_mod_layout_policy_find();
   if (!file) return 1;

   /* try to load the policy */
   if (!_e_mod_layout_policy_load(file)) return 1;

   return 1;
}

static int 
_e_mod_layout_cb_border_add(void *data, int type, void *event) 
{
   E_Event_Border_Add *ev;

   ev = event;
   if (ev->border->stolen) return 1;
   if ((policy) && (policy->funcs.border_add)) 
     policy->funcs.border_add(ev->border);
   return 1;
}

static int 
_e_mod_layout_cb_border_del(void *data, int type, void *event) 
{
   E_Event_Border_Remove *ev;

   ev = event;
   if (e_illume_border_is_quickpanel(ev->border)) 
     {
        E_Quickpanel *qp;

        if (qp = e_quickpanel_by_zone_get(ev->border->zone)) 
          qp->borders = eina_list_remove(qp->borders, ev->border);
     }
   if (ev->border->stolen) return 1; 
   if ((policy) && (policy->funcs.border_del)) 
     policy->funcs.border_del(ev->border);
   return 1;
}

static int 
_e_mod_layout_cb_border_focus_in(void *data, int type, void *event) 
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   if (ev->border->stolen) return 1;
   if ((policy) && (policy->funcs.border_focus_in)) 
     policy->funcs.border_focus_in(ev->border);
   return 1;
}

static int 
_e_mod_layout_cb_border_focus_out(void *data, int type, void *event) 
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   if (ev->border->stolen) return 1;
   if ((policy) && (policy->funcs.border_focus_out)) 
     policy->funcs.border_focus_out(ev->border);
   return 1;
}

static int 
_e_mod_layout_cb_border_property_change(void *data, int type, void *event) 
{
   Ecore_X_Event_Window_Property *ev;
   E_Border *bd;

   ev = event;
   if (!(bd = e_border_find_by_client_window(ev->win))) return 1;
   if ((policy) && (policy->funcs.border_property_change)) 
     policy->funcs.border_property_change(bd, ev);
   return 1;
}

static int 
_e_mod_layout_cb_zone_move_resize(void *data, int type, void *event) 
{
   E_Event_Zone_Move_Resize *ev;

   ev = event;
   if ((policy) && (policy->funcs.zone_move_resize))
     policy->funcs.zone_move_resize(ev->zone);
   return 1;
}

static int 
_e_mod_layout_cb_client_message(void *data, int type, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW) 
     {
        E_Border *bd;

        bd = e_border_find_by_client_window(ev->win);
        if ((!bd) || (bd->stolen)) return 1;
        if ((policy) && (policy->funcs.border_activate))
          policy->funcs.border_activate(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_MODE) 
     {
        E_Border *bd;
        E_Manager *man;
        E_Container *con;
        E_Zone *zone;
        Eina_List *l, *ll, *lll;
        int lock = 1;

        EINA_LIST_FOREACH(e_manager_list(), lll, man) 
          EINA_LIST_FOREACH(man->containers, ll, con) 
            EINA_LIST_FOREACH(con->zones, l, zone) 
              {
                 if (zone->black_win == ev->win) 
                   {
                      E_Illume_Config_Zone *cz;

                      cz = e_illume_zone_config_get(zone->id);
                      if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_SINGLE)
                        cz->mode.dual = 0;
                      else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_TOP) 
                        {
                           if (e_illume_border_valid_count_get(zone) < 2)
                             ecore_x_e_illume_home_send(zone->black_win);
                           cz->mode.dual = 1;
                           cz->mode.side = 0;
                           lock = 0;
                        }
                      else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_LEFT) 
                        {
                           if (e_illume_border_valid_count_get(zone) < 2)
                             ecore_x_e_illume_home_send(zone->black_win);
                           cz->mode.dual = 1;
                           cz->mode.side = 1;
                        }
                      else
                        cz->mode.dual = 0;
                      e_config_save_queue();

                      bd = e_illume_border_top_shelf_get(zone);
                      if (bd)
                        ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
                      bd = e_illume_border_bottom_panel_get(zone);
                      if (bd)
                        ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
                      break;
                   }
              }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_BACK) 
     {
        E_Zone *zone;
        Eina_List *focused, *l, *f2 = NULL;
        E_Border *fbd;

        zone = e_util_zone_window_find(ev->win);
        if (!zone) return 1;

        focused = e_border_focus_stack_get();
        if (!focused) return 1;

        EINA_LIST_FOREACH(focused, l, fbd) 
          {
             if (fbd->zone != zone) continue;
             f2 = eina_list_append(f2, fbd);
          }

        if (!f2) return 1;
        if (eina_list_count(f2) < 1) return 1;

        EINA_LIST_REVERSE_FOREACH(f2, l, fbd) 
          {
             if (e_object_is_del(E_OBJECT(fbd))) continue;
             if ((!fbd->client.icccm.accepts_focus) && 
                 (!fbd->client.icccm.take_focus)) continue;
             if (fbd->client.netwm.state.skip_taskbar) continue;
             if (fbd == e_border_focused_get()) 
               {
                  E_Border *fb;

                  if (!(fb = f2->next->data)) continue;
                  if (e_object_is_del(E_OBJECT(fb))) continue;
                  if ((!fb->client.icccm.accepts_focus) && 
                      (!fb->client.icccm.take_focus)) continue;
                  if (fb->client.netwm.state.skip_taskbar) continue;
                  e_border_raise(fb);
                  e_border_focus_set(fb, 1, 1);
                  break;
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_CLOSE) 
     {
        E_Border *bd;

        if (!(bd = e_border_focused_get())) return 1;
        e_border_act_close_begin(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_START) 
     {
        E_Border *bd;

        bd = e_border_find_by_client_window(ev->win);
        if ((!bd) || (bd->stolen)) return 1;
        if ((policy) && (policy->funcs.drag_start))
          policy->funcs.drag_start(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_END) 
     {
        E_Border *bd;

        bd = e_border_find_by_client_window(ev->win);
        if ((!bd) || (bd->stolen)) return 1;
        if ((policy) && (policy->funcs.drag_end))
          policy->funcs.drag_end(bd);
     }
   return 1;
}
