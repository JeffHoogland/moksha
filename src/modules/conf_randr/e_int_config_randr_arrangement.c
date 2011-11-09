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

Eina_Bool                dialog_subdialog_arrangement_create_data(E_Config_Dialog_Data *e_config_runtime_info);
Evas_Object             *dialog_subdialog_arrangement_basic_create_widgets(Evas *canvas);
Eina_Bool                dialog_subdialog_arrangement_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool                dialog_subdialog_arrangement_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void                     dialog_subdialog_arrangement_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static inline Eina_List *_dialog_subdialog_arrangement_neighbors_get(Evas_Object *obj);
static void              _dialog_subdialog_arrangement_determine_positions_recursive(Evas_Object *obj);

//static inline E_Config_Randr_Dialog_Output_Dialog_Data *_dialog_subdialog_arrangement_output_dialog_data_new        (E_Randr_Crtc_Info *crtc_info, E_Randr_Output_Info *output_info);
static inline void       _dialog_subdialog_arrangement_suggestion_add(Evas *evas);
static inline void       _dialog_subdialog_arrangement_make_suggestion(Evas_Object *obj);
static void              _dialog_subdialog_arrangement_smart_class_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static Evas_Object      *_dialog_subdialog_arrangement_output_add(Evas *canvas, E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data);
static void              _dialog_subdialog_arrangement_output_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void              _dialog_subdialog_arrangement_output_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void              _dialog_subdialog_arrangement_output_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

// Function for the resolutions subdialog interaction
extern void              dialog_subdialog_resolutions_update_list(Evas_Object *crtc);
// Function for the orientation subdialog interaction
extern void              dialog_subdialog_orientation_update_radio_buttons(Evas_Object *crtc);
extern void              dialog_subdialog_orientation_update_edje(Evas_Object *crtc);
// Functions for the orientation subdialog interaction
extern void              dialog_subdialog_policies_update_radio_buttons(Evas_Object *crtc);

static Evas_Smart_Class screen_setup_smart_class = EVAS_SMART_CLASS_INIT_NAME_VERSION("EvasObjectSmartScreenSetup");
static Evas_Smart *screen_setup_smart = NULL;

extern E_Config_Dialog_Data *e_config_runtime_info;
extern char _theme_file_path[];

static void
_dialog_subdialog_arrangement_output_dialog_data_fill(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
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
        //try to get a mode from the preferred list, else use default list
        if (!(odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(eina_list_last(odd->output->preferred_modes))))
          odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(eina_list_last(odd->output->modes));
        odd->previous_pos.x = Ecore_X_Randr_Unset;
        odd->previous_pos.y = Ecore_X_Randr_Unset;
     }

   odd->new_pos.x = Ecore_X_Randr_Unset;
   odd->new_pos.y = Ecore_X_Randr_Unset;
}

Eina_Bool
dialog_subdialog_arrangement_create_data(E_Config_Dialog_Data *data)
{
   Eina_List *iter;
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;

   EINA_LIST_FOREACH (data->output_dialog_data_list, iter, dialog_data)
     {
        _dialog_subdialog_arrangement_output_dialog_data_fill(dialog_data);
     }

   return EINA_TRUE;
}

