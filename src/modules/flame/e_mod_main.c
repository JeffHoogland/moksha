#include "e.h"
#include "e_mod_main.h"

#include <time.h>

/* TODO List:
 * 
 * * Add more palettes
 * * Change the icon
 * * Bad hack for ff->im and the evas object (see face_init)
 * 
 */

/* module private routines */
static Flame  *_flame_init                 (E_Module *m);
static void    _flame_shutdown             (Flame *f);
static E_Menu *_flame_config_menu_new      (Flame *f);
static void    _flame_menu_gold_palette    (void *data, E_Menu *m, E_Menu_Item *mi);
static void    _flame_menu_fire_palette    (void *data, E_Menu *m, E_Menu_Item *mi);
static void    _flame_menu_plasma_palette  (void *data, E_Menu *m, E_Menu_Item *mi);
static void    _flame_config_menu_del      (Flame *f, E_Menu *m);
static void    _flame_config_palette_set   (Flame *f, Flame_Palette_Type type);

static int  _flame_face_init           (Flame_Face *ff);
static void _flame_face_free           (Flame_Face *ff);
static void _flame_face_anim_handle    (Flame_Face *ff);

static void _flame_palette_gold_set    (Flame_Face *ff);
static void _flame_palette_fire_set    (Flame_Face *ff);
static void _flame_palette_plasma_set  (Flame_Face *ff);
static void _flame_zero_set            (Flame_Face *ff);
static void _flame_base_random_set     (Flame_Face *ff);
static void _flame_base_random_modify  (Flame_Face *ff);
static void _flame_process             (Flame_Face *ff);
static int  _flame_cb_draw             (void *data);
static int  _flame_cb_event_container_resize(void *data, int type, void *event);

static int powerof (unsigned int n);

char          *_flame_module_dir;

/* public module routines. all modules must have these */
void *
init (E_Module *m)
{
   Flame *f;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show ("Module API Error",
			     "Error initializing Module: Flame\n"
			     "It requires a minimum module API version of: %i.\n"
			     "The module API advertized by Enlightenment is: %i.\n"
			     "Aborting module.",
			     E_MODULE_API_VERSION,
			     m->api->version);
	return NULL;
     }
   /* actually init ibar */
   f = _flame_init (m);
   m->config_menu = _flame_config_menu_new (f);
   
   return f;
}

int
shutdown (E_Module *m)
{
   Flame *f;
   
   f = m->data;
   if (f)
     {
	if (m->config_menu)
	  {
	     _flame_config_menu_del (f, m->config_menu);
	     m->config_menu = NULL;
	  }
	_flame_shutdown (f);
     }
   
   return 1;
}

int
save (E_Module *m)
{
   Flame *f;
   
   f = m->data;
   e_config_domain_save("module.flame", f->conf_edd, f->conf);
   
   return 1;
}

int
info (E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Flame");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   
   return 1;
}

int
about (E_Module *m)
{
   e_error_dialog_show ("Enlightenment Flame Module",
			"A simple module to display flames.");
   return 1;
}

