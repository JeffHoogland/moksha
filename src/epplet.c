#include "debug.h"
#include "epplet.h"
#include "globals.h"
#include "file.h"
#include "observer.h"
#include "desktops.h"

#include "e_ferite.h"

static void e_epplet_cleanup(E_Epplet *epp);
static void e_epplet_mouse_down_cb (void *_data, Ebits_Object _o,
				    char *_c, int _b, int _x, int _y,
				    int _ox, int _oy, int _ow, int _oh);

static void e_epplet_mouse_up_cb (void *_data, Ebits_Object _o,
				  char *_c, int _b, int _x, int _y,
				  int _ox, int _oy, int _ow, int _oh);

static void e_epplet_mouse_move_cb (void *_data, Ebits_Object _o,
				    char *_c, int _b, int _x, int _y,
				    int _ox, int _oy, int _ow, int _oh);

static void e_epplet_observer_cleanup(E_Object *o);

E_Epplet *
e_epplet_new(void *scr)
{
#ifdef USE_FERITE
   E_Epplet *epp;
   FeriteScript *script;
   char buf[PATH_MAX];
   
   D_ENTER;

   script = (FeriteScript *)scr;

   epp = NEW(E_Epplet, 1);
   ZERO(epp, E_Epplet, 1);

   e_object_init(E_OBJECT(epp), (E_Cleanup_Func) e_epplet_cleanup);

   epp->context = e_epplet_get_context_from_script(script);
   if (!(epp->context))
   {
      D("Error: epplet context not found\n");
      D_RETURN_(NULL);
   }
   
   epp->view = epp->context->view;
   epp->name = strdup(epp->context->name);
   epp->current.x = epp->context->geom.x;
   epp->current.y = epp->context->geom.y;
   epp->current.w = epp->context->geom.w;
   epp->current.h = epp->context->geom.h;
   epp->context->epp = epp;

   snprintf(buf, PATH_MAX, "%s/.e_epplets/%s/", epp->view->dir, epp->name);
   epp->dir = strdup(buf);

   if (!(epp->view))
   {
      D("Error: no view found for epplet: %s\n", epp->name);
      e_object_unref(E_OBJECT(epp)); 
      D_RETURN_(NULL);
   }
   else
   {
      D_RETURN_(epp);
   }
#endif
   
}
  
static void
e_epplet_cleanup(E_Epplet *epp)
{
   Evas_List l;
   
   D_ENTER;
   
#ifdef USE_FERITE
   for (l = epp->ebits; l; l = l->next)
   {
      Ebits_Object o = l->data;
      ebits_free(o);
   }
   
   for (l = epp->evas_objects; l; l = l->next)
   {
      Evas_Object_Wrapper *o = l->data;
      evas_del_object(o->evas, o->obj);

      free(o);
   }

   if (epp->layout)  ebits_free(epp->layout);
   if (epp->ui) ebits_free(epp->ui);
#endif
   D_RETURN;   
}

void
e_epplet_load_from_layout (E_View * v)
{
  char buf[PATH_MAX];
  Evas_List bit_names, l;

  D_ENTER;

#ifdef USE_FERITE   
  snprintf (buf, PATH_MAX, "%s/.e_epplets.bits.db", v->dir);
  v->epplet_layout = ebits_load (buf);
  if (!v->epplet_layout)
    D_RETURN;
  ebits_add_to_evas (v->epplet_layout, v->evas);
  ebits_resize (v->epplet_layout, v->size.w, v->size.h);
  ebits_move (v->epplet_layout, 0, 0);

  bit_names = ebits_get_bit_names (v->epplet_layout);

  for (l = bit_names; l; l = l->next)
    {
      double x, y, w, h;
      E_Epplet_Context *context;

      context = NEW (E_Epplet_Context, 1);
      ZERO (context, E_Epplet_Context, 1);

      context->name = l->data;

      ebits_get_named_bit_geometry (v->epplet_layout, context->name, &x, &y,
				    &w, &h);

      context->geom.x = x;
      context->geom.y = y;
      context->geom.w = w;
      context->geom.h = h;
      context->view = v;
      D ("epplet has following info:\n");
      D ("x: %f, y: %f, w: %f, h: %f\n", x, y, w, h);

      v->epplet_contexts = evas_list_append (v->epplet_contexts, context);

      snprintf (buf, PATH_MAX,  "%s/.e_epplets/%s/%s.fe", v->dir, context->name,
	       context->name);
      if (e_file_exists (buf))
	e_epplet_script_load (context, buf);
      else
	D ("Error: Can't find epplet `%s'\n", buf);
    }
#endif
   
  D_RETURN;
}



