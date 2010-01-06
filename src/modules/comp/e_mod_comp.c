#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "config.h"

typedef struct _Comp     Comp;
typedef struct _Comp_Win Comp_Win;

struct _Comp
{
   Ecore_X_Window  win;
   Ecore_Evas     *ee;
   Ecore_X_Window  ee_win;
   Evas           *evas;
   E_Manager      *man;
   Eina_Inlist    *wins;
};

struct _Comp_Win
{
   EINA_INLIST;
   
   Comp           *c; // parent compositor
   Ecore_X_Window  win; // raw window - for menus etc.
   E_Border       *bd; // if its a border - later
   E_Popup        *pop; // if its a popup - later
   int             x, y, w, h; // geometry
   int             border; // border width
   Ecore_X_Pixmap  pixmap; // the compositing pixmap
   Ecore_X_Damage  damage; // damage region
   Ecore_X_Visual  vis;
   int             depth;
   Evas_Object    *obj; // shadow object
   Ecore_X_Image  *xim; // x image - software fallback
   // fixme: evas object its mapped to
   // fixme: shape rects
   // ...
   Eina_Bool       visible : 1; // is visible
   Eina_Bool       input_only : 1; // is input_only
};

static Eina_List *handlers = NULL;
static Eina_List *compositors = NULL;
static Eina_Hash *windows = NULL;
static Eina_Hash *damages = NULL;

static void _e_mod_comp_win_damage(Comp_Win *cw, int x, int y, int w, int h);

static Comp *
_e_mod_comp_find(Ecore_X_Window root)
{
   Eina_List *l;
   Comp *c;
   
   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (c->man->root == root) return c;
     }
   return NULL;
}

static Comp_Win *
_e_mod_comp_win_find(Ecore_X_Window win)
{
   Eina_List *l;
   Comp_Win *cw;

   cw = eina_hash_find(windows, e_util_winid_str_get(win));
   return cw;
}

static Comp_Win *
_e_mod_comp_win_damage_find(Ecore_X_Damage damage)
{
   Comp_Win *cw;
   
   cw = eina_hash_find(damages, e_util_winid_str_get(damage));
   return cw;
}

static Comp_Win *
_e_mod_comp_win_add(Comp *c, Ecore_X_Window win)
{
   Ecore_X_Window_Attributes att;
   Comp_Win *cw;
   
   cw = calloc(1, sizeof(Comp_Win));
   if (!cw) return NULL;
   cw->win = win;
   // FIXME: check if bd or pop - track
   c->wins = eina_inlist_append(c->wins, EINA_INLIST_GET(cw));
   cw->c = c;
   ecore_x_window_attributes_get(cw->win, &att);
   cw->input_only = att.input_only;
   cw->vis = att.visual;
   cw->depth = att.depth;
   eina_hash_add(windows, e_util_winid_str_get(cw->win), cw);
   if (!cw->input_only)
     {
        cw->damage = ecore_x_damage_new(cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
        cw->obj = evas_object_image_filled_add(c->evas);
        evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(cw->obj, 0);
     }
   return cw;
}

static void
_e_mod_comp_win_del(Comp_Win *cw)
{
   if (cw->xim)
     {
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
     }
   if (cw->obj)
     {
        evas_object_del(cw->obj);
     }
   eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);
   if (cw->damage)
     {
        eina_hash_del(damages, e_util_winid_str_get(cw->damage), cw);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   free(cw);
}

static void
_e_mod_comp_win_show(Comp_Win *cw)
{
   if (cw->visible) return;
   cw->visible = 1;
   if (cw->input_only) return;
   if (!cw->pixmap)
     {
        ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h);
     }
   evas_object_show(cw->obj);
}

static void
_e_mod_comp_win_hide(Comp_Win *cw)
{
   if (!cw->visible) return;
   cw->visible = 0;
   if (cw->input_only) return;
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->pixmap = 0;
     }
   evas_object_hide(cw->obj);
}

static void
_e_mod_comp_win_raise_above(Comp_Win *cw, Comp_Win *cw2)
{
   if (cw->input_only) return;
   evas_object_stack_above(cw->obj, cw2->obj);
}

static void
_e_mod_comp_win_raise(Comp_Win *cw)
{
   if (cw->input_only) return;
   evas_object_raise(cw->obj);
}

