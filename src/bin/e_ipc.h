/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#ifdef USE_IPC
#include      "e_ipc_handlers_list.h"

typedef enum _E_Ipc_Domain
{
   E_IPC_DOMAIN_NONE,
   E_IPC_DOMAIN_SETUP,
   E_IPC_DOMAIN_REQUEST,
   E_IPC_DOMAIN_REPLY,
   E_IPC_DOMAIN_EVENT,
   E_IPC_DOMAIN_THUMB,
   E_IPC_DOMAIN_FM,
   E_IPC_DOMAIN_INIT,
   E_IPC_DOMAIN_LAST
} E_Ipc_Domain;

typedef int E_Ipc_Op;
#endif

#else
#ifndef E_IPC_H
#define E_IPC_H

EAPI int e_ipc_init(void);
EAPI int e_ipc_shutdown(void);

#endif
#endif
