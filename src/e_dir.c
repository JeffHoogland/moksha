#include "file.h"
#include "e_dir.h"
#include "e_view_machine.h"
#include "view.h"
#include "icons.h"
#include "util.h"
#include "libefsd.h"
#include "e_file.h"
#include "globals.h"

static void         e_dir_handle_fs_restart(void *data);
static void         e_dir_handle_fs(EfsdEvent * ev);
static void         e_dir_handle_efsd_event_reply(EfsdEvent * ev);
static void         e_dir_handle_efsd_event_reply_stat(EfsdEvent * ev);
static void         e_dir_handle_efsd_event_reply_readlink(EfsdEvent * ev);
static void         e_dir_handle_efsd_event_reply_getfiletype(EfsdEvent * ev);
static void         e_dir_handle_efsd_event_reply_getmeta(EfsdEvent * ev);

static void         e_dir_cleanup(E_Dir * d);
static void         e_dir_bg_reload_timeout(int val, void *data);
static void         e_dir_set_default_background(E_Dir * d);

void
e_dir_init(void)
{
   D_ENTER;
   e_fs_add_event_handler(e_dir_handle_fs);
   D_RETURN;
}

static void
e_dir_cleanup(E_Dir * d)
{
   D_ENTER;

   if (!d)
      D_RETURN;

   efsd_stop_monitor(e_fs_get_connection(), d->dir, TRUE);
   if (d->restarter)
      e_fs_del_restart_handler(d->restarter);

   d->restarter = NULL;
   e_view_machine_unregister_dir(d);

   e_object_cleanup(E_OBJECT(d));

   D_RETURN;
}

E_Dir              *
e_dir_new(void)
{
   E_Dir              *d;

   D_ENTER;

   d = NEW(E_Dir, 1);
   ZERO(d, E_Dir, 1);
   d->dir = NULL;
   d->views = NULL;

   e_object_init(E_OBJECT(d), (E_Cleanup_Func) e_dir_cleanup);

   e_view_machine_register_dir(d);
   D_RETURN_(d);
}

void
e_dir_register_view(E_Dir * d, E_View * v)
{
   D_ENTER;
   v->dir = d;
   d->views = evas_list_append(d->views, v);
/* dont ref the first time */
   if (d->views->next)
      e_object_ref(E_OBJECT(v->dir));
   D_RETURN;
}

void
e_dir_unregister_view(E_View * v)
{
   D_ENTER;
   v->dir->views = evas_list_remove(v->dir->views, v);
   e_object_unref(E_OBJECT(v->dir));
   D_RETURN;
}

static void
e_dir_set_default_background(E_Dir * d)
{
   char                buf[PATH_MAX];

   D_ENTER;

   if (!d)
      D_RETURN;

   IF_FREE(d->bg_file);

   if (d->is_desktop)
      snprintf(buf, PATH_MAX, "%s/default.bg.db", e_config_get("backgrounds"));
   else
      snprintf(buf, PATH_MAX, "%s/view.bg.db", e_config_get("backgrounds"));

   e_strdup(d->bg_file, buf);
   snprintf(buf, PATH_MAX, "background_reload:%s", d->dir);

   ecore_add_event_timer(buf, 0.5, e_dir_bg_reload_timeout, 0, d);

   D_RETURN;
}

static void
e_dir_handle_fs_restart(void *data)
{
   E_Dir              *d;

   D_ENTER;
   d = data;
   D("e_dir_handle_fs_restart\n");
   if (e_fs_get_connection())
     {
	EfsdOptions        *ops;

	/* FIXME restart with metadata pending for views */

	ops = efsd_ops(3,
		       efsd_op_get_stat(),
		       efsd_op_get_filetype(), efsd_op_list_all());
	d->monitor_id = efsd_start_monitor(e_fs_get_connection(), d->dir,
					   ops, TRUE);

     }
   D("restarted monitor id (connection = %p), %i for %s\n",
     e_fs_get_connection(), d->monitor_id, d->dir);

   D_RETURN;
}

