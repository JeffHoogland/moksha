#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 * 
 * 
 */

/* module private routines */
static Snow       *_snow_init(E_Module *m);
static void        _snow_shutdown(Snow *snow);
static E_Menu     *_snow_config_menu_new(Snow *snow);
static int         _snow_cb_animator(void *data);
static void        _snow_trees_load(Snow *snow);
static void        _snow_flakes_load(char type, Snow *snow);
    
static void        _snow_cb_density_sparse(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _snow_cb_density_medium(void *data, E_Menu *m, E_Menu_Item *
i);
static void        _snow_cb_density_dense(void *data, E_Menu *m, E_Menu_Item *mi);

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Snow *snow;
   
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: dropshadow\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   snow = _snow_init(m);
   m->config_menu = _snow_config_menu_new(snow);
   return snow;
}

int
shutdown(E_Module *m)
{
   Snow *snow;
   
   snow = m->data;
   if (snow)
     {
	if (m->config_menu)
	  {
	     e_menu_deactivate(m->config_menu);
	     e_object_del(E_OBJECT(m->config_menu));
	     m->config_menu = NULL;
	  }
	_snow_shutdown(snow);
     }
   return 1;
}

int
save(E_Module *m)
{
   Snow *snow;
   
   snow = m->data;
   if (!snow) return;
   e_config_domain_save("module.snow", snow->conf_edd, snow->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Snow");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Snow Module",
		       "This is a snow module that may replace xsnow.");
   return 1;
}

/* module private routines */
static Snow *
_snow_init(E_Module *m)
{
   Snow *snow;
   Evas_List *managers, *l, *l2;
   Evas_Object *o;
   Evas_Coord xx, yy, ww, hh;
   char buf[4096];
   int tw, th, i;
   
   snow = calloc(1, sizeof(Snow));
   if (!snow) return  NULL;

   snow->module = m;
   snow->conf_edd = E_CONFIG_DD_NEW("Snow_Config", Config);
#undef T
#undef D
#define T Config
#define D snow->conf_edd
   E_CONFIG_VAL(D, T, tree_count, INT);
   E_CONFIG_VAL(D, T, flake_count, INT);
   
   snow->conf = e_config_domain_load("module.snow", snow->conf_edd);
   if (!snow->conf)
     {
	snow->conf = E_NEW(Config, 1);
	snow->conf->tree_count = 10;
	snow->conf->flake_count = 60;
     }
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     snow->cons = evas_list_append(snow->cons, con);
	     snow->canvas = con->bg_evas;
	  }
     }

   _snow_trees_load(snow);
   _snow_flakes_load('s', snow);
   _snow_flakes_load('m', snow);
   _snow_flakes_load('l', snow);

   snow->animator = ecore_animator_add(_snow_cb_animator, snow);
     
   return snow;
}

static void
_snow_trees_free(Snow *snow)
{
   while (snow->trees)
     {
	Evas_Object *tree;
	
	tree = snow->trees->data;
	evas_object_del(tree);
	snow->trees = evas_list_remove_list(snow->trees, snow->trees);
     }
}

static void
_snow_flakes_free(Snow *snow)
{
   while (snow->flakes)
     {
	Snow_Flake *flake;

	flake = snow->flakes->data;
	evas_object_del(flake->flake);
	snow->flakes = evas_list_remove_list(snow->flakes, snow->flakes);
	free(flake);
     }
}

static void
_snow_shutdown(Snow *snow)
{
   free(snow->conf);
   E_CONFIG_DD_FREE(snow->conf_edd);
   while (snow->cons)
     {
	E_Container *con;
	
	con = snow->cons->data;
	snow->cons = evas_list_remove_list(snow->cons, snow->cons);
     }
   _snow_trees_free(snow);
   _snow_flakes_free(snow);
   if (snow->animator)
     ecore_animator_del(snow->animator);
   free(snow);
}

static E_Menu *
_snow_config_menu_new(Snow *snow)
{
   E_Menu *mn;
   E_Menu_Item *mi;
   char buf[4096];
   
   mn = e_menu_new();
     
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Sparse");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (snow->conf->tree_count == 5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _snow_cb_density_sparse, snow);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Medium");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (snow->conf->tree_count == 10) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _snow_cb_density_medium, snow);
	       
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Dense");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (snow->conf->tree_count == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _snow_cb_density_dense, snow);
	       
   return mn;
}

