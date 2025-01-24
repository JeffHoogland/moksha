#include "e_mod_main.h"
#include "history.h"

#define NOTIFICATION_MOD_NAME "notification"
#define DATA_DIR    NOTIFICATION_MOD_NAME
#define HISTORY_NAME "history"
#define HISTORY_VERSION     0 /* must be < 9  */

/* convenience macros to compress code */
#define PATH_MAX_ERR                                              \
do {                                                              \
  ERR("PATH_MAX exceeded. Need Len %d, PATH_MAX %d", len, PATH_MAX); \
  memset(path,0,PATH_MAX);                                        \
  success = EINA_FALSE;                                           \
 } while(0)                                                       \

#define HIST_ADD(member, eet_type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC             \
    (_history_descriptor, Hist_eet, # member, member, eet_type)
#define HIST_NOTIF_ADD_BASIC(member, eet_type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC                 \
    (_notif_sub_descriptor, Popup_Items, # member, member, eet_type)
#define ITEM_ACTION_ADD_BASIC(member, eet_type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC                 \
    (_notif_act_descriptor, E_Notification_Notify_Action, # member, member, eet_type)

static const char DATA_FILE_ENTRY[] = "data";
static Eet_Data_Descriptor *_history_descriptor;
static Eet_Data_Descriptor *_notif_sub_descriptor;
static Eet_Data_Descriptor *_notif_act_descriptor;


/* Private Funcions */
Eina_Bool _mkpath_if_not_exists(const char *path);
Eina_Bool _data_path(char *path);
Eina_Bool _history_path(char *path);

/**
 * @brief Creates path if non-existant
 *
 * @param path char array to create.
 * @return EINA_TRUE on success EINA_FALSE otherwise
 *
 */
Eina_Bool
_mkpath_if_not_exists(const char *path)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

   Eina_Bool success = EINA_TRUE;

   if(!ecore_file_exists(path))
      return ecore_file_mkdir(path);
   return success;
}

/**
 * @brief Sets the XDG_DATA_HOME location for saving Notification history data.
 *
 * @param path char array to store history path in.
 * @return EINA_TRUE on success EINA_FALSE otherwise
 *
 */
Eina_Bool
_data_path(char *path)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

   const char *temp_str = efreet_data_home_get();
   if (!temp_str)
      // Should never happen
      return EINA_FALSE;
   Eina_Bool success = EINA_TRUE;
   const int len = snprintf(NULL, 0, "%s", temp_str)
                             + 1 + (temp_str[strlen(temp_str)-1] != '/');
   if (temp_str[0] == '/' ) {

     if (len <= PATH_MAX) {
       snprintf(path, strlen(temp_str)+1, "%s", temp_str);
       // Ensure XDG_DATA_HOME terminates in '/'
       if (path[strlen(path)-1] != '/')
         strncat(path, "/", PATH_MAX-strlen(path)-1);
     }
     else
       PATH_MAX_ERR;
   }
   else
     PATH_MAX_ERR;

   return success;
}

/**
 * @brief Sets the path and file name for saving notification history data.
 *
 * @param history_path char array to store history path in.
 * @return EINA_TRUE on success EINA_FALSE otherwise
 *
 * This function not only sets the history path but also creates the needed
 * directories if they do not exist.
 *
 */
Eina_Bool
_history_path(char *path)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

   char temp_str[PATH_MAX] = {0};
   Eina_Bool success = EINA_TRUE;

   if(_data_path(path)) {
       const int len = snprintf(NULL, 0, "%s%s/%s", path, NOTIFICATION_MOD_NAME, HISTORY_NAME) + 1;
       if (len <= PATH_MAX) {
         strncpy(temp_str, path, PATH_MAX-1);
         int ret = snprintf(path, PATH_MAX, "%s%s/", temp_str, NOTIFICATION_MOD_NAME);
         if (ret < 0) PATH_MAX_ERR;
         success = _mkpath_if_not_exists(path);
         strncat(path, HISTORY_NAME, PATH_MAX-strlen(path)-1);
       }
       else
        PATH_MAX_ERR;
   } else
       success = EINA_FALSE;

   return success;
}

