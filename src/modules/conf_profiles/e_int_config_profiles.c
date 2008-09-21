#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _apply_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _ilist_fill(E_Config_Dialog_Data *cfdata);
static void _ilist_cb_selected(void *data);
static void _cb_add(void *data, void *data2);
static void _cb_select(void *data, void *data2);
static void _cb_delete(void *data, void *data2);
static void _cb_dialog_yes(void *data);
static void _cb_dialog_destroy(void *data);

EAPI E_Dialog *_dia_new_profile(E_Config_Dialog_Data *cfdata);
static void _new_profile_cb_close(void *data, E_Dialog *dia);
static void _new_profile_cb_ok(void *data, E_Dialog *dia);
static void _new_profile_cb_dia_del(void *obj);


struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas_Object *o_list;
   Evas_Object *o_delete;
   char *sel_profile;

   E_Dialog *dia_new_profile;
   char *new_profile;
   int new_profile_type;
};

typedef struct _Del_Profile_Confirm_Data Del_Profile_Confirm_Data;
struct _Del_Profile_Confirm_Data
{
   E_Config_Dialog_Data *cfdata;
};

EAPI E_Config_Dialog *
e_int_config_profiles(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_profiles_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _apply_cfdata;
   v->basic.create_widgets = _create_widgets;

   cfd = e_config_dialog_new(con,
			     _("Profile Selector"),
			    "E", "_config_profiles_dialog",
			     "enlightenment/profiles", 0, v, NULL);
   e_config_dialog_changed_auto_set(cfd, 0);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_apply_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   const char *cur_profile;
   E_Action *a;

   cur_profile = e_config_profile_get();
   if (strcmp (cur_profile, cfdata->sel_profile) == 0)
     return 1;

   e_config_save_flush();
   e_config_profile_set(cfdata->sel_profile);
   e_config_profile_save();
   e_config_save_block_set(1);

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   return 1;
}

static Evas_Object *
_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ot, *ob;

   o = e_widget_list_add(evas, 0, 1);

   of = e_widget_framelist_add(evas, _("Available Profiles"), 0);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, &(cfdata->sel_profile));
   e_widget_min_size_set(cfdata->o_list, 155, 150);
   e_widget_framelist_object_append(of, cfdata->o_list);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   ob = e_widget_button_add(evas, _("Add"), "widget/add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_delete = e_widget_button_add(evas, _("Delete"), "widget/del", _cb_delete, cfdata, NULL);
   e_widget_table_object_append(ot, cfdata->o_delete, 0, 1, 1, 1, 1, 1, 0, 0);

   e_widget_disabled_set(cfdata->o_delete, 1);

   e_widget_list_object_append(o, ot, 1, 0, 0.0);

   _ilist_fill(cfdata);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static void
_ilist_fill(E_Config_Dialog_Data *cfdata)
{
   Evas *evas;
   Evas_List *l;
   const char *cur_profile;

   if (!cfdata) return;
   if (!cfdata->o_list) return;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   e_widget_ilist_clear(cfdata->o_list);
   e_widget_ilist_go(cfdata->o_list);

   cur_profile = e_config_profile_get();
   for (l = e_config_profile_list(); l; l = l->next)
     {
	Evas_Object *ob;

	ob = edje_object_add(evas);
	e_widget_ilist_append(cfdata->o_list, ob, l->data, _ilist_cb_selected, cfdata, l->data);
	if (!strcmp(cur_profile, l->data))
	  {
	     e_util_edje_icon_set(ob, "enlightenment/check");
	     e_widget_ilist_selected_set(cfdata->o_list, e_widget_ilist_count(cfdata->o_list));
	  }
     }

   e_widget_min_size_set(cfdata->o_list, 155, 250);
   e_widget_ilist_go(cfdata->o_list);

   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_ilist_cb_selected(void *data)
{
   E_Config_Dialog_Data *cfdata;
   const char *cur_profile;
   unsigned char v;

   cfdata = data;
   if (!cfdata) return;

   cur_profile = e_config_profile_get();

   v = strcmp(cur_profile, cfdata->sel_profile) == 0;
   e_widget_disabled_set(cfdata->o_delete, v);
   e_config_dialog_changed_set(cfdata->cfd, !v);
}

static void
_cb_add(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;

   if (cfdata->dia_new_profile)
     e_win_raise(cfdata->dia_new_profile->win);
   else
     cfdata->dia_new_profile = _dia_new_profile(cfdata);
}

static void
_cb_delete(void *data, void *data2)
{
   Del_Profile_Confirm_Data *d;
   char buf[4096];

   d = E_NEW(Del_Profile_Confirm_Data, 1);
   if (!d) return;
   d->cfdata = data;
   if (!d->cfdata) return;

   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\".<br><br>"
				"Are you sure you want to delete this profile?"),
                                d->cfdata->sel_profile);
   e_confirm_dialog_show(_("Are you sure you want to delete this profile?"),
			   "enlightenment/exit", buf, NULL, NULL, _cb_dialog_yes, NULL, d, NULL,
                           _cb_dialog_destroy, d);
}

