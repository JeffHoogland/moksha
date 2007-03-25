/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *o_edje;
   Evas_Object *o_rect;

   E_Color_Dialog *dia;
   E_Color *color;
   E_Container *con; /* container to pop a color dialog up on */
   int show_color_dialog;
};

static void _e_wid_update(E_Widget_Data *wd);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_color_select_cb(E_Color_Dialog *dia, E_Color *color, void *data);
static void _e_wid_color_cancel_cb(E_Color_Dialog *dia, E_Color *color, void *data);
static void _e_wid_color_change_cb(E_Color_Dialog *dia, E_Color *color, void *data);
static void _e_wid_disable_hook(Evas_Object *obj);

static void
_e_wid_update(E_Widget_Data *wd)
{
   if (!wd) return;

   evas_object_color_set(wd->o_rect, 
			 (wd->color->r * wd->color->a) / 255,
			 (wd->color->g * wd->color->a) / 255,
			 (wd->color->b * wd->color->a) / 255,
			 wd->color->a);
   e_widget_change(wd->obj);
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Evas_Object *wid;
   E_Widget_Data *wd;

   wid = data;
   wd = e_widget_data_get(wid);

   if (!wd->show_color_dialog || !wd->con) return;
   if (!wd->dia)
     {
	wd->dia = e_color_dialog_new(wd->con, wd->color);
	e_color_dialog_select_callback_set(wd->dia, _e_wid_color_select_cb, wd);
	e_color_dialog_cancel_callback_set(wd->dia, _e_wid_color_cancel_cb, wd);
	e_color_dialog_change_callback_set(wd->dia, _e_wid_color_change_cb, wd);
     }
   e_color_dialog_show(wd->dia);
}

static void
_e_wid_color_select_cb(E_Color_Dialog *dia, E_Color *color, void *data)
{
   E_Widget_Data *wd;
   wd = data;
   e_color_copy(color, wd->color);
   _e_wid_update(wd);
   wd->dia = NULL;
}

static void
_e_wid_color_cancel_cb(E_Color_Dialog *dia, E_Color *color, void *data)
{
   E_Widget_Data *wd;
   wd = data;
   /* cancel sends the original color back */
   e_color_copy(color, wd->color);
   _e_wid_update(wd);
   wd->dia = NULL;
}

static void
_e_wid_color_change_cb(E_Color_Dialog *dia, E_Color *color, void *data)
{
   E_Widget_Data *wd;
   wd = data;
   e_color_copy(color, wd->color);
   _e_wid_update(wd);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   if (wd->dia) e_object_unref(E_OBJECT(wd->dia));
   E_FREE(wd);
}

static void 
_e_wid_disable_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj)) 
     {
	wd->show_color_dialog = 0;
	edje_object_signal_emit(wd->o_edje, "e,state,disabled", "e");
     }
   else 
     {
	wd->show_color_dialog = 1;	
	edje_object_signal_emit(wd->o_edje, "e,state,enabled", "e");
     }
}

/**
 * Add a color well widget to an evas.
 * An optional E_Container may be passed in. 
 If not NULL, when clicked a color dialog will pop up.
 */
Evas_Object *
e_widget_color_well_add(Evas *evas, E_Color *color, int show_color_dialog)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   E_Win *win;

   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   wd->obj = obj;

   wd->color = color;
   win = e_win_evas_object_win_get(obj);
   wd->con = win->container;

   wd->show_color_dialog = show_color_dialog;

   o = edje_object_add(evas);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/color_well");
   edje_object_signal_callback_add(o, "e,action,click", "", 
				   _e_wid_signal_cb1, obj);
   evas_object_show(o); 
   wd->o_edje = o;

   e_widget_min_size_set(obj, 50, 50);

   o = evas_object_rectangle_add(evas);
   e_widget_sub_object_add(obj, o);
   evas_object_color_set(o,
			 (color->r * color->a) / 255,
			 (color->g * color->a) / 255,
			 (color->b * color->a) / 255,
			 color->a);
   edje_object_part_swallow(wd->o_edje, "e.swallow.content", o);
   evas_object_show(o);
   wd->o_rect = o;

   return obj;
}

/**
 * Let the color well know that its color data has changed.
 */
void
e_widget_color_well_update(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   _e_wid_update(wd);
}
