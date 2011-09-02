#include "e_mod_main.h"

#define HISTORY_VERSION 2

#define SEVEN_DAYS 604800.0

/* old history entries will be removed when threshold is reached */
#define CLEANUP_THRESHOLD 500

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
static E_Config_DD *hist_types_edd = NULL;
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
   E_CONFIG_VAL(D, T, data,      STR);
#undef T
#undef D
   hist_entry_edd = E_CONFIG_DD_NEW("History_Entry", History_Entry);
#define T History_Entry
#define D hist_entry_edd
   E_CONFIG_LIST(D, T, items, hist_item_edd);
#undef T
#undef D
   hist_types_edd = E_CONFIG_DD_NEW("History_Types", History_Types);
#define T History_Types
#define D hist_types_edd
   E_CONFIG_HASH(D, T, types, hist_entry_edd);
#undef T
#undef D
   hist_edd = E_CONFIG_DD_NEW("History", Evry_History);
#define T Evry_History
#define D hist_edd
   E_CONFIG_VAL(D, T,  version,  INT);
   E_CONFIG_VAL(D, T,  begin,    DOUBLE);
   E_CONFIG_HASH(D, T, subjects,  hist_types_edd);
#undef T
#undef D
}

static Eina_Bool
_hist_entry_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
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
	if (hi->data)
	  eina_stringshare_del(hi->data);
	E_FREE(hi);
     }

   E_FREE(he);

   return EINA_TRUE;
}

static Eina_Bool
_hist_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   History_Types *ht = data;

   if (ht->types)
     {
	eina_hash_foreach(ht->types, _hist_entry_free_cb, NULL);
	eina_hash_free(ht->types);
     }

   E_FREE(ht);

   return EINA_TRUE;
}

static Eina_Bool
_hist_entry_cleanup_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   Cleanup_Data *d = fdata;
   History_Item *hi;
   Eina_List *l, *ll;

   EINA_LIST_FOREACH_SAFE(he->items, l, ll, hi)
     {
	if (hi->last_used < d->time - SEVEN_DAYS)
	  {
	     hi->count--;
	     hi->last_used = d->time - SEVEN_DAYS/2.0;
	  }

	/* item is transient or too old */
	if ((hi->count < 1) || hi->transient)
	  {
	     if (hi->input)
	       eina_stringshare_del(hi->input);
	     if (hi->plugin)
	       eina_stringshare_del(hi->plugin);
	     if (hi->context)
	       eina_stringshare_del(hi->context);
	     if (hi->data)
	       eina_stringshare_del(hi->data);
	     E_FREE(hi);

	     he->items = eina_list_remove_list(he->items, l);
	  }
     }

   if (!he->items)
     {
	E_FREE(he);
	d->keys = eina_list_append(d->keys, key);
     }

   return EINA_TRUE;
}

static Eina_Bool
_hist_cleanup_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
{
   History_Types *ht = data;
   Cleanup_Data *d = fdata;

   if (ht->types)
     {
	eina_hash_foreach(ht->types, _hist_entry_cleanup_cb, fdata);

	EINA_LIST_FREE(d->keys, key)
	  eina_hash_del_by_key(ht->types, key);
     }

   return EINA_TRUE;
}
void
evry_history_free(void)
{
   evry_hist = e_config_domain_load("module.everything.cache", hist_edd);

   if ((evry_hist) && (evry_hist->subjects) &&
       (eina_hash_population(evry_hist->subjects) > CLEANUP_THRESHOLD))
     {
	Cleanup_Data *d;

	d = E_NEW(Cleanup_Data, 1);
	d->time = ecore_time_unix_get();
	eina_hash_foreach(evry_hist->subjects, _hist_cleanup_cb, d);
	E_FREE(d);
     }

   evry_history_unload();

   E_CONFIG_DD_FREE(hist_item_edd);
   E_CONFIG_DD_FREE(hist_entry_edd);
   E_CONFIG_DD_FREE(hist_types_edd);
   E_CONFIG_DD_FREE(hist_edd);
}

void
evry_history_load(void)
{
   if (evry_hist) return;

   evry_hist = e_config_domain_load("module.everything.cache", hist_edd);

   if (evry_hist && evry_hist->version != HISTORY_VERSION)
     {
	eina_hash_foreach(evry_hist->subjects, _hist_free_cb, NULL);
	eina_hash_free(evry_hist->subjects);

	E_FREE(evry_hist);
	evry_hist = NULL;
     }

   if (!evry_hist)
     {
	evry_hist = E_NEW(Evry_History, 1);
	evry_hist->version = HISTORY_VERSION;
	evry_hist->begin = ecore_time_unix_get() - SEVEN_DAYS;
     }
   if (!evry_hist->subjects)
     evry_hist->subjects = eina_hash_string_superfast_new(NULL);
}


void
evry_history_unload(void)
{
   if (!evry_hist) return;

   e_config_domain_save("module.everything.cache", hist_edd, evry_hist);

   eina_hash_foreach(evry_hist->subjects, _hist_free_cb, NULL);
   eina_hash_free(evry_hist->subjects);

   E_FREE(evry_hist);
   evry_hist = NULL;
}

