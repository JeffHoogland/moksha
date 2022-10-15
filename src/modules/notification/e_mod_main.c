#include "e_mod_main.h"
#include "history.h"

/* gadcon globals */
E_Module *notification_mod = NULL;
Config *notification_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

/* Gadcon functions */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);

/* Callback function protos */
static void             _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void             _cb_menu_post_deactivate(void *data, E_Menu *menu __UNUSED__);
static void             _cb_config_show(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__);
static unsigned int     _notification_cb_notify(void *data __UNUSED__, E_Notification_Notify *n);
static void             _cb_menu_item(void *selected_item, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__);
static void             _clear_menu_cb(void);
static void              _notification_cb_close(void *data __UNUSED__, unsigned int id);
static Eina_Bool        _notification_cb_config_mode_changed(Config *m_cfg, int   type __UNUSED__, void *event __UNUSED__);

/* Utility functions */


/* Notify Functions */
static void _notification_show_actions(Popup_Items *sel_item, const char *icon);

/* Config function protos */
static Config *_notification_cfg_new(void);
static void    _notification_cfg_free(Config *cfg);

/* Global variables */
EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Notification"};

static const E_Notification_Server_Info server_info = {
   .name = "Notification Service",
   .vendor = "Enlightenment",
   .version = "0.17",
   .spec_version = "1.2",
   .capabilities = {
      "body", "body-markup",
      "body-hyperlinks", "body-images",
      "actions",
//      "action-icons",
//      "icon-multi",
// or
      "icon-static",
      "persistence",
      "sound",
      NULL }
};

/* Gadget support */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "notification",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* Module Api Functions */

EAPI void *
e_modapi_init(E_Module *m)
{
   /* register config panel entry */
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add("extensions/notification", 30,
                                 _("Notification"), NULL,
                                 "preferences-system-notifications",
                                 e_int_config_notification_module);


   conf_edd = E_CONFIG_DD_NEW("Notification_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, show_low, INT);
   E_CONFIG_VAL(D, T, show_normal, INT);
   E_CONFIG_VAL(D, T, show_critical, INT);
   E_CONFIG_VAL(D, T, corner, INT);
   E_CONFIG_VAL(D, T, timeout, FLOAT);
   E_CONFIG_VAL(D, T, force_timeout, INT);
   E_CONFIG_VAL(D, T, ignore_replacement, INT);
   E_CONFIG_VAL(D, T, dual_screen, INT);
   E_CONFIG_VAL(D, T, time_stamp, INT);
   E_CONFIG_VAL(D, T, show_app, INT);
   E_CONFIG_VAL(D, T, show_count, INT);
   E_CONFIG_VAL(D, T, secure_clear, INT);
   E_CONFIG_VAL(D, T, reverse, INT);
   E_CONFIG_VAL(D, T, menu_items, DOUBLE);
   E_CONFIG_VAL(D, T, item_length, DOUBLE);
   E_CONFIG_VAL(D, T, jump_delay, DOUBLE);
   E_CONFIG_VAL(D, T, blacklist, STR);

   notification_cfg = e_config_domain_load("module.notification", conf_edd);
   if (notification_cfg &&
       !(e_util_module_config_check(_("Notification Module"),
                                    notification_cfg->version,
                                    MOD_CONFIG_FILE_VERSION)))
     {
        _notification_cfg_free(notification_cfg);
        notification_cfg = NULL;
     }

   if (!notification_cfg)
     notification_cfg = _notification_cfg_new();
   /* upgrades */
   if (notification_cfg->version - (MOD_CONFIG_FILE_EPOCH * 1000000) < 1)
     {
        if (notification_cfg->dual_screen) notification_cfg->dual_screen = POPUP_DISPLAY_POLICY_MULTI;
     }
   notification_cfg->version = MOD_CONFIG_FILE_VERSION;

   /* set up the notification daemon */
   if (!e_notification_server_register(&server_info, _notification_cb_notify,
                                       _notification_cb_close, NULL))
     {
        e_util_dialog_show(_("Error during notification server initialization"),
                           _("Ensure there's no other module acting as a server "
                             "and that D-Bus is correctly installed and "
                             "running"));
        return NULL;
     }
   notification_cfg->last_config_mode.presentation = e_config->mode.presentation;
   notification_cfg->last_config_mode.offline = e_config->mode.offline;
   notification_cfg->last_config_mode.presentation = e_config->mode.presentation;
   notification_cfg->last_config_mode.offline = e_config->mode.offline;

   notification_cfg->handler = ecore_event_handler_add
         (E_EVENT_CONFIG_MODE_CHANGED, (Ecore_Event_Handler_Cb)_notification_cb_config_mode_changed,
         notification_cfg);
   notification_mod = m;

   notification_cfg->clicked_item = EINA_FALSE;
   notification_cfg->new_item = 0;

   e_gadcon_provider_register(&_gadcon_class);
   // Should not happen in normal usage but happened during development
   //   running code at different commits, was NULL later segfaults.
   if (!notification_cfg->blacklist)
      notification_cfg->blacklist = eina_stringshare_add("");
      
   notification_cfg->hist = history_init();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m EINA_UNUSED)
{
   if (notification_cfg->jump_timer)
     ecore_timer_del(notification_cfg->jump_timer);
   if (notification_cfg->cfd) e_object_del(E_OBJECT(notification_cfg->cfd));
   e_configure_registry_item_del("extensions/notification");
   e_configure_registry_category_del("extensions");

   notification_popup_shutdown();
   e_notification_server_unregister();

   _notification_cfg_free(notification_cfg);
   E_CONFIG_DD_FREE(conf_edd);
   notification_mod = NULL;
   return 1;
}

static Config *
_notification_cfg_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);
   cfg->cfd = NULL;
   cfg->show_low = 1;
   cfg->show_normal = 1;
   cfg->show_critical = 1;
   cfg->timeout = 5.0;
   cfg->force_timeout = 0;
   cfg->ignore_replacement = 0;
   cfg->dual_screen = 0;
   cfg->corner = CORNER_TR;
   cfg->time_stamp = 1;
   cfg->show_app = 0;
   cfg->show_count = 1;
   cfg->reverse = 0;
   cfg->item_length = 60;
   cfg->menu_items = 20;
   cfg->jump_delay = 10;
   cfg->blacklist = eina_stringshare_add("EPulse");

   return cfg;
}

