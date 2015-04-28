#include "e.h"
#include "e_mod_main.h"

#define E_EVDEV_ABSOLUTE_MOTION (1 << 0)
#define E_EVDEV_ABSOLUTE_MT_DOWN (1 << 1)
#define E_EVDEV_ABSOLUTE_MT_MOTION (1 << 2)
#define E_EVDEV_ABSOLUTE_MT_UP (1 << 3)
#define E_EVDEV_RELATIVE_MOTION (1 << 4)

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x) ((x)%BITS_PER_LONG)
#define BIT(x) (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define TEST_BIT(array, bit) ((array[LONG(bit)] >> OFF(bit)) & 1)

#define MODIFIER_CTRL (1 << 8)
#define MODIFIER_ALT (1 << 9)
#define MODIFIER_SUPER (1 << 10)

/* local function prototypes */
static Eina_Bool _e_evdev_config_udev_monitor(struct udev *udev, E_Evdev_Input *master);
static E_Evdev_Input_Device *_e_evdev_input_device_create(E_Evdev_Input *master, struct wl_display *display __UNUSED__, const char *path);
static Eina_Bool _e_evdev_configure_device(E_Evdev_Input_Device *dev);
static Eina_Bool _e_evdev_is_motion_event(struct input_event *e);
static void _e_evdev_flush_motion(E_Evdev_Input_Device *dev, unsigned int timestamp);
static void _e_evdev_process_touch(E_Evdev_Input_Device *device, struct input_event *e);
static void _e_evdev_process_events(E_Evdev_Input_Device *device, struct input_event *ev, int count);

static int _e_evdev_cb_udev(int fd __UNUSED__, unsigned int mask __UNUSED__, void *data);
static void _e_evdev_cb_device_added(struct udev_device *dev, E_Evdev_Input *master);
static void _e_evdev_cb_device_removed(struct udev_device *dev, E_Evdev_Input *master);
static int _e_evdev_cb_device_data(int fd, unsigned int mask __UNUSED__, void *data);

/* local variables */
/* wayland interfaces */
/* external variables */

EINTERN void 
e_evdev_add_devices(struct udev *udev, E_Input_Device *base)
{
   E_Evdev_Input *input;
   struct udev_enumerate *ue;
   struct udev_list_entry *entry;
   struct udev_device *device;
   const char *path = NULL;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   input = (E_Evdev_Input *)base;
   ue = udev_enumerate_new(udev);
   udev_enumerate_add_match_subsystem(ue, "input");
   udev_enumerate_scan_devices(ue);
   udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(ue))
     {
        path = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(udev, path);
        if (strncmp("event", udev_device_get_sysname(device), 5) != 0)
          continue;
        _e_evdev_cb_device_added(device, input);
        udev_device_unref(device);
     }
   udev_enumerate_unref(ue);

   if (wl_list_empty(&input->devices))
     printf("No Input Devices Found\n");
}

EINTERN void 
e_evdev_remove_devices(E_Input_Device *base)
{
   E_Evdev_Input *input;
   E_Evdev_Input_Device *device, *next;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   input = (E_Evdev_Input *)base;
   wl_list_for_each_safe(device, next, &input->devices, link)
     {
        wl_event_source_remove(device->source);
        wl_list_remove(&device->link);
        if (device->mtdev) mtdev_close_delete(device->mtdev);
        close(device->fd);
        free(device->devnode);
        free(device);
     }
}

EINTERN void 
e_evdev_input_create(E_Compositor *comp, struct udev *udev, const char *seat)
{
   E_Evdev_Input *input;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = malloc(sizeof(E_Evdev_Input)))) return;
   memset(input, 0, sizeof(E_Evdev_Input));
   e_input_device_init(&input->base, comp);
   wl_list_init(&input->devices);
   input->seat = strdup(seat);
   if (!_e_evdev_config_udev_monitor(udev, input))
     {
        free(input->seat);
        free(input);
        return;
     }
   e_evdev_add_devices(udev, &input->base);
   comp->input_device = &input->base.input_device;
}

EINTERN void 
e_evdev_input_destroy(E_Input_Device *base)
{
   E_Evdev_Input *input;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   input = (E_Evdev_Input *)base;
   e_evdev_remove_devices(base);
   wl_list_remove(&input->base.link);
   free(input->seat);
   free(input);
}

