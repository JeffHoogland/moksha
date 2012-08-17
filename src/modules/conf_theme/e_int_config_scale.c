#include "e.h"

/* local function prototypes */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _fill_data(E_Config_Dialog_Data *cfdata);
static void         _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int          _basic_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int          _adv_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void         _adv_policy_changed(void *data, Evas_Object *obj __UNUSED__);

struct _E_Config_Dialog_Data
{
   int    use_dpi;
   double min, max, factor;
   int    use_mode, base_dpi, use_custom;
   struct
   {
      struct
      {
         Evas_Object *o_lbl, *o_slider;
      } basic;
      struct
      {
         Evas_Object *dpi_lbl, *dpi_slider;
         Evas_Object *custom_slider;
         Evas_Object *min_lbl, *min_slider;
         Evas_Object *max_lbl, *max_slider;
      } adv;
   } gui;
   Eina_List *obs;
};

static void
_scale_preview_sel_set(Evas_Object *ob, int sel)
{
   Evas_Object *rc, *ob2;
   double *sc, scl;
   int v;
   Eina_List *l;
   E_Config_Dialog_Data *cfdata;
   
   cfdata = evas_object_data_get(ob, "cfdata");
   rc = evas_object_data_get(ob, "rec");
   if (sel)
     {
        evas_object_color_set(rc, 0, 0, 0, 0);
        sc = evas_object_data_get(ob, "scalep");
        v = (int)(unsigned long)evas_object_data_get(ob, "scale");
        scl = (double)v / 1000.0;
        if (sc) *sc = scl;
        EINA_LIST_FOREACH(cfdata->obs, l, ob2)
          {
             if (ob == ob2) continue;
             _scale_preview_sel_set(ob2, 0);
          }
        if (evas_object_data_get(ob, "dpi"))
          cfdata->use_dpi = EINA_TRUE;
        else
          cfdata->use_dpi = EINA_FALSE;
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
_scale_preview_new(E_Config_Dialog_Data *cfdata, Evas *e, double sc, double *scp, const char *tit, Eina_Bool dpi)
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

   if (!tit)
     {
        snprintf(buf, sizeof(buf), "%1.1f %s", sc, _("Factor"));
        edje_object_part_text_set(bd, "e.text.title", buf);
     }
   else
     edje_object_part_text_set(bd, "e.text.title", tit);
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
   evas_object_data_set(ob, "dpi", (void *)(unsigned long)dpi);
   evas_object_data_set(ob, "cfdata", cfdata);
   
   evas_object_event_callback_add(rc,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _scale_down, ob);
   cfdata->obs = eina_list_append(cfdata->obs, ob);
   
   return ob;
}

E_Config_Dialog *
e_int_config_scale(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/scale")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_changed;
   v->advanced.create_widgets = _adv_create;
   v->advanced.apply_cfdata = _adv_apply;
   v->advanced.check_changed = _adv_changed;

   cfd = e_config_dialog_new(con, _("Scale Settings"), "E", "appearance/scale",
                             "preferences-scale", 0, v, NULL);
   return cfd;
}

/* local function prototypes */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->use_dpi = e_config->scale.use_dpi;
   cfdata->use_custom = e_config->scale.use_custom;
   cfdata->use_mode = 0;
   if (cfdata->use_dpi) cfdata->use_mode = 1;
   else if (cfdata->use_custom)
     cfdata->use_mode = 2;
   cfdata->min = e_config->scale.min;
   cfdata->max = e_config->scale.max;
   cfdata->factor = e_config->scale.factor;
   cfdata->base_dpi = e_config->scale.base_dpi;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->obs);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob;
   double sc = 1.0;
   int dpi, x = 0, y = 0;

   o = e_widget_table_add(evas, 1);

   dpi = ecore_x_dpi_get();
   if ((dpi > 0) && (cfdata->base_dpi > 0))
     sc = (double)dpi / (double)cfdata->base_dpi;
   
   ob = _scale_preview_new(cfdata, evas, sc, &(cfdata->factor), _("DPI Scaling"), EINA_TRUE);
   e_widget_table_object_align_append(o, ob, 0, 0, 1, 1, 0, 0, 0, 0, 0.5, 0.5);
   if (cfdata->use_dpi) _scale_preview_sel_set(ob, 1);
   
   x = 1;

