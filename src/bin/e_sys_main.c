#include "config.h"

#define __USE_MISC
#define _SVID_SOURCE
#ifdef HAVE_FEATURES_H
# include <features.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_ENVIRON
# define _GNU_SOURCE 1
#endif
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include <ctype.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <Eina.h>

#ifdef HAVE_ENVIRON
extern char **environ;
#endif

/* local subsystem functions */
#ifdef HAVE_EEZE_MOUNT
static Eina_Bool mountopts_check(const char *opts);
static Eina_Bool mount_args_check(int argc, char **argv, const char *action);
#endif
static int auth_action_ok(char *a,
                          gid_t gid,
                          gid_t *gl,
                          int gn,
                          gid_t egid);
static int auth_etc_enlightenment_sysactions(char *a,
                                             char *u,
                                             char **g);
static void auth_etc_enlightenment_sysactions_perm(char *path);
static char *get_word(char *s,
                      char *d);

/* local subsystem globals */
static Eina_Hash *actions = NULL;
static uid_t uid = -1;

/* externally accessible functions */
int
main(int argc,
     char **argv)
{
   int i, gn;
   int test = 0;
   char *action = NULL, *cmd;
   char *output = NULL;
#ifdef HAVE_EEZE_MOUNT
   Eina_Bool mnt = EINA_FALSE;
   const char *act;
#endif
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
#ifdef HAVE_EEZE_MOUNT
        else
          {
             const char *s;

             s = strrchr(argv[1], '/');
             if ((!s) || (!s[1])) exit(1); /* eeze always uses complete path */
             s++;
             if (strcmp(s, "mount") && strcmp(s, "umount") && strcmp(s, "eject")) exit(1);
             mnt = EINA_TRUE;
             act = s;
             action = argv[1];
          }
#endif
     }
   else if (argc == 2)
     {
        action = argv[1];
     }
   else
     {
        exit(1);
     }
   if (!action) exit(1);
   fprintf(stderr, "action %s %i\n", action, argc);

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

   if (!auth_action_ok(action, gid, gl, gn, egid))
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

   /* sanitize environment */
#ifdef HAVE_UNSETENV
# define NOENV(x) unsetenv(x)
   /* pass 1 - just nuke known dangerous env vars brutally if possible via
    * unsetenv(). if you don't have unsetenv... there's pass 2 and 3 */
   NOENV("IFS");
   NOENV("CDPATH");
   NOENV("LOCALDOMAIN");
   NOENV("RES_OPTIONS");
   NOENV("HOSTALIASES");
   NOENV("NLSPATH");
   NOENV("PATH_LOCALE");
   NOENV("COLORTERM");
   NOENV("LANG");
   NOENV("LANGUAGE");
   NOENV("LINGUAS");
   NOENV("TERM");
   NOENV("LD_PRELOAD");
   NOENV("LD_LIBRARY_PATH");
   NOENV("SHLIB_PATH");
   NOENV("LIBPATH");
   NOENV("AUTHSTATE");
   NOENV("DYLD_*");
   NOENV("KRB_CONF*");
   NOENV("KRBCONFDIR");
   NOENV("KRBTKFILE");
   NOENV("KRB5_CONFIG*");
   NOENV("KRB5_KTNAME");
   NOENV("VAR_ACE");
   NOENV("USR_ACE");
   NOENV("DLC_ACE");
   NOENV("TERMINFO");
   NOENV("TERMINFO_DIRS");
   NOENV("TERMPATH");
   NOENV("TERMCAP");
   NOENV("ENV");
   NOENV("BASH_ENV");
   NOENV("PS4");
   NOENV("GLOBIGNORE");
   NOENV("SHELLOPTS");
   NOENV("JAVA_TOOL_OPTIONS");
   NOENV("PERLIO_DEBUG");
   NOENV("PERLLIB");
   NOENV("PERL5LIB");
   NOENV("PERL5OPT");
   NOENV("PERL5DB");
   NOENV("FPATH");
   NOENV("NULLCMD");
   NOENV("READNULLCMD");
   NOENV("ZDOTDIR");
   NOENV("TMPPREFIX");
   NOENV("PYTHONPATH");
   NOENV("PYTHONHOME");
   NOENV("PYTHONINSPECT");
   NOENV("RUBYLIB");
   NOENV("RUBYOPT");
