/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* IGNORE this code for now! */

typedef struct _E_Fm2_Smart_Data E_Fm2_Smart_Data;
typedef struct _E_Fm2_Region     E_Fm2_Region;
typedef struct _E_Fm2_Icon       E_Fm2_Icon;

struct _E_Fm2_Smart_Data
{
   Evas_Coord        x, y, w, h;
   Evas_Object      *obj;
   Evas_Object      *clip;
   char             *dev;
   char             *path;
   char             *realpath;
   E_Fm2_View_Mode   view_mode;
   struct {
      Evas_Coord     w, h;
   } max;
   struct {
      Evas_Coord     x, y;
   } pos;
   struct {
      Evas_List     *list;
      int            member_max;
   } regions;
   Evas_List        *icons;
   Evas_List        *queue;
   Ecore_Idler      *scan_idler;
   Ecore_Timer      *scan_timer;
   Ecore_Job        *scroll_job;
   Ecore_Job        *resize_job;
   DIR              *dir;
   unsigned char     no_case_sort : 1;
//   unsigned char     no_dnd : 1;
//   unsigned char     single_select : 1;
//   unsigned char     single_click : 1;
//   unsigned char     show_ext : 1;
};

struct _E_Fm2_Region
{
   E_Fm2_Smart_Data *sd;
   Evas_Coord        x, y, w, h;
   Evas_List        *list;
   unsigned char     realized : 1;
};

struct _E_Fm2_Icon
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Region     *region;
   Evas_Coord        x, y, w, h;
   Evas_Object      *obj;
   char             *file;
   char             *mime;
   unsigned char     realized : 1;
   unsigned char     selected : 1;
   unsigned char     thumb : 1;
//   unsigned char     single_click : 1;
};

static char *_e_fm2_dev_path_map(char *dev, char *path);
static void _e_fm2_file_add(Evas_Object *obj, char *file);
static void _e_fm2_file_del(Evas_Object *obj, char *file);
static void _e_fm2_scan_start(Evas_Object *obj);
static void _e_fm2_scan_stop(Evas_Object *obj);
static void _e_fm2_queue_process(Evas_Object *obj); 
static void _e_fm2_queue_free(Evas_Object *obj);
static void _e_fm2_regions_free(Evas_Object *obj);
static void _e_fm2_regions_populate(Evas_Object *obj);
static void _e_fm2_icons_place(Evas_Object *obj);
static void _e_fm2_icons_free(Evas_Object *obj);
static void _e_fm2_regions_eval(Evas_Object *obj);

static E_Fm2_Icon *_e_fm2_icon_new(E_Fm2_Smart_Data *sd, char *file);
static void _e_fm2_icon_free(E_Fm2_Icon *ic);
static void _e_fm2_icon_realize(E_Fm2_Icon *ic);
static void _e_fm2_icon_unrealize(E_Fm2_Icon *ic);
static int _e_fm2_icon_visible(E_Fm2_Icon *ic);

static E_Fm2_Region *_e_fm2_region_new(E_Fm2_Smart_Data *sd);
static void _e_fm2_region_free(E_Fm2_Region *rg);
static void _e_fm2_region_realize(E_Fm2_Region *rg);
static void _e_fm2_region_unrealize(E_Fm2_Region *rg);
static int _e_fm2_region_visible(E_Fm2_Region *rg);

static void _e_fm2_cb_scroll_job(void *data);
static void _e_fm2_cb_resize_job(void *data);
static int _e_fm2_cb_icon_sort(void *data1, void *data2);
static int _e_fm2_cb_scan_idler(void *data);
static int _e_fm2_cb_scan_timer(void *data);

static void _e_fm2_obj_icons_place(E_Fm2_Smart_Data *sd);

static void _e_fm2_smart_add(Evas_Object *object);
static void _e_fm2_smart_del(Evas_Object *object);
static void _e_fm2_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_fm2_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_fm2_smart_show(Evas_Object *object);
static void _e_fm2_smart_hide(Evas_Object *object);
static void _e_fm2_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_fm2_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _e_fm2_smart_clip_unset(Evas_Object *obj);

static char *_meta_path = NULL;
static Evas_Smart *_e_fm2_smart = NULL;

