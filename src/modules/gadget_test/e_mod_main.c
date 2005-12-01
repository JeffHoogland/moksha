/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "Gadget Test"
};

static void _test_face_init(void *data, E_Gadget_Face *face);
static void _test_face_free(void *data, E_Gadget_Face *face);
static void _test_face_change(void *data, E_Gadget_Face *face, E_Gadman_Client *gmc, E_Gadman_Change change);
static void _test_menu_init(void *data, E_Gadget *gad);
static void _test_face_menu_init(void *data, E_Gadget_Face *face);


void *
e_modapi_init(E_Module *m)
{
   E_Gadget *gad = NULL;
   
   Test *t = E_NEW(Test, 1);

   gad = e_gadget_new(m,
	 "test_gadget",
	 _test_face_init,
	 _test_face_free,
	 _test_face_change,
	 _test_menu_init,
	 _test_face_menu_init,
	 t);

   return gad;
}

int
e_modapi_shutdown(E_Module *m)
{
   E_Gadget *gad;

   gad = m->data;
   if (gad) e_object_del(E_OBJECT(gad));
   return 1;
}

int
e_modapi_save(E_Module *m)
{
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment Test Module"), _("Gadget test"));
   return 1;
}

static void
_test_face_init(void *data, E_Gadget_Face *face)
{
   Test *t = data;

   e_gadget_face_theme_set(face, "base/theme/modules/clock", "modules/clock/main");
   
}

static void
_test_face_free(void *data, E_Gadget_Face *face)
{
   Test *t;
   t = data;
   if (t) free(t);
}

static void
_test_face_change(void *data, E_Gadget_Face *face, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   printf("change face!\n");
}

static void
_test_menu_init(void *data, E_Gadget *gad)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(gad->menu);
   e_menu_item_label_set(mi, _("Test Menu Item"));
}

static void
_test_face_menu_init(void *data, E_Gadget_Face *face)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(face->menu);
   e_menu_item_label_set(mi, _("Test Face Menu Item"));
}
