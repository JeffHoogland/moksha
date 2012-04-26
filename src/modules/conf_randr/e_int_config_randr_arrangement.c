#include "e_int_config_randr.h"
#include "e_randr.h"
#include "Ecore_X.h"

#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)
#endif

#ifdef  Ecore_X_Randr_Unset
#undef  Ecore_X_Randr_Unset
#endif
#define Ecore_X_Randr_Unset -1

#define DOUBLECLICK_TIMEOUT 0.2
#define CRTC_THUMB_SIZE_W   300
#define CRTC_THUMB_SIZE_H   300

/*
 *
 *                               TOP_TOP
 *           ************************************************
 *           *                  TOP_BOTTOM                  *
 *           *                                              *
 *           *                                              *
 *           *                                              *
 * LEFT_LEFT * LEFT_RIGHT                        RIGHT_LEFT * RIGHT_RIGHT
 *           *                                              *
 *           *                                              *
 *           *                                              *
 *           *                  BOTTOM_TOP                  *
 *           ************************************************
 *                             BOTTOM_BOTTOM
 *
 */
typedef enum {
     EVAS_OBJECT_REL_POS_NONE = 0,
     EVAS_OBJECT_REL_POS_TOP_TOP = (1 << 0),
     EVAS_OBJECT_REL_POS_TOP_BOTTOM = (1 << 1),
     EVAS_OBJECT_REL_POS_RIGHT_LEFT = (1 << 2),
     EVAS_OBJECT_REL_POS_RIGHT_RIGHT = (1 << 3),
     EVAS_OBJECT_REL_POS_BOTTOM_TOP = (1 << 4),
     EVAS_OBJECT_REL_POS_BOTTOM_BOTTOM = (1 << 5),
     EVAS_OBJECT_REL_POS_LEFT_LEFT = (1 << 6),
     EVAS_OBJECT_REL_POS_LEFT_RIGHT = (1 << 7),
     EVAS_OBJECT_REL_POS_X_ZERO = (1 << 8),
     EVAS_OBJECT_REL_POS_Y_ZERO = (1 << 9),
     EVAS_OBJECT_REL_POS_INSIDE = (
           EVAS_OBJECT_REL_POS_TOP_BOTTOM |
           EVAS_OBJECT_REL_POS_RIGHT_LEFT |
           EVAS_OBJECT_REL_POS_BOTTOM_TOP |
           EVAS_OBJECT_REL_POS_LEFT_RIGHT),
     EVAS_OBJECT_REL_POS_OUTSIDE = (
           EVAS_OBJECT_REL_POS_TOP_TOP |
           EVAS_OBJECT_REL_POS_RIGHT_RIGHT |
           EVAS_OBJECT_REL_POS_BOTTOM_BOTTOM |
           EVAS_OBJECT_REL_POS_LEFT_LEFT),
     EVAS_OBJECT_REL_POS_ALL = (
           EVAS_OBJECT_REL_POS_INSIDE |
           EVAS_OBJECT_REL_POS_OUTSIDE)
} Evas_Object_Rel_Pos;

typedef enum {
     EVAS_OBJECT_DIRECTION_TOP = (1 << 0),
     EVAS_OBJECT_DIRECTION_RIGHT = (1 << 1),
     EVAS_OBJECT_DIRECTION_BOTTOM = (1 << 2),
     EVAS_OBJECT_DIRECTION_LEFT = (1 << 3)
} Evas_Object_Direction;

typedef struct {
     struct {
          Evas_Object *x, *y;
          struct {
               Evas_Object_Rel_Pos x, y;
          } pos_rel;
     } closest_objects;
     Evas_Coord_Point pos;
     int distance;
} Position_Suggestion;

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
static Position_Suggestion *e_layout_pos_sug_from_children_get(Evas_Object *child, Eina_List *children, int max_distance, Evas_Object_Rel_Pos allowed_pos);
Eina_Bool                _arrangemnet_rep_illegal_overlapping(E_Config_Randr_Dialog_Output_Dialog_Data *odd);

extern E_Config_Dialog_Data *e_config_runtime_info;
extern Config *randr_dialog_config;
extern char _theme_file_path[];

