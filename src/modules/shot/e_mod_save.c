#include "e_mod_main.h"

typedef struct
{
   char *path, *outfile;
   void *data;
   int w, h, stride, quality, server;
   size_t size;
   int fd;
   Eina_Bool copy;
} Rgba_Writer_Data;

static void
_rgba_data_free(Rgba_Writer_Data *rdata)
{
   free(rdata->path);
   free(rdata->outfile);
   free(rdata->data);
   close(rdata->fd);
   free(rdata);
}

static void
_cb_rgba_writer_do(void *data, Ecore_Thread *th __UNUSED__)
{
   Rgba_Writer_Data *rdata = data;
   if (write(rdata->fd, rdata->data, rdata->size) < 0)
     ERR("Write of shot rgba data failed");
}

static void
_cb_rgba_writer_done(void *data, Ecore_Thread *th __UNUSED__)
{
   Rgba_Writer_Data *rdata = data;
   char buf[PATH_MAX];

   if (rdata->outfile)
     snprintf(buf, sizeof(buf), "%s/%s/upload '%s' %i %i %i %i %i '%s'",
              e_module_dir_get(shot_module), MODULE_ARCH,
              rdata->path, rdata->w, rdata->h, rdata->stride,
              rdata->quality, rdata->server, rdata->outfile);
   else
     snprintf(buf, sizeof(buf), "%s/%s/upload '%s' %i %i %i %i %i",
              e_module_dir_get(shot_module), MODULE_ARCH,
              rdata->path, rdata->w, rdata->h, rdata->stride,
              rdata->quality, rdata->server);

   share_save(buf, rdata->outfile, rdata->copy);
   _rgba_data_free(rdata);
}

static void
_cb_rgba_writer_cancel(void *data, Ecore_Thread *th __UNUSED__)
{
   Rgba_Writer_Data *rdata = data;
   _rgba_data_free(rdata);
}

void
save_to(const char *file, Eina_Bool copy)
{
   int fd;
   char tmpf[256] = "e-shot-rgba-XXXXXX";
   Eina_Tmpstr *path = NULL;
   int imw = 0, imh = 0, imstride;

   fd = eina_file_mkstemp(tmpf, &path);
   if (fd >= 0)
     {
        unsigned char *data = NULL;
        Rgba_Writer_Data *thdat = NULL;
        size_t size = 0;
        Evas_Object *img = preview_image_get();

        ui_edit_prepare();
        if ((crop.x == 0) && (crop.y == 0) &&
            (crop.w == 0) && (crop.h == 0))
          {
             if (img)
               {
                  int w = 0, h = 0;
                  int stride = evas_object_image_stride_get(img);
                  unsigned char *src_data = evas_object_image_data_get(img, EINA_FALSE);

                  evas_object_image_size_get(img, &w, &h);
                  if ((stride > 0) && (src_data) && (h > 0))
                    {
                       imw = w;
                       imh = h;
                       imstride = stride;
                       size = stride * h;
                       data = malloc(size);
                       if (data) memcpy(data, src_data, size);
                    }
               }
          }
        else
          {
             if (img)
               {
                  int w = 0, h = 0;
                  int stride = evas_object_image_stride_get(img);
                  unsigned char *src_data = evas_object_image_data_get(img, EINA_FALSE);
                  evas_object_image_size_get(img, &w, &h);
                  if ((stride > 0) && (src_data) && (h > 0))
                    {
                       size = crop.w * crop.h * 4;
                       data = malloc(size);
                       if (data)
                         {
                            int y;
                            unsigned char *s, *d;

                            imw = crop.w;
                            imh = crop.h;
                            imstride = imw * 4;
                            d = data;
                            for (y = crop.y; y < (crop.y + crop.h); y++)
                              {
                                 s = src_data + (stride * y) + (crop.x * 4);
                                 memcpy(d, s, crop.w * 4);
                                 d += crop.w * 4;
                              }
                         }
                    }
               }
          }
        if (data)
          {
             thdat = calloc(1, sizeof(Rgba_Writer_Data));
             if (thdat)
               {
                  thdat->path = strdup(path);
                  if (file) thdat->outfile = strdup(file);
                  if ((thdat->path) &&
                      (((file) && (thdat->outfile)) ||
                       (!file)))
                    {
                       thdat->data = data;
                       thdat->size = size;
                       thdat->fd = fd;
                       thdat->w = imw;
                       thdat->h = imh;
                       thdat->stride = imstride;
                       thdat->quality = quality;
                       thdat->copy = copy;
                       thdat->server = server;
                       ecore_thread_run(_cb_rgba_writer_do,
                                        _cb_rgba_writer_done,
                                        _cb_rgba_writer_cancel, thdat);
                    }
                  else
                    {
                       free(data);
                       free(thdat->path);
                       free(thdat->outfile);
                       free(thdat);
                       thdat = NULL;
                    }
               }
             else
               free(data);
          }
        if (!thdat) close(fd);
        eina_tmpstr_del(path);
     }
   return;
}

void
save_show(Eina_Bool copy)
{
   char path[PATH_MAX + 512];
   char path2[PATH_MAX + 512];
   char buf[256];
   const char *dirs[] = { "shots", NULL };
   time_t tt;
   struct tm *tm;

   ecore_file_mksubdirs(e_user_dir_get(), dirs);
   time(&tt);
   tm = localtime(&tt);
   if (quality == 100)
     strftime(buf, sizeof(buf), "shot-%Y-%m-%d_%H-%M-%S.png", tm);
   else
     strftime(buf, sizeof(buf), "shot-%Y-%m-%d_%H-%M-%S.jpg", tm);
   e_user_dir_snprintf(path, sizeof(path), "shots/%s", buf);
   save_to(path, copy);
   snprintf(path, sizeof(path), "%s/shots.desktop",
            e_module_dir_get(shot_module));
   snprintf(path2, sizeof(path2), "%s/fileman/favorites/shots.desktop",
            e_user_dir_get());
   if (!ecore_file_exists(path2)) ecore_file_cp(path, path2);
   if (!copy)
     {
        char exec[PATH_MAX];
        const char *home;
        E_Zone *zone = NULL;
        E_Exec_Instance *exe_inst = NULL;

        zone = e_util_zone_current_get(e_manager_current_get());
        home = "$E_HOME_DIR/shots";

        if (home)
          {
            int ret = snprintf(exec, sizeof(exec), "xdg-open %s", home);
            if ((ret >= 0))
              exe_inst = e_exec(zone, NULL, exec, NULL, NULL);
            if (!exe_inst)
              e_util_dialog_show
               (_("Error - No Filemanager"),
                _("No filemanager action and/or module was found.<br>"
                  "Cannot show the location of your screenshots."));
         }
     }
   preview_abort();
}
