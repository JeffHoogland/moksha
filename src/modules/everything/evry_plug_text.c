#include "e_mod_main.h"

static Evry_Plugin *p1;
static Evry_Plugin *p2;


static void
_cleanup(Evry_Plugin *p)
{
   EVRY_PLUGIN_ITEMS_FREE(p);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Item *it;

   EVRY_PLUGIN_ITEMS_FREE(p);

   if (input)
     {
	it = evry_item_new(NULL, p, input, NULL, NULL);
	it->fuzzy_match = 999;
	EVRY_PLUGIN_ITEM_APPEND(p, it);

	return 1;
     }

   return 0;
}

Eina_Bool
evry_plug_text_init(void)
{
   p1 = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Text"),
			"accessories-text-editor", EVRY_TYPE_TEXT,
			NULL, _cleanup, _fetch, NULL);

   p2 = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Text"),
			"accessories-text-editor", EVRY_TYPE_TEXT,
			NULL, _cleanup, _fetch, NULL);

   if (evry_plugin_register(p1, EVRY_PLUGIN_OBJECT,999))
     {
	p1->config->trigger_only = 1;
	p1->config->trigger = eina_stringshare_add(" ");	
     }
   
   if (evry_plugin_register(p2, EVRY_PLUGIN_SUBJECT, 999))
     {
	p2->config->trigger_only = 1;
	p2->config->trigger = eina_stringshare_add(" ");
     }
   
   return EINA_TRUE;
}

void
evry_plug_text_shutdown(void)
{
   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);
}
