#include "e_mod_main.h"

extern const char _Name[];

typedef struct E_Mixer_App_Dialog_Data
{
   E_Mixer_System *sys;
   char *card;
   char *channel_name;
   int lock_sliders;
   Eina_List *cards;
   Eina_List *channels_infos;
   struct channel_info *channel_info;
   E_Mixer_Channel_State state;

   struct e_mixer_app_ui
     {
        Evas_Object *hlayout;
        struct e_mixer_app_ui_cards
	  {
	     Evas_Object *frame;
	     Evas_Object *list;
	  } cards;
        struct e_mixer_app_ui_channels
	  {
	     Evas_Object *frame;
	     Evas_Object *list;
	  } channels;
        struct e_mixer_app_ui_channel_editor
	  {
 	     Evas_Object *frame;
	     Evas_Object *label_card;
	     Evas_Object *card;
	     Evas_Object *label_channel;
	     Evas_Object *channel;
	     Evas_Object *label_type;
	     Evas_Object *type;
	     Evas_Object *label_left;
	     Evas_Object *left;
	     Evas_Object *label_right;
	     Evas_Object *right;
	     Evas_Object *mute;
	     Evas_Object *lock_sliders;
   	  } channel_editor;
     } ui;

   struct
     {
        void *data;
        void (*func)(E_Dialog *dialog, void *data);
     } del;
} E_Mixer_App_Dialog_Data;

struct channel_info
{
   int has_capture;
   char *name;
   E_Mixer_Channel *id;
   E_Mixer_App_Dialog_Data *app;
};

static void
_cb_changed_left(void *data, Evas_Object *obj)
{
   E_Mixer_App_Dialog_Data *app = data;
   E_Mixer_Channel_State *state;

   state = &app->state;
   if (app->lock_sliders && (state->left != state->right))
     {
	state->right = state->left;
	e_widget_slider_value_int_set(app->ui.channel_editor.right,
				      state->right);
     }

   e_mixer_system_set_volume(app->sys, app->channel_info->id,
			     state->left, state->right);
}

static void
_cb_changed_right(void *data, Evas_Object *obj)
{
   E_Mixer_App_Dialog_Data *app = data;
   E_Mixer_Channel_State *state;

   state = &app->state;
   if (app->lock_sliders && (state->right != state->left))
     {
	state->left = state->right;
	e_widget_slider_value_int_set(app->ui.channel_editor.left,
				      state->left);
     }

   e_mixer_system_set_volume(app->sys, app->channel_info->id,
			     state->left, state->right);
}

static void
_cb_changed_mute(void *data, Evas_Object *obj)
{
   E_Mixer_App_Dialog_Data *app = data;

   e_mixer_system_set_mute(app->sys, app->channel_info->id, app->state.mute);
}

static void
_cb_changed_lock_sliders(void *data, Evas_Object *obj)
{
   E_Mixer_App_Dialog_Data *app = data;
   E_Mixer_Channel_State *state;

   if (!app->lock_sliders)
     return;

   state = &app->state;
   if (state->left == state->right)
     return;

   state->left = state->right = (state->left + state->right) / 2;

   e_widget_slider_value_int_set(app->ui.channel_editor.left, state->left);
   e_widget_slider_value_int_set(app->ui.channel_editor.right, state->right);
   e_mixer_system_set_volume(app->sys, app->channel_info->id,
			     state->left, state->right);
}

static void
_update_channel_editor_state(E_Mixer_App_Dialog_Data *app, const E_Mixer_Channel_State state)
{
   struct e_mixer_app_ui_channel_editor *ui;

   ui = &app->ui.channel_editor;

   e_widget_slider_value_int_set(ui->left, state.left);
   e_widget_slider_value_int_set(ui->right, state.right);

   if (e_mixer_system_can_mute(app->sys, app->channel_info->id))
     {
	e_widget_disabled_set(ui->mute, 0);
	e_widget_check_checked_set(ui->mute, state.mute);
     }
   else
     {
	e_widget_disabled_set(ui->mute, 1);
	e_widget_check_checked_set(ui->mute, 0);
     }
}

static void
_populate_channel_editor(E_Mixer_App_Dialog_Data *app)
{
   struct e_mixer_app_ui_channel_editor *ui;
   E_Mixer_Channel_State state;
   char *card_name;

   ui = &app->ui.channel_editor;

   card_name = e_mixer_system_get_card_name(app->card);
   e_widget_entry_text_set(ui->card, card_name);
   if (card_name)
     free(card_name);

   e_widget_entry_text_set(ui->channel, app->channel_name);

   if (e_mixer_system_has_capture(app->sys, app->channel_info->id))
     e_widget_entry_text_set(ui->type, _("Capture"));
   else
     e_widget_entry_text_set(ui->type, _("Playback"));

   e_mixer_system_get_state(app->sys, app->channel_info->id, &state);
   _update_channel_editor_state(app, state);

   app->lock_sliders = (state.left == state.right);
   e_widget_check_checked_set(ui->lock_sliders, app->lock_sliders);
}

