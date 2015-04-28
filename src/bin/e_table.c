#include "e.h"

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Table_Item E_Table_Item;

struct _E_Smart_Data
{
   Evas_Coord    x, y, w, h;
   Evas_Object  *obj;
   Evas_Object  *clip;
   int           frozen;
   unsigned char changed : 1;
   unsigned char homogenous : 1;
   Eina_List    *items;
   struct
   {
      Evas_Coord w, h;
   } min, max;
   struct
   {
      double x, y;
   } align;
   struct
   {
      int cols, rows;
   } size;
};

struct _E_Table_Item
{
   E_Smart_Data *sd;
   int           col, row, colspan, rowspan;
   unsigned char fill_w : 1;
   unsigned char fill_h : 1;
   unsigned char expand_w : 1;
   unsigned char expand_h : 1;
   struct
   {
      Evas_Coord w, h;
   } min, max;
   struct
   {
      double x, y;
   } align;
   Evas_Object  *obj;
};

/* local subsystem functions */
static E_Table_Item *_e_table_smart_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void          _e_table_smart_disown(Evas_Object *obj);
static void          _e_table_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void          _e_table_smart_reconfigure(E_Smart_Data *sd);
static void          _e_table_smart_extents_calcuate(E_Smart_Data *sd);

static void          _e_table_smart_init(void);
static void          _e_table_smart_add(Evas_Object *obj);
static void          _e_table_smart_del(Evas_Object *obj);
static void          _e_table_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void          _e_table_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void          _e_table_smart_show(Evas_Object *obj);
static void          _e_table_smart_hide(Evas_Object *obj);
static void          _e_table_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void          _e_table_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void          _e_table_smart_clip_unset(Evas_Object *obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_table_add(Evas *evas)
{
   _e_table_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI int
e_table_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   sd = evas_object_smart_data_get(obj);
   sd->frozen++;
   return sd->frozen;
}

EAPI int
e_table_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERR(0);
   sd = evas_object_smart_data_get(obj);
   sd->frozen--;
   if (sd->frozen <= 0) _e_table_smart_reconfigure(sd);
   return sd->frozen;
}

EAPI void
e_table_homogenous_set(Evas_Object *obj, int homogenous)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   if (sd->homogenous == homogenous) return;
   sd->homogenous = homogenous;
   sd->changed = 1;
   if (sd->frozen <= 0) _e_table_smart_reconfigure(sd);
}

EAPI void
e_table_pack(Evas_Object *obj, Evas_Object *child, int col, int row, int colspan, int rowspan)
{
   E_Smart_Data *sd;
   E_Table_Item *ti;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   _e_table_smart_adopt(sd, child);
   sd->items = eina_list_append(sd->items, child);
   ti = evas_object_data_get(child, "e_table_data");
   if (ti)
     {
        ti->col = col;
        ti->row = row;
        ti->colspan = colspan;
        ti->rowspan = rowspan;
        if (sd->size.cols < (col + colspan)) sd->size.cols = col + colspan;
        if (sd->size.rows < (row + rowspan)) sd->size.rows = row + rowspan;
     }
   sd->changed = 1;
   if (sd->frozen <= 0) _e_table_smart_reconfigure(sd);
}

EAPI void
e_table_pack_options_set(Evas_Object *obj, int fill_w, int fill_h, int expand_w, int expand_h, double align_x, double align_y, Evas_Coord min_w, Evas_Coord min_h, Evas_Coord max_w, Evas_Coord max_h)
{
   E_Table_Item *ti;

   ti = evas_object_data_get(obj, "e_table_data");
   if (!ti) return;
   ti->fill_w = fill_w;
   ti->fill_h = fill_h;
   ti->expand_w = expand_w;
   ti->expand_h = expand_h;
   ti->align.x = align_x;
   ti->align.y = align_y;
   ti->min.w = min_w;
   ti->min.h = min_h;
   ti->max.w = max_w;
   ti->max.h = max_h;
   ti->sd->changed = 1;
   if (ti->sd->frozen <= 0) _e_table_smart_reconfigure(ti->sd);
}

