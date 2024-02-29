#include "e.h"

static void _e_xkb_update_event(int);
static void border_xkb_add(int cur_group);

static int _e_xkb_cur_group = -1;
static int _e_focus = 0;

static Ecore_Exe *cur_exe;

EAPI int E_EVENT_XKB_CHANGED = 0;

static Eina_Bool
_e_xkb_init_timer(void *data)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl2, *cl = data;
   int cur_group = -1;

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl2)
     {
        cur_group++;
        if (!cl2->name) continue;
        if (e_config_xkb_layout_eq(cl, cl2))
          {
             INF("Setting keyboard layout: %s|%s|%s", cl2->name, cl2->model, cl2->variant);
             e_xkb_update(cur_group);
             break;
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
kb_exe_del(void *d __UNUSED__, int t __UNUSED__, Ecore_Exe_Event_Del *ev)
{
   if (ev->exe == cur_exe)
     cur_exe = NULL;
   return ECORE_CALLBACK_RENEW;
}



static Eina_Bool
border_focus(void *d __UNUSED__, int t __UNUSED__, Ecore_Exe_Event_Del *ev __UNUSED__)
{
   E_Border *bd;
   Eina_List *l, *ll;
   E_Config_XKB_Layout *cl;

   if (e_config->xkb.wins_xkb == XKB_GLOBAL) return ECORE_CALLBACK_RENEW;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
       if (bd->focused)
         {
           _e_focus = 1;
           /* add the first layout to newly opened app window without rem */
           if ((!bd->remember) && (!bd->cl))
             {
                bd->cl = eina_list_nth(e_config->xkb.used_layouts, 0);
             }
           /* just set the layout, no need to remember it */
           if (bd->cl)
             {
                EINA_LIST_FOREACH(e_config->xkb.used_layouts, ll, cl)
                  {
                     if (bd->cl == cl)
                        {
                           e_xkb_layout_set(bd->cl);
                           _e_focus = 0;
                            break;
                        }
                  }
                return ECORE_CALLBACK_RENEW;
             }
           /* retrieve xkb settings from remember struct after app restart */
           if (bd->remember && bd->remember->prop.xkb)
             {
               EINA_LIST_FOREACH(e_config->xkb.used_layouts, ll, cl)
                 {
                   if (!strcmp(cl->name, bd->remember->prop.cl_name) &&
                       !strcmp(cl->model, bd->remember->prop.cl_model) &&
                       !strcmp(cl->variant, bd->remember->prop.cl_variant))
                     {
                        bd->cl = cl;
                        if (bd->cl)
                          {
                             e_xkb_layout_set(bd->cl);
                             break;
                          }
                     }
                 }
             }
           _e_focus = 0;
         }
     }
   return ECORE_CALLBACK_RENEW;
}

static void
border_xkb_add(int cur_group)
{
   E_Border *bd;
   Eina_List *l;
   E_Remember *rem = NULL;
   
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
       if (bd->focused)
         {
           rem = bd->remember;
           if (!rem)
             rem = e_remember_new();
           if (rem)
             {
               /* delete rem if the first layout is set */
               if (cur_group == 0 && rem->apply == (1 << 17))
                 {
                  e_remember_del(rem);
                 }
               else if (cur_group != _e_xkb_cur_group)
                 {
                   bd->cl = eina_list_nth(e_config->xkb.used_layouts, cur_group);
                   rem->prop.cl_name = bd->cl->name;
                   rem->prop.cl_model = bd->cl->model;
                   rem->prop.cl_variant = bd->cl->variant;

                   /* activate keeping xkb layer */
                   rem->apply |= E_REMEMBER_APPLY_XKB;
                   rem->prop.xkb = 1;
                    
                   /* store border rem structure */
                   bd->remember = rem;
                   e_remember_default_match_set(rem, bd);
                   if (bd->client.icccm.window_role)
                     rem->match &= ~E_REMEMBER_MATCH_ROLE;

                   /* exclude app open/save file dialogs etc... */
                   rem->match &= ~E_REMEMBER_MATCH_TYPE;
                   rem->match &= ~E_REMEMBER_MATCH_TRANSIENT;

                   /* libreoffice hack for icccm name and class */
                   if (e_util_glob_match(bd->client.icccm.name, "libreoffice"))
                     {
                       rem->name = eina_stringshare_add("soffice");
                       rem->class = eina_stringshare_add("Soffice");
                     }
                   e_remember_use(rem);
                   e_remember_update(bd);
                 }
             }
         }
     }
}

