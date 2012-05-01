#include "e.h"

static void _e_xkb_update_event(void);

EAPI int E_EVENT_XKB_CHANGED = 0;

/* externally accessible functions */
EAPI int
e_xkb_init(void)
{
   E_EVENT_XKB_CHANGED = ecore_event_type_new();
   e_xkb_update();
   return 1;
}

EAPI int
e_xkb_shutdown(void)
{
   return 1;
}

EAPI void
e_xkb_update(void)
{  
   E_Config_XKB_Layout *cl;
   E_Config_XKB_Option *op;
   Eina_List *l;
   Eina_Strbuf *buf;
   
   if (!e_config->xkb.used_layouts) return;
   
   /* We put an empty -option here in order to override all previously
    * set options.
    */
   
   buf = eina_strbuf_new();
   eina_strbuf_append(buf, "setxkbmap '");
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        eina_strbuf_append(buf, cl->name);
        break;
        //if (l->next) eina_strbuf_append(buf, ",");
     }
   eina_strbuf_append(buf, "'");
   
   eina_strbuf_append(buf, " -variant '");
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        eina_strbuf_append(buf, cl->variant);
        eina_strbuf_append(buf, ",");
        break;
     }
   eina_strbuf_append(buf, "'");
   
   cl = eina_list_data_get(e_config->xkb.used_layouts);
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
   
   EINA_LIST_FOREACH(e_config->xkb.used_options, l, op)
     {
        if (op->name)
          {
             eina_strbuf_append(buf, " -option '");
             eina_strbuf_append(buf, op->name);
             eina_strbuf_append(buf, "'");
          }
        break;
     }
   printf("SEWT XKB RUN:\  %s\n", eina_strbuf_string_get(buf));
   ecore_exe_run(eina_strbuf_string_get(buf), NULL);
   eina_strbuf_free(buf);
}

EAPI void
e_xkb_layout_next(void)
{  
   void *odata, *ndata;
   Eina_List *l;
   
   odata = eina_list_data_get(e_config->xkb.used_layouts);
   
   EINA_LIST_FOREACH(eina_list_next(e_config->xkb.used_layouts), l, ndata)
     {
        eina_list_data_set(eina_list_prev(l), ndata);
     }
   
   eina_list_data_set(eina_list_last(e_config->xkb.used_layouts), odata);
   e_xkb_update();
   _e_xkb_update_event();
   e_config_save_queue();
}

EAPI void
e_xkb_layout_prev(void)
{  
   void *odata, *ndata;
   Eina_List *l;
   
   odata = eina_list_data_get(eina_list_last(e_config->xkb.used_layouts));
   
   for (l = e_config->xkb.used_layouts, ndata = eina_list_data_get(l);
        l; l = eina_list_next(l))
     {
        if (eina_list_next(l))
          ndata = eina_list_data_set(eina_list_next(l), ndata);
     }
   
   eina_list_data_set(e_config->xkb.used_layouts, odata);
   e_xkb_update();
   _e_xkb_update_event();
   e_config_save_queue();
}

EAPI void
e_xkb_layout_set(const char *name)
{  
   Eina_List *l;
   E_Config_XKB_Layout *cl;
   
   if (!name) return;
   EINA_LIST_FOREACH(eina_list_next(e_config->xkb.used_layouts), l, cl)
     {
        if (!cl->name) continue;
        if (!strcmp(cl->name, name))
          {
             e_config->xkb.used_layouts =
               eina_list_remove_list(e_config->xkb.used_layouts, l);
             e_config->xkb.used_layouts =
               eina_list_prepend(e_config->xkb.used_layouts, cl);
             e_xkb_update();
             _e_xkb_update_event();
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
_e_xkb_update_event(void)
{
   ecore_event_add(E_EVENT_XKB_CHANGED, NULL, NULL, NULL);
}
