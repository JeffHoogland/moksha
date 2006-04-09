/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void      _e_path_free(E_Path *ep);
static void      _e_path_cache_free(E_Path *ep);
static Evas_Bool _e_path_cache_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata);

/* externally accessible functions */
EAPI E_Path *
e_path_new(void)
{
   E_Path *ep;
   
   ep = E_OBJECT_ALLOC(E_Path, E_PATH_TYPE, _e_path_free);
   return ep;
}

EAPI void
e_path_default_path_append(E_Path *ep, const char *path)
{
   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   if (!path) return;
   if (path[0] == '~')
     {
        E_Path_Dir *epd;
	char *new_path;
	char *home_dir;
	int len1, len2;
	
	home_dir = e_user_homedir_get();
	if (!home_dir) return;
	len1 = strlen(home_dir);
	len2 = strlen(path);
	new_path = malloc(len1 + len2 + 1);
	if (!new_path)
	  {
	     free(home_dir);
	     return;
	  }
	epd = malloc(sizeof(E_Path_Dir));
        if (!epd)
	  {
	     free(home_dir);
	     free(new_path);
	     return;
	  }

	strcpy(new_path, home_dir);
	strcat(new_path, path + 1);
        epd->dir = evas_stringshare_add(new_path);
	free(new_path);
	free(home_dir);
	ep->default_dir_list = evas_list_append(ep->default_dir_list, epd);	
     }
   else
     {
	E_Path_Dir *epd;
	epd = malloc(sizeof(E_Path_Dir));
	if (!epd)
	  return;
	epd->dir = evas_stringshare_add(path);
	ep->default_dir_list = evas_list_append(ep->default_dir_list, epd);
     }
   _e_path_cache_free(ep);
}

EAPI void
e_path_user_path_set(E_Path *ep, Evas_List **user_dir_list)
{

   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   
   ep->user_dir_list = user_dir_list;
   _e_path_cache_free(ep);
}

EAPI void
e_path_user_path_append(E_Path *ep, const char *path)
{
   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   if (!path) return;
   if (path[0] == '~')
     {
        E_Path_Dir *epd;
	char *new_path;
	char *home_dir;
	int len1, len2;
	
	home_dir = e_user_homedir_get();
	if (!home_dir) return;
	len1 = strlen(home_dir);
	len2 = strlen(path);
	new_path = malloc(len1 + len2 + 1);
	if (!new_path)
	  {
	     free(home_dir);
	     return;
	  }
	epd = malloc(sizeof(E_Path_Dir));
        if (!epd)
	  {
	     free(home_dir);
	     free(new_path);
	     return;
	  }

	strcpy(new_path, home_dir);
	strcat(new_path, path + 1);
        epd->dir = evas_stringshare_add(new_path);
	free(new_path);
	free(home_dir);
	*(ep->user_dir_list) = evas_list_append(*(ep->user_dir_list), epd);	
     }
   else
     {
	E_Path_Dir *epd;
	epd = malloc(sizeof(E_Path_Dir));
	if (!epd)
	  return;
	epd->dir = evas_stringshare_add(path);
	*(ep->user_dir_list) = evas_list_append(*(ep->user_dir_list), epd);
     }
   _e_path_cache_free(ep);
}

EAPI void
e_path_user_path_prepend(E_Path *ep, const char *path)
{
   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   if (!path) return;
   if (path[0] == '~')
     {
        E_Path_Dir *epd;
	char *new_path;
	char *home_dir;
	int len1, len2;
	
	home_dir = e_user_homedir_get();
	if (!home_dir) return;
	len1 = strlen(home_dir);
	len2 = strlen(path);
	new_path = malloc(len1 + len2 + 1);
	if (!new_path)
	  {
	     free(home_dir);
	     return;
	  }
	epd = malloc(sizeof(E_Path_Dir));
        if (!epd)
	  {
	     free(home_dir);
	     free(new_path);
	     return;
	  }

	strcpy(new_path, home_dir);
	strcat(new_path, path + 1);
        epd->dir = evas_stringshare_add(new_path);
	free(new_path);
	free(home_dir);
	*(ep->user_dir_list) = evas_list_prepend(*(ep->user_dir_list), epd);	
     }
   else
     {
	E_Path_Dir *epd;
	epd = malloc(sizeof(E_Path_Dir));
	if (!epd)
	  return;
	epd->dir = evas_stringshare_add(path);
	*(ep->user_dir_list) = evas_list_prepend(*(ep->user_dir_list), epd);
     }
   _e_path_cache_free(ep);
}

EAPI void
e_path_user_path_remove(E_Path *ep, const char *path)
{
   Evas_List *l;

   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   if (!path) return;
   if (path[0] == '~')
     {
	char *new_path;
	char *home_dir;
	int len1, len2;
	
	home_dir = e_user_homedir_get();
	if (!home_dir) return;
	len1 = strlen(home_dir);
	len2 = strlen(path);
	new_path = malloc(len1 + len2 + 1);
	if (!new_path)
	  {
	     free(home_dir);
	     return;
	  }
	strcpy(new_path, home_dir);
	strcat(new_path, path + 1);
	free(home_dir);
	for (l = *(ep->user_dir_list); l; l = l->next)
	  {
	     E_Path_Dir *epd;
	     
	     epd = l->data;
	     if (epd->dir)
	       {
		  if (!strcmp(epd->dir, new_path))
		    {
		       *(ep->user_dir_list) = evas_list_remove_list(
						*(ep->user_dir_list), l);
		       evas_stringshare_del(epd->dir);
		       free(epd);
		       free(new_path);
		       _e_path_cache_free(ep);
		       return;
		    }
	       }
	  }
	free(new_path);
      }
   else
     {
	for (l = *(ep->user_dir_list); l; l = l->next)
	  {
	     E_Path_Dir *epd;
	     epd = l->data;
	     if (epd->dir)
	       {
		  if (!strcmp(epd->dir, path))
		    {
		       *(ep->user_dir_list) = evas_list_remove_list(
						*(ep->user_dir_list), l);
		       evas_stringshare_del(epd->dir);
		       free(epd);
		       _e_path_cache_free(ep);
		       return;
		    }
	       }
	  }
     }
}