static void
_snow_canvas_reset(Snow *snow)
{
   _snow_trees_free(snow);
   _snow_flakes_free(snow);

   _snow_trees_load(snow);
   _snow_flakes_load('s', snow); 
   _snow_flakes_load('m', snow);
   _snow_flakes_load('l', snow);	    
}

static void
_snow_cb_density_sparse(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Snow *snow;

   snow = data;
   snow->conf->tree_count = 5;
   snow->conf->flake_count = 20;

   _snow_canvas_reset(snow);
}

static void
_snow_cb_density_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Snow *snow;

   snow = data;
   snow->conf->tree_count = 10;
   snow->conf->flake_count = 60;
   _snow_canvas_reset(snow);
}

static void
_snow_cb_density_dense(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Snow *snow;

   snow = data;
   snow->conf->tree_count = 20;
   snow->conf->flake_count = 100;
   _snow_canvas_reset(snow);
}

static void
_snow_trees_load(Snow *snow) {
   Evas_Object *o;
   Evas_Coord xx, yy, ww, hh;
   char buf[4096];
   int tw, th, i;

   evas_output_viewport_get(snow->canvas, &xx, &yy, &ww, &hh);
   snprintf(buf, sizeof(buf), "%s/tree.png", e_module_dir_get(snow->module));

   o = evas_object_image_add(snow->canvas);
   evas_object_image_file_set(o, buf, "");
   evas_object_image_size_get(o, &tw, &th);

for (i = 0; i < snow->conf->tree_count; i++)
   {
     Evas_Coord tx, ty;
if (i != 0) {
     o = evas_object_image_add(snow->canvas);
     evas_object_image_file_set(o, buf, "");
}
     evas_object_resize(o, tw, th);
     evas_object_image_alpha_set(o, 1);
     evas_object_image_fill_set(o, 0, 0, tw, th);

     tx = random() % (ww - tw);
     ty = random() % (hh - th);
     evas_object_move(o, tx, ty);
     evas_object_show(o);
     snow->trees = evas_list_append(snow->trees, o);
   }


}

static void
_snow_flakes_load(char type, Snow *snow) {
   Evas_Object *o;
   Evas_Coord xx, yy, ww, hh;
   char buf[4096];
   int tw, th, i;
   Snow_Flake *flake;

   evas_output_viewport_get(snow->canvas, &xx, &yy, &ww, &hh);
   snprintf(buf, sizeof(buf), "%s/flake-%c.png", e_module_dir_get(snow->module),
	    type);

   o = evas_object_image_add(snow->canvas);
   evas_object_image_file_set(o, buf, "");
   evas_object_image_size_get(o, &tw, &th);

for (i = 0; i < snow->conf->flake_count / 3; i++)
   {
     Evas_Coord tx, ty;

     flake = malloc(sizeof(Snow_Flake));
if (i != 0) {
     o = evas_object_image_add(snow->canvas);
     evas_object_image_file_set(o, buf, "");
}
     evas_object_resize(o, tw, th);
     evas_object_image_alpha_set(o, 1);
     evas_object_image_fill_set(o, 0, 0, tw, th);

     tx = random() % ww;
     ty = random() % hh;
     evas_object_move(o, tx, ty);
     evas_object_show(o);
     flake->flake = o;
     flake->start_time = ecore_time_get() + (double)(random() % (th * 10)) / (double) th;
     switch (type) {
       case 's':
         flake->speed = 1;
	 break;
       case 'm':
         flake->speed = 2;
	 break;
       case 'l':
         flake->speed = 3;
	 break;
     }
     snow->flakes = evas_list_append(snow->flakes, flake);
   }


}


static int
_snow_cb_animator(void *data)
{
   Snow *snow;
   Evas_List *next;
   double d;

   snow = data;
   next = snow->flakes;
   while (next)
     {
	Snow_Flake *flake;
	Evas_Coord x, y;

	flake = next->data;
	d = ecore_time_get() - flake->start_time;
	y = 100 * d * flake->speed;
	evas_object_geometry_get(flake->flake, &x, NULL, NULL, NULL);
	if (y > 1024) /* FIXME */
	  flake->start_time = ecore_time_get() + (double) (random() % 100) / (double) 100;
	evas_object_move(flake->flake, x, y);

	next = evas_list_next(next);
     }
						
   return 1;
}
