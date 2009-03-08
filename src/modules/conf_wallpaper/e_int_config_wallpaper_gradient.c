/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#define GRAD_H 0
#define GRAD_V 1
#define GRAD_DU 2
#define GRAD_DD 3
#define GRAD_RAD 4

#define TYPE_LINEAR 0
#define TYPE_RADIAL 1

typedef struct _Import Import;

struct _Import 
{
   E_Config_Dialog *parent;
   E_Config_Dialog_Data *cfdata;
  
   E_Dialog    *dia;
   
   Evas_Object *fill_h_obj;
   Evas_Object *fill_v_obj;
   Evas_Object *fill_du_obj;
   Evas_Object *fill_dd_obj;
   Evas_Object *fill_rad_obj;

   Evas_Object *well1, *well2;

   Evas_Object *preview;

   Ecore_Exe *exe;
   Ecore_Event_Handler *exe_handler;
   char *tmpf;
   char *fdest;
};

struct _E_Config_Dialog_Data 
{
   char *name;   
   int mode;
   int spread;

   E_Color *color1, *color2;
};

static void _import_edj_gen(Import *import);
static int _import_cb_edje_cc_exit(void *data, int type, void *event);
static void _import_cb_close(void *data, E_Dialog *dia);
static void _import_cb_ok(void *data, E_Dialog *dia);
static void _import_config_save(Import *import);
static void _import_cb_dia_del(void *obj);
static Evas_Object *_preview_widget_add(Evas *evas);
static void _import_cb_on_change(void *data, Evas_Object *obj);
static void _import_cb_color_swap(void *data, void *data2);

