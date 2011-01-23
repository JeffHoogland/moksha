#include "e_mod_main.h"

static Evry_View *view;
static Evas_Object *o_text = NULL;

static void
_view_clear(Evry_View *v)
{
   v->active = 0;
   evas_object_del(v->o_list);
   evas_object_del(o_text);
   o_text = NULL;
}

static int
_view_update(Evry_View *v __UNUSED__)
{
   return 1;
}

static int
_cb_key_down(Evry_View *v, const Ecore_Event_Key *ev)
{
   Evas_Object *o;
   double align;
   int h;

   if (!strcmp(ev->key, "Down"))
     {
	o = v->o_list;
	evas_object_geometry_get(o, NULL, NULL, NULL, &h);
	if (!h) h = 1;
	e_box_align_get(o, NULL, &align);

	align = align - 10.0/(double)h;
	if (align < 0.0) align = 0.0;

	e_box_align_set(v->o_list, 0.5, align);

	return 1;
     }
   else if (!strcmp(ev->key, "Up"))
     {
	o = v->o_list;
	evas_object_geometry_get(o, NULL, NULL, NULL, &h);
	if (!h) h = 1;
	e_box_align_get(o, NULL, &align);

	align = align + 10.0/(double)h;
	if (align > 1.0) align = 1.0;

	e_box_align_set(v->o_list, 0.5, align);
	return 1;
     }

   evry_view_toggle(v->state, NULL);
   return 1;
}

static Evry_View *
_view_create(Evry_View *v, const Evry_State *s __UNUSED__, const Evas_Object *swallow)
{
   Evas_Object *o;
   int mw, mh;
   
   char *text =
     _("  Ok, here comes the explanation of <hilight>everything</hilight>...<br>"
       "  Just type a few letters of the thing you are looking for. <br>"
       "  Use cursor <hilight>&lt;up/down&gt;</hilight> to choose from the list of things.<br>"
       "  Press  <hilight>&lt;tab&gt;</hilight> to select"
       " an action, then press  <hilight>&lt;return&gt;</hilight>.<br>"
       " This page will not show up next time you run <hilight>everything</hilight>.<br>"
       "    <hilight>&lt;Esc&gt;</hilight> close this Dialog<br>"
       "    <hilight>&lt;?&gt;</hilight> show this page<br>"
       "    <hilight>&lt;return&gt;</hilight> run action<br>"
       "    <hilight>&lt;ctrl+return&gt;</hilight> run action and continue<br>"
       "    <hilight>&lt;tab&gt;</hilight> toggle between selectors<br>"
       "    <hilight>&lt;ctrl+tab&gt;</hilight> complete input (depends on plugin)<br>"
       "    <hilight>&lt;ctrl+'x'&gt;</hilight> jump to plugin beginning with 'x'<br>"
       "    <hilight>&lt;ctrl+left/right&gt;</hilight> cycle through plugins<br>"
       "    <hilight>&lt;ctrl+up/down&gt;</hilight> go to first/last item<br>"
       "    <hilight>&lt;ctrl+1&gt;</hilight> toggle view modes (exit this page ;)<br>"
       "    <hilight>&lt;ctrl+2&gt;</hilight> toggle list view modes<br>"
       "    <hilight>&lt;ctrl+3&gt;</hilight> toggle thumb view modes"
       );

   if (v->active) return v;

   o = e_box_add(evas_object_evas_get(swallow));
   e_box_orientation_set(o, 0);
   e_box_align_set(o, 0.5, 1.0);
   v->o_list = o;
   e_box_freeze(v->o_list);
   o = edje_object_add(evas_object_evas_get(swallow));
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/modules/everything/textblock");

   edje_object_part_text_set(o, "e.textblock.text", text);
   e_box_pack_start(v->o_list, o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_box_pack_options_set(o, 1, 0, 1, 0, 0.5, 0.5, mw, mh + 200, 999, 999);
   e_box_thaw(v->o_list);
   evas_object_show(o);
   o_text = o;

   v->active = 1;

   return v;
}

static void
_view_destroy(Evry_View *v)
{
   v->active = 0;
}

Eina_Bool
evry_view_help_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   view = E_NEW(Evry_View, 1);
   view->id = view;
   view->name = "Help";
   view->create = &_view_create;
   view->destroy = &_view_destroy;
   view->update = &_view_update;
   view->clear = &_view_clear;
   view->cb_key_down = &_cb_key_down;
   view->trigger = "?";
   /* view->types = "NONE"; */
   evry_view_register(view, 2);

   return EINA_TRUE;
}

void
evry_view_help_shutdown(void)
{
   evry_view_unregister(view);
   E_FREE(view);
}
