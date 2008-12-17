/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * Implementation of a protected file registry. Any files that are
 * currently being used by E in core components should be registered
 * here and will be protected as best as E can. :)
 */
static Eina_Hash *_e_filereg = NULL;

typedef struct _Filereg_Item Filereg_Item;
struct _Filereg_Item
{
   const char *path;
   int ref_count;
};

static Eina_Bool _filereg_hash_cb_free(const Eina_Hash *hash __UNUSED__,
				       const void *key __UNUSED__,
				       void *data, void *fdata __UNUSED__);

/* Externally accessible functions */
EAPI int
e_filereg_init(void)
{
   _e_filereg = eina_hash_string_superfast_new(NULL);

   return 1;
}

EAPI int
e_filereg_shutdown(void)
{
   eina_hash_foreach(_e_filereg, _filereg_hash_cb_free, NULL);
   eina_hash_free(_e_filereg);
   _e_filereg = NULL;
   return 1;
}

EAPI int
e_filereg_register(const char *path)
{
   Filereg_Item *fi = NULL;

   fi = eina_hash_find(_e_filereg, path);
   if (fi)
     {
	fi->ref_count++;
	return 1;
     }
   fi = E_NEW(Filereg_Item, 1);
   if (!fi) return 0;
   fi->path = eina_stringshare_add(path);
   fi->ref_count = 1;
   eina_hash_add(_e_filereg, path, fi);
   return 1;
}

EAPI void
e_filereg_deregister(const char *path)
{
   Filereg_Item *fi = NULL;

   fi = eina_hash_find(_e_filereg, path);
   if (fi)
     {
	fi->ref_count--;
	if (fi->ref_count == 0)
	  {
	     eina_hash_del(_e_filereg, path, fi);
	     if (fi->path) eina_stringshare_del(fi->path);
	     E_FREE(fi);
	  }
     }
}

EAPI Evas_Bool
e_filereg_file_protected(const char *path)
{
   Filereg_Item *fi = NULL;

   fi = eina_hash_find(_e_filereg, path);
   if (!fi) return 0;
   else return 1;
}

/* Private Functions */
static Evas_Bool
_filereg_hash_cb_free(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__,
		      void *data, void *fdata __UNUSED__)
{
   Filereg_Item *fi;

   fi = data;
   if (!fi) return 1;
   if (fi->path) eina_stringshare_del(fi->path);
   E_FREE(fi);
   return 1;
}
