#include "e_mod_main.h"

/* gadcon requirements */
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
static int              _notification_cb_notify(E_Notification_Daemon *daemon, E_Notification *n);
static void             _notification_cb_close_notification(E_Notification_Daemon *daemon,
                                                unsigned int id);
static void             _cb_menu_item(void *selected_item, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__);
static void             _clear_menu_cb(void);
static int               read_items_eet(Eina_List **popup_items);


/* Config function protos */
static Config *_notification_cfg_new(void);
static void    _notification_cfg_free(Config *cfg);

/* Global variables */
E_Module *notification_mod = NULL;
//~ E_Module *notification_cfg->module = NULL;

Config *notification_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "notification",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* actual module specifics */


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

static int
read_items_eet(Eina_List **popup_items)
{
  int size = 0;
  char str[10];
  char *ret;
  char file_path[PATH_MAX];
  Eina_List *l = NULL;
  unsigned int items_number, i;
  Popup_Items *items = NULL;
  Eet_File *history_file = NULL;
  
  snprintf(file_path, sizeof(file_path), "%s/notification/notif_list", efreet_data_home_get()); 
  
  history_file = eet_open(file_path, EET_FILE_MODE_READ);
  if (!history_file) {
      printf("Failed to open notification eet file\n");
      *popup_items = NULL;
      return 0;
    }
  /* Read Number of items */
  ret = eet_read(history_file, "ITEMS", &size);
  
  if (!ret) {
      printf("Notification file corruption\n");
      *popup_items = NULL;
      return eet_close(history_file);
  }
   items_number = strtol(ret, NULL, 10);
   
   for (i = 1; i <= items_number; i++){
        items = E_NEW(Popup_Items, 1);
        snprintf(str, 10, "dtime%d", i);
        ret = eet_read(history_file, str, &size);
        items->item_date_time = strdup(ret);
        snprintf(str, 10, "app%d", i);
        ret = eet_read(history_file, str, &size);
        items->item_app = strdup(ret);
        snprintf(str, 10, "icon%d", i);
        ret = eet_read(history_file, str, &size);
        items->item_icon = strdup(ret);
        snprintf(str, 10, "img%d", i);
        ret = eet_read(history_file, str, &size);
        items->item_icon_img = strdup(ret);
        snprintf(str, 10, "title%d", i);
        ret = eet_read(history_file, str, &size);
        items->item_title = strdup(ret);
        snprintf(str, 10, "body%d", i);
        ret = eet_read(history_file, str, &size);
        items->item_body = strdup(ret);
        
        l = eina_list_append(l, items);
    }
   *popup_items = l;  
  eet_close(history_file);
return 1; 
}

