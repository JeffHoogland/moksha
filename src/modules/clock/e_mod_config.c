#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "config.h"

typedef struct _Cfg_File_Data Cfg_File_Data;

struct _E_Config_Dialog_Data
{   
   int digital_style;
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

void 
_config_clock_module(E_Container *con, Clock_Face *c) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   /* Create The Dialog */
   cfd = e_config_dialog_new(con, _("Clock Configuration"), NULL, 0, v, c);   
   c->config_dialog = cfd;
}

static void 
_fill_data(Clock_Face *clk, E_Config_Dialog_Data *cfdata) 
{
   cfdata->digital_style = clk->conf->digitalStyle;
   cfdata->allow_overlap = clk->clock->conf->allow_overlap;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   Clock_Face *cf;
   
   cf = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cf, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Clock_Face *c;
   
   c = cfd->data;
   c->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   rg = e_widget_radio_group_new(&(cfdata->digital_style));
   ob = e_widget_radio_add(evas, _("No Digital Display"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("12 Hour Display"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("24 Hour Display"), 2, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Clock_Face *clk;
   
   clk = cfd->data;
   e_border_button_bindings_ungrab_all();
   clk->conf->digitalStyle = cfdata->digital_style;

   if (cfdata->allow_overlap && !clk->clock->conf->allow_overlap)
     clk->clock->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && clk->clock->conf->allow_overlap)
     clk->clock->conf->allow_overlap = 0;

   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _clock_face_cb_config_updated(clk);
   return 1;
}

