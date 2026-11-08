#include "history.h"
#include "image.h"

#define CLIPBOARD_MOD_NAME "clipboard"
#define DATA_DIR    CLIPBOARD_MOD_NAME
#define HISTORY_NAME "history"
/* 1: text only
 * 2: adds "<n>_type" (T = text, I = image). Version 1 files are still read.
 * must be < 9  */
#define HISTORY_VERSION     2
#define LOCK_DIGITS (HIST_MAX_DIGITS + 7)

/* convenience macros to compress code */
#define PATH_MAX_ERR                                              \
do {                                                              \
  ERR("PATH_MAX exceeded. Need Len %d, PATH_MAX %d", len, PATH_MAX); \
  memset(path,0,PATH_MAX);                                        \
  success = EINA_FALSE;                                           \
 } while(0)                                                       \

/* Private Funcions */
Eina_Bool _mkpath_if_not_exists(const char *path);
Eina_Bool _set_data_path(char *path);
Eina_Bool _set_history_path(char *path);

/**
 * @brief Creates path if non-existant
 *
 * @param path char array to create.
 * @return EINA_TRUE on success EINA_FALSE otherwise
 *
 */
Eina_Bool
_mkpath_if_not_exists(const char *path)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

    Eina_Bool success = EINA_TRUE;

    if(!ecore_file_exists(path))
       return ecore_file_mkdir(path);
    return success;
}

/**
 * @brief Sets the XDG_DATA_HOME location for saving clipboard history data.
 *
 * @param path char array to store history path in.
 * @return EINA_TRUE on success EINA_FALSE otherwise
 *
 */
Eina_Bool
_set_data_path(char *path)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

    const char *temp_str = efreet_data_home_get();
    Eina_Bool success = EINA_TRUE;
    const int len = snprintf(NULL, 0, "%s", temp_str)
                              + 1 + (temp_str[strlen(temp_str)-1] != '/');

    if (temp_str[0] == '/' ) {

      if (len <= PATH_MAX) {
        snprintf(path, strlen(temp_str)+1, "%s", temp_str);
        // Ensure XDG_DATA_HOME terminates in '/'
        if (path[strlen(path)-1] != '/')
          strncat(path, "/", PATH_MAX-strlen(path)-1);
      }
      else
        PATH_MAX_ERR;
    }
    else
      PATH_MAX_ERR;
    return success;
}

/**
 * @brief Sets the path and file name for saving clipboard history data.
 *
 * @param history_path char array to store history path in.
 * @return EINA_TRUE on success EINA_FALSE otherwise
 *
 * This function not only sets the history path but also creates the needed
 * directories if they do not exist.
 *
 */
Eina_Bool
_set_history_path(char *path)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

   char temp_str[PATH_MAX] = {0};
   Eina_Bool success = EINA_TRUE;

   if(_set_data_path(path)) {
       const int len = snprintf(NULL, 0, "%s%s/%s", path, CLIPBOARD_MOD_NAME, HISTORY_NAME) + 1;
       if (len <= PATH_MAX) {
         strncpy(temp_str, path, PATH_MAX-1);
         int ret = snprintf(path, PATH_MAX, "%s%s/", temp_str, CLIPBOARD_MOD_NAME);
         if (ret < 0) PATH_MAX_ERR;
         success = _mkpath_if_not_exists(path);
         strncat(path, HISTORY_NAME, PATH_MAX-strlen(path)-1);
       }
       else
        PATH_MAX_ERR;
   } else
       success = EINA_FALSE;
   return success;
}

/* Copy of an eet string item: eet_read() does not guarantee '\0' termination
 * in a damaged file. */
static char *
_item_dup(const char *data, int size)
{
    char *ret;

    if (!data || size <= 0) return NULL;
    ret = malloc(size + 1);
    if (!ret) return NULL;
    memcpy(ret, data, size);
    ret[size] = 0;
    return ret;
}

/**
 * @brief  Reads clipboard Instantance history  from a binary file with location
 *           specified by FreeDesktop XDG specifications
 *
 * @param  items,        the address of an Eina_List to fill with history file
 *         ignore_ws     flag to ignore whitespace in setting label name
 *         label_length  number of characters to store in label name
 *
 * @return  EET_ERROR_NONE on success
 *          Eet error identifier of error otherwise.
 *          On error *items set to NULL
 */
