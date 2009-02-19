#include "e.h"
#include "e_winilist.h"

/* internal calls */
typedef struct _Data Data;
typedef struct _Special Special;
  
struct _Data
{
   Evas_Object *o_frame;
   Evas_Object *o_ilist;
   struct {
      void (*func) (void *data, E_Border *bd);
      void *data;
   } select;
   struct {
      Eina_List *prepend;
      Eina_List *append;
      unsigned char changed : 1;
   } special;
   struct {
      Evas_Coord w, h;
   } optimal_size;
   Eina_List *borders;
   Eina_List *labels;
};

struct _Special
{
   Evas_Object *icon;
   const char *label;
   void (*func) (void *data1, void *data2);
   void *data1;
   void *data2;
};

static void _cb_item_sel(void *data, void *data2);
static void _cb_special_sel(void *data, void *data2);
    
static void _cb_object_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _cb_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _refill(Data *d);

static int _cb_border_add(void *data, int ev_type, void *ev);
static int _cb_border_remove(void *data, int ev_type, void *ev);
static int _cb_border_show(void *data, int ev_type, void *ev);
static int _cb_border_hide(void *data, int ev_type, void *ev);
static int _cb_border_property(void *data, int ev_type, void *ev);
static int _cb_desk_show(void *data, int ev_type, void *event);

/* state */
static Eina_List *handlers = NULL;
static Eina_List *winilists = NULL;

/* called from the module core */
EAPI int
e_winilist_init(void)
{
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ADD, _cb_border_add, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE, _cb_border_remove, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_SHOW, _cb_border_show, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_HIDE, _cb_border_hide, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_PROPERTY, _cb_border_property, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_DESK_SHOW, _cb_desk_show, NULL));
   return 1;
}

EAPI int
e_winilist_shutdown(void)
{
   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = eina_list_remove_list(handlers, handlers);
     }
   return 1;
}

EAPI Evas_Object *
e_winilist_add(Evas *e)
{
   Data *d;
   
   d = E_NEW(Data, 1);
   d->o_frame = e_scrollframe_add(e);
   d->o_ilist = e_ilist_add(e);
   evas_object_data_set(d->o_frame, "..[winilist]", d);
   e_ilist_selector_set(d->o_ilist, 1);

   e_scrollframe_child_set(d->o_frame, d->o_ilist);
   evas_object_show(d->o_ilist);
   
   winilists = eina_list_append(winilists, d);
   evas_object_event_callback_add(d->o_frame, EVAS_CALLBACK_DEL, _cb_object_del, NULL);
   evas_object_event_callback_add(d->o_frame, EVAS_CALLBACK_RESIZE, _cb_object_resize, NULL);
   
   _refill(d);
   
   return d->o_frame;
}

EAPI void
e_winilist_border_select_callback_set(Evas_Object *obj, void (*func) (void *data, E_Border *bd), void *data)
{
   Data *d;
   
   d = evas_object_data_get(obj, "..[winilist]");
   if (!d) return;
   d->select.func = func;
   d->select.data = data;
}

EAPI void
e_winilist_special_append(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), void *data1, void *data2)
{
   Data *d;
   
   d = evas_object_data_get(obj, "..[winilist]");
   if (!d) return;
     {
	Special *s;
	
	s = E_NEW(Special, 1);
	d->special.prepend = eina_list_prepend(d->special.prepend, s);
	s->icon = icon;
	if (label) s->label = evas_stringshare_add(label);
	s->func = func;
	s->data1 = data1;
	s->data2 = data2;
	d->special.changed = 1;
     }
   _refill(d);
}

EAPI void
e_winilist_special_prepend(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), void *data1, void *data2)
{
   Data *d;
   
   d = evas_object_data_get(obj, "..[winilist]");
   if (!d) return;
     {
	Special *s;
	
	s = E_NEW(Special, 1);
	d->special.append = eina_list_append(d->special.append, s);
	s->icon = icon;
	if (label) s->label = evas_stringshare_add(label);
	s->func = func;
	s->data1 = data1;
	s->data2 = data2;
	d->special.changed = 1;
     }
   _refill(d);
}

EAPI void
e_winilist_optimial_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Data *d;
   
   if (w) *w = 0;
   if (h) *h = 0;
   d = evas_object_data_get(obj, "..[winilist]");
   if (!d) return;
   if (w) *w = d->optimal_size.w;
   if (h) *h = d->optimal_size.h;
}

