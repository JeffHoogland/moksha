#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  E_Border *border;
};

static Evry_Plugin * _src_border_new(void);
static void _src_border_free(Evry_Plugin *p);
static int  _src_border_fetch(Evry_Plugin *p, const char *input);
static int  _src_border_action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _src_border_cleanup(Evry_Plugin *p);
static void _src_border_item_add(Evry_Plugin *p, E_Border *bd, int prio);
static int  _src_border_cb_sort(const void *data1, const void *data2);
static void _src_border_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

static Evry_Plugin * _act_border_new(void);
static void _act_border_free(Evry_Plugin *p);
static int  _act_border_begin(Evry_Plugin *p, Evry_Item *item);
static int  _act_border_fetch(Evry_Plugin *p, const char *input);
static int  _act_border_action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _act_border_cleanup(Evry_Plugin *p);
static void _act_border_item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon);
static void _act_border_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

static Evry_Plugin_Class source;
static Evry_Plugin_Class action;


EAPI int
evry_plug_border_init(void)
{
   source.name = "Windows";
   source.type_in  = "NONE";
   source.type_out = "BORDER";
   source.need_query = 0;
   source.prio = 0;
   source.new = &_src_border_new;
   source.free = &_src_border_free;
   evry_plugin_register(&source);

   action.name = "Window Action";
   action.type_in  = "BORDER";
   action.type_out = "NONE";
   action.need_query = 0;
   action.prio = 0;
   action.new = &_act_border_new;
   action.free = &_act_border_free;
   evry_plugin_register(&action);
   
   return 1;
}

EAPI int
evry_plug_border_shutdown(void)
{
   evry_plugin_unregister(&source);
   evry_plugin_unregister(&action);
   
   return 1;
}

static Evry_Plugin *
_src_border_new()
{
   Evry_Plugin *p;
   
   p = E_NEW(Evry_Plugin, 1);
   p->class = &source;
   p->fetch = &_src_border_fetch;
   p->action = &_src_border_action;
   p->cleanup = &_src_border_cleanup;
   p->icon_get = &_src_border_item_icon_get;
   p->items = NULL;   

   return p;
}

static void
_src_border_free(Evry_Plugin *p)
{
   _src_border_cleanup(p);
   E_FREE(p);
}


static int
_src_border_action(Evry_Plugin *p, Evry_Item *it, const char *input)
{
   E_Border *bd;
   E_Zone *zone;

   if (!it) return 0;
   
   bd = (E_Border *)it->data[0];
   zone = e_util_zone_current_get(e_manager_current_get());
   
   if (bd->desk != (e_desk_current_get(zone)))
     e_desk_show(bd->desk);
   
   if (bd->shaded)
     e_border_unshade(bd, E_DIRECTION_UP);

   if (bd->iconic)
     e_border_uniconify(bd);
   else
     e_border_raise(bd);

   /* e_border_focus_set(bd, 1, 1); */
   e_border_focus_set_with_pointer(bd);
   
   return 1;
}

static void
_src_border_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	/* if (it->data[0]) e_object_unref(E_OBJECT(it->data[0])); */
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	free(it);
     }
}

static int
_src_border_fetch(Evry_Plugin *p, const char *input)
{
   E_Manager *man;
   E_Zone *zone;
   
   char match1[4096];
   char match2[4096];
   Eina_List *list;
   E_Border *bd;
   E_Border_List *bl;

   _src_border_cleanup(p); 

   man = e_manager_current_get();
   zone = e_util_zone_current_get(man);
   
   if (input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	snprintf(match2, sizeof(match2), "*%s*", input);
     }
   
   bl = e_container_border_list_first(e_container_current_get(man));
   while ((bd = e_container_border_list_next(bl)))
     {
	if (zone == bd->zone)
	  {
	     if (!input)
	       _src_border_item_add(p, bd, 1);
	     else if (bd->client.icccm.name &&
		      e_util_glob_case_match(bd->client.icccm.name, match1))
	       _src_border_item_add(p, bd, 1);
	     else  if (e_util_glob_case_match(e_border_name_get(bd), match1))
	       _src_border_item_add(p, bd, 1);
	     else if (bd->client.icccm.name &&
		      e_util_glob_case_match(bd->client.icccm.name, match2))
	       _src_border_item_add(p, bd, 2);
	     else if (e_util_glob_case_match(e_border_name_get(bd), match2))
	       _src_border_item_add(p, bd, 2);
	  }
     }
   e_container_border_list_free(bl);

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items),
				  _src_border_cb_sort);
	return 1;
     }

   return 0;
}

