#include "e_mod_tiling.h"

/* types {{{ */

#define TILING_OVERLAY_TIMEOUT 5.0
#define TILING_RESIZE_STEP 5
#define TILING_WRAP_SPEED 0.1

typedef enum {
    TILING_RESIZE,
    TILING_MOVE,
} tiling_change_t;

typedef enum {
    INPUT_MODE_NONE,
    INPUT_MODE_SWAPPING,
    INPUT_MODE_MOVING,
    INPUT_MODE_GOING,
    INPUT_MODE_TRANSITION,
} tiling_input_mode_t;

typedef enum {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,

    MOVE_COUNT
} tiling_move_t;

typedef struct geom_t {
    int x, y, w, h;
} geom_t;

typedef struct overlay_t {
    E_Popup *popup;
    Evas_Object *obj;
} overlay_t;

typedef struct transition_overlay_t {
    overlay_t overlay;
    int stack;
    char key[4];
    E_Border *bd;
} transition_overlay_t;

typedef struct Border_Extra {
    E_Border *border;
    geom_t expected;
    struct {
         geom_t geom;
         unsigned int layer;
         E_Stacking  stacking;
         E_Maximize  maximized;
         const char *bordername;
    } orig;
    overlay_t overlay;
    char key[4];
} Border_Extra;

struct tiling_g tiling_g = {
    .module = NULL,
    .config = NULL,
    .log_domain = -1,
    .default_keyhints = "asdfg;lkjh",
};

static void
_add_border(E_Border *bd);

/* }}} */
/* Globals {{{ */

static struct tiling_mod_main_g
{
    char                  edj_path[PATH_MAX];
    E_Config_DD          *config_edd,
                         *vdesk_edd;
    int                   currently_switching_desktop;
    Ecore_X_Window        action_input_win;
    Ecore_Event_Handler  *handler_key,
                         *handler_border_resize,
                         *handler_border_move,
                         *handler_border_add,
                         *handler_border_remove,
                         *handler_border_iconify,
                         *handler_border_uniconify,
                         *handler_border_stick,
                         *handler_border_unstick,
                         *handler_desk_show,
                         *handler_desk_before_show,
                         *handler_desk_set,
                         *handler_container_resize;
    E_Border_Hook        *pre_border_assign_hook;

    Tiling_Info          *tinfo;
    Eina_Hash            *info_hash;
    Eina_Hash            *border_extras;
    Eina_Hash            *overlays;

    E_Action             *act_togglefloat,
                         *act_addstack,
                         *act_removestack,
                         *act_tg_stack,
                         *act_swap,
                         *act_move,
                         *act_move_left,
                         *act_move_right,
                         *act_move_up,
                         *act_move_down,
                         *act_adjusttransitions,
                         *act_go,
                         *act_send_ne,
                         *act_send_nw,
                         *act_send_se,
                         *act_send_sw;

    int                   warp_x,
                          warp_y,
                          old_warp_x,
                          old_warp_y,
                          warp_to_x,
                          warp_to_y;
    Ecore_Timer          *warp_timer;

    overlay_t             move_overlays[MOVE_COUNT];
    transition_overlay_t *transition_overlay;
    Ecore_Timer          *action_timer;
    E_Border             *focused_bd;
    void (*action_cb)(E_Border *bd, Border_Extra *extra);

    tiling_input_mode_t   input_mode;
    char                  keys[4];
} _G = {
    .input_mode = INPUT_MODE_NONE,
};

/* }}} */
/* Utils {{{ */

/* I wonder why noone has implemented the following one yet? */
static E_Desk *
get_current_desk(void)
{
    E_Manager *m = e_manager_current_get();
    E_Container *c = e_container_current_get(m);
    E_Zone *z = e_zone_current_get(c);

    return e_desk_current_get(z);
}

static Tiling_Info *
_initialize_tinfo(const E_Desk *desk)
{
    Tiling_Info *tinfo;

    tinfo = E_NEW(Tiling_Info, 1);
    tinfo->desk = desk;
    eina_hash_direct_add(_G.info_hash, &tinfo->desk, tinfo);

    tinfo->conf = get_vdesk(tiling_g.config->vdesks, desk->x, desk->y,
                            desk->zone->num);

    return tinfo;
}

static void
check_tinfo(const E_Desk *desk)
{
    if (!_G.tinfo || _G.tinfo->desk != desk) {
        _G.tinfo = eina_hash_find(_G.info_hash, &desk);
        if (!_G.tinfo) {
            /* lazy init */
            _G.tinfo = _initialize_tinfo(desk);
        }
        if (!_G.tinfo->conf) {
            _G.tinfo->conf = get_vdesk(tiling_g.config->vdesks,
                                       desk->x, desk->y,
                                       desk->zone->num);
        }
    }
}

static int
is_floating_window(const E_Border *bd)
{
    return EINA_LIST_IS_IN(_G.tinfo->floating_windows, bd);
}

static int
is_tilable(const E_Border *bd)
{
    if (bd->client.icccm.min_h == bd->client.icccm.max_h
    &&  bd->client.icccm.max_h > 0)
        return false;

    if (bd->client.icccm.gravity == ECORE_X_GRAVITY_STATIC)
        return false;

    if (!tiling_g.config->tile_dialogs
            && ((bd->client.icccm.transient_for != 0)
                || (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)))
        return false;

    return true;
}

static void
change_window_border(E_Border   *bd,
                     const char *bordername)
{
    eina_stringshare_replace(&bd->bordername, bordername);
    bd->client.border.changed = true;
    bd->changes.border = true;
    bd->changed = true;

    DBG("%p -> border %s", bd, bordername);
}

static int
get_stack(const E_Border *bd)
{
    int i;
    for (i = 0; i < TILING_MAX_STACKS; i++) {
        if (EINA_LIST_IS_IN(_G.tinfo->stacks[i], bd))
            return i;
    }
    return -1;
}

static int
get_stack_count(void)
{
    int i;
    for (i = 0; i < TILING_MAX_STACKS; i++) {
        if (!_G.tinfo->stacks[i])
            return i;
    }
    return TILING_MAX_STACKS;
}

static int
get_window_count(void)
{
    int res = 0;
    int i;

    for (i = 0; i < TILING_MAX_STACKS; i++) {
        if (!_G.tinfo->stacks[i])
            break;
        res += eina_list_count(_G.tinfo->stacks[i]);
    }
    return res;
}

static int
get_transition_count(void)
{
    int res = 0;
    int i;

    for (i = 0; i < TILING_MAX_STACKS; i++) {
        if (!_G.tinfo->stacks[i])
            break;
        res += eina_list_count(_G.tinfo->stacks[i]);
    }
    if (_G.tinfo->stacks[0])
        res--;
    return res;
}

static void
_theme_edje_object_set_aux(Evas_Object *obj, const char *group)
{
    if (!e_theme_edje_object_set(obj, "base/theme/modules/tiling",
                                 group)) {
        edje_object_file_set(obj, _G.edj_path, group);
    }
}
#define _theme_edje_object_set(_obj, _group)                                 \
    if (e_config->use_composite)                                             \
        _theme_edje_object_set_aux(_obj, _group"/composite");                \
    else                                                                     \
        _theme_edje_object_set_aux(_obj, _group);

static Eina_Bool
_info_hash_update(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__,
                  void *data, void *fdata __UNUSED__)
{
    Tiling_Info *tinfo = data;

    if (tinfo->desk) {
        tinfo->conf = get_vdesk(tiling_g.config->vdesks,
                                tinfo->desk->x, tinfo->desk->y,
                                tinfo->desk->zone->num);
    } else {
        tinfo->conf = NULL;
    }

    return true;
}

void
e_tiling_update_conf(void)
{
    eina_hash_foreach(_G.info_hash, _info_hash_update, NULL);
}

static void
_e_border_move_resize(E_Border *bd,
                      int       x,
                      int       y,
                      int       w,
                      int       h)
{
    DBG("%p -> %dx%d+%d+%d", bd, w, h, x, y);
    e_border_move_resize(bd, x, y, w, h);
}

static void
_e_border_move(E_Border *bd,
               int       x,
               int       y)
{
    DBG("%p -> +%d+%d", bd, x, y);
    e_border_move(bd, x, y);
}

static void
_e_border_resize(E_Border *bd,
                 int       w,
                 int       h)
{
    DBG("%p -> %dx%d", bd, w, h);
    e_border_resize(bd, w, h);
}

static void
_e_border_maximize(E_Border *bd, E_Maximize max)
{
    DBG("%p -> %s", bd,
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH");
    DBG("new_client:%s, bd->maximized=%x",
        bd->new_client? "true": "false",
        bd->maximized);
    e_border_maximize(bd, max);
}

static void
_e_border_unmaximize(E_Border *bd, E_Maximize max)
{
    DBG("%p -> %s", bd,
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH");
    e_border_unmaximize(bd, max);
}

static void
_restore_border(E_Border *bd)
{
    Border_Extra *extra;

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
         ERR("No extra for %p", bd);
         return;
    }
    _e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
    _e_border_move_resize(bd,
                          extra->orig.geom.x,
                          extra->orig.geom.y,
                          extra->orig.geom.w,
                          extra->orig.geom.h);
    e_border_layer_set(bd, extra->orig.layer);
    if (extra->orig.maximized) {
        e_border_maximize(bd, extra->orig.maximized);
        bd->maximized = extra->orig.maximized;
    }

    DBG("Change window border back to %s for %p",
        extra->orig.bordername, bd);
    change_window_border(bd, (extra->orig.bordername)
                         ? extra->orig.bordername : "default");
}

static Border_Extra *
_get_or_create_border_extra(E_Border *bd)
{
    Border_Extra *extra;

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
        extra = E_NEW(Border_Extra, 1);
        *extra = (Border_Extra) {
            .border = bd,
            .expected = {
                .x = bd->x,
                .y = bd->y,
                .w = bd->w,
                .h = bd->h,
            },
            .orig = {
                .geom = {
                    .x = bd->x,
                    .y = bd->y,
                    .w = bd->w,
                    .h = bd->h,
                },
                .layer = bd->layer,
                .stacking = bd->client.netwm.state.stacking,
                .maximized = bd->maximized,
                .bordername = eina_stringshare_add(bd->bordername),
            },
        };
        eina_hash_direct_add(_G.border_extras, &extra->border, extra);
    } else {
        extra->expected = (geom_t) {
            .x = bd->x,
            .y = bd->y,
            .w = bd->w,
            .h = bd->h,
        };
    }

    return extra;
}

/* }}} */
/* Overlays {{{*/

static void
_overlays_free_cb(void *data)
{
    Border_Extra *extra = data;

    if (extra->overlay.obj) {
        evas_object_del(extra->overlay.obj);
        extra->overlay.obj = NULL;
    }
    if (extra->overlay.popup) {
        e_object_del(E_OBJECT(extra->overlay.popup));
        extra->overlay.popup = NULL;
    }

    extra->key[0] = '\0';
}

static void
end_special_input(void)
{
    int i;

    if (_G.input_mode == INPUT_MODE_NONE)
        return;

    if (_G.overlays) {
        eina_hash_free(_G.overlays);
        _G.overlays = NULL;
    }

    if (_G.handler_key) {
        ecore_event_handler_del(_G.handler_key);
        _G.handler_key = NULL;
    }
    if (_G.action_input_win) {
        e_grabinput_release(_G.action_input_win, _G.action_input_win);
        ecore_x_window_free(_G.action_input_win);
        _G.action_input_win = 0;
    }
    if (_G.action_timer) {
        ecore_timer_del(_G.action_timer);
        _G.action_timer = NULL;
    }

    _G.focused_bd = NULL;
    _G.action_cb = NULL;

    switch(_G.input_mode) {
      case INPUT_MODE_MOVING:
        for (i = 0; i < MOVE_COUNT; i++) {
            overlay_t *overlay = &_G.move_overlays[i];

            if (overlay->obj) {
                evas_object_del(overlay->obj);
                overlay->obj = NULL;
            }
            if (overlay->popup) {
                e_object_del(E_OBJECT(overlay->popup));
                overlay->popup = NULL;
            }
        }
        break;
      case INPUT_MODE_TRANSITION:
        if (_G.transition_overlay) {
            if (_G.transition_overlay->overlay.obj) {
                evas_object_del(_G.transition_overlay->overlay.obj);
            }
            if (_G.transition_overlay->overlay.popup) {
                e_object_del(E_OBJECT(_G.transition_overlay->overlay.popup));
            }
            E_FREE(_G.transition_overlay);
            _G.transition_overlay = NULL;
        }
        break;
      default:
        break;
    }

    _G.input_mode = INPUT_MODE_NONE;
}

static Eina_Bool
overlay_key_down(void *data __UNUSED__,
		 int type __UNUSED__,
		 void *event)
{
    Ecore_Event_Key *ev = event;
    Border_Extra *extra;

    if (ev->event_window != _G.action_input_win)
        return ECORE_CALLBACK_PASS_ON;

    if (strcmp(ev->key, "Return") == 0)
        goto stop;
    if (strcmp(ev->key, "Escape") == 0)
        goto stop;
    if (strcmp(ev->key, "Backspace") == 0) {
        char *key = _G.keys;

        while (*key)
            key++;
        *key = '\0';
        return ECORE_CALLBACK_RENEW;
    }

    if (ev->key[0] && !ev->key[1] && strchr(tiling_g.config->keyhints,
                                            ev->key[1])) {
        char *key = _G.keys;

        while (*key)
            key++;
        *key++ = ev->key[0];
        *key = '\0';

        extra = eina_hash_find(_G.overlays, _G.keys);
        if (extra) {
            _G.action_cb(_G.focused_bd, extra);
        } else {
            return ECORE_CALLBACK_RENEW;
        }
    }

stop:
    end_special_input();
    return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_timeout_cb(void *data __UNUSED__)
{
    end_special_input();
    return ECORE_CALLBACK_CANCEL;
}

static void
_do_overlay(E_Border *focused_bd,
            void (*action_cb)(E_Border *, Border_Extra *),
            tiling_input_mode_t input_mode)
{
    Ecore_X_Window parent;
    int nb_win;
    int hints_len;
    int key_len;
    int n = 0;
    int nmax;
    int i;

    end_special_input();

    nb_win = get_window_count();
    if (nb_win < 2) {
        return;
    }

    _G.input_mode = input_mode;

    _G.focused_bd = focused_bd;
    _G.action_cb = action_cb;

    _G.overlays = eina_hash_string_small_new(_overlays_free_cb);

    hints_len = strlen(tiling_g.config->keyhints);
    key_len = 1;
    nmax = hints_len;
    if (hints_len < nb_win) {
        key_len = 2;
        nmax *= hints_len;
        if (hints_len * hints_len < nb_win) {
            key_len = 3;
            nmax *= hints_len;
        }
    }

    for (i = 0; i < TILING_MAX_STACKS; i++) {
        Eina_List *l;
        E_Border *bd;

        if (!_G.tinfo->stacks[i])
            break;
        EINA_LIST_FOREACH(_G.tinfo->stacks[i], l, bd) {
            if (bd != focused_bd && n < nmax) {
                Border_Extra *extra;
                Evas_Coord ew, eh;

                extra = eina_hash_find(_G.border_extras, &bd);
                if (!extra) {
                    ERR("No extra for %p", bd);
                    continue;
                }

                extra->overlay.popup = e_popup_new(bd->zone, 0, 0, 1, 1);
                if (!extra->overlay.popup)
                    continue;

                e_popup_layer_set(extra->overlay.popup, E_LAYER_NORMAL);
                extra->overlay.obj =
                    edje_object_add(extra->overlay.popup->evas);
                e_theme_edje_object_set(extra->overlay.obj,
                                        "base/theme/borders",
                                        "e/widgets/border/default/resize");

                switch (key_len) {
                  case 1:
                    extra->key[0] = tiling_g.config->keyhints[n];
                    extra->key[1] = '\0';
                    break;
                  case 2:
                    extra->key[0] = tiling_g.config->keyhints[n / hints_len];
                    extra->key[1] = tiling_g.config->keyhints[n % hints_len];
                    extra->key[2] = '\0';
                    break;
                  case 3:
                    extra->key[0] = tiling_g.config->keyhints[n / hints_len / hints_len];
                    extra->key[0] = tiling_g.config->keyhints[n / hints_len];
                    extra->key[1] = tiling_g.config->keyhints[n % hints_len];
                    extra->key[2] = '\0';
                    break;
                }
                n++;

                eina_hash_add(_G.overlays, extra->key, extra);
                edje_object_part_text_set(extra->overlay.obj,
                                          "e.text.label",
                                          extra->key);
                edje_object_size_min_calc(extra->overlay.obj, &ew, &eh);
                evas_object_move(extra->overlay.obj, 0, 0);
                evas_object_resize(extra->overlay.obj, ew, eh);
                evas_object_show(extra->overlay.obj);
                e_popup_edje_bg_object_set(extra->overlay.popup,
                                           extra->overlay.obj);

                evas_object_show(extra->overlay.obj);
                e_popup_show(extra->overlay.popup);

                e_popup_move_resize(extra->overlay.popup,
                                    (bd->x - extra->overlay.popup->zone->x) +
                                    ((bd->w - ew) / 2),
                                    (bd->y - extra->overlay.popup->zone->y) +
                                    ((bd->h - eh) / 2),
                                    ew, eh);

                e_popup_show(extra->overlay.popup);
            }
        }
    }

    /* Get input */
    parent = _G.tinfo->desk->zone->container->win;
    _G.action_input_win = ecore_x_window_input_new(parent, 0, 0, 1, 1);
    if (!_G.action_input_win) {
        end_special_input();
        return;
    }

    ecore_x_window_show(_G.action_input_win);
    if (!e_grabinput_get(_G.action_input_win, 0, _G.action_input_win)) {
        end_special_input();
        return;
    }
    _G.action_timer = ecore_timer_add(TILING_OVERLAY_TIMEOUT,
                                      _timeout_cb, NULL);

    _G.keys[0] = '\0';
    _G.handler_key = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                             overlay_key_down, NULL);
}