static void
_history_descriptor_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Hist_eet);
   _history_descriptor = eet_data_descriptor_stream_new(&eddc);

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Popup_Items);
   _notif_sub_descriptor = eet_data_descriptor_stream_new(&eddc);

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, E_Notification_Notify_Action);
   _notif_act_descriptor = eet_data_descriptor_stream_new(&eddc);

   HIST_ADD(version, EET_T_UINT);
   // Popup_Items struct
   HIST_NOTIF_ADD_BASIC(item_date_time, EET_T_STRING);
   HIST_NOTIF_ADD_BASIC(item_app,       EET_T_STRING);
   HIST_NOTIF_ADD_BASIC(item_icon,      EET_T_STRING);
   HIST_NOTIF_ADD_BASIC(item_icon_img,  EET_T_STRING);
   HIST_NOTIF_ADD_BASIC(item_title,     EET_T_STRING);
   HIST_NOTIF_ADD_BASIC(item_body,      EET_T_STRING);
   HIST_NOTIF_ADD_BASIC(notif_id,       EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_LIST
     (_notif_sub_descriptor, Popup_Items, "actions", actions, _notif_act_descriptor);

   // E_Notification_Notify_Action struct
   ITEM_ACTION_ADD_BASIC(action,        EET_T_STRING);
   ITEM_ACTION_ADD_BASIC(label,         EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST
     (_history_descriptor, Hist_eet, "history", history, _notif_sub_descriptor);
}

Hist_eet *
history_init(void)
{
   Hist_eet    *hist = E_NEW(Hist_eet, 1);
   char path[PATH_MAX] = {0};

   _history_descriptor_init();
   if (_history_path(path) &&  ecore_file_exists(path))
     hist = load_history(path);
   else
     {
        hist = E_NEW(Hist_eet, 1);
        hist->version = HISTORY_VERSION;
        // fixme: hist->path = ??
     }
   hist->path=strdup(path);
   // printf("Notify hist init %s\n", hist->path);
   return hist;
}

Eina_Stringshare *
get_time(const char *delimiter)
{
   time_t rawtime;
   struct tm * timeinfo;
   char buf[64] = "";
   char hour[32];
   Eina_Stringshare *ret=NULL;
   time(&rawtime);
   timeinfo = localtime( &rawtime );

   // FIXME: use eina_convert stuff
   snprintf(hour, sizeof(hour), "%02d", timeinfo->tm_hour);

   snprintf(buf, sizeof(buf), "%04d-%02d-%02d %s%s%02d%s%02d", timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1, timeinfo->tm_mday, hour, delimiter, timeinfo->tm_min,
            delimiter, timeinfo->tm_sec);

   ret = eina_stringshare_add(buf);

   return ret;
}

void
list_add_item(Popup_Data *popup)
{
   EINA_SAFETY_ON_NULL_RETURN(popup);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg);
   EINA_SAFETY_ON_NULL_RETURN(notification_cfg->hist);
   PRINT("NOTIFY LIST ITEM ADD \n");
   /* Apps blacklist check */
   if (strstr(notification_cfg->blacklist, popup->app_name)) return;

   Popup_Items *items;
   Eina_Stringshare *file;
   E_Notification_Notify_Action  *act;

   if (!notification_cfg->instances) return;
   items = E_NEW(Popup_Items, 1);
   if (!items) return;
   
   items->item_date_time = get_time(":");
   items->item_app = eina_stringshare_add(popup->app_name);
   items->item_title = eina_stringshare_add(popup->notif->summary);
   items->item_body = eina_stringshare_add(popup->notif->body);
   items->notif_id = notification_cfg->next_id;
   items->notif = popup->notif;

   if (strstr(popup->notif->icon.icon, "tmp"))
     items->item_icon = eina_stringshare_add("");
   else
     items->item_icon = eina_stringshare_add(popup->notif->icon.icon);

   if (popup->app_icon_image && popup->app_icon_image[0])      // do we have an icon image?
     items->item_icon_img = eina_stringshare_add(popup->app_icon_image);
   else
     items->item_icon_img = eina_stringshare_add("noimage");

   if (popup->notif->actions)
     {
       for (int i = 0; popup->notif->actions[i].action; i++)
          {
             act = E_NEW(E_Notification_Notify_Action, 1);
             act->action = eina_stringshare_add(popup->notif->actions[i].action);
             act->label  = eina_stringshare_add(popup->notif->actions[i].label);
             items->actions = eina_list_append(items->actions, act);
          }
     }
    // f = e_theme_edje_file_get("base/theme/modules/notification",  "e/modules/notification/logo");
    // PRINT("%s\n", f);

   if (notification_cfg->new_item < notification_cfg->menu_items)
     notification_cfg->new_item++;
     
   /* Add item to the menu if less than menu items limit */
   if (eina_list_count(notification_cfg->hist->history) < notification_cfg->menu_items)
     notification_cfg->hist->history = eina_list_prepend(notification_cfg->hist->history, items);
   else
     {
       file = ((Popup_Items *) eina_list_last_data_get(notification_cfg->hist->history))->item_icon_img;

       if (ecore_file_exists(file))
         {
            if (!ecore_file_remove(file))
              printf("Notification: Error during hint file removing!\n");
         }
       notification_cfg->hist->history = eina_list_remove_list(notification_cfg->hist->history,
                                         eina_list_last(notification_cfg->hist->history));
       notification_cfg->hist->history = eina_list_prepend(notification_cfg->hist->history, items);
     }

   notification_cfg->item_click = EINA_FALSE;
   gadget_text(notification_cfg->new_item, 1);
   store_history(notification_cfg->hist);
}

