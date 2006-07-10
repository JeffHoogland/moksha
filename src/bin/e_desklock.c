#include "e.h"
#ifdef HAVE_PAM
# include <security/pam_appl.h>
# include <pwd.h>
# include <limits.h>
#endif

#define ELOCK_POPUP_LAYER 10000
#define PASSWD_LEN 256

/**************************** private data ******************************/
typedef struct _E_Desklock_Data		E_Desklock_Data;
typedef struct _E_Desklock_Popup_Data	E_Desklock_Popup_Data;
#ifdef HAVE_PAM
typedef struct _E_Desklock_Auth         E_Desklock_Auth;
#endif

struct _E_Desklock_Popup_Data
{
   E_Popup	*popup_wnd;
   Evas_Object	*bg_object;
   Evas_Object	*login_box;
};

struct _E_Desklock_Data
{
   Evas_List	  *elock_wnd_list;
   Ecore_X_Window  elock_wnd;
   Evas_List	  *handlers;
   Ecore_X_Window  elock_grab_break_wnd;
   char		  passwd[PASSWD_LEN];
};
#ifdef HAVE_PAM
struct _E_Desklock_Auth
{
   struct {
      struct pam_conv conv;
      pam_handle_t    *handle;
   } pam;
   
   char user[PATH_MAX];
   char passwd[PATH_MAX];
};
#endif

static	E_Desklock_Data	   *edd = NULL;
static	E_Zone		   *last_active_zone = NULL;
#ifdef HAVE_PAM
static Ecore_Event_Handler *_e_desklock_exit_handler = NULL;
static pid_t                _e_desklock_child_pid = -1;
#endif

/***********************************************************************/

static int _e_desklock_cb_key_down(void *data, int type, void *event);
static int _e_desklock_cb_mouse_down(void *data, int type, void *event);
static int _e_desklock_cb_mouse_up(void *data, int type, void *event);
static int _e_desklock_cb_mouse_wheel(void *data, int type, void *event);
static int _e_desklock_cb_mouse_move(void *data, int type, void *event);

static void _e_desklock_passwd_update();
static void _e_desklock_backspace();
static void _e_desklock_delete();
static int  _e_desklock_zone_num_get();

static int _e_desklock_check_auth();

#ifdef HAVE_PAM
static int _e_desklock_cb_exit(void *data, int type, void *event);
static int _desklock_auth(const char *passwd);
static int _desklock_pam_init(E_Desklock_Auth *da);
static int _desklock_auth_pam_conv(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr);
static char *_desklock_auth_get_current_user(void);
static char *_desklock_auth_get_current_host(void);
#endif

EAPI int
e_desklock_init(void)
{
   if (e_config->desklock_disable_screensaver)
     ecore_x_screensaver_timeout_set(0);
   else
     {
	if (e_config->desklock_use_timeout)
	  ecore_x_screensaver_timeout_set(e_config->desklock_timeout);
     }
   return 1;
}

EAPI int
e_desklock_shutdown(void)
{
   e_desklock_hide();
   return 1;
}

