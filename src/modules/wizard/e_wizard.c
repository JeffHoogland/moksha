/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static E_Popup *_e_wizard_main_new(E_Zone *zone);
static E_Popup *_e_wizard_extra_new(E_Zone *zone);
static void _e_wizard_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_wizard_cb_next(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wizard_cb_back(void *data, Evas_Object *obj, const char *emission, const char *source);

static E_Popup *pop = NULL;
static Evas_List *pops = NULL;
static Evas_Object *o_bg = NULL;
static Evas_Object *o_content = NULL;

EAPI int
e_wizard_init(void)
{
   Evas_List *l;
   
   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man;
	Evas_List *l2;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Evas_List *l3;
	     
	     con = l2->data;
	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  E_Zone *zone;
		  
		  zone = l3->data;
		  if (!pop)
		    pop = _e_wizard_main_new(zone);
		  else
		    pops = evas_list_append(pops, _e_wizard_extra_new(zone));
	       }
	  }
     }
   return 1;
}

EAPI int
e_wizard_shutdown(void)
{
   if (pop)
     {
	e_object_del(E_OBJECT(pop));
	pop = NULL;
     }
   while (pops)
     {
	e_object_del(E_OBJECT(pops->data));
	pops = evas_list_remove_list(pops, pops);
     }
   return 1;
}

static E_Popup *
_e_wizard_main_new(E_Zone *zone)
{
   E_Popup *pop;
   Evas_Object *o, *o_ev;
   Evas_Modifier_Mask mask;
   
   pop = e_popup_new(zone, zone->x, zone->y, zone->w, zone->h);
   e_popup_layer_set(pop, 255);
   o = edje_object_add(pop->evas);
   /* FIXME: need a theme */
   e_theme_edje_object_set(o, "base/theme/wizard",
			   "e/wizard/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_show(o);
   edje_object_signal_callback_add(o_bg, "e,action,next", "e",
				   _e_wizard_cb_next, pop);
   edje_object_signal_callback_add(o_bg, "e,action,back", "e",
				   _e_wizard_cb_back, pop);
   o_bg = o;
   
   o = evas_object_rectangle_add(pop->evas);
   mask = 0;
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = evas_key_modifier_mask_get(pop->evas, "Shift");
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "Return", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN,
				  _e_wizard_cb_key_down, pop);
   o_ev = o;

   /* set up next/prev buttons */
   edje_object_part_text_set(o_bg, "e.text.title", _("Welcome to Enlightenment"));
   edje_object_part_text_set(o_bg, "e.text.page", "");
   edje_object_part_text_set(o_bg, "e.text.next", _("Next"));
   edje_object_part_text_set(o_bg, "e.text.back", _("Back"));
   edje_object_signal_emit(o_bg, "e,state,next,disable", "e");
   edje_object_signal_emit(o_bg, "e,state,back,disable", "e");
   
   /* set up rest here */
   //evas_object_show(o);
   //edje_object_part_swallow(o_bg, "e.swallow.content", o);
   //e_widget_focus_set(o, 1);
   //o_content = o;
   
   e_popup_edje_bg_object_set(pop, o_bg);
   e_popup_show(pop);
   if (!e_grabinput_get(ecore_evas_software_x11_subwindow_get(pop->ecore_evas),
			1, ecore_evas_software_x11_subwindow_get(pop->ecore_evas)))
     {
	e_object_del(E_OBJECT(pop));
	pop = NULL;
     }
   return pop;
}

static E_Popup *
_e_wizard_extra_new(E_Zone *zone)
{
   E_Popup *pop;
   Evas_Object *o;
   
   pop = e_popup_new(zone, zone->x, zone->y, zone->w, zone->h);
   e_popup_layer_set(pop, 255);
   o = edje_object_add(pop->evas);
   /* FIXME: need a theme */
   e_theme_edje_object_set(o, "base/theme/wizard",
			   "e/wizard/extra");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(pop, o);
   e_popup_show(pop);
   return pop;
}

static void
_e_wizard_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event)
{
   Evas_Event_Key_Down *ev;
   E_Popup *pop;
   
   ev = event;
   pop = (E_Popup *)data;
   if (!o_content) return;
   if (!strcmp(ev->keyname, "Tab"))
     {
	if (evas_key_modifier_is_set(pop->evas, "Shift"))
	  e_widget_focus_jump(o_content, 0);
	else
	  e_widget_focus_jump(o_content, 1);
     }
   else if (((!strcmp(ev->keyname, "Return")) ||
	     (!strcmp(ev->keyname, "KP_Enter")) ||
	     (!strcmp(ev->keyname, "space"))))
     {
	Evas_Object *o;
	
	o = e_widget_focused_object_get(o_content);
	if (o) e_widget_activate(o);
     }
}

static void
_e_wizard_cb_next(void *data, Evas_Object *obj, const char *emission, const char *source)
{
}

static void
_e_wizard_cb_back(void *data, Evas_Object *obj, const char *emission, const char *source)
{
}