/* externally accessible functions */
EAPI int
e_fm2_init(void)
{
   char *homedir;
   char  path[PATH_MAX];

   homedir = e_user_homedir_get();
   if (homedir)
     {
	snprintf(path, sizeof(path), "%s/.e/e/fileman/metadata", homedir);
	ecore_file_mkpath(path);
	_meta_path = strdup(path);
	free(homedir);
     }
   else return 0;

   _e_fm2_smart = evas_smart_new("e_fm",
				 _e_fm2_smart_add, /* add */
				 _e_fm2_smart_del, /* del */
				 NULL, NULL, NULL, NULL, NULL,
				 _e_fm2_smart_move, /* move */
				 _e_fm2_smart_resize, /* resize */
				 _e_fm2_smart_show,/* show */
				 _e_fm2_smart_hide,/* hide */
				 _e_fm2_smart_color_set, /* color_set */
				 _e_fm2_smart_clip_set, /* clip_set */
				 _e_fm2_smart_clip_unset, /* clip_unset */
				 NULL); /* data*/
   return 1;
}

EAPI int
e_fm2_shutdown(void)
{
   evas_smart_free(_e_fm2_smart);
   _e_fm2_smart = NULL;
   E_FREE(_meta_path);
   return 1;
}

EAPI Evas_Object *
e_fm2_add(Evas *evas)
{
   return evas_object_smart_add(evas, _e_fm2_smart);
}

EAPI void
e_fm2_path_set(Evas_Object *obj, char *dev, char *path)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety

   /* we split up files into regions. each region can have a maximum of 128 
    * icons and we realize/unrealize whole regions at once when that region 
    * becomes visible - this saves of object  count and memory */
   sd->regions.member_max = 128;
   
   _e_fm2_scan_stop(obj);
   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   E_FREE(sd->dev);
   E_FREE(sd->path);
   E_FREE(sd->realpath);
   if (dev) sd->dev = strdup(dev);
   sd->path = strdup(path);
   sd->realpath = _e_fm2_dev_path_map(sd->dev, sd->path);
   // FIXME: begin dir scan/build
   printf("FM: %s\n", sd->realpath);
   _e_fm2_scan_start(obj);
}

EAPI void
e_fm2_path_get(Evas_Object *obj, char **dev, char **path)
{
   E_Fm2_Smart_Data *sd;

   if (dev) *dev = NULL;
   if (path) *path = NULL;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (dev) *dev = sd->dev;
   if (path) *path = sd->path;
}

EAPI void
e_fm2_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (x > (sd->max.w - sd->w)) x = sd->max.w - sd->w;
   if (x < 0) x = 0;
   if (y > (sd->max.h - sd->h)) y = sd->max.h - sd->h;
   if (y < 0) y = 0;
   if ((sd->pos.x == x) && (sd->pos.y == y)) return;
   sd->pos.x = x;
   sd->pos.y = y;
   printf("POS %i %i\n", x, y);
   if (sd->scroll_job) ecore_job_del(sd->scroll_job);
   sd->scroll_job = ecore_job_add(_e_fm2_cb_scroll_job, obj);
}

EAPI void
e_fm2_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (x) *x = sd->pos.x;
   if (y) *y = sd->pos.y;
}

EAPI void
e_fm2_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Fm2_Smart_Data *sd;
   Evas_Coord mx, my;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   mx = sd->max.w - sd->w;
   if (mx < 0) mx = 0;
   my = sd->max.h - sd->h;
   if (my < 0) my = 0;
   if (x) *x = mx;
   if (y) *y = my;
}

EAPI void
e_fm2_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (w) *w = sd->max.w;
   if (h) *h = sd->max.h;
}




