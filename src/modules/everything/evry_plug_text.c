#include "e_mod_main.h"

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
};

static Evry_Plugin *p1, *p2;

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it __UNUSED__)
{
   Plugin *p;

   EVRY_PLUGIN_INSTANCE(p, plugin);
   
   return EVRY_PLUGIN(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   
   EVRY_PLUGIN_ITEMS_FREE(p);
   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Evry_Item *it;
   
   GET_PLUGIN(p, plugin);

   if (input)
     {
	if (!p->base.items)
	  {
	     it = evry_item_new(NULL, EVRY_PLUGIN(p), input, NULL, NULL);
	     it->fuzzy_match = 999;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	  }
	else
	  {
	     it = p->base.items->data;
	     EVRY_ITEM_LABEL_SET(it, input);
	     evry_item_changed(it, 0, 0);
	  }
	return 1;
     }

   EVRY_PLUGIN_ITEMS_FREE(p);
   
   return 0;
}

Eina_Bool
evry_plug_text_init(void)
{
   p1 = EVRY_PLUGIN_BASE("Text", "accessories-text-editor",
			 EVRY_TYPE_TEXT, _begin, _finish, _fetch);

   p2 = EVRY_PLUGIN_BASE("Text", "accessories-text-editor",
			 EVRY_TYPE_TEXT, _begin, _finish, _fetch);

   if (evry_plugin_register(p1, EVRY_PLUGIN_OBJECT, 999))
     {
	p1->config->trigger = eina_stringshare_add(" ");
	p1->config->aggregate = EINA_FALSE;
	p1->config->top_level = EINA_FALSE;
	p1->config->view_mode = VIEW_MODE_LIST;
     }

   if (evry_plugin_register(p2, EVRY_PLUGIN_SUBJECT, 999))
     {
	p2->config->trigger = eina_stringshare_add(" ");
	p2->config->aggregate = EINA_FALSE;
	p2->config->top_level = EINA_FALSE;
	p2->config->view_mode = VIEW_MODE_LIST;
     }

   return EINA_TRUE;
}

void
evry_plug_text_shutdown(void)
{
   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);
}
