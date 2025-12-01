#include "e.h"

typedef struct _E_Rect_Smart_Data E_Rect_Smart_Data;
struct _E_Rect_Smart_Data
{
   Evas_Object *evas_object;
};

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_entry, *o_inout;
   char **text_location;
   void (*func) (void *data, void *data2);
   void *data;
   void *data2;
   Eina_Bool have_pointer : 1;
};

/* local subsystem functions */
static void _e_rect_smart_add(Evas_Object *object);
static void _e_rect_smart_del(Evas_Object *object);
static void _e_rect_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_rect_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_rect_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_wid_keydown(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_movresz(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Evas_Smart *_e_rect_smart = NULL;
static int _e_rect_smart_use = 0;

static Evas_Object *
e_rect_add(Evas *evas)
{
   if (!_e_rect_smart)
     {
       static const Evas_Smart_Class sc = {
         .name       = "e_rect",
         .version    = EVAS_SMART_CLASS_VERSION,
         .add        = _e_rect_smart_add,
         .del        = _e_rect_smart_del,
         .move       = _e_rect_smart_move,
         .resize     = _e_rect_smart_resize,
         .show       = NULL,
         .hide       = NULL,
         .color_set  = _e_rect_color_set,
         .clip_set   = NULL,
         .clip_unset = NULL,
         .calculate  = NULL,
         .member_add = NULL,
         .member_del = NULL,
       };
       _e_rect_smart = evas_smart_class_new(&sc);
       _e_rect_smart_use = 0;
     }

   _e_rect_smart_use++;
   return evas_object_smart_add(evas, _e_rect_smart);
}

/* externally accessible functions */

/**
 * Creates a new entry widget
 *
 * @param evas the evas where to add the new entry widget
 * @param text_location the location where to store the text of the entry.
 * @param func DOCUMENT ME!
 * @param data  DOCUMENT ME!
 * @param data2 DOCUMENT ME!
 * The current value will be used to initialize the entry
 * @return Returns the new entry widget
 */

EAPI Evas_Object *
e_widget_entry_add(Evas *evas, char **text_location, void (*func) (void *data, void *data2), void *data, void *data2)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord minw, minh;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);

   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   wd->text_location = text_location;

   o = e_entry_add(evas);
   wd->o_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_show(o);
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _e_wid_keydown, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _e_wid_movresz, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _e_wid_movresz, obj);
   
   //~ o = evas_object_rectangle_add(evas);
   o = e_rect_add(evas);
   wd->o_inout = o;
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN, _e_wid_in, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _e_wid_out, obj);

   o = wd->o_entry;
   if ((text_location) && (*text_location))
     e_entry_text_set(o, *text_location);

   e_entry_size_min_get(o, &minw, &minh);
   e_widget_size_min_set(obj, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);

   wd->func = func;
   wd->data = data;
   wd->data2 = data2;
   evas_object_smart_callback_add(o, "changed", _e_wid_changed_cb, obj);

   return obj;
}

/**
 * Sets the text of the entry widget
 *
 * @param entry an entry widget
 * @param text the text to set
 */
EAPI void
e_widget_entry_text_set(Evas_Object *entry, const char *text)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return;
   e_entry_text_set(wd->o_entry, text);
}

/**
 * Gets the text of the entry widget
 *
 * @param entry an entry widget
 * @return Returns the text of the entry widget
 */
EAPI const char *
e_widget_entry_text_get(Evas_Object *entry)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return NULL;
   return e_entry_text_get(wd->o_entry);
}

/**
 * Clears the entry widget
 *
 * @param entry an entry widget
 */
EAPI void
e_widget_entry_clear(Evas_Object *entry)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return;
   e_entry_clear(wd->o_entry);
}

/**
 * Sets whether or not the entry widget is in password mode. In password mode,
 * the entry displays '*' instead of the characters
 *
 * @param entry an entry widget
 * @param password_mode 1 to turn on password mode, 0 to turn it off
 */
EAPI void
e_widget_entry_password_set(Evas_Object *entry, int password_mode)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return;
   e_entry_password_set(wd->o_entry, password_mode);
}

