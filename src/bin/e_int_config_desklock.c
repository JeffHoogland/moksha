#include "e.h"

#define LOGINBOX_SHOW_ALL_SCREENS	  -1
#define LOGINBOX_SHOW_CURRENT_SCREENS     -2
#define LOGINBOX_SHOW_SPECIFIC_SCREEN	  0

#define BG_LIST_ICON_SIZE_W 32
#define BG_LIST_ICON_SIZE_H 32

#define BG_PREVIEW_W 280
#define BG_PREVIEW_H 200

#define DEF_DESKLOCK_BACKGROUND	"theme_desklock_background"
#define DEF_THEME_BACKGROUND	"theme_background"


static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int  _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					   E_Config_Dialog_Data *cfdata);

static int  _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					      E_Config_Dialog_Data *cfdata);


/******************************************************************************************/

static void _e_desklock_passwd_cb_change(void *data, Evas_Object *obj);
static void _e_desklock_cb_show_passwd(void *data, Evas_Object *obj, const char *emission,
				       const char *source);
static int  _e_desklock_zone_num_get();

static void _load_bgs(E_Config_Dialog_Data *cfdata);
static void _ibg_list_cb_bg_selected(void *data);

static void _e_desklock_cb_lb_show_on_all_screens(void *data, Evas_Object *obj,
						  const char *emission, const char *source);
static void _e_desklock_cb_lb_show_on_current_screen(void *data, Evas_Object *obj,
						     const char *emission, const char *source);
static void _e_desklock_cb_lb_show_on_specific_screen(void *data, Evas_Object *obj,
						      const char *emission, const char *source);

#ifdef HAVE_PAM
static void _e_desklock_cb_syswide_auth_method(void *data, Evas_Object *obj,
					       const char *emission, const char *source);
static void _e_desklock_cb_personilized_auth_method(void *data, Evas_Object *obj,
						    const char *emission, const char *source);
#endif

/*******************************************************************************************/

struct _E_Config_Dialog_Data
{
   char *desklock_passwd;
   char *desklock_passwd_cp;
   int show_password; // local
   
   int autolock; // in e_config;
   int use_timeout; // in e_config;
   double timeout; // in e_config;
   
   int login_box_zone; // in e_config;
   int specific_lb_zone; // local variable
   int specific_lb_zone_backup; // used to have smart iface
   
   int zone_count; // local variable;
   
   char *cur_bg; // local variable;
   Evas *evas; // local variable
   Evas_Object *preview_image; // local variable

#ifdef HAVE_PAM
   int auth_method;
#endif
   
   struct {
      Evas_Object *passwd_field;
      Evas_Object *bg_list;

      Evas_Object *show_passwd_check;
      
      struct {
	 Evas_Object *show_all_screens;
	 Evas_Object *show_current_screen;
	 Evas_Object *show_specific_screen;
	 Evas_Object *screen_slider;
      } loginbox_obj;
   } gui;
};

typedef struct _E_Widget_Entry_Data   E_Widget_Entry_Data;
typedef struct _E_Widget_Check_Data   E_Widget_Check_Data;
typedef	struct _E_Widget_Radio_Data   E_Widget_Radio_Data;
typedef struct _E_Widget_Slider_Data  E_Widget_Slider_Data;

struct _E_Widget_Entry_Data
{
   Evas_Object *o_entry;
   Evas_Object *obj;
   char **valptr;
   void (*on_change_func) (void *data, Evas_Object *obj);
   void  *on_change_data;
};

struct _E_Widget_Check_Data
{
   Evas_Object *o_check;
   int *valptr;
};

struct _E_Widget_Radio_Data
{
  E_Radio_Group *group;
  Evas_Object	*o_radio;
  int		valnum;
};

struct _E_Widget_Slider_Data
{
  Evas_Object *o_widget, *o_slider;
  double *dval;
  int	 *ival;
};

