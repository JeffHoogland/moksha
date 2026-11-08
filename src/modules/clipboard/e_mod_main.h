#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "common.h"
#include "e_mod_config.h"
#include "history.h"
#include "utility.h"
#include "image.h"

/* Key Board Bindings action names */
#define ACT_FLOAT   _("Show History")
#define ACT_CONFIG  _("Show Settings")
#define ACT_CLEAR   _("Clear History")

/* Macros used for config file versioning */
/* You can increment the EPOCH value if the old configuration is not
 * compatible anymore, it creates an entire new one.
 * You need to increment GENERATION when you add new values to the
 * configuration file but is not needed to delete the existing conf  */
#define MOD_CONFIG_FILE_EPOCH 1
#define MOD_CONFIG_FILE_GENERATION 6
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

/* Setup the E Module Version, Needed to check if module can run. */
/* The version is stored at compilation time in the module, and is checked
 * by E in order to know if the module is compatible with the actual version */
EAPI extern E_Module_Api e_modapi;

/* E API Module Interface Declarations
 *
 * e_modapi_init:     it is called when e17 initialize the module, note that
 *                    a module can be loaded but not initialized (running)
 *                    Note that this is not the same as _gc_init, that is called
 *                    when the module appears on his container
 * e_modapi_shutdown: it is called when e17 is closing, so calling the modules
 *                    to finish
 * e_modapi_save:     this is called when e17 or by another reason is requested
 *                    to save the configuration file                      */
EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m __UNUSED__);
EAPI int   e_modapi_save     (E_Module *m __UNUSED__);

/* Needed elsewhere */
extern Mod_Inst *clip_inst;   /* Defined in e_mod_main.c */

Eina_Bool   cb_clipboard_save(void *data __UNUSED__);
Eet_Error   clip_save(Eina_List *items, Eina_Bool force);

/* ---- History operations (e_mod_main.c), used by the list window ----
 *
 * Items are always referred to by id, never by pointer, when the operation
 * is asynchronous (dialogs): the item may be gone by the time they finish. */
Clip_Data  *clip_find(unsigned int id);          /* NULL if item is gone    */
void        clip_use_item(Clip_Data *cd);        /* put it in the clipboard */
void        clip_add_text(const char *text);     /* add a text item by hand */
void        clip_edit_text(unsigned int id, const char *text);
void        clip_toggle_lock(Clip_Data *cd);
void        clip_delete_request(Clip_Data *cd);  /* asks first if configured */
void        clip_clear_request(void);            /* asks first if configured */
void        clip_labels_refresh(void);           /* after label config change */
void        clip_config_show(void);
/* Frees an item no longer in the list and deletes its image file */
void        clip_discard(Clip_Data *cd);

/* ---- The history window (e_mod_list.c) ---- */
void        clip_list_toggle(void);
void        clip_list_show(void);
void        clip_list_close(Eina_Bool now);
/* Rebuild the list. Does nothing if the window is not open. */
void        clip_list_refresh(void);

#endif
