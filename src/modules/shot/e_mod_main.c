#include "e.h"
#include "e_mod_main.h"

static E_Module *shot_module = NULL;

static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static Ecore_Timer *timer = NULL;
static E_Win *win = NULL;
static Evas_Object *o_bg = NULL, *o_box = NULL, *o_content = NULL;
static Evas_Object *o_event = NULL, *o_img = NULL, *o_hlist = NULL;
static E_Manager *sman = NULL;
static E_Container *scon = NULL;
static int quality = 90;
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
   
   if (!strcmp(ev->keyname, "Tab"))
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
   else if (((!strcmp(ev->keyname, "Return")) ||
             (!strcmp(ev->keyname, "KP_Enter")) ||
             (!strcmp(ev->keyname, "space"))))
     {
        Evas_Object *o = NULL;
        
        if ((o_content) && (e_widget_focus_get(o_content)))
           o = e_widget_focused_object_get(o_content);
        else
           o = e_widget_focused_object_get(o_box);
        if (o) e_widget_activate(o);
     }            
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
_save_to(const char *file)
{
   char *extn = strrchr(file, '.');
   char opts[256];
   
   if (!extn) 
     {
        e_util_dialog_show
           (_("Error - Unknown format"),
               _("File has an unspecified extension.<br>"
                 "Please use '.jpg' or '.png' extensions<br>"
                 "only as other formats are not<br>"
                 "supported currently."));
        return;
     }
   if (!((!strcasecmp(extn, ".png")) ||
         (!strcasecmp(extn, ".jpg")) ||
         (!strcasecmp(extn, ".jpeg"))))
     {
        e_util_dialog_show
           (_("Error - Unknown format"),
               _("File has an unrecognized extension.<br>"
                 "Please use '.jpg' or '.png' extensions<br>"
                 "only as other formats are not<br>"
                 "supported currently."));
        return;
     }
   if (!strcasecmp(extn, ".png"))
      snprintf(opts, sizeof(opts), "compress=%i", 9);
   else
      snprintf(opts, sizeof(opts), "quality=%i", quality);
   if (screen == -1)
     {
        evas_object_image_save(o_img, file, NULL, opts);
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
             for (y = 0; y < z->h; y++)
               {
                  s = src + (sstd * y) + (z->x * 4);
                  memcpy(d, s, z->w * 4);
                  d += z->w * 4;
               }
             evas_object_image_save(o, file, NULL, opts);
             evas_object_del(o);
          }
     }
}

static void
_file_select_ok_cb(void *data __UNUSED__, E_Dialog *dia)
{
   const char *file;

   file = e_widget_fsel_selection_path_get(o_fsel);
   printf("SAVE: %s\n", file);
   if (file) _save_to(file);
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
}

static void
_file_select_cancel_cb(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
}

static void
_win_save_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{ 
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   
   dia = e_dialog_new(scon, "E", "_e_shot_fsel");
   e_dialog_title_set(dia, _("Select screenshot save location"));
   o = e_widget_fsel_add(dia->win->evas, "~/", "/", "shot.jpg", NULL,
                         NULL, NULL,
                         NULL, NULL, 1);
   e_widget_fsel_window_object_set(o, E_OBJECT(dia->win));
   o_fsel = o;
   evas_object_show(o);
   e_widget_size_min_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);
   e_dialog_button_add(dia, _("OK"), NULL,
                       _file_select_ok_cb, NULL);
   e_dialog_button_add(dia, _("Cancel"), NULL,
                       _file_select_cancel_cb, NULL);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static void
_share_done(void)
{
   Ecore_Event_Handler *h;

   EINA_LIST_FREE(handlers, h) ecore_event_handler_del(h);
   o_label = NULL;
   if (url_ret)
     {
        free(url_ret);
        url_ret = NULL;
     }
   if (url_up)
     {
        ecore_con_url_free(url_up);
        url_up = NULL;
     }
   ecore_con_url_shutdown();
}

static void
_upload_ok_cb(void *data __UNUSED__, E_Dialog *dia)
{
   // ok just hides dialog and does background upload
   o_label = NULL;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
   _share_done();
}

