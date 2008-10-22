#include "e.h"
#include "e_mod_main.h"

#define BUTTON_DRAG 0
#define BUTTON_NOPLACE 1
#define BUTTON_DESK 2

struct _E_Config_Dialog_Data 
{
   struct 
     {
	int show, urgent_show, urgent_stick;
	double speed, urgent_speed;
	int height;
	int act_height;
     } popup;
   int drag_resist, flip_desk, show_desk_names;
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
	Evas_Object *o_btn1, *o_btn2, *o_btn3;
     } gui;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _adv_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _grab_wnd_show(void *data1, void *data2);
static int _grab_cb_mouse_down(void *data, int type, void *event);
static int _grab_cb_key_down(void *data, int type, void *event);
static void _grab_wnd_hide(E_Config_Dialog_Data *cfdata);
static void _adv_update_btn_lbl(E_Config_Dialog_Data *cfdata);

void 
_config_pager_module(Config_Item *ci) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;
   char buf[4096];

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   snprintf(buf, sizeof(buf), "%s/e-module-pager.edj", 
	    e_module_dir_get(pager_config->module));
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->advanced.create_widgets = _adv_create;
   v->advanced.apply_cfdata = _adv_apply;

   con = e_container_current_get(e_manager_current_get());
   cfd = e_config_dialog_new(con, _("Pager Configuration"), "E", 
			     "_e_mod_pager_config_dialog", buf, 0, v, ci);
   pager_config->config_dialog = cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfd->data, cfdata);
   return cfdata;
}