E_Epplet_Context *
e_epplet_get_context_from_script (FeriteScript * script)
{
  Evas_List l, ll;

  D_ENTER;

  D ("script address: %p\n", script);

#ifdef USE_FERITE   
  for (l = views; l; l = l->next)
    {
      E_View *v;

      v = l->data;
      D ("searching view: %s\n", v->dir);

      if (v->epplet_contexts == NULL)
	D ("no scripts in view\n");
      for (ll = v->epplet_contexts; ll; ll = ll->next)
	{
	  E_Epplet_Context *context = ll->data;
	  D ("found script: %p\n", context->script);

	  if (context->script == script)
	    D_RETURN_ (context);
	}
    }
#endif

  D_RETURN_ (NULL);
}


void
e_epplet_script_load (E_Epplet_Context * context, char *path)
{
  FeriteScript *script = NULL;

  D_ENTER;

#ifdef USE_FERITE   
  D ("Ferite: Compiling epplet script `%s'\n", path);
  script = ferite_script_compile (path);

  if (!script)
    {
      D ("Error compiling script... aborting\n");
      D_RETURN;
    }
  context->script = script;
  e_ferite_register (script, script->mainns);
  script->error_cb = e_ferite_script_error;
  script->warning_cb = e_ferite_script_warning;
  D ("Ferite: executing epplet.\n");

  ferite_script_execute (script);
  D ("Ferite: epplet executed.\n");
  /*ferite_script_delete(script); */
#endif

  D_RETURN;
}


void
e_epplet_set_common_callbacks (E_Epplet * epp)
{
/*
  D ("setting callbacks\n");

#ifdef USE_FERITE   
  if (!epp->bits)
    {
      D ("Error: no bits to set callbacks on\n");
      D_RETURN;
    }


  ebits_set_classed_bit_callback (epp->bits, "Title_Bar",
				  CALLBACK_MOUSE_DOWN, e_epplet_mouse_down_cb,
				  epp);
  ebits_set_classed_bit_callback (epp->bits, "Title_Bar", CALLBACK_MOUSE_UP,
				  e_epplet_mouse_up_cb, epp);
  ebits_set_classed_bit_callback (epp->bits, "Title_Bar", CALLBACK_MOUSE_MOVE,
				  e_epplet_mouse_move_cb, epp);

  ebits_set_classed_bit_callback (epp->bits, "Resize",
				  CALLBACK_MOUSE_DOWN, e_epplet_mouse_down_cb,
				  epp);
  ebits_set_classed_bit_callback (epp->bits, "Resize", CALLBACK_MOUSE_UP,
				  e_epplet_mouse_up_cb, epp);
  ebits_set_classed_bit_callback (epp->bits, "Resize", CALLBACK_MOUSE_MOVE,
				  e_epplet_mouse_move_cb, epp);

  ebits_set_classed_bit_callback (epp->bits, "Resize_Vertical",
				  CALLBACK_MOUSE_DOWN, e_epplet_mouse_down_cb,
				  epp);
  ebits_set_classed_bit_callback (epp->bits, "Resize_Vertical",
				  CALLBACK_MOUSE_UP, e_epplet_mouse_up_cb,
				  epp);
  ebits_set_classed_bit_callback (epp->bits, "Resize_Vertical",
				  CALLBACK_MOUSE_MOVE, e_epplet_mouse_move_cb,
				  epp);

  ebits_set_classed_bit_callback (epp->bits, "Resize_Horizontal",
				  CALLBACK_MOUSE_DOWN, e_epplet_mouse_down_cb,
				  epp);
  ebits_set_classed_bit_callback (epp->bits, "Resize_Horizontal",
				  CALLBACK_MOUSE_UP, e_epplet_mouse_up_cb,
				  epp);
  ebits_set_classed_bit_callback (epp->bits, "Resize_Horizontal",
				  CALLBACK_MOUSE_MOVE, e_epplet_mouse_move_cb,
				  epp);

#endif

  D ("callbacks set\n");
*/
}

