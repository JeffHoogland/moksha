#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   int        show_cursor;
   int        idle_cursor;
   int        use_e_cursor;
   int        cursor_size;

   struct
   {
      Evas_Object *idle_cursor;
   } gui;

   int mouse_hand;

   double mouse_accel;
   double mouse_accel_threshold;
   int mouse_natural_scroll;
   int mouse_emulate_middle_button;

   double touch_accel;
   int touch_natural_scroll;
   int touch_emulate_middle_button;
   int touch_tap_to_click;
   int touch_clickpad;
   int touch_scrolling_2finger;
   int touch_scrolling_edge;
   int touch_scrolling_circular;
   int touch_scrolling_horiz;
   int touch_palm_detect;
   int touch_extras;
};

E_Config_Dialog *
e_int_config_mouse(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "keyboard_and_mouse/mouse_settings"))
     return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Mouse and Touchpad Settings"), "E",
                             "keyboard_and_mouse/mouse_settings",
                             "preferences-desktop-mouse", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->show_cursor = e_config->show_cursor;
   cfdata->idle_cursor = e_config->idle_cursor;
   cfdata->use_e_cursor = e_config->use_e_cursor;
   cfdata->cursor_size = e_config->cursor_size;

   cfdata->mouse_hand = e_config->mouse_hand;

   cfdata->mouse_accel = e_config->mouse_accel;
   cfdata->mouse_accel_threshold = e_config->mouse_accel_threshold;
   cfdata->mouse_natural_scroll = e_config->mouse_natural_scroll;
   cfdata->mouse_emulate_middle_button = e_config->mouse_emulate_middle_button;

   cfdata->touch_accel = e_config->touch_accel;
   cfdata->touch_natural_scroll = e_config->touch_natural_scroll;
   cfdata->touch_emulate_middle_button = e_config->touch_emulate_middle_button;
   cfdata->touch_tap_to_click = e_config->touch_tap_to_click;
   cfdata->touch_clickpad = e_config->touch_clickpad;
   cfdata->touch_scrolling_2finger = e_config->touch_scrolling_2finger;
   cfdata->touch_scrolling_edge = e_config->touch_scrolling_edge;
   cfdata->touch_scrolling_circular = e_config->touch_scrolling_circular;
   cfdata->touch_scrolling_horiz = e_config->touch_scrolling_horiz;
   cfdata->touch_palm_detect = e_config->touch_palm_detect;
   cfdata->touch_extras = e_config->touch_extras;
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

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
    return !((cfdata->show_cursor == e_config->show_cursor) &&
            (cfdata->idle_cursor == e_config->idle_cursor) &&
            (cfdata->use_e_cursor == e_config->use_e_cursor) &&
            (cfdata->cursor_size == e_config->cursor_size) &&
            (cfdata->mouse_hand == e_config->mouse_hand) &&
            EINA_DBL_EQ(cfdata->mouse_accel, e_config->mouse_accel) &&
            EINA_DBL_EQ(cfdata->mouse_accel_threshold, e_config->mouse_accel_threshold) &&
            (cfdata->mouse_natural_scroll == e_config->mouse_natural_scroll) &&
            (cfdata->mouse_emulate_middle_button == e_config->mouse_emulate_middle_button) &&
            EINA_DBL_EQ(cfdata->touch_accel, e_config->touch_accel) &&
            (cfdata->touch_natural_scroll == e_config->touch_natural_scroll) &&
            (cfdata->touch_extras == e_config->touch_extras) &&
            (cfdata->touch_emulate_middle_button == e_config->touch_emulate_middle_button) &&
            (cfdata->touch_tap_to_click == e_config->touch_tap_to_click) &&
            (cfdata->touch_clickpad == e_config->touch_clickpad) &&
            (cfdata->touch_scrolling_2finger == e_config->touch_scrolling_2finger) &&
            (cfdata->touch_scrolling_edge == e_config->touch_scrolling_edge) &&
            (cfdata->touch_scrolling_circular == e_config->touch_scrolling_circular) &&
            (cfdata->touch_scrolling_horiz == e_config->touch_scrolling_horiz) &&
            (cfdata->touch_palm_detect == e_config->touch_palm_detect));
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

/* advanced window */
static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Manager *man;

   e_config->use_e_cursor = cfdata->use_e_cursor;
   e_config->show_cursor = cfdata->show_cursor;
   e_config->idle_cursor = cfdata->idle_cursor;
   e_config->cursor_size = cfdata->cursor_size;

   e_config->mouse_hand = cfdata->mouse_hand;

   e_config->mouse_accel = cfdata->mouse_accel;
   e_config->mouse_accel_threshold = cfdata->mouse_accel_threshold;
   e_config->mouse_natural_scroll = cfdata->mouse_natural_scroll;
   e_config->mouse_emulate_middle_button = cfdata->mouse_emulate_middle_button;

   e_config->touch_extras = cfdata->touch_extras;
   e_config->touch_accel = cfdata->touch_accel;
   e_config->touch_natural_scroll = cfdata->touch_natural_scroll;
   e_config->touch_emulate_middle_button = cfdata->touch_emulate_middle_button;
   e_config->touch_tap_to_click = cfdata->touch_tap_to_click;
   e_config->touch_clickpad = cfdata->touch_clickpad;
   e_config->touch_scrolling_2finger = cfdata->touch_scrolling_2finger;
   e_config->touch_scrolling_edge = cfdata->touch_scrolling_edge;
   e_config->touch_scrolling_circular = cfdata->touch_scrolling_circular;
   e_config->touch_scrolling_horiz = cfdata->touch_scrolling_horiz;
   e_config->touch_palm_detect = cfdata->touch_palm_detect;

   e_config_save_queue();

   /* Apply the above settings */
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        if (man->pointer && !e_config->show_cursor)
          {
             e_pointer_hide(man->pointer);
             continue;
          }
        if (man->pointer) e_object_del(E_OBJECT(man->pointer));
        man->pointer = e_pointer_window_new(man->root, 1);
     }

   e_mouse_update();

   return 1;
}


