/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Eina_List *sliders;
   Eina_List *entries;
   Evas_Object *spectrum, *vert, *well;
   E_Color *cv;
   char **values;
   int mode;
   int changing;
};

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   int i;
   
   wd = e_widget_data_get(obj);
   if (!wd) return;

   for (i = 0; i < E_COLOR_COMPONENT_MAX; i++)
     {
	E_FREE(wd->values[i]);
     }
   E_FREE(wd->values);

   eina_list_free(wd->sliders);
   eina_list_free(wd->entries);

   E_FREE(wd);
}

static void
_e_wid_cb_radio_changed(void *data, Evas_Object *o)
{
   E_Widget_Data *wd = data;

   e_widget_spectrum_mode_set(wd->spectrum, wd->mode);
   e_widget_cslider_mode_set(wd->vert, wd->mode);
}

static void
_e_wid_cb_color_changed(void *data, Evas_Object *o)
{
   E_Widget_Data *wd = data;
   Eina_List *l;
   int i;
   int changed = -1;

   if (wd->changing) return;

   wd->changing = 1;

   /* entry changed */
   for (l = wd->entries, i = 0; l; l = l->next, i++)
     {
	if (o == l->data)
	  {
	     changed = i;
	     switch (i)
	       {
		case E_COLOR_COMPONENT_R:
		   wd->cv->r = atoi(wd->values[i]);
		   if (wd->cv->r > 255) wd->cv->r = 255;
		   if (wd->cv->r < 0) wd->cv->r = 0;
		   break;
		case E_COLOR_COMPONENT_G:
		   wd->cv->g = atoi(wd->values[i]);
		   if (wd->cv->g > 255) wd->cv->g = 255;
		   if (wd->cv->g < 0) wd->cv->g = 0;
		   break;
		case E_COLOR_COMPONENT_B:
		   wd->cv->b = atoi(wd->values[i]);
		   if (wd->cv->b > 255) wd->cv->b = 255;
		   if (wd->cv->b < 0) wd->cv->b = 0;
		   break;
		case E_COLOR_COMPONENT_H:
		   wd->cv->h = atof(wd->values[i]);
		   if (wd->cv->h > 360) wd->cv->h = 360;
		   if (wd->cv->h < 0) wd->cv->h = 0;
		   break;
		case E_COLOR_COMPONENT_S:
		   wd->cv->s = atof(wd->values[i]);
		   if (wd->cv->s > 1) wd->cv->s = 1;
		   if (wd->cv->s < 0) wd->cv->s = 0;
		   break;
		case E_COLOR_COMPONENT_V:
		   wd->cv->v = atof(wd->values[i]);
		   if (wd->cv->v > 1) wd->cv->v = 1;
		   if (wd->cv->v < 0) wd->cv->v = 0;
		   break;
	       }
	     break;
	  }
     }

   if (changed != -1)
     {
	if (changed>= E_COLOR_COMPONENT_H)
	  e_color_update_hsv(wd->cv);
	else if (changed >= E_COLOR_COMPONENT_R)
	  e_color_update_rgb(wd->cv);
     }

   if (o == wd->vert)
     changed = wd->mode;
   else
     e_widget_cslider_update(wd->vert);


   /* update the sliders */
   for (l = wd->sliders, i = 0; l; l = l->next, i++)
     {
	Evas_Object *so = l->data;
	if (o != so)
	  {
	     e_widget_cslider_update(so);
	  }
	else
	  {
	     changed = i;
	  }
     }

   /* update the spectrum */
   if (o != wd->spectrum && changed != -1)
     {
	if (wd->mode == changed ||
	      (wd->mode >= E_COLOR_COMPONENT_H && changed <= E_COLOR_COMPONENT_B) ||
	      (wd->mode <= E_COLOR_COMPONENT_B && changed >= E_COLOR_COMPONENT_H))
	  e_widget_spectrum_update(wd->spectrum, 1);
	else
	  e_widget_spectrum_update(wd->spectrum, 0);
     }

   e_widget_color_well_update(wd->well);

   /* now update the text fields to show current values */
   for (l = wd->entries, i = 0; l; l = l->next, i++)
     {
	char buf[10];
	if (o == l->data) continue;
	switch (i)
	  {
	   case E_COLOR_COMPONENT_R:
	      snprintf(buf, 10, "%i", wd->cv->r);
	      break;
	   case E_COLOR_COMPONENT_G:
	      snprintf(buf, 10, "%i", wd->cv->g);
	      break;
	   case E_COLOR_COMPONENT_B:
	      snprintf(buf, 10, "%i", wd->cv->b);
	      break;
	   case E_COLOR_COMPONENT_H:
	      snprintf(buf, 10, "%.0f", wd->cv->h);
	      break;
	   case E_COLOR_COMPONENT_S:
	      snprintf(buf, 10, "%.2f", wd->cv->s);
	      break;
	   case E_COLOR_COMPONENT_V:
	      snprintf(buf, 10, "%.2f", wd->cv->v);
	      break;
	  }
	e_widget_entry_text_set((Evas_Object *)(l->data), buf);
     }

   wd->changing = 0;

   e_widget_change(wd->obj);
}

