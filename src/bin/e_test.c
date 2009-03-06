/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_test_internal(E_Container *con);

EAPI void
e_test(void)
{
   Eina_List *managers, *l, *ll;
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     E_Container *con;
	     
	     con = ll->data;
	     _e_test_internal(con);
	  }
     }
}

#if 0
static int
_e_test_timer(void *data)
{
   E_Menu *m;
   Eina_List *managers, *l;
   
   m = data;
   if (m)
     {
	e_menu_deactivate(m);
	e_object_del(E_OBJECT(m));
	ecore_timer_add(0.05, _e_test_timer, NULL);
	return 0;
     }
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	m = e_int_menus_main_new();
	e_menu_activate_mouse(m,
			      e_container_zone_number_get(e_container_current_get(man), 0),
			      0, 0, 1, 1, E_MENU_POP_DIRECTION_DOWN, 0);
	ecore_timer_add(0.05, _e_test_timer, m);
	return 0;
     }
   return 0;
}

static void
_e_test_internal(E_Container *con)
{
   _e_test_timer(NULL);
}
#elif 0
static void
_e_test_resize(E_Win *win)
{
   Evas_Object *o;
   
   o = win->data;
   printf("RESIZE %i %i\n", win->w, win->h);
   evas_object_resize(o, win->w, win->h);
   evas_object_color_set(o, rand() & 0xff, rand() & 0xff, rand() & 0xff, 255);
}

static void
_e_test_delete(E_Win *win)
{
   printf("DEL!\n");
   e_object_del(E_OBJECT(win));
}

static void
_e_test_internal(E_Container *con)
{
   E_Win *win;
   Evas_Object *o;
   
   win = e_win_new(con);
   e_win_resize_callback_set(win, _e_test_resize);
   e_win_delete_callback_set(win, _e_test_delete);
   e_win_placed_set(win, 0);
   e_win_move_resize(win, 10, 80, 400, 200);
   e_win_name_class_set(win, "E", "_test_window");
   e_win_title_set(win, "A test window");
   e_win_raise(win);
   e_win_show(win);
   
   o = evas_object_rectangle_add(e_win_evas_get(win));
   evas_object_color_set(o, 255, 200, 100, 255);
   evas_object_resize(o, 400, 200);
   evas_object_show(o);
   
   win->data = o;
}
#elif 0
static int
_e_test_timer(void *data)
{
   E_Menu *m;
   static int y = 0;
   
   m = data;
   ecore_x_pointer_warp(m->evas_win, 20, y);
   y += 10;
   if (y > m->cur.h) y = 0;
   return 1;
}

static void
_e_test_internal(E_Container *con)
{
   E_Menu *m;
   Eina_List *managers, *l;
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	m = e_int_menus_main_new();
	e_menu_activate_mouse(m,
			      e_container_zone_number_get(e_container_current_get(man), 0),
			      0, 0, 1, 1, E_MENU_POP_DIRECTION_DOWN, 0);
	ecore_timer_add(0.02, _e_test_timer, m);
     }
}
#elif 0
static void
_e_test_dialog_del(void *obj)
{
   E_Dialog *dia;
   
   dia = obj;
   printf("dialog delete hook!\n");
}

static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   
   dia = e_dialog_new(con, "E", "_test");
   e_object_del_attach_func_set(E_OBJECT(dia), _e_test_dialog_del);
   e_dialog_title_set(dia, "A Test Dialog");
   e_dialog_text_set(dia, "A Test Dialog<br>And another line<br><hilight>Hilighted Text</hilight>");
   e_dialog_icon_set(dia, "preference-plugin", 64);
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_dialog_button_add(dia, "Apply", "system-restart", NULL, NULL);
   e_dialog_button_add(dia, "Cancel", "application-exit", NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}
#elif 0
static void
_e_test_click(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
#if 1
   double size;
   
   size = (double)(rand() % 1000) / 999;
   evas_object_resize(obj, size * 1024, size * 768);
#else
   e_scrollframe_child_region_show(data, 1024, 768, 100, 100);
#endif
}