EAPI void
e_table_unpack(Evas_Object *obj)
{
   E_Table_Item *ti;
   E_Smart_Data *sd;

   ti = evas_object_data_get(obj, "e_table_data");
   if (!ti) return;
   sd = ti->sd;
   sd->items = eina_list_remove(sd->items, obj);
   _e_table_smart_disown(obj);
   sd->changed = 1;
   if (sd->frozen <= 0) _e_table_smart_reconfigure(sd);
}

EAPI void
e_table_col_row_size_get(Evas_Object *obj, int *cols, int *rows)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   if (sd->changed) _e_table_smart_extents_calcuate(sd);
   if (cols) *cols = sd->size.cols;
   if (rows) *rows = sd->size.rows;
}

EAPI void
e_table_size_min_get(Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   if (sd->changed) _e_table_smart_extents_calcuate(sd);
   if (minw) *minw = sd->min.w;
   if (minh) *minh = sd->min.h;
}

EAPI void
e_table_size_max_get(Evas_Object *obj, Evas_Coord *maxw, Evas_Coord *maxh)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   if (sd->changed) _e_table_smart_extents_calcuate(sd);
   if (maxw) *maxw = sd->max.w;
   if (maxh) *maxh = sd->max.h;
}

EAPI void
e_table_align_get(Evas_Object *obj, double *ax, double *ay)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   if (ax) *ax = sd->align.x;
   if (ay) *ay = sd->align.y;
}

EAPI void
e_table_align_set(Evas_Object *obj, double ax, double ay)
{
   E_Smart_Data *sd;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   if ((sd->align.x == ax) && (sd->align.y == ay)) return;
   sd->align.x = ax;
   sd->align.y = ay;
   sd->changed = 1;
   if (sd->frozen <= 0) _e_table_smart_reconfigure(sd);
}

/* local subsystem functions */
static E_Table_Item *
_e_table_smart_adopt(E_Smart_Data *sd, Evas_Object *obj)
{
   E_Table_Item *ti;

   ti = calloc(1, sizeof(E_Table_Item));
   if (!ti) return NULL;
   ti->sd = sd;
   ti->obj = obj;
   /* defaults */
   ti->col = 0;
   ti->row = 0;
   ti->colspan = 1;
   ti->rowspan = 1;
   ti->fill_w = 0;
   ti->fill_h = 0;
   ti->expand_w = 0;
   ti->expand_h = 0;
   ti->align.x = 0.5;
   ti->align.y = 0.5;
   ti->min.w = 0;
   ti->min.h = 0;
   ti->max.w = 0;
   ti->max.h = 0;
   evas_object_clip_set(obj, sd->clip);
//   evas_object_stack_above(obj, sd->obj);
   evas_object_smart_member_add(obj, ti->sd->obj);
   evas_object_data_set(obj, "e_table_data", ti);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
                                  _e_table_smart_item_del_hook, NULL);
//   evas_object_stack_below(obj, sd->obj);
   if ((!evas_object_visible_get(sd->clip)) &&
       (evas_object_visible_get(sd->obj)))
     evas_object_show(sd->clip);
   return ti;
}