/* local subsystem functions */
static char *
_e_fm2_dev_path_map(char *dev, char *path)
{
   char buf[4096] = "", *s;
   int len;
   
   /* map a device name to a mount point/path on the os (and append path) */
   if (!dev) return strdup(path);

   /* FIXME: load mappings from config and use them first - maybe device
    * discovery should be done through config and not the below (except
    * maybe for home directory and root fs and other simple thngs */
   /* FIXME: also add other virtualized dirs like "backgrounds", "themes",
    * "favorites" */
#define CMP(x) (e_util_glob_case_match(dev, x))   
#define PRT(args...) snprintf(buf, sizeof(buf), ##args)
   
   if      (CMP("/")) {
      PRT("%s", path);
   }
   else if (CMP("~/")) {
      s = e_user_homedir_get();
      PRT("%s%s", s, path);
      free(s);
   }
   else if (CMP("dvd") || CMP("dvd-*"))  {
      /* FIXME: find dvd mountpoint optionally for dvd no. X */
      /* maybe make part of the device mappings config? */
   }
   else if (CMP("cd") || CMP("cd-*") || CMP("cdrom") || CMP("cdrom-*") ||
	    CMP("dvd") || CMP("dvd-*")) {
      /* FIXME: find cdrom or dvd mountpoint optionally for cd/dvd no. X */
      /* maybe make part of the device mappings config? */
   }
   /* FIXME: add code to find USB devices (multi-card readers or single,
    * usb thumb drives, other usb storage devices (usb hd's etc.)
    */
   /* maybe make part of the device mappings config? */
   /* FIXME: add code for finding nfs shares, smb shares etc. */
   /* maybe make part of the device mappings config? */
   
   len = strlen(buf);
   while ((len > 1) && (buf[len - 1] == '/'))
     {
	buf[len - 1] = 0;
	len--;
     }
   return strdup(buf);
}

static void
_e_fm2_file_add(Evas_Object *obj, char *file)
{
   E_Fm2_Smart_Data *sd;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* create icon obj and append to unsorted list */
   ic = _e_fm2_icon_new(sd, file);
   sd->queue = evas_list_append(sd->queue, ic);
}

static void
_e_fm2_file_del(Evas_Object *obj, char *file)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* find icon of file and remove from unsorted or main list */
   /* FIXME: find and remove */
}

static void
_e_fm2_scan_start(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* start scanning the dir in an idler and putting found items on the */
   /* queue as icons - waiting for sorting and region insertion by a */
   /* timer that ticks off taking queued icons and putting them in a */
   /* sorted position in the icon list */
   /* this system is great - it completely does the dir scanning in idle
    * time when e has nothing better to do - BUt it will mean that the dir
    * will first draw with an empty view then slowly fill up as the scan
    * happens - this means e remains interactive, but could mean for more
    * redraws that we want. what we want to do is maybe bring some of the
    * scan forward to here for a short period of time so when the window
    * is drawn - its drawn with contents ... if they didnt take too long
    * to fill
    */
   /* if i add the above pre-scan and it doesnt finish - continue here */
   sd->scan_idler = ecore_idler_add(_e_fm2_cb_scan_idler, obj);
   sd->scan_timer = ecore_timer_add(0.2, _e_fm2_cb_scan_timer, obj);
}

static void
_e_fm2_scan_stop(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* stop the scan idler, the sort timer and free the queue */
   if (sd->dir)
     {
	closedir(sd->dir);
	sd->dir = NULL;
     }
   if (sd->scan_idler)
     {
	ecore_idler_del(sd->scan_idler);
	sd->scan_idler = NULL;
     }
   if (sd->scan_timer)
     {
	ecore_timer_del(sd->scan_timer);
	sd->scan_timer = NULL;
     }
   _e_fm2_queue_free(obj);
}

static void
_e_fm2_queue_process(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take unsorted and insert into the icon list - reprocess regions */
   while (sd->queue)
     {
	E_Fm2_Icon *ic;

	ic = sd->queue->data;
	sd->icons = evas_list_append(sd->icons, ic);
	/* insertion sort - better than qsort for the way we are doing
	 * things - incrimentally scan and sort as we go */
	sd->queue = evas_list_remove_list(sd->queue, sd->queue);
     }
   sd->icons = evas_list_sort(sd->icons, evas_list_count(sd->icons), _e_fm2_cb_icon_sort);
   if (sd->resize_job) ecore_job_del(sd->resize_job);
   sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, obj);
}

static void
_e_fm2_queue_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* just free the icons in the queue  and the queue itself */
   while (sd->queue)
     {
	_e_fm2_icon_free(sd->queue->data);
	sd->queue = evas_list_remove_list(sd->queue, sd->queue);
     }
}

static void
_e_fm2_regions_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* free up all regions */
   while (sd->regions.list)
     {
	_e_fm2_region_free(sd->regions.list->data);
        sd->regions.list = evas_list_remove_list(sd->regions.list, sd->regions.list);
     }
}

