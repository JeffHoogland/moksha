/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Startup_Mode
{
   E_STARTUP_START,
   E_STARTUP_RESTART
} E_Startup_Mode;

#else
#ifndef E_STARTUP_H
#define E_STARTUP_H

EAPI void e_startup(E_Startup_Mode mode);

#endif
#endif
