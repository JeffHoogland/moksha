#include "e_mod_main.h"

#define HISTORY_VERSION 8

#define SEVEN_DAYS 604800

#define TIME_FACTOR(_now) (1.0 - (evry_hist->begin / _now)) / 1000000000000000.0

typedef struct _Cleanup_Data Cleanup_Data;

struct _Cleanup_Data
{
  double time;
  Eina_List *keys;
  Eina_Bool normalize;
  const char *plugin;
};

static E_Config_DD *hist_entry_edd = NULL;
static E_Config_DD *hist_item_edd = NULL;
static E_Config_DD *hist_edd = NULL;

Evry_History *evry_hist = NULL;

void
evry_history_init(void)
{
#undef T
#undef D
   hist_item_edd = E_CONFIG_DD_NEW("History_Item", History_Item);
#define T History_Item
#define D hist_item_edd
   E_CONFIG_VAL(D, T, plugin,    STR);
   E_CONFIG_VAL(D, T, context,   STR);
   E_CONFIG_VAL(D, T, input,     STR);
   E_CONFIG_VAL(D, T, last_used, DOUBLE);
   E_CONFIG_VAL(D, T, usage,     DOUBLE);
   E_CONFIG_VAL(D, T, count,     INT);
   E_CONFIG_VAL(D, T, transient, INT);
#undef T
#undef D
   hist_entry_edd = E_CONFIG_DD_NEW("History_Entry", History_Entry);
#define T History_Entry
#define D hist_entry_edd
   E_CONFIG_LIST(D, T, items, hist_item_edd);
#undef T
#undef D
   hist_edd = E_CONFIG_DD_NEW("History", Evry_History);
#define T Evry_History
#define D hist_edd
   E_CONFIG_VAL(D, T,  version,  INT);
   E_CONFIG_VAL(D, T,  begin,    DOUBLE);
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

static Eina_Bool
_hist_cleanup_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   Cleanup_Data *d = fdata;
   History_Item *hi;
   Eina_List *l, *ll;

   EINA_LIST_FOREACH_SAFE(he->items, l, ll, hi)
     {
	/* item for this plugi nneed to be removed, e.g. on updates */
	if (d->plugin)
	  {
	     if (hi->plugin == d->plugin)
	       hi->transient = 1;
	  }

	if (hi->last_used < d->time - SEVEN_DAYS)
	  {
	     hi->count--;
	     hi->last_used = d->time - SEVEN_DAYS/2;
	  }
	
	/* item is transient or too old */
	if (!hi->count || hi->transient)
	  {
	     if (hi->input)
	       eina_stringshare_del(hi->input);
	     if (hi->plugin)
	       eina_stringshare_del(hi->plugin);
	     if (hi->context)
	       eina_stringshare_del(hi->context);
	     E_FREE(hi);

	     he->items = eina_list_remove_list(he->items, l);
	  }
     }

   if (!he->items)
     {
	E_FREE(he);
	d->keys = eina_list_append(d->keys, key);
     }

   return 1;
}

void
evry_history_free(void)
{
   Cleanup_Data *d;
   char *key;

   evry_hist = e_config_domain_load("module.everything.history", hist_edd);
   if (evry_hist)
     {
	d = E_NEW(Cleanup_Data, 1);
	d->time = ecore_time_get();

	if (evry_hist->subjects)
	  {
	     eina_hash_foreach(evry_hist->subjects, _hist_cleanup_cb, d);
	     EINA_LIST_FREE(d->keys, key)
	       eina_hash_del_by_key(evry_hist->subjects, key);
	  }

	if (evry_hist->actions)
	  {
	     eina_hash_foreach(evry_hist->actions, _hist_cleanup_cb, d);
	     EINA_LIST_FREE(d->keys, key)
	       eina_hash_del_by_key(evry_hist->actions, key);
	  }

	E_FREE(d);
	evry_history_unload();
     }

   E_CONFIG_DD_FREE(hist_item_edd);
   E_CONFIG_DD_FREE(hist_entry_edd);
   E_CONFIG_DD_FREE(hist_edd);
}

