#ifndef __BORDERS_H_
#define __BORDERS_H_

#include "e.h"

static void e_idle(void *data);
static void e_map_request(Eevent * ev);
static void e_configure_request(Eevent * ev);
static void e_property(Eevent * ev);
static void e_unmap(Eevent * ev);
static void e_destroy(Eevent * ev);
static void e_circulate_request(Eevent * ev);
static void e_reparent(Eevent * ev);
static void e_shape(Eevent * ev);
static void e_focus_in(Eevent * ev);
static void e_focus_out(Eevent * ev);
static void e_colormap(Eevent * ev);
static void e_mouse_down(Eevent * ev);
static void e_mouse_up(Eevent * ev);
static void e_mouse_in(Eevent * ev);
static void e_mouse_out(Eevent * ev);
static void e_window_expose(Eevent * ev);

static void e_cb_mouse_in(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_out(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_down(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_up(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_move(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);

static void e_cb_border_mouse_in(E_Border *b, Eevent *e);
static void e_cb_border_mouse_out(E_Border *b, Eevent *e);
static void e_cb_border_mouse_down(E_Border *b, Eevent *e);
static void e_cb_border_mouse_up(E_Border *b, Eevent *e);
static void e_cb_border_mouse_move(E_Border *b, Eevent *e);
static void e_cb_border_move_resize(E_Border *b);
static void e_cb_border_visibility(E_Border *b);

static void e_border_poll(int val, void *data);


#endif