static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Object *o, *o2, *o3;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");
   
   o = e_icon_add(dia->win->evas);
   e_icon_file_set(o, "/home/raster/scroll.png");
   evas_object_resize(o, 1024, 768);
   evas_object_focus_set(o, 1);
   evas_object_show(o);
      
   o2 = e_scrollframe_add(dia->win->evas);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_test_click, o2);
//   e_scrollframe_custom_theme_set(o2, "base/theme/widgets", "e/widgets/scrollframe");
   evas_object_show(o2);
#if 0   
   o3 = e_pan_add(dia->win->evas);
   e_pan_child_set(o3, o);
   e_scrollframe_extern_pan_set(o2, o3, e_pan_set, e_pan_get, e_pan_max_get, e_pan_child_size_get);
#else
   e_scrollframe_child_set(o2, o);
#endif
   
   e_dialog_content_set(dia, o2, 500, 300);
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   
   evas_object_focus_set(o, 1);
}
#elif 0
static E_Dialog *
_e_test_dia(E_Container *con)
{
   E_Config_Dialog *dia;
   
   dia = e_int_config_modules(con);
   return dia;
}

static E_Container *tcon = NULL;

static int
_e_test_timer(void *data)
{
   E_Config_Dialog *dia;
	
   if (data == NULL)
     {
	dia = _e_test_dia(tcon);
	ecore_timer_add(0.2, _e_test_timer, dia);
     }
   else
     {
	dia = data;
	e_object_del(E_OBJECT(dia));
	ecore_timer_add(0.2, _e_test_timer, NULL);
     }
   return 0;
}

static void
_e_test_internal(E_Container *con)
{
   tcon = con;
   _e_test_timer(NULL);
}
#elif 0

struct _tmp
{
   Evas_Object *ilist, *scrollframe;
};

static struct _tmp tmp = {NULL, NULL};

static void
_e_test_sel(void *data, void *data2)
{
   Evas_Coord x, y, w, h;
   
   e_ilist_selected_geometry_get(tmp.ilist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(tmp.scrollframe, x, y, w, h);
}

static void
_e_test_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord mw, mh, vw, vh, w, h;
   
   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_ilist_min_size_get(data, &mw, &mh);
   evas_object_geometry_get(data, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
	if (w != vw) evas_object_resize(data, vw, h);
     }
}

static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Coord mw, mh, vw, vh;
   Evas_Object *o, *o2, *o3, *o4;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");
   
   o = e_ilist_add(dia->win->evas);
   e_ilist_icon_size_set(o, 80, 48);
   
   o3 = e_livethumb_add(dia->win->evas);
   e_livethumb_vsize_set(o3, 160, 96);
   o4 = edje_object_add(e_livethumb_evas_get(o3));
   e_theme_edje_object_set(o4, "base/theme/borders",
			   "e/widgets/border/default/border");
   e_livethumb_thumb_set(o3, o4);
   e_ilist_append(o, o3, "Item 1", 0, _e_test_sel, NULL, NULL, NULL);
   
   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/palette.png");
   e_ilist_append(o, o3, "Item 2 (Some really long text goes here for testing)", 0, _e_test_sel, NULL, NULL, NULL);
   
   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/mozilla.png");
   e_ilist_append(o, o3, "Item 3 (Medium length)", 0, _e_test_sel, NULL, NULL, NULL);
   
   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/trash_open.png");
   e_ilist_append(o, o3, "Item POOP", 0, _e_test_sel, NULL, NULL, NULL);

   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/watch.png");
   e_ilist_append(o, o3, "Item BLING BLING", 0, _e_test_sel, NULL, NULL, NULL);

   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/quake3.png");
   e_ilist_append(o, o3, "Sukebelinth", 0, _e_test_sel, NULL, NULL, NULL);

   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/opera6.png");
   e_ilist_append(o, o3, "A header", 1, NULL, NULL, NULL, NULL);
   
   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/opera6.png");
   e_ilist_append(o, o3, "Panties", 0, _e_test_sel, NULL, NULL, NULL);

   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/drawer_open.png");
   e_ilist_append(o, o3, "Flimbert the cagey", 0, _e_test_sel, NULL, NULL, NULL);

   o3 = e_icon_add(dia->win->evas);
   e_icon_file_set(o3, "/home/raster/C/stuff/icons/cd.png");
   e_ilist_append(o, o3, "Norbert", 0, _e_test_sel, NULL, NULL, NULL);
   
   e_ilist_min_size_get(o, &mw, &mh);
   evas_object_resize(o, mw, mh);
   evas_object_focus_set(o, 1);
   evas_object_show(o);
      
   o2 = e_scrollframe_add(dia->win->evas);
   evas_object_event_callback_add(o2, EVAS_CALLBACK_RESIZE, _e_test_resize, o);
   evas_object_resize(o2, mw, 150);
   evas_object_show(o2);
   e_scrollframe_child_set(o2, o);

   e_scrollframe_child_viewport_size_get(o2, &vw, &vh);
   e_dialog_content_set(dia, o2, 200, 150);
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_resizable_set(dia, 1);
   e_dialog_show(dia);
   
   tmp.ilist = o;
   tmp.scrollframe = o2;
   
   evas_object_focus_set(o, 1);
}
#elif 0
static void
_e_test_cb_e_smart_pan_changed_hook(void *data, Evas_Object *obj, void *event_info)
{
   printf("VAL: %3.3f\n", e_slider_value_get(obj));
}

