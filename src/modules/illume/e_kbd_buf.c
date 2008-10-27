#include <e.h>
#include "e_kbd_buf.h"
#include "e_kbd_dict.h"
#include "e_cfg.h"

#include <math.h>

static E_Kbd_Buf_Layout *
_e_kbd_buf_new(void)
{
   E_Kbd_Buf_Layout *kbl;
   
   kbl = E_NEW(E_Kbd_Buf_Layout, 1);
   kbl->ref =1;
   return kbl;
}

static void
_e_kbd_buf_layout_ref(E_Kbd_Buf_Layout *kbl)
{
   kbl->ref++;
}

static void
_e_kbd_buf_layout_unref(E_Kbd_Buf_Layout *kbl)
{
   kbl->ref--;
   if (kbl->ref > 0) return;
   while (kbl->keys)
     {
	E_Kbd_Buf_Key *ky;
	
	ky = kbl->keys->data;
	if (ky->key) evas_stringshare_del(ky->key);
	if (ky->key_shift) evas_stringshare_del(ky->key_shift);
	if (ky->key_capslock) evas_stringshare_del(ky->key_capslock);
	free(ky);
	kbl->keys = eina_list_remove_list(kbl->keys, kbl->keys);
     }
   free(kbl);
}

static void
_e_kbd_buf_string_matches_clear(E_Kbd_Buf *kb)
{
   while (kb->string_matches)
     {
	if (kb->string_matches->data)
	  evas_stringshare_del(kb->string_matches->data);
	kb->string_matches = eina_list_remove_list(kb->string_matches, kb->string_matches);
     }
}

static void
_e_kbd_buf_actual_string_clear(E_Kbd_Buf *kb)
{
   if (kb->actual_string) evas_stringshare_del(kb->actual_string);
   kb->actual_string = NULL;
}

static E_Kbd_Buf_Key *
_e_kbd_buf_at_coord_get(E_Kbd_Buf *kb, E_Kbd_Buf_Layout *kbl, int x, int y)
{
   Eina_List *l;
   
   for (l = kbl->keys; l; l = l->next)
     {
	E_Kbd_Buf_Key *ky;
	
	ky = l->data;
	if (ky->key)
	  {
	     if ((x >= ky->x) && (y >= ky->y) &&
		 (x < (ky->x + ky->w)) && (y < (ky->y + ky->h)))
	       return ky;
	  }
     }
   return NULL;
}

static E_Kbd_Buf_Key *
_e_kbd_buf_closest_get(E_Kbd_Buf *kb, E_Kbd_Buf_Layout *kbl, int x, int y)
{
   Eina_List *l;
   E_Kbd_Buf_Key *ky_closest = NULL;
   int dist_closest = 0x7fffffff;
   
   for (l = kbl->keys; l; l = l->next)
     {
	E_Kbd_Buf_Key *ky;
	int dist, dx, dy;
	
	ky = l->data;
	if (ky->key)
	  {
	     dx = x - (ky->x + (ky->w / 2));
	     dy = y - (ky->y + (ky->h / 2));
	     dist = (dx * dx) + (dy * dy);
	     if (dist < dist_closest)
	       {
		  ky_closest = ky;
		  dist_closest = dist;
	       }
	  }
     }
   return ky_closest;
}

static const char *
_e_kbd_buf_keystroke_key_string_get(E_Kbd_Buf *kb, E_Kbd_Buf_Keystroke *ks, E_Kbd_Buf_Key *ky)
{
   const char *str = NULL;
   
   if ((ky) && (ky->key))
     {
	if (ks->shift)
	  {
	     if (ky->key_shift) str = ky->key_shift;
	     else str = ky->key;
	  }
	else if (ks->capslock)
	  {
	     if (ky->key_capslock) str = ky->key_capslock;
	     else str = ky->key;
	  }
	else str = ky->key;
     }
   return str;
}

