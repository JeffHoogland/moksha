/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Ipc_Domain {
   E_IPC_DOMAIN_NONE,
   E_IPC_DOMAIN_SETUP,
   E_IPC_DOMAIN_REQUEST,
   E_IPC_DOMAIN_REPLY,
   E_IPC_DOMAIN_EVENT,
   E_IPC_DOMAIN_LAST
} E_Ipc_Domain;

typedef enum _E_Ipc_Op {
   E_IPC_OP_NONE,
   E_IPC_OP_MODULE_LOAD,
   E_IPC_OP_MODULE_UNLOAD,
   E_IPC_OP_MODULE_ENABLE,
   E_IPC_OP_MODULE_DISABLE,
   E_IPC_OP_MODULE_LIST,
   E_IPC_OP_MODULE_LIST_REPLY,
   E_IPC_OP_BG_SET,
   E_IPC_OP_BG_GET,
   E_IPC_OP_BG_GET_REPLY,
   E_IPC_OP_RESTART,
   E_IPC_OP_LAST
} E_Ipc_Op;

#else
#ifndef E_IPC_H
#define E_IPC_H

EAPI int  e_ipc_init(void);
EAPI void e_ipc_shutdown(void);

#endif
#endif
