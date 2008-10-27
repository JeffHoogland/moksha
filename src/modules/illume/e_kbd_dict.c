#include <e.h>
#include "e_kbd_dict.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


#define MAXLATIN 0x100
static unsigned char _e_kbd_normalise_base[MAXLATIN];
static unsigned char _e_kbd_normalise_ready = 0;

static void
_e_kbd_normalise_init(void)
{
   int i;
   const char *table[][2] =
     {
	  {"À", "a"},
	  {"Á", "a"},
	  {"Â", "a"},
	  {"Ã", "a"},
	  {"Ä", "a"},
	  {"Å", "a"},
	  {"Æ", "a"},
	  {"Ç", "c"},
	  {"È", "e"},
	  {"É", "e"},
	  {"Ê", "e"},
	  {"Ë", "e"},
	  {"Ì", "i"},
	  {"Í", "i"},
	  {"Î", "i"},
	  {"Ï", "i"},
	  {"Ð", "d"},
	  {"Ñ", "n"},
	  {"Ò", "o"},
	  {"Ó", "o"},
	  {"Ô", "o"},
	  {"Õ", "o"},
	  {"Ö", "o"},
	  {"×", "x"},
	  {"Ø", "o"},
	  {"Ù", "u"},
	  {"Ú", "u"},
	  {"Û", "u"},
	  {"Ü", "u"},
	  {"Ý", "y"},
	  {"Þ", "p"},
	  {"ß", "s"},
	  {"à", "a"},
	  {"á", "a"},
	  {"â", "a"},
	  {"ã", "a"},
	  {"ä", "a"},
	  {"å", "a"},
	  {"æ", "a"},
	  {"ç", "c"},
	  {"è", "e"},
	  {"é", "e"},
	  {"ê", "e"},
	  {"ë", "e"},
	  {"ì", "i"},
	  {"í", "i"},
	  {"î", "i"},
	  {"ï", "i"},
	  {"ð", "o"},
	  {"ñ", "n"},
	  {"ò", "o"},
	  {"ó", "o"},
	  {"ô", "o"},
	  {"õ", "o"},
	  {"ö", "o"},
	  {"ø", "o"},
	  {"ù", "u"},
	  {"ú", "u"},
	  {"û", "u"},
	  {"ü", "u"},
	  {"ý", "y"},
	  {"þ", "p"},
	  {"ÿ", "y"}
     }; // 63 items
   
   if (_e_kbd_normalise_ready) return;
   _e_kbd_normalise_ready = 1;
   for (i = 0; i < 128; i++)
     _e_kbd_normalise_base[i] = tolower(i);
   for (;i < MAXLATIN; i++)
     {
	int glyph;
	int j;
	
	for (j = 0; j < 63; j++)
	  {
	     evas_string_char_next_get(table[j][0], 0, &glyph);
	     if (glyph == i)
	       {
		  _e_kbd_normalise_base[i] = *table[j][1];
		  break;
	       }
	  }
     }
}

static int
_e_kbd_dict_letter_normalise(int glyph)
{
   // FIXME: ö -> o, ä -> a, Ó -> o etc. - ie normalise to latin-1
   if (glyph < MAXLATIN) return _e_kbd_normalise_base[glyph];
   return tolower(glyph) & 0x7f;
}

static int
_e_kbd_dict_normalized_strncmp(const char *a, const char *b, int len)
{
   // FIXME: normalise 2 strings and then compare
   if (len < 0) return strcasecmp(a, b);
   return strncasecmp(a, b, len);
}

static int
_e_kbd_dict_normalized_strcmp(const char *a, const char *b)
{
   return _e_kbd_dict_normalized_strncmp(a, b, -1);
}

static void
_e_kbd_dict_normalized_strcpy(char *dst, const char *src)
{
   const char *p;
   char *d;
   
   for (p = src, d = dst; *p; p++, d++)
     {
	*d = _e_kbd_dict_letter_normalise(*p);
     }
   *d = 0;
}

static int
_e_kbd_dict_matches_loolup_cb_sort(const void *d1, const void *d2)
{
   const E_Kbd_Dict_Word *kw1, *kw2;
   
   kw1 = d1;
   kw2 = d2;
   if (kw1->usage < kw2->usage) return 1;
   else if (kw1->usage > kw2->usage) return -1;
   return 0;
}

