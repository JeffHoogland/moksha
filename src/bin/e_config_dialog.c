/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_config_dialog_free(E_Config_Dialog *cfd);
static void _e_config_dialog_go(E_Config_Dialog *cfd, E_Config_Dialog_CFData_Type type);
static void _e_config_dialog_cb_dialog_del(void *obj);
static void _e_config_dialog_cb_ok(void *data, E_Dialog *dia);
static void _e_config_dialog_cb_apply(void *data, E_Dialog *dia);
static void _e_config_dialog_cb_advanced(void *data, void *data2);
static void _e_config_dialog_cb_basic(void *data, void *data2);

/* local subsystem globals */

/* externally accessible functions */

E_Config_Dialog *
e_config_dialog_new(E_Container *con, char *title, E_Config_Dialog_View *basic, E_Config_Dialog_View *advanced)
{
   E_Config_Dialog *cfd;
   
   cfd = E_OBJECT_ALLOC(E_Config_Dialog, E_CONFIG_DIALOG_TYPE, _e_config_dialog_free);
   cfd->basic = *basic;
   if (advanced) cfd->advanced = *advanced;
   cfd->con = con;
   cfd->title = strdup(title);
   
   _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_BASIC);
   
   return cfd;
}

/* local subsystem functions */

static void
_e_config_dialog_free(E_Config_Dialog *cfd)
{
   E_FREE(cfd->title);
   if (cfd->cfdata)
     {
	if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
	  cfd->basic.free_cfdata(cfd->cfdata);
	else
	  cfd->advanced.free_cfdata(cfd->cfdata);
	cfd->cfdata = NULL;
     }
   if (cfd->dia)
     {
	e_object_del_attach_func_set(E_OBJECT(cfd->dia), NULL);
	e_object_del(E_OBJECT(cfd->dia));
	cfd->dia = NULL;
     }
   free(cfd);
}

static void
_e_config_dialog_go(E_Config_Dialog *cfd, E_Config_Dialog_CFData_Type type)
{
   void *pdata;
   E_Dialog *pdia;
   Evas_Object *o, *ob;
   Evas_Coord mw = 0, mh = 0;
   
   pdata = cfd->cfdata;
   pdia = cfd->dia;
   
   cfd->dia = e_dialog_new(cfd->con);
   cfd->dia->data = cfd;
   e_object_del_attach_func_set(E_OBJECT(cfd->dia), _e_config_dialog_cb_dialog_del);
   e_dialog_title_set(cfd->dia, cfd->title);
   
   if (type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     {
	cfd->cfdata = cfd->basic.create_cfdata(pdata, cfd->view_type);
	if (cfd->advanced.free_cfdata)
	  {
	     if (pdata) cfd->advanced.free_cfdata(pdata);
	     o = e_widget_list_add(e_win_evas_get(cfd->dia->win), 0, 0);
	     ob = cfd->basic.create_widgets(e_win_evas_get(cfd->dia->win), cfd->cfdata);
	     e_widget_list_object_append(o, ob, 1, 1, 0.0);
	     ob = e_widget_button_add(e_win_evas_get(cfd->dia->win),
				      _("Advanced..."), NULL,
				      _e_config_dialog_cb_advanced, cfd, NULL);
	     e_widget_list_object_append(o, ob, 0, 0, 1.0);
	  }
	else
	  o = cfd->basic.create_widgets(e_win_evas_get(cfd->dia->win), cfd->cfdata);
     }
   else
     {
	cfd->cfdata = cfd->advanced.create_cfdata(pdata, cfd->view_type);
	if (cfd->basic.free_cfdata)
	  {
	     if (pdata) cfd->basic.free_cfdata(pdata);
	     o = e_widget_list_add(e_win_evas_get(cfd->dia->win), 0, 0);
	     ob = cfd->advanced.create_widgets(e_win_evas_get(cfd->dia->win), cfd->cfdata);
	     e_widget_list_object_append(o, ob, 1, 1, 0.0);
	     ob = e_widget_button_add(e_win_evas_get(cfd->dia->win), 
				      _("Basic..."), NULL,
				      _e_config_dialog_cb_basic, cfd, NULL);
	     e_widget_list_object_append(o, ob, 0, 0, 1.0);
	  }
	else
	  o = cfd->advanced.create_widgets(e_win_evas_get(cfd->dia->win), cfd->cfdata);
     }
   
   e_widget_min_size_get(o, &mw, &mh);
   e_dialog_content_set(cfd->dia, o, mw, mh);
   
   e_dialog_button_add(cfd->dia, _("OK"), NULL, _e_config_dialog_cb_ok, cfd);
   e_dialog_button_add(cfd->dia, _("Apply"), NULL, _e_config_dialog_cb_apply, cfd);
   e_dialog_button_add(cfd->dia, _("Cancel"), NULL, NULL, NULL);
   e_win_centered_set(cfd->dia->win, 1);
   e_dialog_show(cfd->dia);
   cfd->view_type = type;
   if (pdia)
     {
	e_object_del_attach_func_set(E_OBJECT(pdia), NULL);
	e_object_del(E_OBJECT(pdia));
     }
}

static void
_e_config_dialog_cb_dialog_del(void *obj)
{
   E_Dialog *dia;
   E_Config_Dialog *cfd;
   
   dia = obj;
   cfd = dia->data;
   cfd->dia = NULL;
   e_object_del(E_OBJECT(cfd));
}

static void
_e_config_dialog_cb_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog *cfd;

   cfd = dia->data;
   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     cfd->basic.apply_cfdata(cfd->cfdata);
   else
     cfd->advanced.apply_cfdata(cfd->cfdata);
   e_object_del(E_OBJECT(cfd));
}

static void
_e_config_dialog_cb_apply(void *data, E_Dialog *dia)
{
   E_Config_Dialog *cfd;

   cfd = dia->data;
   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     cfd->basic.apply_cfdata(cfd->cfdata);
   else
     cfd->advanced.apply_cfdata(cfd->cfdata);
}

static void
_e_config_dialog_cb_advanced(void *data, void *data2)
{
   E_Config_Dialog *cfd;
   
   cfd = data;
   _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED);
}

static void
_e_config_dialog_cb_basic(void *data, void *data2)
{
   E_Config_Dialog *cfd;
   
   cfd = data;
   _e_config_dialog_go(cfd, E_CONFIG_DIALOG_CFDATA_TYPE_BASIC);
}
