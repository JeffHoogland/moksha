#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  E_Border *border;
};

static int  _begin(Evry_Plugin *p, Evry_Item *item);
static int  _fetch(Evry_Plugin *p, const char *input);
static int  _action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _cleanup(Evry_Plugin *p);
static void _item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);
static void _item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon);

static Eina_Bool _init(void);
static void _shutdown(void);
EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

static Evry_Plugin *p;
static Inst *inst;

static Eina_Bool
_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Window Action";
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

static int
_begin(Evry_Plugin *p __UNUSED__, Evry_Item *item)
{
   E_Border *bd;

   bd = item->data[0];
   /* e_object_ref(E_OBJECT(bd)); */
   inst->border = bd;

   return 1;
}

static int
_fetch(Evry_Plugin *p, const char *input __UNUSED__)
{
   _cleanup(p);

   _item_add(p, _("Iconify"), _act_cb_border_minimize,
	     "e/widgets/border/default/minimize");

   if (!inst->border->fullscreen)
     _item_add(p, _("Fullscreen"), _act_cb_border_fullscreen,
	       "e/widgets/border/default/fullscreen");
   else
     _item_add(p, _("Unfullscreen"), _act_cb_border_fullscreen,
	       "e/widgets/border/default/fullscreen");

   _item_add(p, _("Close"), _act_cb_border_close,
	     "e/widgets/border/default/close");
   return 1;
}

static int
_action(Evry_Plugin *p __UNUSED__, Evry_Item *item, const char *input __UNUSED__)
{
   void (*border_action) (E_Border *bd);
   border_action = item->data[0];
   border_action(inst->border);

   return EVRY_ACTION_FINISHED;
}

static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->data[1]) eina_stringshare_del(it->data[1]);
	if (it->label) eina_stringshare_del(it->label);
	E_FREE(it);
     }
}

static void
_item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon)
{
   Evry_Item *it;

   it = E_NEW(Evry_Item, 1);
   it->data[0] = action_cb;
   it->data[1] = (void *) eina_stringshare_add(icon);
   it->label = eina_stringshare_add(label);
   p->items = eina_list_append(p->items, it);
}

static void
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e)
{
   it->o_icon = edje_object_add(e);
   e_theme_edje_object_set(it->o_icon, "base/theme/borders", (const char *)it->data[1]);
}
