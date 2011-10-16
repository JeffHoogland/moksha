#ifdef E_TYPEDEFS

typedef enum {
   E_BG_TRANSITION_NONE,
     E_BG_TRANSITION_START,
     E_BG_TRANSITION_DESK,
     E_BG_TRANSITION_CHANGE
} E_Bg_Transition;

typedef struct _E_Event_Bg_Update E_Event_Bg_Update;

typedef struct _E_Bg_Image_Import_Handle E_Bg_Image_Import_Handle;

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

EINTERN int e_bg_init(void);
EINTERN int e_bg_shutdown(void);

EAPI const E_Config_Desktop_Background *e_bg_config_get(int container_num, int zone_num, int desk_x, int desk_y);
EAPI const char *e_bg_file_get(int container_num, int zone_num,  int desk_x, int desk_y);
EAPI void e_bg_zone_update(E_Zone *zone, E_Bg_Transition transition);
EAPI void e_bg_add(int container, int zone, int desk_x, int desk_y, const char *file);
EAPI void e_bg_del(int container, int zone, int desk_x, int desk_y);
EAPI void e_bg_default_set(const char *file);
EAPI void e_bg_update(void);
EAPI void e_bg_handler_set(Evas_Object *obj, const char *path, void *data);
EAPI int  e_bg_handler_test(Evas_Object *obj, const char *path, void *data);

EAPI E_Bg_Image_Import_Handle *e_bg_image_import_new(const char *image_file, void (*cb)(void *data, const char *edje_file), const void *data);
EAPI void                      e_bg_image_import_cancel(E_Bg_Image_Import_Handle *handle);

#endif
#endif
