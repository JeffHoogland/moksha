/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Binding_Context
{
   E_BINDING_CONTEXT_BORDER,
   E_BINDING_CONTEXT_ZONE,
   E_BINDING_CONTEXT_OTHER
} E_Binding_Context;

#else
#ifndef E_BINDINGS_H
#define E_BINDINGS_H

EAPI int         e_bindings_init(void);
EAPI int         e_bindings_shutdown(void);

//EAPI void        e_bindings_key_event_handle(E_Binding_Context context, E_Object *obj, char *key); /* finxish... */
					 
#endif
#endif
