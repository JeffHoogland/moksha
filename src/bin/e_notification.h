#ifndef E_TYPEDEFS

#ifndef _E_NOTIFICATION_H
#define _E_NOTIFICATION_H

#define E_NOTIFICATION_TYPE 0x12342166

typedef enum _E_Notification_Notify_Urgency
{
  E_NOTIFICATION_NOTIFY_URGENCY_LOW = 0,
  E_NOTIFICATION_NOTIFY_URGENCY_NORMAL = 1,
  E_NOTIFICATION_NOTIFY_URGENCY_CRITICAL = 2
} E_Notification_Notify_Urgency;

typedef enum _E_Notification_Notify_Closed_Reason
{
  E_NOTIFICATION_NOTIFY_CLOSED_REASON_EXPIRED   = 1,
  E_NOTIFICATION_NOTIFY_CLOSED_REASON_DISMISSED = 2,
  E_NOTIFICATION_NOTIFY_CLOSED_REASON_REQUESTED = 3,
  E_NOTIFICATION_NOTIFY_CLOSED_REASON_UNDEFINED = 4
} E_Notification_Notify_Closed_Reason;

typedef struct _E_Notification_Notify_Action
{
   const char *action;
   const char *label;
} E_Notification_Notify_Action;

typedef struct _E_Notification_Notify
{
   E_Object e_obj_inherit;
   unsigned int id;
   const char *app_name;
   unsigned int replaces_id;
   const char *summary;
   const char *body;
   int timeout; // time in ms
   E_Notification_Notify_Urgency urgency;
   struct {
      const char *icon;
      const char *icon_path;
      struct {
         int width;
         int height;
         int rowstride;
         Eina_Bool has_alpha;
         int bits_per_sample;
         int channels;
         unsigned char *data;
         int data_size;
      } raw;
   } icon;
   const char *category;
   const char *desktop_entry;
   const char *sound_file;
   const char *sound_name;
   int x, y;
   Eina_Bool have_xy;
   Eina_Bool icon_actions;
   Eina_Bool resident;
   Eina_Bool suppress_sound;
   Eina_Bool transient;
   E_Notification_Notify_Action *actions;
} E_Notification_Notify;

typedef unsigned int (*E_Notification_Notify_Cb)(void *data, E_Notification_Notify *n);
typedef void (*E_Notification_Close_Cb)(void *data, unsigned int id);

typedef struct _E_Notification_Server_Info
{
   const char *name;
   const char *vendor;
   const char *version;
   const char *spec_version;
   const char *capabilities[];
} E_Notification_Server_Info;

/**
 * Register a notification server
 *
 * It's only possible to have one server registered at a time. If this function
 * is called twice it will return EINA_FALSE on the second time.
 *
 * @return EINA_TRUE if server was registered, EINA_FALSE otherwise.
 */
EAPI Eina_Bool e_notification_server_register(const E_Notification_Server_Info *server_info, E_Notification_Notify_Cb notify_cb, E_Notification_Close_Cb close_cb, const void *data);

/**
 * Unregister the sole notification server
 */
EAPI void e_notification_server_unregister(void);

EAPI void e_notification_notify_close(E_Notification_Notify *notify, E_Notification_Notify_Closed_Reason reason);
EAPI Evas_Object *e_notification_notify_raw_image_get(E_Notification_Notify *notify, Evas *evas);

//client
typedef void (*E_Notification_Client_Send_Cb)(void *data, unsigned int id);

EAPI Eina_Bool e_notification_client_send(E_Notification_Notify *notify, E_Notification_Client_Send_Cb cb, const void *data);
EAPI void e_notification_notify_action(E_Notification_Notify *notify, const char *action);
EAPI Eina_Bool e_notification_util_send(const char *summary, const char *body);
#endif

#endif