static void
e_epplet_mouse_down_cb (void *_data, Ebits_Object _o,
			char *_c, int _b, int _x, int _y,
			int _ox, int _oy, int _ow, int _oh)
{
  E_Epplet *epp;

  D_ENTER;

#ifdef USE_FERITE   
  epp = _data;

  if (!strcmp (_c, "Title_Bar"))
    {
      epp->state.moving = 1;
      epp->offset.x = _x - epp->current.x;
      epp->offset.y = _y - epp->current.y;
    }

  if (!strcmp (_c, "Resize"))
    {
      if (_x < epp->current.x + (epp->current.w / 2))
	{
	  epp->state.resizing.left = 1;
	  epp->offset.x = epp->current.x - _x;
	}
      if (_x >= epp->current.x + (epp->current.w / 2))
	{
	  epp->state.resizing.right = 1;
	  epp->offset.x = epp->current.x + epp->current.w - _x;
	}
      if (_y < epp->current.y + (epp->current.h / 2))
	{
	  epp->state.resizing.up = 1;
	  epp->offset.y = epp->current.y - _y;
	}
      if (_y >= epp->current.y + (epp->current.h / 2))
	{
	  epp->state.resizing.down = 1;
	  epp->offset.y = epp->current.y + epp->current.h - _y;
	}

    }

  if (!strcmp (_c, "Resize_Horizontal"))
    {
      if (_x < epp->current.x + (epp->current.w / 2))
	{
	  epp->state.resizing.left = 1;
	  epp->offset.x = epp->current.x - _x;
	}
      else
	{
	  epp->state.resizing.right = 1;
	  epp->offset.x = epp->current.x + epp->current.w - _x;
	}
    }

  if (!strcmp (_c, "Resize_Vertical"))
    {
      if (_y < epp->current.y + (epp->current.h / 2))
	{
	  epp->state.resizing.up = 1;
	  epp->offset.y = epp->current.y - _y;
	}
      else
	{
	  epp->state.resizing.down = 1;
	  epp->offset.y = epp->current.y + epp->current.h - _y;
	}
    }
#endif
   
  D_RETURN;
}



static void
e_epplet_mouse_up_cb (void *_data, Ebits_Object _o,
		      char *_c, int _b, int _x, int _y,
		      int _ox, int _oy, int _ow, int _oh)
{
  E_Epplet *epp;

  D_ENTER;

#ifdef USE_FERITE   
  epp = _data;

  if (!strcmp (_c, "Title_Bar"))
    {
      epp->state.moving = 0;
    }

  if (!strncmp (_c, "Resize", 6))
    {
      epp->state.resizing.up = 0;
      epp->state.resizing.down = 0;
      epp->state.resizing.left = 0;
      epp->state.resizing.right = 0;
    }
#endif
  D_RETURN;
}

