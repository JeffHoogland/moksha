#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 * 
 * * bug in shadow_x < 0 and shadow_y < 0 needs to be fixed (not urgent though)
 * * add alpha-pixel only pixel space to image objects in evas and make use of it to save cpu and ram
 * * when blurring ALSO cut out the overlayed rect frrom the blur algorithm
 * * handle shaped windows efficiently (as possible).
 * * share shadow pixels between shadow objects for square shapes if big enough to be split into 1 stretched images
 * * look into mmx for the blur function...
 * * handle other shadow pos cases where we cant use 4 objects (3 or 2).
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
static void        _ds_shadow_obj_clear(Shadow *sh);
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
static void        _ds_config_darkness_set(Dropshadow *ds, double v);
static void        _ds_config_shadow_xy_set(Dropshadow *ds, int x, int y);
static void        _ds_config_blur_set(Dropshadow *ds, int blur);
static void        _ds_blur_init(Dropshadow *ds);
static double      _ds_gauss_int(double x);
static void        _ds_gauss_blur_h(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int rx, int ry, int rxx, int ryy);
static void        _ds_gauss_blur_v(unsigned char *pix, unsigned char *pix_dst, int pix_w, int pix_h, unsigned char *lut, int blur, int rx, int ry, int rxx, int ryy);
static Shpix      *_ds_shpix_new(int w, int h);
static void        _ds_shpix_free(Shpix *sp);
static void        _ds_shpix_fill(Shpix *sp, int x, int y, int w, int h, unsigned char val);
static void        _ds_shpix_blur(Shpix *sp, int x, int y, int w, int h, unsigned char *blur_lut, int blur_size);
static void        _ds_shpix_object_set(Shpix *sp, Evas_Object *o, int x, int y, int w, int h);
static void        _ds_shared_free(Dropshadow *ds);
static void        _ds_shared_use(Dropshadow *ds, Shadow *sh);
static void        _ds_shared_unuse(Dropshadow *ds);
static Shstore    *_ds_shstore_new(Shpix *sp, int x, int y, int w, int h);
static void        _ds_shstore_free(Shstore *st);
static void        _ds_shstore_object_set(Shstore *st, Evas_Object *o);
static void        _ds_object_unset(Evas_Object *o);
    
/* public module routines. all modules must have these */
void *
e_modapi_init(E_Module *m)
{
   Dropshadow *ds;
   
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show(_("Module API Error"),
			    _("Error initializing Module: Dropshadow\n"
			      "It requires a minimum module API version of: %i.\n"
			      "The module API advertized by Enlightenment is: %i.\n"
			      "Aborting module."),
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   ds = _ds_init(m);
   m->config_menu = _ds_config_menu_new(ds);
   return ds;
}

int
e_modapi_shutdown(E_Module *m)
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
e_modapi_save(E_Module *m)
{
   Dropshadow *ds;
   
   ds = m->data;
   e_config_domain_save("module.dropshadow", ds->conf_edd, ds->conf);
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup(_("Dropshadow"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_error_dialog_show(_("Enlightenment Dropshadow Module"),
		       _("This is the dropshadow module that allows dropshadows to be cast\n"
			 "on the desktop background - without special X-Server extensions\n"
			 "or hardware acceleration."));
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
   _ds_shared_free(ds);
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
   e_menu_item_label_set(mi, _("Very Fuzzy"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_fuzzy.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 80) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_fuzzy, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Fuzzy"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_fuzzy.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_fuzzy, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Medium"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_medium.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_medium, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Sharp"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_sharp.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 10) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_sharp, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Sharp"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_sharp.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ds->conf->blur_size == 5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_sharp, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Dark"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_dark.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 1.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_dark, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Dark"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_dark.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 0.75) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_dark, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Light"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_light.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 0.5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_light, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Light"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_light.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ds->conf->shadow_darkness == 0.25) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_light, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Far"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_far.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 32) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_far, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Far"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_very_far.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 16) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_far, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Close"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_far.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 8) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_close, ds);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Close"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_close.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 4) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_very_close, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Extremely Close"));
   snprintf(buf, sizeof(buf), "%s/menu_icon_underneath.png", e_module_dir_get(ds->module));
   e_menu_item_icon_file_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 3);
   if (ds->conf->shadow_x == 2) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ds_menu_extremely_close, ds);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Immediately Underneath"));
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
_ds_shadow_obj_clear(Shadow *sh)
{
   int i;
   
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
}


