#include "e.h"

typedef struct _Notification_Data
{
   Eldbus_Connection                *conn;
   Eldbus_Service_Interface         *iface;
   E_Notification_Notify_Cb          notify_cb;
   E_Notification_Close_Cb           close_cb;
   void                             *data;
   const E_Notification_Server_Info *server_info;
} Notification_Data;

static Notification_Data *n_data = NULL;

static void
_notification_free(E_Notification_Notify *notify)
{
   int i;

   EINA_SAFETY_ON_NULL_RETURN(notify);
   if (notify->app_name) eina_stringshare_del(notify->app_name);
   if (notify->body) eina_stringshare_del(notify->body);
   if (notify->icon.icon) eina_stringshare_del(notify->icon.icon);
   if (notify->icon.icon_path) eina_stringshare_del(notify->icon.icon_path);
   if (notify->summary) eina_stringshare_del(notify->summary);
   if (notify->icon.raw.data) free(notify->icon.raw.data);
   if (notify->category) eina_stringshare_del(notify->category);
   if (notify->desktop_entry) eina_stringshare_del(notify->desktop_entry);
   if (notify->sound_file) eina_stringshare_del(notify->sound_file);
   if (notify->sound_name) eina_stringshare_del(notify->sound_name);
   if (notify->actions)
     {
        for (i = 0; notify->actions[i].action; i++)
          {
             eina_stringshare_del(notify->actions[i].action);
             eina_stringshare_del(notify->actions[i].label);
          }
        free(notify->actions);
     }
   free(notify);
}

