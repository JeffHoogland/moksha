#ifndef __ACTIONS_H_
#define __ACTIONS_H_

#include "e.h"

static void _e_action_find(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o);
static void _e_action_free(E_Action *a);

static void e_act_move_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_move_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_move_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_h_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_h_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_h_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_v_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_v_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_v_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_close_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_kill_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_shade_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_raise_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_lower_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_raise_lower_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_exec_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_menu_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_exit_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_restart_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_stick_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_sound_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_iconify_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_max_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_snap_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_zoom_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

#endif

