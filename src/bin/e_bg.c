#include "e.h"

/* local subsystem functions */
static void _e_bg_signal(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_bg_event_bg_update_free(void *data, void *event);
static Eina_Bool  _e_bg_slide_animator(void *data);

static void _e_bg_image_import_dialog_done(void *data, const char *path, Eina_Bool ok, Eina_Bool external, int quality, E_Image_Import_Mode mode);
static void _e_bg_image_import_done(void *data, Eina_Bool ok, const char *image_path, const char *edje_path);
static void _e_bg_handler_image_imported(void *data, const char *image_path);

/* local subsystem globals */
EAPI int E_EVENT_BG_UPDATE = 0;
static E_Fm2_Mime_Handler *bg_hdl = NULL;

typedef struct _E_Bg_Anim_Params E_Bg_Anim_Params;
struct _E_Bg_Anim_Params
{
   E_Zone *zone;
   double start_time;
   int start_x;
   int start_y;
   int end_x;
   int end_y;

   struct {
      Eina_Bool x, y;
   } freedom;
};

struct _E_Bg_Image_Import_Handle
{
   struct {
      void (*func)(void *data, const char *edje_file);
      void *data;
   } cb;
   E_Dialog *dia;
   E_Util_Image_Import_Handle *importer;
   Eina_Bool canceled:1;
};

/* externally accessible functions */
EINTERN int
e_bg_init(void)
{
   Eina_List *l = NULL;
   E_Config_Desktop_Background *cfbg = NULL;

   /* Register mime handler */
   bg_hdl = e_fm2_mime_handler_new(_("Set As Background"),
				   "preferences-desktop-wallpaper",
				   e_bg_handler_set, NULL,
				   e_bg_handler_test, NULL);
   if (bg_hdl)
     {
	e_fm2_mime_handler_glob_add(bg_hdl, "*.edj");
	e_fm2_mime_handler_mime_add(bg_hdl, "image/png");
	e_fm2_mime_handler_mime_add(bg_hdl, "image/jpeg");
     }

   /* Register files in use */
   if (e_config->desktop_default_background)
     e_filereg_register(e_config->desktop_default_background);

   EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cfbg)
     {
	if (!cfbg) continue;
	e_filereg_register(cfbg->file);
     }

   E_EVENT_BG_UPDATE = ecore_event_type_new();
   return 1;
}

EINTERN int
e_bg_shutdown(void)
{
   Eina_List *l = NULL;
   E_Config_Desktop_Background *cfbg = NULL;

   /* Deregister mime handler */
   if (bg_hdl)
     {
	e_fm2_mime_handler_glob_del(bg_hdl, "*.edj");
	e_fm2_mime_handler_free(bg_hdl);
     }

   /* Deregister files in use */
   if (e_config->desktop_default_background)
     e_filereg_deregister(e_config->desktop_default_background);

   EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cfbg)
     {
	if (!cfbg) continue;
	e_filereg_deregister(cfbg->file);
     }

   return 1;
}

/**
 * Find the configuration for a given desktop background
 * Use -1 as a wild card for each parameter.
 * The most specific match will be returned
 */