//IMPROVABLE: Clean up properly if instances can't be created
Evas_Object *
dialog_subdialog_arrangement_basic_create_widgets(Evas *canvas)
{
   Evas_Object *subdialog, *crtc;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Eina_List *iter;

   if (!canvas || !e_config_runtime_info || !e_config_runtime_info->output_dialog_data_list) return NULL;

   //initialize smart object
   evas_object_smart_clipped_smart_set(&screen_setup_smart_class);
   screen_setup_smart_class.resize = _dialog_subdialog_arrangement_smart_class_resize;
   screen_setup_smart = evas_smart_class_new(&screen_setup_smart_class);

   subdialog = evas_object_smart_add(canvas, screen_setup_smart);
   e_config_runtime_info->gui.subdialogs.arrangement.clipper = evas_object_smart_clipped_clipper_get(subdialog);
   fprintf(stderr, "CONF_RANDR: Arrangement subdialog added (%p).\n", subdialog);

   //only use information we can restore.
   EINA_LIST_FOREACH (e_config_runtime_info->output_dialog_data_list, iter, output_dialog_data)
     {
        if ((!output_dialog_data->crtc && !output_dialog_data->output))
          continue;
        crtc = _dialog_subdialog_arrangement_output_add(canvas, output_dialog_data);

        if (!crtc) continue;
        evas_object_show(crtc);

        evas_object_event_callback_add (crtc, EVAS_CALLBACK_MOUSE_DOWN, _dialog_subdialog_arrangement_output_mouse_down_cb, NULL);
        evas_object_event_callback_add (crtc, EVAS_CALLBACK_MOUSE_MOVE, _dialog_subdialog_arrangement_output_mouse_move_cb, NULL);
        evas_object_event_callback_add (crtc, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_arrangement_output_mouse_up_cb, NULL);

        evas_object_smart_member_add(crtc, subdialog);
        fprintf(stderr, "CONF_RANDR: CRTC representation (%p) added to arrangement subdialog (%p).\n", crtc, subdialog);
     }

   e_config_runtime_info->gui.subdialogs.arrangement.smart_parent = subdialog;

   return subdialog;
}

static Evas_Object *
_dialog_subdialog_arrangement_output_add(Evas *canvas, E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data)
{
   E_Randr_Output_Info *output_info;
   Evas_Object *output;
   const char *output_name = NULL;

   if (!canvas || !output_dialog_data || !e_config_runtime_info) return NULL;

   output = edje_object_add(canvas);

   //set instance data for output
   evas_object_data_set(output, "output_info", output_dialog_data);

   //set theme for monitor representation
   EINA_SAFETY_ON_FALSE_GOTO(edje_object_file_set(output, _theme_file_path, "e/conf/randr/dialog/subdialog/arrangement/output"), _dialog_subdialog_arrangement_output_add_edje_set_fail);
   //indicate monitor state
   if (!output_dialog_data->crtc || (output_dialog_data->crtc && !output_dialog_data->previous_mode))
     edje_object_signal_emit(output, "disabled", "e");
   else
     edje_object_signal_emit(output, "enabled", "e");
   //for now use deskpreview widget as background of output, maybe change this to
   //live image from comp module
   output_dialog_data->bg = e_widget_deskpreview_add(canvas, 1, 1);
   edje_object_part_swallow(output, "e.swallow.content", output_dialog_data->bg);

   //Try to get the name of the monitor connected to the output's last output via edid
   //else use the output's name
   if (output_dialog_data->crtc)
     output_info = (E_Randr_Output_Info *)eina_list_data_get(eina_list_last(output_dialog_data->crtc->outputs));
   else
     output_info = output_dialog_data->output;
   if (output_info)
     {
        if (ecore_x_randr_edid_has_valid_header(output_info->edid, output_info->edid_length))
          output_name = ecore_x_randr_edid_display_name_get(output_info->edid, output_info->edid_length);
        else if (output_info->name)
          output_name = output_info->name;
     }
   if (output_name)
     edje_object_part_text_set(output, "output_txt", output_name);

   //set output orientation
   dialog_subdialog_orientation_update_edje(output);
   return output;

_dialog_subdialog_arrangement_output_add_edje_set_fail:
   evas_object_del(output);
   return NULL;
}

