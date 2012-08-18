#include "e.h"
#include "e_mod_parse.h"

Eina_List *layouts = NULL;
Eina_List *models = NULL;
Eina_List *optgroups = NULL;

static const char *rules_file = NULL;

void
find_rules(void)
{
   int i = 0;
   const char *lstfiles[] = {
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

int
parse_rules(void)
{
   char buf[4096];
   E_XKB_Model *model = NULL;
   E_XKB_Layout *layout = NULL;
   E_XKB_Option *option = NULL;
   E_XKB_Variant *variant = NULL;
   E_XKB_Option_Group *group = NULL;
   FILE *f;

   if (!rules_file) return 0;

   layouts = NULL;
   models = NULL;

   f = fopen(rules_file, "r");
   if (!f) return 0;

   /* move on to next line, the first one is useless */
   if (!fgets(buf, sizeof(buf), f)) goto err;

   /* let X decide on this one, also serves as
    * "fallback on global" for layout combinations
    */
   model = E_NEW(E_XKB_Model, 1);
   model->name = eina_stringshare_add("default");
   model->description = eina_stringshare_add("Automatic");
   models = eina_list_append(models, model);

   /* read models here */
   for (;;)
     {
        if (fgets(buf, sizeof(buf), f))
          {
             char *n;
             char *p;
             char *tmp;

             n = strchr(buf, '\n');
             if (n) *n = '\0';

             /* means end of section */
             if (!buf[0]) break;
             /* get rid of initial 2 spaces here */
             p   = buf + 2;
             tmp = strdup(p);

             model = E_NEW(E_XKB_Model, 1);
             model->name = eina_stringshare_add(strtok(tmp, " "));

             free(tmp);

             p += strlen(model->name);
             while (p[0] == ' ') ++p;

             model->description = eina_stringshare_add(p);

             models = eina_list_append(models, model);
          }
        else
          break;
     }

   /* move on again */
   if (!fgets(buf, sizeof(buf), f)) goto err;

   /* read layouts here */
   for (;;)
     {
        if (fgets(buf, sizeof(buf), f))
          {
             char *n;
             char *p;
             char *tmp;

             n = strchr(buf, '\n');
             if   (n) *n = '\0';

             if (!buf[0]) break;

             p   = buf + 2;
             tmp = strdup(p);

             layout = E_NEW(E_XKB_Layout, 1);
             layout->name = eina_stringshare_add(strtok(tmp, " "));

             free(tmp);

             p += strlen(layout->name);
             while (p[0] == ' ') ++p;

             variant = E_NEW(E_XKB_Variant, 1);
             variant->name = eina_stringshare_add("basic");
             variant->description = eina_stringshare_add("Default layout variant");

             layout->description = eina_stringshare_add(p);
             layout->variants = eina_list_append(layout->variants, variant);

             layouts = eina_list_append(layouts, layout);
          }
        else
          break;
     }

   /* move on again */
   if (!fgets(buf, sizeof(buf), f)) goto err;

   /* read variants here */
   for (;;)
     {
        if (fgets(buf, sizeof(buf), f))
          {
             char *n;
             char *p;
             char *tmp;
             char   *tok;

             n = strchr(buf, '\n');
             if   (n) *n = '\0';

             if (!buf[0]) break;

             p   = buf + 2;
             tmp = strdup(p);

             variant = E_NEW(E_XKB_Variant, 1);
             variant->name = eina_stringshare_add(strtok(tmp, " "));

             tok = strtok(NULL, " ");
             *strchr(tok, ':') = '\0';

             layout = eina_list_search_unsorted(layouts, layout_sort_by_name_cb, tok);
             layout->variants = eina_list_append(layout->variants, variant);

             p += strlen(variant->name);
             while (p[0] == ' ') ++p;
             p += strlen(tok);
             p += 2;

             free(tmp);

             variant->description = eina_stringshare_add(p);
          }
        else
          break;
     }

   /* move on again */
   if (!fgets(buf, sizeof(buf), f)) goto err;

   /* read options here */
   for (;;)
     {
        if (fgets(buf, sizeof(buf), f))
          {
             char *n;
             char *p;
             char *tmp;
             char *name;

             n = strchr(buf, '\n');
             if   (n) *n = '\0';

             if (!buf[0]) break;

             p   = buf + 2;
             tmp  = strdup(p);
             name = strtok(tmp, " ");

             p += strlen(name);
             while (p[0] == ' ') ++p;

             /* skip "grp" options for switching kbd layouts */
             //if (strncmp(name, "grp", 3))
               {
                  if (!strchr(name, ':'))
                    {
                       group = E_NEW(E_XKB_Option_Group, 1);

                       /* A hack to get it to parse right if
                        * the group name contains a space
                        */
                       if (strstr(p, "  "))
                         {
                            p = strstr(p, "  ");
                            while (p[0] == ' ') ++p;
                         }

                       group->description = eina_stringshare_add(p);

                       optgroups = eina_list_append(optgroups, group);
                    }
                  else if (group)
                    {
                       option = E_NEW(E_XKB_Option, 1);
                       option->name = eina_stringshare_add(name);
                       option->description = eina_stringshare_add(p);
                       group->options = eina_list_append(group->options,
                                                         option);
                    }
               }

             free(tmp);
          }
        else
          break;
     }

err:
   fclose(f);

   /* Sort layouts */
   layouts =
     eina_list_sort(layouts, eina_list_count(layouts), layout_sort_cb);
   return 1;
}

void
clear_rules(void)
{
   E_XKB_Option_Group *og;
   E_XKB_Variant      *v;
   E_XKB_Option       *o;
   E_XKB_Layout       *la;
   E_XKB_Model        *m;

   EINA_LIST_FREE(layouts, la)
     {
        eina_stringshare_del(la->name);
        eina_stringshare_del(la->description);

        EINA_LIST_FREE(la->variants, v)
          {
             eina_stringshare_del(v->name);
             eina_stringshare_del(v->description);

             E_FREE(v);
          }

        E_FREE(la);
     }

   EINA_LIST_FREE(models, m)
     {
        eina_stringshare_del(m->name);
        eina_stringshare_del(m->description);

        E_FREE(m);
     }

   EINA_LIST_FREE(optgroups, og)
     {
        eina_stringshare_del(og->description);

        EINA_LIST_FREE(og->options, o)
          {
             eina_stringshare_del(o->name);
             eina_stringshare_del(o->description);

             E_FREE(o);
          }

        E_FREE(og);
     }

   optgroups = NULL;
   layouts = NULL;
   models = NULL;
}

int
layout_sort_cb(const void *data1, const void *data2)
{
   const E_XKB_Layout *l1, *l2;

   if (!(l1 = data1)) return 1;
   if (!l1->name) return 1;
   if (!(l2 = data2)) return -1;
   if (!l2->name) return -1;

   return strcmp(l1->name, l2->name);
}

int
layout_sort_by_name_cb(const void *data1, const void *data2)
{
   const E_XKB_Layout *l1 = NULL;
   const char *l2 = NULL;

   if (!(l1 = data1)) return 1;
   if (!l1->name) return 1;
   if (!(l2 = data2)) return -1;

   return strcmp(l1->name, l2);
}
