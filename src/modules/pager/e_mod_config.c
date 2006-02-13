#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "config.h"

typedef struct _Cfg_File_Data Cfg_File_Data;

struct _E_Config_Dialog_Data
{
   int show_name;
   int name_pos;
   int show_popup;
   double popup_speed;
   int allow_overlap;
};

struct _Cfg_File_Data 
{
   E_Config_Dialog *cfd;
   char *file;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void 
_config_pager_module(E_Container *con, Pager *pager)
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
   
   cfd = e_config_dialog_new(con, _("Pager Configuration"), NULL, 0, v, pager);
   pager->config_dialog = cfd;
}

static void 
_fill_data(Pager *p, E_Config_Dialog_Data *cfdata) 
{
   /* Name Pos, Show Popup, popup_speed */
   cfdata->name_pos = p->conf->deskname_pos;
   if (cfdata->name_pos == PAGER_DESKNAME_NONE) 
     {
	cfdata->show_name = 0;
	cfdata->name_pos = PAGER_DESKNAME_TOP;
     }
   else 
     {
	cfdata->show_name = 1;
     }
   
   cfdata->show_popup = p->conf->popup;
   cfdata->popup_speed = p->conf->popup_speed;
   cfdata->allow_overlap = p->conf->allow_overlap;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   Pager *p;
   
   p = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(p, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Pager *p;
   
   p = cfd->data;
   p->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show Popup"), &(cfdata->show_popup));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Show Desktop Name"), &(cfdata->show_name));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Desktop Name Position"), 0);   
   rg = e_widget_radio_group_new(&(cfdata->name_pos));
   ob = e_widget_radio_add(evas, _("Top"), PAGER_DESKNAME_TOP, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Bottom"), PAGER_DESKNAME_BOTTOM, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* Not Supported Yet ??
   ob = e_widget_radio_add(evas, _("Left"), PAGER_DESKNAME_LEFT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Right"), PAGER_DESKNAME_RIGHT, rg);
   e_widget_framelist_object_append(of, ob);
   */
   
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Pager *p;
   
   p = cfd->data;
   p->conf->popup = cfdata->show_popup;
   
   p->conf->deskname_pos = cfdata->name_pos;
   if (!cfdata->show_name) p->conf->deskname_pos = PAGER_DESKNAME_NONE;
   
   if (cfdata->allow_overlap && !p->conf->allow_overlap)
     p->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && p->conf->allow_overlap)
     p->conf->allow_overlap = 0;

   e_config_save_queue();
   
   _pager_cb_config_updated(p);
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show Popup"), &(cfdata->show_popup));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Show Desktop Name"), &(cfdata->show_name));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Desktop Name Position"), 0);   
   rg = e_widget_radio_group_new(&(cfdata->name_pos));
   ob = e_widget_radio_add(evas, _("Top"), PAGER_DESKNAME_TOP, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Bottom"), PAGER_DESKNAME_BOTTOM, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   /* Not Supported Yet ??
   ob = e_widget_radio_add(evas, _("Left"), PAGER_DESKNAME_LEFT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Right"), PAGER_DESKNAME_RIGHT, rg);
   e_widget_framelist_object_append(of, ob);
   */

   of = e_widget_framelist_add(evas, _("Popup Settings"), 0);   
   ob = e_widget_label_add(evas, _("Popup Speed"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.1, 10.0, 0.1, 0, &(cfdata->popup_speed), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Pager *p;
   
   p = cfd->data;
   e_border_button_bindings_ungrab_all();
   p->conf->popup = cfdata->show_popup;
   
   p->conf->deskname_pos = cfdata->name_pos;
   if (!cfdata->show_name) p->conf->deskname_pos = PAGER_DESKNAME_NONE;
   p->conf->popup_speed = cfdata->popup_speed;
   
   if (cfdata->allow_overlap && !p->conf->allow_overlap)
     p->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && p->conf->allow_overlap)
     p->conf->allow_overlap = 0;

   e_border_button_bindings_grab_all();
   e_config_save_queue();

   _pager_cb_config_updated(p);
   return 1;
}