EAPI E_Config_Dialog *
e_int_config_desklock(E_Container *con)
{
  E_Config_Dialog *cfd;
  E_Config_Dialog_View *v;

  v = E_NEW(E_Config_Dialog_View, 1);

  v->create_cfdata = _create_data;
  v->free_cfdata = _free_data;
  v->basic.apply_cfdata = _basic_apply_data;
  v->basic.create_widgets = _basic_create_widgets;
  v->advanced.apply_cfdata = _advanced_apply_data;
  v->advanced.create_widgets = _advanced_create_widgets;

  v->override_auto_apply = 1;

  cfd = e_config_dialog_new(con, _("Desktop Lock Settings"), NULL, 0, v, NULL);
  return cfd;
}

static void
_fill_desklock_data(E_Config_Dialog_Data *cfdata)
{
   // we have to read it from e_config->...
   if (e_config->desklock_personal_passwd)
     {
	cfdata->desklock_passwd = strdup(e_config->desklock_personal_passwd);
	cfdata->desklock_passwd_cp = strdup(e_config->desklock_personal_passwd);
     }
   else
     {
	cfdata->desklock_passwd = strdup("");
	cfdata->desklock_passwd_cp = strdup("");
     }
   
   cfdata->autolock = e_config->desklock_autolock;
   cfdata->use_timeout = e_config->desklock_use_timeout;
   cfdata->timeout = e_config->desklock_timeout;
   
   /* should be taken from e_config */
   //cfdata->login_box_on_zone = -1;
   
   if (e_config->desklock_login_box_zone >= 0)
     {
	cfdata->login_box_zone = LOGINBOX_SHOW_SPECIFIC_SCREEN;
	cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone =
	  e_config->desklock_login_box_zone;
     }
   else
     {
	cfdata->login_box_zone = e_config->desklock_login_box_zone;
	cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone = 0;
     }
   
   cfdata->zone_count = _e_desklock_zone_num_get();
   
   cfdata->show_password = 0;
   
   if (!e_config->desklock_background)
     cfdata->cur_bg = strdup(DEF_DESKLOCK_BACKGROUND);
   else
     cfdata->cur_bg = strdup(e_config->desklock_background);

#ifdef HAVE_PAM
   cfdata->auth_method = e_config->desklock_auth_method;
#endif
   
   //vertical_lb_align = e_config->desklock_login
}

static void *
_create_data(E_Config_Dialog *cfd)
{
  E_Config_Dialog_Data *cfdata;

  cfdata = E_NEW(E_Config_Dialog_Data, 1);
  cfdata->desklock_passwd = strdup("");
  cfdata->desklock_passwd_cp = strdup("");

  _fill_desklock_data(cfdata);

  return cfdata;
}
static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  E_FREE(cfdata->desklock_passwd);
  E_FREE(cfdata->desklock_passwd_cp);
  E_FREE(cfdata->cur_bg);

  free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->desklock_passwd_cp)
     {
	if (e_config->desklock_personal_passwd)
	  evas_stringshare_del(e_config->desklock_personal_passwd);
     }
   e_config->desklock_personal_passwd = (char *)evas_stringshare_add(cfdata->desklock_passwd_cp);
   e_config->desklock_autolock = cfdata->autolock;
   e_config->desklock_use_timeout = cfdata->use_timeout;
   e_config->desklock_timeout = cfdata->timeout;
#ifdef HAVE_PAM
   e_config->desklock_auth_method = cfdata->auth_method;
#endif
   if (e_config->desklock_use_timeout)
     {
	ecore_x_screensaver_timeout_set(e_config->desklock_timeout);
     }
   e_config_save_queue();
  return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Widget_Check_Data *wd;
   E_Widget_Radio_Data *rwd;

#ifdef HAVE_PAM
   E_Radio_Group  *rg_auth;
   Evas_Object	  *oc;
#endif
   
   //_fill_desklock_data(cfdata);
   
   o = e_widget_list_add(evas, 0, 0);

