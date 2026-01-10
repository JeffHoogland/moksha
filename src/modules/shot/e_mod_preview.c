#include "e_mod_main.h"

static Evas_Object *o_img = NULL;

Evas_Object *win = NULL;
int          quality = 100;
Eina_Rectangle crop = { 0, 0, 0, 0 };

static void
_win_save_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   save_show(EINA_FALSE);
}

static void
_win_copy_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   save_show(EINA_TRUE);
}

static void
_win_share_cb(void *d __UNUSED__, void *d2 __UNUSED__)
{
   share_confirm();
}

static void
_win_delay_cb(void *d __UNUSED__, void *d2 __UNUSED__)
{
   E_FREE_FUNC(win, evas_object_del);
   win_delay();
}

static void
_win_cancel_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   E_FREE_FUNC(win, evas_object_del);
}

static void
_win_delete_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   win = NULL;
}

static void
_quality_change_cb(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   quality = elm_slider_value_get(obj);
}

static void
_cb_key_down(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev = event;

   if (!strcmp(ev->key, "Escape"))
     E_FREE_FUNC(win, evas_object_del);
}

void
preview_dialog_show(E_Zone *zone, E_Border *bd, const char *params, void *dst,
                    int sx, int sy, int sw, int sh)
{
   Evas *evas;
   Evas_Object *o, *o_bg, *o_box, *o_sl;
   Evas_Object *o_bx;
   int w, h;
   char smode[128], squal[128], sscreen[128];
   Evas_Modifier_Mask mask;

   win = o = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
   evas = evas_object_evas_get(o);
   elm_win_title_set(o, _("Select action to take with screenshot"));
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL, _win_delete_cb, NULL);
   Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas);
   ecore_evas_name_class_set(ee, "E", "_shot_dialog");

   o_bg = o = elm_layout_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, o);
   e_theme_edje_object_set(o, "base/theme/dialog", "e/widgets/dialog/main");
   evas_object_show(o);

   mask = 0;
   if (evas_object_key_grab(o, "Escape", mask, ~mask, 0))
     evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _cb_key_down, win);

   o_bx = o = ui_edit(win, o_bg, zone, bd, dst, sx, sy, sw, sh, &o_img);

   o_sl = o = elm_slider_add(win);
   elm_object_text_set(o, _("Quality"));
   elm_slider_indicator_show_set(o, EINA_FALSE);
   elm_slider_unit_format_set(o, "%1.0f%%");
   elm_slider_min_max_set(o, 10, 100);
   elm_slider_step_set(o, 5);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 1.0);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(o_bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "delay,changed", _quality_change_cb, NULL);

   ///////////////////////////////////////////////////////////////////////

   o_box = o = e_widget_list_add(evas, 1, 1);
   elm_object_part_content_set(o_bg, "e.swallow.buttons", o);

   o = e_widget_button_add(evas, _("Save"), NULL, _win_save_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Share"), NULL, _win_share_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Copy"), NULL, _win_copy_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   if (!bd)
     {
        o = e_widget_button_add(evas, _("Delay"), NULL, _win_delay_cb, win, NULL);
        e_widget_list_object_append(o_box, o, 1, 0, 0.5);
     }
   o = e_widget_button_add(evas, _("Cancel"), NULL, _win_cancel_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);

   o = o_box;
   e_widget_size_min_get(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   elm_object_part_content_set(o_bg, "e.swallow.buttons", o);
   evas_object_size_hint_min_get(o_bg, &w, &h);
   evas_object_resize(win, w, h);
   evas_object_size_hint_min_set(win, w, h);
   evas_object_size_hint_max_set(win, 99999, 99999);

   ///////////////////////////////////////////////////////////////////////

   crop.x = crop.y = crop.w = crop.h = 0;

   E_Container *con = e_container_current_get(e_manager_current_get());
   if ((params) &&
       (sscanf(params, "%100s %100s %100s", smode, squal, sscreen) == 3))
     {
        if ((zone) && (!strcmp(sscreen, "current")))
          {
             E_Zone *z = e_zone_current_get(con);
             ui_edit_crop_screen_set(z->x, z->y, z->w, z->h);
          }
        else if (!strcmp(sscreen, "all"))
          {
          }
        else
          {
             //~ E_Zone *z = eina_list_nth(e_comp->zones, atoi(sscreen));
             E_Zone *z = e_zone_current_get(con);
             //~ E_Zone *z = zone;
             if (z) ui_edit_crop_screen_set(z->x, z->y, z->w, z->h);
          }
        quality = 90;
        if (!strcmp(squal, "perfect")) quality = 100;
        else if (!strcmp(squal, "high")) quality = 90;
        else if (!strcmp(squal, "medium")) quality = 70;
        else if (!strcmp(squal, "low")) quality = 50;
        else quality = atoi(squal);

        elm_slider_value_set(o_sl, quality);
        if (!strcmp(smode, "save")) _win_save_cb(NULL, NULL);
        else if (!strcmp(smode, "share")) _win_share_cb(NULL, NULL);
     }
   else
     {
        Evas_Coord maxw, maxh, minw, minh;
        Evas_Object *sc_main, *sc_tool;

        elm_slider_value_set(o_sl, quality);

        if (!zone) zone = e_zone_current_get(con);
        maxw = (zone->w - (40 * 2 * e_scale)) * 0.9;
        maxh = (zone->h - (40 * 2 * e_scale)) * 0.9;

        sc_main = evas_object_data_get(win, "scroll/main");
        sc_tool = evas_object_data_get(win, "scroll/tools");
        elm_scroller_content_min_limit(sc_main, EINA_TRUE, EINA_TRUE);
        elm_scroller_content_min_limit(sc_tool, EINA_TRUE, EINA_TRUE);
        evas_smart_objects_calculate(evas_object_evas_get(win));
        evas_object_size_hint_min_get(o_bg, &minw, &minh);
        if (minw > maxw) minw = maxw;
        if (minh > maxh) minh = maxh;
        elm_scroller_content_min_limit(sc_main, EINA_FALSE, EINA_FALSE);
        elm_scroller_content_min_limit(sc_tool, EINA_TRUE, EINA_FALSE);
        evas_smart_objects_calculate(evas_object_evas_get(win));
        evas_object_resize(win, minw, minh);
        elm_win_center(win, 1, 1);
        evas_object_show(win);
        //~ e_win_border_icon_set(win, "screenshot");
        if (!e_widget_focus_get(o_bg)) e_widget_focus_set(o_box, 1);
        if (bd)
          {
             //~ E_Border *b = e_win_border_get(win);
             E_Border *b = bd;
             if (b) evas_object_layer_set(bd->bg_evas, bd->layer);
          }
     }
}

Eina_Bool
preview_have(void)
{
   if (win) return EINA_TRUE;
   else return EINA_FALSE;
}

void
preview_abort(void)
{
   E_FREE_FUNC(win, evas_object_del);
}

Evas_Object *
preview_image_get(void)
{
   return o_img;
}
