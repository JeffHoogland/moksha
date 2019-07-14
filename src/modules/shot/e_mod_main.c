/**
 * @addtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot Screenshot
 *
 * Takes screen shots and can submit them to http://enlightenment.org
 *
 * @}
 */
#include "e.h"
#include <time.h>
#include "e_mod_main.h"
#include "E_Notify.h"

static E_Module *shot_module = NULL;

static E_Action *border_act = NULL, *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static Ecore_Timer *timer=NULL, *border_timer=NULL, *timer_sec = NULL;
static E_Win *win = NULL;
E_Confirm_Dialog *cd = NULL;

static Evas_Object *o_bg = NULL, *o_box = NULL, *o_content = NULL;
static Evas_Object *o_event = NULL, *o_img = NULL, *o_hlist = NULL;
static E_Manager *sman = NULL;
static E_Container *scon = NULL;
static int quality = 80;
static int screen = -1;
#define MAXZONES 64
static Evas_Object *o_rectdim[MAXZONES] = { NULL };
static Evas_Object *o_radio_all = NULL;
static Evas_Object *o_radio[MAXZONES] = { NULL };
static Evas_Object *o_fsel = NULL;
static Evas_Object *o_label = NULL;
static Evas_Object *o_entry = NULL;
static unsigned char *fdata = NULL;
static int fsize = 0;
static Ecore_Con_Url *url_up = NULL;
static Eina_List *handlers = NULL;
static char *url_ret = NULL;
static E_Dialog *fsel_dia = NULL;
static E_Dialog *_show_dialog_dia = NULL;
static E_Border_Menu_Hook *border_hook = NULL;
static Eina_Bool _shot_no_delay(void *data);

static void _file_select_ok_cb(void *data __UNUSED__, E_Dialog *dia);
static void _file_select_cancel_cb(void *data __UNUSED__, E_Dialog *dia);
static void _cb_dialog_yes(void *data __UNUSED__);
static void _cb_dialog_cancel(void *data __UNUSED__);
static Eina_Bool _notify_cb(void *data __UNUSED__);


static void _shot_conf_new(void);
static void _shot_conf_free(void);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
Config *shot_conf = NULL;


static Eina_Bool
_notify(int counter,const char *text_header, const char *text, const int wait, int view)
{
  char buf[200];
   
#ifdef HAVE_ENOTIFY
  static E_Notification *notification;

   if (view)
     snprintf(buf, sizeof(buf), "%s %d",text,counter);
   else
     snprintf(buf, sizeof(buf), "%s",text);

   notification = e_notification_full_new
          (
            _("Screenshot"),
            0,
            "screenshot",
            text_header,
            buf,
            wait
          );
   e_notification_replaces_id_set(notification, EINA_TRUE);
   e_notification_send(notification, NULL, NULL);
   e_notification_unref(notification);
   notification = NULL;
#endif

   
return EINA_FALSE;
}

Eina_Bool _timer_cb(void *data)
{
   if (shot_conf->count>0)
     {
       if (shot_conf->notify)
         _notify(shot_conf->count,_("Screenshot in: "),"... ",1024,1);
       
       shot_conf->count--;
       return  ECORE_CALLBACK_PASS_ON;
     }
   else
     {
       timer = ecore_timer_add(0.2, _shot_no_delay, data);
       return ECORE_CALLBACK_DONE;
     }
}

static Eina_Bool
_notify_cb(void *data __UNUSED__)
{
   _notify(1,_("Screenshot stored in"),shot_conf->path,3000,0);
   timer = NULL;
   return ECORE_CALLBACK_DONE;
}

static void 
_shot_conf_new(void) 
{
   char buf[PATH_MAX], buff[4096];
	
   shot_conf = E_NEW(Config, 1);
   shot_conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((shot_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x008d);
  
   shot_conf->view_enable = 1;
   snprintf(buff, sizeof(buff), "xdg-open");
   shot_conf->viewer = eina_stringshare_add(buff);
   snprintf(buf, sizeof(buf), "desktop");
   shot_conf->path = eina_stringshare_add(buf);
   shot_conf->notify = 1;
   shot_conf->full_dialog = 0;
   shot_conf->mode_dialog = 1;
   shot_conf->delay = 5.0;
   shot_conf->pict_quality = 80.0;
   
   IFMODCFGEND;
   shot_conf->version = MOD_CONFIG_FILE_VERSION;

   E_CONFIG_LIMIT(shot_conf->delay, 0.0, 10.0);
   E_CONFIG_LIMIT(shot_conf->pict_quality, 0.0, 100.0);
   e_config_save_queue();
}

static void 
_shot_conf_free(void) 
{
   /* cleanup any stringshares here */
   if (shot_conf->viewer) eina_stringshare_del(shot_conf->viewer);
   if (shot_conf->path) eina_stringshare_del(shot_conf->path);
   E_FREE(shot_conf);
}


static void
_win_cancel_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
}

static void
_win_delete_cb(E_Win *w __UNUSED__)
{
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
}

static void
_win_resize_cb(E_Win *w __UNUSED__)
{
   evas_object_resize(o_bg, win->w, win->h);
}

