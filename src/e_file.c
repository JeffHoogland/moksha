#include "debug.h"
#include "e_file.h"
#include "file.h"
#include "util.h"

static void e_file_cleanup (E_File *f);

static void
e_file_cleanup(E_File *f)
{
   D_ENTER;

   IF_FREE(f->info.icon);
   IF_FREE(f->info.link)
   IF_FREE(f->info.custom_icon);
   IF_FREE(f->info.mime.base);
   IF_FREE(f->info.mime.type);
   IF_FREE(f->file);
   e_object_cleanup(E_OBJECT(f));

   D_RETURN;
}

E_File *
e_file_new(char *file)
{
   E_File *f;
   D_ENTER;

   if (!file || *file == 0)
     D_RETURN_(NULL);

   f = NEW(E_File, 1);
   
   e_object_init(E_OBJECT(f), 
	 (E_Cleanup_Func) e_file_cleanup);

   f->info.icon = NULL;
   f->info.link = NULL;
   f->info.custom_icon = NULL;
   f->info.mime.base = NULL;
   f->info.mime.type = NULL;
   f->file = strdup(file);
   
   D_RETURN_(f);  
}

E_File  *
e_file_get_by_name(Evas_List l, char *file)
{
   Evas_List ll;
   E_File *f;

   D_ENTER;

   if (!l || !file || *file == 0)
     D_RETURN_(NULL);

   for (ll=l; ll; ll=ll->next)
   {
      f = (E_File*) ll->data;

      if (!strcmp(file, f->file))
      {
	 D_RETURN_(f);
      }
   }

   D_RETURN_(NULL);
}

void
e_file_set_mime(E_File *f, char *base, char *mime)
{
   char icon[PATH_MAX];
   char type[PATH_MAX];	
   char *p;

   D_ENTER;

   if (!f || !base || !mime)
     D_RETURN;

   D("Setting mime: %40s: %s/%s\n", f->file, base, mime);


   if ( ((f->info.mime.base) && !(strcmp(f->info.mime.base, base)))
	&&((f->info.mime.type) && !(strcmp(f->info.mime.type, mime))))
     D_RETURN;

   IF_FREE(f->info.mime.base);
   IF_FREE(f->info.mime.type);
   
   f->info.mime.base = strdup(base);
   f->info.mime.type = strdup(mime);
   
   
   /* effect changes here */
/* 
 *    if (f->info.custom_icon) 
 *      {
 * 	if (f->info.icon) 
 * 	   free(f->info.icon);
 * 	f->info.icon = strdup(f->info.custom_icon);
 * 	evas_set_image_file(f->view->evas, f->obj.icon, f->info.custom_icon);
 * 	e_view_queue_resort(f->view);	
 * 	D_RETURN;
 *      }
 */
   
   /* find an icon */
   strcpy(type, f->info.mime.type);
   p=type;
   do 
   {
      snprintf(icon, PATH_MAX, "%s/data/icons/%s/%s.db", 
	    PACKAGE_DATA_DIR, f->info.mime.base, type);
      p = strrchr(type, '/');
      if (p) *p = 0;
   }
   while (p && !e_file_exists(icon));
   
   /* fallback to base type icon */	  
   if (!e_file_exists(icon))      
      snprintf(icon, PATH_MAX, "%s/data/icons/%s/default.db", 
	       PACKAGE_DATA_DIR, f->info.mime.base);		  
    /* still no luck fall back to default */
   if (!e_file_exists(icon))
      snprintf(icon, PATH_MAX, "%s/data/icons/unknown/default.db",
	       PACKAGE_DATA_DIR);

   f->info.icon = strdup(icon);
   
   D_RETURN;
}

void
e_file_set_link(E_File *f, char *link)
{
   D_ENTER;

   if (!f)
     D_RETURN;

   if ((!link) && (f->info.link))
     {
	free(f->info.link);
	f->info.link = NULL;
	/* effect changes here */
     }
   else if (link)
     {
	if ((f->info.link) && (!strcmp(f->info.link, link)))
	  {
	     free(f->info.link);
	     f->info.link = strdup(link);
	     /* effect changes here */
	  }
     }

   D_RETURN;
}
