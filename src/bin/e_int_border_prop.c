#include "e.h"

#define MODE_NOTHING        0
#define MODE_GEOMETRY       1
#define MODE_LOCKS          2
#define MODE_GEOMETRY_LOCKS 3
#define MODE_ALL            4

static void _bd_cb_dialog_del(void *obj);
static void _bd_cb_dialog_close(void *data, E_Dialog *dia);
static Evas_Object *_bd_icccm_create(E_Dialog *dia, void *data);
static Evas_Object *_bd_netwm_create(E_Dialog *dia, void *data);
static void _bd_go(void *data, void *data2);
static void _create_data(E_Dialog *cfd, E_Border *bd);
static void _free_data(E_Dialog *cfd, E_Config_Dialog_Data *cfdata);

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
   
   struct 
     {
	char *name;
	char *icon_name;
	int modal;
	int sticky;
	int shaded;
	int skip_taskbar;
	int skip_pager;
	int hidden;
	int fullscreen;
	char *stacking;
     } netwm;
};

EAPI void
e_int_border_prop(E_Border *bd) 
{
   E_Dialog *dia;

   if (bd->border_prop_dialog) return;
   
   dia = e_dialog_new(bd->zone->container, "E", "_window_props");
   e_object_del_attach_func_set(E_OBJECT(dia), _bd_cb_dialog_del);
   e_dialog_title_set(dia, _("Window Properties"));

   _create_data(dia, bd);

   _bd_go(dia, (void *)0);
   
   e_dialog_button_add(dia, _("Close"), NULL, _bd_cb_dialog_close, dia);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_dialog_border_icon_set(dia, "enlightenment/windows");
   e_dialog_resizable_set(dia, 1);
}

static void
_create_data(E_Dialog *cfd, E_Border *bd)
{
   E_Config_Dialog_Data *cfdata;
   char buf[4096];
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->border = bd;
   bd->border_prop_dialog = cfd;
   
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

   IFDUP(client.netwm.name, netwm.name);
   IFDUP(client.netwm.icon_name, netwm.icon_name);
   cfdata->netwm.modal = cfdata->border->client.netwm.state.modal;
   cfdata->netwm.sticky = cfdata->border->client.netwm.state.sticky;
   cfdata->netwm.shaded = cfdata->border->client.netwm.state.shaded;
   cfdata->netwm.skip_taskbar = cfdata->border->client.netwm.state.skip_taskbar;
   cfdata->netwm.skip_pager = cfdata->border->client.netwm.state.skip_pager;
   cfdata->netwm.hidden = cfdata->border->client.netwm.state.hidden;
   cfdata->netwm.fullscreen = cfdata->border->client.netwm.state.fullscreen;
   switch (cfdata->border->client.netwm.state.stacking) 
     {
      case 0:
	cfdata->netwm.stacking = strdup("None");
	break;
      case 1:
	cfdata->netwm.stacking = strdup("Above");	
	break;
      case 2:
	cfdata->netwm.stacking = strdup("Below");
	break;
     }
      
   cfd->data = cfdata;
}

static void
_free_data(E_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->border)
     cfdata->border->border_prop_dialog = NULL;
   
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
   
   IFREE(netwm.name);
   IFREE(netwm.icon_name);
   IFREE(netwm.stacking);
   
   free(cfdata);
   cfd->data = NULL;
}

static void 
_bd_cb_dialog_del(void *obj) 
{
   E_Dialog *dia;
   
   dia = obj;
   if (dia->data)
     _free_data(dia, dia->data);
}

static void 
_bd_cb_dialog_close(void *data, E_Dialog *dia) 
{
   if (dia->data)
     _free_data(dia, dia->data);
   e_object_del(E_OBJECT(dia));
}

static void 
_bd_go(void *data, void *data2) 
{
   E_Dialog *dia;
   Evas_Object *c, *o, *ob;
   Evas_Coord w, h;
   
   dia = data;
   if (!dia) return;

   if (dia->content_object)
     evas_object_del(dia->content_object);
   
   c = e_widget_list_add(e_win_evas_get(dia->win), 0, 0);
   
   if (!data2) 
     {
	o = _bd_icccm_create(dia, NULL);
	e_widget_list_object_append(c, o, 1, 1, 0.0);
	ob = e_widget_button_add(e_win_evas_get(dia->win), _("NetWM"), "widget/new_dialog",
				 _bd_go, dia, (void *)1);
     }
   else 
     {
	o = _bd_netwm_create(dia, NULL);
	e_widget_list_object_append(c, o, 1, 1, 0.0);
	ob = e_widget_button_add(e_win_evas_get(dia->win), _("ICCCM"), "widget/new_dialog",
				 _bd_go, dia, (void *)0);
     }
   
   e_widget_list_object_append(c, ob, 0, 0, 1.0);
   
   e_widget_min_size_get(c, &w, &h);
   e_dialog_content_set(dia, c, w, h);
   e_dialog_show(dia);
}

