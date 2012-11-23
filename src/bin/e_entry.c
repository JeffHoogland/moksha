#include "e.h"

#define ENTRY_PART_NAME "e.text.text"

typedef struct _E_Entry_Smart_Data E_Entry_Smart_Data;

struct _E_Entry_Smart_Data
{
   Evas_Object *entry_object;
   Evas_Object *scroll_object;
   E_Menu *popup;
   Ecore_Event_Handler *selection_handler;

   int min_width;
   int height;
   Evas_Coord theme_width;
   Evas_Coord theme_height;
   int preedit_start_pos;
   int preedit_end_pos;
   int changing;
   Eina_Bool enabled : 1;
   Eina_Bool noedit : 1;
   Eina_Bool focused : 1;
   Eina_Bool password_mode : 1;
};

/* local subsystem functions */
static Eina_Bool _e_entry_x_selection_notify_handler(void *data, int type, void *event);

static void _e_entry_x_selection_update(Evas_Object *entry);

static void _e_entry_smart_add(Evas_Object *object);
static void _e_entry_smart_del(Evas_Object *object);
static void _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_entry_smart_show(Evas_Object *object);
static void _e_entry_smart_hide(Evas_Object *object);
static void _e_entry_color_set(Evas_Object *object, int r, int g, int b, int a);
static void _e_entry_clip_set(Evas_Object *object, Evas_Object *clip);
static void _e_entry_clip_unset(Evas_Object *object);
static void _e_entry_cb_menu_post(void *data, E_Menu *m);
static void _e_entry_cb_cut(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_copy(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_paste(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_select_all(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_delete(void *data, E_Menu *m , E_Menu_Item *mi );

/* local subsystem globals */
static Evas_Smart *_e_entry_smart = NULL;
static int _e_entry_smart_use = 0;


/* externally accessible functions */

/**
 * Creates a new entry object. An entry is a field where the user can type
 * single-line text.
 * Use the "changed" smart callback to know when the content of the entry is
 * changed
 *
 * @param evas the evas where the entry object should be added
 * @return Returns the new entry object
 */
EAPI Evas_Object *
e_entry_add(Evas *evas)
{
   if (!_e_entry_smart)
     {
        static const Evas_Smart_Class sc =
          {
             "e_entry",
             EVAS_SMART_CLASS_VERSION,
             _e_entry_smart_add,
             _e_entry_smart_del,
             _e_entry_smart_move,
             _e_entry_smart_resize,
             _e_entry_smart_show,
             _e_entry_smart_hide,
             _e_entry_color_set,
             _e_entry_clip_set,
             _e_entry_clip_unset,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
          };
        _e_entry_smart = evas_smart_class_new(&sc);
        _e_entry_smart_use = 0;
     }

   _e_entry_smart_use++;
   return evas_object_smart_add(evas, _e_entry_smart);
}

/**
 * Sets the text of the entry object
 *
 * @param entry an entry object
 * @param text the text to set
 */
EAPI void
e_entry_text_set(Evas_Object *entry, const char *_text)
{
   E_Entry_Smart_Data *sd;
   char *text = NULL;
   const char *otext;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;

   text = evas_textblock_text_utf8_to_markup(
         edje_object_part_object_get(sd->entry_object, ENTRY_PART_NAME),
         _text);
   otext = edje_object_part_text_get(sd->entry_object, ENTRY_PART_NAME);
   if ((text) && (otext) && (!strcmp(text, otext))) return;
   if ((!text) && (!otext)) return;
   edje_object_part_text_set(sd->entry_object, ENTRY_PART_NAME, text);
   sd->changing++;
   edje_object_message_signal_process(sd->entry_object);
   sd->changing--;
   evas_object_smart_callback_call(entry, "changed", NULL);
   if (text) free(text);
}

/**
 * Gets the text of the entry object
 *
 * @param entry an entry object
 * @return Returns the text of the entry object
 */
EAPI const char *
e_entry_text_get(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   static char *text = NULL;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERR(NULL);
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return NULL;

   if (text)
     {
        E_FREE(text);
     }
   text = evas_textblock_text_markup_to_utf8(
         edje_object_part_object_get(sd->entry_object, ENTRY_PART_NAME),
         edje_object_part_text_get(sd->entry_object, ENTRY_PART_NAME));
   return text;
}

/**
 * Clears the entry object
 *
 * @param entry an entry object
 */
EAPI void
e_entry_clear(Evas_Object *entry)
{
   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   e_entry_text_set(entry, "");
}

/**
 * Selects all the text of the entry.
 *
 * @param entry an entry object
 */
EAPI void
e_entry_select_all(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;

   _e_entry_cb_select_all(sd, NULL, NULL);
}

/**
 * Sets whether or not the entry object is in password mode. In password mode,
 * the entry displays '*' instead of the characters
 *
 * @param entry an entry object
 * @param password_mode 1 to turn on password mode, 0 to turn it off
 */
EAPI void
e_entry_password_set(Evas_Object *entry, int password_mode)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (sd->password_mode == password_mode)
      return;

   sd->password_mode = !!password_mode;
   if (!sd->password_mode)
      e_theme_edje_object_set(sd->entry_object, "base/theme/widgets", "e/widgets/entry/text");
   else
      e_theme_edje_object_set(sd->entry_object, "base/theme/widgets", "e/widgets/entry/password");
}

/**
 * Gets the minimum size of the entry object
 *
 * @param entry an entry object
 * @param minw the location where to store the minimun width of the entry
 * @param minh the location where to store the minimun height of the entry
 */
EAPI void
e_entry_size_min_get(Evas_Object *entry, Evas_Coord *minw, Evas_Coord *minh)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;

   if (minw) *minw = sd->theme_width + sd->min_width;
   if (minh) *minh = sd->theme_height + sd->height;
}

/**
 * Focuses the entry object. It will receives keyboard events and the user could
 * then type some text (the entry should also be enabled. The cursor and the
 * selection will be shown
 *
 * @param entry the entry to focus
 */
EAPI void
e_entry_focus(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (sd->focused)
     return;

   evas_object_focus_set(sd->entry_object, EINA_TRUE);
   edje_object_signal_emit(sd->entry_object, "e,state,focused", "e");

   edje_object_part_text_cursor_end_set(sd->entry_object, ENTRY_PART_NAME, EDJE_CURSOR_MAIN);
   if ((sd->enabled) && (!sd->noedit))
      edje_object_signal_emit(sd->entry_object, "e,action,show,cursor", "e");
   sd->focused = EINA_TRUE;
}

/**
 * Unfocuses the entry object. It will no longer receives keyboard events so
 * the user could no longer type some text. The cursor and the selection will
 * be hidden
 *
 * @param entry the entry object to unfocus
 */
EAPI void
e_entry_unfocus(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!sd->focused)
     return;

   edje_object_signal_emit(sd->entry_object, "e,state,unfocused", "e");
   evas_object_focus_set(sd->entry_object, EINA_FALSE);
   edje_object_signal_emit(sd->entry_object, "e,action,hide,cursor", "e");
   sd->focused = EINA_FALSE;
}

/**
 * Enables the entry object: the user will be able to type text
 *
 * @param entry the entry object to enable
 */
EAPI void
e_entry_enable(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (sd->enabled)
     return;

   edje_object_signal_emit(e_scrollframe_edje_object_get(sd->scroll_object),
         "e,state,enabled", "e");
   edje_object_signal_emit(sd->entry_object, "e,state,enabled", "e");
   if (sd->focused)
      edje_object_signal_emit(sd->entry_object, "e,action,show,cursor", "e");
   sd->enabled = EINA_TRUE;
}

/**
 * Disables the entry object: the user won't be able to type anymore. Selection
 * will still be possible (to copy the text)
 *
 * @param entry the entry object to disable
 */
EAPI void
e_entry_disable(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!sd->enabled)
     return;

   edje_object_signal_emit(e_scrollframe_edje_object_get(sd->scroll_object),
         "e,state,disabled", "e");
   edje_object_signal_emit(sd->entry_object, "e,state,disabled", "e");
   edje_object_signal_emit(sd->entry_object, "e,action,hide,cursor", "e");
   sd->enabled = EINA_FALSE;
}

