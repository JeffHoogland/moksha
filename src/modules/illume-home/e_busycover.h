#ifndef E_BUSYCOVER_H
# define E_BUSYCOVER_H

# define E_BUSYCOVER_TYPE 0xE1b0782

typedef struct _E_Busycover E_Busycover;
typedef struct _E_Busycover_Handle E_Busycover_Handle;

struct _E_Busycover 
{
   E_Object e_obj_inherit;
   Evas_Object *o_base;
   Eina_List *handles;
};
struct _E_Busycover_Handle 
{
   E_Busycover *cover;
   const char *msg, *icon;
};

EAPI E_Busycover *e_busycover_new(E_Win *win);
EAPI E_Busycover_Handle *e_busycover_push(E_Busycover *cover, const char *msg, const char *icon);
EAPI void e_busycover_pop(E_Busycover *cover, E_Busycover_Handle *handle);
EAPI void e_busycover_resize(E_Busycover *cover, int w, int h);

#endif