#ifdef HAVE_PAM
   of = e_widget_framelist_add(evas, _("Password Type"), 0);
   {
     rg_auth = e_widget_radio_group_new((int*)(&cfdata->auth_method));
     oc = e_widget_radio_add(evas, _("Use my login password"), 0, rg_auth);
     e_widget_framelist_object_append(of, oc);

     rwd = e_widget_data_get(oc);
     edje_object_signal_callback_add(rwd->o_radio, "toggle_on", "",
				     _e_desklock_cb_syswide_auth_method, cfdata);

     oc = e_widget_radio_add(evas, _("Personalized password"), 1, rg_auth);
     e_widget_framelist_object_append(of,oc);

     rwd = e_widget_data_get(oc);
     edje_object_signal_callback_add(rwd->o_radio, "toggle_on", "",
				     _e_desklock_cb_personilized_auth_method, cfdata);
   }
   e_widget_list_object_append(o, of, 1, 1, 0.5);
#endif
   
   of = e_widget_framelist_add(evas, _("Personalized Password:"), 0);
   
   cfdata->gui.passwd_field = ob = e_widget_entry_add(evas, &(cfdata->desklock_passwd));
#ifdef HAVE_PAM
   if (cfdata->auth_method == 0)
     e_widget_disabled_set(ob, 1);
#endif
   _e_desklock_passwd_cb_change(cfdata, ob);
   e_widget_entry_on_change_callback_set(ob, _e_desklock_passwd_cb_change, cfdata);
   e_widget_min_size_set(ob, 200, 25);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Show password"), &(cfdata->show_password));
   cfdata->gui.show_passwd_check = ob;
#ifdef HAVE_PAM
   if (cfdata->auth_method == 0)
     e_widget_disabled_set(ob, 1);
#endif
   e_widget_framelist_object_append(of, ob);
   
   wd = (E_Widget_Check_Data*)e_widget_data_get(ob);
   edje_object_signal_callback_add(wd->o_check,"toggle_on", "", _e_desklock_cb_show_passwd, cfdata);
   edje_object_signal_callback_add(wd->o_check,"toggle_off", "", _e_desklock_cb_show_passwd, cfdata);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Automatic Locking"), 0);
   e_widget_disabled_set(of, !ecore_x_screensaver_event_available_get());
   
   ob = e_widget_check_add(evas, _("Enable screensaver"), &(cfdata->use_timeout));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Lock when the screensaver starts"), &(cfdata->autolock));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Time until screensaver starts"));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 600.0,
			    1.0, 0, &(cfdata->timeout), NULL,
			    200);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   e_dialog_resizable_set(cfd->dia, 0);
   
   return o;
}

/* advanced window */

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return 0;
   
   if (cfdata->desklock_passwd_cp)
     {
	if (e_config->desklock_personal_passwd)
	  evas_stringshare_del(e_config->desklock_personal_passwd);
     }
   e_config->desklock_personal_passwd = (char *)evas_stringshare_add(cfdata->desklock_passwd_cp);

   if (cfdata->cur_bg)
     {
	if (e_config->desklock_background)
	  evas_stringshare_del(e_config->desklock_background);
	e_config->desklock_background = (char *)evas_stringshare_add(cfdata->cur_bg);
     }
   
   if (_e_desklock_zone_num_get() > 1)
     {
	if (cfdata->login_box_zone >= 0)
	  e_config->desklock_login_box_zone = cfdata->specific_lb_zone;
	else
	  e_config->desklock_login_box_zone = cfdata->login_box_zone;
     }
   else
     e_config->desklock_login_box_zone = LOGINBOX_SHOW_ALL_SCREENS;

   e_config->desklock_autolock = cfdata->autolock;
   e_config->desklock_use_timeout = cfdata->use_timeout;
   e_config->desklock_timeout = cfdata->timeout;
#ifdef HAVE_PAM
   e_config->desklock_auth_method = cfdata->auth_method;