static void
_ds_shadow_obj_shutdown(Shadow *sh)
{
   int i;
   
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

static void
_ds_shadow_hide(Shadow *sh)
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

static void
_ds_shadow_move(Shadow *sh, int x, int y)
{
   sh->x = x;
   sh->y = y;
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

static void
_ds_shadow_resize(Shadow *sh, int w, int h)
{
   unsigned char toosmall = 0;
   
   if ((w < ((sh->ds->conf->blur_size * 2) + 2)) ||
       (h < ((sh->ds->conf->blur_size * 2) + 2)))
     toosmall = 1;
   sh->w = w;
   sh->h = h;
   if (sh->toosmall != toosmall)
     sh->reshape = 1;
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
	Evas_List *l;
	Shpix *sp;
	int shw, shh, bsz, shx, shy;
	
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
		  for (l = rects; l; l = l->next)
		    {
		       E_Rect *r;
		       
		       r = l->data;
		       _ds_shpix_fill(sp, bsz + r->x, bsz + r->y, r->w, r->h, 255);
		    }
	       }
	     
	     _ds_shpix_blur(sp, 0, 0, shw + (bsz * 2), shh + (bsz * 2),
			 sh->ds->table.gauss, bsz);
		       
	     _ds_shpix_object_set(sp, sh->object[0], 0, 0,
			       shw + (bsz * 2), shh + (bsz * 2));
		       
	     evas_object_move(sh->object[0],
			      sh->x + shx - bsz,
			      sh->y + shy - bsz);
	     evas_object_image_border_set(sh->object[0],
					  0, 0, 0, 0);
	     evas_object_resize(sh->object[0],
				sh->w + (bsz * 2),
				sh->h + (bsz * 2));
	     evas_object_image_fill_set(sh->object[0], 0, 0, 
				sh->w + (bsz * 2),
				sh->h + (bsz * 2));
	     _ds_object_unset(sh->object[1]);
	     _ds_object_unset(sh->object[2]);
	     _ds_object_unset(sh->object[3]);
	     _ds_shpix_free(sp);
	  }
	
	if (evas_object_visible_get(sh->object[0]))
	  {
	     evas_object_hide(sh->object[1]);
	     evas_object_hide(sh->object[2]);
	     evas_object_hide(sh->object[3]);
	  }
     }
   else
     {
	int shw, shh, bsz, shx, shy;
	
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
	
	if (evas_object_visible_get(sh->object[0]))
	  {
	     evas_object_show(sh->object[1]);
	     evas_object_show(sh->object[2]);
	     evas_object_show(sh->object[3]);
	  }
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
   if (ds->conf->blur_size == blur) return;
   ds->conf->blur_size = blur;
   
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
_ds_shpix_blur(Shpix *sp, int x, int y, int w, int h, unsigned char *blur_lut, int blur_size)
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
		    blur_lut, blur_size,
		    x, y, x + w, y + h);
   _ds_gauss_blur_v(sp2->pix, sp->pix,
		    sp->w, sp->h,
		    blur_lut, blur_size,
		    x, y, x + w, y + h);
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
   evas_object_image_smooth_scale_set(o, 0);
   pix2 = evas_object_image_data_get(o, 1);
   if (pix2)
     {
	p = sp->pix + (y * sp->w) + x;
	jump = sp->w - w;
	p2 = pix2;
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
	evas_object_image_data_set(o, pix2);
	evas_object_image_data_update_add(o, 0, 0, w, h);
     }
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
				      ds->table.gauss, bsz);

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
   evas_object_image_data_set(o, st->pix);
   evas_object_image_data_update_add(o, 0, 0, st->w, st->h);
   evas_object_image_alpha_set(o, 1);
   evas_object_image_smooth_scale_set(o, 0);
}

static void
_ds_object_unset(Evas_Object *o)
{
   evas_object_image_data_set(o, NULL);
   evas_object_image_size_set(o, 0, 0);
}
