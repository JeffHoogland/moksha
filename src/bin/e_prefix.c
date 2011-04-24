#include "e.h"

static Eina_Prefix *pfx = NULL;

static const char *_prefix_path_data = NULL;
static unsigned int _prefix_path_data_len = 0;

/* externally accessible functions */
EAPI int
e_prefix_determine(char *argv0)
{
   if (pfx) return 1;
   
   pfx = eina_prefix_new(argv0, e_prefix_determine, 
                         "E", "enlightenment", "AUTHORS",
                         PACKAGE_BIN_DIR,
                         PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR,
                         LOCALE_DIR);
   if (!pfx) return 0;
   
   _prefix_path_data = eina_prefix_data_get(pfx);
   _prefix_path_data_len= strlen(_prefix_path_data);

   printf("=================================\n");
   printf("Enlightenment relocation handling\n");
   printf("=================================\n");
   printf("PREFIX:  %s\n", eina_prefix_get(pfx));
   printf("BINDIR:  %s\n", eina_prefix_bin_get(pfx));
   printf("LIBDIR:  %s\n", eina_prefix_lib_get(pfx));
   printf("DATADIR: %s\n", eina_prefix_data_get(pfx));
   printf("LOCALE:  %s\n", eina_prefix_locale_get(pfx));
   printf("=================================\n");
   e_util_env_set("E_PREFIX", eina_prefix_get(pfx));
   e_util_env_set("E_BIN_DIR", eina_prefix_bin_get(pfx));
   e_util_env_set("E_LIB_DIR", eina_prefix_lib_get(pfx));
   e_util_env_set("E_DATA_DIR", eina_prefix_data_get(pfx));
   e_util_env_set("E_LOCALE_DIR", eina_prefix_locale_get(pfx));
   return 1;
}

EINTERN void
e_prefix_shutdown(void)
{
   if (!pfx) return;
   _prefix_path_data = NULL;
   _prefix_path_data_len = 0;
   eina_prefix_free(pfx);
   pfx = NULL;
}
   
EAPI void
e_prefix_fallback(void)
{
}

EAPI const char *
e_prefix_get(void)
{
   return eina_prefix_get(pfx);
}

EAPI const char *
e_prefix_locale_get(void)
{
   return eina_prefix_locale_get(pfx);
}

EAPI const char *
e_prefix_bin_get(void)
{
   return eina_prefix_bin_get(pfx);
}

EAPI const char *
e_prefix_data_get(void)
{
   return eina_prefix_data_get(pfx);
}

EAPI const char *
e_prefix_lib_get(void)
{
   return eina_prefix_lib_get(pfx);
}

EAPI size_t
e_prefix_data_concat_len(char *dst, size_t size, const char *path, size_t path_len)
{
   return eina_str_join_len(dst, size, '/', _prefix_path_data, _prefix_path_data_len, path, path_len);
}

EAPI size_t
e_prefix_data_concat(char *dst, size_t size, const char *path)
{
   return e_prefix_data_concat_len(dst, size, path, strlen(path));
}

EAPI size_t
e_prefix_data_snprintf(char *dst, size_t size, const char *fmt, ...)
{
   size_t off, ret;
   va_list ap;
   
   va_start(ap, fmt);
   
   off = _prefix_path_data_len + 1;
   if (size < _prefix_path_data_len + 2)
     {
        if (size > 1)
          {
             memcpy(dst, _prefix_path_data, size - 1);
             dst[size - 1] = '\0';
          }
        ret = off + vsnprintf(dst + off, size - off, fmt, ap);
        va_end(ap);
        return ret;
     }
   
   memcpy(dst, _prefix_path_data, _prefix_path_data_len);
   dst[_prefix_path_data_len] = '/';
   
   ret = off + vsnprintf(dst + off, size - off, fmt, ap);
   va_end(ap);
   return ret;
}
