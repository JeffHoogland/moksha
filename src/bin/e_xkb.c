#include "e.h"

static void _e_xkb_update_event(int);

EAPI int E_EVENT_XKB_CHANGED = 0;

/* externally accessible functions */
EAPI int
e_xkb_init(void)
{
   E_EVENT_XKB_CHANGED = ecore_event_type_new();
   e_xkb_update(-1);
   return 1;
}

EAPI int
e_xkb_shutdown(void)
{
   return 1;
}

EAPI void
e_xkb_update(int cur_group)
{
   E_Config_XKB_Layout *cl;
   E_Config_XKB_Option *op;
   Eina_List *l;
   Eina_Strbuf *buf;

   if (!e_config->xkb.used_layouts) return;
   if (cur_group != -1)
     {
	ecore_x_xkb_select_group(cur_group);
	return;
     }
   /* We put an empty -option here in order to override all previously
    * set options.
    */

   buf = eina_strbuf_new();
   eina_strbuf_append(buf, "setxkbmap -layout '");

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        if (cl->name)
          {
             eina_strbuf_append(buf, cl->name);
             eina_strbuf_append(buf, ",");
          }
     }
   eina_strbuf_append(buf, "' -variant '");

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        if (cl->variant)
          {
             if (strcmp(cl->variant, "basic"))
               eina_strbuf_append(buf, cl->variant);
             eina_strbuf_append(buf, ",");
             /* workaround xkb bug where basic variants work with ',' at the
             * end, but dvorak (and mayby others) don't */
//           if (!strcmp(cl->variant, "basic")) eina_strbuf_append(buf, ",");
          }
        else
          eina_strbuf_append(buf, ",");
     }
   eina_strbuf_append(buf, "'");

   /* use first entry in used layouts */
   cl = e_config->xkb.used_layouts->data;

   if (cl->model)
     {
        eina_strbuf_append(buf, " -model '");
        if (strcmp(cl->model, "default"))
          eina_strbuf_append(buf, cl->model);
        else if ((e_config->xkb.default_model) &&
                 (strcmp(e_config->xkb.default_model, "default")))
          eina_strbuf_append(buf, e_config->xkb.default_model);
        else
          eina_strbuf_append(buf, "default");
        eina_strbuf_append(buf, "'");
     }

   /* clear options */
   eina_strbuf_append(buf, " -option ");

   /* add in selected options */
   EINA_LIST_FOREACH(e_config->xkb.used_options, l, op)
     {
        if (op->name)
          {
             eina_strbuf_append(buf, " -option '");
             eina_strbuf_append(buf, op->name);
             eina_strbuf_append(buf, "'");
          }
     }
   INF("SET XKB RUN: %s", eina_strbuf_string_get(buf));
   ecore_exe_run(eina_strbuf_string_get(buf), NULL);
   eina_strbuf_free(buf);
}

EAPI void
e_xkb_layout_next(void)
{
   int nb_groups = eina_list_count(e_config->xkb.used_layouts);
   
   e_config->xkb.cur_group = (e_config->xkb.cur_group + 1) % nb_groups;
   e_xkb_update(e_config->xkb.cur_group);
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

EAPI void
e_xkb_layout_prev(void)
{
   int nb_groups = eina_list_count(e_config->xkb.used_layouts);
   
   e_config->xkb.cur_group = (e_config->xkb.cur_group == 0) ? 
     (nb_groups - 1) : (e_config->xkb.cur_group - 1);
   e_xkb_update(e_config->xkb.cur_group);
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

EAPI void
e_xkb_layout_set(const char *name)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl;
   int cur_group = -1;

   if (!name) return;
   EINA_LIST_FOREACH(eina_list_next(e_config->xkb.used_layouts), l, cl)
     {
        cur_group++;
        if (!cl->name) continue;
        if (!strcmp(cl->name, name))
          {
             e_xkb_update(cur_group);
             _e_xkb_update_event(cur_group);
             break;
          }
     }
   e_config_save_queue();
}

EAPI const char *
e_xkb_layout_name_reduce(const char *name)
{
   if ((name) && (strchr(name, '/'))) name = strchr(name, '/') + 1;
   return name;
}

EAPI void
e_xkb_e_icon_flag_setup(Evas_Object *eicon, const char *name)
{
   int w, h;
   char buf[PATH_MAX];

   e_xkb_flag_file_get(buf, sizeof(buf), name);
   e_icon_file_set(eicon, buf);
   e_icon_size_get(eicon, &w, &h);
   edje_extern_object_aspect_set(eicon, EDJE_ASPECT_CONTROL_BOTH, w, h);
}

EAPI void
e_xkb_flag_file_get(char *buf, size_t bufsize, const char *name)
{
   name = e_xkb_layout_name_reduce(name);
   snprintf(buf, bufsize, "%s/data/flags/%s_flag.png",
            e_prefix_data_get(), name ? name : "unknown");
   if (!ecore_file_exists(buf))
     snprintf(buf, bufsize, "%s/data/flags/unknown_flag.png",
              e_prefix_data_get());
}

static void
_e_xkb_update_event(int cur_group)
{
   ecore_event_add(E_EVENT_XKB_CHANGED, NULL, NULL, (intptr_t*)(long)cur_group);
}
