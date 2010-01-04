#include "E_Illume.h"
#include "e_mod_main.h"
#include "e_mod_layout.h"

/* local function prototypes */
static void _e_mod_layout_free(E_Illume_Layout_Policy *p);
static int _e_mod_layout_policy_load(const char *file);
static void _e_mod_layout_handlers_add(void);
static void _e_mod_layout_handlers_del(void);
static void _e_mod_layout_hooks_add(void);
static void _e_mod_layout_hooks_del(void);
static void _e_mod_layout_cb_hook_container_layout(void *data, void *data2);
static void _e_mod_layout_cb_hook_post_fetch(void *data, void *data2);
static void _e_mod_layout_cb_hook_post_border_assign(void *data, void *data2);
static int _e_mod_layout_cb_border_add(void *data, int type, void *event);
static int _e_mod_layout_cb_border_del(void *data, int type, void *event);
static int _e_mod_layout_cb_border_focus_in(void *data, int type, void *event);
static int _e_mod_layout_cb_border_focus_out(void *data, int type, void *event);
static int _e_mod_layout_cb_zone_move_resize(void *data, int type, void *event);
static int _e_mod_layout_cb_client_message(void *data, int type, void *event);
static int _e_mod_layout_cb_policy_change(void *data, int type, void *event);

/* local variables */
static Eina_List *hooks = NULL, *handlers = NULL;
static E_Illume_Layout_Policy *policy = NULL;
EAPI int E_ILLUME_EVENT_POLICY_CHANGE = 0;

int 
e_mod_layout_init(void) 
{
   Ecore_X_Window xwin;
   Ecore_X_Illume_Mode mode;
   Eina_List *files;
   char buff[PATH_MAX], dir[PATH_MAX], *file;
   int ret = 0;

   E_ILLUME_EVENT_POLICY_CHANGE = ecore_event_type_new();

   snprintf(buff, sizeof(buff), "%s.so", il_cfg->policy.name);
   snprintf(dir, sizeof(dir), "%s/enlightenment/modules/illume2/policies", 
            e_prefix_lib_get());

   files = ecore_file_ls(dir);
   EINA_LIST_FREE(files, file) 
     {
        if (strcmp(file, buff)) 
          {
             free(file);
             continue;
          }
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/%s", 
                 e_prefix_lib_get(), file);
        break;
     }
   if (!file) 
     {
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/illume.so", 
                 e_prefix_lib_get());
     }
   else 
     free(file);

   ret = _e_mod_layout_policy_load(dir);
   if (!ret) return ret;

   _e_mod_layout_hooks_add();
   _e_mod_layout_handlers_add();

   xwin = ecore_x_window_root_first_get();
   if (il_cfg->policy.mode.dual == 0)
     mode = ECORE_X_ILLUME_MODE_SINGLE;
   else 
     {
        if (il_cfg->policy.mode.side == 0)
          mode = ECORE_X_ILLUME_MODE_DUAL_TOP;
        else
          mode = ECORE_X_ILLUME_MODE_DUAL_LEFT;
     }
   ecore_x_e_illume_mode_set(xwin, mode);

   return ret;
}

void 
e_mod_layout_shutdown(void) 
{
   _e_mod_layout_handlers_del();
   _e_mod_layout_hooks_del();
   if (policy) e_object_del(E_OBJECT(policy));
   policy = NULL;
}

EAPI Eina_List *
e_illume_layout_policies_get(void) 
{
   Eina_List *l = NULL, *files;
   char dir[PATH_MAX], *file;

   snprintf(dir, sizeof(dir), "%s/enlightenment/modules/illume2/policies", 
            e_prefix_lib_get());

   files = ecore_file_ls(dir);
   EINA_LIST_FREE(files, file) 
     {
        E_Illume_Layout_Policy *p;

        if (!strstr(file, ".so")) continue;
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/%s", 
                 e_prefix_lib_get(), file);
        p = E_OBJECT_ALLOC(E_Illume_Layout_Policy, E_ILLUME_LAYOUT_TYPE, 
                           _e_mod_layout_free);
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
        l = eina_list_append(l, p);
     }

   return l;
}

/* local functions */
static void 
_e_mod_layout_free(E_Illume_Layout_Policy *p) 
{
   if (p->funcs.shutdown) p->funcs.shutdown(p);
   p->funcs.shutdown = NULL;
   p->api = NULL;
   p->funcs.init = NULL;
   if (p->handle) dlclose(p->handle);
   p->handle = NULL;
   E_FREE(p);
}

static int 
_e_mod_layout_policy_load(const char *file) 
{
   if (!file) return 0;
   policy = E_OBJECT_ALLOC(E_Illume_Layout_Policy, E_ILLUME_LAYOUT_TYPE, 
                           _e_mod_layout_free);
   policy->handle = dlopen(file, RTLD_NOW | RTLD_GLOBAL);
   if (!policy->handle) 
     {
        E_ILLUME_ERR("Error opening policy: %s", file);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }
   policy->api = dlsym(policy->handle, "e_layapi");
   policy->funcs.init = dlsym(policy->handle, "e_layapi_init");
   policy->funcs.shutdown = dlsym(policy->handle, "e_layapi_shutdown");
   if ((!policy->funcs.init) || (!policy->funcs.shutdown) || (!policy->api)) 
     {
        E_ILLUME_ERR("Policy does not support needed functions: %s", file);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }
   if (policy->api->version < E_ILLUME_LAYOUT_API_VERSION) 
     {
        E_ILLUME_ERR("Policy is too old: %s", file);
        e_object_del(E_OBJECT(policy));
        policy = NULL;
        return 0;
     }
   policy->funcs.init(policy);
   return 1;
}

