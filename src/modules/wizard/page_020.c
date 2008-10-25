/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static char *profile = NULL;
static Evas_Object *textblock = NULL;

static void
_profile_change(void *data, Evas_Object *obj)
{
   char buf[PATH_MAX], *dir;
   Efreet_Desktop *desk = NULL;
   
   dir = e_prefix_data_get();
   snprintf(buf, sizeof(buf), "%s/data/config/%s", dir, profile);
   dir = strdup(buf);
   if (!dir)
     {
	e_widget_textblock_markup_set(textblock, _("Unknown"));
	return;
     }
   snprintf(buf, sizeof(buf), "%s/profile.desktop", dir);
   desk = efreet_desktop_get(buf);
   if (desk)
     {
	e_widget_textblock_markup_set(textblock, desk->comment);
     }
   else
     e_widget_textblock_markup_set(textblock, _("Unknown"));
   if (desk) efreet_desktop_free(desk);
   free(dir);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   Eina_List *l, *profiles;
   int i, sel = -1;
   Evas_Object *ilist;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Profile"));
   of = e_widget_framelist_add(pg->evas, _("Select one"), 0);
   
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &profile);
   e_widget_min_size_set(ob, 140 * e_scale, 70 * e_scale);
   ilist = ob;
   e_widget_on_change_hook_set(ob, _profile_change, NULL);
   
   e_widget_ilist_freeze(ob);
   
   profiles = e_config_profile_list();
   for (i = 0, l = profiles; l; l = l->next, i++)
     {
        Efreet_Desktop *desk = NULL;
	char buf[PATH_MAX], *prof, *dir;
	const char *label;
	Evas_Object *ic;
	
	prof = l->data;
	dir = e_prefix_data_get();
	snprintf(buf, sizeof(buf), "%s/data/config/%s", dir, prof);
	dir = strdup(buf);
	if (!dir)
	  {
	     free(prof);
	     continue;
	  }
	
	snprintf(buf, sizeof(buf), "%s/profile.desktop", dir);
        desk = efreet_desktop_get(buf);
	label = prof;
	// FIXME: filter out wizard default profile
	if ((desk) && (desk->name)) label = desk->name;
	snprintf(buf, sizeof(buf), "%s/icon.edj", dir);
	if ((desk) && (desk->icon))
	  snprintf(buf, sizeof(buf), "%s/%s", dir, desk->icon);
	else
	  snprintf(buf, sizeof(buf), "%s/data/images/enlightenment.png", e_prefix_data_get());
	ic = e_util_icon_add(buf, pg->evas);
	e_widget_ilist_append(ob, ic, label, NULL, NULL, prof);
	if (e_config_profile_get())
	  {
	     if (!strcmp(prof, e_config_profile_get())) sel = i;
	  }
	free(dir);
	free(prof);
        if (desk) efreet_desktop_free(desk);
     }
   if (profiles) evas_list_free(profiles);

   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_textblock_add(pg->evas);
   e_widget_min_size_set(ob, 140 * e_scale, 70 * e_scale);
   e_widget_textblock_markup_set(ob, _("Select a profile"));
   textblock = ob;
   
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);

   if (sel >= 0) e_widget_ilist_selected_set(ilist, sel);
   
   evas_object_show(ob);
   evas_object_show(of);
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
   // FIXME: actually apply profile
   if (!profile) profile = "default";
   e_config_profile_set(profile);
   return 1;
}
