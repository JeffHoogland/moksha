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
#define MODE_NOTHING        0
#define MODE_GEOMETRY       1
#define MODE_LOCKS          2
#define MODE_GEOMETRY_LOCKS 3
#define MODE_ALL            4
struct _E_Config_Dialog_Data
{
   E_Border *border;
   /*- BASIC -*/
   struct {
      char *title;
      char *name;
      char *class;
      char *icon_name;
      char *machine;
      char *role;
      
      char *min;
      char *max;
      char *base;
      char *step;
      
      int take_focus;
      int accept_focus;
      int urgent;
      int delete_request;
      int request_pos;
      
   } icccm;
};

/* a nice easy setup function that does the dirty work */
EAPI void
e_int_border_prop(E_Border *bd)
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
         v->advanced.apply_cfdata   = NULL;
         v->advanced.create_widgets = NULL;
	 v->override_auto_apply = 1;
	 
         /* create config dialog for bd object/data */
         cfd = e_config_dialog_new(bd->zone->container, 
				   _("Window Properties"), NULL, 0, v, bd);
      }
}

/**--CREATE--**/
static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   char buf[256];
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->border = cfd->data;
   
#define IFDUP(prop, dest) \
   if (cfdata->border->prop) cfdata->dest = strdup(cfdata->border->prop)

   IFDUP(client.icccm.title, icccm.title);
   IFDUP(client.icccm.name, icccm.name);
   IFDUP(client.icccm.class, icccm.class);
   IFDUP(client.icccm.icon_name, icccm.icon_name);
   IFDUP(client.icccm.machine, icccm.machine);
   IFDUP(client.icccm.window_role, icccm.role);

   if (cfdata->border->client.icccm.min_w >= 0)
     {
	snprintf(buf, sizeof(buf), "%ix%i",
		 cfdata->border->client.icccm.min_w,
		 cfdata->border->client.icccm.min_h);
	cfdata->icccm.min = strdup(buf);
     }
   if (cfdata->border->client.icccm.max_w >= 0)
     {
	snprintf(buf, sizeof(buf), "%ix%i",
		 cfdata->border->client.icccm.max_w,
		 cfdata->border->client.icccm.max_h);
	cfdata->icccm.max = strdup(buf);
     }
   if (cfdata->border->client.icccm.base_w >= 0)
     {
	snprintf(buf, sizeof(buf), "%ix%i",
		 cfdata->border->client.icccm.base_w,
		 cfdata->border->client.icccm.base_h);
	cfdata->icccm.base = strdup(buf);
     }
   if (cfdata->border->client.icccm.step_w >= 0)
     {
	snprintf(buf, sizeof(buf), "%i,%i",
		 cfdata->border->client.icccm.step_w,
		 cfdata->border->client.icccm.step_h);
	cfdata->icccm.step = strdup(buf);
     }
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
#define IFREE(x) E_FREE(cfdata->x)
   IFREE(icccm.title);
   IFREE(icccm.name);
   IFREE(icccm.class);
   IFREE(icccm.icon_name);
   IFREE(icccm.machine);
   IFREE(icccm.role);
   IFREE(icccm.min);
   IFREE(icccm.max);
   IFREE(icccm.base);
   IFREE(icccm.step);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob, *of;
   
   o = e_widget_table_add(evas, 0);
   
#define STR_ENTRY(label, x, y, val) \
   { \
      Evas_Coord mw, mh; \
      ob = e_widget_label_add(evas, label); \
      if (!cfdata->val) e_widget_disabled_set(ob, 1); \
      e_widget_frametable_object_append(of, ob, x, y, 1, 1,    1, 1, 1, 1); \
      ob = e_widget_entry_add(evas, &(cfdata->val)); \
      if (!cfdata->val) e_widget_disabled_set(ob, 1); \
      e_widget_min_size_get(ob, &mw, &mh); \
      e_widget_min_size_set(ob, 80, mh); \
      e_widget_frametable_object_append(of, ob, x + 1, y, 1, 1,    1, 1, 1, 1); \
   }
   
   of = e_widget_frametable_add(evas, _("ICCCM"), 0);

   STR_ENTRY(_("Title"),     0, 0, icccm.title);
   STR_ENTRY(_("Name"),      0, 1, icccm.name);
   STR_ENTRY(_("Class"),     0, 2, icccm.class);
   STR_ENTRY(_("Icon Name"), 0, 3, icccm.icon_name);
   STR_ENTRY(_("Machine"),   0, 4, icccm.machine);
   STR_ENTRY(_("Role"),      0, 5, icccm.role);
   
   STR_ENTRY(_("Minimum Size"),   0, 6,  icccm.min);
   STR_ENTRY(_("Maximum Size"),   0, 7,  icccm.max);
   STR_ENTRY(_("Base Size"),      0, 8,  icccm.base);
   STR_ENTRY(_("Resize Steps"),   0, 9,  icccm.step);
   
   e_widget_table_object_append(o, of, 0, 0, 1, 1,    1, 1, 1, 1);
   
   return o;
}
