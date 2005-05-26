/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_test_internal(E_Container *con);
static void _cb_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);

void
e_test(void)
{
   Evas_List *managers, *l, *ll;
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     E_Container *con;
	     
	     con = ll->data;
	     _e_test_internal(con);
	  }
     }
}

#if 0
/* local subsystem functions */
typedef struct _Dat Dat;
struct _Dat
{
   Evas_Object *table;
   Evas_List *items;
};

static void
_e_test_internal(E_Container *con)
{
   E_Gadman_Client *gmc;
   Dat *dat;
   Evas_Object *o;
   int i, j;
   
   dat = calloc(1, sizeof(Dat));
   dat->table = e_table_add(con->bg_evas);
   e_table_freeze(dat->table);
   e_table_homogenous_set(dat->table, 1);
   for (j = 0; j < 5; j++)
     {
	for (i = 0; i < 5; i++)
	  {
	     o = evas_object_rectangle_add(con->bg_evas);
	     dat->items = evas_list_append(dat->items, o);
	     evas_object_color_set(o, i * 50, j * 50, 100, 100);
	     e_table_pack(dat->table, o, i, j, 1, 1);
	     e_table_pack_options_set(o, 1, 1, 1, 1, 0.5, 0.5, 0, 0, -1, -1);
	     evas_object_show(o);
	  }
     }
   e_table_thaw(dat->table);
   evas_object_show(dat->table);
   
   gmc = e_gadman_client_new(con->gadman);
   e_gadman_client_domain_set(gmc, "TEST", 0);
   e_gadman_client_policy_set(gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(gmc, 10, 10);
   e_gadman_client_auto_size_set(gmc, 128, 128);
   e_gadman_client_align_set(gmc, 0.5, 0.5);
   e_gadman_client_resize(gmc, 128, 128);
   e_gadman_client_change_func_set(gmc, _cb_change, dat);
   e_gadman_client_load(gmc);
}

static void
_cb_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Dat *dat;
   Evas_Coord x, y, w, h;
   
   dat = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	e_gadman_client_geometry_get(gmc, &x, &y, &w, &h);
	evas_object_move(dat->table, x, y);
	evas_object_resize(dat->table, w, h);
	break;
      default:
	break;
     }
}
#elif 0
typedef struct _Dat Dat;
struct _Dat
{
   Evas_Object *layout;
   Evas_List *items;
};

static void
_e_test_internal(E_Container *con)
{
   E_Gadman_Client *gmc;
   Dat *dat;
   Evas_Object *o;
   int i;
   
   dat = calloc(1, sizeof(Dat));
   dat->layout = e_layout_add(con->bg_evas);
   e_layout_freeze(dat->layout);
   e_layout_virtual_size_set(dat->layout, 800, 600);
   for (i = 0; i < 10; i++)
     {
	Evas_Coord x, y, w, h;
	
	o = evas_object_rectangle_add(con->bg_evas);
	dat->items = evas_list_append(dat->items, o);
	evas_object_color_set(o, i * 25, 255 - (i * 25), 100, 100);
	e_layout_pack(dat->layout, o);
	w = rand() % 800;
	h = rand() % 600;
	x = rand() % (800 - w);
	y = rand() % (600 - h);
	e_layout_child_move(o, x, y);
	e_layout_child_resize(o, w, h);
	evas_object_show(o);
     }
   e_layout_thaw(dat->layout);
   evas_object_show(dat->layout);
   
   gmc = e_gadman_client_new(con->gadman);
   e_gadman_client_domain_set(gmc, "TEST", 0);
   e_gadman_client_policy_set(gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(gmc, 10, 10);
   e_gadman_client_auto_size_set(gmc, 128, 128);
   e_gadman_client_align_set(gmc, 0.5, 0.5);
   e_gadman_client_resize(gmc, 128, 128);
   e_gadman_client_change_func_set(gmc, _cb_change, dat);
   e_gadman_client_load(gmc);
}

static void
_cb_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Dat *dat;
   Evas_Coord x, y, w, h;
   
   dat = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	e_gadman_client_geometry_get(gmc, &x, &y, &w, &h);
	evas_object_move(dat->layout, x, y);
	evas_object_resize(dat->layout, w, h);
	break;
      default:
	break;
     }
}
#elif 1
static int
_e_test_timer(void *data)
{
   E_Menu *m;
   Evas_List *managers, *l;
   
   m = data;
   if (m)
     {
	e_menu_deactivate(m);
	e_object_del(E_OBJECT(m));
	ecore_timer_add(0.2, _e_test_timer, NULL);
	return 0;
     }
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	m = e_int_menus_main_new();
	e_menu_activate_mouse(m,
			      e_container_zone_number_get(e_manager_container_current_get(man), 0),
			      0, 0, 1, 1, E_MENU_POP_DIRECTION_DOWN);
	ecore_timer_add(0.2, _e_test_timer, m);
	return 0;
     }
   return 0;
}

static void
_e_test_internal(E_Container *con)
{
   _e_test_timer(NULL);
}
#else
static void
_e_test_internal(E_Container *con)
{    
}
#endif