void
e_dir_set_dir(E_Dir * d, char *dir)
{
   D_ENTER;

   if (!d)
      D_RETURN;

/* stop monitoring old dir */
   if ((d->dir) && (d->monitor_id))
     {
	efsd_stop_monitor(e_fs_get_connection(), d->dir, TRUE);
	d->monitor_id = 0;
     }

   IF_FREE(d->dir);
   d->dir = e_file_realpath(dir);

/* start monitoring new dir */
   d->restarter = e_fs_add_restart_handler(e_dir_handle_fs_restart, d);
   if (e_fs_get_connection())
     {
	EfsdOptions        *ops;

	ops = efsd_ops(3,
		       efsd_op_get_stat(),
		       efsd_op_get_filetype(), efsd_op_list_all());
	d->monitor_id = efsd_start_monitor(e_fs_get_connection(), d->dir,
					   ops, TRUE);
	D("monitor id for %s = %i\n", d->dir, d->monitor_id);
     }
   D_RETURN;
}

static void
e_dir_handle_fs(EfsdEvent * ev)
{
   D_ENTER;

   if (!ev)
      D_RETURN;

   switch (ev->type)
     {
     case EFSD_EVENT_FILECHANGE:
	switch (ev->efsd_filechange_event.changetype)
	  {
	  case EFSD_FILE_CREATED:
	     e_dir_file_added(ev->efsd_filechange_event.id,
			      ev->efsd_filechange_event.file);
	     break;
	  case EFSD_FILE_EXISTS:
	     e_dir_file_added(ev->efsd_filechange_event.id,
			      ev->efsd_filechange_event.file);
	     break;
	  case EFSD_FILE_DELETED:
	     e_dir_file_deleted(ev->efsd_filechange_event.id,
				ev->efsd_filechange_event.file);
	     break;
	  case EFSD_FILE_CHANGED:
	     e_dir_file_changed(ev->efsd_filechange_event.id,
				ev->efsd_filechange_event.file);
	     break;
	  case EFSD_FILE_MOVED:
	     e_dir_file_moved(ev->efsd_filechange_event.id,
			      ev->efsd_filechange_event.file);
	     break;
	  case EFSD_FILE_END_EXISTS:
	     break;
	  default:
	     break;
	  }
	break;
     case EFSD_EVENT_REPLY:
	e_dir_handle_efsd_event_reply(ev);
	break;
     default:
	break;
     }
   D_RETURN;
}

static void
e_dir_handle_efsd_event_reply_getfiletype(EfsdEvent * ev)
{
   E_File             *f;
   char               *file = NULL;
   Evas_List           l;
   E_Dir              *dir;

   char               *m, *p;
   char                mime[PATH_MAX], base[PATH_MAX];

   D_ENTER;

   if (!ev)
      D_RETURN;

   if (!ev->efsd_reply_event.errorcode == 0)
      D_RETURN;

   if ((file = efsd_event_filename(ev)))
     {
	file = e_file_get_file(file);
     }
   dir = e_dir_find_by_monitor_id(efsd_event_id(ev));

   f = e_file_get_by_name(dir->files, file);
/* if its not in the list we care about, its filetype is meaningless */
   if (!f)
      D_RETURN;

   m = ev->efsd_reply_event.data;
   p = strchr(m, '/');
   if (p)
     {
	strcpy(base, m);
	strcpy(mime, p + 1);
	p = strchr(base, '/');
	*p = 0;
     }
   else
     {
	strcpy(base, m);
	strcpy(mime, "unknown");
     }
   e_file_set_mime(f, base, mime);

   for (l = dir->views; l; l = l->next)
     {
	E_View             *v = (E_View *) l->data;
	E_Icon             *ic = e_icon_find_by_file(v, f->file);

	/* Try to update the GUI.
	 * It's just a try because we need to have the file's stat
	 * info as well.  --cK.
	 */
	e_icon_update_state(ic);
	e_icon_initial_show(ic);
     }
   D_RETURN;
}