static void
_arrangement_widget_rep_dialog_data_fill(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   if (!odd) return;

   odd->new_pos.x = Ecore_X_Randr_Unset;
   odd->new_pos.y = Ecore_X_Randr_Unset;

   if (odd->crtc)
     {
        //already enabled screen
        odd->previous_pos.x = odd->crtc->geometry.x;
        odd->previous_pos.y = odd->crtc->geometry.y;
     }
   else
     {
        odd->previous_pos.x = Ecore_X_Randr_Unset;
        odd->previous_pos.y = Ecore_X_Randr_Unset;
     }
}

void
arrangement_widget_rep_update(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   Eina_Rectangle geo = {.x = 0, .y = 0, .w = 0, .h = 0};
   Ecore_X_Randr_Orientation orientation = Ecore_X_Randr_Unset;

   //Get width, height
   if (odd->new_mode)
     {
        geo.w = odd->new_mode->width;
        geo.h = odd->new_mode->height;
     }
   else if (odd->crtc)
     {
        geo.w = odd->crtc->geometry.w;
        geo.h = odd->crtc->geometry.h;
     }
   else if (odd->preferred_mode)
     {
        geo.w = odd->preferred_mode->width;
        geo.h = odd->preferred_mode->height;
     }
   else
     {
        geo.w = e_config_runtime_info->gui.widgets.arrangement.dummy_geo.w;
        geo.h = e_config_runtime_info->gui.widgets.arrangement.dummy_geo.h;
     }

   //Get x, y
   if ((odd->new_pos.x != Ecore_X_Randr_Unset) && (odd->new_pos.y != Ecore_X_Randr_Unset))
     {
        geo.x = odd->new_pos.x;
        geo.y = odd->new_pos.y;
     }
   else if (odd->crtc && odd->crtc->current_mode)
     {
        geo.x = odd->crtc->geometry.x;
        geo.y = odd->crtc->geometry.y;
     }
   else
     {
        geo.x = e_config_runtime_info->gui.widgets.arrangement.dummy_geo.x;
        geo.y = e_config_runtime_info->gui.widgets.arrangement.dummy_geo.y;
     }

   if (odd->crtc)
     {
        orientation = (odd->new_orientation != Ecore_X_Randr_Unset) ? odd->new_orientation : odd->previous_orientation;
     }
   switch (orientation)
     {
      case ECORE_X_RANDR_ORIENTATION_ROT_90:
      case ECORE_X_RANDR_ORIENTATION_ROT_270:
         e_layout_child_resize(odd->rep, geo.h, geo.w);
         break;
      default:
         e_layout_child_resize(odd->rep, geo.w, geo.h);
         break;
     }
   e_layout_child_move(odd->rep, geo.x, geo.y);
   e_layout_child_raise(odd->rep);
   fprintf(stderr, "CONF_RANDR: Representation (%p) updated with geo %d.%d %dx%d.\n", odd->rep, geo.x, geo.y, geo.w, geo.h);
}