/* local functions */
static Eina_Bool 
_e_evdev_config_udev_monitor(struct udev *udev, E_Evdev_Input *master)
{
   struct wl_event_loop *loop;
   E_Compositor *comp;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = master->base.compositor;
   if (!(master->monitor = udev_monitor_new_from_netlink(udev, "udev")))
     return EINA_FALSE;
   udev_monitor_filter_add_match_subsystem_devtype(master->monitor, "input", NULL);
   if (udev_monitor_enable_receiving(master->monitor))
     return EINA_FALSE;
   loop = wl_display_get_event_loop(comp->display);
   wl_event_loop_add_fd(loop, udev_monitor_get_fd(master->monitor), 
                        WL_EVENT_READABLE, _e_evdev_cb_udev, master);
   return EINA_TRUE;
}

static E_Evdev_Input_Device *
_e_evdev_input_device_create(E_Evdev_Input *master, struct wl_display *display __UNUSED__, const char *path)
{
   E_Evdev_Input_Device *device;
   E_Compositor *comp;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(device = malloc(sizeof(E_Evdev_Input_Device)))) return NULL;
   comp = master->base.compositor;
   device->output = container_of(comp->outputs.next, E_Output, link);
   device->master = master;
   device->is_pad = EINA_FALSE;
   device->is_mt = EINA_FALSE;
   device->mtdev = NULL;
   device->devnode = strdup(path);
   device->mt.slot = -1;
   device->rel.dx = 0;
   device->rel.dy = 0;

   device->fd = open(path, O_RDONLY | O_NONBLOCK);
   if (device->fd < 0) goto err0;

   if (!_e_evdev_configure_device(device)) goto err1;

   if (device->is_mt)
     {
        if (!(device->mtdev = mtdev_new_open(device->fd)))
          printf("mtdev Failed to open device: %s\n", path);
     }

   device->source = 
     wl_event_loop_add_fd(comp->input_loop, device->fd, WL_EVENT_READABLE, 
                          _e_evdev_cb_device_data, device);
   if (!device->source) goto err1;

   wl_list_insert(master->devices.prev, &device->link);

   return device;

err1:
   close(device->fd);

err0:
   free(device->devnode);
   free(device);
   return NULL;
}

static Eina_Bool 
_e_evdev_configure_device(E_Evdev_Input_Device *dev)
{
   struct input_absinfo absinfo;
   unsigned long ebits[NBITS(EV_MAX)];
   unsigned long abits[NBITS(ABS_MAX)];
   unsigned long kbits[NBITS(KEY_MAX)];
   Eina_Bool has_key = EINA_FALSE, has_abs = EINA_FALSE;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ioctl(dev->fd, EVIOCGBIT(0, sizeof(ebits)), ebits);
   if (TEST_BIT(ebits, EV_ABS))
     {
        has_abs = EINA_TRUE;
        ioctl(dev->fd, EVIOCGBIT(EV_ABS, sizeof(abits)), abits);
        if (TEST_BIT(abits, ABS_X))
          {
             ioctl(dev->fd, EVIOCGABS(ABS_X), &absinfo);
             dev->absolute.min_x = absinfo.minimum;
             dev->absolute.max_x = absinfo.maximum;
          }
        if (TEST_BIT(abits, ABS_Y))
          {
             ioctl(dev->fd, EVIOCGABS(ABS_Y), &absinfo);
             dev->absolute.min_y = absinfo.minimum;
             dev->absolute.max_y = absinfo.maximum;
          }
        if (TEST_BIT(abits, ABS_MT_SLOT))
          {
             dev->is_mt = EINA_TRUE;
             dev->mt.slot = 0;
          }
     }
   if (TEST_BIT(ebits, EV_KEY))
     {
        has_key = EINA_TRUE;
        ioctl(dev->fd, EVIOCGBIT(EV_KEY, sizeof(kbits)), kbits);
        if ((TEST_BIT(kbits, BTN_TOOL_FINGER)) && 
            (!TEST_BIT(kbits, BTN_TOOL_PEN)))
          dev->is_pad = EINA_TRUE;
     }

   if ((has_abs) && (!has_key)) return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool 
_e_evdev_is_motion_event(struct input_event *e)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   switch (e->type) 
     {
      case EV_REL:
        switch (e->code) 
          {
           case REL_X:
           case REL_Y:
             return EINA_TRUE;
          }
      case EV_ABS:
        switch (e->code) 
          {
           case ABS_X:
           case ABS_Y:
           case ABS_MT_POSITION_X:
           case ABS_MT_POSITION_Y:
             return EINA_TRUE;
          }
     }

   return EINA_FALSE;
}

