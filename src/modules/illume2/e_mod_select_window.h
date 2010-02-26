#ifndef E_MOD_SELECT_WINDOW_H
#define E_MOD_SELECT_WINDOW_H

typedef enum _E_Illume_Select_Window_Type E_Illume_Select_Window_Type;
enum _E_Illume_Select_Window_Type 
{
   E_ILLUME_SELECT_WINDOW_TYPE_HOME, 
     E_ILLUME_SELECT_WINDOW_TYPE_VKBD, 
     E_ILLUME_SELECT_WINDOW_TYPE_SOFTKEY, 
     E_ILLUME_SELECT_WINDOW_TYPE_INDICATOR
};

void e_mod_illume_config_select_window(E_Illume_Select_Window_Type type);

#endif