EAPI const E_Config_Desktop_Background *
e_bg_config_get(int container_num, int zone_num, int desk_x, int desk_y)
{
   Eina_List *l, *ll, *entries;
   E_Config_Desktop_Background *bg = NULL, *cfbg = NULL;
   const char *bgfile = "";
   char *entry;
   int current_spec = 0; /* how specific the setting is - we want the least general one that applies */

   /* look for desk specific background. */
   if (container_num >= 0 || zone_num >= 0 || desk_x >= 0 || desk_y >= 0)
     {
	EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cfbg)
	  {
	     int spec;
             
	     if (!cfbg) continue;
	     spec = 0;
	     if (cfbg->container == container_num) spec++;
	     else if (cfbg->container >= 0) continue;
	     if (cfbg->zone == zone_num) spec++;
	     else if (cfbg->zone >= 0) continue;
	     if (cfbg->desk_x == desk_x) spec++;
	     else if (cfbg->desk_x >= 0) continue;
	     if (cfbg->desk_y == desk_y) spec++;
	     else if (cfbg->desk_y >= 0) continue;

	     if (spec <= current_spec) continue;
	     bgfile = cfbg->file;
	     if (bgfile)
	       {
		  if (bgfile[0] != '/')
		    {
		       const char *bf;

		       bf = e_path_find(path_backgrounds, bgfile);
		       if (bf) bgfile = bf;
		    }
	       }
             if (eina_str_has_extension(bgfile, ".edj"))
               {
                  entries = edje_file_collection_list(bgfile);
                  if (entries)
                    {
                       EINA_LIST_FOREACH(entries, ll, entry)
                         {
                            if (!strcmp(entry, "e/desktop/background"))
                              {
                                 bg = cfbg;
                                 current_spec = spec;
                              }
                         }
                       edje_file_collection_list_free(entries);
                    }
               }
             else
               {
                  bg = cfbg;
                  current_spec = spec;
               }
	  }
     }
   return bg;
}

EAPI const char *
e_bg_file_get(int container_num, int zone_num, int desk_x, int desk_y)
{
   const E_Config_Desktop_Background *cfbg;
   Eina_List *l, *entries;
   const char *bgfile = "";
   char *entry;
   int ok = 0;

   cfbg = e_bg_config_get(container_num, zone_num, desk_x, desk_y);

   /* fall back to default */
   if (cfbg)
     {
	bgfile = cfbg->file;
	if (bgfile)
	  {
	     if (bgfile[0] != '/')
	       {
		  const char *bf;

		  bf = e_path_find(path_backgrounds, bgfile);
		  if (bf) bgfile = bf;
	       }
	  }
     }
   else
     {
	bgfile = e_config->desktop_default_background;
	if (bgfile)
	  {
	     if (bgfile[0] != '/')
	       {
		  const char *bf;

		  bf = e_path_find(path_backgrounds, bgfile);
		  if (bf) bgfile = bf;
	       }
	  }
        if (bgfile && eina_str_has_extension(bgfile, ".edj"))
          {
             entries = edje_file_collection_list(bgfile);
             if (entries)
               {
                  EINA_LIST_FOREACH(entries, l, entry)
                    {
                       if (!strcmp(entry, "e/desktop/background"))
                         {
                            ok = 1;
                            break;
                         }
                    }
                  edje_file_collection_list_free(entries);
               }
          }
        else ok = 1;
	if (!ok)
	  bgfile = e_theme_edje_file_get("base/theme/background",
					 "e/desktop/background");
     }

   return bgfile;
}