/* }}} */
/* Reorganize Stacks {{{*/

static void
_reorganize_stack(int stack)
{
    Eina_List *l;

    if (stack < 0 || stack >= TILING_MAX_STACKS
        || !_G.tinfo->stacks[stack])
        return;

    if (_G.tinfo->stacks[stack]->next) {
        int zx, zy, zw, zh, i = 0, count;

        e_zone_useful_geometry_get(_G.tinfo->desk->zone, &zx, &zy, &zw, &zh);

        count = eina_list_count(_G.tinfo->stacks[stack]);

        if (_G.tinfo->conf->use_rows) {
            int y, w, h, cw;

            y = _G.tinfo->pos[stack];
            cw = 0;
            w = zw / count;
            h = _G.tinfo->size[stack];

            for (l = _G.tinfo->stacks[stack]; l; l = l->next, i++) {
                E_Border *bd = l->data;
                Border_Extra *extra;
                int d = (i * 2 * zw) % count
                    - (2 * cw) % count;

                extra = eina_hash_find(_G.border_extras, &bd);
                if (!extra) {
                    ERR("No extra for %p", bd);
                    continue;
                }

                if ((bd->maximized & E_MAXIMIZE_HORIZONTAL) && count != 1) {
                    _e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
                }
                /* let's use a bresenham here */

                extra->expected.x = cw + zx;
                extra->expected.y = y;
                extra->expected.w = w + d;
                extra->expected.h = h;
                cw += extra->expected.w;

                _e_border_move_resize(bd,
                                      extra->expected.x,
                                      extra->expected.y,
                                      extra->expected.w,
                                      extra->expected.h);
            }
        } else {
            int x, w, h, ch;

            x = _G.tinfo->pos[stack];
            ch = 0;
            w = _G.tinfo->size[stack];
            h = zh / count;

            for (l = _G.tinfo->stacks[stack]; l; l = l->next, i++) {
                E_Border *bd = l->data;
                Border_Extra *extra;
                int d = (i * 2 * zh) % count
                    - (2 * ch) % count;

                extra = eina_hash_find(_G.border_extras, &bd);
                if (!extra) {
                    ERR("No extra for %p", bd);
                    continue;
                }

                if ((bd->maximized & E_MAXIMIZE_VERTICAL) && count != 1) {
                    _e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
                }
                /* let's use a bresenham here */

                extra->expected.x = x;
                extra->expected.y = ch + zy;
                extra->expected.w = w;
                extra->expected.h = h + d;
                ch += extra->expected.h;

                _e_border_move_resize(bd,
                                      extra->expected.x,
                                      extra->expected.y,
                                      extra->expected.w,
                                      extra->expected.h);
            }
        }
    } else {
        Border_Extra *extra;
        E_Border *bd = _G.tinfo->stacks[stack]->data;

        extra = eina_hash_find(_G.border_extras, &bd);
        if (!extra) {
            ERR("No extra for %p", bd);
            return;
        }

        if (_G.tinfo->conf->use_rows) {
            int x, w;

            e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                       &x, NULL, &w, NULL);

            extra->expected.x = x;
            extra->expected.y = _G.tinfo->pos[stack];
            extra->expected.w = w;
            extra->expected.h = _G.tinfo->size[stack];

            _e_border_move_resize(bd,
                                  extra->expected.x,
                                  extra->expected.y,
                                  extra->expected.w,
                                  extra->expected.h);

            _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_HORIZONTAL);
        } else {
            int y, h;

            e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                       NULL, &y, NULL, &h);

            extra->expected.x = _G.tinfo->pos[stack];
            extra->expected.y = y;
            extra->expected.w = _G.tinfo->size[stack];
            extra->expected.h = h;

            _e_border_move_resize(bd,
                                  extra->expected.x,
                                  extra->expected.y,
                                  extra->expected.w,
                                  extra->expected.h);

            _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_VERTICAL);
        }
    }
}

static void
_move_resize_stack(int stack, int delta_pos, int delta_size)
{
    Eina_List *list = _G.tinfo->stacks[stack];
    Eina_List *l;

    for (l = list; l; l = l->next) {
        E_Border *bd = l->data;
        Border_Extra *extra;

        extra = eina_hash_find(_G.border_extras, &bd);
        if (!extra) {
            ERR("No extra for %p", bd);
            continue;
        }

        if (_G.tinfo->conf->use_rows) {
            extra->expected.y += delta_pos;
            extra->expected.h += delta_size;
        } else {
            extra->expected.x += delta_pos;
            extra->expected.w += delta_size;
        }

        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
    }

    _G.tinfo->pos[stack] += delta_pos;
    _G.tinfo->size[stack] += delta_size;
}

static void
_set_stack_geometry(int stack, int pos, int size)
{
    Eina_List *l;
    for (l = _G.tinfo->stacks[stack]; l; l = l->next) {
        E_Border *bd = l->data;
        Border_Extra *extra;

        extra = eina_hash_find(_G.border_extras, &bd);
        if (!extra) {
            ERR("No extra for %p", bd);
            continue;
        }
        DBG("expected: %dx%d+%d+%d (%p)",
            extra->expected.w,
            extra->expected.h,
            extra->expected.x,
            extra->expected.y,
            bd);

        if (_G.tinfo->conf->use_rows) {
            extra->expected.y = pos;
            extra->expected.h = size;

            if (bd->maximized) {
                if (l->next && (bd->maximized & E_MAXIMIZE_HORIZONTAL))
                    _e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
                if (_G.tinfo->stacks[1] && (bd->maximized & E_MAXIMIZE_VERTICAL))
                    _e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
            }
        } else {
            extra->expected.x = pos;
            extra->expected.w = size;

            if (bd->maximized) {
                if (l->next && (bd->maximized & E_MAXIMIZE_VERTICAL))
                    _e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
                if (_G.tinfo->stacks[1] && (bd->maximized & E_MAXIMIZE_HORIZONTAL))
                    _e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
            }
        }

        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
    }
    _G.tinfo->pos[stack] = pos;
    _G.tinfo->size[stack] = size;
}

static void
_add_stack(void)
{
    int nb_borders;
    Eina_List *l;
    int i;

    if (_G.tinfo->conf->nb_stacks == TILING_MAX_STACKS)
        return;

    _G.tinfo->conf->nb_stacks++;

    if (_G.tinfo->conf->nb_stacks == 1) {
        for (l = e_border_focus_stack_get(); l; l = l->next) {
            E_Border *bd;

            bd = l->data;
            if (bd->desk == _G.tinfo->desk)
                _add_border(bd);
        }
    }
    nb_borders = get_window_count();
    if (_G.tinfo->stacks[_G.tinfo->conf->nb_stacks - 2]
    &&  nb_borders >= _G.tinfo->conf->nb_stacks)
    {
        int nb_stacks = _G.tinfo->conf->nb_stacks - 1;
        int pos, s;
        /* Add stack */

        if (_G.tinfo->conf->use_rows)
            e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                       NULL, &pos, NULL, &s);
        else
            e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                       &pos, NULL, &s, NULL);

        for (i = 0; i <= nb_stacks; i++) {
            int size = 0;

            size = s / (nb_stacks + 1 - i);

            _set_stack_geometry(i, pos, size);

            s -= size;
            pos += size;
        }
        for (i = nb_stacks - 1; i >= 0; i--) {
            if (eina_list_count(_G.tinfo->stacks[i]) == 1) {
                _G.tinfo->stacks[i+1] = _G.tinfo->stacks[i];
                _reorganize_stack(i+1);
            } else {
                E_Border *bd = eina_list_last(_G.tinfo->stacks[i])->data;

                EINA_LIST_REMOVE(_G.tinfo->stacks[i], bd);
                _reorganize_stack(i);

                _G.tinfo->stacks[i+1] = NULL;
                EINA_LIST_APPEND(_G.tinfo->stacks[i+1], bd);
                _reorganize_stack(i+1);
                return;
            }
        }
    }
}

static void
_remove_stack(void)
{
    int i;
    Eina_List *l;

    if (!_G.tinfo->conf->nb_stacks)
        return;

    _G.tinfo->conf->nb_stacks--;

    if (!_G.tinfo->conf->nb_stacks) {
        for (i = 0; i < TILING_MAX_STACKS; i++) {
            for (l = _G.tinfo->stacks[i]; l; l = l->next) {
                E_Border *bd = l->data;

                _restore_border(bd);
            }
            eina_list_free(_G.tinfo->stacks[i]);
            _G.tinfo->stacks[i] = NULL;
        }
        e_place_zone_region_smart_cleanup(_G.tinfo->desk->zone);
    } else {
        int nb_stacks = _G.tinfo->conf->nb_stacks;
        int stack = _G.tinfo->conf->nb_stacks;
        int pos, s;

        if (_G.tinfo->stacks[stack]) {
            _G.tinfo->stacks[stack-1] = eina_list_merge(
                _G.tinfo->stacks[stack-1], _G.tinfo->stacks[stack]);
            _reorganize_stack(stack-1);
        }

        if (_G.tinfo->conf->use_rows) {
            e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                       NULL, &pos, NULL, &s);
        } else {
            e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                       &pos, NULL, &s, NULL);
        }
        for (i = 0; i < nb_stacks; i++) {
            int size = 0;

            size = s / (nb_stacks - i);

            _set_stack_geometry(i, pos, size);

            s -= size;
            pos += size;
        }
    }
}

static void
_toggle_rows_cols(void)
{
    int i;
#if 0
    Eina_List *wins = NULL;
    E_Border *bd;

    _G.tinfo->conf->use_rows = !_G.tinfo->conf->use_rows;
    for (i = 0; i < TILING_MAX_STACKS; i++) {
        EINA_LIST_FREE(_G.tinfo->stacks[i], bd) {
            EINA_LIST_APPEND(wins, bd);
            _restore_border(bd);
        }
        _G.tinfo->stacks[i] = NULL;
        _G.tinfo->pos[i] = 0;
        _G.tinfo->size[i] = 0;
    }

    DBG("reinsert (use_rows: %s)",
        _G.tinfo->conf->use_rows ? "true":"false");

    EINA_LIST_FREE(wins, bd) {
        _add_border(bd);
    }
#else
    int nb_stacks = _G.tinfo->conf->nb_stacks;
    int pos, s;

    _G.tinfo->conf->use_rows = !_G.tinfo->conf->use_rows;

    if (_G.tinfo->conf->use_rows)
        e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                   NULL, &pos, NULL, &s);
    else
        e_zone_useful_geometry_get(_G.tinfo->desk->zone,
                                   &pos, NULL, &s, NULL);

    for (i = 0; i < nb_stacks; i++) {
        int size = 0;

        size = s / (nb_stacks - i);

        _set_stack_geometry(i, pos, size);

        s -= size;
        pos += size;
    }
    for (i = 0; i < nb_stacks; i++) {
        _reorganize_stack(i);
    }
#endif
}

void
change_desk_conf(struct _Config_vdesk *newconf)
{
    E_Manager *m;
    E_Container *c;
    E_Zone *z;
    E_Desk *d;
    int old_nb_stacks = 0,
        new_nb_stacks = newconf->nb_stacks;
    int i;
    Eina_List *l;

    m = e_manager_current_get();
    if (!m) return;
    c = e_container_current_get(m);
    if (!c) return;
    z = e_container_zone_number_get(c, newconf->zone_num);
    if (!z) return;
    d = e_desk_at_xy_get(z, newconf->x, newconf->y);
    if (!d) return;

    check_tinfo(d);
    if (_G.tinfo->conf) {
        old_nb_stacks = _G.tinfo->conf->nb_stacks;
        if (_G.tinfo->conf->use_rows != newconf->use_rows) {
            _G.tinfo->conf = newconf;
            _G.tinfo->conf->use_rows = !_G.tinfo->conf->use_rows;
            _toggle_rows_cols();
            return;
        }
    } else {
        newconf->nb_stacks = 0;
    }
    _G.tinfo->conf = newconf;
    _G.tinfo->conf->nb_stacks = old_nb_stacks;

    if (new_nb_stacks == old_nb_stacks)
        return;

    if (new_nb_stacks == 0) {
        for (i = 0; i < TILING_MAX_STACKS; i++) {
            for (l = _G.tinfo->stacks[i]; l; l = l->next) {
                E_Border *bd = l->data;

                _restore_border(bd);
            }
            eina_list_free(_G.tinfo->stacks[i]);
            _G.tinfo->stacks[i] = NULL;
        }
        e_place_zone_region_smart_cleanup(z);
    } else if (new_nb_stacks > old_nb_stacks) {
        for (i = new_nb_stacks; i > old_nb_stacks; i--) {
            _add_stack();
        }
    } else {
        for (i = new_nb_stacks; i < old_nb_stacks; i++) {
            _remove_stack();
        }
    }
    _G.tinfo->conf->nb_stacks = new_nb_stacks;
}

static void
_e_mod_action_add_stack_cb(E_Object   *obj __UNUSED__,
                           const char *params __UNUSED__)
{
    E_Desk *desk = get_current_desk();

    end_special_input();

    check_tinfo(desk);
    if (!_G.tinfo->conf)
        return;

    _add_stack();

    e_config_save_queue();
}

