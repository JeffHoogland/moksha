#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H
# define IL_CONFIG_MAJOR 0
# define IL_CONFIG_MINOR 1
/* define keyboard object type */
# define E_ILLUME_KBD_TYPE 0xE1b0988
# define E_ILLUME_QP_TYPE 0xE1b0990

typedef enum _E_Illume_Select_Window_Type E_Illume_Select_Window_Type;
enum _E_Illume_Select_Window_Type 
{
   E_ILLUME_SELECT_WINDOW_TYPE_HOME, 
     E_ILLUME_SELECT_WINDOW_TYPE_VKBD, 
     E_ILLUME_SELECT_WINDOW_TYPE_SOFTKEY, 
     E_ILLUME_SELECT_WINDOW_TYPE_INDICATOR
};

void e_mod_illume_config_select_window(E_Illume_Select_Window_Type type);

int e_mod_quickpanel_init(void);
int e_mod_quickpanel_shutdown(void);

E_Illume_Quickpanel *e_mod_quickpanel_new(E_Zone *zone);
void e_mod_quickpanel_show(E_Illume_Quickpanel *qp);
void e_mod_quickpanel_hide(E_Illume_Quickpanel *qp);

int e_mod_kbd_init(void);
int e_mod_kbd_shutdown(void);

E_Illume_Keyboard *e_mod_kbd_new(void);
void e_mod_kbd_enable(void);
void e_mod_kbd_disable(void);
void e_mod_kbd_show(void);
void e_mod_kbd_hide(void);
void e_mod_kbd_toggle(void);
void e_mod_kbd_fullscreen_set(E_Zone *zone, int fullscreen);
void e_mod_kbd_layout_set(E_Illume_Keyboard_Layout layout);

int e_mod_illume_config_init(void);
int e_mod_illume_config_shutdown(void);
int e_mod_illume_config_save(void);

void e_mod_illume_config_animation_show(E_Container *con, const char *params __UNUSED__);
void e_mod_illume_config_policy_show(E_Container *con, const char *params __UNUSED__);
void e_mod_illume_config_windows_show(E_Container *con, const char *params __UNUSED__);
void e_mod_kbd_device_init(void);
void e_mod_kbd_device_shutdown(void);
int e_mod_policy_init(void);
int e_mod_policy_shutdown(void);

/**
 * @addtogroup Optional_Mobile
 * @{
 *
 * @defgroup Module_Illume2 Illume2
 *
 * Second generation of Illume mobile environment for Enlightenment.
 *
 * @see @ref Illume_Main_Page
 *
 * @}
 */
#endif
