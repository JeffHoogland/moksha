#ifndef E_MOD_SELECT_WINDOW_H
#define E_MOD_SELECT_WINDOW_H

typedef enum _Il_Select_Window_Type Il_Select_Window_Type;
enum _Il_Select_Window_Type 
{
   IL_SELECT_WINDOW_TYPE_HOME, 
     IL_SELECT_WINDOW_TYPE_VKBD, 
     IL_SELECT_WINDOW_TYPE_SOFTKEY, 
     IL_SELECT_WINDOW_TYPE_INDICATOR
};

void il_config_select_window(Il_Select_Window_Type type);

#endif