/* module private routines */
static Flame *
_flame_init (E_Module *m)
{
   Flame *f;
   Evas_List *managers, *l, *l2;
   
   f = calloc(1, sizeof(Flame));
   if (!f) return NULL;
   
   /* Configuration */
   
   f->conf_edd = E_CONFIG_DD_NEW("Flame_Config", Config);
#undef T
#undef D
#define T Config
#define D f->conf_edd
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, hspread, INT);
   E_CONFIG_VAL(D, T, vspread, INT);
   E_CONFIG_VAL(D, T, variance, INT);
   E_CONFIG_VAL(D, T, vartrend, INT);
   E_CONFIG_VAL(D, T, residual, INT);
   E_CONFIG_VAL(D, T, palette_type, INT);
   if (!f->conf)
     {
	f->conf = E_NEW (Config, 1);
	f->conf->height = 128;
	f->conf->hspread = 26;
	f->conf->vspread = 76;
	f->conf->variance = 5;
	f->conf->vartrend = 2;
	f->conf->residual = 68;
	f->conf->palette_type = GOLD_PALETTE;
     }
   E_CONFIG_LIMIT(f->conf->height, 4, 4096);
   E_CONFIG_LIMIT(f->conf->hspread, 1, 100);
   E_CONFIG_LIMIT(f->conf->vspread, 1, 100);
   E_CONFIG_LIMIT(f->conf->variance, 1, 100);
   E_CONFIG_LIMIT(f->conf->vartrend, 1, 100);
   E_CONFIG_LIMIT(f->conf->residual, 1, 100);
   E_CONFIG_LIMIT(f->conf->palette_type, GOLD_PALETTE, PLASMA_PALETTE);
   
   managers = e_manager_list ();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Flame_Face  *ff;
	     
	     con = l2->data;
	     ff = calloc(1, sizeof(Flame_Face));
	     if (ff)
	       {
		  f->face = ff;
		  ff->flame = f;
		  ff->con   = con;
		  ff->evas  = con->bg_evas;
		  if (!_flame_face_init(ff))
		    return NULL;
	       }
	  }
     }
   
   return f;
}

static void
_flame_shutdown (Flame *f)
{
   free(f->conf);
   E_CONFIG_DD_FREE(f->conf_edd);
   _flame_face_free(f->face);
   free(f);
}

static E_Menu *
_flame_config_menu_new (Flame *f)
{
   E_Menu      *mn;
   E_Menu_Item *mi;
   
   /* FIXME: hook callbacks to each menu item */
   mn = e_menu_new ();
   
   mi = e_menu_item_new (mn);
   e_menu_item_label_set (mi, "Gold Palette");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (f->conf->palette_type == GOLD_PALETTE) e_menu_item_toggle_set (mi, 1);
   e_menu_item_callback_set (mi, _flame_menu_gold_palette, f);
   
   mi = e_menu_item_new (mn);
   e_menu_item_label_set (mi, "Fire Palette");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (f->conf->palette_type == FIRE_PALETTE) e_menu_item_toggle_set (mi, 1);
   e_menu_item_callback_set (mi, _flame_menu_fire_palette, f);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Plasma Palette");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (f->conf->palette_type == PLASMA_PALETTE) e_menu_item_toggle_set (mi, 1);
   e_menu_item_callback_set (mi, _flame_menu_plasma_palette, f);
   
   f->config_menu = mn;
   
   return mn;
}

static void
_flame_menu_gold_palette (void *data, E_Menu *m, E_Menu_Item *mi)
{
   Flame *f;
   
   f = (Flame *)data;
   _flame_config_palette_set (f, GOLD_PALETTE);
}

static void
_flame_menu_fire_palette (void *data, E_Menu *m, E_Menu_Item *mi)
{
   Flame *f;
   
   f = (Flame *)data;
   _flame_config_palette_set (f, FIRE_PALETTE);
}

static void
_flame_menu_plasma_palette (void *data, E_Menu *m, E_Menu_Item *mi)
{
   Flame *f;
   
   f = (Flame *)data;
   _flame_config_palette_set (f, PLASMA_PALETTE);
}

static void
_flame_config_menu_del (Flame *f, E_Menu *m)
{
   e_object_del (E_OBJECT(m));
}

static void
_flame_config_palette_set (Flame *f, Flame_Palette_Type type)
{
   switch (type)
     {
      case GOLD_PALETTE:
	_flame_palette_gold_set (f->face);
	break;
      case FIRE_PALETTE:
	_flame_palette_fire_set (f->face);
	break;
      case PLASMA_PALETTE:
	_flame_palette_plasma_set (f->face);
	break;
      default:
	break;
     }
}