static void
e_epplet_mouse_move_cb (void *_data, Ebits_Object _o,
			char *_c, int _b, int _x, int _y,
			int _ox, int _oy, int _ow, int _oh)
{
  E_Epplet *epp;
  double x, y;

  D_ENTER;

#ifdef USE_FERITE   
/*  epp = _data;

  if (epp->state.moving)
    {
      x = _x - epp->offset.x;
      y = _y - epp->offset.y;
      if (x < 0)
	x = 0;
      if (y < 0)
	y = 0;
      if (x > epp->view->size.w - epp->current.w)
	x = epp->view->size.w - epp->current.w;
      if (y > epp->view->size.h - epp->current.h)
	y = epp->view->size.h - epp->current.h;
      epp->current.x = x;
      epp->current.y = y;

      ebits_move (epp->ui, epp->current.x, epp->current.y);
    }

  if (epp->state.resizing.left || epp->state.resizing.right
      || epp->state.resizing.up || epp->state.resizing.down)
    {
      int w, h, x, y;
      int mw, mh;

      if (epp->state.resizing.left)
	{
	  w = epp->current.x + epp->current.w - _x - epp->offset.x;
	  x = _x + epp->offset.x;
	}
      else if (epp->state.resizing.right)
	{
	  w = _x - epp->current.x + epp->offset.x;
	  x = epp->current.x;
	}
      else
	{
	  w = epp->current.w;
	  x = epp->current.x;
	}

      if (epp->state.resizing.up)
	{
	  h = epp->current.h + epp->current.y - _y - epp->offset.y;
	  y = _y + epp->offset.y;
	}

      else if (epp->state.resizing.down)
	{
	  h = _y - epp->current.y + epp->offset.y;
	  y = epp->current.y;
	}
      else
	{
	  h = epp->current.h;
	  y = epp->current.y;
	}

      ebits_get_max_size (epp->bits, &mw, &mh);
      if (w >= mw)
	{
	  w = mw;
	  x = epp->current.x;
	}
      if (h >= mh)
	{
	  h = mh;
	  y = epp->current.y;
	}

      ebits_get_min_size (epp->bits, &mw, &mh);
      if (w < mw)
	{
	  w = mw;
	  x = epp->current.x;
	}
      if (h < mh)
	{
	  h = mh;
	  y = epp->current.y;
	}

      epp->current.x = x;
      epp->current.y = y;
      epp->current.w = w;
      epp->current.h = h;

      ebits_resize (epp->bits, epp->current.w, epp->current.h);
      ebits_move (epp->bits, epp->current.x, epp->current.y);

    }
*/
#endif

  D_RETURN;
}


E_Epplet_CB_Info *
e_epplet_cb_new( FeriteScript *script, char *func_name, FeriteObject *data, FeriteObject *data2 )
{
  E_Epplet_CB_Info *cb;
  FeriteNamespaceBucket *nsb;
  
  D_ENTER;
  
#ifdef USE_FERITE   
  cb = NEW(E_Epplet_CB_Info, 1);
  ZERO(cb, E_Epplet_CB_Info, 1);

  nsb = __ferite_find_namespace( script, script->mainns, func_name, FENS_FNC);
  if (nsb != NULL)
  {
     cb->func = nsb->data;
     if (data)
     {
	cb->data = data;
	data->refcount++;
     }
     if (data2)
     {
       cb->data2 = data2;
       data2->refcount++;
     }
     cb->script = script;
  } 
#endif

  D_RETURN_(cb);
}

void
e_epplet_cb_cleanup(E_Epplet_CB_Info *cb)
{
#ifdef USE_FERITE
   if (cb->data)
   {
      cb->data->refcount--;
      cb->data = NULL;
   }
   if (cb->data2)
   {
      cb->data2->refcount--;
      cb->data2 = NULL;
   }
   free(cb);
#endif
}

