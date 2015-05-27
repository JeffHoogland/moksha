#include "e.h"

typedef struct _CFColor_Class             CFColor_Class;
typedef struct _CFColor_Class_Description CFColor_Class_Description;

typedef enum {
   COLOR_CLASS_UNKNOWN = 0,
   COLOR_CLASS_SOLID = 1,
   COLOR_CLASS_TEXT = 2
} CFColor_Class_Type;

struct _CFColor_Class
{
   const char    *key;
   const char    *name;

   E_Color_Class *cc;

   struct
   {
      int       r[3], g[3], b[3], a[3];
      Eina_Bool changed;
      Eina_Bool enabled;
   } val;

   struct
   {
      Evas_Object       *icon;
      Evas_Object       *end;
      CFColor_Class_Type type;
   } gui;
};

struct _E_Config_Dialog_Data
{
   int        state;
   E_Color    color[3];

   Eina_List *classes;
   Eina_List *selected;
   Eina_List *changed;

   struct
   {
      Evas        *evas;
      Evas_Object *ilist;
      Evas_Object *frame;
      Evas_Object *state;
      Evas_Object *color[3];
      Evas_Object *reset;
      Evas_Object *text_preview;
      Eina_List   *disable_list;
   } gui;

   Ecore_Timer *delay_load_timer;
   Ecore_Timer *delay_color_timer;
   Ecore_Idler *selection_idler;

   Eina_Bool    populating;
};

struct _CFColor_Class_Description
{
   const char        *key;
   size_t             keylen;
   const char        *name;
   CFColor_Class_Type type;
};

#define CCDESC_S(k, n) {k, sizeof(k) - 1, n, COLOR_CLASS_SOLID}
#define CCDESC_T(k, n) {k, sizeof(k) - 1, n, COLOR_CLASS_TEXT}

/*
 * TODO PRE-RELEASE: check all color classes
 */
static const CFColor_Class_Description _color_classes_wm[] =
{
   CCDESC_T("border_title", N_("Border Title")),
   CCDESC_T("border_title_active", N_("Border Title Active")),
   CCDESC_S("border_frame", N_("Border Frame")),
   CCDESC_S("border_frame_active", N_("Border Frame Active")),
   CCDESC_T("error_text", N_("Error Text")),
   CCDESC_S("menu_base", N_("Menu Background Base")),
   CCDESC_T("menu_title", N_("Menu Title")),
   CCDESC_T("menu_title_active", N_("Menu Title Active")),
   CCDESC_T("menu_item", N_("Menu Item")),
   CCDESC_T("menu_item_active", N_("Menu Item Active")),
   CCDESC_T("menu_item_disabled", N_("Menu Item Disabled")),
   CCDESC_T("move_text", N_("Move Text")),
   CCDESC_T("resize_text", N_("Resize Text")),
   CCDESC_T("winlist_item", N_("Winlist Item")),
   CCDESC_T("winlist_item_active", N_("Winlist Item Active")),
   CCDESC_T("winlist_label", N_("Winlist Label")),
   CCDESC_T("winlist_title", N_("Winlist Title")),
   CCDESC_S("dialog_base", N_("Dialog Background Base")),
   CCDESC_S("shelf_base", N_("Shelf Background Base")),
   CCDESC_S("fileman_base", N_("File Manager Background Base")),
   {NULL, 0, NULL, COLOR_CLASS_UNKNOWN}
};
static const CFColor_Class_Description _color_classes_widgets[] =
{
   CCDESC_S("focus", N_("Focus")),
   CCDESC_T("button_text", N_("Button Text")),
   CCDESC_T("button_text_disabled", N_("Button Text Disabled")),
   CCDESC_T("check_text", N_("Check Text")),
   CCDESC_T("check_text_disabled", N_("Check Text Disabled")),
   CCDESC_T("entry_text", N_("Entry Text")),
   CCDESC_T("entry_text_disabled", N_("Entry Text Disabled")),
   CCDESC_T("label_text", N_("Label Text")),
   CCDESC_T("ilist_item_selected", N_("List Item Text Selected")),
   CCDESC_T("ilist_item", N_("List Item Text (Even)")),
   CCDESC_S("ilist_item_base", N_("List Item Background Base (Even)")),
   CCDESC_T("ilist_item_odd", N_("List Item Text (Odd)")),
   CCDESC_S("ilist_item_odd_base", N_("List Item Background Base (Odd)")),
   CCDESC_T("ilist_item_header", N_("List Header Text (Even)")),
   CCDESC_S("ilist_item_header_base", N_("List Header Background Base (Even)")),
   CCDESC_T("ilist_item_header_odd", N_("List Header Text (Odd)")),
   CCDESC_S("ilist_item_header_odd_base",
            N_("List Header Background Base (Odd)")),
   CCDESC_T("radio_text", N_("Radio Text")),
   CCDESC_T("radio_text_disabled", N_("Radio Text Disabled")),
   CCDESC_T("slider_text", N_("Slider Text")),
   CCDESC_T("slider_text_disabled", N_("Slider Text Disabled")),
   CCDESC_S("frame_base", N_("Frame Background Base")),
   CCDESC_S("scrollframe_base", N_("Scroller Frame Background Base")),
   {NULL, 0, NULL, COLOR_CLASS_UNKNOWN}
};
static const CFColor_Class_Description _color_classes_modules[] =
{
   CCDESC_T("module_label", N_("Module Label")),
   CCDESC_S("comp_focus-out_color", N_("Composite Focus-out Color")),
   {NULL, 0, NULL, COLOR_CLASS_UNKNOWN}
};
#undef CCDESC_S
#undef CCDESC_T

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void         _config_color_class_free(CFColor_Class *ccc);
static void         _config_color_class_icon_state_apply(CFColor_Class *ccc);
static void         _config_color_class_end_state_apply(CFColor_Class *ccc);
static Eina_Bool    _fill_data_delayed(void *data);
static Eina_Bool    _color_changed_delay(void *data);

