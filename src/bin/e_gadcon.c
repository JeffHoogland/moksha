/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_gadcon_free(E_Gadcon *gc);
static void _e_gadcon_client_free(E_Gadcon_Client *gcc);

/* externally accessible functions */
EAPI int
e_gadcon_init(void)
{
   return 1;
}

EAPI int
e_gadcon_shutdown(void)
{
   return 1;
}

EAPI E_Gadcon *
e_gadcon_swallowed_new(Evas_Object *obj, char *swallow_name)
{
   E_Gadcon    *gc;
   
   gc = E_OBJECT_ALLOC(E_Gadcon, E_GADCON_TYPE, _e_gadcon_free);
   if (!gc) return NULL;
   
   gc->type = E_GADCON_TYPE_ARRANGE;
   gc->edje.o_parent = obj;
   gc->edje.swallow_name = strdup(swallow_name);
   
   gc->evas = evas_object_evas_get(obj);
   gc->o_container = NULL; // need to make a new smart obj for the layout
   return gc;
}

EAPI E_Gadcon *
e_gadcon_freefloat_new(Ecore_Evas *ee)
{
   E_Gadcon    *gc;

   gc = E_OBJECT_ALLOC(E_Gadcon, E_GADCON_TYPE, _e_gadcon_free);
   if (!gc) return NULL;
   
   gc->type = E_GADCON_TYPE_FREEFLOAT;
   gc->ecore_evas.ee = ee;
   
   gc->evas = ecore_evas_get(ee);
   gc->o_container = NULL; // use layout smart
   return gc;
}

EAPI void
e_gadcon_orient_set(E_Gadcon *gc, E_Gadcon_Orient orient)
{
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   if (gc->type != E_GADCON_TYPE_ARRANGE) return;
   gc->edje.orient = orient;
}

/* only needed if it's a freefloat type and when the canvas changes size */
EAPI void
e_gadcon_resize(E_Gadcon *gc, int w, int h)
{
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   if (gc->type != E_GADCON_TYPE_FREEFLOAT) return;
   e_layout_virtual_size_set(gc->o_container, w, h);
   evas_object_resize(gc->o_container, w, h);
}




EAPI E_Gadcon_Client *
e_gadcon_client_new(E_Gadcon *gc)
{
   E_Gadcon_Client *gcc;
   
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   
   gcc = E_OBJECT_ALLOC(E_Gadcon_Client, E_GADCON_CLIENT_TYPE, _e_gadcon_client_free);
   if (!gcc) return NULL;
   gcc->gadcon = gc;
   gc->clients = evas_list_append(gc->clients, gcc);
   return gcc;
}

/* local subsystem functions */
static void
_e_gadcon_free(E_Gadcon *gc)
{
   if (gc->o_container) evas_object_del(gc->o_container);
   E_FREE(gc->edje.swallow_name);
   free(gc);
}

static void
_e_gadcon_client_free(E_Gadcon_Client *gcc)
{
   gcc->gadcon->clients = evas_list_remove(gcc->gadcon->clients, gcc);
   free(gcc);
}
