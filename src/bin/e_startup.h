#ifdef E_TYPEDEFS

typedef enum _E_Startup_Mode
{
   E_STARTUP_START,
   E_STARTUP_RESTART
} E_Startup_Mode;

#else
#ifndef E_STARTUP_H
#define E_STARTUP_H

EAPI void e_startup(void);
EAPI void e_startup_mode_set(E_Startup_Mode mode);

#endif
#endif