static int
_e_kbd_dict_writes_cb_sort(const void *d1, const void *d2)
{
   const E_Kbd_Dict_Word *kw1, *kw2;
   
   kw1 = d1;
   kw2 = d2;
   return _e_kbd_dict_normalized_strcmp(kw1->word, kw2->word);
   return 0;
}

static const char *
_e_kbd_dict_line_next(E_Kbd_Dict *kd, const char *p)
{
   const char *e, *pp;
   
   e = kd->file.dict + kd->file.size;
   for (pp = p; pp < e; pp++)
     {
	if (*pp == '\n')
	  return pp + 1;
     }
   return NULL;
}

static char *
_e_kbd_dict_line_parse(E_Kbd_Dict *kd, const char *p, int *usage)
{
   const char *ps;
   char *wd = NULL;
   
   for (ps = p; !isspace(*ps); ps++);
   wd = malloc(ps - p + 1);
   if (!wd) return NULL;
   strncpy(wd, p, ps - p);
   wd[ps - p] = 0;
   if (*ps == '\n') *usage = 0;
   else
     {
	ps++;
	*usage = atoi(ps);
     }
   return wd;
}

static void
_e_kbd_dict_lookup_build_line(E_Kbd_Dict *kd, const char *p, const char *eol, 
			      int *glyphs)
{
   char *s;
   int p2;
   
   s = alloca(eol - p + 1);
   strncpy(s, p, eol - p);
   s[eol - p] = 0;
   p2 = evas_string_char_next_get(s, 0, &(glyphs[0]));
   if ((p2 > 0) && (glyphs[0] > 0))
     p2 = evas_string_char_next_get(s, p2, &(glyphs[1]));
}

static void
_e_kbd_dict_lookup_build(E_Kbd_Dict *kd)
{
   const char *p, *e, *eol;
   int glyphs[2], pglyphs[2];

   p = kd->file.dict;
   e = p + kd->file.size;
   pglyphs[0] = pglyphs[1] = 0;
   while (p < e)
     {
	eol = strchr(p, '\n');
	if (!eol) break;
	if (eol > p)
	  {
	     glyphs[0] = glyphs[1] = 0;
	     _e_kbd_dict_lookup_build_line(kd, p, eol, glyphs);
	     if ((glyphs[1] != pglyphs[1]) ||
		 (glyphs[0] != pglyphs[0]))
	       {
		  int v1, v2;
		  
		  if (isspace(glyphs[0]))
		    {
		       glyphs[0] = 0;
		       glyphs[1] = 0;
		    }
		  else if (isspace(glyphs[1]))
		    glyphs[1] = 0;
		  if (glyphs[0] == 0)
		    {
		       pglyphs[0] = pglyphs[1] = 0;
		       p = eol + 1;
		       continue;
		    }
		  v1 = _e_kbd_dict_letter_normalise(glyphs[0]);
		  v2 = _e_kbd_dict_letter_normalise(glyphs[1]);
		  if (!kd->lookup.tuples[v1][v2])
		    kd->lookup.tuples[v1][v2] = p;
		  pglyphs[0] = v1;
		  pglyphs[1] = v2;
	       }
	  }
	p = eol + 1;
     }
}

static int
_e_kbd_dict_open(E_Kbd_Dict *kd)
{
   struct stat st;
   
   kd->file.fd = open(kd->file.file, O_RDONLY);
   if (kd->file.fd < 0)
     return 0;
   if (fstat(kd->file.fd, &st) < 0)
     {
	close(kd->file.fd);
	return 0;
     }
   kd->file.size = st.st_size;
   kd->file.dict = mmap(NULL, kd->file.size, PROT_READ, MAP_SHARED,
			kd->file.fd, 0);
   if ((kd->file.dict== MAP_FAILED) || (kd->file.dict == NULL))
     {
	close(kd->file.fd);
	return 0;
     }
   return 1;
}

static void
_e_kbd_dict_close(E_Kbd_Dict *kd)
{
   if (kd->file.fd < 0) return;
   memset(kd->lookup.tuples, 0, sizeof(kd->lookup.tuples));
   munmap((void *)kd->file.dict, kd->file.size);
   close(kd->file.fd);
   kd->file.fd = -1;
   kd->file.dict = NULL;
   kd->file.size = 0;
}

