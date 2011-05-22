#include "e.h"
#include "e_mod_main.h"

#define BUTTON_DRAG 0
#define BUTTON_NOPLACE 1
#define BUTTON_DESK 2

struct _E_Config_Dialog_Data
{
   struct
     {
        int show, urgent_show, urgent_stick, urgent_focus;
        double speed, urgent_speed;
        int height, act_height;
     } popup;
   struct
     {
        unsigned int drag, noplace, desk;
     } btn;
   struct
     {
        Ecore_X_Window bind_win;
        E_Dialog *dia;
        Eina_List *hdls;
        int btn;
     } grab;
   struct
     {
        Evas_Object *ob1, *ob2, *ob3;
        Eina_List *popup_list, *urgent_list;
     } gui;
   int drag_resist, flip_desk, show_desk_names;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int _adv_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void _update_btn_lbl(E_Config_Dialog_Data *cfdata);
static void _grab_window_show(void *data1, void *data2);
static Eina_Bool _grab_cb_mouse_down(void *data, int type, void *event);
static Eina_Bool _grab_cb_key_down(void *data, int type, void *event);
static void _grab_window_hide(E_Config_Dialog_Data *cfdata);
static void _cb_disable_check_list(void *data, Evas_Object *obj);

void 
_config_pager_module(Config_Item *ci) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;
   char buff[PATH_MAX];

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;
   v->advanced.create_widgets = _adv_create;
   v->advanced.apply_cfdata = _adv_apply;
   v->advanced.check_changed = _adv_check_changed;

   snprintf(buff, sizeof(buff), "%s/e-module-pager.edj", 
            pager_config->module->dir);
   con = e_container_current_get(e_manager_current_get());
   cfd = e_config_dialog_new(con, _("Pager Settings"), "E", 
                             "_e_mod_pager_config_dialog", buff, 0, v, ci);
   pager_config->config_dialog = cfd;
}

/* local function prototypes */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->popup.show = pager_config->popup;
   cfdata->popup.speed = pager_config->popup_speed;
   cfdata->popup.urgent_show = pager_config->popup_urgent;
   cfdata->popup.urgent_stick = pager_config->popup_urgent_stick;
   cfdata->popup.urgent_focus = pager_config->popup_urgent_focus;
   cfdata->popup.urgent_speed = pager_config->popup_urgent_speed;
   cfdata->popup.height = pager_config->popup_height;
   cfdata->popup.act_height = pager_config->popup_act_height;
   cfdata->drag_resist = pager_config->drag_resist;
   cfdata->btn.drag = pager_config->btn_drag;
   cfdata->btn.noplace = pager_config->btn_noplace;
   cfdata->btn.desk = pager_config->btn_desk;
   cfdata->flip_desk = pager_config->flip_desk;
   cfdata->show_desk_names = pager_config->show_desk_names;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   cfdata->gui.popup_list = eina_list_free(cfdata->gui.popup_list);
   cfdata->gui.urgent_list = eina_list_free(cfdata->gui.urgent_list);
   pager_config->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ol, *of, *ow;

   ol = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   ow = e_widget_check_add(evas, _("Flip desktop on mouse wheel"), 
                           &(cfdata->flip_desk));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Show desktop names"), 
                           &(cfdata->show_desk_names));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Popup"), 0);
   ow = e_widget_check_add(evas, _("Show popup on desktop change"), 
                           &(cfdata->popup.show));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Show popup for urgent windows"), 
                           &(cfdata->popup.urgent_show));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   return ol;
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   pager_config->popup = cfdata->popup.show;
   pager_config->flip_desk = cfdata->flip_desk;
   pager_config->show_desk_names = cfdata->show_desk_names;
   pager_config->popup_urgent = cfdata->popup.urgent_show;
   e_config_save_queue();
   return 1;
}

