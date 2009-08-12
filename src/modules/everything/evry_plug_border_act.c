#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  E_Border *border;
};

static Evry_Plugin *p;
static Inst *inst;


static void
_act_cb_border_switch_to(E_Border *bd)
{
   E_Zone *zone;

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
}

static void
_act_cb_border_fullscreen(E_Border *bd)
{
   if (!bd->fullscreen)
     e_border_fullscreen(bd, E_FULLSCREEN_RESIZE);
   else
     e_border_unfullscreen(bd);
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

static void
_act_cb_border_unminimize(E_Border *bd)
{
   if (!bd->lock_user_iconify) e_border_uniconify(bd);
}

static int
_begin(Evry_Plugin *p __UNUSED__, const Evry_Item *item)
{
   E_Border *bd;

   bd = item->data[0];
   /* e_object_ref(E_OBJECT(bd)); */
   inst->border = bd;

   return 1;
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;

   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}

static void
_item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon, char *m1, char *m2)
{
   Evry_Item *it;
   int prio = 1;

   if (m1[0] && m2[0])
     {
	if (e_util_glob_case_match(label, m1))
	  prio = 1;
	else if (e_util_glob_case_match(label, m2))
	  prio = 2;
	else
	  prio = 0;
     }
   
   if (!prio) return;
   
   it = evry_item_new(p, label);
   it->data[0] = action_cb;
   it->data[1] = (void *) eina_stringshare_add(icon);
   it->priority = prio;
   
   p->items = eina_list_prepend(p->items, it);
}

static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->data[1]) eina_stringshare_del((const char *)it->data[1]);
	evry_item_free(it);
     }
}

static int
_fetch(Evry_Plugin *p, const char *input __UNUSED__)
{
   char m1[128];
   char m2[128];

   _cleanup(p);

   if (input)
     {
	snprintf(m1, sizeof(m1), "%s*", input);
	snprintf(m2, sizeof(m2), "*%s*", input);
     }
   else
     {
	m1[0] = 0;
	m2[0] = 0;
     }
   
   _item_add(p, _("Switch To"), _act_cb_border_switch_to, "go-next", m1, m2);

   if (inst->border->iconic)
     _item_add(p, _("Uniconify"), _act_cb_border_unminimize, "window-minimize", m1, m2);
   else	
     _item_add(p, _("Iconify"), _act_cb_border_minimize, "window-minimize", m1, m2);

   if (!inst->border->fullscreen)
     _item_add(p, _("Fullscreen"), _act_cb_border_fullscreen, "view-fullscreen", m1, m2);
   else
     _item_add(p, _("Unfullscreen"), _act_cb_border_fullscreen, "view-restore", m1, m2);

   _item_add(p, _("Close"), _act_cb_border_close, "window-close", m1, m2);

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	return 1;
     }

   return 0;
}

static int
_action(Evry_Plugin *p __UNUSED__, const Evry_Item *item, const char *input __UNUSED__)
{
   void (*border_action) (E_Border *bd);
   border_action = item->data[0];
   border_action(inst->border);

   return EVRY_ACTION_FINISHED;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o;

   o = e_icon_add(e); 
   evry_icon_theme_set(o, (const char *)it->data[1]);

   /* icon = edje_object_add(e);
    * e_theme_edje_object_set(icon, "base/theme/borders", (const char *)it->data[1]); */

   return o;
}

static Eina_Bool
_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Window Action";
   p->type = type_action;
   p->type_in  = "BORDER";
   p->type_out = "NONE";
   p->need_query = 0;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;

   evry_plugin_register(p);

   inst = E_NEW(Inst, 1);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_unregister(p);
   E_FREE(p);
   E_FREE(inst);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