static void 
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata) 
{
   cfdata->popup.show = pager_config->popup;
   cfdata->popup.speed = pager_config->popup_speed;
   cfdata->popup.urgent_show = pager_config->popup_urgent;
   cfdata->popup.urgent_stick = pager_config->popup_urgent_stick;
   cfdata->popup.urgent_speed = pager_config->popup_urgent_speed;
   cfdata->show_desk_names = pager_config->show_desk_names;
   cfdata->popup.height = pager_config->popup_height;
   cfdata->popup.act_height = pager_config->popup_act_height;
   cfdata->drag_resist = pager_config->drag_resist;
   cfdata->btn.drag = pager_config->btn_drag;
   cfdata->btn.noplace = pager_config->btn_noplace;
   cfdata->btn.desk = pager_config->btn_desk;
   cfdata->flip_desk = pager_config->flip_desk;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   pager_config->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ow = e_widget_check_add(evas, _("Flip desktop on mouse wheel"), 
			   &(cfdata->flip_desk));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Show desktop names"), 
			   &(cfdata->show_desk_names));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Popup Settings"), 0);
   ow = e_widget_check_add(evas, _("Show popup on desktop change"), 
			   &(cfdata->popup.show));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Show popup for urgent windows"), 
			   &(cfdata->popup.urgent_show));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   pager_config->popup = cfdata->popup.show;
   pager_config->flip_desk = cfdata->flip_desk;
   pager_config->show_desk_names = cfdata->show_desk_names;
   pager_config->popup_urgent = cfdata->popup.urgent_show;
   /* FIXME: update gui with desk names */
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_frametable_add(evas, _("General Settings"), 0);
   ow = e_widget_check_add(evas, _("Flip desktop on mouse wheel"), 
			   &(cfdata->flip_desk));
   e_widget_frametable_object_append(of, ow, 0, 0, 2, 1, 1, 0, 1, 0);
   ow = e_widget_check_add(evas, _("Show desktop names"), 
			   &(cfdata->show_desk_names));
   e_widget_frametable_object_append(of, ow, 0, 1, 2, 1, 1, 0, 1, 0);

   ow = e_widget_label_add(evas, _("Select and Slide button"));
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   ow = e_widget_button_add(evas, _("Click to set"), NULL, 
			    _grab_wnd_show, (void *)BUTTON_DRAG, cfdata);
   cfdata->gui.o_btn1 = ow;
   e_widget_frametable_object_append(of, ow, 1, 2, 1, 1, 0, 0, 1, 0);

   ow = e_widget_label_add(evas, _("Drag and Drop button"));
   e_widget_frametable_object_append(of, ow, 0, 3, 1, 1, 1, 0, 1, 0);
   ow = e_widget_button_add(evas, _("Click to set"), NULL, 
			    _grab_wnd_show, (void *)BUTTON_NOPLACE, cfdata);
   cfdata->gui.o_btn2 = ow;
   e_widget_frametable_object_append(of, ow, 1, 3, 1, 1, 0, 0, 1, 0);

   ow = e_widget_label_add(evas, _("Drag whole desktop"));
   e_widget_frametable_object_append(of, ow, 0, 4, 1, 1, 1, 0, 1, 0);
   ow = e_widget_button_add(evas, _("Click to set"), NULL, 
			    _grab_wnd_show, (void *)BUTTON_DESK, cfdata);
   cfdata->gui.o_btn3 = ow;
   e_widget_frametable_object_append(of, ow, 1, 4, 1, 1, 0, 0, 1, 0);
   _adv_update_btn_lbl(cfdata);
   
   /* TODO find better name */                                                                         
   ow = e_widget_label_add(evas, _("Keyaction popup height"));                                       
   e_widget_frametable_object_append(of, ow, 0, 5, 1, 1, 1, 0, 1, 0);                                
   ow = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 20.0, 200.0, 1.0, 0, NULL,                     
                          &(cfdata->popup.act_height), 100);                                         
   e_widget_frametable_object_append(of, ow, 1, 5, 1, 1, 1, 0, 1, 0);  

   ow = e_widget_label_add(evas, _("Resistance to dragging"));
   e_widget_frametable_object_append(of, ow, 0, 6, 1, 1, 1, 0, 1, 0);
   ow = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 0.0, 10.0, 1.0, 0, NULL, 
			    &(cfdata->drag_resist), 100);
   e_widget_frametable_object_append(of, ow, 1, 6, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("Popup Settings"), 0);
   ow = e_widget_check_add(evas, _("Show popup on desktop change"), 
			   &(cfdata->popup.show));
   e_widget_frametable_object_append(of, ow, 0, 0, 2, 1, 1, 0, 1, 0);
   ow = e_widget_label_add(evas, _("Popup pager height"));
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 20.0, 200.0, 1.0, 0, NULL, 
			    &(cfdata->popup.height), 100);
   e_widget_frametable_object_append(of, ow, 1, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_label_add(evas, _("Popup speed"));
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.1, 10.0, 0.1, 0, 
			    &(cfdata->popup.speed), NULL, 100);
   e_widget_frametable_object_append(of, ow, 1, 2, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("Urgent Window Settings"), 0);
   ow = e_widget_check_add(evas, _("Show popup on urgent window"), 
			   &(cfdata->popup.urgent_show));
   e_widget_frametable_object_append(of, ow, 0, 0, 2, 1, 1, 0, 1, 0);
   ow = e_widget_check_add(evas, _("Popup on urgent window sticks on the screen"), 
			   &(cfdata->popup.urgent_stick));
   e_widget_frametable_object_append(of, ow, 0, 1, 2, 1, 1, 0, 1, 0);
   ow = e_widget_label_add(evas, _("Popup speed"));
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.1, 10.0, 0.1, 0, 
			    &(cfdata->popup.urgent_speed), NULL, 100);
   e_widget_frametable_object_append(of, ow, 1, 2, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_adv_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   pager_config->popup = cfdata->popup.show;
   pager_config->popup_speed = cfdata->popup.speed;
   pager_config->flip_desk = cfdata->flip_desk;
   pager_config->popup_urgent = cfdata->popup.urgent_show;
   pager_config->popup_urgent_stick = cfdata->popup.urgent_stick;
   pager_config->popup_urgent_speed = cfdata->popup.urgent_speed;
   pager_config->show_desk_names = cfdata->show_desk_names;
   pager_config->popup_height = cfdata->popup.height;
   pager_config->popup_act_height = cfdata->popup.act_height;
   pager_config->drag_resist = cfdata->drag_resist;
   pager_config->btn_drag = cfdata->btn.drag;
   pager_config->btn_noplace = cfdata->btn.noplace;
   pager_config->btn_desk = cfdata->btn.desk;
   /* FIXME: update gui with desk names */
   e_config_save_queue();
   return 1;
}

