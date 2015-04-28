#include "e.h"

static Ecore_Con_Url *url_up = NULL;
static Eina_List     *handlers = NULL;
static Ecore_Timer   *update_timer = NULL;
static E_Dialog      *dialog = NULL;
static char          *machid = NULL;

static void
_update_done(void)
{
   if (url_up)
     {
        ecore_con_url_free(url_up);
        url_up = NULL;
     }
}

static void
_delete_cb(void *obj __UNUSED__)
{
   dialog = NULL;
}

static void
_ok_cb(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(dialog));
   if (e_config->update.later > 0)
     {
        e_config->update.later = 0;
        e_config_save_queue();
     }
}

static void
_bother_me_later_cb(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(dialog));
   // 5 * 5 * 1hr === about 1 day limit, so bother-me later will wait
   // a day in between botherings. botherings reset on e start or restart
   if (e_config->update.later < 5)
     {
        e_config->update.later++;
        e_config_save_queue();
     }
}

static void
_never_tell_me_cb(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   if (update_timer) ecore_timer_del(update_timer);
   update_timer = NULL;
   e_object_del(E_OBJECT(dialog));
   e_config->update.check = 0;
   e_config->update.later = 0;
   e_config_save_queue();
}

static void
_new_version(const char *ver)
{
   E_Manager *man;
   E_Container *con;
   char text[2048];

   if (dialog) return;
   man = e_manager_current_get();
   if (!man) return;
   con = e_container_current_get(man);
   if (!con) return;

   dialog = e_dialog_new(con, "E", "_update_available");

   e_object_del_attach_func_set(E_OBJECT(dialog), _delete_cb);
   e_dialog_button_add(dialog, _("OK"), NULL,
                       _ok_cb, NULL);
   e_dialog_button_add(dialog, _("Bother me later"), NULL,
                       _bother_me_later_cb, NULL);
   e_dialog_button_add(dialog, _("Never tell me"), NULL,
                       _never_tell_me_cb, NULL);
   e_dialog_button_focus_num(dialog, 1);
   e_dialog_title_set(dialog, _("Update Notice"));
   e_dialog_icon_set(dialog, "dialog-warning", 64);

   snprintf(text, sizeof(text),
            _("Your enlightenment version is<br>"
              "not the current latest release.<br>"
              "The latest version is:<br>"
              "<br>"
              "%s<br>"
              "<br>"
              "Please visit www.enlightenment.org<br>"
              "or update your system packages<br>"
              "to get a new version."), ver);
   e_dialog_text_set(dialog, text);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);
}

static Eina_Bool
_upload_data_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;
   if (ev->url_con != url_up) return EINA_TRUE;
   if (ev->size > 0)
     {
        char *txt = alloca(ev->size + 1);

        memcpy(txt, ev->data, ev->size);
        txt[ev->size] = 0;

        if (!strncmp(txt, "OK", 2))
          {
          }
        else if (!strncmp(txt, "OLD", 3))
          {
             char *ver = strchr(txt, ' ');
             if (ver)
               {
                  ver++;
                  _new_version(ver);
               }
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_upload_progress_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Progress *ev = event;
   if (ev->url_con != url_up) return EINA_TRUE;
   return EINA_FALSE;
}

static Eina_Bool
_upload_complete_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   if (ev->url_con != url_up) return EINA_TRUE;
   if (ev->status != 200)
     {
        _update_done();
        return EINA_FALSE;
     }
   _update_done();
   return EINA_FALSE;
}

static void
_update_machid_get(void)
{
   FILE *f;
   char buf[4096], *c;
   size_t len;

   f = fopen("/etc/machine-id", "r");
   if (!f) f = fopen("/var/lib/dbus/machine-id", "r");
   if (!f)
     {
        e_user_dir_concat_static(buf, ".machid");
        f = fopen(buf, "r");
     }
   if (f)
     {
        len = fread(buf, 1, sizeof(buf) - 1, f);
        if (len > 10)
          {
             buf[len] = 0;
             for (c = buf; *c; c++)
               {
                  if (!isalnum(*c))
                    {
                       *c = 0;
                       break;
                    }
               }
             machid = strdup(buf);
             fclose(f);
             return;
          }
        fclose(f);
     }

   // generate ID
   e_user_dir_concat_static(buf, ".machid");
   f = fopen(buf, "w");
   if (f)
     {
        double t;
        fwrite("GEN-", 4, 1, f);
        t = ecore_time_unix_get();
        fprintf(f, "%1.16f-%i-%i\n", t, rand(), rand());
        fclose(f);
        _update_machid_get();
        return;
     }
   // this just is all a wash - just use this
   machid = strdup("NOIDEAWHATTHEIDOFTHISMACHINEIS");
}

static void
_update_post_generate(char *buf, int size)
{
   if (!machid) _update_machid_get();
   if (!machid) return;
   snprintf(buf, size,
            "CLIENT %s\n"
            "UPDATE enlightenment %s",
            machid, VERSION);
}

static void
_update_check(void)
{
   char buf[1024];

   if (url_up) _update_done();
   url_up = ecore_con_url_new("http://www.enlightenment.org/update.php");
   if (url_up)
     {
        _update_post_generate(buf, sizeof(buf));
        ecore_con_url_http_version_set(url_up, ECORE_CON_URL_HTTP_VERSION_1_0);
        ecore_con_url_post(url_up, buf, strlen(buf), "text/plain");
     }
   else
     _update_done();
}

static Eina_Bool
_update_timeout_cb(void *data)
{
   double t = 3600.0; // base minimum betwene checks -> 1hr min
   int later = e_config->update.later;

   if (e_config->update.check) _update_check();
   if (update_timer) ecore_timer_del(update_timer);
   if (later > 0)
     {
        later++;
        t *= (later * later);
     }
   update_timer = ecore_timer_add(t, _update_timeout_cb, data);
   return EINA_FALSE;
}

EINTERN int
e_update_init(void)
{
   if (ecore_con_url_init())
     {
        handlers = eina_list_append
          (handlers, ecore_event_handler_add
              (ECORE_CON_EVENT_URL_DATA, _upload_data_cb, NULL));
        handlers = eina_list_append
          (handlers, ecore_event_handler_add
              (ECORE_CON_EVENT_URL_PROGRESS, _upload_progress_cb, NULL));
        handlers = eina_list_append
          (handlers, ecore_event_handler_add
              (ECORE_CON_EVENT_URL_COMPLETE, _upload_complete_cb, NULL));
        if (e_config->update.check)
          {
             e_config->update.check = 0;
             _update_timeout_cb(NULL);
             e_config->update.check = 1;
          }
     }
   return 1;
}

EINTERN int
e_update_shutdown(void)
{
   if (handlers)
     {
        _update_done();
        ecore_con_url_shutdown();
     }
   if (update_timer)
     {
        ecore_timer_del(update_timer);
        update_timer = NULL;
     }
   if (dialog) e_object_del(E_OBJECT(dialog));
   dialog = NULL;
   _update_done();
   if (machid)
     {
        free(machid);
        machid = NULL;
     }
   return 1;
}
