#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data 
{
   Evas_Object *obj, *o_frame, *o_clip;
   Evas_Object *prev_bg, *bg, *o_trans;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_done(void *data, Evas_Object *obj, const char *emission, const char *source);

EAPI Evas_Object *
e_widget_trans_preview_add(Evas *evas, int minw, int minh) 
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->obj = obj;

   o = edje_object_add(evas);
   wd->o_frame = o;
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/preview");
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = edje_object_add(evas);
   wd->prev_bg = o;
   e_theme_edje_object_set(o, "base/theme/backgrounds", "e/desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->prev_bg);
   
   e_widget_data_set(obj, wd);
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, minw, minh);

   return obj;
}

void 
e_widget_trans_preview_trans_set(Evas_Object *obj, const char *trans) 
{
   Evas *evas;
   Evas_Object *o;
   E_Widget_Data *wd;
   char buf[4096];
   
   wd = e_widget_data_get(obj);
   evas = evas_object_evas_get(wd->o_frame);
   
   if (wd->o_trans)
     evas_object_del(wd->o_trans);
   if (wd->bg)
     evas_object_del(wd->bg);
   if (wd->prev_bg)
     evas_object_del(wd->prev_bg);
   
   snprintf(buf, sizeof(buf), "e/transitions/%s", trans);

   o = edje_object_add(evas);
   wd->o_trans = o;
   e_theme_edje_object_set(wd->o_trans, "base/theme/transitions", buf);
   edje_object_signal_callback_add(o, "e,state,done", "*", _e_wid_done, wd);
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->o_trans);

   o = edje_object_add(evas);
   wd->bg = o;
   e_theme_edje_object_set(o, "base/theme/icons", "e/icons/enlightenment/e");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);

   o = edje_object_add(evas);
   wd->prev_bg = o;
   e_theme_edje_object_set(o, "base/theme/backgrounds", "e/desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   
   edje_object_part_swallow(wd->o_trans, "e.swallow.bg.old", wd->prev_bg);
   edje_object_part_swallow(wd->o_trans, "e.swallow.bg.new", wd->bg);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->o_trans);
   
   edje_object_signal_emit(wd->o_trans, "e,action,start", "e");
}

static void 
_e_wid_del_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (wd->o_frame)
     evas_object_del(wd->o_frame);
   if (wd->o_trans)
     evas_object_del(wd->o_trans);
   if (wd->bg)
     evas_object_del(wd->bg);
   if (wd->prev_bg)
     evas_object_del(wd->prev_bg);
   E_FREE(wd);
}

static void 
_e_wid_done(void *data, Evas_Object *obj, const char *emission, const char *source) 
{
   E_Widget_Data *wd;
   Evas_Object *o;
   Evas *evas;
   
   wd = data;
   evas = evas_object_evas_get(wd->o_frame);
   
   if (wd->o_trans) 
     evas_object_del(wd->o_trans);
   if (wd->bg)
     evas_object_del(wd->bg);
   if (wd->prev_bg)
     evas_object_del(wd->prev_bg);

   o = edje_object_add(evas);
   wd->prev_bg = o;
   e_theme_edje_object_set(o, "base/theme/backgrounds", "e/desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, wd->o_frame);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->prev_bg);
}
