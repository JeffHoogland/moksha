/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Powersave_Mode
{
   E_POWERSAVE_MODE_NONE,
   E_POWERSAVE_MODE_LOW,
   E_POWERSAVE_MODE_MEDIUM,
   E_POWERSAVE_MODE_HIGH,
   E_POWERSAVE_MODE_EXTREME
} E_Powersave_Mode;

typedef struct _E_Powersave_Deferred_Action E_Powersave_Deferred_Action;
typedef struct _E_Event_Powersave_Update E_Event_Powersave_Update;

#else
#ifndef E_POWERSAVE_H
#define E_POWERSAVE_H

extern EAPI int E_EVENT_POWERSAVE_UPDATE;

struct _E_Event_Powersave_Update 
{
   E_Powersave_Mode	mode;
};

EAPI int e_powersave_init(void);
EAPI int e_powersave_shutdown(void);

EAPI E_Powersave_Deferred_Action *e_powersave_deferred_action_add(void (*func) (void *data), const void *data);
EAPI void                         e_powersave_deferred_action_del(E_Powersave_Deferred_Action *pa);
EAPI void                         e_powersave_mode_min_set(E_Powersave_Mode mode);
EAPI void                         e_powersave_mode_max_set(E_Powersave_Mode mode);
EAPI void                         e_powersave_mode_set(E_Powersave_Mode mode);
EAPI E_Powersave_Mode             e_powersave_mode_min_get(void);
EAPI E_Powersave_Mode             e_powersave_mode_max_get(void);
EAPI E_Powersave_Mode             e_powersave_mode_get(void);

/* FIXME: in the powersave system add things like pre-loading entire files
 * int memory for pre-caching to avoid disk spinup, when in an appropriate
 * powersave mode */

/* FIXME: in powersave mode also add the ability to reduce framerate when
 * at a particular powersave mode */

/* FIXME: in powersave mode also reduce ecore timer precision, so timers
 * will be dispatched together and system will wakeup less. */

/* FIXME: in powersave mode also add the ability to change screenblanker
 * preferences when in powersave mode as well as check in the screensaver
 * has kicked in */

/* FIXME: in powersave mode also if screenblanker has kicked in be able to
 * auto-suspend etc. etc. */

#endif
#endif