static void
_on_focus_cb(void *data __UNUSED__, Evas_Object *obj)
{
   if (obj == o_content) e_widget_focused_object_clear(o_box);
   else if (o_content) e_widget_focused_object_clear(o_content);
}

static void
_key_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev = event;
   
   if (!strcmp(ev->key, "Tab"))
     {
        if (evas_key_modifier_is_set(evas_key_modifier_get(e_win_evas_get(win)), "Shift"))
          {
             if (e_widget_focus_get(o_box))
               {
                  if (!e_widget_focus_jump(o_box, 0))
                    {
                       e_widget_focus_set(o_content, 0);
                       if (!e_widget_focus_get(o_content))
                          e_widget_focus_set(o_box, 0);
                    }
               }
             else
               {
                  if (!e_widget_focus_jump(o_content, 0))
                     e_widget_focus_set(o_box, 0);
               }
          }
        else
          {
             if (e_widget_focus_get(o_box))
               {
                  if (!e_widget_focus_jump(o_box, 1))
                    {
                       e_widget_focus_set(o_content, 1);
                       if (!e_widget_focus_get(o_content))
                          e_widget_focus_set(o_box, 1);
                    }
               }
             else
               {
                  if (!e_widget_focus_jump(o_content, 1))
                     e_widget_focus_set(o_box, 1);
               }
          }
     }
   else if (((!strcmp(ev->key, "Return")) ||
             (!strcmp(ev->key, "KP_Enter")) ||
             (!strcmp(ev->key, "space"))))
     {
        Evas_Object *o = NULL;
        
        if ((o_content) && (e_widget_focus_get(o_content)))
           o = e_widget_focused_object_get(o_content);
        else
           o = e_widget_focused_object_get(o_box);
        if (o) e_widget_activate(o);
     }            
   else if (!strcmp(ev->key, "Escape"))
     _win_cancel_cb(NULL, NULL);
}            

static void
_save_key_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev = event;
   if ((!strcmp(ev->key, "Return")) || (!strcmp(ev->key, "KP_Enter")))
     _file_select_ok_cb(NULL, fsel_dia);
   else if (!strcmp(ev->key, "Escape"))
     _file_select_cancel_cb(NULL, fsel_dia);
}            

static void
_screen_change_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Eina_List *l;
   E_Zone *z;
   
   EINA_LIST_FOREACH(scon->zones, l, z)
     {
        if (screen == -1)
           evas_object_color_set(o_rectdim[z->num], 0, 0, 0, 0);
        else if (screen == (int)z->num)
           evas_object_color_set(o_rectdim[z->num], 0, 0, 0, 0);
        else
           evas_object_color_set(o_rectdim[z->num], 0, 0, 0, 200);
     }
}


static void
_cb_dialog_yes(void *data __UNUSED__)
{
  e_int_config_shot_module(NULL,NULL);
}

static void
_cb_dialog_cancel(void *data __UNUSED__)
{
   return;
}

static void
_save_to(const char *file)
{
   char opts[256];

   if (eina_str_has_extension(file, ".png"))
     snprintf(opts, sizeof(opts), "compress=%i", 9);
   else
     //~ snprintf(opts, sizeof(opts), "quality=%i", quality);
      snprintf(opts, sizeof(opts), "quality=%i",  (int)shot_conf->pict_quality);
   if (screen == -1)
     {
        if (!evas_object_image_save(o_img, file, NULL, opts))
        {
          e_confirm_dialog_show(_("Error - Folder does not exist"),
                          "application-exit",
                          _("Change folder in Take Screenshot settings<br>"
                          "<br>"
                          "Menu-Settings-All-Advanced-Take Screenshot<br>"
                          "<br>"
                          "Would you like to set up it now?"),
                          _("Yes"), _("Cancel"),
                          _cb_dialog_yes, NULL, NULL, NULL,
                          _cb_dialog_cancel, NULL);
        }
        else
        {
           if (shot_conf->notify)
              timer = ecore_timer_add(1.2, _notify_cb, NULL);
	    }
  }
        
        
   else
     {
        Evas_Object *o;
        Eina_List *l;
        E_Zone *z = NULL;
        
        
        
        EINA_LIST_FOREACH(scon->zones, l, z)
          {
             if (screen == (int)z->num) break;
             z = NULL;
          }
        if (z)
          {
             unsigned char *src, *dst, *s, *d;
             int sstd, y;
             
             o = evas_object_image_add(evas_object_evas_get(o_img));
             evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
             evas_object_image_alpha_set(o, EINA_FALSE);
             evas_object_image_size_set(o, z->w, z->h);
             src = evas_object_image_data_get(o_img, EINA_FALSE);
             sstd = evas_object_image_stride_get(o_img);
             dst = evas_object_image_data_get(o, EINA_TRUE);
             d = dst;
             for (y = z->y; y < z->y + z->h; y++)
               {
                  s = src + (sstd * y) + (z->x * 4);
                  memcpy(d, s, z->w * 4);
                  d += z->w * 4;
               }
             if (!evas_object_image_save(o, file, NULL, opts))
               e_util_dialog_show(_("Error saving screenshot file"),
                                  _("Path: %s"), file);
             
             evas_object_del(o);
          }
     }
}



