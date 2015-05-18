#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_check;
   Evas_Object *o_icon;
   int         *valptr;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_do(Evas_Object *obj);
static void _e_wid_activate_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* local subsystem functions */

/* externally accessible functions */

/**  
 * Creates a check box widget
 *
 * @param evas pointer
 * @param label the text to asign to check widget
 * @param val pointer to int where changes are stored
 * @return the newly created evas object
 */
EAPI Evas_Object *
e_widget_check_add(Evas *evas, const char *label, int *val)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_activate_hook_set(obj, _e_wid_activate_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   e_widget_data_set(obj, wd);

   o = edje_object_add(evas);
   wd->o_check = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
                           "e/widgets/check");
   edje_object_signal_callback_add(o, "e,action,toggle", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "e.text.label", label);
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
   if (wd->valptr)
     {
        if (*(wd->valptr)) edje_object_signal_emit(o, "e,state,checked", "e");
     }

   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);

   return obj;
}

/**  
 * Sets the value of the check box
 *
 * @param check the check box widget
 * @param checked the value to set the widget too
 */
EAPI void
e_widget_check_checked_set(Evas_Object *check, int checked)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(check);
   if (wd->valptr)
     *(wd->valptr) = checked;
   if (checked)
     edje_object_signal_emit(wd->o_check, "e,state,checked", "e");
   else
     edje_object_signal_emit(wd->o_check, "e,state,unchecked", "e");
}

/**  
 * Sets the value of the check box
 *
 * @param check the check box widget
 * @param val pointer to int where changes are stored
 */
EAPI void
e_widget_check_valptr_set(Evas_Object *check, int *val)
{
   E_Widget_Data *wd;
   int state;

   wd = e_widget_data_get(check);
   state = wd->valptr ? *wd->valptr : 0;
   wd->valptr = val;
   if (state == (wd->valptr && *wd->valptr)) return;
   if (wd->valptr && *wd->valptr)
     edje_object_signal_emit(wd->o_check, "e,state,checked", "e");
   else
     edje_object_signal_emit(wd->o_check, "e,state,unchecked", "e");
}

/**  
 *Get the value of the check box
 *
 * @param check the check box widget
 * @return the value of the check box
 */
EAPI int
e_widget_check_checked_get(Evas_Object *check)
{
   E_Widget_Data *wd;
   int ret;

   wd = e_widget_data_get(check);
   if (wd->valptr)
     ret = *(wd->valptr);
   else
     ret = -1;

   return ret;
}

/**  
 * Creates a check box widget with icon
 *
 * @param evas pointer
 * @param label the text to asign to check widget
 * @param icon the path to the icon file
 * @param icon_w the minimum wdith of the icon
 * @param icon_h the minumum height of the icon
 * @param val pointer to int where changes are stored
 * @return the newly created evas object
 */
EAPI Evas_Object *
e_widget_check_icon_add(Evas *evas, const char *label, const char *icon, int icon_w, int icon_h, int *val)
{
   Evas_Object *obj, *o, *o2;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_activate_hook_set(obj, _e_wid_activate_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   e_widget_data_set(obj, wd);

   o = edje_object_add(evas);
   wd->o_check = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
                           "e/widgets/check_icon");
   edje_object_signal_callback_add(o, "e,action,toggle", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "e.text.label", label);
   evas_object_show(o);
   if (label)
     {
        edje_object_signal_emit(o, "e,state,labeled", "e");
        edje_object_message_signal_process(o);
     }
   if (icon)
     {
        if (icon[0] == '/')
          {
             o2 = e_icon_add(evas);
             e_icon_file_set(o2, icon);
          }
        else
          {
             o2 = edje_object_add(evas);
             e_util_edje_icon_set(o2, icon);
          }
        evas_object_size_hint_min_set(o2, icon_w, icon_h);
        edje_object_part_swallow(wd->o_check, "e.swallow.icon", o2);
        evas_object_show(o2);
        e_widget_sub_object_add(obj, o2);
        wd->o_icon = o2;
     }

   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
   if (wd->valptr)
     {
        if (*(wd->valptr)) edje_object_signal_emit(o, "e,state,checked", "e");
     }

   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);

   return obj;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->o_check, "e,state,focused", "e");
        evas_object_focus_set(wd->o_check, 1);
     }
   else
     {
        edje_object_signal_emit(wd->o_check, "e,state,unfocused", "e");
        evas_object_focus_set(wd->o_check, 0);
     }
}

static void
_e_wid_do(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (e_widget_disabled_get(obj)) return;

   wd = e_widget_data_get(obj);
   if (wd->valptr)
     {
        if (*(wd->valptr) == 0)
          {
             *(wd->valptr) = 1;
             edje_object_signal_emit(wd->o_check, "e,state,checked", "e");
          }
        else
          {
             *(wd->valptr) = 0;
             edje_object_signal_emit(wd->o_check, "e,state,unchecked", "e");
          }
     }
   evas_object_smart_callback_call(obj, "changed", NULL);
   e_widget_change(obj);
}

static void
_e_wid_activate_hook(Evas_Object *obj)
{
   _e_wid_do(obj);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(wd->o_check, "e,state,disabled", "e");
   else
     edje_object_signal_emit(wd->o_check, "e,state,enabled", "e");
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_wid_do(data);
}

static void
_e_wid_focus_steal(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   e_widget_focus_steal(data);
}