static void
_dialog_subdialog_arrangement_smart_class_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Evas_Object *output;
   Evas_Coord real_sum_w = 0, real_sum_h = 0;
   Eina_Rectangle parent_geo, new_geo;
   Evas_Coord_Point offset = {.x = 0, .y = 0};
   Evas_Coord offset_x_max = 0;
   float scaling_factor = 0.1;
   Eina_List *lst, *itr;
   const E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;

   evas_object_geometry_get(obj, &parent_geo.x, &parent_geo.y, &parent_geo.w, &parent_geo.h);
   fprintf(stderr, "CONF_RANDR: Arrangement dialog shall be resized to %d x %d\n", w, h);
   fprintf(stderr, "CONF_RANDR: Arrangement dialog Smart object geo: %d x %d, %d x %d\n", parent_geo.x, parent_geo.y, parent_geo.w, parent_geo.h);
   if ((w < 1) || (h < 1)) return;

   lst = evas_object_smart_members_get(obj);
   //Calc average aspect ratio from all available monitors
   EINA_LIST_FOREACH (lst, itr, output)
     {
        if (output == e_config_runtime_info->gui.subdialogs.arrangement.clipper) continue;
        output_dialog_data = evas_object_data_get(output, "output_info");
        if (!output_dialog_data) continue;
        if ((!output_dialog_data->previous_mode) && (!output_dialog_data->preferred_mode)) continue;
        if (output_dialog_data->previous_mode)
          {
             real_sum_w += output_dialog_data->previous_mode->width;
             real_sum_h += output_dialog_data->previous_mode->height;
          }
        else
          {
             real_sum_w += output_dialog_data->preferred_mode->width;
             real_sum_h += output_dialog_data->preferred_mode->height;
          }
     }

   scaling_factor = (((float)parent_geo.w / (float)real_sum_w) < ((float)parent_geo.h / (float)real_sum_h)) ? ((float)parent_geo.w / (float)real_sum_w) : ((float)parent_geo.h / (float)real_sum_h);
   scaling_factor *= e_scale;

   EINA_LIST_FOREACH(lst, itr, output)
     {
        //Skip elements that are either the clipped smart object or falsely added
        //to the list of outputs (which should not happen)
        if (output == e_config_runtime_info->gui.subdialogs.arrangement.clipper) continue;
        output_dialog_data = evas_object_data_get(output, "output_info");
        if (!output_dialog_data) continue;
        if (output_dialog_data->previous_mode)
          {
             new_geo.w = (int)((float)output_dialog_data->previous_mode->width * scaling_factor);
             new_geo.h = (int)((float)output_dialog_data->previous_mode->height * scaling_factor);
          }
        else if (output_dialog_data->preferred_mode)
          {
             new_geo.w = (int)((float)output_dialog_data->preferred_mode->width * scaling_factor);
             new_geo.h = (int)((float)output_dialog_data->preferred_mode->height * scaling_factor);
          }
        else
          {
             fprintf(stderr, "CONF_RANDR: Can't resize thumb, as neither mode nor preferred mode are avavailable for %x\n", (output_dialog_data->crtc ? output_dialog_data->crtc->xid : output_dialog_data->output->xid));
             continue;
          }
        if ((new_geo.w <= 0) || (new_geo.h <= 0))
          {
             //this is an effect, occuring during dialog closing.
             //If we don't return here, e_thumb will segfault!
             return;
          }
        if ((output_dialog_data->previous_pos.x == Ecore_X_Randr_Unset) || (output_dialog_data->previous_pos.y == Ecore_X_Randr_Unset))
          {
             //this is a non enabled monitor
             new_geo.x = parent_geo.x + parent_geo.w - new_geo.w - offset.x;
             new_geo.y = parent_geo.y + offset.y;
             offset.y = new_geo.y + new_geo.h;
             if (offset_x_max < new_geo.w)
               {
                  //adopt new max value for x
                  offset_x_max = new_geo.w;
               }
             if ((offset.y + new_geo.h) > (parent_geo.y + parent_geo.h))
               {
                  //reset offset.y and adjust offset.x
                  offset.y = 0;
                  offset.x += offset_x_max;
               }
          }
        else
          {
             new_geo.x = ((int)((float)output_dialog_data->previous_pos.x * scaling_factor)) + parent_geo.x;
             new_geo.y = ((int)((float)output_dialog_data->previous_pos.y * scaling_factor)) + parent_geo.y;
          }
        //resize edje element
        evas_object_resize(output, new_geo.w, new_geo.h);
        //also resize bg
        e_thumb_icon_size_set(output_dialog_data->bg, new_geo.w, new_geo.h); //need to clarify the usage of e_thumb. Usable without e_thumb_icon_file_set??!!
        evas_object_move(output, new_geo.x, new_geo.y);
        fprintf(stderr, "CONF_RANDR: output representation %p was resized to %d x %d\n", output, new_geo.w, new_geo.h);
        fprintf(stderr, "CONF_RANDR: output representation %p was moved to %d x %d\n", output, new_geo.x, new_geo.y);
     }
}