static void
_upload_cancel_cb(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   if (win)
     {
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
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
                  free(url_ret);
                  strcpy(n, url_ret);
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
   if (ev->url_con != url_up) return EINA_TRUE;
   if (o_label)
     {
        char buf[1024];
        
        snprintf(buf, sizeof(buf),
                 "Uploaded %1.1fKB / %1.1fKB", 
                 ev->up.now / 1024, 
                 ev->up.total / 1024);
        e_widget_label_text_set(o_label, buf);
     }
   return EINA_FALSE;
}

static Eina_Bool
_upload_complete_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   if (ev->url_con != url_up) return EINA_TRUE;
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
_win_share_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   E_Dialog *dia;
   Evas_Object *o, *ol;
   Evas_Coord mw, mh;
   char buf[PATH_MAX];
   FILE *f;
   
   if (quality == 100) snprintf(buf, sizeof(buf), "/tmp/e-shot-XXXXXX.png");
   else snprintf(buf, sizeof(buf), "/tmp/e-shot-XXXXXX.jpg");
   if (!mkstemp(buf))
     {
        e_util_dialog_show
           (_("Error - Can't create File"),
               _("Cannot create temporary file:\n"
                 "%s"),
               buf);
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
   f = fopen(buf, "rb");
   if (!f)
     {
        // FIXME: error disp
        return;
     }
   fseek(f, 0, SEEK_END);
   fsize = ftell(f);
   if (fsize < 1)
     {
        // FIXME: error disp
        fclose(f);
        return;
     }
   rewind(f);
   if (fdata) free(fdata);
   fdata = malloc(fsize);
   if (!fdata)
     {
        // FIXME: error disp
        fclose(f);
        return;
     }
   if (fread(fdata, fsize, 1, f) != 1)
     {
        // FIXME: error disp
        free(fdata);
        fdata = NULL;
        fclose(f);
        return;
     }
   fclose(f);
   ecore_file_unlink(buf);
   
   _share_done();
   
   if (!ecore_con_url_init())
     {
        // FIXME: error disp
        free(fdata);
        fdata = NULL;
        return;
     }
   
   handlers = eina_list_append
      (handlers, ecore_event_handler_add
          (ECORE_CON_EVENT_URL_DATA, _upload_data_cb, NULL));
   handlers = eina_list_append
      (handlers, ecore_event_handler_add
          (ECORE_CON_EVENT_URL_PROGRESS, _upload_progress_cb, NULL));
   handlers = eina_list_append
      (handlers, ecore_event_handler_add
          (ECORE_CON_EVENT_URL_COMPLETE, _upload_complete_cb, NULL));
   
   url_up = ecore_con_url_new("http://www.enlightenment.org/shot.php");
   ecore_con_url_post(url_up, fdata, fsize, "application/x-e-shot");
   
   dia = e_dialog_new(scon, "E", "_e_shot_share");
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
   e_dialog_button_add(dia, _("OK"), NULL, _upload_ok_cb, NULL);
   e_dialog_button_add(dia, _("Cancel"), NULL, _upload_cancel_cb, NULL);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
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
_shot_now(E_Zone *zone)
{
   Ecore_X_Image *img;
   Ecore_X_Window_Attributes att;
   unsigned char *src;
   unsigned int *dst;
   int bpl = 0, rows = 0, bpp = 0;
   Evas *evas, *evas2;
   Evas_Object *o, *oa, *op, *ol;
   Evas_Coord w, h;
   Evas_Modifier_Mask mask;
   E_Radio_Group *rg;
   
   sman = zone->container->manager;
   scon = zone->container;
   memset(&att, 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(sman->root, &att);
   img = ecore_x_image_new(sman->w, sman->h, att.visual, att.depth);
   ecore_x_image_get(img, sman->root, 0, 0, 0, 0, sman->w, sman->h);
   src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   if (!ecore_x_image_is_argb32_get(img))
     {
        dst = malloc(sman->w * sman->h * sizeof(int));
        ecore_x_image_to_argb_convert(src, bpp, bpl, att.colormap, att.visual,
                                      0, 0, sman->w, sman->h,
                                      dst, (sman->w * sizeof(int)), 0, 0);
     }
   else
      dst = (unsigned int *)src;
   
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
   edje_extern_object_min_size_set(o, w, h);
   edje_object_part_swallow(o_bg, "e.swallow.content", o);
   evas_object_show(o);

   w = sman->w / 4;
   if (w < 220) w = 220;
   h = (w * sman->h) / sman->w;
   
   o = e_widget_aspect_add(evas, w, h);
   oa = o;
   o = e_widget_preview_add(evas, w, h);
   op = o;

   evas2 = e_widget_preview_evas_get(op);
   
   o = evas_object_image_filled_add(evas2);
   o_img = o;
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, EINA_FALSE);
   evas_object_image_size_set(o, sman->w, sman->h);
   evas_object_image_data_copy_set(o, dst);
   if (dst != (unsigned int *)src) free(dst);
   ecore_x_image_free(img);
   evas_object_image_data_update_add(o, 0, 0, sman->w, sman->h);
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
                              (z->x * w) / sman->w,
                              (z->y * h) / sman->h);
             evas_object_resize(o, 
                                (z->w * w) / sman->w,
                                (z->h * h) / sman->h);
             i++;
          }
        
        e_widget_list_object_append(o_hlist, ol, 1, 0, 0.5);
     }
   
   e_widget_list_object_append(o_content, o_hlist, 0, 0, 0.5);

   o = o_content;
   e_widget_size_min_get(o, &w, &h);
   edje_extern_object_min_size_set(o, w, h);
   edje_object_part_swallow(o_bg, "e.swallow.content", o);
   evas_object_show(o);
   
   ///////////////////////////////////////////////////////////////////////
   
   o = e_widget_list_add(evas, 1, 1);
   o_box = o;
   e_widget_on_focus_hook_set(o, _on_focus_cb, NULL);
   edje_object_part_swallow(o_bg, "e.swallow.buttons", o);

   o = e_widget_button_add(evas, _("Save"), NULL, _win_save_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Share"), NULL, _win_share_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Cancel"), NULL, _win_cancel_cb, win, NULL);
   e_widget_list_object_append(o_box, o, 1, 0, 0.5);
   
   o = o_box;
   e_widget_size_min_get(o, &w, &h);
   edje_extern_object_min_size_set(o, w, h);
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
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _key_down_cb, NULL);
   
   edje_object_size_min_calc(o_bg, &w, &h);
   evas_object_resize(o_bg, w, h);
   e_win_resize(win, w, h);
   e_win_size_min_set(win, w, h);
   e_win_size_max_set(win, 99999, 99999);
   e_win_show(win);
   e_win_border_icon_set(win, "enlightenment/shot");
   
   if (!e_widget_focus_get(o_bg)) e_widget_focus_set(o_box, 1);
}

