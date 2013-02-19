#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

/* local structure */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* canvas variable */
   Evas *evas;

   /* geometry */
   int x, y, w, h;

   struct 
     {
        Evas_Coord mode_width, mode_height;
     } min, max;

   /* reference to the grid we are packed into */
   Evas_Object *grid;

   /* test object */
   /* Evas_Object *o_bg; */

   /* base object */
   Evas_Object *o_base;

   /* frame object */
   Evas_Object *o_frame;

   /* stand object */
   Evas_Object *o_stand;

   /* crtc config */
   Ecore_X_Randr_Crtc crtc;

   /* output config */
   Ecore_X_Randr_Output output;

   /* list of modes */
   Eina_List *modes;

   /* visibility flag */
   Eina_Bool visible : 1;
};

/* smart function prototypes */
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_smart_clip_unset(Evas_Object *obj);

/* local function prototypes */
static void _e_smart_monitor_modes_fill(E_Smart_Data *sd);
static int _e_smart_monitor_modes_sort(const void *data1, const void *data2);

/* external functions exposed by this widget */
Evas_Object *
e_smart_monitor_add(Evas *evas)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   static Evas_Smart *smart = NULL;
   static const Evas_Smart_Class sc = 
     {
        "smart_monitor", EVAS_SMART_CLASS_VERSION, 
        _e_smart_add, _e_smart_del, _e_smart_move, _e_smart_resize, 
        _e_smart_show, _e_smart_hide, NULL, 
        _e_smart_clip_set, _e_smart_clip_unset, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
     };

   /* if we have never created the smart class, do it now */
   if (!smart)
     if (!(smart = evas_smart_class_new(&sc)))
       return NULL;

   /* return a newly created smart randr widget */
   return evas_object_smart_add(evas, smart);
}

void 
e_smart_monitor_crtc_set(Evas_Object *obj, Ecore_X_Randr_Crtc crtc)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the crtc config */
   sd->crtc = crtc;
}

void 
e_smart_monitor_output_set(Evas_Object *obj, Ecore_X_Randr_Output output)
{
   E_Smart_Data *sd;
   Ecore_X_Randr_Mode_Info *mode;
   Evas_Coord mw = 0, mh = 0;
   Evas_Coord aw = 1, ah = 1;
   unsigned char *edid = NULL;
   unsigned long edid_length = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the output config */
   sd->output = output;

   /* since we now have the output, let's be preemptive and fill in modes */
   _e_smart_monitor_modes_fill(sd);
   if (!sd->modes) return;

   /* get the largest mode */
   mode = eina_list_last_data_get(sd->modes);
   mw = mode->width;
   mh = mode->height;

   sd->max.mode_width = mw;
   sd->max.mode_height = mh;

   /* FIXME: ideally this should probably be based on the current mode */

   /* get the edid for this output */
   if ((edid = ecore_x_randr_output_edid_get(0, sd->output, &edid_length)))
     {
        Ecore_X_Randr_Edid_Aspect_Ratio aspect = 0;

        /* get the aspect */
        aspect = 
          ecore_x_randr_edid_display_aspect_ratio_preferred_get(edid, 
                                                                edid_length);

        /* calculate aspect size */
        switch (aspect)
          {
           case ECORE_X_RANDR_EDID_ASPECT_RATIO_4_3:
             aw = 4;
             ah = (3 * mh) / mw;
             break;
           case ECORE_X_RANDR_EDID_ASPECT_RATIO_16_9:
             aw = 16;
             ah = (9 * mh) / mw;
             break;
           case ECORE_X_RANDR_EDID_ASPECT_RATIO_16_10:
             aw = 16;
             ah = (10 * mh) / mw;
             break;
           case ECORE_X_RANDR_EDID_ASPECT_RATIO_5_4:
             aw = 5;
             ah = (4 * mh) / mw;
             break;
           case ECORE_X_RANDR_EDID_ASPECT_RATIO_15_9:
             aw = 15;
             ah = (9 * mh) / mw;
             break;
           default:
             aw = mw;
             ah = mh;
             break;
          }

        /* set the aspect hints */
        evas_object_size_hint_aspect_set(sd->o_frame, 
                                         EVAS_ASPECT_CONTROL_BOTH, aw, ah);
     }

   /* set the align hints */
   /* evas_object_size_hint_align_set(sd->o_base, 0.0, 0.0); */
   evas_object_size_hint_align_set(sd->o_frame, 0.0, 0.0);

   /* get the smallest mode */
   mode = eina_list_nth(sd->modes, 0);
   sd->min.mode_width = mode->width;
   sd->min.mode_height = mode->height;
}

void 
e_smart_monitor_grid_set(Evas_Object *obj, Evas_Object *grid)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   sd->grid = grid;
}

