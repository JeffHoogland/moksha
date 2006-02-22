#include "e.h"


static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int  _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					   E_Config_Dialog_Data *cfdata);


static void _e_desklock_passwd_cb_change(void *data, Evas_Object *obj);
static void _e_desklock_cb_show_passwd(void *data, Evas_Object *obj, const char *emission,
				       const char *source);

struct _E_Config_Dialog_Data
{
  char *desklock_passwd;
  char *desklock_passwd_cp;
  int show_password;

  struct {
    Evas_Object	*passwd_field;
  } gui;
};

typedef struct _E_Widget_Entry_Data E_Widget_Entry_Data;
typedef struct _E_Widget_Check_Data E_Widget_Check_Data;

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
  cfdata->show_password = 0;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
  E_Config_Dialog_Data *cfdata;

  cfdata = E_NEW(E_Config_Dialog_Data, 1);
  cfdata->desklock_passwd = strdup("");
  cfdata->desklock_passwd_cp = strdup("");

  return cfdata;
}
static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  E_FREE(cfdata->desklock_passwd);
  E_FREE(cfdata->desklock_passwd_cp);

  free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  if (cfdata->desklock_passwd_cp)
    {
      if (e_config->desklock_personal_passwd)
	{
	  if (strcmp(e_config->desklock_personal_passwd, cfdata->desklock_passwd_cp) == 0)
	    return 1;
	  evas_stringshare_del(e_config->desklock_personal_passwd);
	}
      e_config->desklock_personal_passwd = (char *)evas_stringshare_add(cfdata->desklock_passwd_cp);
      e_config_save_queue();
    }
  return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
  Evas_Object *o, *of, *ob;
  E_Widget_Check_Data *wd;

  _fill_desklock_data(cfdata);

  o = e_widget_list_add(evas, 0, 0);

  of = e_widget_framelist_add(evas, _("Personalized Password:"), 0);

  cfdata->gui.passwd_field = ob = e_widget_entry_add(evas, &(cfdata->desklock_passwd));
  _e_desklock_passwd_cb_change(cfdata, ob);
  e_widget_entry_on_change_callback_set(ob, _e_desklock_passwd_cb_change, cfdata);
  e_widget_min_size_set(ob, 200, 25);
  e_widget_framelist_object_append(of, ob);

  ob = e_widget_check_add(evas, _("Show Password"), &(cfdata->show_password));
  wd = (E_Widget_Check_Data*)e_widget_data_get(ob);

  edje_object_signal_callback_add(wd->o_check,"toggle_on", "", _e_desklock_cb_show_passwd, cfdata);
  edje_object_signal_callback_add(wd->o_check,"toggle_off", "", _e_desklock_cb_show_passwd, cfdata);

  e_widget_framelist_object_append(of, ob);

  e_widget_list_object_append(o, of, 1, 1, 0.5);
  e_dialog_resizable_set(cfd->dia, 0);

  return o;
}

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
