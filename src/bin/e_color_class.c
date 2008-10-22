/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI int
e_color_class_init(void)
{
  Eina_List *l;

  for (l = e_config->color_classes; l; l = l->next)
    {
       E_Color_Class *cc;

       cc = l->data;
       if (!cc) continue;

       printf("INIT CC: %s, %d %d %d %d\n", cc->name, cc->r, cc->g, cc->b, cc->a);
       edje_color_class_set(cc->name,
			    cc->r, cc->g, cc->b, cc->a,
			    cc->r2, cc->g2, cc->b2, cc->a2,
			    cc->r3, cc->g3, cc->b3, cc->a3);

    }
  return 1;
}

EAPI int
e_color_class_shutdown(void)
{
   return 1;
}

EAPI void
e_color_class_set(const char *color_class, int r, int g, int b, int a, int r2, int b2, int g2, int a2, int r3, int g3, int b3, int a3)
{
  E_Color_Class *cc = NULL;

  cc = e_color_class_find(color_class);
  if (!cc)
    {
       cc = E_NEW(E_Color_Class, 1);
       e_config->color_classes = eina_list_append(e_config->color_classes, cc);
       cc->name = eina_stringshare_add(color_class);
       cc->r = cc->g = cc->b = cc->a = 255;
       cc->r2 = cc->g2 = cc->b2 = cc->a2 = 255;
       cc->r3 = cc->g3 = cc->b3 = cc->a3 = 255;
    }

  if (r != -1) cc->r = E_CLAMP(r, 0, 255);
  if (g != -1) cc->g = E_CLAMP(g, 0, 255);
  if (b != -1) cc->b = E_CLAMP(b, 0, 255);
  if (a != -1) cc->a = E_CLAMP(a, 0, 255);
  if (r2 != -1) cc->r2 = E_CLAMP(r2, 0, 255);
  if (g2 != -1) cc->g2 = E_CLAMP(g2, 0, 255);
  if (b2 != -1) cc->b2 = E_CLAMP(b2, 0, 255);
  if (a2 != -1) cc->a2 = E_CLAMP(a2, 0, 255);
  if (r3 != -1) cc->r3 = E_CLAMP(r3, 0, 255);
  if (g3 != -1) cc->g3 = E_CLAMP(g3, 0, 255);
  if (b3 != -1) cc->b3 = E_CLAMP(b3, 0, 255);
  if (a3 != -1) cc->a3 = E_CLAMP(a3, 0, 255);

  edje_color_class_set(cc->name,
                       cc->r, cc->g, cc->b, cc->a,
                       cc->r2, cc->g2, cc->b2, cc->a2,
                       cc->r3, cc->g3, cc->b3, cc->a3);
  e_config_save_queue();
}

EAPI void
e_color_class_del(const char *name)
{
  E_Color_Class *cc = NULL;

  cc = e_color_class_find(name);
  if (cc)
    {
       e_config->color_classes = eina_list_remove(e_config->color_classes, cc);
       edje_color_class_del(cc->name);
       eina_stringshare_del(cc->name);
       E_FREE(cc);

       e_config_save_queue();
    }
}

EAPI E_Color_Class *
e_color_class_find(const char *name)
{
  Eina_List *l;
  E_Color_Class *cc = NULL;

  for (l = e_config->color_classes; l; l = l->next)
    {
       cc = l->data;
       if (!cc) continue;

       if (!strcmp(cc->name, name))
	 {
	    return cc;
	    break;
	 }
    }
  return NULL;
}


EAPI Eina_List *
e_color_class_list(void)
{
  return e_config->color_classes;
}
