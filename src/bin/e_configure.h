#ifdef E_TYPEDEFS

typedef struct _E_Configure_Cat E_Configure_Cat;
typedef struct _E_Configure_It E_Configure_It;

#else
#ifndef E_CONFIGURE_H
#define E_CONFIGURE_H

struct _E_Configure_Cat
{
   const char *cat;
   int         pri;
   const char *label;
   const char *icon_file;
   const char *icon;
   Eina_List  *items;
};

struct _E_Configure_It
{
   const char        *item;
   int                pri;
   const char        *label;
   const char        *icon_file;
   const char        *icon;
   E_Config_Dialog *(*func) (E_Container *con, const char *params);
   void             (*generic_func) (E_Container *con, const char *params);
   Efreet_Desktop    *desktop;
};

EAPI void e_configure_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params));
EAPI void e_configure_registry_generic_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, void (*generic_func) (E_Container *con, const char *params));
EAPI void e_configure_registry_item_del(const char *path);
EAPI void e_configure_registry_category_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon);
EAPI void e_configure_registry_category_del(const char *path);
EAPI void e_configure_registry_call(const char *path, E_Container *con, const char *params);
EAPI int  e_configure_registry_exists(const char *path);
EAPI void e_configure_registry_custom_desktop_exec_callback_set(void (*func) (const void *data, E_Container *con, const char *params, Efreet_Desktop *desktop), const void *data);
EAPI void e_configure_init(void);

EAPI Eina_List *e_configure_registry;

#endif
#endif
