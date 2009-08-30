#include "e_mod_main.h"

#define HISTORY_VERSION 1

static E_Config_DD *hist_entry_edd = NULL;
static E_Config_DD *hist_item_edd = NULL;
static E_Config_DD *hist_edd = NULL;

History *evry_hist = NULL;

void
evry_history_init(void)
{
#undef T
#undef D
   hist_item_edd = E_CONFIG_DD_NEW("History_Item", History_Item);
#define T History_Item
#define D hist_item_edd
   E_CONFIG_VAL(D, T, plugin, STR);
   E_CONFIG_VAL(D, T, context, STR);
   E_CONFIG_VAL(D, T, input, STR);
   E_CONFIG_VAL(D, T, last_used, DOUBLE);
   E_CONFIG_VAL(D, T, count, INT);  
#undef T
#undef D
   hist_entry_edd = E_CONFIG_DD_NEW("History_Entry", History_Entry);
#define T History_Entry
#define D hist_entry_edd
   E_CONFIG_LIST(D, T, items, hist_item_edd);
#undef T
#undef D
   hist_edd = E_CONFIG_DD_NEW("History_Item", History);
#define T History
#define D hist_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_HASH(D, T, subjects, hist_entry_edd);
   E_CONFIG_HASH(D, T, actions,  hist_entry_edd);
#undef T
#undef D   
}

static Eina_Bool
_hist_free_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   History_Item *hi;
   
   EINA_LIST_FREE(he->items, hi)
     {
	if (hi->input)
	  eina_stringshare_del(hi->input);
	if (hi->plugin)
	  eina_stringshare_del(hi->plugin);
	if (hi->context)
	  eina_stringshare_del(hi->context);
	E_FREE(hi);
     }

   E_FREE(he);
   return 1;
}

void
evry_history_free(void)
{
   E_CONFIG_DD_FREE(hist_item_edd);
   E_CONFIG_DD_FREE(hist_entry_edd);
   E_CONFIG_DD_FREE(hist_edd);
}

void
evry_history_load(void)
{
   evry_hist = e_config_domain_load("module.everything.history", hist_edd);
   
   if (evry_hist && evry_hist->version != HISTORY_VERSION)
     {
	eina_hash_foreach(evry_hist->subjects, _hist_free_cb, NULL);
	eina_hash_foreach(evry_hist->actions,  _hist_free_cb, NULL);
	E_FREE(evry_hist);
	evry_hist = NULL;
     }
   
   if (!evry_hist)
     {
	evry_hist = E_NEW(History, 1);
	evry_hist->version = HISTORY_VERSION;
	
     }
   if (!evry_hist->subjects)
     evry_hist->subjects = eina_hash_string_superfast_new(NULL);
   if (!evry_hist->actions)
     evry_hist->actions  = eina_hash_string_superfast_new(NULL);
}


void
evry_history_unload(void)
{
   if (!evry_hist) return;
   
   e_config_domain_save("module.everything.history", hist_edd, evry_hist);

   eina_hash_foreach(evry_hist->subjects, _hist_free_cb, NULL);
   eina_hash_foreach(evry_hist->actions,  _hist_free_cb, NULL);
   
   E_FREE(evry_hist);
   evry_hist = NULL;
}

void
evry_history_add(Eina_Hash *hist, Evry_State *s)
{
   History_Entry *he;
   History_Item  *hi;
   Evry_Item *it;
   Eina_List *l;
   const char *id;

   if (!s) return;
   
   it = s->cur_item;
   if (!it) return;

   if (it->id)
     id = it->id;
   else
     id = it->label;
   
   he = eina_hash_find(hist, id);
   if (he)
     {
	/* found history entry */
	EINA_LIST_FOREACH(he->items, l, hi)
	  if (hi->plugin == it->plugin->name) break;

	if (hi)
	  {
	     /* found history item */
	     if (hi->input)
	       {
		  if (!s->input || !strncmp (hi->input, s->input, strlen(s->input)))
		    {
		       /* s->input matches hi->input and is equal or shorter */
		       hi->count++;
		       hi->last_used /= 1000.0;
		       hi->last_used += ecore_time_get();
		    }
		  else if (s->input)
		    {
		       if (!strncmp (hi->input, s->input, strlen(hi->input)))
			 {
			    /* s->input matches hi->input but is longer */
			    eina_stringshare_del(hi->input);
			    hi->input = eina_stringshare_add(s->input);
			 }
		       else
			 {
			    /* s->input is different from hi->input
			       -> create new item */
			    hi = NULL;
			 }
		    }
	       }
	     else
	       {
		  /* remember input for item */
		  hi->count++;
		  hi->last_used /= 2.0;
		  hi->last_used += ecore_time_get();

		  if (s->input)
		    hi->input = eina_stringshare_add(s->input);
	       }
	  }

	if (!hi)
	  {	     
	     hi = E_NEW(History_Item, 1);
	     hi->plugin = eina_stringshare_ref(it->plugin->name);
	     hi->last_used = ecore_time_get();
	     hi->count = 1;
	     if (s->input)
	       hi->input = eina_stringshare_add(s->input);

	     he->items = eina_list_append(he->items, hi);
	  }
     }
   else
     {
	he = E_NEW(History_Entry, 1);
	hi = E_NEW(History_Item, 1);
	hi->plugin = eina_stringshare_ref(it->plugin->name);
	hi->last_used = ecore_time_get();
	hi->count = 1;
	if (s->input)
	  hi->input = eina_stringshare_add(s->input);

	he->items = eina_list_append(he->items, hi);
	eina_hash_add(hist, id, he);
     }
}

int
evry_history_item_usage_set(Eina_Hash *hist, Evry_Item *it, const char *input)
{
   History_Entry *he;
   History_Item *hi;
   const char *id;
   Eina_List *l;
   int cnt = 1;
   
   if (it->id)
     id = it->id;
   else
     id = it->label;

   it->usage = 0;
   
   if ((he = eina_hash_find(hist, id)))
     {
	EINA_LIST_FOREACH(he->items, l, hi)
	  {
	     if ((hi->plugin == it->plugin->name) &&
		 ((!input[0]) || (!input[0] && !hi->input) ||
		  (!strncmp(input, hi->input, strlen(input))) ||
		  (!strncmp(input, hi->input, strlen(hi->input)))))
	       {
		  cnt++;
		  it->usage += hi->last_used;
	       }
	  }
	if (it->usage)
	  {
	     it->usage /= (double)cnt;
	     return 1;
	  }
     }
   
   return 0;
}