static void
_e_mod_action_remove_stack_cb(E_Object   *obj __UNUSED__,
                              const char *params __UNUSED__)
{
    E_Desk *desk = get_current_desk();

    end_special_input();

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks)
        return;

    _remove_stack();

    e_config_save_queue();
}

static void
_e_mod_action_tg_stack_cb(E_Object   *obj __UNUSED__,
                          const char *params __UNUSED__)
{
    E_Desk *desk = get_current_desk();

    end_special_input();

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks)
        return;

    _toggle_rows_cols();

    e_config_save_queue();
}


/* }}} */
/* Reorganize windows {{{*/

static void
_add_border(E_Border *bd)
{
    Border_Extra *extra;
    int stack;
    int i;

    if (!bd) {
        return;
    }
    if (is_floating_window(bd)) {
        return;
    }
    if (!is_tilable(bd)) {
        return;
    }
    if (bd->fullscreen) {
         return;
    }

    if (!_G.tinfo || !_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    extra = _get_or_create_border_extra(bd);

    /* Stack tiled window below so that winlist doesn't mix up stacking */
    e_border_layer_set(bd, E_LAYER_BELOW);

    DBG("adding %p", bd);

    if (_G.tinfo->stacks[0]) {
        DBG("got stack 0");
        if (_G.tinfo->stacks[_G.tinfo->conf->nb_stacks - 1]) {
            DBG("using last stack");
            stack = _G.tinfo->conf->nb_stacks - 1;

            if (!_G.tinfo->stacks[stack]->next) {
                _e_border_unmaximize(_G.tinfo->stacks[stack]->data,
                                    E_MAXIMIZE_BOTH);
            }
            EINA_LIST_APPEND(_G.tinfo->stacks[stack], bd);
            _reorganize_stack(stack);
            if (bd->maximized)
                _e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
        } else {
            /* Add stack */
            int nb_stacks = get_stack_count();
            int x, y, w, h;
            int pos, s, size = 0;

            DBG("add stack");

            assert((0 <= nb_stacks) && (nb_stacks < TILING_MAX_STACKS));
            e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

            if (_G.tinfo->conf->use_rows) {
                pos = y;
                s = h;
            } else {
                pos = x;
                s = w;
            }

            EINA_LIST_APPEND(_G.tinfo->stacks[nb_stacks], bd);

            for (i = 0; i < nb_stacks; i++) {

                size = s / (nb_stacks + 1 - i);

                _set_stack_geometry(i, pos, size);

                s -= size;
                pos += size;
            }

            _G.tinfo->pos[nb_stacks] = pos;
            _G.tinfo->size[nb_stacks] = size;
            if (_G.tinfo->conf->use_rows) {
                extra->expected.x = x;
                extra->expected.y = pos;
                extra->expected.w = w;
                extra->expected.h = size;
                _e_border_maximize(bd, E_MAXIMIZE_EXPAND |
                                      E_MAXIMIZE_HORIZONTAL);
            } else {
                extra->expected.x = pos;
                extra->expected.y = y;
                extra->expected.w = size;
                extra->expected.h = h;
                _e_border_maximize(bd, E_MAXIMIZE_EXPAND |
                                      E_MAXIMIZE_VERTICAL);
            }
            _e_border_move_resize(bd,
                                  extra->expected.x,
                                  extra->expected.y,
                                  extra->expected.w,
                                  extra->expected.h);

            stack = nb_stacks;
        }
    } else {
        DBG("lonely window");
        e_zone_useful_geometry_get(bd->zone,
                                   &extra->expected.x,
                                   &extra->expected.y,
                                   &extra->expected.w,
                                   &extra->expected.h);

        if (bd->maximized & E_MAXIMIZE_BOTH)
            _e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
        _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_BOTH);
        EINA_LIST_APPEND(_G.tinfo->stacks[0], bd);
        if (_G.tinfo->conf->use_rows) {
            e_zone_useful_geometry_get(bd->zone,
                                       NULL, &_G.tinfo->pos[0],
                                       NULL, &_G.tinfo->size[0]);
        } else {
            e_zone_useful_geometry_get(bd->zone,
                                       &_G.tinfo->pos[0], NULL,
                                       &_G.tinfo->size[0], NULL);
        }
        stack = 0;
    }
    DBG("expected: %dx%d+%d+%d (%p)",
        extra->expected.w,
        extra->expected.h,
        extra->expected.x,
        extra->expected.y,
        bd);
}

static void
_remove_border(E_Border *bd)
{
    int stack;
    int nb_stacks;
    int i, j;

    nb_stacks = get_stack_count();

    stack = get_stack(bd);
    if (stack < 0)
        return;

    DBG("removing %p (%d%c)", bd, stack, _G.tinfo->conf->use_rows? 'r':'c');

    EINA_LIST_REMOVE(_G.tinfo->stacks[stack], bd);
    eina_hash_del(_G.border_extras, bd, NULL);

    if (_G.tinfo->stacks[stack]) {
        _reorganize_stack(stack);
    } else {
        int nb_borders = get_window_count();

        if (nb_stacks > nb_borders) {
            int pos, s;
            /* Remove stack */

            nb_stacks--;

            assert((0 <= nb_stacks) && (nb_stacks < TILING_MAX_STACKS - 1));

            for (i = stack; i < nb_stacks; i++) {
                _G.tinfo->stacks[i] = _G.tinfo->stacks[i+1];
            }
            _G.tinfo->stacks[nb_stacks] = NULL;
            if (_G.tinfo->conf->use_rows) {
                e_zone_useful_geometry_get(bd->zone,
                                           NULL, &pos, NULL, &s);
            } else {
                e_zone_useful_geometry_get(bd->zone,
                                           &pos, NULL, &s, NULL);
            }
            for (i = 0; i < nb_stacks; i++) {
                int size;

                size = s / (nb_stacks - i);

                _set_stack_geometry(i, pos, size);

                s -= size;
                pos += size;
            }
        } else {
            for (i = stack+1; i < nb_stacks; i++) {
                if (eina_list_count(_G.tinfo->stacks[i]) > 1) {
                    for (j = stack; j < i - 1; j++) {
                        _G.tinfo->stacks[j] = _G.tinfo->stacks[j+1];
                        _reorganize_stack(j);
                    }
                    bd = _G.tinfo->stacks[i]->data;
                    EINA_LIST_REMOVE(_G.tinfo->stacks[i], bd);
                    _reorganize_stack(i);

                    _G.tinfo->stacks[i-1] = NULL;
                    EINA_LIST_APPEND(_G.tinfo->stacks[i-1], bd);
                    _reorganize_stack(i-1);
                    return;
                }
            }
            for (i = stack-1; i >= 0; i--) {
                if (eina_list_count(_G.tinfo->stacks[i]) == 1) {
                    _G.tinfo->stacks[i+1] = _G.tinfo->stacks[i];
                    _reorganize_stack(i+1);
                } else {
                    bd = eina_list_last(_G.tinfo->stacks[i])->data;
                    EINA_LIST_REMOVE(_G.tinfo->stacks[i], bd);
                    _reorganize_stack(i);

                    _G.tinfo->stacks[i+1] = NULL;
                    EINA_LIST_APPEND(_G.tinfo->stacks[i+1], bd);
                    _reorganize_stack(i+1);
                    return;
                }
            }
        }
    }
}

static void
_move_resize_border_stack(E_Border *bd, Border_Extra *extra,
                          int stack, tiling_change_t change)
{
#define _MOVE_RESIZE_BORDER_STACK(_pos, _size)                               \
    if (change == TILING_RESIZE) {                                           \
        if (stack == TILING_MAX_STACKS || !_G.tinfo->stacks[stack + 1]) {    \
            /* You're not allowed to resize */                               \
            bd->_size = extra->expected._size;                               \
        } else {                                                             \
            int delta = bd->_size - extra->expected._size;                   \
                                                                             \
            if (delta + 1 > _G.tinfo->size[stack + 1])                       \
                delta = _G.tinfo->size[stack + 1] - 1;                       \
                                                                             \
            _move_resize_stack(stack, 0, delta);                             \
            _move_resize_stack(stack + 1, delta, -delta);                    \
            extra->expected._size = bd->_size;                               \
        }                                                                    \
    } else {                                                                 \
        if (stack == 0) {                                                    \
            /* You're not allowed to move */                                 \
            bd->_pos = extra->expected._pos;                                 \
        } else {                                                             \
            int delta = bd->_pos - extra->expected._pos;                     \
                                                                             \
            if (delta + 1 > _G.tinfo->size[stack - 1])                       \
                delta = _G.tinfo->size[stack - 1] - 1;                       \
                                                                             \
            _move_resize_stack(stack, delta, -delta);                        \
            _move_resize_stack(stack - 1, 0, delta);                         \
            extra->expected._pos = bd->_pos;                                 \
        }                                                                    \
    }
    if (_G.tinfo->conf->use_rows) {
        _MOVE_RESIZE_BORDER_STACK(y, h)
    } else {
        _MOVE_RESIZE_BORDER_STACK(x, w)
    }
#undef _MOVE_RESIZE_BORDER_STACK
}

static void
_move_resize_border_in_stack(E_Border *bd, Border_Extra *extra,
                              int stack, tiling_change_t change)
{
    Eina_List *l;

    l = eina_list_data_find_list(_G.tinfo->stacks[stack], bd);
    if (!l) {
        ERR("unable to bd %p in stack %d", bd, stack);
        return;
    }

    switch (change) {
      case TILING_RESIZE:
        if (!l->next) {
            if (l->prev) {
                E_Border *prevbd = l->prev->data;
                Border_Extra *prevextra;

                prevextra = eina_hash_find(_G.border_extras, &prevbd);
                if (!prevextra) {
                    ERR("No extra for %p", prevbd);
                    return;
                }

                if (_G.tinfo->conf->use_rows) {
                    int delta;

                    delta = bd->w - extra->expected.w;
                    prevextra->expected.w -= delta;
                    extra->expected.x -= delta;
                    extra->expected.w = bd->w;
                } else {
                    int delta;

                    delta = bd->h - extra->expected.h;
                    prevextra->expected.h -= delta;
                    extra->expected.y -= delta;
                    extra->expected.h = bd->h;
                }

                _e_border_resize(prevbd,
                                 prevextra->expected.w,
                                 prevextra->expected.h);
                _e_border_move(bd,
                               extra->expected.x,
                               extra->expected.y);
            } else {
                /* You're not allowed to resize */
                _e_border_resize(bd,
                                 extra->expected.w,
                                 extra->expected.h);
            }
        } else {
            E_Border *nextbd = l->next->data;
            Border_Extra *nextextra;

            nextextra = eina_hash_find(_G.border_extras, &nextbd);
            if (!nextextra) {
                ERR("No extra for %p", nextbd);
                return;
            }

            if (_G.tinfo->conf->use_rows) {
                int min_width = MAX(nextbd->client.icccm.base_w, 1);
                int delta;

                delta = bd->w - extra->expected.w;
                if (nextextra->expected.w - delta < min_width)
                    delta = nextextra->expected.w - min_width;

                nextextra->expected.x += delta;
                nextextra->expected.w -= delta;

                extra->expected.w += delta;
            } else {
                int min_height = MAX(nextbd->client.icccm.base_h, 1);
                int delta;

                delta = bd->h - extra->expected.h;
                if (nextextra->expected.h - delta < min_height)
                    delta = nextextra->expected.h - min_height;

                nextextra->expected.y += delta;
                nextextra->expected.h -= delta;

                extra->expected.h += delta;
            }

            _e_border_move_resize(nextbd,
                                  nextextra->expected.x,
                                  nextextra->expected.y,
                                  nextextra->expected.w,
                                  nextextra->expected.h);
            _e_border_move_resize(bd,
                                  extra->expected.x,
                                  extra->expected.y,
                                  extra->expected.w,
                                  extra->expected.h);
        }
        break;
      case TILING_MOVE:
        if (!l->prev) {
            /* You're not allowed to move */
            if (_G.tinfo->conf->use_rows) {
                bd->x = extra->expected.x;
            } else {
                bd->y = extra->expected.y;
            }
            _e_border_move(bd,
                           extra->expected.x,
                           extra->expected.y);
            DBG("trying to move %p, but !l->prev", bd);
        } else {
            E_Border *prevbd = l->prev->data;
            Border_Extra *prevextra;

            prevextra = eina_hash_find(_G.border_extras, &prevbd);
            if (!prevextra) {
                ERR("No extra for %p", prevbd);
                return;
            }

            if (_G.tinfo->conf->use_rows) {
                int delta = bd->x - extra->expected.x;
                int min_width = MAX(prevbd->client.icccm.base_w, 1);

                if (prevextra->expected.w - delta < min_width)
                    delta = prevextra->expected.w - min_width;

                prevextra->expected.w += delta;

                extra->expected.x += delta;
                extra->expected.w -= delta;
            } else {
                int delta = bd->y - extra->expected.y;
                int min_height = MAX(prevbd->client.icccm.base_h, 1);

                if (prevextra->expected.h - delta < min_height)
                    delta = prevextra->expected.h - min_height;

                prevextra->expected.h += delta;

                extra->expected.y += delta;
                extra->expected.h -= delta;
            }

            _e_border_resize(prevbd,
                             prevextra->expected.w,
                             prevextra->expected.h);
            _e_border_move_resize(bd,
                                  extra->expected.x,
                                  extra->expected.y,
                                  extra->expected.w,
                                  extra->expected.h);
        }
        break;
      default:
        ERR("invalid tiling change: %d", change);
    }
}

/* }}} */
/* Toggle Floating {{{ */

static void
toggle_floating(E_Border *bd)
{
    if (!bd)
        return;
    check_tinfo(bd->desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks)
        return;

    if (EINA_LIST_IS_IN(_G.tinfo->floating_windows, bd)) {
        EINA_LIST_REMOVE(_G.tinfo->floating_windows, bd);

        _add_border(bd);
    } else {
        _remove_border(bd);
        _restore_border(bd);
        EINA_LIST_APPEND(_G.tinfo->floating_windows, bd);
    }
}

static void
_e_mod_action_toggle_floating_cb(E_Object   *obj __UNUSED__,
                                 const char *params __UNUSED__)
{
    end_special_input();

    toggle_floating(e_border_focused_get());
}

/* }}} */
/* {{{ Swap */

static void
_action_swap(E_Border *bd_1,
             Border_Extra *extra_2)
{
    Border_Extra *extra_1;
    E_Border *bd_2 = extra_2->border;
    Eina_List *l_1 = NULL,
              *l_2 = NULL;
    geom_t gt;
    unsigned int bd_2_maximized;
    int i;

    extra_1 = eina_hash_find(_G.border_extras, &bd_1);
    if (!extra_1) {
        ERR("No extra for %p", bd_1);
        return;
    }

    for (i = 0; i < TILING_MAX_STACKS; i++) {
        if ((l_1 = eina_list_data_find_list(_G.tinfo->stacks[i], bd_1))) {
            break;
        }
    }
    for (i = 0; i < TILING_MAX_STACKS; i++) {
        if ((l_2 = eina_list_data_find_list(_G.tinfo->stacks[i], bd_2))) {
            break;
        }
    }

    if (!l_1 || !l_2) {
        return;
    }

    l_1->data = bd_2;
    l_2->data = bd_1;

    gt = extra_2->expected;
    extra_2->expected = extra_1->expected;
    extra_1->expected = gt;

    bd_2_maximized = bd_2->maximized;
    if (bd_2->maximized)
        _e_border_unmaximize(bd_2, E_MAXIMIZE_BOTH);
    if (bd_1->maximized) {
        _e_border_unmaximize(bd_1, E_MAXIMIZE_BOTH);
        _e_border_maximize(bd_2, bd_1->maximized);
    }
    if (bd_2_maximized) {
        _e_border_maximize(bd_1, bd_2_maximized);
    }
    _e_border_move_resize(bd_1,
                          extra_1->expected.x,
                          extra_1->expected.y,
                          extra_1->expected.w,
                          extra_1->expected.h);
    _e_border_move_resize(bd_2,
                          extra_2->expected.x,
                          extra_2->expected.y,
                          extra_2->expected.w,
                          extra_2->expected.h);
}

