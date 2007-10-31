#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* i measure a mere 9% speedup using mmx for simd sums. :(
 * need to detect mmx capbale cpu's to enable this though
#define MMX 1
*/

/* TODO List:
 * 
 * * bug in shadow_x < 0 and shadow_y < 0 needs to be fixed (not urgent though)
 * * add alpha-pixel only pixel space to image objects in evas and make use of it to save cpu and ram
 * * look into mmx for the blur function...
 * * handle other shadow pos cases where we cant use 4 objects (3 or 2).
 */

/* module private routines */
static Dropshadow *_ds_init(E_Module *m);
static void        _ds_shutdown(Dropshadow *ds);
static void        _ds_container_shapes_add(Dropshadow *ds, E_Container *con);
static void        _ds_shape_change(void *data, E_Container_Shape *es, E_Container_Shape_Change ch);
static Shadow     *_ds_shadow_find(Dropshadow *ds, E_Container_Shape *es);
static Shadow     *_ds_shadow_add(Dropshadow *ds, E_Container_Shape *es);
static void        _ds_shadow_obj_clear(Shadow *sh);
static void        _ds_shadow_obj_init(Shadow *sh);
static void        _ds_shadow_obj_init_rects(Shadow *sh, Evas_List *rects);
static void        _ds_shadow_obj_shutdown(Shadow *sh);
static void        _ds_shadow_del(Shadow *sh);
static void        _ds_shadow_show(Shadow *sh);
static void        _ds_shadow_hide(Shadow *sh);
static void        _ds_shadow_move(Shadow *sh, int x, int y);
static void        _ds_shadow_resize(Shadow *sh, int w, int h);
static void        _ds_shadow_shaperects(Shadow *sh);
static int         _ds_shadow_reshape(void *data);
static void        _ds_edge_scan(Shpix *sp, Tilebuf *tb, int bsz, int x1, int y1, int x2, int y2);
static void        _ds_shadow_recalc(Shadow *sh);
static void        _ds_config_darkness_set(Dropshadow *ds, double v);
static void        _ds_config_shadow_xy_set(Dropshadow *ds, int x, int y);
static void        _ds_config_blur_set(Dropshadow *ds, int blur);
static void        _ds_config_quality_set(Dropshadow *ds, int q);
static void        _ds_blur_init(Dropshadow *ds);
static double      _ds_gauss_int(double x);
static void        _ds_gauss_blur_h(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int q, int rx, int ry, int rxx, int ryy);
static void        _ds_gauss_blur_v(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int q, int rx, int ry, int rxx, int ryy);
static Shpix      *_ds_shpix_new(int w, int h);
static void        _ds_shpix_free(Shpix *sp);
static void        _ds_shpix_fill(Shpix *sp, int x, int y, int w, int h, unsigned char val);
static void        _ds_shpix_blur(Shpix *sp, int x, int y, int w, int h, unsigned char *blur_lut, int blur_size, int q);
static void        _ds_shpix_blur_rects(Shpix *sp, Evas_List *rects, unsigned char *blur_lut, int blur_size, int q);
static void        _ds_shpix_object_set(Shpix *sp, Evas_Object *o, int x, int y, int w, int h);
static void        _ds_shared_free(Dropshadow *ds);
static void        _ds_shared_use(Dropshadow *ds, Shadow *sh);
static void        _ds_shared_unuse(Dropshadow *ds);
static Shstore    *_ds_shstore_new(Shpix *sp, int x, int y, int w, int h);
static void        _ds_shstore_free(Shstore *st);
static void        _ds_shstore_object_set(Shstore *st, Evas_Object *o);
static void        _ds_object_unset(Evas_Object *o);
static int         _tilebuf_x_intersect(Tilebuf *tb, int x, int w, int *x1, int *x2, int *x1_fill, int *x2_fill);
static int         _tilebuf_y_intersect(Tilebuf *tb, int y, int h, int *y1, int *y2, int *y1_fill, int *y2_fill);
static int         _tilebuf_intersect(int tsize, int tlen, int tnum, int x, int w, int *x1, int *x2, int *x1_fill, int *x2_fill);
static void        _tilebuf_setup(Tilebuf *tb);
static Tilebuf    *_tilebuf_new(int w, int h);
static void        _tilebuf_free(Tilebuf *tb);
static void        _tilebuf_set_tile_size(Tilebuf *tb, int tw, int th);
static int         _tilebuf_add_redraw(Tilebuf *tb, int x, int y, int w, int h);
static Evas_List  *_tilebuf_get_render_rects(Tilebuf *tb);
static void        _tilebuf_free_render_rects(Evas_List *rects);

#define TILE(tb, x, y) ((tb)->tiles.tiles[((y) * (tb)->tiles.w) + (x)])

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "Dropshadow"
};

EAPI E_Module *dropshadow_mod = NULL;

EAPI void *
e_modapi_init(E_Module *m)
{
   Dropshadow *ds;
   char buf[4096];
   
   ds = _ds_init(m);
   snprintf(buf, sizeof(buf), "%s/e-module-dropshadow.edj", e_module_dir_get(m));
   e_configure_registry_category_add("appearance", 10, _("Appearance"), NULL, "enlightenment/appearance");
   e_configure_registry_item_add("appearance/dropshadow", 150, _("Dropshadow"), NULL, buf, e_int_config_dropshadow_module);
   
   dropshadow_mod = m;
   
   return ds;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Dropshadow *ds;
   
   e_configure_registry_item_del("appearance/dropshadow");
   e_configure_registry_category_del("appearance");
   
   ds = m->data;
   if (ds) 
     {
	if (ds->config_dialog) 
	  {
	     e_object_del(E_OBJECT(ds->config_dialog));
	     ds->config_dialog = NULL;
	  }
	_ds_shutdown(ds);
     }   
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   Dropshadow *ds;
   
   ds = m->data;
   e_config_domain_save("module.dropshadow", ds->conf_edd, ds->conf);
   return 1;
}

/* module private routines */
static Dropshadow *
_ds_init(E_Module *m)
{
   Dropshadow *ds;
   Evas_List *managers, *l, *l2;
   
   ds = calloc(1, sizeof(Dropshadow));
   if (!ds) return  NULL;

   ds->module = m;
   ds->conf_edd = E_CONFIG_DD_NEW("Dropshadow_Config", Config);
#undef T
#undef D
#define T Config
#define D ds->conf_edd
   E_CONFIG_VAL(D, T, shadow_x, INT);
   E_CONFIG_VAL(D, T, shadow_y, INT);
   E_CONFIG_VAL(D, T, blur_size, INT);
   E_CONFIG_VAL(D, T, quality, INT);
   E_CONFIG_VAL(D, T, shadow_darkness, DOUBLE);
   
   ds->conf = e_config_domain_load("module.dropshadow", ds->conf_edd);
   if (!ds->conf)
     {
	ds->conf = E_NEW(Config, 1);
	ds->conf->shadow_x = 4;
	ds->conf->shadow_y = 4;
	ds->conf->blur_size = 10;
	ds->conf->quality = 2;
	ds->conf->shadow_darkness = 0.5;
     }
   E_CONFIG_LIMIT(ds->conf->shadow_x, -200, 200);
   E_CONFIG_LIMIT(ds->conf->shadow_y, -200, 200);
   E_CONFIG_LIMIT(ds->conf->blur_size, 1, 120);
   E_CONFIG_LIMIT(ds->conf->quality, 1, 4);
   E_CONFIG_LIMIT(ds->conf->shadow_darkness, 0.0, 1.0);
   /* special - must be power of 2 */
   if (ds->conf->quality == 3) ds->conf->quality = 4;
   
   if (ds->conf->shadow_x >= ds->conf->blur_size)
     ds->conf->shadow_x = ds->conf->blur_size - 1;
   if (ds->conf->shadow_y >= ds->conf->blur_size)
     ds->conf->shadow_y = ds->conf->blur_size - 1;
   
   _ds_blur_init(ds);
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     ds->cons = evas_list_append(ds->cons, con);
	     e_container_shape_change_callback_add(con, _ds_shape_change, ds);
	     _ds_container_shapes_add(ds, con);
	  }
     }
   ds->idler_before = e_main_idler_before_add(_ds_shadow_reshape, ds, 0);
   return ds;
}

