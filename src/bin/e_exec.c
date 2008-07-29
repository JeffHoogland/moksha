/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define MAX_OUTPUT_CHARACTERS 5000

/* TODO:
 * - Clear e_exec_instances on shutdown
 * - Clear e_exec_start_pending on shutdown
 * - Create border add handler
 * - Launch .desktop in terminal if .desktop requires it
 */

typedef struct _E_Exec_Launch   E_Exec_Launch;
typedef struct _E_Exec_Search   E_Exec_Search;

struct _E_Exec_Launch
{
   E_Zone         *zone;
   const char     *launch_method;
};

struct _E_Exec_Search
{
   Efreet_Desktop *desktop;
   int             startup_id;
   pid_t           pid;
};

struct _E_Config_Dialog_Data
{
   Efreet_Desktop *desktop;
   char *exec;

   Ecore_Exe_Event_Del   event;
   Ecore_Exe_Event_Data *error;
   Ecore_Exe_Event_Data *read;

   char *label;
   char *exit;
   char *signal;
};

/* local subsystem functions */
static E_Exec_Instance *_e_exec_cb_exec(void *data, Efreet_Desktop *desktop, char *exec, int remaining);
static int  _e_exec_cb_expire_timer(void *data);
static int  _e_exec_cb_exit(void *data, int type, void *event);

static Evas_Bool _e_exec_startup_id_pid_find(const Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, void *value, void *data);

static void         _e_exec_error_dialog(Efreet_Desktop *desktop, const char *exec, Ecore_Exe_Event_Del *event, Ecore_Exe_Event_Data *error, Ecore_Exe_Event_Data *read);
static void         _fill_data(E_Config_Dialog_Data *cfdata);
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_dialog_scrolltext_create(Evas *evas, char *title, Ecore_Exe_Event_Data_Line *lines);
static void         _dialog_save_cb(void *data, void *data2);

/* local subsystem globals */
static Evas_List   *e_exec_start_pending = NULL;
static Evas_Hash   *e_exec_instances = NULL;
static int          startup_id = 0;

static Ecore_Event_Handler *_e_exec_exit_handler = NULL;
static Ecore_Event_Handler *_e_exec_border_add_handler = NULL;

/* externally accessible functions */
EAPI int
e_exec_init(void)
{
   _e_exec_exit_handler =
      ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_exec_cb_exit, NULL);
#if 0
   _e_exec_border_add_handler =
      ecore_event_handler_add(E_EVENT_BORDER_ADD, _e_exec_cb_event_border_add, NULL);
#endif
   return 1;
}

EAPI int
e_exec_shutdown(void)
{
   char buf[256];

   snprintf(buf, sizeof(buf), "%i", startup_id);
   e_util_env_set("E_STARTUP_ID", buf);

   if (_e_exec_exit_handler) ecore_event_handler_del(_e_exec_exit_handler);
   if (_e_exec_border_add_handler) ecore_event_handler_del(_e_exec_border_add_handler);
   evas_hash_free(e_exec_instances);
   evas_list_free(e_exec_start_pending);
   return 1;
}

EAPI E_Exec_Instance *
e_exec(E_Zone *zone, Efreet_Desktop *desktop, const char *exec,
       Ecore_List *files, const char *launch_method)
{
   E_Exec_Launch *launch;
   E_Exec_Instance *inst = NULL;

   if ((!desktop) && (!exec)) return NULL;
   launch = E_NEW(E_Exec_Launch, 1);
   if (!launch) return NULL;
   if (zone)
     {
	launch->zone = zone;
	e_object_ref(E_OBJECT(launch->zone));
     }
   if (launch_method) 
     launch->launch_method = evas_stringshare_add(launch_method);

   if (desktop)
     {
	if (exec)
	  inst = _e_exec_cb_exec(launch, NULL, strdup(exec), 0);
	else
	  inst = efreet_desktop_command_get(desktop, files, _e_exec_cb_exec, launch);
     }
   else
     inst = _e_exec_cb_exec(launch, NULL, strdup(exec), 0);
   return inst;
}

EAPI Efreet_Desktop *
e_exec_startup_id_pid_find(int startup_id, pid_t pid)
{
   E_Exec_Search search;

   search.desktop = NULL;
   search.startup_id = startup_id;
   search.pid = pid;
   evas_hash_foreach(e_exec_instances, _e_exec_startup_id_pid_find, &search);
   return search.desktop;
}