EAPI int
e_desklock_show(void)
{
   Evas_List		  *managers, *l, *l2, *l3;
   E_Desklock_Popup_Data  *edp;
   Evas_Coord		  mw, mh;
   E_Zone		  *current_zone;
   int			  zone_counter;
   int			  total_zone_num;

   if (edd) return 0;
   
#ifdef HAVE_PAM
   if (e_config->desklock_auth_method == 1)
     {
#endif
       if (!e_config->desklock_personal_passwd)
	 {
	    E_Zone  *zone;
	    
	    zone = e_util_zone_current_get(e_manager_current_get());
	    if (zone)
	      {
		 E_Config_Dialog *cfd;
		 
		 cfd = e_int_config_desklock(zone->container);
	      }
	    return 0;
	 }
#ifdef HAVE_PAM
     }
#endif
   
   edd = E_NEW(E_Desklock_Data, 1);
   if (!edd) return 0;
   edd->elock_wnd = ecore_x_window_input_new(e_manager_current_get()->root, 
					     0, 0, 1, 1);
   ecore_x_window_show(edd->elock_wnd);
   managers = e_manager_list();
   if (!e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd))
     {
	for (l = managers; l; l = l->next)
	  {
	     E_Manager *man;
	     Ecore_X_Window *windows;
	     int wnum;
	     
	     man = l->data;
	     windows = ecore_x_window_children_get(man->root, &wnum);
	     if (windows)
	       {
		  int i;
		  
		  for (i = 0; i < wnum; i++)
		    {
		       Ecore_X_Window_Attributes att;
		       memset(&att, 0, sizeof(Ecore_X_Window_Attributes));
		       
		       ecore_x_window_attributes_get(windows[i], &att);
		       if (att.visible)
			 {
			    ecore_x_window_hide(windows[i]);
			    if (e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd))
			      {
				 edd->elock_grab_break_wnd = windows[i];
				 free(windows);
				 goto works;
			      }
			    ecore_x_window_show(windows[i]);
			 }
		    }
		  free(windows);
	       }
	  }
	/* everything failed - cant lock */
	e_util_dialog_show(_("Lock Failed"),
			   _("Locking the desktop failed because some application<br>"
			     "has grabbed either the keyboard or the mouse or both<br>"
			     "and their grab is unable to be broken."));
	ecore_x_window_del(edd->elock_wnd);
	free(edd);
	edd = NULL;
	return 0;
     }
   works:
   
   last_active_zone = current_zone = 
     e_zone_current_get(e_container_current_get(e_manager_current_get()));
   
   zone_counter = 0;
   total_zone_num = _e_desklock_zone_num_get();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  E_Zone *zone;
		  
		  zone = l3->data;
		  edp = E_NEW(E_Desklock_Popup_Data, 1);
		  if (edp)
		    {
		       edp->popup_wnd = e_popup_new(zone, 0, 0, zone->w, zone->h);
		       evas_event_feed_mouse_move(edp->popup_wnd->evas, -1000000, -1000000,
						  ecore_x_current_time_get(), NULL);
		       
		       e_popup_layer_set(edp->popup_wnd, ELOCK_POPUP_LAYER);
		       ecore_evas_raise(edp->popup_wnd->ecore_evas);
		       
		       evas_event_freeze(edp->popup_wnd->evas);
		       edp->bg_object = edje_object_add(edp->popup_wnd->evas);

		       if ((!e_config->desklock_background) ||
			   (!strcmp(e_config->desklock_background, "theme_desklock_background")))
			 {
			   e_theme_edje_object_set(edp->bg_object,
						   "base/theme/desklock",
						   "desklock/background");
			 }
		       else if (!strcmp(e_config->desklock_background, "theme_background"))
			 {
			    e_theme_edje_object_set(edp->bg_object,
						    "base/theme/backgrounds",
						    "desktop/background");
			 }
		       else
			 {
			   if (e_util_edje_collection_exists(e_config->desklock_background,
							     "desklock/background"))
			     {
			       edje_object_file_set(edp->bg_object, e_config->desklock_background,
						    "desklock/background");
			     }
			   else
			     {
			       if (!edje_object_file_set(edp->bg_object,
							 e_config->desklock_background,
							 "desktop/background"))
				 {
				   edje_object_file_set(edp->bg_object,
						      e_theme_edje_file_get("base/theme/desklock",
									    "desklock/background"),
									    "desklock/background");
				 }
			     }
			 }

		       evas_object_move(edp->bg_object, 0, 0);
		       evas_object_resize(edp->bg_object, zone->w, zone->h);
		       evas_object_show(edp->bg_object);
		       edp->login_box = edje_object_add(edp->popup_wnd->evas);
		       e_theme_edje_object_set(edp->login_box,
					       "base/theme/desklock",
					       "desklock/login_box");
		       edje_object_part_text_set(edp->login_box, "title", 
						 _("Please enter your unlock password"));
		       edje_object_part_swallow(edp->bg_object, "login_box", edp->login_box);
		       edje_object_size_min_calc(edp->login_box, &mw, &mh);
		       evas_object_move(edp->login_box, (int)((zone->w - mw)/2),
						    (int)((zone->h - mh)/2));
		       
		       if (total_zone_num > 1)
			 {
			    if (e_config->desklock_login_box_zone == -1)
			      evas_object_show(edp->login_box);
			    else if(e_config->desklock_login_box_zone == -2 && zone == current_zone)
			      evas_object_show(edp->login_box);
			    else if(e_config->desklock_login_box_zone == zone_counter )
			      evas_object_show(edp->login_box);
			 }
		       else
			 evas_object_show(edp->login_box);
		       /**/
		       
		       e_popup_edje_bg_object_set(edp->popup_wnd, edp->bg_object);
		       evas_event_thaw(edp->popup_wnd->evas);
		       
		       e_popup_show(edp->popup_wnd);
		       
		       edd->elock_wnd_list = evas_list_append(edd->elock_wnd_list, edp);
		    }

		  zone_counter ++;
	       }
	  }
     }
   
   /* handlers */
   edd->handlers = evas_list_append(edd->handlers,
				    ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
							    _e_desklock_cb_key_down, NULL));
   edd->handlers = evas_list_append(edd->handlers, 
				    ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
							    _e_desklock_cb_mouse_down, NULL));
   edd->handlers = evas_list_append(edd->handlers,
				    ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
							    _e_desklock_cb_mouse_up, NULL));
   edd->handlers = evas_list_append(edd->handlers,
				    ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
							    _e_desklock_cb_mouse_wheel,
							    NULL));
   if (total_zone_num > 1 && e_config->desklock_login_box_zone == -2 )
     edd->handlers = evas_list_append(edd->handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,
							      _e_desklock_cb_mouse_move,
							      NULL));
   _e_desklock_passwd_update();
   return 1;
}