static void
_file_select_ok_cb(void *data __UNUSED__, E_Dialog *dia)
{
   const char *file;
  
   Ecore_Exe *exe;
   char buf[150];
   

   dia = fsel_dia;
   
   file = e_widget_fsel_selection_path_get(o_fsel);
   
   if ((!file) || (!file[0]) || ((!eina_str_has_extension(file, ".jpg")) && (!eina_str_has_extension(file, ".png"))))
     {
        e_util_dialog_show
           (_("Error - Unknown format"),
               _("File has an unspecified extension.<br>"
                 "Please use '.jpg' or '.png' extensions<br>"
                 "only as other formats are not<br>"
                 "supported currently."));
        return;
     }
   _save_to(file);
   
   if ((shot_conf->view_enable) && (shot_conf->viewer)) 
   {
       snprintf(buf, sizeof(buf), "%s %s",shot_conf->viewer, file);
       exe = e_util_exe_safe_run(buf, NULL);
       if (exe) ecore_exe_free(exe);
   }
   
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
   fsel_dia = NULL;
}

static void
_file_select_cancel_cb(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   fsel_dia = NULL;
}

static void
_file_select_del_cb(void *d __UNUSED__)
{
   fsel_dia = NULL;
}

static void
_win_save_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{ 
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   int mask = 0;
   time_t tt;
   struct tm *tm;
   char buf[PATH_MAX];
   //~ char buff[4096];
   //~ char *dir_path;
   
   time(&tt);
   tm = localtime(&tt);
   //~ if (quality == 100.0)

   shot_conf->pict_quality=(int) quality;

   if (EINA_DBL_EQ(shot_conf->pict_quality, 100.0))
     strftime(buf, sizeof(buf), "shot-%Y-%m-%d_%H-%M-%S.png", tm);
   else
     strftime(buf, sizeof(buf), "shot-%Y-%m-%d_%H-%M-%S.jpg", tm);
     
   fsel_dia = dia = e_dialog_new(scon, "E", "_e_shot_fsel");
   e_dialog_resizable_set(dia, 1);
   e_dialog_title_set(dia, _("Select screenshot save location"));

   o = e_widget_fsel_add(dia->win->evas, shot_conf->path, "/",
                         buf,
                         NULL,
                         NULL, NULL,
                         NULL, NULL, 1);

   e_object_del_attach_func_set(E_OBJECT(dia), _file_select_del_cb);
   e_widget_fsel_window_object_set(o, E_OBJECT(dia->win));
   o_fsel = o;
   evas_object_show(o);
   e_widget_size_min_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);
   e_dialog_button_add(dia, _("Save"), NULL,
                       _file_select_ok_cb, NULL);
   e_dialog_button_add(dia, _("Cancel"), NULL,
                       _file_select_cancel_cb, NULL);
   e_win_centered_set(dia->win, 1);
   o = evas_object_rectangle_add(dia->win->evas);
   if (!evas_object_key_grab(o, "Return", mask, ~mask, 0)) printf("grab err\n");
   mask = 0;
   if (!evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0)) printf("grab err\n");
   mask = 0;
   if (!evas_object_key_grab(o, "Escape", mask, ~mask, 0)) printf("grab err\n");
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _save_key_down_cb, NULL);
   
   if (shot_conf->full_dialog) 
     e_dialog_show(dia);
   else
    _file_select_ok_cb(NULL,NULL);
}

static void
_share_done(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
   o_label = NULL;
   E_FREE(url_ret);
   if (!url_up) return;
   ecore_con_url_free(url_up);
   url_up = NULL;
}

static void
_upload_ok_cb(void *data __UNUSED__, E_Dialog *dia)
{
   // ok just hides dialog and does background upload
   o_label = NULL;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (!win) return;
   e_object_del(E_OBJECT(win));
   win = NULL;
}

static void
_upload_cancel_cb(void *data __UNUSED__, E_Dialog *dia)
{
   o_label = NULL;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
   _share_done();
}