static void
_e_mod_action_swap_cb(E_Object   *obj __UNUSED__,
                      const char *params __UNUSED__)
{
    E_Desk *desk;
    E_Border *focused_bd;

    desk = get_current_desk();
    if (!desk)
        return;

    focused_bd = e_border_focused_get();
    if (!focused_bd || focused_bd->desk != desk)
        return;

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    _do_overlay(focused_bd, _action_swap, INPUT_MODE_SWAPPING);
}

/* }}} */
/* Move {{{*/

static void
_check_moving_anims(const E_Border *bd, const Border_Extra *extra, int stack)
{
    Eina_List *l = NULL;
    overlay_t *overlay;
    int nb_stacks = get_stack_count();

    if (stack < 0) {
        stack = get_stack(bd);
        if (stack < 0)
            return;
    }
    if (!extra) {
        extra = eina_hash_find(_G.border_extras, &bd);
        if (!extra) {
            ERR("No extra for %p", bd);
            return;
        }
    }
    l = eina_list_data_find_list(_G.tinfo->stacks[stack], bd);
    if (!l)
        return;

    /* move left */
    overlay = &_G.move_overlays[MOVE_LEFT];
    if ((_G.tinfo->conf->use_rows && l->prev)
    || (!_G.tinfo->conf->use_rows && !_G.tinfo->stacks[TILING_MAX_STACKS -1]
        && (bd != _G.tinfo->stacks[0]->data || _G.tinfo->stacks[0]->next)))
    {
        if (overlay->popup) {
            Evas_Coord ew, eh;

            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_move(_G.move_overlays[MOVE_LEFT].popup,
                         extra->expected.x - ew/2,
                         extra->expected.y + extra->expected.h/2 - eh/2);
        } else {
            Evas_Coord ew, eh;

            overlay->popup = e_popup_new(bd->zone, 0, 0, 1, 1);
            if (!overlay->popup)
                return;

            e_popup_layer_set(overlay->popup, E_LAYER_NORMAL);
            overlay->obj = edje_object_add(overlay->popup->evas);
            _theme_edje_object_set(overlay->obj,
                                   "modules/tiling/move/left");
            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_edje_bg_object_set(overlay->popup,
                                       overlay->obj);
            evas_object_show(overlay->obj);
            e_popup_move_resize(overlay->popup,
                                extra->expected.x - ew/2
                                                  - overlay->popup->zone->x,
                                extra->expected.y + extra->expected.h/2
                                                  - eh/2
                                                  - overlay->popup->zone->y,
                                ew,
                                eh);
            evas_object_resize(overlay->obj, ew, eh);

            e_popup_show(overlay->popup);
        }
    } else if (overlay->popup) {
        if (overlay->obj) {
            evas_object_del(overlay->obj);
            overlay->obj = NULL;
        }
        if (overlay->popup) {
            e_object_del(E_OBJECT(overlay->popup));
            overlay->popup = NULL;
        }
    }

    /* move right */
    overlay = &_G.move_overlays[MOVE_RIGHT];
    if ((_G.tinfo->conf->use_rows && l->next)
    || (!_G.tinfo->conf->use_rows && (
            stack != TILING_MAX_STACKS - 1
            && ((stack == nb_stacks - 1 && _G.tinfo->stacks[stack]->next)
                || (stack != nb_stacks - 1))))) {
        if (overlay->popup) {
            Evas_Coord ew, eh;

            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_move(_G.move_overlays[MOVE_RIGHT].popup,
                         extra->expected.x + extra->expected.w - ew/2,
                         extra->expected.y + extra->expected.h/2 - eh/2);
        } else {
            Evas_Coord ew, eh;

            overlay->popup = e_popup_new(bd->zone, 0, 0, 1, 1);
            if (!overlay->popup)
                return;

            e_popup_layer_set(overlay->popup, E_LAYER_NORMAL);
            overlay->obj = edje_object_add(overlay->popup->evas);
            _theme_edje_object_set(overlay->obj,
                                   "modules/tiling/move/right");
            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_edje_bg_object_set(overlay->popup,
                                       overlay->obj);
            evas_object_show(overlay->obj);
            e_popup_move_resize(overlay->popup,
                                extra->expected.x + extra->expected.w - ew/2
                                                  - overlay->popup->zone->x,
                                extra->expected.y + extra->expected.h/2
                                                  - eh/2
                                                  - overlay->popup->zone->y,
                                ew,
                                eh);
            evas_object_resize(overlay->obj, ew, eh);

            e_popup_show(overlay->popup);
        }
    } else if (overlay->popup) {
        if (overlay->obj) {
            evas_object_del(overlay->obj);
            overlay->obj = NULL;
        }
        if (overlay->popup) {
            e_object_del(E_OBJECT(overlay->popup));
            overlay->popup = NULL;
        }
    }

    /* move up */
    overlay = &_G.move_overlays[MOVE_UP];
    if ((!_G.tinfo->conf->use_rows && l->prev)
    ||  (_G.tinfo->conf->use_rows && !_G.tinfo->stacks[TILING_MAX_STACKS -1]
        && (bd != _G.tinfo->stacks[0]->data || _G.tinfo->stacks[0]->next)))
    {
        if (overlay->popup) {
            Evas_Coord ew, eh;

            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_move(_G.move_overlays[MOVE_UP].popup,
                         extra->expected.x + extra->expected.w/2 - ew/2,
                         extra->expected.y - eh/2);
        } else {
            Evas_Coord ew, eh;

            overlay->popup = e_popup_new(bd->zone, 0, 0, 1, 1);
            if (!overlay->popup)
                return;

            e_popup_layer_set(overlay->popup, E_LAYER_NORMAL);
            overlay->obj = edje_object_add(overlay->popup->evas);
            _theme_edje_object_set(overlay->obj, "modules/tiling/move/up");
            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_edje_bg_object_set(overlay->popup,
                                       overlay->obj);
            evas_object_show(overlay->obj);
            e_popup_move_resize(overlay->popup,
                                extra->expected.x + extra->expected.w/2
                                                  - ew/2
                                                  - overlay->popup->zone->x,
                                extra->expected.y - eh/2
                                                  - overlay->popup->zone->y,
                                ew,
                                eh);
            evas_object_resize(overlay->obj, ew, eh);

            e_popup_show(overlay->popup);
        }
    } else if (overlay->popup) {
        if (overlay->obj) {
            evas_object_del(overlay->obj);
            overlay->obj = NULL;
        }
        if (overlay->popup) {
            e_object_del(E_OBJECT(overlay->popup));
            overlay->popup = NULL;
        }
    }

    /* move down */
    overlay = &_G.move_overlays[MOVE_DOWN];
    if ((!_G.tinfo->conf->use_rows && l->next)
    || (_G.tinfo->conf->use_rows && (
            stack != TILING_MAX_STACKS - 1
            && ((stack == nb_stacks - 1 && _G.tinfo->stacks[stack]->next)
                || (stack != nb_stacks - 1))))) {
        if (overlay->popup) {
            Evas_Coord ew, eh;

            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_move(_G.move_overlays[MOVE_DOWN].popup,
                         extra->expected.x + extra->expected.w/2 - ew/2,
                         extra->expected.y + extra->expected.h - eh/2);
        } else {
            Evas_Coord ew, eh;

            overlay->popup = e_popup_new(bd->zone, 0, 0, 1, 1);
            if (!overlay->popup)
                return;

            e_popup_layer_set(overlay->popup, E_LAYER_NORMAL);
            overlay->obj = edje_object_add(overlay->popup->evas);
            _theme_edje_object_set(overlay->obj,
                                   "modules/tiling/move/down");
            edje_object_size_min_calc(overlay->obj, &ew, &eh);
            e_popup_edje_bg_object_set(overlay->popup,
                                       overlay->obj);
            evas_object_show(overlay->obj);
            e_popup_move_resize(overlay->popup,
                                extra->expected.x + extra->expected.w/2
                                                  - ew/2
                                                  - overlay->popup->zone->x,
                                extra->expected.y + extra->expected.h - eh/2
                                                  - overlay->popup->zone->y,
                                ew,
                                eh);
            evas_object_resize(overlay->obj, ew, eh);

            e_popup_show(overlay->popup);
        }
    } else if (overlay->popup) {
        if (overlay->obj) {
            evas_object_del(overlay->obj);
            overlay->obj = NULL;
        }
        if (overlay->popup) {
            e_object_del(E_OBJECT(overlay->popup));
            overlay->popup = NULL;
        }
    }
}

static void
_move_up_cols(E_Border *bd, Eina_Bool check_moving_anims)
{
    E_Border *bd_1 = bd,
             *bd_2 = NULL;
    Border_Extra *extra_1 = NULL,
                 *extra_2 = NULL;
    Eina_List *l_1 = NULL,
              *l_2 = NULL;
    int stack;

    stack = get_stack(bd);
    if (stack < 0)
        return;

    if (_G.tinfo->stacks[stack]->data == bd)
        return;

    l_1 = eina_list_data_find_list(_G.tinfo->stacks[stack], bd_1);
    if (!l_1 || !l_1->prev)
        return;
    l_2 = l_1->prev;
    bd_2 = l_2->data;

    extra_1 = eina_hash_find(_G.border_extras, &bd_1);
    if (!extra_1) {
        ERR("No extra for %p", bd_1);
        return;
    }
    extra_2 = eina_hash_find(_G.border_extras, &bd_2);
    if (!extra_2) {
        ERR("No extra for %p", bd_2);
        return;
    }

    l_1->data = bd_2;
    l_2->data = bd_1;

    extra_1->expected.y = extra_2->expected.y;
    extra_2->expected.y += extra_1->expected.h;

    _e_border_move(bd_1,
                   extra_1->expected.x,
                   extra_1->expected.y);
    _e_border_move(bd_2,
                   extra_2->expected.x,
                   extra_2->expected.y);

    if (check_moving_anims)
        _check_moving_anims(bd_1, extra_1, stack);

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra_1->expected.x + extra_1->expected.w/2,
                         extra_1->expected.y + extra_1->expected.h/2);
}

static void
_move_down_cols(E_Border *bd, Eina_Bool check_moving_anims)
{
    E_Border *bd_1 = bd,
             *bd_2 = NULL;
    Border_Extra *extra_1 = NULL,
                 *extra_2 = NULL;
    Eina_List *l_1 = NULL,
              *l_2 = NULL;
    int stack;

    stack = get_stack(bd);
    if (stack < 0)
        return;

    l_1 = eina_list_data_find_list(_G.tinfo->stacks[stack], bd_1);
    if (!l_1 || !l_1->next)
        return;
    l_2 = l_1->next;
    bd_2 = l_2->data;

    extra_1 = eina_hash_find(_G.border_extras, &bd_1);
    if (!extra_1) {
        ERR("No extra for %p", bd_1);
        return;
    }
    extra_2 = eina_hash_find(_G.border_extras, &bd_2);
    if (!extra_2) {
        ERR("No extra for %p", bd_2);
        return;
    }

    l_1->data = bd_2;
    l_2->data = bd_1;

    extra_2->expected.y = extra_1->expected.y;
    extra_1->expected.y += extra_2->expected.h;

    _e_border_move(bd_1,
                   extra_1->expected.x,
                   extra_1->expected.y);
    _e_border_move(bd_2,
                   extra_2->expected.x,
                   extra_2->expected.y);

    if (check_moving_anims)
        _check_moving_anims(bd_1, extra_1, stack);

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra_1->expected.x + extra_1->expected.w/2,
                         extra_1->expected.y + extra_1->expected.h/2);
}

static void
_move_left_cols(E_Border *bd, Eina_Bool check_moving_anims)
{
    Border_Extra *extra;
    int stack;
    int i;
    int nb_stacks;

    stack = get_stack(bd);
    if (stack < 0)
        return;

    nb_stacks = get_stack_count();

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
        ERR("No extra for %p", bd);
        return;
    }

    if (stack == 0) {
        int x, y, w, h;
        int width = 0;

        if (nb_stacks >= TILING_MAX_STACKS)
            return;
        if (_G.tinfo->stacks[0]->data == bd && !_G.tinfo->stacks[0]->next)
            return;

        EINA_LIST_REMOVE(_G.tinfo->stacks[0], bd);
        for (i = TILING_MAX_STACKS - 1; i > 0; i--) {
            _G.tinfo->stacks[i] = _G.tinfo->stacks[i - 1];
        }
        _G.tinfo->stacks[0] = NULL;
        EINA_LIST_APPEND(_G.tinfo->stacks[0], bd);

        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        width = w / (nb_stacks + 1);
        _G.tinfo->pos[0] = x;
        _G.tinfo->size[0] = width;
        extra->expected.x = x;
        extra->expected.y = y;
        extra->expected.w = width;
        extra->expected.h = h;

        w -= width;
        x += width;

        for (i = 1; i <= nb_stacks; i++) {
            width = w / (nb_stacks + 1 - i);

            _set_stack_geometry(i, x, width);

            w -= width;
            x += width;
        }
        _reorganize_stack(1);

        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
        _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_VERTICAL);

        if (nb_stacks + 1 > _G.tinfo->conf->nb_stacks) {
            _G.tinfo->conf->nb_stacks = nb_stacks + 1;
            e_config_save_queue();
        }
        if (check_moving_anims)
            _check_moving_anims(bd, extra, 0);
        ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                             extra->expected.x + extra->expected.w/2,
                             extra->expected.y + extra->expected.h/2);
        return;
    }


    EINA_LIST_REMOVE(_G.tinfo->stacks[stack], bd);
    EINA_LIST_APPEND(_G.tinfo->stacks[stack - 1], bd);

    if (!_G.tinfo->stacks[stack]) {
        int x, y, w, h;

        /* Remove stack */
        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        nb_stacks--;

        assert((0 <= nb_stacks) && (nb_stacks < TILING_MAX_STACKS - 1));
        for (i = stack; i < nb_stacks; i++) {
            _G.tinfo->stacks[i] = _G.tinfo->stacks[i+1];
        }
        _G.tinfo->stacks[nb_stacks] = NULL;
        for (i = 0; i < nb_stacks; i++) {
            int width;

            width = w / (nb_stacks - i);

            _set_stack_geometry(i, x, width);

            w -= width;
            x += width;
        }
        _reorganize_stack(stack - 1);
    } else {
        _reorganize_stack(stack);
        _reorganize_stack(stack - 1);
    }

    if (check_moving_anims)
        _check_moving_anims(bd, extra, stack - 1);

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra->expected.x + extra->expected.w/2,
                         extra->expected.y + extra->expected.h/2);
}