EAPI void
e_bg_zone_update(E_Zone *zone, E_Bg_Transition transition)
{
   Evas_Object *o;
   const char *bgfile = "";
   const char *trans = "";
   E_Desk *desk;

   if (transition == E_BG_TRANSITION_START) trans = e_config->transition_start;
   else if (transition == E_BG_TRANSITION_DESK) trans = e_config->transition_desk;
   else if (transition == E_BG_TRANSITION_CHANGE) trans = e_config->transition_change;
   if ((!trans) || (!trans[0])) transition = E_BG_TRANSITION_NONE;
   if (e_config->desk_flip_pan_bg) transition = E_BG_TRANSITION_NONE;

   desk = e_desk_current_get(zone);
   if (desk)
     bgfile = e_bg_file_get(zone->container->num, zone->num, desk->x, desk->y);
   else
     bgfile = e_bg_file_get(zone->container->num, zone->num, -1, -1);

   if (zone->bg_object)
     {
	const char *pfile = "";

	edje_object_file_get(zone->bg_object, &pfile, NULL);
	if ((!e_util_strcmp(pfile, bgfile)) && !e_config->desk_flip_pan_bg) return;
     }

   if (transition == E_BG_TRANSITION_NONE)
     {
	if (zone->bg_object)
	  {
	     evas_object_del(zone->bg_object);
	     zone->bg_object = NULL;
	  }
     }
   else
     {
	char buf[4096];

	if (zone->bg_object)
	  {
	     if (zone->prev_bg_object)
	       evas_object_del(zone->prev_bg_object);
	     zone->prev_bg_object = zone->bg_object;
	     if (zone->transition_object)
	       evas_object_del(zone->transition_object);
	     zone->transition_object = NULL;
	     zone->bg_object = NULL;
	  }
	o = edje_object_add(zone->container->bg_evas);
	zone->transition_object = o;
	/* FIXME: segv if zone is deleted while up??? */
	evas_object_data_set(o, "e_zone", zone);
	snprintf(buf, sizeof(buf), "e/transitions/%s", trans);
	e_theme_edje_object_set(o, "base/theme/transitions", buf);
	edje_object_signal_callback_add(o, "e,state,done", "*", _e_bg_signal, zone);
	evas_object_move(o, zone->x, zone->y);
	evas_object_resize(o, zone->w, zone->h);
	evas_object_layer_set(o, -1);
	evas_object_clip_set(o, zone->bg_clip_object);
	evas_object_show(o);
     }
   if (eina_str_has_extension(bgfile, ".edj"))
     {
        o = edje_object_add(zone->container->bg_evas);
        evas_object_data_set(o, "e_zone", zone);
        edje_object_file_set(o, bgfile, "e/desktop/background");
     }
   else
     {
        o = e_icon_add(zone->container->bg_evas);
        evas_object_data_set(o, "e_zone", zone);
        e_icon_file_key_set(o, bgfile, NULL);
        e_icon_fill_inside_set(o, 0);
     }
   zone->bg_object = o;
   if (transition == E_BG_TRANSITION_NONE)
     {
	evas_object_move(o, zone->x, zone->y);
	evas_object_resize(o, zone->w, zone->h);
	evas_object_layer_set(o, -1);
     }
   evas_object_clip_set(o, zone->bg_clip_object);
   evas_object_show(o);
   if (e_config->desk_flip_pan_bg)
     {
	int x = 0, y = 0;

	o = zone->bg_scrollframe;
	if (!o)
	  {
	     o = e_scrollframe_add(zone->container->bg_evas);
	     zone->bg_scrollframe = o;
	     e_scrollframe_custom_theme_set(o, "base/theme/background",
					    "e/desktop/background/scrollframe");
	     e_scrollframe_policy_set(o, E_SCROLLFRAME_POLICY_OFF, E_SCROLLFRAME_POLICY_OFF);
	     e_scrollframe_child_pos_set(o, 0, 0);
	     evas_object_show(o);
	  }
	e_scrollframe_child_set(o, zone->bg_object);
	if (desk)
	  {
	     x = desk->x;
	     y = desk->y;
	  }
	e_bg_zone_slide(zone, x, y);
	return;
     }

   if (transition != E_BG_TRANSITION_NONE)
     {
	edje_extern_object_max_size_set(zone->prev_bg_object, 65536, 65536);
	edje_extern_object_min_size_set(zone->prev_bg_object, 0, 0);
	edje_object_part_swallow(zone->transition_object, "e.swallow.bg.old",
				 zone->prev_bg_object);
	edje_extern_object_max_size_set(zone->bg_object, 65536, 65536);
	edje_extern_object_min_size_set(zone->bg_object, 0, 0);
	edje_object_part_swallow(zone->transition_object, "e.swallow.bg.new",
				 zone->bg_object);
	edje_object_signal_emit(zone->transition_object, "e,action,start", "e");
     }
}