#define COL 3   
#define SCALE_OP(v) do { \
   ob = _scale_preview_new(cfdata, evas, v, &(cfdata->factor), NULL, EINA_FALSE); \
   e_widget_table_object_align_append(o, ob, x, y, 1, 1, 0, 0, 0, 0, 0.5, 0.5); \
   if (cfdata->factor == v) _scale_preview_sel_set(ob, 1); \
   x++; if (x >= COL) { x = 0; y++; } \
} while (0)

   SCALE_OP(0.8);
   SCALE_OP(1.0);
   SCALE_OP(1.2);
   SCALE_OP(1.5);
   SCALE_OP(1.7);
   SCALE_OP(1.9);
   SCALE_OP(2.0);
   SCALE_OP(2.2);
   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   cfdata->use_custom = 0;
   if (cfdata->use_dpi) cfdata->use_mode = 1;
   else cfdata->use_mode = 0;

   e_config->scale.use_dpi = cfdata->use_dpi;
   e_config->scale.use_custom = cfdata->use_custom;
   e_config->scale.min = cfdata->min;
   e_config->scale.max = cfdata->max;
   e_config->scale.factor = cfdata->factor;
   e_config->scale.base_dpi = cfdata->base_dpi;

   e_scale_update();
   e_canvas_recache();
   e_config_save_queue();

   return 1;
}

static int
_basic_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return (cfdata->use_dpi != e_config->scale.use_dpi) ||
          (cfdata->base_dpi != e_config->scale.base_dpi);
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *otb, *ow;
   E_Radio_Group *rg;
   char buff[256];

   otb = e_widget_toolbook_add(evas, 24, 24);

   /* Policy */
   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->use_mode));
   ow = e_widget_radio_add(evas, _("Don't Scale"), 0, rg);
   e_widget_on_change_hook_set(ow, _adv_policy_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Scale relative to screen DPI"), 1, rg);
   e_widget_on_change_hook_set(ow, _adv_policy_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 1, 0.5);

   snprintf(buff, sizeof(buff),
            _("Base DPI (Currently %i DPI)"), ecore_x_dpi_get());
   ow = e_widget_label_add(evas, buff);
   cfdata->gui.adv.dpi_lbl = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f DPI"), 30, 600, 1, 0,
                            NULL, &(cfdata->base_dpi), 100);
   cfdata->gui.adv.dpi_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Custom scaling factor"), 2, rg);
   e_widget_on_change_hook_set(ow, _adv_policy_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.2f x"), 0.25, 8.0, 0.05,
                            0, &(cfdata->factor), NULL, 100);
   cfdata->gui.adv.custom_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Policy"), o,
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Constraints */
   o = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Minimum"));
   cfdata->gui.adv.min_lbl = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05,
                            0, &(cfdata->min), NULL, 150);
   cfdata->gui.adv.min_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_label_add(evas, _("Maximum"));
   cfdata->gui.adv.max_lbl = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05,
                            0, &(cfdata->max), NULL, 150);
   cfdata->gui.adv.max_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Constraints"), o,
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   _adv_policy_changed(cfdata, NULL);
   return otb;
}

static int
_adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   cfdata->use_custom = 0;
   cfdata->use_dpi = 0;
   if (cfdata->use_mode == 1)
     cfdata->use_dpi = 1;
   else if (cfdata->use_mode == 2)
     cfdata->use_custom = 1;

   e_config->scale.use_dpi = cfdata->use_dpi;
   e_config->scale.use_custom = cfdata->use_custom;
   e_config->scale.min = cfdata->min;
   e_config->scale.max = cfdata->max;
   e_config->scale.factor = cfdata->factor;
   e_config->scale.base_dpi = cfdata->base_dpi;

   e_scale_update();
   e_canvas_recache();
   e_config_save_queue();

   return 1;
}

static int
_adv_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   int use_dpi = 0, use_custom = 0;

   if (cfdata->use_mode == 1)
     use_dpi = 1;
   else if (cfdata->use_mode == 2)
     use_custom = 1;

   return (use_dpi != e_config->scale.use_dpi) ||
          (use_custom != e_config->scale.use_custom) ||
          (cfdata->min != e_config->scale.min) ||
          (cfdata->max != e_config->scale.max) ||
          (cfdata->factor != e_config->scale.factor) ||
          (cfdata->base_dpi != e_config->scale.base_dpi);
   return 1;
}

static void
_adv_policy_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.adv.dpi_lbl, (cfdata->use_mode != 1));
   e_widget_disabled_set(cfdata->gui.adv.dpi_slider, (cfdata->use_mode != 1));
   e_widget_disabled_set(cfdata->gui.adv.custom_slider, (cfdata->use_mode != 2));
}