static Eina_Bool
_shot_delay(void *data)
{
   timer = NULL;
   _shot_now(data);
   return EINA_FALSE;
}

static void
_shot(E_Zone *zone)
{
   if (timer) ecore_timer_del(timer);
   timer = ecore_timer_add(1.0, _shot_delay, zone);
}

static void
_e_mod_menu_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (m->zone) _shot(m->zone);
}

static void
_e_mod_action_cb(E_Object *obj, const char *params __UNUSED__)
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
      }
   _shot_now(zone);
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
   e_module_delayed_set(m, 1);
   
   shot_module = m;
   act = e_action_add("shot");
   if (act)
     {
        act->func.go = _e_mod_action_cb;
        e_action_predef_name_set(_("Screen"), _("Take Screenshot"),
                                 "shot", NULL, NULL, 0);
     }
   maug = e_int_menus_menu_augmentation_add_sorted
      ("main/2",  _("Take Screenshot"), _e_mod_menu_add, NULL, NULL, NULL);
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
   if (timer)
     {
        ecore_timer_del(timer);
        timer = NULL;
     }
   if (maug)
     {
        e_int_menus_menu_augmentation_del("main/2", maug);
        maug = NULL;
     }
   if (act)
     {
        e_action_predef_name_del(_("Screen"), _("Take Screenshot"));
        e_action_del("shot");
        act = NULL;
     }
   shot_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