static void
_e_mod_comp_win_lower(Comp_Win *cw)
{
   if (cw->input_only) return;
   evas_object_lower(cw->obj);
}

static void
_e_mod_comp_win_configure(Comp_Win *cw, int x, int y, int w, int h, int border)
{
   if (!((x == cw->c) && (y == cw->y)))
     {
        cw->x = x;
        cw->y = y;
        evas_object_move(cw->obj, cw->x, cw->y);
     }
   if (!((w == cw->w) && (h == cw->h)))
     {
//        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
//        ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        if (cw->pixmap)
          {
             ecore_x_pixmap_free(cw->pixmap);
             cw->pixmap = 0;
          }
        cw->w = w;
        cw->h = h;
        evas_object_resize(cw->obj, cw->w, cw->h);
        if (cw->xim)
          {
             evas_object_image_data_set(cw->obj, NULL);
             ecore_x_image_free(cw->xim);
             cw->xim = NULL;
          }
     }
   cw->border = border;
   if (cw->input_only) return;
}

static void
_e_mod_comp_win_damage(Comp_Win *cw, int x, int y, int w, int h)
{
   Ecore_X_Region parts;
   
   if (cw->input_only) return;
   parts = ecore_x_region_new(NULL, 0);
   ecore_x_damage_subtract(cw->damage, 0, parts);
   ecore_x_region_free(parts);
   // FIXME: add rect to queue for this compwin
   if (!cw->pixmap)
     cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
   
   if (!cw->xim)
     {
        if (cw->xim = ecore_x_image_new(cw->w, cw->h, cw->vis, cw->depth))
          {
             x = 0;
             y = 0;
             w = cw->w;
             h = cw->h;
          }
     }
   if (cw->xim)
     {
        unsigned int *pix;
        
        ecore_x_image_get(cw->xim, cw->pixmap, x, y, x, y, w, h);
        pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
        if (0)
          {
             int i;
             
             for (i = 0; i < (w * h); i++)
               pix[i] = i | 0xff0000ff;
          }
        evas_object_image_data_set(cw->obj, pix);
        evas_object_image_size_set(cw->obj, cw->w, cw->h);
        evas_object_image_data_update_add(cw->obj, x, y, w, h);
     }
}

//////////////////////////////////////////////////////////////////////////

static int
_e_mod_comp_create(void *data, int type, void *event)
{ // do nothing. only worry about maps
   Ecore_X_Event_Window_Create *ev = event;
   Comp_Win *cw;
   Comp *c = _e_mod_comp_find(ev->parent);
   if (!c) return 1;
   if (_e_mod_comp_win_find(ev->win)) return 1;
   if (c->win == ev->win) return 1;
   if (c->ee_win == ev->win) return 1;
   cw = _e_mod_comp_win_add(c, ev->win);
   _e_mod_comp_win_configure(cw, ev->x, ev->y, ev->w, ev->h, ev->border);
   return 1;
}

static int
_e_mod_comp_destroy(void *data, int type, void *event)
{ // do nothing. only worry about unmaps
   Ecore_X_Event_Window_Destroy *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
   if (ev->event_win != cw->c->man->root) return 1;
   _e_mod_comp_win_del(cw);
   return 1;
}

static int
_e_mod_comp_show(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Show *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   if (cw->visible) return 1;
   _e_mod_comp_win_show(cw);
   return 1;
}

static int
_e_mod_comp_hide(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Hide *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   if (!cw->visible) return 1;
   _e_mod_comp_win_hide(cw);
   return 1;
}

static int
_e_mod_comp_reparent(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
   if (ev->parent != cw->c->man->root)
     _e_mod_comp_win_del(cw);
   return 1;
}

static int
_e_mod_comp_configure(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
   if (ev->event_win != cw->c->man->root) return 1;

   if (ev->abovewin == 0)
     {
        if (EINA_INLIST_GET(cw)->prev) _e_mod_comp_win_lower(cw);
     }
   else
     {
        Comp_Win *cw2 = _e_mod_comp_win_find(ev->abovewin);

        if (cw2)
          {
             Comp_Win *cw3 = (Comp_Win *)(EINA_INLIST_GET(cw)->prev);
             
             if (cw3 != cw2) _e_mod_comp_win_raise_above(cw, cw2);
          }
     }
   
   if (!((cw->x == ev->x) && (cw->y == ev->y) &&
         (cw->w == ev->h) && (cw->h == ev->h) &&
         (cw->border == ev->border)))
     {
        _e_mod_comp_win_configure(cw, ev->x, ev->y, ev->w, ev->h, ev->border);
     }
   return 1;
}

