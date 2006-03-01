/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

struct _E_Config_Dialog_Data
{
   char               *label;
   char               *exit;
   char               *signal;
};

/* Protos */
static void        *_e_app_error_dialog_create_data(E_Config_Dialog * cfd);
static void         _e_app_error_dialog_free_data(E_Config_Dialog * cfd,
						  E_Config_Dialog_Data *
						  cfdata);
static Evas_Object *_e_app_error_dialog_basic_create_widgets(E_Config_Dialog *
							     cfd, Evas * evas,
							     E_Config_Dialog_Data
							     * cfdata);
static Evas_Object *_e_app_error_dialog_advanced_create_widgets(E_Config_Dialog
								* cfd,
								Evas * evas,
								E_Config_Dialog_Data
								* cfdata);
static void         _e_app_error_dialog_resize(void *data, Evas * e,
					       Evas_Object * obj,
					       void *event_info);
static Evas_Object *_e_app_error_dialog_scrolltext_create(Evas * evas,
							  char *title,
							  Ecore_Exe_Event_Data_Line
							  * lines);
static void         _e_app_error_dialog_save_cb(void *data, void *data2);

EAPI void
e_app_error_dialog(E_Container * con, E_App_Autopsy * app)
{
   E_Config_Dialog    *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);	/* This gets freed by e_config_dialog. */
   if (v)
     {
	/* Dialog Methods */
	v->create_cfdata = _e_app_error_dialog_create_data;
	v->free_cfdata = _e_app_error_dialog_free_data;
	v->basic.create_widgets = _e_app_error_dialog_basic_create_widgets;
	v->advanced.create_widgets =
	   _e_app_error_dialog_advanced_create_widgets;

	/* Create The Dialog */
	cfd =
	   e_config_dialog_new(con, _("Application Execution Error"), NULL, 0,
			       v, app);
	app->error_dialog = cfd;
     }
}