static void
_button_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(event_info);
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
        
        if (notification_cfg->popup_items)
        {
          if (notification_cfg->reverse)
            notification_cfg->popup_items = eina_list_reverse(notification_cfg->popup_items);
        
          EINA_LIST_FOREACH(notification_cfg->popup_items, l, items) {
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
        e_menu_item_callback_set(mi, (E_Menu_Cb) _clear_menu_cb, notification_cfg->popup_items);
        
        if (notification_cfg->popup_items)
           e_menu_item_disabled_set(mi, EINA_FALSE);
        else
           e_menu_item_disabled_set(mi, EINA_TRUE);
        
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
_cb_menu_post_deactivate(void *data, E_Menu *menu __UNUSED__)
{
  EINA_SAFETY_ON_NULL_RETURN(data);
  Instance *inst = data;

  if (inst->gcc)
     e_gadcon_locked_set(inst->gcc->gadcon, EINA_FALSE);
  //~ if (inst->o_notif && e_icon_edje_get(inst->o_notif))
     //~ e_icon_edje_emit(inst->o_button, "e,state,unfocused", "e");
  if (inst->menu) {
    e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
    e_object_del(E_OBJECT(inst->menu));
    inst->menu = NULL;
  }
  if (notification_cfg->reverse)
     notification_cfg->popup_items = eina_list_reverse(notification_cfg->popup_items);
}

static void
_cb_config_show(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
  if (!notification_cfg) return;
  if (notification_cfg->cfd) return;
  e_int_config_notification_module(m->zone->container, NULL);
} 

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
  char *buf = (char *)malloc(sizeof(char) * number);   
  
  if (!notification_cfg->instances) return;
  inst = eina_list_data_get(notification_cfg->instances);  
  
  snprintf(buf, sizeof(number), "%d", number);

  if (number > 0)
     edje_object_part_text_set(inst->o_notif, "e.text.counter", buf); 
  else
     edje_object_part_text_set(inst->o_notif, "e.text.counter", ""); 
  
  if (notification_cfg->jump_timer){
      ecore_timer_del(notification_cfg->jump_timer);
      notification_cfg->jump_timer = NULL;
  }
  
  if ((notification_cfg->new_item) && (notification_cfg->jump_delay > 0)){
       edje_object_signal_emit(inst->o_notif, "blink", "");
       notification_cfg->jump_timer = ecore_timer_add(notification_cfg->jump_delay, 
                     (Ecore_Task_Cb)_notif_delay_stop_cb, inst);
  }
  else
    edje_object_signal_emit(inst->o_notif, "stop", "");
    
  free(buf);
}

static unsigned int
_notification_notify(E_Notification *n)
{ 
   const char *appname;
   unsigned int replaces_id, new_id;
   int popuped;

   if (e_desklock_state_get()) return 0;
   appname = e_notification_app_name_get(n);
   replaces_id = e_notification_replaces_id_get(n);
   if (replaces_id) new_id = replaces_id;
   else new_id = notification_cfg->next_id++;
  
   e_notification_id_set(n, new_id);
   popuped = notification_popup_notify(n, replaces_id, appname);
   if (!popuped)
     {
        e_notification_hint_urgency_set(n, 4);
        notification_popup_notify(n, replaces_id, appname);
     }
     
   return new_id;
}

static void
_notification_show_common(const char *summary,
                          const char *body,
                          const char *icon,
                          int         replaces_id)
{
   E_Notification *n = e_notification_full_new
       ("enlightenment", replaces_id, icon, summary, body, -1);
   if (!n)
     return;

   _notification_notify(n);
   e_notification_unref(n);
}

static void
_cb_menu_item(void *selected_item, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Popup_Items *sel_item = (Popup_Items *) selected_item;
   
   notification_cfg->clicked_item = EINA_TRUE;
   /* remove the current item from the list */
   notification_cfg->popup_items = eina_list_remove(notification_cfg->popup_items, sel_item);
   
   /* show the current item as notification */
   if (strlen(sel_item->item_icon) > 0)
     _notification_show_common(sel_item->item_title, sel_item->item_body, sel_item->item_icon, 0);
   else if (sel_item->item_icon_img){
     _notification_show_common(sel_item->item_title, sel_item->item_body, sel_item->item_icon_img, 0);
     ecore_file_remove(sel_item->item_icon_img);
   }

   if (!notification_cfg->popup_items)
      notification_cfg->clicked_item = EINA_FALSE;
}

void
free_menu_data(Popup_Items *items)
{
  EINA_SAFETY_ON_NULL_RETURN(items);
  free(items->item_app);
  free(items->item_body);
  free(items->item_date_time);
  free(items->item_icon);
  free(items->item_title);
  free(items);
}

static void
clear_menu(void)
{
  EINA_SAFETY_ON_NULL_RETURN(notification_cfg); 
  if (notification_cfg->popup_items)
    E_FREE_LIST(notification_cfg->popup_items, free_menu_data);
}

static void
_clear_menu_cb(void)
{
  EINA_SAFETY_ON_NULL_RETURN(notification_cfg); 
  Popup_Items *items;
  Eina_List *l;
  Eina_Bool ret;
  
  EINA_LIST_FOREACH(notification_cfg->popup_items, l, items) {
    ret = ecore_file_remove(items->item_icon_img);    
    if (!ret) 
       printf("Notif: Error during files removing!\n");
  }    
  clear_menu();
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

   _notification_show_common(summary, body, "enlightenment", -1);
}

static void
_notification_show_offline(Eina_Bool enabled)
{
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

   _notification_show_common(summary, body, "enlightenment", -1);
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

static Eina_Bool
_notification_cb_initial_mode_timer(Config *m_cfg)
{
   if (e_config->mode.presentation)
     _notification_show_presentation(1);
   if (e_config->mode.offline)
     _notification_show_offline(1);

   m_cfg->initial_mode_timer = NULL;
   return EINA_FALSE;
}

/* Module Api Functions */
EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Notification"};

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Notification_Daemon *d;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", m->dir);
   /* register config panel entry */
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add("extensions/notification", 30, 
                                 _("Notification"), NULL,
                                 buf, e_int_config_notification_module);


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
     {
        notification_cfg = _notification_cfg_new();
     }
     notification_cfg->version = MOD_CONFIG_FILE_VERSION;
   /* set up the notification daemon */
   e_notification_daemon_init();
   d = e_notification_daemon_add("e_notification_module", "Moksha");
   if (!d)
     {
        _notification_cfg_free(notification_cfg);
        notification_cfg = NULL;
        e_util_dialog_show(_("Error During DBus Init!"),
                           _("Error during DBus init! Please check if "
                              "dbus is correctly installed and running."));
        return NULL;
     }
   notification_cfg->daemon = d;
   e_notification_daemon_data_set(d, notification_cfg);
   e_notification_daemon_callback_notify_set(d, _notification_cb_notify);
   e_notification_daemon_callback_close_notification_set(d, _notification_cb_close_notification);

   notification_cfg->last_config_mode.presentation = e_config->mode.presentation;
   notification_cfg->last_config_mode.offline = e_config->mode.offline;
   notification_cfg->handler = ecore_event_handler_add
         (E_EVENT_CONFIG_MODE_CHANGED, (Ecore_Event_Handler_Cb)_notification_cb_config_mode_changed,
         notification_cfg);
   notification_cfg->initial_mode_timer = ecore_timer_add
       (0.1, (Ecore_Task_Cb)_notification_cb_initial_mode_timer, notification_cfg);
   
   e_gadcon_provider_register(&_gadcon_class);
   read_items_eet(&(notification_cfg->popup_items));
   
   notification_cfg->clicked_item = EINA_FALSE;
   notification_cfg->new_item = 0;
   //~ notification_cfg->module = m;
   notification_mod = m;
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   if (notification_cfg->initial_mode_timer)
     ecore_timer_del(notification_cfg->initial_mode_timer);
   
   if (notification_cfg->jump_timer)
     ecore_timer_del(notification_cfg->jump_timer);

   if (notification_cfg->handler)
     ecore_event_handler_del(notification_cfg->handler);

   if (notification_cfg->cfd) e_object_del(E_OBJECT(notification_cfg->cfd));
   e_configure_registry_item_del("extensions/notification");
   e_configure_registry_category_del("extensions");

   notification_popup_shutdown();

   e_notification_daemon_free(notification_cfg->daemon);
   e_notification_daemon_shutdown();
   _notification_cfg_free(notification_cfg);
   E_CONFIG_DD_FREE(conf_edd);
   notification_mod = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return e_config_domain_save("module.notification", conf_edd, notification_cfg);
}

/* Callbacks */
static int
_notification_cb_notify(E_Notification_Daemon *d __UNUSED__,
                        E_Notification        *n)
{
   return _notification_notify(n);
}

static void
_notification_cb_close_notification(E_Notification_Daemon *d __UNUSED__,
                                    unsigned int           id)
{
   notification_popup_close(id);
}

static Config *
_notification_cfg_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);
   cfg->cfd = NULL;
   cfg->version = MOD_CONFIG_FILE_VERSION;
   cfg->show_low = 0;
   cfg->show_normal = 1;
   cfg->show_critical = 1;
   cfg->timeout = 5.0;
   cfg->force_timeout = 0;
   cfg->ignore_replacement = 0;
   cfg->dual_screen = 0;
   cfg->corner = CORNER_TR;
   cfg->time_stamp = 1;
   cfg->show_app = 0;
   cfg->reverse = 0;
   cfg->item_length = 60;
   cfg->menu_items = 20;
   cfg->jump_delay = 10;
   cfg->blacklist = eina_stringshare_add("");

   return cfg;
}

static void
_notification_cfg_free(Config *cfg)
{
   if (cfg->blacklist) eina_stringshare_del(cfg->blacklist); 
   clear_menu();
   free(cfg);
}

