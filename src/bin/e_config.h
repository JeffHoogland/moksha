#ifndef E_CONFIG_H
#define E_CONFIG_H

int e_config_init(void);
int e_config_shutdown(void);

int e_config_save(void);
void e_config_save_queue(void);
    
extern char   *e_config_val_desktop_default_background;
extern double  e_config_val_menus_scroll_speed;
extern double  e_config_val_menus_fast_mouse_move_thresthold;
extern double  e_config_val_menus_click_drag_timeout;
extern double  e_config_val_framerate;
extern int     e_config_val_image_cache;
extern int     e_config_val_font_cache;

#endif