static void
_notification_cfg_free(Config *cfg)
{
   E_FREE(cfg);
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return e_config_domain_save("module.notification", conf_edd, notification_cfg);
}



/* Gadget API functions */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;


   inst = E_NEW(Instance, 1);
   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/notification",
                           "e/modules/notification/logo");

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_notif = o;

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _button_cb_mouse_down, inst);
   notification_cfg->instances =
     eina_list_append(notification_cfg->instances, inst);
   gadget_text(notification_cfg->new_item);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   if (inst->menu) {
    e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
    e_object_del(E_OBJECT(inst->menu));
    inst->menu = NULL;
  }
   if (notification_cfg)
     notification_cfg->instances = eina_list_remove(notification_cfg->instances, inst);
   evas_object_del(inst->o_notif);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set (gcc, 16, 16);
   e_gadcon_client_min_size_set (gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Notification");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj",
            e_module_dir_get(notification_mod));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
   static char buf[4096];

   snprintf(buf, sizeof(buf), "%s.%d", client_class->name,
            eina_list_count(notification_cfg->instances) + 1);
   return buf;
}

/* Gadget support functions */

static void
_mute_cb(void)
{
   Instance *inst = NULL;
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);

   notification_cfg->mute = !notification_cfg->mute;

   inst = eina_list_data_get(notification_cfg->instances);
   if (notification_cfg->mute)
     edje_object_part_text_set(inst->o_notif, "e.text.counter", "X");
   else
     edje_object_part_text_set(inst->o_notif, "e.text.counter", "");
}