static void
e_dir_handle_efsd_event_reply_stat(EfsdEvent * ev)
{
   E_Dir              *d;
   E_File             *f;
   Evas_List           l;

   D_ENTER;

   if (!ev)
      D_RETURN;

   if (!ev->efsd_reply_event.errorcode == 0)
      D_RETURN;

   d = e_dir_find_by_monitor_id(efsd_event_id(ev));
   f = e_file_get_by_name(d->files, e_file_get_file(efsd_event_filename(ev)));
   /* if its not in the list we care about, return */
   if (!f)
      D_RETURN;

   /* When everything went okay and we can find a dir,
    * set the file stat data for the file and try to update the gui. 
    * It's just a try because we need to have received the filetype 
    * info too. --cK.  */
   f->stat = *((struct stat *)efsd_event_data(ev));
   for (l = d->views; l; l = l->next)
     {
	E_View             *v = (E_View *) l->data;
	E_Icon             *ic = e_icon_find_by_file(v, f->file);

	e_icon_update_state(ic);
	e_icon_initial_show(ic);
     }

   D_RETURN;
}

static void
e_dir_handle_efsd_event_reply_readlink(EfsdEvent * ev)
{
   E_Dir              *d;
   E_File             *f;
   Evas_List           l;

   D_ENTER;

   if (!ev)
      D_RETURN;

   if (!ev->efsd_reply_event.errorcode == 0)
      D_RETURN;

   d = e_dir_find_by_monitor_id(efsd_event_id(ev));
   f = e_file_get_by_name(d->files, e_file_get_file(efsd_event_filename(ev)));
   if (f)
     {
	e_file_set_link(f, (char *)efsd_event_data(ev));
     }
   for (l = d->views; l; l = l->next)
     {
	E_View             *v = (E_View *) l->data;
	E_Icon             *ic = e_icon_find_by_file(v, f->file);

	e_icon_update_state(ic);
	e_icon_initial_show(ic);
     }

   D_RETURN;
}

static void
e_dir_handle_efsd_event_reply_getmeta(EfsdEvent * ev)
{
   Evas_List           l;
   EfsdCmdId           cmd;

   D_ENTER;

   if (!ev)
      D_RETURN;

   cmd = efsd_event_id(ev);
   for (l = VM->views; l; l = l->next)
     {
	E_View             *v;

	v = l->data;
	/* ignore metadata for desktops */
	if (v->dir->is_desktop)
	   continue;
	if (v->geom_get.x == cmd)
	  {
	     v->geom_get.x = 0;
	     if (efsd_metadata_get_type(ev) == EFSD_INT)
	       {
		  if (ev->efsd_reply_event.errorcode == 0)
		     efsd_metadata_get_int(ev, &(v->location.x));
		  else
		     v->location.x = 0;
	       }
	  }
	else if (v->geom_get.y == cmd)
	  {
	     v->geom_get.y = 0;
	     if (efsd_metadata_get_type(ev) == EFSD_INT)
	       {
		  if (ev->efsd_reply_event.errorcode == 0)
		     efsd_metadata_get_int(ev, &(v->location.y));
		  else
		     v->location.y = 0;

	       }
	  }
	else if (v->geom_get.w == cmd)
	  {
	     v->geom_get.w = 0;
	     if (efsd_metadata_get_type(ev) == EFSD_INT)
	       {
		  if (ev->efsd_reply_event.errorcode == 0)
		     efsd_metadata_get_int(ev, &(v->size.w));
		  else
		     v->size.w = 400;
	       }
	  }
	else if (v->geom_get.h == cmd)
	  {
	     v->geom_get.h = 0;
	     if (ev->efsd_reply_event.errorcode == 0)
	       {
		  if (ev->efsd_reply_event.errorcode == 0)
		     efsd_metadata_get_int(ev, &(v->size.h));
		  else
		     v->size.h = 401;
	       }
	  }
	/* FIXME currently, the bg info is not set via metadata */
/* 
 *       else if (v->getbg == cmd)
 *       {
 * 	 v->getbg = 0;
 * 	 if (efsd_metadata_get_type(ev) == EFSD_STRING)
 * 	 {
 * 	    if (ev->efsd_reply_event.errorcode == 0)
 * 	    {
 * 	       char buf[PATH_MAX];
 * 
 * 	       IF_FREE(v->dir->bg_file);
 * 	       e_strdup(v->dir->bg_file, efsd_metadata_get_str(ev));
 * 	       snprintf(buf, PATH_MAX, "background_reload:%s", v->dir->dir);
 * 	       ecore_add_event_timer(buf, 0.5, e_dir_bg_reload_timeout, 0, v->dir);
 * 	    }
 * 	    else
 * 	       e_dir_set_default_background(v->dir);
 * 	 }
 *       }
 */
	/* We have received all metadata we need, display the view */
	if ((!v->geom_get.x) &&
	    (!v->geom_get.y) &&
	    (!v->geom_get.w) && (!v->geom_get.h) && (v->geom_get.busy))
	  {
	     E_Border           *b;

	     ecore_window_move_resize(v->win.base, v->location.x, v->location.y,
				      v->size.w, v->size.h);
	     ecore_window_set_xy_hints(v->win.base, v->location.x,
				       v->location.y);
	     v->size.force = 1;
	     v->geom_get.busy = 0;
	     if (v->bg)
		e_bg_resize(v->bg, v->size.w, v->size.h);
	     if (v->options.back_pixmap)
		e_view_update(v);
	     b = e_border_adopt(v->win.base, 1);
	     b->client.internal = 1;
	     e_border_remove_click_grab(b);
	  }
     }
   D_RETURN;
}