void
_arrangement_widget_update(void)
{
   static Evas_Object *area = NULL;
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Eina_List *iter;

   area = e_config_runtime_info->gui.widgets.arrangement.area;

   if (!e_config_runtime_info || !e_config_runtime_info->gui.canvas || !e_config_runtime_info->output_dialog_data_list || !area) return;

   fprintf(stderr, "CONF_RANDR: Display disconnected outputs: %s\n", (randr_dialog_config->display_disconnected_outputs ? "YES" : "NO"));
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
        e_layout_pack(area, odd->rep);
        arrangement_widget_rep_update(odd);

        evas_object_data_set(odd->rep, "rep_info", odd);
        evas_object_show(odd->rep);
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
   char *theme_data_item = NULL;
   Eina_Rectangle dummy_geo;
   int max_dist = 0;

   EINA_LIST_FOREACH(data->output_dialog_data_list, iter, dialog_data)
      _arrangement_widget_rep_dialog_data_fill(dialog_data);

   //Read in maximum distance of objects before a position is suggested
   if ((theme_data_item = edje_file_data_get(_theme_file_path, "distance_max")))
     max_dist = atoi(theme_data_item);
   else
     max_dist = 100;

   e_config_runtime_info->gui.widgets.arrangement.suggestion_dist_max = max_dist;


   dummy_geo.x = e_randr_screen_info.rrvd_info.randr_info_12->max_size.width - dummy_geo.w;
   dummy_geo.y = 0;
   //Read in size used for disabled outputs
   theme_data_item = edje_file_data_get(_theme_file_path, "disabled_output_width");
   dummy_geo.w = theme_data_item ? atoi(theme_data_item) : 1024;
   theme_data_item = edje_file_data_get(_theme_file_path, "disabled_output_height");
   dummy_geo.h = theme_data_item ? atoi(theme_data_item) : 768;

   memcpy(&e_config_runtime_info->gui.widgets.arrangement.dummy_geo, &dummy_geo, sizeof(e_config_runtime_info->gui.widgets.arrangement.dummy_geo));

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

   evas_object_event_callback_add(rep, EVAS_CALLBACK_MOUSE_DOWN, _arrangement_widget_rep_mouse_down_cb, NULL);
   evas_object_event_callback_add(rep, EVAS_CALLBACK_MOUSE_MOVE, _arrangement_widget_rep_mouse_move_cb, NULL);
   evas_object_event_callback_add(rep, EVAS_CALLBACK_MOUSE_UP, _arrangement_widget_rep_mouse_up_cb, NULL);

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
   orientation_widget_update_radio_buttons(NULL);

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

   e_layout_child_geometry_get(obj, &e_config_runtime_info->gui.widgets.arrangement.sel_rep_previous_pos.x, &e_config_runtime_info->gui.widgets.arrangement.sel_rep_previous_pos.y, NULL, NULL);
   //update data for other logs
   e_config_runtime_info->gui.selected_output_dd = (E_Config_Randr_Dialog_Output_Dialog_Data*)evas_object_data_get(obj, "rep_info");

   //update resolutions list
   resolution_widget_update_list(e_config_runtime_info->gui.selected_output_dd);

   //update orientation radio buttons
   orientation_widget_update_radio_buttons(e_config_runtime_info->gui.selected_output_dd);

   //update policy radio buttons
   policy_widget_update_radio_buttons(e_config_runtime_info->gui.selected_output_dd);
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
     }
   else
     {
        coords.x = e_config_runtime_info->gui.widgets.arrangement.sel_rep_previous_pos.x;
        coords.y = e_config_runtime_info->gui.widgets.arrangement.sel_rep_previous_pos.y;
     }
   e_layout_child_move(obj, coords.x, coords.y);
}

Evas_Object *
_arrangement_widget_suggestion_add(Evas *evas)
{
   Evas_Object *sug = NULL;

   sug = edje_object_add(evas);
   edje_object_file_set(sug, _theme_file_path, "e/conf/randr/dialog/widget/arrangement/suggestion");

   return sug;
}

