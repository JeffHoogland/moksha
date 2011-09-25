#include "e.h"
#include "e_mod_main.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _cb_method_change(void *data, Evas_Object *obj, void *event_info);
static void _cb_login_change(void *data, Evas_Object *obj);
static int _zone_count_get(void);

static void _cb_bg_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _cb_ask_presentation_changed(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd, *bg_fsel;

   /* Common vars */
   int use_xscreensaver;
   int fmdir;
   int zone_count;

   /* Locking */
   int start_locked;
   int lock_on_suspend;
   int auto_lock;
   int locking_method;
   int login_zone;
   int zone;
   char *custom_lock_cmd;
   
   /* Timers */
   int screensaver_lock;
   double idle_time;
   double post_screensaver_time;

   /* Adv props */
   int bg_method;
   const char *bg;
   int custom_lock;
   int ask_presentation;
   double ask_presentation_timeout;

   struct
     {
        Evas_Object *loginbox_slider;
        Evas_Object *post_screensaver_slider;
        Evas_Object *auto_lock_slider;
        Evas_Object *ask_presentation_slider;
        Evas_Object *o_bg;
     } gui;
};


E_Config_Dialog *
e_int_config_desklock(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "screen/screen_lock")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("Screen Lock Settings"), "E",
			     "screen/screen_lock", "preferences-system-lock-screen",
			     0, v, NULL);
   return cfd;
}

void 
e_int_config_desklock_fsel_done(E_Config_Dialog *cfd, const char *bg_file) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = cfd->cfdata)) return;
   cfdata->bg_fsel = NULL;
   if (bg_file) 
     {
        eina_stringshare_replace(&cfdata->bg, bg_file);
        e_widget_preview_edje_set(cfdata->gui.o_bg, cfdata->bg, 
                                  "e/desktop/background");
     }
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->fmdir = 0;
   if (e_config->desklock_background)
     {
	cfdata->bg = eina_stringshare_ref(e_config->desklock_background);
	if (!strstr(cfdata->bg, e_user_homedir_get()))
	  cfdata->fmdir = 1;
     }
   else
     cfdata->bg = eina_stringshare_add("theme_desklock_background");

   if (!strcmp(cfdata->bg, "theme_desklock_background"))
     cfdata->bg_method = 0;
   else if (!strcmp(cfdata->bg, "theme_background"))
     cfdata->bg_method = 1;
   else if (!strcmp(cfdata->bg, "user_background"))
     cfdata->bg_method = 2;
   else
     cfdata->bg_method = 3;

   cfdata->use_xscreensaver = ecore_x_screensaver_event_available_get();
   cfdata->zone_count = _zone_count_get();

   cfdata->custom_lock = e_config->desklock_use_custom_desklock;
   if (e_config->desklock_custom_desklock_cmd)
     cfdata->custom_lock_cmd = strdup(e_config->desklock_custom_desklock_cmd);

   cfdata->start_locked = e_config->desklock_start_locked;
   cfdata->lock_on_suspend = e_config->desklock_on_suspend;
   cfdata->auto_lock = e_config->desklock_autolock_idle;
   cfdata->screensaver_lock = e_config->desklock_autolock_screensaver;
   cfdata->idle_time = e_config->desklock_autolock_idle_timeout / 60;
   cfdata->post_screensaver_time = e_config->desklock_post_screensaver_time;
   if (e_config->desklock_login_box_zone >= 0)
     {
	cfdata->login_zone = 0;
	cfdata->zone = e_config->desklock_login_box_zone;
     }
   else
     {
	cfdata->login_zone = e_config->desklock_login_box_zone;
	cfdata->zone = 0;
     }

   cfdata->ask_presentation = e_config->desklock_ask_presentation;
   cfdata->ask_presentation_timeout = 
     e_config->desklock_ask_presentation_timeout;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->bg_fsel) 
     e_object_del(E_OBJECT(cfdata->bg_fsel));
   E_FREE(cfdata->custom_lock_cmd);
   if (cfdata->bg) eina_stringshare_del(cfdata->bg);
   E_FREE(cfdata);
}

static void
_basic_auto_lock_cb_changed(void *data, Evas_Object *o __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   int disable;

   if (!(cfdata = data)) return;
   disable = ((!cfdata->use_xscreensaver) || (!cfdata->auto_lock));
   e_widget_disabled_set(cfdata->gui.auto_lock_slider, disable);
}

