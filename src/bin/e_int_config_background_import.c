#include "e.h"

#define E_BG_SCALE 0
#define E_BG_TILE 1
#define E_BG_CENTER 2

/* Personally I hate having to define this twice, but Tileing needs a fill */
#define IMG_EDC_TMPL_TILE \
"images {\n" \
"  image: \"%s\" COMP;\n" \
"}\n" \
"collections {\n" \
"  group {\n" \
"    name: \"desktop/background\";\n" \
"    max: %d %d;\n" \
"    parts {\n" \
"      part {\n" \
"        name: \"background_image\";\n" \
"        type: IMAGE;\n" \
"        mouse_events: 0;\n" \
"        description {\n" \
"          state: \"default\" 0.0;\n" \
"          visible: 1;\n" \
"          rel1 {\n" \
"            relative: 0.0 0.0;\n" \
"            offset: 0 0;\n" \
"          }\n" \
"          rel2 {\n" \
"            relative: 1.0 1.0;\n" \
"            offset: -1 -1;\n" \
"          }\n" \
"          image {\n" \
"            normal: \"%s\";\n" \
"          }\n" \
"          fill {\n" \
"            size {\n" \
"              relative: 0.0 0.0;\n" \
"              offset: %d %d;\n" \
"            }\n" \
"          }\n" \
"        }\n" \
"      }\n" \
"    }\n" \
"  }\n" \
"}\n"

#define IMG_EDC_TMPL \
"images {\n" \
"  image: \"%s\" COMP;\n" \
"}\n" \
"collections {\n" \
"  group {\n" \
"    name: \"desktop/background\";\n" \
"    max: %d %d;\n" \
"    parts {\n" \
"      part {\n" \
"        name: \"background_image\";\n" \
"        type: IMAGE;\n" \
"        mouse_events: 0;\n" \
"        description {\n" \
"          state: \"default\" 0.0;\n" \
"          visible: 1;\n" \
"          rel1 {\n" \
"            relative: 0.0 0.0;\n" \
"            offset: 0 0;\n" \
"          }\n" \
"          rel2 {\n" \
"            relative: 1.0 1.0;\n" \
"            offset: -1 -1;\n" \
"          }\n" \
"          image {\n" \
"            normal: \"%s\";\n" \
"          }\n" \
"        }\n" \
"      }\n" \
"    }\n" \
"  }\n" \
"}\n"

static Ecore_Event_Handler *_edj_exe_exit_handler = NULL;