static void
_ds_shutdown(Dropshadow *ds)
{
   free(ds->conf);
   E_CONFIG_DD_FREE(ds->conf_edd);
   while (ds->cons)
     {
	E_Container *con;
	
	con = ds->cons->data;
	ds->cons = evas_list_remove_list(ds->cons, ds->cons);
	e_container_shape_change_callback_del(con, _ds_shape_change, ds);
     }
   while (ds->shadows)
     {
	Shadow *sh;
	
	sh = ds->shadows->data;
	_ds_shadow_del(sh);
     }
   if (ds->idler_before) e_main_idler_before_del(ds->idler_before);
   if (ds->table.gauss) free(ds->table.gauss);
   if (ds->table.gauss2) free(ds->table.gauss2);
   _ds_shared_free(ds);
   free(ds);
}

static void
_ds_container_shapes_add(Dropshadow *ds, E_Container *con)
{
   Evas_List *shapes, *l;
   
   shapes = e_container_shape_list_get(con);
   for (l = shapes; l; l = l->next)
     {
	E_Container_Shape *es;
	Shadow *sh;
	int x, y, w, h;
	
	es = l->data;
	sh = _ds_shadow_add(ds, es);
	e_container_shape_geometry_get(es, &x, &y, &w, &h);
	_ds_shadow_move(sh, x, y);
	_ds_shadow_resize(sh, w, h);
	if (es->visible) _ds_shadow_show(sh);
     }
}

static void
_ds_shape_change(void *data, E_Container_Shape *es, E_Container_Shape_Change ch)
{
   Dropshadow *ds;
   Shadow *sh;
   int x, y, w, h;
   
   ds = data;
   switch (ch)
     {
      case E_CONTAINER_SHAPE_ADD:
	_ds_shadow_add(ds, es);
	break;
      case E_CONTAINER_SHAPE_DEL:
	sh = _ds_shadow_find(ds, es);
	if (sh) _ds_shadow_del(sh);
	break;
      case E_CONTAINER_SHAPE_SHOW:
	sh = _ds_shadow_find(ds, es);
	if (sh) _ds_shadow_show(sh);
	break;
      case E_CONTAINER_SHAPE_HIDE:
	sh = _ds_shadow_find(ds, es);
	if (sh) _ds_shadow_hide(sh);
	break;
      case E_CONTAINER_SHAPE_MOVE:
	sh = _ds_shadow_find(ds, es);
	e_container_shape_geometry_get(es, &x, &y, &w, &h);
	if (sh) _ds_shadow_move(sh, x, y);
	break;
      case E_CONTAINER_SHAPE_RESIZE:
	sh = _ds_shadow_find(ds, es);
	e_container_shape_geometry_get(es, &x, &y, &w, &h);
	if (sh) _ds_shadow_resize(sh, w, h);
	break;
      case E_CONTAINER_SHAPE_RECTS:
	sh = _ds_shadow_find(ds, es);
	if (sh) _ds_shadow_shaperects(sh);
	break;
      default:
	break;
     }
}

static Shadow *
_ds_shadow_find(Dropshadow *ds, E_Container_Shape *es)
{
   Evas_List *l;
   
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	
	sh = l->data;
	if (sh->shape == es) return sh;
     }
   return NULL;
}

static Shadow *
_ds_shadow_add(Dropshadow *ds, E_Container_Shape *es)
{
   Shadow *sh;
   
   sh = calloc(1, sizeof(Shadow));
   ds->shadows = evas_list_append(ds->shadows, sh);
   sh->ds = ds;
   sh->shape = es;
   e_object_ref(E_OBJECT(sh->shape));
   return sh;
}

static void
_ds_shadow_obj_init(Shadow *sh)
{
   E_Container *con;
   int i;
   
   if (sh->initted) return;
   sh->initted = 1;
   con = e_container_shape_container_get(sh->shape);
   for (i = 0; i < 4; i++)
     {
	sh->object[i] = evas_object_image_add(con->bg_evas);
	evas_object_image_alpha_set(sh->object[i], 1);
	evas_object_layer_set(sh->object[i], 10);
	evas_object_pass_events_set(sh->object[i], 1);
	evas_object_move(sh->object[i], 0, 0);
	evas_object_resize(sh->object[i], 0, 0);
	evas_object_color_set(sh->object[i],
			      0, 0, 0, 
			      255 * sh->ds->conf->shadow_darkness);
	if (sh->visible)
	  evas_object_show(sh->object[i]);
     }
}

static void
_ds_shadow_obj_init_rects(Shadow *sh, Evas_List *rects)
{
   E_Container *con;
   Evas_List *l;
   
   if (sh->initted) return;
   sh->initted = 1;
   con = e_container_shape_container_get(sh->shape);
   for (l = rects; l; l = l->next)
     {
	E_Rect *r;
	Evas_Object *o;
	Shadow_Object *so;
	
	r = l->data;
	so = calloc(1, sizeof(Shadow_Object));
	if (so)
	  {
	     o = evas_object_image_add(con->bg_evas);
	     evas_object_image_alpha_set(o, 1);
	     evas_object_layer_set(o, 10);
	     evas_object_pass_events_set(o, 1);
	     evas_object_move(o, r->x, r->y);
	     evas_object_resize(o, r->w, r->h);
	     evas_object_color_set(o,
				   0, 0, 0,
				   255 * sh->ds->conf->shadow_darkness);
	     if (sh->visible)
	       evas_object_show(o);
	     so->obj = o;
	     so->x = r->x;
	     so->y = r->y;
	     so->w = r->w;
	     so->h = r->h;
	     sh->object_list = evas_list_append(sh->object_list, so);
	  }
     }
}

static void
_ds_shadow_obj_clear(Shadow *sh)
{
   int i;
   Evas_List *l;
   
   for (i = 0; i < 4; i++)
     {
	if (sh->object[i])
	  _ds_object_unset(sh->object[i]);
     }
   if (sh->use_shared)
     {
	_ds_shared_unuse(sh->ds);
	sh->use_shared = 0;
     }
   for (l = sh->object_list; l; l = l->next)
     {
	Shadow_Object *so;
	
	so = l->data;
	_ds_object_unset(so->obj);
     }
}


static void
_ds_shadow_obj_shutdown(Shadow *sh)
{
   int i;
   
   if (!sh->initted) return;
   sh->initted = 0;
   for (i = 0; i < 4; i++)
     {
	if (sh->object[i])
	  {
	     _ds_object_unset(sh->object[i]);
	     evas_object_del(sh->object[i]);
	     sh->object[i] = NULL;
	  }
     }
   if (sh->use_shared)
     {
	_ds_shared_unuse(sh->ds);
	sh->use_shared = 0;
     }
   while (sh->object_list)
     {
	Shadow_Object *so;
	
	so = sh->object_list->data;
	evas_object_del(so->obj);
	free(so);
	sh->object_list = evas_list_remove_list(sh->object_list, sh->object_list);
     }
}

static void
_ds_shadow_del(Shadow *sh)
{
   if (sh->use_shared)
     {
	_ds_shared_unuse(sh->ds);
	sh->use_shared = 0;
     }
   sh->ds->shadows = evas_list_remove(sh->ds->shadows, sh);
   _ds_shadow_obj_shutdown(sh);
   e_object_unref(E_OBJECT(sh->shape));
   free(sh);
}

static void
_ds_shadow_show(Shadow *sh)
{
   Evas_List *l;
   
   _ds_shadow_obj_init(sh);
   if (!sh->object_list)
     {
	if (sh->square)
	  {
	     int i;
	     
	     for (i = 0; i < 4; i++)
	       evas_object_show(sh->object[i]);
	  }
	else
	  {
	     evas_object_show(sh->object[0]);
	  }
     }
   else
     {
	for (l = sh->object_list; l; l = l->next)
	  {
	     Shadow_Object *so;
	     
	     so = l->data;
	     evas_object_show(so->obj);
	  }
     }
   sh->visible = 1;
}

static void
_ds_shadow_hide(Shadow *sh)
{
   Evas_List *l;
   
   _ds_shadow_obj_init(sh);
   if (!sh->object_list)
     {
	if (sh->square)
	  {
	     int i;
	     
	     for (i = 0; i < 4; i++)
	       evas_object_hide(sh->object[i]);
	  }
	else
	  {
	     evas_object_hide(sh->object[0]);
	  }
     }
   else
     {
	for (l = sh->object_list; l; l = l->next)
	  {
	     Shadow_Object *so;
	     
	     so = l->data;
	     evas_object_hide(so->obj);
	  }
     }
   sh->visible = 0;
}