void
_arrangement_widget_make_suggestion(Evas_Object *obj)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Eina_List *possible_reps = NULL, *li;
   Position_Suggestion *ps;
   int max_dist;
   Eina_Bool visible;
   Eina_Rectangle o_geo;
   const Evas_Object_Rel_Pos allowed_pos = (EVAS_OBJECT_REL_POS_ALL | EVAS_OBJECT_REL_POS_X_ZERO | EVAS_OBJECT_REL_POS_Y_ZERO);

   if (!obj) return;

   max_dist = e_config_runtime_info->gui.widgets.arrangement.suggestion_dist_max;

   //iterate rep list
   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, li, odd)
     {
        if (!odd || !odd->rep || (odd->rep == obj) || !(odd->crtc) || !(odd->crtc->current_mode))
          continue;
        possible_reps = eina_list_append(possible_reps, odd->rep);
     }

   ps = e_layout_pos_sug_from_children_get(obj, possible_reps, max_dist, allowed_pos);
   visible = evas_object_visible_get(e_config_runtime_info->gui.widgets.arrangement.suggestion);
   if (!ps)
     {
        if (visible)
          {
             edje_object_signal_emit(e_config_runtime_info->gui.widgets.arrangement.suggestion, "hide", "e");
             evas_object_hide(e_config_runtime_info->gui.widgets.arrangement.suggestion);
          }
        goto _mk_sug_free_ret;
     }

   /*
   fprintf(stderr, "CONF_RANDR: Suggestion:\n"
           "\tpos.x: %d\n"
           "\tpos.y: %d\n"
           "\tclsst.x: %p\n"
           "\tclsst.y: %p\n",
           ps->pos.x, ps->pos.y, ps->closest_objects.x, ps->closest_objects.y);
   */
   odd = evas_object_data_get(obj, "rep_info");

   e_layout_child_geometry_get(obj, &o_geo.x, &o_geo.y, &o_geo.w, &o_geo.h);

   if (ps->pos.x != Ecore_X_Randr_Unset)
     {
        odd->new_pos.x = ps->pos.x;
        if ((odd->new_pos.x + o_geo.w) > e_randr_screen_info.rrvd_info.randr_info_12->max_size.width) //FIXME:Rotation
          odd->new_pos.x = Ecore_X_Randr_Unset;
        else if (odd->new_pos.x < 0)
          odd->new_pos.x = 0;
     }

   if (ps->pos.y != Ecore_X_Randr_Unset)
     {
        odd->new_pos.y = ps->pos.y;
        if ((odd->new_pos.y + o_geo.h) > e_randr_screen_info.rrvd_info.randr_info_12->max_size.height) //FIXME: Rotation
          odd->new_pos.y = Ecore_X_Randr_Unset;
        else if (odd->new_pos.y < 0)
          odd->new_pos.y = 0;
     }

   if (_arrangemnet_rep_illegal_overlapping(odd) || ps->distance > max_dist)
     odd->new_pos.x = odd->new_pos.y = Ecore_X_Randr_Unset;

   if ((odd->new_pos.x != Ecore_X_Randr_Unset) && (odd->new_pos.y != Ecore_X_Randr_Unset))
     {
        if (!visible)
          {
             evas_object_show(e_config_runtime_info->gui.widgets.arrangement.suggestion);
             edje_object_signal_emit(e_config_runtime_info->gui.widgets.arrangement.suggestion, "show", "e");
          }

        e_layout_child_move(e_config_runtime_info->gui.widgets.arrangement.suggestion, odd->new_pos.x, odd->new_pos.y);
        e_layout_child_resize(e_config_runtime_info->gui.widgets.arrangement.suggestion, o_geo.w, o_geo.h);
        e_layout_child_raise(e_config_runtime_info->gui.widgets.arrangement.suggestion);
     }
   else if (visible)
     {
        edje_object_signal_emit(e_config_runtime_info->gui.widgets.arrangement.suggestion, "hide", "e");
        evas_object_hide(e_config_runtime_info->gui.widgets.arrangement.suggestion);
     }
_mk_sug_free_ret:
   free(ps);
}

void
arrangement_widget_free_cfdata(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
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
   Eina_Bool success = EINA_TRUE;
   Eina_List *iter;

   //create list of all evas objects we concern
   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || !odd->crtc->current_mode || (odd->new_pos.x == Ecore_X_Randr_Unset) || (odd->new_pos.y == Ecore_X_Randr_Unset))
          continue;
        fprintf(stderr, "CONF_RANDR: Rearranging CRTC %d to %d,%d\n", odd->crtc->xid, odd->new_pos.x, odd->new_pos.y);
#define EQL(c) (odd->new_pos.c == odd->crtc->geometry.c)
        if (!EQL(x) || !EQL(y))
          {
             if (!ecore_x_randr_crtc_pos_set(cfd->con->manager->root, odd->crtc->xid, odd->new_pos.x, odd->new_pos.y))
               success = EINA_FALSE;
          }
#undef EQL
     }
   ecore_x_randr_screen_reset(cfd->con->manager->root);
   return success;
}

Eina_Bool
arrangement_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Eina_List *iter;

   /*
    * This assumes that only positions for elements are allowed, that have been
    * previously suggested. Thus every element has its position already assigned.
    */
   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        fprintf(stderr, "CONF_RANDR: Checking coord of odd %p. new_pos is: %d,%d\n", odd, odd->new_pos.x, odd->new_pos.y);
        if (!odd->crtc || !odd->crtc->current_mode || (odd->new_pos.x == Ecore_X_Randr_Unset) || (odd->new_pos.y == Ecore_X_Randr_Unset))
          continue;
#define EQL(c) (odd->new_pos.c == odd->crtc->geometry.c)
        if (!EQL(x) || !EQL(y))
          return EINA_TRUE;
#undef EQL
     }
   return EINA_FALSE;
}

