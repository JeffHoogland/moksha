#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
//static void        _cb_disable_check(void *data, Evas_Object *obj);
static void        _cb_enable_check(void *data, Evas_Object *obj);
static void        _cb_disable_check_list(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data 
{
   int warp_while_selecting;
   int warp_at_end;
   int scroll_animate;
   int list_show_iconified;
   int list_show_other_desk_iconified;
   int list_show_other_screen_iconified;
   int list_show_other_desk_windows;
   int list_show_other_screen_windows;
   int list_uncover_while_selecting;
   int list_jump_desk_while_selecting;
   int list_focus_while_selecting;
   int list_raise_while_selecting;

   /* Advanced */
   double scroll_speed;
   double warp_speed;
   double pos_align_x;
   double pos_align_y;
   double pos_size_w;
   double pos_size_h;
   int pos_min_w;
   int pos_min_h;
   int pos_max_w;
   int pos_max_h;
};

Eina_List *iconified_list = NULL;
Eina_List *warp_at_end_list = NULL;
Eina_List *scroll_animation_list = NULL;

E_Config_Dialog *
e_int_config_winlist(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "advanced/window_list")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;

   cfd = e_config_dialog_new(con,
			     _("Window List Settings"),
			     "E", "advanced/window_list",
			     "preferences-winlist", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->warp_while_selecting = e_config->winlist_warp_while_selecting;
   cfdata->warp_at_end = e_config->winlist_warp_at_end;
   cfdata->warp_speed = e_config->winlist_warp_speed;
   cfdata->scroll_animate = e_config->winlist_scroll_animate;
   cfdata->scroll_speed = e_config->winlist_scroll_speed;
   cfdata->list_show_iconified = e_config->winlist_list_show_iconified;
   cfdata->list_show_other_desk_iconified = e_config->winlist_list_show_other_desk_iconified;
   cfdata->list_show_other_screen_iconified = e_config->winlist_list_show_other_screen_iconified;
   cfdata->list_show_other_desk_windows = e_config->winlist_list_show_other_desk_windows;
   cfdata->list_show_other_screen_windows = e_config->winlist_list_show_other_screen_windows;
   cfdata->list_uncover_while_selecting = e_config->winlist_list_uncover_while_selecting;
   cfdata->list_jump_desk_while_selecting = e_config->winlist_list_jump_desk_while_selecting;
   cfdata->list_focus_while_selecting = e_config->winlist_list_focus_while_selecting;
   cfdata->list_raise_while_selecting = e_config->winlist_list_raise_while_selecting;
   cfdata->pos_align_x = e_config->winlist_pos_align_x;
   cfdata->pos_align_y = e_config->winlist_pos_align_y;
   cfdata->pos_size_w = e_config->winlist_pos_size_w;
   cfdata->pos_size_h = e_config->winlist_pos_size_h;
   cfdata->pos_min_w = e_config->winlist_pos_min_w;
   cfdata->pos_min_h = e_config->winlist_pos_min_h;
   cfdata->pos_max_w = e_config->winlist_pos_max_w;
   cfdata->pos_max_h = e_config->winlist_pos_max_h;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   iconified_list = eina_list_free(iconified_list);
   warp_at_end_list = eina_list_free(warp_at_end_list);
   scroll_animation_list = eina_list_free(scroll_animation_list);

   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->winlist_list_show_iconified = cfdata->list_show_iconified;
   e_config->winlist_list_show_other_desk_iconified = cfdata->list_show_other_desk_iconified;
   e_config->winlist_list_show_other_screen_iconified = cfdata->list_show_other_screen_iconified;
   e_config->winlist_list_show_other_desk_windows = cfdata->list_show_other_desk_windows;
   e_config->winlist_list_show_other_screen_windows = cfdata->list_show_other_screen_windows;
   e_config->winlist_list_uncover_while_selecting = cfdata->list_uncover_while_selecting;
   e_config->winlist_list_jump_desk_while_selecting = cfdata->list_jump_desk_while_selecting;
   e_config->winlist_warp_while_selecting = cfdata->warp_while_selecting;
   e_config->winlist_warp_at_end = cfdata->warp_at_end;
   e_config->winlist_scroll_animate = cfdata->scroll_animate;
   e_config->winlist_list_focus_while_selecting = cfdata->list_focus_while_selecting;
   e_config->winlist_list_raise_while_selecting = cfdata->list_raise_while_selecting;
   e_config_save_queue();
   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   Evas_Object *iconified_check;
   Evas_Object *warp_mouse_check;
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   iconified_check = e_widget_check_add(evas, _("Show iconified windows"), &(cfdata->list_show_iconified));
   e_widget_framelist_object_append(of, iconified_check);
   ob = e_widget_check_add(evas, _("Show iconified windows from other desks"), &(cfdata->list_show_other_desk_iconified));
   iconified_list = eina_list_append (iconified_list, ob);
   e_widget_disabled_set(ob, !cfdata->list_show_iconified); // set state from saved config
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Show iconified windows from other screens"), &(cfdata->list_show_other_screen_iconified));
   iconified_list = eina_list_append (iconified_list, ob);
   e_widget_disabled_set(ob, !cfdata->list_show_iconified); // set state from saved config
   e_widget_framelist_object_append(of, ob);
   // handler for enable/disable widget array
   e_widget_on_change_hook_set(iconified_check, _cb_disable_check_list, iconified_list);
   ob = e_widget_check_add(evas, _("Show windows from other desks"), &(cfdata->list_show_other_desk_windows));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Show windows from other screens"), &(cfdata->list_show_other_screen_windows));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Selection Settings"), 0);   
   ob = e_widget_check_add(evas, _("Focus window while selecting"), &(cfdata->list_focus_while_selecting));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Raise window while selecting"), &(cfdata->list_raise_while_selecting));
   e_widget_framelist_object_append(of, ob);
   warp_mouse_check = e_widget_check_add(evas, _("Warp mouse to window while selecting"), &(cfdata->warp_while_selecting));
   e_widget_framelist_object_append(of, warp_mouse_check);
   ob = e_widget_check_add(evas, _("Uncover windows while selecting"), &(cfdata->list_uncover_while_selecting));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Jump to desk while selecting"), &(cfdata->list_jump_desk_while_selecting));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
      
   of = e_widget_framelist_add(evas, _("Warp Settings"), 0);   
   ob = e_widget_check_add(evas, _("Warp At End"), &(cfdata->warp_at_end));
   e_widget_disabled_set(ob, cfdata->warp_while_selecting); // set state from saved config
   e_widget_on_change_hook_set(warp_mouse_check, _cb_enable_check, ob);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);   
   ob = e_widget_check_add(evas, _("Scroll Animate"), &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->winlist_list_show_iconified = cfdata->list_show_iconified;
   e_config->winlist_list_show_other_desk_iconified = cfdata->list_show_other_desk_iconified;
   e_config->winlist_list_show_other_screen_iconified = cfdata->list_show_other_screen_iconified;
   e_config->winlist_list_show_other_desk_windows = cfdata->list_show_other_desk_windows;
   e_config->winlist_list_show_other_screen_windows = cfdata->list_show_other_screen_windows;
   e_config->winlist_list_uncover_while_selecting = cfdata->list_uncover_while_selecting;
   e_config->winlist_list_jump_desk_while_selecting = cfdata->list_jump_desk_while_selecting;
   e_config->winlist_warp_while_selecting = cfdata->warp_while_selecting;
   e_config->winlist_warp_at_end = cfdata->warp_at_end;
   e_config->winlist_scroll_animate = cfdata->scroll_animate;
   e_config->winlist_list_focus_while_selecting = cfdata->list_focus_while_selecting;
   e_config->winlist_list_raise_while_selecting = cfdata->list_raise_while_selecting;
   e_config->winlist_warp_speed = cfdata->warp_speed;
   e_config->winlist_scroll_speed = cfdata->scroll_speed;
   e_config->winlist_pos_align_x = cfdata->pos_align_x;
   e_config->winlist_pos_align_y = cfdata->pos_align_y;
   e_config->winlist_pos_min_w = cfdata->pos_min_w;
   e_config->winlist_pos_min_h = cfdata->pos_min_h;
   e_config->winlist_pos_max_w = cfdata->pos_max_w;
   e_config->winlist_pos_max_h = cfdata->pos_max_h;
   e_config_save_queue();

   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob, *ot;
   Evas_Object *warp_at_end_check;
   Evas_Object *scroll_animation_check;
   
   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 0);

   /* REMOVED AS ADVANCED DOESN"T FIT INTO 640x480 WITH IT */
   /*
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show iconified windows"), &(cfdata->list_show_iconified));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Show windows from other desks"), &(cfdata->list_show_other_desk_windows));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Show windows from other screens"), &(cfdata->list_show_other_screen_windows));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   */   

   of = e_widget_framelist_add(evas, _("Selection Settings"), 0);   
   ob = e_widget_check_add(evas, _("Focus window while selecting"), &(cfdata->list_focus_while_selecting));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Raise window while selecting"), &(cfdata->list_raise_while_selecting));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Warp mouse to window while selecting"), &(cfdata->warp_while_selecting));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Uncover windows while selecting"), &(cfdata->list_uncover_while_selecting));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Jump to desk while selecting"), &(cfdata->list_jump_desk_while_selecting));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
      
   of = e_widget_framelist_add(evas, _("Warp Settings"), 0);   
   warp_at_end_check = e_widget_check_add(evas, _("Warp At End"), &(cfdata->warp_at_end));
   e_widget_framelist_object_append(of, warp_at_end_check);
   ob = e_widget_label_add(evas, _("Warp Speed"));
   warp_at_end_list = eina_list_append (warp_at_end_list, ob);
   e_widget_disabled_set(ob, !cfdata->warp_at_end); // set state from saved config
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->warp_speed), NULL, 200);
   warp_at_end_list = eina_list_append (warp_at_end_list, ob);
   e_widget_disabled_set(ob, !cfdata->warp_at_end); // set state from saved config
   e_widget_framelist_object_append(of, ob);   
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);
   // handler for enable/disable widget array
   e_widget_on_change_hook_set(warp_at_end_check, _cb_disable_check_list, warp_at_end_list);

   of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);   
   scroll_animation_check = e_widget_check_add(evas, _("Scroll Animate"), &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, scroll_animation_check);
   ob = e_widget_label_add(evas, _("Scroll Speed"));
   scroll_animation_list  = eina_list_append (scroll_animation_list , ob);
   e_widget_disabled_set(ob, !cfdata->scroll_animate); // set state from saved config
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->scroll_speed), NULL, 200);
   scroll_animation_list  = eina_list_append (scroll_animation_list , ob);
   e_widget_disabled_set(ob, !cfdata->scroll_animate); // set state from saved config
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);
   // handler for enable/disable widget array
   e_widget_on_change_hook_set(scroll_animation_check, _cb_disable_check_list, scroll_animation_list);

   of = e_widget_framelist_add(evas, _("Position Settings"), 0);   
   ob = e_widget_label_add(evas, _("X-Axis Alignment"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->pos_align_x), NULL, 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Y-Axis Alignment"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->pos_align_y), NULL, 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Size Settings"), 0);      
   ob = e_widget_label_add(evas, _("Minimum Width"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_min_w), 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Minimum Height"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_min_h), 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Maximum Width"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_max_w), 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Maximum Height"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_max_h), 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_table_object_append(ot, of, 1, 1, 1, 2, 1, 1, 1, 1);

   e_widget_list_object_append(o, ot, 1, 1, 0.5);      
   return o;   
}