static void
_ds_shadow_move(Shadow *sh, int x, int y)
{
   Evas_List *l;
   
   _ds_shadow_obj_init(sh);
   sh->x = x;
   sh->y = y;
   if (!sh->object_list)
     {
	if ((sh->square) && (!sh->toosmall))
	  {
	     evas_object_move(sh->object[0],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y + sh->ds->conf->shadow_y - sh->ds->conf->blur_size);
	     evas_object_move(sh->object[1],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y);
	     evas_object_move(sh->object[2],
			      sh->x + sh->w, 
			      sh->y);
	     evas_object_move(sh->object[3],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y + sh->h);
	  }
	else
	  {
	     evas_object_move(sh->object[0],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y + sh->ds->conf->shadow_y - sh->ds->conf->blur_size);
	  }
     }
   else
     {
	for (l = sh->object_list; l; l = l->next)
	  {
	     Shadow_Object *so;
	     
	     so = l->data;
	     evas_object_move(so->obj,
			      sh->x + so->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y + so->y + sh->ds->conf->shadow_y - sh->ds->conf->blur_size);
	  }
     }
}

static void
_ds_shadow_resize(Shadow *sh, int w, int h)
{
   unsigned char toosmall = 0;

   _ds_shadow_obj_init(sh);
   if ((w < ((sh->ds->conf->blur_size * 2) + 2)) ||
       (h < ((sh->ds->conf->blur_size * 2) + 2)))
     toosmall = 1;
   sh->w = w;
   sh->h = h;
   if (sh->toosmall != toosmall)
     sh->reshape = 1;
   if ((sh->square) && (!sh->toosmall))
     {
	if (!sh->object_list)
	  {
	     evas_object_move(sh->object[0],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y + sh->ds->conf->shadow_y - sh->ds->conf->blur_size);
	     evas_object_move(sh->object[1],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y);
	     evas_object_move(sh->object[2],
			      sh->x + sh->w, 
			      sh->y);
	     evas_object_move(sh->object[3],
			      sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			      sh->y + sh->h);
	     
	     evas_object_resize(sh->object[0], sh->w + (sh->ds->conf->blur_size) * 2, sh->ds->conf->blur_size - sh->ds->conf->shadow_y);
	     evas_object_image_fill_set(sh->object[0], 0, 0, sh->w + (sh->ds->conf->blur_size) * 2, sh->ds->conf->blur_size - sh->ds->conf->shadow_y);
	     
	     evas_object_resize(sh->object[1], sh->ds->conf->blur_size - sh->ds->conf->shadow_x, sh->h);
	     evas_object_image_fill_set(sh->object[1], 0, 0, sh->ds->conf->blur_size - sh->ds->conf->shadow_x, sh->h);
	     
	     evas_object_resize(sh->object[2], sh->ds->conf->shadow_x + sh->ds->conf->blur_size, sh->h);
	     evas_object_image_fill_set(sh->object[2], 0, 0, sh->ds->conf->blur_size + sh->ds->conf->shadow_x, sh->h);
	     
	     evas_object_resize(sh->object[3], sh->w + (sh->ds->conf->blur_size * 2), sh->ds->conf->blur_size + sh->ds->conf->shadow_y);
	     evas_object_image_fill_set(sh->object[3], 0, 0, sh->w + (sh->ds->conf->blur_size * 2), sh->ds->conf->blur_size + sh->ds->conf->shadow_y);
	  }
	else
	  {
	     sh->reshape = 1;
	  }
     }
   else
     {
	sh->reshape = 1;
	sh->toosmall = toosmall;
     }
}

static void
_ds_shadow_shaperects(Shadow *sh)
{
   /* the window shape changed - well we have to recalc it */
   sh->reshape = 1;
}

static int
_ds_shadow_reshape(void *data)
{
   Dropshadow *ds;
   Evas_List *l;
   
   ds = data;
   /* in idle time - if something needs a recalc... do it */
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	
	sh = l->data;
	if (sh->reshape)
	  {
	     sh->reshape = 0;
	     _ds_shadow_recalc(sh);
	  }
     }
   return 1;
}

static void
_ds_edge_scan(Shpix *sp, Tilebuf *tb, int bsz, int x1, int y1, int x2, int y2)
{
   int x, y;
   unsigned char *ptr, *pptr;
   int val;

   if (x1 == x2) /* scan vert */
     {
	pptr = sp->pix + ((y1 - 1) * sp->w) + x1;
	ptr = sp->pix + (y1 * sp->w) + x1;
	for (y = y1; y <= y2; y++)
	  {
	     val = ptr[0] + ptr[-1] + pptr[0] + pptr[-1];
	     if ((val != 0) && (val != (255 * 4)))
	       _tilebuf_add_redraw(tb, 
				   x1 - (bsz + 1),
				   y - (bsz + 1),
				   (bsz + 1) * 2,
				   (bsz + 1) * 2);
	     ptr += sp->w;
	     pptr += sp->w;
	  }
     }
   else if (y1 == y2) /* scan horiz */
     {
	pptr = sp->pix + ((y1 - 1) * sp->w) + x1;
	ptr = sp->pix + (y1 * sp->w) + x1;
	for (x = x1; x <= x2; x++)
	  {
	     val = ptr[0] + ptr[-1] + pptr[0] + pptr[-1];
	     if ((val != 0) && (val != (255 * 4)))
	       _tilebuf_add_redraw(tb, 
				   x - (bsz + 1),
				   y1 - (bsz + 1),
				   (bsz + 1) * 2,
				   (bsz + 1) * 2);
	     ptr++;
	     pptr++;
	  }
     }
}