#endif

   if (e_config->desklock_use_timeout)
     {
	ecore_x_screensaver_timeout_set(e_config->desklock_timeout);
     }
   
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
  Evas_Object *o, *of, *of1, *ob;
  E_Widget_Radio_Data *wd;

  E_Radio_Group *rg, *rg_bkg;
  Evas_Object *ot, *ol;

  E_Radio_Group *rg_auth;
  Evas_Object	*oc;
  E_Widget_Check_Data *cwd;

  cfdata->evas = evas;

  ot = e_widget_table_add(evas, 0);
  {
    Evas_Object *ot1;
    /* start: bkg list */
    cfdata->gui.bg_list = e_widget_ilist_add(evas, BG_LIST_ICON_SIZE_W,
      				       BG_LIST_ICON_SIZE_H, &(cfdata->cur_bg));
    {
      e_widget_ilist_selector_set(cfdata->gui.bg_list, 1);
      e_widget_min_size_set(cfdata->gui.bg_list, 180, 200);

      _load_bgs(cfdata);

      e_widget_focus_set(cfdata->gui.bg_list, 1);
      e_widget_ilist_go(cfdata->gui.bg_list);
    }
    e_widget_table_object_append(ot, cfdata->gui.bg_list, 0, 0, 1, 2, 1, 1, 1, 1);
    /* end: bkg list */

    /* start: Desk Lock Window Preview */
     
    e_widget_table_object_append(ot, cfdata->preview_image, 1, 0, 1, 1, 1, 1, 1, 1);
    /* end: Desk Lock Window Preview */

    /* start: login box options */

    /* Actually I do not know if I have to enable this if. However, if it is enabled
     * this options will be seen only for those peoples who has xinerama.
     * Otherwise, all the other world will not even know about them.
     * Let the world know about them. Maybe in the feature somebody will turn this if
     */

    if (1 || _e_desklock_zone_num_get() > 1)
      {
        of = e_widget_framelist_add(evas, _("Login Box Settings"), 0);
        {
          rg = e_widget_radio_group_new((int *)(&(cfdata->login_box_zone)));

          ob = e_widget_radio_add(evas, _("Show on all screen zones"), LOGINBOX_SHOW_ALL_SCREENS,
      			    rg);
          cfdata->gui.loginbox_obj.show_all_screens = ob;
          if (cfdata->zone_count == 1) e_widget_disabled_set(ob, 1);
          e_widget_framelist_object_append(of, ob);

          ob = e_widget_radio_add(evas, _("Show on current screen zone"),
      			    LOGINBOX_SHOW_CURRENT_SCREENS, rg);
          cfdata->gui.loginbox_obj.show_current_screen = ob;
          if (cfdata->zone_count == 1) e_widget_disabled_set(ob, 1);
          e_widget_framelist_object_append(of, ob);

          ob = e_widget_radio_add(evas, _("Show on screen zone #:"), LOGINBOX_SHOW_SPECIFIC_SCREEN,
      			    rg);
          cfdata->gui.loginbox_obj.show_specific_screen = ob;
          if (cfdata->zone_count == 1) e_widget_disabled_set(ob, 1);
          e_widget_framelist_object_append(of, ob);

          ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, (double)(cfdata->zone_count - 1),
      			     1.0, 0, NULL, &(cfdata->specific_lb_zone), 100);
          cfdata->gui.loginbox_obj.screen_slider = ob;
          if (cfdata->zone_count == 1 || cfdata->login_box_zone == LOGINBOX_SHOW_ALL_SCREENS)
	    e_widget_disabled_set(ob, 1);
          e_widget_framelist_object_append(of, ob);
        }
        e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);
      }
    /* end: login box options */

#ifdef HAVE_PAM
      of = e_widget_framelist_add(evas, _("Password Type"), 0);
      {
        rg_auth = e_widget_radio_group_new((int*)(&cfdata->auth_method));
        oc = e_widget_radio_add(evas, _("Use my login password"), 0, rg_auth);
        e_widget_framelist_object_append(of, oc);

        wd = e_widget_data_get(oc);
        edje_object_signal_callback_add(wd->o_radio, "toggle_on", "",
       				 _e_desklock_cb_syswide_auth_method, cfdata);

        oc = e_widget_radio_add(evas, _("Personalized password"), 1, rg_auth);
        e_widget_framelist_object_append(of,oc);

        wd = e_widget_data_get(oc);
        edje_object_signal_callback_add(wd->o_radio, "toggle_on", "",
       				 _e_desklock_cb_personilized_auth_method, cfdata);
      }
      e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);