void
e_epplet_bits_cb (void *_data, Ebits_Object _o,
			char *_c, int _b, int _x, int _y,
			int _ox, int _oy, int _ow, int _oh)
{
  E_Epplet_CB_Info *cb;
  FeriteVariable **params;
  
  D_ENTER;

#ifdef USE_FERITE   
  cb = _data;

  if (cb->script) {
    D("creating params and calling func\n");
    params = __ferite_create_parameter_list_from_data( cb->script, "osnnnnnnn",
       cb->data, _c, _b, _x, _y, _ox, _oy, _ow, _oh );
     __ferite_variable_destroy( cb->script, __ferite_call_function( cb->script, cb->func, params));
     __ferite_delete_parameter_list( cb->script, params );
    D("func called, params deleted\n");
  }
  else 
  {
     D("ERROR: script does not exist\n");
  }
 /* e_epplet_cb_cleanup(cb); */
#endif
  D_RETURN; 
}

void
e_epplet_evas_cb (void *_data, Evas _e, Evas_Object _o,
			int _b, int _x, int _y)
			
{
  E_Epplet_CB_Info *cb;
  FeriteVariable **params;
  
  D_ENTER;

#ifdef USE_FERITE   
  cb = _data;

  D("d1: %s, d2: %s\n", cb->data->name, cb->data2->name);

  if (cb->script) {
    D("creating params\n");
    params = __ferite_create_parameter_list_from_data( cb->script, "oonnn",
       cb->data, cb->data2, (double)_b, (double)_x, (double)_y );
    D("calling func: %s\n", cb->func->name);
     __ferite_variable_destroy( cb->script, __ferite_call_function( cb->script, cb->func, params));
     __ferite_delete_parameter_list( cb->script, params );
    D("func called, params deleted\n");
  }
  else 
  {
     D("ERROR: script does not exist\n");
  }
/*  e_epplet_cb_cleanup(cb); */
#endif
  D_RETURN; 
}

void
e_epplet_timer_func(int val, void *data)
{
  E_Epplet_CB_Info *cb;
  FeriteVariable **params;
  
  D_ENTER;

#ifdef USE_FERITE   
/*  D("in timer func\n"); */
  cb = data;

  if (cb->script) {
/*    D("creating params\n"); */
    params = __ferite_create_parameter_list_from_data( cb->script, "on",
       cb->data, (float)val );
/*    D("calling func\n"); */
     __ferite_variable_destroy( cb->script, __ferite_call_function( cb->script, cb->func, params));
     __ferite_delete_parameter_list( cb->script, params );
/*    D("func called, params deleted\n"); */
  }
  else
  {
     D("ERROR: script does not exist\n");
  }
  
/*  e_epplet_cb_cleanup(cb); */
#endif
  D_RETURN;
}

E_Epplet_Observer *
e_epplet_observer_new(FeriteScript *script, char *func_name, FeriteObject *data, char *event_type)
{
   E_Epplet_Observer *obs = NULL;
   FeriteNamespaceBucket *nsb;

   D_ENTER;
   
#ifdef USE_FERITE   
   obs = NEW(E_Epplet_Observer, 1);
   memset(obs, 0, sizeof(E_Epplet_Observer));

   if (!strcmp(event_type, "DESKTOP_SWITCH"))
      e_observer_init(E_OBSERVER(obs), E_EVENT_DESKTOP_SWITCH, 
	              e_epplet_desktop_observer_func,
		      (E_Cleanup_Func)e_epplet_observer_cleanup);
#if 0
   else if (!strcmp(event_type, "ICONIFY"))
      e_observer_init(E_OBSERVER(obs), E_EVENT_BORDER_ICONIFY, 
	              e_epplet_border_observer_func,
		      (E_Cleanup_Func)e_epplet_observer_cleanup);
   else if (!strcmp(event_type, "UNICONIFY"))
      e_observer_init(E_OBSERVER(obs), E_EVENT_BORDER_UNICONIFY, 
	              e_epplet_border_observer_func,
		      (E_Cleanup_Func)e_epplet_observer_cleanup);
   else
      e_observer_init(E_OBSERVER(obs), E_EVENT_MAX, 
	              e_epplet_desktop_observer_func,
		      (E_Cleanup_Func)e_epplet_observer_cleanup);
#endif

   nsb = __ferite_find_namespace( script, script->mainns, func_name, FENS_FNC);
   if (nsb != NULL)
   {
      obs->script = script;
      obs->func = nsb->data;
      if (data)
      {
	 ((E_Epplet_Observer *)obs)->data = data;
	 data->refcount++;
      }
   } 
   D("returning, event: %i\n", E_OBSERVER(obs)->event);
#endif
   D_RETURN_(obs); 
}