History_Types *
evry_history_types_get(Evry_Type _type)
{
   History_Types *ht;
   const char *type = evry_type_get(_type);
   
   if (!evry_hist)
     return NULL;
   
   if (!type)
     return NULL;

   ht = eina_hash_find(evry_hist->subjects, type);

   if (!ht)
     {
	ht = E_NEW(History_Types, 1);
	eina_hash_add(evry_hist->subjects, type, ht);
     }

   if (!ht->types)
     ht->types = eina_hash_string_superfast_new(NULL);

   return ht;
}

History_Item *
evry_history_item_add(Evry_Item *it, const char *ctxt, const char *input)
{
   History_Entry *he;
   History_Types *ht;
   History_Item  *hi = NULL;
   Eina_List *l;
   int rem_ctxt = 1;
   const char *id;

   if (!evry_hist)
     return NULL;

   if (!it)
     return NULL;

   if ((!it->plugin->history) && (!CHECK_TYPE(it, EVRY_TYPE_PLUGIN)))
       return NULL;

   if (it->type == EVRY_TYPE_ACTION)
     {
	GET_ACTION(act, it);
	if (!act->remember_context)
	  rem_ctxt = 0;
     }

   if (it->hi)
     {
	/* keep hi when context didn't change */
	if ((!rem_ctxt) || (!it->hi->context && !ctxt) ||
	    (it->hi->context && ctxt && !strcmp(it->hi->context, ctxt)))
	  hi = it->hi;
     }

   if (!hi)
     {
	id = (it->id ? it->id : it->label);
	ht = evry_history_types_get(it->type);
	if (!ht)
	  return NULL;
	
	he = eina_hash_find(ht->types, id);

	if (!he)
	  {
	     he = E_NEW(History_Entry, 1);
	     eina_hash_add(ht->types, id, he);
	  }
	else
	  {
	     EINA_LIST_FOREACH(he->items, l, hi)
	       if ((hi->plugin == it->plugin->name) &&
		   (!rem_ctxt || (ctxt == hi->context)))
		 break;
	  }
     }

   if (!hi)
     {
	hi = E_NEW(History_Item, 1);
	hi->plugin = eina_stringshare_ref(it->plugin->name);
	he->items = eina_list_append(he->items, hi);
     }

   if (hi)
     {
	it->hi = hi;

	hi->last_used = ecore_time_unix_get();
	hi->usage /= 4.0;
	hi->usage += TIME_FACTOR(hi->last_used);
	hi->transient = it->plugin->transient;
	hi->count += 1;

	if (ctxt && !hi->context && rem_ctxt)
	  {
	     hi->context = eina_stringshare_ref(ctxt);
	  }

	if (input && hi->input)
	  {
	     if (strncmp(hi->input, input, strlen(input)))
	       {
		  eina_stringshare_del(hi->input);
		  hi->input = eina_stringshare_add(input);
	       }
	  }
	else if (input)
	  {
	     hi->input = eina_stringshare_add(input);
	  }
     }

   /* reset usage */
   it->usage = 0.0;

   return hi;
}

int
evry_history_item_usage_set(Evry_Item *it, const char *input, const char *ctxt)
{
   History_Entry *he;
   History_Types *ht;
   History_Item *hi = NULL;
   Eina_List *l;
   int rem_ctxt = 1;

   if (evry_conf->history_sort_mode == 3)
     {	
	it->usage = -1;
	return 1;
     }
   else
     it->usage = 0.0;

   if ((!it->plugin->history) && (!CHECK_TYPE(it, EVRY_TYPE_PLUGIN)))
     return 0;

   if (it->hi)
     {
	/* keep hi when context didn't change */
	if ((!rem_ctxt) || (!it->hi->context && !ctxt) ||
	    (it->hi->context && ctxt && !strcmp(it->hi->context, ctxt)))
	  hi = it->hi;
     }

   if (!hi)
     {
	ht = evry_history_types_get(it->type);

	if (!ht)
	  return 0;
	
	if (!(he = eina_hash_find(ht->types, (it->id ? it->id : it->label))))
	  return 0;

	if (it->type == EVRY_TYPE_ACTION)
	  {
	     GET_ACTION(act, it);
	     if (!act->remember_context)
	       rem_ctxt = 0;
	  }

	EINA_LIST_FOREACH(he->items, l, hi)
	  {
	     if (hi->plugin != it->plugin->name)
	       continue;

	     if (rem_ctxt && ctxt && (hi->context != ctxt))
	       {
		 it->hi = hi;
		  continue;
	       }

	     it->hi = hi;
	     break;
	  }
     }

   if (!hi) return 0;

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

	if (ctxt && hi->context && (hi->context == ctxt))
	  {
	     it->usage += hi->usage * hi->count * 10.0;
	  }
     }
   else if (evry_conf->history_sort_mode == 1)
     {
	it->usage = hi->count * (hi->last_used / 10000000000.0);

	if (ctxt && hi->context && (hi->context == ctxt))
	  {
	     it->usage += hi->usage * hi->count * 10.0;
	  }
     }
   else if (evry_conf->history_sort_mode == 2)
     {
	if (hi->last_used > it->usage)
	  it->usage = hi->last_used;
     }
   if (it->fuzzy_match > 0)
     it->usage /= (double) it->fuzzy_match;
   else
     it->usage /= 100.0;
   
   if (it->usage > 0.0)
     return 1;

   it->usage = -1.0;

   return 0;
}
