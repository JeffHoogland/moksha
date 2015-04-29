#ifdef E_TYPEDEFS

typedef struct _E_Msg_Handler E_Msg_Handler;

#else
#ifndef E_MSG_H
#define E_MSG_H

EINTERN int            e_msg_init(void);
EINTERN int            e_msg_shutdown(void);

EAPI void           e_msg_send(const char *name, const char *info, int val, E_Object *obj, void *msgdata, void (*afterfunc) (void *data, E_Object *obj, void *msgdata), void *afterdata);
EAPI E_Msg_Handler *e_msg_handler_add(void (*func) (void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata), void *data);
EAPI void           e_msg_handler_del(E_Msg_Handler *emsgh);

#endif
#endif
