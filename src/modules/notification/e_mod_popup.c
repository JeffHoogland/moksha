#include "e_mod_main.h"
#include <time.h>

/* Popup function protos */
static Popup_Data *_notification_popup_new(E_Notification *n);
static Popup_Data *_notification_popup_find(unsigned int id);
static Popup_Data *_notification_popup_merge(E_Notification *n);

static int         _notification_popup_place(Popup_Data *popup,
                                             int         num);
static void        _notification_popup_refresh(Popup_Data *popup);
static void        _notification_popup_del(unsigned int                 id,
                                           E_Notification_Closed_Reason reason);
static void        _notification_popdown(Popup_Data                  *popup,
                                         E_Notification_Closed_Reason reason);
static void        _notification_actions(Popup_Data *popup);
static void         list_add_item(Popup_Data *popup);
/* this function should be external in edje for use in cases such as this module.
 *
 * happily, it was decided that the function would not be external so that it could
 * be duplicated into the module in full.
 */

static int
_text_escape(Eina_Strbuf *txt, const char *text)
{
   const char *escaped;
   int advance;

   escaped = evas_textblock_string_escape_get(text, &advance);
   if (!escaped)
     {
        eina_strbuf_append_char(txt, text[0]);
        advance = 1;
     }
   else
     eina_strbuf_append(txt, escaped);
   return advance;
}

/* hardcoded list of allowed tags based on
 * https://people.gnome.org/~mccann/docs/notification-spec/notification-spec-latest.html#markup
 */
static const char *tags[] =
{
   "<b",
   "<i",
   "<u",
   //"<a", FIXME: we can't actually display these right now
   //"<img",
};

static const char *
_get_tag(const char *c)
{
   unsigned int i;

   if (c[1] != '>') return NULL;
   for (i = 0; i < EINA_C_ARRAY_LENGTH(tags); i++)
     if (tags[i][1] == c[0]) return tags[i];
   return NULL;
}

char *
_nedje_text_escape(const char *text)
{
   Eina_Strbuf *txt;
   char *ret;
   const char *text_end;
   size_t text_len;
   Eina_Array *arr;
   const char *cur_tag = NULL;

   if (!text) return NULL;

   txt = eina_strbuf_new();
   text_len = strlen(text);
   arr = eina_array_new(3);

   text_end = text + text_len;
   while (text < text_end)
     {
        int advance;

        if ((text[0] == '<') && text[1])
          {
             const char *tag, *popped;
             Eina_Bool closing = EINA_FALSE;

             if (text[1] == '/') //closing tag
               {
                  closing = EINA_TRUE;
                  tag = _get_tag(text + 2);
               }
             else
               tag = _get_tag(text + 1);
             if (closing)
               {
                  if (cur_tag && (tag != cur_tag))
                    {
                       /* tag mismatch: autoclose all failure tags
                        * not technically required by the spec,
                        * but it makes me feel better about myself
                        */
                       do
                         {
                            popped = eina_array_pop(arr);
                            if (eina_array_count(arr))
                              cur_tag = eina_array_data_get(arr, eina_array_count(arr) - 1);
                            else
                              cur_tag = NULL;
                            eina_strbuf_append_printf(txt, "</%c>", popped[1]);
                         } while (cur_tag && (popped != tag));
                       advance = 4;
                    }
                  else if (cur_tag)
                    {
                       /* tag match: just pop */
                       popped = eina_array_pop(arr);
                       if (eina_array_count(arr))
                         cur_tag = eina_array_data_get(arr, eina_array_count(arr) - 1);
                       else
                         cur_tag = NULL;
                       eina_strbuf_append_printf(txt, "</%c>", popped[1]);
                       advance = 4;
                    }
                  else
                    {
                       /* no current tag: escape */
                       advance = _text_escape(txt, text);
                    }
               }
             else
               {
                  if (tag)
                    {
                       cur_tag = tag;
                       eina_array_push(arr, tag);
                       eina_strbuf_append_printf(txt, "<%c>", tag[1]);
                       advance = 3;
                    }
                  else
                    advance = _text_escape(txt, text);
               }
          }
        else if (text[0] == '&')
          {
             const char *s;

             s = strchr(text, ';');
             if (s)
               s = evas_textblock_escape_string_range_get(text, s + 1);
             if (s)
               {
                  eina_strbuf_append_char(txt, text[0]);
                  advance = 1;
               }
             else
               advance = _text_escape(txt, text);
          }
        else
          advance = _text_escape(txt, text);

        text += advance;
     }

   eina_array_free(arr);
   ret = eina_strbuf_string_steal(txt);
   eina_strbuf_free(txt);
   return ret;
}