/* local subsystem functions */
static E_Exec_Instance *
_e_exec_cb_exec(void *data, Efreet_Desktop *desktop, char *exec, int remaining)
{
   E_Exec_Instance *inst = NULL;
   E_Exec_Launch *launch;
   Ecore_Exe *exe;
   char *penv_display;
   char buf[4096];

   launch = data;
   if (desktop)
     {
	inst = E_NEW(E_Exec_Instance, 1);
	if (!inst) return NULL;
     }

   if (startup_id == 0)
     {
	const char *p;
	p = getenv("E_STARTUP_ID");
	if (p) startup_id = atoi(p);
	e_util_env_set("E_STARTUP_ID", NULL);
     }
   if (++startup_id < 1) startup_id = 1;
   /* save previous env vars we need to save */
   penv_display = getenv("DISPLAY");
   if (penv_display) penv_display = strdup(penv_display);
   if ((penv_display) && (launch->zone))
     {
	const char *p1, *p2;
	char buf2[32];
	int head;

	head = launch->zone->container->manager->num;

	/* set env vars */
	p1 = strrchr(penv_display, ':');
	p2 = strrchr(penv_display, '.');
	if ((p1) && (p2) && (p2 > p1)) /* "blah:x.y" */
	  {
	     /* yes it could overflow... but who will overflow DISPLAY eh? why? to
	      * "exploit" your own applications running as you?
	      */
	     strcpy(buf, penv_display);
	     buf[p2 - penv_display + 1] = 0;
	     snprintf(buf2, sizeof(buf2), "%i", head);
	     strcat(buf, buf2);
	  }
	else if (p1) /* "blah:x */
	  {
	     strcpy(buf, penv_display);
	     snprintf(buf2, sizeof(buf2), ".%i", head);
	     strcat(buf, buf2);
	  }
	else
	  strcpy(buf, penv_display);
	e_util_env_set("DISPLAY", buf);
     }
   snprintf(buf, sizeof(buf), "E_START|%i", startup_id);
   e_util_env_set("DESKTOP_STARTUP_ID", buf);

   e_util_library_path_strip();
//// FIXME: seem to be some issues with the pipe and filling up ram - need to
//// check. for now disable.   
//   exe = ecore_exe_pipe_run(exec,
//			    ECORE_EXE_PIPE_AUTO | ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR |
//			    ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR_LINE_BUFFERED,
//			    inst);
   exe = ecore_exe_run(exec, inst);
   e_util_library_path_restore();
   if (penv_display)
     {
       	e_util_env_set("DISPLAY", penv_display);
	free(penv_display);
     }
   if (!exe)
     {
	E_FREE(inst);
	e_util_dialog_show(_("Run Error"),
			   _("Enlightenment was unable to fork a child process:<br>"
			     "<br>"
			     "%s<br>"),
			   exec);
	return NULL;
     }
   /* reset env vars */
   if (launch->launch_method) e_exehist_add(launch->launch_method, exec);
   free(exec);
   /* 20 lines at start and end, 20x100 limit on bytes at each end. */
//// FIXME: seem to be some issues with the pipe and filling up ram - need to
//// check. for now disable.   
//   ecore_exe_auto_limits_set(exe, 2000, 2000, 20, 20);
   ecore_exe_tag_set(exe, "E/exec");

   if (desktop)
     {
	Evas_List *l;

	efreet_desktop_ref(desktop);
	inst->desktop = desktop;
	inst->exe = exe;
	inst->startup_id = startup_id;
	inst->launch_time = ecore_time_get();
	inst->expire_timer = ecore_timer_add(10.0, _e_exec_cb_expire_timer, inst);

	l = evas_hash_find(e_exec_instances, desktop->orig_path);
	if (l)
	  {
	     l = evas_list_append(l, inst);
	     evas_hash_modify(e_exec_instances, desktop->orig_path, l);
	  }
	else
	  {
	     l = evas_list_append(l, inst);
	     e_exec_instances = evas_hash_add(e_exec_instances, desktop->orig_path, l);
	  }
	e_exec_start_pending = evas_list_append(e_exec_start_pending, desktop);
     }
   else if (exe)
     {
	E_FREE(inst);
	inst = NULL;
	ecore_exe_free(exe);
     }
   
   if (!remaining)
     {
	if (launch->launch_method) evas_stringshare_del(launch->launch_method);
	if (launch->zone) e_object_unref(E_OBJECT(launch->zone));
      	free(launch);
     }
   return inst;
}