static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");
   
   o = e_slider_add(dia->win->evas);
   e_slider_orientation_set(o, 1);
   e_slider_value_set(o, 0.5);
   e_slider_value_step_count_set(o, 4);
   e_slider_value_format_display_set(o, "%1.2f V");
   e_slider_min_size_get(o, &mw, &mh);
   evas_object_smart_callback_add(o, "changed", _e_test_cb_e_smart_pan_changed_hook, NULL);
   evas_object_show(o);
   
   e_dialog_content_set(dia, o, 240 + mw, mh);
   
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   
   evas_object_focus_set(o, 1);
}
#elif 0
static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");
   
   o = e_widget_textblock_add(dia->win->evas);
#if 0
   e_widget_textblock_markup_set(o,
				 "<title>A title</title>"
				 "This is some text<br>"
				 "Blah blah<br>"
				 "<hilight>hilighted text</hilight><br>"
				 "<br>"
				 "More lines of text<br>"
				 "And yet more lines of text<br>"
				 "A very very long line of text that SHOULD be getting word wrapped because it is so long.<br>"
				 "And another line<br>"
				 "Some more<br>"
				 "Smelly fish on a stick<br>"
				 "Whatever."
				 );
#else
   e_widget_textblock_plain_set(o,
				"And here is some plaintext\n"
				"with some newlines & other special characters\n"
				"that should get escaped like < and >.\n"
				"\n"
				"\tTabs should become 8 spaces too.\n"
				);
#endif   
   evas_object_show(o);
   
   e_dialog_content_set(dia, o, 160, 160);
   
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   
   evas_object_focus_set(o, 1);
}
#elif 0
static void
_e_test_cb_button(void *data1, void *data2)
{
   e_fm2_parent_go(data1);
}

static void
_e_test_cb_changed(void *data, Evas_Object *obj, void *event_info)
{
   if (!e_fm2_has_parent_get(obj)) e_widget_disabled_set(data, 1);
   else e_widget_disabled_set(data, 0);
}
    
static void
_e_test_cb_favorites_selected(void *data, Evas_Object *obj, void *event_info)
{
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   
   printf("FAV SELECTED\n");
   selected = e_fm2_selected_list_get(obj);
   if (!selected) return;
   ici = selected->data;
   if ((ici->link) && (ici->mount))
     e_fm2_path_set(data, ici->link, "/");
   else if (ici->link)
     e_fm2_path_set(data, NULL, ici->link);
// FIXME: this should happen on the scrollframe not the fm obj   
//   e_widget_scrollframe_child_pos_set(data, 0, 0);
   eina_list_free(selected);
}
    
