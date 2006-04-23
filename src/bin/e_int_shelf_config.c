/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
//typedef struct _CFBorder CFBorder;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   E_Shelf *es;
   E_Config_Shelf *escfg;
   char *style;
   int orient;
   int fit_along;
   int fit_size;
   int size;
   int layering;
};

//struct _CFBorder
//{
//   E_Config_Dialog_Data     *cfdata;
//   const char *bordername;
//};

/* a nice easy setup function that does the dirty work */
EAPI void
e_int_shelf_config(E_Shelf *es)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   if (v)
     {
	/* methods */
	v->create_cfdata           = _create_data;
	v->free_cfdata             = _free_data;
	v->basic.apply_cfdata      = _basic_apply_data;
	v->basic.create_widgets    = _basic_create_widgets;
	v->override_auto_apply = 1;
	
	/* create config diaolg for bd object/data */
	cfd = e_config_dialog_new(es->zone->container, 
				  _("BLAH"), NULL, 0, v, es);
	es->config_dialog = cfd;
     }
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   if (cfdata->escfg->style)
     cfdata->style = strdup(cfdata->escfg->style);
   else
     cfdata->style = strdup("");
   cfdata->orient = cfdata->escfg->orient;
   cfdata->fit_along = cfdata->escfg->fit_along;
   cfdata->fit_size = cfdata->escfg->fit_size;
   cfdata->size = cfdata->escfg->size;
   if ((!cfdata->escfg->popup) && 
       (cfdata->escfg->layer == 1))
     cfdata->layering = 0;
   else if ((cfdata->escfg->popup) && 
	    (cfdata->escfg->layer == 0))
     cfdata->layering = 1;
   else if ((cfdata->escfg->popup) && 
	    (cfdata->escfg->layer == 200))
     cfdata->layering = 2;
   else
     cfdata->layering = 2;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->es = cfd->data;
   cfdata->escfg = cfdata->es->cfg;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   cfdata->es->config_dialog = NULL;
   if (cfdata->style) free(cfdata->style);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Zone *zone;
   int id;
   
   cfdata->escfg->orient = cfdata->orient;
   cfdata->escfg->fit_along = cfdata->fit_along;
   cfdata->escfg->fit_size = cfdata->fit_size;
   cfdata->escfg->size = cfdata->size;
   
   printf("%i %i %i\n", cfdata->fit_along, cfdata->fit_size, cfdata->size);
   
   if (cfdata->escfg->style) evas_stringshare_del(cfdata->escfg->style);
   cfdata->escfg->style = evas_stringshare_add(cfdata->style);
   if (cfdata->layering == 0)
     {
	cfdata->escfg->popup = 0;
	cfdata->escfg->layer = 1;
     }
   else if (cfdata->layering == 1)
     {
	cfdata->escfg->popup = 1;
	cfdata->escfg->layer = 0;
     }
   else if (cfdata->layering == 2)
     {
	cfdata->escfg->popup = 1;
	cfdata->escfg->layer = 200;
     }
   zone = cfdata->es->zone;
   id = cfdata->es->id;
   cfdata->es->config_dialog = NULL;
   e_object_del(cfdata->es);
   cfdata->es = e_shelf_zone_new(zone, cfdata->escfg->name, 
				 cfdata->escfg->style,
				 cfdata->escfg->popup,
				 cfdata->escfg->layer, id);
   cfdata->es->cfg = cfdata->escfg;
   cfdata->es->fit_along = cfdata->escfg->fit_along;
   cfdata->es->fit_size = cfdata->escfg->fit_size;
   e_shelf_orient(cfdata->es, cfdata->escfg->orient);
   e_shelf_position_calc(cfdata->es);
   e_shelf_populate(cfdata->es);
   e_shelf_show(cfdata->es);
   e_config_save_queue();
   cfdata->es->config_dialog = cfd;
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   int sel, n;

   /* FIXME: this is just raw config now - it needs UI improvments */
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Stacking"), 0);
   rg = e_widget_radio_group_new(&(cfdata->layering));
   ob = e_widget_radio_add(evas, _("Below Everything"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Below Windows"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Above Everything"), 2, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Size"), 0);
   ob = e_widget_check_add(evas, _("Shrink length fit contents"), &(cfdata->fit_along));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Expand width to fit contents"), &(cfdata->fit_size));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Shelf Size"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%3.0f pixels"), 4, 200, 4, 0, NULL, &(cfdata->size), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Layout"), 0);
   rg = e_widget_radio_group_new(&(cfdata->orient));
   ob = e_widget_radio_add(evas, _("Left"), E_GADCON_ORIENT_LEFT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Right"), E_GADCON_ORIENT_RIGHT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Top"), E_GADCON_ORIENT_TOP, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Bottom"), E_GADCON_ORIENT_BOTTOM, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Top Left"), E_GADCON_ORIENT_CORNER_TL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Top Right"), E_GADCON_ORIENT_CORNER_TR, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Bottom Left"), E_GADCON_ORIENT_CORNER_BL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Bottom Right"), E_GADCON_ORIENT_CORNER_BR, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Left Top"), E_GADCON_ORIENT_CORNER_LT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Right Top"), E_GADCON_ORIENT_CORNER_RT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Left Bottom"), E_GADCON_ORIENT_CORNER_LB, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Right Bottom"), E_GADCON_ORIENT_CORNER_RB, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}