/**
 * Sets whether or not the entry widget is user-editable. This still
 * allows copying and selecting, just no inserting or deleting of text.
 *
 * @param entry an entry widget
 * @param readonly_mode 1 to enable read-only mode, 0 to turn it off
 */
EAPI void
e_widget_entry_readonly_set(Evas_Object *entry, int readonly_mode)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return;

   if (readonly_mode)
     e_entry_noedit(wd->o_entry);
   else
     e_entry_edit(wd->o_entry);
}

/**
 * Selects the content of the entry.
 *
 * @param entry an entry widget
 */
EAPI void
e_widget_entry_select_all(Evas_Object *entry)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return;
   e_entry_select_all(wd->o_entry);
}


/* Private functions */

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!(obj) || (!(wd = e_widget_data_get(obj))))
      return;
   free(wd);
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!(obj) || (!(wd = e_widget_data_get(obj))))
      return;

   if (e_widget_focus_get(obj))
     e_entry_focus(wd->o_entry);
   else
     e_entry_unfocus(wd->o_entry);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!(obj) || (!(wd = e_widget_data_get(obj))))
      return;

   if (e_widget_disabled_get(obj))
     e_entry_disable(wd->o_entry);
   else
     e_entry_enable(wd->o_entry);
}

static void
_e_wid_focus_steal(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_in(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Pointer *p;
   E_Widget_Data *wd;
   
   if (!(data) || (!(wd = e_widget_data_get(data))))
     return;
   if (wd->have_pointer) return;
   p = e_widget_pointer_get(data);
   if (p) e_pointer_type_push(p, data, "entry");
   wd->have_pointer = EINA_TRUE;
}

static void
_e_wid_out(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Pointer *p;
   E_Widget_Data *wd;
   
   if (!(data) || (!(wd = e_widget_data_get(data))))
     return;
   if (!wd->have_pointer) return;
   p = e_widget_pointer_get(data);
   if (p) e_pointer_type_pop(p, data, "entry");
   wd->have_pointer = EINA_FALSE;
}

static void
_e_wid_changed_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *entry;
   E_Widget_Data *wd;
   const char *text;

   if (!(entry = data) || (!(wd = e_widget_data_get(entry))))
      return;

   if (wd->text_location)
     {
        text = e_entry_text_get(wd->o_entry);
        free(*wd->text_location);
        *wd->text_location = text ? strdup(text) : NULL;
     }
   e_widget_change(data);

   if (wd->func) wd->func(wd->data, wd->data2);
}

static void
_e_wid_keydown(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   evas_object_smart_callback_call(data, "key_down", event_info);
}

static void
_e_wid_movresz(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;
   
   if (!(data) || (!(wd = e_widget_data_get(data))))
     return;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   if (wd->o_inout)
     {
        evas_object_move(wd->o_inout, x, y);
        evas_object_resize(wd->o_inout, w, h);
     }
}

static void
_e_rect_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Rect_Smart_Data *sd;

   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = calloc(1, sizeof(E_Rect_Smart_Data));
   if (!sd) return;

   evas_object_smart_data_set(object, sd);

   sd->evas_object = evas_object_rectangle_add(evas);
   evas_object_smart_member_add(sd->evas_object, object);
   evas_object_repeat_events_set(sd->evas_object, EINA_TRUE);
   evas_object_color_set(sd->evas_object, 0, 0, 0, 0);
   evas_object_show(sd->evas_object);
}

static void
_e_rect_smart_del(Evas_Object *object)
{
   E_Rect_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_del(sd->evas_object);
   free(sd);
   evas_object_smart_data_set(object, NULL);
}

static void _e_rect_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
    E_Rect_Smart_Data *sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_move(sd->evas_object, x, y);
}

static void _e_rect_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
    E_Rect_Smart_Data *sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_resize(sd->evas_object, w, h);
}

static void
_e_rect_color_set(Evas_Object *object, int r, int g, int b, int a)
{
   E_Rect_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_color_set(sd->evas_object, r, g, b, a);
}
