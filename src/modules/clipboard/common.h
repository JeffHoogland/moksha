#ifndef CLIPBOARD_COMMON_H
#define CLIPBOARD_COMMON_H

#include <string.h>
#include <stdint.h>
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

/* Images */
#define IMG_THUMB_SIZE   64   /* thumbnail size in the history list (pixels) */
#define MAGIC_IMG_MAX_MB  8   /* default limit for a single image            */
#define IMG_MB_MIN        1
#define IMG_MB_MAX       32

/* Stuff for convenience to compress code */
#define IF_TRUE_RETURN(exp)             \
do {                                    \
  if (exp) return;                      \
} while(0)

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

typedef enum _Clip_Type
{
    CLIP_TYPE_TEXT  = 0,   /* must stay 0: E_NEW() zeroes new items */
    CLIP_TYPE_IMAGE = 1
} Clip_Type;

typedef struct _Clip_Data
{
    /* A structure used for storing clipboard data in */
    unsigned int id;     /* unique during the session, never dereferenced
                          *  pointers are NOT used to refer to items from
                          *  asynchronous dialogs, ids are.               */
    Clip_Type    type;
    char        *name;   /* label shown in the list                      */
    char        *content;/* TEXT:  the text itself
                          * IMAGE: image id (see image.h), the PNG itself
                          *        lives in a file in the image cache     */
    char        *lock;   /* "L" locked, "U" unlocked                     */
    /* IMAGE only */
    unsigned int w, h;
    size_t       size;   /* size of the PNG file in bytes                */
} Clip_Data;

typedef struct _Instance Instance;
struct _Instance
{
    /* An instance of our module with its elements */

    /* pointer to this gadget's container */
    E_Gadcon_Client *gcc;
    /* Pointer to the gadget's context (right click) menu */
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

    /* A pointer to an Ecore window used to
     * recieve or send clipboard events to */
    Ecore_X_Window win;

    /* Callback function to handle clipboard events */
    Eina_List *handle;
    /* Stores Clipboard History, newest item first */
    Eina_List *items;
    /* History update Flag */
    Eina_Bool update_history;
    /* Timer callback function to save Clipboard History */
    Ecore_Timer *save_timer;
};

void         free_clip_data(Clip_Data *clip);
unsigned int clip_id_next(void);
#endif
