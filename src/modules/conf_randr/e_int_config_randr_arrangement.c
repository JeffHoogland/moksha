#include "e_int_config_randr.h"
#include "e_randr.h"
#include "Ecore_X.h"

#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)
#endif

#ifndef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset -1
#endif

#define DOUBLECLICK_TIMEOUT 0.2
#define CRTC_THUMB_SIZE_W   300
#define CRTC_THUMB_SIZE_H   300

Eina_Bool                arrangement_widget_create_data(E_Config_Dialog_Data *e_config_runtime_info);
Evas_Object             *arrangement_widget_basic_create_widgets(Evas *canvas);
Eina_Bool                arrangement_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool                arrangement_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void                     arrangement_widget_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

//static inline E_Config_Randr_Dialog_Output_Dialog_Data *_arrangement_widget_rep_dialog_data_new        (E_Randr_Crtc_Info *crtc_info, E_Randr_Output_Info *output_info);
static inline Evas_Object *_arrangement_widget_suggestion_add(Evas *evas);
static inline void       _arrangement_widget_make_suggestion(Evas_Object *obj);
static Evas_Object      *_arrangement_widget_rep_add(Evas *canvas, E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data);
static void              _arrangement_widget_rep_del(E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data);
static void              _arrangement_widget_rep_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void              _arrangement_widget_rep_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void              _arrangement_widget_rep_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void              _arrangement_widget_check_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void              _arrangement_widget_update(void);

extern E_Config_Dialog_Data *e_config_runtime_info;
extern Config *randr_dialog_config;
extern char _theme_file_path[];

static void
_arrangement_widget_rep_dialog_data_fill(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   if (!odd) return;

   if (odd->crtc)
     {
        //already enabled screen
        odd->previous_pos.x = odd->crtc->geometry.x;
        odd->previous_pos.y = odd->crtc->geometry.y;
        odd->previous_mode = odd->crtc->current_mode;
     }
   else if (odd->output)
     {
        //disabled monitor
        odd->previous_mode = NULL;
        if (odd->output->monitor)
          {
             //try to get a mode from the preferred list, else use default list
             if (!(odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(eina_list_last(odd->output->monitor->preferred_modes))))
               {
                  if (odd->output->monitor->modes)
                    odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(eina_list_last(odd->output->monitor->modes));
               }
          }
        else
          {
             odd->preferred_mode = NULL;
          }

        odd->previous_pos.x = 0;
        odd->previous_pos.y = 0;
     }
}