static void
_basic_screensaver_lock_cb_changed(void *data, Evas_Object *o __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   int disable;

   if (!(cfdata = data)) return;
   disable = ((!cfdata->use_xscreensaver) || (!cfdata->screensaver_lock));
   e_widget_disabled_set(cfdata->gui.post_screensaver_slider, disable);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ow, *of;
   E_Radio_Group *rg;
   E_Zone *zone;
   int screen_count;

   zone = e_zone_current_get(cfd->con);
   screen_count = ecore_x_xinerama_screen_count_get();

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   /* Locking */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Lock on Startup"), &cfdata->start_locked);
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_check_add(evas, _("Lock on Suspend"), &cfdata->lock_on_suspend);
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Custom Screenlock Command"), 0);
   ow = e_widget_entry_add(evas, &(cfdata->custom_lock_cmd), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Locking"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Login */
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->login_zone));
   ow = e_widget_radio_add(evas, _("Show on all screens"), -1, rg);
   e_widget_on_change_hook_set(ow, _cb_login_change, cfdata);
   e_widget_disabled_set(ow, (screen_count <= 0));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_radio_add(evas, _("Show on current screen"), -2, rg);
   e_widget_on_change_hook_set(ow, _cb_login_change, cfdata);
   e_widget_disabled_set(ow, (screen_count <= 0));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_radio_add(evas, _("Show on screen #:"), 0, rg);
   e_widget_on_change_hook_set(ow, _cb_login_change, cfdata);
   e_widget_disabled_set(ow, (screen_count <= 0));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   cfdata->gui.loginbox_slider = 
     e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, (cfdata->zone_count - 1),
                         1.0, 0, NULL, &(cfdata->zone), 100);
   e_widget_disabled_set(cfdata->gui.loginbox_slider, (screen_count <= 0));
   e_widget_list_object_append(ol, cfdata->gui.loginbox_slider, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Login Box"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Timers */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Lock after X screensaver activates"),
			   &cfdata->screensaver_lock);
   e_widget_on_change_hook_set(ow, _basic_screensaver_lock_cb_changed, cfdata);
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"), 0.0, 300.0, 10.0, 0,
			    &(cfdata->post_screensaver_time), NULL, 100);
   cfdata->gui.post_screensaver_slider = ow;
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_check_add(evas, _("Lock when idle time exceeded"),
			   &cfdata->auto_lock);
   e_widget_on_change_hook_set(ow, _basic_auto_lock_cb_changed, cfdata);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0,
			    &(cfdata->idle_time), NULL, 100);
   cfdata->gui.auto_lock_slider = ow;
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Timers"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Presentation */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Suggest if deactivated before"),
                           &(cfdata->ask_presentation));
   e_widget_on_change_hook_set(ow, _cb_ask_presentation_changed, cfdata);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"), 1.0, 300.0, 10.0, 0,
                            &(cfdata->ask_presentation_timeout), NULL, 100);
   cfdata->gui.ask_presentation_slider = ow;
   e_widget_disabled_set(ow, (!cfdata->ask_presentation));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Presentation Mode"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Wallpapers */
   ol = e_widget_list_add(evas, 0, 0);
   of = e_widget_table_add(evas, 1);
   rg = e_widget_radio_group_new(&(cfdata->bg_method));
   ow = e_widget_radio_add(evas, _("Theme Defined"), 0, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Theme Wallpaper"), 1, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Current Wallpaper"), 2, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 1, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Custom"), 3, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 1, 1, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   ow = e_widget_preview_add(evas, 100, 140);
   cfdata->gui.o_bg = ow;
   e_widget_disabled_set(cfdata->gui.o_bg, (cfdata->bg_method < 3));
   evas_object_event_callback_add(ow, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _cb_bg_mouse_down, cfdata);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Wallpaper"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   _basic_auto_lock_cb_changed(cfdata, NULL);
   _basic_screensaver_lock_cb_changed(cfdata, NULL);

   return otb;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->desklock_start_locked = cfdata->start_locked;
   e_config->desklock_on_suspend = cfdata->lock_on_suspend;
   e_config->desklock_autolock_idle = cfdata->auto_lock;
   e_config->desklock_post_screensaver_time = cfdata->post_screensaver_time;
   e_config->desklock_autolock_screensaver = cfdata->screensaver_lock;
   e_config->desklock_autolock_idle_timeout = (cfdata->idle_time * 60);
   e_config->desklock_ask_presentation = cfdata->ask_presentation;
   e_config->desklock_ask_presentation_timeout = 
     cfdata->ask_presentation_timeout;

   if (cfdata->bg)
     {
	if (e_config->desklock_background)
	  e_filereg_deregister(e_config->desklock_background);
	eina_stringshare_replace(&e_config->desklock_background, cfdata->bg);
	e_filereg_register(e_config->desklock_background);
     }

   if (cfdata->login_zone < 0)
     e_config->desklock_login_box_zone = cfdata->login_zone;
   else
     e_config->desklock_login_box_zone = cfdata->zone;

   e_config->desklock_use_custom_desklock = cfdata->custom_lock;
   if (cfdata->custom_lock_cmd)
     eina_stringshare_replace(&e_config->desklock_custom_desklock_cmd, 
                              cfdata->custom_lock_cmd);

   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (e_config->desklock_start_locked != cfdata->start_locked)
     return 1;

   if (e_config->desklock_on_suspend != cfdata->lock_on_suspend)
     return 1;

   if (e_config->desklock_autolock_idle != cfdata->auto_lock)
     return 1;

   if (e_config->desklock_autolock_screensaver != cfdata->screensaver_lock)
     return 1;

   if (e_config->desklock_post_screensaver_time !=
       cfdata->post_screensaver_time)
     return 1;

   if (e_config->desklock_autolock_idle_timeout != cfdata->idle_time * 60)
     return 1;

   if (e_config->desklock_background != cfdata->bg)
     return 1;

   if (cfdata->login_zone < 0)
     {
	if (e_config->desklock_login_box_zone != cfdata->login_zone)
	  return 1;
     }
   else
     {
	if (e_config->desklock_login_box_zone != cfdata->zone)
	  return 1;
     }

   if (e_config->desklock_use_custom_desklock != cfdata->custom_lock)
     return 1;

   if (e_config->desklock_custom_desklock_cmd && cfdata->custom_lock_cmd)
     {
	if (strcmp(e_config->desklock_custom_desklock_cmd, cfdata->custom_lock_cmd) != 0)
	  return 1;
     }
   else if (e_config->desklock_custom_desklock_cmd != cfdata->custom_lock_cmd)
     return 1;

   return ((e_config->desklock_ask_presentation != cfdata->ask_presentation) ||
	   (e_config->desklock_ask_presentation_timeout != cfdata->ask_presentation_timeout));
}

