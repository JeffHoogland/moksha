#include "e.h"
#include "e_mod_main.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _fill_data(E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void         _cb_method_change(void *data, Evas_Object *obj, void *event_info);
static void         _cb_login_change(void *data, Evas_Object *obj);
static int          _zone_count_get(void);

static void         _cb_bg_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void         _cb_ask_presentation_changed(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd, *bg_fsel;

   /* Common vars */
   int              use_xscreensaver;
   int              zone_count;

   /* Locking */
   int              start_locked;
   int              lock_on_suspend;
   int              auto_lock;
   int              locking_method;
   int              login_zone;
   int              zone;
   char            *custom_lock_cmd;

   /* Layout */
   const char     *desklock_layout;

   /* Timers */
   int              screensaver_lock;
   double           idle_time;
   double           post_screensaver_time;

   /* Adv props */
   E_Desklock_Background_Method bg_method;
   int              bg_method_prev;
   Eina_List       *bgs;
   int              custom_lock;
   int              ask_presentation;
   double           ask_presentation_timeout;

   struct
   {
      Evas_Object *kbd_list;
      Evas_Object *loginbox_slider;
      Evas_Object *post_screensaver_slider;
      Evas_Object *auto_lock_slider;
      Evas_Object *ask_presentation_slider;
      Evas_Object *o_table;
      Eina_List *bgs;
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
e_int_config_desklock_fsel_done(E_Config_Dialog *cfd, Evas_Object *bg, const char *bg_file)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   const char *cbg;

   if (!(cfdata = cfd->cfdata)) return;
   cfdata->bg_fsel = NULL;
   if (!bg_file) return;
   e_widget_preview_file_get(bg, &cbg, NULL);
   l = eina_list_data_find_list(cfdata->bgs, cbg);
   if (l && l->data)
     eina_stringshare_replace((const char**)&l->data, bg_file);
   else
     eina_list_data_set(l, eina_stringshare_add(bg_file));
   e_widget_preview_edje_set(bg, bg_file, "e/desktop/background");
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Desklock_Background *bg;
   int x;

   cfdata->zone_count = _zone_count_get();
   EINA_LIST_FOREACH(e_config->desklock_backgrounds, l, bg)
     cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_ref(bg->file));
   if (!cfdata->bgs)
     for (x = 0; x < cfdata->zone_count; x++)
     cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_add("theme_desklock_background"));

   if (!e_util_strcmp(eina_list_data_get(cfdata->bgs), "theme_desklock_background"))
     cfdata->bg_method = E_DESKLOCK_BACKGROUND_METHOD_THEME_DESKLOCK;
   else if (!e_util_strcmp(eina_list_data_get(cfdata->bgs), "theme_background"))
     cfdata->bg_method = E_DESKLOCK_BACKGROUND_METHOD_THEME;
   else if (!e_util_strcmp(eina_list_data_get(cfdata->bgs), "user_background"))
     cfdata->bg_method = E_DESKLOCK_BACKGROUND_METHOD_WALLPAPER;
   else
     cfdata->bg_method = E_DESKLOCK_BACKGROUND_METHOD_CUSTOM;

   cfdata->bg_method_prev = cfdata->bg_method;
   cfdata->use_xscreensaver = ecore_x_screensaver_event_available_get();

   cfdata->custom_lock = e_config->desklock_use_custom_desklock;
   if (e_config->desklock_custom_desklock_cmd)
     cfdata->custom_lock_cmd = strdup(e_config->desklock_custom_desklock_cmd);

   cfdata->desklock_layout = e_config->xkb.desklock_layout;
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
   const char *bg;
   if (cfdata->bg_fsel)
     e_object_del(E_OBJECT(cfdata->bg_fsel));
   E_FREE(cfdata->custom_lock_cmd);
   EINA_LIST_FREE(cfdata->bgs, bg)
     eina_stringshare_del(bg);
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
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   E_Config_XKB_Layout *cl;
   int grp = 0;
   Evas_Object *otb, *ol, *ow, *of, *ot;
   Eina_List *l, *ll, *lll;
   E_Zone *zone;
   E_Radio_Group *rg;
   E_Manager *man;
   E_Container *con;
   int screen_count, x = 0;

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

   ow = e_widget_check_add(evas, _("Use Custom Screenlock Command"), &cfdata->custom_lock);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Locking"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Keyboard Layout */
   cfdata->gui.kbd_list = ol = e_widget_ilist_add(evas, 32 * e_scale, 32 * e_scale, &cfdata->desklock_layout);
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        Evas_Object *icon, *end;
        char buf[4096];
        const char *name = cl->name;

        end = edje_object_add(evas);
        if (e_theme_edje_object_set(end, "base/theme/widgets",
                                    "e/widgets/ilist/toggle_end"))
          {
             if (name == cfdata->desklock_layout)
               {
                  edje_object_signal_emit(end, "e,state,checked", "e");
                  e_widget_ilist_selected_set(ol, grp);
               }
             else
               edje_object_signal_emit(end, "e,state,unchecked", "e");
          }
        else
          {
             evas_object_del(end);
             end = NULL;
          }
        e_xkb_flag_file_get(buf, sizeof(buf), name);
        icon = e_icon_add(evas);
        if (!e_icon_file_set(icon, buf))
          {
             evas_object_del(icon);
             icon = NULL;
          }
        if (cl->variant)
          snprintf(buf, sizeof(buf), "%s (%s, %s)", cl->name, cl->model, cl->variant);
        else
          snprintf(buf, sizeof(buf), "%s (%s)", cl->name, cl->model);
        e_widget_ilist_append_full(ol, icon, end, buf, NULL, cfdata, cl->name);
        grp++;
     }
   e_widget_toolbook_page_append(otb, NULL, _("Keyboard Layout"), ol,
                                 1, 1, 1, 1, 0.5, 0.0);

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
   rg = e_widget_radio_group_new((int *)&(cfdata->bg_method));
   ow = e_widget_radio_add(evas, _("Theme Defined"),
                           E_DESKLOCK_BACKGROUND_METHOD_THEME_DESKLOCK, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Theme Wallpaper"),
                           E_DESKLOCK_BACKGROUND_METHOD_THEME, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Current Wallpaper"),
                           E_DESKLOCK_BACKGROUND_METHOD_WALLPAPER, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 1, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Custom"),
                           E_DESKLOCK_BACKGROUND_METHOD_CUSTOM, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_table_object_append(of, ow, 1, 1, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   cfdata->gui.o_table = ot = e_widget_table_add(evas, 1);

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     EINA_LIST_FOREACH(man->containers, ll, con)
       EINA_LIST_FOREACH(con->zones, lll, zone)
         {
            ow = e_widget_preview_add(evas, 100, 140);
            cfdata->gui.bgs = eina_list_append(cfdata->gui.bgs, ow);
            evas_object_data_set(ow, "zone", zone);
            e_widget_disabled_set(ow,
              (cfdata->bg_method < E_DESKLOCK_BACKGROUND_METHOD_CUSTOM));
            evas_object_event_callback_add(ow, EVAS_CALLBACK_MOUSE_DOWN, _cb_bg_mouse_down, cfdata);
            e_widget_table_object_append(cfdata->gui.o_table, ow, x++, 0, 1, 1, 1, 1, 1, 1);
         }
   _cb_method_change(cfdata, NULL, NULL);
   e_widget_list_object_append(ol, cfdata->gui.o_table, 1, 1, 0.5);
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
   const Eina_List *l;
   const char *bg;
   E_Config_Desklock_Background *cbg;

   e_config->desklock_start_locked = cfdata->start_locked;
   e_config->desklock_on_suspend = cfdata->lock_on_suspend;
   e_config->desklock_autolock_idle = cfdata->auto_lock;
   e_config->desklock_post_screensaver_time = cfdata->post_screensaver_time;
   e_config->desklock_autolock_screensaver = cfdata->screensaver_lock;
   e_config->desklock_autolock_idle_timeout = (cfdata->idle_time * 60);
   e_config->desklock_ask_presentation = cfdata->ask_presentation;
   e_config->desklock_ask_presentation_timeout = cfdata->ask_presentation_timeout;
   if (e_config->xkb.desklock_layout != cfdata->desklock_layout)
     {
        e_config->xkb.desklock_layout = eina_stringshare_ref(cfdata->desklock_layout);
        if (cfdata->desklock_layout)
          {
             E_Ilist_Item *ili;

             EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->gui.kbd_list), l, ili)
               {
                  if (ili->selected)
                    edje_object_signal_emit(ili->o_end, "e,state,checked", "e");
                  else
                    edje_object_signal_emit(ili->o_end, "e,state,unchecked", "e");
               }
          }
     }

   if (cfdata->bgs)
     {
        EINA_LIST_FREE(e_config->desklock_backgrounds, cbg)
          {
             e_filereg_deregister(cbg->file);
             eina_stringshare_del(cbg->file);
             free(cbg);
          }
        EINA_LIST_FOREACH(cfdata->bgs, l, bg)
          {
             cbg = E_NEW(E_Config_Desklock_Background, 1);
             cbg->file = eina_stringshare_ref(bg);
             e_config->desklock_backgrounds = eina_list_append(e_config->desklock_backgrounds, cbg);
             e_filereg_register(bg);
          }
     }

   if (cfdata->login_zone < 0)
     e_config->desklock_login_box_zone = cfdata->login_zone;
   else
     e_config->desklock_login_box_zone = cfdata->zone;

   e_config->desklock_use_custom_desklock = cfdata->custom_lock;
   if (cfdata->custom_lock_cmd)
     eina_stringshare_replace(&e_config->desklock_custom_desklock_cmd,
                              cfdata->custom_lock_cmd);

   cfdata->bg_method_prev = cfdata->bg_method;
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l, *ll;
   E_Config_Desklock_Background *cbg;

   if (e_config->xkb.desklock_layout != cfdata->desklock_layout)
     return 1;

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

   if (cfdata->bg_method_prev != (int)cfdata->bg_method) return 1;
   ll = cfdata->bgs;
   EINA_LIST_FOREACH(e_config->desklock_backgrounds, l, cbg)
     {
        if (!ll) return 1;
        if (cbg->file != ll->data) return 1;
        ll = ll->next;
     }

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

   return (e_config->desklock_ask_presentation != cfdata->ask_presentation) ||
          (e_config->desklock_ask_presentation_timeout != cfdata->ask_presentation_timeout);
}

