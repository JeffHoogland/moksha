/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_gadman_free(E_Gadman *gm);
static void _e_gadman_client_free(E_Gadman_Client *gmc);

/* externally accessible functions */
int
e_gadman_init(void)
{
   return 1;
}

int
e_gadman_shutdown(void)
{
   return 1;
}

E_Gadman *
e_gadman_new(E_Container *con)
{
   E_Gadman    *gm;

   gm = E_OBJECT_ALLOC(E_Gadman, _e_gadman_free);
   if (!gm) return NULL;
   gm->container = con;
   return gm;
}

E_Gadman_Client *
e_gadman_client_new(E_Gadman *gm)
{
   E_Gadman_Client *gmc;
   E_OBJECT_CHECK_RETURN(gm, NULL);
   
   gmc = E_OBJECT_ALLOC(E_Gadman_Client, _e_gadman_client_free);
   if (!gmc) return NULL;
   gmc->gadman = gm;
   gmc->policy = E_GADMAN_POLICY_ANYWHERE;
   gmc->minw = 1;
   gmc->minh = 1;
   gmc->maxw = 0;
   gmc->maxh = 0;
   gmc->ax = 0.0;
   gmc->ay = 1.0;
   gmc->mina = 0.0;
   gmc->maxa = 9999999.0;
   gm->clients = evas_list_append(gm->clients, gmc);
   return gmc;
}

void
e_gadman_client_domain_set(E_Gadman_Client *gmc, char *domain, int instance)
{
   E_OBJECT_CHECK(gmc);
   if (gmc->domain) free(gmc->domain);
   gmc->domain = strdup(domain);
   gmc->instance = instance;
}

void
e_gadman_client_save(E_Gadman_Client *gmc, char *domain, int instance)
{
   E_OBJECT_CHECK(gmc);
}

void
e_gadman_client_load(E_Gadman_Client *gmc, char *domain, int instance)
{
   E_OBJECT_CHECK(gmc);
}

void
e_gadman_client_policy_set(E_Gadman_Client *gmc, E_Gadman_Policy pol)
{
   E_OBJECT_CHECK(gmc);
   gmc->policy = pol;
}

void
e_gadman_client_min_size_set(E_Gadman_Client *gmc, Evas_Coord minw, Evas_Coord minh)
{
   E_OBJECT_CHECK(gmc);
   gmc->minw = minw;
   gmc->minh = minh;
}

void
e_gadman_client_max_size_set(E_Gadman_Client *gmc, Evas_Coord maxw, Evas_Coord maxh)
{
   E_OBJECT_CHECK(gmc);
   gmc->maxw = maxw;
   gmc->maxh = maxh;
}

void
e_gadman_client_align_set(E_Gadman_Client *gmc, double xalign, double yalign)
{
   E_OBJECT_CHECK(gmc);
   gmc->ax = xalign;
   gmc->ay = yalign;
}

void
e_gadman_client_aspect_set(E_Gadman_Client *gmc, double mina, double maxa)
{
   E_OBJECT_CHECK(gmc);
   gmc->mina = mina;
   gmc->maxa = maxa;
}

void
e_gadman_client_geometry_get(E_Gadman_Client *gmc, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_OBJECT_CHECK(gmc);
   if (x) *x = gmc->x;
   if (y) *y = gmc->y;
   if (w) *w = gmc->w;
   if (h) *h = gmc->h;
}

void
e_gadman_client_change_func_set(E_Gadman_Client *gmc, void (*func) (void *data, E_Gadman_Client *gmc, E_Gadman_Change change), void *data)
{
   E_OBJECT_CHECK(gmc);
   gmc->func = func;
   gmc->data = data;
}

/* local subsystem functions */
static void
_e_gadman_free(E_Gadman *gm)
{
   free(gm);
}

static void
_e_gadman_client_free(E_Gadman_Client *gmc)
{
   gmc->gadman->clients = evas_list_remove(gmc->gadman->clients, gmc);
   if (gmc->domain) free(gmc->domain);
   free(gmc);
}