static void
_e_fm2_regions_populate(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Region *rg;
   E_Fm2_Icon *ic;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take the icon list and split into regions */
   rg = NULL;
   evas_event_freeze(evas_object_evas_get(obj));
   edje_freeze();
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if (!rg)
	  {
	     rg = _e_fm2_region_new(sd);
	     sd->regions.list = evas_list_append(sd->regions.list, rg);
	  }
	ic->region = rg;
	rg->list = evas_list_append(rg->list, ic);
	if (rg->w == 0)
	  {
	     rg->x = ic->x;
	     rg->y = ic->y;
	     rg->w = ic->w;
	     rg->h = ic->h;
	  }
	else
	  {
	     if (ic->x < rg->x)
	       {
		  rg->w += rg->x - ic->x;
		  rg->x = ic->x;
	       }
	     if ((ic->x + ic->w) > (rg->x + rg->w))
	       {
		  rg->w += (ic->x + ic->w) - (rg->x + rg->w);
	       }
	     if (ic->y < rg->y)
	       {
		  rg->h += rg->y - ic->y;
		  rg->y = ic->y;
	       }
	     if ((ic->y + ic->h) > (rg->y + rg->h))
	       {
		  rg->h += (ic->y + ic->h) - (rg->y + rg->h);
	       }
	  }
	if (evas_list_count(rg->list) > sd->regions.member_max)
	  rg = NULL;
     }
   _e_fm2_regions_eval(obj);
   for (l = sd->icons; l; l = l->next)
     {
        ic = l->data;
	if ((!ic->region->realized) && (ic->realized))
	  _e_fm2_icon_unrealize(ic);
     }
   printf("pop\n");
   _e_fm2_obj_icons_place(sd);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(obj));
}

static void
_e_fm2_icons_place(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Icon *ic;
   Evas_Coord x, y, rh;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take the icon list and find a location for them */
   x = 0; y = 0;
   rh = 0;
   sd->max.w = 0;
   sd->max.h = 0;
   for (l = sd->icons; l; l = l->next)
     {
	ic = l->data;
	if ((x > 0) && ((x + ic->w) > sd->w))
	  {
	     x = 0;
	     y += rh;
	     rh = 0;
	  }
	ic->x = x;
	ic->y = y;
	x += ic->w;
	if (ic->h > rh) rh = ic->h;
	if ((ic->x + ic->w) > sd->max.w) sd->max.w = ic->x + ic->w;
	if ((ic->y + ic->h) > sd->max.h) sd->max.h = ic->y + ic->h;
     }
   evas_object_smart_callback_call(sd->obj, "changed", NULL);
}

static void
_e_fm2_icons_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   _e_fm2_queue_free(obj);
   /* free all icons */
   while (sd->icons)
     {
	_e_fm2_icon_free(sd->icons->data);
        sd->icons = evas_list_remove_list(sd->icons, sd->icons);
     }
}

static void
_e_fm2_regions_eval(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;
   Evas_List *l;
   E_Fm2_Region *rg;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_event_freeze(evas_object_evas_get(obj));
   edje_freeze();
   for (l = sd->regions.list; l; l = l->next)
     {
	rg = l->data;
	
	if (_e_fm2_region_visible(rg))
	  _e_fm2_region_realize(rg);
	else
	  _e_fm2_region_unrealize(rg);
     }
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(obj));
}

/**************************/

static E_Fm2_Icon *
_e_fm2_icon_new(E_Fm2_Smart_Data *sd, char *file)
{
   E_Fm2_Icon *ic;
   
   /* create icon */
   ic = E_NEW(E_Fm2_Icon, 1);
   ic->sd = sd;
   ic->file = strdup(file);
   ic->w = 64;
   ic->h = 64;
   printf("FM: IC+ %s\n", ic->file);
   return ic;
}

static void
_e_fm2_icon_free(E_Fm2_Icon *ic)
{
   /* free icon, object data etc. etc. */
   _e_fm2_icon_unrealize(ic);
   E_FREE(ic->file);
   free(ic);
}