static void
_cb_method_change(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   Evas_Object *bg;
   const char *theme = NULL;
   int x;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(cfdata->gui.bgs, l, bg)
     e_widget_disabled_set(bg,
       (cfdata->bg_method < E_DESKLOCK_BACKGROUND_METHOD_CUSTOM));

   switch (cfdata->bg_method)
     {
      case E_DESKLOCK_BACKGROUND_METHOD_THEME_DESKLOCK:
        EINA_LIST_FREE(cfdata->bgs, theme)
          eina_stringshare_del(theme);
        for (x = 0; x < cfdata->zone_count; x++)
          cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_add("theme_desklock_background"));
        theme = e_theme_edje_file_get("base/theme/desklock",
                                      "e/desklock/background");
        if (theme)
          EINA_LIST_FOREACH(cfdata->gui.bgs, l, bg)
            e_widget_preview_edje_set(bg, theme, "e/desklock/background");
        break;

      case E_DESKLOCK_BACKGROUND_METHOD_THEME:
        theme = e_theme_edje_file_get("base/theme/backgrounds",
                                      "e/desktop/background");
        if (theme)
          {
             EINA_LIST_FOREACH(cfdata->gui.bgs, l, bg)
               e_widget_preview_edje_set(bg, theme, "e/desktop/background");
             EINA_LIST_FREE(cfdata->bgs, theme)
               eina_stringshare_del(theme);
             for (x = 0; x < cfdata->zone_count; x++)
               cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_add("theme_background"));
          }
        break;

      case E_DESKLOCK_BACKGROUND_METHOD_WALLPAPER:
        if (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cdb;
             int y = 0;
             if (eina_str_has_extension(e_config->desktop_default_background, "edj"))
               {
                  EINA_LIST_FOREACH(cfdata->gui.bgs, l, bg)
                    e_widget_preview_edje_set(bg, e_config->desktop_default_background, "e/desktop/background");
                  EINA_LIST_FREE(cfdata->bgs, theme)
                    eina_stringshare_del(theme);
                  for (y = 0; y < cfdata->zone_count; y++)
                    cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_add("user_background"));
               }
             /* attempt to set wallpaper from desktop 0,0 on each zone as a desklock bg */
             EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cdb)
               if ((!cdb->desk_x) && (!cdb->desk_y))
                 {
                    Eina_List *ll;
                    ll = eina_list_nth_list(cfdata->bgs, cdb->zone);
                    if (ll)
                      {
                         theme = eina_list_data_get(ll);
                         eina_stringshare_replace(&theme, cdb->file);
                         eina_list_data_set(ll, theme);
                      }
                    else
                      cfdata->bgs = eina_list_append(cfdata->bgs, cdb->file);
                    bg = eina_list_nth(cfdata->gui.bgs, cdb->zone);
                    if (bg)
                      e_widget_preview_edje_set(bg, cdb->file, "e/desktop/background");
                 }
          }
        else if (e_config->desktop_default_background && eina_str_has_extension(e_config->desktop_default_background, "edj"))
          {
             EINA_LIST_FOREACH(cfdata->gui.bgs, l, bg)
               e_widget_preview_edje_set(bg, e_config->desktop_default_background, "e/desktop/background");
             EINA_LIST_FREE(cfdata->bgs, theme)
               eina_stringshare_del(theme);
             for (x = 0; x < cfdata->zone_count; x++)
               cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_add("user_background"));
          }
        break;

      case E_DESKLOCK_BACKGROUND_METHOD_CUSTOM:
        {
             Eina_List *ll;
             E_Config_Desklock_Background *cbg;

             EINA_LIST_FREE(cfdata->bgs, theme)
               eina_stringshare_del(theme);
             ll = cfdata->gui.bgs;
             EINA_LIST_FOREACH(e_config->desklock_backgrounds, l, cbg)
               {
                  if (!ll) break;
                  e_widget_preview_edje_set(ll->data, cbg->file, "e/desktop/background");
                  cfdata->bgs = eina_list_append(cfdata->bgs, eina_stringshare_ref(cbg->file));
                  ll = ll->next;
               }
        }
        break;

      default:
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
     cfdata->bg_fsel = e_int_config_desklock_fsel(cfdata->cfd, obj);
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