EAPI void
e_bg_zone_slide(E_Zone *zone, int prev_x, int prev_y)
{
   Evas_Object *o;
   E_Desk *desk;
   Evas_Coord w, h, maxw, maxh, step_w, step_h;
   Ecore_Animator *anim;
   E_Bg_Anim_Params *params;
   Evas_Coord vw, vh, px, py;
   int fx, fy;
   const void *data;

   desk = e_desk_current_get(zone);
   edje_object_size_max_get(zone->bg_object, &w, &h);
   maxw = zone->w * zone->desk_x_count;
   maxh = zone->h * zone->desk_y_count;
   if (!w) w = maxw;
   if (!h) h = maxh;
   evas_object_resize(zone->bg_object, w, h);
   if (zone->desk_x_count > 1)
     step_w = ((double) (w - zone->w)) / (zone->desk_x_count - 1);
   else step_w = 0;
   if (zone->desk_y_count > 1)
     step_h = ((double) (h - zone->h)) / (zone->desk_y_count - 1);
   else step_h = 0;

   o = zone->bg_scrollframe;
   evas_object_move(o, zone->x, zone->y);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, zone->bg_clip_object);

   data = edje_object_data_get(zone->bg_object, "directional_freedom");
   e_scrollframe_child_viewport_size_get(o, &vw, &vh);
   e_scrollframe_child_pos_get(o, &px, &py);
   params = evas_object_data_get(zone->bg_object, "switch_animator_params");
   if (!params)
     params = E_NEW(E_Bg_Anim_Params, 1);
   params->zone = zone;
   params->start_x = px;
   params->start_y = py;
   params->end_x = desk->x * step_w * e_config->desk_flip_pan_x_axis_factor;
   params->end_y = desk->y * step_h * e_config->desk_flip_pan_y_axis_factor;
   params->start_time = 0.0;
   if ((data) && (sscanf(data, "%d %d", &fx, &fy) == 2))
     {
	if (fx)
	  {
	     params->freedom.x = EINA_TRUE;
	     params->start_x = prev_x * step_w * e_config->desk_flip_pan_x_axis_factor;
	  }
	if (fy)
	  {
	     params->freedom.y = EINA_TRUE;
	     params->start_y = prev_y * step_h * e_config->desk_flip_pan_y_axis_factor;
	  }
     }

   anim = evas_object_data_get(zone->bg_object, "switch_animator");
   if (anim) ecore_animator_del(anim);
   anim = ecore_animator_add(_e_bg_slide_animator, params);
   evas_object_data_set(zone->bg_object, "switch_animator", anim);
   evas_object_data_set(zone->bg_object, "switch_animator_params", params);
}

EAPI void
e_bg_default_set(const char *file)
{
   E_Event_Bg_Update *ev;
   Eina_Bool changed;

   file = eina_stringshare_add(file);
   changed = file != e_config->desktop_default_background;

   if (!changed)
     {
	eina_stringshare_del(file);
	return;
     }

   if (e_config->desktop_default_background)
     {
	e_filereg_deregister(e_config->desktop_default_background);
	eina_stringshare_del(e_config->desktop_default_background);
     }

   if (file)
     {
	e_filereg_register(file);
	e_config->desktop_default_background = file;
     }
   else
     e_config->desktop_default_background = NULL;

   ev = E_NEW(E_Event_Bg_Update, 1);
   ev->container = -1;
   ev->zone = -1;
   ev->desk_x = -1;
   ev->desk_y = -1;
   ecore_event_add(E_EVENT_BG_UPDATE, ev, _e_bg_event_bg_update_free, NULL);
}

EAPI void
e_bg_add(int container, int zone, int desk_x, int desk_y, const char *file)
{
   const Eina_List *l;
   E_Config_Desktop_Background *cfbg;
   E_Event_Bg_Update *ev;

   file = eina_stringshare_add(file);

   EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cfbg)
     {
	if ((cfbg) &&
	    (cfbg->container == container) &&
	    (cfbg->zone == zone) &&
	    (cfbg->desk_x == desk_x) &&
	    (cfbg->desk_y == desk_y) &&
	    (cfbg->file == file))
	  {
	     eina_stringshare_del(file);
	     return;
	  }
     }

   e_bg_del(container, zone, desk_x, desk_y);
   cfbg = E_NEW(E_Config_Desktop_Background, 1);
   cfbg->container = container;
   cfbg->zone = zone;
   cfbg->desk_x = desk_x;
   cfbg->desk_y = desk_y;
   cfbg->file = file;
   e_config->desktop_backgrounds = eina_list_append(e_config->desktop_backgrounds, cfbg);

   e_filereg_register(cfbg->file);

   ev = E_NEW(E_Event_Bg_Update, 1);
   ev->container = container;
   ev->zone = zone;
   ev->desk_x = desk_x;
   ev->desk_y = desk_y;
   ecore_event_add(E_EVENT_BG_UPDATE, ev, _e_bg_event_bg_update_free, NULL);
}

