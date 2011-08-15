#include "e.h"

#ifdef USE_IPC
/* local subsystem functions */
static Eina_Bool _e_ipc_cb_client_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_ipc_cb_client_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_ipc_cb_client_data(void *data __UNUSED__, int type __UNUSED__, void *event);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server = NULL;
#endif

/* externally accessible functions */
EINTERN int
e_ipc_init(void)
{
#ifdef USE_IPC
   char buf[1024];
   char *tmp, *user, *disp;
   int pid;

   tmp = getenv("TMPDIR");
   if (!tmp) tmp = "/tmp";
   user = getenv("USER");
   if (!user) user = "__unknown__";
   disp = getenv("DISPLAY");
   if (!disp) disp = ":0";
   pid = (int)getpid();
   snprintf(buf, sizeof(buf), "%s/enlightenment-%s", tmp, user);
   if (mkdir(buf, S_IRWXU) == 0)
     {
     }
   else
     {
        struct stat st;

        if (stat(buf, &st) == 0)
          {
             if ((st.st_uid == getuid()) &&
                 ((st.st_mode & (S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO)) ==
                  (S_IRWXU | S_IFDIR)))
               {
               }
             else
               {
                  e_error_message_show(_("Possible IPC Hack Attempt. The IPC socket\n"
                                         "directory already exists BUT has permissions\n"
                                         "that are too leanient (must only be readable\n" "and writable by the owner, and nobody else)\n"
                                                                                          "or is not owned by you. Please check:\n"
                                                                                          "%s/enlightenment-%s\n"), tmp, user);
                  return 0;
               }
          }
        else
          {
             e_error_message_show(_("The IPC socket directory cannot be created or\n"
                                    "examined.\n"
                                    "Please check:\n"
                                    "%s/enlightenment-%s\n"),
                                  tmp, user);
             return 0;
          }
     }
   snprintf(buf, sizeof(buf), "%s/enlightenment-%s/disp-%s-%i", 
            tmp, user, disp, pid);
   _e_ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_SYSTEM, buf, 0, NULL);
   e_util_env_set("E_IPC_SOCKET", "");
   if (!_e_ipc_server) return 0;
   e_util_env_set("E_IPC_SOCKET", buf);
   printf("INFO: E_IPC_SOCKET=%s\n", buf);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, 
                           _e_ipc_cb_client_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, 
                           _e_ipc_cb_client_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, 
                           _e_ipc_cb_client_data, NULL);

   e_ipc_codec_init();
#endif
   return 1;
}

EINTERN int
e_ipc_shutdown(void)
{
#ifdef USE_IPC
   e_ipc_codec_shutdown();
   if (_e_ipc_server)
     {
        ecore_ipc_server_del(_e_ipc_server);
        _e_ipc_server = NULL;
     }
#endif
   return 1;
}

#ifdef USE_IPC
/* local subsystem globals */
static Eina_Bool
_e_ipc_cb_client_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;

   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) 
     return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_ipc_cb_client_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Del *e;

   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) 
     return ECORE_CALLBACK_PASS_ON;
   /* delete client sruct */
   e_thumb_client_del(e);
   e_fm2_client_del(e);
   e_init_client_del(e);
   ecore_ipc_client_del(e->client);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_ipc_cb_client_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;

   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) 
     return ECORE_CALLBACK_PASS_ON;
   switch (e->major)
     {
      case E_IPC_DOMAIN_SETUP:
      case E_IPC_DOMAIN_REQUEST:
      case E_IPC_DOMAIN_REPLY:
      case E_IPC_DOMAIN_EVENT:
        switch (e->minor)
          {
           case E_IPC_OP_EXEC_ACTION:
               {
                  E_Ipc_2Str *req = NULL;

                  if (e_ipc_codec_2str_dec(e->data, e->size, &req))
                    {
                       Eina_List *m = e_manager_list();
                       int len, ok = 0;
                       void *d;

                       if (m)
                         {
                            E_Manager *man = eina_list_data_get(m);

                            if (man)
                              {
                                 E_Action *act = e_action_find(req->str1);

                                 if ((act) && (act->func.go))
                                   {
                                      act->func.go(E_OBJECT(man), req->str2);
                                      ok = 1;
                                   }
                              }
                         }

                       d = e_ipc_codec_int_enc(ok, &len);
                       if (d)
                         {
                            ecore_ipc_client_send(e->client,
                                                  E_IPC_DOMAIN_REPLY,
                                                  E_IPC_OP_EXEC_ACTION_REPLY,
                                                  0, 0, 0, d, len);
                            free(d);
                         }

                       if (req)
                         {
                            E_FREE(req->str1);
                            E_FREE(req->str2);
                            E_FREE(req);
                         }
                    }
               }
             break;

           default:
             break;
          }
        break;

      case E_IPC_DOMAIN_THUMB:
        e_thumb_client_data(e);
        break;

      case E_IPC_DOMAIN_FM:
        e_fm2_client_data(e);
        break;

      case E_IPC_DOMAIN_INIT:
        e_init_client_data(e);
        break;

      case E_IPC_DOMAIN_ALERT: 
          {
             E_Action *a = NULL;

             switch (e->minor) 
               {
                case E_ALERT_OP_RESTART:
                  a = e_action_find("restart");
                  break;
                case E_ALERT_OP_EXIT:
                  a = e_action_find("exit_now");
                  break;
               }
             if ((a) && (a->func.go)) a->func.go(NULL, NULL);
             break;
          }

      default:
        break;
     }
   return 1;
}

#endif
