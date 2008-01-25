/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_textblock;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_textblock_add(Evas *evas)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   o = e_scrollframe_add(evas);
   wd->o_scrollframe = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/textblock");
   wd->o_textblock = o;
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE, _e_wid_cb_scrollframe_resize, wd);
   e_scrollframe_child_set(wd->o_scrollframe, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
		       
   evas_object_resize(obj, 32, 32);
   e_widget_min_size_set(obj, 32, 32);
   return obj;
}

EAPI void
e_widget_textblock_markup_set(Evas_Object *obj, const char *text)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh;

   wd = e_widget_data_get(obj);

   evas_object_resize(wd->o_textblock, 0, 0);

   edje_object_part_text_set(wd->o_textblock, "e.textblock.text", text);
   edje_object_size_min_calc(wd->o_textblock, &mw, &mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   if (vw > mw) mw = vw;
   if (vh > mh) mh = vh;
   evas_object_resize(wd->o_textblock, mw, mh);
}

EAPI void
e_widget_textblock_plain_set(Evas_Object *obj, const char *text)
{
   char *markup;
   char *d;
   const char *p; 
   int mlen;

   if (!text)
     {
	e_widget_textblock_markup_set(obj, NULL);
	return;
     }
   mlen = strlen(text);
   /* need to look for these and replace with a new string (on the right)
    * '\n' -> "<br>"
    * '\t' -> "        "
    * '<'  -> "&lt;"
    * '>'  -> "&gt;"
    * '&'  -> "&amp;"
    */
   for (p = text; *p != 0; p++)
     {
	if (*p == '\n') mlen += 4 - 1;
	else if (*p == '\t') mlen += 8 - 1;
	else if (*p == '<') mlen += 4 - 1;
	else if (*p == '>') mlen += 4 - 1;
	else if (*p == '&') mlen += 5 - 1;
     }
   markup = alloca(mlen + 1);
   if (!markup) return;
   markup[0] = 0;
   for (d = markup, p = text; *p != 0; p++, d++)
     {
	if (*p == '\n')
	  {
	     strcpy(d, "<br>");
	     d += 4 - 1;
	  }
	else if (*p == '\t')
	  {
	     strcpy(d, "        ");
	     d += 8 - 1;
	  }
	else if (*p == '<')
	  {
	     strcpy(d, "&lt;");
	     d += 4 - 1;
	  }
	else if (*p == '>')
	  {
	     strcpy(d, "&gt;");
	     d += 4 - 1;
	  }
	else if (*p == '&')
	  {
	     strcpy(d, "&amp;");
	     d += 5 - 1;
	  }
	else *d = *p;
     }
   *d = 0;
   e_widget_textblock_markup_set(obj, markup);
}


static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "e,state,focused", "e");
	evas_object_focus_set(wd->o_scrollframe, 1);
     }
   else
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "e,state,unfocused", "e");
	evas_object_focus_set(wd->o_scrollframe, 0);
     }
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh;
   
   wd = data;
   edje_object_size_min_calc(wd->o_textblock, &mw, &mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   if (vw > mw) mw = vw;
   if (vh > mh) mh = vh;
   evas_object_resize(wd->o_textblock, mw, mh);
}
   
static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