static void
_move_right_cols(E_Border *bd, Eina_Bool check_moving_anims)
{
    int stack;
    int nb_stacks;
    Border_Extra *extra;
    int i;

    stack = get_stack(bd);
    assert(stack >= 0);
    if (stack == TILING_MAX_STACKS - 1)
        return;

    nb_stacks = get_stack_count();
    assert((0 < nb_stacks) && (nb_stacks < TILING_MAX_STACKS));
    if (stack == nb_stacks - 1 && !_G.tinfo->stacks[stack]->next)
        return;

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
        ERR("No extra for %p", bd);
        return;
    }

    EINA_LIST_REMOVE(_G.tinfo->stacks[stack], bd);
    EINA_LIST_APPEND(_G.tinfo->stacks[stack + 1], bd);

    if (_G.tinfo->stacks[stack] && _G.tinfo->stacks[stack + 1]->next) {
        _reorganize_stack(stack);
        _reorganize_stack(stack + 1);
        if (check_moving_anims)
            _check_moving_anims(bd, extra, stack + 1);
    } else
    if (_G.tinfo->stacks[stack]) {
        /* Add stack */
        int x, y, w, h;
        int width = 0;

        assert(nb_stacks < TILING_MAX_STACKS);

        _reorganize_stack(stack);

        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        for (i = 0; i < nb_stacks; i++) {
            width = w / (nb_stacks + 1 - i);

            _set_stack_geometry(i, x, width);

            w -= width;
            x += width;
        }

        _G.tinfo->pos[nb_stacks] = x;
        _G.tinfo->size[nb_stacks] = width;
        extra->expected.x = x;
        extra->expected.y = y;
        extra->expected.w = width;
        extra->expected.h = h;
        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
        _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_VERTICAL);

        if (nb_stacks + 1 > _G.tinfo->conf->nb_stacks) {
            _G.tinfo->conf->nb_stacks = nb_stacks + 1;
            e_config_save_queue();
        }
        if (check_moving_anims)
            _check_moving_anims(bd, extra, stack + 1);
    } else {
        int x, y, w, h;

        /* Remove stack */
        e_zone_useful_geometry_get(_G.tinfo->desk->zone, &x, &y, &w, &h);

        nb_stacks--;

        assert((0 <= nb_stacks) && (nb_stacks < TILING_MAX_STACKS - 1));
        for (i = stack; i < nb_stacks; i++) {
             _G.tinfo->stacks[i] = _G.tinfo->stacks[i + 1];
        }
        for (i = 0; i < nb_stacks; i++) {
            int width;

            width = w / (nb_stacks - i);

            _set_stack_geometry(i, x, width);

            w -= width;
            x += width;
        }
        _G.tinfo->stacks[nb_stacks] = NULL;
        _G.tinfo->pos[nb_stacks] = 0;
        _G.tinfo->size[nb_stacks] = 0;
        _reorganize_stack(stack);
        if (check_moving_anims)
            _check_moving_anims(bd, extra, stack);
    }

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra->expected.x + extra->expected.w/2,
                         extra->expected.y + extra->expected.h/2);
}

static void
_move_left_rows(E_Border *bd, Eina_Bool check_moving_anims)
{
    E_Border *bd_1 = bd,
             *bd_2 = NULL;
    Border_Extra *extra_1 = NULL,
                 *extra_2 = NULL;
    Eina_List *l_1 = NULL,
              *l_2 = NULL;
    int stack;

    stack = get_stack(bd);
    assert(stack >= 0);

    if (_G.tinfo->stacks[stack]->data == bd)
        return;

    l_1 = eina_list_data_find_list(_G.tinfo->stacks[stack], bd_1);
    if (!l_1 || !l_1->prev)
        return;
    l_2 = l_1->prev;
    bd_2 = l_2->data;

    extra_1 = eina_hash_find(_G.border_extras, &bd_1);
    if (!extra_1) {
        ERR("No extra for %p", bd_1);
        return;
    }
    extra_2 = eina_hash_find(_G.border_extras, &bd_2);
    if (!extra_2) {
        ERR("No extra for %p", bd_2);
        return;
    }

    l_1->data = bd_2;
    l_2->data = bd_1;

    extra_1->expected.x = extra_2->expected.x;
    extra_2->expected.x += extra_1->expected.w;

    _e_border_move(bd_1,
                   extra_1->expected.x,
                   extra_1->expected.y);
    _e_border_move(bd_2,
                   extra_2->expected.x,
                   extra_2->expected.y);

    if (check_moving_anims)
        _check_moving_anims(bd_1, extra_1, stack);

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra_1->expected.x + extra_1->expected.w/2,
                         extra_1->expected.y + extra_1->expected.h/2);
}

static void
_move_right_rows(E_Border *bd, Eina_Bool check_moving_anims)
{
    E_Border *bd_1 = bd,
             *bd_2 = NULL;
    Border_Extra *extra_1 = NULL,
                 *extra_2 = NULL;
    Eina_List *l_1 = NULL,
              *l_2 = NULL;
    int stack;

    stack = get_stack(bd);
    assert(stack >= 0);

    l_1 = eina_list_data_find_list(_G.tinfo->stacks[stack], bd_1);
    if (!l_1 || !l_1->next)
        return;
    l_2 = l_1->next;
    bd_2 = l_2->data;

    extra_1 = eina_hash_find(_G.border_extras, &bd_1);
    if (!extra_1) {
        ERR("No extra for %p", bd_1);
        return;
    }
    extra_2 = eina_hash_find(_G.border_extras, &bd_2);
    if (!extra_2) {
        ERR("No extra for %p", bd_2);
        return;
    }

    l_1->data = bd_2;
    l_2->data = bd_1;

    extra_2->expected.x = extra_1->expected.x;
    extra_1->expected.x += extra_2->expected.w;

    _e_border_move(bd_1,
                   extra_1->expected.x,
                   extra_1->expected.y);
    _e_border_move(bd_2,
                   extra_2->expected.x,
                   extra_2->expected.y);

    if (check_moving_anims)
        _check_moving_anims(bd_1, extra_1, stack);

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra_1->expected.x + extra_1->expected.w/2,
                         extra_1->expected.y + extra_1->expected.h/2);
}

static void
_move_up_rows(E_Border *bd, Eina_Bool check_moving_anims)
{
    Border_Extra *extra;
    int stack;
    int i;
    int nb_stacks;

    stack = get_stack(bd);
    assert(stack >= 0);

    nb_stacks = get_stack_count();

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
        ERR("No extra for %p", bd);
        return;
    }

    if (stack <= 0) {
        int x, y, w, h;
        int height = 0;

        if (nb_stacks >= TILING_MAX_STACKS)
            return;
        if (_G.tinfo->stacks[0]->data == bd && !_G.tinfo->stacks[0]->next)
            return;

        EINA_LIST_REMOVE(_G.tinfo->stacks[0], bd);
        for (i = TILING_MAX_STACKS - 1; i > 0; i--) {
            _G.tinfo->stacks[i] = _G.tinfo->stacks[i-1];
        }
        _G.tinfo->stacks[0] = NULL;
        EINA_LIST_APPEND(_G.tinfo->stacks[0], bd);

        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        height = h / (nb_stacks + 1);
        _G.tinfo->pos[0] = y;
        _G.tinfo->size[0] = height;
        extra->expected.x = x;
        extra->expected.y = y;
        extra->expected.w = w;
        extra->expected.h = height;

        h -= height;
        y += height;

        for (i = 1; i <= nb_stacks; i++) {
            height = h / (nb_stacks + 1 - i);

            _set_stack_geometry(i, y, height);

            h -= height;
            y += height;
        }
        _reorganize_stack(1);

        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
        _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_VERTICAL);

        if (nb_stacks + 1 > _G.tinfo->conf->nb_stacks) {
            _G.tinfo->conf->nb_stacks = nb_stacks + 1;
            e_config_save_queue();
        }
        if (check_moving_anims)
            _check_moving_anims(bd, extra, 0);
        ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                             extra->expected.x + extra->expected.w/2,
                             extra->expected.y + extra->expected.h/2);
        return;
    }


    EINA_LIST_REMOVE(_G.tinfo->stacks[stack], bd);
    EINA_LIST_APPEND(_G.tinfo->stacks[stack - 1], bd);

    if (!_G.tinfo->stacks[stack]) {
        int x, y, w, h;

        /* Remove stack */
        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        nb_stacks--;

        assert((0 <= nb_stacks) && (nb_stacks < TILING_MAX_STACKS - 1));
        for (i = stack; i < nb_stacks; i++) {
            _G.tinfo->stacks[i] = _G.tinfo->stacks[i+1];
        }
        _G.tinfo->stacks[nb_stacks] = NULL;
        for (i = 0; i < nb_stacks; i++) {
            int height;

            height = h / (nb_stacks - i);

            _set_stack_geometry(i, y, height);

            h -= height;
            y += height;
        }
        _reorganize_stack(stack - 1);
    } else {
        _reorganize_stack(stack);
        _reorganize_stack(stack - 1);
    }

    if (check_moving_anims)
        _check_moving_anims(bd, extra, stack - 1);

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra->expected.x + extra->expected.w/2,
                         extra->expected.y + extra->expected.h/2);
}

static void
_move_down_rows(E_Border *bd, Eina_Bool check_moving_anims)
{
    int stack;
    int nb_stacks;
    Border_Extra *extra;
    int i;

    stack = get_stack(bd);
    assert(stack >= 0);
    if (stack == TILING_MAX_STACKS - 1)
        return;

    nb_stacks = get_stack_count();
    assert(nb_stacks >= 1);
    if (stack == nb_stacks - 1 && !_G.tinfo->stacks[stack]->next)
        return;

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
        ERR("No extra for %p", bd);
        return;
    }

    EINA_LIST_REMOVE(_G.tinfo->stacks[stack], bd);
    EINA_LIST_APPEND(_G.tinfo->stacks[stack + 1], bd);

    if (_G.tinfo->stacks[stack] && _G.tinfo->stacks[stack + 1]->next) {
        _reorganize_stack(stack);
        _reorganize_stack(stack + 1);
        if (check_moving_anims)
            _check_moving_anims(bd, extra, stack + 1);
    } else
    if (_G.tinfo->stacks[stack]) {
        /* Add stack */
        int x, y, w, h;
        int height = 0;

        assert(nb_stacks < TILING_MAX_STACKS);

        _reorganize_stack(stack);

        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        for (i = 0; i < nb_stacks; i++) {
            height = h / (nb_stacks + 1 - i);

            _set_stack_geometry(i, y, height);

            h -= height;
            y += height;
        }

        _G.tinfo->pos[nb_stacks] = y;
        _G.tinfo->size[nb_stacks] = height;
        extra->expected.x = x;
        extra->expected.y = y;
        extra->expected.w = w;
        extra->expected.h = height;
        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);
        _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_HORIZONTAL);

        if (nb_stacks + 1 > _G.tinfo->conf->nb_stacks) {
            _G.tinfo->conf->nb_stacks = nb_stacks + 1;
            e_config_save_queue();
        }
        if (check_moving_anims)
            _check_moving_anims(bd, extra, stack + 1);
    } else {
        int x, y, w, h;

        /* Remove stack */
        e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);

        nb_stacks--;

        assert((0 <= nb_stacks) && (nb_stacks < TILING_MAX_STACKS - 1));
        for (i = stack; i < nb_stacks; i++) {
             _G.tinfo->stacks[i] = _G.tinfo->stacks[i + 1];
        }
        for (i = 0; i < nb_stacks; i++) {
            int height;

            height = h / (nb_stacks - i);

            _set_stack_geometry(i, y, height);

            h -= height;
            y += height;
        }
        _G.tinfo->stacks[nb_stacks] = NULL;
        _G.tinfo->pos[nb_stacks] = 0;
        _G.tinfo->size[nb_stacks] = 0;
        _reorganize_stack(stack);
        if (check_moving_anims)
            _check_moving_anims(bd, extra, stack);
    }

    ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                         extra->expected.x + extra->expected.w/2,
                         extra->expected.y + extra->expected.h/2);
}

static Eina_Bool
move_key_down(void *data __UNUSED__,
              int type __UNUSED__,
              void *event)
{
    Ecore_Event_Key *ev = event;

    if (ev->event_window != _G.action_input_win)
        return ECORE_CALLBACK_PASS_ON;

    /* reset timer */
    ecore_timer_delay(_G.action_timer, TILING_OVERLAY_TIMEOUT
                      - ecore_timer_pending_get(_G.action_timer));

    if ((strcmp(ev->key, "Up") == 0)
    ||  (strcmp(ev->key, "k") == 0))
    {
        if (_G.tinfo->conf->use_rows)
            _move_up_rows(_G.focused_bd, true);
        else
            _move_up_cols(_G.focused_bd, true);
        return ECORE_CALLBACK_PASS_ON;
    } else if ((strcmp(ev->key, "Down") == 0)
           ||  (strcmp(ev->key, "j") == 0))
    {
        if (_G.tinfo->conf->use_rows)
            _move_down_rows(_G.focused_bd, true);
        else
            _move_down_cols(_G.focused_bd, true);
        return ECORE_CALLBACK_PASS_ON;
    } else if ((strcmp(ev->key, "Left") == 0)
           ||  (strcmp(ev->key, "h") == 0))
    {
        if (_G.tinfo->conf->use_rows)
            _move_left_rows(_G.focused_bd, true);
        else
            _move_left_cols(_G.focused_bd, true);
        return ECORE_CALLBACK_PASS_ON;
    } else if ((strcmp(ev->key, "Right") == 0)
           ||  (strcmp(ev->key, "l") == 0))
    {
        if (_G.tinfo->conf->use_rows)
            _move_right_rows(_G.focused_bd, true);
        else
            _move_right_cols(_G.focused_bd, true);
        return ECORE_CALLBACK_PASS_ON;
    }

    if (strcmp(ev->key, "Return") == 0)
        goto stop;
    if (strcmp(ev->key, "Escape") == 0)
        goto stop; /* TODO: fallback */

    return ECORE_CALLBACK_PASS_ON;
stop:
    end_special_input();
    return ECORE_CALLBACK_DONE;
}

static void
_e_mod_action_move_direct_cb(E_Object   *obj __UNUSED__,
                             const char *params)
{
    E_Desk *desk;
    E_Border *focused_bd;

    desk = get_current_desk();
    if (!desk)
        return;

    focused_bd = e_border_focused_get();
    if (!focused_bd || focused_bd->desk != desk)
        return;

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }


    assert(params != NULL);

    switch (params[0]) {
      case 'l': /* left */
        if (_G.tinfo->conf->use_rows)
            _move_left_rows(focused_bd, false);
        else
            _move_left_cols(focused_bd, false);
        break;
      case 'r': /* right */
        if (_G.tinfo->conf->use_rows)
            _move_right_rows(focused_bd, false);
        else
            _move_right_cols(focused_bd, false);
        break;
      case 'u': /* up */
        if (_G.tinfo->conf->use_rows)
            _move_up_rows(focused_bd, false);
        else
            _move_up_cols(focused_bd, false);
        break;
      case 'd': /* down */
        if (_G.tinfo->conf->use_rows)
            _move_down_rows(focused_bd, false);
        else
            _move_down_cols(focused_bd, false);
        break;
    }
}

static void
_e_mod_action_move_cb(E_Object   *obj __UNUSED__,
                      const char *params __UNUSED__)
{
    E_Desk *desk;
    E_Border *focused_bd;
    Ecore_X_Window parent;

    desk = get_current_desk();
    if (!desk)
        return;

    focused_bd = e_border_focused_get();
    if (!focused_bd || focused_bd->desk != desk)
        return;

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    _G.focused_bd = focused_bd;

    _G.input_mode = INPUT_MODE_MOVING;

    /* Get input */
    parent = focused_bd->zone->container->win;
    _G.action_input_win = ecore_x_window_input_new(parent, 0, 0, 1, 1);
    if (!_G.action_input_win) {
        end_special_input();
        return;
    }

    ecore_x_window_show(_G.action_input_win);
    if (!e_grabinput_get(_G.action_input_win, 0, _G.action_input_win)) {
        end_special_input();
        return;
    }
    _G.action_timer = ecore_timer_add(TILING_OVERLAY_TIMEOUT,
                                      _timeout_cb, NULL);

    _G.handler_key = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                             move_key_down, NULL);
    _check_moving_anims(focused_bd, NULL, -1);
}