static const char *
_e_kbd_buf_keystroke_string_get(E_Kbd_Buf *kb, E_Kbd_Buf_Keystroke *ks)
{
   const char *str = NULL;
   
   if (ks->key) str = ks->key;
   else
     {
	E_Kbd_Buf_Key *ky;
	
	ky = _e_kbd_buf_at_coord_get(kb, ks->layout, ks->x, ks->y);
	if (!ky) ky = _e_kbd_buf_closest_get(kb, ks->layout, ks->x, ks->y);
	str = _e_kbd_buf_keystroke_key_string_get(kb, ks, ky);
     }
   return str;
}

static void
_e_kbd_buf_actual_string_update(E_Kbd_Buf *kb)
{
   Eina_List *l;
   char *actual = NULL;
   int actual_len = 0;
   int actual_size = 0;
   
   _e_kbd_buf_actual_string_clear(kb);
   for (l = kb->keystrokes; l; l = l->next)
     {
	E_Kbd_Buf_Keystroke *ks;
	const char *str;
	
	ks = l->data;
	str = _e_kbd_buf_keystroke_string_get(kb, ks);
	if (str)
	  {
	     if ((actual_len + strlen(str) + 1) > actual_size)
	       {
		  actual_size += 64;
		  actual = realloc(actual, actual_size);
	       }
	     strcpy(actual + actual_len, str);
	     actual_len += strlen(str);
	  }
     }
   if (actual)
     {
	kb->actual_string = evas_stringshare_add(actual);
	if (actual) free(actual);
     }
}

static const char *
_e_kbd_buf_matches_find(Eina_List *matches, const char *s)
{
   Eina_List *l;
   
   for (l = matches; l; l = l->next)
     {
	if (!strcmp(l->data, s)) return s;
     }
   return NULL;
}

static void
_e_kbd_buf_matches_update(E_Kbd_Buf *kb)
{
   Eina_List *matches = NULL;
   const char *word;
   int pri, i;
   E_Kbd_Dict *dicts[3];

   _e_kbd_buf_string_matches_clear(kb);
   dicts[0] = kb->dict.personal;
   dicts[1] = kb->dict.sys;
   dicts[2] = kb->dict.data;
   for (i = 0; i < 3; i++)
     {
	if (!dicts[i]) continue;
	e_kbd_dict_matches_lookup(dicts[i]);
	e_kbd_dict_matches_first(dicts[i]);
	for (;;)
	  {
	     word = e_kbd_dict_matches_match_get(dicts[i], &pri);
	     if (!word) break;
	     if (!_e_kbd_buf_matches_find(kb->string_matches, word))
	       kb->string_matches = eina_list_append(kb->string_matches,
						     evas_stringshare_add(word));
	     e_kbd_dict_matches_next(dicts[i]);
	  }
     }
}

static int
_e_kbd_buf_cb_data_dict_reload(void *data)
{
   E_Kbd_Buf *kb;
   char buf[PATH_MAX];
   const char *homedir;
   
   kb = data;
   kb->dict.data_reload_delay = NULL;
   e_kbd_buf_clear(kb);
   if (kb->dict.data) e_kbd_dict_free(kb->dict.data);
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts-dynamic/data.dic", homedir);
   kb->dict.data = e_kbd_dict_new(buf);
   return 0;
}

static void
_e_kbd_buf_cb_data_dict_change(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
   E_Kbd_Buf *kb;
   
   kb = data;
   if (kb->dict.data_reload_delay) ecore_timer_del(kb->dict.data_reload_delay);
   kb->dict.data_reload_delay = ecore_timer_add(2.0, _e_kbd_buf_cb_data_dict_reload, kb);
}
    