static void
_cb_dialog_yes(void *data)
{
   Del_Profile_Confirm_Data *d;

   d = data;
   if (!data) return;

   e_config_profile_del(d->cfdata->sel_profile);
   e_config_save_queue();
   _ilist_fill(d->cfdata);
}

static void
_cb_dialog_destroy(void *data)
{
   Del_Profile_Confirm_Data *d;

   d = data;
   if (!data) return;

   E_FREE(d);
}

EAPI E_Dialog *
_dia_new_profile(E_Config_Dialog_Data *cfdata)
{
   E_Dialog *dia;
   E_Radio_Group *rg;
   Evas *evas;
   Evas_Coord mw, mh;
   Evas_Object *ot, *ob;

   dia = e_dialog_new(cfdata->cfd->con, "E", "profiles_new_profile_dialog");
   if (!dia) return NULL;
   dia->data = cfdata;

   e_object_del_attach_func_set(E_OBJECT(dia), _new_profile_cb_dia_del);
   e_win_centered_set(dia->win, 1);

   evas = e_win_evas_get(dia->win);

   e_dialog_title_set(dia, _("Add New Profile"));

   ot = e_widget_table_add(evas, 0);
   ob = e_widget_label_add(evas, _("Name:"));
   e_widget_table_object_append(ot, ob,
				     0, 0, 1, 1,
				     0, 1, 0, 0);
   ob = e_widget_entry_add(evas, &(cfdata->new_profile), NULL, NULL, NULL);
   e_widget_min_size_set(ob, 100, 1);
   e_widget_table_object_append(ot, ob,
				     1, 0, 1, 1,
				     1, 1, 1, 0);

   rg = e_widget_radio_group_new(&cfdata->new_profile_type);
   ob = e_widget_radio_add(evas, _("Plain Profile"), 0, rg);
   e_widget_table_object_append(ot, ob,
				     0, 1, 2, 1,
				     1, 1, 1, 0);
   ob = e_widget_radio_add(evas, _("Clone Current Profile"), 1, rg);
   e_widget_table_object_append(ot, ob,
				     0, 2, 2, 1,
				     1, 1, 1, 0);

   e_widget_min_size_get(ot, &mw, &mh);
   e_dialog_content_set(dia, ot, mw, mh);

   e_dialog_button_add(dia, _("OK"), NULL, _new_profile_cb_ok, cfdata);
   e_dialog_button_add(dia, _("Cancel"), NULL, _new_profile_cb_close, cfdata);

   e_dialog_resizable_set(dia, 0);
   e_dialog_show(dia);

   return dia;
}

static void
_new_profile_cb_close(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;
   if (!cfdata) return;

   e_object_unref(E_OBJECT(dia));
   cfdata->dia_new_profile = NULL;
   cfdata->new_profile = NULL;
   cfdata->new_profile_type = 0;
}

static void
_new_profile_cb_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   char cur_profile[1024];

   cfdata = data;
   if (!cfdata) return;

   snprintf(cur_profile, sizeof (cur_profile), "%s", e_config_profile_get());

   if (cfdata->new_profile)
     {
	e_config_profile_add(cfdata->new_profile);
	if (cfdata->new_profile_type)
	  {
	     e_config_profile_set(cfdata->new_profile);
	     e_config_save();
	     e_config_profile_set(cur_profile);
	  }
     }

   e_object_unref(E_OBJECT(dia));
   cfdata->dia_new_profile = NULL;
   cfdata->new_profile = NULL;
   cfdata->new_profile_type = 0;
   _ilist_fill(cfdata);
}

static void
_new_profile_cb_dia_del(void *obj)
{
   E_Dialog *dia = obj;
   E_Config_Dialog_Data *cfdata = dia->data;

   cfdata->dia_new_profile = NULL;
   cfdata->new_profile = NULL;
   cfdata->new_profile_type = 0;
   e_object_unref(E_OBJECT(dia));
}
