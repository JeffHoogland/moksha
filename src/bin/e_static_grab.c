#include <Eina.h>
#include <Eet.h>

typedef struct _E_Static_Grab E_Static_Grab;
typedef struct _E_Static_Grab_Module E_Static_Grab_Module;

struct _E_Static_Grab
{
   struct
   {
      int core_count;
      int thread_count;
      int bogo;
      int model;
      int family;
      int stepping;
      int cache_size;
      int addr_space;

      const char *vendor;
      const char *model_name;

      Eina_Bool fpu;
   } cpu;

   struct
   {
      const char *name;
      const char *release;
      const char *os;
      const char *kernel_release;
   } distribution;

   struct {
      struct
      {
         const char *vendor;
         const char *renderer;
         const char *version;
      } gpu;

      const char *name;
      const char *release_date;
      const char *build_date;

      Eina_List *modules;
   } x;
};

struct _E_Static_Grab_Module
{
   const char *name;
   const char *vendor;
   const char *compiled_for;
   const char *version;
   const char *class;
   const char *ABI_class;
   const char *ABI_version;
};

/* For /proc/cpuinfo */
#define CPU_CORES  "cpu cores"
#define CACHE_SIZE "cache size"
#define CPU_FAMILY "cpu family"
#define STEPPING "stepping"
#define SIBLINGS "siblings"
#define BOGOMIPS "bogomips"
#define MODEL "model"
#define MODEL_NAME "model name"
#define VENDOR_ID "vendor_id"
#define FPU "fpu"

/* For /var/log/Xorg.0.log */
#define RELEASE_DATE "Release Date"
#define BUILD_DATE "Build Date"
#define LOAD_MODULE "(II) LoadModule"
#define UNLOAD_MODULE "(II) UnloadModule"
#define MODULE_OF "(II) Module %s"
#define LOADING "(II) Loading"
#define VENDOR "vendor=\""
#define COMPILED "compiled for "
#define MODULE_CLASS "Module class"
#define ABI_CLASS "ABI class"
#define VERSION ", version "

static const char *
_e_static_grab_name(const char *line, const char *end, const char *name)
{
   size_t l;

   l = strlen(name);
   if (line + l + 3 > end) return end;
   if (strncmp(name, line, l)) return end;

   return line + l;
}

static Eina_Bool
_e_static_grab_int(const char *line, const char *end, const char *txt, int *v)
{
   static const char *value = "0123456789";
   const char *over;
   int r = 0;

   over = _e_static_grab_name(line, end, txt);
   while (over < end)
     {
        if (strchr(value, *over)) break;
        over++;
     }
   if (over == end) return EINA_FALSE;

   while (over < end)
     {
        const char *offset = strchr(value, *over);

        if (!offset) break;
        r = r * 10 + (offset - value);

        over++;
     }

   *v = r;

   return EINA_TRUE;
}

static Eina_Bool
_e_static_grab_string(const char *line, const char *end, const char *txt, const char **s)
{
   static const char *value = " :\t";
   const char *over;

   over = _e_static_grab_name(line, end, txt);
   while (over < end)
     {
        if (!strchr(value, *over)) break;
        over++;
     }
   if (over == end) return EINA_FALSE;

   if (*over == '"' && *(end - 2) == '"')
     {
        over++;
        end--;
     }

   *s = eina_stringshare_add_length(over, end - over - 1);
   return EINA_TRUE;
}

static const char *
_e_static_grab_discard(const char *current, Eina_File_Line *line, int begin, int end)
{
   while (current < line->end &&
          strchr(" \t", *current))
     current++;

   if (*current != begin) return current;

   while (current < line->end &&
          *current != end)
     current++;

   if (current == line->end) return current;

   for (current++;
        current < line->end && strchr(" \t", *current);
        current++)
     ;

   return current;
}