static void 
_grab_wnd_show(void *data1, void *data2) 
{
   E_Manager *man = NULL;
   E_Config_Dialog_Data *cfdata = NULL;
   Ecore_Event_Handler *hdl = NULL;

   if (!(cfdata = data2)) return;
   man = e_manager_current_get();

   cfdata->grab.btn = 0;
   if ((int)data1 == BUTTON_DRAG)
     cfdata->grab.btn = 1;
   else if ((int)data1 == BUTTON_NOPLACE)
     cfdata->grab.btn = 2;

   cfdata->grab.dia = e_dialog_new(e_container_current_get(man), "Pager", 
				   "_pager_button_grab_dialog");
   if (!cfdata->grab.dia) return;
   e_dialog_title_set(cfdata->grab.dia, _("Pager Button Grab"));
   e_dialog_icon_set(cfdata->grab.dia, "enlightenment/mouse_clean", 48);
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
	ecore_x_window_del(cfdata->grab.bind_win);
	cfdata->grab.bind_win = 0;
	e_object_del(E_OBJECT(cfdata->grab.dia));
	cfdata->grab.dia = NULL;
	return;
     }
   hdl = ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN, 
				 _grab_cb_key_down, cfdata);
   cfdata->grab.hdls = eina_list_append(cfdata->grab.hdls, hdl);
   hdl = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN, 
				 _grab_cb_mouse_down, cfdata);
   cfdata->grab.hdls = eina_list_append(cfdata->grab.hdls, hdl);

   e_dialog_show(cfdata->grab.dia);
   ecore_x_icccm_transient_for_set(cfdata->grab.dia->win->evas_win, 
				   pager_config->config_dialog->dia->win->evas_win);
}

static int 
_grab_cb_mouse_down(void *data, int type, void *event) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Ecore_X_Event_Mouse_Button_Down *ev;

   ev = event;
   if (!(cfdata = data)) return 1;
   if (ev->win != cfdata->grab.bind_win) return 1;

   if(ev->button == cfdata->btn.drag)
     cfdata->btn.drag = 0;
   else if (ev->button == cfdata->btn.noplace)
     cfdata->btn.noplace = 0;
   else if (ev->button == cfdata->btn.desk)
     cfdata->btn.desk = 0;
        
   if (cfdata->grab.btn == 1)
     cfdata->btn.drag = ev->button;
   else if (cfdata->grab.btn == 2)
     cfdata->btn.noplace = ev->button;
   else
     cfdata->btn.desk = ev->button;

   if(ev->button == 3) 
     {
	e_util_dialog_show(_("Attetion"), 
			   _("You cannot use the right mouse button in the<br>"
			     "shelf for this as it is already taken by internal<br>"
			     "code for context menus. <br>"
			     "This button only works in the Popup"));
     }
   _grab_wnd_hide(cfdata);
   return 1;
}

static int 
_grab_cb_key_down(void *data, int type, void *event) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Ecore_X_Event_Key_Down *ev;

   ev = event;
   if (!(cfdata = data)) return 1;
   if (ev->win != cfdata->grab.bind_win) return 1;
   if (!strcmp(ev->keyname, "Escape")) _grab_wnd_hide(cfdata);
   if (!strcmp(ev->keyname, "Delete")) 
     {
	if (cfdata->grab.btn == 1)
	  cfdata->btn.drag = 0;
	else if (cfdata->grab.btn == 2)
	  cfdata->btn.noplace = 0;
	else
	  cfdata->btn.desk = 0;
	_grab_wnd_hide(cfdata);
     }
   return 1;
}

static void 
_grab_wnd_hide(E_Config_Dialog_Data *cfdata) 
{
   while (cfdata->grab.hdls) 
     {
	ecore_event_handler_del(cfdata->grab.hdls->data);
	cfdata->grab.hdls = eina_list_remove_list(cfdata->grab.hdls, cfdata->grab.hdls);
     }
   cfdata->grab.hdls = NULL;
   e_grabinput_release(cfdata->grab.bind_win, cfdata->grab.bind_win);
   if (cfdata->grab.bind_win) ecore_x_window_del(cfdata->grab.bind_win);
   cfdata->grab.bind_win = 0;

   if (cfdata->grab.dia) 
     e_object_del(E_OBJECT(cfdata->grab.dia));
   cfdata->grab.dia = NULL;

   _adv_update_btn_lbl(cfdata);
}

static void 
_adv_update_btn_lbl(E_Config_Dialog_Data *cfdata) 
{
   char lbl[256] = "";

   snprintf(lbl, sizeof(lbl), _("Click to set"));
   if (cfdata->btn.drag)
     snprintf(lbl, sizeof(lbl), _("Button %i"), cfdata->btn.drag);
   e_widget_button_label_set(cfdata->gui.o_btn1, lbl);

   snprintf(lbl, sizeof(lbl), _("Click to set"));
   if (cfdata->btn.noplace)
     snprintf(lbl, sizeof(lbl), _("Button %i"), cfdata->btn.noplace);
   e_widget_button_label_set(cfdata->gui.o_btn2, lbl);

   snprintf(lbl, sizeof(lbl), _("Click to set"));
   if (cfdata->btn.desk)
     snprintf(lbl, sizeof(lbl), _("Button %i"), cfdata->btn.desk);
   e_widget_button_label_set(cfdata->gui.o_btn3, lbl);
}