static void
_e_fm2_icon_realize(E_Fm2_Icon *ic)
{
   if (ic->realized) return;
   /* actually create evas objects etc. */
   ic->realized = 1;
   evas_event_freeze(evas_object_evas_get(ic->sd->obj));
   ic->obj = edje_object_add(evas_object_evas_get(ic->sd->obj));
   edje_object_freeze(ic->obj);
   evas_object_smart_member_add(ic->obj, ic->sd->obj);
   e_theme_edje_object_set(ic->obj, "base/theme/fileman",
			   "fileman/icon_normal");
   evas_object_clip_set(ic->obj, ic->sd->clip);
   evas_object_move(ic->obj,
		    ic->sd->x + ic->x - ic->sd->pos.x,
		    ic->sd->y + ic->y - ic->sd->pos.y);
   evas_object_resize(ic->obj, ic->w, ic->h);
   edje_object_part_text_set(ic->obj, "icon_title", ic->file);
   edje_object_thaw(ic->obj);
   evas_event_thaw(evas_object_evas_get(ic->sd->obj));
   evas_object_show(ic->obj);
}

static void
_e_fm2_icon_unrealize(E_Fm2_Icon *ic)
{
   if (!ic->realized) return;
   /* delete evas objects */
   ic->realized = 0;
   evas_object_del(ic->obj);
   ic->obj = NULL;
}

static int
_e_fm2_icon_visible(E_Fm2_Icon *ic)
{
   /* return if the icon is visible */
   if (
       ((ic->x - ic->sd->pos.x) < (ic->sd->w)) &&
       ((ic->x + ic->w - ic->sd->pos.x) > 0) &&
       ((ic->y - ic->sd->pos.y) < (ic->sd->h)) &&
       ((ic->y + ic->h - ic->sd->pos.y) > 0)
       )
     return 1;
   return 0;
}

/**************************/
static E_Fm2_Region *
_e_fm2_region_new(E_Fm2_Smart_Data *sd)
{
   E_Fm2_Region *rg;
   
   rg = E_NEW(E_Fm2_Region, 1);
   rg->sd = sd;
   return rg;
}

static void
_e_fm2_region_free(E_Fm2_Region *rg)
{
   E_Fm2_Icon *ic;
   
   while (rg->list)
     {
	ic = rg->list->data;
	ic->region = NULL;
	rg->list = evas_list_remove_list(rg->list, rg->list);
     }
   free(rg);
}

static void
_e_fm2_region_realize(E_Fm2_Region *rg)
{
   Evas_List *l;
   
   if (rg->realized) return;
   /* actually create evas objects etc. */
   rg->realized = 1;
   printf("REG %p REALIZE\n", rg);
   edje_freeze();
   for (l = rg->list; l; l = l->next) _e_fm2_icon_realize(l->data);
   edje_thaw();
}

static void
_e_fm2_region_unrealize(E_Fm2_Region *rg)
{
   Evas_List *l;
   
   if (!rg->realized) return;
   /* delete evas objects */
   rg->realized = 0;
   printf("REG %p UNREALIZE\n", rg);
   edje_freeze();
   for (l = rg->list; l; l = l->next) _e_fm2_icon_unrealize(l->data);
   edje_thaw();
}

static int
_e_fm2_region_visible(E_Fm2_Region *rg)
{
   /* return if the icon is visible */
   if (
       ((rg->x - rg->sd->pos.x) < (rg->sd->w)) &&
       ((rg->x + rg->w - rg->sd->pos.x) > 0) &&
       ((rg->y - rg->sd->pos.y) < (rg->sd->h)) &&
       ((rg->y + rg->h - rg->sd->pos.y) > 0)
       )
     return 1;
   return 0;
}

/**************************/

static void
_e_fm2_cb_scroll_job(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->scroll_job = NULL;
   printf("DO scroll!\n");
   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   _e_fm2_regions_eval(sd->obj);
   _e_fm2_obj_icons_place(sd);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(sd->obj));
}

static void
_e_fm2_cb_resize_job(void *data)
{
   E_Fm2_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->resize_job = NULL;
   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   _e_fm2_regions_free(sd->obj);
   _e_fm2_icons_place(sd->obj);
   _e_fm2_regions_populate(sd->obj);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(sd->obj));
}

static int
_e_fm2_cb_icon_sort(void *data1, void *data2)
{
   E_Fm2_Icon *ic1, *ic2;
   
   ic1 = data1;
   ic2 = data2;
   if (ic1->sd->no_case_sort)
     {
	char buf1[4096], buf2[4096], *p;
	
	strncpy(buf1, ic1->file, sizeof(buf1) - 2);
	strncpy(buf2, ic2->file, sizeof(buf2) - 2);
	buf1[sizeof(buf1) - 1] = 0;
	buf2[sizeof(buf2) - 1] = 0;
	p = buf1;
	while (*p)
	  {
	     *p = tolower(*p);
	     p++;
	  }
	p = buf2;
	while (*p)
	  {
	     *p = tolower(*p);
	     p++;
	  }
	return strcmp(buf1, buf2);
     }
   return strcmp(ic1->file, ic2->file);
}

