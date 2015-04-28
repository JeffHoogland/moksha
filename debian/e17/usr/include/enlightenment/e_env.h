#ifdef E_TYPEDEFS

#else
#ifndef E_ENV_H
#define E_ENV_H

/* init and setup */
EINTERN int              e_env_init(void);
EINTERN int              e_env_shutdown(void);

EAPI void                e_env_set(const char *var, const char *val);
EAPI void                e_env_unset(const char *var);

#endif
#endif