static void
_src_border_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{ 
   it->o_icon = e_border_icon_add(((E_Border *)it->data[0]), e); 
}

static void
_src_border_item_add(Evry_Plugin *p, E_Border *bd, int prio)
{
   Evry_Item *it;   

   it = calloc(1, sizeof(Evry_Item));
   /* e_object_ref(E_OBJECT(bd)); */
   it->data[0] = bd;
   it->priority = prio;
   it->label = eina_stringshare_add(e_border_name_get(bd));
	     
   p->items = eina_list_append(p->items, it);
}

/* TODO sort by focus history and name? */
static int
_src_border_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   
   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}



static Evry_Plugin *
_act_border_new()
{
   Evry_Plugin *p;
   Inst *inst;
   
   p = E_NEW(Evry_Plugin, 1);
   p->class = &source;
   p->begin = &_act_border_begin;
   p->fetch = &_act_border_fetch;
   p->action = &_act_border_action;
   p->cleanup = &_act_border_cleanup;
   p->icon_get = &_act_border_item_icon_get;
   p->items = NULL;   

   inst = E_NEW(Inst, 1);
   p->priv = inst;
   
   return p;
}

static void
_act_border_free(Evry_Plugin *p)
{
   Inst *inst = p->priv;

   /* if (inst->border) e_object_unref(E_OBJECT(inst->border)); */

   _act_border_cleanup(p);

   E_FREE(p);
   E_FREE(inst);
}

static void
_act_cb_border_close(E_Border *bd)
{
   if (!bd->lock_close) e_border_act_close_begin(bd);
}

static void
_act_cb_border_minimize(E_Border *bd)
{
   if (!bd->lock_user_iconify) e_border_iconify(bd);
}

static int
_act_border_begin(Evry_Plugin *p, Evry_Item *item)
{
   Inst *inst = p->priv;
   E_Border *bd;

   bd = item->data[0];
   /* e_object_ref(E_OBJECT(bd)); */
   inst->border = bd;

   return 1;
}

static int
_act_border_fetch(Evry_Plugin *p, const char *input)
{
   _act_border_cleanup(p);

   _act_border_item_add(p, _("Iconify"), _act_cb_border_minimize,
			     "e/widgets/border/default/minimize");

   _act_border_item_add(p, _("Close"), _act_cb_border_close,
			     "e/widgets/border/default/close");
   return 1;
}

static int
_act_border_action(Evry_Plugin *p, Evry_Item *item, const char *input)
{
   Inst *inst = p->priv;
   
   void (*border_action) (E_Border *bd);
   border_action = item->data[0];
   border_action(inst->border);
   return 1;
}

static void
_act_border_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->data[1]) eina_stringshare_del(it->data[1]);
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	free(it);
     }
}

static void
_act_border_item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon)
{
   Evry_Item *it;   

   it = calloc(1, sizeof(Evry_Item));
   it->data[0] = action_cb;
   it->data[1] = (void *) eina_stringshare_add(icon);
   it->label = eina_stringshare_add(label);
   p->items = eina_list_append(p->items, it);   
}

static void
_act_border_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   it->o_icon = edje_object_add(e);
   e_theme_edje_object_set(it->o_icon, "base/theme/borders", (const char *)it->data[1]);
}

