/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Smart_Data       E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;

   Evas            *evas;
   Evas_Object     *obj;

   char            *thumb_path;
   
   Evas_Object     *event_object;
   Evas_Object     *icon_object;
   Evas_Object     *image_object;
   Evas_Object     *entry_object;

   E_Fm_File       *file;

   unsigned char    visible : 1;
};

/* local subsystem functions */
static void _e_fm_icon_smart_add         (Evas_Object *obj);
static void _e_fm_icon_smart_del         (Evas_Object *obj);
static void _e_fm_icon_smart_layer_set   (Evas_Object *obj, int l);
static void _e_fm_icon_smart_raise       (Evas_Object *obj);
static void _e_fm_icon_smart_lower       (Evas_Object *obj);
static void _e_fm_icon_smart_stack_above (Evas_Object *obj, Evas_Object *above);
static void _e_fm_icon_smart_stack_below (Evas_Object *obj, Evas_Object *below);
static void _e_fm_icon_smart_move        (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_fm_icon_smart_resize      (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_fm_icon_smart_show        (Evas_Object *obj);
static void _e_fm_icon_smart_hide        (Evas_Object *obj);
static void _e_fm_icon_smart_clip_set    (Evas_Object *obj, Evas_Object *clip);
static void _e_fm_icon_smart_clip_unset  (Evas_Object *obj);

/* Create icons */
static void  _e_fm_icon_icon_mime_get(E_Smart_Data *sd);

static void  _e_fm_icon_thumb_generate(void);
static int   _e_fm_icon_thumb_cb_exe_exit(void *data, int type, void *event);

/* local subsystem globals */
static Evas_Smart *e_smart = NULL;

static pid_t       pid = -1;
static Evas_List  *thumb_files = NULL;

static Evas_List  *event_handlers = NULL;

/* externally accessible functions */
int
e_fm_icon_init(void)
{
   char *homedir;
   char  path[PATH_MAX];

   event_handlers = evas_list_append(event_handlers,
                                     ecore_event_handler_add(ECORE_EVENT_EXE_EXIT,
                                                             _e_fm_icon_thumb_cb_exe_exit,
                                                              NULL));
   return 1;
}

int
e_fm_icon_shutdown(void)
{
   while (event_handlers)
     {
	ecore_event_handler_del(event_handlers->data);
	event_handlers = evas_list_remove_list(event_handlers, event_handlers);
     }
   evas_list_free(thumb_files);
   evas_smart_free(e_smart);
   return 1;
}

Evas_Object *
e_fm_icon_add(Evas *evas)
{
   Evas_Object *e_fm_icon_smart;

   if (!e_smart)
     {
	e_smart = evas_smart_new("e_fm_icon_smart",
	                         _e_fm_icon_smart_add,
	                         _e_fm_icon_smart_del,
	                         _e_fm_icon_smart_layer_set,
	                         _e_fm_icon_smart_raise,
	                         _e_fm_icon_smart_lower,
	                         _e_fm_icon_smart_stack_above,
	                         _e_fm_icon_smart_stack_below,
	                         _e_fm_icon_smart_move,
	                         _e_fm_icon_smart_resize,
	                         _e_fm_icon_smart_show,
	                         _e_fm_icon_smart_hide,
	                         NULL,
	                         _e_fm_icon_smart_clip_set,
	                         _e_fm_icon_smart_clip_unset,
	                         NULL);
     }

   e_fm_icon_smart = evas_object_smart_add(evas, e_smart);

   return e_fm_icon_smart;
}

void
e_fm_icon_file_set(Evas_Object *obj, E_Fm_File *file)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   e_object_ref(E_OBJECT(file));
   sd->file = file;
   file->icon_object = obj;
}

void
e_fm_icon_title_set(Evas_Object *obj, const char *title)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->icon_object) edje_object_part_text_set(sd->icon_object, "icon_title", title);
}

void
e_fm_icon_edit_entry_set(Evas_Object *obj, Evas_Object *entry)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (entry)
     {
	sd->entry_object = entry;
	edje_object_part_swallow(sd->icon_object, "icon_title_edit_swallow", sd->entry_object);
     }
   else
     {
	edje_object_part_unswallow(sd->icon_object, sd->entry_object);
	sd->entry_object = NULL;
     }
}

