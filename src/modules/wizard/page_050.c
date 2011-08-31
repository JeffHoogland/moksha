/* Ask about Scaling */
#include "e.h"
#include "e_mod_main.h"

static double scale = 1.0;
static Eina_List *obs = NULL;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

static void
_scale_preview_sel_set(Evas_Object *ob, int sel)
{
   Evas_Object *rc, *ob2;
   double *sc, scl;
   int v;
   Eina_List *l;
   
   rc = evas_object_data_get(ob, "rec");
   if (sel)
     {
        evas_object_color_set(rc, 0, 0, 0, 0);
        sc = evas_object_data_get(ob, "scalep");
        v = (int)(unsigned long)evas_object_data_get(ob, "scale");
        scl = (double)v / 1000.0;
        if (sc) *sc = scl;
        EINA_LIST_FOREACH(obs, l, ob2)
          {
             if (ob == ob2) continue;
             _scale_preview_sel_set(ob2, 0);
          }
     }
   else evas_object_color_set(rc, 0, 0, 0, 192);
}

static void
_scale_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *ob = data;
   
   _scale_preview_sel_set(ob, 1);
}

static Evas_Object *
_scale_preview_new(Evas *e, double sc, double *scp)
{
   Evas_Object *ob, *bg, *cm, *bd, *wb, *rc;
   const char *file;
   char buf[64];
   int v;
   
#define SZW 110
#define SZH 80
   ob = e_widget_preview_add(e, SZW, SZH);
   e_widget_preview_vsize_set(ob, SZW, SZH);
   
   bg = edje_object_add(e_widget_preview_evas_get(ob));
   file = e_bg_file_get(0, 0, 0, 0);
   edje_object_file_set(bg, file, "e/desktop/background");
   evas_object_move(bg, 0, 0);
   evas_object_resize(bg, 640, 480);
   evas_object_show(bg);
   
   cm = edje_object_add(e_widget_preview_evas_get(ob));
   e_theme_edje_object_set(cm, "base/theme/borders", "e/comp/default");
   evas_object_move(cm, 16, 16);
   evas_object_resize(cm, 320, 400);
   evas_object_show(cm);
   
   bd = edje_object_add(e_widget_preview_evas_get(ob));
   e_theme_edje_object_set(bd, "base/theme/borders", "e/widgets/border/default/border");
   edje_object_part_swallow(cm, "e.swallow.content", bd);
   evas_object_show(bd);
   
   wb = edje_object_add(e_widget_preview_evas_get(ob));
   e_theme_edje_object_set(wb, "base/theme/dialog", "e/widgets/dialog/main");
   edje_object_part_swallow(bd, "e.swallow.client", wb);
   evas_object_show(wb);
   
   rc = evas_object_rectangle_add(e_widget_preview_evas_get(ob));
   evas_object_move(rc, 0, 0);
   evas_object_resize(rc, 640, 480);
   evas_object_color_set(rc, 0, 0, 0, 192);
   evas_object_show(rc);
   
   snprintf(buf, sizeof(buf), "%1.1f %s", sc, _("Title"));
   edje_object_part_text_set(bd, "e.text.title", buf);
   edje_object_signal_emit(bd, "e,state,focused", "e");
   
   edje_object_signal_emit(cm, "e,state,visible,on", "e");
   edje_object_signal_emit(cm, "e,state,shadow,on", "e");
   edje_object_signal_emit(cm, "e,state,focus,on", "e");
   
   edje_object_scale_set(bd, sc);
   edje_object_scale_set(cm, sc);
   edje_object_scale_set(bg, sc);
   edje_object_scale_set(wb, sc);
   
   evas_object_data_set(ob, "rec", rc);
   v = sc * 1000;
   evas_object_data_set(ob, "scale", (void *)(unsigned long)v);
   evas_object_data_set(ob, "scalep", scp);

   evas_object_event_callback_add(rc,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _scale_down, ob);
   obs = eina_list_append(obs, ob);
   
   return ob;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   Evas_Coord sw, sh;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Sizing"));
   
   of = e_widget_frametable_add(pg->evas, _("Select preferred size"), 1);
   e_widget_frametable_content_align_set(of, 0.5, 0.5);
   
   ob = _scale_preview_new(pg->evas, 0.80, &scale);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(of, ob, 0, 0, 1, 1, 0, 0, 1, 1, 0.5, 0.5, sw, sh, sw, sh);
   evas_object_show(ob);
   
   ob = _scale_preview_new(pg->evas, 1.00, &scale);
   _scale_preview_sel_set(ob, 1);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(of, ob, 1, 0, 1, 1, 0, 0, 1, 1, 0.5, 0.5, sw, sh, sw, sh);
   evas_object_show(ob);
   
   ob = _scale_preview_new(pg->evas, 1.20, &scale);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(of, ob, 0, 1, 1, 1, 0, 0, 1, 1, 0.5, 0.5, sw, sh, sw, sh);
   evas_object_show(ob);
   
   ob = _scale_preview_new(pg->evas, 1.50, &scale);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(of, ob, 1, 1, 1, 1, 0, 0, 1, 1, 0.5, 0.5, sw, sh, sw, sh);
   evas_object_show(ob);
   
   ob = _scale_preview_new(pg->evas, 1.70, &scale);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(of, ob, 0, 2, 1, 1, 0, 0, 1, 1, 0.5, 0.5, sw, sh, sw, sh);
   evas_object_show(ob);
   
   ob = _scale_preview_new(pg->evas, 2.00, &scale);
   e_widget_size_min_get(ob, &sw, &sh);
   e_widget_frametable_object_append_full(of, ob, 1, 2, 1, 1, 0, 0, 1, 1, 0.5, 0.5, sw, sh, sw, sh);
   evas_object_show(ob);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   
   e_wizard_page_show(o);
//   pg->data = o;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   obs = eina_list_free(obs);
//   evas_object_del(pg->data);
   
   e_config->scale.use_dpi = 0;
   e_config->scale.use_custom = 1;
   e_config->scale.factor = scale;
   e_scale_update();
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
