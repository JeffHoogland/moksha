#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Ipc.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/shape.h>
#include <X11/keysym.h>

#define WINDOW_WIDTH   320
#define WINDOW_HEIGHT  240

#ifndef XCB_ATOM_NONE
# define XCB_ATOM_NONE 0
#endif

/* local function prototypes */
static int           _e_alert_connect(void);
static void          _e_alert_create(void);
static void          _e_alert_display(void);
static void          _e_alert_button_move_resize(xcb_window_t btn, int x, int y, int w, int h);
static void          _e_alert_window_raise(xcb_window_t win);
static void          _e_alert_sync(void);
static void          _e_alert_shutdown(void);
static void          _e_alert_run(void);
static void          _e_alert_draw(void);
static int           _e_alert_handle_key_press(xcb_generic_event_t *event);
static int           _e_alert_handle_button_press(xcb_generic_event_t *event);
static xcb_char2b_t *_e_alert_build_string(const char *str);
static void          _e_alert_draw_outline(void);
static void          _e_alert_draw_title_outline(void);
static void          _e_alert_draw_title(void);
static void          _e_alert_draw_text(void);
static void          _e_alert_draw_button_outlines(void);
static void          _e_alert_draw_button_text(void);

/* local variables */
static xcb_connection_t *conn = NULL;
static xcb_screen_t *screen = NULL;
static xcb_window_t win = 0, comp_win = 0;
static xcb_window_t btn1 = 0;
static xcb_window_t btn2 = 0;
static xcb_font_t font = 0;
static xcb_gcontext_t gc = 0;
static int sw = 0, sh = 0;
static int fa = 0, fh = 0, fw = 0;
static const char *title = NULL, *str1 = NULL, *str2 = NULL;
static int ret = 0, sig = 0;
static pid_t pid;
static Eina_Bool tainted = EINA_TRUE;
static const char *backtrace_str = NULL;
static int exit_gdb = 0;

int
main(int argc, char **argv)
{
   const char *tmp;
   int i = 0;

   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "-h")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "--help")))
          {
             printf("This is an internal tool for Enlightenment.\n"
                    "do not use it.\n");
             exit(0);
          }
        else if (i == 1)
          sig = atoi(argv[i]);  // signal
        else if (i == 2)
          pid = atoi(argv[i]);  // E's pid
        else if (i == 3)
          backtrace_str = argv[i];
	else if (i == 4)
	  exit_gdb = atoi(argv[i]);
     }

   fprintf(stderr, "exit_gdb: %i\n", exit_gdb);

   tmp = getenv("E17_TAINTED");
   if (tmp && !strcmp(tmp, "NO"))
     tainted = EINA_FALSE;

   if (!ecore_init()) return EXIT_FAILURE;
   ecore_app_args_set(argc, (const char **)argv);

   if (!_e_alert_connect())
     {
        printf("FAILED TO INIT ALERT SYSTEM!!!\n");
        ecore_shutdown();
        return EXIT_FAILURE;
     }

   title = "Enlightenment Error";
   str1 = "(F1) Recover";
   str2 = "(F12) Logout";

   _e_alert_create();
   _e_alert_display();
   _e_alert_run();
   _e_alert_shutdown();

   ecore_shutdown();

   /* ret == 1 => restart e => exit code 1 */
   /* ret == 2 => exit e => any code will do that */
   return ret;
}

/* local functions */
static int
_e_alert_connect(void)
{
   conn = xcb_connect(NULL, NULL);
   if ((!conn) || (xcb_connection_has_error(conn)))
     {
        printf("E_Alert_Main: Error Trying to Connect!!\n");
        return 0;
     }

   /* grab default screen */
   screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
   sw = screen->width_in_pixels;
   sh = screen->height_in_pixels;

   return 1;
}