EAPI E_Dialog *
e_int_config_wallpaper_gradient(E_Config_Dialog *parent)
{
   Evas *evas;
   E_Dialog *dia;
   Import *import;
   Evas_Object *o, *ol, *of, *ord, *ot, *ott;
   Evas_Coord mw, mh;
   E_Radio_Group *rg;
   E_Config_Dialog_Data *cfdata;

   import = E_NEW(Import, 1);
   if (!import) return NULL;

   dia = e_dialog_new(parent->con, "E", "_wallpaper_gradient_dialog");
   if (!dia) 
     { 
	free(import);
	return NULL;	
     }

   dia->data = import;
   e_object_del_attach_func_set(E_OBJECT(dia), _import_cb_dia_del);
   e_win_centered_set(dia->win, 1);

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->mode = GRAD_H;
   cfdata->spread = 0;
   import->cfdata = cfdata;
   import->dia = dia;

   cfdata->name = strdup("gradient");

   evas = e_win_evas_get(dia->win);

   import->parent = parent;

   e_dialog_title_set(dia, _("Create a gradient..."));

   cfdata->color1 = calloc(1, sizeof(E_Color));
   cfdata->color1->a = 255;
   cfdata->color2 = calloc(1, sizeof(E_Color));
   cfdata->color2->a = 255;

   cfdata->color1->r = e_config->wallpaper_grad_c1_r;
   cfdata->color1->g = e_config->wallpaper_grad_c1_g;
   cfdata->color1->b = e_config->wallpaper_grad_c1_b;
   cfdata->color2->r = e_config->wallpaper_grad_c2_r;
   cfdata->color2->g = e_config->wallpaper_grad_c2_g;
   cfdata->color2->b = e_config->wallpaper_grad_c2_b;
   e_color_update_rgb(cfdata->color1);
   e_color_update_rgb(cfdata->color2);

   ol = e_widget_list_add(evas, 0, 0);

   ot = e_widget_table_add(evas, 0);
   evas_object_show(ot);
   ott = e_widget_table_add(evas, 0);
   evas_object_show(ott);

   o = e_widget_label_add(evas, _("Name:"));
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 0, 0, 1, 1, 0, 1, 0, 1);

   o = e_widget_label_add(evas, _("Color 1:"));
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 0, 1, 1, 1, 0, 1, 0, 1);

   o = e_widget_label_add(evas, _("Color 2:"));
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 0, 2, 1, 1, 0, 1, 0, 1);

   o = e_widget_entry_add(evas, &(cfdata->name), NULL, NULL, NULL);
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 1, 0, 1, 1, 1, 1, 1, 1);

   o = e_widget_color_well_add(evas, cfdata->color1, 1);
   e_widget_on_change_hook_set(o, _import_cb_on_change, import);
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 1, 1, 1, 1, 1, 1, 1, 1);
   import->well1 = o;

   o = e_widget_color_well_add(evas, cfdata->color2, 1);
   e_widget_on_change_hook_set(o, _import_cb_on_change, import);
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 1, 2, 1, 1, 1, 1, 1, 1);
   import->well2 = o;

   o = e_widget_button_add(evas, "Swap Colors", "edit-swap", _import_cb_color_swap, import, NULL);
   evas_object_show(o);
   e_widget_table_object_append(ott, o, 1, 3, 1, 1, 1, 1, 1, 1);

   e_widget_table_object_append(ot, ott, 0, 0, 1, 1, 1, 1, 1, 1);   

   of = e_widget_framelist_add(evas, _("Preview"), 1);   
   o = _preview_widget_add(evas);
   evas_object_show(o);
   evas_object_resize(o, 200, 150);
   e_widget_min_size_set(o, 200, 150);
   e_widget_framelist_object_append(of, o);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   import->preview = o;
   _import_cb_on_change(import, NULL);
   
   of = e_widget_framelist_add(evas, _("Fill Options"), 1);
   rg = e_widget_radio_group_new(&(cfdata->mode));

   ord = e_widget_radio_icon_add(evas, _("Horizontal"), "enlightenment/gradient_h", 24, 24, GRAD_H, rg);
   e_widget_on_change_hook_set(ord, _import_cb_on_change, import);
   import->fill_h_obj = ord;
   e_widget_framelist_object_append(of, ord);

   ord = e_widget_radio_icon_add(evas, _("Vertical"), "enlightenment/gradient_v", 24, 24, GRAD_V, rg);
   e_widget_on_change_hook_set(ord, _import_cb_on_change, import);
   import->fill_h_obj = ord;
   e_widget_framelist_object_append(of, ord);

   ord = e_widget_radio_icon_add(evas, _("Diagonal Up"), "enlightenment/gradient_du", 24, 24, GRAD_DU, rg);
   e_widget_on_change_hook_set(ord, _import_cb_on_change, import);
   import->fill_h_obj = ord;
   e_widget_framelist_object_append(of, ord);

   ord = e_widget_radio_icon_add(evas, _("Diagonal Down"), "enlightenment/gradient_dd", 24, 24, GRAD_DD, rg);
   e_widget_on_change_hook_set(ord, _import_cb_on_change, import);
   import->fill_h_obj = ord;
   e_widget_framelist_object_append(of, ord);

   ord = e_widget_radio_icon_add(evas, _("Radial"), "enlightenment/gradient_rad", 24, 24, GRAD_RAD, rg);
   e_widget_on_change_hook_set(ord, _import_cb_on_change, import);
   import->fill_h_obj = ord;
   e_widget_framelist_object_append(of, ord);

   e_widget_table_object_append(ot, of, 0, 2, 2, 1, 1, 1, 1, 1);
   e_widget_list_object_append(ol, ot, 1, 1, 0.5);

   e_widget_min_size_get(ol, &mw, &mh);
   e_dialog_content_set(dia, ol, mw, mh);

   e_dialog_button_add(dia, _("OK"), NULL, _import_cb_ok, cfdata);
   e_dialog_button_add(dia, _("Cancel"), NULL, _import_cb_close, cfdata);

   e_dialog_resizable_set(dia, 0);
   e_dialog_show(dia);
   
   e_dialog_border_icon_set(dia, "enlightenment/gradient");
   return dia;
}

void
e_int_config_wallpaper_gradient_del(E_Dialog *dia)
{
   Import *import;

   import = dia->data;
   _import_config_save(import);

   if (import->exe_handler) ecore_event_handler_del(import->exe_handler);
   import->exe_handler = NULL;
   if (import->tmpf) unlink(import->tmpf);
   E_FREE(import->tmpf);
   E_FREE(import->fdest);
   import->exe = NULL;

   e_int_config_wallpaper_gradient_done(import->parent);
   E_FREE(import->cfdata->name);
   E_FREE(import->cfdata->color1);
   E_FREE(import->cfdata->color2);
   E_FREE(import->cfdata);
   E_FREE(import);
   e_object_unref(E_OBJECT(dia));

}