#endif

      of = e_widget_framelist_add(evas, _("Personalized Password:"), 0);
      {
	cfdata->gui.passwd_field = ob = e_widget_entry_add(evas, &(cfdata->desklock_passwd));
#ifdef HAVE_PAM
	if (cfdata->auth_method == 0)
	  e_widget_disabled_set(ob, 1);
#endif

	_e_desklock_passwd_cb_change(cfdata, ob);
	e_widget_entry_on_change_callback_set(ob, _e_desklock_passwd_cb_change, cfdata);
	e_widget_min_size_set(ob, 200, 25);
	e_widget_framelist_object_append(of, ob);
     
	ob = e_widget_check_add(evas, _("Show password"), &(cfdata->show_password));
	cfdata->gui.show_passwd_check = ob;

#ifdef HAVE_PAM
	if (cfdata->auth_method == 0)
	  e_widget_disabled_set(ob, 1);
#endif
	e_widget_framelist_object_append(of, ob);
     
	cwd = (E_Widget_Check_Data*)e_widget_data_get(ob);
	edje_object_signal_callback_add(cwd->o_check,"toggle_on", "",
					_e_desklock_cb_show_passwd, cfdata);
	edje_object_signal_callback_add(cwd->o_check,"toggle_off", "",
					_e_desklock_cb_show_passwd, cfdata);
      }
#ifdef HAVE_PAM
      e_widget_table_object_append(ot, of, 0, 3, 1, 1, 1, 1, 1, 1);
#else
      e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);
#endif

      of = e_widget_framelist_add(evas, _("Automatic Locking"), 0);
      {
	e_widget_disabled_set(of, !ecore_x_screensaver_event_available_get());
   
	ob = e_widget_check_add(evas, _("Enable screensaver"), &(cfdata->use_timeout));
	e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
	e_widget_framelist_object_append(of, ob);
   
	ob = e_widget_check_add(evas, _("Lock when the screensaver starts"), &(cfdata->autolock));
	e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
	e_widget_framelist_object_append(of, ob);

	ob = e_widget_label_add(evas, _("Time until screensaver starts"));
	e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
				 1.0, 600.0,
				 1.0, 0, &(cfdata->timeout), NULL,
				 200);
	e_widget_framelist_object_append(of, ob);
      }
#ifdef HAVE_PAM
      e_widget_table_object_append(ot, of, 1, 2, 1, 2 ,1 ,1 ,1 ,1);
#else
      e_widget_table_object_append(ot, of, 1, 2, 1, 1 ,1 ,1 ,1 ,1);
#endif
  }

  /* register callbacks for the radios in login box options
   * in order to propertly update interface
   */

  wd = e_widget_data_get(cfdata->gui.loginbox_obj.show_all_screens);
  edje_object_signal_callback_add(wd->o_radio, "toggle_on", "",
				  _e_desklock_cb_lb_show_on_all_screens, cfdata);

  wd = e_widget_data_get(cfdata->gui.loginbox_obj.show_current_screen);
  edje_object_signal_callback_add(wd->o_radio, "toggle_on", "",
				  _e_desklock_cb_lb_show_on_current_screen, cfdata);

  wd = e_widget_data_get(cfdata->gui.loginbox_obj.show_specific_screen);
  edje_object_signal_callback_add(wd->o_radio, "toggle_on", "",
				  _e_desklock_cb_lb_show_on_specific_screen, cfdata);


  e_dialog_resizable_set(cfd->dia, 0);
  return ot;
}


/* general functionality/callbacks */

