#include "e_mod_tiling.h"

/* HACK: Needed to get subobjs of the widget. Is there a better way? */
typedef struct _E_Widget_Smart_Data E_Widget_Smart_Data;

struct _E_Widget_Smart_Data
{
   Evas_Object *parent_obj;
   Evas_Coord   x, y, w, h;
   Evas_Coord   minw, minh;
   Eina_List   *subobjs;
};

/* Some defines to make coding with the e_widget_* easier for
 * configuration panel */
#define RADIO(title, value, radiogroup) \
  e_widget_radio_add(evas, _(title), value, radiogroup)
#define LIST_ADD(list, object) \
  e_widget_list_object_append(list, object, 1, 1, 0.5)

struct _Config_vdesk *
get_vdesk(Eina_List *vdesks,
          int x,
          int y,
          unsigned int zone_num)
{
    Eina_List *l;
    for (l = vdesks; l; l = l->next) {
        struct _Config_vdesk *vd = l->data;

        if (!vd)
            continue;
        if (vd->nb_stacks < 0 || vd->nb_stacks > TILING_MAX_STACKS)
            vd->nb_stacks = 0;
        if (vd->x == x && vd->y == y && vd->zone_num == zone_num)
            return vd;
    }

    return NULL;
}

/*
 * Fills the E_Config_Dialog-struct with the data currently in use
 *
 */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
    E_Config_Dialog_Data *cfdata;
    Eina_List *l;

    cfdata = E_NEW(E_Config_Dialog_Data, 1);

    /* Because we save a lot of lines here by using memcpy,
     * the structs have to be ordered the same */
    memcpy(cfdata, tiling_g.config, sizeof(Config));
    cfdata->config.keyhints = strdup(tiling_g.config->keyhints);

    /* Handle things which can't be easily memcpy'd */
    cfdata->config.vdesks = NULL;

    for (l = tiling_g.config->vdesks; l; l = l->next) {
        struct _Config_vdesk *vd = l->data,
                             *newvd;

        if (!vd)
            continue;

        newvd = E_NEW(struct _Config_vdesk, 1);
        newvd->x = vd->x;
        newvd->y = vd->y;
        newvd->zone_num = vd->zone_num;
        newvd->nb_stacks = vd->nb_stacks;
        newvd->use_rows = vd->use_rows;

        EINA_LIST_APPEND(cfdata->config.vdesks, newvd);
    }

    return cfdata;
}

static void
_free_data(E_Config_Dialog      *cfd __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
    eina_list_free(cfdata->config.vdesks);
    free(cfdata->config.keyhints);
    free(cfdata);
}

static void
_fill_zone_config(E_Zone               *zone,
                  E_Config_Dialog_Data *cfdata)
{
    Evas *evas;
    int i;

    evas = cfdata->evas;

    /* Clear old entries first */
    evas_object_del(cfdata->o_desklist);

    cfdata->o_desklist = e_widget_list_add(evas, 1, 0);

    for (i = 0; i < zone->desk_y_count * zone->desk_x_count; i++) {
        E_Desk *desk = zone->desks[i];
        struct _Config_vdesk *vd;
        Evas_Object *list, *slider, *radio;
        E_Radio_Group *rg;

        if (!desk)
            continue;

        vd = get_vdesk(cfdata->config.vdesks, desk->x, desk->y, zone->num);
        if (!vd) {
            vd = E_NEW(struct _Config_vdesk, 1);
            vd->x = desk->x;
            vd->y = desk->y;
            vd->zone_num = zone->num;
            vd->nb_stacks = 0;
            vd->use_rows = 0;

            EINA_LIST_APPEND(cfdata->config.vdesks, vd);
        }

        list = e_widget_list_add(evas, false, true);

        LIST_ADD(list, e_widget_label_add(evas, desk->name));
        slider = e_widget_slider_add(evas, 1, 0, _("%1.0f"),
                                     0.0, 8.0, 1.0, 0, NULL,
                                     &vd->nb_stacks, 150);
        LIST_ADD(list, slider);

        rg = e_widget_radio_group_new(&vd->use_rows);
        radio = e_widget_radio_add(evas, _("columns"), 0, rg);
        LIST_ADD(list, radio);
        radio = e_widget_radio_add(evas, _("rows"), 1, rg);
        LIST_ADD(list, radio);

        LIST_ADD(cfdata->o_desklist, list);
    }

    /* Get the correct sizes of desklist and scrollframe */
    LIST_ADD(cfdata->osf, cfdata->o_desklist);
}