/**
 * Enables the entry object: the user will be able to type text
 *
 * @param entry the entry object to enable
 */
EAPI void
e_entry_edit(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!sd->noedit)
     return;

   edje_object_signal_emit(e_scrollframe_edje_object_get(sd->scroll_object),
         "e,state,edit", "e");
   edje_object_signal_emit(sd->entry_object, "e,state,edit", "e");
   if (sd->focused)
      edje_object_signal_emit(sd->entry_object, "e,action,show,cursor", "e");
   sd->noedit = EINA_FALSE;
}

/**
 * Disables the entry object: the user won't be able to type anymore. Selection
 * will still be possible (to copy the text)
 *
 * @param entry the entry object to disable
 */
EAPI void
e_entry_noedit(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if (evas_object_smart_smart_get(entry) != _e_entry_smart) SMARTERRNR();
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (sd->noedit)
     return;

   edje_object_signal_emit(e_scrollframe_edje_object_get(sd->scroll_object),
         "e,state,noedit", "e");
   edje_object_signal_emit(sd->entry_object, "e,state,noedit", "e");
   edje_object_signal_emit(sd->entry_object, "e,action,hide,cursor", "e");
   sd->noedit = EINA_TRUE;
}


/* Private functions */

static Eina_Bool
_e_entry_is_empty(const Evas_Object *entry_edje)
{
   const Evas_Object *tb;
   Evas_Textblock_Cursor *cur;
   Eina_Bool ret;

   /* It's a hack until we get the support suggested above.  We just
    * create a cursor, point it to the begining, and then try to
    * advance it, if it can advance, the tb is not empty, otherwise it
    * is. */
   tb = edje_object_part_object_get(entry_edje, ENTRY_PART_NAME);

   cur = evas_object_textblock_cursor_new((Evas_Object *)tb);
   evas_textblock_cursor_pos_set(cur, 0);
   ret = evas_textblock_cursor_char_next(cur);
   evas_textblock_cursor_free(cur);

   return !ret;

}