void
arrangement_widget_keep_changes(E_Config_Dialog_Data *cfdata __UNUSED__)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || !odd->crtc->current_mode || (odd->new_pos.x == Ecore_X_Randr_Unset) || (odd->new_pos.y == Ecore_X_Randr_Unset))
          continue;
        //FIXME Rely on RandRR events to update data!
        odd->previous_pos.x = odd->new_pos.x;
        odd->previous_pos.y = odd->new_pos.y;
        odd->new_pos.x = odd->new_pos.y = Ecore_X_Randr_Unset;
     }
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
#define EQL(c) (odd->previous_pos.c == odd->crtc->geometry.c)
        if (!EQL(x) || !EQL(y))
          ecore_x_randr_crtc_pos_set(cfdata->manager->root, odd->crtc->xid, odd->previous_pos.x, odd->previous_pos.y);
#undef EQL
     }
   ecore_x_randr_screen_reset(cfdata->manager->root);
}

/**
 * @brief suggests a position for an e_layout child @p child based on positions of objects in @p list.
 * @param child The object for which a suggestion should be made
 * @param children A list of e_layout children on whose positions the suggestion will be
 * base
 * @param max_distance The maximum distance a border is allowed to be away form
 * @p obj border
 * @param allowed_pos A mask defining which positions are allowed to be
 * suggested.
 * @return a suggested position together with the closest children
 */
