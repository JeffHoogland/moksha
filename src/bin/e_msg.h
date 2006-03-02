/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Msg_Handler E_Msg_Handler;

#else
#ifndef E_MSG_H
#define E_MSG_H

EAPI int            e_msg_init(void);
EAPI int            e_msg_shutdown(void);

EAPI void           e_msg_send(const char *name, const char *info, int val, E_Object *obj);
EAPI E_Msg_Handler *e_msg_handler_add(void (*func) (void *data, const char *name, const char *info, int val, E_Object *obj), void *data);
EAPI void           e_msg_handler_del(E_Msg_Handler *emsgh);

#endif
#endif