Eet_Error
read_history(Eina_List **items, unsigned ignore_ws, unsigned label_length)
{
    Eet_File *history_file = NULL;
    Clip_Data *cd = NULL;
    Eina_List *l = NULL;
    char history_path[PATH_MAX] = {0};
    char key[LOCK_DIGITS];
    char *val = NULL;
    int size = 0;
    unsigned int i = 0;
    unsigned int item_num = 0;
    unsigned int version = 0;

    *items = NULL;

    /* Open history file */
    if(!_set_history_path(history_path)) {
      ERR("History File Creation Error: %s", history_path);
      return EET_ERROR_BAD_OBJECT;
    }
    history_file = eet_open(history_path, EET_FILE_MODE_READ);
    if (!history_file) {
      ERR("Failed to open history file: %s", history_path);
      return EET_ERROR_BAD_OBJECT;
    }
    /* Check History Version */
    val = eet_read(history_file, "VERSION", &size);
    if (!val)
      INF("No version number in history file");
    else {
      version = strtol(val, NULL, 10);
      free(val);
    }
    if (version > HISTORY_VERSION) {
      INF("History file version mismatch, deleting history");
      return eet_close(history_file);
    }
    /* Read Number of items */
    val = eet_read(history_file, "MAX_ITEMS", &size);
    if (!val) {
      ERR("History file corruption: %s", history_path);
      return eet_close(history_file);
    }
    /* If we have no items in history wrap it up and return. */
    item_num = strtol(val, NULL, 10);
    free(val);
    if (item_num <= 0) {
      INF("History file empty or corrupt: %s", history_path);
      return eet_close(history_file);
    }
    /* Keys have at most HIST_MAX_DIGITS digits */
    if (item_num > HIST_MAX)
      item_num = HIST_MAX;

    /* Read each item */
    for (i = 1; i <= item_num; i++){
        char *content;
        Eina_Bool is_image = EINA_FALSE;

        snprintf(key, sizeof(key), "%u", i);
        val = eet_read(history_file, key, &size);
        content = _item_dup(val, size);
        free(val);
        if (!content) {
          ERR("History file corruption: %s", history_path);
          E_FREE_LIST(l, free_clip_data);
          return eet_close(history_file);
        }

        /* type: missing in version 1 files, those only hold text */
        snprintf(key, sizeof(key), "%u_type", i);
        val = eet_read(history_file, key, &size);
        if (val && (size > 0) && (val[0] == 'I'))
          is_image = EINA_TRUE;
        free(val);

        cd = E_NEW(Clip_Data, 1);
        if (!cd) {
          free(content);
          E_FREE_LIST(l, free_clip_data);
          return eet_close(history_file);
        }
        cd->id = clip_id_next();
        cd->content = content;

        /* lock: prevention for new eet file lock item */
        snprintf(key, sizeof(key), "%u_lock", i);
        val = eet_read(history_file, key, &size);
        cd->lock = strdup((val && (size > 0) && (val[0] == 'L')) ? "L" : "U");
        free(val);

        if (is_image) {
          cd->type = CLIP_TYPE_IMAGE;
          /* The image file might be gone: just skip the item then */
          if (!clip_image_load_info(cd->content, &cd->w, &cd->h, &cd->size)) {
            WRN("Image %s of history item %u is missing", cd->content, i);
            free_clip_data(cd);
            continue;
          }
          cd->name = clip_image_label_new(cd->w, cd->h, cd->size);
        } else {
          cd->type = CLIP_TYPE_TEXT;
          set_clip_name(&cd->name, cd->content, ignore_ws, label_length);
        }
        if (!cd->lock || !cd->name) {
          free_clip_data(cd);
          continue;
        }
        l = eina_list_append(l, cd);
    }
    /* and wrap it up */
    *items = l;
    return eet_close(history_file);
}

/**
 * @brief  Saves clipboard Instantance history  in a binary file with location
 *           specified by FreeDesktop XDG specifications
 *
 * @param  items, the Eina_List to save to the history file
 *
 * @return  EET_ERROR_NONE on success
 *          Eet error identifier of error otherwise.
 *
 * This function not only sets the history path but will also create the needed
 *    directories if they do not exist. See warnings in auxillary functions.
 *
 * Images are not stored in the history file, only their ids.
 */
Eet_Error
save_history(Eina_List *items)
{
    Eet_File *history_file = NULL;
    Eina_List *l = NULL;
    Clip_Data *cd = NULL;
    char history_path[PATH_MAX] = {0};
    char str[HIST_MAX_DIGITS + 2];
    char key[LOCK_DIGITS];
    unsigned int i = 1;

    /* Open history file */
    if(!_set_history_path(history_path)) {
      ERR("History File Creation Error: %s", history_path);
      return EET_ERROR_BAD_OBJECT;
    }
    history_file = eet_open(history_path, EET_FILE_MODE_WRITE);
    if (!history_file) {
      ERR("Unable to open history file: %s", history_path);
      return  EET_ERROR_BAD_OBJECT;
    }

    /* Write history version */
    snprintf(str, sizeof(str), "%d", (HISTORY_VERSION > 9 ? 9 : HISTORY_VERSION));
    eet_write(history_file, "VERSION",  str, strlen(str) + 1, 0);

    /* Write each item */
    EINA_LIST_FOREACH(items, l, cd) {
      snprintf(key, sizeof(key), "%u", i);
      eet_write(history_file, key, cd->content, strlen(cd->content) + 1, 0);
      snprintf(key, sizeof(key), "%u_lock", i);
      eet_write(history_file, key, cd->lock, strlen(cd->lock) + 1, 0);
      snprintf(key, sizeof(key), "%u_type", i);
      eet_write(history_file, key, (cd->type == CLIP_TYPE_IMAGE) ? "I" : "T", 2, 0);
      i++;
    }
    /* and wrap it up */
    snprintf(str, sizeof(str), "%u", i - 1);
    eet_write(history_file, "MAX_ITEMS",  str, strlen(str) + 1, 0);
    return eet_close(history_file);
}
