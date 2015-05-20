#include "e.h"
#include "e_mod_main.h"

typedef struct _Import Import;

struct _Import
{
   E_Config_Dialog      *parent;
   E_Config_Dialog_Data *cfdata;

   Evas_Object          *bg_obj;
   Evas_Object          *box_obj;
   Evas_Object          *content_obj;
   Evas_Object          *event_obj;
   Evas_Object          *fsel_obj;

   Evas_Object          *ok_obj;
   Evas_Object          *cancel_obj;

   E_Win                *win;
};

struct _E_Config_Dialog_Data
{
   char *file;
};

static void _theme_import_cb_delete(E_Win *win);
static void _theme_import_cb_resize(E_Win *win);
static void _theme_import_cb_wid_focus(void *data, Evas_Object *obj);
static void _theme_import_cb_selected(void *data, Evas_Object *obj);
static void _theme_import_cb_changed(void *data, Evas_Object *obj);
static void _theme_import_cb_ok(void *data, void *data2);
static void _theme_import_cb_close(void *data, void *data2);
static void _theme_import_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);

E_Win *
e_int_config_theme_import(E_Config_Dialog *parent)
{
   Evas *evas;
   E_Win *win;
   Evas_Object *o, *ofm;
   Import *import;
   E_Config_Dialog_Data *cfdata;
   Evas_Modifier_Mask mask;
   Evas_Coord w, h;
   Eina_Bool kg;

   import = E_NEW(Import, 1);
   if (!import) return NULL;

   win = e_win_new(parent->con);
   if (!win)
     {
        E_FREE(import);
        return NULL;
     }

   evas = e_win_evas_get(win);

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   import->cfdata = cfdata;
   import->win = win;
   import->parent = parent;

   e_win_title_set(win, _("Select a Theme..."));
   e_win_delete_callback_set(win, _theme_import_cb_delete);
   e_win_resize_callback_set(win, _theme_import_cb_resize);
   e_win_dialog_set(win, 1);
   e_win_name_class_set(win, "E", "_theme_import_dialog");

   o = edje_object_add(evas);
   import->bg_obj = o;
   e_theme_edje_object_set(o, "base/theme/dialog", "e/widgets/dialog/main");
   evas_object_move(o, 0, 0);
   evas_object_show(o);

   o = e_widget_list_add(evas, 1, 1);
   e_widget_on_focus_hook_set(o, _theme_import_cb_wid_focus, import);
   import->box_obj = o;
   edje_object_part_swallow(import->bg_obj, "e.swallow.buttons", o);

   o = evas_object_rectangle_add(evas);
   import->event_obj = o;
   mask = 0;
   kg = evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: Unable to redirect \"Tab\" key events to object %p.\n", o);
   mask = evas_key_modifier_mask_get(evas, "Shift");
   kg = evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"Tab\" key events to object %p.\n", o);
   mask = 0;
   kg = evas_object_key_grab(o, "Return", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"Return\" key events to object %p.\n", o);
   mask = 0;
   kg = evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0);
   if (!kg)
     fprintf(stderr, "ERROR: unable to redirect \"KP_Enter\" key events to object %p.\n", o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN,
                                  _theme_import_cb_key_down, import);

   o = e_widget_list_add(evas, 0, 0);
   import->content_obj = o;

   ofm = e_widget_fsel_add(evas, e_user_homedir_get(), "/",
                           NULL, NULL,
                           _theme_import_cb_selected, import,
                           _theme_import_cb_changed, import, 1);
   import->fsel_obj = ofm;
   e_widget_list_object_append(o, ofm, 1, 1, 0.5);

   e_widget_size_min_get(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   edje_object_part_swallow(import->bg_obj, "e.swallow.content", o);
   evas_object_show(o);

   import->ok_obj = e_widget_button_add(evas, _("OK"), NULL,
                                        _theme_import_cb_ok, win, cfdata);
   e_widget_list_object_append(import->box_obj, import->ok_obj, 1, 0, 0.5);

   import->cancel_obj = e_widget_button_add(evas, _("Cancel"), NULL,
                                            _theme_import_cb_close,
                                            win, cfdata);
   e_widget_list_object_append(import->box_obj, import->cancel_obj, 1, 0, 0.5);

   e_widget_disabled_set(import->ok_obj, 1);

   e_win_centered_set(win, 1);

   o = import->box_obj;
   e_widget_size_min_get(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   edje_object_part_swallow(import->bg_obj, "e.swallow.buttons", o);

   edje_object_size_min_calc(import->bg_obj, &w, &h);
   evas_object_resize(import->bg_obj, w, h);
   e_win_resize(win, w, h);
   e_win_size_min_set(win, w, h);
   e_win_size_max_set(win, 99999, 99999);
   e_win_show(win);
   e_win_border_icon_set(win, "preferences-desktop-theme");

   win->data = import;

   return win;
}

void
e_int_config_theme_del(E_Win *win)
{
   Import *import;

   import = win->data;

   e_object_del(E_OBJECT(import->win));
   e_int_config_theme_import_done(import->parent);

   E_FREE(import->cfdata->file);
   E_FREE(import->cfdata);
   E_FREE(import);
   return;
}

static void
_theme_import_cb_delete(E_Win *win)
{
   e_int_config_theme_del(win);
}

static void
_theme_import_cb_resize(E_Win *win)
{
   Import *import;

   import = win->data;
   if (!import) return;
   evas_object_resize(import->bg_obj, win->w, win->h);
}

static void
_theme_import_cb_wid_focus(void *data, Evas_Object *obj)
{
   Import *import;

   import = data;
   if (obj == import->content_obj)
     e_widget_focused_object_clear(import->box_obj);
   else if (import->content_obj)
     e_widget_focused_object_clear(import->content_obj);
}

static void
_theme_import_cb_selected(void *data, Evas_Object *obj __UNUSED__)
{
   Import *import;

   import = data;
   _theme_import_cb_ok(import->win, NULL);
}

static void
_theme_import_cb_changed(void *data, Evas_Object *obj __UNUSED__)
{
   Import *import;
   const char *path;
   const char *file;

   import = data;
   if (!import) return;
   if (!import->fsel_obj) return;

   path = e_widget_fsel_selection_path_get(import->fsel_obj);
   E_FREE(import->cfdata->file);
   if (path)
     import->cfdata->file = strdup(path);

   if (import->cfdata->file)
     {
        char *strip;

        file = ecore_file_file_get(import->cfdata->file);
        strip = ecore_file_strip_ext(file);
        if (!strip)
          {
             E_FREE(import->cfdata->file);
             e_widget_disabled_set(import->ok_obj, 1);
             return;
          }
        free(strip);
        if (!e_util_glob_case_match(file, "*.edj"))
          {
             E_FREE(import->cfdata->file);
             e_widget_disabled_set(import->ok_obj, 1);
             return;
          }
        e_widget_disabled_set(import->ok_obj, 0);
     }
   else
     e_widget_disabled_set(import->ok_obj, 1);
}

static void
_theme_import_cb_ok(void *data, void *data2 __UNUSED__)
{
   Import *import;
   E_Win *win;
   const char *path;
   const char *file;
   char buf[4096];

   win = data;
   import = win->data;
   if (!import) return;

   path = e_widget_fsel_selection_path_get(import->fsel_obj);
   E_FREE(import->cfdata->file);
   if (path)
     import->cfdata->file = strdup(path);

   if (import->cfdata->file)
     {
        char *strip;

        file = ecore_file_file_get(import->cfdata->file);
        e_user_dir_snprintf(buf, sizeof(buf), "themes/%s", file);

        if (ecore_file_exists(buf))
          ecore_file_unlink(buf);

        strip = ecore_file_strip_ext(file);
        if (!strip)
          return;
        free(strip);

        if (!e_util_glob_case_match(file, "*.edj"))
          return;

        if (!edje_file_group_exists(import->cfdata->file,
                                    "e/widgets/border/default/border"))
          {
             e_util_dialog_show(_("Theme Import Error"),
                                _("Enlightenment was unable to import "
                                  "the theme.<br><br>Are you sure this "
                                  "is really a valid theme?"));
          }
        else
          {
             if (!ecore_file_cp(import->cfdata->file, buf))
               {
                  e_util_dialog_show(_("Theme Import Error"),
                                     _("Enlightenment was unable to import "
                                       "the theme<br>due to a copy error."));
               }
             else
               e_int_config_theme_update(import->parent, buf);
          }
     }

   e_int_config_theme_del(import->win);
}

static void
_theme_import_cb_close(void *data, void *data2 __UNUSED__)
{
   E_Win *win;

   win = data;
   e_int_config_theme_del(win);
}

static void
_theme_import_cb_key_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev;
   Import *import;

   ev = event;
   import = data;
   if ((!e_widget_fsel_typebuf_visible_get(import->fsel_obj)) && (!strcmp(ev->key, "Tab")))
     {
        if (evas_key_modifier_is_set(evas_key_modifier_get(e_win_evas_get(import->win)), "Shift"))
          {
             if (e_widget_focus_get(import->box_obj))
               {
                  if (!e_widget_focus_jump(import->box_obj, 0))
                    {
                       e_widget_focus_set(import->content_obj, 0);
                       if (!e_widget_focus_get(import->content_obj))
                         e_widget_focus_set(import->box_obj, 0);
                    }
               }
             else
               {
                  if (!e_widget_focus_jump(import->content_obj, 0))
                    e_widget_focus_set(import->box_obj, 0);
               }
          }
        else
          {
             if (e_widget_focus_get(import->box_obj))
               {
                  if (!e_widget_focus_jump(import->box_obj, 1))
                    {
                       e_widget_focus_set(import->content_obj, 1);
                       if (!e_widget_focus_get(import->content_obj))
                         e_widget_focus_set(import->box_obj, 1);
                    }
               }
             else
               {
                  if (!e_widget_focus_jump(import->content_obj, 1))
                    e_widget_focus_set(import->box_obj, 1);
               }
          }
     }
   else if (((!strcmp(ev->key, "Return")) ||
             (!strcmp(ev->key, "KP_Enter")) ||
             (!strcmp(ev->key, "space"))))
     {
        Evas_Object *o = NULL;

        if ((import->content_obj) && (e_widget_focus_get(import->content_obj)))
          o = e_widget_focused_object_get(import->content_obj);
        else
          o = e_widget_focused_object_get(import->box_obj);
        if (o) e_widget_activate(o);
     }
}

