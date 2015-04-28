#ifdef E_TYPEDEFS

#else
#ifndef E_INT_BORDER_MENU_H
#define E_INT_BORDER_MENU_H

typedef void (*E_Border_Menu_Hook_Cb)(void *, E_Border *);
typedef struct E_Border_Menu_Hook
{
   E_Border_Menu_Hook_Cb cb;
   void *data;
} E_Border_Menu_Hook;

EAPI E_Border_Menu_Hook *e_int_border_menu_hook_add(E_Border_Menu_Hook_Cb cb, const void *data);
EAPI void e_int_border_menu_hook_del(E_Border_Menu_Hook *hook);
EAPI void e_int_border_menu_hooks_clear(void);
EAPI void e_int_border_menu_create(E_Border *bd);
EAPI void e_int_border_menu_show(E_Border *bd, Evas_Coord x, Evas_Coord y, int key, Ecore_X_Time timestamp);
EAPI void e_int_border_menu_del(E_Border *bd);

#endif
#endif