static void
_e_table_smart_disown(Evas_Object *obj)
{
   E_Table_Item *ti;

   ti = evas_object_data_get(obj, "e_table_data");
   if (!ti) return;
   if (!ti->sd->items)
     {
        if (evas_object_visible_get(ti->sd->clip))
          evas_object_hide(ti->sd->clip);
     }
   evas_object_event_callback_del(obj,
                                  EVAS_CALLBACK_FREE,
                                  _e_table_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_data_del(obj, "e_table_data");
   free(ti);
}

static void
_e_table_smart_item_del_hook(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   e_table_unpack(obj);
}

static void
_e_table_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord x, y, w, h, xx, yy;
   Eina_List *l;
   Evas_Object *obj;
   int minw, minh, expandw, expandh;

   if (!sd->changed) return;

   w = sd->w;
   h = sd->h;

   _e_table_smart_extents_calcuate(sd);

   minw = sd->min.w;
   minh = sd->min.h;
   expandw = 0;
   expandh = 0;
   if (w < minw) w = minw;
   if (h < minh) h = minh;
   EINA_LIST_FOREACH(sd->items, l, obj)
     {
        E_Table_Item *ti;

        ti = evas_object_data_get(obj, "e_table_data");
        if (ti->expand_w) expandw++;
        if (ti->expand_h) expandh++;
     }
   if (expandw == 0)
     {
        w = minw;
     }
   if (expandh == 0)
     {
        h = minh;
     }
   x = sd->x;
   y = sd->y;
   if (sd->homogenous)
     {
        EINA_LIST_FOREACH(sd->items, l, obj)
          {
             E_Table_Item *ti;
             Evas_Coord ww, hh, ow, oh;

             ti = evas_object_data_get(obj, "e_table_data");

             xx = x + ((ti->col) * (w / (Evas_Coord)sd->size.cols));
             yy = y + ((ti->row) * (h / (Evas_Coord)sd->size.rows));
             ww = ((w / (Evas_Coord)sd->size.cols) * (ti->colspan));
             hh = ((h / (Evas_Coord)sd->size.rows) * (ti->rowspan));
             ow = ti->min.w;
             if (ti->expand_w) ow = ww;
             if ((ti->max.w >= 0) && (ti->max.w < ow)) ow = ti->max.w;
             oh = ti->min.h;
             if (ti->expand_h) oh = hh;
             if ((ti->max.h >= 0) && (ti->max.h < oh)) oh = ti->max.h;
             evas_object_move(obj,
                              xx + (Evas_Coord)(((double)(ww - ow)) * ti->align.x),
                              yy + (Evas_Coord)(((double)(hh - oh)) * ti->align.y));
             evas_object_resize(obj, ow, oh);
          }
     }
   else
     {
        int i, ex, tot, need, num, dif, left, nx;
        EINA_LIST_FOREACH(sd->items, l, obj)
          {
             E_Table_Item *ti;

             ti = evas_object_data_get(obj, "e_table_data");
             if (sd->size.cols < (ti->col + ti->colspan))
               sd->size.cols = ti->col + ti->colspan;
             if (sd->size.rows < (ti->row + ti->rowspan))
               sd->size.rows = ti->row + ti->rowspan;
          }
        if ((sd->size.cols > 0) && (sd->size.rows > 0))
          {
             int *cols, *rows, *colsx, *rowsx;

             cols = calloc(sd->size.cols, sizeof(int));
             rows = calloc(sd->size.rows, sizeof(int));
             colsx = calloc(sd->size.cols, sizeof(int));
             rowsx = calloc(sd->size.rows, sizeof(int));

             EINA_LIST_FOREACH(sd->items, l, obj)
               {
                  E_Table_Item *ti;

                  ti = evas_object_data_get(obj, "e_table_data");
                  for (i = ti->col; i < (ti->col + ti->colspan); i++)
                    colsx[i] |= ti->expand_w;
                  for (i = ti->row; i < (ti->row + ti->rowspan); i++)
                    rowsx[i] |= ti->expand_h;
               }

             EINA_LIST_FOREACH(sd->items, l, obj)
               {
                  E_Table_Item *ti;

                  ti = evas_object_data_get(obj, "e_table_data");

                  /* handle horizontal */
                  ex = 0;
                  tot = 0;
                  num = ti->colspan;
                  for (i = ti->col; i < (ti->col + num); i++)
                    {
                       if (colsx[i]) ex++;
                       tot += cols[i];
                    }
                  need = ti->min.w;
                  if (tot < need)
                    {
                       dif = need - tot;
                       left = dif;
                       if (ex == 0)
                         {
                            nx = num;
                            for (i = ti->col; i < (ti->col + num); i++)
                              {
                                 if (nx > 1)
                                   {
                                      cols[i] += dif / num;
                                      left -= dif / num;
                                   }
                                 else
                                   {
                                      cols[i] += left;
                                      left = 0;
                                   }
                                 nx--;
                              }
                         }
                       else
                         {
                            nx = ex;
                            for (i = ti->col; i < (ti->col + num); i++)
                              {
                                 if (colsx[i])
                                   {
                                      if (nx > 1)
                                        {
                                           cols[i] += dif / ex;
                                           left -= dif / ex;
                                        }
                                      else
                                        {
                                           cols[i] += left;
                                           left = 0;
                                        }
                                      nx--;
                                   }
                              }
                         }
                    }

                  /* handle vertical */
                  ex = 0;
                  tot = 0;
                  num = ti->rowspan;
                  for (i = ti->row; i < (ti->row + num); i++)
                    {
                       if (rowsx[i]) ex++;
                       tot += rows[i];
                    }
                  need = ti->min.h;
                  if (tot < need)
                    {
                       dif = need - tot;
                       left = dif;
                       if (ex == 0)
                         {
                            nx = num;
                            for (i = ti->row; i < (ti->row + num); i++)
                              {
                                 if (nx > 1)
                                   {
                                      rows[i] += dif / num;
                                      left -= dif / num;
                                   }
                                 else
                                   {
                                      rows[i] += left;
                                      left = 0;
                                   }
                                 nx--;
                              }
                         }
                       else
                         {
                            nx = ex;
                            for (i = ti->row; i < (ti->row + num); i++)
                              {
                                 if (rowsx[i])
                                   {
                                      if (nx > 1)
                                        {
                                           rows[i] += dif / ex;
                                           left -= dif / ex;
                                        }
                                      else
                                        {
                                           rows[i] += left;
                                           left = 0;
                                        }
                                      nx--;
                                   }
                              }
                         }
                    }
               }

             ex = 0;
             for (i = 0; i < sd->size.cols; i++) {
                  if (colsx[i])
                    ex++;
               }
             tot = 0;
             for (i = 0; i < sd->size.cols; i++)
               tot += cols[i];
             dif = w - tot;
             if ((ex > 0) && (dif > 0))
               {
                  int exl;

                  left = dif;
                  exl = ex;
                  for (i = 0; i < sd->size.cols; i++)
                    {
                       if (colsx[i])
                         {
                            if (exl == 1)
                              {
                                 cols[i] += left;
                                 exl--;
                                 left = 0;
                              }
                            else
                              {
                                 cols[i] += dif / ex;
                                 exl--;
                                 left -= dif / ex;
                              }
                         }
                    }
               }

             ex = 0;
             for (i = 0; i < sd->size.rows; i++) {
                  if (rowsx[i])
                    ex++;
               }
             tot = 0;
             for (i = 0; i < sd->size.rows; i++)
               tot += rows[i];
             dif = h - tot;
             if ((ex > 0) && (dif > 0))
               {
                  int exl;

                  left = dif;
                  exl = ex;
                  for (i = 0; i < sd->size.rows; i++)
                    {
                       if (rowsx[i])
                         {
                            if (exl == 1)
                              {
                                 rows[i] += left;
                                 exl--;
                                 left = 0;
                              }
                            else
                              {
                                 rows[i] += dif / ex;
                                 exl--;
                                 left -= dif / ex;
                              }
                         }
                    }
               }

             EINA_LIST_FOREACH(sd->items, l, obj)
               {
                  E_Table_Item *ti;
                  Evas_Coord ww, hh, ow, oh, idx;

                  ti = evas_object_data_get(obj, "e_table_data");

                  xx = x;
                  for (idx = 0; idx < ti->col; idx++)
                    xx += cols[idx];
                  ww = 0;
                  for (idx = ti->col; idx < (ti->col + ti->colspan); idx++)
                    ww += cols[idx];
                  yy = y;
                  for (idx = 0; idx < ti->row; idx++)
                    yy += rows[idx];
                  hh = 0;
                  for (idx = ti->row; idx < (ti->row + ti->rowspan); idx++)
                    hh += rows[idx];

                  ow = ti->min.w;
                  if (ti->fill_w) ow = ww;
                  if ((ti->max.w >= 0) && (ti->max.w < ow)) ow = ti->max.w;
                  oh = ti->min.h;
                  if (ti->fill_h) oh = hh;
                  if ((ti->max.h >= 0) && (ti->max.h < oh)) oh = ti->max.h;
                  evas_object_move(obj,
                                   xx + (Evas_Coord)(((double)(ww - ow)) * ti->align.x),
                                   yy + (Evas_Coord)(((double)(hh - oh)) * ti->align.y));
                  evas_object_resize(obj, ow, oh);
               }
             free(rows);
             free(cols);
             free(rowsx);
             free(colsx);
          }
     }
   sd->changed = 0;
}