static void
_ds_shadow_recalc(Shadow *sh)
{
   Evas_List *rects = NULL;
   
   rects = e_container_shape_rects_get(sh->shape);
   if ((sh->w < ((sh->ds->conf->blur_size * 2) + 2)) ||
       (sh->h < ((sh->ds->conf->blur_size * 2) + 2)))
     sh->toosmall = 1;
   else
     sh->toosmall = 0;
   if ((rects) || (sh->toosmall))
     {
	Evas_List *l, *ll;
	Shpix *sp;
	int shw, shh, bsz, shx, shy;
	int x1, y1, x2, y2;
	
	if ((!rects) && (sh->toosmall))
	  sh->square = 1;
	else
	  sh->square = 0;
	
	shx = sh->ds->conf->shadow_x;
	shy = sh->ds->conf->shadow_y;
	shw = sh->w;
	shh = sh->h;
	bsz = sh->ds->conf->blur_size;

	if (sh->use_shared)
	  {
	     _ds_shared_unuse(sh->ds);
	     sh->use_shared = 0;
	  }
	
	sp = _ds_shpix_new(shw + (bsz * 2), shh + (bsz * 2));
	if (sp)
	  {
	     Tilebuf *tb;
	     
	     _ds_shadow_obj_shutdown(sh);
	     if (!rects)
	       {
		  _ds_shpix_fill(sp, 0,         0,         shw + (bsz * 2), bsz, 0);
		  _ds_shpix_fill(sp, 0,         bsz + shh, shw + (bsz * 2), bsz, 0);
		  _ds_shpix_fill(sp, 0,         bsz,       bsz,             shh, 0);
		  _ds_shpix_fill(sp, bsz + shw, bsz,       bsz,             shh, 0);
		  _ds_shpix_fill(sp, bsz,       bsz,       shw,             shh, 255);
	       }
	     else
	       {
		  _ds_shpix_fill(sp, 0, 0, shw + (bsz * 2), shh + (bsz * 2), 0);
		  for (l = rects; l; l = l->next)
		    {
		       E_Rect *r;
		       
		       r = l->data;
		       x1 = bsz + r->x;
		       y1 = bsz + r->y; 
		       x2 = bsz + r->x + r->w - 1;
		       y2 = bsz + r->y + r->h - 1;
		       _ds_shpix_fill(sp, x1, y1, (x2 - x1) + 1, (y2 - y1) + 1, 255);
		    }
	       }
	     
	     tb = _tilebuf_new(shw + (bsz * 2), shh + (bsz * 2));
	     if (tb)
	       {
		  Evas_List *brects;
		  
		  _tilebuf_set_tile_size(tb, 16, 16);
		  /* find edges */
		  if (rects)
		    {
		       for (l = rects; l; l = l->next)
			 {
			    E_Rect *r;
			    
			    r = l->data;
			    x1 = bsz + r->x;
			    y1 = bsz + r->y;
			    x2 = bsz + r->x + r->w - 1;
			    y2 = bsz + r->y + r->h - 1;
			    if (x1 < 1) x1 = 1;
			    if (x1 >= (sp->w - 1)) x1 = (sp->w - 1) - 1;
			    if (x2 < 1) x1 = 1;
			    if (x2 >= (sp->w - 1)) x2 = (sp->w - 1) - 1;
			    if (y1 < 1) y1 = 1;
			    if (y1 >= (sp->h - 1)) y1 = (sp->h - 1) - 1;
			    if (y2 < 1) y1 = 1;
			    if (y2 >= (sp->h - 1)) y2 = (sp->h - 1) - 1;
			    _ds_edge_scan(sp, tb, bsz, x1, y1, x2 + 1, y1);
			    _ds_edge_scan(sp, tb, bsz, x1, y2 + 1, x2 + 1, y2 + 1);
			    _ds_edge_scan(sp, tb, bsz, x1, y1, x1, y2 + 1);
			    _ds_edge_scan(sp, tb, bsz, x2 + 1, y1, x2 + 1, y2 + 1);
			 }
		    }
		  /* its a rect - just add the rect outline */
		  else
		    {
		       _tilebuf_add_redraw(tb, 
					   0, 
					   0,
					   shw + (bsz * 2),
					   (bsz + 1) * 2);
		       _tilebuf_add_redraw(tb, 
					   0, 
					   (bsz + 1) * 2,
					   (bsz + 1) * 2,
					   sp->h - (2 * ((bsz + 1) * 2)));
		       _tilebuf_add_redraw(tb, 
					   sp->w - ((bsz + 1) * 2), 
					   (bsz + 1) * 2,
					   (bsz + 1) * 2,
					   sp->h - (2 * ((bsz + 1) * 2)));
		       _tilebuf_add_redraw(tb, 
					   0, 
					   sp->h - ((bsz + 1) * 2),
					   shw + (bsz * 2),
					   (bsz + 1) * 2);
		    }
		  brects = _tilebuf_get_render_rects(tb);
#if 0 /* enable this to see how dropshadow minimises what it has to go blur */
		  printf("BRTECTS:\n");
 		  for (l = brects; l; l = l->next)
		    {
		       E_Rect *r;
		       r = l->data;
		       _ds_shpix_fill(sp, r->x, r->y, r->w, r->h, 255);
/*		       printf("  %i,%i %ix%i\n", r->x, r->y, r->w, r->h);*/
		    }
		  printf("done\n");
#else		  
		  _ds_shpix_blur_rects(sp, brects,
				       sh->ds->table.gauss2, bsz, sh->ds->conf->quality);
#endif		  
		  _ds_shadow_obj_init_rects(sh, brects);
		  for (l = brects, ll = sh->object_list; 
		       l && ll; 
		       l = l->next, ll = ll->next)
		    {
		       Shadow_Object *so;
		       E_Rect *r;
		       
		       r = l->data;
		       so = ll->data;
		       evas_object_image_smooth_scale_set(so->obj, 1);
		       evas_object_move(so->obj,
					sh->x + so->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
					sh->y + so->y + sh->ds->conf->shadow_y - sh->ds->conf->blur_size);
		       evas_object_resize(so->obj,
					  r->w, r->h);
		       evas_object_image_fill_set(so->obj,
						  0, 0,
						  r->w, r->h);
		       if (sh->visible)
			 evas_object_show(so->obj);
		       _ds_shpix_object_set(sp, so->obj, 
					    r->x, r->y, r->w, r->h);
		    }
		  _ds_shpix_free(sp);
		  
		  _tilebuf_free_render_rects(brects);
		  _tilebuf_free(tb);
	       }
	  }
     }
   else
     {
	int shw, shh, bsz, shx, shy;
	
	_ds_shadow_obj_init(sh);
	sh->square = 1;
	
	shx = sh->ds->conf->shadow_x;
	shy = sh->ds->conf->shadow_y;
	shw = sh->w;
	shh = sh->h;
	bsz = sh->ds->conf->blur_size;
	if (shw > ((bsz * 2) + 2)) shw = (bsz * 2) + 2;
	if (shh > ((bsz * 2) + 2)) shh = (bsz * 2) + 2;

	if (sh->use_shared)
	  {
	     printf("EEEK useing shared already!!\n");
	  }
	else
	  {
	     _ds_shadow_obj_shutdown(sh);
	     _ds_shadow_obj_init(sh);
	     _ds_shared_use(sh->ds, sh);
	     sh->use_shared = 1;
	  }
	
	if (shx >= bsz)
	  {
	     if (shy >= bsz)
	       {
		  /* Case 4:
		   * X2
		   * 33
		   */
	       }
	     else
	       {
		  /* Case 3:
		   * 00
		   * X2
		   * 33
		   */
	       }
	  }
	else
	  {
	     if (shy >= bsz)
	       {
		  /* Case 2:
		   * 1X2
		   * 333
		   */
	       }
	     else
	       {
		  /* Case 1:
		   * 000
		   * 1X2
		   * 333
		   */
		  
		  _ds_shstore_object_set(sh->ds->shared.shadow[0], sh->object[0]);
		  _ds_shstore_object_set(sh->ds->shared.shadow[1], sh->object[1]);
		  _ds_shstore_object_set(sh->ds->shared.shadow[2], sh->object[2]);
		  _ds_shstore_object_set(sh->ds->shared.shadow[3], sh->object[3]);
		       
		  evas_object_image_smooth_scale_set(sh->object[0], 0);
		  evas_object_move(sh->object[0],
				   sh->x + shx - bsz,
				   sh->y + shy - bsz);
		  evas_object_image_border_set(sh->object[0],
					       (bsz * 2), (bsz * 2), 0, 0);
		  evas_object_resize(sh->object[0],
				     sh->w + (bsz * 2),
				     bsz - shy);
		  evas_object_image_fill_set(sh->object[0], 0, 0, 
					     sh->w + (bsz) * 2,
					     bsz - shy);
		  
		  evas_object_image_smooth_scale_set(sh->object[1], 0);
		  evas_object_move(sh->object[1],
				   sh->x + shx - bsz,
				   sh->y);
		  evas_object_image_border_set(sh->object[1],
					       0, 0, bsz + shy, bsz - shy);
		  evas_object_resize(sh->object[1],
				     bsz - shx,
				     sh->h);
		  evas_object_image_fill_set(sh->object[1], 0, 0, 
					     bsz - shx,
					     sh->h);
		  
		  evas_object_image_smooth_scale_set(sh->object[2], 0);
		  evas_object_move(sh->object[2],
				   sh->x + sh->w, 
				   sh->y);
		  evas_object_image_border_set(sh->object[2],
					       0, 0, bsz + shy, bsz - shy);
		  evas_object_resize(sh->object[2],
				     bsz + shx,
				     sh->h);
		  evas_object_image_fill_set(sh->object[2], 0, 0,
					     bsz + shx,
					     sh->h);
		  
		  evas_object_image_smooth_scale_set(sh->object[3], 0);
		  evas_object_move(sh->object[3],
				   sh->x + shx - bsz,
				   sh->y + sh->h);
		  evas_object_image_border_set(sh->object[3],
					       (bsz * 2), (bsz * 2), 0, 0);
		  evas_object_resize(sh->object[3],
				     sh->w + (bsz * 2),
				     bsz + shy);
		  evas_object_image_fill_set(sh->object[3], 0, 0,
					     sh->w + (bsz * 2),
					     bsz + shy);
	       }
	  }
	
	if (sh->visible)
	  {
	     evas_object_show(sh->object[0]);
	     evas_object_show(sh->object[1]);
	     evas_object_show(sh->object[2]);
	     evas_object_show(sh->object[3]);
	  }
     }
}

static void
_ds_config_darkness_set(Dropshadow *ds, double v)
{
   Evas_List *l, *ll;
   
   if (v < 0.0) v = 0.0;
   else if (v > 1.0) v = 1.0;
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	int i;

	sh = l->data;
	if (sh->object_list)
	  {
	     for (ll = sh->object_list; ll; ll = ll->next)
	       {
		  Shadow_Object *so;
		  
		  so = ll->data;
		  evas_object_color_set(so->obj,
					0, 0, 0,
					255 * ds->conf->shadow_darkness);
	       }
	  }
	else
	  {
	     for (i = 0; i < 4; i++)
	       evas_object_color_set(sh->object[i],
				     0, 0, 0,
				     255 * ds->conf->shadow_darkness);
	  }
     }
}

static void
_ds_config_shadow_xy_set(Dropshadow *ds, int x, int y)
{
   Evas_List *l;
   
   if (ds->conf->shadow_x >= ds->conf->blur_size)
     ds->conf->shadow_x = ds->conf->blur_size - 1;
   if (ds->conf->shadow_y >= ds->conf->blur_size)
     ds->conf->shadow_y = ds->conf->blur_size - 1;
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;

	sh = l->data;
	_ds_shadow_obj_clear(sh);
	_ds_shadow_shaperects(sh);
     }
   e_config_save_queue();
}