static Eina_Bool
_upload_data_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;
   if (ev->url_con != url_up) return EINA_TRUE;
   if ((o_label) && (ev->size < 1024))
     {
        char *txt = alloca(ev->size + 1);

        memcpy(txt, ev->data, ev->size);
        txt[ev->size] = 0;
/*        
        printf("GOT %i bytes: '%s'\n", ev->size, txt);
        int i;
        for (i = 0; i < ev->size; i++) printf("%02x.", ev->data[i]);
        printf("\n");
 */
        if (!url_ret) url_ret = strdup(txt);
        else
          {
             char *n;
             
             n = malloc(strlen(url_ret) + ev->size + 1);
             if (n)
               {
                  strcpy(n, url_ret);
                  free(url_ret);
                  strcat(n, txt);
                  url_ret = n;
               }
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_upload_progress_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Progress *ev = event;
   //if (ev->url_con != url_up) return EINA_TRUE;
   if (ev->url_con != url_up) return ECORE_CALLBACK_RENEW;

   if (o_label)
     {
        char buf[1024];
        char *buf_now, *buf_total;

        buf_now = e_util_size_string_get(ev->up.now);
        buf_total = e_util_size_string_get(ev->up.total);
        snprintf(buf, sizeof(buf),
                 _("Uploaded %s / %s"), 
                 buf_now, buf_total); 
        E_FREE(buf_now);
        E_FREE(buf_total);
        e_widget_label_text_set(o_label, buf);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_upload_complete_cb(void *data, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   if (ev->url_con != url_up) return EINA_TRUE;

   if (data)
     e_widget_disabled_set(data, 1);
   if (ev->status != 200)
     {
        e_util_dialog_show
           (_("Error - Upload Failed"),
               _("Upload failed with status code:<br>"
                 "%i"),
               ev->status);
        _share_done();
        return EINA_FALSE;
     }
   if ((o_entry) && (url_ret))
      e_widget_entry_text_set(o_entry, url_ret);
   _share_done();
   return EINA_FALSE;
}

static void
_win_share_del(void *data __UNUSED__)
{
   if (handlers) ecore_event_handler_data_set(eina_list_last_data_get(handlers), NULL);
   _upload_cancel_cb(NULL, NULL);
   if (cd) e_object_del(E_OBJECT(cd));
}

static void
_win_share_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   E_Dialog *dia;
   Evas_Object *o, *ol;
   Evas_Coord mw, mh;
   char buf[PATH_MAX];
   FILE *f;
   int i, fd = -1;
   
   srand(time(NULL));
   for (i = 0; i < 10240; i++)
     {
        int v = rand();
        
        //~ if (quality == 100.0)
        if (EINA_DBL_EQ(shot_conf->pict_quality, 100.0))
          snprintf(buf, sizeof(buf), "/tmp/e-shot-%x.png", v);
        else
          snprintf(buf, sizeof(buf), "/tmp/e-shot-%x.jpg", v);
        fd = open(buf, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd >= 0) break;
     }
   if (fd < 0)
     {
        e_util_dialog_show(_("Error - Can't create file"),
                           _("Cannot create temporary file '%s': %s"),
                           buf, strerror(errno));
        if (win)
          {
             e_object_del(E_OBJECT(win));
             win = NULL;
          }
        return;
     }
   _save_to(buf);
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
   f = fdopen(fd, "rb");
   if (!f)
     {
        e_util_dialog_show(_("Error - Can't open file"),
                           _("Cannot open temporary file '%s': %s"),
                           buf, strerror(errno));
        return;
     }
   fseek(f, 0, SEEK_END);
   fsize = ftell(f);
   if (fsize < 1)
     {
        e_util_dialog_show(_("Error - Bad size"),
                           _("Cannot get size of file '%s'"),
                           buf);
        fclose(f);
        return;
     }
   rewind(f);
   free(fdata);
   fdata = malloc(fsize);
   if (!fdata)
     {
        e_util_dialog_show(_("Error - Can't allocate memory"),
                           _("Cannot allocate memory for picture: %s"),
                           strerror(errno));
        fclose(f);
        return;
     }
   if (fread(fdata, fsize, 1, f) != 1)
     {
        e_util_dialog_show(_("Error - Can't read picture"),
                           _("Cannot read picture"));
        E_FREE(fdata);
        fclose(f);
        return;
     }
   fclose(f);
   ecore_file_unlink(buf);
   
   _share_done();
   
   E_LIST_HANDLER_APPEND(handlers, ECORE_CON_EVENT_URL_DATA, _upload_data_cb, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_CON_EVENT_URL_PROGRESS, _upload_progress_cb, NULL);
   
   url_up = ecore_con_url_new("https://www.enlightenment.org/shot.php");
   // why use http 1.1? proxies like squid don't handle 1.1 posts with expect
   // like curl uses by default, so go to 1.0 and this all works dandily
   // out of the box
   ecore_con_url_http_version_set(url_up, ECORE_CON_URL_HTTP_VERSION_1_0);
   ecore_con_url_post(url_up, fdata, fsize, "application/x-e-shot");

   dia = e_dialog_new(scon, "E", "_e_shot_share");
   e_dialog_resizable_set(dia, 1);
   e_dialog_title_set(dia, _("Uploading screenshot"));
   
   o = e_widget_list_add(dia->win->evas, 0, 0);
   ol = o;
   
   o = e_widget_label_add(dia->win->evas, _("Uploading ..."));
   o_label = o;
   e_widget_list_object_append(ol, o, 0, 0, 0.5);
   
   o = e_widget_label_add(dia->win->evas, 
                          _("Screenshot is available at this location:"));
   e_widget_list_object_append(ol, o, 0, 0, 0.5);
   
   o = e_widget_entry_add(dia->win->evas, NULL, NULL, NULL, NULL);
   o_entry = o;
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
      
   e_widget_size_min_get(ol, &mw, &mh);
   e_dialog_content_set(dia, ol, mw, mh);
   e_dialog_button_add(dia, _("Hide"), NULL, _upload_ok_cb, NULL);
   e_dialog_button_add(dia, _("Cancel"), NULL, _upload_cancel_cb, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_CON_EVENT_URL_COMPLETE, _upload_complete_cb, eina_list_last_data_get(dia->buttons));
   e_object_del_attach_func_set(E_OBJECT(dia), _win_share_del);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static void
_win_share_confirm_del(void *d EINA_UNUSED)
{
   cd = NULL;
}

static void
_win_share_confirm_yes(void *d EINA_UNUSED)
{
   _win_share_cb(NULL, NULL);
}

static void
_win_share_confirm_cb(void *d EINA_UNUSED, void *d2 EINA_UNUSED)
{
   if (cd) return;
   cd = e_confirm_dialog_show(_("Confirm Share"), NULL,
                                _("This image will be uploaded<br>"
                                  "to enlightenment.org. It will be publicly visible."),
                                _("Confirm"), _("Cancel"), _win_share_confirm_yes, NULL,
                                NULL, NULL, _win_share_confirm_del, NULL);
}

static void
_rect_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Eina_List *l;
   E_Zone *z;
   
   if (ev->button != 1) return;
   
   e_widget_radio_toggle_set(o_radio_all, 0);
   EINA_LIST_FOREACH(scon->zones, l, z)
     {
        if (obj == o_rectdim[z->num])
          {
             screen = z->num;
             e_widget_radio_toggle_set(o_radio[z->num], 1);
          }
        else
           e_widget_radio_toggle_set(o_radio[z->num], 0);
     }
   
   EINA_LIST_FOREACH(scon->zones, l, z)
     {
        if (screen == -1)
           evas_object_color_set(o_rectdim[z->num], 0, 0, 0, 0);
        else if (screen == (int)z->num)
           evas_object_color_set(o_rectdim[z->num], 0, 0, 0, 0);
        else
           evas_object_color_set(o_rectdim[z->num], 0, 0, 0, 200);
     }
}

static void
_shot_now(E_Zone *zone, E_Border *bd)
{
   Ecore_X_Image *img;
   unsigned char *src;
   unsigned int *dst;
   int bpl = 0, rows = 0, bpp = 0, sw, sh;
   Evas *evas, *evas2;
   Evas_Object *o, *oa, *op, *ol;
   int x, y, w, h;
   Evas_Modifier_Mask mask;
   Ecore_X_Window xwin, root;
   E_Radio_Group *rg;
   Ecore_X_Visual visual;
   Ecore_X_Display *display;
   Ecore_X_Screen *scr;
   Ecore_X_Window_Attributes watt;
   Ecore_X_Colormap colormap;
   
   watt.visual = 0;
   if ((!zone) && (!bd)) return;
   if (zone)
     {
        sman = zone->container->manager;
        scon = zone->container;
        xwin = sman->root;
        w = sw = sman->w;
        h = sh = sman->h;
        x = y = 0;
     }
   else
     {
        root = bd->zone->container->manager->root;
        xwin = bd->client.win;
        while (xwin != root)
          {
             if (ecore_x_window_parent_get(xwin) == root) break;
             xwin = ecore_x_window_parent_get(xwin);
          }
        ecore_x_window_geometry_get(xwin, &x, &y, &sw, &sh);
        w = sw;
        h = sh;
        xwin = root;
        x = E_CLAMP(bd->x, bd->zone->x, bd->zone->x + bd->zone->w);
        y = E_CLAMP(bd->y, bd->zone->y, bd->zone->y + bd->zone->h);
        sw = E_CLAMP(sw, 1, bd->zone->x + bd->zone->w - x);
        sh = E_CLAMP(sh, 1, bd->zone->y + bd->zone->h - y);
     }
   if (!ecore_x_window_attributes_get(xwin, &watt)) return;
   visual = watt.visual;
   img = ecore_x_image_new(w, h, visual, ecore_x_window_depth_get(xwin));
   ecore_x_image_get(img, xwin, x, y, 0, 0, sw, sh);
   src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   display = ecore_x_display_get();
   scr = ecore_x_default_screen_get();
   colormap = ecore_x_default_colormap_get(display, scr);
   dst = malloc(sw * sh * sizeof(unsigned int));
   ecore_x_image_to_argb_convert(src, bpp, bpl, colormap, visual,
                                 0, 0, sw, sh,
                                 dst, (sw * sizeof(int)), 0, 0);
   
   if (win) e_object_del(E_OBJECT(win));
   win = e_win_new(e_container_current_get(e_manager_current_get()));
   
   evas = e_win_evas_get(win);
   e_win_title_set(win, _("Where to put Screenshot..."));
   e_win_delete_callback_set(win, _win_delete_cb);
   e_win_resize_callback_set(win, _win_resize_cb);
   e_win_dialog_set(win, 1);
   e_win_centered_set(win, 1);
   e_win_name_class_set(win, "E", "_shot_dialog");
   
   o = edje_object_add(evas);
   o_bg = o;;
   e_theme_edje_object_set(o, "base/theme/dialog", "e/widgets/dialog/main");
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   o = e_widget_list_add(evas, 0, 0);
   o_content = o;
   e_widget_size_min_get(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   edje_object_part_swallow(o_bg, "e.swallow.content", o);
   evas_object_show(o);

   w = sw / 4;
   if (w < 220) w = 220;
   h = (w * sh) / sw;
   
   o = e_widget_aspect_add(evas, w, h);
   oa = o;
   o = e_widget_preview_add(evas, w, h);
   op = o;

   evas2 = e_widget_preview_evas_get(op);
   
   o = evas_object_image_filled_add(evas2);
   o_img = o;
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, EINA_FALSE);
   evas_object_image_size_set(o, sw, sh);
   evas_object_image_data_copy_set(o, dst);
   free(dst);
   ecore_x_image_free(img);
   evas_object_image_data_update_add(o, 0, 0, sw, sh);
   e_widget_preview_extern_object_set(op, o);
   evas_object_show(o);

   evas_object_show(op);
   evas_object_show(oa);
   
   e_widget_aspect_child_set(oa, op);
   e_widget_list_object_append(o_content, oa, 0, 0, 0.5);
   
   o = e_widget_list_add(evas, 1, 1);
   o_hlist = o;

   o = e_widget_framelist_add(evas, _("Quality"), 0);
   ol = o;

   if (EINA_DBL_EQ(shot_conf->pict_quality, 100.0)) quality=100;
   if ((shot_conf->pict_quality>=80) && (shot_conf->pict_quality<100)) quality=90;
   if ((shot_conf->pict_quality>=60) && (shot_conf->pict_quality<80)) quality=70;
   if (shot_conf->pict_quality<60) quality=50;
   
   rg = e_widget_radio_group_new(&quality);
   o = e_widget_radio_add(evas, _("Perfect"), 100, rg);
   e_widget_framelist_object_append(ol, o);
   o = e_widget_radio_add(evas, _("High"), 90, rg);
   e_widget_framelist_object_append(ol, o);
   o = e_widget_radio_add(evas, _("Medium"), 70, rg);
   e_widget_framelist_object_append(ol, o);
   o = e_widget_radio_add(evas, _("Low"), 50, rg);
   e_widget_framelist_object_append(ol, o);
   e_widget_list_object_append(o_hlist, ol, 1, 0, 0.5);

   if (zone)
     {
        screen = -1;
        if (eina_list_count(scon->zones) > 1)
          {
             Eina_List *l;
             E_Zone *z;
             int i;
             
             o = e_widget_framelist_add(evas, _("Screen"), 0);
             ol = o;
             
             rg = e_widget_radio_group_new(&screen);
             o = e_widget_radio_add(evas, _("All"), -1, rg);
             o_radio_all = o;
             evas_object_smart_callback_add(o, "changed", _screen_change_cb, NULL);
             e_widget_framelist_object_append(ol, o);
             i = 0;
             EINA_LIST_FOREACH(scon->zones, l, z)
               {
                  char buf[32];

                  if (z->num >= MAXZONES) continue;
                  snprintf(buf, sizeof(buf), "%i", z->num);
                  o = e_widget_radio_add(evas, buf, z->num, rg);
                  o_radio[z->num] = o;
                  evas_object_smart_callback_add(o, "changed", _screen_change_cb, NULL);
                  e_widget_framelist_object_append(ol, o);
                  
                  o = evas_object_rectangle_add(evas2);
                  evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                                 _rect_down_cb, NULL);
                  o_rectdim[z->num] = o;
                  evas_object_color_set(o, 0, 0, 0, 0);
                  evas_object_show(o);
                  evas_object_geometry_get(o_img, NULL, NULL, &w, &h);
                  evas_object_move(o, 
                                   (z->x * w) / sw,
                                   (z->y * h) / sh);
                  evas_object_resize(o, 
                                     (z->w * w) / sw,
                                     (z->h * h) / sh);
                  i++;
               }
             
             e_widget_list_object_append(o_hlist, ol, 1, 0, 0.5);
          }
        
     }
   e_widget_list_object_append(o_content, o_hlist, 0, 0, 0.5);

   o = o_content;
   e_widget_size_min_get(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   edje_object_part_swallow(o_bg, "e.swallow.content", o);
   evas_object_show(o);
   
   ///////////////////////////////////////////////////////////////////////
   
   o = e_widget_list_add(evas, 1, 1);
   o_box = o;
   e_widget_on_focus_hook_set(o, _on_focus_cb, NULL);
   edje_object_part_swallow(o_bg, "e.swallow.buttons", o);
   
   o = e_widget_button_add(evas, _("Save"), NULL, _win_save_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Share"), NULL, _win_share_confirm_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Cancel"), NULL, _win_cancel_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   
   o = o_box;
   e_widget_size_min_get(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   edje_object_part_swallow(o_bg, "e.swallow.buttons", o);
   
   o = evas_object_rectangle_add(evas);
   o_event = o;
   mask = 0;
   if (!evas_object_key_grab(o, "Tab", mask, ~mask, 0)) printf("grab err\n");
   mask = evas_key_modifier_mask_get(evas, "Shift");
   if (!evas_object_key_grab(o, "Tab", mask, ~mask, 0)) printf("grab err\n");
   mask = 0;
   if (!evas_object_key_grab(o, "Return", mask, ~mask, 0)) printf("grab err\n");
   mask = 0;
   if (!evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0)) printf("grab err\n");
   mask = 0;
   if (!evas_object_key_grab(o, "space", mask, ~mask, 0)) printf("grab err\n");
   mask = 0;
   if (!evas_object_key_grab(o, "Escape", mask, ~mask, 0)) printf("grab err\n");
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _key_down_cb, NULL);
   
   edje_object_size_min_calc(o_bg, &w, &h);
   evas_object_resize(o_bg, w, h);
   e_win_resize(win, w, h);
   e_win_size_min_set(win, w, h);
   e_win_size_max_set(win, 99999, 99999);
   if (shot_conf->full_dialog) e_win_show(win);
   e_win_border_icon_set(win, "screenshot");
   
   if (!e_widget_focus_get(o_bg)) e_widget_focus_set(o_box, 1);
   if (!shot_conf->full_dialog) _win_save_cb(win,NULL);
}

static Eina_Bool
_shot_no_delay(void *data)
{
   timer_sec = timer = NULL;
   _shot_now(data, NULL);
   return EINA_FALSE;
}

static Eina_Bool
_shot_delay_border(void *data)
{
   border_timer = NULL;
   _shot_now(NULL, data);
   return EINA_FALSE;
}

static void
_shot_border(E_Border *bd)
{
   if (border_timer)  ecore_timer_del(border_timer);
   border_timer = ecore_timer_add(1.0, _shot_delay_border, bd);
}

static void
_shot(E_Zone *zone, Eina_Bool instant)
{
   if (!instant)
       shot_conf->count = shot_conf->delay;
 
   if (timer) ecore_timer_del(timer);
   if (timer_sec) ecore_timer_del(timer_sec);
  
   if (shot_conf->delay > 0)
       timer_sec = ecore_timer_add(1.0, _timer_cb, zone);
   else
       timer = ecore_timer_add(0.2, _shot_no_delay, zone);
}


static void
_e_mod_menu_border_cb(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   _shot_border(data);
}


static void
_e_mod_action_border_cb(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   E_Border *bd;
   bd = e_border_focused_get();
   if (!bd) return;
   if (border_timer)
      {
         ecore_timer_del(border_timer);
         border_timer = NULL;
      }
   _shot_now(NULL, bd);
}

static void
_e_mod_action_cb(E_Object *obj, Eina_Bool instant)
{
   E_Zone *zone = NULL;
   
   if (obj)
     {
        if (obj->type == E_MANAGER_TYPE)
           zone = e_util_zone_current_get((E_Manager *)obj);
        else if (obj->type == E_CONTAINER_TYPE)
           zone = e_util_zone_current_get(((E_Container *)obj)->manager);
        else if (obj->type == E_ZONE_TYPE)
           zone = ((E_Zone *)obj);
        else
           zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone) return;
   if (timer)
      {
         ecore_timer_del(timer);
         timer = NULL;
         ecore_timer_del(timer_sec);
         timer_sec = NULL;
      }
      
  _shot(zone, instant);
}

static void
_shot_dialog_del(void *data)
{
   if (_show_dialog_dia == data)
     _show_dialog_dia = NULL;
}

static void
_shot_dialog_instant_cb(void *data, E_Dialog *dia)
{
    e_object_del(E_OBJECT(dia));
   _e_mod_action_cb(data, EINA_TRUE);
}

static void
_shot_dialog_delayed_cb(void *data, E_Dialog *dia)
{
    e_object_del(E_OBJECT(dia));
   _e_mod_action_cb(data, EINA_FALSE);
}

static void
_shot_dialog_settings_cb(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_int_config_shot_module(NULL, NULL);
}

static void
_shot_dialog_key_down(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev = event;
   E_Dialog *dia = data;

   if (strcmp(ev->key, "Return") == 0)
     _shot_dialog_instant_cb(NULL, dia);
   else if (strcmp(ev->key, "Escape") == 0)
     _shot_dialog_del(dia);
}

static void
_show_dialog(E_Object *obj, const char *params __UNUSED__)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dia;
   char buf[128];

   if (_show_dialog_dia) return;
   
   if (!shot_conf->mode_dialog){
       _e_mod_action_cb(obj, EINA_FALSE);
       return;
   }
   
   if (!(man = e_manager_current_get())) return;
   if (!(con = e_container_current_get(man))) return;
   if (!(dia = e_dialog_new(con, "E", "_shot_dialog"))) return;

   snprintf(buf, sizeof(buf), _("Select the screenshot mode... <br>"
        "Delay time is set in the Screenshot module settings<br>"
        "The current delay time: <b> %d s</b>"), (int)shot_conf->delay);
   e_dialog_title_set(dia, _("Screenshot"));
   e_dialog_icon_set(dia, "screenshot", 64);
   e_dialog_text_set(dia, buf);

   e_object_del_attach_func_set(E_OBJECT(dia), _shot_dialog_del);

   e_dialog_button_add(dia, _("Instant shot"), NULL, _shot_dialog_instant_cb, obj);
   e_dialog_button_add(dia, _("Delayed shot"), NULL, _shot_dialog_delayed_cb, obj);
   e_dialog_button_add(dia, _("Settings"), NULL, _shot_dialog_settings_cb, NULL);
   e_dialog_button_add(dia, _("Close"), NULL, NULL, NULL);

   e_dialog_button_focus_num(dia, 0);
   e_widget_list_homogeneous_set(dia->box_object, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);

   evas_object_event_callback_add(dia->bg_object, EVAS_CALLBACK_KEY_DOWN,
        _shot_dialog_key_down, dia);

   _show_dialog_dia = dia;
}

