/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_entry;
   char **text_location;
   void (*func) (void *data, void *data2);
   void *data;
   void *data2;
};

/* local subsystem functions */
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_wid_keydown(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* externally accessible functions */

/**
 * Creates a new entry widget
 *
 * @param evas the evas where to add the new entry widget
 * @param text_location the location where to store the text of the entry.
 * The current value will be used to initialize the entry
 * @return Returns the new entry widget
 */
EAPI Evas_Object
*e_widget_entry_add(Evas *evas, char **text_location, void (*func) (void *data, void *data2), void *data, void *data2)
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
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN, _e_wid_in, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _e_wid_out, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _e_wid_keydown, obj);

   if ((text_location) && (*text_location))
     e_entry_text_set(o, *text_location);
   
   e_entry_min_size_get(o, &minw, &minh);
   e_widget_min_size_set(obj, minw, minh);

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
 * @param readonly 1 to enable read-only mode, 0 to turn it off
 */
EAPI void
e_widget_entry_readonly_set(Evas_Object *entry, int readonly_mode)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return;

   if (readonly_mode)
     e_entry_disable(wd->o_entry);
   else
     e_entry_enable(wd->o_entry);
}

/**
 * Gets the editable object of the entry widget. It will allow you to have
 * better control on the text, the cursor or the selection of the entry with
 * the e_editable_*() functions.
 *
 * @param entry an entry widget
 * @return Returns the editable object of the entry widget
 */
EAPI Evas_Object *
e_widget_entry_editable_object_get(Evas_Object *entry)
{
   E_Widget_Data *wd;

   if (!(entry) || (!(wd = e_widget_data_get(entry))))
      return NULL;
   return e_entry_editable_object_get(wd->o_entry);
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
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Pointer *p;
   
   p = e_widget_pointer_get(data);
   if (p) e_pointer_type_push(p, data, "entry");
}

static void
_e_wid_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Pointer *p;
   
   p = e_widget_pointer_get(data);
   if (p) e_pointer_type_pop(p, data, "entry");
}

static void
_e_wid_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *entry;
   E_Widget_Data *wd;
   const char *text;

   if (!(entry = data) || (!(wd = e_widget_data_get(entry))))
      return;
   
   if (wd->text_location)
     {
        text = e_widget_entry_text_get(entry);
        free(*wd->text_location);
        *wd->text_location = text ? strdup(text) : NULL;
     }
   e_widget_change(data);

   if (wd->func) wd->func(wd->data, wd->data2);
}

static void 
_e_wid_keydown(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   evas_object_smart_callback_call(data, "key_down", event_info);
}