static void
_e_app_error_dialog_fill_data(E_App_Autopsy * app,
			      E_Config_Dialog_Data * cfdata)
{
   char                buf[4096];

   if (!cfdata->label)
     {
	snprintf(buf, sizeof(buf), _("%s stopped running unexpectedly."),
		 app->app->name);
	cfdata->label = strdup(buf);
     }

   if ((app->del.exited) && (!cfdata->exit))
     {
	snprintf(buf, sizeof(buf), _("An exit code of %i was returned from %s"),
		 app->del.exit_code, app->app->exe);
	cfdata->exit = strdup(buf);
     }
   if ((app->del.signalled) && (!cfdata->signal))
     {
	if (app->del.exit_signal == SIGINT)
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by an Interrupt Signal"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGQUIT)
	   snprintf(buf, sizeof(buf), _("%s was interrupted by a Quit Signal"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGABRT)
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by an Abort Signal"), app->app->exe);
	else if (app->del.exit_signal == SIGFPE)
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by a Floating Point Error"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGKILL)
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by an Uninterruptable Kill Signal"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGSEGV)
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by a Segmentation Fault"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGPIPE)
	   snprintf(buf, sizeof(buf), _("%s was interrupted by a Broken Pipe"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGTERM)
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by a Termination Singal"),
		    app->app->exe);
	else if (app->del.exit_signal == SIGBUS)
	   snprintf(buf, sizeof(buf), _("%s was interrupted by a Bus Error"),
		    app->app->exe);
	else
	   snprintf(buf, sizeof(buf),
		    _("%s was interrupted by the signal number %i"),
		    app->app->exe, app->del.exit_signal);
	cfdata->signal = strdup(buf);
	/* FIXME: Add  sigchld_info stuff
	 * app->del.data
	 *    siginfo_t
	 *    {
	 *       int      si_signo;     Signal number 
	 *       int      si_errno;     An errno value 
	 *       int      si_code;      Signal code 
	 *       pid_t    si_pid;       Sending process ID 
	 *       uid_t    si_uid;       Real user ID of sending process 
	 *       int      si_status;    Exit value or signal 
	 *       clock_t  si_utime;     User time consumed 
	 *       clock_t  si_stime;     System time consumed 
	 *       sigval_t si_value;     Signal value 
	 *       int      si_int;       POSIX.1b signal 
	 *       void *   si_ptr;       POSIX.1b signal 
	 *       void *   si_addr;      Memory location which caused fault 
	 *       int      si_band;      Band event 
	 *       int      si_fd;        File descriptor 
	 *    }
	 */
     }
}

static void        *
_e_app_error_dialog_create_data(E_Config_Dialog * cfd)
{
   E_Config_Dialog_Data *cfdata;
   E_App_Autopsy      *app;

   app = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _e_app_error_dialog_fill_data(app, cfdata);
   return cfdata;
}

static void
_e_app_error_dialog_free_data(E_Config_Dialog * cfd,
			      E_Config_Dialog_Data * cfdata)
{
   E_App_Autopsy      *app;

   app = cfd->data;

   if (app)
     {
	app->error_dialog = NULL;
	if (app->error)
	   ecore_exe_event_data_free(app->error);
	if (app->read)
	   ecore_exe_event_data_free(app->read);
	free(app);
     }
   if (cfdata->signal)
      free(cfdata->signal);
   if (cfdata->exit)
      free(cfdata->exit);
   if (cfdata->label)
      free(cfdata->label);

   free(cfdata);
}

static void
_e_app_error_dialog_resize(void *data, Evas * e, Evas_Object * obj,
			   void *event_info)
{
   Evas_Coord          mw, mh, vw, vh, w, h;

   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_widget_min_size_get(data, &mw, &mh);
   evas_object_geometry_get(data, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
	if (w != vw)
	   evas_object_resize(data, vw, h);
     }
}

static Evas_Object *
_e_app_error_dialog_scrolltext_create(Evas * evas, char *title,
				      Ecore_Exe_Event_Data_Line * lines)
{
   int                 i;
   Evas_Object        *obj, *os;
   char               *text;
   int                 tlen;

   os = e_widget_framelist_add(evas, _(title), 0);

   obj = e_widget_textblock_add(evas);

   tlen = 0;
   for (i = 0; lines[i].line != NULL; i++)
      tlen += lines[i].size + 1;
   text = alloca(tlen + 1);
   if (text)
     {
	text[0] = 0;
	for (i = 0; lines[i].line != NULL; i++)
	  {
	     strcat(text, lines[i].line);
	     strcat(text, "\n");
	  }
	e_widget_textblock_plain_set(obj, text);
     }
   e_widget_min_size_set(obj, 240, 120);

   e_widget_framelist_object_append(os, obj);

   return os;
}

static Evas_Object *
_e_app_error_dialog_basic_create_widgets(E_Config_Dialog * cfd, Evas * evas,
					 E_Config_Dialog_Data * cfdata)
{
   int                 error_length = 0;
   Evas_Object        *o, *ob, *os;
   E_App_Autopsy      *app;

   app = cfd->data;
   _e_app_error_dialog_fill_data(app, cfdata);

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_label_add(evas, cfdata->label);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   if (app->error)
      error_length = app->error->size;
   if (error_length)
     {
	os =
	   _e_app_error_dialog_scrolltext_create(evas, _("Error Logs"),
						 app->error->lines);
	e_widget_list_object_append(o, os, 1, 1, 0.5);
     }
   else
     {
	ob = e_widget_label_add(evas, _("There was no error message."));
	e_widget_list_object_append(o, ob, 1, 1, 0.5);
     }

   ob =
      e_widget_button_add(evas, _("Save This Message"), "enlightenment/run",
			  _e_app_error_dialog_save_cb, app, cfdata);
   e_widget_list_object_append(o, ob, 0, 0, 0.5);

   return o;
}

static Evas_Object *
_e_app_error_dialog_advanced_create_widgets(E_Config_Dialog * cfd, Evas * evas,
					    E_Config_Dialog_Data * cfdata)
{
   int                 read_length = 0;
   int                 error_length = 0;
   Evas_Object        *o, *of, *ob, *ot;
   E_App_Autopsy      *app;

   app = cfd->data;
   _e_app_error_dialog_fill_data(app, cfdata);

   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 0);

   ob = e_widget_label_add(evas, cfdata->label);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   if (cfdata->exit)
     {
	of = e_widget_framelist_add(evas, _("Error Information"), 0);
	ob = e_widget_label_add(evas, _(cfdata->exit));
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }

   if (cfdata->signal)
     {
	of = e_widget_framelist_add(evas, _("Error Signal Information"), 0);
	ob = e_widget_label_add(evas, _(cfdata->signal));
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }

   if (app->read)
      read_length = app->read->size;

   if (read_length)
     {
	of =
	   _e_app_error_dialog_scrolltext_create(evas, _("Output Data"),
						 app->read->lines);
	/* FIXME: Add stdout "start". */
	/* FIXME: Add stdout "end". */
     }
   else
     {
	of = e_widget_framelist_add(evas, _("Output Data"), 0);
	ob = e_widget_label_add(evas, _("There was no output."));
	e_widget_framelist_object_append(of, ob);
     }
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   if (app->error)
      error_length = app->error->size;
   if (error_length)
     {
	of =
	   _e_app_error_dialog_scrolltext_create(evas, _("Error Logs"),
						 app->error->lines);
	/* FIXME: Add stderr "start". */
	/* FIXME: Add stderr "end". */
     }
   else
     {
	of = e_widget_framelist_add(evas, _("Error Logs"), 0);
	ob = e_widget_label_add(evas, _("There was no error message."));
	e_widget_framelist_object_append(of, ob);
     }
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   ob =
      e_widget_button_add(evas, _("Save This Message"), "enlightenment/run",
			  _e_app_error_dialog_save_cb, app, cfdata);
   e_widget_list_object_append(o, ob, 0, 0, 0.5);

   return o;
}