static void
_dialog_subdialog_arrangement_output_mouse_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *element = NULL;
   Eina_List *iter;
   Eina_Bool crtc_selected = EINA_FALSE;

   EINA_LIST_FOREACH (evas_object_smart_members_get(evas_object_smart_parent_get(obj)), iter, element)
     {
        if (e_config_runtime_info->gui.subdialogs.arrangement.clipper == obj) continue;
        if (element != obj)
          edje_object_signal_emit(element, "deselect", "e");
        else
          {
             edje_object_signal_emit(element, "select", "e");
             //update data for other dialogs
             e_config_runtime_info->gui.selected_eo = obj;

             //update resolutions list
             dialog_subdialog_resolutions_update_list(obj);

             //update orientation radio buttons
             dialog_subdialog_orientation_update_radio_buttons(obj);

             //update policy radio buttons
             dialog_subdialog_policies_update_radio_buttons(obj);

             crtc_selected = EINA_TRUE;
          }
     }
   if (!crtc_selected)
     {
        //update data for other dialogs
        e_config_runtime_info->gui.selected_eo = NULL;

        //update resolutions list
        dialog_subdialog_resolutions_update_list(NULL);

        //update orientation radio buttons
        dialog_subdialog_orientation_update_radio_buttons(NULL);

        //update policy radio buttons
        dialog_subdialog_policies_update_radio_buttons(NULL);
     }

   evas_object_geometry_get(obj, &e_config_runtime_info->gui.subdialogs.arrangement.previous_pos.x, &e_config_runtime_info->gui.subdialogs.arrangement.previous_pos.y, NULL, NULL);
}

static void
_dialog_subdialog_arrangement_output_mouse_move_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Eina_Rectangle geo, parent;
   Evas_Coord_Point delta, new;

   if (ev->buttons != 1) return;
   evas_object_geometry_get (obj, &geo.x, &geo.y, &geo.w, &geo.h);
   evas_object_geometry_get (evas_object_smart_parent_get(obj), &parent.x, &parent.y, &parent.w, &parent.h);
   delta.x = ev->cur.canvas.x - ev->prev.canvas.x;
   delta.y = ev->cur.canvas.y - ev->prev.canvas.y;

   new.x = geo.x + delta.x;
   new.y = geo.y + delta.y;
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
        evas_object_move(obj, new.x, new.y);
        _dialog_subdialog_arrangement_make_suggestion(obj);
     }
}

static void
_dialog_subdialog_arrangement_output_mouse_up_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Coord_Point coords;

   if (evas_object_visible_get(e_config_runtime_info->gui.subdialogs.arrangement.suggestion))
     {
        edje_object_signal_emit(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, "hide", "e");
        evas_object_geometry_get(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, &coords.x, &coords.y, NULL, NULL);
        evas_object_move(obj, coords.x, coords.y);
     }
   else
     {
        evas_object_move(obj, e_config_runtime_info->gui.subdialogs.arrangement.previous_pos.x, e_config_runtime_info->gui.subdialogs.arrangement.previous_pos.y);
     }
}

void
_dialog_subdialog_arrangement_suggestion_add(Evas *evas)
{
   const char *theme_data_item = NULL;

   e_config_runtime_info->gui.subdialogs.arrangement.suggestion = edje_object_add(evas);
   edje_object_file_set(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, _theme_file_path, "e/conf/randr/dialog/subdialog/arrangement/suggestion");
   if ((theme_data_item = edje_object_data_get(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, "distance_min")))
     e_config_runtime_info->gui.subdialogs.arrangement.suggestion_dist_min = MIN(MAX(atoi(theme_data_item), 0), 100);
   else
     e_config_runtime_info->gui.subdialogs.arrangement.suggestion_dist_min = 20;
}

