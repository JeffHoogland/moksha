#ifndef E_CONFIG_H
#define E_CONFIG_H

EAPI int e_config_init(void);
EAPI int e_config_shutdown(void);

EAPI int e_config_save(void);
EAPI void e_config_save_queue(void);
    
extern EAPI char   *e_config_val_desktop_default_background;
extern EAPI double  e_config_val_menus_scroll_speed;
extern EAPI double  e_config_val_menus_fast_mouse_move_thresthold;
extern EAPI double  e_config_val_menus_click_drag_timeout;
extern EAPI double  e_config_val_framerate;
extern EAPI int     e_config_val_image_cache;
extern EAPI int     e_config_val_font_cache;

#endif
