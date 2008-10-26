/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static char *xdg_sel = NULL;
static Eina_List *menus = NULL;

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   const char *dirs[] = 
     {
	"/etc/xdg",
	  "/usr/etc/xdg",
	  "/usr/local/etc/xdg",
	  "/usr/opt/etc/xdg",
	  "/usr/opt/xdg",
	  // FIXME: add more "known locations"
	  NULL
     };
   int i;

   for (i = 0; dirs[i]; i++)
     {
	Ecore_List *files;
	char buf[PATH_MAX], *file;
	
	snprintf(buf, sizeof(buf), "%s/menus", dirs[i]);
	files = ecore_file_ls(buf);
	if (files)
	  {
	     ecore_list_first_goto(files);
	     while ((file = ecore_list_current(files)))
	       {
		  if (e_util_glob_match(file, "*.menu"))
		    {
		       snprintf(buf, sizeof(buf), "%s/menus/%s", dirs[i], file);
		       menus = eina_list_append(menus, strdup(buf));
		    }
		  ecore_list_next(files);
	       }
	     ecore_list_destroy(files);
	  }
     }
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   // FIXME: free menus
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   Eina_List *l;
   int i, sel = -1;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Menus"));
   
   if (!menus)
     {
	of = e_widget_framelist_add(pg->evas, _("Error"), 0);
	
	ob = e_widget_textblock_add(pg->evas);
        e_widget_min_size_set(ob, 140 * e_scale, 140 * e_scale);
        e_widget_textblock_markup_set
	  (ob, 
	   _("No menu files were<br>"
	     "found on your system.<br>"
	     "Please see the<br>"
	     "documentation on<br>"
	     "www.enlightenment.org<br>"
	     "for more details on<br>"
	     "how to get your<br>"
	     "application menus<br>"
	     "working."
	     )
	   );
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
	evas_object_show(ob);
	evas_object_show(of);
     }
   else
     {
	of = e_widget_framelist_add(pg->evas, _("Select application menu"), 0);
	
	ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &xdg_sel);
	e_widget_min_size_set(ob, 140 * e_scale, 140 * e_scale);
	
	e_widget_ilist_freeze(ob);
	for (i = 0, l = menus; l; l = l->next, i++)
	  {
	     char buf[PATH_MAX], *file, *p, *p2, *tlabel, *tdesc;
	     const char *label;
	     
	     file = l->data;
	     label = file;
	     tlabel = NULL;
	     tdesc = NULL;
	     if (!strcmp("/etc/xdg/menus/applications.menu", file))
	       {
		  label = _("System Default");
		  sel = i;
	       }
	     else
	       {
		  p = strrchr(file, '/');
		  if (p)
		    {
		       p++;
		       p2 = strchr(p, '-');
		       if (!p2) p2 = strrchr(p, '.');
		       if (p2)
			 {
			    tlabel = malloc(p2 - p + 1);
			    if (tlabel)
			      {
				 strncpy(tlabel, p, p2 - p);
				 tlabel[p2 - p] = 0;
				 tlabel[0] = toupper(tlabel[0]);
				 if (*p2 == '-')
				   {
				      p2++;
				      p = strrchr(p2, '.');
				      if (p)
					{
					   tdesc = malloc(p - p2 + 1);
					   if (tdesc)
					     {
						strncpy(tdesc, p2, p - p2);
						tdesc[p - p2] = 0;
						tdesc[0] = toupper(tdesc[0]);
						snprintf(buf, sizeof(buf), "%s (%s)", tlabel, tdesc);
					     }
					   else
					     snprintf(buf, sizeof(buf), "%s", tlabel);
					}
				      else
					snprintf(buf, sizeof(buf), "%s", tlabel);
				   }
				 else
				   snprintf(buf, sizeof(buf), "%s", tlabel);
				 label = buf;
			      }
			 }
		       else
			 label = p;
		    }
	       }
	     e_widget_ilist_append(ob, NULL, label, NULL, NULL, file);
	     if (tlabel) free(tlabel);
	     if (tdesc) free(tdesc);
	     free(file);
	  }
	evas_list_free(menus);
	menus = NULL;
	e_widget_ilist_go(ob);
	e_widget_ilist_thaw(ob);
   
	if (sel >= 0) e_widget_ilist_selected_set(ob, sel);
   
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
	evas_object_show(ob);
	evas_object_show(of);
     }

   e_wizard_page_show(o);
   pg->data = of;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   evas_object_del(pg->data);
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   if ((xdg_sel) && (!strcmp("/etc/xdg/menus/applications.menu", xdg_sel)))
     xdg_sel = NULL;
   if (xdg_sel)
     e_config->default_system_menu = eina_stringshare_add(xdg_sel);
   else
     e_config->default_system_menu = NULL;
   efreet_menu_file_set(e_config->default_system_menu);
   // FIXME: no normal config dialog to change this!
   return 1;
}