static void
_e_desklock_passwd_cb_change(void *data, Evas_Object *obj)
{
  E_Widget_Entry_Data *wd;
  E_Config_Dialog_Data	*cfdata;
  char *buf, *ptr;
  int i;

  cfdata = data;

  // here goes the hack to have e_widget_entry look like
  // password entry. However, I think, this should be implemented
  // at least on the e_widget_entry level. The best would be
  // e_entry.
  if (!cfdata->desklock_passwd[0])
  {
    E_FREE(cfdata->desklock_passwd_cp);
    cfdata->desklock_passwd_cp = strdup("");
    return;
  }

  if (strlen(cfdata->desklock_passwd) > strlen(cfdata->desklock_passwd_cp))
    {
      for (i = 0; i < strlen(cfdata->desklock_passwd_cp); i++)
	cfdata->desklock_passwd[i] = cfdata->desklock_passwd_cp[i];
      E_FREE(cfdata->desklock_passwd_cp);
      cfdata->desklock_passwd_cp = strdup(cfdata->desklock_passwd);
    }
  else if (strlen(cfdata->desklock_passwd) < strlen(cfdata->desklock_passwd_cp))
    {
      cfdata->desklock_passwd_cp[strlen(cfdata->desklock_passwd)] = 0;
      E_FREE(cfdata->desklock_passwd);
      cfdata->desklock_passwd = strdup(cfdata->desklock_passwd_cp);
    }
  else
    {
      E_FREE(cfdata->desklock_passwd);
      cfdata->desklock_passwd = strdup(cfdata->desklock_passwd_cp);
    }

  wd = e_widget_data_get(obj);
  
  if (cfdata->show_password)
    {
      e_entry_text_set(wd->o_entry, cfdata->desklock_passwd);
      return;
    }

  for (ptr = cfdata->desklock_passwd; *ptr; ptr++) *ptr = '*';
  e_entry_text_set(wd->o_entry, cfdata->desklock_passwd);
}