static int
_flame_face_init (Flame_Face *ff)
{
   Evas_Object *o;
   Evas_Coord   ww, hh;
   int         size;
   int         flame_width, flame_height;
   
   ff->ev_handler_container_resize =
     ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
			     _flame_cb_event_container_resize,
			     ff);
   /* set up the flame object */
   o = evas_object_image_add (ff->evas);
   evas_output_viewport_get(ff->evas, NULL, NULL, &ww, &hh);
   ff->ww = ww;
   printf ("Size : %d %d\n", ww, hh);
   evas_object_move (o, 0, hh - ff->flame->conf->height + 3);
   evas_object_resize (o, ff->ww, ff->flame->conf->height);
   evas_object_image_fill_set (o, 0, 0, ff->ww, ff->flame->conf->height);
   evas_object_pass_events_set(o, 1);
   evas_object_layer_set (o, 20);
   evas_object_image_alpha_set(o, 1);
   evas_object_show (o);
   ff->flame_object = o;
   
   /* Allocation of the flame arrays */
   flame_width  = ff->ww >> 1;
   flame_height = ff->flame->conf->height >> 1;
   ff->ws = powerof (flame_width);
   size = (1 << ff->ws) * flame_height * sizeof (int);
   ff->f_array1 = (unsigned int *)malloc (size);
   if (!ff->f_array1)
     return 0;
   ff->f_array2 = (unsigned int *)malloc (size);
   if (!ff->f_array2)
     return 0;
   
   /* allocation of the image */
   ff->ims = powerof (ff->ww);
   evas_object_image_size_set (ff->flame_object,
			       1<< ff->ims, ff->flame->conf->height);
   evas_object_image_fill_set (o, 0, 0, 1<< ff->ims, ff->flame->conf->height);
   printf ("Size : %d %d\n", 1<< ff->ims, ff->flame->conf->height);
   ff->im = (unsigned int *)evas_object_image_data_get (ff->flame_object, 1);
   
   /* initialization of the palette */
   ff->palette = (unsigned int *)malloc (300 * sizeof (unsigned int));
   if (!ff->palette) return 0;
   
   _flame_config_palette_set (ff->flame, ff->flame->conf->palette_type);
   
   /* set the flame array to ZERO */
   _flame_zero_set (ff);
   
   /* set the base of the flame to something random */
   _flame_base_random_set (ff);
   
   /* set the animator for generating and displaying flames */
   _flame_face_anim_handle (ff);
   
   return 1;
}

static void
_flame_face_free(Flame_Face *ff)
{
   ecore_event_handler_del(ff->ev_handler_container_resize);
   evas_object_del (ff->flame_object);
   if (ff->anim) ecore_animator_del(ff->anim);
   if (ff->f_array1) free (ff->f_array1);
   if (ff->f_array2) free (ff->f_array2);
   if (ff->palette) free (ff->palette);
   free (ff);
}

static void
_flame_face_anim_handle (Flame_Face *ff)
{
   if (!ff->anim)
     ff->anim = ecore_animator_add (_flame_cb_draw, ff);
}