static void
_button_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(event_info);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);

   Popup_Items *items = NULL;

   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   notification_cfg->clicked_item = EINA_FALSE;

   inst = data;
   ev = event_info;
   if (ev->button == 3)  //right button
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int cx, cy;

        m = e_menu_new();
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _cb_config_show, NULL);

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
                                          &cx, &cy, NULL, NULL);
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
   if (ev->button == 2)  //middle button
     _mute_cb();

   if (ev->button == 1)  //left button
      {
        unsigned dir = E_GADCON_ORIENT_VERT;
        Evas_Coord x, y, w, h;
        int cx, cy;

        evas_object_geometry_get(inst->o_notif, &x, &y, &w, &h);
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy,
                                          NULL, NULL);
        x += cx;
        y += cy;

        E_Menu_Item *mi;
        inst->menu = e_menu_new();
        Eina_List *l = NULL;

        if (notification_cfg->hist->history)
        {
          if (notification_cfg->reverse)
            notification_cfg->hist->history = eina_list_reverse(notification_cfg->hist->history);

          EINA_LIST_FOREACH(notification_cfg->hist->history, l, items) {
             mi = e_menu_item_new(inst->menu);
             Eina_Strbuf *buff = eina_strbuf_new();
             int i;

             if (notification_cfg->time_stamp){
                eina_strbuf_append(buff, items->item_date_time);
                eina_strbuf_append(buff, " ");
             }
             if (notification_cfg->show_app){
                eina_strbuf_append(buff, items->item_app);
                eina_strbuf_append(buff, ": ");
             }
             eina_strbuf_append(buff, items->item_title);
             eina_strbuf_append(buff, ", ");
             eina_strbuf_append(buff, items->item_body);
             eina_strbuf_append(buff, " ");
             eina_strbuf_replace_all(buff, "\n", " ");

             if (eina_strbuf_length_get(buff) <  notification_cfg->item_length)
               i = eina_strbuf_length_get(buff);
             else
               i = notification_cfg->item_length;

             e_menu_item_label_set(mi, evas_textblock_text_markup_to_utf8(NULL,
                         eina_strbuf_string_get(eina_strbuf_substr_get(buff, 0, i))));

             eina_strbuf_free(buff);

             notification_cfg->new_item = 0;
             gadget_text(notification_cfg->new_item);
             e_menu_item_disabled_set(mi, EINA_FALSE);
             e_menu_item_callback_set(mi, (E_Menu_Cb)_cb_menu_item, items);

             if (strlen(items->item_icon) > 0)
               e_util_menu_item_theme_icon_set(mi, items->item_icon);
             else
               e_menu_item_icon_file_set(mi,  items->item_icon_img);
           }
          }
          else
          {
            mi = e_menu_item_new(inst->menu);
            e_menu_item_label_set(mi, _("Empty"));
            e_menu_item_disabled_set(mi, EINA_TRUE);
          }

        mi = e_menu_item_new(inst->menu);
        e_menu_item_separator_set(mi, EINA_TRUE);

        mi = e_menu_item_new(inst->menu);
        e_menu_item_label_set(mi, _("Clear"));
        e_util_menu_item_theme_icon_set(mi, "edit-clear");
        e_menu_item_callback_set(mi, (E_Menu_Cb) _clear_menu_cb, notification_cfg->hist->history);

        if (notification_cfg->hist->history)
          e_menu_item_disabled_set(mi, EINA_FALSE);
        else
          e_menu_item_disabled_set(mi, EINA_TRUE);

        mi = e_menu_item_new(inst->menu);
        e_menu_item_label_set(mi, _("Mute"));
        e_menu_item_check_set(mi, 1);
        e_util_menu_item_theme_icon_set(mi, "audio-volume-muted");
        if (notification_cfg->mute)
          e_menu_item_toggle_set(mi, 1);
        else
          e_menu_item_toggle_set(mi, 0);
        e_menu_item_callback_set(mi, (E_Menu_Cb) _mute_cb, notification_cfg->hist->history);

        mi = e_menu_item_new(inst->menu);
        e_menu_item_separator_set(mi, EINA_TRUE);

        mi = e_menu_item_new(inst->menu);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _cb_config_show, NULL);

        if (ev) {
          e_menu_post_deactivate_callback_set(inst->menu, _cb_menu_post_deactivate, inst);

          switch (inst->gcc->gadcon->orient) {
          case E_GADCON_ORIENT_TOP:
          case E_GADCON_ORIENT_CORNER_TL:
          case E_GADCON_ORIENT_CORNER_TR:
            dir = E_MENU_POP_DIRECTION_DOWN;
            break;

          case E_GADCON_ORIENT_BOTTOM:
          case E_GADCON_ORIENT_CORNER_BL:
          case E_GADCON_ORIENT_CORNER_BR:
            dir = E_MENU_POP_DIRECTION_UP;
            break;

          case E_GADCON_ORIENT_LEFT:
          case E_GADCON_ORIENT_CORNER_LT:
          case E_GADCON_ORIENT_CORNER_LB:
            dir = E_MENU_POP_DIRECTION_RIGHT;
            break;

          case E_GADCON_ORIENT_RIGHT:
          case E_GADCON_ORIENT_CORNER_RT:
          case E_GADCON_ORIENT_CORNER_RB:
            dir = E_MENU_POP_DIRECTION_LEFT;
            break;

          case E_GADCON_ORIENT_FLOAT:
          case E_GADCON_ORIENT_HORIZ:
          case E_GADCON_ORIENT_VERT:
            default:
            dir = E_MENU_POP_DIRECTION_AUTO;
           break;
          }
       }
       e_menu_activate_mouse(inst->menu,
                        e_util_zone_current_get(e_manager_current_get()),
                        x, y, w, h, dir, ev->timestamp);
    }
}

