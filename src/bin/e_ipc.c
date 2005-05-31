#include "e.h"

/* local subsystem functions */
static int _e_ipc_cb_client_add(void *data, int type, void *event);
static int _e_ipc_cb_client_del(void *data, int type, void *event);
static int _e_ipc_cb_client_data(void *data, int type, void *event);

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

   e_ipc_codec_init();
   return 1;
}

void
e_ipc_shutdown(void)
{
   e_ipc_codec_shutdown();
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

/* local subsystem globals */
static int
_e_ipc_cb_client_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p connected to server!\n", e->client);
   return 1;
}

static int
_e_ipc_cb_client_del(void *data __UNUSED__, int type __UNUSED__, void *event)
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
_e_ipc_cb_client_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   switch (e->minor)
     {
#define TYPE  E_WM_IN
#include      "e_ipc_handlers.h"
#undef TYPE	
/* here to steal from to port over to the new e_ipc_handlers.h */	
#if 0
      case E_IPC_OP_BG_SET:
	  {
	     char *file = NULL;
	     
	     if (e_ipc_codec_str_dec(e->data, e->size, &file))
	       {
		  Evas_List *managers, *l;

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
          }
	break;
      case E_IPC_OP_BG_GET:
	  {
	     void *data;
	     int bytes;
	     
	     data = e_ipc_codec_str_enc(e_config->desktop_default_background, &bytes);
	     if (data)
	       {
		  ecore_ipc_client_send(e->client,
					E_IPC_DOMAIN_REPLY,
					E_IPC_OP_BG_GET_REPLY,
					0/*ref*/, 0/*ref_to*/, 0/*response*/,
					data, bytes);
		  free(data);
	       }
 	  }
	break;
      case E_IPC_OP_FONT_AVAILABLE_LIST:
	  {
	     E_Font_Available *fa;
	     Evas_List *dat = NULL, *l, *fa_list;
	     void *data;
	     int bytes;
	     
	     fa_list = e_font_available_list();
	     for (l = fa_list; l; l = l->next)
	       {
		  fa = l->data;
		  dat = evas_list_append(dat, fa->name);
	       }
	     data = e_ipc_codec_str_list_enc(dat, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_AVAILABLE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	     evas_list_free(dat);
	     e_font_available_list_free(fa_list);
	  }
	break;
      case E_IPC_OP_FONT_APPLY:
	  {
	     e_font_apply();
             e_config_save_queue();
	  }
 	break;
      case E_IPC_OP_FONT_FALLBACK_CLEAR:
	  {
	     e_font_fallback_clear();
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_APPEND:
	  {
	     char *font_name = NULL;
	     
	     if (e_ipc_codec_str_dec(e->data, e->size, &font_name))
	       {
		  e_font_fallback_append(font_name);
		  free(font_name);
		  e_config_save_queue();
	       }
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_PREPEND:
	  {
	     char *font_name = NULL;
	     
	     if (e_ipc_codec_str_dec(e->data, e->size, &font_name))
	       {
		  e_font_fallback_prepend(font_name);
		  free(font_name);	   
		  e_config_save_queue();  
	       }
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_LIST:
	  {
	     E_Font_Fallback *ff;
	     Evas_List *dat = NULL, *l;
	     void *data;
	     int bytes;
	     
	     for (l = e_font_available_list(); l; l = l->next)
	       {
		  ff = l->data;
		  dat = evas_list_append(dat, ff->name);
	       }
	     data = e_ipc_codec_str_list_enc(dat, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_FALLBACK_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	     evas_list_free(dat);
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_REMOVE:
	  {
	     char *font_name = NULL;
	     
	     if (e_ipc_codec_str_dec(e->data, e->size, &font_name))
	       {
		  e_font_fallback_remove(font_name);
		  free(font_name);	     
		  e_config_save_queue();
	       }
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_SET:
	  {
	     E_Ipc_2Str_Int *v = NULL;
	     
	     if (e_ipc_codec_2str_int_dec(e->data, e->size, &v))
	       {
		  e_font_default_set(v->str1, v->str2, v->val);
		  free(v->str1);
		  free(v->str2);
		  free(v);
		  e_config_save_queue();
	       }
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_GET:
	  {
	     E_Font_Default *efd;
	     char *tclass = NULL;
	     void *data;
	     int bytes;
	     
	     if (e_ipc_codec_str_dec(e->data, e->size, &tclass))
	       {
		  efd = e_font_default_get(tclass);
		  data = e_ipc_codec_2str_int_enc(efd->text_class, efd->font, efd->size, &bytes);
		  ecore_ipc_client_send(e->client,
					E_IPC_DOMAIN_REPLY,
					E_IPC_OP_FONT_DEFAULT_GET_REPLY,
					0/*ref*/, 0/*ref_to*/, 0/*response*/,
					data, bytes);
		  free(data);
		  free(tclass);
	       }
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_REMOVE:
	  {	  
	     char *tclass = NULL;
	     
	     if (e_ipc_codec_str_dec(e->data, e->size, &tclass))
	       {
		  e_font_default_remove(tclass);
		  free(tclass);
		  e_config_save_queue(); 
	       }
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_LIST:
	  {
	     E_Ipc_2Str_Int *v;
	     E_Font_Default *efd;
	     Evas_List *dat = NULL, *l;
	     void *data;
	     int bytes;
	     
	     for (l = e_font_default_list(); l; l = l->next)
	       {
		  efd = l->data;
		  v = calloc(1, sizeof(E_Ipc_2Str_Int));
		  v->str1 = efd->text_class;
		  v->str2 = efd->font;
		  v->val = efd->size;
		  dat = evas_list_append(dat, v);
	       }
	     data = e_ipc_codec_2str_int_list_enc(dat, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DEFAULT_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     while (dat)
	       {
		  v = dat->data;
		  free(v);
		  dat = evas_list_remove_list(dat, dat);
	       }
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
      case E_IPC_OP_LANG_LIST:
	  {
	     Evas_List *langs;
	     int bytes;
	     char *data;
	     
	     langs = (Evas_List *)e_intl_language_list();
	     data = _e_ipc_str_list_get(langs, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_LANG_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_LANG_SET:
	  {
	     char *lang;
	     
	     lang = _e_ipc_simple_str_dec(e->data, e->size);
	     IF_FREE(e_config->language);
	     e_config->language = lang;
	     e_intl_language_set(e_config->language);
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_LANG_GET:
	  {
	     char *lang;
	     
	     lang = e_config->language;
	     if (!lang) lang = "";
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_LANG_GET_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   lang, strlen(lang) + 1);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_LIST:
	  {
	     Evas_List *bindings;
	     int bytes;
	     char *data;
	     
	     bindings = e_config->mouse_bindings;
	     data = _e_ipc_mouse_binding_list_enc(bindings, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BINDING_MOUSE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_ADD:
	  {
	     E_Config_Binding_Mouse bind, *eb;
	     
	     _e_ipc_mouse_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_mouse_match(&bind);
		  if (!eb)
		    {
		       eb = E_NEW(E_Config_Binding_Key, 1);
		       e_config->mouse_bindings = evas_list_append(e_config->mouse_bindings, eb);
		       eb->context = bind.context;
		       eb->button = bind.button;
		       eb->modifiers = bind.modifiers;
		       eb->any_mod = bind.any_mod;
		       eb->action = strdup(bind.action);
		       eb->params = strdup(bind.params);
		       e_border_button_bindings_ungrab_all();
		       e_bindings_mouse_add(bind.context, bind.button, bind.modifiers,
					    bind.any_mod, bind.action, bind.params);
		       e_border_button_bindings_grab_all();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_DEL:
	  {
	     E_Config_Binding_Mouse bind, *eb;
	     
	     _e_ipc_mouse_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_mouse_match(&bind);
		  if (eb)
		    {
		       e_config->mouse_bindings = evas_list_remove(e_config->mouse_bindings, eb);
		       IF_FREE(eb->action);
		       IF_FREE(eb->params);
		       IF_FREE(eb);
		       e_border_button_bindings_ungrab_all();
		       e_bindings_mouse_del(bind.context, bind.button, bind.modifiers,
					    bind.any_mod, bind.action, bind.params);
		       e_border_button_bindings_grab_all();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_KEY_LIST:
	  {
	     Evas_List *bindings;
	     int bytes;
	     char *data;
	     
	     bindings = e_config->key_bindings;
	     data = _e_ipc_key_binding_list_enc(bindings, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BINDING_KEY_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_KEY_ADD:
	  {
	     E_Config_Binding_Key bind, *eb;
	     
	     _e_ipc_key_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_key_match(&bind);
		  if (!eb)
		    {
		       eb = E_NEW(E_Config_Binding_Key, 1);
		       e_config->key_bindings = evas_list_append(e_config->key_bindings, eb);
		       eb->context = bind.context;
		       eb->modifiers = bind.modifiers;
		       eb->any_mod = bind.any_mod;
		       eb->key = strdup(bind.key);
		       eb->action = strdup(bind.action);
		       eb->params = strdup(bind.params);
		       e_managers_keys_ungrab();
		       e_bindings_key_add(bind.context, bind.key, bind.modifiers,
					  bind.any_mod, bind.action, bind.params);
		       e_managers_keys_grab();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_KEY_DEL:
	  {
	     E_Config_Binding_Key bind, *eb;
	     
	     _e_ipc_key_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_key_match(&bind);
		  if (eb)
		    {
		       e_config->key_bindings = evas_list_remove(e_config->key_bindings, eb);
		       IF_FREE(eb->key);
		       IF_FREE(eb->action);
		       IF_FREE(eb->params);
		       IF_FREE(eb);
		       e_managers_keys_ungrab();
		       e_bindings_key_del(bind.context, bind.key, bind.modifiers,
					  bind.any_mod, bind.action, bind.params);
		       e_managers_keys_grab();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_MENUS_SCROLL_SPEED_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->menus_scroll_speed)))
	  {
	     E_CONFIG_LIMIT(e_config->menus_scroll_speed, 1.0, 20000.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MENUS_SCROLL_SPEED_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->menus_scroll_speed, 
				 E_IPC_OP_MENUS_SCROLL_SPEED_GET_REPLY);
	break;
      case E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->menus_fast_mouse_move_threshhold)))
	  {
	     E_CONFIG_LIMIT(e_config->menus_fast_mouse_move_threshhold, 1.0, 2000.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->menus_fast_mouse_move_threshhold, 
				 E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_GET_REPLY);
	break;
      case E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->menus_click_drag_timeout)))
	  {
	     E_CONFIG_LIMIT(e_config->menus_click_drag_timeout, 0.0, 10.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->menus_click_drag_timeout,
				 E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET_REPLY);
	break;
      case E_IPC_OP_BORDER_SHADE_ANIMATE_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->border_shade_animate)))
	  {
	     E_CONFIG_LIMIT(e_config->border_shade_animate, 0, 1);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_ANIMATE_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->border_shade_animate,
			      E_IPC_OP_BORDER_SHADE_ANIMATE_GET_REPLY);
	break;
      case E_IPC_OP_BORDER_SHADE_TRANSITION_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->border_shade_transition)))
	  {
	     E_CONFIG_LIMIT(e_config->border_shade_transition, 0, 3);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_TRANSITION_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->border_shade_transition,
			      E_IPC_OP_BORDER_SHADE_TRANSITION_GET_REPLY);
	break;
      case E_IPC_OP_BORDER_SHADE_SPEED_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->border_shade_speed)))
	  {
	     E_CONFIG_LIMIT(e_config->border_shade_speed, 1.0, 20000.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_SPEED_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->border_shade_speed,
				 E_IPC_OP_BORDER_SHADE_SPEED_GET_REPLY);
	break;
      case E_IPC_OP_FRAMERATE_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->framerate)))
	  {
	     E_CONFIG_LIMIT(e_config->image_cache, 1.0, 200.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FRAMERATE_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->framerate,
				 E_IPC_OP_FRAMERATE_GET_REPLY);
	break;
      case E_IPC_OP_IMAGE_CACHE_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->image_cache)))
	  {
	     E_CONFIG_LIMIT(e_config->image_cache, 0, 256 * 1024);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_IMAGE_CACHE_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->image_cache,
			      E_IPC_OP_IMAGE_CACHE_GET_REPLY);
	break;
      case E_IPC_OP_FONT_CACHE_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->font_cache)))
	  {
	     E_CONFIG_LIMIT(e_config->font_cache, 0, 32 * 1024);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_CACHE_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->font_cache,
			      E_IPC_OP_FONT_CACHE_GET_REPLY);
	break;
      case E_IPC_OP_USE_EDGE_FLIP_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->use_edge_flip)))
	  {
	     E_CONFIG_LIMIT(e_config->use_edge_flip, 0, 1);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_USE_EDGE_FLIP_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->use_edge_flip,
			      E_IPC_OP_USE_EDGE_FLIP_GET_REPLY);
	break;
      case E_IPC_OP_EDGE_FLIP_TIMEOUT_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->edge_flip_timeout)))
	  {
	     E_CONFIG_LIMIT(e_config->edge_flip_timeout, 0.0, 2.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_EDGE_FLIP_TIMEOUT_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->edge_flip_timeout,
				 E_IPC_OP_EDGE_FLIP_TIMEOUT_GET_REPLY);
	break;
      case E_IPC_OP_DESKS_SET:
	if (e_ipc_codec_2int_dec(e->data, e->size,
				 &(e_config->zone_desks_x_count),
				 &(e_config->zone_desks_y_count)))
	  {
	     Evas_List *l;
	     
	     E_CONFIG_LIMIT(e_config->zone_desks_x_count, 1, 64);
	     E_CONFIG_LIMIT(e_config->zone_desks_y_count, 1, 64);
	     for (l = e_manager_list(); l; l = l->next)
	       {
		  E_Manager *man;
		  Evas_List *l2;
		  
		  man = l->data;
		  for (l2 = man->containers; l2; l2 = l2->next)
		    {
		       E_Container *con;
		       Evas_List *l3;
		       
		       con = l2->data;
		       for (l3 = con->zones; l3; l3 = l3->next)
			 {
			    E_Zone *zone;
			    
			    zone = l3->data;
			    e_zone_desk_count_set(zone,
						  e_config->zone_desks_x_count,
						  e_config->zone_desks_y_count);
			 }
		    }
	       }
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_DESKS_GET:
	_e_ipc_reply_2int_send(e->client,
			       e_config->zone_desks_x_count,
			       e_config->zone_desks_y_count,
			       E_IPC_OP_DESKS_GET_REPLY);
	break;
      case E_IPC_OP_FOCUS_POLICY_SET:
	e_border_button_bindings_ungrab_all();
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->focus_policy)))
	  {
	     e_config_save_queue();
	  }
	e_border_button_bindings_grab_all();
	break;
      case E_IPC_OP_FOCUS_POLICY_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->focus_policy,
			      E_IPC_OP_FOCUS_POLICY_GET_REPLY);
	break;
      case E_IPC_OP_MODULE_DIRS_LIST:
	  {
	     Evas_List *dir_list;
	     char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_modules);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_MODULE_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_MODULE_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_modules, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_MODULE_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_modules, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_MODULE_DIRS_REMOVE:
          {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_modules, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Theme PATH IPC */
      case E_IPC_OP_THEME_DIRS_LIST:
	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_themes);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_THEME_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_THEME_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_themes, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_THEME_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_themes, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_THEME_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_themes, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Font Path IPC */
      case E_IPC_OP_FONT_DIRS_LIST:
          {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_fonts);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_FONT_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_fonts, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_FONT_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_fonts, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_FONT_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_fonts, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
   
      /* data Path IPC */
      case E_IPC_OP_DATA_DIRS_LIST:
	  {
             Evas_List *dir_list;
	     char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_data);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_DATA_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_DATA_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_data, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_DATA_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_data, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_DATA_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_data, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Images Path IPC */
      case E_IPC_OP_IMAGE_DIRS_LIST:
   	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_images);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_IMAGE_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_IMAGE_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_images, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
     case E_IPC_OP_IMAGE_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_images, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_IMAGE_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_images, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Init Path IPC */
      case E_IPC_OP_INIT_DIRS_LIST:
   	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_init);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_INIT_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_INIT_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_init, dir);
	     
	     free(dir);	   
	     e_config_save_queue();
	     break;
	  }
      case E_IPC_OP_INIT_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_init, dir);
	     
	     free(dir);	   
	     e_config_save_queue();
	     break;
	  }
      case E_IPC_OP_INIT_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_init, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Icon Path IPC */
      case E_IPC_OP_ICON_DIRS_LIST:
	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_icons);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_ICON_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_ICON_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_icons, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_ICON_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_icons, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_ICON_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_icons, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Icon Path IPC */
      case E_IPC_OP_BG_DIRS_LIST:
	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_backgrounds);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BG_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_BG_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_backgrounds, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_BG_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_backgrounds, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_BG_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_backgrounds, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
#endif
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

#if 0
static void
_e_ipc_reply_double_send(Ecore_Ipc_Client *client, double val, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_double_enc(val, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

static void
_e_ipc_reply_int_send(Ecore_Ipc_Client *client, int val, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_int_enc(val, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

static void
_e_ipc_reply_2int_send(Ecore_Ipc_Client *client, int val1, int val2, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_2int_enc(val1, val2, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}
#endif
