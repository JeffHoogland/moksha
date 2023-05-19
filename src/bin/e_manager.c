#include "e.h"

/* local subsystem functions */
static void       _e_manager_free(E_Manager *man);

static Eina_Bool  _e_manager_cb_window_show_request(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_window_configure(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_key_up(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_key_down(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_frame_extents_request(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_ping(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_screensaver_on(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_screensaver_off(void *data, int ev_type, void *ev);
static Eina_Bool  _e_manager_cb_client_message(void *data, int ev_type, void *ev);

static Eina_Bool  _e_manager_frame_extents_free_cb(const Eina_Hash *hash __UNUSED__,
                                                   const void *key __UNUSED__,
                                                   void *data, void *fdata __UNUSED__);
static E_Manager *_e_manager_get_for_root(Ecore_X_Window root);
static Eina_Bool  _e_manager_clear_timer(void *data);
#if 0 /* use later - maybe */
static int        _e_manager_cb_window_destroy(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_hide(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_reparent(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_create(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_configure_request(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_gravity(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_stack(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_stack_request(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_property(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_colormap(void *data, int ev_type, void *ev);
static int        _e_manager_cb_window_shape(void *data, int ev_type, void *ev);
static int        _e_manager_cb_client_message(void *data, int ev_type, void *ev);
#endif

/* local subsystem globals */

typedef struct _Frame_Extents Frame_Extents;

struct _Frame_Extents
{
   int l, r, t, b;
};

EAPI int E_EVENT_MANAGER_KEYS_GRAB = -1;

static Eina_List *managers = NULL;
static Eina_Hash *frame_extents = NULL;
static Ecore_Timer *timer_post_screensaver_lock = NULL;

/* externally accessible functions */
EINTERN int
e_manager_init(void)
{
   ecore_x_screensaver_event_listen_set(1);
   frame_extents = eina_hash_string_superfast_new(NULL);
   E_EVENT_MANAGER_KEYS_GRAB = ecore_event_type_new();
   return 1;
}

EINTERN int
e_manager_shutdown(void)
{
   E_FREE_LIST(managers, e_object_del);

   if (frame_extents)
     {
        eina_hash_foreach(frame_extents, _e_manager_frame_extents_free_cb, NULL);
        eina_hash_free(frame_extents);
        frame_extents = NULL;
     }

   if (timer_post_screensaver_lock)
     {
        ecore_timer_del(timer_post_screensaver_lock);
        timer_post_screensaver_lock = NULL;
     }

   return 1;
}

EAPI Eina_List *
e_manager_list(void)
{
   return managers;
}

EAPI E_Manager *
e_manager_new(Ecore_X_Window root, int num)
{
   E_Manager *man;

   if (!ecore_x_window_manage(root)) return NULL;
   man = E_OBJECT_ALLOC(E_Manager, E_MANAGER_TYPE, _e_manager_free);
   if (!man) return NULL;
   managers = eina_list_append(managers, man);
   man->root = root;
   man->num = num;
   ecore_x_window_size_get(man->root, &(man->w), &(man->h));
   man->win = man->root;

   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST,
                                              _e_manager_cb_window_show_request,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,
                                              _e_manager_cb_window_configure,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                              _e_manager_cb_key_down,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_UP,
                                              _e_manager_cb_key_up,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_FRAME_EXTENTS_REQUEST,
                                              _e_manager_cb_frame_extents_request,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_PING,
                                              _e_manager_cb_ping,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(E_EVENT_SCREENSAVER_ON,
                                              _e_manager_cb_screensaver_on,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(E_EVENT_SCREENSAVER_OFF,
                                              _e_manager_cb_screensaver_off,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                              _e_manager_cb_client_message,
                                              man));

   man->pointer = e_pointer_window_new(man->root, 1);

   ecore_x_window_background_color_set(man->root, 0, 0, 0);

   man->clear_timer = ecore_timer_add(10.0, _e_manager_clear_timer, man);
   return man;
}

EAPI void
e_manager_manage_windows(E_Manager *man)
{
   Ecore_X_Window *windows;
   int wnum;

   /* a manager is designated for each root. lets get all the windows in
      the managers root */
   windows = ecore_x_window_children_get(man->root, &wnum);
   if (windows)
     {
        int i;
        const char *atom_names[] =
        {
           "_XEMBED_INFO",
           "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
           "KWM_DOCKWINDOW"
        };
        Ecore_X_Atom atoms[3];
        Ecore_X_Atom atom_xmbed, atom_kde_netwm_systray, atom_kwm_dockwindow;
        unsigned char *data = NULL;
        int count;

        ecore_x_atoms_get(atom_names, 3, atoms);
        atom_xmbed = atoms[0];
        atom_kde_netwm_systray = atoms[1];
        atom_kwm_dockwindow = atoms[2];
        //~ for (i = 0; i < wnum; i++)
        for (i = wnum - 1; i >= 0; i--)
          {
             Ecore_X_Window_Attributes att;
             unsigned int ret_val, deskxy[2];
             int ret;

             if (e_border_find_by_client_window(windows[i]))
               continue;
             ecore_x_window_attributes_get(windows[i], &att);
             if ((att.override) || (att.input_only))
               {
                  if (att.override)
                    {
                       char *wname = NULL, *wclass = NULL;

                       ecore_x_icccm_name_class_get(windows[i],
                                                    &wname, &wclass);
                       if ((wname) && (wclass) &&
                           (!strcmp(wname, "E")) &&
                           (!strcmp(wclass, "Init_Window")))
                         {
                            free(wname);
                            free(wclass);
                            man->initwin = windows[i];
                         }
                       else
                         {
                            free(wname);
                            free(wclass);
                            continue;
                         }
                    }
                  else
                    continue;
               }
             /* XXX manage xembed windows as long as they are not override_redirect..
              * if (!ecore_x_window_prop_property_get(windows[i],
              *					   atom_xmbed,
              *					   atom_xmbed, 32,
              *					   &data, &count))
              *   data = NULL;
              * if (!data) */
             {
                if (!ecore_x_window_prop_property_get(windows[i],
                                                      atom_kde_netwm_systray,
                                                      atom_xmbed, 32,
                                                      &data, &count))
                  data = NULL;
             }
             if (!data)
               {
                  if (!ecore_x_window_prop_property_get(windows[i],
                                                        atom_kwm_dockwindow,
                                                        atom_kwm_dockwindow, 32,
                                                        &data, &count))
                    data = NULL;
               }
             if (data)
               {
                  E_FREE(data);
                  continue;
               }
             ret = ecore_x_window_prop_card32_get(windows[i],
                                                  E_ATOM_MANAGED,
                                                  &ret_val, 1);

             /* we have seen this window before */
             if ((ret > -1) && (ret_val == 1))
               {
                  E_Container *con = NULL;
                  E_Zone *zone = NULL;
                  E_Desk *desk = NULL;
                  E_Border *bd = NULL;
                  unsigned int id;
                  char *path;
                  Efreet_Desktop *desktop = NULL;

                  /* get all information from window before it is
                   * reset by e_border_new */
                  ret = ecore_x_window_prop_card32_get(windows[i],
                                                       E_ATOM_CONTAINER,
                                                       &id, 1);
                  if (ret == 1)
                    con = e_container_number_get(man, id);
                  if (!con)
                    con = e_container_current_get(man);

                  ret = ecore_x_window_prop_card32_get(windows[i],
                                                       E_ATOM_ZONE,
                                                       &id, 1);
                  if (ret == 1)
                    zone = e_container_zone_number_get(con, id);
                  if (!zone)
                    zone = e_zone_current_get(con);
                  ret = ecore_x_window_prop_card32_get(windows[i],
                                                       E_ATOM_DESK,
                                                       deskxy, 2);
                  if (ret == 2)
                    desk = e_desk_at_xy_get(zone,
                                            deskxy[0],
                                            deskxy[1]);

                  path = ecore_x_window_prop_string_get(windows[i],
                                                        E_ATOM_DESKTOP_FILE);
                  if (path)
                    {
                       desktop = efreet_desktop_get(path);
                       free(path);
                    }

                  {
                     bd = e_border_new(con, windows[i], 1, 0);
                     if (bd)
                       {
                          bd->ignore_first_unmap = 1;
                          /* FIXME:
                           * It's enough to set the desk, the zone will
                           * be set according to the desk */
//			    if (zone) e_border_zone_set(bd, zone);
                          if (desk) e_border_desk_set(bd, desk);
                          bd->desktop = desktop;
                       }
                  }
               }
             else if ((att.visible) && (!att.override) &&
                      (!att.input_only))
               {
                  /* We have not seen this window, and X tells us it
                   * should be seen */
                  E_Container *con;
                  E_Border *bd;

                  con = e_container_current_get(man);
                  bd = e_border_new(con, windows[i], 1, 0);
                  if (bd)
                    {
                       bd->ignore_first_unmap = 1;
                       e_border_show(bd);
                    }
               }
          }
        free(windows);
     }
}

EAPI void
e_manager_show(E_Manager *man)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->visible) return;
   EINA_LIST_FOREACH(man->containers, l, con)
     {
        e_container_show(con);
     }
   if (man->root != man->win)
     {
        Ecore_X_Window mwin;

        mwin = e_menu_grab_window_get();
        if (!mwin) mwin = man->initwin;
        if (!mwin)
          ecore_x_window_raise(man->win);
        else
          ecore_x_window_configure(man->win,
                                   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                   0, 0, 0, 0, 0,
                                   mwin, ECORE_X_WINDOW_STACK_BELOW);
        ecore_x_window_show(man->win);
     }
   man->visible = 1;
}

EAPI void
e_manager_hide(E_Manager *man)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->visible) return;
   EINA_LIST_FOREACH(man->containers, l, con)
     {
        e_container_hide(con);
     }
   if (man->root != man->win)
     ecore_x_window_hide(man->win);
   man->visible = 0;
}

EAPI void
e_manager_move(E_Manager *man, int x, int y)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if ((x == man->x) && (y == man->y)) return;
   if (man->root != man->win)
     {
        man->x = x;
        man->y = y;
        ecore_x_window_move(man->win, man->x, man->y);
     }
}

EAPI void
e_manager_resize(E_Manager *man, int w, int h)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   man->w = w;
   man->h = h;
   if (man->root != man->win)
     ecore_x_window_resize(man->win, man->w, man->h);

   EINA_LIST_FOREACH(man->containers, l, con)
     {
        e_container_resize(con, man->w, man->h);
     }

   ecore_x_netwm_desk_size_set(man->root, man->w, man->h);
}

EAPI void
e_manager_move_resize(E_Manager *man, int x, int y, int w, int h)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     {
        man->x = x;
        man->y = y;
     }
   man->w = w;
   man->h = h;
   ecore_x_window_move_resize(man->win, man->x, man->y, man->w, man->h);

   EINA_LIST_FOREACH(man->containers, l, con)
     {
        e_container_resize(con, man->w, man->h);
     }
}

EAPI void
e_manager_raise(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     {
        Ecore_X_Window mwin;

        mwin = e_menu_grab_window_get();
        if (!mwin) mwin = man->initwin;
        if (!mwin)
          ecore_x_window_raise(man->win);
        else
          ecore_x_window_configure(man->win,
                                   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                   0, 0, 0, 0, 0,
                                   mwin, ECORE_X_WINDOW_STACK_BELOW);
     }
}

EAPI void
e_manager_lower(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     ecore_x_window_lower(man->win);
}

EAPI E_Manager *
e_manager_current_get(void)
{
   Eina_List *l;
   E_Manager *man;
   int x, y;

   if (!managers) return NULL;
   EINA_LIST_FOREACH(managers, l, man)
     {
        ecore_x_pointer_xy_get(man->win, &x, &y);
        if (x == -1 && y == -1)
          continue;
        if (E_INSIDE(x, y, man->x, man->y, man->w, man->h))
          return man;
     }
   return eina_list_data_get(managers);
}

EAPI E_Manager *
e_manager_number_get(int num)
{
   Eina_List *l;
   E_Manager *man;

   if (!managers) return NULL;
   EINA_LIST_FOREACH(managers, l, man)
     {
        if (man->num == num)
          return man;
     }
   return NULL;
}

EAPI void
e_managers_keys_grab(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(managers, l, man)
     {
        e_bindings_key_grab(E_BINDING_CONTEXT_ANY, man->root);
     }
   ecore_event_add(E_EVENT_MANAGER_KEYS_GRAB, NULL, NULL, NULL);
}

EAPI void
e_managers_keys_ungrab(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(managers, l, man)
     {
        e_bindings_key_ungrab(E_BINDING_CONTEXT_ANY, man->root);
     }
}

EAPI void
e_manager_comp_set(E_Manager *man, E_Manager_Comp *comp)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   man->comp = comp;
   e_msg_send("comp.manager", "change.comp", // name + info
              0, // val
              E_OBJECT(man), // obj
              NULL, // msgdata
              NULL, NULL); // afterfunc + afterdata
}

EAPI Evas *
e_manager_comp_evas_get(E_Manager *man)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   return man->comp->func.evas_get(man->comp->data, man);
}

EAPI void
e_manager_comp_evas_update(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   man->comp->func.update(man->comp->data, man);
}

EAPI const Eina_List *
e_manager_comp_src_list(E_Manager *man)
{
   return man->comp->func.src_list_get(man->comp->data, man);
}

EAPI E_Manager_Comp_Source *
e_manager_comp_border_src_get(E_Manager *man, Ecore_X_Window win)
{
   return man->comp->func.border_src_get(man->comp->data, man, win);
}

EAPI E_Manager_Comp_Source *
e_manager_comp_src_get(E_Manager *man, Ecore_X_Window win)
{
   return man->comp->func.src_get(man->comp->data, man, win);
}

EAPI Evas_Object *
e_manager_comp_src_image_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_image_get(man->comp->data, man, src);
}

EAPI Evas_Object *
e_manager_comp_src_shadow_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_shadow_get(man->comp->data, man, src);
}

EAPI Evas_Object *
e_manager_comp_src_image_mirror_add(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_image_mirror_add(man->comp->data, man, src);
}

EAPI Eina_Bool
e_manager_comp_src_visible_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_visible_get(man->comp->data, man, src);
}

EAPI void
e_manager_comp_src_hidden_set(E_Manager *man, E_Manager_Comp_Source *src, Eina_Bool hidden)
{
   man->comp->func.src_hidden_set(man->comp->data, man, src, hidden);
}

EAPI Eina_Bool
e_manager_comp_src_hidden_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_hidden_get(man->comp->data, man, src);
}

EAPI Ecore_X_Window
e_manager_comp_src_window_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_window_get(man->comp->data, man, src);
}

EAPI E_Popup *
e_manager_comp_src_popup_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_popup_get(man->comp->data, man, src);
}

EAPI E_Border *
e_manager_comp_src_border_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   return man->comp->func.src_border_get(man->comp->data, man, src);
}

