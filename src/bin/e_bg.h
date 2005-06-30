/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum {
   E_BG_TRANSITION_NONE,
     E_BG_TRANSITION_START,
     E_BG_TRANSITION_DESK,
     E_BG_TRANSITION_CHANGE
} E_Bg_Transition;

typedef enum {
   E_BG_TRANSITION_MODE_NONE,
   E_BG_TRANSITION_MODE_RANDOM,
   E_BG_TRANSITION_MODE_FADE,
   E_BG_TRANSITION_MODE_SINUSOUDAL_FADE,
   E_BG_TRANSITION_MODE_LAST
} E_Bg_Transition_Mode;
#else
#ifndef E_BG_H
#define E_BG_H

EAPI void e_bg_zone_update(E_Zone *zone, E_Bg_Transition transition);
EAPI void e_bg_add(int container, int zone, char *desk, char *file);
EAPI void e_bg_del(int container, int zone, char *desk);
EAPI void e_bg_update(void);
    
#endif
#endif