/* Called when the entry object is pressed by the mouse */
static void
_e_entry_mouse_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;
   Evas_Event_Mouse_Down *event;

   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   if (!(event = event_info))
     return;

   if (event->button == 3)
     {
        E_Menu_Item *mi;
        E_Manager *man;
        E_Container *con;
        Evas_Coord x, y;
        int s_enabled, s_selecting, s_passwd, s_empty;

        s_selecting = !!edje_object_part_text_selection_get(sd->entry_object, ENTRY_PART_NAME);
        s_enabled = sd->enabled;
        s_passwd = sd->password_mode;
        s_empty = _e_entry_is_empty(sd->entry_object);

        if (!s_selecting && !s_enabled && s_empty) return;

        man = e_manager_current_get();
        con = e_container_current_get(man);
        ecore_x_pointer_xy_get(con->win, &x, &y);

        /* Popup a menu */
        sd->popup = e_menu_new();
        e_menu_post_deactivate_callback_set(sd->popup,
                                            _e_entry_cb_menu_post, sd);
        if (s_selecting)
          {
             if (s_enabled)
               {
                  if (!sd->noedit)
                    {
                       mi = e_menu_item_new(sd->popup);
                       e_menu_item_label_set(mi, _("Delete"));
                       e_util_menu_item_theme_icon_set(mi, "edit-delete");
                       e_menu_item_callback_set(mi, _e_entry_cb_delete, sd);
                       
                       mi = e_menu_item_new(sd->popup);
                       e_menu_item_separator_set(mi, 1);
                       
                       if (!s_passwd)
                         {
                            mi = e_menu_item_new(sd->popup);
                            e_menu_item_label_set(mi, _("Cut"));
                            e_util_menu_item_theme_icon_set(mi, "edit-cut");
                            e_menu_item_callback_set(mi, _e_entry_cb_cut, sd);
                         }
                    }
               }
             if (!s_passwd)
               {
                  mi = e_menu_item_new(sd->popup);
                  e_menu_item_label_set(mi, _("Copy"));
                  e_util_menu_item_theme_icon_set(mi, "edit-copy");
                  e_menu_item_callback_set(mi, _e_entry_cb_copy, sd);
               }
          }
        if (sd->enabled)
          {
             if (!sd->noedit)
               {
                  mi = e_menu_item_new(sd->popup);
                  e_menu_item_label_set(mi, _("Paste"));
                  e_util_menu_item_theme_icon_set(mi, "edit-paste");
                  e_menu_item_callback_set(mi, _e_entry_cb_paste, sd);
               }
          }
        if (!s_empty)
          {
             mi = e_menu_item_new(sd->popup);
             e_menu_item_separator_set(mi, 1);

             mi = e_menu_item_new(sd->popup);
             e_menu_item_label_set(mi, _("Select All"));
             e_util_menu_item_theme_icon_set(mi, "edit-select-all");
             e_menu_item_callback_set(mi, _e_entry_cb_select_all, sd);
          }

        e_menu_activate_mouse(sd->popup, e_util_zone_current_get(man),
                              x, y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, event->timestamp);
     }
}