static int
_e_exec_cb_expire_timer(void *data)
{
   E_Exec_Instance *inst;
 
   inst = data;
   e_exec_start_pending = evas_list_remove(e_exec_start_pending, inst->desktop);
   inst->expire_timer = NULL;
   return 0;
}

static int
_e_exec_cb_exit(void *data, int type, void *event)
{
   Evas_List *instances;
   Ecore_Exe_Event_Del *ev;
   E_Exec_Instance *inst;

   ev = event;
   printf("child exit...\n");
   if (!ev->exe) return 1;
   if (ecore_exe_tag_get(ev->exe)) printf("  tag %s\n", ecore_exe_tag_get(ev->exe));
   if (!(ecore_exe_tag_get(ev->exe) && 
	 (!strcmp(ecore_exe_tag_get(ev->exe), "E/exec")))) return 1;
   inst = ecore_exe_data_get(ev->exe);
   printf("  inst = %p\n", inst);
   if (!inst) return 1;

   printf("  inst exec line -- '%s'\n", ecore_exe_cmd_get(inst->exe));
   
   /* /bin/sh uses this if cmd not found */
   if ((ev->exited) &&
       ((ev->exit_code == 127) || (ev->exit_code == 255)))
     {
	E_Dialog *dia;
	
	dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
			   "E", "_e_exec_run_error_dialog");
	if (dia)
	  {
	     char buf[4096];
	     
	     e_dialog_title_set(dia, _("Application run error"));
	     snprintf(buf, sizeof(buf),
		      _("Enlightenment was unable to run the application:<br>"
			"<br>"
			"%s<br>"
			"<br>"
			"The application failed to start."),
		      ecore_exe_cmd_get(ev->exe));
	     e_dialog_text_set(dia, buf);
	     e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	     e_dialog_button_focus_num(dia, 1);
	     e_win_centered_set(dia->win, 1);
	     e_dialog_show(dia);
	  }
     }
   /* Let's hope that everything returns this properly. */
   else if (!((ev->exited) && (ev->exit_code == EXIT_SUCCESS))) 
     {
	/* filter out common exits via signals - int/term/quit. not really
	 * worth popping up a dialog for */
	if (!
	    ((ev->signalled) &&
	     ((ev->exit_signal == SIGINT) ||
	      (ev->exit_signal == SIGQUIT) ||
	      (ev->exit_signal == SIGTERM)))
	    )
	  {
	     /* Show the error dialog with details from the exe. */
	     _e_exec_error_dialog(inst->desktop, ecore_exe_cmd_get(ev->exe), ev,
				  ecore_exe_event_data_get(ev->exe, ECORE_EXE_PIPE_ERROR),
				  ecore_exe_event_data_get(ev->exe, ECORE_EXE_PIPE_READ));
	  }
     }
   if (inst->desktop)
     {
	instances = evas_hash_find(e_exec_instances, inst->desktop->orig_path);
	if (instances)
	  {
	     instances = evas_list_remove(instances, inst);
	     if (instances)
	       evas_hash_modify(e_exec_instances, inst->desktop->orig_path, instances);
	     else
	       e_exec_instances = evas_hash_del(e_exec_instances, inst->desktop->orig_path, NULL);
	  }
     }
   e_exec_start_pending = evas_list_remove(e_exec_start_pending, inst->desktop);
   if (inst->expire_timer) ecore_timer_del(inst->expire_timer);
   if (inst->desktop) efreet_desktop_free(inst->desktop);
   free(inst);
   return 1;
}

static Evas_Bool
_e_exec_startup_id_pid_find(const Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, void *value, void *data)
{
   E_Exec_Search *search;
   Evas_List *instances, *l;

   search = data;
   instances = value;
   for (l = instances; l; l = l->next)
     {
	E_Exec_Instance *inst;

	inst = l->data;
	if (((search->startup_id > 0) && (search->startup_id == inst->startup_id)) ||
	    ((inst->exe) && (search->pid > 1) && 
             (search->pid == ecore_exe_pid_get(inst->exe))))
	  {
	     search->desktop = inst->desktop;
	     return 0;
	  }
     }
   return 1;
}

