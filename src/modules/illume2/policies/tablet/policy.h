#ifndef _POLICY_H
# define _POLICY_H

/* define layer values here so we don't have to grep through code to change */
# define POL_INDICATOR_LAYER 200
# define POL_QUICKPANEL_LAYER 160
# define POL_KEYBOARD_LAYER 150
# define POL_ACTIVATE_LAYER 145
# define POL_FULLSCREEN_LAYER 140
# define POL_DIALOG_LAYER 120
# define POL_SPLASH_LAYER 120
# define POL_SOFTKEY_LAYER 110
# define POL_CONFORMANT_LAYER 100
# define POL_APP_LAYER 100
# define POL_HOME_LAYER 90

void _policy_border_add(E_Border *bd);
void _policy_border_del(E_Border *bd);
void _policy_border_focus_in(E_Border *bd);
void _policy_border_focus_out(E_Border *bd);
void _policy_border_activate(E_Border *bd);
void _policy_border_post_fetch(E_Border *bd);
void _policy_border_post_assign(E_Border *bd);
void _policy_border_show(E_Border *bd);
void _policy_border_hide(E_Border *bd);
void _policy_zone_layout(E_Zone *zone);
void _policy_zone_move_resize(E_Zone *zone);
void _policy_zone_mode_change(E_Zone *zone, Ecore_X_Atom mode);
void _policy_zone_close(E_Zone *zone);
void _policy_drag_start(E_Border *bd);
void _policy_drag_end(E_Border *bd);
void _policy_focus_back(E_Zone *zone);
void _policy_focus_forward(E_Zone *zone);
void _policy_focus_home(E_Zone *zone);
void _policy_property_change(Ecore_X_Event_Window_Property *event);

#endif
