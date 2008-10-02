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
   int use_resist;
   /*- ADVANCED -*/
   int desk_resist;
   int window_resist;
   int gadget_resist;
   struct {
	double timeout;
	struct {
	     int dx;
	     int dy;
	} move;
	struct {
	     int dx;
	     int dy;
	} resize;
   } border_keyboard;
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_int_config_window_geometry(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_window_geometry_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   /* methods */
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con,
			     _("Window Geometry"),
			     "E", "_config_window_geometry_dialog",
			     "enlightenment/window_manipulation", 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->use_resist = e_config->use_resist;
   cfdata->desk_resist = e_config->desk_resist;
   cfdata->window_resist = e_config->window_resist;
   cfdata->gadget_resist = e_config->gadget_resist;
   cfdata->border_keyboard.timeout = e_config->border_keyboard.timeout;
   cfdata->border_keyboard.move.dx = e_config->border_keyboard.move.dx;
//   cfdata->border_keyboard.move.dy = e_config->border_keyboard.move.dy;
   cfdata->border_keyboard.resize.dx = e_config->border_keyboard.resize.dx;
//   cfdata->border_keyboard.resize.dy = e_config->border_keyboard.resize.dy;
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
   E_FREE(cfdata);
}

/**--APPLY--**/

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_config->use_resist = cfdata->use_resist;
   e_config->desk_resist = cfdata->desk_resist;
   e_config->window_resist = cfdata->window_resist;
   e_config->gadget_resist = cfdata->gadget_resist;
   e_config->border_keyboard.timeout = cfdata->border_keyboard.timeout;
   e_config->border_keyboard.move.dx = cfdata->border_keyboard.move.dx;
//   e_config->border_keyboard.move.dy = cfdata->border_keyboard.move.dy;
   e_config->border_keyboard.move.dy = cfdata->border_keyboard.move.dx;
   e_config->border_keyboard.resize.dx = cfdata->border_keyboard.resize.dx;
//   e_config->border_keyboard.resize.dy = cfdata->border_keyboard.resize.dy;
   e_config->border_keyboard.resize.dy = cfdata->border_keyboard.resize.dx;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Resistance"), 0);
   ob = e_widget_check_add(evas, _("Resist moving or resizing a window over an obstacle"), &(cfdata->use_resist));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resistance between windows:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, NULL, &(cfdata->window_resist), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resistance at the edge of the screen:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, NULL, &(cfdata->desk_resist), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resistance to desktop gadgets:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, NULL, &(cfdata->gadget_resist), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Keyboard move and resize"), 0);
   ob = e_widget_label_add(evas, _("Automatically accept changes after:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f sec"), 0.0, 9.9, 0.1, 0, &(cfdata->border_keyboard.timeout), NULL, 100);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Move by:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 1, 255, 1, 0, NULL, &(cfdata->border_keyboard.move.dx), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resize by:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 1, 255, 1, 0, NULL, &(cfdata->border_keyboard.resize.dx), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}