static void 
_e_evdev_flush_motion(E_Evdev_Input_Device *dev, unsigned int timestamp)
{
   struct wl_input_device *master;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!dev->type) return;
   master = &dev->master->base.input_device;
   if (dev->type & E_EVDEV_RELATIVE_MOTION)
     {
        e_input_notify_motion(master, timestamp, 
                              master->x + dev->rel.dx, 
                              master->y + dev->rel.dy);
        dev->type &= ~E_EVDEV_RELATIVE_MOTION;
        dev->rel.dx = 0;
        dev->rel.dy = 0;
     }
   if (dev->type & E_EVDEV_ABSOLUTE_MT_DOWN)
     {
        e_input_notify_touch(master, timestamp, dev->mt.slot, 
                             dev->mt.x[dev->mt.slot], 
                             dev->mt.y[dev->mt.slot],
                             WL_INPUT_DEVICE_TOUCH_DOWN);
        dev->type &= ~E_EVDEV_ABSOLUTE_MT_DOWN;
        dev->type &= ~E_EVDEV_ABSOLUTE_MT_MOTION;
     }
   if (dev->type & E_EVDEV_ABSOLUTE_MT_MOTION)
     {
        e_input_notify_touch(master, timestamp, dev->mt.slot, 
                             dev->mt.x[dev->mt.slot], 
                             dev->mt.y[dev->mt.slot],
                             WL_INPUT_DEVICE_TOUCH_MOTION);
        dev->type &= ~E_EVDEV_ABSOLUTE_MT_DOWN;
        dev->type &= ~E_EVDEV_ABSOLUTE_MT_MOTION;
     }
   if (dev->type & E_EVDEV_ABSOLUTE_MT_UP)
     {
        e_input_notify_touch(master, timestamp, dev->mt.slot, 0, 0, 
                             WL_INPUT_DEVICE_TOUCH_UP);
        dev->type &= ~E_EVDEV_ABSOLUTE_MT_UP;
     }
   if (dev->type & E_EVDEV_ABSOLUTE_MOTION)
     {
        e_input_notify_motion(master, timestamp, 
                              dev->absolute.x, dev->absolute.y);
        dev->type &= ~E_EVDEV_ABSOLUTE_MOTION;
     }
}

static inline void
_e_evdev_process_key(E_Evdev_Input_Device *device, struct input_event *e, int timestamp)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (e->value == 2) return;

   switch (e->code) 
     {
      case BTN_TOOL_PEN:
      case BTN_TOOL_RUBBER:
      case BTN_TOOL_BRUSH:
      case BTN_TOOL_PENCIL:
      case BTN_TOOL_AIRBRUSH:
      case BTN_TOOL_FINGER:
      case BTN_TOOL_MOUSE:
      case BTN_TOOL_LENS:
        if (device->is_pad)
          {
             device->absolute.rx = 1;
             device->absolute.ry = 1;
          }
        break;
      case BTN_TOUCH:
        if (device->is_mt) break;
        e->code = BTN_LEFT;
      case BTN_LEFT:
      case BTN_RIGHT:
      case BTN_MIDDLE:
      case BTN_SIDE:
      case BTN_EXTRA:
      case BTN_FORWARD:
      case BTN_BACK:
      case BTN_TASK:
        e_input_notify_button(&device->master->base.input_device, 
                              timestamp, e->code, e->value);
        break;
      default:
        e_input_notify_key(&device->master->base.input_device, 
                           timestamp, e->code, e->value);
        break;
     }
}

