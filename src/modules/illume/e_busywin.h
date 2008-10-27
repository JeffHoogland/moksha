#ifndef E_BUSYWIN_H
#define E_BUSYWIN_H

typedef struct _E_Busywin E_Busywin;
typedef struct _E_Busywin_Handle E_Busywin_Handle;
typedef struct _E_Event_Busywin_Simple E_Event_Busywin_Del;

#define E_BUSYWIN_TYPE 0xE1b0976

struct _E_Busywin
{
   E_Object             e_obj_inherit;
   E_Zone              *zone;
   E_Popup             *popup;
   Evas_Object         *base_obj;
   Eina_List           *handlers;
   const char          *themedir;
   Ecore_Animator      *animator;
   Eina_List           *handles;
   Ecore_X_Window       clickwin;
   int                  adjust_start;
   int                  adjust_target;
   int                  adjust;
   double               start;
   double               len;
   unsigned char        out : 1;
};

struct _E_Busywin_Handle
{
   E_Busywin *busywin;
   const char *message;
   const char *icon;
};

struct _E_Event_Busywin_Simple
{
   E_Busywin *busywin;
};

EAPI int e_busywin_init(void);
EAPI int e_busywin_shutdown(void);
EAPI E_Busywin *e_busywin_new(E_Zone *zone, const char *themedir);
EAPI E_Busywin_Handle *e_busywin_push(E_Busywin *esw, const char *message, const char *icon);
EAPI void e_busywin_pop(E_Busywin *esw, E_Busywin_Handle *handle);

extern EAPI int E_EVENT_BUSYWIN_DEL;

#endif
