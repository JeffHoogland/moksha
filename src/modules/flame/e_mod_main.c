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
static void    _flame_menu_default_palette (void *data, E_Menu *m, E_Menu_Item *mi);
static void    _flame_menu_plasma_palette  (void *data, E_Menu *m, E_Menu_Item *mi);
static void    _flame_config_menu_del      (Flame *f, E_Menu *m);
static void    _flame_config_palette_set   (Flame *f, Flame_Palette_Type type);

static int  _flame_face_init         (Flame_Face *ff);
static void _flame_face_free         (Flame_Face *ff);
static void _flame_face_anim_handle (Flame_Face *ff);

static void _flame_palette_default_set (Flame_Face *ff);
static void _flame_palette_plasma_set  (Flame_Face *ff);
static void _flame_zero_set            (Flame_Face *ff);
static void _flame_base_random_set     (Flame_Face *ff);
static void _flame_base_random_modify  (Flame_Face *ff);
static void _flame_process             (Flame_Face *ff);
static int  _flame_cb_draw             (void *data);

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
	f->conf->vspread = 48;
	f->conf->variance = 5;
	f->conf->vartrend = 2;
	f->conf->residual = 68;
	f->conf->palette_type = DEFAULT_PALETTE;
     }
   E_CONFIG_LIMIT(f->conf->height, 4, 4096);
   E_CONFIG_LIMIT(f->conf->hspread, 1, 100);
   E_CONFIG_LIMIT(f->conf->vspread, 1, 100);
   E_CONFIG_LIMIT(f->conf->variance, 1, 100);
   E_CONFIG_LIMIT(f->conf->vartrend, 1, 100);
   E_CONFIG_LIMIT(f->conf->residual, 1, 100);
   E_CONFIG_LIMIT(f->conf->palette_type, 0, 100);
   
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
   e_menu_item_label_set (mi, "Default Palette");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (f->conf->palette_type == DEFAULT_PALETTE) e_menu_item_toggle_set (mi, 1);
   e_menu_item_callback_set (mi, _flame_menu_default_palette, f);
   
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
_flame_menu_default_palette (void *data, E_Menu *m, E_Menu_Item *mi)
{
   Flame *f;
   
   f = (Flame *)data;
   _flame_config_palette_set (f, DEFAULT_PALETTE);
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
      case DEFAULT_PALETTE:
	_flame_palette_default_set (f->face);
	break;
      case PLASMA_PALETTE:
	_flame_palette_plasma_set (f->face);
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
   
   /* set up the flame object */
   o = evas_object_image_add (ff->evas);
   evas_output_viewport_get(ff->evas, NULL, NULL, &ww, &hh);
   ff->ww = ww;
   printf ("Size : %d %d\n", ww, hh);
   evas_object_move (o, 0, hh - ff->flame->conf->height + 1);
   evas_object_resize (o, ff->ww, ff->flame->conf->height);
   evas_object_image_fill_set (o, 0, 0, ff->ww, ff->flame->conf->height);
   evas_object_layer_set (o, -1);
   evas_object_focus_set (o, 1);
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

/* set the default flame palette */
static void
_flame_palette_default_set (Flame_Face *ff)
{
   int i, r, g, b;
   
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
	
	if ((r*r + g*g + b*b) <= 100)
	  ff->palette[i] = ((r*r + g*g + b*b)                |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
	else
	  ff->palette[i] = ((255 << 24)                |
			    (((unsigned char)r) << 16) |
			    (((unsigned char)g) << 8)  |
			    ((unsigned char)b));
     }
}

/* set the plasma flame palette */
static void
_flame_palette_plasma_set (Flame_Face *ff)
{
   int i, r, g, b;
   
   for (i = 0 ; i < 80 ; i++)
     {
	r = 0;
	g = 0;
	b = (i*255) / 80;
	
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
	if (val < 0)    *ptr = 0;
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
