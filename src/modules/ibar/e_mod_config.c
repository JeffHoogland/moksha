/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include <Ecore_File.h>

struct _E_Config_Dialog_Data
{
   char *dir;
   int   show_label;
   int   eap_label;
   
   Evas_Object *tlist;
   Evas_Object *radio_name;
   Evas_Object *radio_comment;
   Evas_Object *radio_generic;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static void _cb_entry_ok(char *text, void *data);
static void _cb_confirm_dialog_yes(void *data);
static void _load_tlist(E_Config_Dialog_Data *cfdata);
static void _show_label_cb_change(void *data, Evas_Object *obj);

void 
_config_ibar_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   
   v = E_NEW(E_Config_Dialog_View, 1);

   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;

   snprintf(buf, sizeof(buf), "%s/module.eap", e_module_dir_get(ibar_config->module));
   /* Create The Dialog */
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     _("IBar Configuration"), buf, 0, v, ci);
   ibar_config->config_dialog = cfd;
}

static void 
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata)
{
   if (ci->dir)
     cfdata->dir = strdup(ci->dir);
   else
     cfdata->dir = strdup("");
   cfdata->show_label = ci->show_label;
   cfdata->eap_label = ci->eap_label;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;
   
   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->dir) free(cfdata->dir);
   ibar_config->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ol, *ob, *ot;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_frametable_add(evas, _("Selected Bar Source"), 0);
   ol = e_widget_tlist_add(evas, &(cfdata->dir));
   cfdata->tlist = ol;
   _load_tlist(cfdata);
   e_widget_min_size_set(ol, 140, 140);
//   e_widget_framelist_object_append(of, ol);
   
   e_widget_frametable_object_append(of, ol, 0, 0, 1, 2, 1, 1, 1, 0);
   
   ot = e_widget_table_add(evas, 0);
   ob = e_widget_button_add(evas, _("Add"), "widget/add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "widget/del", _cb_del, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_frametable_object_append(of, ot, 1, 0, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Icon Labels"), 0);
   ob = e_widget_check_add(evas, _("Show Icon Label"), &(cfdata->show_label));
   e_widget_on_change_hook_set(ob, _show_label_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);  
   
   rg = e_widget_radio_group_new(&(cfdata->eap_label));

   cfdata->radio_name = e_widget_radio_add(evas, _("Display Eap Name"), 0, rg);
   e_widget_framelist_object_append(of, cfdata->radio_name);
   if (!cfdata->show_label) e_widget_disabled_set(cfdata->radio_name, 1);

   cfdata->radio_comment = e_widget_radio_add(evas, _("Display Eap Comment"), 1, rg);
   e_widget_framelist_object_append(of, cfdata->radio_comment);
   if (!cfdata->show_label) e_widget_disabled_set(cfdata->radio_comment, 1);
   
   cfdata->radio_generic = e_widget_radio_add(evas, _("Display Eap Generic"), 2, rg);
   e_widget_framelist_object_append(of, cfdata->radio_generic);
   if (!cfdata->show_label) e_widget_disabled_set(cfdata->radio_generic, 1);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;
   
   ci = cfd->data;
   if (ci->dir) evas_stringshare_del(ci->dir);
   ci->dir = NULL;
   if (cfdata->dir) ci->dir = evas_stringshare_add(cfdata->dir);
   ci->show_label = cfdata->show_label;
   ci->eap_label = cfdata->eap_label;
   _ibar_config_update();
   e_config_save_queue();
   return 1;
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   e_entry_dialog_show(_("Create new ibar source"), "enlightenment/e",
		       _("Enter a name for this new source"), "", NULL, NULL,
		       _cb_entry_ok, NULL, cfdata);
}

static void 
_cb_del(void *data, void *data2) 
{
   char buf[4096];
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\".<br>"
				"<br>"
				"Are you sure you want to delete this bar source?"),
	    cfdata->dir);
   
   e_confirm_dialog_show(_("Are you sure you want to delete this bar source?"),
			 "enlightenment/exit", buf, NULL, NULL, _cb_confirm_dialog_yes, NULL, cfdata, NULL);
}

static void
_cb_entry_ok(char *text, void *data) 
{
   char buf[4096];
   char tmp[4096];
   FILE *f;
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", 
	    e_user_homedir_get(), text);

   if (!ecore_file_exists(buf)) 
     {
	ecore_file_mkdir(buf);
	snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s/.order", 
		 e_user_homedir_get(), text);
	f = fopen(buf, "w");
	if (f) 
	  {
	     /* Populate this .order file with some defaults */
	     snprintf(tmp, sizeof(tmp), "xterm.eap\n" "sylpheed.eap\n" 
		      "firefox.eap\n" "openoffice.eap\n" "xchat.eap\n"
		      "gimp.eap\n" "xmms.eap\n");
	     fwrite(tmp, sizeof(char), strlen(tmp), f);
	     fclose(f);
	  }
     }
   
   _load_tlist(data);
}

static void
_cb_confirm_dialog_yes(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   char buf[4096];
   
   cfdata = data;
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", e_user_homedir_get(), cfdata->dir);
   if (ecore_file_is_dir(buf))
     ecore_file_recursive_rm(buf);
   
   _load_tlist(cfdata);
}

static void 
_load_tlist(E_Config_Dialog_Data *cfdata) 
{
   Ecore_List *dirs;
   char *home, buf[4096], *file;
   int selnum = -1;

   e_widget_tlist_clear(cfdata->tlist);
   
   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar", home);
   dirs = ecore_file_ls(buf);
   
   if (dirs)
     {
	int i;
	
	i = 0;
	while ((file = ecore_list_next(dirs)))
	  {
	     if (file[0] == '.') continue;
	     snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", home, file);
	     if (ecore_file_is_dir(buf))
	       {
		  e_widget_tlist_append(cfdata->tlist, file, NULL, NULL, file);
		  if ((cfdata->dir) && (!strcmp(cfdata->dir, file)))
		    selnum = i;
		  i++;
	       }
	  }
	ecore_list_destroy(dirs);
     }
   free(home);
   e_widget_tlist_go(cfdata->tlist);
   if (selnum >= 0)
     e_widget_tlist_selected_set(cfdata->tlist, selnum);   
}

static void 
_show_label_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   
   e_widget_disabled_set(cfdata->radio_name, !cfdata->show_label);
   e_widget_disabled_set(cfdata->radio_comment, !cfdata->show_label);
   e_widget_disabled_set(cfdata->radio_generic, !cfdata->show_label);   
}