static void
_cb_zone_change(void        *data,
                Evas_Object *obj __UNUSED__)
{
    int n;
    E_Config_Dialog_Data *cfdata = data;
    E_Zone *zone;

    if (!cfdata || !cfdata->o_zonelist)
        return;

    n = e_widget_ilist_selected_get(cfdata->o_zonelist);
    zone = e_widget_ilist_nth_data_get(cfdata->o_zonelist, n);
    if (!zone)
        return;
    _fill_zone_config(zone, cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog      *cfd __UNUSED__,
                      Evas                 *evas,
                      E_Config_Dialog_Data *cfdata)
{
    Evas_Object *o, *oc, *of;
    E_Container *con;
    E_Zone *zone;
    Eina_List *l;

    con = e_container_current_get(e_manager_current_get());

    o = e_widget_list_add(evas, 0, 0);

    /* General settings */
    of = e_widget_framelist_add(evas, _("General"), 0);
    e_widget_framelist_object_append(of,
      e_widget_check_add(evas, _("Tile dialog windows as well"),
                         &cfdata->config.tile_dialogs));
    e_widget_framelist_object_append(of,
      e_widget_check_add(evas, _("Show window titles"),
                         &cfdata->config.show_titles));
    oc = e_widget_list_add(evas, false, true);
    e_widget_list_object_append(oc,
      e_widget_label_add(evas, _("Key hints")), 1, 0, 0.5);
    e_widget_list_object_append(oc,
      e_widget_entry_add(evas, &cfdata->config.keyhints, NULL, NULL, NULL),
      1, 1, 0.5);
    e_widget_framelist_object_append(of, oc);

    LIST_ADD(o, of);

    /* Virtual desktop settings */
    of = e_widget_framelist_add(evas, _("Virtual Desktops"), 0);
    e_widget_label_add(evas,
                       _("Number of columns used to tile per desk"
                          " (0 → tiling disabled):"));
    cfdata->osf = e_widget_list_add(evas, 0, 1);

    /* Zone list */
    cfdata->o_zonelist = e_widget_ilist_add(evas, 0, 0, NULL);
    e_widget_ilist_multi_select_set(cfdata->o_zonelist, false);
    e_widget_size_min_set(cfdata->o_zonelist, 100, 100);
    e_widget_on_change_hook_set(cfdata->o_zonelist, _cb_zone_change, cfdata);
    for (l = con->zones; l; l = l->next) {
        if (!(zone = l->data))
            continue;
        e_widget_ilist_append(cfdata->o_zonelist, NULL, zone->name, NULL, zone, NULL);
    }
    e_widget_ilist_go(cfdata->o_zonelist);
    e_widget_ilist_thaw(cfdata->o_zonelist);

    LIST_ADD(cfdata->osf, cfdata->o_zonelist);

    /* List of individual tiling modes */
    cfdata->evas = evas;

    _fill_zone_config(eina_list_data_get(con->zones), cfdata);

    e_widget_ilist_selected_set(cfdata->o_zonelist, 0);

    e_widget_framelist_object_append(of, cfdata->osf);

    LIST_ADD(o, of);

    return o;
}

static int
_basic_apply_data(E_Config_Dialog      *cfd __UNUSED__,
                  E_Config_Dialog_Data *cfdata)
{
    struct _Config_vdesk *vd;
    Eina_List *l;

    tiling_g.config->tile_dialogs = cfdata->config.tile_dialogs;
    tiling_g.config->show_titles = cfdata->config.show_titles;
    if (strcmp(tiling_g.config->keyhints, cfdata->config.keyhints)) {
        free(tiling_g.config->keyhints);
        if (!cfdata->config.keyhints || !*cfdata->config.keyhints) {
            tiling_g.config->keyhints = strdup(tiling_g.default_keyhints);
        } else {
            char *c = cfdata->config.keyhints;
            int len = strlen(cfdata->config.keyhints);

            /* Remove duplicates */
            while (*c) {
                char *f = c + 1;

                while ((f = strchr(f, *c))) {
                    *f = cfdata->config.keyhints[--len];
                    cfdata->config.keyhints[len] = '\0';
                }
                c++;
            }
            tiling_g.config->keyhints = strdup(cfdata->config.keyhints);
        }
    }

    /* Check if the layout for one of the vdesks has changed */
    for (l = tiling_g.config->vdesks; l; l = l->next) {
        struct _Config_vdesk *newvd;

        vd = l->data;

        if (!vd)
            continue;
        if (!(newvd = get_vdesk(cfdata->config.vdesks,
                                vd->x, vd->y, vd->zone_num))) {
            change_desk_conf(vd);
            continue;
        }

        if (newvd->nb_stacks != vd->nb_stacks
        ||  newvd->use_rows != vd->use_rows) {
            DBG("number of columns for (%d, %d, %d) changed from %d|%d"
                " to %d|%d",
                vd->x, vd->y, vd->zone_num,
                vd->nb_stacks, vd->use_rows,
                newvd->nb_stacks, newvd->use_rows);
            change_desk_conf(newvd);
            free(vd);
            l->data = NULL;
        }
    }

    for (l = cfdata->config.vdesks; l; l = l->next) {
        vd = l->data;

        if (!vd)
            continue;
        if (!get_vdesk(tiling_g.config->vdesks,
                       vd->x, vd->y, vd->zone_num)) {
            change_desk_conf(vd);
            continue;
        }
    }

    EINA_LIST_FREE(tiling_g.config->vdesks, vd) {
        free(vd);
    }
    tiling_g.config->vdesks = NULL;

    for (l = cfdata->config.vdesks; l; l = l->next) {
        struct _Config_vdesk *newvd;

        vd = l->data;
        if (!vd)
            continue;

        newvd = E_NEW(struct _Config_vdesk, 1);
        newvd->x = vd->x;
        newvd->y = vd->y;
        newvd->zone_num = vd->zone_num;
        newvd->nb_stacks = vd->nb_stacks;
        newvd->use_rows = vd->use_rows;

        EINA_LIST_APPEND(tiling_g.config->vdesks, newvd);
    }

    e_tiling_update_conf();

    e_config_save_queue();

    return EINA_TRUE;
}

E_Config_Dialog *
e_int_config_tiling_module(E_Container *con,
                           const char  *params __UNUSED__)
{
    E_Config_Dialog *cfd;
    E_Config_Dialog_View *v;
    char buf[PATH_MAX];

    if (e_config_dialog_find("E", "windows/tiling"))
        return NULL;

    v = E_NEW(E_Config_Dialog_View, 1);

    v->create_cfdata = _create_data;
    v->free_cfdata = _free_data;
    v->basic.apply_cfdata = _basic_apply_data;
    v->basic.create_widgets = _basic_create_widgets;

    snprintf(buf, sizeof(buf), "%s/e-module-tiling.edj",
             e_module_dir_get(tiling_g.module));
    cfd = e_config_dialog_new(con,
                              _("Tiling Configuration"),
                              "E", "windows/tiling",
                              buf, 0, v, NULL);
    return cfd;
}
