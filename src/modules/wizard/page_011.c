/* Language chooser */
#include "e.h"
#include "e_mod_main.h"

typedef struct _Layout Layout;

struct _Layout
{
   const char *name;
   const char *label;
};

static const char *rules_file = NULL;
static const char *layout = NULL;
static Eina_List *layouts = NULL;

static void
find_rules(void)
{
   int i = 0;
   const char *lstfiles[] = {
#if defined __NetBSD__
      "/usr/X11R7/lib/X11/xkb/rules/xorg.lst",
#elif defined __OpenBSD__
      "/usr/X11R6/share/X11/xkb/rules/base.lst",
#endif
      "/usr/share/X11/xkb/rules/xorg.lst",
      "/usr/share/X11/xkb/rules/xfree86.lst",
      "/usr/local/share/X11/xkb/rules/xorg.lst",
      "/usr/local/share/X11/xkb/rules/xfree86.lst",
      "/usr/X11R6/lib/X11/xkb/rules/xorg.lst",
      "/usr/X11R6/lib/X11/xkb/rules/xfree86.lst",
      "/usr/local/X11R6/lib/X11/xkb/rules/xorg.lst",
      "/usr/local/X11R6/lib/X11/xkb/rules/xfree86.lst",
      NULL
   };

   for (; lstfiles[i]; i++)
     {
        FILE *f = fopen(lstfiles[i], "r");
        if (f)
          {
             fclose(f);
             rules_file = lstfiles[i];
             break;
          }
     }
}

static int
_layout_sort_cb(const void *data1, const void *data2)
{
   const Layout *l1 = data1;
   const Layout *l2 = data2;
   return e_util_strcasecmp(l1->label ?: l1->name, l2->label ?: l2->name);
}

int
parse_rules(void)
{
   char buf[4096];
   FILE *f = fopen(rules_file, "r");
   if (!f) return 0;

   for (;; )
     {
        if (!fgets(buf, sizeof(buf), f)) goto err;
        if (!strncmp(buf, "! layout", 8))
          {
             for (;; )
               {
                  Layout *lay;
                  char name[4096], label[4096];

                  if (!fgets(buf, sizeof(buf), f)) goto err;
                  if (sscanf(buf, "%s %[^\n]", name, label) != 2) break;
                  lay = calloc(1, sizeof(Layout));
                  lay->name = eina_stringshare_add(name);
                  lay->label = eina_stringshare_add(label);
                  layouts = eina_list_append(layouts, lay);
               }
             break;
          }
     }
err:
   fclose(f);
   layouts = eina_list_sort(layouts, 0, _layout_sort_cb);
   return 1;
}

static void
implement_layout(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *nl;
   Eina_Bool found = EINA_FALSE;

   if (!layout) return;

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, nl)
     {
        if ((nl->name) && (!strcmp(layout, nl->name)))
          {
             found = EINA_TRUE;
             break;
          }
     }
   if (!found) return;
   e_xkb_layout_set(layout);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   // parse kbd rules here
   find_rules();
   parse_rules();
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
   Evas_Object *o, *of, *ob, *ic;
   Eina_List *l;
   int i, sel = -1;

   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Keyboard"));
   of = e_widget_framelist_add(pg->evas, _("Select one"), 0);
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &layout);
   e_widget_size_min_set(ob, 140 * e_scale, 140 * e_scale);

   e_widget_ilist_freeze(ob);
   for (i = 0, l = layouts; l; l = l->next, i++)
     {
        Layout *lay;
        const char *label;

        lay = l->data;
        ic = e_icon_add(pg->evas);
        e_xkb_e_icon_flag_setup(ic, lay->name);
        label = lay->label;
        if (!label) label = "Unknown";
        e_widget_ilist_append(ob, ic, _(label), NULL, NULL, lay->name);
        if (lay->name)
          {
             if (!strcmp(lay->name, "us")) sel = i;
          }
     }

   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   if (sel >= 0)
     {
        e_widget_ilist_selected_set(ob, sel);
        e_widget_ilist_nth_show(ob, sel, 0);
     }

   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   e_wizard_page_show(o);
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   /* special - key layout inits its stuff the moment it goes away */
   implement_layout();
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   // do this again as we want it to apply to the new profile
   implement_layout();
   return 1;
}

