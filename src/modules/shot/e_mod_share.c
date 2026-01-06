#include "e_mod_main.h"

static E_Confirm_Dialog *cd = NULL;
static Ecore_Exe        *img_write_exe = NULL;
static Evas_Object      *o_label = NULL;
static Evas_Object      *o_entry = NULL;
static Eina_List        *handlers = NULL;
static char             *url_ret = NULL;
static const char       *cnp_file = NULL;
static Eina_Bool         cnp = EINA_FALSE;

static Evas_Object * win_cp = NULL;

// clean up and be done
static void
_share_done(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
   free(url_ret);
   o_label = NULL;
   img_write_exe = NULL;
   url_ret = NULL;
   preview_abort();
}

static void
_cnp_thread_io(void *data)
{
   char *file = data;
   unsigned char *fdata = NULL;
   ssize_t fsize = 0;
   FILE *f = fopen(file, "r");
   
   win_cp = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
    
   if (!f) goto err;
   fseek(f, 0, SEEK_END);
   fsize = ftell(f);
   fseek(f, 0, SEEK_SET);

   if (fsize > 0)
     {
        fdata = malloc(fsize);
        if (fdata)
          {
             if (fread(fdata, fsize, 1, f) == 1)
               {
                  elm_cnp_selection_set(win_cp,
                                        ELM_SEL_TYPE_CLIPBOARD,
                                        ELM_SEL_FORMAT_IMAGE,
                                        fdata, fsize);
               }
             free(fdata);
          }
     }
   fclose(f);
   eina_file_unlink(file);
err:
   free(file);
}

static void
_cnp_file(const char *file)
{
   //~ ecore_thread_run(_cnp_thread_io, NULL, NULL, strdup(file));
   _cnp_thread_io(strdup(file));
}

// the upload dialog
static void
_upload_ok_cb(void *data __UNUSED__, E_Dialog *dia)
{
   // ok just hides dialog and does background upload
   o_label = NULL;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (!win) return;
   E_FREE_FUNC(win, evas_object_del);
}

static void
_upload_cancel_cb(void *data __UNUSED__, E_Dialog *dia)
{
   o_label = NULL;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   E_FREE_FUNC(win, evas_object_del);
   _share_done();
}

static Eina_Bool
_img_write_end_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev = event;

   if (ev->exe != img_write_exe) return EINA_TRUE;
   _share_done();
   if ((cnp) && (cnp_file))
     {
        _cnp_file(cnp_file);
        eina_stringshare_replace(&cnp_file, NULL);
        cnp = EINA_FALSE;
     }
   return EINA_FALSE;
}

static char *
imgur_link_from_string(const char *s)
{
   const char *p, *q;
   char *out;
   int len;

   p = strstr(s, "\"link\":\"");
   if (!p) return NULL;
   p += 8;

   q = strchr(p, '"');
   if (!q) return NULL;

   len = q - p;
   out = malloc(len + 1);
   if (!out) return NULL;

   memcpy(out, p, len);
   out[len] = '\0';

   return out;
}

static Eina_Bool
_img_write_out_cb(void *data, int ev_type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;
   int i;

   if (ev->exe != img_write_exe) return EINA_TRUE;
   if (!((ev->lines) && (ev->lines[0].line))) goto done;
   for (i = 0; ev->lines[i].line; i++)
     {
        const char *l = ev->lines[i].line;

        if ((l[0] == 'U') && (l[1] == ' '))
          {
             int v = atoi(l + 2);
             if ((v >= 0) && (v <= 1000))
               {
                  char buf[128];
                  // update gui...
                  snprintf(buf, sizeof(buf), _("Uploaded %i%%"), (v * 100) / 1000);
                  e_widget_label_text_set(o_label, buf);
               }
          }
        else if ((l[0] == 'R') && (l[1] == ' '))
          {
             const char *r = l + 2;
             // finished - got final url
             if (!url_ret) url_ret = strdup(r);
          }
        else if ((l[0] == 'E') && (l[1] == ' '))
          {
             int err = atoi(l + 2);
             if (data) e_widget_disabled_set(data, 1);
             e_util_dialog_show(_("Error - Upload Failed"),
                                _("Upload failed with status code:<ps/>%i"),
                                err);
             _share_done();
             break;
          }
        else if ((l[0] == 'O'))
          {
             if (data) e_widget_disabled_set(data, 1);
             if ((o_entry) && (url_ret))
               {
                 char *link = imgur_link_from_string(url_ret);
                 e_widget_entry_text_set(o_entry, link);
                 free(link);
                 _share_done();
               }
             break;
          }
     }
done:
   return EINA_FALSE;
}