#define POPUP_LIMIT 7
static int popups_displayed = 0;

/* Util function protos */
static void _notification_format_message(Popup_Data *popup);

static int next_pos = 0;

static Eina_Bool
_notification_timer_cb(Popup_Data *popup)
{
   _notification_popup_del(e_notification_id_get(popup->notif),
                           E_NOTIFICATION_CLOSED_EXPIRED);
   return EINA_FALSE;
}

int
notification_popup_notify(E_Notification *n,
                          unsigned int    replaces_id,
                          const char     *appname __UNUSED__)
{
   double timeout;
   Popup_Data *popup = NULL;
   
   char *esc;
   char urgency;

   urgency = e_notification_hint_urgency_get(n);

   switch (urgency)
     {
      case E_NOTIFICATION_URGENCY_LOW:
        if (!notification_cfg->show_low) return 0;
        if (e_config->mode.presentation) return 0;
        break;
      case E_NOTIFICATION_URGENCY_NORMAL:
        if (!notification_cfg->show_normal) return 0;
        if (e_config->mode.presentation) return 0;
        break;
      case E_NOTIFICATION_URGENCY_CRITICAL:
        if (!notification_cfg->show_critical) return 0;
        break;
      default:
        break;
     }

   if (notification_cfg->ignore_replacement) replaces_id = 0;

   esc = _nedje_text_escape(e_notification_body_get(n));
   e_notification_body_set(n, esc);
   free(esc);

   if (replaces_id && (popup = _notification_popup_find(replaces_id)))
     {
        e_notification_ref(n);

        if (popup->notif)
          e_notification_unref(popup->notif);

        popup->notif = n;
         
        _notification_popup_refresh(popup);
     }
   else if (!replaces_id)
     {
        if ((popup = _notification_popup_merge(n)))
          _notification_popup_refresh(popup);
     }

   if (!popup)
     {
        popup = _notification_popup_new(n);
        
        if (!popup) return 0;
        notification_cfg->popups = eina_list_append(notification_cfg->popups, popup);
        edje_object_signal_emit(popup->theme, "notification,new", "notification");
     }
   
   if (popup->timer)
     {
        ecore_timer_del(popup->timer);
        popup->timer = NULL;
     }

   timeout = e_notification_timeout_get(popup->notif);

   if (timeout < 0 || notification_cfg->force_timeout)
     timeout = notification_cfg->timeout;
   else timeout = (double)timeout / 1000.0;

   if (timeout > 0)
     popup->timer = ecore_timer_add(timeout, (Ecore_Task_Cb)_notification_timer_cb, popup);

   return 1;
}