EAPI E_Kbd_Buf *
e_kbd_buf_new(const char *sysdicts, const char *dict)
{
   E_Kbd_Buf *kb;
   char buf[PATH_MAX];
   const char *homedir;
   
   kb = E_NEW(E_Kbd_Buf, 1);
   if (!kb) return NULL;
   kb->sysdicts = evas_stringshare_add(sysdicts);
   
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts", homedir);
   if (!ecore_file_exists(buf)) ecore_file_mkpath(buf);
   
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts/%s", homedir, dict);
   kb->dict.sys = e_kbd_dict_new(buf);
   if (!kb->dict.sys)
     {
	snprintf(buf, sizeof(buf), "%s/dicts/%s", kb->sysdicts, dict);
	kb->dict.sys = e_kbd_dict_new(buf);
     }
   
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts-dynamic", homedir);
   if (!ecore_file_exists(buf)) ecore_file_mkpath(buf);
   
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts-dynamic/personal.dic", homedir);
   kb->dict.personal = e_kbd_dict_new(buf);
   if (!kb->dict.personal)
     {
	FILE *f;
	
	f = fopen(buf, "w");
	if (f)
	  {
	     fprintf(f, "\n");
	     fclose(f);
	  }
	kb->dict.personal = e_kbd_dict_new(buf);
     }
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts-dynamic/data.dic", homedir);
   kb->dict.data = e_kbd_dict_new(buf);
   kb->dict.data_monitor = 
     ecore_file_monitor_add(buf, _e_kbd_buf_cb_data_dict_change, kb);
   return kb;
}

EAPI void
e_kbd_buf_free(E_Kbd_Buf *kb)
{
   e_kbd_buf_clear(kb);
   e_kbd_buf_layout_clear(kb);
   e_kbd_buf_lookup_cancel(kb);
   evas_stringshare_del(kb->sysdicts);
   if (kb->dict.sys) e_kbd_dict_free(kb->dict.sys);
   if (kb->dict.personal) e_kbd_dict_free(kb->dict.personal);
   if (kb->dict.data) e_kbd_dict_free(kb->dict.data);
   if (kb->dict.data_monitor) ecore_file_monitor_del(kb->dict.data_monitor);
   if (kb->dict.data_reload_delay) ecore_timer_del(kb->dict.data_reload_delay);
   free(kb);
}

EAPI void
e_kbd_buf_dict_set(E_Kbd_Buf *kb, const char *dict)
{
   char buf[PATH_MAX];
   const char *homedir;
   
   e_kbd_buf_clear(kb);
   
   if (kb->dict.sys) e_kbd_dict_free(kb->dict.sys);
   
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts", homedir);
   if (!ecore_file_exists(buf)) ecore_file_mkpath(buf);
   
   snprintf(buf, sizeof(buf), "%s/.e/e/dicts/%s", homedir, dict);
   kb->dict.sys = e_kbd_dict_new(buf);
   if (!kb->dict.sys)
     {
	snprintf(buf, sizeof(buf), "%s/dicts/%s", kb->sysdicts, dict);
	kb->dict.sys = e_kbd_dict_new(buf);
     }
}

EAPI void
e_kbd_buf_clear(E_Kbd_Buf *kb)
{
   e_kbd_buf_lookup_cancel(kb);
   while (kb->keystrokes)
     {
	E_Kbd_Buf_Keystroke *ks;
	
	ks = kb->keystrokes->data;
	if (ks->key) evas_stringshare_del(ks->key);
	_e_kbd_buf_layout_unref(ks->layout);
	free(ks);
	kb->keystrokes = eina_list_remove_list(kb->keystrokes, kb->keystrokes);
     }
   _e_kbd_buf_string_matches_clear(kb);
   if (kb->dict.sys) e_kbd_dict_word_letter_clear(kb->dict.sys);
   if (kb->dict.personal) e_kbd_dict_word_letter_clear(kb->dict.personal);
   if (kb->dict.data) e_kbd_dict_word_letter_clear(kb->dict.data);
   _e_kbd_buf_actual_string_clear(kb);
}

EAPI void
e_kbd_buf_layout_clear(E_Kbd_Buf *kb)
{
   if (kb->layout)
     {
	_e_kbd_buf_layout_unref(kb->layout);
	kb->layout = NULL;
     }
}

EAPI void
e_kbd_buf_layout_size_set(E_Kbd_Buf *kb, int w, int h)
{
   if (!kb->layout) kb->layout = _e_kbd_buf_new();
   if (!kb->layout) return;
   kb->layout->w = w;
   kb->layout->h = h;
}