static void
_ds_config_blur_set(Dropshadow *ds, int blur)
{
   Evas_List *l;
   
   if (blur < 0) blur = 0;
   
   if (ds->conf->shadow_x >= ds->conf->blur_size)
     ds->conf->shadow_x = ds->conf->blur_size - 1;
   if (ds->conf->shadow_y >= ds->conf->blur_size)
     ds->conf->shadow_y = ds->conf->blur_size - 1;

   _ds_blur_init(ds);
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	
	sh = l->data;
	_ds_shadow_obj_clear(sh);
	_ds_shadow_shaperects(sh);
     }
   e_config_save_queue();
}

static void
_ds_config_quality_set(Dropshadow *ds, int q)
{
   Evas_List *l;
   
   if (q < 1) q = 1;
   if (q > 4) q = 4;
   if (q == 3) q = 4;
   if (ds->conf->quality == q) return;
   ds->conf->quality = q;
   _ds_blur_init(ds);
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	
	sh = l->data;
	_ds_shadow_obj_clear(sh);
	_ds_shadow_shaperects(sh);
     }
   e_config_save_queue();
}

static void
_ds_blur_init(Dropshadow *ds)
{
   int i;
   
   if (ds->table.gauss) free(ds->table.gauss);
   ds->table.gauss_size = (ds->conf->blur_size * 2) - 1;
   ds->table.gauss = calloc(1, ds->table.gauss_size * sizeof(unsigned char));
   
   ds->table.gauss[ds->conf->blur_size - 1] = 255;
   for (i = 1; i < (ds->conf->blur_size - 1); i++)
     {
	double v;
	
	v = (double)i / (ds->conf->blur_size - 2);
	ds->table.gauss[ds->conf->blur_size - 1 + i] =
	  ds->table.gauss[ds->conf->blur_size - 1 - i] =
	  _ds_gauss_int(-1.5 + (v * 3.0)) * 255.0;
     }
   
   if (ds->table.gauss2) free(ds->table.gauss2);
   ds->table.gauss2_size = (ds->conf->blur_size * 2) - 1;
   ds->table.gauss2 = calloc(1, ds->table.gauss2_size * sizeof(unsigned char));
   
   ds->table.gauss2[ds->conf->blur_size - 1] = 255;
   for (i = 1; i < (ds->conf->blur_size - 1); i++)
     {
	double v;
	
	v = (double)i / (ds->conf->blur_size - 2);
	ds->table.gauss2[ds->conf->blur_size - 1 + i] =
	  ds->table.gauss2[ds->conf->blur_size - 1 - i] =
	  _ds_gauss_int(-1.5 + (v * 3.0)) * 255.0;
     }
}

static double
_ds_gauss_int(double x)
{
   double x2;
   double x3;
   
   if (x > 1.5) return 0.0;
   if (x < -1.5) return 1.0;
   
   x2 = x * x;
   x3 = x2 * x;
   
   if (x >  0.5)
     return .5625 - ( x3 * (1.0 / 6.0) - 3 * x2 * (1.0 / 4.0) + 1.125 * x);
   
   if (x > -0.5)
     return 0.5 - (0.75 * x - x3 * (1.0 / 3.0));
   
   return 0.4375 + (-x3 * (1.0 / 6.0) - 3 * x2 * (1.0 / 4.0) - 1.125 * x);
}

static void
_ds_gauss_blur_h(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int q, int rx, int ry, int rxx, int ryy)
{
   int x, y;
   int i, sum, weight, x1, x2, l, l1, l2, wt;
   unsigned char *p1, *p2, *pp;
   int full, usefull;
   
   full = 0;
   for (i = 0; i < (blur * 2) - 1; i++)
     full += lut[i];
   for (x = rx; x < rxx; x += q)
     {
	usefull = 1;
	
	x1 = x - (blur - 1);
	l1 = 0;
	x2 = x + (blur - 1);
	l2 = (blur * 2) - 2;
	if (x1 < 0)
	  {
	     usefull = 0;
	     l1 -= x1;
	     x1 = 0;
	  }
	if (x2 >= pix_w)
	  {
	     usefull = 0;
	     l2 -= x2 - pix_w + 1;
	     x2 = pix_w - 1;
	  }
	
	pp = pix + x1 + (ry * pix_w);
	p2 = pix_dst + x + (ry * pix_w);
	if (usefull)
	  {
	     for (y = ry; y < ryy; y++)
	       {
		  p1 = pp;
#ifdef MMX
		  /* sum 4 pixels at once */
		  pxor_r2r(mm7, mm7); /* mm7 = 00000000 */
		  pxor_r2r(mm2, mm2); /* mm2 = 00000000 */
		  for (l = 0; l <= (l2 - 3); l += 4)
		    {
		       movd_m2r(((int *)p1)[0], mm0); /* mm0 = 0000abcd */
		       movd_m2r(((int *)(&lut[l]))[0], mm1); /* mm1 = 0000wxyz */
		       punpcklbw_r2r(mm2, mm0); /* mm0 = 0a0b0c0d */
		       punpcklbw_r2r(mm2, mm1); /* mm1 = 0w0x0y0z */
		       pmaddwd_r2r(mm0, mm1); /* mm1 = (a * w) + (b * x) | (c * y) + (d * z) */
		       paddd_r2r(mm1, mm7); /* mm7 += (c * y) + (d * z) */
		       psrlq_i2r(32, mm1); /* mm0 = 0000 | (a * w) + (b * x) */
		       paddd_r2r(mm1, mm7); /* mm7 += (a * w) + (b * x) */
		       p1 += 4;
		    }
		  movd_r2m(mm7, sum); /* sum = mm7 */
		  for (; l <= l2; l++)
		    {
		       sum += (int)(*p1) * (int)lut[l];
		       p1++;
		    }
#else
		  sum = 0;
		  for (l = 0; l <= l2; l++)
		    {
		       sum += (int)(*p1) * (int)lut[l];
		       p1++;
		    }
#endif		       
		  *p2 = sum / full;
		  p2 += pix_w;
		  pp += pix_w;
	       }
	  }
	else
	  {
	     for (y = ry; y < ryy; y++)
	       {
		  p1 = pp;
		  sum = 0;
		  weight = 0;
		  for (l = l1; l <= l2; l++)
		    {
		       wt = lut[l];
		       weight += wt;
		       sum += (int)(*p1) * (int)wt;
		       p1++;
		    }
		  *p2 = sum / weight;
		  p2 += pix_w;
		  pp += pix_w;
	       }
	  }
	/* if its an even number of pixels - force the last row to be done too
	 * as we can't really interpolate that
	 */
	if ((q == 2) && (x == (rxx - 2)))
	  x--;
	else if ((q == 4) && (x >= (rxx - 4)) && (x < (rxx - 1)))
	  x = rxx - 1 - 4;
     }
   if (q == 2)
     {
	for (x = rx + 1; x < rxx; x += q)
	  {
	     p2 = pix_dst + x + (ry * pix_w);
	     for (y = ry; y < ryy; y++)
	       {
		  *p2 = ((*(p2 - 1)) + (*(p2 + 1))) / 2;
		  p2 += pix_w;
	       }
	     if (x == (rxx - 3)) break;
	  }
     }
   else if (q == 4)
     {
	for (x = rx + 1; x < rxx; x += q)
	  {
	     /* 3 pix left to interp */
	     if (x <= (rxx - 4))
	       {
		  p2 = pix_dst + x + 1 + (ry * pix_w);
		  for (y = ry; y < ryy; y++)
		    {
		       *p2 = ((*(p2 - 2)) + (*(p2 + 2))) / 2;
		       *(p2 - 1) = ((*(p2 - 2)) + (*(p2))) / 2;
		       *(p2 + 1) = ((*(p2 + 2)) + (*(p2))) / 2;
		       p2 += pix_w;
		    }
	       }
	     /* 1 pix left to interp */
	     else if (x == (rxx - 2))
	       {
		  p2 = pix_dst + x + (ry * pix_w);
		  for (y = ry; y < ryy; y++)
		    {
		       *p2 = ((*(p2 - 1)) + (*(p2 + 1))) / 2;
		       p2 += pix_w;
		    }
		  break;
	       }
	     /* 2 pix left to interp */
	     else if (x == (rxx - 3))
	       {
		  p2 = pix_dst + x + (ry * pix_w);
		  for (y = ry; y < ryy; y++)
		    {
		       *p2 = (((*(p2 - 1)) * 2) + (*(p2 + 2))) / 3;
		       *(p2 + 1) = ((*(p2 - 1)) + ((*(p2 + 2)) * 2)) / 3;
		       p2 += pix_w;
		    }
		  break;
	       }
	     else if (x == (rxx - 1))
	       break;
	  }
     }
#ifdef MMX
   emms();
#endif   
}