/* Called when the the "selection_notify" event is emitted */
static Eina_Bool
_e_entry_x_selection_notify_handler(void *data, int type __UNUSED__, void *event)
{
   Evas_Object *entry;
   E_Entry_Smart_Data *sd;
   Ecore_X_Event_Selection_Notify *ev;

   if ((!(entry = data)) || (!(sd = evas_object_smart_data_get(entry))))
     return EINA_TRUE;
   if (!sd->focused)
     return EINA_TRUE;

   ev = event;
   if (((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
            (ev->selection == ECORE_X_SELECTION_PRIMARY)) &&
         (strcmp(ev->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0))
     {
        Ecore_X_Selection_Data_Text *text_data;

        text_data = ev->data;
        if ((text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT) &&
              (text_data->text))
          {
             char *txt = evas_textblock_text_utf8_to_markup(NULL, text_data->text);
             if (txt)
               {
                  edje_object_part_text_user_insert(sd->entry_object,
                        ENTRY_PART_NAME, txt);
                  free(txt);
               }
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

/* Updates the X selection with the selected text of the entry */
static void
_e_entry_x_selection_update(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   Ecore_X_Window xwin;
   E_Win *win;
   E_Container *con;
   const char *text;

   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!(win = e_win_evas_object_win_get(entry)))
     {
        con = e_container_evas_object_container_get(entry);
        if (!con) return;
        xwin = ecore_evas_window_get(con->bg_ecore_evas);
     }
   else
     xwin = win->evas_win;

   if (sd->password_mode)
      return;

   text = edje_object_part_text_selection_get(sd->entry_object, ENTRY_PART_NAME);
   if (text)
     ecore_x_selection_primary_set(xwin, text, strlen(text) + 1);
}

static void
_entry_paste_request_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission,
      const char *source EINA_UNUSED)
{
   Ecore_X_Window xwin;
   E_Win *win;
   E_Container *con;
   
   if (!(win = e_win_evas_object_win_get(data)))
     {
        con = e_container_evas_object_container_get(data);
        if (!con) return;
        xwin = ecore_evas_window_get(con->bg_ecore_evas);
     }
   else
     xwin = win->evas_win;

   if (emission[sizeof("ntry,paste,request,")] == '1')
     {
        ecore_x_selection_primary_request(xwin,
              ECORE_X_SELECTION_TARGET_UTF8_STRING);
     }
   else
     {
        ecore_x_selection_clipboard_request(xwin,
              ECORE_X_SELECTION_TARGET_UTF8_STRING);
     }
}

static void
_entry_selection_changed_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   _e_entry_x_selection_update(data);
}

static void
_entry_selection_all_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   e_entry_select_all(data);
}

static void
_entry_copy_notify_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   E_Entry_Smart_Data *sd;

   if ((!data) || !(sd = evas_object_smart_data_get(data)))
     return;

   _e_entry_cb_copy(sd, NULL, NULL);
}

static void
_entry_cut_notify_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   E_Entry_Smart_Data *sd;

   if ((!data) || !(sd = evas_object_smart_data_get(data)))
     return;

   _e_entry_cb_cut(sd, NULL, NULL);
}

static void
_entry_recalc_size(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   Evas_Coord vw, vh, w, h, pw = 0, ph = 0;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   e_scrollframe_child_viewport_size_get(sd->scroll_object, &vw, &vh);
   w = (sd->min_width < vw) ? vw : sd->min_width;
   h = (sd->height < vh) ? vh : sd->height;
   evas_object_geometry_get(sd->entry_object, NULL, NULL, &pw, &ph);
   if ((w == pw) && (h == ph)) return;
   evas_object_resize(sd->entry_object, w, h);
}