EAPI void
e_desklock_hide(void)
{
   E_Desklock_Popup_Data	*edp;
   
   if (!edd) return;

   if (edd->elock_grab_break_wnd)
     ecore_x_window_show(edd->elock_grab_break_wnd);
   while (edd->elock_wnd_list)
     {
	edp = edd->elock_wnd_list->data;
	if (edp)
	  {
	     e_popup_hide(edp->popup_wnd);
	     
	     evas_event_freeze(edp->popup_wnd->evas);
	     evas_object_del(edp->bg_object);
	     evas_object_del(edp->login_box);
	     evas_event_thaw(edp->popup_wnd->evas);
	     
	     e_object_del(E_OBJECT(edp->popup_wnd));
	     E_FREE(edp);
	  }
	edd->elock_wnd_list = evas_list_remove_list(edd->elock_wnd_list, edd->elock_wnd_list);
     }
   
   while (edd->handlers)
     {
	ecore_event_handler_del(edd->handlers->data);
	edd->handlers = evas_list_remove_list(edd->handlers, edd->handlers);
     }
   
   e_grabinput_release(edd->elock_wnd, edd->elock_wnd);
   ecore_x_window_del(edd->elock_wnd);
   
   E_FREE(edd);
   edd = NULL;
}

static int
_e_desklock_cb_key_down(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev;
   
   ev = event;
   if (ev->win != edd->elock_wnd) return 1;
   
   if (!strcmp(ev->keysymbol, "Escape"))
    ;
   else if (!strcmp(ev->keysymbol, "KP_Enter"))
     _e_desklock_check_auth();
   else if (!strcmp(ev->keysymbol, "Return"))
     _e_desklock_check_auth();
   else if (!strcmp(ev->keysymbol, "BackSpace"))
     _e_desklock_backspace();
   else if (!strcmp(ev->keysymbol, "Delete"))
     _e_desklock_delete();
   else
     {
	// here we have to grab a password
	if (ev->key_compose)
	  {
	     if ((strlen(edd->passwd) < (PASSWD_LEN - strlen(ev->key_compose))))
	       {
		  strcat(edd->passwd, ev->key_compose);
		  _e_desklock_passwd_update();
	       }
	  }
     }
   
   return 1;
}