static void
_ds_gauss_blur_v(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int q, int rx, int ry, int rxx, int ryy)
{
   int x, y;
   int i, sum, weight, l, l1, l2, wt, y1, y2;
   unsigned char *p1, *p2, *pp;
   int full, usefull;
   
   full = 0;
   for (i = 0; i < (blur * 2) - 1; i++)
     full += lut[i];
   for (y = ry; y < ryy; y += q)
     {
	usefull = 1;
	
	y1 = y - (blur - 1);
	l1 = 0;
	y2 = y + (blur - 1);
	l2 = (blur * 2) - 2;
	if (y1 < 0)
	  {
	     usefull = 0;
	     l1 -= y1;
	     y1 = 0;
	  }
	if (y2 >= pix_h)
	  {
	     usefull = 0;
	     l2 -= y2 - pix_h + 1;
	     y2 = pix_h - 1;
	  }
	
	pp = pix + (y1 * pix_w) + rx;
	p2 = pix_dst + (y * pix_w) + rx;
	if (usefull)
	  {
	     for (x = rx; x < rxx; x++)
	       {
		  p1 = pp;
#ifdef MMX
		  /* sum 4 pixels at once */
		  pxor_r2r(mm7, mm7); /* mm7 = 00000000 */
		  pxor_r2r(mm2, mm2); /* mm2 = 00000000 */
		  for (l = 0; l <= (l2 - 3); l += 4)
		    {
		       tpix = (p1[0]); p1 += pix_w;
		       tpix |= (p1[0] << 8); p1 += pix_w;
		       tpix |= (p1[0] << 16); p1 += pix_w;
		       tpix |= (p1[0] << 24); p1 += pix_w;
		       movd_m2r(tpix, mm0); /* mm0 = 0000abcd */
		       movd_m2r(((int *)(&lut[l]))[0], mm1); /* mm1 = 0000wxyz */
		       punpcklbw_r2r(mm2, mm0); /* mm0 = 0a0b0c0d */
		       punpcklbw_r2r(mm2, mm1); /* mm1 = 0w0x0y0z */
		       pmaddwd_r2r(mm0, mm1); /* mm1 = (a * w) + (b * x) | (c * y) + (d * z) */
		       paddd_r2r(mm1, mm7); /* mm7 += (c * y) + (d * z) */
		       psrlq_i2r(32, mm1); /* mm0 = 0000 | (a * w) + (b * x) */
		       paddd_r2r(mm1, mm7); /* mm7 += (a * w) + (b * x) */
		    }
		  movd_r2m(mm7, sum); /* sum = mm7 */
		  for (; l <= l2; l++)
		    {
		       sum += (int)(*p1) * (int)lut[l];
		       p1 += pix_w;
		    }
#else
		  sum = 0;
		  for (l = 0; l <= l2; l++)
		    {
		       sum += (int)(*p1) * (int)lut[l];
		       p1 += pix_w;
		    }
#endif		  
		  *p2 = sum / full;
		  p2++;
		  pp++;
	       }
	  }
	else
	  {
	     for (x = rx; x < rxx; x++)
	       {
		  p1 = pp;
		  sum = 0;
		  weight = 0;
		  for (l = l1; l <= l2; l++)
		    {
		       wt = lut[l];
		       weight += wt;
		       sum += (int)(*p1) * wt;
		       p1 += pix_w;
		    }
		  *p2 = sum / weight;
		  p2++;
		  pp++;
	       }
	  }
	/* if its an even number of pixels - force the last row to be done too
	 * as we can't really interpolate that
	 */
	if ((q == 2) && (y == (ryy - 2))) y--;
	else if ((q == 4) && (y >= (ryy - 4)) && (y < (ryy - 1)))
	  y = ryy - 1 - 4;
     }
   if (q == 2)
     {
	for (y = ry + 1; y < ryy; y += q)
	  {
	     p2 = pix_dst + (y * pix_w) + rx;
	     for (x = rx; x < rxx; x++)
	       {
		  *p2 = ((*(p2 - pix_w)) + (*(p2 + pix_w))) / 2;
		  p2++;
	       }
	     if (y == (ryy - 3)) break;
	  }
     }
   else if (q == 4)
     {
	for (y = ry + 1; y < ryy; y += q)
	  {
	     /* 3 pix left to interp */
	     if (y <= (ryy - 4))
	       {
		  p2 = pix_dst + ((y + 1) * pix_w) + rx;
		  for (x = rx; x < rxx; x++)
		    {
		       *p2 = ((*(p2 - (pix_w * 2))) + (*(p2 + (pix_w * 2)))) / 2;
		       *(p2 - pix_w) = ((*(p2 - (pix_w * 2))) + (*(p2))) / 2;
		       *(p2 + pix_w) = ((*(p2 + (pix_w * 2))) + (*(p2))) / 2;
		       p2++;
		    }
	       }
	     /* 1 pix left to interp */
	     else if (y == (ryy - 2))
	       {
		  p2 = pix_dst + (y * pix_w) + rx;
		  for (x = rx; x < rxx; x++)
		    {
		       *p2 = ((*(p2 - pix_w)) + (*(p2 + pix_w))) / 2;
		       p2++;
		    }
		  break;
	       }
	     /* 2 pix left to interp */
	     else if (y == (ryy - 3))
	       {
		  p2 = pix_dst + (y * pix_w) + rx;
		  for (x = rx; x < rxx; x++)
		    {
		       *p2 = (((*(p2 - pix_w)) * 2) + (*(p2 + (pix_w * 2)))) / 3;
		       *(p2 + pix_w) = ((*(p2 - pix_w)) + ((*(p2 + (pix_w * 2))) * 2)) / 3;
		       p2++;
		    }
		  break;
	       }
	     else if (y == (ryy - 1))
	       break;
	  }
     }
#ifdef MMX
   emms();
#endif   
}

static Shpix *
_ds_shpix_new(int w, int h)
{
   Shpix *sp;
   
   sp = calloc(1, sizeof(Shpix));
   sp->w = w;
   sp->h = h;
   sp->pix = malloc(w * h * sizeof(unsigned char));
   if (!sp->pix)
     {
	free(sp);
	return NULL;
     }
   return sp;
}

static void
_ds_shpix_free(Shpix *sp)
{
   if (!sp) return;
   if (sp->pix) free(sp->pix);
   free(sp);
}

static void
_ds_shpix_fill(Shpix *sp, int x, int y, int w, int h, unsigned char val)
{
   int xx, yy, jump;
   unsigned char *p;
   
   if (!sp) return;
   if ((w < 1) || (h < 1)) return;
   
   if (x < 0)
     {
	w += x;
	x = 0;
	if (w < 1) return;
     }
   if (x >= sp->w) return;
   if ((x + w) > (sp->w)) w = sp->w - x;
   
   if (y < 0)
     {
	h += y;
	y = 0;
	if (h < 1) return;
     }
   if (y >= sp->h) return;
   if ((y + h) > (sp->h)) h = sp->h - y;
	
   p = sp->pix + (y * sp->w) + x;
   jump = sp->w - w;
   for (yy = 0; yy < h; yy++)
     {
	for (xx = 0; xx < w; xx++)
	  {
	     *p = val;
	     p++;
	  }
	p += jump;
     }
}

static void
_ds_shpix_blur(Shpix *sp, int x, int y, int w, int h, unsigned char *blur_lut, int blur_size, int q)
{
   Shpix *sp2;
   
   if (!sp) return;
   if (blur_size < 1) return;
   if ((w < 1) || (h < 1)) return;
   
   if (x < 0)
     {
	w += x;
	x = 0;
	if (w < 1) return;
     }
   if (x >= sp->w) return;
   if ((x + w) > (sp->w)) w = sp->w - x;
   
   if (y < 0)
     {
	h += y;
	y = 0;
	if (h < 1) return;
     }
   if (y >= sp->h) return;
   if ((y + h) > (sp->h)) h = sp->h - y;
   
   sp2 = _ds_shpix_new(sp->w, sp->h);
   if (!sp2) return;
   /* FIXME: copy the inverse rects from rects list */
   memcpy(sp2->pix, sp->pix, sp->w * sp->h);
   _ds_gauss_blur_h(sp->pix, sp2->pix,
		    sp->w, sp->h,
		    blur_lut, blur_size, q,
		    x, y, x + w, y + h);
   _ds_gauss_blur_v(sp2->pix, sp->pix,
		    sp->w, sp->h,
		    blur_lut, blur_size, q,
		    x, y, x + w, y + h);
   _ds_shpix_free(sp2);
}