///////////////////////////////////////////////////////////////////////////////

/* internal calls */
static void
_cb_item_sel(void *data, void *data2)
{
   Data *d;
   
   d = data;
   if (d->select.func)
     {
	d->select.func(d->select.data, data2);
     }
}

static void
_cb_special_sel(void *data, void *data2)
{
   Special *s;
   
   s = data;
   if (s->func)
     {
	s->func(s->data1, s->data2);
     }
}

static void
_cb_object_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Data *d;
   
   d = evas_object_data_get(obj, "..[winilist]");
   if (!d) return;
   evas_object_del(d->o_ilist);
   winilists = eina_list_remove(winilists, d);
   
   while (d->borders)
     {
	e_object_unref(E_OBJECT(d->borders->data));
	d->borders = eina_list_remove_list(d->borders, d->borders);
     }
   while (d->labels)
     {
	evas_stringshare_del(d->labels->data);
	d->labels = eina_list_remove_list(d->labels, d->labels);
     }
   
   while (d->special.prepend)
     {
	Special *s;
	
	s = d->special.prepend->data;
	if (s->icon)
	  {
	     evas_object_del(s->icon);
	     s->icon = NULL;
	  }
	if (s->label)
	  {
	     evas_stringshare_del(s->label);
	     s->label = NULL;
	  }
	free(s);
	d->special.prepend = eina_list_remove_list(d->special.prepend, d->special.prepend);
     }
   while (d->special.append)
     {
	Special *s;
	
	s = d->special.append->data;
	if (s->icon)
	  {
	     evas_object_del(s->icon);
	     s->icon = NULL;
	  }
	if (s->label)
	  {
	     evas_stringshare_del(s->label);
	     s->label = NULL;
	  }
	free(s);
	d->special.append = eina_list_remove_list(d->special.append, d->special.append);
     }
   
   free(d);
}

static void
_cb_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Data *d;
   Evas_Coord lw, lh, vw, vh;
   
   d = evas_object_data_get(obj, "..[winilist]");
   if (!d) return;
   e_ilist_min_size_get(d->o_ilist, &lw, &lh);
   if (lh < (120 * e_scale)) lh = 120 * e_scale;
   printf("%i\n", lh);
   e_scrollframe_child_viewport_size_get(d->o_frame, &vw, &vh);
   evas_object_resize(d->o_ilist, vw, lh);
}