static void
_e_desklock_cb_show_passwd(void *data, Evas_Object *obj, const char *emission, const char *source)
{
  E_Config_Dialog_Data *cfdata;
  E_Widget_Entry_Data *wd;

  cfdata = data;
  _e_desklock_passwd_cb_change(cfdata, cfdata->gui.passwd_field);
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

static void
_load_bgs(E_Config_Dialog_Data *cfdata)
{
  Evas_Object *o, *ic, *im;
  Ecore_Evas *eebuf;
  Evas *evasbuf;
  Evas_List *bg_dirs, *bg;
  const char *f, *f1;
  char *c;

  if (!cfdata || !cfdata->gui.bg_list)
    return;

  eebuf = ecore_evas_buffer_new(1, 1);
  evasbuf = ecore_evas_get(eebuf);

  /* Desklock background */
  o = edje_object_add(evasbuf);
  f1 = e_theme_edje_file_get("base/theme/desklock", "desklock/background");
  c = strdup(f1);

  if (edje_object_file_set(o, f1, "desklock/background"))
    {
      if (!e_thumb_exists(c))
	ic = e_thumb_generate_begin(c, BG_LIST_ICON_SIZE_W, BG_LIST_ICON_SIZE_H,
				   cfdata->evas, &ic, NULL, NULL);
      else
	ic = e_thumb_evas_object_get(c, cfdata->evas, BG_LIST_ICON_SIZE_W, BG_LIST_ICON_SIZE_H, 1);

      e_widget_ilist_append(cfdata->gui.bg_list, ic, "Theme DeskLock Background",
			    _ibg_list_cb_bg_selected, cfdata, DEF_DESKLOCK_BACKGROUND);
    }

  if ((!e_config->desklock_background) ||
      (!strcmp(e_config->desklock_background, DEF_DESKLOCK_BACKGROUND)))
    e_widget_ilist_selected_set(cfdata->gui.bg_list, 0);

   im = e_widget_preview_add(cfdata->evas, BG_PREVIEW_W, BG_PREVIEW_H);
   e_widget_preview_edje_set(im, c, "desktop/background");
//   im = e_widget_image_add_from_object(cfdata->evas, bg_obj, BG_PREVIEW_W, BG_PREVIEW_H);
//   e_widget_image_object_set(im, e_thumb_evas_object_get(c, cfdata->evas, BG_PREVIEW_W,
//							 BG_PREVIEW_H, 1));

  evas_object_del(o);
  ecore_evas_free(eebuf);
  free(c);
  /* end: Desklock background */

  /* Theme Background */


  eebuf = ecore_evas_buffer_new(1, 1);
  evasbuf = ecore_evas_get(eebuf);

  o = edje_object_add(evasbuf);
  f = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
  c = strdup(f);
  if (edje_object_file_set(o, f, "desktop/background"))
    {
      if (!e_thumb_exists(c))
	ic = e_thumb_generate_begin(c, BG_LIST_ICON_SIZE_W, BG_LIST_ICON_SIZE_H,
				   cfdata->evas, &ic, NULL, NULL);
      else
	ic = e_thumb_evas_object_get(c, cfdata->evas, BG_LIST_ICON_SIZE_W, BG_LIST_ICON_SIZE_H, 1);

      e_widget_ilist_append(cfdata->gui.bg_list, ic, "Theme Background", _ibg_list_cb_bg_selected,
			    cfdata, DEF_THEME_BACKGROUND);
    }

  if ((e_config->desklock_background) &&
      (!strcmp(e_config->desklock_background, DEF_THEME_BACKGROUND)))
    {
       e_widget_ilist_selected_set(cfdata->gui.bg_list, 1);
       im = e_widget_preview_add(cfdata->evas, BG_PREVIEW_W, BG_PREVIEW_H);
       e_widget_preview_edje_set(im, c, "desktop/background");
//      im = e_widget_image_add_from_object(cfdata->evas, bg_obj, BG_PREVIEW_W, BG_PREVIEW_H);
//      e_widget_image_object_set(im, e_thumb_evas_object_get(c, cfdata->evas, BG_PREVIEW_W,
//							    BG_PREVIEW_H, 1));
    }

  evas_object_del(o);
  ecore_evas_free(eebuf);
  free(c);

  bg_dirs = e_path_dir_list_get(path_backgrounds);
  for (bg = bg_dirs; bg; bg = bg->next)
    {
      E_Path_Dir *d;

      d = bg->data;
      if (ecore_file_is_dir(d->dir))
	{
	  char *bg_file;
	  Ecore_List *bgs;
	  int i = e_widget_ilist_count(cfdata->gui.bg_list);

	  bgs = ecore_file_ls(d->dir);
	  if (!bgs) continue;
	  while ((bg_file = ecore_list_next(bgs)))
	    {
	      char full_path[4096];

	      snprintf(full_path, sizeof(full_path), "%s/%s", d->dir, bg_file);
	      if (ecore_file_is_dir(full_path)) continue;
	      if (!e_util_edje_collection_exists(full_path, "desktop/background")) continue;

	      if (!e_thumb_exists(full_path))
		ic = e_thumb_generate_begin(full_path, BG_LIST_ICON_SIZE_W, BG_LIST_ICON_SIZE_H,
					    cfdata->evas, &ic, NULL, NULL);
	      else
		ic = e_thumb_evas_object_get(full_path, cfdata->evas, BG_LIST_ICON_SIZE_W,
					     BG_LIST_ICON_SIZE_H, 1);

	      e_widget_ilist_append(cfdata->gui.bg_list, ic, ecore_file_strip_ext(bg_file),
				    _ibg_list_cb_bg_selected, cfdata, full_path);

	      if ((e_config->desklock_background) &&
		  (!strcmp(e_config->desklock_background, full_path)))
		{
		   e_widget_ilist_selected_set(cfdata->gui.bg_list, i);
		   evas_object_del(im);
		   im = e_widget_preview_add(cfdata->evas, BG_PREVIEW_W, BG_PREVIEW_H);
		   e_widget_preview_edje_set(im, full_path, "desktop/background");
//		  im = e_widget_image_add_from_object(cfdata->evas, o, BG_PREVIEW_W, BG_PREVIEW_H);
//		  e_widget_image_object_set(im, e_thumb_evas_object_get(full_path, cfdata->evas,
//									BG_PREVIEW_W, BG_PREVIEW_H,
//									1));
		}
	      i++;
	    }
	  free(bg_file);
	  ecore_list_destroy(bgs);
	}
      free(d);
    }
  evas_list_free(bg);
  evas_list_free(bg_dirs);

  cfdata->preview_image = im;
}

static void
_ibg_list_cb_bg_selected(void *data)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   
   if (cfdata->cur_bg[0])
     {
	if (strcmp(cfdata->cur_bg, DEF_DESKLOCK_BACKGROUND) == 0)
	  {
	     const char *theme;
	     
	     theme = e_theme_edje_file_get("base/theme/desklock", "desklock/background");
	     e_widget_preview_edje_set(cfdata->preview_image, theme, "desktop/background");
//	   e_widget_image_object_set(cfdata->preview_image, 
//				    e_thumb_evas_object_get(strdup(theme), cfdata->evas,
//							    BG_PREVIEW_W, BG_PREVIEW_H, 1));
	  }
	else if (strcmp(cfdata->cur_bg, DEF_THEME_BACKGROUND) == 0)
	  {
	     const char *theme;
	     
	     theme = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
	     e_widget_preview_edje_set(cfdata->preview_image, theme, "desktop/background");
//	   e_widget_image_object_set(cfdata->preview_image, 
//				     e_thumb_evas_object_get(strdup(theme), cfdata->evas,
//							     BG_PREVIEW_W, BG_PREVIEW_H, 1));
	  }
	else
	  {
	     e_widget_preview_edje_set(cfdata->preview_image, cfdata->cur_bg, "desktop/background");
//	    e_widget_image_object_set(cfdata->preview_image,
//				      e_thumb_evas_object_get(cfdata->cur_bg, cfdata->evas,
//							      BG_PREVIEW_W, BG_PREVIEW_H, 1));
	  }
     }
   else
     {
	const char *theme;
	
	theme = e_theme_edje_file_get("base/theme/desklock", "desklock/background");
	e_widget_preview_edje_set(cfdata->preview_image, theme, "desktop/background");
//	e_widget_image_object_set(cfdata->preview_image, 
//				  e_thumb_evas_object_get(strdup(theme), cfdata->evas,
//							  BG_PREVIEW_W, BG_PREVIEW_H, 1));
     }
}

