#include <stdio.h>

#include <Eina.h>
#include <eina_safety_checks.h>
#include "E_Connman.h"

extern int _e_dbus_connman_log_dom;

#ifndef EINA_LOG_DEFAULT_COLOR
#define EINA_LOG_DEFAULT_COLOR EINA_COLOR_CYAN
#endif

#undef DBG
#undef INF
#undef WRN
#undef ERR

#define DBG(...) EINA_LOG_DOM_DBG(_e_dbus_connman_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_e_dbus_connman_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_e_dbus_connman_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_e_dbus_connman_log_dom, __VA_ARGS__)