int
write_history(Eina_List *popup_items)
{
   Eina_List *l = NULL;
   char str[10] = "";
   char id[10] = "";
   char dir[PATH_MAX];
   char file_path[PATH_MAX + 12];
   unsigned int i = 1;
   int ret;
   Eet_File *history_file = NULL;
   Popup_Items *items = NULL;

   snprintf(dir, sizeof(dir), "%s/notification", efreet_data_home_get());
   if (!ecore_file_exists(dir)) ecore_file_mkdir(dir);
   snprintf(file_path, sizeof(file_path), "%s/notif_list", dir); 
   printf("NOTIFY PATH %s\n", file_path);
   history_file = eet_open(file_path, EET_FILE_MODE_WRITE);

   if (history_file){
     if(!notification_cfg->popup_items) 
     {
        snprintf(str, sizeof(str), "%d", 0);
        eet_write(history_file, "ITEMS_NUM",  str, strlen(str) + 1, 0);
        ret = eet_close(history_file); 
        return ret;
      }
   EINA_LIST_FOREACH(popup_items, l, items) {
        snprintf(str, sizeof(str), "dtime%d", i);
        eet_write(history_file, str, items->item_date_time, strlen(items->item_date_time) + 1, 0);
        snprintf(str, sizeof(str), "app%d", i);
        eet_write(history_file, str, items->item_app, strlen(items->item_app) + 1, 0);
        snprintf(str, sizeof(str), "icon%d", i);
        eet_write(history_file, str, items->item_icon, strlen(items->item_icon) + 1, 0);
        snprintf(str, sizeof(str), "img%d", i);
        eet_write(history_file, str, items->item_icon_img, strlen(items->item_icon_img) + 1, 0);
        snprintf(str, sizeof(str), "title%d", i);
        eet_write(history_file, str, items->item_title, strlen(items->item_title) + 1, 0);
        snprintf(str, sizeof(str), "body%d", i);
        eet_write(history_file, str, items->item_body, strlen(items->item_body) + 1, 0);
        snprintf(str, sizeof(str), "key1%d", i);
        eet_write(history_file, str, items->item_key_1, strlen(items->item_key_1) + 1, 0);
        snprintf(str, sizeof(str), "key2%d", i);
        eet_write(history_file, str, items->item_key_2, strlen(items->item_key_2) + 1, 0);
        snprintf(str, sizeof(str), "key3%d", i);
        eet_write(history_file, str, items->item_key_3, strlen(items->item_key_3) + 1, 0);
        snprintf(str, sizeof(str), "butt1%d", i);
        eet_write(history_file, str, items->item_but_1, strlen(items->item_but_1) + 1, 0);
        snprintf(str, sizeof(str), "butt2%d", i);
        eet_write(history_file, str, items->item_but_2, strlen(items->item_but_2) + 1, 0);
        snprintf(str, sizeof(str), "butt3%d", i);
        eet_write(history_file, str, items->item_but_3, strlen(items->item_but_3) + 1, 0);
        snprintf(str, sizeof(str), "id%d", i);
        eina_convert_itoa(items->notif_id, id);
        eet_write(history_file, str, id, strlen(id) + 1, 0);
        i++;
      }
   int count = eina_list_count(notification_cfg->popup_items);
   snprintf(str, sizeof(str), "%d", count);
   eet_write(history_file, "ITEMS_NUM",  str, strlen(str) + 1, 0);
   ret = eet_close(history_file); 
   }
   else {
      ERR("Unable to open notfication file");
      ret = 0;
   }
   return ret;
}

void
notification_popup_shutdown(void)
{
   Popup_Data *popup;

   EINA_LIST_FREE(notification_cfg->popups, popup)
     _notification_popdown(popup, E_NOTIFICATION_CLOSED_REQUESTED);
}

void
notification_popup_close(unsigned int id)
{
   _notification_popup_del(id, E_NOTIFICATION_CLOSED_REQUESTED);
}

static Popup_Data *
_notification_popup_merge(E_Notification *n)
{
   Eina_List *l;
   Popup_Data *popup;
   const char *str1, *str2;
   const char *body_old;
   const char *body_new;
   char *body_final;
   size_t len;

   str1 = e_notification_app_name_get(n);
   if (!str1) return NULL;

   EINA_LIST_FOREACH(notification_cfg->popups, l, popup)
     {
        if (!popup->notif) continue;
        if (!(str2 = e_notification_app_name_get(popup->notif)))
          continue;
        if (str1 == str2) break;
     }

   if (!popup)
     {
        /* printf("- no poup to merge\n"); */
        return NULL;
     }

   str1 = e_notification_summary_get(n);
   str2 = e_notification_summary_get(popup->notif);

   if (str1 && str2 && (str1 != str2))
     {
        /* printf("- summary doesn match, %s, %s\n", str1, str2); */
        return NULL;
     }

   /* TODO  p->n is not fallback alert..*/
   /* TODO  both allow merging */

   body_old = e_notification_body_get(popup->notif);
   body_new = e_notification_body_get(n);

   len = strlen(body_old);
   len += strlen(body_new);
   len += 5; /* \xE2\x80\xA9 or <PS/> */
   if (len < 65536) body_final = alloca(len + 1);
   else body_final = malloc(len + 1);
   /* Hack to allow e to include markup */
   snprintf(body_final, len + 1, "%s<ps/>%s", body_old, body_new);

   /* printf("set body %s\n", body_final); */

   e_notification_body_set(n, body_final);

   e_notification_unref(popup->notif);
   popup->notif = n;
   e_notification_ref(popup->notif);
   if (len >= 65536) free(body_final);

   return popup;
}


static void
_notification_theme_cb_deleted(Popup_Data *popup,
                               Evas_Object *obj __UNUSED__,
                               const char  *emission __UNUSED__,
                               const char  *source __UNUSED__)
{
   _notification_popup_refresh(popup);
   edje_object_signal_emit(popup->theme, "notification,new", "notification");
}