EAPI E_Kbd_Dict *
e_kbd_dict_new(const char *file)
{
   // alloc and load new dict - build quick-lookup table. words MUST be sorted
   E_Kbd_Dict *kd;

   _e_kbd_normalise_init();
   kd = E_NEW(E_Kbd_Dict, 1);
   if (!kd) return NULL;
   kd->file.file = evas_stringshare_add(file);
   if (!kd->file.file)
     {
	free(kd);
	return NULL;
     }
   kd->file.fd = -1;
   if (!_e_kbd_dict_open(kd))
     {
	evas_stringshare_del(kd->file.file);
	free(kd);
	return NULL;
     }
   _e_kbd_dict_lookup_build(kd);
   return kd;
}

EAPI void
e_kbd_dict_free(E_Kbd_Dict *kd)
{
   // free dict and anything in it
   e_kbd_dict_word_letter_clear(kd);
   e_kbd_dict_save(kd);
   _e_kbd_dict_close(kd);
   free(kd);
}

static E_Kbd_Dict_Word *
_e_kbd_dict_changed_write_find(E_Kbd_Dict *kd, const char *word)
{
   Eina_List *l;
   
   for (l = kd->changed.writes; l; l = l->next)
     {
	E_Kbd_Dict_Word *kw;
	
	kw = l->data;
	if (!strcmp(kw->word, word)) return kw;
     }
   return NULL;
}

EAPI void
e_kbd_dict_save(E_Kbd_Dict *kd)
{
   FILE *f;
   
   // save any changes (new words added, usage adjustments).
   // all words MUST be sorted
   if (!kd->changed.writes) return;
   if (kd->changed.flush_timer)
     {
	ecore_timer_del(kd->changed.flush_timer);
	kd->changed.flush_timer = NULL;
     }
   ecore_file_unlink(kd->file.file);
   f = fopen(kd->file.file, "w");
   kd->changed.writes = eina_list_sort(kd->changed.writes,
				       eina_list_count(kd->changed.writes),
				       _e_kbd_dict_writes_cb_sort);
   if (f)
     {
	const char *p, *pn;
	
	p = kd->file.dict;
	while (p)
	  {
	     char *wd;
	     int usage = 0;
	     
	     pn = _e_kbd_dict_line_next(kd, p);
	     if (!pn) return;
	     wd = _e_kbd_dict_line_parse(kd, p, &usage);
	     if ((wd) && (strlen(wd) > 0))
	       {
		  if (kd->changed.writes)
		    {
		       int writeline;
		       
		       writeline = 0;
		       while (kd->changed.writes)
			 {
			    E_Kbd_Dict_Word *kw;
			    int cmp;
			    
			    kw = kd->changed.writes->data;
			    cmp = _e_kbd_dict_normalized_strcmp(kw->word, wd);
			    if (cmp < 0)
			      {
				 fprintf(f, "%s %i\n", kw->word, kw->usage);
				 writeline = 1;
				 evas_stringshare_del(kw->word);
				 free(kw);
				 kd->changed.writes  = eina_list_remove_list(kd->changed.writes, kd->changed.writes);
				 writeline = 1;
			      }
			    else if (cmp == 0)
			      {
				 fprintf(f, "%s %i\n", wd, kw->usage);
				 if (!strcmp(kw->word, wd))
				   writeline = 0;
				 else
				   writeline = 1;
				 evas_stringshare_del(kw->word);
				 free(kw);
				 kd->changed.writes  = eina_list_remove_list(kd->changed.writes, kd->changed.writes);
				 break;
			      }
			    else if (cmp > 0)
			      {
				 writeline = 1;
				 break;
			      }
			 }
		       if (writeline)
			 fprintf(f, "%s %i\n", wd, usage);
		    }
		  else
		    fprintf(f, "%s %i\n", wd, usage);
	       }
	     if (wd) free(wd);
	     p = pn;
	     if (p >= (kd->file.dict + kd->file.size)) break;
	  }
	while (kd->changed.writes)
	  {
	     E_Kbd_Dict_Word *kw;
			    
	     kw = kd->changed.writes->data;
	     fprintf(f, "%s %i\n", kw->word, kw->usage);
	     evas_stringshare_del(kw->word);
	     free(kw);
	     kd->changed.writes  = eina_list_remove_list(kd->changed.writes, kd->changed.writes);
	  }
	fclose(f);
     }
   _e_kbd_dict_close(kd);
   if (_e_kbd_dict_open(kd))
     _e_kbd_dict_lookup_build(kd);
}