void
_arrangement_widget_update(void)
{
   static Evas_Object *area = NULL;
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   static Eina_Rectangle default_geo;
   Eina_Rectangle geo = {.x = 0, .y = 0, .w = 0, .h = 0};
   static char *edje_data_item = NULL;
   Eina_List *iter;

   area = e_config_runtime_info->gui.widgets.arrangement.area;

   if (!e_config_runtime_info || !e_config_runtime_info->gui.canvas || !e_config_runtime_info->output_dialog_data_list || !area) return;

   fprintf(stderr, "CONF_RANDR: Display disconnected outputs: %s\n", (randr_dialog_config->display_disconnected_outputs ? "YES" : "NO"));

   if (!edje_data_item)
     {
        if(!(edje_data_item = edje_file_data_get(_theme_file_path, "disabled_output_width")))
          edje_data_item = "1024";
        default_geo.w = atoi(edje_data_item);

        if(!(edje_data_item = edje_file_data_get(_theme_file_path, "disabled_output_height")))
          edje_data_item = "768";
        default_geo.h = atoi(edje_data_item);

        default_geo.x = e_randr_screen_info.rrvd_info.randr_info_12->max_size.width - default_geo.w;
        default_geo.y = 0;
     }

   e_layout_freeze(area);
   e_layout_unpack(area);
   e_layout_virtual_size_set(area, e_randr_screen_info.rrvd_info.randr_info_12->max_size.width, e_randr_screen_info.rrvd_info.randr_info_12->max_size.height);
   fprintf(stderr, "CONF_RANDR: Set virtual size of arrangement e_layout to %dx%d\n", e_randr_screen_info.rrvd_info.randr_info_12->max_size.width, e_randr_screen_info.rrvd_info.randr_info_12->max_size.height);
   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, iter, odd)
     {
        _arrangement_widget_rep_del(odd);

        if(!odd->crtc &&
              (!odd->output->monitor && (randr_dialog_config && !randr_dialog_config->display_disconnected_outputs)))
          continue;

        if(!(odd->rep = _arrangement_widget_rep_add(e_config_runtime_info->gui.canvas, odd)))
          {
             fprintf(stderr, "CONF_RANDR: Could not add rep for CRTC %p/ output %p.\n", odd->crtc, odd->output);
             continue;
          }
        if (odd->crtc && odd->crtc->current_mode)
          {
             geo.x = odd->crtc->geometry.x;
             geo.y = odd->crtc->geometry.y;
             geo.w = odd->crtc->geometry.w;
             geo.h = odd->crtc->geometry.h;
          }
        else
          {
             memcpy(&geo, &default_geo, sizeof(geo));
          }
        evas_object_data_set(odd->rep, "rep_info", odd);
        evas_object_show(odd->rep);
        e_layout_pack(area, odd->rep);
        e_layout_child_raise(odd->rep);
        e_layout_child_resize(odd->rep, geo.w, geo.h);
        e_layout_child_move(odd->rep, geo.x, geo.y);

        fprintf(stderr, "CONF_RANDR: Representation (%p) added with geo %d.%d %dx%d.\n", odd->rep, geo.x, geo.y, geo.w, geo.h);
     }

   if (e_config_runtime_info->gui.widgets.arrangement.suggestion)
     {
        e_layout_pack(area, e_config_runtime_info->gui.widgets.arrangement.suggestion);
     }

   e_layout_thaw(area);
}

Eina_Bool
arrangement_widget_create_data(E_Config_Dialog_Data *data)
{
   Eina_List *iter;
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;

   EINA_LIST_FOREACH(data->output_dialog_data_list, iter, dialog_data)
     {
        _arrangement_widget_rep_dialog_data_fill(dialog_data);
     }

   return EINA_TRUE;
}

Evas_Object *
arrangement_widget_basic_create_widgets(Evas *canvas)
{
   Evas_Object *widget, *scrollframe, *area, *check;

   if (!canvas || !e_config_runtime_info || !e_config_runtime_info->output_dialog_data_list) return NULL;

   widget = e_widget_list_add(canvas, 0, 0);
   fprintf(stderr, "CONF_RANDR: Arrangement widget added (%p).\n", widget);

   //Add checkbox
   check = e_widget_check_add(canvas, _("Display disconnected outputs"), &e_config_runtime_info->gui.widgets.arrangement.check_val_display_disconnected_outputs);
   if (randr_dialog_config)
     e_widget_check_checked_set(check, randr_dialog_config->display_disconnected_outputs);
   evas_object_event_callback_add(check, EVAS_CALLBACK_MOUSE_DOWN, _arrangement_widget_check_mouse_down_cb, NULL);
   e_config_runtime_info->gui.widgets.arrangement.check_display_disconnected_outputs = check;

   area = e_layout_add(canvas);
   e_config_runtime_info->gui.widgets.arrangement.area = area;
   e_layout_virtual_size_set(area, e_randr_screen_info.rrvd_info.randr_info_12->max_size.width, e_randr_screen_info.rrvd_info.randr_info_12->max_size.height);
   evas_object_resize(area, 500, 500);
   evas_object_show(area);

   // Add suggestion element, hidden
   e_config_runtime_info->gui.widgets.arrangement.suggestion = _arrangement_widget_suggestion_add(canvas);

   _arrangement_widget_update();

   scrollframe = e_scrollframe_add(canvas);
   e_scrollframe_child_set(scrollframe, area);
   e_config_runtime_info->gui.widgets.arrangement.scrollframe = scrollframe;

   // Append both objects to widget list
   e_widget_list_object_append(widget, scrollframe, 1, 1, 0.0);
   e_widget_list_object_append(widget, check, 0, 0, 1.0);

   e_config_runtime_info->gui.widgets.arrangement.widget_list = widget;

   return widget;
}

