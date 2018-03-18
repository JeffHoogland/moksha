#ifndef CLIP_LOG_H
#define CLIP_LOG_H

extern int clipboard_log;

/* EINA_LOG support macros and global */
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(clipboard_log, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(clipboard_log, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(clipboard_log, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(clipboard_log, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(clipboard_log, __VA_ARGS__)

Eina_Bool logger_init(char *package);
int logger_shutdown(char *package);
void cb_mod_log(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc, int line, const char *fmt, void *data, va_list args);

#endif /* CLIP_LOG_H */