/* externally accessible functions */
EAPI int
e_xkb_init(void)
{
   if (!E_EVENT_XKB_CHANGED)
     {
        E_EVENT_XKB_CHANGED = ecore_event_type_new();
        ecore_event_handler_add(ECORE_EXE_EVENT_DEL, (Ecore_Event_Handler_Cb)kb_exe_del, NULL);
     }
   ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, (Ecore_Event_Handler_Cb)border_focus, NULL);
   e_xkb_update(-1);
   if (e_config->xkb.cur_layout)
     ecore_timer_add(1.5, _e_xkb_init_timer, e_config->xkb.current_layout);
   else if (e_config->xkb.selected_layout)
     ecore_timer_add(1.5, _e_xkb_init_timer, e_config->xkb.sel_layout);
   else if (e_config->xkb.used_layouts)
     ecore_timer_add(1.5, _e_xkb_init_timer, eina_list_data_get(e_config->xkb.used_layouts));
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

   if ((!e_config->xkb.used_layouts) && (!e_config->xkb.used_options) && (!e_config->xkb.default_model)) return;
   
   if (cur_group != -1)
     {
        if (e_config->xkb.wins_xkb == XKB_PER_APP)
          {
            if (_e_focus == 0)
              border_xkb_add(cur_group);
          }
        _e_xkb_cur_group = cur_group;
        ecore_x_xkb_select_group(cur_group);
        e_deskenv_xmodmap_run();
        _e_xkb_update_event(cur_group);
        return;
     }
   /* We put an empty -option here in order to override all previously
    * set options.
    */
   buf = eina_strbuf_new();
   eina_strbuf_append(buf, "setxkbmap ");

   if (e_config->xkb.used_layouts)
     {
        eina_strbuf_append(buf, "-layout '");
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
             if ((cl->variant) && (strcmp(cl->variant, "basic")))
               {
                  eina_strbuf_append(buf, cl->variant);
                  eina_strbuf_append(buf, ",");
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
     }
   else if (e_config->xkb.default_model)
     {
        eina_strbuf_append(buf, " -model '");
        if (strcmp(e_config->xkb.default_model, "default"))
          eina_strbuf_append(buf, e_config->xkb.default_model);
        else if ((e_config->xkb.default_model) &&
                 (strcmp(e_config->xkb.default_model, "default")))
          eina_strbuf_append(buf, e_config->xkb.default_model);
        else
          eina_strbuf_append(buf, "default");
        eina_strbuf_append(buf, "'");
     }

   if (e_config->xkb.used_options)
     {
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
     }
   INF("SET XKB RUN: %s", eina_strbuf_string_get(buf));
   E_FREE_FUNC(cur_exe, ecore_exe_kill);
   cur_exe = ecore_exe_run(eina_strbuf_string_get(buf), NULL);
     
   eina_strbuf_free(buf);
}

