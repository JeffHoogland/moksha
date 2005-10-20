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

/* externally accessible functions */
E_Fm_File *
e_fm_file_new(const char *filename)
{
   E_Fm_File *file;
   struct stat st;

   if (stat(filename, &st) == -1) return NULL;

   file = E_OBJECT_ALLOC(E_Fm_File, E_FM_FILE_TYPE, _e_fm_file_free);
   if (!file) return NULL;
   file->path = strdup(filename);
   if (!file->path) goto error;
   file->name = strdup(ecore_file_get_file(filename));
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

   if (S_ISDIR(file->mode))
     file->type |= E_FM_FILE_TYPE_DIRECTORY;
   else if (S_ISREG(file->mode))
     file->type = E_FM_FILE_TYPE_FILE;
   else if (S_ISLNK(file->mode))
     file->type = E_FM_FILE_TYPE_SYMLINK;
   else
     file->type = E_FM_FILE_TYPE_UNKNOWN;

   if (file->name[0] == '.')
     file->type |= E_FM_FILE_TYPE_HIDDEN;

   D(("e_fm_file_new: %s\n", filename));
   return file;

error:
   if (file->path) free(file->path);
   if (file->name) free(file->name);
   free(file);
   return NULL;
}

int
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

int
e_fm_file_delete(E_Fm_File *file)
{
   if (ecore_file_unlink(file->path))
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

int
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
e_fm_file_is_image(E_Fm_File *file)
{
   /* We need to check if it is a filetype supported by evas.
    * If it isn't supported by evas, we can't show it in the
    * canvas.
    */
   char *ext;

   if ((file->type != E_FM_FILE_TYPE_FILE) && (file->type != E_FM_FILE_TYPE_SYMLINK)) return 0;

   ext = strrchr(file->name, '.');
   if (!ext) return 0;
   
   D(("e_fm_file_is_image: (%p)\n", file));
   return (!strcasecmp(ext, ".jpg")) || (!strcasecmp(ext, ".png"));
}

int
e_fm_file_can_exec(E_Fm_File *file)
{
   char *ext;
   char *fullname;
   
   ext = strrchr(file->name, '.');
   if(ext)
    {
       if(!strcasecmp(ext, ".eap"))
	{
	   D(("e_fm_file_can_exec: true (%p) (%s)\n", file, file->name));
	   return TRUE;
	}
    }
   
   if(ecore_file_can_exec(file->path))
    {
       D(("e_fm_file_can_exec: true (%p) (%s)\n", file, file->name));
       return TRUE;
    }
   
   D(("e_fm_file_can_exec: false (%p) (%s)\n", file, file->name));
   return FALSE;
}

int
e_fm_file_exec(E_Fm_File *file)
{
   Ecore_Exe *exe;
   char *ext;
                         
                      
   ext = strrchr(file->name, '.');
   if(ext)
    {
       if(!strcasecmp(ext, ".eap"))
	{
	   E_App *e_app;
	   Ecore_Exe *exe;
	   
	   e_app = e_app_new(file->path, NULL);
	   
	   if(!e_app) return;
	   
	   exe = ecore_exe_run(e_app->exe, NULL);
	   if (exe) ecore_exe_free(exe);
	   e_object_unref(E_OBJECT(e_app));
	   D(("e_fm_file_exec: eap (%p) (%s)\n", file, file->name));
	   return 1;
	}
    }
   
   exe = ecore_exe_run(file->path, NULL);
   if (!exe)
     {
	e_error_dialog_show(_("Run Error"),
			    _("Enlightenment was unable fork a child process:\n"
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

/* local subsystem functions */
static void
_e_fm_file_free(E_Fm_File *file)
{
   D(("_e_fm_file_free: (%p) (%s)\n", file, file->name));   
   if (file->path) free(file->path);
   if (file->name) free(file->name);
   free(file);
}