static void
_ds_shpix_blur_rects(Shpix *sp, Evas_List *rects, unsigned char *blur_lut, int blur_size, int q)
{
   Shpix *sp2;
   Evas_List *l;
   
   if (!sp) return;
   if (blur_size < 1) return;
   
   sp2 = _ds_shpix_new(sp->w, sp->h);
   if (!sp2) return;
   /* FIXME: copy the inverse rects from rects list */
   memcpy(sp2->pix, sp->pix, sp->w * sp->h);
   for (l = rects; l; l = l->next)
     {
	E_Rect *r;
	int x, y, w, h;
	
	r = l->data;
	x = r->x; y = r->y; w = r->w; h = r->h;
	if ((w < 1) || (h < 1)) continue;
	
	if (x < 0)
	  {
	     w += x;
	     x = 0;
	     if (w < 1) continue;
	  }
	if (x >= sp->w) continue;
	if ((x + w) > (sp->w)) w = sp->w - x;
	
	if (y < 0)
	  {
	     h += y;
	     y = 0;
	     if (h < 1) continue;
	  }
	if (y >= sp->h) continue;
	if ((y + h) > (sp->h)) h = sp->h - y;
	_ds_gauss_blur_h(sp->pix, sp2->pix,
			 sp->w, sp->h,
			 blur_lut, blur_size, q,
			 x, y, x + w, y + h);
     }
   for (l = rects; l; l = l->next)
     {
	E_Rect *r;
	int x, y, w, h;
	
	r = l->data;
	x = r->x; y = r->y; w = r->w; h = r->h;
	if ((w < 1) || (h < 1)) continue;
	
	if (x < 0)
	  {
	     w += x;
	     x = 0;
	     if (w < 1) continue;
	  }
	if (x >= sp->w) continue;
	if ((x + w) > (sp->w)) w = sp->w - x;
	
	if (y < 0)
	  {
	     h += y;
	     y = 0;
	     if (h < 1) continue;
	  }
	if (y >= sp->h) continue;
	if ((y + h) > (sp->h)) h = sp->h - y;
	_ds_gauss_blur_v(sp2->pix, sp->pix,
			 sp2->w, sp2->h,
			 blur_lut, blur_size, q,
			 x, y, x + w, y + h);
     }
   _ds_shpix_free(sp2);
}

static void
_ds_shpix_object_set(Shpix *sp, Evas_Object *o, int x, int y, int w, int h)
{
   unsigned char *p;
   unsigned int *pix2, *p2;
   int xx, yy, jump;

   if (!sp) return;
   if (!o) return;
   if ((w < 1) || (h < 1)) return;
   
   if (x < 0)
     {
	w += x;
	x = 0;
	if (w < 1) return;
     }
   if (x >= sp->w) return;
   if ((x + w) > (sp->w)) w = sp->w - x;
   
   if (y < 0)
     {
	h += y;
	y = 0;
	if (h < 1) return;
     }
   if (y >= sp->h) return;
   if ((y + h) > (sp->h)) h = sp->h - y;
   
   evas_object_image_size_set(o, w, h);
   evas_object_image_alpha_set(o, 1);
   pix2 = evas_object_image_data_get(o, 1);
   if (pix2)
     {
	p = sp->pix + (y * sp->w) + x;
	jump = sp->w - w;
	p2 = pix2;
	for (yy = 0; yy < h; yy++)
	  {
#ifdef MMX
	     /* expand 2 pixels at once */
	     for (xx = 0; xx < (w - 1); xx += 2)
	       {
		  pix = (p[1] << 24) | (p[0] << 8);
		  movd_m2r(pix, mm1); /* mm1 = A0a0 */
		  pxor_r2r(mm0, mm0); /* mm0 = 00000000 */
		  punpcklbw_r2r(mm1, mm0); /* mm0 = A000a000 */
		  movq_r2m(mm0, p2[0]); /* *p2 = mm0; */
		  p2 += 2;
		  p += 2;
	       }
	     for (; xx < w; xx++)
	       {
		  *p2 = ((*p) << 24);
		  p2++;
		  p++;
	       }
#else
	     for (xx = 0; xx < w; xx++)
	       {
		  *p2 = ((*p) << 24);
		  p2++;
		  p++;
	       }
#endif	     
	     p += jump;
	  }
	evas_object_image_data_set(o, pix2);
	evas_object_image_data_update_add(o, 0, 0, w, h);
     }
#ifdef MMX
   /* we did mmx stuff - cleanup */
   emms();
#else
#endif
   
}

static void
_ds_shared_free(Dropshadow *ds)
{
   int i;

   for (i = 0; i < 4; i++)
     {
	if (ds->shared.shadow[i])
	  {
	     _ds_shstore_free(ds->shared.shadow[i]);
	     ds->shared.shadow[i] = NULL;
	  }
     }
   ds->shared.ref = 0;
}

static void
_ds_shared_use(Dropshadow *ds, Shadow *sh)
{
   if (ds->shared.ref == 0)
     {
	Shpix *sp;
	int shw, shh, bsz, shx, shy;
	
	shx = sh->ds->conf->shadow_x;
	shy = sh->ds->conf->shadow_y;
	shw = sh->w;
	shh = sh->h;
	bsz = sh->ds->conf->blur_size;
	if (shw > ((bsz * 2) + 2)) shw = (bsz * 2) + 2;
	if (shh > ((bsz * 2) + 2)) shh = (bsz * 2) + 2;
	
	sp = _ds_shpix_new(shw + (bsz * 2), shh + (bsz * 2));
	if (sp)
	  {
	     _ds_shpix_fill(sp, 0,         0,         shw + (bsz * 2), bsz, 0);
	     _ds_shpix_fill(sp, 0,         bsz + shh, shw + (bsz * 2), bsz, 0);
	     _ds_shpix_fill(sp, 0,         bsz,       bsz,             shh, 0);
	     _ds_shpix_fill(sp, bsz + shw, bsz,       bsz,             shh, 0);
	     _ds_shpix_fill(sp, bsz,       bsz,       shw,             shh, 255);
	     
	     if (shx >= bsz)
	       {
		  if (shy >= bsz)
		    {
		       /* Case 4:
			* X2
			* 33
			*/
		    }
		  else
		    {
		       /* Case 3:
			* 00
			* X2
			* 33
			*/
		    }
	       }
	     else
	       {
		  if (shy >= bsz)
		    {
		       /* Case 2:
			* 1X2
			* 333
			*/
		    }
		  else
		    {
		       /* Case 1:
			* 000
			* 1X2
			* 333
			*/
		       _ds_shpix_blur(sp, 0, 0, 
				      shw + (bsz * 2), shh + (bsz * 2),
				      ds->table.gauss, bsz, ds->conf->quality);

		       ds->shared.shadow[0] = 
			 _ds_shstore_new(sp,
					 0, 0,
					 shw + (bsz * 2), bsz - shy);
		       ds->shared.shadow[1] = 
			 _ds_shstore_new(sp,
					 0, bsz - shy,
					 bsz - shx, shh);
		       ds->shared.shadow[2] = 
			 _ds_shstore_new(sp,
					 shw + bsz - shx, bsz - shy,
					 bsz + shx, shh);
		       ds->shared.shadow[3] = 
			 _ds_shstore_new(sp,
					 0, bsz - shy + shh,
					 shw + (bsz * 2), bsz + shy);
		    }
	       }
	     _ds_shpix_free(sp);
	  }
     }
   ds->shared.ref++;
}

static void
_ds_shared_unuse(Dropshadow *ds)
{
   ds->shared.ref--;
   if (ds->shared.ref == 0)
     _ds_shared_free(ds);
}