/*!
 * If the check is disabled then disable the chained object.
 *
 * @param data A Evas_Object to chain together with the checkbox
 * @param obj A Evas_Object checkbox created with e_widget_check_add()
 */
#if 0
/* FIXME unused */
static void
_cb_disable_check(void *data, Evas_Object *obj)
{
   e_widget_disabled_set((Evas_Object *) data, 
                         !e_widget_check_checked_get(obj));
}
#endif

/*!
 * If the check is disabled then enable the chained object.
 *
 * @param data A Evas_Object to chain together with the checkbox
 * @param obj A Evas_Object checkbox created with e_widget_check_add()
 */
static void
_cb_enable_check(void *data, Evas_Object *obj)
{
   e_widget_disabled_set((Evas_Object *) data, 
                         e_widget_check_checked_get(obj));
}

/*!
 *  * If the check is disabled then disable the chained objects in the list.
 *
 * @param data A Eina_List of Evas_Object to chain widgets together with the checkbox
 * @param obj A Evas_Object checkbox created with e_widget_check_add()
 */
static void
_cb_disable_check_list(void *data, Evas_Object *obj)
{
   Eina_List *list = (Eina_List*) data;
   Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(list, l, o)
      e_widget_disabled_set(o, !e_widget_check_checked_get(obj));
}
