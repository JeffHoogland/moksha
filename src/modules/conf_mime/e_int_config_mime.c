/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_int_config_mime_edit.h"

typedef struct _Config_Glob Config_Glob;
typedef struct _Config_Mime Config_Mime;
typedef struct _Config_Type Config_Type;
struct _Config_Glob 
{
   const char *name;
};
struct _Config_Mime 
{
   const char *mime;
   Evas_List *globs;
};
struct _Config_Type 
{
   const char *name;
   const char *type;
};
struct _E_Config_Dialog_Data 
{
   Evas_List *mimes;
   char *cur_type;
   struct 
     {
	Evas_Object *tlist, *list;
     } gui;
   E_Config_Dialog *cfd, *edit_dlg;
};

static void        *_create_data     (E_Config_Dialog *cfd);
static void         _free_data       (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _fill_list       (E_Config_Dialog_Data *cfdata, const char *mtype);
static void         _fill_tlist      (E_Config_Dialog_Data *cfdata);
static void         _load_mimes      (E_Config_Dialog_Data *cfdata, char *file);
static void         _load_globs      (E_Config_Dialog_Data *cfdata, char *file);
static void         _fill_types      (E_Config_Dialog_Data *cfdata);
static void         _tlist_cb_change (void *data);
static int          _sort_mimes      (void *data1, void *data2);
static Config_Mime *_find_mime       (E_Config_Dialog_Data *cfdata, char *mime);
static Config_Glob *_find_glob       (Config_Mime *mime, char *glob);
static void         _cb_config       (void *data, void *data2);

Evas_List *types = NULL;

EAPI E_Config_Dialog *
e_int_config_mime(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_mime_dialog")) return NULL;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   
   cfd = e_config_dialog_new(con, _("File Icons"), "E", "_config_mime_dialog",
			     "enlightenment/file_icons", 0, v, NULL);
   return cfd;
}

EAPI void
e_int_config_mime_edit_done(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   if (cfdata->edit_dlg)
     cfdata->edit_dlg = NULL;
   _tlist_cb_change(cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   const char *homedir;
   char buf[4096];

   if (!cfdata) return;
   homedir = e_user_homedir_get();

   snprintf(buf, sizeof(buf), "/usr/local/etc/mime.types");
   if (ecore_file_exists(buf))
     _load_mimes(cfdata, buf);

   snprintf(buf, sizeof(buf), "/etc/mime.types");
   if (ecore_file_exists(buf))
     _load_mimes(cfdata, buf);

   snprintf(buf, sizeof(buf), "/usr/local/share/mime/globs");
   if (ecore_file_exists(buf))
     _load_globs(cfdata, buf);

   snprintf(buf, sizeof(buf), "/usr/share/mime/globs");
   if (ecore_file_exists(buf))
     _load_globs(cfdata, buf);

   snprintf(buf, sizeof(buf), "%s/.mime.types", homedir);
   if (ecore_file_exists(buf))
     _load_mimes(cfdata, buf);

   snprintf(buf, sizeof(buf), "%s/.local/share/mime/globs", homedir);
   if (ecore_file_exists(buf))
     _load_globs(cfdata, buf);

   if (cfdata->mimes)
     cfdata->mimes = evas_list_sort(cfdata->mimes, 
				    evas_list_count(cfdata->mimes), _sort_mimes);
   
   _fill_types(cfdata);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->edit_dlg) 
     {
	e_object_del(E_OBJECT(cfdata->edit_dlg));
	cfdata->edit_dlg = NULL;
     }
   
   while (types) 
     {
	Config_Type *t;
	
	t = types->data;
	if (!t) continue;
	if (t->name)
	  evas_stringshare_del(t->name);
	if (t->type)
	  evas_stringshare_del(t->type);
	types = evas_list_remove_list(types, types);
	E_FREE(t);
     }

   while (cfdata->mimes) 
     {
	Config_Mime *m;
	
	m = cfdata->mimes->data;
	if (!m) continue;
	while (m->globs) 
	  {
	     Config_Glob *g;
	     
	     g = m->globs->data;
	     if (!g) continue;
	     if (g->name)
	       evas_stringshare_del(g->name);
	     m->globs = evas_list_remove_list(m->globs, m->globs);
	     E_FREE(g);
	  }
	if (m->mime)
	  evas_stringshare_del(m->mime);
	
	cfdata->mimes = evas_list_remove_list(cfdata->mimes, cfdata->mimes);
	E_FREE(m);
     }
   
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ol;
   Evas_Object *ob;
   
   o = e_widget_list_add(evas, 0, 1);
   of  = e_widget_framelist_add(evas, _("Categories"), 0);
   ol = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_type));
   cfdata->gui.tlist = ol;
   _fill_tlist(cfdata);
   e_widget_framelist_object_append(of, ol);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("File Types"), 0);
   ol = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.list = ol;
   e_widget_ilist_go(ol);
   e_widget_min_size_set(cfdata->gui.list, 250, 200);
   e_widget_frametable_object_append(of, ol, 0, 0, 3, 1, 1, 1, 1, 1);

   ob = e_widget_button_add(evas, _("Configure"), "widget/config", _cb_config, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static void 
_fill_list(E_Config_Dialog_Data *cfdata, const char *mtype) 
{
   Evas_List *l;
   Evas_Coord w, h;
   Evas *evas;

   evas = evas_object_evas_get(cfdata->gui.list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.list);

   e_widget_ilist_clear(cfdata->gui.list);
   for (l = cfdata->mimes; l; l = l->next) 
     {
	Config_Mime *m;
	Evas_Object *icon = NULL;
	const char *tmp;
	char buf[4096];
	int edj = 0, img = 0;
	
	m = l->data;
	if (!m) return;
	if (!strstr(m->mime, mtype)) continue;
	tmp = e_fm_mime_icon_get(m->mime);
	if (!tmp) 
	  snprintf(buf, sizeof(buf), "e/icons/fileman/file");
	else if (!strcmp(tmp, "THUMB"))
	  snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", m->mime);
	else if (!strncmp(tmp, "e/icons/fileman/mime/", 21))
	  snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", m->mime);
	else 
	  {
	     char *p;
	     
	     p = strrchr(tmp, '.');
	     if ((p) && (!strcmp(p, ".edj")))
	       edj = 1;
	     else if (p)
	       img = 1;
	  }
	if (edj) 
	  {
	     icon = edje_object_add(evas);
	     if (!e_theme_edje_object_set(icon, tmp, "icon"))
	       e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
	  }
	else if (img) 
	  icon = e_widget_image_add_from_file(evas, tmp, 16, 16);
	else 
	  {
	     icon = edje_object_add(evas);
	     if (!e_theme_edje_object_set(icon, "base/theme/fileman", buf))
	       e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
	  }
	e_widget_ilist_append(cfdata->gui.list, icon, m->mime, NULL, NULL, NULL);
     }
   e_widget_ilist_go(cfdata->gui.list);
   e_widget_min_size_get(cfdata->gui.list, &w, &h);
   e_widget_min_size_set(cfdata->gui.list, w, 200);
   e_widget_ilist_thaw(cfdata->gui.list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_fill_tlist(E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l;
   Evas_Coord w, h;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.tlist));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.tlist);
   e_widget_ilist_clear(cfdata->gui.tlist);
   for (l = types; l; l = l->next) 
     {
	Config_Type *tmp;
	Evas_Object *icon;
	char buf[4096];
	char *t;
	
	tmp = l->data;
	if (!tmp) continue;
	t = strdup(tmp->name);
	t[0] = tolower(t[0]);
	icon = edje_object_add(evas_object_evas_get(cfdata->gui.tlist));
	snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", t);
	if (!e_theme_edje_object_set(icon, "base/theme/fileman", buf))
	  e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
	e_widget_ilist_append(cfdata->gui.tlist, icon, tmp->name, _tlist_cb_change, cfdata, tmp->type);
     }
   
   e_widget_ilist_go(cfdata->gui.tlist);
   e_widget_min_size_get(cfdata->gui.tlist, &w, &h);
   e_widget_min_size_set(cfdata->gui.tlist, w, 225);
   e_widget_ilist_thaw(cfdata->gui.tlist);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.tlist));
}