E_Config_Dialog *
e_int_config_color_classes(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/colors")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   cfd = e_config_dialog_new(con, _("Colors"), "E", "appearance/colors",
                             "preferences-desktop-color", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   CFColor_Class *ccc;

   EINA_LIST_FREE(cfdata->classes, ccc)
     _config_color_class_free(ccc);

   eina_list_free(cfdata->selected);
   eina_list_free(cfdata->changed);

   if (cfdata->delay_load_timer)
     ecore_timer_del(cfdata->delay_load_timer);
   if (cfdata->delay_color_timer)
     ecore_timer_del(cfdata->delay_color_timer);
   if (cfdata->selection_idler)
     ecore_idler_del(cfdata->selection_idler);

   E_FREE(cfdata);
}

static Eina_Bool
_color_class_list_selection_idler(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   const E_Ilist_Item *it;
   CFColor_Class *ccc = NULL;
   const char *description;
   Evas_Object *o;
   char buf[256];
   unsigned int i, count;
   int r[3] = {-1, -1, -1};
   int g[3] = {-1, -1, -1};
   int b[3] = {-1, -1, -1};
   int a[3] = {-1, -1, -1};
   Eina_Bool mixed[3];
   Eina_Bool unset[3];
   Eina_Bool enabled = EINA_FALSE;

   if (cfdata->delay_color_timer)
     {
        ecore_timer_del(cfdata->delay_color_timer);
        cfdata->delay_color_timer = NULL;
        _color_changed_delay(cfdata);
     }

   eina_list_free(cfdata->selected);
   cfdata->selected = NULL;

   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->gui.ilist), l, it)
     {
        if ((!it->selected) || (it->header)) continue;
        ccc = e_widget_ilist_item_data_get(it);
        cfdata->selected = eina_list_append(cfdata->selected, ccc);

        if ((!enabled) && (ccc->val.enabled))
          enabled = EINA_TRUE;

#define _CX(_n)                                      \
  do                                                 \
    {                                                \
       for (i = 0; i < 3; i++)                       \
         {                                           \
            if (ccc->val._n[i] >= 0)                 \
              {                                      \
                 if (_n[i] == -1)                    \
                   _n[i] = ccc->val._n[i];           \
                 else if ((_n[i] >= 0) &&            \
                          (ccc->val._n[i] != _n[i])) \
                   _n[i] = -2;                       \
              }                                      \
         }                                           \
    }                                                \
  while (0)

        _CX(r);
        _CX(g);
        _CX(b);
        _CX(a);
#undef _CX
     }

   for (i = 0; i < 3; i++)
     {
        mixed[i] = ((r[i] == -2) || (g[i] == -2) ||
                    (b[i] == -2) || (a[i] == -2));

        unset[i] = ((r[i] == -1) && (g[i] == -1) &&
                    (b[i] == -1) && (a[i] == -1));
     }

   count = eina_list_count(cfdata->selected);
   if ((ccc) && (count == 1))
     {
        snprintf(buf, sizeof(buf), _("Color class: %s"), ccc->name);
        description = buf;
     }
   else if (count > 1)
     {
        if (mixed[0] || mixed[1] || mixed[2])
          snprintf(buf, sizeof(buf),
                   _("Selected %u mixed colors classes"), count);
        else if (unset[0] && unset[1] && unset[2])
          snprintf(buf, sizeof(buf),
                   _("Selected %u unset colors classes"), count);
        else
          snprintf(buf, sizeof(buf),
                   _("Selected %u uniform colors classes"), count);
        description = buf;
     }
   else
     description = _("No selected color class");

   cfdata->populating = EINA_TRUE;

   for (i = 0; i < 3; i++)
     {
        if (unset[i] || mixed[i])
          r[i] = g[i] = b[i] = a[i] = 0;

        cfdata->color[i].r = r[i];
        cfdata->color[i].g = g[i];
        cfdata->color[i].b = b[i];
        cfdata->color[i].a = a[i];
        e_color_update_rgb(cfdata->color + i);
        e_widget_color_well_update(cfdata->gui.color[i]);
     }

   edje_object_color_class_set
     (cfdata->gui.text_preview, "color_preview",
     r[0], g[0], b[0], a[0],
     r[1], g[1], b[1], a[1],
     r[2], g[2], b[2], a[2]);

   e_widget_frametable_label_set(cfdata->gui.frame, description);

   e_widget_disabled_set(cfdata->gui.state, count == 0);
   e_widget_check_checked_set(cfdata->gui.state, enabled);

   EINA_LIST_FOREACH(cfdata->gui.disable_list, l, o)
     e_widget_disabled_set(o, ((!enabled) || (count == 0)));

   cfdata->populating = EINA_FALSE;
   cfdata->selection_idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_color_class_list_selection_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->selection_idler)
     ecore_idler_del(cfdata->selection_idler);
   cfdata->selection_idler = ecore_idler_add
       (_color_class_list_selection_idler, cfdata);
}