EAPI void
e_bg_del(int container, int zone, int desk_x, int desk_y)
{
   Eina_List *l;
   E_Config_Desktop_Background *cfbg;
   E_Event_Bg_Update *ev;

   EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cfbg)
     {
	if (!cfbg) continue;
	if ((cfbg->container == container) && (cfbg->zone == zone) &&
	    (cfbg->desk_x == desk_x) && (cfbg->desk_y == desk_y))
	  {
	     e_config->desktop_backgrounds = eina_list_remove_list(e_config->desktop_backgrounds, l);
	     e_filereg_deregister(cfbg->file);
	     if (cfbg->file) eina_stringshare_del(cfbg->file);
	     free(cfbg);
	     break;
	  }
     }

   ev = E_NEW(E_Event_Bg_Update, 1);
   ev->container = container;
   ev->zone = zone;
   ev->desk_x = desk_x;
   ev->desk_y = desk_y;
   ecore_event_add(E_EVENT_BG_UPDATE, ev, _e_bg_event_bg_update_free, NULL);
}

EAPI void
e_bg_update(void)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
	EINA_LIST_FOREACH(man->containers, ll, con)
	  {
	     EINA_LIST_FOREACH(con->zones, lll, zone)
	       {
		  e_zone_bg_reconfigure(zone);
	       }
	  }
     }
}

static inline Eina_Bool
_e_bg_file_edje_check(const char *path)
{
   const char *ext;
   const size_t extlen = sizeof(".edj") - 1;
   size_t len;

   if (!path) return EINA_FALSE;

   len = strlen(path);
   if (len <= extlen) return EINA_FALSE;
   ext = path + len - extlen;
   return memcmp(ext, ".edj", extlen) == 0;
}

/**
 * Go through process of importing an image as E background.
 *
 * This will go through process of importing an image as E
 * background. It will ask fill/tile mode of the image, as well as
 * quality to use.
 *
 * User can cancel operation at any time, and in this case callback is
 * called with @c NULL as second parameter.
 *
 * The operation can be canceled by application/module as well using
 * e_bg_image_import_cancel(). Even in this case the callback is called so user
 * can free possibly allocated data.
 *
 * @param image_file source file to use, must be supported by Evas.
 * @param cb callback to call when import process is done. The first
 *        argument is the given data, while the second is the path to
 *        the imported background file (edje) that can be used with
 *        e_bg_add() or e_bg_default_set(). Note that if @a edje_file
 *        is @c NULL, then the import process was canceled!
 *        This function is @b always called and after it returns
 *        E_Bg_Image_Import_Handle is deleted.
 * @param data pointer to data to be given to callback.
 *
 * @return handle to the import process. It will die automatically
 *         when user cancels the process or when code automatically
 *         calls e_bg_image_import_cancel().  Before dying, callback
 *         will always be called.
 */
EAPI E_Bg_Image_Import_Handle *
e_bg_image_import(const char *image_file, void (*cb)(void *data, const char *edje_file), const void *data)
{
   E_Bg_Image_Import_Handle *handle;

   if (!image_file) return NULL;
   if (!cb) return NULL;

   handle = E_NEW(E_Bg_Image_Import_Handle, 1);
   if (!handle) return NULL;
   handle->cb.func = cb;
   handle->cb.data = (void *)data;

   handle->dia = e_util_image_import_settings_new
     (image_file, _e_bg_image_import_dialog_done, handle);
   if (!handle->dia)
     {
	free(handle);
	return NULL;
     }
   e_dialog_show(handle->dia);

   return handle;
}

/**
 * Cancels previously started import process.
 *
 * Note that his handle will be deleted when process import, so don't
 * call it after your callback is called!
 */