static int
_e_desklock_cb_mouse_down(void *data, int type, void *event)
{
   return 1;
}

static int
_e_desklock_cb_mouse_up(void *data, int type, void *event)
{
   return 1;
}

static int
_e_desklock_cb_mouse_wheel(void *data, int type, void *event)
{
   return 1;
}

static int
_e_desklock_cb_mouse_move(void *data, int type, void *event)
{
   E_Desklock_Popup_Data	*edp;
   E_Zone *current_zone;
   Evas_List *l;
   
   current_zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
   
   if (current_zone == last_active_zone)
     return 1;
   
   for (l = edd->elock_wnd_list; l; l = l->next)
     {
	edp = l->data;
	
	if (!edp) continue;
	
	if (edp->popup_wnd->zone == last_active_zone)
	  evas_object_hide(edp->login_box);
	else if (edp->popup_wnd->zone == current_zone)
	  evas_object_show(edp->login_box);
     }
   last_active_zone = current_zone;
   return 1;
}

static void
_e_desklock_passwd_update()
{
   int ii;
   char passwd_hidden[PASSWD_LEN * 3]="";
   E_Desklock_Popup_Data	*edp;
   Evas_List *l;
   
   if (!edd) return;
   
   for (ii = 0; ii < strlen(edd->passwd); ii ++)
     {
	passwd_hidden[ii] = '*';
	passwd_hidden[ii+1] = 0;
     }
   
   for (l = edd->elock_wnd_list; l; l = l->next)
     {
	edp = l->data;
	edje_object_part_text_set(edp->login_box, "passwd", passwd_hidden);
     }
}

static void
_e_desklock_backspace()
{
  int len, val, pos;

  if (!edd) return;
   
   len = strlen(edd->passwd);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(edd->passwd, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     edd->passwd[pos] = 0;
	     _e_desklock_passwd_update();
	  }
     }
}

static void
_e_desklock_delete()
{
   _e_desklock_backspace();
}

static int
_e_desklock_zone_num_get()
{
   int num;
   Evas_List *l, *l2;
   
   num = 0;
   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man = l->data;
	
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con = l2->data;
	     
	     num += evas_list_count(con->zones);
	  }
     }
   
   return num;
}

static int
_e_desklock_check_auth()
{
   if (!edd) return 0;
#ifdef HAVE_PAM
   if (e_config->desklock_auth_method == 0)
     return _desklock_auth(edd->passwd);
   else if (e_config->desklock_auth_method == 1)
     {
#endif
	if ((e_config->desklock_personal_passwd) &&
	    (!strcmp(edd->passwd == NULL ? "" : edd->passwd,
		     e_config->desklock_personal_passwd == NULL ? "" :
		     e_config->desklock_personal_passwd)))
	  {
	     /* password ok */
	     memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
	     e_desklock_hide();
	     return 1;
	  }
#ifdef HAVE_PAM
     }
#endif
   /* passowrd is definitely wrong */
   memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
   _e_desklock_passwd_update();
   return 0;
}

#ifdef HAVE_PAM
static int
_e_desklock_cb_exit(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   
   ev = event;
   if (ev->pid == _e_desklock_child_pid)
     {
	_e_desklock_child_pid = -1;
	/* ok */
	if (ev->exit_code == 0)
	  {
	     memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
	     e_desklock_hide();
	  }
	/* error */
	else if (ev->exit_code < 128)
	  {
	     memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
	     e_desklock_hide();
	     e_util_dialog_show(_("Authentication System Error"),
				_("Authentication via PAM had errors setting up the<br>"
				  "authentication session. The error code was <hilight>%i</hilight>.<br>"
				  "This is bad and should not be happening. Please report this bug.")
				, ev->exit_code);
	  }
	/* failed auth */
	else
	  {
	     memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
	     _e_desklock_passwd_update();
	  }
	ecore_event_handler_del(_e_desklock_exit_handler);
	_e_desklock_exit_handler = NULL;
     }
   return 1;
}
    