static int
_e_kbd_dict_cb_save_flush(void *data)
{
   E_Kbd_Dict *kd;
   
   kd = data;
   if ((kd->matches.list) ||
       (kd->word.letters) ||
       (kd->matches.deadends) ||
       (kd->matches.leads))
     return 1;
   kd->changed.flush_timer = NULL;
   e_kbd_dict_save(kd);
   return 0;
}

static void
_e_kbd_dict_changed_write_add(E_Kbd_Dict *kd, const char *word, int usage)
{
   E_Kbd_Dict_Word *kw;
   
   kw = E_NEW(E_Kbd_Dict_Word, 1);
   kw->word = evas_stringshare_add(word);
   kw->usage = usage;
   kd->changed.writes = eina_list_prepend(kd->changed.writes, kw);
   if (eina_list_count(kd->changed.writes) > 64)
     {
	e_kbd_dict_save(kd);
     }
   else
     {
	if (kd->changed.flush_timer)
	  ecore_timer_del(kd->changed.flush_timer);
	kd->changed.flush_timer = ecore_timer_add(5.0, _e_kbd_dict_cb_save_flush, kd);
     }
}

static const char *
_e_kbd_dict_find_pointer(E_Kbd_Dict *kd, const char *p, int baselen, const char *word)
{
   const char *pn;
   int len;

   if (!p) return NULL;
   len = strlen(word);
   while (p)
     {
	pn = _e_kbd_dict_line_next(kd, p);
	if (!pn) return NULL;
	if ((pn - p) > len)
	  {
	     if (!_e_kbd_dict_normalized_strncmp(p, word, len))
	       return p;
	  }
	if (_e_kbd_dict_normalized_strncmp(p, word, baselen))
	  return NULL;
	p = pn;
	if (p >= (kd->file.dict + kd->file.size)) break;
     }
   return NULL;
}

static const char *
_e_kbd_dict_find(E_Kbd_Dict *kd, const char *word)
{
   const char *p;
   char *tword;
   int glyphs[2], p2, v1, v2, i;

   /* work backwards in leads. i.e.:
    * going
    * goin
    * goi
    * go
    * g
    */
   tword = alloca(strlen(word) + 1);
   _e_kbd_dict_normalized_strcpy(tword, word);
   p = evas_hash_find(kd->matches.leads, tword);
   if (p) return p;
   p2 = strlen(tword);
   while (tword[0])
     {
	p2 = evas_string_char_prev_get(tword, p2, &i);
	if (p2 < 0) break;
	tword[p2] = 0;
	p = evas_hash_find(kd->matches.leads, tword);
	if (p)
	  return _e_kbd_dict_find_pointer(kd, p, p2, word);
     }
   /* looking at leads going back letters didn't work */
   p = kd->file.dict;
   if ((p[0] == '\n') && (kd->file.size <= 1)) return NULL;
   glyphs[0] = glyphs[1] = 0;
   p2 = evas_string_char_next_get(word, 0, &(glyphs[0]));
   if ((p2 > 0) && (glyphs[0] > 0))
     p2 = evas_string_char_next_get(word, p2, &(glyphs[1]));
   v1 = _e_kbd_dict_letter_normalise(glyphs[0]);
   if (glyphs[1] != 0)
     {
	v2 = _e_kbd_dict_letter_normalise(glyphs[1]);
	p = kd->lookup.tuples[v1][v2];
     }
   else
     {
	for (i = 0; i < 128; i++)
	  {
	     p = kd->lookup.tuples[v1][i];
	     if (p) break;
	  }
     }
   return _e_kbd_dict_find_pointer(kd, p, p2, word);
}

static const char *
_e_kbd_dict_find_full(E_Kbd_Dict *kd, const char *word)
{
   const char *p;
   int len;
   
   p = _e_kbd_dict_find(kd, word);
   if (!p) return NULL;
   len = strlen(word);
   if (isspace(p[len])) return p;
   return NULL;
}

EAPI void
e_kbd_dict_word_usage_adjust(E_Kbd_Dict *kd, const char *word, int adjust)
{
   // add "adjust" to word usage count
   E_Kbd_Dict_Word *kw;
   
   kw = _e_kbd_dict_changed_write_find(kd, word);
   if (kw)
     {
	kw->usage += adjust;
	if (kd->changed.flush_timer)
	  ecore_timer_del(kd->changed.flush_timer);
	kd->changed.flush_timer = ecore_timer_add(5.0, _e_kbd_dict_cb_save_flush, kd);
     }
   else
     {
	const char *line;
	int usage = 0;

	line = _e_kbd_dict_find_full(kd, word);
	if (line)
	  {
	     char *wd;

	     // FIXME: we need to find an EXACT line match - case and all
	     wd = _e_kbd_dict_line_parse(kd, line, &usage);
	     if (wd) free(wd);
	  }
	usage += adjust;
	_e_kbd_dict_changed_write_add(kd, word, usage);
     }
}