static void
_entry_changed_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   Evas_Object *object = data;
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   if (sd->changing) return;
   evas_object_smart_callback_call(object, "changed", NULL);
   edje_object_size_min_calc(sd->entry_object, &sd->min_width, &sd->height);
   _entry_recalc_size(object);
}

static void
_entry_cursor_changed_signal_cb(void *data,
      Evas_Object *obj EINA_UNUSED,
      const char *emission EINA_UNUSED,
      const char *source EINA_UNUSED)
{
   Evas_Object *object = data;
   E_Entry_Smart_Data *sd;
   Evas_Coord cx, cy, cw, ch;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   edje_object_part_text_cursor_geometry_get(sd->entry_object, ENTRY_PART_NAME,
         &cx, &cy, &cw, &ch);
   e_scrollframe_child_region_show(sd->scroll_object, cx, cy, cw, ch);
}

/* Editable object's smart methods */

static void
_e_entry_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Entry_Smart_Data *sd;
   Evas_Object *o;

   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = calloc(1, sizeof(E_Entry_Smart_Data));
   if (!sd) return;

   evas_object_smart_data_set(object, sd);

   sd->enabled = EINA_TRUE;
   sd->noedit = EINA_FALSE;
   sd->focused = EINA_FALSE;

   sd->scroll_object = e_scrollframe_add(evas);
   e_scrollframe_key_navigation_set(sd->scroll_object, EINA_FALSE);
   /* We need that, as currently mouse grabbing breaks if that's not set. That's
    * what you get when you have scrolling and selection in the same place.
    * We can just use selection for scrolling or fix the issue. The fix would
    * probably require using the ON_SCROLL flag instead of the ON_HOLD and
    * actually handle it correctly. */
   e_scrollframe_thumbscroll_force(sd->scroll_object, EINA_FALSE);
   evas_object_propagate_events_set(sd->scroll_object, EINA_TRUE);
   e_scrollframe_custom_theme_set(sd->scroll_object, "base/theme/widgets", "e/widgets/entry/scrollframe");
   edje_object_size_min_calc(e_scrollframe_edje_object_get(sd->scroll_object),
         &sd->theme_width, &sd->theme_height);

   evas_object_smart_member_add(sd->scroll_object, object);
   evas_object_show(sd->scroll_object);

   o = edje_object_add(evas);
   sd->entry_object = o;
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/entry/text");
   edje_object_size_min_calc(sd->entry_object, &sd->min_width, &sd->height);
   evas_object_propagate_events_set(sd->entry_object, EINA_TRUE);

   e_scrollframe_child_set(sd->scroll_object, sd->entry_object);
   evas_object_show(o);

   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_DOWN,
         _e_entry_mouse_down_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "selection,changed", ENTRY_PART_NAME,
         _entry_selection_changed_signal_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "entry,selection,all,request", ENTRY_PART_NAME,
         _entry_selection_all_signal_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "entry,changed", ENTRY_PART_NAME,
         _entry_changed_signal_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "entry,paste,request,*", ENTRY_PART_NAME,
         _entry_paste_request_signal_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "entry,copy,notify", ENTRY_PART_NAME,
         _entry_copy_notify_signal_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "entry,cut,notify", ENTRY_PART_NAME,
         _entry_cut_notify_signal_cb, object);
   edje_object_signal_callback_add(sd->entry_object,
         "cursor,changed", ENTRY_PART_NAME,
         _entry_cursor_changed_signal_cb, object);

   _entry_recalc_size(object);

   sd->selection_handler =
     ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY,
                             _e_entry_x_selection_notify_handler, object);
}