static inline void
_e_evdev_process_absolute_motion(E_Evdev_Input_Device *device, struct input_event *e)
{
   int sw, sh;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   sw = device->output->current->w;
   sh = device->output->current->h;

   switch (e->code) 
     {
      case ABS_X:
        device->absolute.x =
          (e->value - device->absolute.min_x) * sw /
          (device->absolute.max_x - device->absolute.min_x) + 
          device->output->x;
        device->type |= E_EVDEV_ABSOLUTE_MOTION;
        break;
      case ABS_Y:
        device->absolute.y =
          (e->value - device->absolute.min_y) * sh /
          (device->absolute.max_y - device->absolute.min_y) +
          device->output->y;
        device->type |= E_EVDEV_ABSOLUTE_MOTION;
        break;
     }
}

static inline void
_e_evdev_process_absolute_motion_touchpad(E_Evdev_Input_Device *device, struct input_event *e)
{
   /* FIXME: Make this configurable. */
   const int touchpad_speed = 700;

   switch (e->code) 
     {
      case ABS_X:
        e->value -= device->absolute.min_x;
        if (device->absolute.rx)
          device->absolute.rx = 0;
        else 
          {
             device->rel.dx =
               (e->value - device->absolute.ox) * touchpad_speed /
               (device->absolute.max_x - device->absolute.min_x);
          }
        device->absolute.ox = e->value;
        device->type |= E_EVDEV_RELATIVE_MOTION;
        break;
      case ABS_Y:
        e->value -= device->absolute.min_y;
        if (device->absolute.ry)
          device->absolute.ry = 0;
        else 
          {
             device->rel.dy =
               (e->value - device->absolute.oy) * touchpad_speed /
               (device->absolute.max_y - device->absolute.min_y);
          }
        device->absolute.oy = e->value;
        device->type |= E_EVDEV_RELATIVE_MOTION;
        break;
     }
}

static inline void 
_e_evdev_process_relative(E_Evdev_Input_Device *device, struct input_event *e, unsigned int timestamp)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   switch (e->code)
     {
      case REL_X:
        device->rel.dx += e->value;
        device->type |= E_EVDEV_RELATIVE_MOTION;
        break;
      case REL_Y:
        device->rel.dy += e->value;
        device->type |= E_EVDEV_RELATIVE_MOTION;
        break;
      case REL_WHEEL:
        e_input_notify_axis(&device->master->base.input_device, timestamp, 
                            WL_INPUT_DEVICE_AXIS_VERTICAL_SCROLL, e->value);
        break;
      case REL_HWHEEL:
        e_input_notify_axis(&device->master->base.input_device, timestamp, 
                            WL_INPUT_DEVICE_AXIS_HORIZONTAL_SCROLL, e->value);
        break;
     }
}

static inline void 
_e_evdev_process_absolute(E_Evdev_Input_Device *device, struct input_event *e)
{
   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (device->is_pad)
     _e_evdev_process_absolute_motion_touchpad(device, e);
   else if (device->is_mt)
     _e_evdev_process_touch(device, e);
   else
     _e_evdev_process_absolute_motion(device, e);
}

static void
_e_evdev_process_touch(E_Evdev_Input_Device *device, struct input_event *e)
{
   int sw, sh;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   sw = device->output->current->w;
   sh = device->output->current->h;

   switch (e->code) 
     {
      case ABS_MT_SLOT:
        device->mt.slot = e->value;
        break;
      case ABS_MT_TRACKING_ID:
        if (e->value >= 0)
          device->type |= E_EVDEV_ABSOLUTE_MT_DOWN;
        else
          device->type |= E_EVDEV_ABSOLUTE_MT_UP;
        break;
      case ABS_MT_POSITION_X:
        device->mt.x[device->mt.slot] =
          (e->value - device->absolute.min_x) * sw /
          (device->absolute.max_x - device->absolute.min_x) +
          device->output->x;
        device->type |= E_EVDEV_ABSOLUTE_MT_MOTION;
        break;
      case ABS_MT_POSITION_Y:
        device->mt.y[device->mt.slot] =
          (e->value - device->absolute.min_y) * sh /
          (device->absolute.max_y - device->absolute.min_y) +
          device->output->y;
        device->type |= E_EVDEV_ABSOLUTE_MT_MOTION;
        break;
     }
}