static void
_flame_palette_gold_set (Flame_Face *ff)
{
   const unsigned char gold_cmap[300 * 4] =
     "\256\256\0\1\254i\24\3\312\2165\5"
     "\330\212<\7\340\244I\10\344\235R\12\332\236P\15\334\247P\17\340\242U\20\343"
     "\253X\22\340\253V\25\335\245U\27\344\254X\31\353\250[\32\352\254Y\34\346"
     "\251X\37\347\251Z!\351\255\\\"\351\253[$\345\253Z'\346\252\\)\347\253]+\347"
     "\256\\,\346\253\\/\344\253\\1\346\256]3\347\254\\4\351\256]6\351\253\\9\351"
     "\254^;\352\256\\=\352\255^>\350\255]A\350\254^C\350\255\\E\351\256^F\350"
     "\255^I\347\255^K\347\255]M\350\256]O\352\257^Q\351\255_S\352\256^U\352\255"
     "]W\352\255_X\351\256_[\351\255]]\351\256^_\351\256^a\351\256_c\350\256_e"
     "\350\256^g\351\256^i\351\256_k\350\255_m\352\256^o\352\256_q\353\257_s\352"
     "\257_u\351\255^w\351\256_y\352\256_{\352\256`}\350\256^\177\351\255_\201"
     "\351\257_\203\351\256_\205\351\256_\207\351\256_\211\352\256_\213\353\257"
     "_\215\352\256_\217\351\256_\221\352\257_\223\352\256_\225\352\257_\227\351"
     "\256_\231\351\256_\233\351\257_\235\352\256`\237\351\256_\241\351\257_\243"
     "\352\256_\245\353\257`\247\353\257`\251\352\256_\253\352\257_\255\352\256"
     "`\257\352\257`\261\351\257_\263\351\256_\265\352\257`\267\352\257`\271\352"
     "\257`\273\351\257_\275\352\256`\277\352\257`\301\352\256`\303\352\257_\305"
     "\352\257`\307\352\257`\311\352\257`\313\352\257_\315\352\256`\317\352\257"
     "`\321\352\257`\324\352\260a\326\352\261`\330\353\262b\332\353\262c\335\353"
     "\264c\337\353\265d\342\353\265e\344\354\266f\346\354\267f\350\354\270g\352"
     "\354\271g\354\354\272g\356\354\273i\360\355\274i\362\355\275j\364\355\275"
     "k\365\355\276k\367\355\277l\371\356\300l\372\356\301m\373\356\301m\375\356"
     "\301n\376\356\303o\376\357\304p\376\357\304p\377\357\305q\377\357\306q\377"
     "\357\307r\377\360\310s\377\360\311s\377\360\311t\377\360\312u\377\361\313"
     "v\377\361\314v\377\361\315w\377\361\316w\377\362\317x\377\362\320y\377\362"
     "\320y\377\362\321z\377\362\322{\377\363\323|\377\363\324|\377\363\325}\377"
     "\363\326~\377\364\327~\377\364\327\177\377\364\330\200\377\364\331\200\377"
     "\365\332\201\377\365\333\202\377\365\334\202\377\365\335\203\377\365\336"
     "\204\377\366\337\204\377\366\337\205\377\366\340\206\377\366\341\206\377"
     "\367\342\207\377\367\343\210\377\367\344\210\377\367\345\211\377\367\346"
     "\212\377\370\347\212\377\370\347\213\377\370\350\214\377\370\351\214\377"
     "\371\352\215\377\371\353\216\377\371\354\216\377\371\355\217\377\372\356"
     "\220\377\372\357\220\377\372\357\221\377\372\360\222\377\372\361\223\377"
     "\373\362\224\377\373\363\226\377\373\363\230\377\373\364\232\377\374\365"
     "\235\377\374\366\237\377\374\367\241\377\374\370\244\377\375\371\246\377"
     "\375\372\250\377\375\373\252\377\375\373\254\377\375\373\256\377\376\373"
     "\260\377\376\374\262\377\376\374\262\377\376\374\264\377\376\374\266\377"
     "\376\374\270\377\376\374\272\377\376\374\273\377\376\374\274\377\376\374"
     "\276\377\376\374\300\377\376\374\301\377\376\374\303\377\376\374\305\377"
     "\376\374\306\377\376\374\310\377\376\374\311\377\376\374\313\377\376\374"
     "\315\377\376\374\317\377\376\374\320\377\376\375\321\377\376\375\323\377"
     "\376\375\325\377\376\375\327\377\376\375\330\377\376\375\332\377\376\375"
     "\333\377\376\375\335\377\376\375\336\377\376\375\340\377\376\375\342\377"
     "\376\375\343\377\376\375\345\377\376\375\346\377\376\375\350\377\376\375"
     "\352\377\376\375\354\377\376\375\355\377\376\375\356\377\376\375\360\377"
     "\376\375\362\377\376\375\364\377\376\375\365\377\376\375\367\377\376\375"
     "\370\377\376\375\372\377\376\375\373\377\376\375\374\377\376\375\375\377"
     "\376\375\375\377\376\376\375\377\376\376\375\377\376\376\375\377\376\376"
     "\375\377\376\376\375\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
     "\377\377\377\377"
     ;
   int i;
   
   for (i = 0 ; i < 300 ; i++)
     {
	ff->palette[i] =
	  (gold_cmap[(i * 4) + 3] << 24) |
	  (gold_cmap[(i * 4) + 0] << 16) |
	  (gold_cmap[(i * 4) + 1] << 8 ) |
	  (gold_cmap[(i * 4) + 2]);
     }
}