static void
_cb_menu_item(void *selected_item, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{  // fix me 
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg->hist);
   EINA_SAFETY_ON_NULL_RETURN(selected_item);

   Popup_Items *sel_item = (Popup_Items *) selected_item;
   Eina_Stringshare *temp_icon = NULL;
   int check = 0;

   PRINT("MENU ITEM CALL BACK %ld %p %p \n", strlen(sel_item->item_icon), sel_item->item_icon, sel_item->item_icon_img);
   notification_cfg->clicked_item = EINA_TRUE;
   /* remove the current item from the list */
   notification_cfg->hist->history = eina_list_remove(notification_cfg->hist->history, sel_item);
   // Fixme: icons are now much more compilcated
   if (strlen(sel_item->item_icon) > 0)
     temp_icon = eina_stringshare_add(sel_item->item_icon);
   else if (sel_item->item_icon_img)
   {
     temp_icon = eina_stringshare_add(sel_item->item_icon_img);
     check = 1;
   }

   /* show the current item as notification */
   // FIXME: ACTIONS
   _notification_show_actions(sel_item, temp_icon);

   if (check) ecore_file_remove(sel_item->item_icon_img);

   if (!notification_cfg->hist->history)
      notification_cfg->clicked_item = EINA_FALSE;

   eina_stringshare_del(temp_icon);
}

static void
_cb_menu_post_deactivate(void *data, E_Menu *menu __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);
 
   Instance *inst = data;

   if (inst->gcc)
      e_gadcon_locked_set(inst->gcc->gadcon, EINA_FALSE);
   if (inst->menu)
    {
      e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
      e_object_del(E_OBJECT(inst->menu));
      inst->menu = NULL;
    }

   if (notification_cfg->reverse && notification_cfg->hist->history)
      notification_cfg->hist->history = eina_list_reverse(notification_cfg->hist->history);
}


static void
clear_menu(void)
{
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg->hist);
   if (notification_cfg->hist->history)
     E_FREE_LIST(notification_cfg->hist->history, popup_items_free);
   if (notification_cfg->secure_clear)
      store_history(notification_cfg->hist);
}

static void
_clear_menu_cb(void)
{
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg->hist);
   Popup_Items *items;
   Eina_List *l;

   EINA_LIST_FOREACH(notification_cfg->hist->history, l, items) {
     if (ecore_file_exists(items->item_icon_img))
       {
         if (!ecore_file_remove(items->item_icon_img))
           printf("Notif: Error during files removing!\n");
       }
   }

   clear_menu();
}

/* Actual Notification functions */

static unsigned int
_notification_notify(E_Notification_Notify *n)
{
   unsigned int new_id;
   PRINT("notification_notify\n");

   if (notification_cfg->mute)
     return 0;

   // if (e_desklock_state_get()) return 0;

   notification_cfg->next_id++;
   new_id = notification_cfg->next_id;
   notification_popup_notify(n, new_id);

   return new_id;
}

static void
_notification_show_common(const char *summary,
                          const char *body,
                          int         replaces_id)
{
   E_Notification_Notify n;
   memset(&n, 0, sizeof(E_Notification_Notify));
   n.app_name = "Moksha";
   n.replaces_id = replaces_id;
   n.icon.icon = "enlightenment";
   n.summary = summary;
   n.body = body;
   n.urgency = E_NOTIFICATION_NOTIFY_URGENCY_CRITICAL;
   e_notification_client_send(&n, NULL, NULL);
}

static void
_notification_show_presentation(Eina_Bool enabled)
{
   if(getenv("MOKSHA_NO_NOTIFY")) return;
   const char *summary, *body;

   if (enabled)
     {
        summary = _("Enter Presentation Mode");
        body = _("Moksha is in <b>presentation</b> mode.");
     }
   else
     {
        summary = _("Exited Presentation Mode");
        body = _("Presentation mode is over.");
     }

   _notification_show_common(summary, body, -1);
}

static void
_notification_show_offline(Eina_Bool enabled)
{
   if(getenv("MOKSHA_NO_NOTIFY")) return;
   const char *summary, *body;

   if (enabled)
     {
        summary = _("Enter Offline Mode");
        body = _("Moksha is in <b>offline</b> mode.<br>"
                 "During offline mode, modules that use network will stop "
                 "polling remote services.");
     }
   else
     {
        summary = _("Exited Offline Mode");
        body = _("Now in <b>online</b> mode.<br>"
                 "Now modules that use network will "
                 "resume regular tasks.");
     }

   _notification_show_common(summary, body, -1);
}