EAPI void
e_path_user_path_clear(E_Path *ep)
{	
   while (*(ep->user_dir_list))
     {
	E_Path_Dir *epd;
	epd = (*(ep->user_dir_list))->data;
	*(ep->user_dir_list) = evas_list_remove_list(*(ep->user_dir_list), *(ep->user_dir_list));
	evas_stringshare_del(epd->dir);
	free(epd);     
     }
   _e_path_cache_free(ep);
}

EAPI const char *
e_path_find(E_Path *ep, const char *file)
{
   Evas_List *l;
   char *str;
   char buf[PATH_MAX] = "";
   
   E_OBJECT_CHECK_RETURN(ep, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(ep, E_PATH_TYPE, NULL);

   if (!file) return NULL;
   str = evas_hash_find(ep->hash, file);
   if (str) return evas_stringshare_add(str);
   /* Look in the default dir list */
   for (l = ep->default_dir_list; l; l = l->next)
     {
	E_Path_Dir *epd;
	char *rp;
	
	epd = l->data;
	if (epd->dir)
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", epd->dir, file);
	     rp = ecore_file_realpath(buf);
	     if ((rp) && (rp[0] != 0))
	       {
		  strncpy(buf, rp, sizeof(buf) - 1);
		  buf[sizeof(buf) - 1] = 0;
		  free(rp);
		  if (evas_hash_size(ep->hash) >= 512)
		    _e_path_cache_free(ep);
		  ep->hash = evas_hash_add(ep->hash, file,
					   evas_stringshare_add(buf));
		  return evas_stringshare_add(buf);
	       }
	     if (rp) free(rp);
	  }
     }
   /* Look in the users dir list */
   for (l = *(ep->user_dir_list); l; l = l->next)
     {
	E_Path_Dir *epd;
	char *rp;
	
	epd = l->data;
	if (epd->dir)
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", epd->dir, file);
	     rp = ecore_file_realpath(buf);
	     if ((rp) && (rp[0] != 0))
	       {
		  strncpy(buf, rp, sizeof(buf) - 1);
		  buf[sizeof(buf) - 1] = 0;
		  free(rp);
		  if (evas_hash_size(ep->hash) >= 512)
		    _e_path_cache_free(ep);
		  ep->hash = evas_hash_add(ep->hash, file, 
					   evas_stringshare_add(buf));
		  return evas_stringshare_add(buf);
	       }
	     if (rp) free(rp);
	  }
     }
   return NULL;
}

EAPI void
e_path_evas_append(E_Path *ep, Evas *evas)
{
   Evas_List *l;
   Evas_List *dir_list;
   
   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   if (!evas) return;

   dir_list = e_path_dir_list_get(ep);

   for (l = dir_list; l; l = l->next)
     {
	E_Path_Dir *epd;
	
	epd = l->data;
	if (epd->dir) evas_font_path_append(evas, epd->dir);
	evas_stringshare_del(epd->dir);
	free(epd);
     }
   if (dir_list) evas_list_free(dir_list);
}

/* combine default_list and and user_list in and easy to use list */
EAPI Evas_List *
e_path_dir_list_get(E_Path *ep)
{
   Evas_List	*dir_list;
   Evas_List	*l;
   E_Path_Dir	*new_epd;
   E_Path_Dir	*epd;

   dir_list = NULL;

   if (ep->user_dir_list)
     {
	for (l = *(ep->user_dir_list); l; l = l->next)
	  {
	     epd = l->data;
	     new_epd = malloc(sizeof(E_Path_Dir));
	     new_epd->dir = evas_stringshare_add(epd->dir);
	     dir_list = evas_list_append(dir_list, new_epd);
	  }
     }

   for (l = ep->default_dir_list; l; l = l->next)
     {
	epd = l->data;
	new_epd = malloc(sizeof(E_Path_Dir));
	new_epd->dir = evas_stringshare_add(epd->dir);
	dir_list = evas_list_append(dir_list, new_epd);
     }

   return dir_list;
}

EAPI void
e_path_dir_list_free(Evas_List *dir_list)
{
   E_Path_Dir *epd;

   while (dir_list)
     {
	epd = dir_list->data;
	dir_list = evas_list_remove_list(dir_list, dir_list);
	evas_stringshare_del(epd->dir);
	free(epd);
     }
}

/* local subsystem functions */
static void 
_e_path_free(E_Path *ep)
{
   _e_path_cache_free(ep);
   while (ep->default_dir_list)
     {
	E_Path_Dir *epd;
	epd = ep->default_dir_list->data;
	evas_stringshare_del(epd->dir);
	free(epd);
	ep->default_dir_list = evas_list_remove_list(ep->default_dir_list, 
						     ep->default_dir_list);
     }
   free(ep);
}

static void
_e_path_cache_free(E_Path *ep)
{
   if (!ep->hash) return;
   evas_hash_foreach(ep->hash, _e_path_cache_free_cb, NULL);
   evas_hash_free(ep->hash);
   ep->hash = NULL;
}

static Evas_Bool
_e_path_cache_free_cb(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   evas_stringshare_del(data);
   return 1;
}