static void
_e_table_smart_extents_calcuate(E_Smart_Data *sd)
{
   Eina_List *l;
   Evas_Object *obj;
   int minw, minh;

   sd->max.w = -1; /* max < 0 == unlimited */
   sd->max.h = -1;
   sd->size.cols = 0;
   sd->size.rows = 0;

   minw = 0;
   minh = 0;
   if (sd->homogenous)
     {
        EINA_LIST_FOREACH(sd->items, l, obj)
          {
             E_Table_Item *ti;
             int mw, mh;

             ti = evas_object_data_get(obj, "e_table_data");
             if (sd->size.cols < (ti->col + ti->colspan))
               sd->size.cols = ti->col + ti->colspan;
             if (sd->size.rows < (ti->row + ti->rowspan))
               sd->size.rows = ti->row + ti->rowspan;
             mw = (ti->min.w + (ti->colspan - 1)) / ti->colspan;
             mh = (ti->min.h + (ti->rowspan - 1)) / ti->rowspan;
             if (minw < mw) minw = mw;
             if (minh < mh) minh = mh;
          }
        minw *= sd->size.cols;
        minh *= sd->size.rows;
     }
   else
     {
        int i, ex, tot, need, num, dif, left, nx;
        EINA_LIST_FOREACH(sd->items, l, obj)
          {
             E_Table_Item *ti;

             ti = evas_object_data_get(obj, "e_table_data");
             if (sd->size.cols < (ti->col + ti->colspan))
               sd->size.cols = ti->col + ti->colspan;
             if (sd->size.rows < (ti->row + ti->rowspan))
               sd->size.rows = ti->row + ti->rowspan;
          }
        if ((sd->size.cols > 0) && (sd->size.rows > 0))
          {
             int *cols, *rows, *colsx, *rowsx;

             cols = calloc(sd->size.cols, sizeof(int));
             rows = calloc(sd->size.rows, sizeof(int));
             colsx = calloc(sd->size.cols, sizeof(int));
             rowsx = calloc(sd->size.rows, sizeof(int));

             EINA_LIST_FOREACH(sd->items, l, obj)
               {
                  E_Table_Item *ti;

                  ti = evas_object_data_get(obj, "e_table_data");
                  for (i = ti->col; i < (ti->col + ti->colspan); i++)
                    colsx[i] |= ti->expand_w;
                  for (i = ti->row; i < (ti->row + ti->rowspan); i++)
                    rowsx[i] |= ti->expand_h;
               }

             EINA_LIST_FOREACH(sd->items, l, obj)
               {
                  E_Table_Item *ti;

                  ti = evas_object_data_get(obj, "e_table_data");

                  /* handle horizontal */
                  ex = 0;
                  tot = 0;
                  num = ti->colspan;
                  for (i = ti->col; i < (ti->col + num); i++)
                    {
                       if (colsx[i]) ex++;
                       tot += cols[i];
                    }
                  need = ti->min.w;
                  if (tot < need)
                    {
                       dif = need - tot;
                       left = dif;
                       if (ex == 0)
                         {
                            nx = num;
                            for (i = ti->col; i < (ti->col + num); i++)
                              {
                                 if (nx > 1)
                                   {
                                      cols[i] += dif / num;
                                      left -= dif / num;
                                   }
                                 else
                                   {
                                      cols[i] += left;
                                      left = 0;
                                   }
                                 nx--;
                              }
                         }
                       else
                         {
                            nx = ex;
                            for (i = ti->col; i < (ti->col + num); i++)
                              {
                                 if (colsx[i])
                                   {
                                      if (nx > 1)
                                        {
                                           cols[i] += dif / ex;
                                           left -= dif / ex;
                                        }
                                      else
                                        {
                                           cols[i] += left;
                                           left = 0;
                                        }
                                      nx--;
                                   }
                              }
                         }
                    }

                  /* handle vertical */
                  ex = 0;
                  tot = 0;
                  num = ti->rowspan;
                  for (i = ti->row; i < (ti->row + num); i++)
                    {
                       if (rowsx[i]) ex++;
                       tot += rows[i];
                    }
                  need = ti->min.h;
                  if (tot < need)
                    {
                       dif = need - tot;
                       left = dif;
                       if (ex == 0)
                         {
                            nx = num;
                            for (i = ti->row; i < (ti->row + num); i++)
                              {
                                 if (nx > 1)
                                   {
                                      rows[i] += dif / num;
                                      left -= dif / num;
                                   }
                                 else
                                   {
                                      rows[i] += left;
                                      left = 0;
                                   }
                                 nx--;
                              }
                         }
                       else
                         {
                            nx = ex;
                            for (i = ti->row; i < (ti->row + num); i++)
                              {
                                 if (rowsx[i])
                                   {
                                      if (nx > 1)
                                        {
                                           rows[i] += dif / ex;
                                           left -= dif / ex;
                                        }
                                      else
                                        {
                                           rows[i] += left;
                                           left = 0;
                                        }
                                      nx--;
                                   }
                              }
                         }
                    }
               }
             for (i = 0; i < sd->size.cols; i++)
               minw += cols[i];
             for (i = 0; i < sd->size.rows; i++)
               minh += rows[i];
             free(rows);
             free(cols);
             free(rowsx);
             free(colsx);
          }
     }
   sd->min.w = minw;
   sd->min.h = minh;
}