void
_dialog_subdialog_arrangement_make_suggestion(Evas_Object *obj)
{
   Eina_List *li, *crtcs = evas_object_smart_members_get(evas_object_smart_parent_get(obj));
   Evas_Object *crtc = NULL;
   Eina_Rectangle p_geo, geo, crtc_geo, s_geo;
   int dxa = 10000, dya = 10000, tmp, min_dist;

   if (!obj) return;

   if (!e_config_runtime_info->gui.subdialogs.arrangement.suggestion)
     {
        _dialog_subdialog_arrangement_suggestion_add(evas_object_evas_get(obj));
        evas_object_show(e_config_runtime_info->gui.subdialogs.arrangement.suggestion);
     }

   min_dist = e_config_runtime_info->gui.subdialogs.arrangement.suggestion_dist_min;

   evas_object_geometry_get(evas_object_smart_parent_get(obj), &p_geo.x, &p_geo.y, &p_geo.w, &p_geo.h);
   evas_object_geometry_get(obj, &geo.x, &geo.y, &geo.w, &geo.h);

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
   //iterate crtc list
   EINA_LIST_FOREACH (crtcs, li, crtc)
     {
        if ((crtc == obj) || (crtc == e_config_runtime_info->gui.subdialogs.arrangement.clipper)) continue;
        evas_object_geometry_get(crtc, &crtc_geo.x, &crtc_geo.y, &crtc_geo.w, &crtc_geo.h);
        //X-Axis
        tmp = abs(s_geo.x - crtc_geo.x);
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = crtc_geo.x;
             dxa = abs(s_geo.x - crtc_geo.x);
          }

        tmp = abs(s_geo.x - (crtc_geo.x + crtc_geo.w));
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = (crtc_geo.x + crtc_geo.w);
             dxa = tmp;
          }

        tmp = abs((s_geo.x + s_geo.w) - (crtc_geo.x - 1));
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = (crtc_geo.x - s_geo.w);
             dxa = tmp;
          }

        tmp = abs((s_geo.x + s_geo.w) - (crtc_geo.x + crtc_geo.w));
        if ((tmp < dxa) && (tmp < min_dist))
          {
             s_geo.x = (crtc_geo.x + crtc_geo.w - s_geo.w);
             dxa = tmp;
          }

        //Y-Axis
        tmp = abs(s_geo.y - crtc_geo.y);
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = crtc_geo.y;
             dya = abs(s_geo.y - crtc_geo.y);
          }

        tmp = abs(s_geo.y - (crtc_geo.y + crtc_geo.h));
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = (crtc_geo.y + crtc_geo.h);
             dya = tmp;
          }

        tmp = abs((s_geo.y + s_geo.h) - (crtc_geo.y - 1));
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = (crtc_geo.y - s_geo.h);
             dya = tmp;
          }

        tmp = abs((s_geo.y + s_geo.h) - (crtc_geo.y + crtc_geo.h));
        if ((tmp < dya) && (tmp < min_dist))
          {
             s_geo.y = (crtc_geo.y + crtc_geo.h - s_geo.h);
             dya = tmp;
          }
     }

   if ((s_geo.x != geo.x) && (s_geo.y != geo.y))
     {
        if (s_geo.x < p_geo.x) s_geo.x = p_geo.x;
        if ((s_geo.x + s_geo.w) > (p_geo.x + p_geo.w)) s_geo.x = ((p_geo.x + p_geo.w) - s_geo.w);
        if (s_geo.y < p_geo.y) s_geo.y = p_geo.y;
        if ((s_geo.y + s_geo.h) > (p_geo.y + p_geo.h)) s_geo.y = ((p_geo.y + p_geo.h) - s_geo.h);

        if (!evas_object_visible_get(e_config_runtime_info->gui.subdialogs.arrangement.suggestion))
          {
             evas_object_show(e_config_runtime_info->gui.subdialogs.arrangement.suggestion);
             edje_object_signal_emit(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, "show", "e");
          }

        evas_object_resize(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, s_geo.w, s_geo.h);
        evas_object_move(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, s_geo.x, s_geo.y);
     }
   else
     {
        edje_object_signal_emit(e_config_runtime_info->gui.subdialogs.arrangement.suggestion, "hide", "e");
        evas_object_hide(e_config_runtime_info->gui.subdialogs.arrangement.suggestion);
     }
}