static Eina_Bool
_config_color_class_color_reset(CFColor_Class *ccc)
{
   Eina_Bool ret = EINA_FALSE;
   int *r = ccc->val.r;
   int *g = ccc->val.g;
   int *b = ccc->val.b;
   int *a = ccc->val.a;

   /* trick: use object so it will look up all 3 levels:
    *   1. object (should not have)
    *   2. global set values (may have)
    *   3. theme file provided value (may have)
    */
   if (ccc->gui.icon)
     ret = edje_object_color_class_get
         (ccc->gui.icon, ccc->key,
         r + 0, g + 0, b + 0, a + 0,
         r + 1, g + 1, b + 1, a + 1,
         r + 2, g + 2, b + 2, a + 2);

   if (!ret)
     {
        unsigned int i;
        for (i = 0; i < 3; i++)
          {
             r[i] = -1;
             g[i] = -1;
             b[i] = -1;
             a[i] = -1;
          }
     }

   return ret;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   CFColor_Class *ccc;

   if (cfdata->delay_color_timer)
     {
        ecore_timer_del(cfdata->delay_color_timer);
        cfdata->delay_color_timer = NULL;
        _color_changed_delay(cfdata);
     }

   EINA_LIST_FREE(cfdata->changed, ccc)
     {
        ccc->val.changed = EINA_FALSE;

        if (ccc->val.enabled)
          {
             int *r = ccc->val.r;
             int *g = ccc->val.g;
             int *b = ccc->val.b;
             int *a = ccc->val.a;

             if (ccc->cc)
               {
                  e_color_class_instance_set
                    (ccc->cc,
                    r[0], g[0], b[0], a[0],
                    r[1], g[1], b[1], a[1],
                    r[2], g[2], b[2], a[2]);
               }
             else
               {
                  ccc->cc = e_color_class_set_stringshared
                      (ccc->key,
                      r[0], g[0], b[0], a[0],
                      r[1], g[1], b[1], a[1],
                      r[2], g[2], b[2], a[2]);
               }
          }
        else
          {
             if (ccc->cc)
               {
                  e_color_class_instance_del(ccc->cc);
                  ccc->cc = NULL;
               }
             _config_color_class_color_reset(ccc);
             _config_color_class_icon_state_apply(ccc);
          }
     }

   return 1;
}