static void
_e_alert_create(void)
{
   uint32_t mask, mask_list[4];
   int wx = 0, wy = 0;

   wx = ((sw - WINDOW_WIDTH) / 2);
   wy = ((sh - WINDOW_HEIGHT) / 2);

   font = xcb_generate_id(conn);
   xcb_open_font(conn, font, strlen("fixed"), "fixed");

   /* create main window */
   mask = (XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
           XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK);
   mask_list[0] = screen->white_pixel;
   mask_list[1] = screen->black_pixel;
   mask_list[2] = 1;
   mask_list[3] = (XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                   XCB_EVENT_MASK_EXPOSURE);

   win = xcb_generate_id(conn);
   xcb_create_window(conn, XCB_COPY_FROM_PARENT, win, screen->root,
                     wx, wy, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                     XCB_WINDOW_CLASS_INPUT_OUTPUT,
                     XCB_COPY_FROM_PARENT, mask, mask_list);

   /* create button 1 */
   mask_list[3] = (XCB_EVENT_MASK_BUTTON_PRESS |
                   XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_EXPOSURE);

   btn1 = xcb_generate_id(conn);
   xcb_create_window(conn, XCB_COPY_FROM_PARENT, btn1, win,
                     -100, -100, 1, 1, 0,
                     XCB_WINDOW_CLASS_INPUT_OUTPUT,
                     XCB_COPY_FROM_PARENT, mask, mask_list);
   xcb_map_window(conn, btn1);

   /* create button 2 */
   btn2 = xcb_generate_id(conn);
   xcb_create_window(conn, XCB_COPY_FROM_PARENT, btn2, win,
                     -100, -100, 1, 1, 0,
                     XCB_WINDOW_CLASS_INPUT_OUTPUT,
                     XCB_COPY_FROM_PARENT, mask, mask_list);
   xcb_map_window(conn, btn2);

   /* create drawing gc */
   mask = (XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT);
   mask_list[0] = screen->black_pixel;
   mask_list[1] = screen->white_pixel;
   mask_list[2] = font;

   gc = xcb_generate_id(conn);
   xcb_create_gc(conn, gc, win, mask, mask_list);
}

static int
_e_alert_atom_get(const char *name)
{
   xcb_intern_atom_cookie_t cookie;
   xcb_intern_atom_reply_t *reply;
   int a;

   cookie = xcb_intern_atom_unchecked(conn, 0, strlen(name), name);
   reply = xcb_intern_atom_reply(conn, cookie, NULL);
   if (!reply) return XCB_ATOM_NONE;
   a = reply->atom;
   free(reply);
   return a;
}

static xcb_window_t
_e_alert_comp_win_get(void)
{
   xcb_get_property_cookie_t cookie;
   xcb_get_property_reply_t *reply;
   uint32_t *v;
   int atom_cardinal, atom_composite_win;
   xcb_window_t r = 0;

   atom_cardinal = _e_alert_atom_get("CARDINAL");
   atom_composite_win = _e_alert_atom_get("_E_COMP_WINDOW");

   cookie = xcb_get_property_unchecked(conn, 0, screen->root, atom_composite_win,
                                       atom_cardinal, 0, 0x7fffffff);
   reply = xcb_get_property_reply(conn, cookie, NULL);
   if (!reply) return 0;

   v = xcb_get_property_value(reply);
   if (v) r = v[0];

   free(reply);
   return r;
}

static Eina_Bool
_e_alert_root_tainted_get(void)
{
   xcb_get_property_cookie_t cookie;
   xcb_get_property_reply_t *reply;
   uint32_t *v;
   int atom_cardinal, atom_tainted;
   int r = 0;

   atom_cardinal = _e_alert_atom_get("CARDINAL");
   atom_tainted = _e_alert_atom_get("_E_TAINTED");

   cookie = xcb_get_property_unchecked(conn, 0, screen->root, atom_tainted,
                                       atom_cardinal, 0, 0x7fffffff);
   reply = xcb_get_property_reply(conn, cookie, NULL);
   if (!reply) return EINA_TRUE;

   v = xcb_get_property_value(reply);
   if (v) r = v[0];

   free(reply);
   return !!r;
}

