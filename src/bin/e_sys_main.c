#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include <ctype.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <Eina.h>

/* local subsystem functions */
static int auth_action_ok(char *a,
                          uid_t uid,
                          gid_t gid,
                          gid_t *gl,
                          int gn,
                          gid_t egid);
static int auth_etc_enlightenment_sysactions(char *a,
                                             char *u,
                                             char **g);
static char *get_word(char *s,
                      char *d);

/* local subsystem globals */
static Eina_Hash *actions = NULL;

/* externally accessible functions */
int
main(int argc,
     char **argv)
{
   int i, gn;
   int test = 0;
   Eina_Bool mnt = EINA_FALSE;
   char *action, *cmd;
   uid_t uid;
   gid_t gid, gl[65536], egid;

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
   if (argc >= 3)
     {
        if ((argc == 3) && (!strcmp(argv[1], "-t")))
          {
             test = 1;
             action = argv[2];
          }
        else
          {
             const char *s;

             s = strrchr(argv[1], '/');
             if ((!s) || (!(++s))) exit(1); /* eeze always uses complete path */
             if (strcmp(s, "mount") && strcmp(s, "umount") && strcmp(s, "eject")) exit(1);
             mnt = EINA_TRUE;
             action = argv[1];
          }
     }
   else if (argc == 2)
     {
        action = argv[1];
     }
   else
     {
        exit(1);
     }

   uid = getuid();
   gid = getgid();
   egid = getegid();
   gn = getgroups(65536, gl);
   if (gn < 0)
     {
        printf("ERROR: MEMBER OF MORE THAN 65536 GROUPS\n");
        exit(3);
     }
   if (setuid(0) != 0)
     {
        printf("ERROR: UNABLE TO ASSUME ROOT PRIVILEGES\n");
        exit(5);
     }
   if (setgid(0) != 0)
     {
        printf("ERROR: UNABLE TO ASSUME ROOT GROUP PRIVILEGES\n");
        exit(7);
     }

   eina_init();

   if (!auth_action_ok(action, uid, gid, gl, gn, egid))
     {
        printf("ERROR: ACTION NOT ALLOWED: %s\n", action);
        exit(10);
     }
   /* we can add more levels of auth here */

   /* when mounting, this will match the exact path to the exe,
    * as required in sysactions.conf
    * this is intentionally pedantic for security
    */
   cmd = eina_hash_find(actions, action);
   if (!cmd)
     {
        printf("ERROR: UNDEFINED ACTION: %s\n", action);
        exit(20);
     }
   if ((!test) && (!mnt)) return system(cmd);
   if (mnt)
     {
        Eina_Strbuf *buf;
        int ret = 0;
        const char *mp = NULL;

        buf = eina_strbuf_new();
        if (!buf) goto err;
        for (i = 1; i < argc; i++)
          {
             if (!strncmp(argv[i], "/media/", 7))
               {
                  mp = argv[i];
                  if (!strcmp(action, "mount"))
                    {
                       struct stat s;

                       if (stat("/media", &s))
                         {
                            mode_t um;

                            um = umask(0);
                            if (mkdir("/media", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
                              {
                                 printf("ERROR: COULD NOT CREATE DIRECTORY /media\n");
                                 exit(40);
                              }
                            umask(um);
                         }
                       else if (!S_ISDIR(s.st_mode))
                         {
                            printf("ERROR: NOT A DIRECTORY: /media\n");
                            exit(40);
                         }

                       if (stat(argv[i], &s))
                         {
                            mode_t um;

                            um = umask(0);
                            if (mkdir(argv[i], S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
                              {
                                 printf("ERROR: COULD NOT CREATE DIRECTORY %s\n", argv[i]);
                                 exit(40);
                              }
                            umask(um);
                         }
                       else if (!S_ISDIR(s.st_mode))
                         {
                            printf("ERROR: NOT A DIRECTORY: %s\n", argv[i]);
                            exit(40);
                         }
                    }
               }
             eina_strbuf_append_printf(buf, "%s ", argv[i]);
          }
        ret = system(eina_strbuf_string_get(buf));
        if (mp && (!strcmp(action, "umount")) && (!ret))
          {
               if (rmdir(mp))
                 printf("ERROR: COULD NOT UNLINK MOUNT POINT %s\n", mp);
          }
        return ret;
     }

   eina_shutdown();

   return 0;

err:
   printf("ERROR: MEMORY CRISIS\n");
   eina_shutdown();
   return 30;
}

/* local subsystem functions */
static int
auth_action_ok(char *a,
               uid_t uid,
               gid_t gid,
               gid_t *gl,
               int gn,
               gid_t egid)
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
   else if (ret == -1)
     return 0;
   /* the DEFAULT - allow */
   return 1;
}

static int
auth_etc_enlightenment_sysactions(char *a,
                                  char *u,
                                  char **g)
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
        snprintf(file, sizeof(file), PACKAGE_SYSCONF_DIR "/enlightenment/sysactions.conf");
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
                  else if (!strcmp(perm, "deny:"))
                    deny = 1;
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
                       else if (!strcmp(perm, "deny:"))
                         deny = 1;
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
             if (s) eina_hash_del(actions, ugname, s);
             if (!actions) actions = eina_hash_string_superfast_new(free);
             eina_hash_add(actions, ugname, strdup(pp));
             continue;
          }
        else if (id[0] == 0)
          continue;
        else
          goto malformed;

        for (;; )
          {
             p = get_word(p, act);
             if (act[0] == 0) break;
             if (!fnmatch(act, a, 0))
               {
                  if (allow) ok = 1;
                  else if (deny)
                    ok = -1;
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
get_word(char *s,
         char *d)
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