static void 
_e_mod_layout_handlers_add(void) 
{
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_ILLUME_EVENT_POLICY_CHANGE, 
                                              _e_mod_layout_cb_policy_change, NULL));
   handlers = 
     eina_list_append(handlers, 
                       ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                               _e_mod_layout_cb_border_add, NULL));
   handlers = 
     eina_list_append(handlers, 
                       ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                               _e_mod_layout_cb_border_del, NULL));
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
}

static void 
_e_mod_layout_handlers_del(void) 
{
   Ecore_Event_Handler *handler;

   EINA_LIST_FREE(handlers, handler)
     ecore_event_handler_del(handler);
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
                                        _e_mod_layout_cb_hook_post_border_assign, NULL));
   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_CONTAINER_LAYOUT, 
                                        _e_mod_layout_cb_hook_container_layout, NULL));
}

static void 
_e_mod_layout_hooks_del(void) 
{
   E_Border_Hook *hook;

   EINA_LIST_FREE(hooks, hook)
     e_border_hook_del(hook);
}

static void 
_e_mod_layout_cb_hook_container_layout(void *data, void *data2) 
{
   Eina_List *l;
   E_Zone *zone;
   E_Container *con;

   if (!(con = data2)) return;
   EINA_LIST_FOREACH(con->zones, l, zone) 
     {
        if ((policy) && (policy->funcs.zone_layout))
          policy->funcs.zone_layout(zone);
     }
}

static void 
_e_mod_layout_cb_hook_post_fetch(void *data, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (bd->stolen) return;
   if (bd->new_client)
     {
	if (bd->remember)
	  {
	     if (bd->bordername)
	       {
		  eina_stringshare_del(bd->bordername);
		  bd->bordername = NULL;
		  bd->client.border.changed = 1;
	       }
	     e_remember_unuse(bd->remember);
	     bd->remember = NULL;
	  }
        eina_stringshare_replace(&bd->bordername, "borderless");
        bd->client.border.changed = 1;
        bd->client.e.state.centered = 0;
     }
}

static void 
_e_mod_layout_cb_hook_post_border_assign(void *data, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (bd->stolen) return;
   bd->placed = 1;
   bd->client.e.state.centered = 0;
   if (bd->remember) 
     {
        e_remember_unuse(bd->remember);
        bd->remember = NULL;
     }
   bd->lock_border = 1;
   bd->lock_client_size = 1;
   bd->lock_client_desk = 1;
   bd->lock_client_sticky = 1;
   bd->lock_client_shade = 1;
   bd->lock_client_maximize = 1;
   bd->lock_user_size = 1;
   bd->lock_user_sticky = 1;
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
        E_Zone *zone;
        int lock = 1;

        zone = e_util_zone_current_get(e_manager_current_get());

        if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_SINGLE)
          il_cfg->policy.mode.dual = 0;
        else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_TOP) 
          {
             if (e_illume_border_valid_count_get(zone) < 2) 
               ecore_x_e_illume_home_send(ecore_x_window_root_first_get());
             il_cfg->policy.mode.dual = 1;
             il_cfg->policy.mode.side = 0;
             lock = 0;
          }
        else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_LEFT) 
          {
             if (e_illume_border_valid_count_get(zone) < 2) 
               ecore_x_e_illume_home_send(ecore_x_window_root_first_get());
             il_cfg->policy.mode.dual = 1;
             il_cfg->policy.mode.side = 1;
          }
        else /* unknown */
          il_cfg->policy.mode.dual = 0;
        e_config_save_queue();

        bd = e_illume_border_top_shelf_get(zone);
        if (bd) 
          ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
        bd = e_illume_border_bottom_panel_get(zone);
        if (bd) 
          ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_BACK) 
     {
        E_Border *bd, *fbd;
        Eina_List *focused, *l;

        if (!(bd = e_border_focused_get())) return 1;
        focused = e_border_focus_stack_get();
        EINA_LIST_REVERSE_FOREACH(focused, l, fbd) 
          {
             E_Border *fb;

             if (e_object_is_del(E_OBJECT(fbd))) continue;
             if ((!fbd->client.icccm.accepts_focus) && 
                 (!fbd->client.icccm.take_focus)) continue;
             if (fbd->client.netwm.state.skip_taskbar) continue;
             if (fbd == bd) 
               {
                  if (!(fb = focused->next->data)) continue;
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

static int 
_e_mod_layout_cb_policy_change(void *data, int type, void *event) 
{
   Eina_List *files;
   char buff[PATH_MAX], dir[PATH_MAX], *file;

   if (type != E_ILLUME_EVENT_POLICY_CHANGE) return 1;
   if (policy) 
     {
        e_object_del(E_OBJECT(policy));
        policy = NULL;
     }

   snprintf(buff, sizeof(buff), "%s.so", il_cfg->policy.name);
   snprintf(dir, sizeof(dir), "%s/enlightenment/modules/illume2/policies", 
            e_prefix_lib_get());

   files = ecore_file_ls(dir);
   EINA_LIST_FREE(files, file) 
     {
        if (strcmp(file, buff)) 
          {
             free(file);
             continue;
          }
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/%s", 
                 e_prefix_lib_get(), file);
        break;
     }
   if (!file) 
     {
        snprintf(dir, sizeof(dir), 
                 "%s/enlightenment/modules/illume2/policies/illume.so", 
                 e_prefix_lib_get());
     }
   else 
     free(file);

   _e_mod_layout_policy_load(dir);

   return 1;
}
