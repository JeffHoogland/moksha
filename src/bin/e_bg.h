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

typedef struct _E_Event_Bg_Update E_Event_Bg_Update;

#else
#ifndef E_BG_H
#define E_BG_H

extern EAPI int E_EVENT_BG_UPDATE;

struct _E_Event_Bg_Update 
{
   int container;
   int zone;
   int desk_x;
   int desk_y;
};

EAPI int e_bg_init(void);
EAPI int e_bg_shutdown(void);

EAPI const E_Config_Desktop_Background *e_bg_config_get(int container_num, int zone_num, int desk_x, int desk_y);
EAPI const char *e_bg_file_get(int container_num, int zone_num,  int desk_x, int desk_y);
EAPI void e_bg_zone_update(E_Zone *zone, E_Bg_Transition transition);
EAPI void e_bg_add(int container, int zone, int desk_x, int desk_y, char *file);
EAPI void e_bg_del(int container, int zone, int desk_x, int desk_y);
EAPI void e_bg_default_set(char *file);
EAPI void e_bg_update(void);
EAPI void e_bg_handler_set(Evas_Object *obj, const char *path, void *data);
EAPI int  e_bg_handler_test(Evas_Object *obj, const char *path, void *data);

#endif
#endif