void
popup_items_free(Popup_Items *items)
{
   EINA_SAFETY_ON_NULL_RETURN(items);

   if (items->actions)
     {
        Eina_List *l;
        Eina_List *l_next;
        E_Notification_Notify_Action *act;


       EINA_LIST_FOREACH_SAFE(items->actions, l, l_next, act)
        {
           eina_stringshare_del(act->label);
           eina_stringshare_del(act->action);
           free(act);
           items->actions = eina_list_remove_list(items->actions, l);
        }
      eina_list_free(items->actions);
    }
   eina_stringshare_del(items->item_date_time);
   eina_stringshare_del(items->item_app);
   eina_stringshare_del(items->item_icon);
   eina_stringshare_del(items->item_icon_img);
   eina_stringshare_del(items->item_title);
   eina_stringshare_del(items->item_body);
   E_FREE(items->notif);

   E_FREE(items);
}

Hist_eet *
load_history(const char *filename)
{
   Hist_eet * hist = NULL;
   Eet_File *ef = eet_open(filename, EET_FILE_MODE_READ);
   if (!ef)
     {
        fprintf(stderr, "ERROR: could not open '%s' for read\n", filename);
        return NULL;
     }

   hist = eet_data_read(ef, _history_descriptor, DATA_FILE_ENTRY);
   if (!hist)
     goto end;

   if (hist->version != HISTORY_VERSION)
     {
        fprintf(stderr,
                "WARNING: version %#x was too old, upgrading it to %#x\n",
                hist->version, HISTORY_VERSION);

       hist->version = HISTORY_VERSION;
     }

end:
   eet_close(ef);
   return hist;
}

Eina_Bool
store_history(const Hist_eet *hist)
{
   Eina_Bool ret;
   Eet_File *ef;
   char *filename = hist->path;
   ef = eet_open(filename, EET_FILE_MODE_WRITE);
   if (!ef)
     {
        fprintf(stderr, "ERROR: could not open '%s' for write\n", filename);
        return EINA_FALSE;
     }
   ret = eet_data_write(ef, _history_descriptor, DATA_FILE_ENTRY, hist, EINA_TRUE);
   eet_close(ef);
   return ret;
}

#undef HIST_ADD
#undef HIST_NOTIF_ADD_BASIC
#undef ITEM_ACTION_ADD_BASIC
