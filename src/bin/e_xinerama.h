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

EINTERN int           e_xinerama_init(void);
EINTERN int           e_xinerama_shutdown(void);
EAPI void             e_xinerama_update(void);
EAPI const Eina_List *e_xinerama_screens_get(void);
EAPI const Eina_List *e_xinerama_screens_all_get(void);
EAPI void             e_xinerama_fake_screen_add(int x, int y, int w, int h);
EAPI Eina_Bool        e_xinerama_fake_screens_exist(void);

#endif
#endif