static Position_Suggestion
*e_layout_pos_sug_from_children_get(Evas_Object *child, Eina_List *children, int max_distance, Evas_Object_Rel_Pos allowed_pos)
{
   Position_Suggestion *ps = NULL;
   Evas_Object *io = NULL;
   Eina_Rectangle og, ig;
   Evas_Coord_Point cd, d;
   Eina_List *iter;

   if (!child || !children || !allowed_pos)
     return NULL;

   //General initialization
   e_layout_child_geometry_get(child, &og.x, &og.y, &og.w, &og.h);
   cd.x = d.x = INT_MAX;
   cd.y = d.y = INT_MAX;

   //initialize returned object
   ps = E_NEW(Position_Suggestion, 1);
   ps->closest_objects.pos_rel.x = ps->closest_objects.pos_rel.y = EVAS_OBJECT_REL_POS_NONE;
   ps->pos.x = ps->pos.y = Ecore_X_Randr_Unset;

   EINA_LIST_FOREACH(children, iter, io)
     {
        if (!io)
          continue;

        e_layout_child_geometry_get(io, &ig.x, &ig.y, &ig.w, &ig.h);

        // Top
        if (allowed_pos & EVAS_OBJECT_REL_POS_TOP_TOP)
          {
             cd.y = abs((og.y + og.h) - ig.y);
             if ((cd.y <= max_distance) && (cd.y < d.y))
               {
                  d.y = cd.y;
                  ps->pos.y = ig.y - og.h;
                  ps->closest_objects.y = io;
                  ps->closest_objects.pos_rel.y = EVAS_OBJECT_REL_POS_TOP_TOP;
               }
          }

        if (allowed_pos & EVAS_OBJECT_REL_POS_TOP_BOTTOM)
          {
             cd.y = abs(og.y - ig.y);
             if ((cd.y <= max_distance) && (cd.y < d.y))
               {
                  d.y = cd.y;
                  ps->pos.y = ig.y;
                  ps->closest_objects.y = io;
                  ps->closest_objects.pos_rel.y = EVAS_OBJECT_REL_POS_TOP_BOTTOM;
               }
          }


        // Right
        if (allowed_pos & EVAS_OBJECT_REL_POS_RIGHT_RIGHT)
          {
             cd.x = abs(og.x - (ig.x + ig.w));
             if ((cd.x <= max_distance) && (cd.x < d.x))
               {
                  d.x = cd.x;
                  ps->pos.x = ig.x + ig.w;
                  ps->closest_objects.x = io;
                  ps->closest_objects.pos_rel.x = EVAS_OBJECT_REL_POS_RIGHT_RIGHT;
               }
          }

        if (allowed_pos & EVAS_OBJECT_REL_POS_RIGHT_LEFT)
          {
             cd.x = abs((og.x + og.w) - (ig.x + ig.w));
             if ((cd.x <= max_distance) && (cd.x < d.x))
               {
                  d.x = cd.x;
                  ps->pos.x = (ig.x + ig.w) - og.w;
                  ps->closest_objects.x = io;
                  ps->closest_objects.pos_rel.x = EVAS_OBJECT_REL_POS_RIGHT_LEFT;
               }
          }


        // BOTTOM
        if (allowed_pos & EVAS_OBJECT_REL_POS_BOTTOM_BOTTOM)
          {
             cd.y = abs(og.y - (ig.y + ig.h));
             if ((cd.y <= max_distance) && (cd.y < d.y))
               {
                  d.y = cd.y;
                  ps->pos.y = ig.y + ig.h;
                  ps->closest_objects.y = io;
                  ps->closest_objects.pos_rel.y = EVAS_OBJECT_REL_POS_BOTTOM_BOTTOM;
               }
          }

        if (allowed_pos & EVAS_OBJECT_REL_POS_BOTTOM_TOP)
          {
             cd.y = abs((ig.y + ig.h) - (og.y + og.h));
             if ((cd.y <= max_distance) && (cd.y < d.y))
               {
                  d.y = cd.y;
                  ps->pos.y = (ig.y + ig.h) - og.h;
                  ps->closest_objects.y = io;
                  ps->closest_objects.pos_rel.y = EVAS_OBJECT_REL_POS_BOTTOM_TOP;
               }
          }


        // Left
        if (allowed_pos & EVAS_OBJECT_REL_POS_LEFT_LEFT)
          {
             cd.x = abs((og.x + og.w) - ig.x);
             if ((cd.x <= max_distance) && (cd.x < d.x))
               {
                  d.x = cd.x;
                  ps->pos.x = (ig.x - og.w);
                  ps->closest_objects.x = io;
                  ps->closest_objects.pos_rel.x = EVAS_OBJECT_REL_POS_LEFT_LEFT;
               }
          }

        if (allowed_pos & EVAS_OBJECT_REL_POS_LEFT_RIGHT)
          {
             cd.x = abs(og.x - ig.x);
             if ((cd.x <= max_distance) && (cd.x < d.x))
               {
                  d.x = cd.x;
                  ps->pos.x = ig.x;
                  ps->closest_objects.x = io;
                  ps->closest_objects.pos_rel.x = EVAS_OBJECT_REL_POS_LEFT_RIGHT;
               }
          }
     }

   //FIXME: these are copied from the loop above, thus dupclicated code!
   if (allowed_pos & EVAS_OBJECT_REL_POS_X_ZERO)
     {
        io = NULL;
        ig.x = 0;
        cd.x = abs(og.x - ig.x);
        if ((cd.x <= max_distance) && (cd.x < d.x))
          {
             d.x = cd.x;
             ps->pos.x = ig.x;
             ps->closest_objects.x = io;
             ps->closest_objects.pos_rel.x = EVAS_OBJECT_REL_POS_X_ZERO;
          }
      }
   if (allowed_pos & EVAS_OBJECT_REL_POS_Y_ZERO)
     {
        io = NULL;
        ig.y = 0;
        cd.y = abs(og.y - ig.y);
        if ((cd.y <= max_distance) && (cd.y < d.y))
          {
             d.y = cd.y;
             ps->pos.y = ig.y;
             ps->closest_objects.y = io;
             ps->closest_objects.pos_rel.y = EVAS_OBJECT_REL_POS_Y_ZERO;
          }
     }
   ps->distance = MIN(d.x, d.y);

   return ps;
}

/*
 * E17 does not allow CRTCs (or rather zones) to overlap.
 * This functions evaluates whether a new position does interfere with already
 * available zones.
 */
Eina_Bool
_arrangemnet_rep_illegal_overlapping(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *r_odd = NULL;
   Eina_List *it;
   Eina_Rectangle o_geo, r_geo;

   e_layout_child_geometry_get(odd->rep, &o_geo.x, &o_geo.y, &o_geo.w, &o_geo.h);
   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, it, r_odd)
     {
        if (!odd || (r_odd == odd))
          continue;
        e_layout_child_geometry_get(r_odd->rep, &r_geo.x, &r_geo.y, &r_geo.w, &r_geo.h);

        if (eina_rectangles_intersect(&o_geo, &r_geo)
           && memcmp(&o_geo, &r_geo, sizeof(Eina_Rectangle)))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}