static void
_e_test_cb_favorites_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   Eina_List *icons, *l;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char *p1, *p2;
   
   printf("FAV LIST CHANGE!\n");
   icons = e_fm2_all_list_get(obj);
   if (!icons) return;
   realpath = e_fm2_real_path_get(data);
   p1 = ecore_file_realpath(realpath);
   if (!p1) goto done;
   for (l = icons; l; l = l->next)
     {
	ici = l->data;
	if (ici->link)
	  {
	     p2 = ecore_file_realpath(ici->link);
	     if (!strcmp(p1, p2))
	       {
		  e_fm2_select_set(obj, ici->file);
		  E_FREE(p2);
		  goto done;
	       }
	     E_FREE(p2);
	  }
     }
   done:
   E_FREE(p1);
   eina_list_free(icons);
}
    
static void
_e_test_cb_selected(void *data, Evas_Object *obj, void *event_info)
{
   printf("SELECTED!\n");
}
    
static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Object *ofm, *ofm2, *of, *ob, *ot;
   Evas_Coord mw, mh;
   E_Fm2_Config fmc;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");

   /* a table for layout */
   ot = e_widget_table_add(dia->win->evas, 0);

   /* actual files */
   ofm = e_fm2_add(dia->win->evas);
   
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(ofm, &fmc);
   
   e_fm2_path_set(ofm, "~/", "/");
   ob = e_widget_button_add(dia->win->evas, "Up a directory", NULL,
			   _e_test_cb_button, ofm, NULL);
   e_widget_table_object_append(ot, ob, 1, 0, 1, 1, 0, 0, 1, 0);
   evas_object_show(ob);
   evas_object_smart_callback_add(ofm, "changed", _e_test_cb_changed, ob);
   evas_object_smart_callback_add(ofm, "selected", _e_test_cb_selected, NULL);
   of = e_widget_scrollframe_pan_add(dia->win->evas, ofm,
				     e_fm2_pan_set, e_fm2_pan_get,
				     e_fm2_pan_max_get, e_fm2_pan_child_size_get);
   e_widget_min_size_set(of, 128, 128);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);
   evas_object_show(ofm);
   evas_object_show(of);
   
   ofm2 = ofm;

   /* shortcut list */
   ofm = e_fm2_add(dia->win->evas);

   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 1;
   fmc.view.no_subdir_jump = 1;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(ofm, &fmc);
   
   e_fm2_path_set(ofm, "favorites", "/");
   evas_object_smart_callback_add(ofm, "files_changed", _e_test_cb_favorites_files_changed, ofm2);
   evas_object_smart_callback_add(ofm, "selected", _e_test_cb_favorites_selected, ofm2);
   of = e_widget_scrollframe_pan_add(dia->win->evas, ofm,
				     e_fm2_pan_set, e_fm2_pan_get,
				     e_fm2_pan_max_get, e_fm2_pan_child_size_get);
   e_widget_min_size_set(of, 128, 128);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 0, 1, 0, 1);
   evas_object_show(ofm);
   evas_object_show(of);
   
   /* show and pack table */
   evas_object_show(ot);
   e_widget_min_size_get(ot, &mw, &mh);
   e_dialog_content_set(dia, ot, mw, mh);
   
   /* buttons at the bottom */
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_win_resize(dia->win, 400, 300); 
   
}
#elif 0
static void
_e_test_cb_changed(void *data, Evas_Object *obj)
{
//   printf("CHANGED \"%s\"\n", e_widget_fsel_selection_path_get(obj));
}
    
static void
_e_test_cb_selected(void *data, Evas_Object *obj)
{
   printf("SELECTED \"%s\"\n", e_widget_fsel_selection_path_get(obj));
   e_object_del(E_OBJECT(data));
}
    
static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");

   o = e_widget_fsel_add(dia->win->evas, "~/", "/tst", NULL, NULL, 
			 _e_test_cb_selected, dia,
			 _e_test_cb_changed, dia, 0);
   evas_object_show(o);
   e_widget_min_size_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);
   
   /* buttons at the bottom */
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_win_resize(dia->win, 400, 300); 
   
}
#elif 0

static void
_e_test_cb_ok(E_Color_Dialog *dia, E_Color *color, void *data)
{
   printf("Current color: %d, %d, %d\n", color->r, color->g, color->b);
}

static void
_e_test_internal(E_Container *con)
{
   E_Color_Dialog *d;

   d = e_color_dialog_new(con, NULL);
   e_color_dialog_show(d);
   e_color_dialog_select_callback_set(d, _e_test_cb_ok, NULL);
}

