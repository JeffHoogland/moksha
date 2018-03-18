#include <e.h>
#include "clip_log.h"
#include "e_mod_main.h"

/* The fn cb_mod_log was adapted from Elive's e17 module productivity.
 *   https://github.com/Elive/emodule-productivity
 *   Thanks to princeamd for implementing this.
 *
 * The MIT License:
 *   https://opensource.org/licenses/MIT
 */

int clipboard_log = -1;

Eina_Bool
logger_init(char *package)
{
  if(clipboard_log < 0) {
    clip_cfg->log_name = eina_stringshare_add(package);
    clipboard_log = eina_log_domain_register(clip_cfg->log_name, EINA_COLOR_CYAN);
    if(clipboard_log < 0) {
      EINA_LOG_CRIT("Could not register log domain %s", package);
      return EINA_FALSE;
    }
    eina_log_domain_level_set(clip_cfg->log_name, EINA_LOG_LEVEL_DBG);
  }
  return EINA_TRUE;
}

int
logger_shutdown(char *package)
{
  Eina_Stringshare *temp;

  if (temp = eina_stringshare_add(package))
    eina_stringshare_del(temp);

  if(clipboard_log >= 0) {
    eina_log_domain_unregister(clipboard_log);
    clipboard_log = -1;
  }
  return clipboard_log;
}

void
cb_mod_log(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file,
              const char *fnc, int line, const char *fmt, void *data EINA_UNUSED, va_list args)
{
  if ((d->name) && (d->namelen == sizeof(clip_cfg->log_name) - 1) &&
       (memcmp(d->name, clip_cfg->log_name, sizeof(clip_cfg->log_name) - 1) == 0))
  {
    const char *prefix;
    Eina_Bool use_color = !eina_log_color_disable_get();

    if (use_color)
      fputs(eina_log_level_color_get(level), stderr);

    switch (level) {
      case EINA_LOG_LEVEL_CRITICAL:
        prefix = "Critical. ";
        break;
      case EINA_LOG_LEVEL_ERR:
        prefix = "Error. ";
        break;
      case EINA_LOG_LEVEL_WARN:
        prefix = "Warning. ";
        break;
      default:
        prefix = "";
    }
    fprintf(stderr, "%s: %s", "E_CLIPBOARD", prefix);

    if (use_color)
      fputs(EINA_COLOR_RESET, stderr);

    vfprintf(stderr, fmt, args);
    putc('\n', stderr);
  } else
    eina_log_print_cb_stderr(d, level, file, fnc, line, fmt, NULL, args);
}