static int
_desklock_auth(const char *passwd)
{
   if ((_e_desklock_child_pid = fork()))
     {
	/* parent */
	_e_desklock_exit_handler = 
	  ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_desklock_cb_exit, 
				  NULL);
     }
   else
     {
	/* child */
	int pamerr;
	E_Desklock_Auth da;
        char *current_user;

	current_user = _desklock_auth_get_current_user();
	strncpy(da.user, current_user, PATH_MAX);
	strncpy(da.passwd, passwd, PATH_MAX);
	da.pam.handle = NULL;
	da.pam.conv.conv = NULL;
	da.pam.conv.appdata_ptr = NULL;
	
	pamerr = _desklock_pam_init(&da);
	if (pamerr != PAM_SUCCESS) 
	  {
	    free(current_user);
	    exit(pamerr);
	  }
	pamerr = pam_authenticate(da.pam.handle, 0);
	pam_end(da.pam.handle, pamerr);
	memset(da.passwd, 0, sizeof(da.passwd));
	if (pamerr == PAM_SUCCESS)
	  {
	    free(current_user);
	    exit(0);
	  }
	free(current_user);
	exit(-1);
     }
   return 1;
}

static char *
_desklock_auth_get_current_user(void)
{
   char *user;
   struct passwd *pwent = NULL;

   pwent = getpwuid(getuid());
   user = strdup(pwent->pw_name);
   return user;
}

static int
_desklock_pam_init(E_Desklock_Auth *da)
{
   int pamerr;
   char *current_host;
   char *current_user;
   
   if (!da) return -1;
   
   da->pam.conv.conv = _desklock_auth_pam_conv;
   da->pam.conv.appdata_ptr = da;
   da->pam.handle = NULL;
   
   if ((pamerr = pam_start("system-auth", da->user, &(da->pam.conv),
			   &(da->pam.handle))) != PAM_SUCCESS)
     return pamerr;

   current_user = _desklock_auth_get_current_user();

   if ((pamerr = pam_set_item(da->pam.handle, PAM_USER,
			      current_user)) != PAM_SUCCESS)
     {
       free(current_user);
       return pamerr;
     }
   
   current_host = _desklock_auth_get_current_host();
   if ((pamerr = pam_set_item(da->pam.handle, PAM_RHOST,
			      current_host)) != PAM_SUCCESS)
     {
       free(current_user);
       free(current_host);
       return pamerr;
     }
   
   free(current_user);
   free(current_host);
   return 0;
}

static int
_desklock_auth_pam_conv(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
{
   int replies = 0;
   E_Desklock_Auth *da = (E_Desklock_Auth *)appdata_ptr;
   struct pam_response *reply = NULL;
   
   reply = (struct pam_response *)malloc(sizeof(struct pam_response) *num_msg);
   
   if (!reply)
     return PAM_CONV_ERR;
   
   for (replies = 0; replies < num_msg; replies++)
     {
	switch (msg[replies]->msg_style)
	  {
	   case PAM_PROMPT_ECHO_ON:
	     reply[replies].resp_retcode = PAM_SUCCESS;
	     reply[replies].resp = (char *)strdup(da->user);
	     break;
	   case PAM_PROMPT_ECHO_OFF:
	     reply[replies].resp_retcode = PAM_SUCCESS;
	     reply[replies].resp = (char *)strdup(da->passwd);
	     break;
	   case PAM_ERROR_MSG:
	   case PAM_TEXT_INFO:
	     reply[replies].resp_retcode = PAM_SUCCESS;
	     reply[replies].resp = NULL;
	     break;
	   default:
	     free(reply);
	     return PAM_CONV_ERR;
	  }
     }
   *resp = reply;
   return PAM_SUCCESS;
}

static char *
_desklock_auth_get_current_host(void)
{
   return strdup("localhost");
}
#endif
