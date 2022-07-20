#ifndef E_FM_MAIN_H
#define E_FM_MAIN_H

#include <Ecore_Ipc.h>
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(efm_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(efm_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(efm_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(efm_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(efm_log_dom, __VA_ARGS__)

extern Ecore_Ipc_Server *_e_fm_ipc_server;
extern int efm_log_dom;

void _e_fm_main_catch(unsigned int val);

#define E_FM_MOUNT_TIMEOUT 30.0
#define E_FM_UNMOUNT_TIMEOUT 60.0
#define E_FM_EJECT_TIMEOUT 15.0

#endif