static void
e_dir_handle_efsd_event_reply(EfsdEvent * ev)
{
   D_ENTER;

   if (!ev)
      D_RETURN;

   switch (ev->efsd_reply_event.command.type)
     {
     case EFSD_CMD_REMOVE:
	break;
     case EFSD_CMD_MOVE:
	break;
     case EFSD_CMD_SYMLINK:
	break;
     case EFSD_CMD_LISTDIR:
	break;
     case EFSD_CMD_MAKEDIR:
	break;
     case EFSD_CMD_CHMOD:
	break;
     case EFSD_CMD_GETFILETYPE:
	e_dir_handle_efsd_event_reply_getfiletype(ev);
	break;
     case EFSD_CMD_STAT:
	e_dir_handle_efsd_event_reply_stat(ev);
	break;
     case EFSD_CMD_READLINK:
	e_dir_handle_efsd_event_reply_readlink(ev);
	break;
     case EFSD_CMD_CLOSE:
	break;
     case EFSD_CMD_SETMETA:
	break;
     case EFSD_CMD_GETMETA:
	e_dir_handle_efsd_event_reply_getmeta(ev);
	break;
     case EFSD_CMD_STARTMON_DIR:
	break;
     case EFSD_CMD_STARTMON_FILE:
	break;
     case EFSD_CMD_STOPMON_DIR:
	break;
     case EFSD_CMD_STOPMON_FILE:
	break;
     default:
	break;
     }
   D_RETURN;
}
static void
e_dir_ib_reload_timeout(int val, void *data)
{
   Evas_List           l;
   E_View             *v;
   E_Dir              *d;

   D_ENTER;
   d = data;

   for (l = d->views; l; l = l->next)
     {
	v = (E_View *) l->data;
	e_view_ib_reload(v);
     }
   D_RETURN;
   UN(val);
}

static void
e_dir_bg_reload_timeout(int val, void *data)
{
   Evas_List           l;
   E_View             *v;
   E_Dir              *d;

   D_ENTER;
   d = data;

   for (l = d->views; l; l = l->next)
     {
	v = (E_View *) l->data;
	e_view_bg_reload(v);
     }
   D_RETURN;
   UN(val);
}

