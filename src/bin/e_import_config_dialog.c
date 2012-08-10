#include "e.h"

#define IMPORT_STRETCH          0
#define IMPORT_TILE             1
#define IMPORT_CENTER           2
#define IMPORT_SCALE_ASPECT_IN  3
#define IMPORT_SCALE_ASPECT_OUT 4
#define IMPORT_PAN              5

static void      _import_edj_gen(E_Import_Config_Dialog *import);
static Eina_Bool _import_cb_edje_cc_exit(void *data, int type, void *event);


static void
_import_edj_gen(E_Import_Config_Dialog *import)
{
   Evas *evas;
   Evas_Object *img;
   Eina_Bool anim = EINA_FALSE;
   int fd, num = 1;
   int w = 0, h = 0;
   const char *file, *locale;
   char buf[PATH_MAX], cmd[PATH_MAX], tmpn[PATH_MAX], ipart[PATH_MAX], enc[128];
   char *imgdir = NULL, *fstrip;
   int cr, cg, cb, ca;
   FILE *f;
   size_t len, off;

   evas = e_win_evas_get(import->dia->win);
   file = ecore_file_file_get(import->file);
   fstrip = ecore_file_strip_ext(file);
   if (!fstrip) return;
   len = e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s.edj", fstrip);
   if (len >= sizeof(buf)) return;
   off = len - (sizeof(".edj") - 1);
   for (num = 1; ecore_file_exists(buf) && num < 100; num++)
     snprintf(buf + off, sizeof(buf) - off, "-%d.edj", num);
   free(fstrip);
   cr = import->color.r;
   cg = import->color.g;
   cb = import->color.b;
   ca = import->color.a;

   if (num == 100)
     {
        printf("Couldn't come up with another filename for %s\n", buf);
        return;
     }

   strcpy(tmpn, "/tmp/e_bgdlg_new.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0)
     {
        printf("Error Creating tmp file: %s\n", strerror(errno));
        return;
     }

   f = fdopen(fd, "w");
   if (!f)
     {
        printf("Cannot open %s for writing\n", tmpn);
        return;
     }

   anim = eina_str_has_extension(import->file, "gif");
   imgdir = ecore_file_dir_get(import->file);
   if (!imgdir) ipart[0] = '\0';
   else
     {
        snprintf(ipart, sizeof(ipart), "-id %s", e_util_filename_escape(imgdir));
        free(imgdir);
     }

   img = evas_object_image_add(evas);
   evas_object_image_file_set(img, import->file, NULL);
   evas_object_image_size_get(img, &w, &h);
   evas_object_del(img);

   if (import->external)
     {
        fstrip = strdupa(e_util_filename_escape(import->file));
        snprintf(enc, sizeof(enc), "USER");
     }
   else
     {
        fstrip = strdupa(e_util_filename_escape(file));
        if (import->quality == 100)
          snprintf(enc, sizeof(enc), "COMP");
        else
          snprintf(enc, sizeof(enc), "LOSSY %i", import->quality);
     }
   switch (import->method)
     {
      case IMPORT_STRETCH:
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "%s"
                "data { item: \"style\" \"0\"; }\n"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "image { normal: \"%s\"; scale_hint: STATIC; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n", w, h, fstrip);
        break;

      case IMPORT_TILE:
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"1\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "image { normal: \"%s\"; }\n"
                "fill { size {\n"
                "relative: 0.0 0.0;\n"
                "offset: %i %i;\n"
                "} } } } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n", w, h, fstrip, w, h);
        break;

      case IMPORT_CENTER:
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"2\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "color: %i %i %i %i;\n"
                "} }\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "min: %i %i; max: %i %i;\n"
                "image { normal: \"%s\"; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n", w, h, cr, cg, cb, ca, w, h, w, h, fstrip);
        break;

      case IMPORT_SCALE_ASPECT_IN:
        locale = e_intl_language_get();
        setlocale(LC_NUMERIC, "C");
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"3\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "color: %i %i %i %i;\n"
                "} }\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "aspect: %1.9f %1.9f; aspect_preference: BOTH;\n"
                "image { normal: \"%s\";  scale_hint: STATIC; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n",
                w, h, cr, cg, cb, ca, (double)w / (double)h, (double)w / (double)h, fstrip);
        setlocale(LC_NUMERIC, locale);
        break;

      case IMPORT_SCALE_ASPECT_OUT:
        locale = e_intl_language_get();
        setlocale(LC_NUMERIC, "C");
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"4\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "aspect: %1.9f %1.9f; aspect_preference: NONE;\n"
                "image { normal: \"%s\";  scale_hint: STATIC; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n",
                w, h, (double)w / (double)h, (double)w / (double)h, fstrip);
        setlocale(LC_NUMERIC, locale);
        break;

      case IMPORT_PAN:
        locale = e_intl_language_get();
        setlocale(LC_NUMERIC, "C");
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"4\"; }\n"
                "%s"
                "max: %i %i;\n"
                "script {\n"
                "public cur_anim; public cur_x; public cur_y; public prev_x;\n"
                "public prev_y; public total_x; public total_y; \n"
                "public pan_bg(val, Float:v) {\n"
                "new Float:x, Float:y, Float:px, Float: py;\n"

                "px = get_float(prev_x); py = get_float(prev_y);\n"
                "if (get_int(total_x) > 1) {\n"
                "x = float(get_int(cur_x)) / (get_int(total_x) - 1);\n"
                "x = px - (px - x) * v;\n"
                "} else { x = 0.0; v = 1.0; }\n"
                "if (get_int(total_y) > 1) {\n"
                "y = float(get_int(cur_y)) / (get_int(total_y) - 1);\n"
                "y = py - (py - y) * v;\n"
                "} else { y = 0.0; v = 1.0; }\n"

                "set_state_val(PART:\"bg\", STATE_ALIGNMENT, x, y);\n"

                "if (v >= 1.0) {\n"
                "set_int(cur_anim, 0); set_float(prev_x, x);\n"
                "set_float(prev_y, y); return 0;\n"
                "}\n"
                "return 1;\n"
                "}\n"
                "public message(Msg_Type:type, id, ...) {\n"
                "if ((type == MSG_FLOAT_SET) && (id == 0)) {\n"
                "new ani;\n"

                "get_state_val(PART:\"bg\", STATE_ALIGNMENT, prev_x, prev_y);\n"
                "set_int(cur_x, round(getfarg(3))); set_int(total_x, round(getfarg(4)));\n"
                "set_int(cur_y, round(getfarg(5))); set_int(total_y, round(getfarg(6)));\n"

                "ani = get_int(cur_anim); if (ani > 0) cancel_anim(ani);\n"
                "ani = anim(getfarg(2), \"pan_bg\", 0); set_int(cur_anim, ani);\n"
                "} } }\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "aspect: %1.9f %1.9f; aspect_preference: NONE;\n"
                "image { normal: \"%s\";  scale_hint: STATIC; }\n"
                "} } }\n"
                "programs { program {\n"
                " name: \"init\";\n"
                " signal: \"load\";\n"
                " source: \"\";\n"
                " script { custom_state(PART:\"bg\", \"default\", 0.0);\n"
                " set_state(PART:\"bg\", \"custom\", 0.0);\n"
                " set_float(prev_x, 0.0); set_float(prev_y, 0.0);\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n",
                w, h, (double)w / (double)h, (double)w / (double)h, fstrip);
        setlocale(LC_NUMERIC, locale);
        break;

      default:
        /* won't happen */
        break;
     }

   fclose(f);

   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s %s",
            ipart, tmpn, e_util_filename_escape(buf));

   import->tmpf = strdup(tmpn);
   import->fdest = eina_stringshare_add(buf);
   import->exe_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                             _import_cb_edje_cc_exit, import);
   import->exe = ecore_exe_run(cmd, import);
}