static void
_cb_channel_selected(void *data)
{
   struct channel_info *info = data;
   E_Mixer_App_Dialog_Data *app;

   app = info->app;
   app->channel_info = info;
   _populate_channel_editor(app);
}

static int
_channel_info_cmp(const void *data_a, const void *data_b)
{
   const struct channel_info *a = data_a, *b = data_b;

   if (a->has_capture < b->has_capture)
     return -1;
   else if (a->has_capture > b->has_capture)
     return 1;

   return strcmp(a->name, b->name);
}

static Eina_List *
_channels_info_new(E_Mixer_System *sys)
{
   Eina_List *channels, *channels_infos, *l;

   channels = e_mixer_system_get_channels(sys);
   channels_infos = NULL;
   for (l = channels; l != NULL; l = l->next)
     {
	struct channel_info *info;

	info = malloc(sizeof(*info));
	info->id = l->data;
	info->name = e_mixer_system_get_channel_name(sys, info->id);
	info->has_capture = e_mixer_system_has_capture(sys, info->id);

	channels_infos = eina_list_append(channels_infos, info);
     }
   e_mixer_system_free_channels(channels);

   return eina_list_sort(channels_infos, -1, _channel_info_cmp);
}

static void
_channels_info_free(Eina_List *list)
{
   Eina_List *l;

   for (l = list; l != NULL; l = l->next)
     {
	struct channel_info *info = l->data;
	free(info->name);
	free(info);
     }

   eina_list_free(list);
}

static int
_cb_system_update(void *data, E_Mixer_System *sys)
{
   E_Mixer_App_Dialog_Data *app;
   E_Mixer_Channel_State state;

   app = data;

   if ((!app->sys) || (!app->channel_info))
     return 1;

   e_mixer_system_get_state(app->sys, app->channel_info->id, &state);
   _update_channel_editor_state(app, state);

   return 1;
}


static void
_populate_channels(E_Mixer_App_Dialog_Data *app)
{
   Eina_List *l;
   Evas_Object *ilist;
   int header_input;
   int i;

   ilist = app->ui.channels.list;
   edje_freeze();
   e_widget_ilist_freeze(ilist);
   e_widget_ilist_clear(ilist);

   if (app->sys)
     e_mixer_system_del(app->sys);
   app->sys = e_mixer_system_new(app->card);
   e_mixer_system_callback_set(app->sys, _cb_system_update, app);

   if (app->channel_name)
     free(app->channel_name);
   app->channel_name = e_mixer_system_get_default_channel_name(app->sys);

   if (app->channels_infos)
     _channels_info_free(app->channels_infos);
   app->channels_infos = _channels_info_new(app->sys);

   if (app->channels_infos)
     {
	struct channel_info *info = app->channels_infos->data;
	if (info->has_capture)
	  {
	     e_widget_ilist_header_append(ilist, NULL, _("Input"));
	     header_input = 1;
	     i = 1;
	  }
	else
	  {
	     e_widget_ilist_header_append(ilist, NULL, _("Output"));
	     header_input = 0;
	     i = 1;
	  }
     }

   for (l = app->channels_infos; l != NULL; l = l->next, i++)
     {
	struct channel_info *info = l->data;

	if ((!header_input) && info->has_capture)
	  {
	     e_widget_ilist_header_append(ilist, NULL, _("Input"));
	     header_input = 1;
	     i++;
	  }

	info->app = app;
	e_widget_ilist_append(ilist, NULL, info->name, _cb_channel_selected,
			      info, info->name);
	if (app->channel_name && info->name &&
	    (strcmp(app->channel_name, info->name) == 0))
	  {
	     e_widget_ilist_selected_set(ilist, i);
	     app->channel_info = info;
	  }
     }

   e_widget_ilist_go(ilist);
   e_widget_ilist_thaw(ilist);
   edje_thaw();
}

static void
select_card(E_Mixer_App_Dialog_Data *app)
{
   _populate_channels(app);
   e_widget_ilist_selected_set(app->ui.channels.list, 1);
}

static void
_cb_card_selected(void *data)
{
   select_card(data);
}

