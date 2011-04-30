#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _iconified_changed(void *data, Evas_Object *obj);
static void _warp_changed(void *data, Evas_Object *obj __UNUSED__);
static void _scroll_animate_changed(void *data, Evas_Object *obj);
static void _width_limits_changed(void *data, Evas_Object *obj __UNUSED__);
static void _height_limits_changed(void *data, Evas_Object *obj __UNUSED__);

struct _E_Config_Dialog_Data
{
   int windows_other_desks;
   int windows_other_screens;
   int iconified;
   int iconified_other_desks;
   int iconified_other_screens;

   int focus, raise, uncover;
   int warp_while_selecting;
   int warp_at_end;
   double warp_speed;
   int jump_desk;

   int scroll_animate;
   double scroll_speed;

   double align_x, align_y;
   int min_w, min_h, max_w, max_h;

   struct 
     {
        Eina_List *disable_iconified;
        Eina_List *disable_scroll_animate;
        Eina_List *disable_warp;
        Evas_Object *min_w, *min_h;
     } gui;
};

E_Config_Dialog *
e_int_config_winlist(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/window_list")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Window List Settings"),
			     "E", "windows/window_list",
			     "preferences-winlist", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->focus = e_config->winlist_list_focus_while_selecting;
   cfdata->raise = e_config->winlist_list_raise_while_selecting;
   cfdata->uncover = e_config->winlist_list_uncover_while_selecting;
   cfdata->jump_desk = e_config->winlist_list_jump_desk_while_selecting;

   cfdata->windows_other_desks =
     e_config->winlist_list_show_other_desk_windows;
   cfdata->windows_other_screens =
     e_config->winlist_list_show_other_screen_windows;

   cfdata->iconified = e_config->winlist_list_show_iconified;
   cfdata->iconified_other_desks =
     e_config->winlist_list_show_other_desk_iconified;
   cfdata->iconified_other_screens =
     e_config->winlist_list_show_other_screen_iconified;

   cfdata->warp_while_selecting = e_config->winlist_warp_while_selecting;
   cfdata->warp_at_end = e_config->winlist_warp_at_end;
   cfdata->warp_speed = e_config->winlist_warp_speed;

   cfdata->scroll_animate = e_config->winlist_scroll_animate;
   cfdata->scroll_speed = e_config->winlist_scroll_speed;

   cfdata->align_x = e_config->winlist_pos_align_x;
   cfdata->align_y = e_config->winlist_pos_align_y;
   cfdata->min_w = e_config->winlist_pos_min_w;
   cfdata->min_h = e_config->winlist_pos_min_h;
   cfdata->max_w = e_config->winlist_pos_max_w;
   cfdata->max_h = e_config->winlist_pos_max_h;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->gui.disable_iconified);
   eina_list_free(cfdata->gui.disable_scroll_animate);
   eina_list_free(cfdata->gui.disable_warp);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define DO(_e_config, _cfdata) \
   e_config->winlist_##_e_config = cfdata->_cfdata

   DO(list_show_iconified, iconified);
   DO(list_show_other_desk_iconified, iconified_other_desks);
   DO(list_show_other_screen_iconified, iconified_other_screens);
   DO(list_show_other_desk_windows, windows_other_desks);
   DO(list_show_other_screen_windows, windows_other_screens);
   DO(list_uncover_while_selecting, uncover);
   DO(list_jump_desk_while_selecting, jump_desk);
   DO(warp_while_selecting, warp_while_selecting);
   DO(warp_at_end, warp_at_end);
   DO(warp_speed, warp_speed);
   DO(scroll_animate, scroll_animate);
   DO(scroll_speed, scroll_speed);
   DO(list_focus_while_selecting, focus);
   DO(list_raise_while_selecting, raise);
   DO(pos_align_x, align_x);
   DO(pos_align_y, align_y);
   DO(pos_min_w, min_w);
   DO(pos_min_h, min_h);
   DO(pos_max_w, max_w);
   DO(pos_max_h, max_h);
