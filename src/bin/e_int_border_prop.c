/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
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

      char *aspect;
      char *initial_state;
      char *state;
      char *window_id;
      char *window_group;
      char *transient_for;
      char *client_leader;
      char *gravity;
      char *command;
      
      int take_focus;
      int accepts_focus;
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
	v->basic.apply_cfdata      = NULL;
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
   char buf[4096];
   
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
   if ((cfdata->border->client.icccm.min_aspect > 0.0) &&
       (cfdata->border->client.icccm.max_aspect > 0.0))
     {
	snprintf(buf, sizeof(buf), "%1.3f-%1.3f",
		 cfdata->border->client.icccm.min_aspect,
		 cfdata->border->client.icccm.max_aspect);
	cfdata->icccm.aspect = strdup(buf);
     }
   if (cfdata->border->client.icccm.initial_state != ECORE_X_WINDOW_STATE_HINT_NONE)
     {
	switch (cfdata->border->client.icccm.initial_state)
	  {
	   case ECORE_X_WINDOW_STATE_HINT_WITHDRAWN:
	     snprintf(buf, sizeof(buf), "Withdrawn");
	     break;
	   case ECORE_X_WINDOW_STATE_HINT_NORMAL:
	     snprintf(buf, sizeof(buf), "Normal");
	     break;
	   case ECORE_X_WINDOW_STATE_HINT_ICONIC:
	     snprintf(buf, sizeof(buf), "Iconic");
	     break;
	   default:
	     buf[0] = 0;
	     break;
	  }
	cfdata->icccm.initial_state = strdup(buf);
     }
   if (cfdata->border->client.icccm.state != ECORE_X_WINDOW_STATE_HINT_NONE)
     {
	switch (cfdata->border->client.icccm.state)
	  {
	   case ECORE_X_WINDOW_STATE_HINT_WITHDRAWN:
	     snprintf(buf, sizeof(buf), "Withdrawn");
	     break;
	   case ECORE_X_WINDOW_STATE_HINT_NORMAL:
	     snprintf(buf, sizeof(buf), "Normal");
	     break;
	   case ECORE_X_WINDOW_STATE_HINT_ICONIC:
	     snprintf(buf, sizeof(buf), "Iconic");
	     break;
	   default:
	     buf[0] = 0;
	     break;
	  }
	cfdata->icccm.state = strdup(buf);
     }
   snprintf(buf, sizeof(buf), "0x%08x",
	    cfdata->border->client.win);
   cfdata->icccm.window_id = strdup(buf);
   if (cfdata->border->client.icccm.window_group != 0)
     {
	snprintf(buf, sizeof(buf), "0x%08x",
		 cfdata->border->client.icccm.window_group);
	cfdata->icccm.window_group = strdup(buf);
     }
   if (cfdata->border->client.icccm.transient_for != 0)
     {
	snprintf(buf, sizeof(buf), "0x%08x",
		 cfdata->border->client.icccm.transient_for);
	cfdata->icccm.transient_for = strdup(buf);
     }
   if (cfdata->border->client.icccm.client_leader != 0)
     {
	snprintf(buf, sizeof(buf), "0x%08x",
		 cfdata->border->client.icccm.client_leader);
	cfdata->icccm.client_leader = strdup(buf);
     }
   switch (cfdata->border->client.icccm.gravity)
     {
      case ECORE_X_GRAVITY_FORGET:
	snprintf(buf, sizeof(buf), "Forget/Unmap");
	break;
      case ECORE_X_GRAVITY_NW:
	snprintf(buf, sizeof(buf), "Northwest");
	break;
      case ECORE_X_GRAVITY_N:
	snprintf(buf, sizeof(buf), "North");
	break;
      case ECORE_X_GRAVITY_NE:
	snprintf(buf, sizeof(buf), "Northeast");
	break;
      case ECORE_X_GRAVITY_W:
	snprintf(buf, sizeof(buf), "West");
	break;
      case ECORE_X_GRAVITY_CENTER:
	snprintf(buf, sizeof(buf), "Center");
	break;
      case ECORE_X_GRAVITY_E:
	snprintf(buf, sizeof(buf), "East");
	break;
      case ECORE_X_GRAVITY_SW:
	snprintf(buf, sizeof(buf), "Sowthwest");
	break;
      case ECORE_X_GRAVITY_S:
	snprintf(buf, sizeof(buf), "South");
	break;
      case ECORE_X_GRAVITY_SE:
	snprintf(buf, sizeof(buf), "Southeast");
	break;
      case ECORE_X_GRAVITY_STATIC:
	snprintf(buf, sizeof(buf), "Static");
	break;
      default:
	buf[0] = 0;
	break;
     }
   cfdata->icccm.gravity = strdup(buf);
   if (cfdata->border->client.icccm.command.argv)
     {
	int i;
	
	buf[0] = 0;
	for (i = 0; i < cfdata->border->client.icccm.command.argc; i++)
	  {
	     if ((sizeof(buf) - strlen(buf)) < 
		 (strlen(cfdata->border->client.icccm.command.argv[i]) - 2))
	       break;
	     strcat(buf, cfdata->border->client.icccm.command.argv[i]);
	     strcat(buf, " ");
	  }
	cfdata->icccm.command = strdup(buf);
     }
   
   cfdata->icccm.take_focus = cfdata->border->client.icccm.take_focus;
   cfdata->icccm.accepts_focus = cfdata->border->client.icccm.accepts_focus;
   cfdata->icccm.urgent = cfdata->border->client.icccm.urgent;
   cfdata->icccm.delete_request = cfdata->border->client.icccm.delete_request;
   cfdata->icccm.request_pos = cfdata->border->client.icccm.request_pos;
   
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
   IFREE(icccm.aspect);
   IFREE(icccm.initial_state);
   IFREE(icccm.state);
   IFREE(icccm.window_id);
   IFREE(icccm.window_group);
   IFREE(icccm.transient_for);
   IFREE(icccm.client_leader);
   IFREE(icccm.gravity);
   IFREE(icccm.command);
   free(cfdata);
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
      e_widget_min_size_set(ob, 160, mh); \
      e_widget_frametable_object_append(of, ob, x + 1, y, 1, 1,    1, 1, 1, 1); \
   }
