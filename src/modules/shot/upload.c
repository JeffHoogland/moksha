#include <Elementary.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static Ecore_Con_Url *url_up = NULL;

static Eina_Bool
_upload_data_cb(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;

   if (ev->url_con != url_up) return EINA_TRUE;
   if (ev->size < 1024)
     {
        char *txt = alloca(ev->size + 1);

        memcpy(txt, ev->data, ev->size);
        txt[ev->size] = 0;
        printf("R %s\n", txt);
        fflush(stdout);
     }
   return EINA_FALSE;
}

static Eina_Bool
_upload_progress_cb(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *event)
{
   size_t total, current;
   Ecore_Con_Event_Url_Progress *ev = event;

   if (ev->url_con != url_up) return EINA_TRUE;
   total = ev->up.total;
   current = ev->up.now;
   if (total > 0)
     {
        printf("U %i\n", (int)((current * 1000) / total));
        fflush(stdout);
     }
   return EINA_FALSE;
}

static Eina_Bool
_upload_complete_cb(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;

   if (ev->url_con != url_up) return EINA_TRUE;
   if (ev->status != 200) printf("E %i\n", ev->status);
   else printf("O\n");
   fflush(stdout);
   elm_exit();
   return EINA_FALSE;
}

static Eina_Bool
find_tmpfile(int quality, char *buf, size_t buf_size)
{
   int i;

   // come up with a tmp file - not really that critical as its due for
   // sharing to the internet as a whole
   for (i = 0; i < 100; i++)
     {
        int fd, v = rand();

        if (quality == 100) snprintf(buf, buf_size, "/tmp/e-shot-%x.png", v);
        else snprintf(buf, buf_size, "/tmp/e-shot-%x.jpg", v);
        fd = open(buf, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd >= 0)
          {
             close(fd);
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

EAPI int
elm_main(int argc, char **argv)
{
   Evas_Object *win, *image;
   Eina_File *infile;
   void *fdata;
   size_t fsize;
   Eina_Bool upload = EINA_FALSE;
   const char *rgba_file, *out_file = NULL;
   int w, h, stride, quality, image_stride, y;
   char *image_data, *src;
   char opts[256];

   if (argc < 6) return 1;

   rgba_file = argv[1]; // path to raw linear memory format rgba32 pixel data
   w         = atoi(argv[2]); // width in pixels
   h         = atoi(argv[3]); // height in pixels
   stride    = atoi(argv[4]); // stride per line in bytes
   quality   = atoi(argv[5]); // qwuality to save out as (100 == lossless png)
   if (argc >= 7) out_file = eina_stringshare_add(argv[6]); // out file path
   if ((w <= 0) || (w > 65535) || (h <= 0) || (h > 65535) ||
       (stride <= 0) || (stride > (65535 * 4)) ||
       (quality < 0) || (quality > 100))
     return 1;

   // set up buffer window as scratch space
   elm_config_preferred_engine_set("buffer");
   win = elm_win_add(NULL, "Shot-Upload", ELM_WIN_BASIC);
   elm_win_norender_push(win);

   // come up with tmp out file if no dest out provided
   if (!out_file)
     {
        char buf[PATH_MAX];

        upload = EINA_TRUE;
        if (find_tmpfile(quality, buf, sizeof(buf)))
          out_file = eina_stringshare_add(buf);
     }
   // open raw rgba data file which we will mmap
   infile = eina_file_open(rgba_file, EINA_FALSE);
   if (!infile) return 2;
   fsize = eina_file_size_get(infile);
   fdata = eina_file_map_all(infile, EINA_FILE_SEQUENTIAL);
   if (!((fsize > 0) && (fdata)))
     {
        ecore_file_unlink(rgba_file);
        return 3;
     }

   // create image objectfor saving out with right format and size
   image = evas_object_image_add(evas_object_evas_get(win));
   evas_object_image_colorspace_set(image, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(image, EINA_FALSE);
   evas_object_image_size_set(image, w, h);
   image_stride = evas_object_image_stride_get(image);
   image_data = evas_object_image_data_get(image, EINA_TRUE);
   if (!((image_stride > 0) && (image_data)))
     {
        ecore_file_unlink(rgba_file);
        return 4;
     }
   // copy data into output image (could also set data straight in
   src = fdata;
   for (y = 0; y < h; y++)
     {
        memcpy(image_data, src, w * 4);
        image_data += image_stride;
        src += stride;
     }
   if (quality == 100)
     snprintf(opts, sizeof(opts), "compress=%i", 9);
   else
     snprintf(opts, sizeof(opts), "quality=%i", quality);
   eina_file_close(infile);
   ecore_file_unlink(rgba_file);
   // save the file
   if (!evas_object_image_save(image, out_file, NULL, opts))
     return 5;

   // if we have to upload it, open our output file, mmap it and upload
   if (upload)
     {
        infile = eina_file_open(out_file, EINA_FALSE);
        if (infile)
          {
             fsize = eina_file_size_get(infile);
             fdata = eina_file_map_all(infile, EINA_FILE_SEQUENTIAL);
             if ((fsize > 0) && (fdata))
               {
                  Ecore_Event_Handler *h1, *h2, *h3;

                  h1 = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                                               _upload_data_cb, NULL);
                  h2 = ecore_event_handler_add(ECORE_CON_EVENT_URL_PROGRESS,
                                               _upload_progress_cb, NULL);
                  h3 = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                                               _upload_complete_cb, NULL);
                  if ((h1) && (h2) && (h3))
                    {
                       url_up = ecore_con_url_new
                         ("https://www.enlightenment.org/shot.php");
                       if (url_up)
                         {
                            // why use http 1.1? proxies like squid don't
                            // handle 1.1 posts with expect like curl uses
                            // by default, so go to 1.0 and this all works
                            // dandily out of the box
                            ecore_con_url_http_version_set
                              (url_up, ECORE_CON_URL_HTTP_VERSION_1_0);
                            ecore_con_url_post
                              (url_up, fdata, fsize, "application/x-e-shot");
                            // need loop to run to drive the uploading
                            elm_run();
                         }
                    }
                  ecore_event_handler_del(h1);
                  ecore_event_handler_del(h2);
                  ecore_event_handler_del(h3);
               }
             if (fdata) eina_file_map_free(infile, fdata);
             eina_file_close(infile);
          }
        // output was temporary here
        ecore_file_unlink(out_file);
     }
   evas_object_del(win);
   eina_stringshare_del(out_file);
   return 0;
}
ELM_MAIN()