static void 
_e_evdev_process_events(E_Evdev_Input_Device *device, struct input_event *ev, int count)
{
   struct input_event *e, *end;
   unsigned int timestamp = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   device->type = 0;
   e = ev;
   end = e + count;
   for (e = ev; e < end; e++)
     {
        timestamp = e->time.tv_sec * 1000 + e->time.tv_usec / 1000;
        if (!_e_evdev_is_motion_event(e))
          _e_evdev_flush_motion(device, timestamp);
        switch (e->type)
          {
           case EV_REL:
             _e_evdev_process_relative(device, e, timestamp);
             break;
           case EV_ABS:
             _e_evdev_process_absolute(device, e);
             break;
           case EV_KEY:
             _e_evdev_process_key(device, e, timestamp);
             break;
          }
     }
   _e_evdev_flush_motion(device, timestamp);
}

static int 
_e_evdev_cb_udev(int fd __UNUSED__, unsigned int mask __UNUSED__, void *data)
{
   E_Evdev_Input *master;
   struct udev_device *dev;
   const char *action;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(master = data)) return 1;
   if (!(dev = udev_monitor_receive_device(master->monitor)))
     return 1;
   if ((action = udev_device_get_action(dev)))
     {
        if (strncmp("event", udev_device_get_sysname(dev), 5) != 0)
          return 0;
        if (!strcmp(action, "add"))
          _e_evdev_cb_device_added(dev, master);
        else if (!strcmp(action, "remove"))
          _e_evdev_cb_device_removed(dev, master);
     }
   udev_device_unref(dev);
   return 0;
}

static void 
_e_evdev_cb_device_added(struct udev_device *dev, E_Evdev_Input *master)
{
   E_Compositor *comp;
   const char *node, *seat;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(seat = udev_device_get_property_value(dev, "ID_SEAT")))
     seat = "seat0";
   if (strcmp(seat, master->seat)) return;
   comp = master->base.compositor;
   node = udev_device_get_devnode(dev);
   _e_evdev_input_device_create(master, comp->display, node);
}

static void 
_e_evdev_cb_device_removed(struct udev_device *dev, E_Evdev_Input *master)
{
   const char *node;
   E_Evdev_Input_Device *device, *next;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   node = udev_device_get_devnode(dev);
   wl_list_for_each_safe(device, next, &master->devices, link)
     {
        if (!strcmp(device->devnode, node))
          {
             wl_event_source_remove(device->source);
             wl_list_remove(&device->link);
             if (device->mtdev) mtdev_close_delete(device->mtdev);
             close(device->fd);
             free(device->devnode);
             free(device);
             break;
          }
     }
}

static int 
_e_evdev_cb_device_data(int fd, unsigned int mask __UNUSED__, void *data)
{
   E_Compositor *comp;
   E_Evdev_Input_Device *device;
   struct input_event ev[32];
   int len = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   device = data;
   comp = device->master->base.compositor;
   if (!comp->focus) return 1;

   do
     {
        if (device->mtdev)
          len = mtdev_get(device->mtdev, fd, ev, 
                          (sizeof(ev) / sizeof(ev)[0]) * 
                          sizeof(struct input_event));
        else
          len = read(fd, &ev, sizeof(ev));

        if ((len < 0) || (len % sizeof(ev[0]) != 0))
          return 1;

        printf("Process Input Events Len: %d\n", len);

        _e_evdev_process_events(device, ev, (len / sizeof(ev[0])));

     } while (len > 0);

   /* len = read(fd, &ev, sizeof(ev)); */

   /* device->type = 0; */
   /* e = ev; */
   /* end = (void *)ev + len; */
   /* for (e = ev; e < end; e++) */
   /*   { */
   /*      timestamp = (e->time.tv_sec * 1000 + e->time.tv_usec / 1000); */
   /*      if (!_e_evdev_is_motion_event(e)) */
   /*        _e_evdev_flush_motion(device, timestamp); */
   /*      switch (e->type) */
   /*        { */
   /*         case EV_REL: */
   /*           _e_evdev_process_relative_motion(device, e); */
   /*           break; */
   /*         case EV_ABS: */
   /*           _e_evdev_process_absolute(device, e); */
   /*           break; */
   /*         case EV_KEY: */
   /*           _e_evdev_process_key(device, e, timestamp); */
   /*           break; */
   /*        } */
   /*   } */

   /* _e_evdev_flush_motion(device, timestamp); */

   return 1;
}
