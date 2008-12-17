#ifndef E_KBD_DICT_H
#define E_KBD_DICT_H

typedef struct _E_Kbd_Dict E_Kbd_Dict;
typedef struct _E_Kbd_Dict_Word E_Kbd_Dict_Word;
typedef struct _E_Kbd_Dict_Letter E_Kbd_Dict_Letter;

struct _E_Kbd_Dict_Word
{
   const char *word;
   int usage;
};

struct _E_Kbd_Dict_Letter
{
   const char *letter;
   int         dist;
};

struct _E_Kbd_Dict
{
   struct {
      const char *file;
      int         fd;
      const char *dict;
      int         size;
   } file;
   struct {
      const char *tuples[128][128];
   } lookup;
   struct {
      Ecore_Timer *flush_timer;
      Eina_List *writes;
   } changed;
   struct {
      Eina_List *letters;
   } word;
   struct {
      Eina_Hash *deadends;
      Eina_Hash *leads;
      Eina_List *list;
      Eina_List *list_ptr;
   } matches;
};


EAPI E_Kbd_Dict *e_kbd_dict_new(const char *file);
EAPI void e_kbd_dict_free(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_save(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_word_usage_adjust(E_Kbd_Dict *kd, const char *word, int adjust);
EAPI void e_kbd_dict_word_delete(E_Kbd_Dict *kd, const char *word);
EAPI void e_kbd_dict_word_letter_clear(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_word_letter_add(E_Kbd_Dict *kd, const char *letter, int dist);
EAPI void e_kbd_dict_word_letter_advance(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_word_letter_delete(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_matches_lookup(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_matches_first(E_Kbd_Dict *kd);
EAPI void e_kbd_dict_matches_next(E_Kbd_Dict *kd);
EAPI const char *e_kbd_dict_matches_match_get(E_Kbd_Dict *kd, int *pri_ret);

#endif
