#include "e.h"

static void        *_create_data    (E_Config_Dialog *cfd);
static void         _fill_data      (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _free_data      (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create   (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply    (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _cb_icon_sel    (void *data, void *data2);
static Evas_Object *_get_icon       (void *data);
static void         _cb_type        (void *data, Evas_Object *obj, void *event_info);
static void         _cb_fsel_sel    (void *data, Evas_Object *obj);
static void         _cb_fsel_ok     (void *data, E_Dialog *dia);
static void         _cb_fsel_cancel (void *data, E_Dialog *dia);
static void         _cb_file_change (void *data);

typedef enum _Icon_Type Icon_Type;
enum _Icon_Type 
{
     THUMB,
     THEME,
     EDJ
};

struct _E_Config_Dialog_Data 
{
   char *mime;
   char *icon;
   int type;

   char *file;
   struct 
     {
	Evas_Object *icon, *fsel_wid;
	E_Dialog *fsel;
     } gui;
};

#define IFDUP(src, dst) if (src) dst = strdup(src); else dst = NULL;
#define IFFREE(src) if (src) free(src); src = NULL;

EAPI E_Config_Dialog *
e_int_config_mime_edit(E_Config_Mime_Icon *mime) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;
   
   if (e_config_dialog_find("E", "_config_mime_edit_dialog")) return NULL;

   con = e_container_current_get(e_manager_current_get());
   
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   
   cfd = e_config_dialog_new(con, _("Mime Settings"), "E", 
			     "_config_mime_edit_dialog", "enlightenment/e", 
			     0, v, mime);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfd, cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Mime_Icon *mime;
   char *p;
   
   if (!cfd->data) return;

   mime = cfd->data;
   if (!mime) return;
   IFDUP(mime->mime, cfdata->mime);
   IFDUP(mime->icon, cfdata->icon);
   if (!cfdata->icon) 
     {
	cfdata->type = THUMB;
	return;
     }
   
   if (!strcmp(cfdata->icon, "THUMB")) 
     cfdata->type = THUMB;
   else
     {
	p = strrchr(cfdata->icon, '.');
	if ((p) && (!strcmp(p, ".edj"))) 
	  cfdata->type = EDJ;
	else
	  cfdata->type = THEME;
     }
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->gui.fsel) e_object_del(E_OBJECT(cfdata->gui.fsel));
   IFFREE(cfdata->file);
   IFFREE(cfdata->mime);
   IFFREE(cfdata->icon);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of;
   Evas_Object *ob, *oi, *icon;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, _("Basic Info"), 0);
   ob = e_widget_label_add(evas, _("Mime:"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_entry_add(evas, &(cfdata->mime));
   e_widget_entry_readonly_set(ob, 1);
   e_widget_min_size_set(ob, 100, 1);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_frametable_add(evas, _("Icon"), 0);
   rg = e_widget_radio_group_new(&cfdata->type);
   ob = e_widget_radio_add(evas, _("Use Generated Thumbnail"), 0, rg);
   evas_object_smart_callback_add(ob, "changed", _cb_type, cfdata);
   e_widget_frametable_object_append(of, ob, 0, 0, 3, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Use Theme Icon"), 1, rg);
   evas_object_smart_callback_add(ob, "changed", _cb_type, cfdata);
   e_widget_frametable_object_append(of, ob, 0, 1, 3, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Use Edje File"), 2, rg);
   evas_object_smart_callback_add(ob, "changed", _cb_type, cfdata);
   e_widget_frametable_object_append(of, ob, 0, 2, 3, 1, 1, 1, 1, 1);
   e_widget_disabled_set(ob, 1);
   
   oi = e_widget_button_add(evas, "", NULL, _cb_icon_sel, cfdata, cfd);
   cfdata->gui.icon = oi;
   if (cfdata->icon) 
     {
	icon = _get_icon(cfdata);
	if (icon)
	  e_widget_button_icon_set(oi, icon);
     }
   
   e_widget_min_size_set(oi, 48, 48);
   e_widget_frametable_object_append(of, oi, 1, 3, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
      
   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Mime_Icon *mime;
   Evas_List *l;
   char buf[4096];
   
   for (l = e_config->mime_icons; l; l = l->next) 
     {
	mime = l->data;
	if (!mime) continue;
	if (strcmp(mime->mime, cfdata->mime)) continue;
	if (mime->mime)
	  evas_stringshare_del(mime->mime);
	mime->mime = evas_stringshare_add(cfdata->mime);
	if (mime->icon)
	  evas_stringshare_del(mime->icon);
	switch (cfdata->type) 
	  {
	   case THUMB:
	     mime->icon = evas_stringshare_add("THUMB");
	     break;
	   case THEME:
	     snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", cfdata->mime);
	     mime->icon = evas_stringshare_add(buf);
	     break;
	   case EDJ:
	     break;
	  }
	break;
     }
   e_config_save_queue();
   return 1;
}

static void 
_cb_icon_sel(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Dialog *cfd;
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord w, h;
   
   cfdata = data;
   if (!cfdata) return;
   if (cfdata->gui.fsel) return;

   cfd = data2;
   if (!cfd) return;
   
   dia = e_dialog_new(cfd->con, "E", "_mime_icon_select_dialog");
   if (!dia) return;
   e_dialog_title_set(dia, _("Select an Icon"));
   dia->data = cfdata;
   o = e_widget_fsel_add(dia->win->evas, "~/", "/", NULL, NULL,
			 _cb_fsel_sel, cfdata, NULL, cfdata, 1);

   cfdata->gui.fsel_wid = o;
   evas_object_show(o);
   e_widget_min_size_get(o, &w, &h);
   e_dialog_content_set(dia, o, w, h);
   
   e_dialog_button_add(dia, _("OK"), NULL, _cb_fsel_ok, cfdata);
   e_dialog_button_add(dia, _("Cancel"), NULL, _cb_fsel_cancel, cfdata);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_win_resize(dia->win, 475, 341);
   
   cfdata->gui.fsel = dia;
}

static Evas_Object *
_get_icon(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *icon = NULL;
   const char *tmp;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata) return icon;

   e_widget_disabled_set(cfdata->gui.icon, 1);
   
   tmp = e_fm_mime_icon_get(cfdata->icon);
   switch (cfdata->type) 
     {
      case THUMB:
	icon = edje_object_add(evas_object_evas_get(cfdata->gui.icon));
	e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
	break;
      case THEME:
	snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", cfdata->mime);
	icon = edje_object_add(evas_object_evas_get(cfdata->gui.icon));
	if (!e_theme_edje_object_set(icon, "base/theme/fileman", buf))
	  e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
	break;
      case EDJ:
	icon = edje_object_add(evas_object_evas_get(cfdata->gui.icon));
	if (!e_theme_edje_object_set(icon, "base/theme/fileman", tmp))
	  e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
	e_widget_disabled_set(cfdata->gui.icon, 0);
	break;
      default:
	break;
     }
   
   return icon;
}

static void 
_cb_type(void *data, Evas_Object *obj, void *event_info) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   switch (cfdata->type) 
     {
      case THUMB:
	e_widget_disabled_set(cfdata->gui.icon, 1);
	break;
      case THEME:
	e_widget_disabled_set(cfdata->gui.icon, 1);
	break;
      case EDJ:
	e_widget_disabled_set(cfdata->gui.icon, 0);
	break;
      default:
	break;
     }
}

static void 
_cb_fsel_sel(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
}

static void 
_cb_fsel_ok(void *data, E_Dialog *dia) 
{
   E_Config_Dialog_Data *cfdata;
   const char *file;
   
   cfdata = data;
   if (!cfdata) return;
   
   file = e_widget_fsel_selection_path_get(cfdata->gui.fsel_wid);
   IFDUP(file, cfdata->file);
   _cb_fsel_cancel(data, dia);
   if (cfdata->file)
     _cb_file_change(cfdata);
}

static void 
_cb_fsel_cancel(void *data, E_Dialog *dia) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   e_object_del(E_OBJECT(dia));
   cfdata->gui.fsel = NULL;
}

static void 
_cb_file_change(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   if (!cfdata->file) return;
   switch (cfdata->type) 
     {
      case EDJ:
	if (!strstr(cfdata->file, ".edj")) return;
//	if (!edje_file_group_exists(cfdata->file, "")) return;
      default:
	return;
	break;
     }
   
   printf("File: %s\n", cfdata->file);
}