void
dialog_subdialog_arrangement_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;

   EINA_SAFETY_ON_NULL_RETURN(cfdata);

   EINA_LIST_FREE (cfdata->output_dialog_data_list, dialog_data)
     {
        if (!dialog_data) continue;
        if (dialog_data->bg)
          {
             evas_object_del(dialog_data->bg);
             dialog_data->bg = NULL;
          }
        free(dialog_data);
     }
}

static Eina_List *
_dialog_subdialog_arrangement_neighbors_get(Evas_Object *obj)
{
   Evas_Object *smart_parent, *crtc;
   Eina_List *crtcs, *iter, *neighbors = NULL;
   Eina_Rectangle geo, neighbor_geo;
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data, *neighbor_info;

   smart_parent = evas_object_smart_parent_get(obj);
   crtcs = evas_object_smart_members_get(smart_parent);

   EINA_SAFETY_ON_FALSE_RETURN_VAL((dialog_data = evas_object_data_get(obj, "output_info")), NULL);
   evas_object_geometry_get(obj, &geo.x, &geo.y, &geo.w, &geo.h);
   EINA_LIST_FOREACH (crtcs, iter, crtc)
     {
        if ((crtc == obj)
            || (crtc == e_config_runtime_info->gui.subdialogs.arrangement.clipper)) continue;
        evas_object_geometry_get(crtc, &neighbor_geo.x, &neighbor_geo.y, &neighbor_geo.w, &neighbor_geo.h);
        if (!(neighbor_info = evas_object_data_get(crtc, "output_info"))) continue;

        if (((geo.x + geo.w) == neighbor_geo.x)
            || (geo.x == (neighbor_geo.x + neighbor_geo.w))
            || (geo.x == neighbor_geo.x)
            || ((geo.x + geo.w) == (neighbor_geo.x + neighbor_geo.w))
            || ((geo.y + geo.h) == neighbor_geo.y)
            || (geo.y == (neighbor_geo.y + neighbor_geo.h))
            || (geo.y == neighbor_geo.y)
            || ((geo.y + geo.h) == (neighbor_geo.y + neighbor_geo.h)))
          {
             neighbors = eina_list_append(neighbors, crtc);
          }
     }

   return neighbors;
}

