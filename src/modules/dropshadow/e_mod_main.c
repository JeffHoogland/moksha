#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 * 
 * * bug in shadow_x < 0 and shadow_y < 0 needs to be fixed (not urgent though)
 * * add alpha-pixel only pixel space to image objects in evas and make use of it to save cpu and ram
 * * when blurring ALSO cut out the overlayed rect frrom the blur algorithm
 * * handle shaped windows efficiently (as possible).
 * 
 */

/* module private routines */
static Dropshadow *_ds_init(E_Module *m);
static void        _ds_shutdown(Dropshadow *ds);
static E_Menu     *_ds_config_menu_new(Dropshadow *ds);
static void        _ds_menu_very_fuzzy(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_fuzzy(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_sharp(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_very_sharp(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_very_dark(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_dark(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_light(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_very_light(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_very_far(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_far(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_close(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_very_close(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_extremely_close(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_menu_under(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _ds_container_shapes_add(Dropshadow *ds, E_Container *con);
static void        _ds_shape_change(void *data, E_Container_Shape *es, E_Container_Shape_Change ch);
static Shadow     *_ds_shadow_find(Dropshadow *ds, E_Container_Shape *es);
static Shadow     *_ds_shadow_add(Dropshadow *ds, E_Container_Shape *es);
static void        _ds_shadow_obj_init(Shadow *sh);
static void        _ds_shadow_obj_shutdown(Shadow *sh);
static void        _ds_shadow_del(Shadow *sh);
static void        _ds_shadow_show(Shadow *sh);
static void        _ds_shadow_hide(Shadow *sh);
static void        _ds_shadow_move(Shadow *sh, int x, int y);
static void        _ds_shadow_resize(Shadow *sh, int w, int h);
static void        _ds_shadow_shaperects(Shadow *sh);
static int         _ds_shadow_reshape(void *data);
static void        _ds_shadow_recalc(Shadow *sh);
static void        _ds_shadow_object_pixels_set(Evas_Object *o, unsigned char *pix, int pix_w, int pix_h, int sx, int sy, int sw, int sh);
static void        _ds_config_darkness_set(Dropshadow *ds, double v);
static void        _ds_config_shadow_xy_set(Dropshadow *ds, int x, int y);
static void        _ds_config_blur_set(Dropshadow *ds, int blur);
static void        _ds_blur_init(Dropshadow *ds);
static double      _ds_gauss_int(double x);
static void        _ds_gauss_blur_h(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int rx, int ry, int rxx, int ryy);
static void        _ds_gauss_blur_v(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int rx, int ry, int rxx, int ryy);
static void        _ds_gauss_fill(unsigned char *pix, int pix_w, int pix_h, unsigned char v, int rx, int ry, int rxx, int ryy);
static void        _ds_gauss_copy(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, int rx, int ry, int rxx, int ryy);
static void        _ds_gauss_blur(unsigned char *pix, int pix_w, int pix_h, unsigned char *lut, int blur, int sx, int sy, int sw, int sh);
    
/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Dropshadow *ds;
   
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Dropshadow\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   ds = _ds_init(m);
   m->config_menu = _ds_config_menu_new(ds);
   return ds;
}

int
shutdown(E_Module *m)
{
   Dropshadow *ds;
   
   ds = m->data;
   if (ds)
     {
	if (m->config_menu)
	  {
	     e_menu_deactivate(m->config_menu);
	     e_object_del(E_OBJECT(m->config_menu));
	     m->config_menu = NULL;
	  }
	_ds_shutdown(ds);
     }
   return 1;
}

int
save(E_Module *m)
{
   Dropshadow *ds;
   
   ds = m->data;
   e_config_domain_save("module.dropshadow", ds->conf_edd, ds->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Dropshadow");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Dropshadow Module",
		       "This is the dropshadow module that allows dropshadows to be cast\n"
		       "on the desktop background - without special X-Server extensions\n"
		       "or hardware acceleration.");
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
   E_CONFIG_VAL(D, T, shadow_darkness, DOUBLE);
   
   ds->conf = e_config_domain_load("module.dropshadow", ds->conf_edd);
   if (!ds->conf)
     {
	ds->conf = E_NEW(Config, 1);
	ds->conf->shadow_x = 4;
	ds->conf->shadow_y = 4;
	ds->conf->blur_size = 10;
	ds->conf->shadow_darkness = 0.5;
     }
   E_CONFIG_LIMIT(ds->conf->shadow_x, -200, 200);
   E_CONFIG_LIMIT(ds->conf->shadow_y, -200, 200);
   E_CONFIG_LIMIT(ds->conf->blur_size, 1, 120);
   E_CONFIG_LIMIT(ds->conf->shadow_darkness, 0.0, 1.0);
   
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
   free(ds);
}

static E_Menu *
_ds_config_menu_new(Dropshadow *ds)
{
   E_Menu *mn;
   E_Menu_Item *mi;
   char buf[4096];
   
   mn = e_menu_new();
     
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Fuzzy");
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_fuzzy.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 80) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_fuzzy, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Fuzzy");
   snprintf(buf, sizeof(buf), "%s/menu_icon_fuzzy.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_fuzzy, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Medium");
   snprintf(buf, sizeof(buf), "%s/menu_icon_medium.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_medium, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Sharp");
   snprintf(buf, sizeof(buf), "%s/menu_icon_sharp.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 10) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_sharp, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Sharp");
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_sharp.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_sharp, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Dark");
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_dark.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 1.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_dark, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Dark");
   snprintf(buf, sizeof(buf), "%s/menu_icon_dark.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 0.75) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_dark, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Light");
   snprintf(buf, sizeof(buf), "%s/menu_icon_light.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 0.5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_light, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Light");
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_light.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 0.25) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_light, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Far");
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_far.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 32) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_far, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Far");
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_far.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 16) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_far, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Close");
   snprintf(buf, sizeof(buf), "%s/menu_icon_far.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 8) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_close, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Close");
   snprintf(buf, sizeof(buf), "%s/menu_icon_close.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 4) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_close, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Extremely Close");
   snprintf(buf, sizeof(buf), "%s/menu_icon_underneath.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 2) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_extremely_close, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Immediately Underneath");
   snprintf(buf, sizeof(buf), "%s/menu_icon_underneath.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_under, ds);
   return mn;
}

static void
_ds_menu_very_fuzzy(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_blur_set(ds, 80);
}

static void
_ds_menu_fuzzy(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_blur_set(ds, 40);
}

static void
_ds_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_blur_set(ds, 20);
}

static void
_ds_menu_sharp(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_blur_set(ds, 10);
}

static void
_ds_menu_very_sharp(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_blur_set(ds, 5);
}

static void
_ds_menu_very_dark(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_darkness_set(ds, 1.0);
}

static void
_ds_menu_dark(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_darkness_set(ds, 0.75);
}

static void
_ds_menu_light(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_darkness_set(ds, 0.5);
}

static void
_ds_menu_very_light(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_darkness_set(ds, 0.25);
}

static void
_ds_menu_very_far(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_shadow_xy_set(ds, 32, 32);
}

static void
_ds_menu_far(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_shadow_xy_set(ds, 16, 16);
}

static void
_ds_menu_close(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_shadow_xy_set(ds, 8, 8);
}

static void
_ds_menu_very_close(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_shadow_xy_set(ds, 4, 4);
}

static void
_ds_menu_extremely_close(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_shadow_xy_set(ds, 2, 2);
}

static void
_ds_menu_under(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Dropshadow *ds;
   
   ds = data;
   _ds_config_shadow_xy_set(ds, 0, 0);
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
   _ds_shadow_obj_init(sh);
   return sh;
}

static void
_ds_shadow_obj_init(Shadow *sh)
{
   E_Container *con;
   int i;
   
   con = e_container_shape_container_get(sh->shape);
   for (i = 0; i < 4; i++)
     {
	sh->object[i] = evas_object_image_add(con->bg_evas);
	evas_object_layer_set(sh->object[i], 10);
	evas_object_pass_events_set(sh->object[i], 1);
	evas_object_move(sh->object[i], 0, 0);
	evas_object_resize(sh->object[i], 0, 0);
	evas_object_color_set(sh->object[i],
			      255, 255, 255, 
			      255 * sh->ds->conf->shadow_darkness);
     }
}

static void
_ds_shadow_obj_shutdown(Shadow *sh)
{
   evas_object_del(sh->object[0]);
   evas_object_del(sh->object[1]);
   evas_object_del(sh->object[2]);
   evas_object_del(sh->object[3]);
   sh->object[0] = NULL;
   sh->object[1] = NULL;
   sh->object[2] = NULL;
   sh->object[3] = NULL;
}

static void
_ds_shadow_del(Shadow *sh)
{
   sh->ds->shadows = evas_list_remove(sh->ds->shadows, sh);
   _ds_shadow_obj_shutdown(sh);
   e_object_unref(E_OBJECT(sh->shape));
   free(sh);
}

static void
_ds_shadow_show(Shadow *sh)
{
   if (sh->square)
     {
	evas_object_show(sh->object[0]);
	evas_object_show(sh->object[1]);
	evas_object_show(sh->object[2]);
	evas_object_show(sh->object[3]);
     }
   else
     {
	evas_object_show(sh->object[0]);
     }
}

static void
_ds_shadow_hide(Shadow *sh)
{
   if (sh->square)
     {
	evas_object_hide(sh->object[0]);
	evas_object_hide(sh->object[1]);
	evas_object_hide(sh->object[2]);
	evas_object_hide(sh->object[3]);
     }
   else
     {
	evas_object_hide(sh->object[0]);
     }
}

static void
_ds_shadow_move(Shadow *sh, int x, int y)
{
   sh->x = x;
   sh->y = y;
   if (sh->square)
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

static void
_ds_shadow_resize(Shadow *sh, int w, int h)
{
   sh->w = w;
   sh->h = h;
   sh->reshape = 1;
}

static void
_ds_shadow_shaperects(Shadow *sh)
{
   sh->reshape = 1;
}

static int
_ds_shadow_reshape(void *data)
{
   Dropshadow *ds;
   Evas_List *l;
   
   ds = data;
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
_ds_shadow_recalc(Shadow *sh)
{
   Evas_List *rects;
   unsigned char *pix;
   int pix_w, pix_h;
   int sx, sy, sxx, syy, ssw, ssh;
   
   rects = e_container_shape_rects_get(sh->shape);
   if (rects)
     {
	Evas_List *l;
	
	sh->square = 0;
	pix_w = sh->w + (sh->ds->conf->blur_size * 2);
	pix_h = sh->h + (sh->ds->conf->blur_size * 2);
	pix = calloc(1, pix_w * pix_h * sizeof(unsigned char));

	/* for every rect in the shape - fill it */
	for (l = rects; l; l = l->next)
	  {
	     E_Rect *r;
	     
	     r = l->data;
	     _ds_gauss_fill(pix, pix_w, pix_h, 255, r->x, r->y, r->x + r->w, r->y + r->h);
	  }
	/* FIXME: need to find an optimal "inner rect" fromt he above rect list */
/*	     
	sx = sh->ds->conf->blur_size;
	sy = sh->ds->conf->blur_size;
	sxx = pix_w - sh->ds->conf->blur_size;
	syy = pix_h - sh->ds->conf->blur_size;
*/
	sx = 0;
	sy = 0;
	sxx = 0;
	syy = 0;
	     
	_ds_gauss_blur(pix, pix_w, pix_h,
		       sh->ds->table.gauss, sh->ds->conf->blur_size,
		       sx, sy, sxx, syy);
	evas_object_move(sh->object[0],
			 sh->x + sh->ds->conf->shadow_x - sh->ds->conf->blur_size,
			 sh->y + sh->ds->conf->shadow_y - sh->ds->conf->blur_size); 
	sx = 0;
	sy = 0;
	ssw = sh->w + (sh->ds->conf->blur_size * 2);
	ssh = sh->h + (sh->ds->conf->blur_size * 2);
	_ds_shadow_object_pixels_set(sh->object[0], pix, pix_w, pix_h,
				     sx, sy, ssw, ssh);
	if (evas_object_visible_get(sh->object[0]))
	  {
	     evas_object_hide(sh->object[1]);
	     evas_object_hide(sh->object[2]);
	     evas_object_hide(sh->object[3]);
	  }
	free(pix);
     }
   else
     {
	sh->square = 1;
	pix_w = sh->w + (sh->ds->conf->blur_size * 2);
	pix_h = sh->h + (sh->ds->conf->blur_size * 2);
	pix = calloc(1, pix_w * pix_h * sizeof(unsigned char));
	sx = sh->ds->conf->blur_size;
	sy = sh->ds->conf->blur_size;
	sxx = pix_w - sh->ds->conf->blur_size;
	syy = pix_h - sh->ds->conf->blur_size;
	_ds_gauss_fill(pix, pix_w, pix_h, 255, sx, sy, sxx, syy);
	sx = sh->ds->conf->blur_size * 2;
	sy = sh->ds->conf->blur_size * 2;
	ssw = pix_w - (sh->ds->conf->blur_size * 4);
	ssh = pix_h - (sh->ds->conf->blur_size * 4);
	_ds_gauss_blur(pix, pix_w, pix_h,
		       sh->ds->table.gauss, sh->ds->conf->blur_size,
		       sx, sy, ssw, ssh);
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
	sx = 0;
	sy = 0;
	ssw = sh->w + (sh->ds->conf->blur_size * 2);
	ssh = sh->ds->conf->blur_size - sh->ds->conf->shadow_y;
	_ds_shadow_object_pixels_set(sh->object[0], pix, pix_w, pix_h,
				     sx, sy, ssw, ssh);
	sx = 0;
	sy = sh->ds->conf->blur_size - sh->ds->conf->shadow_y;
	ssw = sh->ds->conf->blur_size - sh->ds->conf->shadow_x;
	ssh = sh->h;
	_ds_shadow_object_pixels_set(sh->object[1], pix, pix_w, pix_h,
				     sx, sy, ssw, ssh);
	sx = sh->ds->conf->blur_size - sh->ds->conf->shadow_y + sh->w;
	sy = sh->ds->conf->blur_size - sh->ds->conf->shadow_y;
	ssw = sh->ds->conf->blur_size + sh->ds->conf->shadow_x;
	ssh = sh->h;
	_ds_shadow_object_pixels_set(sh->object[2], pix, pix_w, pix_h,
				     sx, sy, ssw, ssh);
	sx = 0;
	sy = sh->ds->conf->blur_size - sh->ds->conf->shadow_y + sh->h;
	ssw = sh->w + (sh->ds->conf->blur_size * 2);
	ssh = sh->ds->conf->blur_size + sh->ds->conf->shadow_y;
	_ds_shadow_object_pixels_set(sh->object[3], pix, pix_w, pix_h,
				     sx, sy, ssw, ssh);
	if (evas_object_visible_get(sh->object[0]))
	  {
	     evas_object_show(sh->object[1]);
	     evas_object_show(sh->object[2]);
	     evas_object_show(sh->object[3]);
	  }
	free(pix);
     }
}

static void
_ds_shadow_object_pixels_set(Evas_Object *o, unsigned char *pix, int pix_w, int pix_h, int sx, int sy, int sw, int sh)
{
   unsigned char *p;
   unsigned int *pix2, *p2;
   int x, y;
   
   if (sw < 0) sw = 0;
   if (sh < 0) sh = 0;
   evas_object_image_size_set(o, sw, sh);
   evas_object_resize(o, sw, sh);
   evas_object_image_fill_set(o, 0, 0, sw, sh);
   evas_object_image_alpha_set(o, 1);
   evas_object_image_smooth_scale_set(o, 0);
   pix2 = evas_object_image_data_get(o, 1);
   if (pix2)
     {
	if ((sy >= 0) && (sx >= 0))
	  {
	     p2 = pix2;
	     for (y = 0; y < sh; y++)
	       {
		  p = pix + ((y + sy) * pix_w) + sx;
		  for (x = 0; x < sw; x++)
		    {
		       *p2 = ((*p) << 24);
		       p2++;
		       p++;
		    }
	       }
	  }
	else if (sy < 0)
	  {
	     p2 = pix2;
	     for (y = 0; y < (-sy); y++)
	       {
		  for (x = 0; x < sw; x++)
		    {
		       *p2 = 0;
		       p2++;
		    }
	       }
	     sh += sy;
	     sy = 0;
	     for (y = 0; y < sh; y++)
	       {
		  p = pix + ((y + sy) * pix_w) + sx;
		  for (x = 0; x < sw; x++)
		    {
		       *p2 = ((*p) << 24);
		       p2++;
		       p++;
		    }
	       }
	  }
	else if (sx < 0)
	  {
	     int ox;
	     
	     ox = 0;
	     for (y = 0; y < sh; y++)
	       {
		  p2 = pix2 + (y * sw);
		  for (x = 0; x < (-sx); x++)
		    {
		       *p2 = 0;
		       p2++;
		    }
	       }
	     sw += sx;
	     ox = -sx;
	     sx = 0;
	     for (y = 0; y < sh; y++)
	       {
		  p2 = pix2 + (y * sw) + ox;
		  p = pix + ((y + sy) * pix_w) + sx;
		  for (x = 0; x < sw; x++)
		    {
		       *p2 = ((*p) << 24);
		       p2++;
		       p++;
		    }
	       }
	  }
	evas_object_image_data_set(o, pix2);
	evas_object_image_data_update_add(o, 0, 0, sw, sh);
     }
}

static void
_ds_config_darkness_set(Dropshadow *ds, double v)
{
   Evas_List *l;
   
   if (v < 0.0) v = 0.0;
   else if (v > 1.0) v = 1.0;
   if (ds->conf->shadow_darkness == v) return;
   ds->conf->shadow_darkness = v;
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	int i;

	sh = l->data;
	for (i = 0; i < 4; i++)
	  evas_object_color_set(sh->object[i],
				255, 255, 255, 
				255 * ds->conf->shadow_darkness);
     }
   e_config_save_queue();
}

static void
_ds_config_shadow_xy_set(Dropshadow *ds, int x, int y)
{
   Evas_List *l;
   
   if ((ds->conf->shadow_x == x) && (ds->conf->shadow_y == y)) return;
   ds->conf->shadow_x = x;
   ds->conf->shadow_y = y;
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;

	sh = l->data;
	_ds_shadow_shaperects(sh);
     }
   e_config_save_queue();
}

static void
_ds_config_blur_set(Dropshadow *ds, int blur)
{
   Evas_List *l;
   
   if (blur < 0) blur = 0;
   if (ds->conf->blur_size == blur) return;
   ds->conf->blur_size = blur;
   
   _ds_blur_init(ds);
   for (l = ds->shadows; l; l = l->next)
     {
	Shadow *sh;
	
	sh = l->data;
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
_ds_gauss_blur_h(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int rx, int ry, int rxx, int ryy)
{
   int x, y;
   int i, sum, weight, x1, x2, l, l1, l2, wt;
   unsigned char *p1, *p2, *pp;
   int full, usefull;
   
   full = 0;
   for (i = 0; i < (blur * 2) - 1; i++)
     full += lut[i];
   for (x = rx; x < rxx; x++)
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
		  sum = 0;
		  for (l = 0; l <= l2; l++)
		    {
		       sum += (*p1) * lut[l];
		       p1++;
		    }
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
		       sum += (*p1) * wt;
		       p1++;
		    }
		  *p2 = sum / weight;
		  p2 += pix_w;
		  pp += pix_w;
	       }
	  }
     }
}

static void
_ds_gauss_blur_v(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int rx, int ry, int rxx, int ryy)
{
   int x, y;
   int i, sum, weight, l, l1, l2, wt, y1, y2;
   unsigned char *p1, *p2, *pp;
   int full, usefull;
   
   full = 0;
   for (i = 0; i < (blur * 2) - 1; i++)
     full += lut[i];
   for (y = ry; y < ryy; y++)
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
		  sum = 0;
		  for (l = 0; l <= l2; l++)
		    {
		       sum += (*p1) * lut[l];
		       p1 += pix_w;
		    }
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
		       sum += (*p1) * wt;
		       p1 += pix_w;
		    }
		  *p2 = sum / weight;
		  p2++;
		  pp++;
	       }
	  }
     }
}

static void
_ds_gauss_fill(unsigned char *pix, int pix_w, int pix_h, unsigned char v, int rx, int ry, int rxx, int ryy)
{
   int y;
   char *p1;
   
   p1 = pix + rx + (ry * pix_w);
   for (y = ry; y < ryy; y++)
     {
	memset(p1, v, (rxx - rx));
	p1 += pix_w;
     }
}

static void
_ds_gauss_copy(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, int rx, int ry, int rxx, int ryy)
{
   int y;
   char *p1, *p2;
   
   p1 = pix + rx + (ry * pix_w);
   p2 = pix_dst + rx + (ry * pix_w);
   for (y = ry; y < ryy; y++)
     {
	memcpy(p2, p1, (rxx - rx));
	p2 += pix_w;
	p1 += pix_w;
     }
}

static void
_ds_gauss_blur(unsigned char *pix, int pix_w, int pix_h, unsigned char *lut, int blur, int sx, int sy, int sw, int sh)
{
   unsigned char *pix2;
   
   pix2 = malloc(pix_w * pix_h * sizeof(unsigned char *));
   if ((sw <= 0) || (sh <= 0))
     {
	_ds_gauss_blur_h(pix, pix2, pix_w, pix_h, lut, blur, 0, 0, pix_w, pix_h);
	_ds_gauss_blur_v(pix2, pix, pix_w, pix_h, lut, blur, 0, 0, pix_w, pix_h);
     }
   else
     {
	int x, y, w, h;
	
	x = 0; y = 0; w = pix_w; h = sy;
	if (h > 0) _ds_gauss_blur_h(pix, pix2, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	x = 0; y = sy; w = sx; h = sh;
	if (w > 0) _ds_gauss_blur_h(pix, pix2, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	x = sx + sw; y = sy; w = pix_w - x; h = sh;
	if (w > 0) _ds_gauss_blur_h(pix, pix2, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	x = 0; y = sy + sh; w = pix_w; h = pix_h - y;
	if (h > 0) _ds_gauss_blur_h(pix, pix2, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	_ds_gauss_copy(pix, pix2, pix_w, pix_h,
		       sx, sy, sx + sw, sy + sh);
	x = 0; y = 0; w = pix_w; h = sy;
	if (h > 0) _ds_gauss_blur_v(pix2, pix, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	x = 0; y = sy; w = sx; h = sh;
	if (w > 0) _ds_gauss_blur_v(pix2, pix, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	x = sx + sw; y = sy; w = pix_w - x; h = sh;
	if (w > 0) _ds_gauss_blur_v(pix2, pix, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	x = 0; y = sy + sh; w = pix_w; h = pix_h - y;
	if (h > 0) _ds_gauss_blur_v(pix2, pix, pix_w, pix_h, lut, blur,
				    x, y, x + w, y + h);
	_ds_gauss_copy(pix2, pix, pix_w, pix_h,
		       sx, sy, sx + sw, sy + sh);
     }
   free(pix2);
}