static void
_notification_theme_cb_close(Popup_Data *popup,
                             Evas_Object *obj __UNUSED__,
                             const char  *emission __UNUSED__,
                             const char  *source __UNUSED__)
{
   _notification_popup_del(e_notification_id_get(popup->notif),
                           E_NOTIFICATION_CLOSED_DISMISSED);
}

static void
_notification_theme_cb_find(Popup_Data *popup,
                            Evas_Object *obj __UNUSED__,
                            const char  *emission __UNUSED__,
                            const char  *source __UNUSED__)
{
   e_notification_daemon_signal_action_invoked(notification_cfg->daemon,
                     e_notification_id_get(popup->notif), "default");
   notification_popup_close(e_notification_id_get(popup->notif));
}

static void
_notification_button_1_cb(Popup_Data *popup,
                            Evas_Object *obj __UNUSED__,
                            const char  *emission __UNUSED__,
                            const char  *source __UNUSED__)
{
   e_notification_daemon_signal_action_invoked(notification_cfg->daemon,
                     e_notification_id_get(popup->notif), 
                     popup->act_name_1);
   notification_popup_close(e_notification_id_get(popup->notif));
   popup->reg1 = popup->reg2 = popup->reg3 = EINA_TRUE;
}

static void
_notification_button_2_cb(Popup_Data *popup,
                            Evas_Object *obj __UNUSED__,
                            const char  *emission __UNUSED__,
                            const char  *source __UNUSED__)
{
   e_notification_daemon_signal_action_invoked(notification_cfg->daemon,
                     e_notification_id_get(popup->notif), 
                     popup->act_name_2);
   notification_popup_close(e_notification_id_get(popup->notif));
   popup->reg1 = popup->reg2 = popup->reg3 = EINA_TRUE;
}

static void
_notification_button_3_cb(Popup_Data *popup,
                            Evas_Object *obj __UNUSED__,
                            const char  *emission __UNUSED__,
                            const char  *source __UNUSED__)
{
   e_notification_daemon_signal_action_invoked(notification_cfg->daemon,
                     e_notification_id_get(popup->notif), 
                     popup->act_name_3);
   notification_popup_close(e_notification_id_get(popup->notif));
   popup->reg1 = popup->reg2 = popup->reg3 = EINA_TRUE;
}

static void
_notification_button_1(Popup_Data *popup, E_Notification_Action *a)
 {
   popup->act_name_1 = strdup(e_notification_action_id_get(a));
   edje_object_signal_emit(popup->theme, "e,button1,show", "theme");
   popup->but_name_1 = strdup(e_notification_action_name_get(a));
   edje_object_part_text_set(popup->theme, "e.text.action_1", popup->but_name_1);
   if (popup->reg1 == EINA_FALSE)
     edje_object_signal_callback_add(popup->theme, "notification,action_1", "",
                              (Edje_Signal_Cb)_notification_button_1_cb, popup);
}

static void
_notification_button_2(Popup_Data *popup, E_Notification_Action *a)
 {
   popup->act_name_2 = strdup(e_notification_action_id_get(a));
   edje_object_signal_emit(popup->theme, "e,button2,show", "theme");
   popup->but_name_2 = strdup(e_notification_action_name_get(a));
   edje_object_part_text_set(popup->theme, "e.text.action_2", popup->but_name_2);
   if (popup->reg2 == EINA_FALSE)
     edje_object_signal_callback_add(popup->theme, "notification,action_2", "",
                              (Edje_Signal_Cb)_notification_button_2_cb, popup);
}

static void
_notification_button_3(Popup_Data *popup, E_Notification_Action *a)
 {
   popup->act_name_3 = strdup(e_notification_action_id_get(a));
   edje_object_signal_emit(popup->theme, "e,button3,show", "theme");
   popup->but_name_3 = strdup(e_notification_action_name_get(a));
   edje_object_part_text_set(popup->theme, "e.text.action_3", popup->but_name_3);
   if (popup->reg3 == EINA_FALSE)
     edje_object_signal_callback_add(popup->theme, "notification,action_3", "",
                              (Edje_Signal_Cb)_notification_button_3_cb, popup);
}

