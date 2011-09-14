/* Ask about compositing */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_cfdata.h"

static int do_comp = 1;
static int do_gl = 0;
static int do_vsync = 0;

static int
match_file_glob(FILE *f, const char *glob)
{
   char buf[32768];
   int found = 0;
   
   while (fgets(buf, sizeof(buf), f))
     {
        if (e_util_glob_match(buf, glob))
          {
             found = 1;
             break;
          }
     }
   fclose(f);
   return found;
}

static int
match_xorg_log(const char *glob)
{
   FILE *f;
   int i;
   char buf[PATH_MAX];
   
   for (i = 0; i < 5; i++)
     {
        snprintf(buf, sizeof(buf), "/var/log/Xorg.%i.log", i);
        f = fopen(buf, "rb");
        if (f)
          {
             if (match_file_glob(f, glob)) return 1;
          }
     }
   return 0;
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
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
   Ecore_Evas *ee;
   Ecore_X_Window_Attributes att;
   
   if (!ecore_x_composite_query()) return 0;
   if (!ecore_x_damage_query()) return 0;
   
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(ecore_x_window_root_first_get(), &att);
   if ((att.depth != 24) && (att.depth != 32)) return 0;
   
   ee = ecore_evas_gl_x11_new(NULL, 0, 0, 0, 320, 240);
   if (ee)
     {
        ecore_evas_free(ee);
        if (
            (match_xorg_log("*(II)*NVIDIA*: Creating default Display*")) ||
            (match_xorg_log("*(II)*intel*: Creating default Display*")) ||
            (match_xorg_log("*(II)*NOUVEAU*: Creating default Display*")) ||
            (match_xorg_log("*(II)*RADEON*: Creating default Display*"))
           )
          {
             do_gl = 1;
             do_vsync = 1;
          }
     }
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Compositing"));
   
   of = e_widget_framelist_add(pg->evas, _("Transparent windows and effects"), 0);
   
   ob = e_widget_textblock_add(pg->evas);
   e_widget_size_min_set(ob, 200 * e_scale, 180 * e_scale);
   e_widget_textblock_markup_set
      (ob, 
          _("Compositing provides translucency<br>"
            "for windows, window effects like<br>"
            "fading in and out and zooming<br>"
            "when they appear and dissapear.<br>"
            "It is highly recommended to<br>"
            "enable this for a better<br>"
            "experience, but it comes at a<br>"
            "cost. It requires extra CPU<br>"
            "or a GLSL Shader capable GPU<br>"
            "with well written drivers.<br>"
            "It also will add between 10 to<br>"
            "100 MB to the memory needed<br>"
            "for Enlightenment."
              )
      );
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(pg->evas, _("Enable Compositing"), &(do_comp));
   e_widget_framelist_object_append(of, ob);
   
   if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_OPENGL_X11))
     {
        ob = e_widget_check_add(pg->evas, _("Hardware Accelerated (OpenGL)"), &(do_gl));
        e_widget_framelist_object_append(of, ob);
        
        ob = e_widget_check_add(pg->evas, _("Tear-free Rendering (OpenGL only)"), &(do_vsync));
        e_widget_framelist_object_append(of, ob);
     }
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);

   evas_object_show(of);

   e_wizard_page_show(o);
//   pg->data = o;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   if (!do_comp)
     {
        E_Config_Module *em;
        Eina_List *l;
        
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!em->name) continue;
             if (!strcmp(em->name, "comp"))
               {
                  e_config->modules = eina_list_remove_list
                     (e_config->modules, l);
                  if (em->name) eina_stringshare_del(em->name);
                  free(em);
                  break;
               }
          }
        e_config->use_composite = 0;
     }
   else
     {
        E_Config_Module *em;
        Eina_List *l;
        E_Config_DD *conf_edd = NULL;
        E_Config_DD *conf_match_edd = NULL;
        Config *cfg = NULL;
        
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!em->name) continue;
             if (!strcmp(em->name, "dropshadow"))
               {
                  e_config->modules = eina_list_remove_list
                     (e_config->modules, l);
                  if (em->name) eina_stringshare_del(em->name);
                  free(em);
                  break;
               }
          }
        
        e_config->use_composite = 1;
        
        e_mod_comp_cfdata_edd_init(&(conf_edd), &(conf_match_edd));
        cfg = e_mod_comp_cfdata_config_new();
        
        if (do_gl)
          {
             cfg->engine = E_EVAS_ENGINE_GL_X11;
             cfg->smooth_windows = 1;
             cfg->vsync = do_vsync;
          }
        else
          {
             cfg->engine = E_EVAS_ENGINE_SOFTWARE_X11;
             cfg->smooth_windows = 0;
             cfg->vsync = 0;
          }
        
        e_config_domain_save("module.comp", conf_edd, cfg);
        E_CONFIG_DD_FREE(conf_match_edd);
        E_CONFIG_DD_FREE(conf_edd);
     }
   e_config_save_queue();
//   if (pg->data) evas_object_del(pg->data);
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