static void
_e_static_grab_cpu(E_Static_Grab *grab)
{
   char buf[4096];
   FILE *f;
   const char *fpu = NULL;
   int siblings = 0;

   grab->cpu.addr_space = sizeof (void*);

   f = fopen("/proc/cpuinfo", "r");
   if (!f) return ;

   while (!feof(f))
     {
        char *end;
        size_t length = 0;

        fgets(buf, sizeof (buf), f);
        length = strlen(buf);
        end = buf + length;

        if (length < 3) continue ;

        switch (*buf)
          {
           case 'c':
              if (_e_static_grab_int(buf, end, CPU_CORES, &grab->cpu.core_count)) break;
              if (_e_static_grab_int(buf, end, CACHE_SIZE, &grab->cpu.cache_size)) break;
              _e_static_grab_int(buf, end, CPU_FAMILY, &grab->cpu.family);
              break;
           case 's':
              if (_e_static_grab_int(buf, end, STEPPING, &grab->cpu.stepping)) break;
              _e_static_grab_int(buf, end, SIBLINGS, &siblings);
              break;
           case 'b':
              _e_static_grab_int(buf, end, BOGOMIPS, &grab->cpu.bogo);
              break;
           case 'm':
              if (_e_static_grab_string(buf, end, MODEL_NAME, &grab->cpu.model_name)) break;
              _e_static_grab_int(buf, end, MODEL, &grab->cpu.model);
              break;
           case 'v':
              _e_static_grab_string(buf, end, VENDOR_ID, &grab->cpu.vendor);
              break;
           case 'f':
              _e_static_grab_string(buf, end, FPU, &fpu);
              break;
          }
     }
   fclose(f);

   if (siblings) grab->cpu.thread_count = siblings / grab->cpu.core_count;
   if (fpu)
     {
        if (!strcmp(fpu, "yes")) grab->cpu.fpu = EINA_TRUE;
        eina_stringshare_del(fpu);
     }
}

static void
_e_static_grab_x(E_Static_Grab *grab)
{
   Eina_Bool in_module = EINA_FALSE;
   E_Static_Grab_Module *module;
   Eina_File_Line *line;
   Eina_Iterator *it;
   Eina_File *f;

   module = calloc(1, sizeof (E_Static_Grab_Module));
   if (!module) return ;

   f = eina_file_open("/var/log/Xorg.0.log", EINA_FALSE);
   if (!f) return ;

   it = eina_file_map_lines(f);
   EINA_ITERATOR_FOREACH(it, line)
     {
        const char *current;

        current = _e_static_grab_discard(line->start, line, '[', ']');
        if (current >= line->end) continue ;

        if (!grab->x.name)
          {
             if (line->end - current - 1 <= 0) continue ;
             grab->x.name = eina_stringshare_add_length(current, line->end - current - 1);
             continue ;
          }

        if (!in_module)
          {
             switch (*current)
               {
                case 'R':
                   if (_e_static_grab_string(current, line->end, RELEASE_DATE, &grab->x.release_date)) break;
                   break;

                case 'B':
                   if (_e_static_grab_string(current, line->end, BUILD_DATE, &grab->x.build_date)) break;
                   break;

                case '(':
                  {
                     E_Static_Grab_Module *md;
                     Eina_List *l;
                     const char *tmp;

                     if (_e_static_grab_string(current, line->end, LOAD_MODULE, &module->name))
                       {
                          EINA_LIST_FOREACH(grab->x.modules, l, md)
                            if (md->name == module->name)
                              {
                                 eina_stringshare_del(module->name);
                                 module->name = NULL;
                                 break;
                              }

                          if (module->name) in_module = EINA_TRUE;
                          break;
                       }
                     else if (_e_static_grab_string(current, line->end, UNLOAD_MODULE, &tmp))
                       {
                          EINA_LIST_FOREACH(grab->x.modules, l, md)
                            if (md->name == tmp)
                              {
                                 grab->x.modules = eina_list_remove_list(grab->x.modules, l);

                                 eina_stringshare_del(md->name);
                                 eina_stringshare_del(md->vendor);
                                 eina_stringshare_del(md->compiled_for);
                                 eina_stringshare_del(md->version);
                                 eina_stringshare_del(md->class);
                                 eina_stringshare_del(md->ABI_class);
                                 eina_stringshare_del(md->ABI_version);
                                 free(md);
                                 break;
                              }
                          eina_stringshare_del(tmp);
                       }
                     break;
                  }
               }
          }
        else
          {
             char *buffer;
             Eina_Bool found = EINA_FALSE;

             switch (*current)
               {
                case '(':
                  {
                     const char *vendor;

                     if (strncmp(current, LOADING, strlen(LOADING)) == 0)
                       {
                          found = EINA_TRUE;
                          break;
                       }

                     buffer = alloca(strlen(MODULE_OF) + strlen(module->name));
                     sprintf(buffer, MODULE_OF, module->name);

                     if (_e_static_grab_string(current, line->end, buffer, &vendor))
                       {
                          if (strncmp(vendor, VENDOR, strlen(VENDOR)) == 0)
                            {
                               module->vendor = eina_stringshare_add_length(vendor + strlen(VENDOR),
                                                                            eina_stringshare_strlen(vendor) - strlen(VENDOR) - 1);
                               found = EINA_TRUE;
                            }
                          eina_stringshare_del(vendor);
                       }
                     break;
                  }
                case 'c':
                   if (strncmp(current, COMPILED, strlen(COMPILED)) == 0)
                     {
                        const char *lookup = current + strlen(COMPILED);

                        while (lookup < line->end && *lookup != ',')
                          lookup++;

                        module->compiled_for = eina_stringshare_add_length(current + strlen(COMPILED),
                                                                           lookup - current - strlen(COMPILED));
                        found = EINA_TRUE;
                        break;
                     }
                   break;
                case 'M':
                   if (_e_static_grab_string(current, line->end, MODULE_CLASS, &module->class))
                     {
                        found = EINA_TRUE;
                        break;
                     }
                   break;
                case 'A':
                  {
                     const char *tmp;

                     if (_e_static_grab_string(current, line->end, ABI_CLASS, &tmp))
                       {
                          const char *lookup = strchr(tmp, ',');

                          if (lookup)
                            {
                               module->ABI_class = eina_stringshare_add_length(tmp, lookup - tmp);
                               if (strncmp(lookup, VERSION, strlen(VERSION)) == 0)
                                 {
                                    module->ABI_version = eina_stringshare_add_length(lookup + strlen(VERSION),
                                                                                      eina_stringshare_strlen(tmp) - (lookup - tmp + strlen(VERSION) - 1));
                                 }
                               eina_stringshare_del(tmp);
                            }
                          else
                            {
                               module->ABI_class = tmp;
                            }
                       }
                     break;
                  }
               }

             if (!found)
               {
                  in_module = EINA_FALSE;
                  grab->x.modules = eina_list_append(grab->x.modules, module);
                  module = calloc(1, sizeof (E_Static_Grab_Module));
               }
          }
     }

   free(module);

   eina_iterator_free(it);
   eina_file_close(f);
}