static void
_notification_actions(Popup_Data *popup)
{
   Eina_List *k, *l;
   E_Notification_Action *a;
   int act = 0, act_num;
   
   k = e_notification_actions_get(popup->notif);
   act_num = eina_list_count(k);
   popup->act_numbers = act_num;

   // hide all 3 action buttons
   edje_object_signal_emit(popup->theme, "e,button*,hide", "theme");

   EINA_LIST_FOREACH(k, l, a)
     {
        act++;
        if (act_num == 1)
          {
              _notification_button_2(popup, a);
          }

        if (act_num == 2)
          {
            switch (act)
              {
                case 1:
                  _notification_button_1(popup, a);
                 break;
                case 2:
                  _notification_button_3(popup, a);
                 break;
              }
          }
 
        if (act_num == 3)
          {
            switch (act)
              {
                case 1:
                  _notification_button_1(popup, a);
                 break;
                case 2:
                  _notification_button_2(popup, a);
                 break;
                case 3:
                  _notification_button_3(popup, a);
                 break;
              }
          }
     }  
}

static Popup_Data *
_notification_popup_new(E_Notification *n)
{
   E_Container *con;
   Popup_Data *popup;
  
   const Eina_List *l, *screens;
   E_Screen *scr;
   E_Zone *zone = NULL;
   
   if (popups_displayed > POPUP_LIMIT) return 0;
   popup = E_NEW(Popup_Data, 1);
   if (!popup) return NULL;
   e_notification_ref(n);
   popup->notif = n;

   con = e_container_current_get(e_manager_current_get());
   screens = e_xinerama_screens_get();
   if (notification_cfg->dual_screen &&
       ((notification_cfg->corner == CORNER_BR) ||
       (notification_cfg->corner == CORNER_TR)))
     l = eina_list_last(screens);
   else
     l = screens;
   if (l)
     {
        scr = eina_list_data_get(l);
        EINA_SAFETY_ON_NULL_GOTO(scr, error);
        EINA_LIST_FOREACH(con->zones, l, zone)
          if ((int)zone->num == scr->screen) break;
        if (zone && ((int)zone->num != scr->screen)) goto error;
     }
   if (!zone)
     zone = e_zone_current_get(con);
   popup->zone = zone;

   /* Create the popup window */
   popup->win = e_popup_new(zone, 0, 0, 0, 0);
   //edje_object_signal_emit((Evas_Object *) popup->win, "e,state,shadow,off", "e");
   e_popup_name_set(popup->win, "_e_popup_notification");
   popup->e = popup->win->evas;

   /* Setup the theme */
   popup->theme = edje_object_add(popup->e);
   e_theme_edje_object_set(popup->theme,
                           "base/theme/modules/notification",
                           "e/modules/notification/main");

   e_popup_edje_bg_object_set(popup->win, popup->theme);

   evas_object_show(popup->theme);
   edje_object_signal_callback_add
     (popup->theme, "notification,deleted", "theme",
     (Edje_Signal_Cb)_notification_theme_cb_deleted, popup);
   edje_object_signal_callback_add
     (popup->theme, "notification,close", "theme",
     (Edje_Signal_Cb)_notification_theme_cb_close, popup);
   edje_object_signal_callback_add
     (popup->theme, "notification,find", "theme",
     (Edje_Signal_Cb)_notification_theme_cb_find, popup);

   popup->reg1 = popup->reg2 = popup->reg3 = EINA_FALSE;

   _notification_popup_refresh(popup);
   next_pos = _notification_popup_place(popup, next_pos);

   e_popup_show(popup->win);
   e_popup_layer_set(popup->win, E_LAYER_POPUP);
   
   popups_displayed++;
   
   return popup;
error:
   free(popup);
   e_notification_unref(n);
   return NULL;
}

static int
_notification_popup_place(Popup_Data *popup,
                          int         pos)
{
   int w, h, sw, sh;
   int gap = 10;
   int to_edge = 15;

   sw = popup->zone->w;
   sh = popup->zone->h;
   evas_object_geometry_get(popup->theme, NULL, NULL, &w, &h);

   /* XXX for now ignore placement requests */

   switch (notification_cfg->corner)
     {
      case CORNER_TL:
        e_popup_move(popup->win,
                     to_edge, to_edge + pos);
        break;
      case CORNER_TR:
        e_popup_move(popup->win,
                     sw - (w + to_edge),
                     to_edge + pos);
        break;
      case CORNER_BL:
        e_popup_move(popup->win,
                     to_edge,
                     (sh - h) - (to_edge + pos));
        break;
      case CORNER_BR:
        e_popup_move(popup->win,
                     sw - (w + to_edge),
                     (sh - h) - (to_edge + pos));
        break;
      case CORNER_R:
        e_popup_move(popup->win,
                     sw - (w + to_edge),
                     sh / 2 - h / 2 + pos);
        break;
      case CORNER_T:
        e_popup_move(popup->win,
                     sw / 2 - w / 2,
                     to_edge + pos);
        break;
      case CORNER_B:
        e_popup_move(popup->win,
                     sw / 2 - w / 2,
                     (sh - h) - (to_edge + pos));
        break;
      case CORNER_L:
        e_popup_move(popup->win,
                     to_edge,
                     sh / 2 - h / 2 + pos);
        break;
      default:
        break;
     }
   return pos + h + gap;
}

