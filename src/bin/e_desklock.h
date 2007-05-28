#ifdef E_TYPEDEFS
#else
#ifndef E_DESKLOCK_H
#define E_DESKLOCK_H

EAPI int e_desklock_init(void);
EAPI int e_desklock_shutdown(void);

EAPI int e_desklock_show(void);
EAPI void e_desklock_hide(void);

extern EAPI int E_EVENT_DESKLOCK_SHOW;
extern EAPI int E_EVENT_DESKLOCK_HIDE;

#endif
#endif