static void
hints_dict_iter(void *data, const void *key, Eldbus_Message_Iter *var)
{
   E_Notification_Notify *n = data;

   if ((!strcmp(key, "image-data")) || (!strcmp(key, "image_data")) ||
       (!strcmp(key, "icon_data")))
     {
        Eldbus_Message_Iter *st, *data_iter;
        int w, h, r, bits, channels;
        Eina_Bool alpha;
        unsigned char *raw_data;

        if (!eldbus_message_iter_arguments_get(var, "(iiibiiay)", &st))
          return;
        if (!eldbus_message_iter_arguments_get(st, "iiibiiay", &w, &h, &r,
                                              &alpha, &bits, &channels,
                                              &data_iter))
          return;
        eldbus_message_iter_fixed_array_get(data_iter, 'y', &raw_data,
                                           &n->icon.raw.data_size);
        n->icon.raw.width = w;
        n->icon.raw.height = h;
        n->icon.raw.has_alpha = alpha;
        n->icon.raw.rowstride = r;
        n->icon.raw.bits_per_sample = bits;
        n->icon.raw.channels = channels;
        n->icon.raw.data = malloc(sizeof(char) * n->icon.raw.data_size);
        EINA_SAFETY_ON_NULL_RETURN(n->icon.raw.data);
        memcpy(n->icon.raw.data, raw_data, sizeof(char) * n->icon.raw.data_size);
        printf("NOT: image-data=%ix%i,a=%i\n", w, h, alpha);
     }
   else if (!strcmp(key, "image-path") || !strcmp(key, "image_path"))
     {
        eldbus_message_iter_arguments_get(var, "s", &n->icon.icon_path);
        n->icon.icon_path = eina_stringshare_add(n->icon.icon_path);
        printf("NOT: image-path=[%s]\n", n->icon.icon_path);
        // path to image file
     }
   else if (!strcmp(key, "urgency"))
     {
        unsigned char urgency;

        eldbus_message_iter_arguments_get(var, "y", &urgency);
        if (urgency < 3) n->urgency = urgency;
        printf("NOT: urgency=%i\n", n->urgency);
        // 0=low, 1=normal, 2=critical
     }
   else if (!strcmp(key, "category"))
     { // XXX: store category
        const char *val = NULL;

        eldbus_message_iter_arguments_get(var, "s", &val);
        printf("NOT: category=[%s]\n", val);
        // "device"               A generic device-related notification that doesn't fit into any other category.
        // "device.added"         A device, such as a USB device, was added to the system.
        // "device.error"         A device had some kind of error.
        // "device.removed"       A device, such as a USB device, was removed from the system.
        // "email"                A generic e-mail-related notification that doesn't fit into any other category.
        // "email.arrived"        A new e-mail notification.
        // "email.bounced"        A notification stating that an e-mail has bounced.
        // "im"                   A generic instant message-related notification that doesn't fit into any other category.
        // "im.error"             An instant message error notification.
        // "im.received"          A received instant message notification.
        // "network"              A generic network notification that doesn't fit into any other category.
        // "network.connected"    A network connection notification, such as successful sign-on to a network service. This should not be confused with device.added for new network devices.
        // "network.disconnected" A network disconnected notification. This should not be confused with device.removed for disconnected network devices.
        // "network.error"        A network-related or connection-related error.
        // "presence"             A generic presence change notification that doesn't fit into any other category, such as going away or idle.
        // "presence.offline"     An offline presence change notification.
        // "presence.online"      An online presence change notification.
        // "transfer"             A generic file transfer or download notification that doesn't fit into any other category.
        // "transfer.complete"    A file transfer or download complete notification.
        // "transfer.error"       A file transfer or download error.
        if (val) n->category = eina_stringshare_add(val);
     }
   else if (!strcmp(key, "desktop-entry"))
     {
        const char *val = NULL;

        eldbus_message_iter_arguments_get(var, "s", &val);
        printf("NOT: desktop-entry=[%s]\n", val);
        // if rage.desktop -> "rage"
        // if terminology.desktop -> "terminology"
        if (val) n->desktop_entry = eina_stringshare_add(val);
     }
   else if (!strcmp(key, "icon-actions"))
     {
        Eina_Bool val = 0;

        eldbus_message_iter_arguments_get(var, "b", &val);
        printf("NOT: icon-actions=%i\n", val);
        // 1 == interpret action identifier == named icon in icon naming standards
        n->icon_actions = val;
     }
   else if (!strcmp(key, "resident"))
     {
        Eina_Bool val = 0;

        eldbus_message_iter_arguments_get(var, "b", &val);
        printf("NOT: resident=%i\n", val);
        // 1== remove  notification when action invoked - no timeout
        n->resident = val;
     }
   else if (!strcmp(key, "suppress-sound"))
     {
        Eina_Bool val = 0;

        eldbus_message_iter_arguments_get(var, "b", &val);
        printf("NOT: suppress-sound=%i\n", val);
        // 1== remove  notification when action invoked - no timeout
        n->suppress_sound = val;
     }
   else if (!strcmp(key, "sound-file"))
     {
        const char *val = NULL;

        eldbus_message_iter_arguments_get(var, "s", &val);
        printf("NOT: sound-file=[%s]\n", val);
        // path to sound file to play
        if (val) n->sound_file = eina_stringshare_add(val);
     }
   else if (!strcmp(key, "sound-name"))
     {
        const char *val = NULL;

        eldbus_message_iter_arguments_get(var, "s", &val);
        printf("NOT: sound-file=[%s]\n", val);
        // sound naming spec to play
        // http://0pointer.de/public/sound-naming-spec.html
        if (val) n->sound_name = eina_stringshare_add(val);
     }
   else if (!strcmp(key, "transient"))
     {
        Eina_Bool val = 0;

        eldbus_message_iter_arguments_get(var, "b", &val);
        printf("NOT: transient=%i\n", val);
        n->transient = val;
     }
   else if (!strcmp(key, "x"))
     {
        int val = 0;

        eldbus_message_iter_arguments_get(var, "i", &val);
        printf("NOT: x=%i\n", val);
        n->x = val;
        n->have_xy = EINA_TRUE;
     }
   else if (!strcmp(key, "y"))
     {
        int val = 0;

        eldbus_message_iter_arguments_get(var, "i", &val);
        printf("NOT: y=%i\n", val);
        n->y = val;
        n->have_xy = EINA_TRUE;
     }
}

