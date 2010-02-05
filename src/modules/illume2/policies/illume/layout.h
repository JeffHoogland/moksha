#ifndef _LAYOUT_H
# define _LAYOUT_H

/* define some values here for easily changing layers so we don't have to 
 * grep through code to change layers */
#define IL_TOP_SHELF_LAYER 200
#define IL_BOTTOM_PANEL_LAYER 110
#define IL_KEYBOARD_LAYER 150
#define IL_DIALOG_LAYER 120
#define IL_CONFORM_LAYER 100
#define IL_FULLSCREEN_LAYER 140
#define IL_QUICKPANEL_LAYER 160
#define IL_APP_LAYER 100

void _layout_border_add(E_Border *bd);
void _layout_border_del(E_Border *bd);
void _layout_border_focus_in(E_Border *bd);
void _layout_border_focus_out(E_Border *bd);
void _layout_border_activate(E_Border *bd);
void _layout_zone_layout(E_Zone *zone);
void _layout_zone_move_resize(E_Zone *zone);
void _layout_drag_start(E_Border *bd);
void _layout_drag_end(E_Border *bd);

#endif