static void
_flame_palette_fire_set (Flame_Face *ff)
{
   int i, r, g, b, a;
   
   for (i = 0 ; i < 300 ; i++)
     {
	r = i * 3;
	g = (i - 80) * 3;
	b = (i - 160) * 3;
	
	if (r < 0)   r = 0;
	if (r > 255) r = 255;
	if (g < 0)   g = 0;
	if (g > 255) g = 255;
	if (b < 0)   b = 0;
	if (b > 255) b = 255;
	a = (int)((r * 0.299) + (g * 0.587) + (b * 0.114));
	ff->palette[i] = ((((unsigned char)a) << 24) |
			  (((unsigned char)r) << 16) |
			  (((unsigned char)g) << 8)  |
			  ((unsigned char)b));
     }
}

/* set the plasma flame palette */
static void
_flame_palette_plasma_set (Flame_Face *ff)
{
   int i, r, g, b, a;
   
   for (i = 0 ; i < 80 ; i++)
     {
	r = 0;
	g = 0;
	b = (i*255) / 80;
	a = (int)((r * 0.299) + (g * 0.587) + (b * 0.114));
	ff->palette[i] = ((((unsigned char)a) << 24) |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
     }
   for (i = 80 ; i < 160 ; i++)
     {
	r = ((i-80)*186) / 80;
	g = ((i-80)*229) / 80;
	b = 255;
	
	if ((r*r + g*g + b*b) <= 100)
	  ff->palette[i] = ((r*r + g*g + b*b)                |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
	else
	  ff->palette[i] = ((255 << 24)        |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
     }
   for (i = 160 ; i < 300 ; i++)
     {
	r = ((i-160)*(255 - 186) + 186 * 139) / 139;
	g = ((i-160)*(255 - 229) + 229 * 139) / 139;
	b = 255;
	
	if ((r*r + g*g + b*b) <= 100)
	  ff->palette[i] = ((r*r + g*g + b*b)                |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
	else
	  ff->palette[i] = ((255 << 24)        |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
     }
}

/* set the flame array to zero */
static void
_flame_zero_set (Flame_Face *ff)
{
   int x, y;
   unsigned int *ptr;
   
   for (y = 0 ; y < (ff->flame->conf->height >> 1) ; y++)
     {
	for (x = 0 ; x < (ff->ww >> 1) ; x++)
	  {
	     ptr = ff->f_array1 + (y << ff->ws) + x;
	     *ptr = 0;
	  }
     }
   
   for (y = 0 ; y < (ff->flame->conf->height >> 1) ; y++)
     {
	for (x = 0 ; x < (ff->ww >> 1) ; x++)
	  {
	     ptr = ff->f_array2 + (y << ff->ws) + x;
	     *ptr = 0;
	  }
     }
}

/* set the base of the flame */
static void
_flame_base_random_set (Flame_Face *ff)
{
   int x, y;
   unsigned int *ptr;
   
   /* initialize a random number seed from the time, so we get random */
   /* numbers each time */
//   srand (time(NULL));
   y = (ff->flame->conf->height >> 1) - 1;
   for (x = 0 ; x < (ff->ww >> 1) ; x++)
     {
	ptr = ff->f_array1 + (y << ff->ws) + x;
	*ptr = rand ()%300;
     }
}

/* modify the base of the flame with random values */
static void
_flame_base_random_modify (Flame_Face *ff)
{
   int x, y;
   unsigned int *ptr, val;
   
   y = (ff->flame->conf->height >> 1) - 1;
   for (x = 0 ; x < (ff->ww >> 1) ; x++)
     {
	ptr = ff->f_array1 + (y << ff->ws) + x;
	*ptr += ((rand ()%ff->flame->conf->variance) - ff->flame->conf->vartrend);
	val = *ptr;
	if (val > 300) *ptr = 0;
     }
}

/* process entire flame array */
static void
_flame_process (Flame_Face *ff)
{
   int x, y;
   unsigned int *ptr, *p, tmp, val;
   
   for (y = ((ff->flame->conf->height >> 1) - 1) ; y >= 2 ; y--)
     {
	for (x = 1 ; x < ((ff->ww >> 1) - 1) ; x++)
	  {
	     ptr = ff->f_array1 + (y << ff->ws) + x;
	     val = (int)*ptr;
	     if (val > 300)
	       *ptr = 300;
	     val = (int)*ptr;
	     if (val > 0)
	       {
		  tmp = (val * ff->flame->conf->vspread) >> 8;
		  p   = ptr - (2 << ff->ws);
		  *p  = *p + (tmp >> 1);
		  p   = ptr - (1 << ff->ws);
		  *p  = *p + tmp;
		  tmp = (val * ff->flame->conf->hspread) >> 8;
		  p   = ptr - (1 << ff->ws) - 1;
		  *p  = *p + tmp;
		  p   = ptr - (1 << ff->ws) + 1;
		  *p  = *p + tmp;
		  p   = ptr - 1;
		  *p  = *p + (tmp >>1 );
		  p   = ptr + 1;
		  *p  = *p + (tmp >> 1);
		  p   = ff->f_array2 + (y << ff->ws) + x;
		  *p  = val;
		  if (y < ((ff->flame->conf->height >> 1) - 1))
		    *ptr = (val * ff->flame->conf->residual) >> 8;
	       }
	  }
     }
}

/* draw a flame on the evas */
static int
_flame_cb_draw (void *data)
{
   Flame_Face    *ff;
   unsigned int  *ptr;
   int            x, y, xx, yy;
   unsigned int   cl, cl1, cl2, cl3, cl4;
   unsigned int  *cptr;
   
   ff = (Flame_Face *)data;
   
   /* modify the base of the flame */
   _flame_base_random_modify (ff);
   /* process the flame array, propagating the flames up the array */
   _flame_process (ff);
   
   
   for (y = 0 ; y < ((ff->flame->conf->height >> 1) - 1) ; y++)
     {
	for (x = 0 ; x < ((ff->ww >> 1) - 1) ; x++)
	  {
	     xx = x << 1;
	     yy = y << 1;
	     
	     ptr = ff->f_array2 + (y << ff->ws) + x;
	     cl1 = cl = (unsigned int)*ptr;
	     ptr = ff->f_array2 + (y << ff->ws) + x + 1;
	     cl2 = (unsigned int)*ptr;
	     ptr = ff->f_array2 + ((y + 1) << ff->ws) + x + 1;
	     cl3 = (unsigned int)*ptr;
	     ptr = ff->f_array2 + ((y + 1) << ff->ws) + x;
	     cl4 = (unsigned int)*ptr;
	     
	     cptr = ff->im + (yy << ff->ims) + xx;
	     *cptr = ff->palette[cl];
	     *(cptr + 1) = ff->palette[((cl1+cl2) >> 1)];
	     *(cptr + 1 + (1 << ff->ims)) = ff->palette[((cl1 + cl3) >> 1)];
	     *(cptr + (1 << ff->ims)) = ff->palette[((cl1 + cl4) >> 1)];
	  }
     }
   
   evas_object_image_data_set (ff->flame_object, ff->im);
   evas_object_image_data_update_add (ff->flame_object,
				      0, 0,
				      ff->ww, ff->flame->conf->height);
   
   /* we loop indefinitely */
   return 1;
}

static int
_flame_cb_event_container_resize(void *data, int type, void *event)
{
   Flame_Face *ff;
   Evas_Object *o;
   Evas_Coord   ww, hh;
   int         size;
   int         flame_width, flame_height;
   
   ff = data;
   evas_output_viewport_get(ff->evas, NULL, NULL, &ww, &hh);
   ff->ww = ww;
   o = ff->flame_object;
   printf ("Size : %d %d\n", ww, hh);
   evas_object_move (o, 0, hh - ff->flame->conf->height + 3);
   evas_object_resize (o, ff->ww, ff->flame->conf->height);
   evas_object_image_fill_set (o, 0, 0, ff->ww, ff->flame->conf->height);
   
   /* Allocation of the flame arrays */
   flame_width  = ff->ww >> 1;
   flame_height = ff->flame->conf->height >> 1;
   ff->ws = powerof (flame_width);
   size = (1 << ff->ws) * flame_height * sizeof (int);
   if (ff->f_array1) free(ff->f_array1);
   ff->f_array1 = (unsigned int *)malloc (size);
   if (!ff->f_array1)
     return 0;
   if (ff->f_array2) free(ff->f_array2);
   ff->f_array2 = (unsigned int *)malloc (size);
   if (!ff->f_array2)
     return 0;
   
   /* allocation of the image */
   ff->ims = powerof (ff->ww);
   evas_object_image_size_set (ff->flame_object,
			       1<< ff->ims, ff->flame->conf->height);
   evas_object_image_fill_set (o, 0, 0, 1<< ff->ims, ff->flame->conf->height);
   printf ("Size : %d %d\n", 1<< ff->ims, ff->flame->conf->height);
   ff->im = (unsigned int *)evas_object_image_data_get (ff->flame_object, 1);
   return 1;
}

/* return the power of a number (eg powerof(8)==3, powerof(256)==8,
 * powerof(1367)==11, powerof(2568)==12) */
static int
powerof (unsigned int n)
{
   int p = 32;
   
   if (n<=0x80000000) p=31;
   if (n<=0x40000000) p=30;
   if (n<=0x20000000) p=29;
   if (n<=0x10000000) p=28;
   if (n<=0x08000000) p=27;
   if (n<=0x04000000) p=26;
   if (n<=0x02000000) p=25;
   if (n<=0x01000000) p=24;
   if (n<=0x00800000) p=23;
   if (n<=0x00400000) p=22;
   if (n<=0x00200000) p=21;
   if (n<=0x00100000) p=20;
   if (n<=0x00080000) p=19;
   if (n<=0x00040000) p=18;
   if (n<=0x00020000) p=17;
   if (n<=0x00010000) p=16;
   if (n<=0x00008000) p=15;
   if (n<=0x00004000) p=14;
   if (n<=0x00002000) p=13;
   if (n<=0x00001000) p=12;
   if (n<=0x00000800) p=11;
   if (n<=0x00000400) p=10;
   if (n<=0x00000200) p=9;
   if (n<=0x00000100) p=8;
   if (n<=0x00000080) p=7;
   if (n<=0x00000040) p=6;
   if (n<=0x00000020) p=5;
   if (n<=0x00000010) p=4;
   if (n<=0x00000008) p=3;
   if (n<=0x00000004) p=2;
   if (n<=0x00000002) p=1;
   if (n<=0x00000001) p=0;
   return p;
}
