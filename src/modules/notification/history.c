#include "e_mod_main.h"
#include "history.h"

Eina_Stringshare *
get_time(const char *delimiter)
{
   time_t rawtime;
   struct tm * timeinfo;
   char buf[64] = "";
   char hour[32];
   Eina_Stringshare *ret = NULL;
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
   if (eina_list_count(notification_cfg->history) < notification_cfg->menu_items)
     notification_cfg->history = eina_list_prepend(notification_cfg->history, items);
   else
     {
       file = ((Popup_Items *) eina_list_last_data_get(notification_cfg->history))->item_icon_img;

       if (ecore_file_exists(file))
         {
            if (!ecore_file_remove(file))
              printf("Notification: Error during hint file removing!\n");
         }
       notification_cfg->history = eina_list_remove_list(notification_cfg->history,
                                         eina_list_last(notification_cfg->history));
       notification_cfg->history = eina_list_prepend(notification_cfg->history, items);
     }

   notification_cfg->item_click = EINA_FALSE;
   gadget_text(notification_cfg->new_item, 1);
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