EAPI void
e_bg_image_import_cancel(E_Bg_Image_Import_Handle *handle)
{
   if (!handle) return;

   handle->canceled = EINA_TRUE;

   if (handle->cb.func)
     {
	handle->cb.func(handle->cb.data, NULL);
	handle->cb.func = NULL;
     }
   if (handle->dia)
     {
	e_object_del(E_OBJECT(handle->dia));
	handle->dia = NULL;
     }
   else if (handle->importer)
     {
	e_util_image_import_cancel(handle->importer);
	handle->importer = NULL;
     }
   E_FREE(handle);
}

/**
 * Set background to image, as required in e_fm2_mime_handler_new()
 */
EAPI void
e_bg_handler_set(Evas_Object *obj __UNUSED__, const char *path, void *data __UNUSED__)
{
   if (!path) return;

   if (_e_bg_file_edje_check(path))
     {
        char buf[PATH_MAX];
        int copy = 1;

	E_Container *con = e_container_current_get(e_manager_current_get());
	E_Zone *zone = e_zone_current_get(con);
	E_Desk *desk = e_desk_current_get(zone);

        /* if not in system dir or user dir, copy to user dir */
        e_prefix_data_concat_static(buf, "data/backgrounds");
        if (!strncmp(buf, path, strlen(buf)))
           copy = 0;
        if (copy)
          {
             e_user_dir_concat_static(buf, "backgrounds");
             if (!strncmp(buf, path, strlen(buf)))
                copy = 0;
          }
        if (copy)
          {
             const char *file;
             char *name;

             file = ecore_file_file_get(path);
             name = ecore_file_strip_ext(file);

             e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s-%f.edj", name, ecore_time_unix_get());
             free(name);

             if (!ecore_file_exists(buf))
               {
                  ecore_file_cp(path, buf);
                  e_bg_add(con->num, zone->num, desk->x, desk->y, buf);
               }
             else
                e_bg_add(con->num, zone->num, desk->x, desk->y, path);
          }
        else
           e_bg_add(con->num, zone->num, desk->x, desk->y, path);

        e_bg_update();
        e_config_save_queue();
        return;
     }

   e_bg_image_import(path, _e_bg_handler_image_imported, NULL);
}

/**
 * Test if possible to set background to file, as required in
 * e_fm2_mime_handler_new()
 *
 * This handler tests for files that would be acceptable for setting
 * background.
 *
 * You should just register it with "*.edj" (glob matching extension)
 * or "image/" (mimetypes)that are acceptable with Evas loaders.
 *
 * Just edje files with "e/desktop/background" group are used.
 */
EAPI int
e_bg_handler_test(Evas_Object *obj __UNUSED__, const char *path, void *data __UNUSED__)
{

   if (!path) return 0;

   if (_e_bg_file_edje_check(path))
     {
	if (edje_file_group_exists(path, "e/desktop/background")) return 1;
	return 0;
     }

   /* it's image/png or image/jpeg, we'll import it. */
   return 1;
}

/* local subsystem functions */
static void
_e_bg_signal(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Zone *zone = data;

   if (zone->prev_bg_object)
     {
	evas_object_del(zone->prev_bg_object);
	zone->prev_bg_object = NULL;
     }
   if (zone->transition_object)
     {
	evas_object_del(zone->transition_object);
	zone->transition_object = NULL;
     }
   evas_object_move(zone->bg_object, zone->x, zone->y);
   evas_object_resize(zone->bg_object, zone->w, zone->h);
   evas_object_layer_set(zone->bg_object, -1);
   evas_object_clip_set(zone->bg_object, zone->bg_clip_object);
   evas_object_show(zone->bg_object);
}

static void
_e_bg_event_bg_update_free(void *data __UNUSED__, void *event)
{
   free(event);
}

