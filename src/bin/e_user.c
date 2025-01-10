#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <Efreet.h>

#include "e_macros.h"

static const char *_e_user_homedir = NULL;
static size_t _e_user_homedir_len = 0;

/* externally accessible functions */
EAPI const char *
e_user_homedir_get(void)
{
   char *d;

   if (_e_user_homedir)
     return _e_user_homedir;

   _e_user_homedir = d = getenv("HOME");
   if (!_e_user_homedir)
     {
        _e_user_homedir = "/tmp";
        _e_user_homedir_len = sizeof("/tmp") - 1;
        return _e_user_homedir;
     }

   _e_user_homedir_len = strlen(_e_user_homedir);
   while ((_e_user_homedir_len > 1) &&
          (d[_e_user_homedir_len - 1] == '/'))
     {
        _e_user_homedir_len--;
        d[_e_user_homedir_len] = '\0';
     }
   return _e_user_homedir;
}

/**
 * Concatenate '~/' and @a path.
 *
 * @return similar to snprintf(), this returns the number of bytes written or
 *     that would be required to write if greater or equal than size.
 */
EAPI size_t
e_user_homedir_concat_len(char *dst, size_t size, const char *path, size_t path_len)
{
   if (!_e_user_homedir)
     e_user_homedir_get();

   return eina_str_join_len(dst, size, '/', _e_user_homedir, _e_user_homedir_len, path, path_len);
}

EAPI size_t
e_user_homedir_concat(char *dst, size_t size, const char *path)
{
   return e_user_homedir_concat_len(dst, size, path, strlen(path));
}

/**
 * same as snprintf("~/"fmt, ...).
 */
EAPI size_t EINA_PRINTF(3, 4)
e_user_homedir_snprintf(char *dst, size_t size, const char *fmt, ...)
{
   size_t off, ret;
   va_list ap;

   if (!_e_user_homedir)
     e_user_homedir_get();
   if (!_e_user_homedir)
     return 0;

   va_start(ap, fmt);

   off = _e_user_homedir_len + 1;
   if (size < _e_user_homedir_len + 2)
     {
        if (size > 1)
          {
             memcpy(dst, _e_user_homedir, size - 1);
             dst[size - 1] = '\0';
          }
        ret = off + vsnprintf(dst + off, size - off, fmt, ap);
        va_end(ap);
        return ret;
     }

   memcpy(dst, _e_user_homedir, _e_user_homedir_len);
   dst[_e_user_homedir_len] = '/';

   ret = off + vsnprintf(dst + off, size - off, fmt, ap);
   va_end(ap);
   return ret;
}

/**
 * Return the directory where user .desktop files should be stored.
 * If the directory does not exist, it will be created. If it cannot be
 * created, a dialog will be displayed an NULL will be returned
 */
EAPI const char *
e_user_desktop_dir_get(void)
{
   static char dir[PATH_MAX] = "";

   if (!dir[0])
     snprintf(dir, sizeof(dir), "%s/applications", efreet_data_home_get());

   return dir;
}

/**
 * Return the directory where user .icon files should be stored.
 * If the directory does not exist, it will be created. If it cannot be
 * created, a dialog will be displayed an NULL will be returned
 */
EAPI const char *
e_user_icon_dir_get(void)
{
   static char dir[PATH_MAX] = "";

   if (!dir[0])
     snprintf(dir, sizeof(dir), "%s/icons", efreet_data_home_get());

   return dir;
}

static const char *_e_user_dir = NULL;
static size_t _e_user_dir_len = 0;

EAPI void         e_util_env_set(const char *var, const char *val);

/**
 * Return ~/.e/e
 */
EAPI const char *
e_user_dir_get(void)
{
   static char dir[PATH_MAX] = "";

   if (!dir[0])
     {
        char *d = getenv("E_HOME");

        if (d)
          snprintf(dir, sizeof(dir), "E_HOME_DIR=%s/e", d);
        else
          {
#ifdef DOXDG
             if ((d = getenv("XDG_CONFIG_HOME")))
               snprintf(dir, sizeof(dir), "E_HOME_DIR=%s/e", d);
             else
#endif
               {
                  char buf[PATH_MAX - 128];
#ifdef DOXDG
                  e_user_homedir_concat(buf, sizeof(buf), ".config/e");
#else
                  e_user_homedir_concat(buf, sizeof(buf), ".e/e");
#endif
                  snprintf(dir, sizeof(dir), "E_HOME_DIR=%s", buf);
               }
          }
        _e_user_dir = dir + strlen("E_HOME_DIR=");
        _e_user_dir_len = strlen(_e_user_dir);
        putenv(dir);
     }
   return _e_user_dir;
}

/**
 * Concatenate '~/.e/e' and @a path.
 *
 * @return similar to snprintf(), this returns the number of bytes written or
 *     that would be required to write if greater or equal than size.
 */
EAPI size_t
e_user_dir_concat_len(char *dst, size_t size, const char *path, size_t path_len)
{
   if (!_e_user_dir)
     e_user_dir_get();

   return eina_str_join_len(dst, size, '/', _e_user_dir, _e_user_dir_len, path, path_len);
}

EAPI size_t
e_user_dir_concat(char *dst, size_t size, const char *path)
{
   return e_user_dir_concat_len(dst, size, path, strlen(path));
}

/**
 * same as snprintf("~/.e/e/"fmt, ...).
 */
EAPI size_t EINA_PRINTF(3, 4)
e_user_dir_snprintf(char *dst, size_t size, const char *fmt, ...)
{
   size_t off, ret;
   va_list ap;

   if (!_e_user_dir)
     e_user_dir_get();
   if (!_e_user_dir)
     return 0;

   va_start(ap, fmt);

   off = _e_user_dir_len + 1;
   if (size < _e_user_dir_len + 2)
     {
        if (size > 1)
          {
             memcpy(dst, _e_user_dir, size - 1);
             dst[size - 1] = '\0';
          }
        ret = off + vsnprintf(dst + off, size - off, fmt, ap);
        va_end(ap);
        return ret;
     }

   memcpy(dst, _e_user_dir, _e_user_dir_len);
   dst[_e_user_dir_len] = '/';

   ret = off + vsnprintf(dst + off, size - off, fmt, ap);
   va_end(ap);
   return ret;
}