static void
_cb_method_change(void *data, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *theme = NULL;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.o_bg, (cfdata->bg_method < 3));
   switch(cfdata->bg_method)
     {
      case 0:
	eina_stringshare_replace(&cfdata->bg, "theme_desklock_background");
        theme = e_theme_edje_file_get("base/theme/desklock", 
                                      "e/desklock/background");
        if (theme)
          e_widget_preview_edje_set(cfdata->gui.o_bg, theme, 
                                    "e/desklock/background");
	break;
      case 1:
	eina_stringshare_replace(&cfdata->bg, "theme_background");
        theme = e_theme_edje_file_get("base/theme/backgrounds", 
                                      "e/desktop/background");
        if (theme)
          e_widget_preview_edje_set(cfdata->gui.o_bg, theme, 
                                    "e/desktop/background");
	break;
      case 2:
	eina_stringshare_replace(&cfdata->bg, "user_background");
        if (e_config->desktop_default_background)
          theme = e_config->desktop_default_background;
        else 
          {
             const E_Config_Desktop_Background *cbg;
             const Eina_List *l;

             EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cbg)
               {
                  if (cbg->file) 
                    {
                       theme = cbg->file;
                       break;
                    }
               }
          }
        if (theme)
          e_widget_preview_edje_set(cfdata->gui.o_bg, theme, 
                                    "e/desktop/background");
	break;
      default:
        e_widget_preview_edje_set(cfdata->gui.o_bg, cfdata->bg, 
                                  "e/desktop/background");
        break;
     }
}

static void
_cb_login_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.loginbox_slider, (cfdata->login_zone < 0));
}

static int
_zone_count_get(void)
{
   int num = 0;
   E_Manager *m;
   Eina_List *ml;

   EINA_LIST_FOREACH(e_manager_list(), ml, m) 
     {
        Eina_List *cl;
        E_Container *con;

        EINA_LIST_FOREACH(m->containers, cl, con) 
          num += eina_list_count(con->zones);
     }
   return num;
}

static void 
_cb_bg_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   if (e_widget_disabled_get(obj)) return;
   if (!(cfdata = data)) return;
   if (cfdata->bg_fsel) 
     e_win_raise(cfdata->bg_fsel->dia->win);
   else
     cfdata->bg_fsel = e_int_config_desklock_fsel(cfdata->cfd);
}

static void
_cb_ask_presentation_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_Bool disable;

   if (!(cfdata = data)) return;
   disable = (!cfdata->ask_presentation);
   e_widget_disabled_set(cfdata->gui.ask_presentation_slider, disable);
}