static void
_custom_color_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   CFColor_Class *ccc;
   Eina_Bool enabled;
   Evas_Object *o;

   if (cfdata->populating) return;

   enabled = e_widget_check_checked_get(obj);
   EINA_LIST_FOREACH(cfdata->selected, l, ccc)
     {
        ccc->val.enabled = enabled;
        _config_color_class_end_state_apply(ccc);
        if (!enabled)
          {
             _config_color_class_color_reset(ccc);
             _config_color_class_icon_state_apply(ccc);
          }

        if (!ccc->val.changed)
          {
             ccc->val.changed = EINA_TRUE;
             cfdata->changed = eina_list_append(cfdata->changed, ccc);
          }
     }

   EINA_LIST_FOREACH(cfdata->gui.disable_list, l, o)
     e_widget_disabled_set(o, !enabled);

   _color_class_list_selection_changed(cfdata, NULL);
}

static Eina_Bool
_color_changed_delay(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   CFColor_Class *ccc;
   E_Color *col;

   EINA_LIST_FOREACH(cfdata->selected, l, ccc)
     _config_color_class_icon_state_apply(ccc);

   col = cfdata->color;
   edje_object_color_class_set
     (cfdata->gui.text_preview, "color_preview",
     col[0].r, col[0].g, col[0].b, col[0].a,
     col[1].r, col[1].g, col[1].b, col[1].a,
     col[2].r, col[2].g, col[2].b, col[2].a);

   cfdata->delay_color_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_color_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   CFColor_Class *ccc;
   E_Color *col;
   unsigned int i;

   if (cfdata->populating) return;

   for (i = 0; i < 3; i++)
     if (cfdata->gui.color[i] == obj)
       break;

   if (i == 3)
     {
        EINA_LOG_ERR("unknown widget changed color: %p\n", obj);
        return;
     }

   col = cfdata->color + i;
   EINA_LIST_FOREACH(cfdata->selected, l, ccc)
     {
        ccc->val.r[i] = col->r;
        ccc->val.g[i] = col->g;
        ccc->val.b[i] = col->b;
        ccc->val.a[i] = col->a;

        if (!ccc->val.changed)
          {
             ccc->val.changed = EINA_TRUE;
             cfdata->changed = eina_list_append(cfdata->changed, ccc);
          }
     }

   if (!cfdata->delay_color_timer)
     cfdata->delay_color_timer = ecore_timer_add
         (0.2, _color_changed_delay, cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ol, *ot, *o;
   Evas_Coord mw, mh;
   unsigned int i;
   const Eina_List *l;

   e_dialog_resizable_set(cfd->dia, 1);
   cfdata->gui.evas = evas;

   ol = e_widget_list_add(evas, 0, 0);

   cfdata->gui.ilist = e_widget_ilist_add(evas, 32, 24, NULL);
   e_widget_on_change_hook_set
     (cfdata->gui.ilist, _color_class_list_selection_changed, cfdata);
   e_widget_ilist_multi_select_set(cfdata->gui.ilist, 1);
   e_widget_size_min_get(cfdata->gui.ilist, &mw, &mh);
   if (mw < 200 * e_scale) mw = 200 * e_scale;
   if (mh < 150 * e_scale) mh = 150 * e_scale;
   e_widget_size_min_set(cfdata->gui.ilist, mw, mh);
   e_widget_list_object_append(ol, cfdata->gui.ilist, 1, 1, 0.0);

   cfdata->gui.frame = ot = e_widget_frametable_add
         (evas, _("No selected color class"), 0);

   cfdata->gui.state = o = e_widget_check_add
         (evas, _("Custom colors"), &(cfdata->state));
   e_widget_on_change_hook_set(o, _custom_color_cb_change, cfdata);
   e_widget_size_min_get(o, &mw, &mh);
   e_widget_frametable_object_append_full
     (ot, o, 0, 0, 3, 1, 1, 0, 1, 0, 0.0, 0.0, mw, mh, 9999, 9999);

   o = e_widget_label_add(evas, _("Object:"));
   cfdata->gui.disable_list = eina_list_append(cfdata->gui.disable_list, o);
   e_widget_size_min_get(o, &mw, &mh);
   e_widget_frametable_object_append_full
     (ot, o, 0, 1, 1, 1, 0, 0, 0, 0, 0.0, 0.5, mw, mh, 9999, 9999);

   o = e_widget_label_add(evas, _("Outline:"));
   cfdata->gui.disable_list = eina_list_append(cfdata->gui.disable_list, o);
   e_widget_size_min_get(o, &mw, &mh);
   e_widget_frametable_object_append_full
     (ot, o, 1, 1, 1, 1, 0, 0, 0, 0, 0.0, 0.5, mw, mh, 9999, 9999);

   o = e_widget_label_add(evas, _("Shadow:"));
   cfdata->gui.disable_list = eina_list_append(cfdata->gui.disable_list, o);
   e_widget_size_min_get(o, &mw, &mh);
   e_widget_frametable_object_append_full
     (ot, o, 2, 1, 1, 1, 0, 0, 0, 0, 0.0, 0.5, mw, mh, 9999, 9999);

   if (mh < 32) mh = 32;

   for (i = 0; i < 3; i++)
     {
        o = e_widget_color_well_add_full(evas, cfdata->color + i, 1, 1);
        cfdata->gui.color[i] = o;
        cfdata->gui.disable_list = eina_list_append
            (cfdata->gui.disable_list, o);
        e_widget_on_change_hook_set(o, _color_cb_change, cfdata);
        e_widget_size_min_get(o, &mw, NULL);
        mw = 32 * e_scale;
        e_widget_frametable_object_append_full
          (ot, o, i, 2, 1, 1, 1, 1, 1, 0, 0.0, 0.0, mw, mh, 9999, 9999);
     }

   o = edje_object_add(evas);
   if (!e_theme_edje_object_set(o, "base/theme/widgets",
                                "e/modules/conf_colors/preview/text"))
     {
        evas_object_del(o);
     }
   else
     {
        cfdata->gui.text_preview = o;
        edje_object_color_class_set
          (o, "color_preview", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        edje_object_part_text_set(o, "e.text", _("Text with applied colors."));
        edje_object_size_min_calc(o, &mw, &mh);
        e_widget_frametable_object_append_full
          (ot, o, 0, 3, 3, 1, 0, 0, 0, 0, 0.5, 0.5, mw, mh, 9999, 9999);
     }

   o = e_widget_label_add(evas, _("Colors depend on theme capabilities."));
   e_widget_size_min_get(o, &mw, &mh);
   e_widget_frametable_object_append_full
     (ot, o, 0, 4, 3, 1, 0, 0, 0, 0, 0.0, 0.5, mw, mh, 9999, 9999);

   e_widget_list_object_append(ol, cfdata->gui.frame, 1, 0, 0.0);

   e_util_win_auto_resize_fill(cfd->dia->win);
   e_win_centered_set(cfd->dia->win, 1);

   cfdata->delay_load_timer = ecore_timer_add(0.15, _fill_data_delayed, cfdata);

   EINA_LIST_FOREACH(cfdata->gui.disable_list, l, o)
     e_widget_disabled_set(o, 1);

   e_widget_disabled_set(cfdata->gui.state, 1);

   return ol;
}

static void
_config_color_class_free(CFColor_Class *ccc)
{
   eina_stringshare_del(ccc->key);
   eina_stringshare_del(ccc->name);
   E_FREE(ccc);
}

static CFColor_Class *
_config_color_class_new(const char *key_stringshared, const char *name, E_Color_Class *cc)
{
   CFColor_Class *ccc = E_NEW(CFColor_Class, 1);
   if (!ccc) return NULL;

   ccc->name = eina_stringshare_add(_(name));
   ccc->cc = cc;
   ccc->key = eina_stringshare_ref(key_stringshared);

   if (cc)
     {
        ccc->val.r[0] = cc->r;
        ccc->val.g[0] = cc->g;
        ccc->val.b[0] = cc->b;
        ccc->val.a[0] = cc->a;

        ccc->val.r[1] = cc->r2;
        ccc->val.g[1] = cc->g2;
        ccc->val.b[1] = cc->b2;
        ccc->val.a[1] = cc->a2;

        ccc->val.r[2] = cc->r3;
        ccc->val.g[2] = cc->g3;
        ccc->val.b[2] = cc->b3;
        ccc->val.a[2] = cc->a3;

        ccc->val.enabled = EINA_TRUE;
     }
   else
     {
        unsigned int i;
        for (i = 0; i < 3; i++)
          ccc->val.r[i] = ccc->val.g[i] = ccc->val.b[i] = ccc->val.a[i] = -1;

        ccc->val.enabled = EINA_FALSE;
     }

   return ccc;
}

static void
_config_color_class_icon_state_apply(CFColor_Class *ccc)
{
   if (!ccc->val.enabled)
     _config_color_class_color_reset(ccc);

   if ((ccc->gui.type == COLOR_CLASS_TEXT) ||
       (ccc->gui.type == COLOR_CLASS_SOLID))
     {
        edje_object_color_class_set
          (ccc->gui.icon, "color_preview",
          ccc->val.r[0], ccc->val.g[0], ccc->val.b[0], ccc->val.a[0],
          ccc->val.r[1], ccc->val.g[1], ccc->val.b[1], ccc->val.a[1],
          ccc->val.r[2], ccc->val.g[2], ccc->val.b[2], ccc->val.a[2]);
     }
   else
     {
        edje_object_color_class_set
          (ccc->gui.icon, "color_preview_c1",
          ccc->val.r[0], ccc->val.g[0], ccc->val.b[0], ccc->val.a[0],
          0, 0, 0, 0, 0, 0, 0, 0);

        edje_object_color_class_set
          (ccc->gui.icon, "color_preview_c2",
          ccc->val.r[1], ccc->val.g[1], ccc->val.b[1], ccc->val.a[1],
          0, 0, 0, 0, 0, 0, 0, 0);

        edje_object_color_class_set
          (ccc->gui.icon, "color_preview_c3",
          ccc->val.r[2], ccc->val.g[2], ccc->val.b[2], ccc->val.a[2],
          0, 0, 0, 0, 0, 0, 0, 0);
     }
}

static void
_config_color_class_end_state_apply(CFColor_Class *ccc)
{
   const char *sig;
   if (!ccc->gui.end) return;
   sig = ccc->val.enabled ? "e,state,checked" : "e,state,unchecked";
   edje_object_signal_emit(ccc->gui.end, sig, "e");
}

static int
_config_color_class_sort(const void *data1, const void *data2)
{
   const CFColor_Class *ccc1 = data1, *ccc2 = data2;
   return strcmp(ccc1->name, ccc2->name);
}

static int
_color_class_sort(const void *data1, const void *data2)
{
   const E_Color_Class *cc1 = data1, *cc2 = data2;
   return strcmp(cc1->name, cc2->name);
}

static void
_fill_data_add_header(E_Config_Dialog_Data *cfdata, const char *name, const char *icon)
{
   Evas_Object *ic;

   if (icon)
     {
        ic = e_icon_add(cfdata->gui.evas);
        e_util_icon_theme_set(ic, icon);
     }
   else
     ic = NULL;

   e_widget_ilist_header_append(cfdata->gui.ilist, ic, name);
}

static void
_fill_data_add_item(E_Config_Dialog_Data *cfdata, CFColor_Class *ccc)
{
   Evas_Object *icon, *end;

   icon = edje_object_add(cfdata->gui.evas);
   if (icon)
     {
        const char *group;

        switch (ccc->gui.type)
          {
           case COLOR_CLASS_TEXT:
             group = "e/modules/conf_colors/preview/text";
             break;

           case COLOR_CLASS_SOLID:
             group = "e/modules/conf_colors/preview/solid";
             break;

           default:
             group = "e/modules/conf_colors/preview/unknown";
          }

        if (e_theme_edje_object_set(icon, "base/theme/widgets", group))
          {
             ccc->gui.icon = icon;
             _config_color_class_icon_state_apply(ccc);
          }
        else
          {
             EINA_LOG_ERR("your theme misses '%s'!", group);
             evas_object_del(icon);
             icon = NULL;
          }
     }

   end = edje_object_add(cfdata->gui.evas);
   if (end)
     {
        if (e_theme_edje_object_set(end, "base/theme/widgets",
                                    "e/widgets/ilist/toggle_end"))
          {
             ccc->gui.end = end;
             _config_color_class_end_state_apply(ccc);
          }
        else
          {
             EINA_LOG_ERR("your theme misses 'e/widgets/ilist/toggle_end'!");
             evas_object_del(end);
             end = NULL;
          }
     }

   e_widget_ilist_append_full
     (cfdata->gui.ilist, icon, end, ccc->name, NULL, ccc, NULL);
}

static void
_fill_data_add_batch(E_Config_Dialog_Data *cfdata, Eina_List **p_todo, const CFColor_Class_Description *descs)
{
   CFColor_Class *ccc;
   Eina_List *batch = NULL;
   const CFColor_Class_Description *itr;

   for (itr = descs; itr->key; itr++)
     {
        E_Color_Class *cc;
        Eina_List *l, *found_node = NULL;
        const char *key_stringshared = eina_stringshare_add(itr->key);

        EINA_LIST_FOREACH(*p_todo, l, cc)
          {
             if (cc->name == key_stringshared)
               {
                  found_node = l;
                  break;
               }
          }

        if (!found_node)
          cc = NULL;
        else
          {
             cc = found_node->data;
             *p_todo = eina_list_remove_list(*p_todo, found_node);
          }

        ccc = _config_color_class_new(key_stringshared, itr->name, cc);
        eina_stringshare_del(key_stringshared);

        if (ccc)
          {
             batch = eina_list_append(batch, ccc);
             ccc->gui.type = itr->type;
          }
     }

   batch = eina_list_sort(batch, -1, _config_color_class_sort);
   EINA_LIST_FREE(batch, ccc)
     _fill_data_add_item(cfdata, ccc);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *todo = eina_list_clone(e_color_class_list());
   E_Color_Class *cc;

   _fill_data_add_header(cfdata, _("Window Manager"), "enlightenment");
   _fill_data_add_batch(cfdata, &todo, _color_classes_wm);
   _fill_data_add_header(cfdata, _("Widgets"), NULL);
   _fill_data_add_batch(cfdata, &todo, _color_classes_widgets);
   _fill_data_add_header(cfdata, _("Modules"), "preferences-plugin");
   _fill_data_add_batch(cfdata, &todo, _color_classes_modules);

   if (!todo) return;
   _fill_data_add_header(cfdata, _("Others"), NULL);
   todo = eina_list_sort(todo, -1, _color_class_sort);
   EINA_LIST_FREE(todo, cc)
     {
        CFColor_Class *ccc;
        char name[256], *d = name, *d_end = name + sizeof(name) - 1;
        const char *s = cc->name;
        Eina_Bool needs_upcase = EINA_TRUE;

        for (; *s && d < d_end; s++, d++)
          {
             if (!isalnum(*s))
               {
                  *d = ' ';
                  needs_upcase = EINA_TRUE;
               }
             else if (needs_upcase)
               {
                  *d = toupper(*s);
                  needs_upcase = EINA_FALSE;
               }
             else
               *d = *s;
          }
        *d = '\0';

        ccc = _config_color_class_new(cc->name, name, cc);
        if (!ccc) continue;
        _fill_data_add_item(cfdata, ccc);
     }
}

static Eina_Bool
_fill_data_delayed(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->delay_load_timer = NULL;
   _fill_data(cfdata);
   return ECORE_CALLBACK_CANCEL;
}