static int
_tag_len(const char *txt)
{
   const char *s;
   Eina_Bool backslash = EINA_FALSE;
   Eina_Bool inquote = EINA_FALSE, indblquote = EINA_FALSE;

   if (txt[0] != '<') return 0;
   for (s = txt; *s; s++)
     {
        if (!backslash)
          {
             if (*s == '\\') backslash = EINA_TRUE;
             else
               {
                  if (inquote)
                    {
                       if (*s == '\'') inquote = EINA_FALSE;
                    }
                  else if (indblquote)
                    {
                       if (*s == '"') indblquote = EINA_FALSE;
                    }
                  else
                    {
                       if (*s == '>')
                         {
                            s++;
                            break;
                         }
                       else if (*s == '\'') inquote = EINA_TRUE;
                       else if (*s == '\"') indblquote = EINA_TRUE;
                    }
               }
          }
        else backslash = EINA_FALSE;
     }
   return s - txt;
}

static char *
_path_get(const char *txt)
{
   Eina_Strbuf *buf;
   char *ret;
   const char *s;
   Eina_Bool backslash = EINA_FALSE;
   Eina_Bool inquote = EINA_FALSE, indblquote = EINA_FALSE;

   if (txt[0] == '>') return NULL;

   buf = eina_strbuf_new();
   if (!buf) return NULL;

   for (s = txt; *s; s++)
     {
        if (!backslash)
          {
             if (*s == '\\') backslash = EINA_TRUE;
             else
               {
                  if (inquote)
                    {
                       if (*s == '\'') inquote = EINA_FALSE;
                       else eina_strbuf_append_char(buf, *s);
                    }
                  else if (indblquote)
                    {
                       if (*s == '"') indblquote = EINA_FALSE;
                       else eina_strbuf_append_char(buf, *s);
                    }
                  else
                    {
                       if      (*s == '>') break;
                       else if (*s == ' ') break;
                       else if (*s == '\'') inquote = EINA_TRUE;
                       else if (*s == '\"') indblquote = EINA_TRUE;
                       else eina_strbuf_append_char(buf, *s);
                    }
               }
          }
        else
          {
             eina_strbuf_append_char(buf, *s);
             backslash = EINA_FALSE;
          }
     }
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static char *
_nedje_text_escape(const char *text)
{
   Eina_Strbuf *txt;
   char *ret;
   const char *text_end;
   int taglen;

   if (!text) return NULL;

   txt = eina_strbuf_new();
   if (!txt) return NULL;

   text_end = text + strlen(text);

   while (text < text_end)
     {
        taglen = _tag_len(text);
        if (taglen == 0)
          {
             eina_strbuf_append_char(txt, text[0]);
             text++;
          }
        else
          {
             if      (!strncmp(text, "<b>", 3))  eina_strbuf_append(txt, "<b>");
             else if (!strncmp(text, "</b>", 4)) eina_strbuf_append(txt, "</b>");
             else if (!strncmp(text, "<i>", 3))  eina_strbuf_append(txt, "<i>");
             else if (!strncmp(text, "</i>", 4)) eina_strbuf_append(txt, "</i>");
             else if (!strncmp(text, "<u>", 3))  eina_strbuf_append(txt, "<u>");
             else if (!strncmp(text, "</u>", 4)) eina_strbuf_append(txt, "</u>");
             else if (!strncmp(text, "<a ", 3))
               {
                  eina_strbuf_append(txt, "<link>");
                  eina_strbuf_append_n(txt, text, taglen);
               }
             else if (!strncmp(text, "</a>", 3))
               {
                  eina_strbuf_append(txt, "</a></link>");
               }
             else if (!strncmp(text, "<img src=", 9))
               {
                  Evas_Object *o;
                  int w = 0, h = 0;
                  char *path;
                  // need a temporary evas for  querying image properties from it.
                  // FIXME: prob an easier way to do this
                  E_Container *con = e_container_current_get(e_manager_current_get());
                  E_Zone     *zone = e_zone_current_get(con);
                  E_Popup    *temp = e_popup_new(zone, 0, 0, 0, 0);

                  path = _path_get(text + 9);
                  if ((path) && (strlen(path) > 0))
                    {
                       o = evas_object_image_add(temp->evas);
                       evas_object_image_file_set(o, path, NULL);
                       evas_object_image_size_get(o, &w, &h);
                       printf("NOT: imgpath=%s %ix%i\n", path, w, h);
                       if ((w > 0) && (h > 0))
                         {
                            double neww = w, newh = h;

                            if (neww > 200.0)
                              {
                                 double oldw = neww;

                                 neww = 200.0;
                                 newh = (newh * neww) / oldw;
                              }
                            if (newh > 100.0)
                              {
                                 double oldh = newh;

                                 newh = 100.0;
                                 neww = (neww * newh) / oldh;
                              }
                            neww *= e_scale;
                            newh *= e_scale;
                            w = neww + 0.5;
                            h = newh + 0.5;
                            eina_strbuf_append_printf
                              (txt, "<item absize=%ix%i href=", w, h);
                            eina_strbuf_append_n(txt, text + 9, taglen - 9);
                            eina_strbuf_append(txt, "</item>");
                         }
                       evas_object_del(o);
                    }
                  free(path);
                  e_object_del(E_OBJECT(temp));
               }
             text += taglen;
          }
     }

   ret = eina_strbuf_string_steal(txt);
   printf("NOT: body -> [%s]\n", ret);
   eina_strbuf_free(txt);
   return ret;
}

static Eldbus_Message *
notify_cb(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   E_Notification_Notify *n;
   Eldbus_Message_Iter *actions_iter, *hints_iter;
   Eldbus_Message *reply;
   char *txt, *txt2;
   int num;

   if (!n_data->notify_cb) return NULL;

   n = E_OBJECT_ALLOC(E_Notification_Notify, E_NOTIFICATION_TYPE, _notification_free);
   n->urgency = E_NOTIFICATION_NOTIFY_URGENCY_NORMAL;
   if (!eldbus_message_arguments_get(msg, "susssasa{sv}i",
                                     &n->app_name, &n->replaces_id,
                                     &n->icon.icon, &n->summary, &n->body,
                                     &actions_iter, &hints_iter, &n->timeout))
     {
        ERR("Reading message.");
        e_object_del(E_OBJECT(n));
        return NULL;
     }
   /*if (e_screensaver_on_get() && e_config->screensaver_wake_on_notify)
     { // XXX: this is an attempt to wake the screen? should be an option
        int x, y;

        ecore_evas_pointer_xy_get(e_comp->ee, &x, &y);
        ecore_evas_pointer_warp(e_comp->ee, x, y);
     }*/
   // walk hints
   eldbus_message_iter_dict_iterate(hints_iter, "sv", hints_dict_iter, n);
   n->app_name = eina_stringshare_add(n->app_name);
   n->icon.icon = eina_stringshare_add(n->icon.icon);
   n->summary = eina_stringshare_add(n->summary);
   txt = _nedje_text_escape(n->body);
   n->body = eina_stringshare_add(txt);
   free(txt);

   num = 0;
   while (eldbus_message_iter_get_and_next(actions_iter, 's', &txt))
     {
        if (eldbus_message_iter_get_and_next(actions_iter, 's', &txt2))
          { // XXX: add actions to notification
             E_Notification_Notify_Action *actions;

             printf("NOT: act=[%s] [%s]\n", txt, txt2);
             num++;
             actions = realloc(n->actions, (num + 1) * sizeof(E_Notification_Notify_Action));
             if (actions)
               {
                  n->actions = actions;
                  n->actions[num - 1].action = eina_stringshare_add(txt);
                  n->actions[num - 1].label = eina_stringshare_add(txt2);
                  n->actions[num].action = NULL;
                  n->actions[num].label = NULL;
               }
          }
     }

   e_object_ref(E_OBJECT(n));
   n->id = n_data->notify_cb(n_data->data, n);
   reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "u", n->id);
   e_object_unref(E_OBJECT(n));

   return reply;
}