static void    
_e_exec_error_dialog(Efreet_Desktop *desktop, const char *exec, Ecore_Exe_Event_Del *event,
		     Ecore_Exe_Event_Data *error, Ecore_Exe_Event_Data *read)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Config_Dialog_Data *cfdata;
   E_Container *con;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata)
     {
	free(v);
	return;
     }
   cfdata->desktop = desktop;
   if (exec) cfdata->exec = strdup(exec);
   cfdata->error = error;
   cfdata->read = read;
   cfdata->event = *event;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.create_widgets = _advanced_create_widgets;

   con = e_container_current_get(e_manager_current_get());
   /* Create The Dialog */
   cfd = e_config_dialog_new(con, _("Application Execution Error"), 
			     "E", "_e_exec_error_exit_dialog",
			     NULL, 0, v, cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   char buf[4096];

   if (!cfdata->label)
     {
	snprintf(buf, sizeof(buf), _("%s stopped running unexpectedly."), cfdata->desktop->name);
	cfdata->label = strdup(buf);
     }
   if ((cfdata->event.exited) && (!cfdata->exit))
     {
	snprintf(buf, sizeof(buf), 
		 _("An exit code of %i was returned from %s."), cfdata->event.exit_code, cfdata->exec);
	cfdata->exit = strdup(buf);
     }
   if ((cfdata->event.signalled) && (!cfdata->signal))
     {
	if (cfdata->event.exit_signal == SIGINT)
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by an Interrupt Signal."), cfdata->desktop->exec);
	else if (cfdata->event.exit_signal == SIGQUIT)
	  snprintf(buf, sizeof(buf), _("%s was interrupted by a Quit Signal."),
		   cfdata->exec);
	else if (cfdata->event.exit_signal == SIGABRT)
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by an Abort Signal."), cfdata->exec);
	else if (cfdata->event.exit_signal == SIGFPE)
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by a Floating Point Error."), cfdata->exec);
	else if (cfdata->event.exit_signal == SIGKILL)
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by an Uninterruptable Kill Signal."), cfdata->exec);
	else if (cfdata->event.exit_signal == SIGSEGV)
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by a Segmentation Fault."), cfdata->exec);
	else if (cfdata->event.exit_signal == SIGPIPE)
	  snprintf(buf, sizeof(buf), 
		   _("%s was interrupted by a Broken Pipe."), cfdata->exec);
	else if (cfdata->event.exit_signal == SIGTERM)
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by a Termination Signal."), cfdata->exec);
	else if (cfdata->event.exit_signal == SIGBUS)
	  snprintf(buf, sizeof(buf), 
		   _("%s was interrupted by a Bus Error."), cfdata->exec);
	else
	  snprintf(buf, sizeof(buf),
		   _("%s was interrupted by the signal number %i."),
		   cfdata->exec, cfdata->event.exit_signal);
	cfdata->signal = strdup(buf);
	/* FIXME: Add  sigchld_info stuff
	 * cfdata->event.data
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
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->error) ecore_exe_event_data_free(cfdata->error);
   if (cfdata->read) ecore_exe_event_data_free(cfdata->read);

   E_FREE(cfdata->exec);
   E_FREE(cfdata->signal);
   E_FREE(cfdata->exit);
   E_FREE(cfdata->label);

   free(cfdata);
}

static Evas_Object *
_dialog_scrolltext_create(Evas *evas, char *title, Ecore_Exe_Event_Data_Line *lines)
{
   int i;
   Evas_Object *obj, *os;
   char *text;
   char *trunc_note = _("***The remaining output has been truncated. Save the output to view.***\n");
   int tlen, max_lines;

   os = e_widget_framelist_add(evas, _(title), 0);

   obj = e_widget_textblock_add(evas);

   tlen = 0;
   for (i = 0; lines[i].line != NULL; i++)
     {
	tlen += lines[i].size + 1;
	/* When the program output is extraordinarily long, it can cause
	 * significant delays during text rendering. Limit to a fixed
	 * number of characters. */
	if (tlen > MAX_OUTPUT_CHARACTERS)
	  {
	     tlen -= lines[i].size + 1;
	     tlen += strlen(trunc_note);
	     break;
	  }
     }
   max_lines = i;
   text = alloca(tlen + 1);

   if (text)
     {
	text[0] = 0;
	for (i = 0; i < max_lines; i++)
	  {
	     strcat(text, lines[i].line);
	     strcat(text, "\n");
	  }

	/* Append the warning about truncated output. */
	if (lines[max_lines].line != NULL)
	  strcat(text, trunc_note);

	e_widget_textblock_plain_set(obj, text);
     }
   e_widget_min_size_set(obj, 240, 120);

   e_widget_framelist_object_append(os, obj);

   return os;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   char buf[4096];
   int error_length = 0;
   Evas_Object *o, *ob, *os;

   _fill_data(cfdata);

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_label_add(evas, cfdata->label);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   if (cfdata->error)
     error_length = cfdata->error->size;
   if (error_length)
     {
	os = _dialog_scrolltext_create(evas, _("Error Logs"), cfdata->error->lines);
	e_widget_list_object_append(o, os, 1, 1, 0.5);
     }
   else
     {
	ob = e_widget_label_add(evas, _("There was no error message."));
	e_widget_list_object_append(o, ob, 1, 1, 0.5);
     }

   ob = e_widget_button_add(evas, _("Save This Message"), "enlightenment/run", 
			    _dialog_save_cb, NULL, cfdata);
   e_widget_list_object_append(o, ob, 0, 0, 0.5);
   
   snprintf(buf, sizeof(buf), _("This error log will be saved as %s/%s.log"), 
	    e_user_homedir_get(), cfdata->desktop->name);
   ob = e_widget_label_add(evas, buf);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   char buf[4096];
   int read_length = 0;
   int error_length = 0;
   Evas_Object *o, *of, *ob, *ot;

   _fill_data(cfdata);

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

   if (cfdata->read)
     read_length = cfdata->read->size;

   if (read_length)
     {
	of = _dialog_scrolltext_create(evas, _("Output Data"), cfdata->read->lines);
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

   if (cfdata->error)
     error_length = cfdata->error->size;
   if (error_length)
     {
	of = _dialog_scrolltext_create(evas, _("Error Logs"), cfdata->error->lines);
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

   ob = e_widget_button_add(evas, _("Save This Message"), "enlightenment/run", _dialog_save_cb, NULL, cfdata);
   e_widget_list_object_append(o, ob, 0, 0, 0.5);

   snprintf(buf, sizeof(buf), _("This error log will be saved as %s/%s.log"), 
	    e_user_homedir_get(), cfdata->desktop->name);
   ob = e_widget_label_add(evas, buf);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   return o;
}

static void
_dialog_save_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   FILE *f;
   char *text;
   char buf[1024];
   char buffer[4096];
   int read_length = 0;
   int i, tlen;

   cfdata = data2;

   snprintf(buf, sizeof(buf), "%s/%s.log", e_user_homedir_get(),
	    e_util_filename_escape(cfdata->desktop->name));
   f = fopen(buf, "w");
   if (!f) return;

   if (cfdata->exit)
     {
	snprintf(buffer, sizeof(buffer), "Error Information:\n\t%s\n\n", cfdata->exit);
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }
   if (cfdata->signal)
     {
	snprintf(buffer, sizeof(buffer), "Error Signal Information:\n\t%s\n\n", cfdata->signal);
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }

   if (cfdata->read)
     read_length = cfdata->read->size;

   if (read_length)
     {
	tlen = 0;
	for (i = 0; cfdata->read->lines[i].line != NULL; i++)
	  tlen += cfdata->read->lines[i].size + 2;
	text = alloca(tlen + 1);
	if (text)
	  {
	     text[0] = 0;
	     for (i = 0; cfdata->read->lines[i].line != NULL; i++)
	       {
		  strcat(text, "\t");
		  strcat(text, cfdata->read->lines[i].line);
		  strcat(text, "\n");
	       }
	     snprintf(buffer, sizeof(buffer), "Output Data:\n%s\n\n", text);
	     fwrite(buffer, sizeof(char), strlen(buffer), f);
	  }
     }
   else
     {
	snprintf(buffer, sizeof(buffer), "Output Data:\n\tThere was no output\n\n");
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }

   /* Reusing this var */
   read_length = 0;
   if (cfdata->error)
     read_length = cfdata->error->size;

   if (read_length)
     {
	tlen = 0;
	for (i = 0; cfdata->error->lines[i].line != NULL; i++)
	  tlen += cfdata->error->lines[i].size + 1;
	text = alloca(tlen + 1);
	if (text)
	  {
	     text[0] = 0;
	     for (i = 0; cfdata->error->lines[i].line != NULL; i++)
	       {
		  strcat(text, "\t");
		  strcat(text, cfdata->error->lines[i].line);
		  strcat(text, "\n");
	       }
	     snprintf(buffer, sizeof(buffer), "Error Logs:\n%s\n", text);
	     fwrite(buffer, sizeof(char), strlen(buffer), f);
	  }
     }
   else
     {
	snprintf(buffer, sizeof(buffer), "Error Logs:\n\tThere was no error message\n");
	fwrite(buffer, sizeof(char), strlen(buffer), f);
     }

   fclose(f);
}