static void 
_load_mimes(E_Config_Dialog_Data *cfdata, char *file) 
{
   FILE *f;
   char buf[4096], mimetype[4096], ext[4096];
   char *p, *pp;
   Config_Mime *mime;
   Config_Glob *glob;

   if (!cfdata) return;
   
   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
	p = buf;
	while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	if (*p == '#') continue;
	if ((*p == '\n') || (*p == 0)) continue;
	pp = p;
	while (!isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	if ((*p == '\n') || (*p == 0)) continue;
	strncpy(mimetype, pp, (p - pp));
	mimetype[p - pp] = 0;
	do 
	  {
	     while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	     if ((*p == '\n') || (*p == 0)) continue;
	     pp = p;
	     while (!isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	     strncpy(ext, pp, (p - pp));
	     ext[p - pp] = 0;
	     mime = _find_mime(cfdata, mimetype);
	     if (!mime) 
	       {
		  mime = E_NEW(Config_Mime, 1);
		  if (mime)
		    {
		       mime->mime = evas_stringshare_add(mimetype);
		       if (!mime->mime) 
			 free(mime);
		       else 
			 {
			    glob = E_NEW(Config_Glob, 1);
			    glob->name = evas_stringshare_add(ext);
			    mime->globs = evas_list_append(mime->globs, glob);
			    cfdata->mimes = evas_list_append(cfdata->mimes, mime);
			 }
		    }
	       }
	  }
	while ((*p != '\n') && (*p != 0));
     }
   fclose(f);
}

static void 
_load_globs(E_Config_Dialog_Data *cfdata, char *file) 
{
   FILE *f;
   char buf[4096], mimetype[4096], ext[4096];
   char *p, *pp;
   Config_Mime *mime;
   Config_Glob *glob;

   if (!cfdata) return;
   
   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
	p = buf;
	while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
	if (*p == '#') continue;
	if ((*p == '\n') || (*p == 0)) continue;
	pp = p;
	while ((*p != ':') && (*p != 0) && (*p != '\n')) p++;
	if ((*p == '\n') || (*p == 0)) continue;
	strncpy(mimetype, pp, (p - pp));
	mimetype[p - pp] = 0;
	p++;
	pp = ext;
	while ((*p != 0) && (*p != '\n')) 
	  {
	     *pp = *p;
	     pp++;
	     p++;
	  }
	*pp = 0;
	mime = _find_mime(cfdata, mimetype);
	if (!mime) 
	  {
	     mime = E_NEW(Config_Mime, 1);
	     if (mime)
	       {
		  mime->mime = evas_stringshare_add(mimetype);
		  if (!mime->mime) 
		    free(mime);
		  else 
		    {
		       glob = E_NEW(Config_Glob, 1);
		       glob->name = evas_stringshare_add(ext);
		       mime->globs = evas_list_append(mime->globs, glob);
		       cfdata->mimes = evas_list_append(cfdata->mimes, mime);
		    }
	       }
	  }
	else 
	  {
	     glob = _find_glob(mime, ext);
	     if (!glob) 
	       {
		  glob = E_NEW(Config_Glob, 1);
		  glob->name = evas_stringshare_add(ext);
		  mime->globs = evas_list_append(mime->globs, glob);
	       }
	  }
     }
   fclose(f);
}

