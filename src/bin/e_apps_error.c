/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"


struct _E_Config_Dialog_Data
{
   char *label;
   char *exit;
   char *signal;
};


/* Protos */
static void *       _e_app_error_dialog_create_data(E_Config_Dialog *cfd);
static void         _e_app_error_dialog_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_e_app_error_dialog_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_e_app_error_dialog_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

void 
_e_app_error_dialog(E_Container *con, E_App_Autopsy *app)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);  /* This gets freed by e_config_dialog. */
   if (v)
      {
         /* Dialog Methods */
         v->create_cfdata = _e_app_error_dialog_create_data;
         v->free_cfdata = _e_app_error_dialog_free_data;
         v->basic.create_widgets = _e_app_error_dialog_basic_create_widgets;
         v->advanced.create_widgets = _e_app_error_dialog_advanced_create_widgets;

         /* Create The Dialog */
         cfd = e_config_dialog_new(con, _("Run error, wtf?  That sux."), NULL, 0, v, app);
         app->error_dialog = cfd;
      }
}

static void 
_e_app_error_dialog_fill_data(E_App_Autopsy *app, E_Config_Dialog_Data *cfdata)
{
   int length;

   length = strlen(app->app->name);
   if (!cfdata->label)
      {
         cfdata->label = malloc((length + 20) * sizeof(char));
         if (cfdata->label)
            sprintf(cfdata->label, "%s may have crashed.", app->app->name);
      }

   length = strlen(app->app->exe);
   if ((app->del.exited) && (!cfdata->exit))
      {
         cfdata->exit = malloc((length + 64) * sizeof(char));
         if (cfdata->exit)
            sprintf(cfdata->exit, "An exit code of %i was returned from %s", app->del.exit_code, app->app->exe);
      }
   if ((app->del.signalled) && (!cfdata->signal))
      {
         cfdata->signal = malloc((length + 64) * sizeof(char));
         if (cfdata->signal)
            sprintf(cfdata->signal, "%s was interupted by signal %i", app->app->exe, app->del.exit_signal);   /* FIXME: add a description of the signal. */
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

static void *
_e_app_error_dialog_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   E_App_Autopsy *app;
   
   app = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _e_app_error_dialog_fill_data(app, cfdata);
   return cfdata;
}

static void 
_e_app_error_dialog_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_App_Autopsy *app;

   app = cfd->data;

   if(app)
      {
         app->error_dialog = NULL;
         if (app->error)
            ecore_exe_event_data_free(app->error);
         if (app->read)
            ecore_exe_event_data_free(app->read);
         free(app);
      }
   if (cfdata->label)
      free(cfdata->signal);
   if (cfdata->label)
      free(cfdata->exit);
   if (cfdata->label)
      free(cfdata->label);

   free(cfdata);
}

static void
_e_app_error_dialog_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord mw, mh, vw, vh, w, h;
   
   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_widget_min_size_get(data, &mw, &mh);
   evas_object_geometry_get(data, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
	if (w != vw) evas_object_resize(data, vw, h);
     }
}

static Evas_Object *
_e_app_error_dialog_scrolltext_create(Evas *evas, char *title, Ecore_Exe_Event_Data_Line *lines)
{
   int i;
   Evas_Object *obj, *os;

   os = e_widget_framelist_add(evas, _(title), 0);

   obj = e_widget_ilist_add(evas, 0, 0, NULL);

   for (i = 0; lines[i].line != NULL; i++)
      e_widget_ilist_append(obj, NULL, lines[i].line, NULL, NULL, NULL);
   e_widget_min_size_set(obj, 100, 100);

   e_widget_framelist_object_append(os, obj);

   return os;
}

static Evas_Object *
_e_app_error_dialog_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   int error_length = 0;
   Evas_Object *o, *o2, *of, *ob, *os;
   E_App_Autopsy *app;

   app = cfd->data;
   _e_app_error_dialog_fill_data(app, cfdata);

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_label_add(evas, cfdata->label);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   if (app->error)
      error_length = app->error->size;
   if (error_length)
      {
         os = _e_app_error_dialog_scrolltext_create(evas, "Error", app->error->lines);
         e_widget_list_object_append(o, os, 1, 1, 0.5);
      }
   else
      {
         ob = e_widget_label_add(evas, _("There was no error message."));
         e_widget_list_object_append(o, ob, 1, 1, 0.5);
      }

   return o;
}

static Evas_Object *
_e_app_error_dialog_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   int read_length = 0;
   int error_length = 0;
   Evas_Object *o, *of, *ob, *ot, *os;
   E_App_Autopsy *app;
   
   app = cfd->data;
   _e_app_error_dialog_fill_data(app, cfdata);

   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 0);

   ob = e_widget_label_add(evas, cfdata->label);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   if (cfdata->exit)
      {
         of = e_widget_framelist_add(evas, _("Exit code"), 0);
         ob = e_widget_label_add(evas, _(cfdata->exit));
         e_widget_framelist_object_append(of, ob);
         e_widget_list_object_append(o, of, 1, 1, 0.5);
      }

   if (cfdata->signal)
      {
         of = e_widget_framelist_add(evas, _("Signal"), 0);
         ob = e_widget_label_add(evas, _(cfdata->signal));
         e_widget_framelist_object_append(of, ob);
         e_widget_list_object_append(o, of, 1, 1, 0.5);
      }

   if (app->read)
      read_length = app->read->size;

   if (read_length)
      {
         of = _e_app_error_dialog_scrolltext_create(evas, "Output", app->read->lines);
/* FIXME: Add stdout "start". */
/* FIXME: Add stdout "end". */
      }
   else
      {
         of = e_widget_framelist_add(evas, _("Output"), 0);
         ob = e_widget_label_add(evas, _("There was no output."));
         e_widget_framelist_object_append(of, ob);
      }
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   if (app->error)
      error_length = app->error->size;
   if (error_length)
      {
         of = _e_app_error_dialog_scrolltext_create(evas, "Error", app->error->lines);
/* FIXME: Add stderr "start". */
/* FIXME: Add stderr "end". */
      }
   else
      {
         of = e_widget_framelist_add(evas, _("Error"), 0);
         ob = e_widget_label_add(evas, _("There was no error message."));
         e_widget_framelist_object_append(of, ob);
      }
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   
   return o;
}