static void        *_create_data               (E_Config_Dialog *cfd);
static void         _free_data                 (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data          (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets      (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data       (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets   (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _fill_data                 (E_Config_Dialog_Data *cfdata);
static void         _efm_hilite_cb             (Evas_Object *obj, char *file, void *data);
static void         _bg_edj_gen                (Evas *evas, char *filename, int method);
static int          _edj_exe_exit_cb           (void *data, int type, void *event);

struct _E_Config_Dialog_Data 
{
   char *file;   
   int method;
};

EAPI E_Config_Dialog *
e_int_config_background_import(E_Config_Dialog *parent)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   /* Create A New Import Dialog */
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->advanced.apply_cfdata   = NULL;
   v->advanced.create_widgets = NULL;
   
   cfd = e_config_dialog_new(parent->con, _("Import An Image"), NULL, 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   ecore_x_icccm_transient_for_set(cfd->dia->win->evas_win, parent->dia->win->evas_win);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfd->cfdata = cfdata;
   _fill_data(cfdata);
   return cfdata;   
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{  
   E_Fm_File *f;
   Evas *evas;
   
   if (!cfdata->file[0]) return 0;

   f = e_fm_file_new(cfdata->file);
   if (!f) return;
      
   if (!e_fm_file_is_image(f)) return 0;
   free(f);
   
   evas = e_win_evas_get(cfd->dia->win);
   _bg_edj_gen(evas, cfdata->file, cfdata->method);
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ot, *of, *ofm;
   E_Dialog *dia;
   E_Radio_Group *rg;
   
   _fill_data(cfdata);
   
   dia = cfd->dia;
   
   ot = e_widget_table_add(evas, 0);
   
   of = e_widget_framelist_add(evas, _("Image To Import"), 0);

   ofm = e_widget_fileman_add(evas, (&(cfdata->file)));
   e_widget_fileman_hilite_callback_add(ofm, _efm_hilite_cb, dia);
   
   e_widget_framelist_object_append(of, ofm);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Options"), 0);
   rg = e_widget_radio_group_new(&cfdata->method);
   o = e_widget_radio_add(evas, _("Center"), E_BG_CENTER, rg);
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(evas, _("Scale"), E_BG_SCALE, rg);
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(evas, _("Tile"), E_BG_TILE, rg);
   e_widget_framelist_object_append(of, o);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 0, 1, 0);
   
   return ot;
}

static int 
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Fm_File *f;
   Evas *evas;
   
   if (!cfdata->file[0]) return 0;

   f = e_fm_file_new(cfdata->file);
   if (!f) return;
      
   if (!e_fm_file_is_image(f)) return 0;
   free(f);
   
   evas = e_win_evas_get(cfd->dia->win);
   _bg_edj_gen(evas, cfdata->file, cfdata->method);
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ot, *of, *ofm;
   E_Dialog *dia;
   E_Radio_Group *rg;
   
   _fill_data(cfdata);
   
   dia = cfd->dia;
   
   ot = e_widget_table_add(evas, 0);
   
   of = e_widget_framelist_add(evas, _("Image To Import"), 0);

   ofm = e_widget_fileman_add(evas, (&(cfdata->file)));
   e_widget_fileman_hilite_callback_add(ofm, _efm_hilite_cb, dia);
   
   e_widget_framelist_object_append(of, ofm);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Options"), 0);
   rg = e_widget_radio_group_new(&cfdata->method);
   o = e_widget_radio_add(evas, _("Center"), E_BG_CENTER, rg);
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(evas, _("Scale"), E_BG_SCALE, rg);
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(evas, _("Tile"), E_BG_TILE, rg);
   e_widget_framelist_object_append(of, o);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 0, 1, 0);
   
   return ot;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->method = E_BG_SCALE;
}

static void 
_efm_hilite_cb(Evas_Object *obj, char *file, void *data) 
{
   E_Dialog *dia;
   E_Fm_File *f;
   
   dia = (E_Dialog *)data;
   if (!dia) return;

   f = e_fm_file_new(file);
   if (!f) return;
   
   if (e_fm_file_is_image(f)) 
     {
	e_dialog_button_disable_num_set(dia, 0, 0);   
	e_dialog_button_disable_num_set(dia, 1, 0);	
     }
   free(f);
}

static void 
_bg_edj_gen(Evas *evas, char *filename, int method) 
{
   Evas_Object *img;
   int fd = 0;
   int w, h, ret;
   const char *file;
   char buff[4096], cmd[4096];
   char ipart[512];
   char *imgdir = NULL;
   static char tmpn[1024];
   FILE *out = NULL;
   Ecore_Exe *x;
   
   if (!filename) return;
   file = ecore_file_get_file(filename);
   
   snprintf(buff, sizeof(buff), "%s/.e/e/backgrounds/%s.edj", e_user_homedir_get(), ecore_file_strip_ext(file));
   strcpy(tmpn, "/tmp/e_bgdlg_new.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0) 
     {
	printf("Error Creating tmp file: %s\n", strerror(errno));
	return;
     }
   close(fd);
   
   out = fopen(tmpn, "w");
   if (!out) 
     {
	printf("Cannot open %s for writting\n", tmpn);
	return;
     }
   
   imgdir = ecore_file_get_dir(filename);
   if (!imgdir) ipart[0] = '\0';
   if (imgdir) 
     {
	snprintf(ipart, sizeof(ipart), "-id %s", imgdir);
	free(imgdir);
     }

   img = evas_object_image_add(evas);
   evas_object_image_file_set(img, filename, NULL);
   evas_object_image_size_get(img, &w, &h);
   evas_object_del(img);   
   
   switch (method) 
     {
      case E_BG_CENTER:
	fprintf(out, IMG_EDC_TMPL, file, w, h, file);
	break;
      case E_BG_TILE:
	fprintf(out, IMG_EDC_TMPL_TILE, file, w, h, file, w, h);
	break;
      case E_BG_SCALE:
	fprintf(out, IMG_EDC_TMPL, file, w, h, file);
	break;
     }

   fclose(out);

   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s %s", ipart, tmpn, buff);

   _edj_exe_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edj_exe_exit_cb, NULL);
   x = ecore_exe_run(cmd, tmpn);   
}

static int
_edj_exe_exit_cb(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   Ecore_Exe *x;
   
   ev = event;
   if (!ev->exe) return 1;
   
   x = ev->exe;
   if (!x) return 1;
   
   x = NULL;
   ecore_event_handler_del(_edj_exe_exit_handler);

   unlink(data);   

   return 0;
}
