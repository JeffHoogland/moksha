#include "block.h"
#include "debug.h"
#include "util.h"

typedef struct _e_block E_Block;

struct _e_block
{
   char               *name;
   int                 refs;
};

static Evas_List    blocks = NULL;

static E_Block     *e_block_find(char *name);

static E_Block     *
e_block_find(char *name)
{
   Evas_List           l;

   D_ENTER;
   for (l = blocks; l; l = l->next)
     {
	E_Block            *b;

	b = l->data;
	if (!strcmp(b->name, name))
	  {
	     blocks = evas_list_remove(blocks, b);
	     blocks = evas_list_prepend(blocks, b);
	     D_RETURN_(b);
	  }
     }
   D_RETURN_(NULL);
}

void
e_block_start(char *name)
{
   E_Block            *b;

   D_ENTER;
   b = e_block_find(name);
   if (b)
     {
	b->refs++;
	D_RETURN;
     }
   b = NEW(E_Block, 1);
   ZERO(b, E_Block, 1);
   e_strdup(b->name, name);
   b->refs = 1;
   blocks = evas_list_prepend(blocks, b);
   D_RETURN;
}

void
e_block_stop(char *name)
{
   E_Block            *b;

   D_ENTER;
   b = e_block_find(name);
   if (b)
     {
	b->refs--;
	if (b->refs < 1)
	  {
	     blocks = evas_list_remove(blocks, b);
	     IF_FREE(b->name);
	     FREE(b);
	  }
     }
   D_RETURN;
}

int
e_block_is_active(char *name)
{
   E_Block            *b;

   D_ENTER;
   b = e_block_find(name);
   if (b)
     {
	D_RETURN_(b->refs);
     }
   D_RETURN_(0);
}
