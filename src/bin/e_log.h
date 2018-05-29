#ifdef E_TYPEDEFS

#else
#ifndef E_LOG_H
#define E_LOG_H


#ifdef E_LOGGING
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(e_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(e_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(e_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(e_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(e_log_dom, __VA_ARGS__)

EINTERN extern int e_log_dom;

EINTERN int e_log_init(void);
EINTERN int e_log_shutdown(void);
#else
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            do { printf(__VA_ARGS__); putc('\n', stdout); } while(0)
#define INF(...)            do { printf(__VA_ARGS__); putc('\n', stdout); } while(0)
#define WRN(...)            do { printf(__VA_ARGS__); putc('\n', stdout); } while(0)
#define ERR(...)            do { printf(__VA_ARGS__); putc('\n', stdout); } while(0)
#define CRI(...)            do { printf(__VA_ARGS__); putc('\n', stdout); } while(0)
#endif

#endif
#endif