void
share_save(const char *cmd, const char *file, Eina_Bool copy)
{
   if (copy)
     {
        eina_stringshare_replace(&cnp_file, file);
        cnp = copy;
     }
   share_write_end_watch(NULL);
   img_write_exe = ecore_exe_pipe_run
     (cmd, ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED |
      ECORE_EXE_NOT_LEADER | ECORE_EXE_TERM_WITH_PARENT, NULL);
}

void
share_write_end_watch(void *data)
{
   E_LIST_HANDLER_APPEND(handlers, ECORE_EXE_EVENT_DEL,
                         _img_write_end_cb, data);
}

void
share_write_status_watch(void *data)
{
   E_LIST_HANDLER_APPEND(handlers, ECORE_EXE_EVENT_DATA,
                         _img_write_out_cb, data);
}

static void
_win_share_del(void *data __UNUSED__)
{
   if (handlers)
     ecore_event_handler_data_set(eina_list_last_data_get(handlers), NULL);
   _upload_cancel_cb(NULL, NULL);
   if (cd) e_object_del(E_OBJECT(cd));
}

void
share_dialog_show(void)
{
   E_Dialog *dia;
   Evas_Object *o, *ol;
   Evas_Coord mw, mh;

   E_FREE_LIST(handlers, ecore_event_handler_del);

   save_to(NULL, EINA_FALSE);

   E_FREE_FUNC(win, evas_object_del);

   dia = e_dialog_new(NULL, "E", "_e_shot_share");
   e_dialog_resizable_set(dia, EINA_TRUE);
   e_dialog_title_set(dia, _("Uploading screenshot"));

   Evas *evas = e_win_evas_get(dia->win);
   o = e_widget_list_add(evas_object_evas_get(evas), 0, 0);
   ol = o;

   o = e_widget_label_add(evas_object_evas_get(evas), _("Uploading ..."));
   o_label = o;
   e_widget_list_object_append(ol, o, 0, 0, 0.5);

   o = e_widget_label_add(evas_object_evas_get(evas),
                          _("Screenshot is available at this location:"));
   e_widget_list_object_append(ol, o, 0, 0, 0.5);

   o = e_widget_entry_add(evas, NULL, NULL, NULL, NULL);
   o_entry = o;
   e_widget_list_object_append(ol, o, 1, 0, 0.5);

   e_widget_size_min_get(ol, &mw, &mh);
   e_dialog_content_set(dia, ol, mw, mh);
   e_dialog_button_add(dia, _("Hide"), NULL, _upload_ok_cb, NULL);
   e_dialog_button_add(dia, _("Cancel"), NULL, _upload_cancel_cb, NULL);
   e_object_del_attach_func_set(E_OBJECT(dia), _win_share_del);
   share_write_status_watch(eina_list_last_data_get(dia->buttons));
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

// confirm dialog that it's ok to share
static void
_win_share_confirm_del(void *d __UNUSED__)
{
   cd = NULL;
}

static void
_win_share_confirm_yes(void *d __UNUSED__)
{
   share_dialog_show();
}

void
share_confirm(void)
{
   if (cd) return;
   cd = e_confirm_dialog_show
     (_("Confirm Share"), NULL,
      _("Are you sure you want to upload this image<ps/>"
        "publically to imgur.com?"),
      _("Confirm"), _("Cancel"),
      _win_share_confirm_yes, NULL,
      NULL, NULL, _win_share_confirm_del, NULL);
}

Eina_Bool
share_have(void)
{
   if (img_write_exe) return EINA_TRUE;
   return EINA_FALSE;
}

void
share_abort(void)
{
   E_FREE_FUNC(cd, e_object_del);
   E_FREE_FUNC(win, evas_object_del);
}