EAPI void
e_kbd_buf_layout_fuzz_set(E_Kbd_Buf *kb, int fuzz)
{
   if (!kb->layout) kb->layout = _e_kbd_buf_new();
   if (!kb->layout) return;
   kb->layout->fuzz = fuzz;
}

EAPI void
e_kbd_buf_layout_key_add(E_Kbd_Buf *kb, const char *key,  const char *key_shift, const char *key_capslock, int x, int y, int w, int h)
{
   E_Kbd_Buf_Key *ky;
   
   if (!key) return;
   if (!kb->layout) kb->layout = _e_kbd_buf_new();
   if (!kb->layout) return;
   ky = E_NEW(E_Kbd_Buf_Key, 1);
   if (!ky) return;
   if (key) ky->key = evas_stringshare_add(key);
   if (key_shift) ky->key_shift = evas_stringshare_add(key_shift);
   if (key_capslock) ky->key_capslock = evas_stringshare_add(key_capslock);
   ky->x = x;
   ky->y = y;
   ky->w = w;
   ky->h = h;
   kb->layout->keys = eina_list_append(kb->layout->keys, ky);
}

static void
_e_kbd_buf_keystroke_add(E_Kbd_Buf *kb, E_Kbd_Buf_Keystroke *ks)
{
   const char *str;

   str = _e_kbd_buf_keystroke_string_get(kb, ks);
   if (str)
     {
	if (kb->dict.sys) e_kbd_dict_word_letter_add(kb->dict.sys, str, 0);
	if (kb->dict.personal) e_kbd_dict_word_letter_add(kb->dict.personal, str, 0);
	if (kb->dict.data) e_kbd_dict_word_letter_add(kb->dict.data, str, 0);
     }
}

EAPI void
e_kbd_buf_pressed_key_add(E_Kbd_Buf *kb, const char *key, int shift, int capslock)
{
   E_Kbd_Buf_Keystroke *ks;

   e_kbd_buf_lookup_cancel(kb);
   if (!key) return;
   if (!kb->layout) kb->layout = _e_kbd_buf_new();
   if (!kb->layout) return;
   ks = E_NEW(E_Kbd_Buf_Keystroke, 1);
   if (!ks) return;
   ks->key = evas_stringshare_add(key);
   if (shift) ks->shift = 1;
   if (capslock) ks->capslock = 1;
   ks->layout = kb->layout;
   _e_kbd_buf_layout_ref(ks->layout);
   kb->keystrokes = eina_list_append(kb->keystrokes, ks);
   
   if (kb->dict.sys) e_kbd_dict_word_letter_advance(kb->dict.sys);
   if (kb->dict.personal) e_kbd_dict_word_letter_advance(kb->dict.personal);
   if (kb->dict.data) e_kbd_dict_word_letter_advance(kb->dict.data);
   _e_kbd_buf_keystroke_add(kb, ks);
   
   _e_kbd_buf_actual_string_update(kb);
   _e_kbd_buf_matches_update(kb);
}

static void
_e_kbd_buf_keystroke_point_add(E_Kbd_Buf *kb, E_Kbd_Buf_Keystroke *ks)
{
   Eina_List *l;
   
   for (l = ks->layout->keys; l; l = l->next)
     {
	E_Kbd_Buf_Key *ky;
	const char *str;
	int px, py, dx, dy, d, fuzz;
	
	ky = l->data;
	px = ky->x + (ky->w / 2);
	py = ky->y + (ky->h / 2);
	dx = ks->x - px;
	dy = ks->y - py;
	d = sqrt((dx * dx) + (dy * dy));
	if (d <= ks->layout->fuzz)
	  {
	     str = _e_kbd_buf_keystroke_key_string_get(kb, ks, ky);
	     if (str)
	       {
		  if (kb->dict.sys) e_kbd_dict_word_letter_add(kb->dict.sys, str, d);
		  if (kb->dict.personal) e_kbd_dict_word_letter_add(kb->dict.personal, str, d);
		  if (kb->dict.data) e_kbd_dict_word_letter_add(kb->dict.data, str, d);
	       }
	  }
     }
}

