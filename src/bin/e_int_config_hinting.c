/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   /*- BASIC -*/
   int hinting;
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_int_config_hinting(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   /* methods */
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->advanced.apply_cfdata   = NULL;
   v->advanced.create_widgets = NULL;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Font Hinting Settings"), NULL, 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->hinting = e_config->font_hinting;
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
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_config->font_hinting = cfdata->hinting;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob;
   E_Radio_Group *rg;
      
   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->hinting));
   if (evas_imaging_font_hinting_can_hint(EVAS_FONT_HINTING_BYTECODE))
     {
	ob = e_widget_radio_add(evas, _("Bytecode Hinting"), 0, rg);
	e_widget_list_object_append(o, ob, 1, 1, 0.5);
     }
   if (evas_imaging_font_hinting_can_hint(EVAS_FONT_HINTING_AUTO))
     {
	ob = e_widget_radio_add(evas, _("Automatic Hinting"), 1, rg);
	e_widget_list_object_append(o, ob, 1, 1, 0.5);
     }
   if (evas_imaging_font_hinting_can_hint(EVAS_FONT_HINTING_NONE))
     {
	ob = e_widget_radio_add(evas, _("No Hinting"), 2, rg);
	e_widget_list_object_append(o, ob, 1, 1, 0.5);
     }
   return o;
}