static void
_e_desklock_cb_lb_show_on_all_screens(void *data, Evas_Object *obj, const char *emission,
				      const char *source)
{
  E_Config_Dialog_Data *cfdata;

  if (!(cfdata = data)) return;

  cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone;

  e_widget_disabled_set(cfdata->gui.loginbox_obj.screen_slider, 1);
}

static void
_e_desklock_cb_lb_show_on_current_screen(void *data, Evas_Object *obj, const char *emission,
					 const char *source)
{
  _e_desklock_cb_lb_show_on_all_screens(data, obj, emission, source);
}

static void
_e_desklock_cb_lb_show_on_specific_screen(void *data, Evas_Object *obj, const char *emission,
					 const char *source)
{
  E_Widget_Slider_Data	*wd;
  E_Config_Dialog_Data	*cfdata;

  if (!(cfdata = data)) return;

  wd = e_widget_data_get(cfdata->gui.loginbox_obj.screen_slider);
  e_slider_value_set(wd->o_slider, cfdata->specific_lb_zone_backup);
  cfdata->specific_lb_zone = cfdata->specific_lb_zone_backup;

  e_widget_disabled_set(wd->o_widget, 0);
}

#ifdef HAVE_PAM
static void
_e_desklock_cb_syswide_auth_method(void *data, Evas_Object *obj, const char *emission,
				   const char *source)
{
  E_Widget_Entry_Data *ewd;
  E_Widget_Check_Data *cwd;
  E_Config_Dialog_Data *cfdata;

  if (!(cfdata = data)) return;

  e_widget_disabled_set(cfdata->gui.passwd_field, 1);
  e_widget_disabled_set(cfdata->gui.show_passwd_check, 1);
}

static void
_e_desklock_cb_personilized_auth_method(void *data, Evas_Object *obj, const char *emission,
					const char *source)
{
  E_Config_Dialog_Data *cfdata;

  if (!(cfdata = data)) return;

  e_widget_disabled_set(cfdata->gui.passwd_field, 0);
  e_widget_disabled_set(cfdata->gui.show_passwd_check, 0);
}
#endif
