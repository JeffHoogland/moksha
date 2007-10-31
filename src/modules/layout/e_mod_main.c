/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static E_Module *layout_module = NULL;
static E_Border_Hook *hook = NULL;

static void
_e_module_layout_cb_hook(void *data, E_Border *bd)
{
   /* FIXME: make some modification based on policy */
   printf("Window:\n"
	  "  Title:    [%s][%s]\n"
	  "  Class:    %s::%s\n"
	  "  Geometry: %ix%i+%i+%i\n"
	  "  New:      %i\n"
	  , bd->client.icccm.title, bd->client.netwm.name
	  , bd->client.icccm.name, bd->client.icccm.class
	  , bd->x, bd->y, bd->w, bd->h
	  , bd->new_client
	  );
   if ((bd->client.icccm.transient_for != 0) ||
       (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG))
     {
	bd->client.e.state.centered = 1;
     }
   else
     {
	e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
	e_border_move(bd,
		      bd->zone->x + (bd->zone->w / 2),
		      bd->zone->y + (bd->zone->h / 2));
	e_border_resize(bd, 1, 1);
	if (bd->bordername) evas_stringshare_del(bd->bordername);
	bd->bordername = evas_stringshare_add("borderless");
	bd->client.icccm.base_w = 1;
	bd->client.icccm.base_h = 1;
	bd->client.icccm.min_w = 1;
	bd->client.icccm.min_h = 1;
	bd->client.icccm.max_w = 32767;
	bd->client.icccm.max_h = 32767;
	bd->client.icccm.min_aspect = 0.0;
	bd->client.icccm.max_aspect = 0.0;
     }
   e_border_maximize(bd, E_MAXIMIZE_FILL | E_MAXIMIZE_BOTH);
}

/**/
/***************************************************************************/

/***************************************************************************/
/**/

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Layout"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   layout_module = m;

   hook = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH,
			    _e_module_layout_cb_hook, NULL);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (hook)
     {
	e_border_hook_del(hook);
	hook = NULL;
     }
   layout_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}
