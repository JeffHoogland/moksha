#include "e.h"
#include "config.h"

/* local subsystem functions */
static int _e_ipc_cb_client_add(void *data, int type, void *event);
static int _e_ipc_cb_client_del(void *data, int type, void *event);
static int _e_ipc_cb_client_data(void *data, int type, void *event);
static char *_e_ipc_path_str_get(char **paths, int *bytes);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server  = NULL;

/* externally accessible functions */
int
e_ipc_init(void)
{
   char buf[1024];
   char *disp;
   
   disp = getenv("DISPLAY");
   if (!disp) disp = ":0";
   snprintf(buf, sizeof(buf), "enlightenment-(%s)", disp);
   _e_ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_USER, buf, 0, NULL);
   if (!_e_ipc_server) return 0;
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, _e_ipc_cb_client_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, _e_ipc_cb_client_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, _e_ipc_cb_client_data, NULL);
   return 1;
}

void
e_ipc_shutdown(void)
{
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

/* local subsystem globals */
static int
_e_ipc_cb_client_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p connected to server!\n", e->client);
   return 1;
}

static int
_e_ipc_cb_client_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Del *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p disconnected from server!\n", e->client);
   /* delete client sruct */
   ecore_ipc_client_del(e->client);
   return 1;
}

static int
_e_ipc_cb_client_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   switch (e->minor)
     {
      case E_IPC_OP_MODULE_LOAD:
	if (e->data)
	  {
	     char *name;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if (!e_module_find(name))
	       {
		  e_module_new(name);
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_UNLOAD:
	if (e->data)
	  {
	     char *name;
	     E_Module *m;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if ((m = e_module_find(name)))
	       {
		  if (e_module_enabled_get(m))
		    e_module_disable(m);
		  e_object_del(E_OBJECT(m));
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_ENABLE:
	  {
	     char *name;
	     E_Module *m;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if ((m = e_module_find(name)))
	       {
		  if (!e_module_enabled_get(m))
		    e_module_enable(m);
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_DISABLE:
	  {
	     char *name;
	     E_Module *m;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if ((m = e_module_find(name)))
	       {
		  if (e_module_enabled_get(m))
		    e_module_disable(m);
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_LIST:
	  {
	     Evas_List *modules, *l;
	     int bytes;
	     E_Module *m;
	     char *data, *p;
		  
	     bytes = 0;
	     modules = e_module_list();
	     for (l = modules; l; l = l->next)
	       {
		  m = l->data;
		  bytes += strlen(m->name) + 1 + 1;
	       }
	     data = malloc(bytes);
	     p = data;
	     for (l = modules; l; l = l->next)
	       {
		  m = l->data;
		  strcpy(p, m->name);
		  p += strlen(m->name);
		  *p = 0;
		  p++;
		  *p = e_module_enabled_get(m);
		  p++;
	       }
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_MODULE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_MODULE_DIRS_LIST:
	  {
	     char *dirs[] = {
		PACKAGE_LIB_DIR"/enlightenment/modules",
		PACKAGE_LIB_DIR"/enlightenment/modules_extra",
		"~/.e/e/modules",
		NULL
	     };
	     char *data;
	     int bytes = 0;

	     data = _e_ipc_path_str_get(dirs, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_MODULE_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
 	  }
	break;
      case E_IPC_OP_BG_SET:
	  {
	     char *file;
	     Evas_List *managers, *l;
	     
	     file = malloc(e->size + 1);
	     file[e->size] = 0;
	     memcpy(file, e->data, e->size);
	     E_FREE(e_config->desktop_default_background);
	     e_config->desktop_default_background = file;
	     
	     managers = e_manager_list();
	     for (l = managers; l; l = l->next)
	       {
		  Evas_List *ll;
		  E_Manager *man;
		  
		  man = l->data;
		  for (ll = man->containers; ll; ll = ll->next)
		    {
		       E_Container *con;
             E_Zone *zone;
		       
		       con = ll->data;
             zone = e_zone_current_get(con);
		       e_zone_bg_reconfigure(zone);
		    }
	       }
	     e_config_save_queue();
          }
	break;
      case E_IPC_OP_BG_GET:
	  {
	     char *bg;
	     bg = e_config->desktop_default_background;
	     if (!bg)
	       bg = "";
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BG_GET_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   bg, strlen(bg) + 1);
 	  }
	break;
      case E_IPC_OP_FONT_AVAILABLE_LIST:
	  {
	     Evas_List *fonts_available, *l;
	     int bytes;
	     char *font_name, *data, *p;
		  
	     bytes = 0;
	     fonts_available = e_font_available_list();
	     printf("ipc font av: %d\n", fonts_available);
	     for (l = fonts_available; l; l = l->next)
	       {
		  font_name = evas_list_data(l);
		  bytes += strlen(font_name) + 1;
	       }

	     data = malloc(bytes);
	     p = data;
	     for (l = fonts_available; l; l = l->next)
	       {
		  font_name = evas_list_data(l);
		  strcpy(p, font_name);
		  p += strlen(font_name);
		  *p = 0;
		  p++;
	       }
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_AVAILABLE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     
	     e_font_available_list_free(fonts_available);
	     free(data);
	  }
	break;
      case E_IPC_OP_FONT_APPLY:
	  {
	     e_font_apply();
	  }
 	break;
      case E_IPC_OP_FONT_FALLBACK_CLEAR:
	  {
	     e_font_fallback_clear();
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_APPEND:
	  {
	     char * font_name;
	     font_name = malloc(e->size + 1);
	     font_name[e->size] = 0;
	     memcpy(font_name, e->data, e->size);
	     e_font_fallback_append(font_name);
	     free(font_name);

	     e_config_save_queue(); 
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_PREPEND:
	  {
	     char * font_name;
	     font_name = malloc(e->size + 1);
	     font_name[e->size] = 0;
	     memcpy(font_name, e->data, e->size);
	     e_font_fallback_prepend(font_name);
	     free(font_name);	   
		
	     e_config_save_queue();  
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_LIST:
	  {
	     Evas_List *fallbacks, *l;
	     int bytes;
	     E_Font_Fallback *eff;
	     char *data, *p;
		  
	     bytes = 0;
	     fallbacks = e_font_fallback_list();
	     for (l = fallbacks; l; l = l->next)
	       {
		  eff = evas_list_data(l);
		  bytes += strlen(eff->name) + 1;
	       }
	     data = malloc(bytes);
	     p = data;
	     for (l = fallbacks; l; l = l->next)
	       {
		  eff = evas_list_data(l);
		  strcpy(p, eff->name);
		  p += strlen(eff->name);
		  *p = 0;
		  p++;
	       }
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_FALLBACK_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);

	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_REMOVE:
	  {
	     char * font_name;
	     font_name = malloc(e->size + 1);
	     font_name[e->size] = 0;
	     memcpy(font_name, e->data, e->size);
	     e_font_fallback_remove(font_name);
	     free(font_name);	     

	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_SET:
	  {
	     char * p;
	     char * font_name;
	     char * text_class;
	     int font_size;

	     
	     p = e->data;
	     
	     /* Make sure our data is packed for us <str>0<str>0 */
	     if( p[e->size - 1] != 0) {
		break;
	     }

	     text_class = strdup(p);

	     p += strlen(text_class) + 1;
	     font_name = strdup(p);
	
	     p += strlen(font_name) + 1;
	     font_size = atoi(p);

	     e_font_default_set(text_class, font_name, font_size);

	     free(font_name);
	     free(text_class);

	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_GET:
	  {
	     int bytes;
	     E_Font_Default *efd;
	     char *data, *p, *text_class;
	     
	     text_class = malloc(e->size + 1);
	     text_class[e->size] = 0;
	     memcpy(text_class, e->data, e->size);
	     
	     efd = e_font_default_get (text_class);
	     
	     free(text_class);
		  
	     bytes = 0;
	     if (efd) {
	         bytes += strlen(efd->text_class) + 1;
	         bytes += strlen(efd->font) + 1;
	         bytes++; /* efd->size */
	     }
	     
	     data = malloc(bytes);
	     p = data;
	     
	     if (efd) {
	         strcpy(p, efd->text_class);
                 p += strlen(efd->text_class);
                 *p = 0;
	         p++;
	     
	         strcpy(p, efd->font);
	         p += strlen(efd->font);
	         *p = 0;
	         p++;
		  
                 /* FIXME: should this be packed like this (int to char) ? */
	         *p = (char) efd->size;
	         p++;
	     }

	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DEFAULT_GET_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_REMOVE:
	  {	  
	     char * text_class;
	     text_class = malloc(e->size + 1);
	     text_class[e->size] = 0;
	     memcpy(text_class, e->data, e->size);
	     e_font_default_remove(text_class);
	     free(text_class);	   

	     e_config_save_queue(); 
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_LIST:
	  {
	     Evas_List *defaults, *l;
	     int bytes;
	     E_Font_Default *efd;
	     char *data, *p;
		  
	     bytes = 0;
	     defaults = e_font_default_list();
	     for (l = defaults; l; l = l->next)
	       {
		  efd = l->data;
		  bytes += strlen(efd->text_class) + 1;
		  bytes += strlen(efd->font) + 1;
		  bytes++; /* efd->size */
	       }
	     data = malloc(bytes);
	     p = data;
	     for (l =defaults; l; l = l->next)
	       {
		  efd = l->data;
		  strcpy(p, efd->text_class);
		  p += strlen(efd->text_class);
		  *p = 0;
		  p++;
		  strcpy(p, efd->font);
		  p += strlen(efd->font);
		  *p = 0;
		  p++;
		  /* FIXME: should this be packed like this (int to char) ? */
		  *p = (char) efd->size;
		  p++;
	       }
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DEFAULT_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);

	  }
	break;
      case E_IPC_OP_BG_DIRS_LIST:
	  {
	     char *dirs[] = {
		PACKAGE_DATA_DIR"/data/themes",
		"~/.e/e/backgrounds",
		"~/.e/e/themes",
		NULL
	     };
	     char *data;
	     int bytes = 0;

	     data = _e_ipc_path_str_get(dirs, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BG_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
 	  }
	break;
      case E_IPC_OP_RESTART:
	  {
	     restart = 1;
	     ecore_main_loop_quit();
 	  }
	break;
      case E_IPC_OP_SHUTDOWN:
	  {
	     ecore_main_loop_quit();
 	  }
	break;
      default:
	break;
     }
   printf("E-IPC: client sent: [%i] [%i] (%i) \"%p\"\n", e->major, e->minor, e->size, e->data);
   /* ecore_ipc_client_send(e->client, 1, 2, 7, 77, 0, "ABC", 4); */
   /* we can disconnect a client like this: */
   /* ecore_ipc_client_del(e->client); */
   /* or we can end a server by: */
   /* ecore_ipc_server_del(ecore_ipc_client_server_get(e->client)); */
   return 1;
}  

/*
 * FIXME: This dosen't handle the case where one of the paths is of the
 *        form: ~moo/bar/baz need to figure out the correct path to the 
 *        specified users homedir
 */
static char *
_e_ipc_path_str_get(char **paths, int *bytes)
{
   char *data = NULL, **cur, *home;
   int pos = 0;
   char tmp[PATH_MAX];

   *bytes = 0;
   home = e_user_homedir_get();
   for (cur = paths; *cur != NULL; cur++)
     {
	int len;
	char *p;

	p = *cur;
	if (*p == '~') snprintf(tmp, PATH_MAX, "%s%s", home, ++p);
	else snprintf(tmp, PATH_MAX, "%s", p);

	*bytes += strlen(tmp) + 1;
	data = realloc(data, *bytes);

	memcpy(data + pos, tmp, strlen(tmp));
	pos = *bytes;
	data[pos - 1] = 0;
     }
   free(home);
   return data;
}