static void
_import_config_save(Import *import)
{
   if (import->cfdata->color1)
     {
	e_config->wallpaper_grad_c1_r = import->cfdata->color1->r;
	e_config->wallpaper_grad_c1_g = import->cfdata->color1->g;
	e_config->wallpaper_grad_c1_b = import->cfdata->color1->b;
     }
   if (import->cfdata->color2)
     {
	e_config->wallpaper_grad_c2_r = import->cfdata->color2->r;
	e_config->wallpaper_grad_c2_g = import->cfdata->color2->g;
	e_config->wallpaper_grad_c2_b = import->cfdata->color2->b;
     }
   e_config_save_queue();
}

static void 
_import_edj_gen(Import *import)
{
   Evas *evas;
   int fd, num = 1;
   const char *file;
   const char *homedir;
   char buf[4096], cmd[4096], tmpn[4096];
   char *fstrip;
   FILE *f;

   evas = e_win_evas_get(import->dia->win);

   file = import->cfdata->name;
   homedir = e_user_homedir_get();
   fstrip = ecore_file_strip_ext(file);
   if (!fstrip) return;
   snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds/%s.edj", homedir, fstrip);
   while (ecore_file_exists(buf))
     {
	snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds/%s-%i.edj", homedir, fstrip, num);
	num++;
     }
   free(fstrip);
   strcpy(tmpn, "/tmp/e_bgdlg_new.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0) 
     {
	printf("Error Creating tmp file: %s\n", strerror(errno));
	return;
     }
   close(fd);
  
   f = fopen(tmpn, "w");
   if (!f) 
     {
	printf("Cannot open %s for writing\n", tmpn);
	return;
     }
   
   fstrip = strdup(e_util_filename_escape(file));

   fprintf(f,
	 "spectra { spectrum { name: \"gradient\"; color: %d %d %d 255 1; color: %d %d %d 255 1; } }\n"
	 "collections {\n"
	 "group {\n"
	 "name: \"e/desktop/background\";\n"
	 "parts {\n"
	 "part {\n"
	 "  name: \"gradient\";\n"
	 "  type: GRADIENT;\n"
	 "  description {\n"
	 "    state: \"default\" 0.0;\n"
	 "    gradient.spectrum: \"gradient\";\n",
	 import->cfdata->color1->r, import->cfdata->color1->g, import->cfdata->color1->b, 
	 import->cfdata->color2->r, import->cfdata->color2->g, import->cfdata->color2->b);

   switch (import->cfdata->mode) 
     {
      case GRAD_H:
	 fprintf(f,
	       "    gradient.rel1.relative: 0 0.5;\n"
	       "    gradient.rel1.offset: 0 0;\n"
	       "    gradient.rel2.relative: 1 0.5;\n"
	       "    gradient.rel2.offset: -1 0;\n");
	 break;
      case GRAD_V:
	 break;
      case GRAD_DU:
	 fprintf(f,
	       "    gradient.rel1.relative: 0 1;\n"
	       "    gradient.rel1.offset: 0 -1;\n"
	       "    gradient.rel2.relative: 1 0;\n"
	       "    gradient.rel2.offset: -1 0;\n");
	 break;
      case GRAD_DD:
	 fprintf(f,
	       "    gradient.rel1.relative: 0 0;\n"
	       "    gradient.rel1.offset: 0 0;\n"
	       "    gradient.rel2.relative: 1 1;\n"
	       "    gradient.rel2.offset: -1 -1;\n");
	 break;
      case GRAD_RAD:
	 fprintf(f,
	       "    gradient.type: \"radial\";\n"
	       "    fill.origin.relative: 0.5 0.5;\n");
	 break;
      default:
	/* won't happen */
	break;
	
     }
   fprintf(f, "}}}}}\n");
   free(fstrip);
   
   fclose(f);

   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s", 
	    tmpn, e_util_filename_escape(buf));

   import->tmpf = strdup(tmpn);
   import->fdest = strdup(buf);
   import->exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _import_cb_edje_cc_exit, import);
   import->exe = ecore_exe_run(cmd, NULL);   
}