static void
_notification_popups_place(void)
{
   Popup_Data *popup;
   Eina_List *l;
   int pos = 0;

   EINA_LIST_FOREACH(notification_cfg->popups, l, popup)
     {
        pos = _notification_popup_place(popup, pos);
     }
   next_pos = pos;
}

static char *
get_time()
{
   time_t rawtime;
   struct tm * timeinfo;
   char buf[64] = "";
   char hour[32];
   time(&rawtime);
   timeinfo = localtime( &rawtime );
   
   if (timeinfo->tm_hour < 10) 
     snprintf(hour, sizeof(hour), "0%d", timeinfo->tm_hour); 
   else
     snprintf(hour, sizeof(hour), "%d", timeinfo->tm_hour); 
     
   snprintf(buf, sizeof(buf), "%04d-%02d-%02d %s:%02d:%02d ", timeinfo->tm_year + 1900, 
            timeinfo->tm_mon, timeinfo->tm_mday, hour, timeinfo->tm_min, timeinfo->tm_sec);
   return strdup(buf);
}

static void
_notification_popup_refresh(Popup_Data *popup)
{
   const char *icon_path;
   const char *app_icon_max;
   void *img;
   int w, h, width = 80, height = 80;
 
   if (!popup) return;

   popup->app_name = e_notification_app_name_get(popup->notif);

   if (popup->app_icon)
     {
        evas_object_del(popup->app_icon);
        popup->app_icon = NULL;
     }

   app_icon_max = edje_object_data_get(popup->theme, "app_icon_max");
   if (app_icon_max)
     {
        char *endptr;

        errno = 0;
        width = strtol(app_icon_max, &endptr, 10);
        if (errno || (width < 1) || (endptr == app_icon_max))
          {
             width = 80;
             height = 80;
          }
        else
          {
             if (!endptr[0])
               height = width;
             else
               {
                  endptr++;
                  if (endptr[0])
                    {
                       height = strtol(endptr, NULL, 10);
                       if (errno || (height < 1)) height = width;
                    }
                  else height = width;
               }
          }
     }

   /* Check if the app specify an icon either by a path or by a hint */
   img = e_notification_hint_image_data_get(popup->notif);
   if (!img)
     {  
        icon_path = e_notification_hint_image_path_get(popup->notif);
           
        if ((!icon_path) || (!icon_path[0])){
          icon_path = e_notification_app_icon_get(popup->notif);
        }
        if (icon_path)
          {
             if (!strncmp(icon_path, "file://", 7)) icon_path += 7;
             
             /* Grab icon stored in /tmp and copy to notif dir */
            if (notification_cfg->instances){
               char dir[PATH_MAX];
               const char *file;
               file = ecore_file_file_get(icon_path);
               if (*file == '.') file = file + 1;
               snprintf(dir, sizeof(dir), "%s/notification/%s.png", efreet_data_home_get(),file); 
               if (!notification_cfg->clicked_item) 
                  ecore_file_cp(icon_path, dir); 
                
               popup->app_icon_image = strdup(dir);
             }
             /*                                                 */
             
             if (!ecore_file_exists(icon_path))
               {
                  const char *new_path;
                  unsigned int size;
                  popup->app_icon_image = strdup(icon_path);
                  size = e_util_icon_size_normalize(width * e_scale);
                  new_path = efreet_icon_path_find(e_config->icon_theme, 
                                                   icon_path, size);
                  if (new_path)
                    icon_path = new_path;
                  else
                    {
                       Evas_Object *o = e_icon_add(popup->e);
                       if (!e_util_icon_theme_set(o, icon_path)) evas_object_del(o);
                       else
                         {
                            popup->app_icon = o;
                            w = width;
                            h = height;
                         }
                    }
               }

             if (!popup->app_icon)
               {
                  popup->app_icon = e_icon_add(popup->e);
                  if (!e_icon_file_set(popup->app_icon, icon_path))
                    {
                       evas_object_del(popup->app_icon);
                       popup->app_icon = NULL;
                    }
                  else e_icon_size_get(popup->app_icon, &w, &h);
               }
          }
     }
   if ((!img) && (!popup->app_icon)){
     img = e_notification_hint_icon_data_get(popup->notif);
     }
   if (img)
     {
        char dir[PATH_MAX];
        
        popup->app_icon = e_notification_image_evas_object_add(popup->e, img);
        evas_object_image_filled_set(popup->app_icon, EINA_TRUE);
        evas_object_image_alpha_set(popup->app_icon, EINA_TRUE);
        evas_object_image_size_get(popup->app_icon, &w, &h);
        
        if (notification_cfg->instances){
          snprintf(dir, sizeof(dir), "%s/notification", efreet_data_home_get()); 
          if (!ecore_file_exists(dir)) ecore_file_mkdir(dir);

          int n = snprintf(0, 0, "%s/%s_%s.png", dir,
                   e_notification_summary_get(popup->notif), get_time());
          if (n < 0)
            {
              perror ("snprintf failed");
              abort ();
            }

          char *image_path = (char *) malloc(n + 1);
          snprintf(image_path, n + 1, "%s/%s_%s.png", dir,
                   e_notification_summary_get(popup->notif), get_time()); 
          if (n < 0)
            {
              perror ("snprintf failed");
              abort ();
            }
          if (!strstr(notification_cfg->blacklist, popup->app_name))
            evas_object_image_save(popup->app_icon, image_path, NULL, NULL);
          popup->app_icon_image = strdup(image_path);
          free (image_path);
        }
     }

   if (!popup->app_icon)
     {
        char buf[PATH_MAX];
        
        snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", 
                 notification_mod->dir);
        popup->app_icon = edje_object_add(popup->e);
        if (!e_theme_edje_object_set(popup->app_icon, 
                                       "base/theme/modules/notification",
                                       "e/modules/notification/logo"))
            edje_object_file_set(popup->app_icon, buf, 
                                 "e/modules/notification/logo");
        w = width;
        h = height;
     }

   if ((w > width) || (h > height))
     {
        int v;
        v = w > h ? w : h;
        h = h * height / v;
        w = w * width / v;
     }
   evas_object_size_hint_min_set(popup->app_icon, w, h);
   evas_object_size_hint_max_set(popup->app_icon, w, h);

   edje_object_calc_force(popup->theme);
   edje_object_part_swallow(popup->theme, "notification.swallow.app_icon", 
                            popup->app_icon);
   edje_object_signal_emit(popup->theme, "notification,icon", "notification");

    /* Read notification actions */
   _notification_actions(popup);

   /* Fill up the event message */
   _notification_format_message(popup);

   /* Compute the new size of the popup */
   edje_object_calc_force(popup->theme);
   edje_object_size_min_calc(popup->theme, &w, &h);
   w = MIN(w, popup->zone->w / 2);
   h = MIN(h, popup->zone->h / 2);

   if (popup->act_numbers == 0)
      h = h / 1.2;
   e_popup_resize(popup->win, w, h);
   evas_object_resize(popup->theme, w, h); 
   _notification_popups_place();
}

