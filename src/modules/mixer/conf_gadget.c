#include "e_mod_main.h"

extern const char _Name[];

struct _E_Config_Dialog_Data
{
   int lock_sliders;
   int show_locked;
   int card_num;
   int channel;
   const char *card;
   const char *channel_name;
   Evas_List *cards;
   Evas_List *cards_names;
   Evas_List *channels_names;
   struct mixer_config_ui
     {
        Evas_Object *table;
        struct mixer_config_ui_general
	  {
	     Evas_Object *frame;
	     Evas_Object *lock_sliders;
	     Evas_Object *show_locked;
	  } general;
        struct mixer_config_ui_cards
	  {
	     Evas_Object *frame;
	     E_Radio_Group *radio;
	  } cards;
        struct mixer_config_ui_channels
	  {
	     Evas_Object *frame;
 	     Evas_Object *scroll;
 	     Evas_Object *list;
	     E_Radio_Group *radio;
	     Evas_List *radios;
	  } channels;
     } ui;
   E_Mixer_Gadget_Config *conf;
};

static void
_mixer_fill_cards_info(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;
   int i;

   cfdata->card_num = -1;
   cfdata->cards = e_mixer_system_get_cards();
   cfdata->cards_names = NULL;
   for (l = cfdata->cards, i = 0; l != NULL; l = l->next, i++)
     {
	char *card, *name;

	card = l->data;
	name = e_mixer_system_get_card_name(card);
	if ((cfdata->card_num < 0) && card && cfdata->card &&
	    (strcmp(card, cfdata->card) == 0))
	  cfdata->card_num = i;

	cfdata->cards_names = evas_list_append(cfdata->cards_names, name);
     }

   if (cfdata->card_num < 0)
     cfdata->card_num = 0;
}

static void
_mixer_fill_channels_info(E_Config_Dialog_Data *cfdata)
{
   E_Mixer_System *sys;
   Evas_List *l;
   int i;

   sys = e_mixer_system_new(cfdata->card);
   if (!sys)
     return;

   cfdata->channel = 0;
   cfdata->channel_name = evas_stringshare_add(cfdata->conf->channel_name);
   cfdata->channels_names = e_mixer_system_get_channels_names(sys);
   for (l = cfdata->channels_names, i = 0; l != NULL; l = l->next, i++)
     {
	char *channel;

	channel = l->data;
	if (channel && cfdata->channel_name &&
	    (strcmp(channel, cfdata->channel_name) == 0))
	  {
	     cfdata->channel = i;
	     break;
	  }
     }
   e_mixer_system_del(sys);
}

static void *
_create_data(E_Config_Dialog *dialog)
{
   E_Config_Dialog_Data *cfdata;
   E_Mixer_Gadget_Config *conf;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata)
     return NULL;

   conf = dialog->data;
   cfdata->conf = conf;
   cfdata->lock_sliders = conf->lock_sliders;
   cfdata->show_locked = conf->show_locked;
   cfdata->card = evas_stringshare_add(conf->card);
   _mixer_fill_cards_info(cfdata);
   _mixer_fill_channels_info(cfdata);

   return cfdata;
}

static void
_free_data(E_Config_Dialog *dialog, E_Config_Dialog_Data *cfdata)
{
   E_Mixer_Gadget_Config *conf;
   Evas_List *l;

   conf = dialog->data;
   if (conf)
     conf->dialog = NULL;

   if (!cfdata)
     return;

   for (l = cfdata->cards_names; l != NULL; l = l->next)
     if (l->data)
       free(l->data);
   evas_list_free(cfdata->cards_names);

   if (cfdata->channels_names)
     e_mixer_system_free_channels_names(cfdata->channels_names);
   if (cfdata->cards)
     e_mixer_system_free_cards(cfdata->cards);

   if (cfdata->card)
     evas_stringshare_del(cfdata->card);
   if (cfdata->channel_name)
     evas_stringshare_del(cfdata->channel_name);

   evas_list_free(cfdata->ui.channels.radios);

   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *dialog, E_Config_Dialog_Data *cfdata)
{
   E_Mixer_Gadget_Config *conf;
   const char *card, *channel;

   conf = dialog->data;
   conf->lock_sliders = cfdata->lock_sliders;
   conf->show_locked = cfdata->show_locked;

   card = evas_list_nth(cfdata->cards, cfdata->card_num);
   if (card)
     {
	if (conf->card && (strcmp(card, conf->card) != 0))
	  evas_stringshare_del(conf->card);
	conf->card = evas_stringshare_add(card);
     }

   channel = evas_list_nth(cfdata->channels_names, cfdata->channel);
   if (channel)
     {
	if (conf->channel_name && (strcmp(channel, conf->channel_name) != 0))
	  evas_stringshare_del(conf->channel_name);
	conf->channel_name = evas_stringshare_add(channel);
     }

   e_mixer_update(conf->instance);
   return 1;
}

static void
_lock_change(void *data, Evas_Object *obj, void *event)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_widget_disabled_set(cfdata->ui.general.show_locked, !cfdata->lock_sliders);
}

static void
_basic_create_general(Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct mixer_config_ui_general *ui;

   ui = &cfdata->ui.general;

   ui->frame = e_widget_framelist_add(evas, _("General Settings"), 0);

   ui->lock_sliders = e_widget_check_add(
      evas, _("Lock Sliders"), &cfdata->lock_sliders);
   evas_object_smart_callback_add(
      ui->lock_sliders, "changed", _lock_change, cfdata);
   e_widget_framelist_object_append(ui->frame, ui->lock_sliders);

   ui->show_locked = e_widget_check_add(
      evas, _("Show both sliders when locked"), &cfdata->show_locked);
   e_widget_disabled_set(ui->show_locked, !cfdata->lock_sliders);
   e_widget_framelist_object_append(ui->frame, ui->show_locked);
}