# ifdef HAVE_ENVIRON
   if (environ)
     {
        int again;
        char *tmp, *p;

        /* go over environment array again and again... safely */
        do
          {
             again = 0;

             /* walk through and find first entry that we don't like */
             for (i = 0; environ[i]; i++)
               {
                  /* if it begins with any of these, it's possibly nasty */
                  if ((!strncmp(environ[i], "LD_", 3)) ||
                      (!strncmp(environ[i], "_RLD_", 5)) ||
                      (!strncmp(environ[i], "LC_", 3)) ||
                      (!strncmp(environ[i], "LDR_", 3)))
                    {
                       /* unset it */
                       tmp = strdup(environ[i]);
                       if (!tmp) abort();
                       p = strchr(tmp, '=');
                       if (!p) abort();
                       *p = 0;
                       NOENV(tmp);
                       free(tmp);
                       /* and mark our do to try again from the start in case
                        * unsetenv changes environ ptr */
                       again = 1;
                       break;
                    }
               }
          }
        while (again);
     }
# endif
#endif

   /* pass 2 - clear entire environment so it doesn't exist at all. if you
    * can't do this... you're possibly in trouble... but the worst is still
    * fixed in pass 3 */
#ifdef HAVE_CLEARENV
   clearenv();
#else
# ifdef HAVE_ENVIRON
   environ = NULL;
# endif
#endif

   /* pass 3 - set path and ifs to minimal defaults */
   putenv("PATH=/bin:/usr/bin");
   putenv("IFS= \t\n");

   if ((!test)
#ifdef HAVE_EEZE_MOUNT
    && (!mnt)
#endif
       )
     return system(cmd);
