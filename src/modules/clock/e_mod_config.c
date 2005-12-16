#include "e.h"
#include "e_mod_main.h"
#include "config.h"

typedef struct _cfdata CFData;
typedef struct _Cfg_File_Data Cfg_File_Data;

struct _cfdata 
{   
   int digital_style;
};

struct _Cfg_File_Data 
{
   E_Config_Dialog *cfd;
   char *file;
};

/* Protos */
static Evas_Object *_create_widgets(E_Config_Dialog *cfd, Evas *evas, Config *cfdata);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);

Clock_Face *clk = NULL;

void 
e_int_config_clock(E_Container *con, Clock_Face *c) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   clk = c;
   
   /* Dialog Methods */
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = NULL;
   v.advanced.create_widgets = NULL;

   /* Create The Dialog */
   cfd = e_config_dialog_new(con, _("Clock Module"), NULL, 0, &v, c);   
}

static void 
_fill_data(CFData *cfdata) 
{
   cfdata->digital_style = clk->conf->digitalStyle;
}

static void 
*_create_data(E_Config_Dialog *cfd) 
{
   CFData *cfdata;
   
   cfdata = E_NEW(CFData, 1);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   free(cfdata);
}

static Evas_Object 
*_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   _fill_data(cfdata);
   
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
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   e_border_button_bindings_ungrab_all();
   clk->conf->digitalStyle = cfdata->digital_style;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _clock_face_cb_config_updated(clk);
   return 1;
}