Evas_Object *
e_widget_csel_add(Evas *evas, E_Color *color)
{
   Evas_Object *obj, *o;
   Evas_Object *frame, *table;
   int i;
   E_Radio_Group *grp = NULL;
   char *labels[6] = { "R", "G", "B", "H", "S", "V" };
   E_Widget_Data *wd;

   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->mode = 1;
   wd->cv = color;
   wd->obj = obj;
   e_widget_data_set(obj, wd);

   table = e_widget_table_add(evas, 0);
   e_widget_sub_object_add(obj, table);
   e_widget_resize_object_set(obj, table);

   frame = e_widget_table_add(evas, 0);
   e_widget_sub_object_add(obj, frame);
   grp = e_widget_radio_group_new(&wd->mode);

   wd->values = calloc(E_COLOR_COMPONENT_MAX, sizeof(char *));

   for (i = 0; i < E_COLOR_COMPONENT_MAX; i++)
     {
	wd->values[i] = calloc(10, sizeof(char));
	switch (i)
	  {
	   case E_COLOR_COMPONENT_R:
	      snprintf(wd->values[i], 10, "%i", wd->cv->r);
	      break;
	   case E_COLOR_COMPONENT_G:
	      snprintf(wd->values[i], 10, "%i", wd->cv->g);
	      break;
	   case E_COLOR_COMPONENT_B:
	      snprintf(wd->values[i], 10, "%i", wd->cv->b);
	      break;
	   case E_COLOR_COMPONENT_H:
	      snprintf(wd->values[i], 10, "%.0f", wd->cv->h);
	      break;
	   case E_COLOR_COMPONENT_S:
	      snprintf(wd->values[i], 10, "%.2f", wd->cv->s);
	      break;
	   case E_COLOR_COMPONENT_V:
	      snprintf(wd->values[i], 11, "%.2f", wd->cv->v);
	      break;
	  }

	o = e_widget_radio_add(evas, labels[i], i, grp);
	e_widget_sub_object_add(obj, o);
	e_widget_on_change_hook_set(o, _e_wid_cb_radio_changed, wd);
	e_widget_table_object_append(frame, o, 0, i, 1, 1, 1, 1, 0, 0);

	o = e_widget_cslider_add(evas, i, wd->cv, 0, 0);
	e_widget_sub_object_add(obj, o);
	evas_object_show(o);
	wd->sliders = eina_list_append(wd->sliders, o);
	e_widget_on_change_hook_set(o, _e_wid_cb_color_changed, wd);
	e_widget_table_object_append(frame, o, 1, i, 1, 1, 1, 1, 1, 0);

	o = e_widget_entry_add(evas, &(wd->values[i]), NULL, NULL, NULL);
	e_widget_sub_object_add(obj, o);
	evas_object_show(o);
	wd->entries = eina_list_append(wd->entries, o);
	e_widget_table_object_append(frame, o, 2, i, 1, 1, 1, 1, 1, 1);
	e_widget_on_change_hook_set(o, _e_wid_cb_color_changed, wd);

     }

   o = e_widget_spectrum_add(evas, wd->mode, wd->cv);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   e_widget_on_change_hook_set(o, _e_wid_cb_color_changed, wd);
   wd->spectrum = o;
   e_widget_table_object_append(table, o, 1, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_cslider_add(evas, wd->mode, wd->cv, 1, 1);
   e_widget_sub_object_add(obj, o);
   e_widget_on_change_hook_set(o, _e_wid_cb_color_changed, wd);
   e_widget_min_size_set(o, 30, 50);
   evas_object_show(o);
   wd->vert = o;
   e_widget_table_object_append(table, o, 2, 1, 1, 1, 0, 1, 0, 1);

   e_widget_table_object_append(table, frame, 3, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_color_well_add(evas, wd->cv, 0);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   evas_object_resize(o, 20, 20);
   wd->well = o;
   e_widget_table_object_append(table, o, 3, 2, 1, 1, 1, 1, 1, 1);


   return obj;
}



