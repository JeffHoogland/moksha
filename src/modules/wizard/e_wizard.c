/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static E_Popup *_e_wizard_main_new(E_Zone *zone);
static E_Popup *_e_wizard_extra_new(E_Zone *zone);

static E_Popup *pop = NULL;
static Evas_List *pops = NULL;

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
   Evas_Object *o, *o_bg;
   
   pop = e_popup_new(zone, zone->x, zone->y, zone->w, zone->h);
   e_popup_layer_set(pop, 255);
   o = edje_object_add(pop->evas);
   /* FIXME: need a theme */
   e_theme_edje_object_set(o, "base/theme/wizard",
			   "e/wizard/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_show(o);
   o_bg = o;
   
   /* set up rest here */
   
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