void
e_fm_icon_signal_emit(Evas_Object *obj, const char *source, const char *emission)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->icon_object) edje_object_signal_emit(sd->icon_object, source, emission);
   if (sd->image_object) edje_object_signal_emit(sd->image_object, source, emission);
}

int
e_fm_icon_assoc_set(Evas_Object *obj, const char *assoc)
{
   /* TODO
    * Store the associated exe in a cfg
    */
   return 0;
}

/* local subsystem functions */
static void
_e_fm_icon_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = E_NEW(E_Smart_Data, 1);
   if (!sd) return;

   sd->evas = evas_object_evas_get(obj);
   sd->obj = obj;

   sd->event_object = evas_object_rectangle_add(sd->evas);
   evas_object_color_set(sd->event_object, 0, 0, 0, 0);
   evas_object_smart_member_add(sd->event_object, obj);

   evas_object_smart_data_set(obj, sd);
}


static void
_e_fm_icon_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->entry_object) edje_object_part_unswallow(sd->icon_object, sd->entry_object);
   if (sd->event_object)
     {
	evas_object_smart_member_del(sd->event_object);
       	evas_object_del(sd->event_object);
     }
   if (sd->icon_object)
     {
	evas_object_smart_member_del(sd->icon_object);
       	evas_object_del(sd->icon_object);
     }
   if (sd->image_object)
     {
	evas_object_smart_member_del(sd->image_object);
	evas_object_del(sd->image_object);
     }
   if (sd->file) e_object_unref(E_OBJECT(sd->file));
   free(sd);
}

static void
_e_fm_icon_smart_layer_set(Evas_Object *obj, int l)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_layer_set(sd->event_object, l);
   if (sd->icon_object) evas_object_layer_set(sd->icon_object, l);
   if (sd->image_object) evas_object_layer_set(sd->image_object, l);
}

static void
_e_fm_icon_smart_raise(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_raise(sd->event_object);
   if (sd->icon_object) evas_object_stack_below(sd->icon_object, sd->event_object);
   if (sd->image_object) evas_object_stack_below(sd->image_object, sd->event_object);
}

static void
_e_fm_icon_smart_lower(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_lower(sd->event_object);
   if (sd->icon_object) evas_object_stack_below(sd->icon_object, sd->event_object);
   if (sd->image_object) evas_object_stack_below(sd->image_object, sd->event_object);
}

static void
_e_fm_icon_smart_stack_above(Evas_Object *obj, Evas_Object *above)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_stack_above(sd->event_object, above);
   if (sd->icon_object) evas_object_stack_below(sd->icon_object, sd->event_object);
   if (sd->image_object) evas_object_stack_below(sd->image_object, sd->event_object);
}

static void
_e_fm_icon_smart_stack_below(Evas_Object *obj, Evas_Object *below)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_stack_below(sd->event_object, below);
   if (sd->icon_object) evas_object_stack_below(sd->icon_object, sd->event_object);
   if (sd->image_object) evas_object_stack_below(sd->image_object, sd->event_object);
}

static void
_e_fm_icon_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   
   if(sd->x == x && sd->y == y) return;
   
   evas_object_move(sd->event_object, x, y);
   if (sd->icon_object) evas_object_move(sd->icon_object, x, y);
   //if (sd->image_object) evas_object_move(sd->image_object, x, y);
   sd->x = x;
   sd->y = y;
}

static void
_e_fm_icon_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if(sd->w == w && sd->h == h) return;   
   
   evas_object_resize(sd->event_object, w, h);
   if (sd->icon_object) evas_object_resize(sd->icon_object, w, h);
   sd->w = w;
   sd->h = h;
   //if (sd->image_object) evas_object_resize(sd->image_object, w, h);
}