static int
_import_cb_edje_cc_exit(void *data, int type, void *event)
{
   Import *import;
   Ecore_Exe_Event_Del *ev;
  
   ev = event;
   import = data;
   if (ev->exe != import->exe) return 1;

   if (ev->exit_code != 0)
     {
	e_util_dialog_show(_("Gradient Creation Error"),
			   _("For some reason, Enlightenment was unable to create a gradient."));
     }
   
   e_int_config_wallpaper_update(import->parent, import->fdest);

   e_int_config_wallpaper_gradient_del(import->dia);
   return 0;
}

static void 
_import_cb_close(void *data, E_Dialog *dia) 
{
   e_int_config_wallpaper_gradient_del(dia);
}

static void 
_import_cb_ok(void *data, E_Dialog *dia) 
{
   Import *import;

   import = dia->data;

   if (dia && import->cfdata->name)
     {
	_import_edj_gen(import);
	return;
     }
   e_int_config_wallpaper_gradient_del(dia);
}

static void
_import_cb_dia_del(void *obj)
{
   E_Dialog *dia = obj;
   e_int_config_wallpaper_gradient_del(dia);
}

static Evas_Object *
_preview_widget_add(Evas *evas)
{
   Evas_Object *obj, *o, *g;

   obj = e_widget_add(evas);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/gradpreview");
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_show(o);

   g = evas_object_gradient_add(evas);
   evas_object_show(g);
   e_widget_sub_object_add(obj, g);
   edje_object_part_swallow(o, "e.swallow.content", g);

   e_widget_data_set(obj, g);

   return obj;
}

static void
_import_cb_on_change(void *data, Evas_Object *obj)
{
   Import *import;
   Evas_Object *grad;
   Evas_Coord w, h;

   import = data;

   grad = e_widget_data_get(import->preview);
   evas_object_geometry_get(grad, NULL, NULL, &w, &h);

   evas_object_gradient_clear(grad);
   evas_object_gradient_color_stop_add(grad, import->cfdata->color1->r, import->cfdata->color1->g, import->cfdata->color1->b, 255, 1);
   evas_object_gradient_color_stop_add(grad, import->cfdata->color2->r, import->cfdata->color2->g, import->cfdata->color2->b, 255, 1);

   switch (import->cfdata->mode)
     {
      case GRAD_H:
	 evas_object_gradient_type_set(grad, "linear", NULL);
	 evas_object_gradient_angle_set(grad, 270);
	 evas_object_gradient_fill_set(grad, 0, 0, w, h);
	 break;
      case GRAD_V:
	 evas_object_gradient_type_set(grad, "linear", NULL);
	 evas_object_gradient_angle_set(grad, 0);
	 evas_object_gradient_fill_set(grad, 0, 0, w, h);
	 break;
      case GRAD_DU:
	 evas_object_gradient_type_set(grad, "linear.codiag", NULL);
	 evas_object_gradient_angle_set(grad, 180);
	 evas_object_gradient_fill_set(grad, 0, 0, w, h);
	 break;
      case GRAD_DD:
	 evas_object_gradient_type_set(grad, "linear.diag", NULL);
	 evas_object_gradient_angle_set(grad, 0);
	 evas_object_gradient_fill_set(grad, 0, 0, w, h);
	 break;
      case GRAD_RAD:
	 evas_object_gradient_type_set(grad, "radial", NULL);
	 evas_object_gradient_fill_set(grad, w / 2, h / 2, w, h);
	 break;
     }
}

static void
_import_cb_color_swap(void *data, void *data2)
{
   Import *import;
   E_Color *tmp;

   import = data;

   tmp = malloc(sizeof(E_Color));
   e_color_copy(import->cfdata->color1, tmp);
   e_color_copy(import->cfdata->color2, import->cfdata->color1);
   e_color_copy(tmp, import->cfdata->color2);

   e_widget_color_well_update(import->well1);
   e_widget_color_well_update(import->well2);
   _import_cb_on_change(import, NULL);
}
