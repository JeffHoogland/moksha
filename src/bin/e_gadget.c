/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_gadget_menu_init(E_Gadget *gad);
static void _e_gadget_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadget_free(E_Gadget *gad);
static void _e_gadget_face_cb_gmc_change(void * data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void _e_gadget_menu_init(E_Gadget *gad);
static void _e_gadget_face_menu_init(E_Gadget_Face *face);
static void _e_gadget_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);


E_Gadget *
e_gadget_new(E_Module *module,
	     const char *name,
	     void (*func_face_init) (void *data, E_Gadget_Face *gadget_face),
	     void (*func_face_free) (void *data, E_Gadget_Face *gadget_face),
	     void (*func_change) (void *data, E_Gadget_Face *gadget_face, E_Gadman_Client *gmc, E_Gadman_Change change),
	     void (*func_menu_init) (void *data, E_Gadget *gadget),
	     void (*func_face_menu_init) (void *data, E_Gadget_Face *gadget_face),
	     void *data)
{
   E_Gadget *gad;
   Evas_List *managers, *l = NULL, *l2 = NULL;
   char buf[1024];
   int gadget_count = 0;

   gad = E_OBJECT_ALLOC(E_Gadget, E_GADGET_TYPE, _e_gadget_free);
   if (!gad) return NULL;

   gad->module = module;
   e_object_ref(E_OBJECT(gad->module));

   if(!name) return NULL;
   gad->name = evas_stringshare_add(name);

   gad->funcs.face_init = func_face_init;
   gad->funcs.face_free = func_face_free;
   gad->funcs.change = func_change;
   gad->funcs.menu_init = func_menu_init;
   gad->funcs.face_menu_init = func_face_menu_init;
   gad->data = data;


   /* get all desktop evases, and call init function on them */
   managers = e_manager_list();
   for(l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for(l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     E_Gadget_Face *face;
	     E_Gadget_Change *change;
	     Evas_Object *o;

	     con = l2->data;
	     face = E_NEW(E_Gadget_Face, 1);
	     if (!face) continue;

	     face->gad = gad;
	     face->con = con;
	     e_object_ref(E_OBJECT(con));
	     face->evas = con->bg_evas;
	     
	     evas_event_freeze(face->evas);

	     /* create an event object */
	     
	     o = evas_object_rectangle_add(con->bg_evas);
	     face->event_obj = o;
	     evas_object_layer_set(o, 2);
	     evas_object_repeat_events_set(o, 1);
	     evas_object_color_set(o, 0, 0, 0, 0);
	     evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_gadget_cb_mouse_down, face);
	     evas_object_show(o);
	
	     /* create a gadman client */
	     snprintf(buf, sizeof(buf), "module.%s", gad->name);
	     face->gmc = e_gadman_client_new(con->gadman);
	     e_gadman_client_domain_set(face->gmc, buf, gadget_count++);

	     /* call the user init */
	     if (gad->funcs.face_init) (gad->funcs.face_init)(gad->data, face);

	     _e_gadget_face_menu_init(face);

	     change = E_NEW(E_Gadget_Change, 1);
	     if (change)
	       {
		  printf("set change func\n");
		  change->gadget = gad;
		  change->face = face;
	       }

	     /* set up some gadman defaults */
	     e_gadman_client_policy_set(face->gmc,
					E_GADMAN_POLICY_ANYWHERE |
					E_GADMAN_POLICY_HMOVE |
					E_GADMAN_POLICY_VMOVE |
					E_GADMAN_POLICY_HSIZE |
					E_GADMAN_POLICY_VSIZE );
	     e_gadman_client_min_size_set(face->gmc, 4, 4);
	     e_gadman_client_max_size_set(face->gmc, 128, 128);
	     e_gadman_client_auto_size_set(face->gmc, 40, 40);
	     e_gadman_client_align_set(face->gmc, 1.0, 1.0);
	     e_gadman_client_resize(face->gmc, 40, 40);
	     e_gadman_client_change_func_set(face->gmc, _e_gadget_face_cb_gmc_change, change);
	     e_gadman_client_load(face->gmc);
	     evas_event_thaw(face->evas);

	     gad->faces = evas_list_append(gad->faces, face);
	  }
     }
   _e_gadget_menu_init(gad);

   return gad;
}

void
e_gadget_face_theme_set(E_Gadget_Face *face, char *category, char *group)
{
   Evas_Object *o;
   Evas_Coord x, y, w, h;
   if(!face) return;

   o = edje_object_add(face->evas);
   face->main_obj = o;
   e_theme_edje_object_set(o, category, group);
   evas_object_show(o);
}

static void
_e_gadget_menu_init(E_Gadget *gad)
{
   gad->module->config_menu = gad->menu = e_menu_new();
   if (gad->funcs.menu_init) (gad->funcs.menu_init)(gad->data, gad);
}

static void
_e_gadget_face_menu_init(E_Gadget_Face *face)
{
   E_Menu_Item *mi;

   face->menu = e_menu_new();

   mi = e_menu_item_new(face->menu);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _e_gadget_cb_menu_edit, face);

   if (face->gad->funcs.face_menu_init) (face->gad->funcs.face_menu_init)(face->gad->data, face);
}

static void
_e_gadget_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadget_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}

static void
_e_gadget_free(E_Gadget *gad)
{
   Evas_List *l;

   if (!gad) return;

   for (l = gad->faces; l; l = l->next) {
	E_Gadget_Face *face = l->data;
	
	e_object_unref(E_OBJECT(face->con));
	e_object_del(E_OBJECT(face->gmc));
	e_object_del(E_OBJECT(face->menu));
	
	if (face->main_obj) evas_object_del(face->main_obj);
	if (face->event_obj) evas_object_del(face->event_obj);
	if(gad->funcs.face_free) (gad->funcs.face_free)(gad->data, face);
   }
   
   evas_list_free(gad->faces);
   gad->module->config_menu = NULL;
   e_object_unref(E_OBJECT(gad->module));
   e_object_del(E_OBJECT(gad->menu));
   if (gad->name) evas_stringshare_del(gad->name);
   free(gad);
  
}

static void
_e_gadget_face_cb_gmc_change(void * data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   E_Gadget_Change *gdc = data;
   E_Gadget_Face *face;
   Evas_Coord x, y, w, h;

   if(!gdc || !gdc->gadget || !gdc->face) return;

   face = gdc->face;

   switch(change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	 e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
	 if (face->main_obj) 
	   {
	      evas_object_move(face->main_obj, x, y);
	      evas_object_resize(face->main_obj, w, h);
	   }
	 if (face->event_obj) 
	   {
	      evas_object_move(face->event_obj, x, y);
	      evas_object_resize(face->event_obj, w, h);
	   }
	 break;
      case E_GADMAN_CHANGE_RAISE:
	 if (face->main_obj) 
	   {
	      evas_object_raise(face->main_obj);
	   }
	 if (face->event_obj) 
	   {
	      evas_object_raise(face->event_obj);
	   }
	 break;
     }

   if (gdc->gadget->funcs.change) (gdc->gadget->funcs.change)(gdc->gadget->data, gdc->face, gmc, change);
}

static void
_e_gadget_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadget_Face *face;
   
   ev = event_info;
   face = data;
   if (!face) return;

   if (ev->button == 3 && face->gad->menu)
     {
	e_menu_activate_mouse(face->menu, e_zone_current_get(face->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(face->con);
     }
}