static void
_fill_types(E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l, *ll;

   for (l = cfdata->mimes; l; l = l->next) 
     {
	Config_Type *tmp;	
	Config_Mime *m;
	char *tok;
	int found = 0;
	
	m = l->data;
	if (!m) continue;
	tok = strtok(strdup(m->mime), "/");
	if (!tok) continue;
	for (ll = types; ll; ll = ll->next) 
	  {
	     tmp = ll->data;
	     if (!tmp) continue;
	     
	     if (strcmp(tmp->type, tok) >= 0) 
	       {
		  found = 1;
		  break;
	       }
	  }
	if (!found) 
	  {
	     tmp = E_NEW(Config_Type, 1);
	     tmp->type = evas_stringshare_add(tok);
	     tok[0] = toupper(tok[0]);
	     tmp->name = evas_stringshare_add(tok);
	     
	     types = evas_list_append(types, tmp);
	  }
     }
}

static void 
_tlist_cb_change(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l;

   cfdata = data;
   if (!cfdata) return;
   for (l = types; l; l = l->next) 
     {
	Config_Type *t;
	
	t = l->data;
	if (!t) continue;
	if (strcasecmp(t->name, cfdata->cur_type)) continue;
	_fill_list(cfdata, t->type);
	break;
     }
}

static int 
_sort_mimes(void *data1, void *data2) 
{
   Config_Mime *m1, *m2;
   
   if (!data1) return 1;
   if (!data2) return -1;
   
   m1 = data1;
   m2 = data2;
   
   return (strcmp(m1->mime, m2->mime));
}

static Config_Mime *
_find_mime(E_Config_Dialog_Data *cfdata, char *mime) 
{
   Evas_List *l;
   
   if (!cfdata) return NULL;
   for (l = cfdata->mimes; l; l = l->next) 
     {
	Config_Mime *cm;
	
	cm = l->data;
	if (!cm) continue;
	if (strcmp(cm->mime, mime)) continue;
	return cm;
     }
   return NULL;
}

static Config_Glob *
_find_glob(Config_Mime *mime, char *glob) 
{
   Evas_List *l;
   
   if (!mime) return NULL;
   for (l = mime->globs; l; l = l->next) 
     {
	Config_Glob *g;
	
	g = l->data;
	if (!g) continue;
	if (strcmp(g->name, glob)) continue;
	return g;
     }
   return NULL;
}

static void 
_cb_config(void *data, void *data2) 
{
   Evas_List *l;
   E_Config_Dialog_Data *cfdata;
   E_Config_Mime_Icon *mi = NULL;
   const char *m;
   int found = 0;
   
   cfdata = data;
   if (!cfdata) return;
   m = e_widget_ilist_selected_label_get(cfdata->gui.list);
   if (!m) return;
   
   for (l = e_config->mime_icons; l; l = l->next) 
     {
	mi = l->data;
	if (!mi) continue;
	if (!mi->mime) continue;
	if (strcmp(mi->mime, m)) continue;
	found = 1;
	break;
     }
   if (!found) 
     {
	mi = E_NEW(E_Config_Mime_Icon, 1);
	mi->mime = evas_stringshare_add(m);
     }
   
   cfdata->edit_dlg = e_int_config_mime_edit(mi, cfdata);
}
