/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * Implementation of a protected file registry. Any files that are
 * currently being used by E in core components should be registered
 * here and will be protected as best as E can. :)
 */

/* Local subsystem globals */
static Evas_List *_e_filereg = NULL;

typedef struct _Filereg_Item Filereg_Item;

struct _Filereg_Item
{
   char * path;
   int ref_count;
};

/* Externally accessible functions */
EAPI int
e_filereg_init(void)
{
   return 1;
}

EAPI int
e_filereg_shutdown(void)
{
   Evas_List * ll;
   Filereg_Item * item;

   /* Delete all strings in the hash */
   for (ll = _e_filereg; ll; ll = ll->next)
     {
	item = ll->data;	
	E_FREE(item->path);
	E_FREE(item);
     }

   _e_filereg = evas_list_free(_e_filereg);
   return 1;
}

EAPI int
e_filereg_register(const char * path)
{
   Evas_List * ll;
   Filereg_Item * item;

   for (ll = _e_filereg; ll; ll = ll->next)
     {
	item = ll->data;	
	if (!strcmp(item->path, path))
	  {
	     /* File already registered, increment ref. count */
	     item->ref_count++;
	     return 1;
	  }
     }

   /* Doesn't exist so add to list. */
   item = E_NEW(Filereg_Item, 1);
   if (!item) return 0;

   item->path = strdup(path);
   item->ref_count = 1;
   _e_filereg = evas_list_append(_e_filereg, item);

   return 1;
}

EAPI void
e_filereg_deregister(const char * path)
{
   Evas_List * ll;
   Filereg_Item * item;

   for (ll = _e_filereg; ll; ll = ll->next)
     {
	item = ll->data;	
	if (!strcmp(item->path, path))
	  {
	     item->ref_count--;
	     if (item->ref_count == 0) 
	       {
		_e_filereg = evas_list_remove_list(_e_filereg, ll);	
		E_FREE(item->path);
		E_FREE(item);
	       }

	     return;
	  }
     }
}

EAPI Evas_Bool
e_filereg_file_protected(const char * path)
{
   Evas_List * ll;
   Filereg_Item * item;

   for (ll = _e_filereg; ll; ll = ll->next)
     {
	item = ll->data;	
	if (!strcmp(item->path, path))
	     return 1;
     }

   return 0;
}

