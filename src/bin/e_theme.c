/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
typedef struct _E_Theme_Result E_Theme_Result;

struct _E_Theme_Result
{
   unsigned char generated : 1;
   char *file;
   char *cache;
};

static const char *_e_theme_file_get_internal(char *category, int recursion);

/* local subsystem globals */
static Evas_Hash *mappings = NULL;

/* externally accessible functions */

int
e_theme_init(void)
{
   /* this is a fallback that is ALWAYS there */
   e_theme_file_set("base", "default.edj");
   /* now add more */
   /* FIXME: load these from a config */
   e_theme_file_set("base/theme", "default.edj");
   e_theme_file_set("base/theme/borders", "default.edj");
   e_theme_file_set("base/theme/menus", "default.edj");
   e_theme_file_set("base/theme/background", "default.edj");
   e_theme_file_set("base/theme/error", "default.edj");
   e_theme_file_set("base/theme/gadman", "default.edj");
   e_theme_file_set("base/theme/icons", "default.edj");
   e_theme_file_set("base/theme/cursors", "default.edj");
   e_theme_file_set("base/theme/modules", "default.edj");
   e_theme_file_set("base/theme/modules/pager", "default.edj");
   e_theme_file_set("base/theme/modules/ibar", "default.edj");
   e_theme_file_set("base/theme/modules/clock", "default.edj");
   e_theme_file_set("base/theme/modules/battery", "default.edj");
   e_theme_file_set("base/theme/modules/cpufreq", "default.edj");
   e_theme_file_set("base/theme/modules/temperature", "default.edj");
   /* FIXME: need to not just load, but save TO the config too */
   return 1;
}

int
e_theme_shutdown(void)
{
   /* FIXME; clear out mappings hash */
   return 1;
}

const char *
e_theme_file_get(char *category)
{
   return _e_theme_file_get_internal(category, 0);
}

void
e_theme_file_set(char *category, char *file)
{
   E_Theme_Result *res;
   
   res = evas_hash_find(mappings, category);
   if (res)
     {
	mappings = evas_hash_del(mappings, category, res);
	free(res->file);
	free(res);
     }
   res = calloc(1, sizeof(E_Theme_Result));
   res->file = strdup(file);
   mappings = evas_hash_add(mappings, category, res);
}

/* local subsystem functions */

static const char *
_e_theme_file_get_internal(char *category, int recursion)
{
   const char *str;
   E_Theme_Result *res;
   
   if (strlen(category) == 0) return NULL;
   res = evas_hash_find(mappings, category);
   if (!res)
     {
	char buf[256];
	char *p;
	
	strncpy(buf, category, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = 0;
	p = strrchr(buf, '/');
	if (p)
	  {
	     *p = 0;
	     return
	       _e_theme_file_get_internal(buf, recursion + 1);
	  }
	return NULL;
     }
   str = res->cache;
   if (!str)
     {
	str = res->file;
	if (str[0] != '/')
	  str = e_path_find(path_themes, str);
	if (str)
	  res->cache = strdup(str);
     }
   return str;
}
