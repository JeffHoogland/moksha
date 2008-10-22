/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Screen E_Screen;

#else
#ifndef E_XINERAMA_H
#define E_XINERAMA_H

struct _E_Screen
{
   int screen, escreen;
   int x, y, w, h;
};

EAPI int              e_xinerama_init(void);
EAPI int              e_xinerama_shutdown(void);
EAPI void             e_xinerama_update(void);
EAPI const Eina_List *e_xinerama_screens_get(void);
EAPI const Eina_List *e_xinerama_screens_all_get(void);
EAPI void             e_xinerama_fake_screen_add(int x, int y, int w, int h);
    
#endif
#endif