static Evas_Object *
_arrangement_widget_rep_add(Evas *canvas, E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data)
{
   E_Randr_Output_Info *output_info;
   Evas_Object *rep;
   const char *output_name = NULL, *state_signal;

   if (!canvas || !output_dialog_data || !e_config_runtime_info) return NULL;

   rep = edje_object_add(canvas);

   //set theme for monitor representation
   EINA_SAFETY_ON_FALSE_GOTO(edje_object_file_set(rep, _theme_file_path, "e/conf/randr/dialog/widget/arrangement/output"), _arrangement_widget_rep_add_edje_set_fail);
   //indicate monitor state
   if (!(output_dialog_data->crtc && output_dialog_data->crtc->current_mode))
     state_signal = "disabled";
   else
     state_signal = "enabled";
   edje_object_signal_emit(rep, state_signal, "e");
   //for now use deskpreview widget as background of rep, maybe change this to
   //live image from comp module
   output_dialog_data->bg = e_widget_deskpreview_add(canvas, 1, 1);
   //output_dialog_data->bg = e_livethumb_add(canvas);
   edje_object_part_swallow(rep, "e.swallow.content", output_dialog_data->bg);

   //Try to get the name of the monitor connected to the CRTC's first output via edid
   //else use the output's name
   if (output_dialog_data->crtc)
     output_info = (E_Randr_Output_Info *)eina_list_data_get(output_dialog_data->crtc->outputs);
   else
     output_info = output_dialog_data->output;
   if (output_info)
     {
        if (output_info->monitor)
          output_name = ecore_x_randr_edid_display_name_get(output_info->monitor->edid, output_info->monitor->edid_length);
        if (!output_name && output_info->name)
          output_name = output_info->name;
     }
   if (output_name)
     edje_object_part_text_set(rep, "output_txt", output_name);

   evas_object_event_callback_add (rep, EVAS_CALLBACK_MOUSE_DOWN, _arrangement_widget_rep_mouse_down_cb, NULL);
   evas_object_event_callback_add (rep, EVAS_CALLBACK_MOUSE_MOVE, _arrangement_widget_rep_mouse_move_cb, NULL);
   evas_object_event_callback_add (rep, EVAS_CALLBACK_MOUSE_UP, _arrangement_widget_rep_mouse_up_cb, NULL);

   return rep;

_arrangement_widget_rep_add_edje_set_fail:
   evas_object_del(rep);
   return NULL;
}

static void
_arrangement_widget_rep_del(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   if (!odd)
     return;

   evas_object_hide(odd->rep);

   evas_object_event_callback_del(odd->rep, EVAS_CALLBACK_MOUSE_DOWN, _arrangement_widget_rep_mouse_down_cb);
   evas_object_event_callback_del(odd->rep, EVAS_CALLBACK_MOUSE_MOVE, _arrangement_widget_rep_mouse_move_cb);
   evas_object_event_callback_del(odd->rep, EVAS_CALLBACK_MOUSE_UP, _arrangement_widget_rep_mouse_up_cb);

   //get instance data for output
   edje_object_part_unswallow(odd->rep, odd->bg);
   evas_object_del(odd->bg);
   odd->bg = NULL;

   //update output orientation
   orientation_widget_update_edje(NULL);

   evas_object_del(odd->rep);
   odd->rep = NULL;
}