/* }}} */
/* Adjust Transitions {{{ */

static void
_transition_overlays_free_cb(void *data)
{
    transition_overlay_t *trov = data;

    if (trov->overlay.obj) {
        evas_object_del(trov->overlay.obj);
        trov->overlay.obj = NULL;
    }
    if (trov->overlay.popup) {
        e_object_del(E_OBJECT(trov->overlay.popup));
        trov->overlay.popup = NULL;
    }
    if (trov != _G.transition_overlay) {
        E_FREE(trov);
    }
}

static void
_transition_move_cols(tiling_move_t direction)
{
    int delta = TILING_RESIZE_STEP;
    int stack;
    E_Popup *popup = NULL;

    if (!_G.transition_overlay)
        return;

    stack = _G.transition_overlay->stack;

    if (_G.transition_overlay->bd) {
        Eina_List *l = NULL;
        E_Border *bd = _G.transition_overlay->bd,
                 *nextbd = NULL;
        Border_Extra *extra = NULL,
                     *nextextra = NULL;
        int min_height = 0;

        l = eina_list_data_find_list(_G.tinfo->stacks[stack], bd);
        if (!l) {
            ERR("unable to bd %p in stack %d", bd, stack);
            return;
        }

        extra = eina_hash_find(_G.border_extras, &bd);
        if (!extra) {
            ERR("No extra for %p", bd);
            return;
        }
        nextbd = l->next->data;
        nextextra = eina_hash_find(_G.border_extras, &nextbd);
        if (!nextextra) {
            ERR("No extra for %p", nextbd);
            return;
        }

        if (direction == MOVE_UP) {
            delta *= -1;
        }

        nextbd = l->next->data;
        min_height = MAX(nextbd->client.icccm.base_h, 1);

        if (nextextra->expected.h - delta < min_height)
            delta = nextextra->expected.h - min_height;

        nextextra->expected.y += delta;
        nextextra->expected.h -= delta;
        _e_border_move_resize(nextbd,
                              nextextra->expected.x,
                              nextextra->expected.y,
                              nextextra->expected.w,
                              nextextra->expected.h);

        extra->expected.h += delta;
        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);

        popup = _G.transition_overlay->overlay.popup;
        e_popup_move(popup, popup->x, popup->y + delta);
    } else {

        if (stack == TILING_MAX_STACKS || !_G.tinfo->stacks[stack + 1]) {
            return;
        }
        if (direction == MOVE_LEFT) {
            delta *= -1;
        }

        if (delta + 1 > _G.tinfo->size[stack + 1])
            delta = _G.tinfo->size[stack + 1] - 1;

        _move_resize_stack(stack, 0, delta);
        _move_resize_stack(stack+1, delta, -delta);

        popup = _G.transition_overlay->overlay.popup;
        e_popup_move(popup, popup->x + delta, popup->y);
    }
}

static void
_transition_move_rows(tiling_move_t direction)
{
    int delta = TILING_RESIZE_STEP;
    int stack;
    E_Popup *popup = NULL;

    if (!_G.transition_overlay)
        return;

    stack = _G.transition_overlay->stack;

    if (_G.transition_overlay->bd) {
        Eina_List *l = NULL;
        E_Border *bd = _G.transition_overlay->bd,
                 *nextbd = NULL;
        Border_Extra *extra = NULL,
                     *nextextra = NULL;
        int min_width = 0;

        l = eina_list_data_find_list(_G.tinfo->stacks[stack], bd);
        if (!l) {
            ERR("unable to bd %p in stack %d", bd, stack);
            return;
        }

        extra = eina_hash_find(_G.border_extras, &bd);
        if (!extra) {
            ERR("No extra for %p", bd);
            return;
        }
        nextbd = l->next->data;
        nextextra = eina_hash_find(_G.border_extras, &nextbd);
        if (!nextextra) {
            ERR("No extra for %p", nextbd);
            return;
        }

        if (direction == MOVE_LEFT) {
            delta *= -1;
        }

        nextbd = l->next->data;
        min_width = MAX(nextbd->client.icccm.base_w, 1);

        if (nextextra->expected.w - delta < min_width)
            delta = nextextra->expected.w - min_width;

        nextextra->expected.x += delta;
        nextextra->expected.w -= delta;
        _e_border_move_resize(nextbd,
                              nextextra->expected.x,
                              nextextra->expected.y,
                              nextextra->expected.w,
                              nextextra->expected.h);

        extra->expected.w += delta;
        _e_border_move_resize(bd,
                              extra->expected.x,
                              extra->expected.y,
                              extra->expected.w,
                              extra->expected.h);

        popup = _G.transition_overlay->overlay.popup;
        e_popup_move(popup, popup->x + delta, popup->y);
    } else {

        if (stack == TILING_MAX_STACKS || !_G.tinfo->stacks[stack + 1]) {
            return;
        }
        if (direction == MOVE_UP) {
            delta *= -1;
        }

        if (delta + 1 > _G.tinfo->size[stack + 1])
            delta = _G.tinfo->size[stack + 1] - 1;

        _move_resize_stack(stack, 0, delta);
        _move_resize_stack(stack+1, delta, -delta);

        popup = _G.transition_overlay->overlay.popup;
        e_popup_move(popup, popup->x, popup->y + delta);
    }
}

static Eina_Bool
_transition_overlay_key_down(void *data __UNUSED__,
                             int type __UNUSED__,
                             void *event)
{
    Ecore_Event_Key *ev = event;

    if (ev->event_window != _G.action_input_win)
        return ECORE_CALLBACK_PASS_ON;

    if (strcmp(ev->key, "Return") == 0)
        goto stop;
    if (strcmp(ev->key, "Escape") == 0)
        goto stop;

    /* reset timer */
    ecore_timer_delay(_G.action_timer, TILING_OVERLAY_TIMEOUT
                      - ecore_timer_pending_get(_G.action_timer));

    if (_G.transition_overlay) {
        DBG("ev->key='%s'; %p %d", ev->key,
            _G.transition_overlay->bd, _G.tinfo->conf->use_rows);
        if ((strcmp(ev->key, "Up") == 0)
            ||  (strcmp(ev->key, "k") == 0))
        {
            if (_G.transition_overlay->bd && !_G.tinfo->conf->use_rows) {
                _transition_move_cols(MOVE_UP);
                return ECORE_CALLBACK_PASS_ON;
            } else
            if (!_G.transition_overlay->bd && _G.tinfo->conf->use_rows) {
                _transition_move_rows(MOVE_UP);
                return ECORE_CALLBACK_PASS_ON;
            }
        } else
        if ((strcmp(ev->key, "Down") == 0)
        ||  (strcmp(ev->key, "j") == 0))
        {
            if (_G.transition_overlay->bd && !_G.tinfo->conf->use_rows) {
                _transition_move_cols(MOVE_DOWN);
                return ECORE_CALLBACK_PASS_ON;
            } else
            if (!_G.transition_overlay->bd && _G.tinfo->conf->use_rows) {
                _transition_move_rows(MOVE_DOWN);
                return ECORE_CALLBACK_PASS_ON;
            }
        } else
        if ((strcmp(ev->key, "Left") == 0)
        ||  (strcmp(ev->key, "h") == 0))
        {
            if (!_G.transition_overlay->bd && !_G.tinfo->conf->use_rows) {
                _transition_move_cols(MOVE_LEFT);
                return ECORE_CALLBACK_PASS_ON;
            } else
            if (_G.transition_overlay->bd && _G.tinfo->conf->use_rows) {
                _transition_move_rows(MOVE_LEFT);
                return ECORE_CALLBACK_PASS_ON;
            }
        } else
        if ((strcmp(ev->key, "Right") == 0)
        ||  (strcmp(ev->key, "l") == 0))
        {
            if (!_G.transition_overlay->bd && !_G.tinfo->conf->use_rows) {
                _transition_move_cols(MOVE_RIGHT);
                return ECORE_CALLBACK_PASS_ON;
            } else
            if (_G.transition_overlay->bd && _G.tinfo->conf->use_rows) {
                _transition_move_rows(MOVE_RIGHT);
                return ECORE_CALLBACK_PASS_ON;
            }
        }

        return ECORE_CALLBACK_RENEW;
    } else {
        if (strcmp(ev->key, "Backspace") == 0) {
            char *key = _G.keys;

            while (*key)
                key++;
            *key = '\0';
            return ECORE_CALLBACK_RENEW;
        }
        if (ev->key[0] && !ev->key[1] && strchr(tiling_g.config->keyhints,
                                                ev->key[1]))
        {
            transition_overlay_t *trov = NULL;
            E_Border *bd = NULL;
            Border_Extra *extra = NULL;
            Evas_Coord ew, eh;
            char *key = _G.keys;

            while (*key)
                key++;
            *key++ = ev->key[0];
            *key = '\0';

            trov = eina_hash_find(_G.overlays, _G.keys);
            if (!trov) {
                return ECORE_CALLBACK_RENEW;
            }
            bd = trov->bd;

            _G.transition_overlay = trov;
            eina_hash_free(_G.overlays);
            _G.overlays = NULL;

            if (bd) {
                extra = eina_hash_find(_G.border_extras, &bd);
                if (!extra) {
                    ERR("No extra for %p", bd);
                    goto stop;
                }
            }
            if (!trov->overlay.popup) {
                trov->overlay.popup = e_popup_new(_G.tinfo->desk->zone,
                                                  0, 0, 1, 1);
                e_popup_layer_set(trov->overlay.popup, E_LAYER_NORMAL);
            }
            if (!trov->overlay.obj) {
                trov->overlay.obj =
                    edje_object_add(trov->overlay.popup->evas);
            }
            if ((bd && !_G.tinfo->conf->use_rows)
            ||  (!bd && _G.tinfo->conf->use_rows)) {
                _theme_edje_object_set(trov->overlay.obj,
                                       "modules/tiling/transition/horizontal");
            } else {
                _theme_edje_object_set(trov->overlay.obj,
                                       "modules/tiling/transition/vertical");
            }

            edje_object_size_min_calc(trov->overlay.obj, &ew, &eh);
            e_popup_edje_bg_object_set(trov->overlay.popup,
                                       trov->overlay.obj);
            evas_object_show(trov->overlay.obj);
            if (bd) {
                if (_G.tinfo->conf->use_rows) {
                    e_popup_move_resize(trov->overlay.popup,
                        (extra->expected.x - trov->overlay.popup->zone->x +
                            extra->expected.w - (ew / 2)),
                        (extra->expected.y - trov->overlay.popup->zone->y +
                            ((extra->expected.h - eh) / 2)),
                        ew, eh);
                } else {
                    e_popup_move_resize(trov->overlay.popup,
                        (extra->expected.x - trov->overlay.popup->zone->x +
                            ((extra->expected.w - ew) / 2)),
                        (extra->expected.y - trov->overlay.popup->zone->y +
                            extra->expected.h - (eh / 2)),
                        ew, eh);
                }
            } else {
                if (_G.tinfo->conf->use_rows) {
                    e_popup_move_resize(trov->overlay.popup,
                                        (trov->overlay.popup->zone->w/2 - ew/2),
                                        (_G.tinfo->pos[trov->stack]
                                         + _G.tinfo->size[trov->stack]
                                         - trov->overlay.popup->zone->y - eh/2),
                                        ew, eh);
                } else {
                    e_popup_move_resize(trov->overlay.popup,
                                        (_G.tinfo->pos[trov->stack]
                                         + _G.tinfo->size[trov->stack]
                                         - trov->overlay.popup->zone->x - ew/2),
                                        (trov->overlay.popup->zone->h/2 - eh/2),
                                        ew, eh);
                }
            }
            evas_object_resize(trov->overlay.obj, ew, eh);
            e_popup_show(trov->overlay.popup);

            return ECORE_CALLBACK_RENEW;
        }
    }

stop:
    end_special_input();
    return ECORE_CALLBACK_DONE;
}