static void
_create_cards(E_Dialog *dialog, Evas *evas, E_Mixer_App_Dialog_Data *app)
{
   struct e_mixer_app_ui_cards *ui;
   Eina_List *l;

   app->card = e_mixer_system_get_default_card();
   app->cards = e_mixer_system_get_cards();
   if (eina_list_count(app->cards) < 2)
       return;

   ui = &app->ui.cards;
   ui->list = e_widget_ilist_add(evas, 32, 32, &app->card);
   e_widget_ilist_go(ui->list);
   for (l = app->cards; l != NULL; l = l->next)
     {
         char *card, *card_name;

	card = l->data;
	card_name = e_mixer_system_get_card_name(card);

	e_widget_ilist_append(ui->list, NULL, card_name, _cb_card_selected,
			      app, card);

	free(card_name);
     }

   ui->frame = e_widget_framelist_add(evas, _("Cards"), 0);
   e_widget_framelist_object_append(ui->frame, ui->list);
   e_widget_list_object_append(app->ui.hlayout, ui->frame, 1, 0, 0.0);
}

static void
_create_channels(E_Dialog *dialog, Evas *evas, E_Mixer_App_Dialog_Data *app)
{
   struct e_mixer_app_ui_channels *ui;

   ui = &app->ui.channels;
   ui->list = e_widget_ilist_add(evas, 24, 24, &app->channel_name);
   e_widget_min_size_set(ui->list, 180, 100);
   e_widget_ilist_go(ui->list);

   ui->frame = e_widget_framelist_add(evas, _("Channels"), 0);
   e_widget_framelist_object_append(ui->frame, ui->list);
   e_widget_list_object_append(app->ui.hlayout, ui->frame, 1, 1, 0.0);
}

static void
_create_channel_editor(E_Dialog *dialog, Evas *evas, E_Mixer_App_Dialog_Data *app)
{
   struct e_mixer_app_ui_channel_editor *ui;

   ui = &app->ui.channel_editor;

   ui->label_card = e_widget_label_add(evas, _("Card:"));
   ui->card = e_widget_entry_add(evas, NULL, NULL, NULL, NULL);
   e_widget_entry_readonly_set(ui->card, 1);

   ui->label_channel = e_widget_label_add(evas, _("Channel:"));
   ui->channel = e_widget_entry_add(evas, NULL, NULL, NULL, NULL);
   e_widget_entry_readonly_set(ui->channel, 1);

   ui->label_type = e_widget_label_add(evas, _("Type:"));
   ui->type = e_widget_entry_add(evas, NULL, NULL, NULL, NULL);
   e_widget_entry_readonly_set(ui->type, 1);

   ui->label_left = e_widget_label_add(evas, _("Left:"));
   ui->left = e_widget_slider_add(evas, 1, 0, "%3.0f", 0.0, 100.0, 10.0, 100.0,
				  NULL, &app->state.left, 150);
   e_widget_on_change_hook_set(ui->left, _cb_changed_left, app);

   ui->label_right = e_widget_label_add(evas, _("Right:"));
   ui->right = e_widget_slider_add(evas, 1, 0, "%3.0f", 0.0, 100.0, 10.0, 100.0,
				   NULL, &app->state.right, 150);
   e_widget_on_change_hook_set(ui->right, _cb_changed_right, app);

   ui->mute = e_widget_check_add(evas, _("Mute"), &app->state.mute);
   e_widget_on_change_hook_set(ui->mute, _cb_changed_mute, app);

   ui->lock_sliders = e_widget_check_add(evas, _("Lock Sliders"),
					 &app->lock_sliders);
   e_widget_on_change_hook_set(ui->lock_sliders, _cb_changed_lock_sliders, app);

   ui->frame = e_widget_framelist_add(evas, _("Edit"), 0);
   e_widget_framelist_object_append(ui->frame, ui->label_card);
   e_widget_framelist_object_append(ui->frame, ui->card);
   e_widget_framelist_object_append(ui->frame, ui->label_channel);
   e_widget_framelist_object_append(ui->frame, ui->channel);
   e_widget_framelist_object_append(ui->frame, ui->label_type);
   e_widget_framelist_object_append(ui->frame, ui->type);
   e_widget_framelist_object_append(ui->frame, ui->label_left);
   e_widget_framelist_object_append(ui->frame, ui->left);
   e_widget_framelist_object_append(ui->frame, ui->label_right);
   e_widget_framelist_object_append(ui->frame, ui->right);
   e_widget_framelist_object_append(ui->frame, ui->mute);
   e_widget_framelist_object_append(ui->frame, ui->lock_sliders);

   e_widget_list_object_append(app->ui.hlayout, ui->frame, 1, 1, 0.5);
}