/* smart functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to allocate the smart data structure */
   if (!(sd = E_NEW(E_Smart_Data, 1))) return;

   /* grab the canvas */
   sd->evas = evas_object_evas_get(obj);

   /* create the bg test object */
   /* sd->o_bg = evas_object_rectangle_add(sd->evas); */
   /* evas_object_color_set(sd->o_bg, 255, 0, 0, 255); */
   /* evas_object_smart_member_add(sd->o_bg, obj); */

   /* create the base object */
   sd->o_base = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_base, "base/theme/widgets", 
                           "e/conf/randr/main/monitor");
   evas_object_smart_member_add(sd->o_base, obj);

   /* create the frame object */
   sd->o_frame = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_frame, "base/theme/widgets", 
                           "e/conf/randr/main/frame");
   edje_object_part_swallow(sd->o_base, "e.swallow.frame", sd->o_frame);

   /* create the stand */
   sd->o_stand = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_stand, "base/theme/widgets", 
                           "e/conf/randr/main/stand");
   edje_object_part_swallow(sd->o_base, "e.swallow.stand", sd->o_stand);

   /* set the objects smart data */
   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Ecore_X_Randr_Mode_Info *mode;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas_object_del(sd->o_stand);
   evas_object_del(sd->o_frame);
   evas_object_del(sd->o_base);
   /* evas_object_del(sd->o_bg); */

   /* free the list of modes */
   EINA_LIST_FREE(sd->modes, mode)
     ecore_x_randr_mode_info_free(mode);

   /* try to free the allocated structure */
   E_FREE(sd);

   /* set the objects smart data to null */
   evas_object_smart_data_set(obj, NULL);
}

static void 
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if there is no position change, then get out */
   if ((sd->x == x) && (sd->y == y)) return;

   sd->x = x;
   sd->y = y;

   /* evas_object_move(sd->o_bg, x, y); */
   evas_object_move(sd->o_base, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   /* Evas_Coord gx, gy, gw, gh; */

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if there is no size change, then get out */
   if ((sd->w == w) && (sd->h == h)) return;

   sd->w = w;
   sd->h = h;

   evas_object_resize(sd->o_base, w, h);
   /* evas_object_resize(sd->o_bg, w, h + 30); */
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already visible, then nothing to do */
   if (sd->visible) return;

   evas_object_show(sd->o_frame);
   evas_object_show(sd->o_stand);
   evas_object_show(sd->o_base);
   /* evas_object_show(sd->o_bg); */

   /* set visibility flag */
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already hidden, then nothing to do */
   if (!sd->visible) return;

   evas_object_hide(sd->o_frame);
   evas_object_hide(sd->o_stand);
   evas_object_hide(sd->o_base);
   /* evas_object_hide(sd->o_bg); */

   /* set visibility flag */
   sd->visible = EINA_FALSE;
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas_object_clip_set(sd->o_frame, clip);
   evas_object_clip_set(sd->o_base, clip);
   /* evas_object_clip_set(sd->o_bg, clip); */
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas_object_clip_unset(sd->o_frame);
   evas_object_clip_unset(sd->o_base);
   /* evas_object_clip_unset(sd->o_bg); */
}

/* local functions */
static void 
_e_smart_monitor_modes_fill(E_Smart_Data *sd)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Mode *modes;
   int num = 0, i = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* safety check */
   if (!sd) return;

   /* try to get the root window */
   root = ecore_x_window_root_first_get();

   /* try to get the modes for this output from ecore_x_randr */
   modes = ecore_x_randr_output_modes_get(root, sd->output, &num, NULL);
   if (!modes) return;

   /* loop the returned modes */
   for (i = 0; i < num; i++)
     {
        Ecore_X_Randr_Mode_Info *mode;

        /* try to get the mode info */
        if (!(mode = ecore_x_randr_mode_info_get(root, modes[i])))
          continue;

        /* append the mode info to our list of modes */
        sd->modes = eina_list_append(sd->modes, mode);
     }

   /* free any memory allocated from ecore_x_randr */
   free(modes);

   /* sort the list of modes (smallest to largest) */
   if (sd->modes)
     sd->modes = eina_list_sort(sd->modes, 0, _e_smart_monitor_modes_sort);
}

static int 
_e_smart_monitor_modes_sort(const void *data1, const void *data2)
{
   const Ecore_X_Randr_Mode_Info *m1, *m2 = NULL;

   if (!(m1 = data1)) return 1;
   if (!(m2 = data2)) return -1;

   /* second one compares to previous to determine position */
   if (m2->width < m1->width) return 1;
   if (m2->width > m1->width) return -1;

   /* width are same, compare heights */
   if ((m2->width == m1->width))
     {
        if (m2->height < m1->height) return 1;
        if (m2->height > m1->height) return -1;
     }

   return 1;
}
