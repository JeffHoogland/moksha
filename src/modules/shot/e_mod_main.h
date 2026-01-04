#include "e.h"
#include <time.h>
#include <sys/mman.h>
#include <Elementary.h>

#if defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/types.h>
# include <sys/sysctl.h>
#endif

extern E_Module *shot_module;

#define MAXZONES 64

void         share_save              (const char *cmd, const char *file, Eina_Bool copy);
void         share_write_end_watch   (void *data);
void         share_write_status_watch(void *data);
void         share_dialog_show       (void);
void         share_confirm           (void);
Eina_Bool    share_have              (void);
void         share_abort             (void);
void         preview_dialog_show     (E_Zone *zone, E_Border *bd, const char *params, void *dst, int sx, int sy, int sw, int sh);
Eina_Bool    preview_have            (void);
void         preview_abort           (void);
Evas_Object *preview_image_get       (void);
void         save_to                 (const char *file, Eina_Bool copy);
void         save_show               (Eina_Bool save);

Evas_Object *ui_edit(Evas_Object *window, Evas_Object *o_bg, E_Zone *zone,
                     E_Border *bd, void *dst, int sx, int sy, int sw, int sh,
                     Evas_Object **o_img_ret);
void         ui_edit_prepare(void);
void         ui_edit_crop_screen_set(int x, int y, int w, int h);

void         win_delay(void);
void         delay_abort(void);


extern Evas_Object *win;
extern int quality;
extern Eina_Rectangle crop;
