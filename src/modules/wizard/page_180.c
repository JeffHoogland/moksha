/* Setup if we need connman? */
#include "e.h"
#include "e_mod_main.h"

static int do_tasks = 1;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Taskbar"));

   of = e_widget_framelist_add(pg->evas, _("Information"), 0);

   ob = e_widget_textblock_add(pg->evas);
   e_widget_size_min_set(ob, 260 * e_scale, 200 * e_scale);
   e_widget_textblock_markup_set
     (ob,
     _("A taskbar can be added to<br>"
       "show open windows and applications."
       )
     );
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(pg->evas, _("Enable Taskbar"), &(do_tasks));
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 0, 0, 0.5);

   evas_object_show(of);

   e_wizard_page_show(o);
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   E_Config_Module *em;
   Eina_List *l;

   if (do_tasks) return 1;

   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
        if (!em->name) continue;
        if (strcmp(em->name, "tasks")) continue;
        e_config->modules = eina_list_remove_list(e_config->modules, l);
        eina_stringshare_del(em->name);
        free(em);
        break;
     }

   return 1;
}

