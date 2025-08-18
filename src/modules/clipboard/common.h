#ifndef CLIPBOARD_COMMON_H
#define CLIPBOARD_COMMON_H

#include <string.h>
#include <e.h>
#include "config.h"

#define MAGIC_LABEL_SIZE 50
#define MAGIC_HIST_SIZE  20
#define MAGIC_IGNORE_WS   0
#define LABEL_MIN  5
#define LABEL_MAX  100
#define TIMER_MIN  0
#define TIMER_MAX  240
#define HIST_MIN   5
#define HIST_MAX   100
#define HIST_MAX_DIGITS 3
/* Stuff for convenience to compress code */
#define IF_TRUE_RETURN(exp)             \
do {                                    \
  if (exp) return;                      \
} while(0)

/* Possible Future Config Options */
// #define FC_IGNORE_WHITE_SPACE EINA_TRUE /*Implemented */

/* EINA_LOG support macros and global */
extern int _e_clipboard_log_dom;
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(_e_clipboard_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(_e_clipboard_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(_e_clipboard_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(_e_clipboard_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(_e_clipboard_log_dom, __VA_ARGS__)

typedef struct _Clip_Data
{
    /* A structure used for storing clipboard data in */
    char *name;
    char *content;
    char *lock;
} Clip_Data;

typedef struct _Instance Instance;
struct _Instance
{
    /* An instance of our module with its elements */

    /* pointer to this gadget's container */
    E_Gadcon_Client *gcc;
    /* Pointer to gadget or float menu */
    E_Menu *menu;
    /* Pointer to mouse button object
     * to add call back to */
    Evas_Object *o_button;
};

typedef struct _Mod_Inst Mod_Inst;
struct _Mod_Inst
{
    /* Structure to store a global module instance in
     * complete with a hidden window for event notification purposes */

    Instance *inst;
    /* A pointer to an Ecore window used to
     * recieve or send clipboard events to */
    Ecore_X_Window win;

    /* Callback function to handle clipboard events */
    Eina_List *handle;
    /* Stores Clipboard History */
    Eina_List *items;
    /* History update Flag */
    Eina_Bool update_history;
    /* Timer callback function to save Clipboard History */
    Ecore_Timer *save_timer;
};

void cb_mod_log(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc, int line, const char *fmt, void *data, va_list args);
extern int clipboard_log;
void   free_clip_data(Clip_Data *clip);
#endif
