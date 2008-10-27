#ifdef E_TYPEDEFS

#else
#ifndef E_SIMPLELOCK_H
#define E_SIMPLELOCK_H

EAPI int e_simplelock_init(E_Module *m);
EAPI int e_simplelock_shutdown(void);

EAPI int e_simplelock_show(void);
EAPI void e_simplelock_hide(void);

#endif
#endif
