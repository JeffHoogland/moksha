#ifndef E_ICCCM_H
#define E_ICCCM_H

#include "e.h"

void e_icccm_move_resize(Window win, int x, int y, int w, int h);
void e_icccm_send_focus_to(Window win, int takes_focus);
void e_icccm_delete(Window win);
void e_icccm_state_mapped(Window win);
void e_icccm_state_iconified(Window win);
void e_icccm_state_withdrawn(Window win);
void e_icccm_adopt(Window win);
void e_icccm_release(Window win);
void e_icccm_get_pos_info(Window win, E_Border *b);
void e_icccm_get_size_info(Window win, E_Border *b);
void e_icccm_get_mwm_hints(Window win, E_Border *b);
void e_icccm_get_layer(Window win, E_Border *b);
void e_icccm_get_title(Window win, E_Border *b);
void e_icccm_get_class(Window win, E_Border *b);
void e_icccm_get_hints(Window win, E_Border *b);
void e_icccm_get_machine(Window win, E_Border *b);
void e_icccm_get_command(Window win, E_Border *b);
void e_icccm_get_icon_name(Window win, E_Border *b);
void e_icccm_get_state(Window win, E_Border *b);
void e_icccm_set_frame_size(Window win, int l, int r, int t, int b);
void e_icccm_set_desk_area(Window win, int ax, int ay);
void e_icccm_set_desk_area_size(Window win, int ax, int ay);
void e_icccm_set_desk(Window win, int d);
int  e_icccm_is_shaped(Window win);
void e_icccm_get_e_hack_launch_id(Window win, E_Border *b);
void e_icccm_handle_property_change(Atom a, E_Border *b);
void e_icccm_handle_client_message(Ecore_Event_Message *e);
void e_icccm_advertise_e_compat(void);
void e_icccm_advertise_mwm_compat(void);
void e_icccm_advertise_gnome_compat(void);
void e_icccm_advertise_kde_compat(void);
void e_icccm_advertise_net_compat(void);

#endif