static void
_e_fm_icon_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if ((!sd) && (!sd->file)) return;
   if (sd->visible) return;

   evas_object_show(sd->event_object);
   sd->visible = 1;
   sd->icon_object = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->icon_object, "base/theme/fileman",
                           "fileman/icon");
   evas_object_smart_member_add(sd->icon_object, obj);

   if (e_fm_file_can_preview(sd->file))
     {
	sd->thumb_path = e_thumb_file_get(sd->file->path);
	if (e_thumb_exists(sd->file->path))
	  sd->image_object = e_thumb_evas_object_get(sd->file->path,
						     sd->evas,
						     sd->w,
						     sd->h);
	else
	  {
	     thumb_files = evas_list_append(thumb_files, sd);
	     if (pid == -1) _e_fm_icon_thumb_generate();
	     _e_fm_icon_icon_mime_get(sd);
	  }
     }
   else
     {
	_e_fm_icon_icon_mime_get(sd);
     }

   if (sd->image_object)
     {
	edje_object_part_swallow(sd->icon_object, "icon_swallow", sd->image_object);
	evas_object_smart_member_add(sd->image_object, obj);
     }
   edje_object_part_text_set(sd->icon_object, "icon_title", sd->file->name);
   
   evas_object_show(sd->icon_object);
   
    {
       Evas_Coord icon_w, icon_h;
       edje_object_size_min_calc(sd->icon_object, &icon_w, &icon_h);
       evas_object_resize(sd->icon_object, icon_w, icon_h);
       evas_object_resize(sd->event_object, icon_w, icon_h);
       evas_object_resize(sd->obj, icon_w, icon_h);       
    }       
}

static void
_e_fm_icon_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->visible) return;

   evas_object_hide(sd->event_object);
   sd->visible = 0;
   if (sd->icon_object)
     {
	evas_object_smart_member_del(sd->icon_object);
       	evas_object_del(sd->icon_object);
     }
   sd->icon_object = NULL;
   if (sd->image_object)
     {
	evas_object_smart_member_del(sd->image_object);
       	evas_object_del(sd->image_object);
     }
   sd->image_object = NULL;
   if (sd->thumb_path) free(sd->thumb_path);
   sd->thumb_path = NULL;
}

static void
_e_fm_icon_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_clip_set(sd->event_object, clip);
}

static void
_e_fm_icon_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_clip_unset(sd->event_object);
}

static void
_e_fm_icon_icon_mime_get(E_Smart_Data *sd)
{
   sd->image_object = edje_object_add(sd->evas);
   
   if (sd->file->type ==  E_FM_FILE_TYPE_DIRECTORY)
     {
	e_theme_edje_object_set(sd->image_object, "base/theme/fileman",
	                        "fileman/icons/folder");
     }
   else
     {
	char *ext;

	ext = strrchr(sd->file->name, '.');
	if (ext)
	  {
	     char part[PATH_MAX];
	     char *ext2;
	     
	     ext = strdup(ext);
	     ext2 = ext;
	     for(;*ext2;ext2++)
	       *ext2 = (unsigned char)tolower((unsigned char)*ext2);
	     
	     snprintf(part, PATH_MAX, "fileman/icons/%s", (ext + 1));
	     
	     if(!e_theme_edje_object_set(sd->image_object, "base/theme/fileman", part))
	       e_theme_edje_object_set(sd->image_object, "base/theme/fileman", "fileman/icons/file");
	     
	     free(ext);		     	     
	  }
	else
	  e_theme_edje_object_set(sd->image_object, "base/theme/fileman", "fileman/icons/file");
     }
}

static void
_e_fm_icon_thumb_generate(void)
{
   E_Smart_Data *sd;
   
   if ((!thumb_files) || (pid != -1)) return;

   pid = fork();
   if (pid == 0)
     {
	/* reset signal handlers for the child */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	
	sd = thumb_files->data;
	if (!e_thumb_exists(sd->file->path))
	  e_thumb_create(sd->file->path, 48, 48); // thumbnail size
	exit(0);
     }
}

static int
_e_fm_icon_thumb_cb_exe_exit(void *data, int type, void *event)
{
   Ecore_Event_Exe_Exit *ev;
   E_Smart_Data         *sd;
   char                 *ext;
   
   ev = event;
   if (ev->pid != pid) return 1;
   if (!thumb_files) return 1;
   
   sd = thumb_files->data;
   thumb_files = evas_list_remove_list(thumb_files, thumb_files);
   
   ext = strrchr(sd->file->name, ".");
   if(ext)
     if(strcasecmp(ext, ".eap"))
       ext = NULL;
   
   if (ecore_file_exists(sd->thumb_path) || ext)
     {
	if (sd->image_object) evas_object_del(sd->image_object);
	sd->image_object = e_thumb_evas_object_get(sd->file->path,
						   sd->evas,
						   sd->w, sd->h);
	edje_object_part_swallow(sd->icon_object, "icon_swallow",
				 sd->image_object);
     }

   pid = -1;
   _e_fm_icon_thumb_generate();
   return 1;
}