static void
_arrangement_widget_check_mouse_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   if (!obj || !e_config_runtime_info || !randr_dialog_config) return;

   if (obj == e_config_runtime_info->gui.widgets.arrangement.check_display_disconnected_outputs)
     {
        //this is bad. The events are called _before_ the value is updated.
        randr_dialog_config->display_disconnected_outputs ^= EINA_TRUE;
        _arrangement_widget_update();
     }
}

static void
_arrangement_widget_rep_mouse_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, iter, odd)
     {
        if (odd->rep == obj)
          continue;
        edje_object_signal_emit(odd->rep, "deselect", "e");
     }

   edje_object_signal_emit(obj, "select", "e");
   //update data for other dialogs
   e_config_runtime_info->gui.selected_eo = obj;
   e_config_runtime_info->gui.selected_output_dd = (E_Config_Randr_Dialog_Output_Dialog_Data*)evas_object_data_get(obj, "rep_info");

   //update resolutions list
   resolution_widget_update_list(obj);

   //update orientation radio buttons
   orientation_widget_update_radio_buttons(obj);

   //update policy radio buttons
   policy_widget_update_radio_buttons(obj);
}

static void
_arrangement_widget_rep_mouse_move_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Eina_Rectangle geo, parent;
   Evas_Coord_Point delta, new;

   if (ev->buttons != 1) return;
   evas_object_geometry_get(e_config_runtime_info->gui.widgets.arrangement.area, &parent.x, &parent.y, NULL, NULL);
   e_layout_virtual_size_get(e_config_runtime_info->gui.widgets.arrangement.area, &parent.w, &parent.h);

   delta.x = ev->cur.canvas.x - ev->prev.canvas.x;
   delta.y = ev->cur.canvas.y - ev->prev.canvas.y;
   //add parent.x and parent.y to delta, as they are subtraced in the e_layouts'
   //coord transformation
   e_layout_coord_canvas_to_virtual(e_config_runtime_info->gui.widgets.arrangement.area, (parent.x + delta.x), (parent.y + delta.y), &new.x, &new.y);

   e_layout_child_geometry_get(obj, &geo.x, &geo.y, &geo.w, &geo.h);
   new.x += geo.x;
   new.y += geo.y;
   //respect container borders
   if (new.x < parent.x + 1)
     new.x = parent.x + 1;
   else if (new.x > parent.x + parent.w - geo.w)
     new.x = parent.x + parent.w - geo.w;
   if (new.y < parent.y + 1)
     new.y = parent.y + 1;
   else if (new.y > parent.y + parent.h - geo.h)
     new.y = parent.y + parent.h - geo.h;
   //only take action if position changed
   if ((geo.x != new.x) || (geo.y != new.y))
     {
        e_layout_child_move(obj, new.x, new.y);
        _arrangement_widget_make_suggestion(obj);
     }
}

static void
_arrangement_widget_rep_mouse_up_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Coord_Point coords;

   if (evas_object_visible_get(e_config_runtime_info->gui.widgets.arrangement.suggestion))
     {
        edje_object_signal_emit(e_config_runtime_info->gui.widgets.arrangement.suggestion, "hide", "e");
        evas_object_hide(e_config_runtime_info->gui.widgets.arrangement.suggestion);
        e_layout_child_geometry_get(e_config_runtime_info->gui.widgets.arrangement.suggestion, &coords.x, &coords.y, NULL, NULL);
        e_layout_child_move(obj, coords.x, coords.y);
     }
}

Evas_Object *
_arrangement_widget_suggestion_add(Evas *evas)
{
   Evas_Object *sug = NULL;
   const char *theme_data_item = NULL;
   int min_dist = 0;
   Evas_Coord parent_x;

   evas_object_geometry_get(e_config_runtime_info->gui.widgets.arrangement.area, &parent_x, NULL, NULL, NULL);

   sug = edje_object_add(evas);
   edje_object_file_set(sug, _theme_file_path, "e/conf/randr/dialog/widget/arrangement/suggestion");
   if ((theme_data_item = edje_object_data_get(sug, "distance_min")))
     min_dist = MAX(atoi(theme_data_item), 0);
   else
     min_dist = 20;

   e_layout_coord_canvas_to_virtual(e_config_runtime_info->gui.widgets.arrangement.area, (parent_x + min_dist), 0, &e_config_runtime_info->gui.widgets.arrangement.suggestion_dist_min, NULL);

   return sug;
}