static void
_e_mod_menu_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (m->zone) _show_dialog(NULL, NULL);
}

static void
_bd_hook(void *d __UNUSED__, E_Border *bd)
{
   E_Menu_Item *mi;
   E_Menu *m;
   Eina_List *l;
   if (!bd->border_menu) return;
   if (bd->iconic || (bd->desk != e_desk_current_get(bd->zone))) return;
   m = bd->border_menu;

   /* position menu item just before first separator */
   EINA_LIST_FOREACH(m->items, l, mi)
     if (mi->separator) break;
   if ((!mi) || (!mi->separator)) return;
   l = eina_list_prev(l);
   mi = eina_list_data_get(l);
   if (!mi) return;

   mi = e_menu_item_new_relative(m, mi);
   e_menu_item_label_set(mi, _("Take Shot"));
   e_util_menu_item_theme_icon_set(mi, "screenshot");
   e_menu_item_callback_set(mi, _e_mod_menu_border_cb, bd);
}

static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Take Screenshot"));
   e_util_menu_item_theme_icon_set(mi, "screenshot");
   e_menu_item_callback_set(mi, _e_mod_menu_cb, NULL);
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Shot"
};

EAPI void *
e_modapi_init(E_Module *m)
{


#ifdef HAVE_ENOTIFY
   e_notification_init();
#endif

   if (!ecore_con_url_init())
     {
        e_util_dialog_show(_("Shot Error"),
                           _("Cannot initialize network"));
        return NULL;
     }
 
   e_module_delayed_set(m, 1);
   
   shot_module = m;
   act = e_action_add("shot");
   if (act)
     {
        act->func.go = _show_dialog;
        e_action_predef_name_set(N_("Screen"), N_("Take Screenshot"),
                                 "shot", NULL, NULL, 0);
     }
     
   border_act = e_action_add("border_shot");
   if (border_act)
     {
        border_act->func.go = _e_mod_action_border_cb;
        e_action_predef_name_set(N_("Window : Actions"), N_("Take Shot"),
                                 "border_shot", NULL, NULL, 0);
     }
   maug = e_int_menus_menu_augmentation_add_sorted
      ("main/2",  _("Take Screenshot"), _e_mod_menu_add, NULL, NULL, NULL);
   border_hook = e_int_border_menu_hook_add(_bd_hook, NULL);
   
      
  e_configure_registry_category_add("extensions", 90, "Takescreenshot", 
                                 NULL, "preferences-extensions");
         
   e_configure_registry_item_add("extensions/takescreenshot", 20, _("Screenshot Settings"), 
                                 NULL, "screenshot", e_int_config_shot_module);
  
   #undef T
   #undef D

   conf_edd = E_CONFIG_DD_NEW("Shot_Config", Config);

   #define T Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   //~ E_CONFIG_VAL(D, T, switch1, UCHAR); /* our var from header */
   E_CONFIG_VAL(D, T, viewer, STR); /* our var from header */
   E_CONFIG_VAL(D, T, path, STR); /* our var from header */
   E_CONFIG_VAL(D, T, view_enable, INT); /* our var from header */
   E_CONFIG_VAL(D, T, notify, INT); /* our var from header */
   E_CONFIG_VAL(D, T, full_dialog, INT); /* our var from header */
   E_CONFIG_VAL(D, T, mode_dialog, INT); /* our var from header */
   E_CONFIG_VAL(D, T, delay, DOUBLE); /* our var from header */
   E_CONFIG_VAL(D, T, pict_quality, DOUBLE); /* our var from header */
   
   E_CONFIG_LIST(D, T, conf_items, conf_item_edd); /* the list */

   shot_conf = e_config_domain_load("module.takescreen", conf_edd);
   if (shot_conf) 
     {
        /* Check config version */
        if ((shot_conf->version >> 16) < MOD_CONFIG_FILE_EPOCH) 
          {
             /* config too old */
	    _shot_conf_free();
	    
          }

        /* Ardvarks */
        else if (shot_conf->version > MOD_CONFIG_FILE_VERSION) 
          {
             /* config too new...wtf ? */
             _shot_conf_free();
	     
          }
     }
     
     
     if (!shot_conf) _shot_conf_new();
   shot_conf->module = m;
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   _share_done();
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
   if (cd) E_FN_DEL(e_object_del, cd);
   if (timer)
     {
        ecore_timer_del(timer);
        timer = NULL;
     }
   if (timer_sec)
     {
        ecore_timer_del(timer_sec);
        timer_sec = NULL;
     }
   if (border_timer)
     {
        ecore_timer_del(border_timer);
        border_timer = NULL;
     }
   if (maug)
     {
        e_int_menus_menu_augmentation_del("main/2", maug);
        maug = NULL;
     }
   if (act)
     {
        e_action_predef_name_del("Screen", "Take Screenshot");
        e_action_del("shot");
        act = NULL;
     }
   if (border_act)
     {
        e_action_predef_name_del("Window : Actions", "Take Shot");
        e_action_del("border_shot");
        border_act = NULL;
     }
   shot_module = NULL;
   e_int_border_menu_hook_del(border_hook);
   ecore_con_url_shutdown();
   
   #ifdef HAVE_ENOTIFY
   e_notification_shutdown();
   #endif
   
   _shot_conf_free();
   
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
	e_config_domain_save("module.takescreen", conf_edd, shot_conf);
   return 1;
}