static void
_e_alert_display(void)
{
   xcb_char2b_t *str = NULL;
   xcb_query_text_extents_cookie_t cookie;
   xcb_query_text_extents_reply_t *reply;
   int x = 0, w = 0;

   tainted = _e_alert_root_tainted_get();

   str = _e_alert_build_string(title);

   cookie =
     xcb_query_text_extents_unchecked(conn, font, strlen(title), str);
   reply = xcb_query_text_extents_reply(conn, cookie, NULL);
   if (reply)
     {
        fa = reply->font_ascent;
        fh = (fa + reply->font_descent);
        fw = reply->overall_width;
        free(reply);
     }
   free(str);

   /* move buttons */
   x = 20;
   w = (WINDOW_WIDTH / 2) - 40;
   _e_alert_button_move_resize(btn1, x, WINDOW_HEIGHT - 20 - (fh + 20),
                               w, (fh + 20));

   x = ((WINDOW_WIDTH / 2) + 20);
   _e_alert_button_move_resize(btn2, x, WINDOW_HEIGHT - 20 - (fh + 20),
                               w, (fh + 20));

   comp_win = _e_alert_comp_win_get();
   if (comp_win)
     {
        xcb_rectangle_t rect;
        int wx = 0, wy = 0;

        wx = ((sw - WINDOW_WIDTH) / 2);
        wy = ((sh - WINDOW_HEIGHT) / 2);

        rect.x = wx;
        rect.y = wy;
        rect.width = WINDOW_WIDTH;
        rect.height = WINDOW_HEIGHT;

        xcb_shape_rectangles(conn, XCB_SHAPE_SO_SET,
                             XCB_SHAPE_SK_INPUT, XCB_CLIP_ORDERING_UNSORTED,
                             comp_win, 0, 0, 1, &rect);

        xcb_reparent_window(conn, win, comp_win, wx, wy);
     }

   /* map and raise main window */
   xcb_map_window(conn, win);
   _e_alert_window_raise(win);

   /* grab pointer & keyboard */
   xcb_grab_pointer_unchecked(conn, 0, win,
                              (XCB_EVENT_MASK_BUTTON_PRESS |
                               XCB_EVENT_MASK_BUTTON_RELEASE),
                              XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                              XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
   xcb_grab_keyboard_unchecked(conn, 0, win, XCB_CURRENT_TIME,
                               XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
   xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                       win, XCB_CURRENT_TIME);

   /* flush screen */
   xcb_flush(conn);

   /* sync */
   _e_alert_sync();
}

static void
_e_alert_button_move_resize(xcb_window_t btn, int x, int y, int w, int h)
{
   uint32_t list[4], mask;

   if (!btn) return;

   mask = (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
           XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
   list[0] = x;
   list[1] = y;
   list[2] = w;
   list[3] = h;

   xcb_configure_window(conn, btn, mask, (const uint32_t *)&list);
}

static void
_e_alert_window_raise(xcb_window_t window)
{
   uint32_t list[] = { XCB_STACK_MODE_ABOVE };

   if (!window) return;
   xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE, list);
}

static void
_e_alert_sync(void)
{
   free(xcb_get_input_focus_reply(conn,
                                  xcb_get_input_focus(conn), NULL));
}

static void
_e_alert_shutdown(void)
{
   if (!xcb_connection_has_error(conn))
     {
        xcb_close_font(conn, font);
        xcb_destroy_window(conn, btn1);
        xcb_destroy_window(conn, btn2);
        xcb_destroy_window(conn, win);
        if (comp_win) xcb_destroy_window(conn, comp_win);
        xcb_free_gc(conn, gc);
        xcb_disconnect(conn);
     }
}

static void
_e_alert_run(void)
{
   xcb_generic_event_t *event = NULL;

   xcb_flush(conn);
   while ((event = xcb_wait_for_event(conn)))
     {
        switch (event->response_type & ~0x80)
          {
           case XCB_BUTTON_PRESS:
             ret = _e_alert_handle_button_press(event);
             break;

           case XCB_KEY_PRESS:
             ret = _e_alert_handle_key_press(event);
             break;

           case XCB_EXPOSE:
           {
              xcb_expose_event_t *ev;

              ev = (xcb_expose_event_t *)event;
              if (ev->window != win) break;

              _e_alert_draw();
              _e_alert_sync();

              break;
           }

           default:
             break;
          }
        free(event);
        if (ret > 0) return;
     }
}

static void
_e_alert_draw(void)
{
   _e_alert_draw_outline();
   _e_alert_draw_title_outline();
   _e_alert_draw_title();
   _e_alert_draw_text();
   _e_alert_draw_button_outlines();
   _e_alert_draw_button_text();

   xcb_flush(conn);
}

static int
_e_alert_handle_key_press(xcb_generic_event_t *event)
{
   xcb_key_symbols_t *symbols;
   xcb_key_press_event_t *ev;
   xcb_keysym_t key;
   int r = 0;

   ev = (xcb_key_press_event_t *)event;
   symbols = xcb_key_symbols_alloc(conn);
   key = xcb_key_symbols_get_keysym(symbols, ev->detail, 0);

   if (key == XK_F1)
     r = 1;
   else if (key == XK_F12)
     r = 2;

   xcb_key_symbols_free(symbols);

   return r;
}

static int
_e_alert_handle_button_press(xcb_generic_event_t *event)
{
   xcb_button_press_event_t *ev;

   ev = (xcb_button_press_event_t *)event;
   if (ev->child == btn1)
     return 1;
   else if (ev->child == btn2)
     return 2;
   else
     return 0;
}

static xcb_char2b_t *
_e_alert_build_string(const char *str)
{
   unsigned int i = 0;
   xcb_char2b_t *r = NULL;

   if (!(r = malloc(strlen(str) * sizeof(xcb_char2b_t))))
     return NULL;

   for (i = 0; i < strlen(str); i++)
     {
        r[i].byte1 = 0;
        r[i].byte2 = str[i];
     }

   return r;
}

static void
_e_alert_draw_outline(void)
{
   xcb_rectangle_t rect;

   /* draw outline */
   rect.x = 0;
   rect.y = 0;
   rect.width = (WINDOW_WIDTH - 1);
   rect.height = (WINDOW_HEIGHT - 1);
   xcb_poly_rectangle(conn, win, gc, 1, &rect);
}

static void
_e_alert_draw_title_outline(void)
{
   xcb_rectangle_t rect;

   /* draw title box */
   rect.x = 2;
   rect.y = 2;
   rect.width = (WINDOW_WIDTH - 4 - 1);
   rect.height = (fh + 4 + 2);
   xcb_poly_rectangle(conn, win, gc, 1, &rect);
}

static void
_e_alert_draw_title(void)
{
   int x = 0, y = 0;

   /* draw title */
   x = (2 + 2 + ((WINDOW_WIDTH - 4 - 4 - fw) / 2));
   y = (2 + 2 + fh);

   xcb_image_text_8(conn, strlen(title), win, gc, x, y, title);
}

struct
{
   int         signal;
   const char *name;
} signal_name[5] = {
   { SIGSEGV, "SEGV" },
   { SIGILL, "SIGILL" },
   { SIGFPE, "SIGFPE" },
   { SIGBUS, "SIGBUS" },
   { SIGABRT, "SIGABRT" }
};

static void
_e_alert_draw_text(void)
{
   char warn[1024], msg[4096], line[1024];
   unsigned int i = 0, j = 0, k = 0;

   if (!tainted)
     {
        if (exit_gdb)
          {
             snprintf(msg, sizeof(msg),
                      "This is not meant to happen and is likely a sign of \n"
                      "a bug in Enlightenment or the libraries it relies \n"
                      "on. We were not able to generate a backtrace, check \n"
                      "if your 'sysactions.conf' has an 'gdb' action line.\n"
                      "\n"
                      "Please compile latest svn E17 and EFL with\n"
                      "-g and -ggdb3 in your CFLAGS.\n");
          }
        else if (backtrace_str)
          {
             snprintf(msg, sizeof(msg),
                      "This is not meant to happen and is likely a sign of \n"
                      "a bug in Enlightenment or the libraries it relies \n"
                      "on. You will find an backtrace of E17 (%d) in :\n"
                      "'%s'\n"
                      "Before reporting issue, compile latest E17 and EFL\n"
                      "from svn with '-g -ggdb3' in your CFLAGS.\n"
                      "You can then report this crash on :\n"
                      "http://trac.enlightenment.org/e/.\n",
                      pid, backtrace_str);
          }
        else
          {
             snprintf(msg, sizeof(msg),
                      "This is not meant to happen and is likely a sign of \n"
                      "a bug in Enlightenment or the libraries it relies \n"
                      "on. You can gdb attach to this process (%d) now \n"
                      "to try debug it or you could logout, or just hit \n"
                      "recover to try and get your desktop back the way \n"
                      "it was.\n"
                      "\n"
                      "Please compile latest svn E17 and EFL with\n"
                      "-g and -ggdb3 in your CFLAGS.\n", pid);
          }
     }
   else
     {
        snprintf(msg, sizeof(msg),
                 "This is not meant to happen and is likely\n"
                 "a sign of a bug, but you are using unsupported\n"
                 "modules; before reporting this issue, please\n"
                 "unload them and try to see if the bug is still\n"
                 "there. Also update to latest svn and be sure to\n"
                 "compile E17 and EFL with -g and -ggdb3 in your CFLAGS");
     }

   strcpy(warn, "");

   for (i = 0; i < sizeof(signal_name) / sizeof(signal_name[0]); ++i)
     if (signal_name[i].signal == sig)
       snprintf(warn, sizeof(warn),
                "This is very bad. Enlightenment %s'd.",
                signal_name[i].name);

   /* draw text */
   k = (fh + 12);
   xcb_image_text_8(conn, strlen(warn), win, gc,
                    4, (k + fa), warn);
   k += (2 * (fh + 2));
   for (i = 0; msg[i]; )
     {
        line[j++] = msg[i++];
        if (line[j - 1] == '\n')
          {
             line[j - 1] = 0;
             j = 0;
             xcb_image_text_8(conn, strlen(line), win, gc,
                              4, (k + fa), line);
             k += (fh + 2);
          }
     }
}

static void
_e_alert_draw_button_outlines(void)
{
   xcb_rectangle_t rect;

   rect.x = 0;
   rect.y = 0;
   rect.width = (WINDOW_WIDTH / 2) - 40 - 1;
   rect.height = (fh + 20) - 1;

   /* draw button outlines */
   xcb_poly_rectangle(conn, btn1, gc, 1, &rect);
   xcb_poly_rectangle(conn, btn2, gc, 1, &rect);
}

static void
_e_alert_draw_button_text(void)
{
   xcb_char2b_t *str = NULL;
   xcb_query_text_extents_cookie_t cookie;
   xcb_query_text_extents_reply_t *reply;
   int x = 0, w = 0, bw = 0;

   bw = (WINDOW_WIDTH / 2) - 40 - 1;

   /* draw button1 text */
   str = _e_alert_build_string(str1);

   cookie =
     xcb_query_text_extents_unchecked(conn, font, strlen(str1), str);
   reply = xcb_query_text_extents_reply(conn, cookie, NULL);
   if (reply)
     {
        w = reply->overall_width;
        free(reply);
     }
   free(str);

   x = (5 + ((bw - w) / 2));

   xcb_image_text_8(conn, strlen(str1), btn1, gc, x, (10 + fa), str1);

   /* draw button2 text */
   str = _e_alert_build_string(str2);

   cookie =
     xcb_query_text_extents_unchecked(conn, font, strlen(str2), str);
   reply = xcb_query_text_extents_reply(conn, cookie, NULL);
   if (reply)
     {
        w = reply->overall_width;
        free(reply);
     }
   free(str);

   x = (5 + ((bw - w) / 2));

   xcb_image_text_8(conn, strlen(str2), btn2, gc, x, (10 + fa), str2);
}

