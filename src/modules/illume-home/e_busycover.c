#include "e.h"
#include "e_busycover.h"
#include "e_mod_config.h"

/* local function prototypes */
static void _e_busycover_cb_free(E_Busycover *cover);

EAPI E_Busycover *
e_busycover_new(E_Win *win) 
{
   E_Busycover *cover;
   char buff[PATH_MAX];

   cover = E_OBJECT_ALLOC(E_Busycover, E_BUSYCOVER_TYPE, _e_busycover_cb_free);
   if (!cover) return NULL;
   snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", 
            il_home_cfg->mod_dir);

   cover->o_base = edje_object_add(e_win_evas_get(win));
   if (!e_theme_edje_object_set(cover->o_base, 
                                "base/theme/modules/illume-home", 
                                "modules/illume-home/busycover")) 
     edje_object_file_set(cover->o_base, buff, "modules/illume-home/busycover");
   edje_object_part_text_set(cover->o_base, "e.text.title", _("LOADING"));
   evas_object_move(cover->o_base, win->x, win->y);
   evas_object_resize(cover->o_base, win->w, win->h);
   evas_object_layer_set(cover->o_base, 999);
   return cover;
}

EAPI E_Busycover_Handle *
e_busycover_push(E_Busycover *cover, const char *msg, const char *icon) 
{
   E_Busycover_Handle *handle;

   E_OBJECT_CHECK(cover);
   E_OBJECT_TYPE_CHECK_RETURN(cover, E_BUSYCOVER_TYPE, NULL);

   handle = E_NEW(E_Busycover_Handle, 1);
   handle->cover = cover;
   if (msg) handle->msg = eina_stringshare_add(msg);
   if (icon) handle->icon = eina_stringshare_add(icon);
   cover->handles = eina_list_append(cover->handles, handle);
   edje_object_part_text_set(cover->o_base, "e.text.title", msg);
   evas_object_show(cover->o_base);
   return handle;
}

EAPI void 
e_busycover_pop(E_Busycover *cover, E_Busycover_Handle *handle) 
{
   E_OBJECT_CHECK(cover);
   E_OBJECT_TYPE_CHECK(cover, E_BUSYCOVER_TYPE);
   if (!eina_list_data_find(cover->handles, handle)) return;
   cover->handles = eina_list_remove(cover->handles, handle);
   if (handle->msg) eina_stringshare_del(handle->msg);
   if (handle->icon) eina_stringshare_del(handle->icon);
   E_FREE(handle);
   if (cover->handles) 
     {
        handle = cover->handles->data;
        edje_object_part_text_set(cover->o_base, "e.text.title", handle->msg);
     }
   else 
     evas_object_hide(cover->o_base);
}

EAPI void 
e_busycover_resize(E_Busycover *cover, int w, int h) 
{
   E_OBJECT_CHECK(cover);
   E_OBJECT_TYPE_CHECK(cover, E_BUSYCOVER_TYPE);
   evas_object_resize(cover->o_base, w, h);
}

/* local function prototypes */
static void 
_e_busycover_cb_free(E_Busycover *cover) 
{
   E_Busycover_Handle *handle;

   EINA_LIST_FREE(cover->handles, handle) 
     {
        if (handle->msg) eina_stringshare_del(handle->msg);
        if (handle->icon) eina_stringshare_del(handle->icon);
        E_FREE(handle);
     }

   if (cover->o_base) evas_object_del(cover->o_base);
   E_FREE(cover);
}