int
main(int argc, char **argv EINA_UNUSED)
{
   E_Static_Grab_Module *module;
   Eina_List *l;
   E_Static_Grab grab;

   if (argc != 2) exit(0);

#if 0
   /* FIXME: Latter push information into an eet file when updated */
   if ((!strcmp(argv[1], "-h")) ||
       (!strcmp(argv[1], "-help")) ||
       (!strcmp(argv[1], "--help")))
     {
        printf("This is an internal tool for Enlightenment.\n"
               "do not use it.\n");
        exit(0);
     }
#endif

   eina_init();
   eet_init();

   memset(&grab, 0, sizeof (grab));
   _e_static_grab_cpu(&grab); /* FIXME: please provide patch for more Unix */
   _e_static_grab_x(&grab);

   fprintf(stderr, "%i core with %i thread running in %i bits at %i bogomips with %i cache from vendor %s with model %s\n",
	   grab.cpu.core_count, grab.cpu.thread_count, grab.cpu.addr_space * 8, grab.cpu.bogo, grab.cpu.cache_size, grab.cpu.vendor, grab.cpu.model_name);
   fprintf(stderr, "X Server: '%s' released '%s' and build '%s'\n",
           grab.x.name, grab.x.release_date, grab.x.build_date);
   fprintf(stderr, "*** X Modules ***\n");
   EINA_LIST_FOREACH(grab.x.modules, l, module)
     fprintf(stderr, "[%s - '%s' - '%s' - '%s' - '%s','%s' ]\n",
             module->name, module->vendor, module->compiled_for,
             module->class, module->ABI_class, module->ABI_version);
   fprintf(stderr, "*** ***\n");

   /* FIXME: Get information about what Unix (Linux, BSD, ...) it is run on */
   /* FIXME: run a mainloop and start gathering result from df, free, lspci and lsusb */
   /* FIXME: use xrandr to figure out the screen config */

   eet_shutdown();
   eina_shutdown();

   return 0;
}
