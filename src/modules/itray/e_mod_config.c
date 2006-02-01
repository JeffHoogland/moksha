#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "config.h"

typedef struct _cfdata CFData;
typedef struct _Cfg_File_Data Cfg_File_Data;

struct _cfdata
{
   int rowsize;
   int allow_overlap;
};

struct _Cfg_File_Data
{
   E_Config_Dialog *cfd;
   char *file;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);

void 
_config_itray_module(E_Container *con, ITray *itray)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   
   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;

   /* Create The Dialog */
   cfd = e_config_dialog_new(con, _("ITray Configuration"), NULL, 0, v, itray);
   itray->config_dialog = cfd;
}

static void 
_fill_data(ITray *ib, CFData *cfdata)
{
   cfdata->rowsize = ib->conf->rowsize;
   cfdata->allow_overlap = ib->conf->allow_overlap;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   CFData *cfdata;
   ITray *ib;
   
   ib = cfd->data;
   cfdata = E_NEW(CFData, 1);
   _fill_data(ib, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   ITray *ib;
   
   ib = cfd->data;
   ib->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   Evas_Object *o, *ob, *of;
   ITray *ib;
   
   ib = cfd->data;
   _fill_data(ib, cfdata);

   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   ITray *ib;
   
   ib = cfd->data;
   if (cfdata->allow_overlap && !ib->conf->allow_overlap)
     ib->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && ib->conf->allow_overlap)
     ib->conf->allow_overlap = 0;
   e_config_save_queue();

   _itray_box_cb_config_updated(ib);
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   Evas_Object *o, *of, *ob;
   ITray *ib;
   
   ib = cfd->data;
   _fill_data(ib, cfdata);

   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Number of Rows"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%3.0f"), 1.0, 6.0, 1.0, 0,  NULL, &(cfdata->rowsize), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

static int 
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   ITray *ib;
   
   ib = cfd->data;
   e_border_button_bindings_ungrab_all();
   if (cfdata->rowsize != ib->conf->rowsize) 
     {
	ib->conf->rowsize = cfdata->rowsize;
     }
   if (cfdata->allow_overlap && !ib->conf->allow_overlap)
     ib->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && ib->conf->allow_overlap)
     ib->conf->allow_overlap = 0;
   e_border_button_bindings_grab_all();
   e_config_save_queue();

   _itray_box_cb_config_updated(ib);
   return 1;
}