EAPI void
e_xkb_layout_next(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl;

   if (!e_config->xkb.used_layouts) return;
   l = eina_list_nth_list(e_config->xkb.used_layouts, e_config->xkb.cur_group);
   l = eina_list_next(l);
   if (!l) l = e_config->xkb.used_layouts;

   e_config->xkb.cur_group = (e_config->xkb.cur_group + 1) % eina_list_count(e_config->xkb.used_layouts);
   cl = eina_list_data_get(l);
   eina_stringshare_replace(&e_config->xkb.cur_layout, cl->name);
   eina_stringshare_replace(&e_config->xkb.selected_layout, cl->name);
   INF("Setting keyboard layout: %s|%s|%s", cl->name, cl->model, cl->variant);
   e_xkb_update(e_config->xkb.cur_group);
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

EAPI void
e_xkb_layout_prev(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl;

   if (!e_config->xkb.used_layouts) return;
   l = eina_list_nth_list(e_config->xkb.used_layouts, e_config->xkb.cur_group);
   l = eina_list_prev(l);
   if (!l) l = eina_list_last(e_config->xkb.used_layouts);

   e_config->xkb.cur_group = (e_config->xkb.cur_group == 0) ?
     ((int)eina_list_count(e_config->xkb.used_layouts) - 1) : (e_config->xkb.cur_group - 1);
   cl = eina_list_data_get(l);
   eina_stringshare_replace(&e_config->xkb.cur_layout, cl->name);
   eina_stringshare_replace(&e_config->xkb.selected_layout, cl->name);
   INF("Setting keyboard layout: %s|%s|%s", cl->name, cl->model, cl->variant);
   e_xkb_update(e_config->xkb.cur_group);
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

/* always use this function to get the current layout's name
 * to ensure the most accurate results!!!
 */
EAPI E_Config_XKB_Layout *
e_xkb_layout_get(void)
{
   unsigned int n = 0;

   if (e_config->xkb.current_layout) return e_config->xkb.current_layout;
   if (_e_xkb_cur_group >= 0)
     n = _e_xkb_cur_group;
   return eina_list_nth(e_config->xkb.used_layouts, n);
}

EAPI void
e_xkb_layout_set(const E_Config_XKB_Layout *cl)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl2;
   int cur_group = -1;

   EINA_SAFETY_ON_NULL_RETURN(cl);
   if (e_config_xkb_layout_eq(e_config->xkb.current_layout, cl)) return;
   e_config_xkb_layout_free(e_config->xkb.current_layout);
   e_config->xkb.current_layout = e_config_xkb_layout_dup(cl);
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl2)
     {
        cur_group++;
        if (!cl2->name) continue;
        if (e_config_xkb_layout_eq(cl, cl2))
          {
             INF("Setting keyboard layout: %s|%s|%s", cl2->name, cl2->model, cl2->variant);
             e_xkb_update(cur_group);
             break;
          }
     }
   e_config_save_queue();
}

EAPI const char *
e_xkb_layout_name_reduce(const char *name)
{
   const char *s;

   if (!name) return NULL;
   s = strchr(name, '/');
   if (s) s++;
   else s = name;
   return s;
}

EAPI void
e_xkb_e_icon_flag_setup(Evas_Object *eicon, const char *name)
{
   int w, h;
   char buf[PATH_MAX];

   e_xkb_flag_file_get(buf, sizeof(buf), name);
   e_icon_file_set(eicon, buf);
   e_icon_size_get(eicon, &w, &h);
   evas_object_size_hint_aspect_set(eicon, EVAS_ASPECT_CONTROL_BOTH, w, h);
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

EAPI Eina_Bool
e_config_xkb_layout_eq(const E_Config_XKB_Layout *a, const E_Config_XKB_Layout *b)
{
   if (a == b) return EINA_TRUE;
   if ((!a) || (!b)) return EINA_FALSE;
   return ((a->name == b->name) && (a->model == b->model) && (a->variant == b->variant));
}

EAPI void
e_config_xkb_layout_free(E_Config_XKB_Layout *cl)
{
   if (!cl) return;

   eina_stringshare_del(cl->name);
   eina_stringshare_del(cl->model);
   eina_stringshare_del(cl->variant);
   free(cl);
}

EAPI E_Config_XKB_Layout *
e_config_xkb_layout_dup(const E_Config_XKB_Layout *cl)
{
   E_Config_XKB_Layout *cl2;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   cl2 = E_NEW(E_Config_XKB_Layout, 1);
   cl2->name = eina_stringshare_ref(cl->name);
   cl2->model = eina_stringshare_ref(cl->model);
   cl2->variant = eina_stringshare_ref(cl->variant);
   return cl2;
}

static void
_e_xkb_update_event(int cur_group)
{
   ecore_event_add(E_EVENT_XKB_CHANGED, NULL, NULL, (intptr_t *)(long)cur_group);
}