static Eldbus_Message *
close_notification_cb(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   unsigned int id;

   if (!eldbus_message_arguments_get(msg, "u", &id)) return NULL;
   if (n_data->close_cb) n_data->close_cb(n_data->data, id);
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
capabilities_cb(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   int i;
   Eldbus_Message_Iter *main_iter, *array;

   main_iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_arguments_append(main_iter, "as", &array);

   for (i = 0; n_data->server_info->capabilities[i]; i++)
     {
        eldbus_message_iter_arguments_append
          (array, "s", n_data->server_info->capabilities[i]);
     }
   eldbus_message_iter_container_close(main_iter, array);
   return reply;
}

static Eldbus_Message *
server_info_cb(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "ssss",
                                   n_data->server_info->name,
                                   n_data->server_info->vendor,
                                   n_data->server_info->version,
                                   n_data->server_info->spec_version);
   return reply;
}

static const Eldbus_Method methods[] = {
   { "Notify",
      ELDBUS_ARGS({"s", "app_name"}, {"u", "replaces_id"}, {"s", "app_icon"}, {"s", "summary"}, {"s", "body"}, {"as", "actions"}, {"a{sv}", "hints"}, {"i", "expire_timeout"}),
      ELDBUS_ARGS({"u", "id"}),
      notify_cb, 0 },
   { "CloseNotification",
      ELDBUS_ARGS({"u", "id"}),
      NULL,
      close_notification_cb, 0 },
   { "GetCapabilities",
      NULL,
      ELDBUS_ARGS({"as", "capabilities"}),
      capabilities_cb, 0 },
   { "GetServerInformation",
      NULL,
      ELDBUS_ARGS({"s", "name"}, {"s", "vendor"}, {"s", "version"}, {"s", "spec_version"}),
      server_info_cb, 0 },

   { NULL, NULL, NULL, NULL, 0 }
};