static void
_dialog_subdialog_arrangement_determine_positions_recursive(Evas_Object *obj)
{
   Eina_List *neighbors, *iter;
   Evas_Object *smart_parent, *crtc;
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data, *neighbor_info;
   Eina_Rectangle geo, neighbor_geo, smart_geo;

   // Each object is seen as a tree. All its edges are compared to their
   // neighbors and wandered recusively.
   EINA_SAFETY_ON_NULL_RETURN(obj);

   smart_parent = e_config_runtime_info->gui.subdialogs.arrangement.smart_parent;
   evas_object_geometry_get(smart_parent, &smart_geo.x, &smart_geo.y, &smart_geo.w, &smart_geo.h);
   //fprintf(stderr, "CONF_RANDR: Smart Parent is at %dx%d\n", smart_geo.x, smart_geo.y);
   neighbors = _dialog_subdialog_arrangement_neighbors_get(obj);

   EINA_SAFETY_ON_FALSE_RETURN((dialog_data = evas_object_data_get(obj, "output_info")));
   evas_object_geometry_get(obj, &geo.x, &geo.y, &geo.w, &geo.h);

   //fprintf(stderr, "CONF_RANDR: Traversed element (%p) is at %dx%d\n", obj, geo.x, geo.y);
   if (geo.x == e_config_runtime_info->gui.subdialogs.arrangement.relative_zero.x) dialog_data->new_pos.x = 0;
   if (geo.y == e_config_runtime_info->gui.subdialogs.arrangement.relative_zero.y) dialog_data->new_pos.y = 0;

   if ((dialog_data->new_pos.x != 0) || (dialog_data->new_pos.y != 0))
     {
        // Find neighbor object we can calculate our own coordinates from
        EINA_LIST_FOREACH (neighbors, iter, crtc)
          {
             evas_object_geometry_get(crtc, &neighbor_geo.x, &neighbor_geo.y, &neighbor_geo.w, &neighbor_geo.h);
             if (!(neighbor_info = evas_object_data_get(crtc, "output_info"))) continue;

             evas_object_geometry_get(crtc, &neighbor_geo.x, &neighbor_geo.y, &neighbor_geo.w, &neighbor_geo.h);

             if ((dialog_data->new_pos.x == Ecore_X_Randr_Unset) && (neighbor_info->new_pos.x != Ecore_X_Randr_Unset))
               {
                  if ((geo.x + geo.w) == neighbor_geo.x)
                    {
                       dialog_data->new_pos.x = neighbor_info->new_pos.x - dialog_data->previous_mode->width;
                    }

                  if (geo.x == (neighbor_geo.x + neighbor_geo.w))
                    {
                       dialog_data->new_pos.x = neighbor_info->new_pos.x + neighbor_info->previous_mode->width;
                    }

                  if (geo.x == neighbor_geo.x)
                    {
                       dialog_data->new_pos.x = neighbor_info->new_pos.x;
                    }

                  if ((geo.x + geo.w) == (neighbor_geo.x + neighbor_geo.w))
                    {
                       dialog_data->new_pos.x = (neighbor_info->new_pos.x + neighbor_info->previous_mode->width) - dialog_data->previous_mode->width;
                    }
               }

             if ((dialog_data->new_pos.y == Ecore_X_Randr_Unset) && (neighbor_info->new_pos.y != Ecore_X_Randr_Unset))
               {
                  if ((geo.y + geo.h) == neighbor_geo.y)
                    {
                       dialog_data->new_pos.y = neighbor_info->new_pos.y - dialog_data->previous_mode->height;
                    }

                  if (geo.y == (neighbor_geo.y + neighbor_geo.h))
                    {
                       dialog_data->new_pos.y = neighbor_info->new_pos.y + neighbor_info->previous_mode->height;
                    }

                  if (geo.y == neighbor_geo.y)
                    {
                       dialog_data->new_pos.y = neighbor_info->new_pos.y;
                    }

                  if ((geo.y + geo.h) == (neighbor_geo.y + neighbor_geo.h))
                    {
                       dialog_data->new_pos.y = (neighbor_info->new_pos.y + neighbor_info->previous_mode->height) - dialog_data->previous_mode->height;
                    }
               }
             if ((dialog_data->new_pos.x != Ecore_X_Randr_Unset)
                 && (dialog_data->new_pos.y != Ecore_X_Randr_Unset))
               {
                  //fprintf(stderr, "CONF_RANDR: Determined position for %p: %dx%d\n", obj, dialog_data->new_pos.x, dialog_data->new_pos.y);
                  break;
               }
          }
     }
   if ((dialog_data->new_pos.x != Ecore_X_Randr_Unset) || (dialog_data->new_pos.y != Ecore_X_Randr_Unset))
     {
        //Only wander all neighbors recursively, if they can use the current
        //element as reference for their position
        EINA_LIST_FOREACH (neighbors, iter, crtc)
          {
             neighbor_info = evas_object_data_get(crtc, "output_info");
             if ((neighbor_info->new_pos.x == Ecore_X_Randr_Unset)
                 || (neighbor_info->new_pos.y == Ecore_X_Randr_Unset))
               {
                  //fprintf(stderr, "CONF_RANDR: Now going to travel %p.\n", crtc);
                  _dialog_subdialog_arrangement_determine_positions_recursive(crtc);
               }
          }
     }
   eina_list_free(neighbors);
}

