/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static void _e_wizard_back_eval(void);
static void _e_wizard_next_eval(void);
static E_Popup *_e_wizard_main_new(E_Zone *zone);
static E_Popup *_e_wizard_extra_new(E_Zone *zone);
static void _e_wizard_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_wizard_cb_next(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wizard_cb_back(void *data, Evas_Object *obj, const char *emission, const char *source);

static E_Popup *pop = NULL;
static Evas_List *pops = NULL;
static Evas_Object *o_bg = NULL;
static Evas_Object *o_content = NULL;
static Evas_List *pages = NULL;
static E_Wizard_Page *curpage = NULL;
static int next_ok = 1;
static int back_ok = 1;
static int next_can = 0;
static int back_can = 0;
static int next_prev = 0;
static int back_prev = 0;

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
   while (pages)
     {
	e_wizard_page_del(pages->data);
     }
   return 1;
}

EAPI void
e_wizard_go(void)
{
   if (!curpage)
     {
	if (pages)
	  {
	     curpage = pages->data;
	     if (pages->next) next_can = 1;
	  }
     }
   if (curpage)
     {
	if ((!curpage->data) && (curpage->init)) curpage->init(curpage);
	_e_wizard_back_eval();
	_e_wizard_next_eval();
	if ((curpage->show) && (!curpage->show(curpage)))
	  {
	     e_wizard_next();
	  }
     }
}

EAPI void
e_wizard_apply(void)
{
   Evas_List *l;
   
   for (l = pages; l; l = l->next)
     {  
	E_Wizard_Page *pg;
	
	pg = l->data;
	if (pg->apply) pg->apply(pg);
     }
}

EAPI void
e_wizard_next(void)
{
   Evas_List *l;
   
   for (l = pages; l; l = l->next)
     {
	if (l->data == curpage)
	  {
	     if (l->next)
	       {
		  if (curpage)
		    {
		       if (curpage->hide)
			 curpage->hide(curpage);
		    }
		  curpage = l->next->data;
		  if (!curpage->data)
		    {
                       if (curpage->init)
			 curpage->init(curpage);
		    }
		  next_can = 1;
		  if (l->prev) back_can = 1;
		  else back_can = 0;
		  _e_wizard_back_eval();
		  _e_wizard_next_eval();
		  if ((curpage->show) && (curpage->show(curpage)))
		    {
		       break;
		    }
	       }
	     else
	       {
		  /* FINISH */
		  e_wizard_apply();
		  e_wizard_shutdown();
		  return;
	       }
	  }
     }
}

EAPI void
e_wizard_back(void)
{
   Evas_List *l;
   
   for (l = evas_list_last(pages); l; l = l->prev)
     {
	if (l->data == curpage)
	  {
	     if (l->prev)
	       {
		  if (curpage)
		    {
                       if (curpage->hide)
			 curpage->hide(curpage);
		    }
		  curpage = l->prev->data;
		  if (!curpage->data)
		    {
                       if (curpage->init)
			 curpage->init(curpage);
		    }
		  next_can = 1;
		  if (l->prev) back_can = 1;
		  else back_can = 0;
		  _e_wizard_back_eval();
		  _e_wizard_next_eval();
                  if ((curpage->show) && (curpage->show(curpage)))
		    {
		       break;
		    }
	       }
	     else
	       {
		  break;
	       }
	  }
     }
}

EAPI void
e_wizard_page_show(Evas_Object *obj)
{
   Evas_Coord minw, minh;

   if (o_content) evas_object_del(o_content);
   o_content = obj;
   if (obj)
     {
	e_widget_min_size_get(obj, &minw, &minh);
	edje_extern_object_min_size_set(obj, minw, minh);
	edje_object_part_swallow(o_bg, "e.swallow.content", obj);
	evas_object_show(obj);
	e_widget_focus_set(obj, 1);
	edje_object_signal_emit(o_bg, "e,action,page,new", "e");
     }
}

/* FIXME: decide how pages are defined - how about an array of page structs? */
EAPI E_Wizard_Page *
e_wizard_page_add(void *handle,
		  int (*init)     (E_Wizard_Page *pg),
		  int (*shutdown) (E_Wizard_Page *pg),
		  int (*show)     (E_Wizard_Page *pg),
		  int (*hide)     (E_Wizard_Page *pg),
		  int (*apply)    (E_Wizard_Page *pg)
		  )
{
   E_Wizard_Page *pg;
   
   pg = E_NEW(E_Wizard_Page, 1);
   if (!pg) return NULL;
   
   pg->handle = handle;
   pg->evas = pop->evas;
   
   pg->init = init;
   pg->shutdown = shutdown;
   pg->show = show;
   pg->hide = hide;
   pg->apply = apply;
   
   pages = evas_list_append(pages, pg);
   
   return pg;
}

EAPI void
e_wizard_page_del(E_Wizard_Page *pg)
{
   if (pg->handle) dlclose(pg->handle);
   pages = evas_list_remove(pages, pg);
   free(pg);
}

EAPI void
e_wizard_button_back_enable_set(int enable)
{
   back_ok = enable;
   _e_wizard_back_eval();
}

EAPI void
e_wizard_button_next_enable_set(int enable)
{
   next_ok = enable;
   _e_wizard_next_eval();
}

EAPI void
e_wizard_title_set(const char *title)
{
   edje_object_part_text_set(o_bg, "e.text.title", title);
}

static void
_e_wizard_back_eval(void)
{
   int ok;
   
   ok = back_can;
   if (!back_ok) ok = 0;
   if (back_prev != ok)
     {
	if (ok) edje_object_signal_emit(o_bg, "e,state,back,enable", "e");
	else edje_object_signal_emit(o_bg, "e,state,back,disable", "e");
	back_prev = ok;
     }
}

static void
_e_wizard_next_eval(void)
{
   int ok;
   
   ok = next_can;
   if (!next_ok) ok = 0;
   if (next_prev != ok)
     {
	if (ok) edje_object_signal_emit(o_bg, "e,state,next,enable", "e");
	else edje_object_signal_emit(o_bg, "e,state,next,disable", "e");
	next_prev = ok;
     }
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
   edje_object_signal_callback_add(o, "e,action,next", "",
				   _e_wizard_cb_next, pop);
   edje_object_signal_callback_add(o, "e,action,back", "",
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
   edje_object_part_text_set(o_bg, "e.text.title", _("Welcome to Enlightenment 東京"));
   edje_object_part_text_set(o_bg, "e.text.page", "");
   edje_object_part_text_set(o_bg, "e.text.next", _("Next"));
   edje_object_part_text_set(o_bg, "e.text.back", _("Back"));
   edje_object_signal_emit(o_bg, "e,state,next,disable", "e");
   edje_object_signal_emit(o_bg, "e,state,back,disable", "e");
   
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
	if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
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
   e_wizard_next();
}

static void
_e_wizard_cb_back(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_wizard_back();
}
