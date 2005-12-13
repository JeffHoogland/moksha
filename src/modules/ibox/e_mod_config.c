#include "e.h"
#include "e_mod_main.h"
#include "config.h"

typedef struct _cfdata CFData;
typedef struct _Cfg_File_Data Cfg_File_Data;

#define ICONSIZE_SMALL 24
#define ICONSIZE_MEDIUM 32
#define ICONSIZE_LARGE 40
#define ICONSIZE_VERYLARGE 48

struct _cfdata
{
   IBox *ibox;

   /* Basic Config */
   int method;
   int icon_method;

   int follower;
   int width;
   int iconsize;

   /* Advanced Config */
   double follow_speed;
   double autoscroll_speed;
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
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);

IBox *ib = NULL;

void e_int_config_ibox(E_Container *con, IBox *ibox)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;

   ib = ibox;

   /* Dialog Methods */
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;

   /* Create The Dialog */
   cfd = e_config_dialog_new(con, _("IBox Module"), NULL, 0, &v, ibox);
}

static void _fill_data(CFData *cfdata)
{
   cfdata->follower = ib->conf->follower;
   cfdata->width = ib->conf->width;
   if (cfdata->width == IBOX_WIDTH_AUTO)
     {
	cfdata->method = 1;
     }
   else
     {
	cfdata->method = 0;
     }

   cfdata->iconsize = ib->conf->iconsize;
   if (cfdata->iconsize <=24) cfdata->icon_method = ICONSIZE_SMALL;
   if ((cfdata->iconsize > 24) && (cfdata->iconsize <=32)) cfdata->icon_method = ICONSIZE_MEDIUM;
   if ((cfdata->iconsize > 32) && (cfdata->iconsize <=40)) cfdata->icon_method = ICONSIZE_LARGE;
   if (cfdata->iconsize > 40) cfdata->icon_method = ICONSIZE_VERYLARGE;

   cfdata->follow_speed = ib->conf->follow_speed;
   cfdata->autoscroll_speed = ib->conf->autoscroll_speed;
}

static void *_create_data(E_Config_Dialog *cfd)
{
   CFData *cfdata;

   cfdata = E_NEW(CFData, 1);
   return cfdata;
}

static void _free_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   free(cfdata);
}

static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;

   _fill_data(cfdata);
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Follower"), &(cfdata->follower));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Auto Fit Icons"), &(cfdata->method));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Icon Size"), 0);
   rg = e_widget_radio_group_new(&(cfdata->icon_method));

   ob = e_widget_radio_add(evas, _("Small"), ICONSIZE_SMALL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Medium"), ICONSIZE_MEDIUM, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Large"), ICONSIZE_LARGE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Very Large"), ICONSIZE_VERYLARGE, rg);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   e_border_button_bindings_ungrab_all();
   ib->conf->follower = cfdata->follower;

   if (cfdata->method == 0)
     {
	cfdata->width = IBOX_WIDTH_FIXED;
     }
   else
     {
	cfdata->width = IBOX_WIDTH_AUTO;
     }
   ib->conf->width = cfdata->width;

   if (cfdata->icon_method == ICONSIZE_SMALL) ib->conf->iconsize = 24;
   if (cfdata->icon_method == ICONSIZE_MEDIUM) ib->conf->iconsize = 32;
   if (cfdata->icon_method == ICONSIZE_LARGE) ib->conf->iconsize = 40;
   if (cfdata->icon_method == ICONSIZE_VERYLARGE) ib->conf->iconsize = 48;
   
   e_border_button_bindings_grab_all();
   e_config_save_queue();

   _ibox_box_cb_config_updated(ib);
   return 1;
}

static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   Evas_Object *o, *of, *ob, *ot;
   E_Radio_Group *rg;

   _fill_data(cfdata);
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Follower"), &(cfdata->follower));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Auto Fit Icons"), &(cfdata->method));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Icon Settings"), 0);
   ob = e_widget_label_add(evas, _("Icon Size:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels"), 8, 128, 1, 0,  NULL, &(cfdata->iconsize), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

	/* Advanced Options */
   of = e_widget_framelist_add(evas, _("Advanced Settings"), 0);
   ob = e_widget_label_add(evas, _("Follow Speed:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.01, 0.99, 0.01, 0,  &(cfdata->follow_speed), NULL,200);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Autoscroll Speed:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.01, 1.0, 0.01, 0,  &(cfdata->autoscroll_speed), NULL,200);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   e_border_button_bindings_ungrab_all();
   ib->conf->follower = cfdata->follower;

   if (cfdata->method == 0)
     {
	cfdata->width = IBOX_WIDTH_FIXED;
     }
   else
     {
	cfdata->width = IBOX_WIDTH_AUTO;
     }
   ib->conf->width = cfdata->width;

   ib->conf->iconsize = cfdata->iconsize;

   ib->conf->follow_speed = cfdata->follow_speed;
   ib->conf->autoscroll_speed = cfdata->autoscroll_speed;

   e_border_button_bindings_grab_all();
   e_config_save_queue();

   _ibox_box_cb_config_updated(ib);
   return 1;
}