EAPI void
e_kbd_dict_word_delete(E_Kbd_Dict *kd, const char *word)
{
   // delete a word from the dictionary
   E_Kbd_Dict_Word *kw;
   
   kw = _e_kbd_dict_changed_write_find(kd, word);
   if (kw)
     kw->usage = -1;
   else
     {
	if (_e_kbd_dict_find_full(kd, word))
	  _e_kbd_dict_changed_write_add(kd, word, -1);
     }
}

EAPI void
e_kbd_dict_word_letter_clear(E_Kbd_Dict *kd)
{
   // clear the current word buffer
   while (kd->word.letters)
     e_kbd_dict_word_letter_delete(kd);
   if (kd->matches.deadends)
     {
	evas_hash_free(kd->matches.deadends);
	kd->matches.deadends = NULL;
     }
   if (kd->matches.leads)
     {
	evas_hash_free(kd->matches.leads);
	kd->matches.leads = NULL;
     }
   while (kd->matches.list)
     {
	E_Kbd_Dict_Word *kw;
	
	kw = kd->matches.list->data;
	evas_stringshare_del(kw->word);
	free(kw);
	kd->matches.list = eina_list_remove_list(kd->matches.list, kd->matches.list);
    }
}

EAPI void
e_kbd_dict_word_letter_add(E_Kbd_Dict *kd, const char *letter, int dist)
{
   // add a letter with a distance (0 == closest) as an option for the current
   // letter position - advance starts a new letter position
   Eina_List *l, *list;
   E_Kbd_Dict_Letter *kl;
   
   l = eina_list_last(kd->word.letters);
   if (!l) return;
   list = l->data;
   kl = E_NEW(E_Kbd_Dict_Letter, 1);
   if (!kl) return;
   kl->letter = evas_stringshare_add(letter);
   kl->dist = dist;
   list = eina_list_append(list, kl);
   l->data = list;
}

EAPI void
e_kbd_dict_word_letter_advance(E_Kbd_Dict *kd)
{
   // start a new letter in the word
   kd->word.letters = eina_list_append(kd->word.letters, NULL);
}

EAPI void
e_kbd_dict_word_letter_delete(E_Kbd_Dict *kd)
{
   // delete the current letter completely
   Eina_List *l, *list;
   
   l = eina_list_last(kd->word.letters);
   if (!l) return;
   list = l->data;
   while (list)
     {
	E_Kbd_Dict_Letter *kl;
	
	kl = list->data;
	evas_stringshare_del(kl->letter);
	free(kl);
	list = eina_list_remove_list(list, list);
     }
   kd->word.letters = eina_list_remove_list(kd->word.letters, l);
}