#ifdef HAVE_EEZE_MOUNT
   if (mnt)
     {
        int ret = 0;
        const char *mp = NULL;
        Eina_Strbuf *buf = NULL;

        if (!mount_args_check(argc, argv, act)) exit(40);
        /* all options are deemed safe at this point, so away we go! */
        if (!strcmp(act, "mount"))
          {
             struct stat s;

             mp = argv[5];
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

             if (stat(mp, &s))
               {
                  mode_t um;

                  um = umask(0);
                  if (mkdir(mp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
                    {
                       printf("ERROR: COULD NOT CREATE DIRECTORY %s\n", mp);
                       exit(40);
                    }
                  umask(um);
               }
             else if (!S_ISDIR(s.st_mode))
               {
                  printf("ERROR: NOT A DIRECTORY: %s\n", mp);
                  exit(40);
               }
          }
        buf = eina_strbuf_new();
        if (!buf) exit(30);
        for (i = 1; i < argc; i++)
          eina_strbuf_append_printf(buf, "%s ", argv[i]);
        ret = system(eina_strbuf_string_get(buf));
        if ((!strcmp(act, "umount")) && (!ret))
          {
             Eina_Iterator *it;
             char path[PATH_MAX];
             const char *s;
             struct stat st;
             Eina_Bool rm = EINA_TRUE;

             mp = strrchr(argv[2], '/');
             if (!mp) return ret;
             snprintf(path, sizeof(path), "/media%s", mp);
             if (stat(path, &st)) return ret;
             if (!S_ISDIR(st.st_mode)) return ret;
             it = eina_file_ls(path);
             EINA_ITERATOR_FOREACH(it, s)
               {
                  /* don't delete any directories with files in them */
                  rm = EINA_FALSE;
                  eina_stringshare_del(s);
               }
             if (rm)
               {
                  if (rmdir(path))
                    printf("ERROR: COULD NOT UNLINK MOUNT POINT %s\n", path);
               }
          }
        return ret;
     }
#endif
   eina_shutdown();

   return 0;
}

/* local subsystem functions */
#ifdef HAVE_EEZE_MOUNT
static Eina_Bool
mountopts_check(const char *opts)
{
   char buf[64];
   const char *p;
   char *end;
   unsigned long muid;
   Eina_Bool nosuid, nodev, noexec, nuid;

   nosuid = nodev = noexec = nuid = EINA_FALSE;

   /* these are the only possible options which can be present here; check them strictly */
   if (eina_strlcpy(buf, opts, sizeof(buf)) >= sizeof(buf)) return EINA_FALSE;
   for (p = buf; p && p[1]; p = strchr(p + 1, ','))
     {
        if (p[0] == ',') p++;
#define CMP(OPT) \
  if (!strncmp(p, OPT, sizeof(OPT) - 1))

        CMP("nosuid,")
          {
             nosuid = EINA_TRUE;
             continue;
          }
        CMP("nodev,")
          {
             nodev = EINA_TRUE;
             continue;
          }
        CMP("noexec,")
          {
             noexec = EINA_TRUE;
             continue;
          }
        CMP("utf8,") continue;
        CMP("utf8=0,") continue;
        CMP("utf8=1,") continue;
        CMP("iocharset=utf8,") continue;
        CMP("uid=")
          {
             p += 4;
             errno = 0;
             muid = strtoul(p, &end, 10);
             if (muid == ULONG_MAX) return EINA_FALSE;
             if (errno) return EINA_FALSE;
             if (end[0] != ',') return EINA_FALSE;
             if (muid != uid) return EINA_FALSE;
             nuid = EINA_TRUE;
             continue;
          }
        return EINA_FALSE;
     }
   if ((!nosuid) || (!nodev) || (!noexec) || (!nuid)) return EINA_FALSE;
   return EINA_TRUE;
}

static Eina_Bool
check_uuid(const char *uuid)
{
   const char *p;

   for (p = uuid; p[0]; p++)
     if ((!isalnum(*p)) && (*p != '-')) return EINA_FALSE;
   return EINA_TRUE;
}

static Eina_Bool
mount_args_check(int argc, char **argv, const char *action)
{
   Eina_Bool opts = EINA_FALSE;
   struct stat st;
   const char *node;
   char buf[PATH_MAX];

   if (!strcmp(action, "mount"))
     {
        /* will ALWAYS be:
         /path/to/mount -o nosuid,uid=XYZ,[utf8,] UUID=XXXX-XXXX[-XXXX-XXXX] /media/$devnode
         */
        if (argc != 6) return EINA_FALSE;
        if (argv[2][0] == '-')
          {
             /* disallow any -options other than -o */
             if (strcmp(argv[2], "-o")) return EINA_FALSE;
             opts = mountopts_check(argv[3]);
          }
        if (!opts) return EINA_FALSE;
        if (!strncmp(argv[4], "UUID=", sizeof("UUID=") - 1))
          {
             if (!check_uuid(argv[4] + 5)) return EINA_FALSE;
          }
        else
          {
             if (strncmp(argv[4], "/dev/", 5)) return EINA_FALSE;
             if (stat(argv[4], &st)) return EINA_FALSE;
          }
        
        node = strrchr(argv[5], '/');
        if (!node) return EINA_FALSE;
        if (!node[1]) return EINA_FALSE;
        if (node - argv[5] != 6) return EINA_FALSE;
        snprintf(buf, sizeof(buf), "/dev%s", node);
        if (stat(buf, &st)) return EINA_FALSE;
     }
   else if (!strcmp(action, "umount"))
     {
        /* will ALWAYS be:
         /path/to/umount /dev/$devnode
         */
        if (argc != 3) return EINA_FALSE;
        if (strncmp(argv[2], "/dev/", 5)) return EINA_FALSE;
        if (stat(argv[2], &st)) return EINA_FALSE;
        node = strrchr(argv[2], '/');
        if (!node) return EINA_FALSE;
        if (!node[1]) return EINA_FALSE;
        if (node - argv[2] != 4) return EINA_FALSE;
        /* this is good, but it prevents umounting user-mounted removable media;
         * need to figure out a better way...
         * 
        snprintf(buf, sizeof(buf), "/media%s", node);
        if (stat(buf, &st)) return EINA_FALSE;
        if (!S_ISDIR(st.st_mode)) return EINA_FALSE;
        */
     }
   else if (!strcmp(action, "eject"))
     {
        /* will ALWAYS be:
         /path/to/eject /dev/$devnode
         */
        if (argc != 3) return EINA_FALSE;
        if (strncmp(argv[2], "/dev/", 5)) return EINA_FALSE;
        if (stat(argv[2], &st)) return EINA_FALSE;
        node = strrchr(argv[2], '/');
        if (!node) return EINA_FALSE;
        if (!node[1]) return EINA_FALSE;
        if (node - argv[2] != 4) return EINA_FALSE;
     }
   else return EINA_FALSE;
   return EINA_TRUE;
}
#endif

static int
auth_action_ok(char *a,
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
   int ok = 0;
   size_t len, line = 0;
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

   auth_etc_enlightenment_sysactions_perm(file);

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
             Eina_Bool matched = EINA_FALSE;

             for (gp = g; *gp; gp++)
               {
                  if (!fnmatch(ugname, *gp, 0))
                    {
                       matched = EINA_TRUE;
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
        printf("WARNING: %s:%zu\n"
               "LINE: '%s'\n"
               "MALFORMED LINE. SKIPPED.\n",
               file, line, buf);
     }
done:
   fclose(f);
   return ok;
}

static void
auth_etc_enlightenment_sysactions_perm(char *path)
{
   struct stat st;
   if (stat(path, &st) == -1)
     return;

   if ((st.st_mode & S_IWGRP) || (st.st_mode & S_IXGRP) ||
       (st.st_mode & S_IWOTH) || (st.st_mode & S_IXOTH))
     {
        printf("ERROR: CONFIGURATION FILE HAS BAD PERMISSIONS (writable by group and/or others)\n");
        exit(10);
     }
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