static Eina_Bool
_import_cb_edje_cc_exit(void *data, __UNUSED__ int type, void *event)
{
   E_Import_Config_Dialog *import;
   Ecore_Exe_Event_Del *ev;
   int r = 1;

   ev = event;
   import = data;
   if (ecore_exe_data_get(ev->exe) != import) return ECORE_CALLBACK_PASS_ON;

   if (ev->exit_code != 0)
     {
        e_util_dialog_show(_("Picture E_Import_Config_Dialog Error"),
                           _("Enlightenment was unable to import the picture<br>"
                             "due to conversion errors."));
        r = 0;
     }

   if (r && import->ok)
     {
        e_object_ref(E_OBJECT(import));
        import->ok((void*)import->fdest, import);
        e_object_del(E_OBJECT(import));
        e_object_unref(E_OBJECT(import));
     }
   else
     e_object_del(E_OBJECT(import));

   return ECORE_CALLBACK_DONE;
}

static void
_import_cb_close(void *data, E_Dialog *dia __UNUSED__)
{
   E_Import_Config_Dialog *import = data;

   e_object_ref(data);
   if (import->cancel) import->cancel(import);
   e_object_del(data);
   e_object_unref(data);
}

static void
_import_cb_ok(void *data, E_Dialog *dia __UNUSED__)
{
   E_Import_Config_Dialog *import = data;
   const char *file;
   char buf[PATH_MAX];
   int is_bg, is_theme, r;

   r = 0;
   if (!import->file) return;
   file = ecore_file_file_get(import->file);
   if (!eina_str_has_extension(file, "edj"))
     {
        _import_edj_gen(import);
        e_win_hide(import->dia->win);
        return;
     }
   e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s", file);

   is_bg = edje_file_group_exists(import->file, "e/desktop/background");
   is_theme = edje_file_group_exists(import->file,
                                     "e/widgets/border/default/border");

   if ((is_bg) && (!is_theme))
     {
        if (!ecore_file_cp(import->file, buf))
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
        e_object_ref(E_OBJECT(import));
        if (import->ok) import->ok((void*)buf, import);
        e_object_del(E_OBJECT(import));
        e_object_unref(E_OBJECT(import));
     }
   else
     _import_cb_close(import, NULL);
}

