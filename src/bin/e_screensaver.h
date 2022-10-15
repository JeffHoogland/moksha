#ifdef E_TYPEDEFS
#else
#ifndef E_SCREENSAVER_H
#define E_SCREENSAVER_H

EINTERN void e_screensaver_preinit(void);
EINTERN int e_screensaver_init(void);
EINTERN int e_screensaver_shutdown(void);

EAPI void e_screensaver_update(void);
EAPI void e_screensaver_force_update(void);

EAPI int e_screensaver_timeout_get(Eina_Bool use_idle);
EAPI Eina_Bool e_screensaver_on_get(void);
EAPI void e_screensaver_activate(void);
EAPI void e_screensaver_deactivate(void);

EAPI extern int E_EVENT_SCREENSAVER_ON;
EAPI extern int E_EVENT_SCREENSAVER_OFF;

#endif
#endif