static void
_e_table_smart_init(void)
{
   if (_e_smart) return;
   {
      static const Evas_Smart_Class sc =
      {
         "e_table",
         EVAS_SMART_CLASS_VERSION,
         _e_table_smart_add,
         _e_table_smart_del,
         _e_table_smart_move,
         _e_table_smart_resize,
         _e_table_smart_show,
         _e_table_smart_hide,
         _e_table_smart_color_set,
         _e_table_smart_clip_set,
         _e_table_smart_clip_unset,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_table_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, -100002, -100002);
   evas_object_resize(sd->clip, 200004, 200004);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
}

static void
_e_table_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   e_table_freeze(obj);
   while (sd->items)
     {
        Evas_Object *child;

        child = eina_list_data_get(sd->items);
        e_table_unpack(child);
     }
   e_table_thaw(obj);
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_table_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((x == sd->x) && (y == sd->y)) return;
   if ((x == sd->x) && (y == sd->y)) return;
   {
      Eina_List *l;
      Evas_Object *item;
      Evas_Coord dx, dy;

      dx = x - sd->x;
      dy = y - sd->y;
      EINA_LIST_FOREACH(sd->items, l, item)
        {
           Evas_Coord ox, oy;

           evas_object_geometry_get(item, &ox, &oy, NULL, NULL);
           evas_object_move(item, ox + dx, oy + dy);
        }
   }
   sd->x = x;
   sd->y = y;
}

static void
_e_table_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;
   sd->w = w;
   sd->h = h;
   sd->changed = 1;
   _e_table_smart_reconfigure(sd);
}

static void
_e_table_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->items) evas_object_show(sd->clip);
}

static void
_e_table_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_table_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_table_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_table_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