enum
{
   SIGNAL_NOTIFICATION_CLOSED = 0,
   SIGNAL_ACTION_INVOKED,
};

static const Eldbus_Signal signals[] = {
   [SIGNAL_NOTIFICATION_CLOSED] =
   { "NotificationClosed", ELDBUS_ARGS({"u", "id"}, {"u", "reason"}), 0 },
   [SIGNAL_ACTION_INVOKED] =
   { "ActionInvoked", ELDBUS_ARGS({"u", "id"}, {"s", "action_key"}), 0 },

   { NULL, NULL, 0}
};

#define PATH      "/org/freedesktop/Notifications"
#define BUS       "org.freedesktop.Notifications"
#define INTERFACE "org.freedesktop.Notifications"

static const Eldbus_Service_Interface_Desc desc = {
   INTERFACE, methods, signals, NULL, NULL, NULL
};

EAPI Eina_Bool
e_notification_server_register(const E_Notification_Server_Info *server_info, E_Notification_Notify_Cb n_cb, E_Notification_Close_Cb close_cb, const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(server_info, EINA_FALSE);

   if (n_data) return EINA_FALSE;
   n_data = calloc(1, sizeof(Notification_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(n_data, EINA_FALSE);

   n_data->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   n_data->iface = eldbus_service_interface_register(n_data->conn, PATH, &desc);
   n_data->notify_cb = n_cb;
   n_data->close_cb = close_cb;
   n_data->data = (void *)data;
   n_data->server_info = server_info;
   eldbus_name_request(n_data->conn, BUS,
                       ELDBUS_NAME_REQUEST_FLAG_REPLACE_EXISTING, NULL, NULL);
   return EINA_TRUE;
}

EAPI void
e_notification_server_unregister(void)
{
   EINA_SAFETY_ON_NULL_RETURN(n_data);
   eldbus_service_interface_unregister(n_data->iface);
   eldbus_connection_unref(n_data->conn);
   free(n_data);
   n_data = NULL;
}

EAPI void
e_notification_notify_close(E_Notification_Notify *notify, E_Notification_Notify_Closed_Reason reason)
{
   EINA_SAFETY_ON_NULL_RETURN(n_data);
   EINA_SAFETY_ON_NULL_RETURN(notify);
   EINA_SAFETY_ON_FALSE_RETURN(reason <= E_NOTIFICATION_NOTIFY_CLOSED_REASON_UNDEFINED);
   eldbus_service_signal_emit(n_data->iface, SIGNAL_NOTIFICATION_CLOSED,
                              notify->id, reason);
}

EAPI void
e_notification_notify_action(E_Notification_Notify *notify, const char *action)
{
   EINA_SAFETY_ON_NULL_RETURN(n_data);
   EINA_SAFETY_ON_NULL_RETURN(notify);
   if (!action) action = "";
   eldbus_service_signal_emit(n_data->iface, SIGNAL_ACTION_INVOKED,
                              notify->id, action);
}

EAPI Evas_Object *
e_notification_notify_raw_image_get(E_Notification_Notify *notify, Evas *evas)
{
   Evas_Object *o;
   unsigned char *imgdata;

   EINA_SAFETY_ON_NULL_RETURN_VAL(notify, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(evas, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(notify->icon.raw.data, NULL);

   o = evas_object_image_filled_add(evas);
   evas_object_resize(o, notify->icon.raw.width, notify->icon.raw.height);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, notify->icon.raw.has_alpha);
   evas_object_image_size_set(o, notify->icon.raw.width, notify->icon.raw.height);

   imgdata = evas_object_image_data_get(o, EINA_TRUE);
   EINA_SAFETY_ON_NULL_GOTO(imgdata, error);

   if (notify->icon.raw.bits_per_sample == 8)
     {
        /* Although not specified.
         * The data are very likely to come from a GdkPixbuf
         * which align each row on a 4-bytes boundary when using RGB.
         * And is RGBA otherwise. */
        int x, y, *d;
        unsigned char *s;
        int rowstride = evas_object_image_stride_get(o);

        for (y = 0; y < notify->icon.raw.height; y++)
          {
             s = notify->icon.raw.data + (y * notify->icon.raw.rowstride);
             d = (int *)(void *)(imgdata + (y * rowstride));

             for (x = 0; x < notify->icon.raw.width;
                  x++, s += notify->icon.raw.channels, d++)
               {
                  if (notify->icon.raw.has_alpha)
                    *d = (s[3] << 24) | (s[0] << 16) | (s[1] << 8) | (s[2]);
                  else
                    *d = (0xff << 24) | (s[0] << 16) | (s[1] << 8) | (s[2]);
               }
          }
     }
   evas_object_image_data_update_add(o, 0, 0, notify->icon.raw.width,
                                     notify->icon.raw.height);
   evas_object_image_data_set(o, imgdata);

   return o;
error:
   evas_object_del(o);
   return NULL;
}

/* client API */

static void
client_notify_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   unsigned id = 0;
   E_Notification_Client_Send_Cb cb = eldbus_pending_data_del(pending, "cb");
   Eldbus_Connection *conn = eldbus_pending_data_del(pending, "conn");
   if (eldbus_message_error_get(msg, NULL, NULL))
     goto end;
   if (!eldbus_message_arguments_get(msg, "u", &id))
     goto end;
end:
   if (cb)
     cb(data, id);
   eldbus_connection_unref(conn);
   eldbus_shutdown();
}

static Eina_Bool
notification_client_dbus_send(E_Notification_Notify *notify, E_Notification_Client_Send_Cb cb, const void *data)
{
   Eldbus_Connection *conn;
   Eldbus_Message *msg;
   Eldbus_Message_Iter *main_iter, *actions, *hints;
   Eldbus_Message_Iter *entry, *var;
   Eldbus_Pending *p;
   EINA_SAFETY_ON_NULL_RETURN_VAL(notify, EINA_FALSE);
   eldbus_init();
   conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, EINA_FALSE);

   msg = eldbus_message_method_call_new(BUS, PATH, INTERFACE, "Notify");
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, EINA_FALSE);

   //build message
   main_iter = eldbus_message_iter_get(msg);
   if (!eldbus_message_iter_arguments_append(main_iter, "susssas",
                                            notify->app_name ? : "",
                                            notify->replaces_id,
                                            notify->icon.icon ? : "",
                                            notify->summary ? : "",
                                            notify->body ? : "",
                                            &actions))
     goto error;
   eldbus_message_iter_container_close(main_iter, actions);
   if (!eldbus_message_iter_arguments_append(main_iter, "a{sv}", &hints))
     goto error;

   if (notify->icon.raw.data)
     {
        Eldbus_Message_Iter *st, *data_iter;
        int i;

        eldbus_message_iter_arguments_append(hints, "{sv}", &entry);
        eldbus_message_iter_arguments_append(entry, "s", "image-data");
        var = eldbus_message_iter_container_new(entry, 'v', "(iiibiiay)");
        eldbus_message_iter_arguments_append(var, "(iiibiiay)", &st);
        eldbus_message_iter_arguments_append(st, "iiibiiay",
                                            notify->icon.raw.width,
                                            notify->icon.raw.height,
                                            notify->icon.raw.rowstride,
                                            notify->icon.raw.has_alpha,
                                            notify->icon.raw.bits_per_sample,
                                            notify->icon.raw.channels,
                                            &data_iter);
        for (i = 0; i < notify->icon.raw.data_size; i++)
          eldbus_message_iter_basic_append(data_iter, 'y', notify->icon.raw.data[i]);
        eldbus_message_iter_container_close(st, data_iter);
        eldbus_message_iter_container_close(var, st);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(hints, entry);
     }
   if (notify->icon.icon_path)
     {
        eldbus_message_iter_arguments_append(hints, "{sv}", &entry);
        eldbus_message_iter_arguments_append(entry, "s", "image-path");
        var = eldbus_message_iter_container_new(entry, 'v', "s");
        eldbus_message_iter_arguments_append(var, "s", notify->icon.icon_path);
        eldbus_message_iter_container_close(entry, var);
        eldbus_message_iter_container_close(hints, entry);
     }

   eldbus_message_iter_arguments_append(hints, "{sv}", &entry);
   eldbus_message_iter_arguments_append(entry, "s", "urgency");
   var = eldbus_message_iter_container_new(entry, 'v', "y");
   eldbus_message_iter_arguments_append(var, "y", notify->urgency);
   eldbus_message_iter_container_close(entry, var);
   eldbus_message_iter_container_close(hints, entry);

   eldbus_message_iter_container_close(main_iter, hints);

   eldbus_message_iter_arguments_append(main_iter, "i", notify->timeout);

   p = eldbus_connection_send(conn, msg, client_notify_cb, data, 5000);
   EINA_SAFETY_ON_NULL_GOTO(p, error);
   if (cb) eldbus_pending_data_set(p, "cb", cb);
   eldbus_pending_data_set(p, "conn", conn);

   return EINA_TRUE;

error:
   eldbus_message_unref(msg);
   return EINA_FALSE;
}