#elif 0
static void
_e_test_internal(E_Container *con)
{
   E_Dialog *dia;
   Evas_Object *o, *ob, *of;
   Evas_Coord mw, mh;
   int i;
   
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");

   of = e_scrollframe_add(dia->win->evas);
   
   ob = e_box_add(dia->win->evas);
   e_box_orientation_set(ob, 0);

   for (i = 0; i < 8; i++)
     {
   o = e_slidesel_add(dia->win->evas);
   e_slidesel_item_distance_set(o, 64);
   e_slidesel_item_add(o, "blah / item 1", 
		       "/home/raster/pix/OLD/Download/Crystalline____a.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / smelly fish", 
		       "/home/raster/pix/OLD/Download/Reluctant_Sunrise.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / pong", 
		       "/home/raster/pix/OLD/Download/Soft_Wings.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / on a stick", 
		       "/home/raster/pix/OLD/Download/Stock_rose_1.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / oath", 
		       "/home/raster/pix/OLD/Download/The_Eyes_Of_A_Killer.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / yiiihaaaaa", 
		       "/home/raster/pix/OLD/Download/lady_bug.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / blah blah blah",
		       "/home/raster/pix/OLD/Download/ocean_rocks_covered_by_ash.jpg",
		       NULL, NULL);
   e_slidesel_item_add(o, "blah / bing bing bing", 
		       "/home/raster/pix/OLD/Download/orange_chair_heaven_falling.jpg",
		       NULL, NULL);
   
   e_box_pack_end(ob, o);
   e_box_pack_options_set(o, 1, 1, 1, 0, 0.5, 0.5, 300, 100, 300, 100);
   evas_object_show(o);
     }
   
   /* fixme... more */
   e_box_min_size_get(ob, &mw, &mh);
   evas_object_resize(ob, mw, mh);

   e_scrollframe_child_set(of, ob);
   evas_object_show(ob);
//   e_widget_min_size_get(o, &mw, &mh);
   mw = 300; mh = 300;
   e_dialog_content_set(dia, of, mw, mh);
   evas_object_show(of);
   
   /* buttons at the bottom */
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_win_resize(dia->win, 400, 400);
   
}
#elif 0
static int
_e_test_timer(void *data)
{
   E_Container *con;
   E_Dialog *dia;
   Evas_Object *o, *ic;
   Evas_Coord mw, mh;
   
   con = data;
   dia = e_dialog_new(con, "E", "_test");
   e_dialog_title_set(dia, "A Test Dialog");

   o = e_widget_toolbar_add(dia->win->evas, 48, 48);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_efm_hdd.png");
   e_widget_toolbar_item_append(o, ic, "HDD", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_efm_cd.png");
   e_widget_toolbar_item_append(o, ic, "CD", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_efm_desktop.png");
   e_widget_toolbar_item_append(o, ic, "Desktop", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_efm_home.png");
   e_widget_toolbar_item_append(o, ic, "Home", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_efm_root.png");
   e_widget_toolbar_item_append(o, ic, "Root", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_efm_tmp.png");
   e_widget_toolbar_item_append(o, ic, "Temp", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_globe.png");
   e_widget_toolbar_item_append(o, ic, "World", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_mixer.png");
   e_widget_toolbar_item_append(o, ic, "Mixer", NULL, NULL, NULL);
   ic = e_icon_add(dia->win->evas);
   e_icon_file_set(ic, "/home/raster/C/e17/data/themes/images/icon_performance.png");
   e_widget_toolbar_item_append(o, ic, "Perform", NULL, NULL, NULL);

   e_widget_toolbar_scrollable_set(o, 1);
   e_widget_toolbar_item_select(o, 1);
   
   /* fixme... more */
   e_widget_min_size_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);
   evas_object_show(o);
   
   /* buttons at the bottom */
   e_dialog_button_add(dia, "OK", NULL, NULL, NULL);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_win_resize(dia->win, 400, 200);
   
   return 0;
}
static void
_e_test_internal(E_Container *con)
{
   ecore_timer_add(1.0, _e_test_timer, con);
}
#else
static void
_e_test_internal(E_Container *con)
{    
}
#endif
