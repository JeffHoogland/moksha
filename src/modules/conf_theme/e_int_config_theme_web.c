/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "e_int_config_theme.h"
#include "e_int_config_theme_web.h"

#ifdef HAVE_EXCHANGE
#include <Exchange.h>

typedef struct _Web Web;
struct _Web
{
   E_Config_Dialog *parent;
   E_Dialog *dia;
};

static void
_web_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Evas_Coord th, vh;

   evas_object_smart_callback_call(obj, "changed", NULL);
   evas_object_size_hint_min_get(obj, NULL, &th);
   evas_object_geometry_get(obj, NULL, NULL, NULL, &vh);

   if (y < 0) y = 0;
   if (y > th - vh) y = th - vh;

   exchange_smart_object_offset_set(obj, 0, y);
}

static void
_web_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   if (!x || !y) return;
   exchange_smart_object_offset_get(obj, x, y);
}

static void
_web_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Evas_Coord tw, th, vw, vh;

   evas_object_size_hint_min_get(obj, &tw, &th);
   evas_object_geometry_get(obj, NULL, NULL, &vw, &vh);

   if (x) *x = tw - vw;
   if (y) *y = th - vh;
}

static void
_web_pan_child_size_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Evas_Coord tw, th, vw, vh;

   evas_object_size_hint_min_get(obj, &tw, &th);
   evas_object_geometry_get(obj, NULL, NULL, &vw, &vh);

   if (x) *x = tw;
   if (y) *y = th;
}

void
e_int_config_theme_web_del(E_Dialog *dia)
{
   Web *web;

   web = dia->data;
   e_int_config_theme_web_done(web->parent);
   E_FREE(web);
   e_object_unref(E_OBJECT(dia));
}

static void
_web_del_cb(void *obj)
{
   E_Dialog *dia = obj;

   e_int_config_theme_web_del(dia);
}

static void
_web_close_cb(void *data, E_Dialog *dia)
{
   e_int_config_theme_web_del(dia);
}

static void 
_web_apply(const char *path, void *data)
{
   E_Action *a;
   E_Config_Theme *ct;

   ct = e_theme_config_get("theme");
   if ((ct) && (!strcmp(ct->file, path))) return;

   e_theme_config_set("theme", path);
   e_config_save_queue();

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

EAPI E_Dialog *
e_int_config_theme_web(E_Config_Dialog *parent)
{
   E_Dialog *dia;
   Web *web;
   Evas_Object *ol, *exsm, *sf;
   Evas_Coord mw, mh;
   E_Fm2_Config fmc;
   char sys_dir[PATH_MAX];
   char usr_dir[PATH_MAX];
   
   
   dia = e_dialog_new(parent->con, "E", "_theme_web_dialog");
   if (!dia) return NULL;

   web = E_NEW(Web, 1);
   if (!web) return NULL;

   web->dia = dia;
   web->parent = parent;

   e_dialog_title_set(dia, _("Exchange themes"));
   e_dialog_border_icon_set(dia, "enlightenment/theme");
   e_dialog_resizable_set(dia, 1);
   e_dialog_button_add(dia, _("Close"), NULL, _web_close_cb, web);

   dia->data = web;
   e_object_del_attach_func_set(E_OBJECT(dia), _web_del_cb);
   e_win_centered_set(dia->win, 1);


   ol = e_widget_list_add(e_win_evas_get(dia->win), 0, 1);

   /* The Exchange Smart Object*/
   snprintf(sys_dir, sizeof(sys_dir), "%s/data/themes", e_prefix_data_get());
   snprintf(usr_dir, sizeof(usr_dir), "%s/.e/e/themes", e_user_homedir_get());
   exsm = exchange_smart_object_add(e_win_evas_get(dia->win));
   exchange_smart_object_remote_group_set(exsm, "Border");
   exchange_smart_object_local_path_set(exsm, usr_dir, sys_dir);
   exchange_smart_object_mode_set(exsm, EXCHANGE_SMART_SHOW_BOTH);
   exchange_smart_object_apply_cb_set(exsm, _web_apply, NULL);

   /* The Scroll Frame */
   sf = e_scrollframe_add(e_win_evas_get(dia->win));
   e_scrollframe_extern_pan_set(sf, exsm, _web_pan_set, _web_pan_get,
                                _web_pan_max_get, _web_pan_child_size_get);
   e_scrollframe_policy_set(sf, E_SCROLLFRAME_POLICY_OFF,
                                  E_SCROLLFRAME_POLICY_ON);
   e_scrollframe_thumbscroll_force(sf, 1);

   e_widget_list_object_append(ol, sf, 1, 1, 0.5);
   e_dialog_content_set(dia, ol, 500, 400);

   e_dialog_show(dia);
   exchange_smart_object_run(exsm);

   return dia;
}

#endif