/* Callbacks */

static void
_cb_config_show(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (!notification_cfg) return;
   if (notification_cfg->cfd) return;
   e_int_config_notification_module(m->zone->container, NULL);
}

static unsigned int
_notification_cb_notify(void *data __UNUSED__, E_Notification_Notify *n)
{
   return _notification_notify(n);
}

static void
_notification_cb_close(void *data __UNUSED__, unsigned int id)
{
   notification_popup_close(id);
}

static Eina_Bool
_notification_cb_config_mode_changed(Config *m_cfg,
                                     int   type __UNUSED__,
                                     void *event __UNUSED__)
{
   if (m_cfg->last_config_mode.presentation != e_config->mode.presentation)
     {
        m_cfg->last_config_mode.presentation = e_config->mode.presentation;
        _notification_show_presentation(e_config->mode.presentation);
     }

   if (m_cfg->last_config_mode.offline != e_config->mode.offline)
     {
        m_cfg->last_config_mode.offline = e_config->mode.offline;
        _notification_show_offline(e_config->mode.offline);
     }

   return EINA_TRUE;
}

// FIXME: for present usage badly named

static void
_notification_show_actions(Popup_Items *sel_item, const char *icon)
{  //FIXME can i just use old n?
   E_Notification_Notify *n = E_NEW(E_Notification_Notify, 1);

   PRINT("Show actions %p\n", icon);
   n->app_name = sel_item->item_app;
   n->replaces_id = sel_item->notif_id;
   n->icon.icon = icon;
   n->summary = sel_item->item_title;
   n->body = sel_item->item_body;
   n->urgency = sel_item->urgency;
   n->category = sel_item->category;
   n->desktop_entry = sel_item->desktop_entry;
   n->sound_file = sel_item->sound_file;
   n->sound_name = sel_item->sound_name;
   n->x = sel_item->x;
   n->y = sel_item->y;
   n->timeout=-1;
 // FIXME: actions
   if (sel_item->actions)
     {  E_Notification_Notify_Action *act;
        E_Notification_Notify_Action *actions;
        Eina_List *l;
        int num = 0;
        EINA_LIST_FOREACH(sel_item->actions, l, act)
        {
           num++;
           actions = realloc(n->actions, (num + 1) * sizeof(E_Notification_Notify));
           if (actions)
            {
               n->actions = actions;
               n->actions[num - 1].action = eina_stringshare_add(act->action);
               n->actions[num - 1].label = eina_stringshare_add(act->label);
               n->actions[num].action = NULL;
               n->actions[num].label = NULL;
            }
        }
     }
   _notification_notify(n);
}

/* FIX ME */
// Should be in a history.c file

static Eina_Bool
_notif_delay_stop_cb(Instance *inst)
{
   edje_object_signal_emit(inst->o_notif, "stop", "");
   return EINA_FALSE;
}

void
gadget_text(int number)
{
   Instance *inst = NULL;
   char *buf = (char *) malloc(sizeof(char) * HIST_MAX_DIGITS + 1);

   eina_convert_itoa(number, buf);
   if (!notification_cfg->instances) return;
   inst = eina_list_data_get(notification_cfg->instances);
   PRINT("Gadget test %d %s\n\n", number, buf);
   if (number > 0 && notification_cfg->show_count)
     edje_object_part_text_set(inst->o_notif, "e.text.counter", buf);
   else
     edje_object_part_text_set(inst->o_notif, "e.text.counter", "");

   if (notification_cfg->mute)
     edje_object_part_text_set(inst->o_notif, "e.text.counter", "X");

   if (notification_cfg->jump_timer)
   { //FIXME: You are trying to destroy a timer which seems dead already.
     //ecore_timer_del(notification_cfg->jump_timer);
     notification_cfg->jump_timer = NULL;
   }

   if ((notification_cfg->new_item) && (notification_cfg->jump_delay > 0))
   {
     edje_object_signal_emit(inst->o_notif, "blink", "");
     notification_cfg->jump_timer = ecore_timer_add(notification_cfg->jump_delay,
                                   (Ecore_Task_Cb)_notif_delay_stop_cb, inst);
   }
   else
     edje_object_signal_emit(inst->o_notif, "stop", "");

   E_FREE(buf);
}

