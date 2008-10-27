#ifndef E_BUSYCOVER_H
#define E_BUSYCOVER_H

typedef struct _E_Busycover E_Busycover;
typedef struct _E_Busycover_Handle E_Busycover_Handle;
typedef struct _E_Event_Busycover_Simple E_Event_Busycover_Del;

#define E_BUSYCOVER_TYPE 0xE1b0782

struct _E_Busycover
{
   E_Object             e_obj_inherit;
   E_Zone              *zone;
   Evas_Object         *base_obj;
   Eina_List           *handlers;
   const char          *themedir;
   Eina_List           *handles;
};

struct _E_Busycover_Handle
{
   E_Busycover *busycover;
   const char *message;
   const char *icon;
};

struct _E_Event_Busycover_Simple
{
   E_Busycover *busycover;
};

EAPI int e_busycover_init(void);
EAPI int e_busycover_shutdown(void);
EAPI E_Busycover *e_busycover_new(E_Zone *zone, const char *themedir);
EAPI E_Busycover_Handle *e_busycover_push(E_Busycover *esw, const char *message, const char *icon);
EAPI void e_busycover_pop(E_Busycover *esw, E_Busycover_Handle *handle);

#endif
