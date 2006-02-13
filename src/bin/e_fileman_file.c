/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#ifdef EFM_DEBUG
# define D(x)  do {printf(__FILE__ ":%d:  ", __LINE__); printf x; fflush(stdout);} while (0)
#else
# define D(x)  ((void) 0)
#endif

/* local subsystem functions */
static void _e_fm_file_free(E_Fm_File *file);

/* TODO Make init and shutdown func that populates the assoc */
static E_Config_DD *assoc_app_edd = NULL;
static Evas_List   *assoc_apps = NULL;

/* externally accessible functions */
EAPI E_Fm_File *
e_fm_file_new(const char *filename)
{
   char *ext;
   E_Fm_File *file;
   struct stat st;

   if (stat(filename, &st) == -1) return NULL;
   /* FIXME: stat above will fail if the file is a BROKEN SYMLINK - maybe we
    * should not fail, but do an lstat here and see if it is a symlink (or
    * just a race condition where the file was deleted as we scan), and
    * if so mark it as a broken synlink. we should do an lstat ANYWAY
    * so we know if a file is a symlink or not regardless what it points
    * to.
    */
   file = E_OBJECT_ALLOC(E_Fm_File, E_FM_FILE_TYPE, _e_fm_file_free);
   if (!file) return NULL;
   file->path = strdup(filename);
   if (!file->path) goto error;
   file->name = strdup(ecore_file_get_file((char *)filename));
   if (!file->name) goto error;
   /* Get attributes */
   file->device = st.st_dev;
   file->inode = st.st_ino;
   file->mode = st.st_mode;
   file->nlink = st.st_nlink;
   file->owner = st.st_uid;
   file->group = st.st_gid;
   file->rdev = st.st_rdev;
   file->size = st.st_size;
   file->atime = st.st_atime;
   file->mtime = st.st_mtime;
   file->ctime = st.st_ctime;

   if (S_ISDIR(file->mode)){
      file->type |= E_FM_FILE_TYPE_DIRECTORY;
      file->mime = "directory";
   }
   else if (S_ISREG(file->mode))
     file->type = E_FM_FILE_TYPE_FILE;
   else if (S_ISLNK(file->mode))
     file->type = E_FM_FILE_TYPE_SYMLINK;
   else
     file->type = E_FM_FILE_TYPE_UNKNOWN;

   if (file->name[0] == '.')
     file->type |= E_FM_FILE_TYPE_HIDDEN;

   file->preview_funcs = E_NEW(E_Fm_File_Preview_Function, 5);
   file->preview_funcs[0] = e_fm_file_is_image;
   file->preview_funcs[1] = e_fm_file_is_etheme;
   file->preview_funcs[2] = e_fm_file_is_ebg;
   file->preview_funcs[3] = e_fm_file_is_eap;
   file->preview_funcs[4] = NULL;

   if(!file->mime)
     {
  ext = strrchr(file->name, '.');
  if (ext)
    {
       char *ext2;
       ext = strdup(ext);
       ext2 = ext;
       for (; *ext2; ext2++) *ext2 = tolower(*ext2);
       file->mime = ext;
    }
  else
    file->mime = "unknown";
     }
   D(("e_fm_file_new: %s\n", filename));
   return file;

   error:
   if (file->path) free(file->path);
   if (file->name) free(file->name);
   free(file);
   return NULL;
}

EAPI int
e_fm_file_rename(E_Fm_File *file, const char *name)
{
   char path[PATH_MAX], *dir;

   if ((!name) || (!name[0])) return 0;

   dir = ecore_file_get_dir(file->path);
   if (!dir) return 0;
   snprintf(path, sizeof(path), "%s/%s", dir, name);

   if (ecore_file_mv(file->path, path))
     {
  free(file->path);
  file->path = strdup(path);
  free(file->name);
  file->name = strdup(name);
  D(("e_fm_file_rename: ok (%p) (%s)\n", file, name));
  return 1;
     }
   else
     {
  D(("e_fm_file_rename: fail (%p) (%s)\n", file, name));
  return 0;
     }
}

