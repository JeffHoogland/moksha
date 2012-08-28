#include "e.h"

static void
_fsel_path_save(E_Import_Dialog *id)
{
   const char *fdev = NULL, *fpath = NULL;

   e_widget_fsel_path_get(id->fsel_obj, &fdev, &fpath);
   if ((fdev) || (fpath))
     {
        eina_stringshare_replace(&e_config->wallpaper_import_last_dev, fdev);
        eina_stringshare_replace(&e_config->wallpaper_import_last_path, fpath);
        e_config_save_queue();
     }
}

static void
_fsel_cb_close(void *data, E_Dialog *dia __UNUSED__)
{
   E_Import_Dialog *id = data;

   e_object_ref(data);
   if (id->cancel) id->cancel(id);
   e_object_del(data);
   e_object_unref(data);
}

static void
_import_ok(void *data, void *data2)
{
   E_Import_Dialog *id;

   id = e_object_data_get(data2);
   e_object_ref(E_OBJECT(id));
   if (id->ok) id->ok(data, id);
   e_object_del(E_OBJECT(id));
   e_object_unref(E_OBJECT(id));
}

static void
_fsel_cb_ok(void *data, E_Dialog *dia __UNUSED__)
{
   E_Import_Dialog *id;
   const char *path, *p;
   int is_bg, is_theme, r;
   const char *file;
   char buf[PATH_MAX];

   id = data;
   path = e_widget_fsel_selection_path_get(id->fsel_obj);
   if (!path) return;

   p = strrchr(path, '.');
   if ((p) && (strcasecmp(p, ".edj")))
     {
        E_Import_Config_Dialog *import;
        import = e_import_config_dialog_show(id->dia->win->container, path, _import_ok, NULL);
        e_dialog_parent_set(import->dia, id->dia->win);
        e_object_data_set(E_OBJECT(import), id);
        return;
     }
   r = 0;
   file = ecore_file_file_get(path);
   e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s", file);

   is_bg = edje_file_group_exists(path, "e/desktop/background");
   is_theme =
     edje_file_group_exists(path, "e/widgets/border/default/border");

   if ((is_bg) && (!is_theme))
     {
        if (!ecore_file_cp(path, buf))
          {
             e_util_dialog_show(_("Import Error"),
                                _("Enlightenment was unable to "
                                  "import the image<br>due to a "
                                  "copy error."));
          }
        else
          r = 1;
     }
   else
     {
        e_util_dialog_show(_("Import Error"),
                           _("Enlightenment was unable to "
                             "import the image.<br><br>"
                             "Are you sure this is a valid "
                             "image?"));
     }

   if (r)
     {
        e_object_ref(E_OBJECT(id));
        if (id->ok) id->ok(buf, id);
        e_object_del(E_OBJECT(id));
        e_object_unref(E_OBJECT(id));
     }
   else
     _fsel_cb_close(id, NULL);
}


static void
_e_import_dia_del(void *data)
{
   E_Dialog *dia = data;

   e_object_del(dia->data);
}

static void
_e_import_dialog_del(void *data)
{
   E_Import_Dialog *id = data;

   _fsel_path_save(id);
   e_object_del(E_OBJECT(id->dia));
   free(id);
}

static void
_e_import_dialog_win_del(E_Win *win)
{
   E_Dialog *dia;
   E_Import_Dialog *id;

   dia = win->data;
   id = dia->data;
   e_object_del(E_OBJECT(id));
}

//////////////////////////////////////////////////////////////////////////////////

EAPI E_Import_Dialog *
e_import_dialog_show(E_Container *con, const char *dev, const char *path, Ecore_End_Cb ok, Ecore_Cb cancel)
{
   Evas *evas;
   E_Import_Dialog *id;
   Evas_Object *ofm;
   int w, h;
   const char *fdev, *fpath;
   char buf[PATH_MAX];
   E_Dialog *dia;

   id = E_OBJECT_ALLOC(E_Import_Dialog, E_IMPORT_DIALOG_TYPE, _e_import_dialog_del);
   if (!id) return NULL;

   dia = e_dialog_new(con, "E", "_import_fsel_dialog");
   dia->data = id;
   id->dia = dia;
   id->ok = ok, id->cancel = cancel;
   e_object_del_attach_func_set(E_OBJECT(dia), _e_import_dia_del);
   e_win_delete_callback_set(dia->win, _e_import_dialog_win_del);

   evas = e_win_evas_get(dia->win);
   e_dialog_title_set(dia, _("Select a Picture..."));

   fdev = dev ?: e_config->wallpaper_import_last_dev;
   fpath = path ?: e_config->wallpaper_import_last_path;
   if (fdev)
     snprintf(buf, sizeof(buf), "%s/%s",
              fdev, path);
   else
     snprintf(buf, sizeof(buf), "%s", path);

   if (!ecore_file_exists(ecore_file_realpath(buf)))
     fpath = "/";
   else
     fpath = path ?: e_config->wallpaper_import_last_path;

   if ((!fdev) && (!fpath))
     {
        fdev = "~/";
        fpath = "/";
     }

   //printf("LAST: [%s] '%s' '%s'\n", buf, fdev, fpath);
   ofm = e_widget_fsel_add(evas, fdev, fpath, NULL, NULL, NULL, NULL,
                           NULL, NULL, 1);
   e_widget_fsel_window_object_set(ofm, E_OBJECT(dia->win));
   id->fsel_obj = ofm;
   e_widget_size_min_get(ofm, &w, &h);
   e_dialog_content_set(dia, ofm, w, h);

   e_dialog_button_add(dia, _("Use"), NULL, _fsel_cb_ok, id);
   e_dialog_button_add(dia, _("Cancel"), NULL, _fsel_cb_close, id);
   e_dialog_border_icon_set(dia, "enlightenment/background");
   e_dialog_show(dia);
   e_win_centered_set(dia->win, 1);
   e_widget_focus_set(ofm, 1);

   return id;
}