EAPI void
evry_history_load(void)
{
   evry_hist = e_config_domain_load("module.everything.history", hist_edd);
     
   if (evry_hist && evry_hist->version != HISTORY_VERSION)
     {
	eina_hash_foreach(evry_hist->subjects, _hist_free_cb, NULL);
	eina_hash_foreach(evry_hist->actions,  _hist_free_cb, NULL);
	eina_hash_free(evry_hist->subjects);
	eina_hash_free(evry_hist->actions);

	E_FREE(evry_hist);
	evry_hist = NULL;
     }

   if (!evry_hist)
     {
	evry_hist = E_NEW(Evry_History, 1);
	evry_hist->version = HISTORY_VERSION;
	evry_hist->begin = ecore_time_get();
     }
   if (!evry_hist->subjects)
     evry_hist->subjects = eina_hash_string_superfast_new(NULL);
   if (!evry_hist->actions)
     evry_hist->actions  = eina_hash_string_superfast_new(NULL);
}


EAPI void
evry_history_unload(void)
{
   if (!evry_hist) return;

   e_config_domain_save("module.everything.history", hist_edd, evry_hist);

   eina_hash_foreach(evry_hist->subjects, _hist_free_cb, NULL);
   eina_hash_foreach(evry_hist->actions,  _hist_free_cb, NULL);
   eina_hash_free(evry_hist->subjects);
   eina_hash_free(evry_hist->actions);
   
   E_FREE(evry_hist);
   evry_hist = NULL;
}

EAPI void
evry_history_add(Eina_Hash *hist, Evry_State *s, const char *ctxt)
{
   History_Entry *he;
   History_Item  *hi = NULL;
   Evry_Item *it;
   Eina_List *l;
   const char *id;
   
   if (!s) return;

   it = s->cur_item;
   if (!it || !it->plugin->history) return;

   id = (it->id ? it->id : it->label);

   he = eina_hash_find(hist, id);
   if (!he)
     {
	he = E_NEW(History_Entry, 1);
	eina_hash_add(hist, id, he);
     }
   else
     {
	EINA_LIST_FOREACH(he->items, l, hi)
	  if ((hi->plugin == it->plugin->name) &&
	      (ctxt == hi->context))
	    break;
     }

   if (!hi)
     {
	hi = E_NEW(History_Item, 1);
	hi->plugin = eina_stringshare_ref(it->plugin->name);
	he->items = eina_list_append(he->items, hi);
     }

   if (hi)
     {
	hi->last_used = ecore_time_get();
	hi->usage /= 4.0;
	hi->usage += TIME_FACTOR(hi->last_used);
	hi->transient = it->plugin->transient;
	hi->count += (hi->transient ? 2:1);
	if (ctxt && !hi->context)
	  hi->context = eina_stringshare_ref(ctxt);

	if (s->input)
	  {

	     if (hi->input)
	       eina_stringshare_del(hi->input);

	     hi->input = eina_stringshare_add(s->input);
	  }
     }   
}

EAPI int
evry_history_item_usage_set(Eina_Hash *hist, Evry_Item *it, const char *input, const char *ctxt)
{
   History_Entry *he;
   History_Item *hi;
   Eina_List *l;

   if (!it->plugin->history)
     return 0;
   
   it->usage = 0.0;
   if (!(he = eina_hash_find(hist, (it->id ? it->id : it->label))))
     return 0;
   
   EINA_LIST_FOREACH(he->items, l, hi)
     {
	if (hi->plugin != it->plugin->name)
	  continue;

	if (ctxt != hi->context)
	  continue;

	if (it->plugin->type == type_action)
	  {
	     if (hi->last_used > it->usage)
	       it->usage = hi->last_used;
	  }
	
	if (evry_conf->history_sort_mode == 0)
	  {
	     
	     if (!input || !hi->input)
	       {
		  it->usage += hi->usage * hi->count;
	       }
	     else
	       {
		  /* higher priority for exact matches */
		  if (!strncmp(input, hi->input, strlen(input)))
		    {
		       it->usage += hi->usage * hi->count;
		    }
		  if (!strncmp(input, hi->input, strlen(hi->input)))
		    {
		       it->usage += hi->usage * hi->count;
		    }
	       }

	     if (ctxt && hi->context &&
		 (hi->context == ctxt))
	       it->usage += hi->usage * hi->count * 10;
	  }
	else if (evry_conf->history_sort_mode == 1)
	  {
	     it->usage = hi->count * (hi->last_used / 10000000000.0);
	     
	  }
	else if (evry_conf->history_sort_mode == 2)
	  {
	     if (hi->last_used > it->usage)
	       it->usage = hi->last_used;
	  }
     }
   
   if (it->usage > 0.0)
     return 1;

   return 0;
}

