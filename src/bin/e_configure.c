#include "e.h"

Evas_List *e_configure_registry = NULL;

EAPI void
e_configure_init(void)
{
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL, "enlightenment/extensions");
   e_configure_registry_item_add("extensions/modules", 10, _("Modules"), NULL, "enlightenment/modules", e_int_config_modules);
}

EAPI void
e_configure_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params))
{
   Evas_List *l;
   char *cat;
   const char *item;
   E_Configure_It *eci;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   eci = E_NEW(E_Configure_It, 1);
   if (!eci) goto done;
   
   eci->item = evas_stringshare_add(item);
   eci->pri = pri;
   eci->label = evas_stringshare_add(label);
   if (icon_file) eci->icon_file = evas_stringshare_add(icon_file);
   if (icon) eci->icon = evas_stringshare_add(icon);
   eci->func = func;
   
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;
	     
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci2;
		  
		  eci2 = ll->data;
		  if (eci2->pri > eci->pri)
		    {
		       ecat->items = evas_list_prepend_relative_list(ecat->items, eci, ll);
		       goto done;
		    }
	       }
	     ecat->items = evas_list_append(ecat->items, eci);
	     goto done;
	  }
     }
   done:
   free(cat);
}

EAPI void
e_configure_registry_item_del(const char *path)
{
   Evas_List *l;
   char *cat;
   const char *item;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;
	     
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  
		  eci = ll->data;
		  if (!strcmp(item, eci->item))
		    {
		       ecat->items = evas_list_remove_list(ecat->items, ll);
		       evas_stringshare_del(eci->item);
		       evas_stringshare_del(eci->label);
		       evas_stringshare_del(eci->icon);
		       free(eci);
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
   free(cat);
}

EAPI void
e_configure_registry_category_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon)
{
   E_Configure_Cat *ecat;
   Evas_List *l;
   
   ecat = E_NEW(E_Configure_Cat, 1);
   if (!ecat) return;
   
   ecat->cat = evas_stringshare_add(path);
   ecat->pri = pri;
   ecat->label = evas_stringshare_add(label);
   if (icon_file) ecat->icon_file = evas_stringshare_add(icon_file);
   if (icon) ecat->icon = evas_stringshare_add(icon);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat2;
	
	ecat2 = l->data;
	if (ecat2->pri > ecat->pri)
	  {
	     e_configure_registry = evas_list_prepend_relative_list(e_configure_registry, ecat, l);
	     return;
	  }
     }
   e_configure_registry = evas_list_append(e_configure_registry, ecat);
}

EAPI void
e_configure_registry_category_del(const char *path)
{
   Evas_List *l;
   char *cat;
   
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
        if (!strcmp(cat, ecat->cat))
	  {
	     if (ecat->items) goto done;
	     e_configure_registry = evas_list_remove_list(e_configure_registry, l);
	     evas_stringshare_del(ecat->cat);
	     evas_stringshare_del(ecat->label);
	     if (ecat->icon) evas_stringshare_del(ecat->icon);
	     free(ecat);
	     goto done;
	  }
     }
   done:
   free(cat);
}

EAPI void
e_configure_registry_call(const char *path, E_Container *con, const char *params)
{
   Evas_List *l;
   char *cat;
   const char *item;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;
	     
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  
		  eci = ll->data;
		  if (!strcmp(item, eci->item))
		    {
		       if (eci->func) eci->func(con, params);
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
   free(cat);
}

EAPI int
e_configure_registry_exists(const char *path)
{
   Evas_List *l;
   char *cat;
   const char *item;
   int ret = 0;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return 0;
   item = ecore_file_file_get(path);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;

	     if (!item)
	       {
		  ret = 1;
		  goto done;
	       }
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  
		  eci = ll->data;
		  if (!strcmp(item, eci->item))
		    {
		       ret = 1;
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
   free(cat);
   return ret;
}
