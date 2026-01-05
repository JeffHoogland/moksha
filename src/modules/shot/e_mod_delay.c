#include "e_mod_main.h"

static Evas_Object *delay_win = NULL;
static double delay = 5.0;

static void
_cb_delay(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   delay = elm_slider_value_get(obj);
}

static void
_win_delete_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *info __UNUSED__)
{
//   E_FREE_FUNC(delay_win, evas_object_del);
   delay_win = NULL;
}

static void
_cb_ok(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *info __UNUSED__)
{
   E_Action *a = e_action_find("shot_delay");
   E_FREE_FUNC(delay_win, evas_object_del);
   if (a)
     {
        char buf[128];

        snprintf(buf, sizeof(buf), "%i", (int)(delay * 1000.0));
        a->func.go(NULL, buf);
     }
}

static void
_cb_cancel(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *info __UNUSED__)
{
   E_FREE_FUNC(delay_win, evas_object_del);
}

void
win_delay(void)
{
   Evas_Object *o, *o_bg, *o_bx, *o_sl;

   if (delay_win) return;

   delay_win = o = elm_win_add(NULL, NULL, ELM_WIN_DIALOG_BASIC);
   elm_win_title_set(o, _("Select action to take with screenshot"));
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL, _win_delete_cb, NULL);
   //~ ecore_evas_name_class_set(e_win_ee_get(o), "E", "_shot_dialog");

   o_bg = o = elm_layout_add(delay_win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(delay_win, o);
   e_theme_edje_object_set(o, "base/theme/dialog", "e/widgets/dialog/main");
   evas_object_show(o);

   o_sl = o = elm_slider_add(delay_win);
   elm_slider_span_size_set(o, 240);
   elm_object_text_set(o, _("Delay"));
   elm_slider_indicator_show_set(o, EINA_FALSE);
   elm_slider_unit_format_set(o, _("%1.1f sec"));
   elm_slider_min_max_set(o, 1, 60);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 1.0);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_part_content_set(o_bg, "e.swallow.content", o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "delay,changed", _cb_delay, NULL);

   elm_slider_value_set(o_sl, delay);

   o_bx = o = elm_box_add(delay_win);
   elm_box_horizontal_set(o, EINA_TRUE);
   elm_box_homogeneous_set(o, EINA_TRUE);
   elm_object_part_content_set(o_bg, "e.swallow.buttons", o);
   evas_object_show(o);

   o = elm_button_add(delay_win);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(o, _("OK"));
   elm_box_pack_end(o_bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "clicked", _cb_ok, NULL);

   o = elm_button_add(delay_win);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(o, _("Cancel"));
   elm_box_pack_end(o_bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "clicked", _cb_cancel, NULL);

   evas_object_show(delay_win);
}

void
delay_abort(void)
{
   E_FREE_FUNC(delay_win, evas_object_del);
}