#define STR_ENTRY(label, x, y, val) \
   { \
      Evas_Coord mw, mh; \
      ob = e_widget_label_add(evas, label); \
      if (!cfdata->val) e_widget_disabled_set(ob, 1); \
      e_widget_frametable_object_append(of, ob, x, y, 1, 1,    1, 1, 0, 1); \
      ob = e_widget_entry_add(evas, &(cfdata->val), NULL, NULL, NULL); \
      if (!cfdata->val) e_widget_disabled_set(ob, 1); \
      e_widget_entry_readonly_set(ob, 1); \
      e_widget_min_size_get(ob, &mw, &mh); \
      e_widget_min_size_set(ob, 160, mh); \
      e_widget_frametable_object_append(of, ob, x + 1, y, 1, 1,    1, 1, 1, 1); \
   }
#define CHK_ENTRY(label, x, y, val) \
   { \
      ob = e_widget_label_add(evas, label); \
      e_widget_frametable_object_append(of, ob, x, y, 1, 1,    1, 1, 0, 1); \
      ob = e_widget_check_add(evas, "", &(cfdata->val)); \
      e_widget_disabled_set(ob, 1); \
      e_widget_frametable_object_append(of, ob, x + 1, y, 1, 1,    1, 1, 1, 1); \
   }

static Evas_Object *
_bd_icccm_create(E_Dialog *dia, void *data) 
{
   Evas *evas;
   Evas_Object *o, *ob, *of;
   E_Config_Dialog_Data *cfdata;

   if (!dia) return NULL;
   cfdata = dia->data;
   
   if (dia->content_object)
     evas_object_del(dia->content_object);
   
   evas = e_win_evas_get(dia->win);
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_frametable_add(evas, _("ICCCM Properties"), 0);
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

   STR_ENTRY(_("Aspect Ratio"),   2, 0,   icccm.aspect);
   STR_ENTRY(_("Initial State"),  2, 1,   icccm.initial_state);
   STR_ENTRY(_("State"),          2, 2,   icccm.state);
   STR_ENTRY(_("Window ID"),      2, 3,   icccm.window_id);
   STR_ENTRY(_("Window Group"),   2, 4,   icccm.window_group);
   STR_ENTRY(_("Transient For"),  2, 5,   icccm.transient_for);
   STR_ENTRY(_("Client Leader"),  2, 6,   icccm.client_leader);
   STR_ENTRY(_("Gravity"),        2, 7,   icccm.gravity);
   STR_ENTRY(_("Command"),        2, 8,   icccm.command);

   CHK_ENTRY(_("Take Focus"),       0, 11, icccm.take_focus);
   CHK_ENTRY(_("Accepts Focus"),    0, 12, icccm.accepts_focus);
   CHK_ENTRY(_("Urgent"),           0, 13, icccm.urgent);
   CHK_ENTRY(_("Request Delete"),   2, 11, icccm.delete_request);
   CHK_ENTRY(_("Request Position"), 2, 12, icccm.request_pos);
   
   e_widget_list_object_append(o, of, 1, 1, 0.0);
   return o;
}

static Evas_Object *
_bd_netwm_create(E_Dialog *dia, void *data) 
{
   Evas *evas;
   Evas_Object *o, *of, *ob;
   E_Config_Dialog_Data *cfdata;
   
   if (!dia) return NULL;
   cfdata = dia->data;

   if (dia->content_object)
     evas_object_del(dia->content_object);
   
   evas = e_win_evas_get(dia->win);
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_frametable_add(evas, _("NetWM Properties"), 0);
   STR_ENTRY(_("Name"),         0, 1,    netwm.name);
   STR_ENTRY(_("Icon Name"),    0, 2,    netwm.icon_name);
   STR_ENTRY(_("Stacking"),     0, 3,    netwm.stacking);

   CHK_ENTRY(_("Modal"),        0, 4, netwm.modal);
   CHK_ENTRY(_("Sticky"),       0, 5, netwm.sticky);
   CHK_ENTRY(_("Shaded"),       0, 6, netwm.shaded);
   CHK_ENTRY(_("Skip Taskbar"), 0, 7, netwm.skip_taskbar);
   CHK_ENTRY(_("Skip Pager"),   0, 8, netwm.skip_pager);
   CHK_ENTRY(_("Hidden"),       0, 9, netwm.hidden);
   CHK_ENTRY(_("Fullscreen"),   0, 10, netwm.fullscreen);
   
   e_widget_list_object_append(o, of, 1, 1, 0.0);
   return o;
}