static void
_do_transition_overlay(void)
{
    int nb_transitions;
    Ecore_X_Window parent;
    int hints_len;
    int key_len;
    int n = 0;
    int nmax;
    int i;

    end_special_input();

    nb_transitions = get_transition_count();
    if (nb_transitions < 1) {
        return;
    }

    _G.input_mode = INPUT_MODE_TRANSITION;

    _G.overlays = eina_hash_string_small_new(_transition_overlays_free_cb);
    hints_len = strlen(tiling_g.config->keyhints);
    key_len = 1;
    nmax = hints_len;
    if (hints_len < nb_transitions) {
        key_len = 2;
        nmax *= hints_len;
        if (hints_len * hints_len < nb_transitions) {
            key_len = 3;
            nmax *= hints_len;
        }
    }


    for (i = 0; i < TILING_MAX_STACKS; i++) {
        Eina_List *l;
        E_Border *bd;

        if (!_G.tinfo->stacks[i])
            break;
        EINA_LIST_FOREACH(_G.tinfo->stacks[i], l, bd) {
            if (l->next && n < nmax) {
                Border_Extra *extra;
                Evas_Coord ew, eh;
                transition_overlay_t *trov;

                extra = eina_hash_find(_G.border_extras, &bd);
                if (!extra) {
                    ERR("No extra for %p", bd);
                    continue;
                }

                trov = E_NEW(transition_overlay_t, 1);

                trov->overlay.popup = e_popup_new(bd->zone, 0, 0, 1, 1);
                if (!trov->overlay.popup)
                  {
                     E_FREE(trov);
                     continue;
                  }

                e_popup_layer_set(trov->overlay.popup, E_LAYER_NORMAL);
                trov->overlay.obj = edje_object_add(trov->overlay.popup->evas);
                e_theme_edje_object_set(trov->overlay.obj,
                                        "base/theme/borders",
                                        "e/widgets/border/default/resize");

                switch (key_len) {
                  case 1:
                    trov->key[0] = tiling_g.config->keyhints[n];
                    trov->key[1] = '\0';
                    break;
                  case 2:
                    trov->key[0] = tiling_g.config->keyhints[n / hints_len];
                    trov->key[1] = tiling_g.config->keyhints[n % hints_len];
                    trov->key[2] = '\0';
                    break;
                  case 3:
                    trov->key[0] = tiling_g.config->keyhints[n / hints_len / hints_len];
                    trov->key[0] = tiling_g.config->keyhints[n / hints_len];
                    trov->key[1] = tiling_g.config->keyhints[n % hints_len];
                    trov->key[2] = '\0';
                    break;
                }
                n++;
                trov->stack = i;
                trov->bd = bd;

                eina_hash_add(_G.overlays, trov->key, trov);
                edje_object_part_text_set(trov->overlay.obj,
                                          "e.text.label",
                                          trov->key);
                edje_object_size_min_calc(trov->overlay.obj, &ew, &eh);
                evas_object_move(trov->overlay.obj, 0, 0);
                evas_object_resize(trov->overlay.obj, ew, eh);
                evas_object_show(trov->overlay.obj);
                e_popup_edje_bg_object_set(trov->overlay.popup,
                                           trov->overlay.obj);

                evas_object_show(trov->overlay.obj);
                e_popup_show(trov->overlay.popup);

                if (_G.tinfo->conf->use_rows) {
                    e_popup_move_resize(trov->overlay.popup,
                        (extra->expected.x - trov->overlay.popup->zone->x +
                            extra->expected.w - (ew / 2)),
                        (extra->expected.y - trov->overlay.popup->zone->y +
                            ((extra->expected.h - eh) / 2)),
                        ew, eh);
                } else {
                    e_popup_move_resize(trov->overlay.popup,
                        (extra->expected.x - trov->overlay.popup->zone->x +
                            ((extra->expected.w - ew) / 2)),
                        (extra->expected.y - trov->overlay.popup->zone->y +
                            extra->expected.h - (eh / 2)),
                        ew, eh);
                }

                e_popup_show(trov->overlay.popup);
            }
        }
        if (i != (TILING_MAX_STACKS - 1) &&
            _G.tinfo->stacks[i+1] && n < nmax)
        {
            Evas_Coord ew, eh;
            transition_overlay_t *trov;

            trov = E_NEW(transition_overlay_t, 1);

            trov->overlay.popup = e_popup_new(_G.tinfo->desk->zone,
                                              0, 0, 1, 1);
            if (!trov->overlay.popup)
                continue;

            e_popup_layer_set(trov->overlay.popup, E_LAYER_NORMAL);
            trov->overlay.obj = edje_object_add(trov->overlay.popup->evas);
            e_theme_edje_object_set(trov->overlay.obj,
                                    "base/theme/borders",
                                    "e/widgets/border/default/resize");

            switch (key_len) {
              case 1:
                trov->key[0] = tiling_g.config->keyhints[n];
                trov->key[1] = '\0';
                break;
              case 2:
                trov->key[0] = tiling_g.config->keyhints[n / hints_len];
                trov->key[1] = tiling_g.config->keyhints[n % hints_len];
                trov->key[2] = '\0';
                break;
              case 3:
                trov->key[0] = tiling_g.config->keyhints[n / hints_len / hints_len];
                trov->key[0] = tiling_g.config->keyhints[n / hints_len];
                trov->key[1] = tiling_g.config->keyhints[n % hints_len];
                trov->key[2] = '\0';
                break;
            }
            n++;
            trov->stack = i;
            trov->bd = NULL;

            eina_hash_add(_G.overlays, trov->key, trov);
            edje_object_part_text_set(trov->overlay.obj,
                                      "e.text.label",
                                      trov->key);
            edje_object_size_min_calc(trov->overlay.obj, &ew, &eh);
            evas_object_move(trov->overlay.obj, 0, 0);
            evas_object_resize(trov->overlay.obj, ew, eh);
            evas_object_show(trov->overlay.obj);
            e_popup_edje_bg_object_set(trov->overlay.popup,
                                       trov->overlay.obj);

            evas_object_show(trov->overlay.obj);
            e_popup_show(trov->overlay.popup);

            if (_G.tinfo->conf->use_rows) {
                e_popup_move_resize(trov->overlay.popup,
                                    (trov->overlay.popup->zone->w/2 - ew/2),
                                    (_G.tinfo->pos[trov->stack]
                                     + _G.tinfo->size[trov->stack]
                                     - trov->overlay.popup->zone->y - eh/2),
                                    ew, eh);
            } else {
                e_popup_move_resize(trov->overlay.popup,
                                    (_G.tinfo->pos[trov->stack]
                                     + _G.tinfo->size[trov->stack]
                                     - trov->overlay.popup->zone->x - ew/2),
                                    (trov->overlay.popup->zone->h/2 - eh/2),
                                    ew, eh);
            }

            e_popup_show(trov->overlay.popup);
        }
    }

    /* Get input */
    parent = _G.tinfo->desk->zone->container->win;
    _G.action_input_win = ecore_x_window_input_new(parent, 0, 0, 1, 1);
    if (!_G.action_input_win) {
        end_special_input();
        return;
    }

    ecore_x_window_show(_G.action_input_win);
    if (!e_grabinput_get(_G.action_input_win, 0, _G.action_input_win)) {
        end_special_input();
        return;
    }
    _G.action_timer = ecore_timer_add(TILING_OVERLAY_TIMEOUT,
                                      _timeout_cb, NULL);

    _G.keys[0] = '\0';
    _G.handler_key = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                             _transition_overlay_key_down,
                                             NULL);
}

static void
_e_mod_action_adjust_transitions(E_Object   *obj __UNUSED__,
                                 const char *params __UNUSED__)
{
    E_Desk *desk;

    desk = get_current_desk();
    if (!desk)
        return;

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    _do_transition_overlay();
}

/* }}} */
/* Go {{{ */

static Eina_Bool
_warp_timer(void *data __UNUSED__)
{
    if (_G.warp_timer) {
        double spd = TILING_WRAP_SPEED;

        _G.old_warp_x = _G.warp_x;
        _G.old_warp_y = _G.warp_y;
        _G.warp_x = (_G.warp_x * (1.0 - spd)) + (_G.warp_to_x * spd);
        _G.warp_y = (_G.warp_y * (1.0 - spd)) + (_G.warp_to_y * spd);

        ecore_x_pointer_warp(_G.tinfo->desk->zone->container->win,
                             _G.warp_x, _G.warp_y);

        if (abs(_G.warp_x - _G.old_warp_x) <= 1
        &&  abs(_G.warp_y - _G.old_warp_y) <= 1) {
            _G.warp_timer = NULL;
            return ECORE_CALLBACK_CANCEL;
        }

        return ECORE_CALLBACK_RENEW;
    }
    _G.warp_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void
_action_go(E_Border *data __UNUSED__,
           Border_Extra *extra_2)
{
    E_Border *bd = extra_2->border;

    _G.warp_to_x = bd->x + (bd->w / 2);
    _G.warp_to_y = bd->y + (bd->h / 2);
    ecore_x_pointer_xy_get(_G.tinfo->desk->zone->container->win,
                           &_G.warp_x, &_G.warp_y);
    e_border_focus_latest_set(bd);
    _G.warp_timer = ecore_timer_add(0.01, _warp_timer, NULL);
}

static void
_e_mod_action_go_cb(E_Object   *obj __UNUSED__,
                    const char *params __UNUSED__)
{
    E_Desk *desk;

    desk = get_current_desk();
    if (!desk)
        return;

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    _do_overlay(NULL, _action_go, INPUT_MODE_GOING);
}

/* }}} */
/* Send to a corner {{{ */

static void
_e_mod_action_send_cb(E_Object   *obj __UNUSED__,
                      const char *params) EINA_ARG_NONNULL(2)
{
    E_Desk *desk;
    E_Border *bd;
    int x, y, w, h;
    geom_t g;

    assert(params != NULL);

    desk = get_current_desk();
    if (!desk)
        return;

    bd = e_border_focused_get();
    if (!bd || bd->desk != desk)
        return;

    if (!is_tilable(bd))
        return;

    check_tinfo(desk);
    if (!_G.tinfo->conf)
        return;

    /* Fill initial values if not already done */
    _get_or_create_border_extra(bd);

    if (!tiling_g.config->show_titles) {
        if ((bd->bordername && strcmp(bd->bordername, "pixel"))
        ||  !bd->bordername)
        {
            change_window_border(bd, "pixel");
        }
    }

    if (bd->maximized)
        _e_border_unmaximize(bd, E_MAXIMIZE_BOTH);

    /* add to floating */
    if (!EINA_LIST_IS_IN(_G.tinfo->floating_windows, bd)) {
        _remove_border(bd);
        EINA_LIST_APPEND(_G.tinfo->floating_windows, bd);
    }

    e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);
    g.w = w / 2;
    g.h = h / 2;

    if (params[0] == 'n') {
        g.y = 0;
        if (params[1] == 'w') {
            /* NW */
            g.x = 0;
        } else {
            /* NE */
            g.x = w / 2;
        }
    } else {
        g.y = h / 2;
        if (params[1] == 'w') {
            /* SW */
            g.x = 0;
        } else {
            /* SE */
            g.x = w / 2;
        }
    }
    _e_border_move_resize(bd, g.x, g.y, g.w, g.h);
}

/* }}} */
/* Hooks {{{*/

static void
_pre_border_assign_hook(void *data __UNUSED__,
                        E_Border *bd)
{
    if (tiling_g.config->show_titles)
        return;

    if (!bd) {
        return;
    }

    check_tinfo(bd->desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    if (is_floating_window(bd)) {
        return;
    }

    if (!is_tilable(bd)) {
        return;
    }

    if (bd->fullscreen) {
         return;
    }

    /* Fill initial values if not already done */
    _get_or_create_border_extra(bd);

    if ((bd->bordername && strcmp(bd->bordername, "pixel"))
    ||  !bd->bordername)
    {
        change_window_border(bd, "pixel");
    }
}

static void _move_or_resize(E_Border *bd)
{
    Border_Extra *extra;
    int stack = -1;

    if (!bd) {
        return;
    }

    check_tinfo(bd->desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    stack = get_stack(bd);
    if (stack < 0) {
        return;
    }

    DBG("Resize: %p / '%s' / '%s', (%d,%d), changes(size=%d, position=%d, border=%d)"
        " g:%dx%d+%d+%d bdname:'%s' (stack:%d%c) maximized:%s fs:%s",
        bd, bd->client.icccm.title, bd->client.netwm.name,
        bd->desk->x, bd->desk->y,
        bd->changes.size, bd->changes.pos, bd->changes.border,
        bd->w, bd->h, bd->x, bd->y, bd->bordername,
        stack, _G.tinfo->conf->use_rows? 'r':'c',
        (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH", bd->fullscreen? "true": "false");

    extra = eina_hash_find(_G.border_extras, &bd);
    if (!extra) {
        ERR("No extra for %p", bd);
        return;
    }

    DBG("expected: %dx%d+%d+%d",
        extra->expected.w,
        extra->expected.h,
        extra->expected.x,
        extra->expected.y);
    DBG("delta:%dx%d,%d,%d. step:%dx%d. base:%dx%d",
        bd->w - extra->expected.w, bd->h - extra->expected.h,
        bd->x - extra->expected.x, bd->y - extra->expected.y,
        bd->client.icccm.step_w, bd->client.icccm.step_h,
        bd->client.icccm.base_w, bd->client.icccm.base_h);

    if (stack == 0 && !_G.tinfo->stacks[1] && !_G.tinfo->stacks[0]->next) {
        if (bd->maximized) {
            extra->expected.x = bd->x;
            extra->expected.y = bd->y;
            extra->expected.w = bd->w;
            extra->expected.h = bd->h;
        } else {
            /* TODO: what if a window doesn't want to be maximized? */
            _e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
            _e_border_maximize(bd, E_MAXIMIZE_EXPAND | E_MAXIMIZE_BOTH);
        }
    }
    if (bd->x == extra->expected.x && bd->y == extra->expected.y
    &&  bd->w == extra->expected.w && bd->h == extra->expected.h)
    {
        return;
    }
    if (bd->maximized) {

        if (_G.tinfo->conf->use_rows) {
            if (bd->maximized & E_MAXIMIZE_VERTICAL) {
                 _e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
                 _e_border_move_resize(bd,
                                       extra->expected.x,
                                       extra->expected.y,
                                       extra->expected.w,
                                       extra->expected.h);
            }
            if (bd->maximized & E_MAXIMIZE_HORIZONTAL
            && eina_list_count(_G.tinfo->stacks[stack]) > 1) {
                 _e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
                 _e_border_move_resize(bd,
                                       extra->expected.x,
                                       extra->expected.y,
                                       extra->expected.w,
                                       extra->expected.h);
            }
        } else {
            if (bd->maximized & E_MAXIMIZE_HORIZONTAL) {
                 _e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
                 _e_border_move_resize(bd,
                                       extra->expected.x,
                                       extra->expected.y,
                                       extra->expected.w,
                                       extra->expected.h);
            }
            if (bd->maximized & E_MAXIMIZE_VERTICAL
            && eina_list_count(_G.tinfo->stacks[stack]) > 1) {
                 _e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
                 _e_border_move_resize(bd,
                                       extra->expected.x,
                                       extra->expected.y,
                                       extra->expected.w,
                                       extra->expected.h);
            }
        }
    }

    if (abs(extra->expected.w - bd->w) >= MAX(bd->client.icccm.step_w, 1)) {
        if (_G.tinfo->conf->use_rows)
            _move_resize_border_in_stack(bd, extra, stack, TILING_RESIZE);
        else
            _move_resize_border_stack(bd, extra, stack, TILING_RESIZE);
    }
    if (abs(extra->expected.h - bd->h) >= MAX(bd->client.icccm.step_h, 1)) {
        if (_G.tinfo->conf->use_rows)
            _move_resize_border_stack(bd, extra, stack, TILING_RESIZE);
        else
            _move_resize_border_in_stack(bd, extra, stack, TILING_RESIZE);
    }
    if (extra->expected.x != bd->x) {
        if (_G.tinfo->conf->use_rows)
            _move_resize_border_in_stack(bd, extra, stack, TILING_MOVE);
        else
            _move_resize_border_stack(bd, extra, stack, TILING_MOVE);
    }
    if (extra->expected.y != bd->y) {
        if (_G.tinfo->conf->use_rows)
            _move_resize_border_stack(bd, extra, stack, TILING_MOVE);
        else
            _move_resize_border_in_stack(bd, extra, stack, TILING_MOVE);
    }

    if (_G.input_mode == INPUT_MODE_MOVING
    &&  bd == _G.focused_bd) {
        _check_moving_anims(bd, extra, stack);
    }
}

static Eina_Bool
_resize_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Resize *event)
{
    E_Border *bd = event->border;

    _move_or_resize(bd);

    return true;
}

static Eina_Bool
_move_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Move*event)
{
    E_Border *bd = event->border;

    _move_or_resize(bd);

    return true;
}

static Eina_Bool
_add_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Add *event)
{
    E_Border *bd = event->border;
    int stack = -1;

    if (_G.input_mode != INPUT_MODE_NONE
    &&  _G.input_mode != INPUT_MODE_MOVING
    &&  _G.input_mode != INPUT_MODE_TRANSITION)
    {
        end_special_input();
    }

    check_tinfo(bd->desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return true;
    }

    if (!is_tilable(bd)) {
        return true;
    }

    stack = get_stack(bd);
    if (stack >= 0) {
        return true;
    }

    DBG("Add: %p / '%s' / '%s', (%d,%d), changes(size=%d, position=%d, border=%d)"
        " g:%dx%d+%d+%d bdname:'%s' (stack:%d%c) maximized:%s fs:%s",
        bd, bd->client.icccm.title, bd->client.netwm.name,
        bd->desk->x, bd->desk->y,
        bd->changes.size, bd->changes.pos, bd->changes.border,
        bd->w, bd->h, bd->x, bd->y, bd->bordername,
        stack, _G.tinfo->conf->use_rows? 'r':'c',
        (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH", bd->fullscreen? "true": "false");

    _add_border(bd);

    return true;
}

static Eina_Bool
_remove_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Remove *event)
{
    E_Border *bd = event->border;

    end_special_input();

    if (_G.currently_switching_desktop)
        return true;

    check_tinfo(bd->desk);
    if (!_G.tinfo->conf)
        return true;

    if (EINA_LIST_IS_IN(_G.tinfo->floating_windows, bd)) {
        EINA_LIST_REMOVE(_G.tinfo->floating_windows, bd);
        return true;
    }

    _remove_border(bd);

    return true;
}

static bool
_iconify_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Iconify *event)
{
    E_Border *bd = event->border;

    DBG("iconify hook: %p", bd);

    end_special_input();

    if (bd->deskshow)
        return true;

    check_tinfo(bd->desk);
    if (!_G.tinfo->conf)
        return true;

    if (EINA_LIST_IS_IN(_G.tinfo->floating_windows, bd)) {
        return true;
    }

    _remove_border(bd);

    return true;
}

static bool
_uniconify_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Uniconify *event)
{
    E_Border *bd = event->border;
    int stack = -1;

    if (_G.input_mode != INPUT_MODE_NONE
    &&  _G.input_mode != INPUT_MODE_MOVING
    &&  _G.input_mode != INPUT_MODE_TRANSITION)
    {
        end_special_input();
    }

    if (bd->deskshow)
        return true;

    check_tinfo(bd->desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return true;
    }

    if (!is_tilable(bd)) {
        return true;
    }

    stack = get_stack(bd);
    if (stack >= 0) {
        return true;
    }
    _add_border(bd);

    return true;
}