static int
_e_fm2_cb_scan_idler(void *data)
{
   E_Fm2_Smart_Data *sd;
   struct dirent *dp;

   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   if (!sd->dir) sd->dir = opendir(sd->realpath);
   if (!sd->dir) goto endscan;
   
   dp = readdir(sd->dir);
   if (!dp) goto endscan;
   /* no - you don't want the cuirrent and parent dir links listed */
   if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, ".."))) return 1;
   /* skip dotfiles */
   if (dp->d_name[0] == '.') return 1;
   _e_fm2_file_add(data, dp->d_name);
   return 1;
   
   endscan:
   if (sd->dir)
     {
	closedir(sd->dir);
	sd->dir = NULL;
     }
   sd->scan_idler = NULL;
   if (sd->scan_timer)
     {
	ecore_timer_del(sd->scan_timer);
	sd->scan_timer = ecore_timer_add(0.0001, _e_fm2_cb_scan_timer, sd->obj);
     }
   return 0;
}

static int
_e_fm2_cb_scan_timer(void *data)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
   _e_fm2_queue_process(data);
   if (!sd->scan_idler)
     {
	Evas_List *l;
	
	/* we finished scanning! */
	for (l = sd->icons; l; l = l->next)
	  {
	     E_Fm2_Icon *ic;
	     
	     ic = l->data;
	     printf("FM: IC: %i %i, %s\n", ic->x, ic->y, ic->file);
	  }
	sd->scan_timer = NULL;
	return 0;
     }
   return 1;
}

/**************************/
static void
_e_fm2_obj_icons_place(E_Fm2_Smart_Data *sd)
{
   Evas_List *l, *ll;
   E_Fm2_Region *rg;
   E_Fm2_Icon *ic;

   evas_event_freeze(evas_object_evas_get(sd->obj));
   edje_freeze();
   for (l = sd->regions.list; l; l = l->next)
     {
	rg = l->data;
	if (rg->realized)
	  {
	     for (ll = rg->list; ll; ll = ll->next)
	       {
		  ic = ll->data;
		  if (ic->realized)
		    evas_object_move(ic->obj, 
				     sd->x + ic->x - sd->pos.x, 
				     sd->y + ic->y - sd->pos.y);
	       }
	  }
     }
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(sd->obj));
}

/**************************/

static void
_e_fm2_smart_add(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = E_NEW(E_Fm2_Smart_Data, 1);
   if (!sd) return;
   sd->obj = obj;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
   evas_object_move(obj, 0, 0);
   evas_object_resize(obj, 0, 0);
}

static void
_e_fm2_smart_del(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   
   _e_fm2_scan_stop(obj);
   _e_fm2_queue_free(obj);
   _e_fm2_regions_free(obj);
   _e_fm2_icons_free(obj);
   if (sd->scroll_job) ecore_job_del(sd->scroll_job);
   if (sd->resize_job) ecore_job_del(sd->resize_job);
   E_FREE(sd->dev);
   E_FREE(sd->path);
   E_FREE(sd->realpath);
   
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_fm2_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->clip, x, y);
   printf("mov\n");
   _e_fm2_obj_icons_place(sd);
}

static void
_e_fm2_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Fm2_Smart_Data *sd;
   int wch = 0, hch = 0;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->w == w) && (sd->h == h)) return;
   if (w != sd->w) wch = 1;
   if (h != sd->h) hch = 1;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, w, h);

   /* for automatic layout - do this - NB; we could put this on a timer delay */
   if (wch)
     {
	if (sd->resize_job) ecore_job_del(sd->resize_job);
	sd->resize_job = ecore_job_add(_e_fm2_cb_resize_job, obj);
     }
   else
     {
	if (sd->scroll_job) ecore_job_del(sd->scroll_job);
	sd->scroll_job = ecore_job_add(_e_fm2_cb_scroll_job, obj);
     }
}

static void
_e_fm2_smart_show(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_fm2_smart_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_fm2_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_fm2_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_fm2_smart_clip_unset(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}
