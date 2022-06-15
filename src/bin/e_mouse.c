#include "e.h"

typedef enum
{
   DEVICE_FLAG_NONE,
   DEVICE_FLAG_TOUCHPAD
} Device_Flags;

static char *devstring = NULL;

static void
_handle_dev_prop(int dev_slot, const char *dev, const char *prop, Device_Flags dev_flags)
{
   int num, size;
   Ecore_X_Atom fmt;

   ///////////////////////////////////////////////////////////////////////////
   // libinput devices
   if (!strcmp(prop, "libinput Middle Emulation Enabled"))
     {
        unsigned char cfval = 0;
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if (dev_flags == DEVICE_FLAG_TOUCHPAD)
          cfval = e_config->touch_emulate_middle_button;
        else
          cfval = e_config->mouse_emulate_middle_button;
        if ((val) && (size == 8) && (num == 1) && (cfval != val[0]))
          {
             val[0] = cfval;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "libinput Accel Speed"))
     {
        float cfval = 0.0;
        float *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if (dev_flags == DEVICE_FLAG_TOUCHPAD)
          cfval = e_config->touch_accel;
        else
          cfval = e_config->mouse_accel;
        if ((val) && (size == 32) && (num == 1) && (fabs(cfval - val[0]) >= 0.01))
          {
             val[0] = cfval;
             printf("DEV: change [%s] [%s] -> %1.3f\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "libinput Tapping Enabled"))
     {
        if (dev_flags == DEVICE_FLAG_TOUCHPAD)
          {
             unsigned char *val = ecore_x_input_device_property_get
               (dev_slot, prop, &num, &fmt, &size);
             if ((val) && (size == 8) && (num == 1) &&
                 (e_config->touch_tap_to_click != val[0]))
               {
                  val[0] = e_config->touch_tap_to_click;
                  printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
                  ecore_x_input_device_property_set
                    (dev_slot, prop, val, num, fmt, size);
               }
             free(val);
          }
     }
//   else if (!strcmp(prop, "libinput Tapping Button Mapping Enabled"))
//     {
//        // 1 bool, 0 = LRM, 1 = LMR
//     }
   else if ((!strcmp(prop, "libinput Horizontal Scrolling Enabled")) ||
            (!strcmp(prop, "libinput Horizontal Scroll Enabled")))
     {
        if (dev_flags == DEVICE_FLAG_TOUCHPAD)
          {
             unsigned char *val = ecore_x_input_device_property_get
               (dev_slot, prop, &num, &fmt, &size);
             if ((val) && (size == 8) && (num == 1) &&
                 (e_config->touch_scrolling_horiz != val[0]))
               {
                  val[0] = e_config->touch_scrolling_horiz;
                  printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
                  ecore_x_input_device_property_set
                    (dev_slot, prop, val, num, fmt, size);
               }
             free(val);
          }
     }
   else if (!strcmp(prop, "libinput Scroll Method Enabled"))
     {
        if (dev_flags == DEVICE_FLAG_TOUCHPAD)
          {
             // 3 bool, 2 finger, edge, button
             unsigned char *val0 = ecore_x_input_device_property_get
               (dev_slot, "libinput Scroll Methods Available", &num, &fmt, &size);
             if ((val0) && (size == 8) && (num >= 3))
               {
                  unsigned char *val = ecore_x_input_device_property_get
                    (dev_slot, prop, &num, &fmt, &size);
                  if ((val) && (size == 8) && (num >= 3))
                    {
                       unsigned char cf_2finger = 0, cf_edge = 0;

                       if (val0[0]) cf_2finger = e_config->touch_scrolling_2finger;
                       else cf_2finger = val[0];
                       if (val0[1]) cf_edge = e_config->touch_scrolling_edge;
                       else cf_edge = val[1];
                       if ((cf_2finger != val[0]) || (cf_edge != val[1]))
                         {
                            val[0] = cf_2finger;
                            val[1] = cf_edge;
                            printf("DEV: change [%s] [%s] -> %i %i %i\n", dev, prop, val[0], val[1], val[2]);
                            ecore_x_input_device_property_set
                              (dev_slot, prop, val, num, fmt, size);
                         }
                    }
                  free(val);
               }
             free(val0);
          }
     }
   else if (!strcmp(prop, "libinput Natural Scrolling Enabled"))
     {
        unsigned char cfval = 0;
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if (dev_flags == DEVICE_FLAG_TOUCHPAD)
          cfval = e_config->touch_natural_scroll;
        else
          cfval = e_config->mouse_natural_scroll;
        if ((val) && (size == 8) && (num == 1) && (cfval != val[0]))
          {
             val[0] = cfval;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
// not for now - default
//   else if (!strcmp(prop, "libinput Accel Profile Enabled"))
//     {
//        // 1 bool, 0 = adaptive, 1 = flat
//     }
// do via button mapping for now - not sure about this evdev can't do this
//   else if (!strcmp(prop, "libinput Left Handed Enabled"))
//     {
//        // 1 bool
//     }

   ///////////////////////////////////////////////////////////////////////////
   // synaptics devices
   else if (!strcmp(prop, "Synaptics Middle Button Timeout"))
     {
        // 1 int - in ms
        unsigned int *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 32) && (num == 1) &&
            (e_config->touch_emulate_middle_button && (val[0] != 50)))
          {
             val[0] = 50;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Move Speed"))
     {
        float chval[4] = { 0.0 };

        // 4 val float min, max, accel, unused
        chval[0] = 1.0;
        chval[1] = 1.75;
        chval[2] = 0.15 + (e_config->touch_accel * 0.15);
        float *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 32) && (num == 4) &&
            ((fabs(chval[0] - val[0]) >= 0.01) ||
             (fabs(chval[1] - val[1]) >= 0.01) ||
             (fabs(chval[2] - val[2]) >= 0.01)))
          {
             val[0] = chval[0];
             val[1] = chval[1];
             val[2] = chval[2];
             printf("DEV: change [%s] [%s] -> %1.3f %1.3f %1.3f %1.3f\n", dev, prop, val[1], val[1], val[2], val[3]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Tap Action"))
     {
        // 7 val, 8 bit bit 0 = off, >0 mouse button reported
        // TR, BR, TL, BL, F1, F2, F3
        // 1, 1, 1, 0, 1, 3, 2 <- tap to click
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        const char tapval[7]   =  { 1, 1, 1, 0, 1, 3, 2 };
        const char notapval[7] =  { 0, 0, 0, 0, 0, 0, 0 };
        Eina_Bool have_tapval = EINA_FALSE;
        Eina_Bool have_notapval = EINA_FALSE;
        int i;

        if (num >= 7)
          {
             have_tapval = EINA_TRUE;
             for (i = 0; i < 7; i++) if (val[i] != tapval[i]) have_tapval = EINA_FALSE;
             have_notapval = EINA_TRUE;
             for (i = 0; i < 7; i++) if (val[i] != notapval[i]) have_notapval = EINA_FALSE;
          }
        if ((val) && (size == 8) && (num >= 7) &&
            (((e_config->touch_tap_to_click) && (!have_tapval)) ||
             ((!e_config->touch_tap_to_click) && (!have_notapval))))
          {
             if (e_config->touch_tap_to_click)
               {
                  for (i = 0; i < 7; i++) val[i] = tapval[i];
               }
             else
               {
                  for (i = 0; i < 7; i++) val[i] = notapval[i];
               }
             printf("DEV: change [%s] [%s] -> %i %i %i %i %i %i %i\n", dev, prop, val[0], val[1], val[2], val[3], val[4], val[5], val[6]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics ClickPad"))
     {
        // 1 bool
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 8) && (num == 1) &&
            (e_config->touch_clickpad) != (val[0]))
          {
             val[0] = e_config->touch_clickpad;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Edge Scrolling"))
     {
        // 3 bool - v, h, corner
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 8) && (num >= 2) &&
            (((e_config->touch_scrolling_edge) &&
              ((!val[0]) || (val[1] != e_config->touch_scrolling_horiz))) ||
             ((!e_config->touch_scrolling_edge) &&
              ((val[0]) || (val[1])))))
          {
             if (e_config->touch_scrolling_edge)
               {
                  val[0] = 1;
                  val[1] = e_config->touch_scrolling_horiz;
               }
             else
               {
                  val[0] = 0;
                  val[1] = 0;
               }
             printf("DEV: change [%s] [%s] -> %i %i\n", dev, prop, val[0], val[1]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Two-Finger Scrolling"))
     {
        // 2 bool - h, v
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 8) && (num >= 2) &&
            (((e_config->touch_scrolling_2finger) &&
              ((!val[0]) || (val[1] != e_config->touch_scrolling_horiz))) ||
             ((!e_config->touch_scrolling_2finger) &&
              ((val[0]) || (val[1])))))
          {
             if (e_config->touch_scrolling_2finger)
               {
                  val[0] = 1;
                  val[1] = e_config->touch_scrolling_horiz;
               }
             else
               {
                  val[0] = 0;
                  val[1] = 0;
               }
             printf("DEV: change [%s] [%s] -> %i %i\n", dev, prop, val[0], val[1]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Circular Scrolling"))
     {
        // 1 bool
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 8) && (num == 1) &&
            (e_config->touch_scrolling_circular != val[0]))
          {
             val[0] = e_config->touch_scrolling_circular;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Palm Detection"))
     {
        // 1 bool - always turn on
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 8) && (num == 1) &&
            (e_config->touch_palm_detect) != (val[0]))
          {
             val[0] = e_config->touch_palm_detect;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Synaptics Scrolling Distance"))
     {
        // 2 ints v, h - invert them for natural (negative)
        int *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 32) && (num == 2) &&
            (((e_config->touch_natural_scroll && ((val[0] > 0) || (val[1] > 0)))) ||
             ((!e_config->touch_natural_scroll && ((val[0] < 0) || (val[1] < 0))))))
          {
             if (e_config->mouse_natural_scroll)
               {
                  if (val[0] > 0) val[0] = -val[0];
                  if (val[1] > 0) val[1] = -val[1];
               }
             else
               {
                  if (val[0] < 0) val[0] = -val[0];
                  if (val[1] < 0) val[1] = -val[1];
               }
             printf("DEV: change [%s] [%s] -> %i %i\n", dev, prop, val[0], val[1]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
//   else if (!strcmp(prop, "Synaptics Off"))
//     {
//        // 8 bit 0 = on, 1 = off (except physical clicks), 2 =
//     }

   ///////////////////////////////////////////////////////////////////////////
   // evdev devices
   else if (!strcmp(prop, "Evdev Middle Button Emulation"))
     {
        unsigned char *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 8) && (num == 1) &&
            (e_config->mouse_emulate_middle_button) != (val[0]))
          {
             val[0] = e_config->mouse_emulate_middle_button;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
   else if (!strcmp(prop, "Evdev Middle Button Timeout"))
     {
        unsigned short *val = ecore_x_input_device_property_get
          (dev_slot, prop, &num, &fmt, &size);
        if ((val) && (size == 16) && (num == 1) &&
            (e_config->mouse_emulate_middle_button && (val[0] != 50)))
          {
             val[0] = 50;
             printf("DEV: change [%s] [%s] -> %i\n", dev, prop, val[0]);
             ecore_x_input_device_property_set
               (dev_slot, prop, val, num, fmt, size);
          }
        free(val);
     }
}

static void
devices_apply(Eina_Bool force)
{
   int num_devs, i;
   Eina_Bool driver_evdev = EINA_FALSE;
   Eina_Bool driver_libinput = EINA_FALSE;
   Eina_Bool driver_synaptics = EINA_FALSE;
   Eina_Strbuf *sbuf;
   Eina_Bool changed = EINA_TRUE;

   num_devs = ecore_x_input_device_num_get();
   sbuf = eina_strbuf_new();
   if (!sbuf) return;
   for (i = 0; i < num_devs; i++)
     {
        eina_strbuf_append(sbuf, ecore_x_input_device_name_get(i));
     }
   if ((devstring) && (!strcmp(devstring, eina_strbuf_string_get(sbuf))))
     changed = EINA_FALSE;
   else
     {
        free(devstring);
        devstring = eina_strbuf_string_steal(sbuf);
     }
   eina_strbuf_free(sbuf);
   changed |= force;
   printf("DEV: CHANGES ... have %i devices, changed=%i\n", num_devs, changed);
   if (!changed) return;
   for (i = 0; i < num_devs; i++)
     {
        const char *name;
        char **props;
        int num_props, j;
        Device_Flags dev_flags = 0;

        name = ecore_x_input_device_name_get(i);
//        printf("DEV: DEV=%i: [%s]\n", i, name);
        props = ecore_x_input_device_properties_list(i, &num_props);
        if (props)
          {
             int num, size;
             Ecore_X_Atom fmt;
             unsigned char *val;

             // figure out device flags - for now is it a mouse or touchpad
             val = ecore_x_input_device_property_get
               (i, "libinput Scroll Methods Available", &num, &fmt, &size);
             if ((val) && (size == 8) && (num >= 3))
               {
                  if ((!val[2]) && (val[0] || val[1]))
                    {
                       dev_flags = DEVICE_FLAG_TOUCHPAD;
                    }
               }
             if (!val)
               {
                  val = ecore_x_input_device_property_get
                    (i, "Synaptics Move Speed", &num, &fmt, &size);
                  if ((val) && (size == 32) && (num == 4))
                    {
                       dev_flags = DEVICE_FLAG_TOUCHPAD;
                    }
               }
             free(val);

             for (j = 0; j < num_props; j++)
               {
//                  printf("DEV:   PROP=%i: [%s]\n", j, props[j]);
                  if ((!driver_evdev) && (!strncmp(props[j], "Evdev ", 6)))
                    driver_evdev = EINA_TRUE;
                  else if ((!driver_libinput) && (!strncmp(props[j], "libinput ", 9)))
                    driver_libinput = EINA_TRUE;
                  else if ((!driver_synaptics) && (!strncmp(props[j], "Synaptics ", 10)))
                    driver_synaptics = EINA_TRUE;
                  _handle_dev_prop(i, name, props[j], dev_flags);
               }
             ecore_x_input_device_properties_free(props, num_props);
          }
     }

   // generic accel - if synatics or evdev props found?
     {
        int accel_numerator, accel_denominator;

        accel_numerator = 20 + (e_config->mouse_accel * 20.0);
        accel_denominator = 10;
        ecore_x_pointer_control_set(accel_numerator, accel_denominator,
                                    e_config->mouse_accel_threshold);
     }

   // button mapping - for mouse hand and natural scroll
     {
        unsigned char map[256] = { 0 };
        int n;

        if (ecore_x_pointer_mapping_get(map, 256))
          {
             for (n = 0; n < 256; n++)
               {
                  if (!map[n]) break;
               }
             if (n < 3)
               {
                  map[0]  = 1;
                  map[1]  = 2;
                  map[2]  = 3;
                  n = 3;
               }
             if (e_config->mouse_hand == E_MOUSE_HAND_RIGHT)
               {
                  map[0] = 1;
                  map[2] = 3;
               }
             else if (e_config->mouse_hand == E_MOUSE_HAND_LEFT)
               {
                  map[0] = 3;
                  map[2] = 1;
               }
             // if we are not on libinput or synaptics drivers anywehre then
             // swap buttons the old fashioned way
             if ((n >= 5) && (!driver_libinput) && (!driver_synaptics))
               {
                  if (e_config->mouse_natural_scroll)
                    {
                       map[3] = 5;
                       map[4] = 4;
                    }
                  else
                    {
                       map[3] = 4;
                       map[4] = 5;
                    }
               }
             ecore_x_pointer_mapping_set(map, n);
          }
     }
}

EAPI int
e_mouse_update(void)
{
    devices_apply(EINA_TRUE);
    return 1;
}
