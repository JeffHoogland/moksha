/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include <ctype.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <Eina.h>

/* local subsystem functions */
static int auth_action_ok(char *a, uid_t uid, gid_t gid, gid_t *gl, int gn, gid_t egid);
static int auth_etc_enlightenment_sysactions(char *a, char *u, char **g);
static char *get_word(char *s, char *d);

/* local subsystem globals */
static Eina_Hash *actions;

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i, gn;
   int test = 0;
   char *action, *cmd;
   uid_t uid;
   gid_t gid, gl[1024], egid;

   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-h")) ||
	    (!strcmp(argv[i], "-help")) ||
	    (!strcmp(argv[i], "--help")))
	  {
	     printf(
		    "This is an internal tool for Enlightenment.\n"
		    "do not use it.\n"
		    );
	     exit(0);
	  }
     }
   if (argc == 3)
     {
	if (!strcmp(argv[1], "-t")) test = 1;
	action = argv[2];
     }
   else if (argc == 2)
     {
	action = argv[1];
     }
   else
     {
	exit(-1);
     }

   uid = getuid();
   gid = getgid();
   egid = getegid();
   gn = getgroups(1024, gl);
   
   if (setuid(0) != 0)
     {
	printf("ERROR: UNABLE TO ASSUME ROOT PRIVILEDGES\n");
	exit(5);
     }
   if (setgid(0) != 0)
     {
	printf("ERROR: UNABLE TO ASSUME ROOT GROUP PRIVILEDGES\n");
	exit(7);
     }
   
   eina_init();
   actions = eina_hash_string_superfast_new(NULL);

   if (!auth_action_ok(action, uid, gid, gl, gn, egid))
     {
	printf("ERROR: ACTION NOT ALLOWED: %s\n", action);
	exit(10);
     }
   /* we can add more levels of auth here */
   
   cmd = eina_hash_find(actions, action);
   if (!cmd)
     {
	printf("ERROR: UNDEFINED ACTION: %s\n", action);
	exit(20);
     }
   if (!test) return system(cmd);

   eina_hash_free(actions);
   eina_shutdown();
   
   return 0;
}

/* local subsystem functions */
static int
auth_action_ok(char *a, uid_t uid, gid_t gid, gid_t *gl, int gn, gid_t egid)
{
   struct passwd *pw;
   struct group *gp;
   char *usr = NULL, **grp, *g;
   int ret, i, j;

   pw = getpwuid(uid);
   if (!pw) return 0;
   usr = pw->pw_name;
   if (!usr) return 0;
   grp = alloca(sizeof(char *) * (gn + 1 + 1));
   j = 0;
   gp = getgrgid(gid);
   if (gp)
     {
	grp[j] = gp->gr_name;
	j++;
     }
   for (i = 0; i < gn; i++)
     {
	if (gl[i] != egid)
	  {
	     gp = getgrgid(gl[i]);
	     if (gp)
	       {
		  g = alloca(strlen(gp->gr_name) + 1);
		  strcpy(g, gp->gr_name);
		  grp[j] = g;
		  j++;
	       }
	  }
     }
   grp[j] = NULL;
   /* first stage - check:
    * PREFIX/etc/enlightenment/sysactions.conf
    */
   ret = auth_etc_enlightenment_sysactions(a, usr, grp);
   if (ret == 1) return 1;
   else if (ret == -1) return 0;
   /* the DEFAULT - allow */
   return 1;
}

static int
auth_etc_enlightenment_sysactions(char *a, char *u, char **g)
{
   FILE *f;
   char file[4096], buf[4096], id[4096], ugname[4096], perm[4096], act[4096];
   char *p, *pp, *s, **gp;
   int len, line = 0, ok = 0;
   int allow = 0;
   int deny = 0;
   
   snprintf(file, sizeof(file), "/etc/enlightenment/sysactions.conf");
   f = fopen(file, "r");
   if (!f)
     {
	snprintf(file, sizeof(file), PACKAGE_SYSCONF_DIR"/enlightenment/sysactions.conf");
	f = fopen(file, "r");
	if (!f) return 0;
     }
   while (fgets(buf, sizeof(buf), f))
     {
	line++;
	len = strlen(buf);
	if (len < 1) continue;
	if (buf[len - 1] == '\n') buf[len - 1] = 0;
	/* format:
	 * 
	 * # comment
	 * user:  username  [allow:|deny:] halt reboot ...
	 * group: groupname [allow:|deny:] suspend ...
	 */
	if (buf[0] == '#') continue;
	p = buf;
	p = get_word(p, id);
	p = get_word(p, ugname);
	pp = p;
	p = get_word(p, perm);
	allow = 0;
	deny = 0;
	if (!strcmp(id, "user:"))
	  {
	     if (!fnmatch(ugname, u, 0))
	       {
		  if (!strcmp(perm, "allow:")) allow = 1;
		  else if (!strcmp(perm, "deny:")) deny = 1;
		  else
		    goto malformed;
	       }
	     else
	       continue;
	  }
	else if (!strcmp(id, "group:"))
	  {
	     int matched;
	     
	     matched = 0;
	     for (gp = g; *gp; gp++)
	       {
		  if (!fnmatch(ugname, *gp, 0))
		    {
		       matched = 1;
		       if (!strcmp(perm, "allow:")) allow = 1;
		       else if (!strcmp(perm, "deny:")) deny = 1;
		       else
			 goto malformed;
		    }
	       }
	     if (!matched) continue;
	  }
	else if (!strcmp(id, "action:"))
	  {
	     while ((*pp) && (isspace(*pp))) pp++;
	     s = eina_hash_find(actions, ugname);
	     if (s)
	       {
		  eina_hash_del(actions, ugname, s);
		  free(s);
	       }
	     eina_hash_add(actions, ugname, strdup(pp));
	     continue;
	  }
	else if (id[0] == 0)
	  continue;
	else
	  goto malformed;
	
	for (;;)
	  {
	     p = get_word(p, act);
	     if (act[0] == 0) break;
	     if (!fnmatch(act, a, 0))
	       {
		  if (allow) ok = 1;
		  else if (deny) ok = -1;
		  goto done;
	       }
	  }
	
	continue;
	malformed:
	printf("WARNING: %s:%i\n"
	       "LINE: '%s'\n"
	       "MALFORMED LINE. SKIPPED.\n",
	       file, line, buf);
     }
   done:
   fclose(f);
   return ok;
}

static char *
get_word(char *s, char *d)
{
   char *p1, *p2;
   
   p1 = s;
   p2 = d;
   while (*p1)
     {
	if (p2 == d)
	  {
	     if (isspace(*p1))
	       {
		  p1++;
		  continue;
	       }
	  }
	if (isspace(*p1)) break;
	*p2 = *p1;
	p1++;
	p2++;
     }
   *p2 = 0;
   return p1;
}

