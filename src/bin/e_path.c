/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void      _e_path_free(E_Path *ep);
static void      _e_path_cache_free(E_Path *ep);
static Evas_Bool _e_path_cache_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata);

/* local subsystem globals */
static char _e_path_buf[PATH_MAX] = "";

/* externally accessible functions */
E_Path *
e_path_new(void)
{
   E_Path *ep;
   
   ep = E_OBJECT_ALLOC(E_Path, E_PATH_TYPE, _e_path_free);
   return ep;
}

void
e_path_path_append(E_Path *ep, const char *path)
{
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
	ep->dir_list = evas_list_append(ep->dir_list, new_path);	
     }
   else
     ep->dir_list = evas_list_append(ep->dir_list, strdup(path));
   _e_path_cache_free(ep);
}

void
e_path_path_prepend(E_Path *ep, const char *path)
{
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
	ep->dir_list = evas_list_prepend(ep->dir_list, new_path);	
     }
   else
     ep->dir_list = evas_list_prepend(ep->dir_list, strdup(path));
   _e_path_cache_free(ep);
}

void
e_path_path_remove(E_Path *ep, const char *path)
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
	for (l = ep->dir_list; l; l = l->next)
	  {
	     char *p;
	     
	     p = l->data;
	     if (p)
	       {
		  if (!strcmp(p, new_path))
		    {
		       ep->dir_list = evas_list_prepend(ep->dir_list, l->data);
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
	for (l = ep->dir_list; l; l = l->next)
	  {
	     char *p;
	     
	     p = l->data;
	     if (p)
	       {
		  if (!strcmp(p, path))
		    {
		       ep->dir_list = evas_list_prepend(ep->dir_list, l->data);
		       _e_path_cache_free(ep);
		       return;
		    }
	       }
	  }
     }
}

char *
e_path_find(E_Path *ep, const char *file)
{
   Evas_List *l;
   char *str;
   
   E_OBJECT_CHECK_RETURN(ep, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(ep, E_PATH_TYPE, NULL);
   if (!file) return NULL;
   _e_path_buf[0] = 0;
   str = evas_hash_find(ep->hash, file);
   if (str)
     {
	strcpy(_e_path_buf, str);
	return _e_path_buf;
     }
   for (l = ep->dir_list; l; l = l->next)
     {
	char *p, *rp;
	
	p = l->data;
	if (p)
	  {
	     snprintf(_e_path_buf, sizeof(_e_path_buf), "%s/%s", p, file);
	     rp = ecore_file_realpath(_e_path_buf);
	     if ((rp) && (rp[0] != 0))
	       {
		  strcpy(_e_path_buf, rp);
		  free(rp);
		  if (evas_hash_size(ep->hash) >= 512)
		    _e_path_cache_free(ep);
		  ep->hash = evas_hash_add(ep->hash, file, strdup(_e_path_buf));
		  return _e_path_buf;
	       }
	     if (rp) free(rp);
	  }
     }
   return _e_path_buf;
}

void
e_path_evas_append(E_Path *ep, Evas *evas)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(ep);
   E_OBJECT_TYPE_CHECK(ep, E_PATH_TYPE);
   if (!evas) return;
   for (l = ep->dir_list; l; l = l->next)
     {
	char *p;
	
	p = l->data;
	if (p) evas_font_path_append(evas, p);
     }
}

/* local subsystem functions */
static void 
_e_path_free(E_Path *ep)
{
   _e_path_cache_free(ep);
   while (ep->dir_list)
     {
	free(ep->dir_list->data);
	ep->dir_list = evas_list_remove_list(ep->dir_list, ep->dir_list);
     }
   free(ep);
}

static void
_e_path_cache_free(E_Path *ep)
{
   if (!ep->hash) return;
   evas_hash_foreach(ep->hash, _e_path_cache_free_cb, NULL);
   evas_hash_free(ep->hash);
}

static Evas_Bool
_e_path_cache_free_cb(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   free(data);
   return 0;
}