static void
_use_e_cursor_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool disabled = ((!cfdata->use_e_cursor) || (!cfdata->show_cursor));
   e_widget_disabled_set(cfdata->gui.idle_cursor, disabled);
}

//~ static void
//~ _mouse_cb_change(void *data, Evas_Object *obj __UNUSED__)
//~ {
   //~ E_Config_Dialog_Data *cfdata = data;
   //~ e_widget_check_checked_set(cfdata->touch_natur, e_widget_check_checked_get(cfdata->mouse_natur));
   //~ cfdata->touch_natural_scroll = cfdata->mouse_natural_scroll;
//~ }

static void
_touch_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   //~ e_widget_check_checked_set(cfdata->mouse_natur, e_widget_check_checked_get(cfdata->touch_natur));
   cfdata->mouse_natural_scroll = cfdata->touch_natural_scroll;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *of, *ob, *oc, *ex;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   /* Cursor */
   ol = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Settings"), 0);

   oc = e_widget_check_add(evas, _("Show Cursor"), &(cfdata->show_cursor));
   e_widget_framelist_object_append(of, oc);
   
   rg = e_widget_radio_group_new(&cfdata->use_e_cursor);

   ob = e_widget_label_add(evas, _("Size"));
   e_widget_framelist_object_append(of, ob);
   e_widget_check_widget_disable_on_unchecked_add(oc, ob);

   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels"),
                            8, 128, 4, 0, NULL, &(cfdata->cursor_size), 100);
   e_widget_framelist_object_append(of, ob);
   e_widget_check_widget_disable_on_unchecked_add(oc, ob);

   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Theme"), 0);

   ob = e_widget_radio_add(evas, _("X"), 0, rg);
   e_widget_on_change_hook_set(ob, _use_e_cursor_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_check_widget_disable_on_unchecked_add(oc, ob);

   ob = e_widget_radio_add(evas, _("Moksha"), 1, rg);
   e_widget_on_change_hook_set(ob, _use_e_cursor_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_check_widget_disable_on_unchecked_add(oc, ob);
   
   ob = e_widget_check_add(evas, _("Idle effects"),
                           &(cfdata->idle_cursor));
   e_widget_framelist_object_append(of, ob);
   cfdata->gui.idle_cursor = ob;

   e_widget_list_object_append(ol, of, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Cursor"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Mouse */
   /* TODO: Get all inputs and hide this tab if none is relative. */
   ol = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, _("Mouse Hand"), 0);
   rg = e_widget_radio_group_new(&(cfdata->mouse_hand));
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-desktop-mouse-right", 48, 48, E_MOUSE_HAND_LEFT, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-desktop-mouse-left", 48, 48, E_MOUSE_HAND_RIGHT, rg);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Acceleration"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f"), -1.0, 1.0, 0.1, 0,
                            &(cfdata->mouse_accel), NULL, 100);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Threshold"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, 10.0, 1.0, 0,
                            &(cfdata->mouse_accel_threshold), NULL, 100);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Buttons"), 0);

   oc = e_widget_check_add(evas, _("Middle mouse button emulation"), &(cfdata->mouse_emulate_middle_button));
   e_widget_framelist_object_append(of, oc);

   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   //~ of = e_widget_framelist_add(evas, _("Scrolling"), 0);

   //~ oc = e_widget_check_add(evas, _("Natural scrolling"), &(cfdata->mouse_natural_scroll));
   //~ cfdata->mouse_natur = oc;
   //~ e_widget_on_change_hook_set(cfdata->mouse_natur, _mouse_cb_change, cfdata);
   //~ e_widget_framelist_object_append(of, oc);

   //~ e_widget_list_object_append(ol, of, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Mouse"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Settings"), 0);

   ex = e_widget_check_add(evas, _("Enable Devices Extras"), &(cfdata->touch_extras));
   e_widget_framelist_object_append(of, ex);

   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Acceleration"), 0);

   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f"), -1.0, 1.0, 0.1, 0,
                            &(cfdata->touch_accel), NULL, 100);
   e_widget_check_widget_disable_on_unchecked_add(ex, ob);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Buttons"), 0);

   oc = e_widget_check_add(evas, _("Tap to click"), &(cfdata->touch_tap_to_click));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("Middle mouse button emulation"), &(cfdata->touch_emulate_middle_button));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("Clickpad"), &(cfdata->touch_clickpad));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("Palm detect"), &(cfdata->touch_palm_detect));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Scrolling"), 0);

   oc = e_widget_check_add(evas, _("Natural scrolling"), &(cfdata->touch_natural_scroll));
   e_widget_on_change_hook_set(oc, _touch_cb_change, cfdata);
   cfdata->mouse_natural_scroll = cfdata->touch_natural_scroll;
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("Horizontal scrolling"), &(cfdata->touch_scrolling_horiz));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("Edge scrolling"), &(cfdata->touch_scrolling_edge));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("2 finger scrolling"), &(cfdata->touch_scrolling_2finger));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   oc = e_widget_check_add(evas, _("Circular scrolling"), &(cfdata->touch_scrolling_circular));
   e_widget_check_widget_disable_on_unchecked_add(ex, oc);
   e_widget_framelist_object_append(of, oc);

   e_widget_list_object_append(ol, of, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Touchpad"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}