static void
normalize_notify(E_Notification_Notify *notify)
{
   if (!notify->timeout) notify->timeout = -1;
}

EAPI Eina_Bool
e_notification_client_send(E_Notification_Notify *notify, E_Notification_Client_Send_Cb cb, const void *data)
{
   unsigned id;
   E_Notification_Notify *copy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(notify, EINA_FALSE);
   normalize_notify(notify);

   if (!n_data)
     {
        fprintf(stderr, "UNHANDLED NOTIFICATION:\nSummary: %s\nBody: %s\n",
                notify->summary, notify->body);
        return notification_client_dbus_send(notify, cb, data);
     }

   // local
   copy = malloc(sizeof(E_Notification_Notify));
   EINA_SAFETY_ON_NULL_RETURN_VAL(copy, EINA_FALSE);
   memcpy(copy, notify, sizeof(E_Notification_Notify));

   copy->app_name = eina_stringshare_add(notify->app_name);
   copy->body = eina_stringshare_add(notify->body);
   copy->summary = eina_stringshare_add(notify->summary);
   copy->icon.icon = eina_stringshare_add(notify->icon.icon);
   if (notify->icon.icon_path)
     copy->icon.icon_path = eina_stringshare_add(notify->icon.icon_path);

   id = n_data->notify_cb(n_data->data, copy);
   if (cb) cb((void *)data, id);
   return EINA_TRUE;
}

EAPI Eina_Bool
e_notification_util_send(const char *summary, const char *body)
{
   E_Notification_Notify n;

   memset(&n, 0, sizeof(E_Notification_Notify));
   n.timeout = 3000;
   n.summary = summary;
   n.body = body;
   n.urgency = E_NOTIFICATION_NOTIFY_URGENCY_NORMAL;
   return e_notification_client_send(&n, NULL, NULL);
}