void
_arrangement_widget_make_suggestion(Evas_Object *obj)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Evas_Object *rep = NULL;
   Eina_Rectangle p_geo = {.x = 0, .y = 0, .w = 0, .h = 0}, geo, rep_geo, s_geo;
   int dxa = INT_MAX, dya = INT_MAX, tmp, min_dist;
   Eina_List *li;

   if (!obj) return;

   min_dist = e_config_runtime_info->gui.widgets.arrangement.suggestion_dist_min;

   e_layout_child_geometry_get(obj, &geo.x, &geo.y, &geo.w, &geo.h);
   e_layout_virtual_size_get(e_config_runtime_info->gui.widgets.arrangement.area, &p_geo.w, &p_geo.h);

   s_geo.x = geo.x;
   s_geo.y = geo.y;
   s_geo.w = geo.w;
   s_geo.h = geo.h;

   //compare possible positions
   //aritifical (relative) 0x0 element
   tmp = s_geo.x;
   if ((tmp < dxa) && (tmp < min_dist))
     {
        s_geo.x = p_geo.x;
        dxa = tmp;
     }
   tmp = s_geo.y;
   if ((tmp < dya) && (tmp < min_dist))
     {
        s_geo.y = p_geo.y;
        dya = tmp;
     }

   //iterate rep list
   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, li, odd)
     {
        if (!odd) continue;
        rep = odd->rep;
        if (!rep || (rep == obj))
          continue;

        e_layout_child_geometry_get(rep, &rep_geo.x, &rep_geo.y, &rep_geo.w, &rep_geo.h);
        //X-Axis
        tmp = abs(s_geo.x - rep_geo.x);
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = rep_geo.x;
             dxa = abs(s_geo.x - rep_geo.x);
          }

        tmp = abs(s_geo.x - (rep_geo.x + rep_geo.w));
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = (rep_geo.x + rep_geo.w);
             dxa = tmp;
          }

        tmp = abs((s_geo.x + s_geo.w) - (rep_geo.x - 1));
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = (rep_geo.x - s_geo.w);
             dxa = tmp;
          }

        tmp = abs((s_geo.x + s_geo.w) - (rep_geo.x + rep_geo.w));
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = (rep_geo.x + rep_geo.w - s_geo.w);
             dxa = tmp;
          }

        //Y-Axis
        tmp = abs(s_geo.y - rep_geo.y);
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = rep_geo.y;
             dya = abs(s_geo.y - rep_geo.y);
          }

        tmp = abs(s_geo.y - (rep_geo.y + rep_geo.h));
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = (rep_geo.y + rep_geo.h);
             dya = tmp;
          }

        tmp = abs((s_geo.y + s_geo.h) - (rep_geo.y - 1));
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = (rep_geo.y - s_geo.h);
             dya = tmp;
          }

        tmp = abs((s_geo.y + s_geo.h) - (rep_geo.y + rep_geo.h));
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = (rep_geo.y + rep_geo.h - s_geo.h);
             dya = tmp;
          }
     }

   if ((s_geo.x != geo.x) || (s_geo.y != geo.y))
     {
        if (s_geo.x < p_geo.x) s_geo.x = p_geo.x;
        if ((s_geo.x + s_geo.w) > (p_geo.x + p_geo.w)) s_geo.x = ((p_geo.x + p_geo.w) - s_geo.w);
        if (s_geo.y < p_geo.y) s_geo.y = p_geo.y;
        if ((s_geo.y + s_geo.h) > (p_geo.y + p_geo.h)) s_geo.y = ((p_geo.y + p_geo.h) - s_geo.h);

        if (!evas_object_visible_get(e_config_runtime_info->gui.widgets.arrangement.suggestion))
          {
             evas_object_show(e_config_runtime_info->gui.widgets.arrangement.suggestion);
             edje_object_signal_emit(e_config_runtime_info->gui.widgets.arrangement.suggestion, "show", "e");
          }

        e_layout_child_resize(e_config_runtime_info->gui.widgets.arrangement.suggestion, s_geo.w, s_geo.h);
        e_layout_child_move(e_config_runtime_info->gui.widgets.arrangement.suggestion, s_geo.x, s_geo.y);
        e_layout_child_raise(e_config_runtime_info->gui.widgets.arrangement.suggestion);
     }
   else
     {
        edje_object_signal_emit(e_config_runtime_info->gui.widgets.arrangement.suggestion, "hide", "e");
        evas_object_hide(e_config_runtime_info->gui.widgets.arrangement.suggestion);
     }
}