static Popup_Data *
_notification_popup_find(unsigned int id)
{
   Eina_List *l;
   Popup_Data *popup;

   if (!id) return NULL;
   EINA_LIST_FOREACH(notification_cfg->popups, l, popup)
     {
        if (e_notification_id_get(popup->notif) == id) return popup;
     }
   return NULL;
}

static void
_notification_popup_del(unsigned int                 id,
                        E_Notification_Closed_Reason reason)
{
   Popup_Data *popup;
   Eina_List *l, *l2;
   int pos = 0;

   EINA_LIST_FOREACH_SAFE(notification_cfg->popups, l, l2, popup)
     {
        if (e_notification_id_get(popup->notif) == id)
          {
             _notification_popdown(popup, reason);
             notification_cfg->popups = eina_list_remove_list(notification_cfg->popups, l);
          }
        else
          pos = _notification_popup_place(popup, pos);
     }

   next_pos = pos;
}

static void
_notification_popdown(Popup_Data                  *popup,
                      E_Notification_Closed_Reason reason)
{
   if (popup->timer) ecore_timer_del(popup->timer);
   e_popup_hide(popup->win);
   popups_displayed--;
   evas_object_del(popup->app_icon);
   evas_object_del(popup->theme);
   e_object_del(E_OBJECT(popup->win));
   e_notification_closed_set(popup->notif, 1);
   e_notification_daemon_signal_notification_closed
     (notification_cfg->daemon, 0, reason);
     //~ (notification_cfg->daemon, e_notification_id_get(popup->notif), reason);
   e_notification_unref(popup->notif);
   
   free(popup->act_name_1);
   free(popup->act_name_2);
   free(popup->act_name_3);
   free(popup->but_name_1);
   free(popup->but_name_2);
   free(popup->but_name_3);
   free(popup);
}