EAPI int
e_fm_file_delete(E_Fm_File *file)
{
   if (ecore_file_recursive_rm(file->path))
     {
  free(file->path);
  file->path = NULL;
  free(file->name);
  file->name = NULL;
  D(("e_fm_file_delete: ok (%p) (%s)\n", file, file->name));
  return 1;
     }
   else
     {
  D(("e_fm_file_delete: fail (%p) (%s)\n", file, file->name));
  return 0;
     }
}

EAPI int
e_fm_file_copy(E_Fm_File *file, const char *name)
{
   if ((!name) || (!name[0])) return 0;

   if (ecore_file_cp(file->path, name))
     {
  free(file->path);
  file->path = strdup(name);
  free(file->name);
  file->name = strdup(ecore_file_get_file(name));
  D(("e_fm_file_copy: ok (%p) (%s)\n", file, name));
  return 1;
     }
   else
     {
  D(("e_fm_file_copy: fail (%p) (%s)\n", file, name));
  return 0;
     }
}

int
e_fm_file_is_regular(E_Fm_File *file) /* TODO: find better name */
{
   return ((file->type == E_FM_FILE_TYPE_FILE) 
        || (file->type == E_FM_FILE_TYPE_SYMLINK)); 
}

EAPI int
e_fm_file_has_mime(E_Fm_File *file, char* mime)
{
   if (!file->mime) return 0;

   D(("e_fm_file_has_mime: (%p) : %s\n", file,file->mime));
   return (!strcasecmp(file->mime, mime));
}

EAPI int
e_fm_file_can_preview(E_Fm_File *file)
{
   int i;

   D(("e_fm_file_can_preview: (%s) (%p)\n", file->name, file));
   for (i = 0; file->preview_funcs[i]; i++)
     {
  E_Fm_File_Preview_Function func;

  func = file->preview_funcs[i];
  if (func(file))
    return 1;
     }
   return 0;
}

EAPI int
e_fm_file_is_image(E_Fm_File *file)
{
   /* We need to check if it is a filetype supported by evas.
    * If it isn't supported by evas, we can't show it in the
    * canvas.
    */
   
   //D(("e_fm_file_is_image: (%p)\n", file));
   
   return e_fm_file_is_regular(file)
        &&(e_fm_file_has_mime(file,".jpg") 
        || e_fm_file_has_mime(file,".jpeg") 
        || e_fm_file_has_mime(file,".png")); 
}

EAPI int
e_fm_file_is_etheme(E_Fm_File *file)
{
   int          val;
   Evas_List   *groups, *l;

   if (!e_fm_file_is_regular(file) || !e_fm_file_has_mime(file,".edj"))
     return 0;

   val = 0;
   groups = edje_file_collection_list(file->path);
   if (!groups)
     return 0;

   for (l = groups; l; l = l->next)
   {
    if (!strcmp(l->data, "widgets/border/default/border"))
    {
       val = 1;
       break;
    }
   }
   edje_file_collection_list_free(groups);
   return val;
}

EAPI int
e_fm_file_is_ebg(E_Fm_File *file)
{
   int          val;
   Evas_List   *groups, *l;

   if (!e_fm_file_is_regular(file) || !e_fm_file_has_mime(file,".edj"))
     return 0;
  
   val = 0;
   groups = edje_file_collection_list(file->path);
   if (!groups)
     return 0;

   for (l = groups; l; l = l->next)
   {
    if (!strcmp(l->data, "desktop/background"))
    {
       val = 1;
       break;
    }
   }
   edje_file_collection_list_free(groups);
   return val;
}

EAPI int
e_fm_file_is_eap(E_Fm_File *file)
{

   E_App *app;

  if (!e_fm_file_is_regular(file) || !e_fm_file_has_mime(file,".eap"))
     return 0;
  
   app = e_app_new(file->path, 0);
   if (!app)
   {
    e_object_unref(E_OBJECT(app));
    return 0;
   }
   e_object_unref(E_OBJECT(app));
   return 1;
}