Eina_Bool
dialog_subdialog_arrangement_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   Eina_List *crtcs, *iter;
   Evas_Object *smart_parent, *crtc, *top_left = NULL;
   Eina_Rectangle geo, smart_geo;
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Evas_Coord_Point relz = { .x = 10000, .y = 10000};
   Eina_Bool arrangement_failed = EINA_FALSE;

   smart_parent = e_config_runtime_info->gui.subdialogs.arrangement.smart_parent;
   evas_object_geometry_get(smart_parent, &smart_geo.x, &smart_geo.y, &smart_geo.w, &smart_geo.h);
   crtcs = evas_object_smart_members_get(smart_parent);

   //Create virtual borders around the displayed representations by finding
   //relative x and y as virtual 0x0
   EINA_LIST_FOREACH (crtcs, iter, crtc)
     {
        if (crtc == e_config_runtime_info->gui.subdialogs.arrangement.clipper) continue;
        //Already reset values for upcoming calculation
        if (!(odd = evas_object_data_get(crtc, "output_info"))) continue;
        odd->new_pos.x = Ecore_X_Randr_Unset;
        odd->new_pos.y = Ecore_X_Randr_Unset;
        odd = NULL;

        //See whether this element is closer to 0x0 than any before
        evas_object_geometry_get(crtc, &geo.x, &geo.y, &geo.w, &geo.h);
        if (geo.x < relz.x)
          {
             relz.x = geo.x;
             top_left = crtc;
          }
        if (geo.y < relz.y)
          {
             relz.y = geo.y;
             top_left = crtc;
          }
     }
   e_config_runtime_info->gui.subdialogs.arrangement.relative_zero.x = relz.x;
   e_config_runtime_info->gui.subdialogs.arrangement.relative_zero.y = relz.y;
   if (top_left) _dialog_subdialog_arrangement_determine_positions_recursive(top_left);

   EINA_LIST_FOREACH (crtcs, iter, crtc)
     {
        if ((crtc == e_config_runtime_info->gui.subdialogs.arrangement.clipper) || !(odd = evas_object_data_get(crtc, "output_info")) || !odd->crtc
            || ((odd->new_pos.x == Ecore_X_Randr_Unset) || (odd->new_pos.y == Ecore_X_Randr_Unset))) continue;
        if ((odd->previous_pos.x != odd->new_pos.x) || (odd->previous_pos.y != odd->new_pos.y))
          {
             fprintf(stderr, "CONF_RANDR: CRTC %x is moved to %dx%d\n", odd->crtc->xid, odd->new_pos.x, odd->new_pos.y);
             if (!ecore_x_randr_crtc_pos_set(cfd->con->manager->root, odd->crtc->xid, odd->new_pos.x, odd->new_pos.y))
               {
                  arrangement_failed = EINA_TRUE;
                  break;
               }
          }
     }
   if (arrangement_failed)
     return EINA_FALSE;
   else
     {
        ecore_x_randr_screen_reset(cfd->con->manager->root);
        return EINA_TRUE;
     }
}

Eina_Bool
dialog_subdialog_arrangement_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *iter;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, output_dialog_data)
     {
        if ((output_dialog_data->previous_pos.x != output_dialog_data->new_pos.x)
            || (output_dialog_data->previous_pos.y != output_dialog_data->new_pos.y)
            ) return EINA_TRUE;
     }
   return EINA_FALSE;
}

void
dialog_subdialog_arrangement_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || ((odd->new_pos.x == Ecore_X_Randr_Unset) || (odd->new_pos.y == Ecore_X_Randr_Unset))) continue;
        odd->previous_pos.x = odd->new_pos.x;
        odd->previous_pos.y = odd->new_pos.y;
        odd->new_pos.x = Ecore_X_Randr_Unset;
        odd->new_pos.y = Ecore_X_Randr_Unset;
     }
}

void
dialog_subdialog_arrangement_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || ((odd->previous_pos.x == Ecore_X_Randr_Unset) || (odd->previous_pos.y == Ecore_X_Randr_Unset))) continue;
        if (ecore_x_randr_crtc_pos_set(cfdata->manager->root, odd->crtc->xid, odd->previous_pos.x, odd->previous_pos.y))
          {
             odd->new_pos.x = odd->previous_pos.x;
             odd->new_pos.y = odd->previous_pos.y;
             odd->previous_pos.x = Ecore_X_Randr_Unset;
             odd->previous_pos.y = Ecore_X_Randr_Unset;
             ecore_x_randr_screen_reset(cfdata->manager->root);
          }
     }
}

