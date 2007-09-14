/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static E_Module *conf_module = NULL;

/**/
/***************************************************************************/

/***************************************************************************/
/**/

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Wizard"
};


static int t0_init     (E_Wizard_Page *pg){
   return 1;
}
static int t0_shutdown (E_Wizard_Page *pg){
   return 1;
}
static int t0_show     (E_Wizard_Page *pg){
   printf("t0\n");
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}
static int t0_hide     (E_Wizard_Page *pg){
   return 1;
}
static int t0_apply    (E_Wizard_Page *pg){
   printf("a0\n");
   return 1;
}

static int t1_init     (E_Wizard_Page *pg){
   return 1;
}
static int t1_shutdown (E_Wizard_Page *pg){
   return 1;
}
static int t1_show     (E_Wizard_Page *pg){
   Evas_Object *ob, *o;
   
   printf("t1\n");
   ob = e_widget_list_add(pg->evas, 1, 0);
   o = e_widget_button_add(pg->evas,
			    "Hello World", NULL, 
			    NULL, NULL, NULL);
   e_widget_list_object_append(ob, o, 0, 0, 0.5);
   evas_object_show(o);
   e_wizard_page_show(ob);
   pg->data = ob;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}
static int t1_hide     (E_Wizard_Page *pg){
   evas_object_del(pg->data);
   return 1;
}
static int t1_apply    (E_Wizard_Page *pg){
   printf("a1\n");
   return 1;
}

static int t2_init     (E_Wizard_Page *pg){
   return 1;
}
static int t2_shutdown (E_Wizard_Page *pg){
   return 1;
}
static int t2_show     (E_Wizard_Page *pg){
   Evas_Object *ob, *o;
   
   printf("t2\n");
   ob = e_widget_list_add(pg->evas, 1, 0);
   o = e_widget_button_add(pg->evas,
			    "Hello to Another World", NULL, 
			    NULL, NULL, NULL);
   e_widget_list_object_append(ob, o, 0, 0, 0.5);
   evas_object_show(o);
   e_wizard_page_show(ob);
   pg->data = ob;
   return 1;
}
static int t2_hide     (E_Wizard_Page *pg){
   evas_object_del(pg->data);
   return 1;
}
static int t2_apply    (E_Wizard_Page *pg){
   printf("a2\n");
   return 1;
}


EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_wizard_init();
   
   e_wizard_page_add(t0_init, t0_shutdown, t0_show, t0_hide, t0_apply);
   e_wizard_page_add(t1_init, t1_shutdown, t1_show, t1_hide, t1_apply);
   e_wizard_page_add(t2_init, t2_shutdown, t2_show, t2_hide, t2_apply);
   
   e_wizard_go();
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_wizard_shutdown();
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(m,
			_("Enlightenment First Run Wizard Module"),
			_("A module for setting up configuration for Enlightenment for the first time."));
   return 1;
}
