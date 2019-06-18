#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define HIST_MIN   5
#define HIST_MAX   100

/* We create a structure config for our module, and also a config structure
 * for every item element (you can have multiple gadgets for the same module) */
typedef struct _Config Config;
typedef struct _Config_Item Config_Item;

struct _Config
{
  Eina_List *items;
  E_Module *module;
  E_Config_Dialog *config_dialog;
  const char *log_name;
  Eina_Bool label_length_changed;  /* Flag indicating a need to update all clip
                                       labels as configfuration changed. */

  int version;          /* Configuration version                           */
  int clip_copy;        /* Clipboard to use                                */
  int clip_select;      /* Clipboard to use                                */
  int sync;             /* Synchronize clipboards flag                     */
  int persistence;      /* History file persistance                        */
  int hist_reverse;     /* Order to display History                        */
  double hist_items;    /* Number of history items to store                */
  int confirm_clear;    /* Display history confirmation dialog on deletion */
  int autosave;         /* Save History on every selection change          */
  double save_timer;    /* Timer for save history                          */
  double label_length;  /* Number of characters of item to display         */
  int   ignore_ws;      /* Should we ignore White space in label           */
  int   ignore_ws_copy; /* Should we not copy White space only             */
  int trim_ws;          /* Should we trim White space from selection       */
  int trim_nl;          /* Should we trim new lines from selection         */
};

struct _Config_Item
{
  const char *id;
};

extern Config *clip_cfg;

E_Config_Dialog *config_clipboard_module(E_Container *con, const char *params __UNUSED__);
Eet_Error        truncate_history(const unsigned int n);

#endif