void
e_dir_file_added(int id, char *file)
{
   Evas_List           l;
   E_Dir              *d;
   E_View             *v;
   E_File             *f;
   char                buf[PATH_MAX];

   D_ENTER;

   /* if we get a path - ignore it - its not a file in the dir */
   if (!file || file[0] == '/')
      D_RETURN;
   d = e_dir_find_by_monitor_id(id);

   if (!strcmp(file, ".e_background.bg.db"))
     {
	IF_FREE(d->bg_file);
	snprintf(buf, PATH_MAX, "%s/%s", d->dir, file);
	e_strdup(d->bg_file, buf);
	snprintf(buf, PATH_MAX, "background_reload:%s", d->dir);
	ecore_add_event_timer(buf, 0.5, e_dir_bg_reload_timeout, 0, d);
     }
   /*else if ((!strcmp(".e_iconbar.db", file)) ||
    * (!strcmp(".e_iconbar.bits.db", file)))
    * {
    * snprintf(buf, PATH_MAX, "iconbar_reload:%s", m->dir);
    * ecore_add_event_timer(buf, 0.5, e_dir_ib_reload_timeout, 0, m);
    * } */
   else if (file[0] != '.')
     {
	f = e_file_new(file);
	d->files = evas_list_append(d->files, f);
	/* tell all views for this dir about the new file */
	for (l = d->views; l; l = l->next)
	  {
	     v = l->data;
	     e_view_file_add(v, f);
	  }
     }
   D_RETURN;
}

void
e_dir_file_deleted(int id, char *file)
{
   Evas_List           l;
   E_File             *f;
   E_Dir              *d;

   D_ENTER;

   if (!file || file[0] == '/')
      D_RETURN;

   d = e_dir_find_by_monitor_id(id);
   f = e_file_get_by_name(d->files, file);
   d->files = evas_list_remove(d->files, f);

   if (!strcmp(file, ".e_background.bg.db"))
     {
	e_dir_set_default_background(d);
     }
   /*else if ((!strcmp(".e_iconbar.db", file)) ||
    * (!strcmp(".e_iconbar.bits.db", file)))
    * {
    * for (l = m->views; l; l = l->next)
    * {
    * E_View             *v = (E_View *) l->data;
    * 
    * e_object_unref(E_OBJECT(v->iconbar));
    * v->iconbar = NULL;
    * }
    * } */
   else if (file[0] != '.')
     {
	for (l = d->views; l; l = l->next)
	  {
	     E_View             *v = (E_View *) l->data;

	     e_view_file_delete(v, f);
	  }
     }
   D_RETURN;
}

void
e_dir_file_changed(int id, char *file)
{
   Evas_List           l;
   E_Dir              *d;
   E_File             *f;
   E_View             *v;
   char                buf[PATH_MAX];

   D_ENTER;

   if (!file || file[0] == '/')
      D_RETURN;
   d = e_dir_find_by_monitor_id(id);
   f = e_file_get_by_name(d->files, file);
   if (!strcmp(file, ".e_background.bg.db"))
     {
	IF_FREE(d->bg_file);
	snprintf(buf, PATH_MAX, "%s/%s", d->dir, file);
	e_strdup(d->bg_file, buf);
	snprintf(buf, PATH_MAX, "background_reload:%s", d->dir);
	ecore_add_event_timer(buf, 0.5, e_dir_bg_reload_timeout, 0, d);
     }
   else if ((!strcmp(".e_iconbar.db", file)) ||
	    (!strcmp(".e_iconbar.bits.db", file)))
     {
	snprintf(buf, PATH_MAX, "iconbar_reload:%s", d->dir);
	ecore_add_event_timer(buf, 0.5, e_dir_ib_reload_timeout, 0, d);
     }
   else if (file[0] != '.')
     {
	for (l = d->views; l; l = l->next)
	  {
	     v = l->data;
	     e_view_file_changed(v, f);
	  }
     }
   D_RETURN;
}

void
e_dir_file_moved(int id, char *file)
{
   Evas_List           l;
   E_Dir              *d;

   D_ENTER;

   if (!file || file[0] == '/')
      D_RETURN;
   d = e_dir_find_by_monitor_id(id);
   for (l = d->views; l; l = l->next)
     {
	E_View             *v = (E_View *) l->data;
	E_Icon             *ic;

	ic = e_icon_find_by_file(v, file);
	if (ic)
	  {
	  }
     }
   D_RETURN;
}

E_Dir              *
e_dir_find_by_monitor_id(int id)
{
   E_Dir              *d;
   Evas_List           l;

   D_ENTER;

   for (l = VM->dirs; l; l = l->next)
     {
	d = l->data;
	if (d->monitor_id == id)
	   D_RETURN_(d);
     }
   D_RETURN_(NULL);
}