static void
_e_app_error_dialog_save_cb(void *data, void *data2)
{
   E_App_Autopsy      *app;
   E_Config_Dialog_Data *cfdata;
   FILE               *f;
   char               *text;
   char                buf[1024];
   char                buffer[4096];
   int                 read_length = 0;
   int                 i, tlen;

   app = data;
   cfdata = data2;

   snprintf(buf, sizeof(buf), "%s/%s.log", e_user_homedir_get(),
	    app->app->name);
   f = fopen(buf, "w");
   if (!f)
      return;

   if (cfdata->exit)
     {
	snprintf(buffer, sizeof(buffer), "Error Information:\n\t%s\n\n",
		 cfdata->exit);
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }
   if (cfdata->signal)
     {
	snprintf(buffer, sizeof(buffer), "Error Signal Information:\n\t%s\n\n",
		 cfdata->signal);
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }

   if (app->read)
      read_length = app->read->size;

   if (read_length)
     {
	tlen = 0;
	for (i = 0; app->read->lines[i].line != NULL; i++)
	   tlen += app->read->lines[i].size + 1;
	text = alloca(tlen + 1);
	if (text)
	  {
	     text[0] = 0;
	     for (i = 0; app->read->lines[i].line != NULL; i++)
	       {
		  strcat(text, "\t");
		  strcat(text, app->read->lines[i].line);
		  strcat(text, "\n");
	       }
	     snprintf(buffer, sizeof(buffer), "Output Data:\n%s\n\n", text);
	     fwrite(buffer, sizeof(char), strlen(buffer), f);
	  }
     }
   else
     {
	snprintf(buffer, sizeof(buffer),
		 "Output Data:\n\tThere was no output\n\n");
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }

   /* Reusing this var */
   read_length = 0;
   if (app->error)
      read_length = app->error->size;

   if (read_length)
     {
	tlen = 0;
	for (i = 0; app->error->lines[i].line != NULL; i++)
	   tlen += app->error->lines[i].size + 1;
	text = alloca(tlen + 1);
	if (text)
	  {
	     text[0] = 0;
	     for (i = 0; app->error->lines[i].line != NULL; i++)
	       {
		  strcat(text, "\t");
		  strcat(text, app->error->lines[i].line);
		  strcat(text, "\n");
	       }
	     snprintf(buffer, sizeof(buffer), "Error Logs:\n%s\n", text);
	     fwrite(buffer, sizeof(char), strlen(buffer), f);
	  }
     }
   else
     {
	snprintf(buffer, sizeof(buffer),
		 "Error Logs:\n\tThere was no error message\n");
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }

   fclose(f);
}