static void
_e_import_config_preview_size_get(int size, int w, int h,int *tw, int *th)
{
   if (size <= 0) return;
   double aspect;
   aspect = (double)w/h;
   
   if(w > size)
     {
        w = size;
        h = (w/aspect);
     }
   *tw = w;
   *th = h;
}

static void
_e_import_config_dia_del(void *data)
{
   E_Dialog *dia = data;

   e_object_del(dia->data);
}

static void
_e_import_config_dialog_del(void *data)
{
   E_Import_Config_Dialog *import = data;

   if (import->exe_handler) ecore_event_handler_del(import->exe_handler);
   import->exe_handler = NULL;
   if (import->tmpf) unlink(import->tmpf);
   free(import->tmpf);
   eina_stringshare_del(import->fdest);
   import->exe = NULL;
   eina_stringshare_del(import->file);
   e_object_del(E_OBJECT(import->dia));
   free(import);
}

static void
_e_import_config_dialog_win_del(E_Win *win)
{
   E_Dialog *dia;
   E_Import_Config_Dialog *import;

   dia = win->data;
   import = dia->data;
   e_object_ref(E_OBJECT(import));
   if (import->cancel) import->cancel(import);
   e_object_del(E_OBJECT(import));
   e_object_unref(E_OBJECT(import));
}
///////////////////////////////////////////////////////////////////////////////////


