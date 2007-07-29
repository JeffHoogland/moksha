/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _fill_data(E_Config_Dialog_Data *cfdata);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   int group_by;
   int include_all_zones;
   int separate_with;
   int sort_by;
   int separate_iconified_apps;
   int warp_to_iconified_desktop;
   int limit_caption_len;
   int max_caption_len;
};

EAPI E_Config_Dialog *
e_int_config_clientlist(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_clientlist_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;

   cfd = e_config_dialog_new(con, _("Client List Settings"), "E", 
			     "_config_clientlist_dialog",
			     "enlightenment/winlist", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->group_by = e_config->clientlist_group_by;
   cfdata->include_all_zones = e_config->clientlist_include_all_zones;
   cfdata->separate_with = e_config->clientlist_separate_with;
   cfdata->sort_by = e_config->clientlist_sort_by;
   cfdata->separate_iconified_apps = e_config->clientlist_separate_iconified_apps;
   cfdata->warp_to_iconified_desktop = e_config->clientlist_warp_to_iconified_desktop;
   cfdata->limit_caption_len = e_config->clientlist_limit_caption_len;
   cfdata->max_caption_len = e_config->clientlist_max_caption_len;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->clientlist_group_by = cfdata->group_by;
   e_config->clientlist_include_all_zones = cfdata->include_all_zones;
   e_config->clientlist_separate_with = cfdata->separate_with;
   e_config->clientlist_sort_by = cfdata->sort_by;
   e_config->clientlist_separate_iconified_apps = cfdata->separate_iconified_apps;
   e_config->clientlist_warp_to_iconified_desktop = cfdata->warp_to_iconified_desktop;
   e_config->clientlist_limit_caption_len = cfdata->limit_caption_len;
   e_config->clientlist_max_caption_len = cfdata->max_caption_len;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob, *ot;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Group By"), 0);
   rg = e_widget_radio_group_new(&(cfdata->group_by));
   ob = e_widget_radio_add(evas, _("None"), E_CLIENTLIST_GROUP_NONE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Desktop"), E_CLIENTLIST_GROUP_DESK, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Window Class"), E_CLIENTLIST_GROUP_CLASS, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Include windows from other screens"), &(cfdata->include_all_zones));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Separate Groups By"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->separate_with));
   ob = e_widget_radio_add(evas, _("None"), E_CLIENTLIST_GROUP_SEP_NONE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Using separator bars"), E_CLIENTLIST_GROUP_SEP_BAR, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Using menus"), E_CLIENTLIST_GROUP_SEP_MENU, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Client Sort Order"), 0);
   rg = e_widget_radio_group_new(&(cfdata->sort_by));
   ob = e_widget_radio_add(evas, _("None"), E_CLIENTLIST_SORT_NONE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Alphabetical"), E_CLIENTLIST_SORT_ALPHA, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Window stacking layer"), E_CLIENTLIST_SORT_ZORDER, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Most recently used"), E_CLIENTLIST_SORT_MOST_RECENT, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Iconified Windows"), 0);
   rg = e_widget_radio_group_new(&(cfdata->separate_iconified_apps));
   ob = e_widget_radio_add(evas, _("Group with owning desktop"), E_CLIENTLIST_GROUPICONS_OWNER, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Group with current desktop"), E_CLIENTLIST_GROUPICONS_CURRENT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Separate group"), E_CLIENTLIST_GROUPICONS_SEP, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Warp to owning desktop"), &(cfdata->warp_to_iconified_desktop));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Menu Item Captions"), 0);
   ob = e_widget_check_add(evas, _("Limit caption length"), &(cfdata->limit_caption_len));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f Chars"), 2.0, E_CLIENTLIST_MAX_CAPTION_LEN, 2.0, 0, NULL, &(cfdata->max_caption_len), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 2, 2, 1, 1, 1, 1, 0);

   e_widget_list_object_append(o, ot, 1, 1, 0.5); 

   return o;
}