static void
_create_ui(E_Dialog *dialog, E_Mixer_App_Dialog_Data *app)
{
   struct e_mixer_app_ui *ui;
   Evas *evas;
   int mw, mh;

   evas = e_win_evas_get(dialog->win);

   ui = &app->ui;

   ui->hlayout = e_widget_list_add(evas, 0, 1);
   _create_cards(dialog, evas, app);
   _create_channels(dialog, evas, app);
   _create_channel_editor(dialog, evas, app);

   if (ui->cards.list)
     e_widget_ilist_selected_set(ui->cards.list, 0);
   else
     select_card(app);
   e_widget_ilist_selected_set(ui->channels.list, 1);

   e_widget_min_size_get(ui->hlayout, &mw, &mh);
   if (mw < 300)
     mw = 300;
   if (mh < 200)
     mh = 200;
   e_dialog_content_set(dialog, ui->hlayout, mw, mh);
}

static void
_mixer_app_dialog_del(E_Dialog *dialog, E_Mixer_App_Dialog_Data *app)
{
   if (app->del.func)
     app->del.func(dialog, app->del.data);

   if (app->card)
     free(app->card);
   if (app->channel_name)
     free(app->channel_name);
   if (app->cards)
     e_mixer_system_free_cards(app->cards);
   if (app->channels_infos)
     _channels_info_free(app->channels_infos);
   e_mixer_system_del(app->sys);

   e_util_defer_object_del(E_OBJECT(dialog));
   dialog->data = NULL;
   E_FREE(app);
}

static void
_cb_win_del(E_Win *win)
{
   E_Dialog *dialog;
   E_Mixer_App_Dialog_Data *app;

   dialog = win->data;
   app = dialog->data;

   _mixer_app_dialog_del(dialog, app);
}

static void
_cb_dialog_dismiss(void *data, E_Dialog *dialog)
{
   _mixer_app_dialog_del(dialog, data);
}

E_Dialog *
e_mixer_app_dialog_new(E_Container *con, void (*func)(E_Dialog *dialog, void *data), void *data)
{
   E_Mixer_App_Dialog_Data *app;
   E_Dialog *dialog;

   dialog = e_dialog_new(con, _Name, "e_mixer_app_dialog");
   if (!dialog)
     return NULL;

   app = E_NEW(E_Mixer_App_Dialog_Data, 1);
   if (!app)
     {
	e_object_del(E_OBJECT(dialog));
	return NULL;
     }

   dialog->data = app;
   app->del.data = data;
   app->del.func = func;

   e_dialog_title_set(dialog, _(_Name));
   e_dialog_resizable_set(dialog, 1);

   e_win_delete_callback_set(dialog->win, _cb_win_del);

   _create_ui(dialog, app);

   e_dialog_button_add(dialog, _("Close"), NULL, _cb_dialog_dismiss, app);
   e_dialog_button_focus_num(dialog, 1);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);
   e_dialog_border_icon_set(dialog, "preferences-desktop-mixer");

   // FIXME: what if module unloaded while mixer_app dialog up? bad!
   
   return dialog;
}

static inline int
_find_card_by_name(E_Mixer_App_Dialog_Data *app, const char *card_name)
{
   Eina_List *l;
   int i;

   for (i = 0, l = app->cards; l != NULL; i++, l = l->next)
     if (strcmp(card_name, l->data) == 0)
       return i;

   return -1;
}

static inline int
_find_channel_by_name(E_Mixer_App_Dialog_Data *app, const char *channel_name)
{
   Eina_List *l;
   int i, header_input;

   if (app->channels_infos)
     {
	struct channel_info *info = app->channels_infos->data;

	header_input = !!info->has_capture;
	i = 1;
     }

   for (l = app->channels_infos; l != NULL; l = l->next, i++)
     {
	struct channel_info *info = l->data;

	if ((!header_input) && info->has_capture)
	  {
	     header_input = 1;
	     i++;
	  }

	if (strcmp(channel_name, info->name) == 0)
	  return i;
     }

   return -1;
}

int
e_mixer_app_dialog_select(E_Dialog *dialog, const char *card_name, const char *channel_name)
{
   E_Mixer_App_Dialog_Data *app;
   int n;

   if (!dialog)
     return 0;

   app = dialog->data;
   if (!app)
     return 0;

   n = _find_card_by_name(app, card_name);
   if (n < 0)
     return 0;
   if (app->ui.cards.list)
     e_widget_ilist_selected_set(app->ui.cards.list, n);

   n = _find_channel_by_name(app, channel_name);
   if (n < 0)
     return 0;
   e_widget_ilist_selected_set(app->ui.channels.list, n);

   return 1;
}