EAPI void
e_kbd_buf_pressed_point_add(E_Kbd_Buf *kb, int x, int y, int shift, int capslock)
{
   E_Kbd_Buf_Keystroke *ks;
   
   e_kbd_buf_lookup_cancel(kb);
   if (!kb->layout) kb->layout = _e_kbd_buf_new();
   if (!kb->layout) return;
   ks = E_NEW(E_Kbd_Buf_Keystroke, 1);
   if (!ks) return;
   ks->x = x;
   ks->y = y;
   if (shift) ks->shift = 1;
   if (capslock) ks->capslock = 1;
   ks->layout = kb->layout;
   _e_kbd_buf_layout_ref(ks->layout);
   kb->keystrokes = eina_list_append(kb->keystrokes, ks);
   
   if (kb->dict.sys) e_kbd_dict_word_letter_advance(kb->dict.sys);
   if (kb->dict.personal) e_kbd_dict_word_letter_advance(kb->dict.personal);
   if (kb->dict.data) e_kbd_dict_word_letter_advance(kb->dict.data);
   
   _e_kbd_buf_keystroke_point_add(kb, ks);
   
   _e_kbd_buf_actual_string_update(kb);
   _e_kbd_buf_matches_update(kb);
}

EAPI const char *
e_kbd_buf_actual_string_get(E_Kbd_Buf *kb)
{
   return kb->actual_string;
}

EAPI const Eina_List *
e_kbd_buf_string_matches_get(E_Kbd_Buf *kb)
{
   return kb->string_matches;
}

EAPI void
e_kbd_buf_backspace(E_Kbd_Buf *kb)
{
   Eina_List *l;
   
   l = eina_list_last(kb->keystrokes);
   if (l)
     {
	E_Kbd_Buf_Keystroke *ks;
	
	ks = l->data;
	if (ks->key) evas_stringshare_del(ks->key);
	_e_kbd_buf_layout_unref(ks->layout);
	free(ks);
	kb->keystrokes = eina_list_remove_list(kb->keystrokes, l);
	if (kb->dict.sys) e_kbd_dict_word_letter_delete(kb->dict.sys);
	if (kb->dict.personal) e_kbd_dict_word_letter_delete(kb->dict.personal);
	if (kb->dict.data) e_kbd_dict_word_letter_delete(kb->dict.data);
	_e_kbd_buf_actual_string_update(kb);
	_e_kbd_buf_matches_update(kb);
     }
}

EAPI void
e_kbd_buf_word_use(E_Kbd_Buf *kb, const char *word)
{
   if (kb->dict.personal)
     e_kbd_dict_word_usage_adjust(kb->dict.personal, word, 1);
}

// FIXME: just faking delayed lookup with timer
static int
_e_kbd_buf_cb_faket(void *data)
{
   E_Kbd_Buf *kb;
   
   kb = data;
   kb->lookup.faket = NULL;
   kb->lookup.func(kb->lookup.data);
   kb->lookup.func = NULL;
   kb->lookup.data = NULL;
   return 0;
}

EAPI void
e_kbd_buf_lookup(E_Kbd_Buf *kb, void (*func) (void *data), const void *data)
{
   e_kbd_buf_lookup_cancel(kb);
   
   kb->lookup.func = func;
   kb->lookup.data = data;

   // FIXME: just faking delayed lookup with timer
   kb->lookup.faket = ecore_timer_add(0.1, _e_kbd_buf_cb_faket, kb);
}

EAPI void
e_kbd_buf_lookup_cancel(E_Kbd_Buf *kb)
{
   // FIXME: just faking delayed lookup with timer
   if (!kb->lookup.faket) return;
   ecore_timer_del(kb->lookup.faket);
   kb->lookup.faket = NULL;
   
   kb->lookup.func = NULL;
   kb->lookup.data = NULL;
}