EAPI E_Import_Config_Dialog *
e_import_config_dialog_show(E_Container *con, const char *path, Ecore_End_Cb ok, Ecore_Cb cancel)
{
   Evas *evas;
   E_Dialog *dia;
   E_Import_Config_Dialog *import;
   Evas_Object *o, *of, *ord, *ot, *ol, *preview, *frame;
   E_Radio_Group *rg;
   int w, h, tw, th;

   if (!path) return NULL;

   import = E_OBJECT_ALLOC(E_Import_Config_Dialog, E_IMPORT_CONFIG_DIALOG_TYPE, _e_import_config_dialog_del);
   if (!import) return NULL;

   dia = e_dialog_new(con, "E", "_import_config_dialog");
   e_dialog_title_set(dia, _("Import Settings..."));
   dia->data = import;
   import->dia = dia;
   import->ok = ok, import->cancel = cancel;
   import->path = eina_stringshare_add(path);
   e_object_del_attach_func_set(E_OBJECT(dia), _e_import_config_dia_del);
   e_win_delete_callback_set(dia->win, _e_import_config_dialog_win_del);

   evas = e_win_evas_get(dia->win);

   import->method = IMPORT_SCALE_ASPECT_OUT;
   import->external = 0;
   import->quality = 90;
   import->file = eina_stringshare_add(path);

   evas = e_win_evas_get(dia->win);

   o = e_widget_list_add(evas, 0, 0);

   ot = e_widget_list_add(evas, 0, 0);
   frame = e_widget_frametable_add(evas, _("Preview"), 1);
   
   preview = evas_object_image_add(evas);
   evas_object_image_file_set(preview, path, NULL);
   evas_object_image_size_get(preview,&w, &h);
   evas_object_del(preview);

   _e_import_config_preview_size_get(320, w, h, &tw, &th);
   
   preview = e_widget_preview_add(evas, tw, th);
   e_widget_preview_thumb_set(preview, path, NULL, tw, th);

   e_widget_frametable_object_append(frame, preview, 0, 0, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(ot, frame, 1, 1, 0);
   of = e_widget_frametable_add(evas, _("Fill and Stretch Options"), 1);
   rg = e_widget_radio_group_new(&import->method);
   ord = e_widget_radio_icon_add(evas, _("Stretch"),
                                 "enlightenment/wallpaper_stretch",
                                 24, 24, IMPORT_STRETCH, rg);
   e_widget_frametable_object_append(of, ord, 0, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Center"),
                                 "enlightenment/wallpaper_center",
                                 24, 24, IMPORT_CENTER, rg);
   e_widget_frametable_object_append(of, ord, 1, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Tile"),
                                 "enlightenment/wallpaper_tile",
                                 24, 24, IMPORT_TILE, rg);
   e_widget_frametable_object_append(of, ord, 2, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Within"),
                                 "enlightenment/wallpaper_scale_aspect_in",
                                 24, 24, IMPORT_SCALE_ASPECT_IN, rg);
   e_widget_frametable_object_append(of, ord, 3, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Fill"),
                                 "enlightenment/wallpaper_scale_aspect_out",
                                 24, 24, IMPORT_SCALE_ASPECT_OUT, rg);
   e_widget_frametable_object_append(of, ord, 4, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Pan"),
                                 "enlightenment/wallpaper_pan",
                                 24, 24, IMPORT_PAN, rg);
   e_widget_frametable_object_append(of, ord, 5, 0, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ot, of, 1, 1, 0);

   ol = e_widget_list_add(evas, 0, 1);

   of = e_widget_frametable_add(evas, _("File Quality"), 0);
   ord = e_widget_check_add(evas, _("Use original file"), &(import->external));
   e_widget_frametable_object_append(of, ord, 0, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_slider_add(evas, 1, 0, _("%3.0f%%"), 0.0, 100.0, 1.0, 0,
                             NULL, &(import->quality), 150);
   e_widget_frametable_object_append(of, ord, 0, 1, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0);

   of = e_widget_framelist_add(evas, _("Fill Color"), 0);
   ord = e_widget_color_well_add(evas, &import->color, 1);
   e_widget_framelist_object_append(of, ord);
   e_widget_list_object_append(ol, of, 1, 0, 1);
   e_widget_list_object_append(ot, ol, 1, 1, 0);

   e_widget_list_object_append(o, ot, 0, 0, 0.5);

   e_widget_size_min_get(o, &w, &h);
   e_dialog_content_set(dia, o, w, h);
   e_dialog_button_add(dia, _("OK"), NULL, _import_cb_ok, import);
   e_dialog_button_add(dia, _("Cancel"), NULL, _import_cb_close, import);
   e_win_centered_set(dia->win, 1);
   e_dialog_border_icon_set(dia, "folder-image");
   e_dialog_button_focus_num(dia, 0);
   e_dialog_show(dia);

   return import;
}