static void
_e_entry_smart_del(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_event_callback_del_full(object, EVAS_CALLBACK_MOUSE_DOWN,
         _e_entry_mouse_down_cb, object);

   edje_object_signal_callback_del_full(sd->entry_object,
         "selection,changed", ENTRY_PART_NAME,
         _entry_selection_changed_signal_cb, object);
   edje_object_signal_callback_del_full(sd->entry_object,
         "entry,selection,all,request", ENTRY_PART_NAME,
         _entry_selection_all_signal_cb, object);
   edje_object_signal_callback_del_full(sd->entry_object,
         "entry,changed", ENTRY_PART_NAME,
         _entry_changed_signal_cb, object);
   edje_object_signal_callback_del_full(sd->entry_object,
         "entry,paste,request,*", ENTRY_PART_NAME,
         _entry_paste_request_signal_cb, object);
   edje_object_signal_callback_del_full(sd->entry_object,
         "entry,copy,notify", ENTRY_PART_NAME,
         _entry_copy_notify_signal_cb, object);
   edje_object_signal_callback_del_full(sd->entry_object,
         "entry,cut,notify", ENTRY_PART_NAME,
         _entry_cut_notify_signal_cb, object);
   edje_object_signal_callback_del_full(sd->entry_object,
         "cursor,changed", ENTRY_PART_NAME,
         _entry_cursor_changed_signal_cb, object);
   if (sd->selection_handler)
     ecore_event_handler_del(sd->selection_handler);
   evas_object_del(sd->entry_object);
   free(sd);
}

static void
_e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_move(sd->scroll_object, x, y);
}

static void
_e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_resize(sd->scroll_object, w, h);

   _entry_recalc_size(object);
}

static void
_e_entry_smart_show(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_show(sd->scroll_object);
}

static void
_e_entry_smart_hide(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_hide(sd->scroll_object);
}

static void
_e_entry_color_set(Evas_Object *object, int r, int g, int b, int a)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_color_set(sd->scroll_object, r, g, b, a);
}

static void
_e_entry_clip_set(Evas_Object *object, Evas_Object *clip)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_clip_set(sd->scroll_object, clip);
}

static void
_e_entry_clip_unset(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_clip_unset(sd->scroll_object);
}

static void
_e_entry_cb_menu_post(void *data, E_Menu *m __UNUSED__)
{
   E_Entry_Smart_Data *sd = data;

   if (!sd) return;
   if (!sd->popup) return;
   e_object_del(E_OBJECT(sd->popup));
   sd->popup = NULL;
}

static void
_e_entry_cb_cut(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Entry_Smart_Data *sd = data;
   if (!sd) return;
   _e_entry_cb_copy(sd, NULL, NULL);
   if ((!sd->enabled) || (sd->noedit)) return;
   edje_object_part_text_user_insert(sd->entry_object, ENTRY_PART_NAME, "");
}

static void
_e_entry_cb_copy(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Entry_Smart_Data *sd = data;
   const char *range;
   Ecore_X_Window xwin;
   E_Win *win;

   if (!sd) return;
   range = edje_object_part_text_selection_get(sd->entry_object, ENTRY_PART_NAME);
   if (range)
     {
        win = e_win_evas_object_win_get(sd->entry_object);
        if (win) xwin = win->evas_win;
        else
          {
             E_Container *con;

             con = e_container_evas_object_container_get(sd->entry_object);
             if (!con) return;
             xwin = ecore_evas_window_get(con->bg_ecore_evas);
          }
        ecore_x_selection_clipboard_set(xwin, range, strlen(range) + 1);
     }
}

static void
_e_entry_cb_paste(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Entry_Smart_Data *sd;
   Ecore_X_Window xwin;
   E_Win *win;

   sd = data;
   if (!sd) return;
   if ((!sd->enabled) || (sd->noedit)) return;

   win = e_win_evas_object_win_get(sd->entry_object);
   if (win) xwin = win->evas_win;
   else
     {
        E_Container *con;

        con = e_container_evas_object_container_get(sd->entry_object);
        if (!con) return;
        xwin = ecore_evas_window_get(con->bg_ecore_evas);
     }
   ecore_x_selection_clipboard_request(xwin, ECORE_X_SELECTION_TARGET_UTF8_STRING);
}

static void
_e_entry_cb_select_all(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Entry_Smart_Data *sd;

   sd = data;
   if (!sd) return;
   edje_object_part_text_select_all(sd->entry_object, ENTRY_PART_NAME);
   _e_entry_x_selection_update(sd->entry_object);
}

static void
_e_entry_cb_delete(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Entry_Smart_Data *sd;

   sd = data;
   if (!sd) return;
   if ((!sd->enabled) || (sd->noedit)) return;
   edje_object_part_text_user_insert(sd->entry_object, ENTRY_PART_NAME, "");
}