#define CHK_ENTRY(label, x, y, val) \
   { \
      ob = e_widget_label_add(evas, label); \
      e_widget_frametable_object_append(of, ob, x, y, 1, 1,    1, 1, 1, 1); \
      ob = e_widget_check_add(evas, "", &(cfdata->val)); \
      e_widget_frametable_object_append(of, ob, x + 1, y, 1, 1,    1, 1, 1, 1); \
   }
   
   of = e_widget_frametable_add(evas, _("ICCCM"), 0);

   STR_ENTRY(_("Title"),          0, 0,    icccm.title);
   STR_ENTRY(_("Name"),           0, 1,    icccm.name);
   STR_ENTRY(_("Class"),          0, 2,    icccm.class);
   STR_ENTRY(_("Icon Name"),      0, 3,    icccm.icon_name);
   STR_ENTRY(_("Machine"),        0, 4,    icccm.machine);
   STR_ENTRY(_("Role"),           0, 5,    icccm.role);
   
   STR_ENTRY(_("Minimum Size"),   0, 6,    icccm.min);
   STR_ENTRY(_("Maximum Size"),   0, 7,    icccm.max);
   STR_ENTRY(_("Base Size"),      0, 8,    icccm.base);
   STR_ENTRY(_("Resize Steps"),   0, 9,    icccm.step);

   STR_ENTRY(_("Aspect Ratio"),   0, 10,   icccm.aspect);
   STR_ENTRY(_("Initial State"),  0, 11,   icccm.initial_state);
   STR_ENTRY(_("State"),          0, 12,   icccm.state);
   STR_ENTRY(_("Window ID"),      0, 13,   icccm.window_id);
   STR_ENTRY(_("Window Group"),   0, 14,   icccm.window_group);
   STR_ENTRY(_("Transient For"),  0, 15,   icccm.transient_for);
   STR_ENTRY(_("Client Leader"),  0, 16,   icccm.client_leader);
   STR_ENTRY(_("Gravity"),        0, 17,   icccm.gravity);
   STR_ENTRY(_("Command"),        0, 18,   icccm.command);

   CHK_ENTRY(_("Take Focus"),       0, 19, icccm.take_focus);
   CHK_ENTRY(_("Accepts Focus"),    0, 20, icccm.accepts_focus);
   CHK_ENTRY(_("Urgent"),           0, 21, icccm.urgent);
   CHK_ENTRY(_("Request Delete"),   0, 22, icccm.delete_request);
   CHK_ENTRY(_("Request Position"), 0, 23, icccm.request_pos);
   
   e_widget_table_object_append(o, of, 0, 0, 1, 1,    1, 1, 1, 1);
   
   return o;
}