static Shstore *
_ds_shstore_new(Shpix *sp, int x, int y, int w, int h)
{
   Shstore *st;
   unsigned char *p;
   unsigned int *p2;
   int xx, yy, jump;

   if (!sp) return NULL;
   
   if ((w < 1) || (h < 1)) return NULL;
   
   if (x < 0)
     {
	w += x;
	x = 0;
	if (w < 1) return NULL;
     }
   if (x >= sp->w) return NULL;
   if ((x + w) > (sp->w)) w = sp->w - x;
   
   if (y < 0)
     {
	h += y;
	y = 0;
	if (h < 1) return NULL;
     }
   if (y >= sp->h) return NULL;
   if ((y + h) > (sp->h)) h = sp->h - y;

   st = calloc(1, sizeof(Shstore));
   if (!st) return NULL;
   st->pix = malloc(w * h * sizeof(unsigned int));
   if (!st->pix)
     {
	free(st);
	return NULL;
     }
   st->w = w;
   st->h = h;
   
   p = sp->pix + (y * sp->w) + x;
   jump = sp->w - w;
   p2 = st->pix;
   for (yy = 0; yy < h; yy++)
     {
	for (xx = 0; xx < w; xx++)
	  {
	     *p2 = ((*p) << 24);
	     p2++;
	     p++;
	  }
	p += jump;
     }
   return st;
}

static void
_ds_shstore_free(Shstore *st)
{
   if (!st) return;
   free(st->pix);
   free(st);
}

static void
_ds_shstore_object_set(Shstore *st, Evas_Object *o)
{
   evas_object_image_size_set(o, st->w, st->h);
   evas_object_image_alpha_set(o, 1);
   evas_object_image_data_set(o, st->pix);
   evas_object_image_data_update_add(o, 0, 0, st->w, st->h);
}

static void
_ds_object_unset(Evas_Object *o)
{
   evas_object_image_data_set(o, NULL);
   evas_object_image_size_set(o, 0, 0);
}

static int
_tilebuf_x_intersect(Tilebuf *tb, int x, int w, int *x1, int *x2, int *x1_fill, int *x2_fill)
{
   return _tilebuf_intersect(tb->tile_size.w, tb->outbuf_w, tb->tiles.w,
			    x, w, x1, x2, x1_fill, x2_fill);
}

static int
_tilebuf_y_intersect(Tilebuf *tb, int y, int h, int *y1, int *y2, int *y1_fill, int *y2_fill)
{
   return _tilebuf_intersect(tb->tile_size.h, tb->outbuf_h, tb->tiles.h,
			    y, h, y1, y2, y1_fill, y2_fill);
}

static int
_tilebuf_intersect(int tsize, int tlen, int tnum, int x, int w, int *x1, int *x2, int *x1_fill, int *x2_fill)
{
   int p1, p2;
   
   /* initial clip out of region */
   if ((x + w) <= 0) return 0;
   if (x >= tlen) return 0;
   
   /* adjust x & w so it all fits in region */
   if (x < 0)
     {
	w += x;
	x = 0;
     }
   if (w < 0) return 0;
   if ((x + w) > tlen) w = tlen - x;
   
   /* now figure if the first edge is fully filling its tile */
   p1 = (x) / tsize;
   if ((p1 * tsize) == (x)) *x1_fill = 1;
   else                     *x1_fill = 0;
   *x1 = p1;
   
   /* now figure if the last edge is fully filling its tile */
   p2 = (x + w - 1) / tsize;
   if (((p2 + 1) * tsize) == (x + w)) *x2_fill = 1;
   else                               *x2_fill = 0;
   *x2 = p2;
   
   return 1;
   tnum = 0;
}

static void
_tilebuf_setup(Tilebuf *tb)
{
   if (tb->tiles.tiles) free(tb->tiles.tiles);
   tb->tiles.tiles = NULL;
   
   tb->tiles.w = (tb->outbuf_w + (tb->tile_size.w - 1)) / tb->tile_size.w;
   tb->tiles.h = (tb->outbuf_h + (tb->tile_size.h - 1)) / tb->tile_size.h;
   
   tb->tiles.tiles = malloc(tb->tiles.w * tb->tiles.h * sizeof(Tilebuf_Tile));
   
   if (!tb->tiles.tiles)
     {
	tb->tiles.w = 0;
	tb->tiles.h = 0;
	return;
     }
   memset(tb->tiles.tiles, 0, tb->tiles.w * tb->tiles.h * sizeof(Tilebuf_Tile));
}

static Tilebuf *
_tilebuf_new(int w, int h)
{
   Tilebuf *tb;
   
   tb = calloc(1, sizeof(Tilebuf));
   if (!tb) return NULL;
   
   tb->tile_size.w = 16;
   tb->tile_size.h = 16;
   tb->outbuf_w = w;
   tb->outbuf_h = h;
   
   return tb;
}

static void
_tilebuf_free(Tilebuf *tb)
{
   if (tb->tiles.tiles) free(tb->tiles.tiles);
   free(tb);
}

static void
_tilebuf_set_tile_size(Tilebuf *tb, int tw, int th)
{
   tb->tile_size.w = tw;
   tb->tile_size.h = th;
   _tilebuf_setup(tb);
}

static int
_tilebuf_add_redraw(Tilebuf *tb, int x, int y, int w, int h)
{
   int tx1, tx2, ty1, ty2, tfx1, tfx2, tfy1, tfy2, xx, yy;
   int num;
   
   num = 0;
   if (_tilebuf_x_intersect(tb, x, w, &tx1, &tx2, &tfx1, &tfx2) &&
       _tilebuf_y_intersect(tb, y, h, &ty1, &ty2, &tfy1, &tfy2))
     {
	for (yy = ty1; yy <= ty2; yy++)
	  {
	     Tilebuf_Tile *tbt;
	     
	     tbt = &(TILE(tb, tx1, yy));
	     for (xx = tx1; xx <= tx2; xx++)
	       {
		  tbt->redraw = 1;
		  num++;
		  tbt++;
	       }
	  }
     }
   return num;
}

static Evas_List *
_tilebuf_get_render_rects(Tilebuf *tb)
{
   Evas_List *rects = NULL;
   int x, y;
   
   for (y = 0; y < tb->tiles.h; y++)
     {
	for (x = 0; x < tb->tiles.w; x++)
	  {
	     if (TILE(tb, x, y).redraw)
	       {
		  int can_expand_x = 1, can_expand_y = 1;
		  E_Rect *r = NULL;
		  int xx = 0, yy = 0;
		  
		  r = calloc(1, sizeof(E_Rect));
		  /* amalgamate tiles */
		  while (can_expand_x)
		    {
		       xx++;
		       if ((x + xx) >= tb->tiles.w)
			 can_expand_x = 0;
		       else if (!(TILE(tb, x + xx, y).redraw))
			 can_expand_x = 0;
		       if (can_expand_x)
			 TILE(tb, x + xx, y).redraw = 0;
		    }
		  while (can_expand_y)
		    {
		       int i;
		       
		       yy++;
		       if ((y + yy) >= tb->tiles.h)
			 can_expand_y = 0;
		       if (can_expand_y)
			 {
			    for (i = x; i < x + xx; i++)
			      {
				 if (!(TILE(tb, i, y + yy).redraw))
				   {
				      can_expand_y = 0;
				      break;
				   }
			      }
			 }
		       if (can_expand_y)
			 {
			    for (i = x; i < x + xx; i++)
			      TILE(tb, i, y + yy).redraw = 0;
			 }
		    }
		  TILE(tb, x, y).redraw = 0;
		  r->x = x * tb->tile_size.w;
		  r->y = y * tb->tile_size.h;
		  r->w = (xx) * tb->tile_size.w;
		  r->h = (yy) * tb->tile_size.h;
		  if (r->x < 0)
		    {
		       r->w += r->x;
		       r->x = 0;
		    }
		  if ((r->x + r->w) > tb->outbuf_w)
		    r->w = tb->outbuf_w - r->x;
		  if (r->y < 0)
		    {
		       r->h += r->y;
		       r->y = 0;
		    }
		  if ((r->y + r->h) > tb->outbuf_h)
		    r->h = tb->outbuf_h - r->y;
		  if ((r->w <= 0) || (r->h <= 0))
		    free(r);
		  else
		    rects = evas_list_append(rects, r);
		  x = x + (xx - 1);
	       }
	  }
     }
   return rects;
}

static void
_tilebuf_free_render_rects(Evas_List *rects)
{
   while (rects)
     {
	E_Rect *r;
	
	r = rects->data;
	rects = evas_list_remove_list(rects, rects);
	free(r);
     }
}

void
_dropshadow_cb_config_updated(void *data) 
{
   Dropshadow *ds;
   
   ds = data;
   if (!ds) return;

   _ds_config_quality_set(ds, ds->conf->quality);
   _ds_config_darkness_set(ds, ds->conf->shadow_darkness);
   _ds_config_shadow_xy_set(ds, ds->conf->shadow_x, ds->conf->shadow_y);
   _ds_config_blur_set(ds, ds->conf->blur_size);   
}
