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
   char buf[4096], buf2[128], buf3[4096];
   char *tmp, *user, *disp, *base;
   int pid, trynum = 0;

   tmp = getenv("TMPDIR");
   if (!tmp) tmp = "/tmp";
   base = tmp;

   tmp = getenv("XDG_RUNTIME_DIR");
   if (tmp) base = tmp;
   tmp = getenv("SD_USER_SOCKETS_DIR");
   if (tmp) base = tmp;
     
   user = getenv("USER");
   if (!user)
     {
        int uidint;
        
        user = "__unknown__";
        uidint = getuid();
        if (uidint >= 0)
          {
             snprintf(buf2, sizeof(buf2), "%i", uidint);
             user = buf2;
          }
     }
   
   disp = getenv("DISPLAY");
   if (!disp) disp = ":0";
   
   e_util_env_set("E_IPC_SOCKET", "");
   
   pid = (int)getpid();
   for (trynum = 0; trynum <= 4096; trynum++)
     {
        struct stat st;
        int id1 = 0;
        
        snprintf(buf, sizeof(buf), "%s/e-%s@%x",
                 base, user, id1);
        mkdir(buf, S_IRWXU);
        if (stat(buf, &st) == 0)
          {
             if ((st.st_uid == getuid()) &&
                  ((st.st_mode & (S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO)) ==
                      (S_IRWXU | S_IFDIR)))
               {
                  snprintf(buf3, sizeof(buf3), "%s/%s-%i",
                           buf, disp, pid);
                  _e_ipc_server = ecore_ipc_server_add
                    (ECORE_IPC_LOCAL_SYSTEM, buf3, 0, NULL);
                  if (_e_ipc_server) break;
               }
          }
        id1 = rand();
     }
   if (!_e_ipc_server) return 0;

   e_util_env_set("E_IPC_SOCKET", buf3);
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
             switch (e->minor)
               {
                case E_ALERT_OP_RESTART:
                  if (getenv("E_START_MTRACK"))
                    e_util_env_set("MTRACK", "track");
                  ecore_app_restart();
                  break;
                case E_ALERT_OP_EXIT:
                  exit(-11);
                  break;
               }
          }
        break;
      default:
        break;
     }
   return ECORE_CALLBACK_PASS_ON;
}

#endif