static void
_notification_format_message(Popup_Data *popup)
{
   Evas_Object *o = popup->theme;
   const char *title = e_notification_summary_get(popup->notif);
   const char *b = e_notification_body_get(popup->notif);
   edje_object_part_text_unescaped_set(o, "notification.text.title", title);
   
   /* FIXME: Filter to only include allowed markup? */
     {
        /* We need to replace \n with <br>. FIXME: We need to handle all the
         * newline kinds, and paragraph separator. ATM this will suffice. */
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append(buf, b);
        eina_strbuf_replace_all(buf, "\n", "<br/>");
        edje_object_part_text_set(o, "notification.textblock.message",
                                  eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   list_add_item (popup); 
}

static void
list_add_item(Popup_Data *popup)
{
   Popup_Items *items; 
   char *file;

   if (!notification_cfg->instances) return;

   items = E_NEW(Popup_Items, 1); 
   if (!items) return;

   const char *icon_path = e_notification_app_icon_get(popup->notif);
   const char *title = e_notification_summary_get(popup->notif);
   const char *b = e_notification_body_get(popup->notif);

   items->item_date_time = get_time();         // add date and time
   items->item_app = strdup(popup->app_name);  // add app name
   items->item_title = strdup(title);          // add title
   items->item_body = strdup(b);               // add text body

   if (strstr(icon_path, "tmp"))               // add icon path
     items->item_icon = strdup(""); 
   else
     items->item_icon = strdup(icon_path); 
  
   if (strlen(popup->app_icon_image) > 0)      // do we have an icon image?
     items->item_icon_img = strdup(popup->app_icon_image); 
   else
     items->item_icon_img = strdup("noimage");
  
   if (popup->act_name_1) items->item_key_1 = strdup(popup->act_name_1);
   else items->item_key_1 = strdup("");

   if (popup->act_name_2) items->item_key_2 = strdup(popup->act_name_2);
   else items->item_key_2 = strdup("");

   if (popup->act_name_3) items->item_key_3 = strdup(popup->act_name_3);
   else items->item_key_3 = strdup("");

   if (popup->but_name_1) items->item_but_1 = strdup(popup->but_name_1);
   else items->item_but_1 = strdup("");

   if (popup->but_name_2) items->item_but_2 = strdup(popup->but_name_2);
   else items->item_but_2 = strdup("");

   if (popup->but_name_3) items->item_but_3 = strdup(popup->but_name_3);
   else items->item_but_3 = strdup("");

   items->notif_id = e_notification_id_get(popup->notif);

   /* Apps blacklist check */
   if (strstr(notification_cfg->blacklist, items->item_app)) return;

   /* Add item to the menu if less then menu items limit */
   if (notification_cfg->clicked_item == EINA_FALSE)
     {
       if (notification_cfg->new_item < notification_cfg->menu_items) 
         notification_cfg->new_item++;
       if (eina_list_count(notification_cfg->popup_items) < notification_cfg->menu_items)
         notification_cfg->popup_items = eina_list_prepend(notification_cfg->popup_items, items);
       else 
         {
           file = ((Popup_Items *) eina_list_last_data_get(notification_cfg->popup_items))->item_icon_img;
           if (!ecore_file_remove(file))
             printf("Notification: Error during hint file removing!\n");
           notification_cfg->popup_items = eina_list_remove_list(notification_cfg->popup_items, 
                                           eina_list_last(notification_cfg->popup_items));
           notification_cfg->popup_items = eina_list_prepend(notification_cfg->popup_items, items);
         }
     }
 
   notification_cfg->clicked_item = EINA_FALSE;
   gadget_text(notification_cfg->new_item);
   write_history(notification_cfg->popup_items);
}