static void
_e_kbd_dict_matches_lookup_iter(E_Kbd_Dict *kd, Eina_List *word, 
				Eina_List *more)
{
   Eina_List *l, *l2, *list;
   const char *p, *pn;
   char *base, *buf, *wd, *bufapp;
   E_Kbd_Dict_Letter *kl;
   int len, dist = 0, d, baselen, maxdist = 0, md;
   
   static int level = 0, lv;
   
   len = 0;
   level++;
   for (l = word; l; l = l->next)
     {
	kl = l->data;
	len += strlen(kl->letter);
	dist += kl->dist;
	if (kl->dist > maxdist) maxdist = kl->dist;
     }
   if (maxdist < 1) maxdist = 1;
   buf = alloca(len + 20); // 20 - just padding enough for 1 more utf8 char
   base = alloca(len + 20);
   base[0] = 0;
   for (l = word; l; l = l->next)
     {
	kl = l->data;
	strcat(base, kl->letter);
     }
   baselen = strlen(base);
   strcpy(buf, base);
   bufapp = buf + baselen;
   list = more->data;
   for (l = list; l; l = l->next)
     {
	kl = l->data;
	if (kl->dist > maxdist) maxdist = kl->dist;
     }
   for (l = list; l; l = l->next)
     {
	kl = l->data;
	strcpy(bufapp, kl->letter);
	if ((kd->matches.deadends) && evas_hash_find(kd->matches.deadends, buf))
	  continue;
	p = evas_hash_find(kd->matches.leads, buf);
	if (p) p = _e_kbd_dict_find_pointer(kd, p, baselen, buf);
	else p = _e_kbd_dict_find(kd, buf);
	if (!p)
	  {
	     kd->matches.deadends = evas_hash_add(kd->matches.deadends, buf, kd);
	     continue;
	  }
	else
	  kd->matches.leads = evas_hash_add(kd->matches.leads, buf, p);
	if ((!more->next) || (!more->next->data))
	  {
	     d = dist + kl->dist;
	     md = maxdist;
	     for (;;)
	       {
		  E_Kbd_Dict_Word *kw;
		  int usage = 0;
		  
		  wd = _e_kbd_dict_line_parse(kd, p, &usage);
		  if (!wd) break;
		  if (_e_kbd_dict_normalized_strcmp(wd, buf))
		    {
		       free(wd);
		       break;
		    }
		  kw = E_NEW(E_Kbd_Dict_Word, 1);
		  if (kw)
		    {
		       int accuracy;
		       int w, b, w2, b2, wc, bc, upper;
		       
		       // match any capitalisation
		       for (w = 0, b = 0; wd[w] && buf[b];)
			 {
			    b2 = evas_string_char_next_get(buf, b, &bc);
			    w2 = evas_string_char_next_get(wd, w, &wc);
			    if (isupper(bc)) wd[w] = toupper(wc);
			    w = w2;
			    b = b2;
			 }
		       kw->word = evas_stringshare_add(wd);
		       // FIXME: magic combination of distance metric and
		       // frequency of useage. this is simple now, but could
		       // be tweaked
		       wc = eina_list_count(word);
		       if (md < 1) md = 1;
		       
		       // basically a metric to see how far away teh keys that
		       // were actually pressed are away from the letters of
		       // this word in a physical on-screen sense
		       accuracy = md - (d / (wc + 1));
		       // usage is the frequency of usage in the dictionary.
		       // it its < 1 time, it's assumed to be 1.
		       if (usage < 1) usage = 1;
		       // multiply usage by a factor of 100 for better detailed
		       // sorting. 10 == 1/10th factor
		       usage = 100 + ((usage - 1) * 10);
		       // and well just multiply and lets see. maybe this can
		       // do with multiplication factors etc. but simple for
		       // now.
		       kw->usage = (usage * accuracy) / md;
		       kd->matches.list = eina_list_append(kd->matches.list, kw);
		    }
		  free(wd);
		  p = _e_kbd_dict_line_next(kd, p);
		  if (p >= (kd->file.dict + kd->file.size)) break;
		  if (!p) break;
	       }
	  }
	else
	  {
	     word = eina_list_append(word, kl);
	     _e_kbd_dict_matches_lookup_iter(kd, word, more->next);
	     word = eina_list_remove_list(word, eina_list_last(word));
	  }
     }
   level--;
}

EAPI void
e_kbd_dict_matches_lookup(E_Kbd_Dict *kd)
{
   // find all matches and sort them
   while (kd->matches.list)
     {
	E_Kbd_Dict_Word *kw;
	
	kw = kd->matches.list->data;
	evas_stringshare_del(kw->word);
	free(kw);
	kd->matches.list = eina_list_remove_list(kd->matches.list, kd->matches.list);
    }
   if (kd->word.letters)
     _e_kbd_dict_matches_lookup_iter(kd, NULL, kd->word.letters);
   kd->matches.list = eina_list_sort(kd->matches.list,
				     eina_list_count(kd->matches.list),
				     _e_kbd_dict_matches_loolup_cb_sort);
}

EAPI void
e_kbd_dict_matches_first(E_Kbd_Dict *kd)
{
   // jump to first match
   kd->matches.list_ptr = kd->matches.list;
}

EAPI void
e_kbd_dict_matches_next(E_Kbd_Dict *kd)
{
   // jump to next match
   kd->matches.list_ptr = kd->matches.list_ptr->next;
}

EAPI const char *
e_kbd_dict_matches_match_get(E_Kbd_Dict *kd, int *pri_ret)
{
   // return the word (string utf-8) for the current match
   if (kd->matches.list_ptr)
     {
	E_Kbd_Dict_Word *kw;
	
	kw = kd->matches.list_ptr->data;
	if (kw)
	  {
	     *pri_ret = kw->usage;
	     return kw->word;
	  }
     }
   return NULL;
}
