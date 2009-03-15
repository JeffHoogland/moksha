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
static void _cb_reset(void *data, void *data2);
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
   Evas_Object *o_reset;
   Evas_Object *o_text;
   const char *sel_profile;

   E_Dialog *dia_new_profile;
   char *new_profile;
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
			     "preferences-profiles", 0, v, NULL);
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
   const char *dir;
   char buf[PATH_MAX];

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Available Profiles"), 0);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, &(cfdata->sel_profile));
   e_widget_min_size_set(cfdata->o_list, 140 * e_scale, 50 * e_scale);
   e_widget_framelist_object_append(of, cfdata->o_list);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   ob = e_widget_textblock_add(evas);
   e_widget_min_size_set(ob, 140 * e_scale, 50 * e_scale);
   e_widget_textblock_markup_set(ob, _("Select a profile"));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   cfdata->o_text = ob;
   
   ot = e_widget_table_add(evas, 0);
   ob = e_widget_button_add(evas, _("Add"), "list-add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_delete = e_widget_button_add(evas, _("Delete"), "list-remove", _cb_delete, cfdata, NULL);
   e_widget_table_object_append(ot, cfdata->o_delete, 1, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_reset = e_widget_button_add(evas, _("Reset"), "system-restart", _cb_reset, cfdata, NULL);
   e_widget_table_object_align_append(ot, cfdata->o_reset, 2, 0, 1, 1, 0, 1, 1, 1, 1.0, 0.5);

   // if there is a system version of the profile - allow reset
   dir = e_prefix_data_get();
   snprintf(buf, sizeof(buf), "%s/data/config/%s/", dir, e_config_profile_get());
   if (ecore_file_is_dir(buf))
     e_widget_disabled_set(cfdata->o_reset, 0);
   else
     e_widget_disabled_set(cfdata->o_reset, 1);
   
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
   Eina_List *l, *profiles;
   const char *cur_profile;
   int selected = -1, i;
   
   if (!cfdata) return;
   if (!cfdata->o_list) return;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   e_widget_ilist_clear(cfdata->o_list);
   e_widget_ilist_go(cfdata->o_list);

   cur_profile = e_config_profile_get();
   profiles = e_config_profile_list();
   for (i = 0, l = profiles; l; l = l->next, i++)
     {
        Efreet_Desktop *desk = NULL;
	Evas_Object *ic;
        char buf[PATH_MAX], *prof, *pdir;
        const char *label, *dir;
        
        prof = l->data;
        if (e_config_profile_get())
          {
             if (!strcmp(prof, e_config_profile_get())) selected = i;
          }
        pdir = e_config_profile_dir_get(prof);
        snprintf(buf, sizeof(buf), "%s/profile.desktop", pdir);
        desk = efreet_desktop_get(buf);
        if (!desk)
          {
             dir = e_prefix_data_get();
             snprintf(buf, sizeof(buf), "%s/data/config/%s/", dir, prof);
             pdir = strdup(buf);
             if (pdir)
               {
                  snprintf(buf, sizeof(buf), "%s/profile.desktop", pdir);
                  desk = efreet_desktop_get(buf);
               }
          }
        label = prof;
        if ((desk) && (desk->name)) label = desk->name;
        buf[0] = 0;
        if (pdir)
          snprintf(buf, sizeof(buf), "%s/icon.edj", pdir);
        if ((desk) && (desk->icon) && (pdir))
          snprintf(buf, sizeof(buf), "%s/%s", pdir, desk->icon);
        else
          snprintf(buf, sizeof(buf), "%s/data/images/enlightenment.png", e_prefix_data_get());
        ic = e_util_icon_add(buf, evas);
        e_widget_ilist_append(cfdata->o_list, ic, label, _ilist_cb_selected, cfdata, prof);
        if (pdir) free(pdir);
        free(prof);
        if (desk) efreet_desktop_free(desk);
     }
   if (profiles) eina_list_free(profiles);
   if (selected >= 0)
     e_widget_ilist_selected_set(cfdata->o_list, selected);   
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
   const char *cur_profile, *dir;
   unsigned char v;
   Efreet_Desktop *desk = NULL;
   char *pdir, buf[PATH_MAX];
   
   cfdata = data;
   if (!cfdata) return;

   cur_profile = e_config_profile_get();

   v = (strcmp(cur_profile, cfdata->sel_profile) == 0);
   e_widget_disabled_set(cfdata->o_delete, v);
   e_config_dialog_changed_set(cfdata->cfd, !v);

   pdir = e_config_profile_dir_get(cfdata->sel_profile);
   snprintf(buf, sizeof(buf), "%s/profile.desktop", pdir);
   desk = efreet_desktop_get(buf);
   if (!desk)
     {
        dir = e_prefix_data_get();
        snprintf(buf, sizeof(buf), "%s/data/config/%s/", dir, cfdata->sel_profile);
        pdir = strdup(buf);
        if (pdir)
          {
             snprintf(buf, sizeof(buf), "%s/profile.desktop", pdir);
             desk = efreet_desktop_get(buf);
          }
     }
   if (desk)
     e_widget_textblock_markup_set(cfdata->o_text, desk->comment);
   else
     e_widget_textblock_markup_set(cfdata->o_text, _("Unknown"));
   if (desk) efreet_desktop_free(desk);
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

   snprintf(buf, sizeof(buf), 
            _("You want to delete the \"%s\" profile.<br><br>"
              "Are you sure?"),
            d->cfdata->sel_profile);
   e_confirm_dialog_show
     (_("Delete OK?"),
      "dialog-warning", buf, NULL, NULL, _cb_dialog_yes, NULL, d, NULL,
      _cb_dialog_destroy, d);
}

static void
_cb_reset(void *data, void *data2)
{
   E_Action *a;
   char *pdir;
   
   e_config_save_flush();
   e_config_save_block_set(1);

   pdir = e_config_profile_dir_get(e_config_profile_get());
   if (pdir)
     {
        ecore_file_recursive_rm(pdir);
        free(pdir);
     }
   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
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
}

static void
_new_profile_cb_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   char cur_profile[1024];

   cfdata = data;
   if (!cfdata) return;

   snprintf(cur_profile, sizeof(cur_profile), "%s", e_config_profile_get());

   if (cfdata->new_profile)
     {
	e_config_profile_add(cfdata->new_profile);
        e_config_profile_set(cfdata->new_profile);
        e_config_save();
        e_config_profile_set(cur_profile);
     }

   e_object_unref(E_OBJECT(dia));
   cfdata->dia_new_profile = NULL;
   cfdata->new_profile = NULL;
   _ilist_fill(cfdata);
}

static void
_new_profile_cb_dia_del(void *obj)
{
   E_Dialog *dia = obj;
   E_Config_Dialog_Data *cfdata = dia->data;

   cfdata->dia_new_profile = NULL;
   cfdata->new_profile = NULL;
   e_object_unref(E_OBJECT(dia));
}