void
arrangement_widget_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;
   Eina_List *iter;

   EINA_SAFETY_ON_NULL_RETURN(cfdata);

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, dialog_data)
     {
        if (dialog_data->bg)
          {
             evas_object_del(dialog_data->bg);
             dialog_data->bg = NULL;
          }
     }
}

Eina_Bool
arrangement_widget_basic_apply_data(E_Config_Dialog *cfd __UNUSED__ , E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Evas_Coord_Point pos = {.x = 0, .y = 0};
   Eina_Bool success = EINA_TRUE;
   Eina_List *iter;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || !odd->crtc->current_mode)
          continue;
        e_layout_child_geometry_get(odd->rep, &pos.x, &pos.y, NULL, NULL);
        fprintf(stderr, "CONF_RANDR: Rearranging CRTC %d to %d,%d\n", odd->crtc->xid, pos.x, pos.y);
        /*
#define EQL(c) (pos.c == odd->crtc->geometry.c)
        if (!EQL(x) || !EQL(y))
          success |= ecore_x_randr_crtc_pos_set(cfd->con->manager->root, odd->crtc->xid, pos.x, pos.y);
#undef EQL
*/
        odd->previous_pos.x = odd->crtc->geometry.x;
        odd->previous_pos.y = odd->crtc->geometry.y;
     }
   /*
   if (success)
     ecore_x_randr_screen_reset(cfd->con->manager->root);
     */
   return success;
}

Eina_Bool
arrangement_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Evas_Coord_Point pos = {.x = 0, .y = 0};
   Eina_List *iter;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || !odd->crtc->current_mode)
          continue;
        e_layout_child_geometry_get(odd->rep, &pos.x, &pos.y, NULL, NULL);
        fprintf(stderr, "CONF_RANDR: Checking coord of CRTC %d. They are: %d,%d\n", odd->crtc->xid, pos.x, pos.y);
#define EQL(c) (pos.c == odd->crtc->geometry.c)
        if (!EQL(x) || !EQL(y))
          return EINA_TRUE;
#undef EQL
     }
   return EINA_FALSE;
}

void
arrangement_widget_keep_changes(E_Config_Dialog_Data *cfdata __UNUSED__)
{
   return;
}

void
arrangement_widget_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || ((odd->previous_pos.x == Ecore_X_Randr_Unset) || (odd->previous_pos.y == Ecore_X_Randr_Unset))) continue;
        if (ecore_x_randr_crtc_pos_set(cfdata->manager->root, odd->crtc->xid, odd->previous_pos.x, odd->previous_pos.y))
          {
#define EQL(c) (odd->previous_pos.c == odd->crtc->geometry.c)
             if (!EQL(x) || !EQL(y))
               ecore_x_randr_crtc_pos_set(cfdata->manager->root, odd->crtc->xid, odd->previous_pos.x, odd->previous_pos.y);
#undef EQL
             ecore_x_randr_screen_reset(cfdata->manager->root);
          }
     }
}