static Eina_Bool
_e_bg_slide_animator(void *data)
{
   E_Bg_Anim_Params *params;
   E_Zone *zone;
   Evas_Object *o;
   double st;
   double t, dt, spd;
   Evas_Coord px, py, rx, ry, bw, bh, panw, panh;
   Edje_Message_Int_Set *msg;

   params = data;
   zone = params->zone;
   t = ecore_loop_time_get();
   spd = e_config->desk_flip_animate_time;

   o = zone->bg_scrollframe;
   if (!params->start_time)
     st = params->start_time = t;
   else
     st = params->start_time;

   dt = (t - st) / spd;
   if (dt > 1.0) dt = 1.0;
   dt = 1.0 - dt;
   dt *= dt; /* decelerate - could be a better hack */

   if (params->end_x > params->start_x)
     rx = params->start_x + (params->end_x - params->start_x) * (1.0 - dt);
   else
     rx = params->end_x + (params->start_x - params->end_x) * dt;
   if (params->freedom.x) px = zone->x;
   else px = rx;

   if (params->end_y > params->start_y)
     ry = params->start_y + (params->end_y - params->start_y) * (1.0 - dt);
   else
     ry = params->end_y + (params->start_y - params->end_y) * dt;
   if (params->freedom.y) py = zone->y;
   else py = ry;

   e_scrollframe_child_pos_set(o, px, py);

   evas_object_geometry_get(zone->bg_object, NULL, NULL, &bw, &bh);
   panw = bw - zone->w;
   if (panw < 0) panw = 0;
   panh = bh - zone->h;
   if (panh < 0) panh = 0;
   msg = alloca(sizeof(Edje_Message_Int_Set) + (5 * sizeof(int)));
   msg->count = 6;
   msg->val[0] = rx;
   msg->val[1] = ry;
   msg->val[2] = panw;
   msg->val[3] = panh;
   msg->val[4] = bw;
   msg->val[5] = bh;
   edje_object_message_send(zone->bg_object, EDJE_MESSAGE_INT_SET, 0, msg);

   if (dt <= 0.0)
     {
	evas_object_data_del(zone->bg_object, "switch_animator");
	evas_object_data_del(zone->bg_object, "switch_animator_params");
	E_FREE(params);
	return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_bg_image_import_dialog_done(void *data, const char *path, Eina_Bool ok, Eina_Bool external, int quality, E_Image_Import_Mode mode)
{
   E_Bg_Image_Import_Handle *handle = data;
   const char *file;
   char *name;
   char buf[PATH_MAX];
   size_t used, off;
   unsigned num;

   if (!ok) goto aborted;

   file = ecore_file_file_get(path);
   if (!file) goto aborted;
   name = ecore_file_strip_ext(file);
   if (!name) goto aborted;

   used = e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s.edj", name);
   free(name);
   if (used >= sizeof(buf)) goto aborted;

   off = used - (sizeof(".edj") - 1);

   for (num = 0; ecore_file_exists(buf); num++)
     snprintf(buf + off, sizeof(buf) - off, "-%u.edj", num);

   handle->importer = e_util_image_import
     (path, buf, "e/desktop/background", external, quality, mode,
      _e_bg_image_import_done, handle);
   if (!handle->importer) goto aborted;

   return;

 aborted:
   if (handle->cb.func)
     {
	handle->cb.func(handle->cb.data, NULL);
	handle->cb.func = NULL;
     }
   if (!handle->canceled) E_FREE(handle);
}

static void
_e_bg_image_import_done(void *data, Eina_Bool ok, const char *image_path __UNUSED__, const char *edje_path)
{
   E_Bg_Image_Import_Handle *handle = data;

   if (!ok) edje_path = NULL;

   if (handle->cb.func)
     {
	handle->cb.func(handle->cb.data, edje_path);
	handle->cb.func = NULL;
     }

   if (!handle->canceled) E_FREE(handle);
}

static void
_e_bg_handler_image_imported(void *data __UNUSED__, const char *image_path)
{
   E_Container *con = e_container_current_get(e_manager_current_get());
   E_Zone *zone = e_zone_current_get(con);
   E_Desk *desk = e_desk_current_get(zone);

   if (!image_path) return;

   e_bg_add(con->num, zone->num, desk->x, desk->y, image_path);
   e_bg_update();
   e_config_save_queue();
}