static void
_refill(Data *d)
{
   Evas_Coord w, h, lw, lh, vw, vh;
   Eina_List *borders, *l, *l2, *l3;
   
   borders = e_border_client_list();
   if (!d->special.changed)
     {
	int changed = 0;
	
	if ((borders) && (d->borders))
	  {
	     Eina_List *tmp = NULL;
	     
	     changed = 0;
	     for (l = borders; l; l = l->next)
	       {
		  E_Border *bd;
		  
		  bd = l->data;
		  if (e_object_is_del(E_OBJECT(bd))) continue;
		  if ((!bd->client.icccm.accepts_focus) &&
		      (!bd->client.icccm.take_focus)) continue;
		  if (bd->client.netwm.state.skip_taskbar) continue;
		  if (bd->user_skip_winlist) continue;
		  if ((!bd->sticky) && 
		      (bd->desk != e_desk_current_get(bd->zone))) continue;
		  tmp = eina_list_append(tmp, bd);
	       }
	     if (!(tmp && d->borders))
	       {
		  changed = 1;
	       }
	     else
	       {
		  if (eina_list_count(tmp) !=
		      eina_list_count(d->borders))
		    {
		       changed = 1;
		    }
		  else
		    {
		       for (l = tmp, l2 = d->borders, l3 = d->labels;
			    l && l2 && l3; 
			    l = l->next, l2 = l2->next, l3 = l3->next)
			 {
			    E_Border *bd, *bd2;
			    const char *title;
			    
			    bd = l->data;
			    bd2 = l2->data;
			    if (bd != bd2)
			      {
				 changed = 1;
				 break;
			      }
			    
			    title = "???";
			    if (bd->client.netwm.name) title = bd->client.netwm.name;
			    else if (bd->client.icccm.title) title = bd->client.icccm.title;
			    if (strcmp(title, l3->data))
			      {
				 changed = 1;
				 break;
			      }
			 }
		    }
	       }
	     if (tmp) eina_list_free(tmp);
	  }
	else
	  changed = 1;
	if (!changed) return;
     }
   d->special.changed = 0;
   while (d->borders)
     {
	e_object_unref(E_OBJECT(d->borders->data));
	d->borders = eina_list_remove_list(d->borders, d->borders);
     }
   while (d->labels)
     {
	evas_stringshare_del(d->labels->data);
	d->labels = eina_list_remove_list(d->labels, d->labels);
     }
   
   e_ilist_freeze(d->o_ilist);
   e_ilist_clear(d->o_ilist);
   for (l = d->special.prepend; l; l = l->next)
     {
	Special *s;
	
	s = l->data;
	e_ilist_append(d->o_ilist, s->icon, s->label, 0, _cb_special_sel, NULL,
		       s, NULL);
     }
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	const char *title;
	
	bd = l->data;
	if (e_object_is_del(E_OBJECT(bd))) continue;
	if ((!bd->client.icccm.accepts_focus) &&
	    (!bd->client.icccm.take_focus)) continue;
	if (bd->client.netwm.state.skip_taskbar) continue;
	if (bd->user_skip_winlist) continue;
	if ((!bd->sticky) && 
	    (bd->desk != e_desk_current_get(bd->zone))) continue;
	
	title = "???";
	if (bd->client.netwm.name) title = bd->client.netwm.name;
	else if (bd->client.icccm.title) title = bd->client.icccm.title;
	e_object_ref(E_OBJECT(bd));
	d->borders = eina_list_append(d->borders, bd);
	d->labels = eina_list_append(d->labels, evas_stringshare_add(title));
	e_ilist_append(d->o_ilist, NULL/*icon*/, title, 0,
		       _cb_item_sel,
		       NULL, d, bd);
     }
   for (l = d->special.append; l; l = l->next)
     {
	Special *s;
	
	s = l->data;
	e_ilist_append(d->o_ilist, s->icon, s->label, 0, _cb_special_sel, NULL,
		       s, NULL);
     }
   
   e_ilist_thaw(d->o_ilist);
   
   /* FIXME: figure out optimal size */
   e_ilist_min_size_get(d->o_ilist, &lw, &lh);
   if (lh < (120 * e_scale)) lh = 120 * e_scale;
   printf("%i\n", lh);
   e_scrollframe_child_viewport_size_get(d->o_frame, &vw, &vh);
   evas_object_geometry_get(d->o_frame, NULL, NULL, &w, &h);
   evas_object_resize(d->o_ilist, vw, lh);

   d->optimal_size.w = lw + (w - vw);
   d->optimal_size.h = lh + (h - vh);
}

static int
_cb_border_add(void *data, int ev_type, void *event)
{
   E_Event_Border_Add *ev;
   
   ev = event;
     {
	Eina_List *l;
	
	for (l = winilists; l; l = l->next) _refill(l->data);
     }
   return 1;
}

static int
_cb_border_remove(void *data, int ev_type, void *event)
{
   E_Event_Border_Remove *ev;
   
   ev = event;
     {
	Eina_List *l;
	
	for (l = winilists; l; l = l->next) _refill(l->data);
     }
   return 1;
}

static int
_cb_border_show(void *data, int ev_type, void *event)
{
   E_Event_Border_Show *ev;
   
   ev = event;
     {
	Eina_List *l;
	
	for (l = winilists; l; l = l->next) _refill(l->data);
     }
   return 1;
}

static int
_cb_border_hide(void *data, int ev_type, void *event)
{
   E_Event_Border_Hide *ev;
   
   ev = event;
     {
	Eina_List *l;
	
	for (l = winilists; l; l = l->next) _refill(l->data);
     }
   return 1;
}

static int
_cb_border_property(void *data, int ev_type, void *event)
{
   E_Event_Border_Property *ev;
   
   ev = event;
   /* FIXME: should really be optimal on what properties warrant a refill */
     {
	Eina_List *l;
	
	for (l = winilists; l; l = l->next) _refill(l->data);
     }
   return 1;
}

static int
_cb_desk_show(void *data, int ev_type, void *event)
{
   E_Event_Desk_Show *ev;
   
   ev = event;
   /* FIXME: should really be optimal on what properties warrant a refill */
     {
	Eina_List *l;
	
	for (l = winilists; l; l = l->next) _refill(l->data);
     }
   return 1;
}

