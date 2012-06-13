#include "e.h"

#ifndef E_LOG_LEVEL
# define E_LOG_LEVEL EINA_LOG_LEVEL_INFO
#endif

EINTERN int e_log_dom = -1;

static const char *_names[] = {
   "CRI",
   "ERR",
   "WRN",
   "INF",
   "DBG",
};

static void
_e_log_cb(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc __UNUSED__, int line, const char *fmt, void *data __UNUSED__, va_list args)
{
   const char *color;

   color = eina_log_level_color_get(level);
   fprintf(stdout,
           "%s%s<" EINA_COLOR_RESET "%s%s>" EINA_COLOR_RESET "%s:%d" EINA_COLOR_RESET " ",
           color, _names[level > EINA_LOG_LEVEL_DBG ? EINA_LOG_LEVEL_DBG : level],
           d->domain_str, color, file, line);
   vfprintf(stdout, fmt, args);
   putc('\n', stdout);
}

EINTERN int
e_log_init(void)
{
   e_log_dom = eina_log_domain_register("e", EINA_COLOR_WHITE);
   eina_log_domain_level_set("e", E_LOG_LEVEL);
   eina_log_print_cb_set(_e_log_cb, NULL);
   return (e_log_dom != -1);
}

EINTERN int
e_log_shutdown(void)
{
   eina_log_domain_unregister(e_log_dom);
   e_log_dom = -1;
   return 0;
}
