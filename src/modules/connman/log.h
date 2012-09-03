#ifndef _E_CONNMAN_LOG_H_
#define _E_CONNMAN_LOG_H_

extern int _e_connman_log_dom;

#undef DBG
#undef INF
#undef WRN
#undef ERR

#define DBG(...) EINA_LOG_DOM_DBG(_e_connman_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_e_connman_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_e_connman_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_e_connman_log_dom, __VA_ARGS__)

#endif