static void
_clear_channels(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;

   for (l = cfdata->ui.channels.radios; l != NULL; l = l->next)
     evas_object_del(l->data);
   cfdata->ui.channels.radios = evas_list_free(cfdata->ui.channels.radios);
}

static void
_fill_channels(Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct mixer_config_ui_channels *ui;
   Evas_Object *selected;
   Evas_Coord mw, mh;
   Evas_List *l;
   int i;

   ui = &cfdata->ui.channels;
   ui->radio = e_widget_radio_group_new(&cfdata->channel);
   for (i = 0, l = cfdata->channels_names; l != NULL; l = l->next, i++)
     {
	Evas_Object *ow;
        const char *name;

	name = l->data;
	if (!name)
	  continue;

        ow = e_widget_radio_add(evas, name, i, ui->radio);
	ui->radios = evas_list_append(ui->radios, ow);
	e_widget_list_object_append(ui->list, ow, 1, 1, 0.0);
     }

   e_widget_min_size_get(ui->list, &mw, &mh);
   evas_object_resize(ui->list, mw, mh);

   selected = evas_list_nth(ui->radios, cfdata->channel);
   if (selected)
     {
	Evas_Coord x, y, w, h, lx, ly;
	evas_object_geometry_get(selected, &x, &y, &w, &h);
	evas_object_geometry_get(ui->list, &lx, &ly, NULL, NULL);
	x -= lx;
	y -= ly - 10;
	h += 20;
	e_widget_scrollframe_child_region_show(ui->scroll, x, y, w, h);
     }
}

static void
_channel_scroll_set_min_size(struct mixer_config_ui_channels *ui)
{
   Evas_Coord w, h;
   int len;

   len = evas_list_count(ui->radios);
   if (len < 1)
     return;

   e_widget_min_size_get(ui->list, &w, &h);
   h = 4 * h / len;
   e_widget_min_size_set(ui->scroll, w, h);
}

static void
_basic_create_channels(Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct mixer_config_ui_channels *ui;

   ui = &cfdata->ui.channels;
   ui->list = e_widget_list_add(evas, 1, 0);
   ui->scroll = e_widget_scrollframe_simple_add(evas, ui->list);
   ui->frame = e_widget_framelist_add(evas, _("Channels"), 0);

   _fill_channels(evas, cfdata);

   _channel_scroll_set_min_size(ui);
   e_widget_framelist_object_append(ui->frame, ui->scroll);
}

static void
_card_change(void *data, Evas_Object *obj, void *event)
{
   E_Config_Dialog_Data *cfdata;
   Evas *evas;
   char *card;

   cfdata = data;

   if (cfdata->card)
     evas_stringshare_del(cfdata->card);

   e_mixer_system_free_channels_names(cfdata->channels_names);
   if (cfdata->channel_name)
     evas_stringshare_del(cfdata->channel_name);

   card = evas_list_nth(cfdata->cards, cfdata->card_num);
   cfdata->card = evas_stringshare_add(card);
   _mixer_fill_channels_info(cfdata);

   evas = evas_object_evas_get(obj);
   _clear_channels(cfdata);
   _fill_channels(evas, cfdata);
}

static void
_basic_create_cards(Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct mixer_config_ui_cards *ui;
   Evas_List *l;
   int i;

   ui = &cfdata->ui.cards;

   ui->frame = e_widget_framelist_add(evas, _("Sound Cards"), 0);
   ui->radio = e_widget_radio_group_new(&cfdata->card_num);
   for (i = 0, l = cfdata->cards_names; l != NULL; l = l->next, i++)
     {
	Evas_Object *ow;
	const char *card;

	card = l->data;
	if (!card)
	  continue;

        ow = e_widget_radio_add(evas, card, i, ui->radio);
        e_widget_framelist_object_append(ui->frame, ow);
	evas_object_smart_callback_add(ow, "changed", _card_change, cfdata);
     }
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata)
     return NULL;

   cfdata->ui.table = e_widget_table_add(evas, 0);
   _basic_create_general(evas, cfdata);
   _basic_create_cards(evas, cfdata);
   _basic_create_channels(evas, cfdata);

   e_widget_table_object_append(cfdata->ui.table, cfdata->ui.general.frame,
                                0, 0, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(cfdata->ui.table, cfdata->ui.cards.frame,
                                0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(cfdata->ui.table, cfdata->ui.channels.frame,
                                0, 2, 1, 1, 1, 1, 1, 1);

   return cfdata->ui.table;
}

E_Config_Dialog *
e_mixer_config_dialog_new(E_Container *con, E_Mixer_Gadget_Config *conf)
{
   E_Config_Dialog *dialog;
   E_Config_Dialog_View *view;

   if (e_config_dialog_find(_Name, "e_mixer_config_dialog_new"))
       return NULL;

   view = E_NEW(E_Config_Dialog_View, 1);
   if (!view)
       return NULL;

   view->create_cfdata = _create_data;
   view->free_cfdata = _free_data;
   view->basic.create_widgets = _basic_create;
   view->basic.apply_cfdata = _basic_apply;

   dialog = e_config_dialog_new(con, _("Mixer Configuration"),
                                _Name, "e_mixer_config_dialog_new",
                                e_mixer_theme_path(), 0, view, conf);
   e_dialog_resizable_set(dialog->dia, 1);

   return dialog;
}