#undef DO

   e_config_save_queue();

   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define DO(_e_config, _cfdata) \
   if (e_config->winlist_##_e_config != cfdata->_cfdata) return 1;

   DO(list_show_iconified, iconified);
   DO(list_show_other_desk_iconified, iconified_other_desks);
   DO(list_show_other_screen_iconified, iconified_other_screens);
   DO(list_show_other_desk_windows, windows_other_desks);
   DO(list_show_other_screen_windows, windows_other_screens);
   DO(list_uncover_while_selecting, uncover);
   DO(list_jump_desk_while_selecting, jump_desk);
   DO(warp_while_selecting, warp_while_selecting);
   DO(warp_at_end, warp_at_end);
   DO(warp_speed, warp_speed);
   DO(scroll_animate, scroll_animate);
   DO(scroll_speed, scroll_speed);
   DO(list_focus_while_selecting, focus);
   DO(list_raise_while_selecting, raise);
   DO(pos_align_x, align_x);
   DO(pos_align_y, align_y);
   DO(pos_min_w, min_w);
   DO(pos_min_h, min_h);
   DO(pos_max_w, max_w);
   DO(pos_max_h, max_h);
#undef DO

   return 0;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ob, *iconified, *scroll_animate;

   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Windows from other desks"), 
                           &(cfdata->windows_other_desks));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Windows from other screens"), 
                           &(cfdata->windows_other_screens));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Iconified"), &(cfdata->iconified));
   iconified = ob;
   e_widget_on_change_hook_set(ob, _iconified_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Iconified from other desks"), 
                           &(cfdata->iconified_other_desks));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   cfdata->gui.disable_iconified = 
     eina_list_append(cfdata->gui.disable_iconified, ob);
   ob = e_widget_check_add(evas, _("Iconified from other screens"),
                           &(cfdata->iconified_other_screens));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   cfdata->gui.disable_iconified = 
     eina_list_append(cfdata->gui.disable_iconified, ob);
   e_widget_toolbook_page_append(otb, NULL, _("Display"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Focus"), &(cfdata->focus));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Raise"), &(cfdata->raise));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Uncover"), &(cfdata->uncover));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Warp mouse while selecting"), 
                           &(cfdata->warp_while_selecting));
   e_widget_on_change_hook_set(ob, _warp_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Warp mouse at end"), 
                           &(cfdata->warp_at_end));
   e_widget_on_change_hook_set(ob, _warp_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Jump to desk"), &(cfdata->jump_desk));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Selecting"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Warp speed"));
   cfdata->gui.disable_warp = 
     eina_list_append(cfdata->gui.disable_warp, ob);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0,
                            &(cfdata->warp_speed), NULL, 100);
   cfdata->gui.disable_warp = eina_list_append(cfdata->gui.disable_warp, ob);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Scroll Animation"), 
                           &(cfdata->scroll_animate));
   e_widget_on_change_hook_set(ob, _scroll_animate_changed, cfdata);
   scroll_animate = ob;
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Scroll speed"));
   cfdata->gui.disable_scroll_animate = 
     eina_list_append(cfdata->gui.disable_scroll_animate, ob);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0,
                            &(cfdata->scroll_speed), NULL, 100);
   cfdata->gui.disable_scroll_animate = 
     eina_list_append(cfdata->gui.disable_scroll_animate, ob);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Animations"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Minimum width"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 10, 0, NULL, 
                            &(cfdata->min_w), 100);
   cfdata->gui.min_w = ob;
   e_widget_on_change_hook_set(ob, _width_limits_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Maximum width"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 10, 0, NULL, 
                            &(cfdata->max_w), 100);
   e_widget_on_change_hook_set(ob, _width_limits_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Minimum height"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 10, 0, NULL, 
                            &(cfdata->min_h), 100);
   cfdata->gui.min_h = ob;
   e_widget_on_change_hook_set(ob, _height_limits_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Maximum height"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 10, 0, NULL, 
                            &(cfdata->max_h), 100);
   e_widget_on_change_hook_set(ob, _height_limits_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Geometry"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Horizontal alignment"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, 
                            &(cfdata->align_x), NULL, 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Vertical alignment"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, 
                            &(cfdata->align_y), NULL, 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Alignment"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   _iconified_changed(cfdata, iconified);
   _warp_changed(cfdata, NULL);
   _scroll_animate_changed(cfdata, scroll_animate);

   e_widget_toolbook_page_show(otb, 0);

   return otb;
/*
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Display"), 0);
   ob = e_widget_check_add(evas, _("From other desks"), &(cfdata->other_desks));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("From other screens"), 
                           &(cfdata->other_screens));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Iconified"), &(cfdata->iconified));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Animations"), &(cfdata->animations));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Selecting"), 0);
   ob = e_widget_check_add(evas, _("Focus"), &(cfdata->focus));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Raise"), &(cfdata->raise));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Uncover"), &(cfdata->uncover));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Warp mouse"), &(cfdata->warp));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Jump to desk"), &(cfdata->jump_desk));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
 */
}

static void
_iconified_changed(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   Evas_Object *o;
   Eina_Bool disabled = !e_widget_check_checked_get(obj);

   EINA_LIST_FOREACH(cfdata->gui.disable_iconified, l, o)
     e_widget_disabled_set(o, disabled);
}

static void
_warp_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   Evas_Object *o;
   Eina_Bool disabled;

   disabled = ((!cfdata->warp_while_selecting) && (!cfdata->warp_at_end));
   EINA_LIST_FOREACH(cfdata->gui.disable_warp, l, o)
     e_widget_disabled_set(o, disabled);
}

static void
_scroll_animate_changed(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   Evas_Object *o;
   Eina_Bool disabled = !e_widget_check_checked_get(obj);

   EINA_LIST_FOREACH(cfdata->gui.disable_scroll_animate, l, o)
     e_widget_disabled_set(o, disabled);
}

static void
_width_limits_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->min_w > cfdata->max_w)
     e_widget_slider_value_int_set(cfdata->gui.min_w, cfdata->max_w);
}

static void
_height_limits_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->min_h > cfdata->max_h)
     e_widget_slider_value_int_set(cfdata->gui.min_h, cfdata->max_h);
}