static void
e_epplet_observer_cleanup(E_Object *o)
{
#ifdef USE_FERITE   
   /*FIXME: we need something here!!! Leeeeaky! */
#endif   
}

void
e_epplet_observer_register_desktops(E_Epplet_Observer *obs)
{
   Evas_List l;

   D_ENTER;
#ifdef USE_FERITE   
   D("odeskregister, bserver func: %s\n", obs->func->name); 
   D("register each desktop in list\n");
   for (l = e_desktops_get_desktops_list(); l; l = l->next)
   { 
      E_Desktop *d = l->data;
      D("registering desktop...\n")
      e_observer_register_observee(E_OBSERVER(obs), E_OBSERVEE(d));
      D("desktop registered\n")
   }
#endif
   D_RETURN;
}
#if 0
void
e_epplet_observer_register_borders(E_Epplet_Observer *obs)
{
   Evas_List l;

   D_ENTER;
#ifdef USE_FERITE   
   for (l = e_border_get_borders_list(); l; l = l->next)
   { 
      E_Border *b = l->data;
      D("registering desktop...\n")
      e_observer_register_observee(E_OBSERVER(obs), E_OBSERVEE(b));
      D("desktop registered\n")
   }
#endif
   D_RETURN;
}
#endif

void
e_epplet_desktop_observer_func(E_Observer *observer, E_Observee *observee, E_Event_Type event)
{
   E_Epplet_Observer *obs;
   E_Desktop *desk;
   FeriteVariable **params;
   
   D_ENTER;

#ifdef USE_FERITE   
   obs = (E_Epplet_Observer *)observer;
   desk = (E_Desktop *)observee;
   
   D("desktop: %i\n", desk->desk.desk);
   D("current: %i\n", e_desktops_get_current());
   if (obs->script)
   {
      D("creating params\n");
       
      params = __ferite_create_parameter_list_from_data( obs->script, "on",
               obs->data, (float)(desk->desk.desk) );
      D("calling func: %s\n", obs->func->name);
      __ferite_variable_destroy( obs->script, __ferite_call_function(
	                          obs->script, obs->func, params));
      D("function called\n");
       __ferite_delete_parameter_list( obs->script, params );
      D("func called, params deleted\n");
   }
   else 
   {
      D("ERROR: script does not exist\n");
   }
#endif
   D_RETURN;
}

#if 0 /* don't use this, its currently broken */
void
e_epplet_border_observer_func(E_Observer *observer, E_Observee *observee)
{
   E_Epplet_Observer *obs;
   E_Border *b;
   FeriteVariable **params;
   
   D_ENTER;

#ifdef USE_FERITE   
   obs = (E_Epplet_Observer *)observer;
   b = (E_Border *)observee;
   
   obs->data->odata = b;
   
   if (obs->script)
   {
      /*D("creating params\n");*/
       
      params = __ferite_create_parameter_list_from_data( obs->script, "o",
               obs->data );
      /*D("calling func: %s\n", obs->func->name);*/
      __ferite_variable_destroy( obs->script, __ferite_call_function(
	                          obs->script, obs->func, params));
      /*D("function called\n");*/
       __ferite_delete_parameter_list( obs->script, params );
      /*D("func called, params deleted\n");*/
   }
   else 
   {
      D("ERROR: script does not exist\n");
   }
#endif
   D_RETURN;
}
#endif