static int 
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   if ((int)pager_config->popup != cfdata->popup.show) return 1;
   if ((int)pager_config->flip_desk != cfdata->flip_desk) return 1;
   if ((int)pager_config->show_desk_names != cfdata->show_desk_names) return 1;
   if ((int)pager_config->popup_urgent != cfdata->popup.urgent_show) return 1;

   return 0;
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *otb, *ol, *ow;
   Evas_Object *pc, *uc;

   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));

   /* General Page */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Flip desktop on mouse wheel"), 
                           &(cfdata->flip_desk));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Show desktop names"), 
                           &(cfdata->show_desk_names));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Resistance to dragging"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 0.0, 10.0, 1.0, 0, NULL, 
                            &(cfdata->drag_resist), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);

#if 0
   ow = e_widget_label_add(evas, _("Select and Slide button"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_button_add(evas, _("Click to set"), NULL, 
                            _grab_window_show, (void *)BUTTON_DRAG, cfdata);
   cfdata->gui.ob1 = ow;
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
#endif

   ow = e_widget_label_add(evas, _("Drag and Drop button"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_button_add(evas, _("Click to set"), NULL, 
                            _grab_window_show, (void *)BUTTON_NOPLACE, cfdata);
   cfdata->gui.ob2 = ow;
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Drag whole desktop"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_button_add(evas, _("Click to set"), NULL, 
                            _grab_window_show, (void *)BUTTON_DESK, cfdata);
   cfdata->gui.ob3 = ow;
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   _update_btn_lbl(cfdata);
   e_widget_toolbook_page_append(otb, NULL, _("General"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   /* Popup Page */
   ol = e_widget_list_add(evas, 0, 0);
   pc = e_widget_check_add(evas, _("Show popup on desktop change"), 
                           &(cfdata->popup.show));
   e_widget_list_object_append(ol, pc, 1, 0, 0.5);

   ow = e_widget_label_add(evas, _("Popup pager height"));
   cfdata->gui.popup_list = eina_list_append(cfdata->gui.popup_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 20.0, 200.0, 1.0, 0, NULL, 
                            &(cfdata->popup.height), 100);
   cfdata->gui.popup_list = eina_list_append(cfdata->gui.popup_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);

   ow = e_widget_label_add(evas, _("Popup speed"));
   cfdata->gui.popup_list = eina_list_append(cfdata->gui.popup_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.1, 10.0, 0.1, 0, 
                            &(cfdata->popup.speed), NULL, 100);
   cfdata->gui.popup_list = eina_list_append(cfdata->gui.popup_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_on_change_hook_set(pc, _cb_disable_check_list, 
                               cfdata->gui.popup_list);
   ow = e_widget_label_add(evas, _("Pager action popup height"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 20.0, 200.0, 1.0, 0, NULL, 
                            &(cfdata->popup.act_height), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Popup"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   /* Urgent Page */
   ol = e_widget_list_add(evas, 0, 0);
   uc = e_widget_check_add(evas, _("Show popup on urgent window"), 
                           &(cfdata->popup.urgent_show));
   e_widget_list_object_append(ol, uc, 1, 0, 0.5);

   ow = e_widget_check_add(evas, _("Urgent popup sticks on screen"),
                           &(cfdata->popup.urgent_stick));
   cfdata->gui.urgent_list = eina_list_append(cfdata->gui.urgent_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.urgent_show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);

   ow = e_widget_check_add(evas, _("Show popup for focused windows"),
                           &(cfdata->popup.urgent_focus));
   cfdata->gui.urgent_list = eina_list_append(cfdata->gui.urgent_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.urgent_show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);

   ow = e_widget_label_add(evas, _("Urgent popup speed"));
   cfdata->gui.urgent_list = eina_list_append(cfdata->gui.urgent_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.urgent_show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.1, 10.0, 0.1, 0, 
                            &(cfdata->popup.urgent_speed), NULL, 100);
   cfdata->gui.urgent_list = eina_list_append(cfdata->gui.urgent_list, ow);
   e_widget_disabled_set(ow, !cfdata->popup.urgent_show);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_on_change_hook_set(uc, _cb_disable_check_list, 
                               cfdata->gui.urgent_list);
   e_widget_toolbook_page_append(otb, NULL, _("Urgent Windows"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int 
_adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   pager_config->popup = cfdata->popup.show;
   pager_config->popup_speed = cfdata->popup.speed;
   pager_config->flip_desk = cfdata->flip_desk;
   pager_config->popup_urgent = cfdata->popup.urgent_show;
   pager_config->popup_urgent_stick = cfdata->popup.urgent_stick;
   pager_config->popup_urgent_focus = cfdata->popup.urgent_focus;
   pager_config->popup_urgent_speed = cfdata->popup.urgent_speed;
   pager_config->show_desk_names = cfdata->show_desk_names;
   pager_config->popup_height = cfdata->popup.height;
   pager_config->popup_act_height = cfdata->popup.act_height;
   pager_config->drag_resist = cfdata->drag_resist;
   pager_config->btn_drag = cfdata->btn.drag;
   pager_config->btn_noplace = cfdata->btn.noplace;
   pager_config->btn_desk = cfdata->btn.desk;
   e_config_save_queue();
   return 1;
}

static int 
_adv_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   if ((int)pager_config->popup != cfdata->popup.show) return 1;
   if ((int)pager_config->flip_desk != cfdata->flip_desk) return 1;
   if ((int)pager_config->show_desk_names != cfdata->show_desk_names) return 1;
   if ((int)pager_config->popup_urgent != cfdata->popup.urgent_show) return 1;
   if (pager_config->popup_speed != cfdata->popup.speed) return 1;
   if ((int)pager_config->popup_urgent_stick != cfdata->popup.urgent_stick) 
     return 1;
   if ((int)pager_config->popup_urgent_focus != cfdata->popup.urgent_focus) 
     return 1;
   if (pager_config->popup_urgent_speed != cfdata->popup.urgent_speed) 
     return 1;
   if (pager_config->popup_height != cfdata->popup.height) return 1;
   if (pager_config->popup_act_height != cfdata->popup.act_height) return 1;
   if ((int)pager_config->drag_resist != cfdata->drag_resist) return 1;
   if (pager_config->btn_drag != cfdata->btn.drag) return 1;
   if (pager_config->btn_noplace != cfdata->btn.noplace) return 1;
   if (pager_config->btn_desk != cfdata->btn.desk) return 1;

   return 0;
}

static void 
_update_btn_lbl(E_Config_Dialog_Data *cfdata) 
{
   char lbl[256];

#if 0
   snprintf(lbl, sizeof(lbl), _("Click to set"));
   if (cfdata->btn.drag)
     snprintf(lbl, sizeof(lbl), _("Button %i"), cfdata->btn.drag);
   e_widget_button_label_set(cfdata->gui.ob1, lbl);
#endif
   snprintf(lbl, sizeof(lbl), _("Click to set"));
   if (cfdata->btn.noplace)
     snprintf(lbl, sizeof(lbl), _("Button %i"), cfdata->btn.noplace);
   e_widget_button_label_set(cfdata->gui.ob2, lbl);
   snprintf(lbl, sizeof(lbl), _("Click to set"));
   if (cfdata->btn.desk)
     snprintf(lbl, sizeof(lbl), _("Button %i"), cfdata->btn.desk);
   e_widget_button_label_set(cfdata->gui.ob3, lbl);
}

static void 
_grab_window_show(void *data1, void *data2) 
{
   E_Manager *man = NULL;
   E_Config_Dialog_Data *cfdata = NULL;
   Ecore_Event_Handler *hdl = NULL;

   if (!(cfdata = data2)) return;
   man = e_manager_current_get();

   cfdata->grab.btn = 0;
   if ((long)data1 == BUTTON_DRAG)
     cfdata->grab.btn = 1;
   else if ((long)data1 == BUTTON_NOPLACE)
     cfdata->grab.btn = 2;

   cfdata->grab.dia = e_dialog_new(e_container_current_get(man), "Pager",
                                   "_pager_button_grab_dialog");
   if (!cfdata->grab.dia) return;
   e_dialog_title_set(cfdata->grab.dia, _("Pager Button Grab"));
   e_dialog_icon_set(cfdata->grab.dia, "preferences-desktop-mouse", 48);
   e_dialog_text_set(cfdata->grab.dia, _("Please press a mouse button<br>"
                                         "Press <hilight>Escape</hilight> to abort.<br>"
                                         "Or <hilight>Del</hilight> to reset the button."));
   e_win_centered_set(cfdata->grab.dia->win, 1);
   e_win_borderless_set(cfdata->grab.dia->win, 1);

   cfdata->grab.bind_win = ecore_x_window_input_new(man->root, 0, 0,
                                                    man->w, man->h);
   ecore_x_window_show(cfdata->grab.bind_win);
   if (!e_grabinput_get(cfdata->grab.bind_win, 0, cfdata->grab.bind_win))
     {
        ecore_x_window_free(cfdata->grab.bind_win);
        cfdata->grab.bind_win = 0;
        e_object_del(E_OBJECT(cfdata->grab.dia));
        cfdata->grab.dia = NULL;
        return;
     }
   hdl = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                 _grab_cb_key_down, cfdata);
   cfdata->grab.hdls = eina_list_append(cfdata->grab.hdls, hdl);
   hdl = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                 _grab_cb_mouse_down, cfdata);
   cfdata->grab.hdls = eina_list_append(cfdata->grab.hdls, hdl);

   e_dialog_show(cfdata->grab.dia);
   ecore_x_icccm_transient_for_set(cfdata->grab.dia->win->evas_win,
                                   pager_config->config_dialog->dia->win->evas_win);
}

static Eina_Bool
_grab_cb_mouse_down(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   E_Config_Dialog_Data *cfdata = NULL;
   Ecore_Event_Mouse_Button *ev;

   ev = event;
   if (!(cfdata = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->window != cfdata->grab.bind_win) return ECORE_CALLBACK_PASS_ON;

   if (ev->buttons == cfdata->btn.drag)
     cfdata->btn.drag = 0;
   else if (ev->buttons == cfdata->btn.noplace)
     cfdata->btn.noplace = 0;
   else if (ev->buttons == cfdata->btn.desk)
     cfdata->btn.desk = 0;

   if (cfdata->grab.btn == 1)
     cfdata->btn.drag = ev->buttons;
   else if (cfdata->grab.btn == 2)
     cfdata->btn.noplace = ev->buttons;
   else
     cfdata->btn.desk = ev->buttons;

   if (ev->buttons == 3)
     {
        e_util_dialog_show(_("Attention"),
                           _("You cannot use the right mouse button in the<br>"
                             "shelf for this as it is already taken by internal<br>"
                             "code for context menus.<br>"
                             "This button only works in the popup."));
     }
   _grab_window_hide(cfdata);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_grab_cb_key_down(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   E_Config_Dialog_Data *cfdata = NULL;
   Ecore_Event_Key *ev;

   ev = event;
   if (!(cfdata = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->window != cfdata->grab.bind_win) return ECORE_CALLBACK_PASS_ON;
   if (!strcmp(ev->keyname, "Escape")) _grab_window_hide(cfdata);
   if (!strcmp(ev->keyname, "Delete"))
     {
        if (cfdata->grab.btn == 1)
          cfdata->btn.drag = 0;
        else if (cfdata->grab.btn == 2)
          cfdata->btn.noplace = 0;
        else
          cfdata->btn.desk = 0;
        _grab_window_hide(cfdata);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_grab_window_hide(E_Config_Dialog_Data *cfdata)
{
   while (cfdata->grab.hdls)
     {
        ecore_event_handler_del(cfdata->grab.hdls->data);
        cfdata->grab.hdls = 
          eina_list_remove_list(cfdata->grab.hdls, cfdata->grab.hdls);
     }
   cfdata->grab.hdls = NULL;
   e_grabinput_release(cfdata->grab.bind_win, cfdata->grab.bind_win);
   if (cfdata->grab.bind_win) ecore_x_window_free(cfdata->grab.bind_win);
   cfdata->grab.bind_win = 0;

   if (cfdata->grab.dia) e_object_del(E_OBJECT(cfdata->grab.dia));
   cfdata->grab.dia = NULL;

   _update_btn_lbl(cfdata);
}

static void 
_cb_disable_check_list(void *data, Evas_Object *obj) 
{
   Eina_List *list = (Eina_List*) data;
   Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(list, l, o)
     e_widget_disabled_set(o, !e_widget_check_checked_get(obj));
}