static int
_e_mod_comp_stack(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Stack *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
   if (ev->event_win != cw->c->man->root) return 1;
   if (ev->detail == ECORE_X_WINDOW_STACK_ABOVE) _e_mod_comp_win_raise(cw);
   else _e_mod_comp_win_lower(cw);
   return 1;
}

static int
_e_mod_comp_property(void *data, int type, void *event)
{ // later
   Ecore_X_Event_Window_Property *ev = event;
   return 1;
}

static int
_e_mod_comp_message(void *data, int type, void *event)
{ // later
   Ecore_X_Event_Client_Message *ev = event;
   return 1;
}

static int
_e_mod_comp_shape(void *data, int type, void *event)
{ // later
   Ecore_X_Event_Window_Shape *ev = event;
   return 1;
}

//////////////////////////////////////////////////////////////////////////

static int
_e_mod_comp_damage(void *data, int type, void *event)
{
   Ecore_X_Event_Damage *ev = event;
   Comp_Win *cw = _e_mod_comp_win_damage_find(ev->damage);
   if (!cw) return 1;
   _e_mod_comp_win_damage(cw, ev->area.x, ev->area.y, ev->area.width, ev->area.height); 
   return 1;
}

static Comp *
_e_mod_comp_add(E_Manager *man)
{
   Comp *c;
   Ecore_X_Window *wins;
   int i, num;
   
   c = calloc(1, sizeof(Comp));
   if (!c) return NULL;
   c->man = man;
   c->win = ecore_x_composite_render_window_enable(man->root);
   c->ee = ecore_evas_software_x11_new(NULL, c->win, 0, 0, man->w, man->h);
   c->evas = ecore_evas_get(c->ee);
   ecore_evas_show(c->ee);
   
     {
        Evas_Object *o;
        
        o = evas_object_rectangle_add(c->evas);
        evas_object_color_set(o, 255, 128, 20, 200);
        evas_object_resize(o, 500, 300);
        evas_object_show(o);
     }
   
   c->ee_win = ecore_evas_software_x11_window_get(c->ee);
//   ecore_x_composite_unredirect_subwindows
//     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);

   wins = ecore_x_window_children_get(c->man->root, &num);
   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             Comp_Win *cw;
             int x, y, w, h, border;
             
             cw = _e_mod_comp_win_add(c, wins[i]);
             if (!cw) continue;
             ecore_x_window_geometry_get(cw->win, &x, &y, &w, &h);
             border = ecore_x_window_border_width_get(cw->win);
             if (wins[i] == c->win) continue;
             _e_mod_comp_win_configure(cw, x, y, w, h, border);
             if (ecore_x_window_visible_get(wins[i]))
               {
                  _e_mod_comp_win_show(cw);
               }
          }
        free(wins);
     }
   
   return c;
}

//////////////////////////////////////////////////////////////////////////

Eina_Bool
e_mod_comp_init(void)
{
   Eina_List *l;
   E_Manager *man;

   windows = eina_hash_string_superfast_new(NULL);
   damages = eina_hash_string_superfast_new(NULL);
   
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,    _e_mod_comp_create,    NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,   _e_mod_comp_destroy,   NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW,      _e_mod_comp_show,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE,      _e_mod_comp_hide,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT,  _e_mod_comp_reparent,  NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, _e_mod_comp_configure, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK,     _e_mod_comp_stack,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,  _e_mod_comp_property,  NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,   _e_mod_comp_message,   NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,     _e_mod_comp_shape,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_DAMAGE_NOTIFY,    _e_mod_comp_damage,    NULL));

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        compositors = eina_list_append(compositors, _e_mod_comp_add(man));
     }
   
   return 1;
}

void
e_mod_comp_shutdown(void)
{
   Comp *c;
   
   EINA_LIST_FREE(compositors, c)
     {
        if (c)
          {
             ecore_evas_free(c->ee);
//             ecore_x_composite_unredirect_subwindows
//               (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
             ecore_x_composite_render_window_disable(c->win);
             free(c);
          }
     }
   
   E_FREE_LIST(handlers, ecore_event_handler_del);
   
   eina_hash_free(damages);
   eina_hash_free(windows);
}
