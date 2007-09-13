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


static int t_init     (E_Wizard_Page *pg){
   return 1;
}
static int t_shutdown (E_Wizard_Page *pg){
   return 1;
}
static int t_show     (E_Wizard_Page *pg){
   Evas_Object *ob, *o;
   
   ob = e_widget_list_add(pg->evas, 1, 0);
   o = e_widget_button_add(pg->evas,
			    "Hello World", NULL, 
			    NULL, NULL, NULL);
   e_widget_list_object_append(ob, o, 0, 0, 0.5);
   evas_object_show(o);
   e_wizard_page_show(ob);
   pg->data = ob;
   return 0;
}
static int t_hide     (E_Wizard_Page *pg){
   evas_object_del(pg->data);
   return 1;
}
static int t_apply    (E_Wizard_Page *pg){
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
   
   ob = e_widget_list_add(pg->evas, 1, 0);
   o = e_widget_button_add(pg->evas,
			    "Hello to Another World", NULL, 
			    NULL, NULL, NULL);
   e_widget_list_object_append(ob, o, 0, 0, 0.5);
   evas_object_show(o);
   e_wizard_page_show(ob);
   pg->data = ob;
   return 0;
}
static int t2_hide     (E_Wizard_Page *pg){
   evas_object_del(pg->data);
   return 1;
}
static int t2_apply    (E_Wizard_Page *pg){
   return 1;
}


EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_wizard_init();
   
   e_wizard_page_add(t_init, t_shutdown, t_show, t_hide, t_apply);
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