EAPI int
e_fm_file_can_exec(E_Fm_File *file)
{
   if (e_fm_file_has_mime(file,".eap"))
   {
     D(("e_fm_file_can_exec: true (%p) (%s)\n", file, file->name));
     return 1;
   }

   if (ecore_file_can_exec(file->path))
   {
    D(("e_fm_file_can_exec: true (%p) (%s)\n", file, file->name));
    return 1;
   }

   D(("e_fm_file_can_exec: false (%p) (%s)\n", file, file->name));
   return 0;
}

EAPI int
e_fm_file_exec(E_Fm_File *file)
{
   Ecore_Exe *exe;

   /* FIXME: use the e app execution mechanisms where possible so we can
    * collect error output
    */
   if(e_fm_file_has_mime(file,".eap"))
   {
       E_App *e_app;
       Ecore_Exe *exe;

       e_app = e_app_new(file->path, 0);

       if (!e_app) return 0;

       exe = ecore_exe_run(e_app->exe, NULL);
       if (exe) ecore_exe_free(exe);
       e_object_unref(E_OBJECT(e_app));
       D(("e_fm_file_exec: eap (%p) (%s)\n", file, file->name));
       return 1;
    } 

   exe = ecore_exe_run(file->path, NULL);
   if (!exe)
   {
    e_error_dialog_show(_("Run Error"),
          _("Enlightenment was unable to fork a child process:\n"
            "\n"
            "%s\n"
            "\n"),
          file->path);
    D(("e_fm_file_exec: fail (%p) (%s)\n", file, file->name));
    return 0;
   }
   /* E/app is the correct tag if the data is en E_App!
   ecore_exe_tag_set(exe, "E/app");
   */
   D(("e_fm_file_exec: ok (%p) (%s)\n", file, file->name));
   return 1;
}

EAPI int
e_fm_file_assoc_set(E_Fm_File *file, const char *assoc)
{
   /* TODO */
   return 1;
}

EAPI int
e_fm_file_assoc_exec(E_Fm_File *file)
{
   char app[PATH_MAX * 2];
   Evas_List *l;
   E_Fm_Assoc_App *assoc;
   Ecore_Exe *exe;

   if (!assoc_apps) return 0;

   /* FIXME: use the e app execution mechanisms where possible so we can
    * collect error output
    */
   for (l = assoc_apps; l; l = l->next)
   {
    assoc = l->data;
    if (e_fm_file_has_mime(file,assoc->mime))
      break;
    assoc = NULL;
   }

   if (!assoc) return 0;

   snprintf(app, PATH_MAX * 2, "%s %s", assoc->app, file->path);
   exe = ecore_exe_run(app, NULL);

   if (!exe)
   {
    e_error_dialog_show(_("Run Error"),
          _("Enlightenment was unable to fork a child process:\n"
            "\n"
            "%s\n"
            "\n"),
          app);
    D(("e_fm_assoc_exec: fail (%s)\n", app));
    return 0;
   }
   /*
    * ecore_exe_tag_set(exe, "E/app");
    */
   D(("e_fm_assoc_exec: ok (%s)\n", app));
   return 1;
}

EAPI int 
e_fm_file_exec_with(E_Fm_File *file, char* exec_with)
{
   Ecore_Exe *exe;
   char app[PATH_MAX * 2];
   if (!exec_with || !file) return 0;

   /* FIXME: use the e app execution mechanisms where possible so we can
    * collect error output
    */
   snprintf(app, PATH_MAX * 2, "%s \"%s\"", exec_with, file->path);
   exe = ecore_exe_run(app, NULL);

   if (!exe)
     {
	e_error_dialog_show(_("Run Error"),
			    _("Enlightenment was unable to fork a child process:\n"
			      "\n"
			      "%s\n"
			      "\n"),
			    app);
	return 0;
     }
   return 1;
}

/* local subsystem functions */
static void
_e_fm_file_free(E_Fm_File *file)
{
   D(("_e_fm_file_free: (%p) (%s)\n", file, file->name));
   free(file->preview_funcs);
   if (file->path) free(file->path);
   if (file->name) free(file->name);
   ///???  if (file->mime) free(file->mime); 
   free(file);
}