EAPI void
e_manager_comp_event_resize_send(E_Manager *man)
{
   e_msg_send("comp.manager", "resize.comp", // name + info
              0, // val
              E_OBJECT(man), // obj
              NULL, // msgdata
              NULL, NULL); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_add_send(E_Manager *man, E_Manager_Comp_Source *src,
                                  void (*afterfunc)(void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                  void *data)
{
   e_msg_send("comp.manager", "add.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_del_send(E_Manager *man, E_Manager_Comp_Source *src,
                                  void (*afterfunc)(void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                  void *data)
{
   e_msg_send("comp.manager", "del.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_config_send(E_Manager *man, E_Manager_Comp_Source *src,
                                     void (*afterfunc)(void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                     void *data)
{
   e_msg_send("comp.manager", "config.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_visibility_send(E_Manager *man, E_Manager_Comp_Source *src,
                                         void (*afterfunc)(void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                         void *data)
{
   e_msg_send("comp.manager", "visibility.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

/* local subsystem functions */
static void
_e_manager_free(E_Manager *man)
{
   Eina_List *l;

   E_FREE_LIST(man->handlers, ecore_event_handler_del);
   l = man->containers;
   man->containers = NULL;
   E_FREE_LIST(l, e_object_del);
   if (man->root != man->win)
     {
        ecore_x_window_free(man->win);
     }
   if (man->pointer) e_object_del(E_OBJECT(man->pointer));
   managers = eina_list_remove(managers, man);
   if (man->clear_timer) ecore_timer_del(man->clear_timer);
   free(man);
}

static Eina_Bool
_e_manager_cb_window_show_request(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Window_Show_Request *e;

   man = data;
   e = ev;
   if (e_stolen_win_get(e->win)) return 1;
   if (ecore_x_window_parent_get(e->win) != man->root)
     return ECORE_CALLBACK_PASS_ON;  /* try other handlers for this */

   {
      E_Container *con;
      E_Border *bd;

      con = e_container_current_get(man);
      if (!e_border_find_by_client_window(e->win))
        {
           bd = e_border_new(con, e->win, 0, 0);
           if (!bd)
             ecore_x_window_show(e->win);
        }
   }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_window_configure(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Window_Configure *e;

   man = data;
   e = ev;
   if (e->win != man->root) return ECORE_CALLBACK_PASS_ON;
   e_manager_resize(man, e->w, e->h);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_key_down(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_Event_Key *e;

   man = data;
   e = ev;

   if (e->event_window != man->root) return ECORE_CALLBACK_PASS_ON;
   if (e->root_window != man->root) man = _e_manager_get_for_root(e->root_window);
   if (e_bindings_key_down_event_handle(E_BINDING_CONTEXT_MANAGER, E_OBJECT(man), e))
     return ECORE_CALLBACK_DONE;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_key_up(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_Event_Key *e;

   man = data;
   e = ev;

   if (e->event_window != man->root) return ECORE_CALLBACK_PASS_ON;
   if (e->root_window != man->root) man = _e_manager_get_for_root(e->root_window);
   if (e_bindings_key_up_event_handle(E_BINDING_CONTEXT_MANAGER, E_OBJECT(man), e))
     return ECORE_CALLBACK_DONE;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_frame_extents_request(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   E_Container *con;
   Ecore_X_Event_Frame_Extents_Request *e;
   Ecore_X_Window_Type type;
   Ecore_X_MWM_Hint_Decor decor;
   Ecore_X_Window_State *state;
   Frame_Extents *extents;
   const char *border, *sig, *key;
   int ok;
   unsigned int i, num;

   man = data;
   con = e_container_current_get(man);
   e = ev;

   if (ecore_x_window_parent_get(e->win) != man->root) return ECORE_CALLBACK_PASS_ON;

   /* TODO:
    * * We need to check if we remember this window, and border locking is set
    */
   border = "default";
   key = border;
   ok = ecore_x_mwm_hints_get(e->win, NULL, &decor, NULL);
   if ((ok) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_ALL)) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_TITLE)) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_BORDER)))
     {
        border = "borderless";
        key = border;
     }

   ok = ecore_x_netwm_window_type_get(e->win, &type);
   if ((ok) &&
       ((type == ECORE_X_WINDOW_TYPE_DESKTOP) ||
        (type == ECORE_X_WINDOW_TYPE_DOCK)))
     {
        border = "borderless";
        key = border;
     }

   sig = NULL;
   ecore_x_netwm_window_state_get(e->win, &state, &num);
   if (state)
     {
        int maximized = 0;

        for (i = 0; i < num; i++)
          {
             switch (state[i])
               {
                case ECORE_X_WINDOW_STATE_MAXIMIZED_VERT:
                  maximized++;
                  break;

                case ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ:
                  maximized++;
                  break;

                case ECORE_X_WINDOW_STATE_FULLSCREEN:
                  border = "borderless";
                  key = border;
                  break;

                case ECORE_X_WINDOW_STATE_SHADED:
                case ECORE_X_WINDOW_STATE_SKIP_TASKBAR:
                case ECORE_X_WINDOW_STATE_SKIP_PAGER:
                case ECORE_X_WINDOW_STATE_HIDDEN:
                case ECORE_X_WINDOW_STATE_ICONIFIED:
                case ECORE_X_WINDOW_STATE_MODAL:
                case ECORE_X_WINDOW_STATE_STICKY:
                case ECORE_X_WINDOW_STATE_ABOVE:
                case ECORE_X_WINDOW_STATE_BELOW:
                case ECORE_X_WINDOW_STATE_DEMANDS_ATTENTION:
                case ECORE_X_WINDOW_STATE_UNKNOWN:
                  break;
               }
          }
        if ((maximized == 2) &&
            (e_config->maximize_policy == E_MAXIMIZE_FULLSCREEN))
          {
             sig = "e,action,maximize,fullscreen";
             key = "maximize,fullscreen";
          }
        free(state);
     }

   extents = eina_hash_find(frame_extents, key);
   if (!extents)
     {
        extents = E_NEW(Frame_Extents, 1);
        if (extents)
          {
             Evas_Object *o;
             char buf[1024];

             o = edje_object_add(con->bg_evas);
             snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", border);
             ok = e_theme_edje_object_set(o, "base/theme/borders", buf);
             if (ok)
               {
                  Evas_Coord x, y, w, h;

                  if (sig)
                    {
                       edje_object_signal_emit(o, sig, "e");
                       edje_object_message_signal_process(o);
                    }

                  evas_object_resize(o, 1000, 1000);
                  edje_object_calc_force(o);
                  edje_object_part_geometry_get(o, "e.swallow.client",
                                                &x, &y, &w, &h);
                  extents->l = x;
                  extents->r = 1000 - (x + w);
                  extents->t = y;
                  extents->b = 1000 - (y + h);
               }
             else
               {
                  extents->l = 0;
                  extents->r = 0;
                  extents->t = 0;
                  extents->b = 0;
               }
             evas_object_del(o);
             eina_hash_add(frame_extents, key, extents);
          }
     }

   if (extents)
     ecore_x_netwm_frame_size_set(e->win, extents->l, extents->r, extents->t, extents->b);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_ping(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   E_Border *bd;
   Ecore_X_Event_Ping *e;

   man = data;
   e = ev;

   if (e->win != man->root) return ECORE_CALLBACK_PASS_ON;

   bd = e_border_find_by_client_window(e->event_win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   bd->ping_ok = 1;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_timer_post_screensaver_lock(void *data __UNUSED__)
{
   e_desklock_show_autolocked();
   timer_post_screensaver_lock = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_manager_cb_screensaver_on(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   if (e_config->desklock_autolock_screensaver)
     {
        if (timer_post_screensaver_lock)
          {
             ecore_timer_del(timer_post_screensaver_lock);
             timer_post_screensaver_lock = NULL;
          }
        if (e_config->desklock_post_screensaver_time <= 1.0)
          e_desklock_show_autolocked();
        else
          timer_post_screensaver_lock = ecore_timer_add
          (e_config->desklock_post_screensaver_time,
              _e_manager_cb_timer_post_screensaver_lock, NULL);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_screensaver_off(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   if (timer_post_screensaver_lock)
     {
        ecore_timer_del(timer_post_screensaver_lock);
        timer_post_screensaver_lock = NULL;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_client_message(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   Ecore_X_Event_Client_Message *e;
   E_Border *bd;

   e = ev;

   if (e->message_type != ECORE_X_ATOM_NET_ACTIVE_WINDOW) return ECORE_CALLBACK_RENEW;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_RENEW;
#if 0 /* notes */
   if (e->data.l[0] == 0 /* 0 == old, 1 == client, 2 == pager */)
     {
     }
   timestamp = e->data.l[1];
   requestor_id e->data.l[2];
#endif
   switch (e_config->window_activehint_policy)
     {
      case 0: break;
      case 1:
        edje_object_signal_emit(bd->bg_object, "e,state,urgent", "e");
        break;
      default:
        if (!bd->focused) e_border_activate(bd, EINA_FALSE);
        else e_border_raise(bd);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_frame_extents_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__,
                                 void *data, void *fdata __UNUSED__)
{
   free(data);
   return EINA_TRUE;
}

static E_Manager *
_e_manager_get_for_root(Ecore_X_Window root)
{
   Eina_List *l;
   E_Manager *man;

   if (!managers) return NULL;
   EINA_LIST_FOREACH(managers, l, man)
     {
        if (man->root == root)
          return man;
     }
   return eina_list_data_get(managers);
}

static Eina_Bool
_e_manager_clear_timer(void *data)
{
   E_Manager *man = data;
   ecore_x_window_background_color_set(man->root, 0, 0, 0);
   man->clear_timer = NULL;
   return EINA_FALSE;
}

#if 0 /* use later - maybe */
static int
_e_manager_cb_window_destroy(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_hide(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_reparent(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_create(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_configure_request(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_configure(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_gravity(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_stack(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_stack_request(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_property(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_colormap(void *data, int ev_type, void *ev){return 1; }

static int
_e_manager_cb_window_shape(void *data, int ev_type, void *ev){return 1; }

#endif
