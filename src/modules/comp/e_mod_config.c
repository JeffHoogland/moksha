#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_comp.h"
#include "config.h"

typedef struct _E_Demo_Style_Item
{
   Evas_Object *preview;
   Evas_Object *frame;
} E_Demo_Style_Item;

struct _E_Config_Dialog_Data
{
   int use_shadow;
   int engine;
   int indirect;
   int texture_from_pixmap;
   int smooth_windows;
   int lock_fps;
   int efl_sync;
   int loose_sync;
   int grab;
   int vsync;

   Evas_Object *style_ilist;
   Ecore_Timer *style_demo_timer;
   Eina_List *style_shadows;
   int style_demo_state;
   const char *shadow_style;
   
   int keep_unmapped;
   int max_unmapped_pixels;
   int max_unmapped_time;
   int min_unmapped_time;
   int send_flush;
   int send_dump;
   int nocomp_fs;
};

/* Protos */
static void        *_create_data          (E_Config_Dialog *cfd);
static void         _free_data            (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_comp_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   Mod *mod = _comp_mod;

   if (e_config_dialog_find("E", "appearance/comp")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   snprintf(buf, sizeof(buf), "%s/e-module-comp.edj", 
            e_module_dir_get(mod->module));
   cfd = e_config_dialog_new(con,
			     _("Composite Settings"),
			     "E", "appearance/comp",
			     buf, 0, v, mod);
   mod->config_dialog = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   cfdata->use_shadow = _comp_mod->conf->use_shadow;
   cfdata->engine = _comp_mod->conf->engine;
   if ((cfdata->engine != E_EVAS_ENGINE_SOFTWARE_X11) &&
       (cfdata->engine != E_EVAS_ENGINE_GL_X11))
     cfdata->engine = E_EVAS_ENGINE_SOFTWARE_X11;
   cfdata->indirect = _comp_mod->conf->indirect;
   cfdata->texture_from_pixmap = _comp_mod->conf->texture_from_pixmap;
   cfdata->smooth_windows = _comp_mod->conf->smooth_windows;
   cfdata->lock_fps = _comp_mod->conf->lock_fps;
   cfdata->efl_sync = _comp_mod->conf->efl_sync;
   cfdata->loose_sync = _comp_mod->conf->loose_sync;
   cfdata->grab = _comp_mod->conf->grab;
   cfdata->vsync = _comp_mod->conf->vsync;
   if (_comp_mod->conf->shadow_style)
     cfdata->shadow_style = eina_stringshare_add(_comp_mod->conf->shadow_style);

   cfdata->keep_unmapped = _comp_mod->conf->keep_unmapped;
   cfdata->max_unmapped_pixels = _comp_mod->conf->max_unmapped_pixels;
   cfdata->max_unmapped_time = _comp_mod->conf->max_unmapped_time;
   cfdata->min_unmapped_time = _comp_mod->conf->min_unmapped_time;
   cfdata->send_flush = _comp_mod->conf->send_flush;
   cfdata->send_dump = _comp_mod->conf->send_dump;
   cfdata->nocomp_fs = _comp_mod->conf->nocomp_fs;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Demo_Style_Item *ds_it;

   _comp_mod->config_dialog = NULL;
   if (cfdata->shadow_style) eina_stringshare_del(cfdata->shadow_style);
   if (cfdata->style_demo_timer) ecore_timer_del(cfdata->style_demo_timer);

   EINA_LIST_FREE(cfdata->style_shadows, ds_it)
     {
	evas_object_del(ds_it->preview);
	evas_object_del(ds_it->frame);
	free(ds_it);
     }

   free(cfdata);
}

static int
_demo_styles(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   const E_Demo_Style_Item *it;
   const Eina_List *l;

   cfdata->style_demo_state = (cfdata->style_demo_state + 1) % 4;
   EINA_LIST_FOREACH(cfdata->style_shadows, l, it)
     {
	Evas_Object *ob = it->preview;
	Evas_Object *of = it->frame;
	switch (cfdata->style_demo_state)
	  {
	   case 0:
	      edje_object_signal_emit(ob, "e,state,visible,on", "e");
	      edje_object_signal_emit(ob, "e,state,focus,on", "e");
	      edje_object_part_text_set(of, "e.text.label", _("Visible"));
	      break;
	   case 1:
	      edje_object_signal_emit(ob, "e,state,focus,off", "e");
	      edje_object_part_text_set(of, "e.text.label", _("Focus-Out"));
	      break;
	   case 2:
	      edje_object_signal_emit(ob, "e,state,focus,on", "e");
	      edje_object_part_text_set(of, "e.text.label", _("Focus-In"));
	      break;
	   case 3:
	      edje_object_signal_emit(ob, "e,state,visible,off", "e");
	      edje_object_part_text_set(of, "e.text.label", _("Hidden"));
	      break;
	   default:
	      break;
	  }
     }
   return 1;
}

static void
_shadow_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata = data;
   const E_Demo_Style_Item *it;
   const Eina_List *l;

   EINA_LIST_FOREACH(cfdata->style_shadows, l, it)
     {
	if (cfdata->use_shadow)
	  edje_object_signal_emit(it->preview, "e,state,shadow,on", "e");
	else
	  edje_object_signal_emit(it->preview, "e,state,shadow,off", "e");
     }
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ob, *ol, *ol2, *of, *otb, *oi, *oly, *oo, *obd, *orec;
   E_Radio_Group *rg;
   Eina_List *styles, *l;
   char *style;
   int n, sel;
   Evas_Coord wmw, wmh;
   
   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);
   
   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Shadows"), &(cfdata->use_shadow));
   evas_object_smart_callback_add(ob, "changed", _shadow_changed, cfdata);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   ob = e_widget_check_add(evas, _("Limit framerate"), &(cfdata->lock_fps));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   ob = e_widget_check_add(evas, _("Smooth scaling"), &(cfdata->smooth_windows));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   
   of = e_widget_frametable_add(evas, _("Default Style"), 0);
   e_widget_frametable_content_align_set(of, 0.5, 0.5);
   oi = e_widget_ilist_add(evas, 80, 80, &(cfdata->shadow_style));
   sel = 0;
   styles = e_theme_comp_list();
   n = 0;
   EINA_LIST_FOREACH(styles, l, style)
     {
	E_Demo_Style_Item *ds_it;
        char buf[PATH_MAX];

        ob = e_livethumb_add(evas);
        e_livethumb_vsize_set(ob, 240, 240);

        oly = e_layout_add(e_livethumb_evas_get(ob));
        e_layout_virtual_size_set(oly, 240, 240);
        e_livethumb_thumb_set(ob, oly);
        evas_object_show(oly);

        oo = edje_object_add(e_livethumb_evas_get(ob));
        snprintf(buf, sizeof(buf), "e/comp/%s", style);
        e_theme_edje_object_set(oo, "base/theme/borders", buf);
        e_layout_pack(oly, oo);
        e_layout_child_move(oo, 39, 39);
        e_layout_child_resize(oo, 162, 162);
        if (cfdata->use_shadow)
          edje_object_signal_emit(oo, "e,state,shadow,on", "e");
        edje_object_signal_emit(oo, "e,state,visible,on", "e");
        evas_object_show(oo);

	ds_it = malloc(sizeof(E_Demo_Style_Item));
	ds_it->preview = oo;
	ds_it->frame = edje_object_add(evas);
	e_theme_edje_object_set
	  (ds_it->frame, "base/theme/modules/comp", "e/modules/comp/preview");
	edje_object_part_swallow(ds_it->frame, "e.swallow.preview", ob);
	evas_object_show(ds_it->frame);
        cfdata->style_shadows = eina_list_append(cfdata->style_shadows, ds_it);
        
        obd = edje_object_add(e_livethumb_evas_get(ob));
        e_theme_edje_object_set(obd, "base/theme/borders",
                                "e/widgets/border/default/border");
        edje_object_part_text_set(obd, "e.text.title", _("Title"));
        edje_object_signal_emit(obd, "e,state,focused", "e");
        edje_object_part_swallow(oo, "e.swallow.content", obd);
        evas_object_show(obd);
        
        orec = evas_object_rectangle_add(e_livethumb_evas_get(ob));
        evas_object_color_set(orec, 255, 255, 255, 255);
        edje_object_part_swallow(obd, "e.swallow.client", orec);
        evas_object_show(orec);
        
        e_widget_ilist_append(oi, ds_it->frame, style, NULL, NULL, style);
        evas_object_show(ob);
        if (cfdata->shadow_style)
          {
             if (!strcmp(cfdata->shadow_style, style)) sel = n;
          }
        n++;
     }
   cfdata->style_ilist = oi;
   cfdata->style_demo_timer = ecore_timer_add(3.0, _demo_styles, cfdata);
   cfdata->style_demo_state = 1;
   e_widget_size_min_get(oi, &wmw, &wmh);
   e_widget_size_min_set(oi, 160, 100);
   e_widget_ilist_selected_set(oi, sel);
   e_widget_ilist_go(oi);
   e_widget_frametable_object_append(of, oi, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Effects"), ol, 1, 1, 1, 1, 0.5, 0.0);
   
   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Sync screen (VBlank)"), &(cfdata->vsync));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sync windows"), &(cfdata->efl_sync));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Loose sync"), &(cfdata->loose_sync));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Grab Server during draw"), &(cfdata->grab));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Sync"), ol, 0, 0, 0, 0, 0.5, 0.0);
   
   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->engine));
   ob = e_widget_radio_add(evas, _("Software"), E_EVAS_ENGINE_SOFTWARE_X11, rg);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_OPENGL_X11))
     {
        ob = e_widget_radio_add(evas, _("OpenGL"), E_EVAS_ENGINE_GL_X11, rg);
        e_widget_list_object_append(ol, ob, 1, 1, 0.5);

        of = e_widget_framelist_add(evas, _("OpenGL options"), 0);
        e_widget_framelist_content_align_set(of, 0.5, 0.0);
        ob = e_widget_check_add(evas, _("Texture from pixmap"), &(cfdata->texture_from_pixmap));
        e_widget_framelist_object_append(of, ob);
        ob = e_widget_check_add(evas, _("Indirect OpenGL"), &(cfdata->indirect));
        e_widget_framelist_object_append(of, ob);
        e_widget_list_object_append(ol, of, 1, 1, 0.5);
     }
   e_widget_toolbook_page_append(otb, NULL, _("Engine"), ol, 0, 0, 0, 0, 0.5, 0.0);
   
   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Send flush"), &(cfdata->send_flush));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Send dump"), &(cfdata->send_dump));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Don't composite fullscreen"), &(cfdata->nocomp_fs));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Keep hidden windows"), &(cfdata->keep_unmapped));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   of = e_widget_frametable_add(evas, _("Maximum hidden pixels"), 0);
   e_widget_frametable_content_align_set(of, 0.5, 0.5);
   rg = e_widget_radio_group_new(&(cfdata->max_unmapped_pixels));
   ob = e_widget_radio_add(evas, _("1M"), 1 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("2M"), 2 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("4M"), 4 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("8M"), 8 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("16M"), 16 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("32M"), 32 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("64M"), 64 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 2, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("128M"), 128 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 2, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("256M"), 256 * 1024, rg);
   e_widget_frametable_object_append(of, ob, 2, 2, 1, 1, 1, 1, 0, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Memory"), ol, 0, 0, 0, 0, 0.5, 0.0);
   
   ///////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ol2 = e_widget_list_add(evas, 1, 1);
   of = e_widget_framelist_add(evas, _("Min hidden"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->min_unmapped_time));
   ob = e_widget_radio_add(evas, _("30 Seconds"), 30, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("1 Minute"), 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("5 Minutes"), 5 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("30 Minutes"), 30 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("2 Hours"), 2 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("10 Hours"), 10 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Forever"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);
   of = e_widget_framelist_add(evas, _("Max hidden"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->max_unmapped_time));
   ob = e_widget_radio_add(evas, _("30 Seconds"), 30, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("1 Minute"), 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("5 Minutes"), 5 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("30 Minutes"), 30 * 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("2 Hours"), 2 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("10 Hours"), 10 * 3600, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Forever"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);
   e_widget_list_object_append(ol, ol2, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Timeouts"), ol, 0, 0, 0, 0, 0.5, 0.0);
   
   e_widget_toolbook_page_show(otb, 0);
   
   e_dialog_resizable_set(cfd->dia, 1);
   return otb;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if ((cfdata->use_shadow != _comp_mod->conf->use_shadow) ||
       (cfdata->lock_fps != _comp_mod->conf->lock_fps) ||
       (cfdata->smooth_windows != _comp_mod->conf->smooth_windows) ||
       (cfdata->grab != _comp_mod->conf->grab) ||
       (cfdata->keep_unmapped != _comp_mod->conf->keep_unmapped) ||
       (cfdata->nocomp_fs != _comp_mod->conf->nocomp_fs) ||
       (cfdata->shadow_style != _comp_mod->conf->shadow_style) ||
       (cfdata->max_unmapped_pixels != _comp_mod->conf->max_unmapped_pixels) ||
       (cfdata->max_unmapped_time != _comp_mod->conf->max_unmapped_time) ||
       (cfdata->min_unmapped_time != _comp_mod->conf->min_unmapped_time) ||
       (cfdata->send_flush != _comp_mod->conf->send_flush) ||
       (cfdata->send_dump != _comp_mod->conf->send_dump)
       )
     {
        _comp_mod->conf->use_shadow = cfdata->use_shadow;
        _comp_mod->conf->lock_fps = cfdata->lock_fps;
        _comp_mod->conf->smooth_windows = cfdata->smooth_windows;
        _comp_mod->conf->grab = cfdata->grab;
        _comp_mod->conf->keep_unmapped = cfdata->keep_unmapped;
        _comp_mod->conf->nocomp_fs = cfdata->nocomp_fs;
        _comp_mod->conf->max_unmapped_pixels =  cfdata->max_unmapped_pixels;
        _comp_mod->conf->max_unmapped_time = cfdata->max_unmapped_time;
        _comp_mod->conf->min_unmapped_time = cfdata->min_unmapped_time;
        _comp_mod->conf->send_flush = cfdata->send_flush;
        _comp_mod->conf->send_dump = cfdata->send_dump;
        if (_comp_mod->conf->shadow_style)
          eina_stringshare_del(_comp_mod->conf->shadow_style);
        _comp_mod->conf->shadow_style = NULL;
        if (cfdata->shadow_style)
          _comp_mod->conf->shadow_style = eina_stringshare_add(cfdata->shadow_style);
        e_mod_comp_shadow_set();
     }
   if ((cfdata->engine != _comp_mod->conf->engine) ||
       (cfdata->indirect != _comp_mod->conf->indirect) ||
       (cfdata->texture_from_pixmap != _comp_mod->conf->texture_from_pixmap) ||
       (cfdata->efl_sync != _comp_mod->conf->efl_sync) ||
       (cfdata->loose_sync != _comp_mod->conf->loose_sync) ||
       (cfdata->vsync != _comp_mod->conf->vsync))
     {
        E_Action *a;
        
        _comp_mod->conf->engine = cfdata->engine;
        _comp_mod->conf->indirect = cfdata->indirect;
        _comp_mod->conf->texture_from_pixmap = cfdata->texture_from_pixmap;
        _comp_mod->conf->efl_sync = cfdata->efl_sync;
        _comp_mod->conf->loose_sync = cfdata->loose_sync;
        _comp_mod->conf->vsync = cfdata->vsync;
        
        a = e_action_find("restart");
        if ((a) && (a->func.go)) a->func.go(NULL, NULL);
//        e_mod_comp_shutdown();
//        e_mod_comp_init();
     }
   e_config_save_queue();
   return 1;
}
