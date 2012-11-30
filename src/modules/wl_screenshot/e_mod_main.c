#include "e.h"
#include <wayland-client.h>
#include "e_screenshooter_client_protocol.h"

typedef struct _Instance Instance;
struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_btn;
};

static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(const E_Gadcon_Client_Class *cc);
static void _cb_btn_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static void _cb_handle_global(struct wl_display *disp, unsigned int id, const char *interface, unsigned int version __UNUSED__, void *data);
static struct wl_buffer *_create_shm_buffer(struct wl_shm *_shm, int width, int height, void **data_out);
static void _cb_handle_geometry(void *data, struct wl_output *wl_output, int x, int y, int w, int h, int subpixel, const char *make, const char *model);
static void _cb_handle_mode(void *data, struct wl_output *wl_output, unsigned int flags, int w, int h, int refresh);
static void _save_png(int w, int h, void *data);
static Eina_Bool _cb_timer(void *data __UNUSED__);

static const E_Gadcon_Client_Class _gc = 
{
   GADCON_CLIENT_CLASS_VERSION, "wl_screenshot", 
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static const struct wl_output_listener _output_listener = 
{
   _cb_handle_geometry,
   _cb_handle_mode
};

static E_Module *_mod = NULL;
static E_Screenshooter *_shooter = NULL;
static struct wl_output *_output;
static int ow = 0, oh = 0;
static Ecore_Timer *_timer = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Screenshooter" };

EAPI void *
e_modapi_init(E_Module *m)
{
   struct wl_display *disp;

   /* if (!ecore_wl_init(NULL)) return NULL; */

   disp = ecore_wl_display_get();

   _mod = m;
   e_gadcon_provider_register(&_gc);

   /* e_module_delayed_set(m, 1); */
   /* e_module_priority_set(m, 1000); */

   wl_display_add_global_listener(disp, _cb_handle_global, NULL);

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m)
{
   _mod = NULL;
   e_gadcon_provider_unregister(&_gc);
   /* ecore_wl_shutdown(); */
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   char buff[PATH_MAX];

   inst = E_NEW(Instance, 1);

   snprintf(buff, sizeof(buff), "%s/e-module-wl_screenshot.edj", _mod->dir);

   o = edje_object_add(gc->evas);
   if (!e_theme_edje_object_set(o, "base/theme/modules/wl_screenshot", 
                                "modules/wl_screenshot/main"))
     edje_object_file_set(o, buff, "modules/wl_screenshot/main");

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_btn = o;

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _cb_btn_down, inst);
   return gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   evas_object_del(inst->o_btn);
   free(inst);
}

static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
   Evas_Coord mw, mh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_btn, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_btn, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *cc)
{
   return _("Screenshooter");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *cc, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-wl_screenshot.edj", _mod->dir);
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *cc)
{
   return _gc.name;
}

static void 
_cb_btn_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event;
   if (ev->button == 1)
     {
        if (_timer) ecore_timer_del(_timer);
        _timer = ecore_timer_add(5.0, _cb_timer, NULL);
     }
}

static void 
_cb_handle_global(struct wl_display *disp, unsigned int id, const char *interface, unsigned int version __UNUSED__, void *data)
{
   if (!strcmp(interface, "screenshooter"))
     _shooter = wl_display_bind(disp, id, &screenshooter_interface);
   else if (!strcmp(interface, "wl_output"))
     {
        _output = wl_display_bind(disp, id, &wl_output_interface);
        wl_output_add_listener(_output, &_output_listener, NULL);
     }
}

static struct wl_buffer *
_create_shm_buffer(struct wl_shm *_shm, int width, int height, void **data_out)
{
   char filename[] = "/tmp/wayland-shm-XXXXXX";
   struct wl_buffer *buffer;
   int fd, size, stride;
   void *data;

   fd = mkstemp(filename);
   if (fd < 0) 
     {
        fprintf(stderr, "open %s failed: %m\n", filename);
        return NULL;
     }

   stride = width * 4;
   size = stride * height;
   if (ftruncate(fd, size) < 0) 
     {
        fprintf(stderr, "ftruncate failed: %m\n");
        close(fd);
        return NULL;
     }

   data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   unlink(filename);

   if (data == MAP_FAILED) 
     {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        return NULL;
     }

   buffer = wl_shm_create_buffer(_shm, fd, width, height, stride,
                                 WL_SHM_FORMAT_ARGB8888);

   close(fd);

   *data_out = data;

   return buffer;
}

static void 
_cb_handle_geometry(void *data, struct wl_output *wl_output, int x, int y, int w, int h, int subpixel, const char *make, const char *model)
{

}

static void 
_cb_handle_mode(void *data, struct wl_output *wl_output, unsigned int flags, int w, int h, int refresh)
{
   if (ow == 0) ow = w;
   if (oh == 0) oh = h;
}

static void 
_save_png(int w, int h, void *data)
{
   Ecore_Evas *ee;
   Evas *evas;
   Evas_Object *img;
   char buff[1024];
   char fname[PATH_MAX];

   ee = ecore_evas_buffer_new(w, h);
   evas = ecore_evas_get(ee);

   img = evas_object_image_filled_add(evas);
   evas_object_image_fill_set(img, 0, 0, w, h);
   evas_object_image_size_set(img, w, h);
   evas_object_image_data_set(img, data);
   evas_object_show(img);

   snprintf(fname, sizeof(fname), "%s/screenshot.png", e_user_homedir_get());
   snprintf(buff, sizeof(buff), "quality=90 compress=9");
   if (!(evas_object_image_save(img, fname, NULL, buff)))
     {
        printf("Error saving shot\n");
     }

   if (img) evas_object_del(img);
   if (ee) ecore_evas_free(ee);
}

static Eina_Bool 
_cb_timer(void *data __UNUSED__)
{
   struct wl_buffer *buffer;
   void *d = NULL;

   if (!_shooter) return EINA_FALSE;

   buffer = _create_shm_buffer(ecore_wl_shm_get(), ow, oh, &d);
   screenshooter_shoot(_shooter, _output, buffer);

   ecore_wl_sync();

   printf("Saving Png\n");
   _save_png(ow, oh, d);

   return EINA_FALSE;
}