static Eina_Bool
_stick_hook(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
    DBG("TODO");
    return true;
}

static Eina_Bool
_unstick_hook(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
    DBG("TODO");
    return true;
}

static Eina_Bool
_desk_show_hook(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
    _G.currently_switching_desktop = 0;

    end_special_input();

    return true;
}

static Eina_Bool
_desk_before_show_hook(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
    end_special_input();

    _G.currently_switching_desktop = 1;

    return true;
}

static bool
_desk_set_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Border_Desk_Set *ev)
{
    DBG("%p: from (%d,%d) to (%d,%d)", ev->border,
        ev->desk->x, ev->desk->y,
        ev->border->desk->x, ev->border->desk->y);

    end_special_input();

    check_tinfo(ev->desk);
    if (!_G.tinfo->conf) {
        return true;
    }

    if (is_floating_window(ev->border)) {
        EINA_LIST_REMOVE(_G.tinfo->floating_windows, ev->border);
    } else {
        if (get_stack(ev->border) >= 0) {
            _remove_border(ev->border);
            _restore_border(ev->border);
        }
    }

    check_tinfo(ev->border->desk);
    if (!_G.tinfo->conf) {
        return true;
    }

    if (get_stack(ev->border) < 0)
        _add_border(ev->border);

    return true;
}

static bool
_container_resize_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Container_Resize *ev)
{
    Eina_List *l;
    E_Zone *zone;
    int x, y, i;

    EINA_LIST_FOREACH(ev->container->zones, l, zone) {
        for (x = 0; x < zone->desk_x_count; x++) {
            for (y = 0; y < zone->desk_y_count; y++) {
                E_Desk *desk = zone->desks[x + (y * zone->desk_x_count)];
                Eina_List *wins = NULL;
                E_Border *bd;

                check_tinfo(desk);
                if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
                    continue;
                }

                for (i = 0; i < TILING_MAX_STACKS; i++) {
                    EINA_LIST_FREE(_G.tinfo->stacks[i], bd) {
                        EINA_LIST_APPEND(wins, bd);
                        _restore_border(bd);
                    }
                    _G.tinfo->stacks[i] = NULL;
                    _G.tinfo->pos[i] = 0;
                    _G.tinfo->size[i] = 0;
                }

                EINA_LIST_FREE(wins, bd) {
                    _add_border(bd);
                }
            }
        }
    }

    return true;
}

/* }}} */
/* Module setup {{{*/

static void
_clear_info_hash(void *data)
{
    Tiling_Info *ti = data;
    int i;

    eina_list_free(ti->floating_windows);
    for (i = 0; i < TILING_MAX_STACKS; i++) {
        eina_list_free(ti->stacks[i]);
        ti->stacks[i] = NULL;
    }
    E_FREE(ti);
}

static void
_clear_border_extras(void *data)
{
    Border_Extra *extra = data;

    eina_stringshare_del(extra->orig.bordername);

    E_FREE(extra);
}

EAPI E_Module_Api e_modapi =
{
    E_MODULE_API_VERSION,
    "Tiling"
};

EAPI void *
e_modapi_init(E_Module *m)
{
    E_Desk *desk;
    Eina_List *l;

    tiling_g.module = m;

    if (tiling_g.log_domain < 0) {
        tiling_g.log_domain = eina_log_domain_register("tiling", NULL);
        if (tiling_g.log_domain < 0) {
            EINA_LOG_CRIT("could not register log domain 'tiling'");
        }
    }

    _G.info_hash = eina_hash_pointer_new(_clear_info_hash);
    _G.border_extras = eina_hash_pointer_new(_clear_border_extras);

    _G.pre_border_assign_hook = e_border_hook_add(
        E_BORDER_HOOK_EVAL_PRE_BORDER_ASSIGN,
        (void (*)(void*,void*))_pre_border_assign_hook, NULL);

#define HANDLER(_h, _e, _f)                                   \
    _h = ecore_event_handler_add(E_EVENT_##_e,                \
                                 (Ecore_Event_Handler_Cb) _f, \
                                 NULL);

    HANDLER(_G.handler_border_resize, BORDER_RESIZE, _resize_hook);
    HANDLER(_G.handler_border_move, BORDER_MOVE, _move_hook);
    HANDLER(_G.handler_border_add, BORDER_ADD, _add_hook);
    HANDLER(_G.handler_border_remove, BORDER_REMOVE, _remove_hook);

    HANDLER(_G.handler_border_iconify, BORDER_ICONIFY, _iconify_hook);
    HANDLER(_G.handler_border_uniconify, BORDER_UNICONIFY, _uniconify_hook);
    HANDLER(_G.handler_border_stick, BORDER_STICK, _stick_hook);
    HANDLER(_G.handler_border_unstick, BORDER_UNSTICK, _unstick_hook);

    HANDLER(_G.handler_desk_show, DESK_SHOW, _desk_show_hook);
    HANDLER(_G.handler_desk_before_show, DESK_BEFORE_SHOW, _desk_before_show_hook);
    HANDLER(_G.handler_desk_set, BORDER_DESK_SET, _desk_set_hook);
    HANDLER(_G.handler_container_resize, CONTAINER_RESIZE, _container_resize_hook);
#undef HANDLER

#define ACTION_ADD(_act, _cb, _title, _value, _params, _example, _editable)  \
    {                                                                        \
        E_Action *_action = _act;                                            \
        const char *_name = _value;                                          \
        if ((_action = e_action_add(_name))) {                               \
            _action->func.go = _cb;                                          \
            e_action_predef_name_set(N_("Tiling"), _title, _name,            \
                                     _params, _example, _editable);          \
        }                                                                    \
    }

    /* Module's actions */
    ACTION_ADD(_G.act_togglefloat, _e_mod_action_toggle_floating_cb,
               N_("Toggle floating"), "toggle_floating",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_addstack, _e_mod_action_add_stack_cb,
               N_("Add a stack"), "add_stack",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_removestack, _e_mod_action_remove_stack_cb,
               N_("Remove a stack"), "remove_stack",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_tg_stack, _e_mod_action_tg_stack_cb,
               N_("Toggle between rows and columns"), "tg_cols_rows",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_swap, _e_mod_action_swap_cb,
               N_("Swap a window with an other"), "swap",
               NULL, NULL, 0);

    ACTION_ADD(_G.act_move, _e_mod_action_move_cb,
               N_("Move window"), "move",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_move_left, _e_mod_action_move_direct_cb,
               N_("Move window to the left"), "move_left",
               "left", NULL, 0);
    ACTION_ADD(_G.act_move_right, _e_mod_action_move_direct_cb,
               N_("Move window to the right"), "move_right",
               "right", NULL, 0);
    ACTION_ADD(_G.act_move_up, _e_mod_action_move_direct_cb,
               N_("Move window up"), "move_up",
               "up", NULL, 0);
    ACTION_ADD(_G.act_move_down, _e_mod_action_move_direct_cb,
               N_("Move window down"), "move_down",
               "down", NULL, 0);

    ACTION_ADD(_G.act_adjusttransitions, _e_mod_action_adjust_transitions,
               N_("Adjust transitions"), "adjust_transitions",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_go, _e_mod_action_go_cb,
               N_("Focus a particular window"), "go",
               NULL, NULL, 0);

    ACTION_ADD(_G.act_send_ne, _e_mod_action_send_cb,
               N_("Send to upper right corner"), "send_ne",
               "ne", NULL, 0);
    ACTION_ADD(_G.act_send_nw, _e_mod_action_send_cb,
               N_("Send to upper left corner"), "send_nw",
               "nw", NULL, 0);
    ACTION_ADD(_G.act_send_se, _e_mod_action_send_cb,
               N_("Send to lower right corner"), "send_se",
               "se", NULL, 0);
    ACTION_ADD(_G.act_send_sw, _e_mod_action_send_cb,
               N_("Send to lower left corner"), "send_sw",
               "sw", NULL, 0);
#undef ACTION_ADD

    /* Configuration entries */
    snprintf(_G.edj_path, sizeof(_G.edj_path),"%s/e-module-tiling.edj",
             e_module_dir_get(m));
    e_configure_registry_category_add("windows", 50, _("Windows"), NULL,
                                      "preferences-system-windows");
    e_configure_registry_item_add("windows/tiling", 150, _("Tiling"),
                                  NULL, _G.edj_path,
                                  e_int_config_tiling_module);

    /* Configuration itself */
    _G.config_edd = E_CONFIG_DD_NEW("Tiling_Config", Config);
    _G.vdesk_edd = E_CONFIG_DD_NEW("Tiling_Config_VDesk",
                                   struct _Config_vdesk);
    E_CONFIG_VAL(_G.config_edd, Config, tile_dialogs, INT);
    E_CONFIG_VAL(_G.config_edd, Config, show_titles, INT);
    E_CONFIG_VAL(_G.config_edd, Config, keyhints, STR);

    E_CONFIG_LIST(_G.config_edd, Config, vdesks, _G.vdesk_edd);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, x, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, y, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, zone_num, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, nb_stacks, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, use_rows, INT);

    tiling_g.config = e_config_domain_load("module.tiling", _G.config_edd);
    if (!tiling_g.config) {
        tiling_g.config = E_NEW(Config, 1);
        tiling_g.config->tile_dialogs = 1;
        tiling_g.config->show_titles = 1;
    }
    if (!tiling_g.config->keyhints)
        tiling_g.config->keyhints = strdup(tiling_g.default_keyhints);
    else
        tiling_g.config->keyhints = strdup(tiling_g.config->keyhints);

    E_CONFIG_LIMIT(tiling_g.config->tile_dialogs, 0, 1);
    E_CONFIG_LIMIT(tiling_g.config->show_titles, 0, 1);

    for (l = tiling_g.config->vdesks; l; l = l->next) {
        struct _Config_vdesk *vd;

        vd = l->data;

        E_CONFIG_LIMIT(vd->nb_stacks, 0, TILING_MAX_STACKS);
        E_CONFIG_LIMIT(vd->use_rows, 0, 1);
    }

    desk = get_current_desk();
    _G.tinfo = _initialize_tinfo(desk);

    _G.input_mode = INPUT_MODE_NONE;
    _G.currently_switching_desktop = 0;
    _G.action_cb = NULL;

    return m;
}

static void
_disable_desk(E_Desk *desk)
{
    Eina_List *l;
    int i;

    check_tinfo(desk);
    if (!_G.tinfo->conf || !_G.tinfo->conf->nb_stacks) {
        return;
    }

    for (i = 0; i < TILING_MAX_STACKS; i++) {
        for (l = _G.tinfo->stacks[i]; l; l = l->next) {
            E_Border *bd = l->data;

            _restore_border(bd);
        }
        eina_list_free(_G.tinfo->stacks[i]);
        _G.tinfo->stacks[i] = NULL;
    }
}

static void
_disable_all_tiling(void)
{
    Eina_List *l, *ll, *lll;
    E_Manager *man;
    E_Container *con;
    E_Zone *zone;
    E_Desk *desk;
    int x, y;

    EINA_LIST_FOREACH(e_manager_list(), l, man) {
        EINA_LIST_FOREACH(man->containers, ll, con) {
            EINA_LIST_FOREACH(con->zones, lll, zone) {
                for (x = 0; x < zone->desk_x_count; x++) {
                    for (y = 0; y < zone->desk_y_count; y++) {
                        desk = zone->desks[x + (y * zone->desk_x_count)];

                        _disable_desk(desk);
                    }
                }
                e_place_zone_region_smart_cleanup(zone);
            }
        }
    }
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
    _disable_all_tiling();

    if (tiling_g.log_domain >= 0) {
        eina_log_domain_unregister(tiling_g.log_domain);
        tiling_g.log_domain = -1;
    }

    if (_G.pre_border_assign_hook) {
        e_border_hook_del(_G.pre_border_assign_hook);
        _G.pre_border_assign_hook = NULL;
    }

#define FREE_HANDLER(x)              \
    if (x) {                         \
        ecore_event_handler_del(x);  \
        x = NULL;                    \
    }
    FREE_HANDLER(_G.handler_border_resize);
    FREE_HANDLER(_G.handler_border_move);
    FREE_HANDLER(_G.handler_border_add);
    FREE_HANDLER(_G.handler_border_remove);

    FREE_HANDLER(_G.handler_border_iconify);
    FREE_HANDLER(_G.handler_border_uniconify);
    FREE_HANDLER(_G.handler_border_stick);
    FREE_HANDLER(_G.handler_border_unstick);

    FREE_HANDLER(_G.handler_desk_show);
    FREE_HANDLER(_G.handler_desk_before_show);
    FREE_HANDLER(_G.handler_desk_set);
#undef FREE_HANDLER


#define ACTION_DEL(act, title, value)                        \
    if (act) {                                               \
        e_action_predef_name_del("Tiling", title);           \
        e_action_del(value);                                 \
        act = NULL;                                          \
    }
    ACTION_DEL(_G.act_togglefloat, "Toggle floating", "toggle_floating");
    ACTION_DEL(_G.act_addstack, "Add a stack", "add_stack");
    ACTION_DEL(_G.act_removestack, "Remove a stack", "remove_stack");
    ACTION_DEL(_G.act_tg_stack, "Toggle between rows and columns", "tg_cols_rows");
    ACTION_DEL(_G.act_swap, "Swap a window with an other", "swap");

    ACTION_DEL(_G.act_move, "Move window", "move");
    ACTION_DEL(_G.act_move_left, "Move window to the left", "move_left");
    ACTION_DEL(_G.act_move_right, "Move window to the right", "move_right");
    ACTION_DEL(_G.act_move_up, "Move window up", "move_up");
    ACTION_DEL(_G.act_move_down, "Move window down", "move_down");

    ACTION_DEL(_G.act_adjusttransitions, "Adjust transitions",
               "adjust_transitions");
    ACTION_DEL(_G.act_go, "Focus a particular window", "go");

    ACTION_DEL(_G.act_send_ne, "Send to upper right corner", "send_ne");
    ACTION_DEL(_G.act_send_nw, "Send to upper left corner", "send_nw");
    ACTION_DEL(_G.act_send_se, "Send to lower right corner", "send_se");
    ACTION_DEL(_G.act_send_sw, "Send to lower left corner", "send_sw");
#undef ACTION_DEL

    e_configure_registry_item_del("windows/tiling");
    e_configure_registry_category_del("windows");

    end_special_input();

    free(tiling_g.config->keyhints);
    E_FREE(tiling_g.config);
    E_CONFIG_DD_FREE(_G.config_edd);
    E_CONFIG_DD_FREE(_G.vdesk_edd);

    tiling_g.module = NULL;

    eina_hash_free(_G.info_hash);
    _G.info_hash = NULL;

    eina_hash_free(_G.border_extras);
    _G.border_extras = NULL;

    _G.tinfo = NULL;

    return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
    e_config_domain_save("module.tiling", _G.config_edd, tiling_g.config);

    return true;
}
/* }}} */
