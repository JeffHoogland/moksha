#include "e.h"
#include "e_mod_main.h"
#include "config.h"

typedef struct _cfdata CFData;
typedef struct _Cfg_File_Data Cfg_File_Data;

struct _cfdata 
{
   int allow_overlap;
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

void 
_config_start_module(E_Container *con, Start *start)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = NULL;
   v.advanced.create_widgets = NULL;
   
   cfd = e_config_dialog_new(con, _("Start Module Configuration"), NULL, 0, &v, start);
}

static void 
_fill_data(Start *s, CFData *cfdata) 
{
   cfdata->allow_overlap = s->conf->allow_overlap;
}

static void 
*_create_data(E_Config_Dialog *cfd) 
{
   CFData *cfdata;
   Start  *s;
   
   s = cfd->data;
   cfdata = E_NEW(CFData, 1);
   _fill_data(s, cfdata);
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
   Start *s;
   
   s = cfd->data;
   _fill_data(s, cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow Overlap"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   Start *s;
   
   s = cfd->data;
   e_border_button_bindings_ungrab_all();

   if (cfdata->allow_overlap && !s->conf->allow_overlap)
     s->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && s->conf->allow_overlap)
     s->conf->allow_overlap = 0;
   
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _start_cb_config_updated(s);
   return 1;
}
